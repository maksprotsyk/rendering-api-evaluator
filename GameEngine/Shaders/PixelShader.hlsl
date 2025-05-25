cbuffer ConstantBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer MaterialBuffer : register(b1)
{
    float3 ambientColor;
    float shininess;
    
    float3 diffuseColor;
    float padding1;
    
    float3 specularColor;
    float padding2;
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
    float4 texColor = texture0.Sample(sampler0, input.texCoord);

    float3 lightDir = normalize(float3(0.0f, 0.0f, -1.0f));
    float3 normal = normalize(input.normal);
    
    float3 ambient = ambientColor * texColor.rgb;

    float diffuseFactor = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = diffuseFactor * diffuseColor * texColor.rgb;

    float3 viewDir = normalize(-input.position.xyz);
    float3 halfwayDir = normalize(lightDir + viewDir);
    float specFactor = pow(max(dot(normal, halfwayDir), 0.0f), shininess);

    float3 specular = specFactor * specularColor;

    float3 finalColor = ambient + diffuse + specular;
    finalColor = clamp(finalColor, 0.0f, 1.0f);
    return float4(finalColor, texColor.a);
}
