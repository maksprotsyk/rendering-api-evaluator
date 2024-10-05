#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <wrl/client.h>
#include <WICTextureLoader.h>
#include <string>

#include "Window.h"
#include "Utils/Vector.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace Engine::Visual
{

    class DirectXRenderer
    {
    public:
        struct Vertex
        {
            XMFLOAT3 position;
            XMFLOAT3 normal;
            XMFLOAT2 texCoord;
        };

        // Constant buffer structure for transformation matrices
        struct ConstantBuffer
        {
            XMMATRIX worldMatrix;
            XMMATRIX viewMatrix;
            XMMATRIX projectionMatrix;
            XMFLOAT3 ambientColor;
            XMFLOAT3 diffuseColor;
            XMFLOAT3 specularColor;  // Material's specular color
            float shininess;         // Material's shininess (specular exponent)
        };

        struct SubMesh
        {
            std::vector<unsigned int> indices;
            ComPtr<ID3D11Buffer> indexBuffer;
            int materialId;
        };

        struct Material
        {
            XMFLOAT3 ambientColor;
            XMFLOAT3 diffuseColor;
            XMFLOAT3 specularColor;
            float shininess;
            ComPtr<ID3D11ShaderResourceView> diffuseTexture; 
        };

        struct Model
        {
            std::vector<SubMesh> meshes;
            ComPtr<ID3D11Buffer> vertexBuffer;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;
            XMMATRIX worldMatrix;
        };


        explicit DirectXRenderer(const Window& window);

        void clearBackground();
        void draw(const Model& model);
        void render();

        void createBuffersFromModel(Model& model) const;

        void loadModel(Model& model, const std::string& filename);
        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation);

        static void transformModel(Model& model, const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale);

    private:
        // DirectX components
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> deviceContext;
        ComPtr<IDXGISwapChain> swapChain;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3D11Buffer> constantBuffer;
        ComPtr<ID3D11SamplerState> samplerState;

        // Camera matrices
        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;

        Material defaultMaterial;

        static XMFLOAT3 computeFaceNormal(const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2);

        void createDeviceAndSwapChain(HWND hwnd);
        void createRenderTarget();
        void createShaders();
        void createViewport(HWND hwnd);

        void loadTexture(ComPtr<ID3D11ShaderResourceView>& texture, const std::wstring& filename) const;
        
    };

}