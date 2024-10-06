#define GLM_FORCE_LEFT_HANDED

#include "OpenGLRenderer.h"
#include <stdexcept>
#include <iostream>
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include <GL/wglext.h>
#include <fstream>

namespace Engine::Visual
{
    static void checkError()
    {

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << error << std::endl;
        }
    }

    void OpenGLRenderer::init(const Window& window)
    {
        HWND hwnd = window.getHandle();
        hdc = GetDC(hwnd); // Get the device context from the HWND

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

        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        if (pixelFormat == 0) {
            throw std::runtime_error("Failed to choose a suitable pixel format");
        }

        if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
            throw std::runtime_error("Failed to set the pixel format");
        }

        // Create a legacy OpenGL rendering context
        HGLRC tempContext = wglCreateContext(hdc);
        if (!tempContext) {
            throw std::runtime_error("Failed to create an OpenGL rendering context");
        }

        if (!wglMakeCurrent(hdc, tempContext)) {
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

            HGLRC hglrcNew = wglCreateContextAttribsARB(hdc, nullptr, attribs);
            if (hglrcNew) {
                wglMakeCurrent(nullptr, nullptr); // Unset the old context
                wglDeleteContext(tempContext);
                tempContext = hglrcNew;

                if (!wglMakeCurrent(hdc, tempContext)) {
                    throw std::runtime_error("Failed to activate the modern OpenGL rendering context");
                }
            }
        }

        // Store the HGLRC for later use
        hglrc = tempContext;

        // Set up the initial OpenGL state
        glClearColor(0.0f, 0.2f, 0.4f, 1.0f); // Set the clear color
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);

        // Create and compile shaders (using GLSL files)
        shaderProgram = createShaderProgram("VertexShader.glsl", "FragmentShader.glsl");
        glUseProgram(shaderProgram);

        // Get uniform locations for matrices
        viewMatrixLoc = glGetUniformLocation(shaderProgram, "viewMatrix");
        projectionMatrixLoc = glGetUniformLocation(shaderProgram, "projectionMatrix");
        modelMatrixLoc = glGetUniformLocation(shaderProgram, "modelMatrix");

        loadTexture(defaultMaterial.diffuseTexture, "../../Resources/cube/default.png");
        defaultMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        defaultMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
        defaultMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        defaultMaterial.shininess = 0.0f;

        RECT rect;
        GetClientRect(window.getHandle(), &rect);
        glViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
        aspectRatio = (float)(rect.right - rect.left) / (float)(rect.bottom - rect.top);
    }

    void OpenGLRenderer::clearBackground()
    {
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLRenderer::draw(const AbstractModel& abstractModel)
    {
        const auto& model = (const Model&)abstractModel;

        // Use the shader program
        glUseProgram(shaderProgram);
        checkError();

        // Bind the VAO
        glBindVertexArray(model.vao);
        checkError();

        // Set view and projection matrices
        glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(model.worldMatrix));
        checkError();

        // Iterate through the meshes in the model and render each
        for (const auto& mesh : model.meshes)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer);
            checkError();

            // Set the material properties in the shader
            const Material& material = mesh.materialId != -1 ? model.materials[mesh.materialId] : defaultMaterial;
            glUniform3fv(glGetUniformLocation(shaderProgram, "ambientColor"), 1, glm::value_ptr(material.ambientColor));
            glUniform3fv(glGetUniformLocation(shaderProgram, "diffuseColor"), 1, glm::value_ptr(material.diffuseColor));
            glUniform3fv(glGetUniformLocation(shaderProgram, "specularColor"), 1, glm::value_ptr(material.specularColor));
            glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), material.shininess);
            checkError();

            // Bind texture
            if (material.diffuseTexture)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, material.diffuseTexture);
                glUniform1i(glGetUniformLocation(shaderProgram, "diffuseTexture"), 0);
                checkError();
            }

            // Draw the mesh
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
            checkError();
        }

        // Unbind the VAO
        glBindVertexArray(0);
    }


    void OpenGLRenderer::render()
    {
        // Swap buffers to display the rendered frame
        SwapBuffers(hdc);

        checkError();
    }

    std::unique_ptr<IRenderer::AbstractModel> OpenGLRenderer::createModel()
    {
        return std::make_unique<Model>();
    }

    void OpenGLRenderer::createBuffersFromModel(AbstractModel& abstractModel)
    {
        auto& model = (Model&)abstractModel;

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

    void OpenGLRenderer::loadModel(AbstractModel& abstractModel, const std::string& filename)
    {
        auto& model = (Model&)abstractModel;

        std::filesystem::path fullPath(filename);
        std::filesystem::path matDir = fullPath.parent_path();

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), matDir.string().c_str());
        if (!success) {
            throw std::runtime_error("Failed to load model: " + warn + err);
        }

        model.vertices.clear();
        model.meshes.clear();
        model.materials.clear();

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
                loadTexture(material.diffuseTexture, texturePath);
            }
            else
            {
                material.diffuseTexture = 0;
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
    }

    void OpenGLRenderer::setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation)
    {
        Utils::Vector3 forward(0.0f, 0.0f, 1.0f);
        forward.rotateArroundVector(Utils::Vector3(1.0f, 0.0f, 0.0f), rotation.x);
        forward.rotateArroundVector(Utils::Vector3(0.0f, 1.0f, 0.0f), rotation.y);

        Utils::Vector3 up = Utils::Vector3::crossProduct(forward, Utils::Vector3(1.0f, 0.0f, 0.0f));

        glm::vec3 pos(position.x, position.y, position.z);

        viewMatrix = glm::lookAt(
            pos,
            pos + glm::vec3(forward.x, forward.y, forward.z),
            glm::vec3(up.x, up.y, up.z)
        );

        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    }

    void OpenGLRenderer::transformModel(AbstractModel& abstractModel, const Utils::Vector3& position, const Utils::Vector3& rotation, const Utils::Vector3& scale)
    {
        auto& model = (Model&)abstractModel;

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, position.z));
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));

        model.worldMatrix = translation * rotationX * rotationY * rotationZ * scaling;
    }

    void OpenGLRenderer::loadTexture(GLuint& texture, const std::string& filename)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + filename);
        }

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }

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
}
