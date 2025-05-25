#pragma once

#include <windows.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Utils/Vector.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "IRenderer.h"
#include <string>
#include <vector>

namespace Engine::Visual
{
    class OpenGLRenderer : public IRenderer
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
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;
        };

        struct SubMesh
        {
            std::vector<unsigned int> indices;
            GLuint indexBuffer;
            int materialId;
        };

        struct Material
        {
            glm::vec3 ambientColor;
            glm::vec3 diffuseColor;
            glm::vec3 specularColor;
            float shininess;
            std::string diffuseTextureId;
        };

        struct ModelData
        {
            std::vector<SubMesh> meshes;
            GLuint vertexBuffer;
            GLuint vao;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;
            glm::mat4 worldMatrix;
        };

    private:
        static glm::mat4 getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale);

        // init parts
        void setPixelFormat();
        void createWglContext();
        void setInitialOpenGLState();
        void createShaderProgram(const std::string& vsSource, const std::string& fsSource);
        void createShaderFields();
        void createFrameBuffer();
        void createViewport();
        void createDefaultMaterial();
        void initUI();
        void cleanUpUI();

        GLuint createShader(const std::string& source, GLenum shaderType);
        const GLuint& getTexture(const std::string& textureId) const;
        void createBuffersForModel(ModelData& model);
        bool loadModelFromFile(ModelData& model, const std::string& filename);

    private:
        HWND m_hwnd;
        HDC m_hdc;
        HGLRC m_hglrc;

        GLuint m_shaderProgram;
        GLuint m_viewMatrixLoc;
        GLuint m_projectionMatrixLoc;
        GLuint m_modelMatrixLoc;
		GLuint m_lightDirectionLoc;
		GLuint m_lightIntensityLoc;
        GLuint m_ambientColorLoc;
        GLuint m_diffuseColorLoc;
        GLuint m_specularColorLoc;
        GLuint m_shininessLoc;
        GLuint m_diffuseTextureLoc;
        GLuint m_frameBuffer;
        GLuint m_frameBufferTexture;

        Material m_defaultMaterial;
        glm::mat4 m_viewMatrix;
        glm::mat4 m_projectionMatrix;

        std::unordered_map<std::string, GLuint> m_textures;
        std::unordered_map<std::string, ModelData> m_models;

    };
}
