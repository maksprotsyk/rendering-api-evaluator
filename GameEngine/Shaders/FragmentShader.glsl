#version 450 core

// Uniforms
uniform mat4 worldMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform vec3 ambientColor;
uniform float shininess;

uniform vec3 diffuseColor;
uniform vec3 specularColor;

// Sampler for texture
uniform sampler2D texture0;

// Inputs from the vertex shader
in vec3 FragPos;    // Position in world space from vertex shader
in vec3 Normal;     // Interpolated normal from vertex shader
in vec2 TexCoord;   // Texture coordinates from vertex shader

// Output color
out vec4 fragColor;

void main()
{
    // Sample the diffuse texture
    vec4 texColor = texture(texture0, TexCoord);

    // Light direction (assuming a directional light)
    vec3 lightDir = normalize(vec3(0.0, 0.0, -1.0)); // Direction towards the light

    // Normalized normal vector from the fragment
    vec3 normal = normalize(Normal);

    // Ambient component
    vec3 ambient = ambientColor * texColor.rgb;

    // Diffuse component
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseFactor * diffuseColor * texColor.rgb;

    // Specular component (using Blinn-Phong model)
    vec3 viewDir = normalize(-FragPos); // Assume the camera is at (0, 0, 0)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specFactor = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specFactor * specularColor;

    // Combine all components
    vec3 finalColor = ambient + diffuse + specular;
    finalColor = clamp(finalColor, 0.0, 1.0);

    fragColor = vec4(finalColor, texColor.a);
}
