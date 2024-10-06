#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    // Transform vertex position to world space using model matrix
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    
    // Pass normal transformed by model matrix (no translation)
    Normal = mat3(transpose(inverse(modelMatrix))) * aNormal;

    // Pass texture coordinate
    TexCoord = aTexCoord;

    // Apply model, view, and projection transformations
    gl_Position = projectionMatrix * viewMatrix * vec4(FragPos, 1.0);
}
