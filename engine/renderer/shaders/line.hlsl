// LUMA Line Shader (HLSL)
// Simple line rendering with vertex colors

cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4x4 lightViewProj;
    float4 unused1;
    float4 unused2;
    float4 unused3;
    float4 unused4;
};

struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.color;
}
