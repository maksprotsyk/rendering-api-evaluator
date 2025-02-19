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
        void render() override;

        bool loadModel(const std::string& filename) override;
        bool loadTexture(const std::string& filename) override;

        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) override;
        std::unique_ptr<IModelInstance> createModelInstance(const std::string& filename) override;

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

        void createFrameBuffer(int width, int height);
        GLuint createShader(const std::string& source, GLenum shaderType);
        GLuint createShaderProgram(const std::string& vsSource, const std::string& fsSource);

        const GLuint& getTexture(const std::string& textureId) const;
        void createBuffersForModel(ModelData& model);
        bool loadModelFromFile(ModelData& model, const std::string& filename);

    private:
        HDC m_hdc;
        HGLRC m_hglrc;
        GLuint m_shaderProgram;
        GLuint m_viewMatrixLoc;
        GLuint m_projectionMatrixLoc;
        GLuint m_modelMatrixLoc;
        Material m_defaultMaterial;
        glm::mat4 m_viewMatrix;
        glm::mat4 m_projectionMatrix;

        std::unordered_map<std::string, GLuint> m_textures;
        std::unordered_map<std::string, ModelData> m_models;

    };
}
