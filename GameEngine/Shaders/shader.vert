#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 worldMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 lightDirection;
    float lightIntensity;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoord;

void main() {
    vec4 worldPosition = ubo.worldMatrix * vec4(inPosition, 1.0);
    FragPos = worldPosition.xyz;
    Normal = mat3(ubo.worldMatrix) * inNormal;
    TexCoord = inTexCoord;
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(FragPos, 1.0);
}
