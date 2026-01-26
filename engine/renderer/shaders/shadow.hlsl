// LUMA Shadow Pass Shader (HLSL)
// Depth-only rendering for shadow mapping

cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4x4 lightViewProj;
    float4 unused1;
    float4 unused2;
    float4 unused3;
    float4 unused4;
    float4 unused5;
};

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
    float4 worldPos = mul(world, float4(input.position, 1.0));
    output.position = mul(lightViewProj, worldPos);
    return output;
}

// No pixel shader needed - depth only pass
