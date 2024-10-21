#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 worldMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} ubo;

layout (binding = 1) uniform MaterialBufferObject {
    vec3 ambientColor; // 12 bytes
    float shininess;   // 4 bytes to align to 16 bytes

    vec3 diffuseColor; // 12 bytes
    float padding1;    // 4 bytes of padding

    vec3 specularColor; // 12 bytes
    float padding2;     // 4 bytes of padding
} mbo;


layout(binding = 2) uniform sampler2D texture0;

// Inputs from vertex shader
layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal; 
layout(location = 2) in vec2 TexCoord;  

// Output color
layout(location = 0) out vec4 fragColor;

void main()
{
    // Sample the diffuse texture
    vec4 texColor = texture(texture0, TexCoord);

    // Light direction (assuming a directional light)
    vec3 lightDir = normalize(vec3(0.0, 0.0, -1.0)); // Direction towards the light

    // Normalized normal vector from the fragment
    vec3 normal = normalize(Normal);

    // Ambient component
    vec3 ambient = mbo.ambientColor * texColor.rgb;

    // Diffuse component
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseFactor * mbo.diffuseColor * texColor.rgb;

    // Specular component (using Blinn-Phong model)
    vec3 viewDir = normalize(-FragPos); // Assume the camera is at (0, 0, 0)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specFactor = pow(max(dot(normal, halfwayDir), 0.0), mbo.shininess);
    vec3 specular = specFactor * mbo.specularColor;

    // Combine all components
    vec3 finalColor = ambient + diffuse + specular;

    finalColor = clamp(finalColor, 0.0, 1.0);

    fragColor = vec4(finalColor, texColor.a);
    
}
