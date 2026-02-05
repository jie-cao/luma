// LUMA Solid Mode Shader (HLSL)
// Simple diffuse lighting for clay/solid rendering, no textures

cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4x4 lightViewProj;
    float4 lightDirAndFlags;
    float4 cameraPosAndMetal;
    float4 baseColorAndRough;  // xyz = solid color
    float4 shadowParams;
    float4 iblParams;
};

// Declare all textures from root signature (required for compatibility)
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);
Texture2D shadowMap : register(t3);
TextureCube irradianceMap : register(t4);
TextureCube prefilteredMap : register(t5);
Texture2D brdfLUT : register(t6);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldNormal : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.worldNormal = normalize(mul((float3x3)world, input.normal));
    output.worldPos = mul(world, float4(input.position, 1.0)).xyz;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Simple diffuse lighting
    float3 N = normalize(input.worldNormal);
    float3 L = normalize(-lightDirAndFlags.xyz);  // Light direction
    float3 V = normalize(cameraPosAndMetal.xyz - input.worldPos);  // View direction
    
    // Diffuse (Lambert)
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = baseColorAndRough.xyz * NdotL * 0.8;
    
    // Simple specular (Blinn-Phong)
    float3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 32.0) * 0.3;
    
    // Ambient
    float3 ambient = baseColorAndRough.xyz * 0.25;
    
    // Hemisphere ambient (subtle)
    float3 skyColor = float3(0.5, 0.6, 0.8);
    float3 groundColor = float3(0.3, 0.25, 0.2);
    float3 hemiAmbient = lerp(groundColor, skyColor, N.y * 0.5 + 0.5) * baseColorAndRough.xyz * 0.1;
    
    float3 finalColor = ambient + hemiAmbient + diffuse + float3(spec, spec, spec);
    
    return float4(finalColor, 1.0);
}
