// Basic PBR-lite shader
// Vertex shader + Pixel shader in one file

cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float3 lightDir;
    float _pad0;
    float3 cameraPos;
    float _pad1;
    float3 baseColor;
    float metallic;
    float roughness;
    float3 _pad2;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.worldPos = mul(world, float4(input.position, 1.0)).xyz;
    output.normal = mul((float3x3)world, input.normal);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir);
    float3 V = normalize(cameraPos - input.worldPos);
    float3 H = normalize(L + V);
    
    // Simple Blinn-Phong with PBR-like parameters
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    
    // Diffuse
    float3 diffuse = input.color * baseColor * NdotL;
    
    // Specular (roughness controls shininess)
    float shininess = (1.0 - roughness) * 128.0 + 1.0;
    float spec = pow(NdotH, shininess);
    float3 specular = float3(1, 1, 1) * spec * (1.0 - roughness) * metallic;
    
    // Ambient
    float3 ambient = input.color * baseColor * 0.15;
    
    float3 finalColor = ambient + diffuse + specular;
    
    return float4(finalColor, 1.0);
}
