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
            GLuint diffuseTexture;
        };

        struct Model : public AbstractModel
        {
            std::vector<SubMesh> meshes;
            GLuint vertexBuffer;
            GLuint vao;
            std::vector<Vertex> vertices;
            std::vector<Material> materials;
            glm::mat4 worldMatrix;

			size_t GetVertexCount() const override
			{
				return vertices.size();
			}
        };

        void init(const Window& window) override;
        void clearBackground() override;
        void draw(const AbstractModel& model) override;
        void render() override;
        std::unique_ptr<AbstractModel> createModel() override;
        void createBuffersFromModel(AbstractModel& model) override;
        void loadModel(AbstractModel& model, const std::string& filename) override;
        void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) override;
        void transformModel(AbstractModel& model, const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale) override;

    private:
        HDC hdc;
        HGLRC hglrc;
        GLuint shaderProgram;
        GLuint viewMatrixLoc;
        GLuint projectionMatrixLoc;
        GLuint modelMatrixLoc;
        Material defaultMaterial;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        GLuint createShader(const std::string& source, GLenum shaderType);
        GLuint createShaderProgram(const std::string& vsSource, const std::string& fsSource);
        void loadTexture(GLuint& texture, const std::string& filename);
    };
}
