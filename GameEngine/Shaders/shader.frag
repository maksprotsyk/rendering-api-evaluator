#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 FragPos;      // World-space position of the fragment
layout(location = 1) in vec3 Normal;       // World-space normal vector of the fragment
layout(location = 2) in vec2 TexCoord;     // Texture coordinates

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

// Uniform block for non-opaque uniforms
layout(binding = 2) uniform LightingUniforms {
    vec3 lightPos;       // Light position in world space
    vec3 viewPos;        // View (camera) position in world space
    vec3 lightColor;     // Light color
    vec3 ambientColor;   // Ambient light color
} lighting;

void main() {
    // Sample the texture color
    vec4 texColor = texture(texSampler, TexCoord);

    // Calculate ambient lighting
    vec3 ambient = lighting.ambientColor * texColor.rgb;

    // Calculate diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lighting.lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    outColor = texColor;
}