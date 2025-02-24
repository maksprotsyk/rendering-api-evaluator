#define GLM_FORCE_LEFT_HANDED
#define WGL_WGLEXT_PROTOTYPES

#include "OpenGLRenderer.h"
#include <stdexcept>
#include <iostream>
#include <GL/wglext.h>
#include <GL/glew.h>

#include "stb_image.h"
#include "tiny_obj_loader.h"

#include "Utils/DebugMacros.h"


namespace Engine::Visual
{

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::init(const Window& window)
    {
        m_hwnd = window.getHandle();
        m_hdc = GetDC(m_hwnd);
    
        setPixelFormat();
        createWglContext();
        setInitialOpenGLState();
        createShaderProgram("VertexShader.glsl", "FragmentShader.glsl");
        createShaderFields();
        createFrameBuffer();
        createViewport();
        createDefaultMaterial();   
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::clearBackground(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(m_shaderProgram);
        ASSERT_OPENGL("Unable to use shader program");
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

        glBindVertexArray(modelData.vao);
        ASSERT_OPENGL("Unable to bind vertex buffer for model: {}", model.GetId());

        glUniformMatrix4fv(m_viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
        glUniformMatrix4fv(m_projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
        glUniformMatrix4fv(m_modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(worldMatrix));

        for (const auto& mesh : modelData.meshes)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
            ASSERT_OPENGL("Unable to bind index buffer for mesh of model: {}", model.GetId());

            const Material& material = mesh.materialId != -1 ? modelData.materials[mesh.materialId] : m_defaultMaterial;
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "ambientColor"), 1, glm::value_ptr(material.ambientColor));
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "diffuseColor"), 1, glm::value_ptr(material.diffuseColor));
            glUniform3fv(glGetUniformLocation(m_shaderProgram, "specularColor"), 1, glm::value_ptr(material.specularColor));
            glUniform1f(glGetUniformLocation(m_shaderProgram, "shininess"), material.shininess);
            ASSERT_OPENGL("Unable to set material properties for mesh of model: {}", model.GetId());

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, getTexture(material.diffuseTextureId));
            glUniform1i(glGetUniformLocation(m_shaderProgram, "diffuseTexture"), 0);
            ASSERT_OPENGL("Unable to set texture for mesh of model: {}", model.GetId());

            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
            ASSERT_OPENGL("Unable to draw mesh of model: {}", model.GetId());
        }

        glBindVertexArray(0);
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::render()
    {
        SwapBuffers(m_hdc);
        ASSERT_OPENGL("Unable to swap buffers and render");
    }

    ////////////////////////////////////////////////////////////////////////

    const GLuint& OpenGLRenderer::getTexture(const std::string& textureId) const
    {
        const auto& textureItr = m_textures.find(textureId);
        ASSERT(textureItr != m_textures.end(), "Can't find texture: {}", textureId);
        if (textureItr != m_textures.end())
        {
            return textureItr->second;
        }

        const auto& defaultTextureItr = m_textures.find(DEFAULT_TEXTURE);
        ASSERT(defaultTextureItr != m_textures.end(), "Can't find default texture: {}", DEFAULT_TEXTURE);
        if (defaultTextureItr != m_textures.end())
        {
            return defaultTextureItr->second;
        }
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createBuffersForModel(ModelData& model)
    {
        glGenVertexArrays(1, &model.vao);
        glBindVertexArray(model.vao);

        glGenBuffers(1, &model.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(Vertex), model.vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(2);

        for (auto& subMesh : model.meshes)
        {
            glGenBuffers(1, &subMesh.indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, subMesh.indices.size() * sizeof(unsigned int), subMesh.indices.data(), GL_STATIC_DRAW);
        }

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
            model.meshes.push_back(std::move(subMesh));
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

    bool OpenGLRenderer::destroyModelInstance(IModelInstance& modelInstance)
    {
        return true;
    }

    ////////////////////////////////////////////////////////////////////////

    bool OpenGLRenderer::unloadTexture(const std::string& filename)
    {
        const auto& itr = m_textures.find(filename);
        if (itr == m_textures.end())
        {
            return true;
        }

        GLuint& texture = itr->second;
        if (texture)
        {
            glDeleteTextures(1, &texture);
        }

        m_textures.erase(itr);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////

    bool OpenGLRenderer::unloadModel(const std::string& filename)
    {
        const auto& itr = m_models.find(filename);
        if (itr == m_models.end())
        {
            return true;
        }

        ModelData& model = itr->second;
        for (SubMesh& subMesh : model.meshes)
        {
            if (subMesh.indexBuffer)
            {
                glDeleteBuffers(1, &subMesh.indexBuffer);
            }
        }
        if (model.vertexBuffer)
        {
            glDeleteBuffers(1, &model.vertexBuffer);
        }

        if (model.vao)
        {
            glDeleteVertexArrays(1, &model.vao);
        }

        m_models.erase(itr);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::cleanUp()
    {
        for (const std::string& modelId : Utils::getKeys(m_models))
        {
            unloadModel(modelId);
        }

        for (const std::string& textureId : Utils::getKeys(m_textures))
        {
            unloadTexture(textureId);
        }

        if (m_shaderProgram)
        {
            glDeleteProgram(m_shaderProgram);
            m_shaderProgram = 0;
        }

        if (m_frameBufferTexture) 
        {
            glDeleteTextures(1, &m_frameBufferTexture);
            m_frameBufferTexture = 0;
        }

        if (m_frameBuffer) 
        {
            glDeleteFramebuffers(1, &m_frameBuffer);
            m_frameBuffer = 0;
        }

        if (m_hglrc)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(m_hglrc);
            m_hglrc = 0;
        }

        if (m_hdc)
        {
            ReleaseDC(m_hwnd, m_hdc);
            m_hdc = 0;
        }


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

    void OpenGLRenderer::setPixelFormat()
    {
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
        ASSERT(pixelFormat != 0, "Can't choose pixel format");
        if (pixelFormat == 0)
        {
            return;
        }

        bool setPixelFormatResult = SetPixelFormat(m_hdc, pixelFormat, &pfd);
        ASSERT(setPixelFormatResult, "Can't set pixel format: {}", pixelFormat);
        if (!setPixelFormatResult)
        {
            return;
        }
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createWglContext()
    {

        HGLRC tempContext = wglCreateContext(m_hdc);
        ASSERT(tempContext, "Failed to create an OpenGL rendering context");
        if (!tempContext)
        {
            return;
        }

        bool wglMakeCurrentResult = wglMakeCurrent(m_hdc, tempContext);
        ASSERT(wglMakeCurrentResult, "Failed to activate the OpenGL rendering context");
        if (!wglMakeCurrentResult)
        {
            return;
        }

        glewExperimental = GL_TRUE;
        GLenum glewInitResult = glewInit();
        ASSERT(glewInitResult == GLEW_OK, "Failed to initialize GLEW, error code: {}", glewInitResult);
        if (glewInitResult != GLEW_OK)
        {
            return;
        }

        // Create a modern OpenGL context
        typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
        wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        if (wglCreateContextAttribsARB) 
        {
            int attribs[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                WGL_CONTEXT_MINOR_VERSION_ARB, 5,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            HGLRC hglrcNew = wglCreateContextAttribsARB(m_hdc, nullptr, attribs);
            ASSERT(hglrcNew, "Failed to create wgl context atrributes");
            if (hglrcNew) 
            {
                wglMakeCurrent(nullptr, nullptr);
                wglDeleteContext(tempContext);
                tempContext = hglrcNew;

                bool wglMakeCurrentResult = wglMakeCurrent(m_hdc, tempContext);
                ASSERT(wglMakeCurrentResult, "Failed to activate the modern OpenGL rendering context");
            }
        }

        // Store the HGLRC for later use
        m_hglrc = tempContext;
        
        auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        ASSERT(wglSwapIntervalEXT, "Failed to get wglSwapIntervalEXT for disabling VSync");
        if (wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(0);
        }
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::setInitialOpenGLState()
    {
        glClearColor(0.0f,0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createFrameBuffer()
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        glGenFramebuffers(1, &m_frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

        glGenTextures(1, &m_frameBufferTexture);
        glBindTexture(GL_TEXTURE_2D, m_frameBufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_frameBufferTexture, 0);

        ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Frame buffer is not complete");

        glEnable(GL_FRAMEBUFFER_SRGB);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createViewport()
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        glViewport(0, 0, width, height);
        float aspectRatio = (float)(width) / (float)(height);

        m_projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
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
            ASSERT(success, "Shader compilation error: {}", infoLog);
            throw std::runtime_error("Shader compilation error");
        }
        return shader;
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createShaderProgram(const std::string& vsSource, const std::string& fsSource)
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
            ASSERT(success, "Shader program linking error: {}", infoLog);
            throw std::runtime_error("Shader program linking failed.");
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        m_shaderProgram = program;
    }

    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createShaderFields()
    {
        glUseProgram(m_shaderProgram);
        m_viewMatrixLoc = glGetUniformLocation(m_shaderProgram, "viewMatrix");
        m_projectionMatrixLoc = glGetUniformLocation(m_shaderProgram, "projectionMatrix");
        m_modelMatrixLoc = glGetUniformLocation(m_shaderProgram, "modelMatrix");
    }


    ////////////////////////////////////////////////////////////////////////

    void OpenGLRenderer::createDefaultMaterial()
    {
        bool loadTextureRes = loadTexture(DEFAULT_TEXTURE);
        ASSERT(loadTextureRes, "Can't load default texture: {}", DEFAULT_TEXTURE);

        m_defaultMaterial.diffuseTextureId = DEFAULT_TEXTURE;
        m_defaultMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
        m_defaultMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
        m_defaultMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
        m_defaultMaterial.shininess = 32.0f;
    }

    ////////////////////////////////////////////////////////////////////////
}
