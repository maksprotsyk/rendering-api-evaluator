#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <wrl/client.h>
#include <WICTextureLoader.h>
#include <string>

#include "GL/glew.h"
#include "GL/wglext.h"

#include "IRenderer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace Engine::Visual
{

    class DirectXRenderer: public IRenderer
    {
    public:
        void init(const Window& window) override;
        void clearBackground(float r, float g, float b, float a) override;

        void draw(
            const IModelInstance& model,
            const Utils::Vector3& position,
            const Utils::Vector3& rotation,
            const Utils::Vector3& scale) override;

        void preRenderUI() override;
        void postRenderUI() override;
        void render() override;

        bool loadModel(const std::string& filename) override;
        bool loadTexture(const std::string& filename) override;

        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) override;
        void setLightProperties(const Utils::Vector3& direction, float intensity) override;
        std::unique_ptr<IModelInstance> createModelInstance(const std::string& filename) override;

        bool destroyModelInstance(IModelInstance& modelInstance) override;
        bool unloadTexture(const std::string& filename) override;
        bool unloadModel(const std::string& filename) override;
        void cleanUp() override;



    private:
        struct Vertex
        {
            XMFLOAT3 position;
            XMFLOAT3 normal;
            XMFLOAT2 texCoord;
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
            std::string diffuseTextureId;
            ComPtr<ID3D11Buffer> materialBuffer;
        };

        struct ModelData
        {
            std::vector<SubMesh> meshes;
            ComPtr<ID3D11Buffer> vertexBuffer;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;
        };

        struct ConstantBuffer
        {
            XMMATRIX worldMatrix;
            XMMATRIX viewMatrix;
            XMMATRIX projectionMatrix;
            XMFLOAT3 lightDirection;
            float lightIntensity;
        };

        struct MaterialBuffer
        {
            XMFLOAT3 ambientColor;
            float shininess;

            XMFLOAT3 diffuseColor;
            float padding1;

            XMFLOAT3 specularColor;
            float padding2;
        };

    private:

        static XMMATRIX getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale);

        template<class T>
        static inline void destroyComPtrSafe(ComPtr<T>& ptr);

        void createDeviceAndSwapChain(HWND hwnd);
        void createRenderTarget(HWND hwnd);
        void createShaders();
        void createViewport(HWND hwnd);
        void createDefaultMaterial();
        void initUI();
        void cleanUpUI();
        bool createBuffersForModel(ModelData& model);
        bool loadModelFromFile(ModelData& model, const std::string& filename);

        const ComPtr<ID3D11ShaderResourceView>& getTexture(const std::string& textureId) const;

    private:

        // DirectX components
        ComPtr<ID3D11Device> m_device;
        ComPtr<ID3D11DeviceContext> m_deviceContext;
        ComPtr<IDXGISwapChain> m_swapChain;
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;
        ComPtr<ID3D11InputLayout> m_inputLayout;
        ComPtr<ID3D11Buffer> m_constantBuffer;
        ComPtr<ID3D11SamplerState> m_samplerState;

        // Camera matrices
        XMMATRIX m_viewMatrix;
        XMMATRIX m_projectionMatrix;

        Material m_defaultMaterial;
        ConstantBuffer m_constantBufferData;

        std::unordered_map<std::string, ModelData> m_models;
        std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> m_textures;
        
    };

    template <class T>
    void inline DirectXRenderer::destroyComPtrSafe(ComPtr<T>& ptr)
    {
        if (ptr)
        {
            ptr.Reset();
        }
    }

}