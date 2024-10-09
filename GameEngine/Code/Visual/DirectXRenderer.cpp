#include "DirectXRenderer.h"

#include <stdexcept>
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "tiny_obj_loader.h"

namespace Engine::Visual
{

	static float gammaCorrection(float value)
	{
		return std::pow(value, 1.0f / 2.2f);
	}

	void DirectXRenderer::init(const Window& window)
	{
		createDeviceAndSwapChain(window.getHandle());
		createRenderTarget(window.getHandle());
		createShaders();
		createViewport(window.getHandle());

		loadTexture(defaultMaterial.diffuseTexture, TEXT("../../Resources/cube/default.png"));
		defaultMaterial.diffuseColor = XMFLOAT3(0.1f, 0.1f, 0.1f);
		defaultMaterial.ambientColor = XMFLOAT3(0.5f, 0.5f, 0.5f);
		defaultMaterial.specularColor = XMFLOAT3(0.5f, 0.5f, 0.5f);
		defaultMaterial.shininess = 32.0f;
	}

	void DirectXRenderer::clearBackground(float r, float g, float b, float a)
	{
		// Clear the render target with a solid color
		float clearColor[] = {
			gammaCorrection(r),
			gammaCorrection(g),
			gammaCorrection(b),
			gammaCorrection(a)
		}; // RGBA
		deviceContext->ClearRenderTargetView(renderTargetView.Get(), clearColor);
		deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	void DirectXRenderer::draw(const AbstractModel& abstractModel)
	{
		const auto& model = (const Model&)abstractModel;

		// Bind the vertex and index buffers
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, model.vertexBuffer.GetAddressOf(), &stride, &offset);

		// Set the primitive topology
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (const auto& mesh : model.meshes)
		{
			// Update the constant buffer (world, view, projection matrices)
			ConstantBuffer cb;
			cb.worldMatrix = XMMatrixTranspose(model.worldMatrix);  // Transpose for HLSL
			cb.viewMatrix = XMMatrixTranspose(viewMatrix);
			cb.projectionMatrix = XMMatrixTranspose(projectionMatrix);

			const Material& material = mesh.materialId != -1 ? model.materials[mesh.materialId] : defaultMaterial;
			cb.ambientColor = material.ambientColor;
			cb.diffuseColor = material.diffuseColor;
			cb.specularColor = material.specularColor;
			cb.shininess = material.shininess;
			deviceContext->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

			// Bind the constant buffer and texture
			deviceContext->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
			deviceContext->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
			deviceContext->PSSetShaderResources(0, 1, material.diffuseTexture.GetAddressOf());
			deviceContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());

			// Bind the index buffer for this sub-mesh
			deviceContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

			// Draw the model
			deviceContext->DrawIndexed(mesh.indices.size(), 0, 0);
		}


	}

	void DirectXRenderer::render()
	{
		// Present the frame
		swapChain->Present(0, 0);
	}


	// Create the Direct3D device, swap chain, and device context
	void DirectXRenderer::createDeviceAndSwapChain(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
			swapChain.GetAddressOf(),
			device.GetAddressOf(),
			nullptr,
			deviceContext.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create DirectX device and swap chain.");
		}
	}

	// Create the render target
	void DirectXRenderer::createRenderTarget(HWND hwnd)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());

		RECT rect;
		GetClientRect(hwnd, &rect);
		auto width = static_cast<float>(rect.right - rect.left);
		auto height = static_cast<float>(rect.bottom - rect.top);

		// Create the depth stencil buffer
		D3D11_TEXTURE2D_DESC depthStencilDesc = {};
		depthStencilDesc.Width = static_cast<UINT>(width);
		depthStencilDesc.Height = static_cast<UINT>(height);
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		ComPtr<ID3D11Texture2D> depthStencilBuffer;
		HRESULT hr = device->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilBuffer.GetAddressOf());
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create depth stencil buffer.");
		}

		// Create the depth stencil view
		hr = device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, depthStencilView.GetAddressOf());
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create depth stencil view.");
		}

		// Set the render target and depth stencil
		deviceContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
	}

	// Create the shaders and input layout
	void DirectXRenderer::createShaders()
	{
		auto vsBytecode = Utils::loadBytesFromFile("VertexShader.cso");
		auto psBytecode = Utils::loadBytesFromFile("PixelShader.cso");

		// Create shaders
		device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, vertexShader.GetAddressOf());
		device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, pixelShader.GetAddressOf());
		deviceContext->VSSetShader(vertexShader.Get(), nullptr, 0);
		deviceContext->PSSetShader(pixelShader.Get(), nullptr, 0);

		// Create input layout
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, texCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBytecode.data(), vsBytecode.size(), inputLayout.GetAddressOf());
		deviceContext->IASetInputLayout(inputLayout.Get());

		// Create constant buffer
		D3D11_BUFFER_DESC cbDesc = {};
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.ByteWidth = sizeof(ConstantBuffer);
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, constantBuffer.GetAddressOf());
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create constant buffer.");
		}

		D3D11_RASTERIZER_DESC rasterizerDesc;
		ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

		// Enable culling of backfaces
		rasterizerDesc.CullMode = D3D11_CULL_BACK;  // Cull back-facing polygons
		rasterizerDesc.FillMode = D3D11_FILL_SOLID; // Render polygons as solid (not wireframe)
		rasterizerDesc.FrontCounterClockwise = FALSE; // Assume clockwise vertices are front-facing
		rasterizerDesc.DepthClipEnable = TRUE;  // Enable depth clipping

		// Create the rasterizer state
		ID3D11RasterizerState* rasterizerState;
		hr = device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create rasterizer state.");
		}

		// Bind the rasterizer state
		deviceContext->RSSetState(rasterizerState);

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

		ComPtr<ID3D11DepthStencilState> depthStencilState;
		hr = device->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf());
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create depth stencil state.");
		}

		// Set the depth stencil state
		deviceContext->OMSetDepthStencilState(depthStencilState.Get(), 1);
	}

	void DirectXRenderer::createBuffersFromModel(AbstractModel& abstractModel)
	{
		auto& model = (Model&)abstractModel;
		// Create vertex buffer
		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Vertex) * model.vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexData = {};
		vertexData.pSysMem = model.vertices.data();
		device->CreateBuffer(&vertexBufferDesc, &vertexData, model.vertexBuffer.GetAddressOf());

		for (auto& subMesh: model.meshes)
		{
			// Create the index buffer for this sub-mesh
			D3D11_BUFFER_DESC indexBufferDesc = {};
			indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			indexBufferDesc.ByteWidth = sizeof(unsigned int) * subMesh.indices.size();  // Size of index buffer
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexBufferDesc.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA indexData = {};
			indexData.pSysMem = subMesh.indices.data();  // Pointer to the index data

			HRESULT hr = device->CreateBuffer(&indexBufferDesc, &indexData, subMesh.indexBuffer.GetAddressOf());
			if (FAILED(hr)) 
			{
				throw std::runtime_error("Failed to create index buffer.");
			}
		}
	}

	std::unique_ptr<IRenderer::AbstractModel> DirectXRenderer::createModel()
	{
		return std::make_unique<Model>();
	}

	void DirectXRenderer::loadTexture(ComPtr<ID3D11ShaderResourceView>& texture, const std::wstring& filename) const
	{
		HRESULT hr = DirectX::CreateWICTextureFromFile(device.Get(), deviceContext.Get(), filename.c_str(), nullptr, texture.GetAddressOf());
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to load texture.");
		}
	}

	void DirectXRenderer::loadModel(AbstractModel& abstractModel, const std::string& filename)
	{
		auto& model = (Model&)abstractModel;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		std::filesystem::path fullPath(filename);
		std::filesystem::path matDir = fullPath.parent_path();
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), matDir.string().c_str());
		if (!ret) 
		{
			return;
		}

		std::vector<DirectX::XMFLOAT3> positions;
		std::vector<DirectX::XMFLOAT3> normals;
		std::vector<DirectX::XMFLOAT2> texcoords;
		std::vector<unsigned int> indices;
		
		for (const auto& shape : shapes)
		{
			model.meshes.emplace_back();
			SubMesh& mesh = model.meshes.back();
			mesh.materialId = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
			for (const auto& index : shape.mesh.indices) 
			{
				positions.emplace_back(
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				);

				if (!attrib.normals.empty()) {
					normals.emplace_back(
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					);
				}

				if (!attrib.texcoords.empty())
				{
					texcoords.emplace_back(
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1]
					);
				}

				mesh.indices.push_back(positions.size() - 1);
			}
		}

		if (normals.empty())
		{
			for (size_t i = 0; i + 2 < positions.size(); i += 3)
			{
				XMFLOAT3 normal = computeFaceNormal(positions[i], positions[i + 1], positions[i + 2]);
				for (int j = 0; j < 3; j++)
				{
					normals.push_back(normal);
				}
			}
		}

		for (int i = 0; i < positions.size(); i++)
		{
			model.vertices.emplace_back(Vertex{ positions[i], normals[i], texcoords[i] });
		}

		for (const tinyobj::material_t& mat : materials)
		{
			Material matX;
			matX.ambientColor = XMFLOAT3(mat.ambient);
			matX.diffuseColor = XMFLOAT3(mat.diffuse);
			matX.shininess = mat.shininess;
			matX.specularColor = XMFLOAT3(mat.specular);
			if (!mat.diffuse_texname.empty())
			{
				loadTexture(matX.diffuseTexture, Utils::stringToWString((matDir / mat.diffuse_texname).string()));
			}
			else
			{
				matX.diffuseTexture = defaultMaterial.diffuseTexture;
			}
			model.materials.emplace_back(matX);
		}
	}

	void DirectXRenderer::setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation)
	{
		XMVECTOR rotationQuaternion = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    
		// Define the forward and up vectors
		XMVECTOR forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    
		// Rotate forward and up vectors by the camera's rotation
		forward = XMVector3Rotate(forward, rotationQuaternion);
		up = XMVector3Rotate(up, rotationQuaternion);
    
		XMFLOAT3 positionX(position.x, position.y, position.z);
		// Calculate the target position based on forward direction
		XMVECTOR target = XMVectorAdd(XMLoadFloat3(&positionX), forward);
    
		// Create the view matrix using LookAt
		viewMatrix = XMMatrixLookAtLH(XMLoadFloat3(&positionX), target, up);
	}

	void DirectXRenderer::transformModel(AbstractModel& abstractModel, const Utils::Vector3& position,
		const Utils::Vector3& rotation, const Utils::Vector3& scale)
	{
		auto& model = (Model&)abstractModel;

		XMMATRIX scalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

		XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

		// Create a translation matrix from the position vector
		XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

		// Combine the rotation and translation matrices to form the world matrix
		model.worldMatrix = XMMatrixMultiply(scalingMatrix, XMMatrixMultiply(rotationMatrix, translationMatrix));
		
	}

	// Set up the viewport
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
		deviceContext->RSSetViewports(1, &viewport);

		// Set up camera view and projection matrices
		viewMatrix = XMMatrixLookAtLH(XMVectorSet(0.0f, 2.0f, -5.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / height, 0.1f, 1000.0f);
	}

	XMFLOAT3 DirectXRenderer::computeFaceNormal(const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2) {
		// Calculate two edges of the triangle
		XMFLOAT3 edge1 = { v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
		XMFLOAT3 edge2 = { v2.x - v0.x, v2.y - v0.y, v2.z - v0.z };

		// Compute the cross product of edge1 and edge2 to get the face normal
		XMFLOAT3 normal = {
			edge1.y * edge2.z - edge1.z * edge2.y,
			edge1.z * edge2.x - edge1.x * edge2.z,
			edge1.x * edge2.y - edge1.y * edge2.x
		};

		// Normalize the normal vector
		float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
		normal.x /= length;
		normal.y /= length;
		normal.z /= length;

		return normal;
	}

}