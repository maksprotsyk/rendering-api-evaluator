#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 worldMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 FragPos;   // Output fragment position in world space
layout(location = 1) out vec3 Normal;    // Output normal in world space
layout(location = 2) out vec2 TexCoord;  // Output texture coordinates

void main() {
    // Calculate the world-space position of the fragment
    vec4 worldPosition = ubo.worldMatrix * vec4(inPosition, 1.0);
    FragPos = worldPosition.xyz;  // Pass world-space position to fragment shader

    // Pass the transformed normal to the fragment shader
    Normal = mat3(transpose(inverse(ubo.worldMatrix))) * inNormal;  // Transform normal by world matrix

    // Pass texture coordinates directly
    TexCoord = inTexCoord;

    // Calculate final vertex position in clip space
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;
}
