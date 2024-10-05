#include "DirectXRenderer.h"

#include <stdexcept>
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Engine::Visual
{
	DirectXRenderer::DirectXRenderer(const Window& window)
	{
		createDeviceAndSwapChain(window._window);
		createRenderTarget();
		createShaders();
		createViewport(window._window);

		loadTexture(defaultMaterial.diffuseTexture, TEXT("C:/Users/sonia/Maksym/white_tex.jpg"));
		defaultMaterial.diffuseColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		defaultMaterial.shininess = 10.0f;
	}

	void DirectXRenderer::clearBackground()
	{
		// Clear the render target with a solid color
		float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f }; // RGBA
		deviceContext->ClearRenderTargetView(renderTargetView.Get(), clearColor);
	}

	void DirectXRenderer::draw(const Model& model)
	{
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

			auto itr = model.materials.find(mesh.material);
			const Material& material = itr != model.materials.end() ? itr->second: defaultMaterial;
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
		swapChain->Present(1, 0);
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
	void DirectXRenderer::createRenderTarget()
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());
		deviceContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);
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
	}

	void DirectXRenderer::createBuffersFromModel(Model& model) const
	{
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

	void DirectXRenderer::loadTexture(ComPtr<ID3D11ShaderResourceView>& texture, const std::wstring& filename) const
	{
		HRESULT hr = DirectX::CreateWICTextureFromFile(device.Get(), deviceContext.Get(), filename.c_str(), nullptr, texture.GetAddressOf());
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to load texture.");
		}
	}

	void DirectXRenderer::loadMaterials(std::unordered_map<std::string, Material>& materials, const std::string& filename) const
	{
		std::ifstream file(filename);
		if (!file.is_open())
		{
			return;
		}
		std::string currentMaterial = "";

		std::string line;
		while (std::getline(file, line))
		{
			std::stringstream ss(line);
			std::string prefix;
			ss >> prefix;
			if (prefix != "newmtl")
			{
				if (currentMaterial.empty())
				{
					continue;
				}
			}
			else
			{
				ss >> currentMaterial;
				materials[currentMaterial] = Material{};
			}

			Material& material = materials[currentMaterial];

			if (prefix == "Ka")
			{
				ss >> material.ambientColor.x >> material.ambientColor.y >> material.ambientColor.z;
			}
			else if (prefix == "Kd")
			{
				ss >> material.diffuseColor.x >> material.diffuseColor.y >> material.diffuseColor.z;
			}
			else if (prefix == "Ks")
			{
				ss >> material.specularColor.x >> material.specularColor.y >> material.specularColor.z;
			}
			else if (prefix == "Ns")
			{
				ss >> material.shininess;
			}
			else if (prefix == "map_Kd")
			{
				std::string textureFile;
				ss >> textureFile;

				std::filesystem::path fullPath(filename);
				std::filesystem::path directoryPath = fullPath.parent_path();
				std::string texturePath = (directoryPath / textureFile).string();

				loadTexture(material.diffuseTexture, Utils::stringToWString(texturePath));
			}
		}

	}

	void DirectXRenderer::loadModel(Model& model, const std::string& filename)
	{

		std::ifstream file(filename);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open OBJ file.");
		}

		std::string line;
		int currentSubMeshIdx = -1;

		std::vector<XMFLOAT3> positions;
		std::vector<XMFLOAT3> normals;
		std::vector<XMFLOAT2> texCoords;

		while (std::getline(file, line))
		{
			std::stringstream ss(line);
			std::string prefix;
			ss >> prefix;

			if (prefix == "v")
			{
				XMFLOAT3 pos;
				ss >> pos.x >> pos.y >> pos.z;
				positions.push_back(pos);
			}
			else if (prefix == "vt")
			{
				XMFLOAT2 tex;
				ss >> tex.x >> tex.y;
				texCoords.push_back(tex);
			}
			else if (prefix == "vn")
			{
				XMFLOAT3 norm;
				ss >> norm.x >> norm.y >> norm.z;
				normals.push_back(norm);
			}
			else if (prefix == "f")
			{
				// If currentSubMesh is valid, add face indices to it
				if (currentSubMeshIdx == - 1) 
				{
					continue;
				}
				SubMesh& mesh = model.meshes[currentSubMeshIdx];
				for (const auto& vertex : parseIndicesLine(ss.str(), positions, normals, texCoords))
				{
					model.vertices.push_back(vertex);
					mesh.indices.push_back(model.vertices.size() - 1);
				}
			}
			else if (prefix == "usemtl")
			{
				// Create a new sub-mesh with a new material
				std::string materialName;
				ss >> materialName;

				SubMesh subMesh;
				subMesh.material = materialName;
				model.meshes.push_back(subMesh);
				currentSubMeshIdx++;
			}
			else if (prefix == "mtllib")
			{
				std::string mtlFile;
				ss >> mtlFile;
				std::filesystem::path fullPath(filename);
				std::filesystem::path directoryPath = fullPath.parent_path();
				loadMaterials(model.materials, (directoryPath / mtlFile).string());
			}
		}

	}

	void DirectXRenderer::setCameraProperties(const Utils::Vector& position)
	{
		viewMatrix = XMMatrixLookAtLH(XMVectorSet(position.x, position.y, position.z, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	}

	// Set up the viewport
	void DirectXRenderer::createViewport(HWND hwnd)
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		float width = static_cast<float>(rect.right - rect.left);
		float height = static_cast<float>(rect.bottom - rect.top);

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

	std::vector<DirectXRenderer::Vertex> DirectXRenderer::parseIndicesLine(const std::string& line, const std::vector<XMFLOAT3>& positions, const std::vector<XMFLOAT3>& normals, const std::vector<XMFLOAT2>& texCoords)
	{
		std::vector<std::string> verticesStr = Utils::splitString(line, ' ');
		verticesStr.erase(verticesStr.begin());
		std::vector<std::array<int, 3>> indices;
		for (const std::string& vStr : verticesStr)
		{
			if (vStr.empty())
			{
				continue;
			}

			std::vector<std::string> indicesStr = Utils::splitString(vStr, '/');
			if (indicesStr.size() == 2)
			{
				indices.emplace_back(std::array<int, 3>{ std::stoi(indicesStr[0]), std::stoi(indicesStr[1]), -1});
			}
			else
			{
				indices.emplace_back(std::array<int, 3>{ std::stoi(indicesStr[0]), std::stoi(indicesStr[1]), std::stoi(indicesStr[2])});
			}
		}

		std::vector<Vertex> vertices;
		if (indices[0][2] < 0)
		{
			XMFLOAT3 faceNormal = computeFaceNormal(positions[indices[0][0] - 1], positions[indices[2][0] - 1], positions[indices[1][0] - 1]);
			for (int i = 0; i < 3; i++)
			{
				const auto& vertIndex = indices[i];
				Vertex vertex { positions[vertIndex[0] - 1], faceNormal, texCoords[vertIndex[1] - 1] };
				vertices.emplace_back(std::move(vertex));
			}

			if (indices.size() >= 4)
			{
				faceNormal = computeFaceNormal(positions[indices[1][0] - 1], positions[indices[2][0] - 1], positions[indices[3][0] - 1]);
				for (int i = 1; i < 4; i++)
				{
					const auto& vertIndex = indices[i];
					Vertex vertex { positions[vertIndex[0] - 1], faceNormal, texCoords[vertIndex[1] - 1] };
					vertices.emplace_back(std::move(vertex));
				}
			}
		}
		else
		{
			for (int i = 0; i < 3; i++)
			{
				const auto& vertIndex = indices[i];
				Vertex vertex{ positions[vertIndex[0] - 1], normals[vertIndex[2] - 1], texCoords[vertIndex[1] - 1]};
				vertices.emplace_back(std::move(vertex));
			}

			if (indices.size() >= 4)
			{
				for (int i = 1; i < 4; i++)
				{
					const auto& vertIndex = indices[i];
					Vertex vertex{ positions[vertIndex[0] - 1], normals[vertIndex[2] - 1], texCoords[vertIndex[1] - 1] };
					vertices.emplace_back(std::move(vertex));
				}
			}
		}

		return vertices;

	
	}



}