// LUMA Wireframe Shader (HLSL)
// Unlit wireframe rendering - direct color output, no lighting

cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4x4 lightViewProj;
    float4 lightDirAndFlags;
    float4 cameraPosAndMetal;
    float4 baseColorAndRough;  // xyz = wireframe color
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
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Direct output of baseColor - no lighting calculation!
    return float4(baseColorAndRough.xyz, 1.0);
}
