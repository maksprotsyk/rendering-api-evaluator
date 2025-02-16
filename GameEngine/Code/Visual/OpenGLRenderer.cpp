#define GLM_FORCE_LEFT_HANDED
#define WGL_WGLEXT_PROTOTYPES

#include "OpenGLRenderer.h"
#include <stdexcept>
#include <iostream>
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include <GL/wglext.h>
#include <GL/glew.h>
#include <fstream>

namespace Engine::Visual
{
    ////////////////////////////////////////////////////////////////////////

    static float gammaCorrection(float value)
    {
        return std::pow(value, 1.0f / 2.2f);
    }

    ////////////////////////////////////////////////////////////////////////

    static void checkError()
    {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << error << std::endl;
        }
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::init(const Window& window)
    {
        HWND hwnd = window.getHandle();
        m_hdc = GetDC(hwnd); // Get the device context from the HWND

        // Set up the pixel format for the HDC
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int pixelFormat = ChoosePixelFormat(m_hdc, &pfd);
        if (pixelFormat == 0) {
            throw std::runtime_error("Failed to choose a suitable pixel format");
        }

        if (!SetPixelFormat(m_hdc, pixelFormat, &pfd)) {
            throw std::runtime_error("Failed to set the pixel format");
        }

        // Create a legacy OpenGL rendering context
        HGLRC tempContext = wglCreateContext(m_hdc);
        if (!tempContext) {
            throw std::runtime_error("Failed to create an OpenGL rendering context");
        }

        if (!wglMakeCurrent(m_hdc, tempContext)) {
            throw std::runtime_error("Failed to activate the OpenGL rendering context");
        }

        // Load modern OpenGL functions using GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize GLEW");
        }

        // Create a modern OpenGL context
        typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        if (wglCreateContextAttribsARB) {
            int attribs[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                WGL_CONTEXT_MINOR_VERSION_ARB, 5,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0 // End of attributes list
            };

            HGLRC hglrcNew = wglCreateContextAttribsARB(m_hdc, nullptr, attribs);
            if (hglrcNew) {
                wglMakeCurrent(nullptr, nullptr); // Unset the old context
                wglDeleteContext(tempContext);
                tempContext = hglrcNew;

                if (!wglMakeCurrent(m_hdc, tempContext)) {
                    throw std::runtime_error("Failed to activate the modern OpenGL rendering context");
                }
            }
        }

        // Store the HGLRC for later use
        m_hglrc = tempContext;

