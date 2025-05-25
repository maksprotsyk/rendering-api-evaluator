#version 450 core

uniform mat4 worldMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 lightDirection;
uniform float lightIntensity;

uniform vec3 ambientColor;
uniform float shininess;

uniform vec3 diffuseColor;
uniform vec3 specularColor;

uniform sampler2D texture0;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 fragColor;

const float ambientIntensity = 0.1;

void main()
{
    vec4 texColor = texture(texture0, TexCoord);

    vec3 normal = normalize(Normal);

    vec3 ambient = ambientIntensity * ambientColor * texColor.rgb;

    float diffuseFactor = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diffuseFactor * lightIntensity * diffuseColor * texColor.rgb;

    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float specFactor = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specFactor * lightIntensity * specularColor;

    vec3 finalColor = ambient + diffuse + specular;
    finalColor = clamp(finalColor, 0.0, 1.0);

    fragColor = vec4(finalColor, texColor.a);
}
