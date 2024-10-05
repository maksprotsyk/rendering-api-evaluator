cbuffer ConstantBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    float3 ambientColor;
    float3 diffuseColor;
    float3 specularColor;
    float shininess;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    // Sample the diffuse texture
    float4 texColor = texture0.Sample(sampler0, input.texCoord);

    // Basic ambient, diffuse, and specular lighting
    float3 lightDir = normalize(float3(0.0f, 0.0f, -1.0f)); // Light direction
    float3 normal = normalize(input.normal);
    
    // Ambient component
    float3 ambient = ambientColor * texColor.rgb;

    // Diffuse component
    float diffuseFactor = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = diffuseFactor * diffuseColor * texColor.rgb;

    // Specular component (using Blinn-Phong model)
    float3 viewDir = normalize(-input.position.xyz); // Assume the camera is at (0,0,0)
    float3 halfwayDir = normalize(lightDir + viewDir);
    float specFactor = pow(max(dot(normal, halfwayDir), 0.0f), shininess);
    float3 specular = specFactor * specularColor;

    // Combine all components
    float3 finalColor = ambient + diffuse + specular;
    
    return float4(finalColor, texColor.a);
}