        // Set up the initial OpenGL state
        glClearColor(0.0f, 0.2f, 0.4f, 1.0f); // Set the clear color
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);

        // Create and compile shaders (using GLSL files)
        m_shaderProgram = createShaderProgram("VertexShader.glsl", "FragmentShader.glsl");
        glUseProgram(m_shaderProgram);

        // Get uniform locations for matrices
        m_viewMatrixLoc = glGetUniformLocation(m_shaderProgram, "viewMatrix");
        m_projectionMatrixLoc = glGetUniformLocation(m_shaderProgram, "projectionMatrix");
        m_modelMatrixLoc = glGetUniformLocation(m_shaderProgram, "modelMatrix");

        if (!loadTexture(DEFAULT_TEXTURE))
        {
            // TODO: asserts
        }
        m_defaultMaterial.diffuseTextureId = DEFAULT_TEXTURE;
        m_defaultMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
        m_defaultMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
        m_defaultMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
        m_defaultMaterial.shininess = 32.0f;

        RECT rect;
        GetClientRect(window.getHandle(), &rect);
        glViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
        float aspectRatio = (float)(rect.right - rect.left) / (float)(rect.bottom - rect.top);

        m_projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

        auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT) {
            wglSwapIntervalEXT(0); // Enable VSync
            std::cout << "VSync enabled successfully." << std::endl;
        }
        else {
            std::cerr << "Failed to load wglSwapIntervalEXT. VSync control is not available." << std::endl;
        }
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::clearBackground(float r, float g, float b, float a)
    {
        glClearColor(
            gammaCorrection(r),
            gammaCorrection(g),
            gammaCorrection(b),
            gammaCorrection(a)
        );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::draw(const IModelInstance& model, const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
    {
        const auto& modelItr = m_models.find(model.GetId());
        if (modelItr == m_models.end())
        {
            //TODO: asserts
            return;
        }
        const ModelData& modelData = modelItr->second;
        glm::mat4 worldMatrix = getWorldMatrix(position, rotation, scale);
        // Use the shader program
        glUseProgram(m_shaderProgram);
        checkError();

        // Bind the VAO
        glBindVertexArray(modelData.vao);
        checkError();

        // Set view and projection matrices
        glUniformMatrix4fv(m_viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
        glUniformMatrix4fv(m_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
        glUniformMatrix4fv(m_modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(worldMatrix));
        checkError();

        // Iterate through the meshes in the model and render each
        for (const auto& mesh : modelData.meshes)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
            checkError();

            // Set the material properties in the shader
            const Material& material = mesh.materialId != -1 ? modelData.materials[mesh.materialId] : m_defaultMaterial;
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "ambientColor"), 1, glm::value_ptr(material.ambientColor));
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "diffuseColor"), 1, glm::value_ptr(material.diffuseColor));
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "specularColor"), 1, glm::value_ptr(material.specularColor));
            glUniform1f(glGetUniformLocation(m_shaderProgram, "shininess"), material.shininess);
            checkError();

            // Bind texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, getTexture(material.diffuseTextureId));
            glUniform1i(glGetUniformLocation(m_shaderProgram, "diffuseTexture"), 0);
            checkError();

            // Draw the mesh
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
            checkError();
        }

        // Unbind the VAO
        glBindVertexArray(0);
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::render()
    {
        // Swap buffers to display the rendered frame
        SwapBuffers(m_hdc);

        checkError();
    }

    ////////////////////////////////////////////////////////////////////////

    const GLuint& OpenGLRenderer::getTexture(const std::string& textureId) const
    {
        const auto& textureItr = m_textures.find(textureId);
        if (textureItr != m_textures.end())
        {
            return textureItr->second;
        }

        const auto& defaultTextureItr = m_textures.find(DEFAULT_TEXTURE);
        if (defaultTextureItr != m_textures.end())
        {
            return defaultTextureItr->second;
        }

        // TODO: asserts
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createBuffersForModel(ModelData& model)
    {
        // Create and bind the VAO
        glGenVertexArrays(1, &model.vao);
        glBindVertexArray(model.vao);

        // Create and bind the vertex buffer
        glGenBuffers(1, &model.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(Vertex), model.vertices.data(), GL_STATIC_DRAW);

        // Define the vertex attributes (position, normal, texCoord)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        // Create and bind the element buffer (EBO) for each sub-mesh
        for (auto& subMesh : model.meshes)
        {
            glGenBuffers(1, &subMesh.indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, subMesh.indices.size() * sizeof(unsigned int), subMesh.indices.data(), GL_STATIC_DRAW);
        }

        // Unbind the VAO
        glBindVertexArray(0);
    }

    ////////////////////////////////////////////////////////////////////////

    bool OpenGLRenderer::loadModelFromFile(ModelData& model, const std::string& filename)
    {
        std::filesystem::path fullPath(filename);
        std::filesystem::path matDir = fullPath.parent_path();

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), matDir.string().c_str());
        if (!success) 
        {
            return false;
        }

        // Load materials
        for (const auto& mat : materials)
        {
            Material material;
            material.ambientColor = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
            material.diffuseColor = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
            material.specularColor = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
            material.shininess = mat.shininess;

            if (!mat.diffuse_texname.empty())
            {

                std::string texturePath = (matDir / mat.diffuse_texname).string();
                if (loadTexture(texturePath))
                {
                    material.diffuseTextureId = texturePath;
                }
                else
                {
                    material.diffuseTextureId = m_defaultMaterial.diffuseTextureId;
                }
            }
            else
            {
                material.diffuseTextureId = m_defaultMaterial.diffuseTextureId;
            }

            model.materials.push_back(material);
        }

        // Load shapes and vertices
        for (const auto& shape : shapes)
        {
            SubMesh subMesh;

            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex = {};
                vertex.position = glm::vec3(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                );

                if (index.normal_index >= 0) {
                    vertex.normal = glm::vec3(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    );
                }

                if (index.texcoord_index >= 0) {
                    vertex.texCoord = glm::vec2(
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    );
                }

                model.vertices.push_back(vertex);
                subMesh.indices.push_back(static_cast<unsigned int>(model.vertices.size() - 1));
            }

            subMesh.materialId = shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[0];
            model.meshes.push_back(subMesh);
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation)
    {
		glm::vec3 pos = glm::vec3(position.x, position.y, position.z);

        float pitch = -rotation.x;
        float yaw = rotation.y; 
        float roll = rotation.z; 

        glm::vec3 forward;
        forward.x = cos(pitch) * sin(yaw);
        forward.y = sin(pitch);
        forward.z = cos(pitch) * cos(yaw);
        forward = glm::normalize(forward);

        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 right = glm::normalize(glm::cross(up, forward));

        glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), roll, forward);
        right = glm::vec3(rollMatrix * glm::vec4(right, 0.0f));
        up = glm::normalize(glm::cross(forward, right));

        m_viewMatrix = glm::lookAt(
            pos,
            pos + forward,
            up
        );
    }

    ////////////////////////////////////////////////////////////////////////

    std::unique_ptr<IModelInstance> OpenGLRenderer::createModelInstance(const std::string& filename)
    {
        return std::make_unique<ModelInstanceBase>(filename);
    }

    ////////////////////////////////////////////////////////////////////////

    glm::mat4 OpenGLRenderer::getWorldMatrix(const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, position.z));
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));

        return translation * rotationX * rotationY * rotationZ * scaling;
    }

    ////////////////////////////////////////////////////////////////////////

    bool OpenGLRenderer::loadModel(const std::string& filename)
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
        createBuffersForModel(modelData);

        m_models.emplace(filename, std::move(modelData));
        return true;
    }

    ////////////////////////////////////////////////////////////////////////

    bool OpenGLRenderer::loadTexture(const std::string& filename)
    {
        if (m_textures.contains(filename))
        {
            return true;
        }

        int width, height, channels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!data) {
            return false;
        }


        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        m_textures.emplace(filename, texture);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////

    GLuint OpenGLRenderer::createShader(const std::string& source, GLenum shaderType)
    {
        std::string content = Utils::readFile(source);
        GLuint shader = glCreateShader(shaderType);
        const char* src = content.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error: " << infoLog << std::endl;
            throw std::runtime_error("Shader compilation failed.");
        }
        return shader;
    }

    ////////////////////////////////////////////////////////////////////////

    GLuint OpenGLRenderer::createShaderProgram(const std::string& vsSource, const std::string& fsSource)
    {
        GLuint vertexShader = createShader(vsSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = createShader(fsSource, GL_FRAGMENT_SHADER);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Program linking error: " << infoLog << std::endl;
            throw std::runtime_error("Shader program linking failed.");
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    ////////////////////////////////////////////////////////////////////////
}
