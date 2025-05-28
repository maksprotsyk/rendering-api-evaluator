#include "DirectXRenderer.h"

#include <stdexcept>
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "tiny_obj_loader.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"

#include "Utils/DebugMacros.h"

namespace Engine::Visual
{
	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::init(const Window& window)
	{
		createDeviceAndSwapChain(window.getHandle());
		createRenderTarget(window.getHandle());
		createSamplerState();
		createShaders();
		createViewport(window.getHandle());
		createDefaultMaterial();
#ifdef _SHOWUI
		initUI();
#endif
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::clearBackground(float r, float g, float b, float a)
	{
		// Clear the render target with a solid color
		float clearColor[] = {r, g, b, a }; // RGBA
		m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
		m_deviceContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::draw(const IModelInstance& model, const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
	{
		const auto& modelItr = m_models.find(model.GetId());
		ASSERT(modelItr != m_models.end(), "Can't find model with id: {}", model.GetId());
		if (modelItr == m_models.end())
		{
			return;
		}

		ModelData& modelData = modelItr->second;

		XMMATRIX worldMatrix = getWorldMatrix(position, rotation, scale);
		m_constantBufferData.worldMatrix = XMMatrixTranspose(worldMatrix);

		m_deviceContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_constantBufferData, 0, 0);

		m_deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
		m_deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

		for (Material& material : modelData.materials)
		{
			MaterialBuffer mb{};
			mb.ambientColor = material.ambientColor;
			mb.diffuseColor = material.diffuseColor;
			mb.specularColor = material.specularColor;
			mb.shininess = material.shininess;
			mb.useDiffuseTexture = material.useDiffuseTexture;
			m_deviceContext->UpdateSubresource(material.materialBuffer.Get(), 0, nullptr, &mb, 0, 0);
		}

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		m_deviceContext->IASetVertexBuffers(0, 1, modelData.vertexBuffer.GetAddressOf(), &stride, &offset);
		m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_deviceContext->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

		std::unordered_map<int, std::vector<size_t>> materialMeshes;
		for (size_t i = 0; i < modelData.meshes.size(); i++)
		{
			materialMeshes[modelData.meshes[i].materialId].push_back(i);
		}

		for (const auto& [materialId, meshIndices] : materialMeshes)
		{
			const Material& material = materialId != -1 ? modelData.materials[materialId] : m_defaultMaterial;
			m_deviceContext->PSSetConstantBuffers(1, 1, material.materialBuffer.GetAddressOf());
			m_deviceContext->PSSetShaderResources(0, 1, getTexture(material.useDiffuseTexture ? material.diffuseTextureId: m_defaultMaterial.diffuseTextureId).GetAddressOf());
			for (size_t meshIndex : meshIndices)
			{
				const SubMesh& mesh = modelData.meshes[meshIndex];
				m_deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
				m_deviceContext->DrawIndexed(mesh.indices.size(), 0, 0);
			}
		}

	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::preRenderUI()
	{
		ImGui_ImplDX11_NewFrame();
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::postRenderUI()
	{
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::render()
	{
		m_swapChain->Present(0, 0);
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::createDeviceAndSwapChain(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;

		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&swapChainDesc,
			m_swapChain.GetAddressOf(),
			m_device.GetAddressOf(),
			nullptr,
			m_deviceContext.GetAddressOf()
		);

		ASSERT(!FAILED(hr), "Can't create device and swapchain, error code: {}", hr);
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::createSamplerState()
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		HRESULT hr = m_device->CreateSamplerState(&samplerDesc, m_samplerState.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create sampler state, error code: {}", hr);
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::createRenderTarget(HWND hwnd)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());

		RECT rect;
		GetClientRect(hwnd, &rect);
		auto width = static_cast<float>(rect.right - rect.left);
		auto height = static_cast<float>(rect.bottom - rect.top);

		D3D11_TEXTURE2D_DESC depthStencilDesc = {};
		depthStencilDesc.Width = static_cast<UINT>(width);
		depthStencilDesc.Height = static_cast<UINT>(height);
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		ComPtr<ID3D11Texture2D> depthStencilBuffer;
		HRESULT hr = m_device->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilBuffer.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create depth stencil buffer, error code: {}", hr);

		hr = m_device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, m_depthStencilView.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create depth stencil view, error code: {}", hr);

		m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::createShaders()
	{
		auto vsBytecode = Utils::loadBytesFromFile("VertexShader.cso");
		auto psBytecode = Utils::loadBytesFromFile("PixelShader.cso");

		m_device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, m_vertexShader.GetAddressOf());
		m_device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, m_pixelShader.GetAddressOf());
		m_deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
		m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, texCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBytecode.data(), vsBytecode.size(), m_inputLayout.GetAddressOf());
		m_deviceContext->IASetInputLayout(m_inputLayout.Get());

		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(ConstantBuffer);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create constant buffer, error code: {}", hr);

		D3D11_RASTERIZER_DESC rasterizerDesc;
		ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

		rasterizerDesc.CullMode = D3D11_CULL_BACK; 
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthClipEnable = TRUE;

		ID3D11RasterizerState* rasterizerState;
		hr = m_device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
		ASSERT(!FAILED(hr), "Can't create rasterizer state, error code: {}", hr);

		m_deviceContext->RSSetState(rasterizerState);

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

		ComPtr<ID3D11DepthStencilState> depthStencilState;
		hr = m_device->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create depth stencil state, error code: {}", hr);

		m_deviceContext->OMSetDepthStencilState(depthStencilState.Get(), 1);
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::createBuffersForModel(ModelData& model)
	{
		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Vertex) * model.vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexData = {};
		vertexData.pSysMem = model.vertices.data();
		HRESULT hr = m_device->CreateBuffer(&vertexBufferDesc, &vertexData, model.vertexBuffer.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create vertex buffer, error code: {}", hr);
		if (FAILED(hr))
		{
			return false;
		}

		for (Material& material : model.materials)
		{
			D3D11_BUFFER_DESC cbDesc = {};
			cbDesc.Usage = D3D11_USAGE_DEFAULT;
			cbDesc.ByteWidth = sizeof(MaterialBuffer);
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = 0;
			HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, material.materialBuffer.GetAddressOf());
			ASSERT(!FAILED(hr), "Can't create constant buffer, error code: {}", hr);
		}

		for (SubMesh& subMesh: model.meshes)
		{
			D3D11_BUFFER_DESC indexBufferDesc = {};
			indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			indexBufferDesc.ByteWidth = sizeof(unsigned int) * subMesh.indices.size();
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexBufferDesc.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA indexData = {};
			indexData.pSysMem = subMesh.indices.data();

			hr = m_device->CreateBuffer(&indexBufferDesc, &indexData, subMesh.indexBuffer.GetAddressOf());
			ASSERT(!FAILED(hr), "Can't create index buffer, error code: {}", hr);
			if (FAILED(hr)) 
			{
				return false;
			}
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	const ComPtr<ID3D11ShaderResourceView>& DirectXRenderer::getTexture(const std::string& textureId) const
	{
		const auto& textureItr = m_textures.find(textureId);

		ASSERT(textureItr != m_textures.end(), "Can't find texture: {}", textureId);
		if (textureItr != m_textures.end())
		{
			return textureItr->second;
		}

		const auto& defaultTextureItr = m_textures.find(DEFAULT_TEXTURE);
		ASSERT(defaultTextureItr != m_textures.end(), "Can't find default texture: {}", textureId);
		if (defaultTextureItr != m_textures.end())
		{
			return defaultTextureItr->second;
		}

		return nullptr;
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::loadModel(const std::string& filename)
	{
		if (m_models.contains(filename))
		{
			return true;
		}

		ModelData modelData;
		if (!loadModelFromFile(modelData, filename))
		{
			return false;
		}

		if (!createBuffersForModel(modelData))
		{
			return false;
		}

		m_models.emplace(filename, std::move(modelData));
		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::loadTexture(const std::string& filename)
	{
		if (m_textures.contains(filename))
		{
			return true;
		}

		ComPtr<ID3D11ShaderResourceView> texture;

		HRESULT hr = DirectX::CreateWICTextureFromFileEx(
			m_device.Get(),
			m_deviceContext.Get(), 
			Utils::stringToWString(filename).c_str(),
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE,
			0,
			D3D11_RESOURCE_MISC_GENERATE_MIPS,
			DirectX::WIC_LOADER_FORCE_SRGB,
			nullptr,
			texture.GetAddressOf()
		);
		ASSERT(!FAILED(hr), "Can't load texture: {}", filename);
		if (FAILED(hr))
		{
			return false;
		}

		m_textures.emplace(filename, std::move(texture));
		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::loadModelFromFile(ModelData& model, const std::string& filename)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		std::filesystem::path fullPath(filename);
		std::filesystem::path matDir = fullPath.parent_path();
		bool res = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), matDir.string().c_str());
		ASSERT(res, "Can't load model: {}", filename);
		if (!res) 
		{
			return false;
		}

		for (const tinyobj::material_t& mat : materials)
		{
			Material matX;
			matX.ambientColor = XMFLOAT3(mat.ambient);
			matX.diffuseColor = XMFLOAT3(mat.diffuse);
			matX.useDiffuseTexture = mat.diffuse_texname.empty() ? 0.0f: 1.0f;
			matX.shininess = mat.shininess;
			matX.specularColor = XMFLOAT3(mat.specular);
			if (!mat.diffuse_texname.empty())
			{
				std::string path = (matDir / mat.diffuse_texname).string();
				if (loadTexture(path))
				{
					matX.diffuseTextureId = path;
				}
				else
				{
					matX.diffuseTextureId = m_defaultMaterial.diffuseTextureId;
				}
			}
			else
			{
				matX.diffuseTextureId = m_defaultMaterial.diffuseTextureId;
			}
			model.materials.emplace_back(matX);
		}

		for (const auto& shape : shapes)
		{
			SubMesh mesh;
			std::vector<Vertex> localVertices;
			std::vector<uint32_t> localIndices;

			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (index.normal_index >= 0) 
				{
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0) 
				{
					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}

				localVertices.push_back(vertex);
				localIndices.push_back(localVertices.size() - 1);
				mesh.indices.push_back(model.vertices.size() + localIndices.back());
			}

			if (attrib.normals.empty())
			{
				for (size_t i = 0; i < localIndices.size(); i += 3)
				{
					Vertex& v0 = localVertices[localIndices[i + 0]];
					Vertex& v1 = localVertices[localIndices[i + 1]];
					Vertex& v2 = localVertices[localIndices[i + 2]];

					XMVECTOR p0 = XMLoadFloat3(&v0.position);
					XMVECTOR p1 = XMLoadFloat3(&v1.position);
					XMVECTOR p2 = XMLoadFloat3(&v2.position);

					XMVECTOR edge1 = XMVectorSubtract(p1, p0);
					XMVECTOR edge2 = XMVectorSubtract(p2, p0);
					XMVECTOR faceNormal = XMVector3Normalize(XMVector3Cross(edge1, edge2));

					XMFLOAT3 fn;
					XMStoreFloat3(&fn, faceNormal);

					v0.normal.x += fn.x;
					v0.normal.y += fn.y;
					v0.normal.z += fn.z;

					v1.normal.x += fn.x;
					v1.normal.y += fn.y;
					v1.normal.z += fn.z;

					v2.normal.x += fn.x;
					v2.normal.y += fn.y;
					v2.normal.z += fn.z;
				}

				for (Vertex& vertex : localVertices)
				{
					XMVECTOR normal = XMLoadFloat3(&vertex.normal);
					normal = XMVector3Normalize(normal);
					XMStoreFloat3(&vertex.normal, normal);
				}
			}

			model.vertices.insert(model.vertices.end(), localVertices.begin(), localVertices.end());
			mesh.materialId = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
			model.meshes.emplace_back(std::move(mesh));
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation)
	{
		XMVECTOR rotationQuaternion = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    
		XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		forward = XMVector3Rotate(forward, rotationQuaternion);
		up = XMVector3Rotate(up, rotationQuaternion);
		XMFLOAT3 positionX(position.x, position.y, position.z);
		XMVECTOR target = XMVectorAdd(XMLoadFloat3(&positionX), forward);
    
		m_viewMatrix = XMMatrixLookAtLH(XMLoadFloat3(&positionX), target, up);
		m_constantBufferData.viewMatrix = XMMatrixTranspose(m_viewMatrix);
	}

	////////////////////////////////////////////////////////////////////////

	std::unique_ptr<IModelInstance> DirectXRenderer::createModelInstance(const std::string& filename)
	{
		return std::make_unique<ModelInstanceBase>(filename);
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::destroyModelInstance(IModelInstance& modelInstance)
	{
		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::unloadTexture(const std::string& filename)
	{
		const auto& itr = m_textures.find(filename);
		if (itr == m_textures.end())
		{
			return true;
		}

		ComPtr<ID3D11ShaderResourceView>& texture = itr->second;
		texture.Reset();

		m_textures.erase(itr);
		return true;
	}

	////////////////////////////////////////////////////////////////////////

	bool DirectXRenderer::unloadModel(const std::string& filename)
	{
		const auto& itr = m_models.find(filename);
		if (itr == m_models.end())
		{
			return true;
		}

		ModelData& modelData = itr->second;
		for (SubMesh& mesh : modelData.meshes)
		{
			mesh.indexBuffer.Reset();
		}

		modelData.vertexBuffer.Reset();

		m_models.erase(itr);

		return true;
		
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::cleanUp()
	{
#ifdef _SHOWUI
		cleanUpUI();
#endif

		for (const std::string& modelId : Utils::getKeys(m_models))
		{
			unloadModel(modelId);
		}

		for (const std::string& textureId : Utils::getKeys(m_textures))
		{
			unloadTexture(textureId);
		}

		destroyComPtrSafe(m_samplerState);
		destroyComPtrSafe(m_constantBuffer);
		destroyComPtrSafe(m_vertexShader);
		destroyComPtrSafe(m_pixelShader);
		destroyComPtrSafe(m_inputLayout);
		destroyComPtrSafe(m_depthStencilView);
		destroyComPtrSafe(m_renderTargetView);
		destroyComPtrSafe(m_swapChain);
		destroyComPtrSafe(m_deviceContext);
		destroyComPtrSafe(m_device);
	}

	////////////////////////////////////////////////////////////////////////

	XMMATRIX DirectXRenderer::getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
	{
		XMMATRIX scalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

		XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

		XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

		return scalingMatrix * rotationMatrix * translationMatrix;
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::createViewport(HWND hwnd)
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		auto width = static_cast<float>(rect.right - rect.left);
		auto height = static_cast<float>(rect.bottom - rect.top);

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		m_deviceContext->RSSetViewports(1, &viewport);

		m_viewMatrix = XMMatrixLookAtLH(XMVectorSet(0.0f, 2.0f, -5.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		m_projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / height, 0.1f, 500.0f);
		m_constantBufferData.viewMatrix = XMMatrixTranspose(m_viewMatrix);
		m_constantBufferData.projectionMatrix = XMMatrixTranspose(m_projectionMatrix);
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::setLightProperties(const Utils::Vector3& direction, float intensity)
	{
		Utils::Vector3 normalizedDirection = direction.normalized();
		m_constantBufferData.lightDirection = XMFLOAT3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z);
		m_constantBufferData.lightIntensity = intensity;
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::createDefaultMaterial()
	{
		bool loadDefaultTexRes = loadTexture(DEFAULT_TEXTURE);
		ASSERT(loadDefaultTexRes, "Can't load default texture");

		m_defaultMaterial.diffuseTextureId = DEFAULT_TEXTURE;
		m_defaultMaterial.ambientColor = XMFLOAT3(DEFAULT_AMBIENT.x, DEFAULT_AMBIENT.y, DEFAULT_AMBIENT.z);
		m_defaultMaterial.diffuseColor = XMFLOAT3(DEFAULT_DIFFUSE.x, DEFAULT_DIFFUSE.y, DEFAULT_DIFFUSE.z);
		m_defaultMaterial.specularColor = XMFLOAT3(DEFAULT_SPECULAR.x, DEFAULT_SPECULAR.y, DEFAULT_SPECULAR.z);
		m_defaultMaterial.shininess = DEFAULT_SHININESS;
		m_defaultMaterial.useDiffuseTexture = 1.0f;

		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(MaterialBuffer);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, m_defaultMaterial.materialBuffer.GetAddressOf());
		ASSERT(!FAILED(hr), "Can't create constant buffer, error code: {}", hr);

		MaterialBuffer mb{};
		mb.ambientColor = m_defaultMaterial.ambientColor;
		mb.diffuseColor = m_defaultMaterial.diffuseColor;
		mb.specularColor = m_defaultMaterial.specularColor;
		mb.shininess = m_defaultMaterial.shininess;
		mb.useDiffuseTexture = m_defaultMaterial.useDiffuseTexture;
		m_deviceContext->UpdateSubresource(m_defaultMaterial.materialBuffer.Get(), 0, nullptr, &mb, 0, 0);
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::initUI()
	{
		ImGui_ImplDX11_Init(m_device.Get(), m_deviceContext.Get());
	}

	////////////////////////////////////////////////////////////////////////

	void DirectXRenderer::cleanUpUI()
	{
		ImGui_ImplDX11_Shutdown();
	}

	////////////////////////////////////////////////////////////////////////

}