#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform MaterialData {
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
} material;

layout(set = 0, binding = 2) uniform sampler2D diffuseTexture;

void main() {
    vec3 lightDir = normalize(vec3(1.0, -1.0, -1.0));
    vec3 normal = normalize(fragNormal);

    // Ambient color
    vec3 ambient = material.ambientColor;

    // Diffuse color
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * material.diffuseColor * texture(diffuseTexture, fragTexCoord).rgb;

    // Specular color (simple approximation)
    vec3 viewDir = normalize(-vec3(0.0, 0.0, 1.0));
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * material.specularColor;

    vec3 color = ambient + diffuse + specular;
    outColor = vec4(color, 1.0);
}
