#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 worldMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    vec3 lightDirection;
    float lightIntensity;
} ubo;

layout (set = 1, binding = 0) uniform MaterialBufferObject {
    vec3 ambientColor;
    float shininess;

    vec3 diffuseColor;
    float useDiffuseTexture;

    vec3 specularColor;
    float padding2;
} mbo;


layout(set = 2, binding = 0) uniform sampler2D texture0;

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal; 
layout(location = 2) in vec2 TexCoord;  

layout(location = 0) out vec4 fragColor;

const float ambientIntensity = 0.1;

void main()
{
    vec4 texColor = texture(texture0, TexCoord);
    vec3 baseDiffuseColor = mix(mbo.diffuseColor, texColor.rgb, mbo.useDiffuseTexture);
    vec3 normal = normalize(Normal);

    vec3 ambient = ambientIntensity * mbo.ambientColor * baseDiffuseColor;

    float diffuseFactor = max(dot(normal, ubo.lightDirection), 0.0);
    vec3 diffuse = diffuseFactor * ubo.lightIntensity * mbo.diffuseColor * baseDiffuseColor;

    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(ubo.lightDirection + viewDir);
    float specFactor = pow(max(dot(normal, halfwayDir), 0.0), mbo.shininess);
    vec3 specular = specFactor * ubo.lightIntensity * mbo.specularColor;

    vec3 finalColor = ambient + diffuse + specular;

    finalColor = clamp(finalColor, 0.0, 1.0);

    fragColor = vec4(finalColor, texColor.a);
    
}
