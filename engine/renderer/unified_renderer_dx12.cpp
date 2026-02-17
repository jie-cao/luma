// Unified Renderer Implementation (DX12)
// Complete PBR rendering with Cook-Torrance BRDF

#if defined(_WIN32)

#include "unified_renderer.h"
#include "engine/mesh/edit_mesh.h"
#include "engine/asset/model_loader.h"
#include "engine/asset/async_texture_loader.h"
#include "engine/asset/hdr_loader.h"
#include "engine/renderer/ibl_generator.h"
#include "engine/util/file_watcher.h"
#include <fstream>
#include <sstream>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <queue>
#include <set>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace luma {

// ===== PBR Shader (Complete Cook-Torrance BRDF) =====
static const char* kPBRShaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4x4 lightViewProj;
    float4 lightDirAndFlags;
    float4 cameraPosAndMetal;
    float4 baseColorAndRough;
    float4 shadowParams;  // x = bias, y = normalBias, z = softness, w = enabled
    float4 iblParams;     // x = intensity, y = rotation, z = maxMipLevel, w = enabled
};

#define lightDir lightDirAndFlags.xyz
#define cameraPos cameraPosAndMetal.xyz
#define metallic cameraPosAndMetal.w
#define baseColor baseColorAndRough.xyz
#define roughness baseColorAndRough.w
#define shadowBias shadowParams.x
#define shadowNormalBias shadowParams.y
#define shadowSoftness shadowParams.z
#define shadowEnabled shadowParams.w
#define iblIntensity iblParams.x
#define iblRotation iblParams.y
#define iblMaxMip iblParams.z
#define iblEnabled iblParams.w

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);
Texture2D shadowMap : register(t3);
TextureCube irradianceMap : register(t4);
TextureCube prefilteredMap : register(t5);
Texture2D brdfLUT : register(t6);
SamplerState texSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

static const float PI = 3.14159265359;

// Rotate direction around Y axis
float3 rotateY(float3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return float3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

// Fresnel-Schlick with roughness (for IBL)
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float r) {
    return F0 + (max(float3(1.0 - r, 1.0 - r, 1.0 - r), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 uv : TEXCOORD4;
    float3 color : COLOR;
    float4 shadowCoord : TEXCOORD5;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    float4 worldPos = mul(world, float4(input.position, 1.0));
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)world, input.normal));
    output.tangent = normalize(mul((float3x3)world, input.tangent.xyz));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    output.uv = input.uv;
    output.color = input.color;
    
    // Calculate shadow coordinates
    output.shadowCoord = mul(lightViewProj, worldPos);
    
    return output;
}

// PCF Shadow Sampling
float sampleShadowPCF(float3 shadowCoord, float3 normal, float3 lDir) {
    if (shadowEnabled < 0.5) return 1.0;
    
    // Check bounds - if outside shadow map, no shadow
    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 || 
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        shadowCoord.z < 0.0 || shadowCoord.z > 1.0) {
        return 1.0;
    }
    
    // Standard bias calculation
    float NdotL = max(dot(normal, -lDir), 0.0);
    float bias = shadowBias + shadowNormalBias * (1.0 - NdotL);
    float depth = shadowCoord.z - bias;
    
    // PCF 3x3
    float shadow = 0.0;
    float2 texelSize = shadowSoftness / 2048.0;
    
    [unroll]
    for (int x = -1; x <= 1; x++) {
        [unroll]
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(shadowSampler, shadowCoord.xy + offset, depth);
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Check for UNLIT mode: if roughness is exactly -1.0, output baseColor directly
    if (roughness < -0.5) {
        return float4(baseColor, 1.0);  // Direct color output - no lighting!
    }
    
    float4 diffuseSample = diffuseTexture.Sample(texSampler, input.uv);
    float4 normalSample = normalTexture.Sample(texSampler, input.uv);
    float4 specularSample = specularTexture.Sample(texSampler, input.uv);
    
    if (diffuseSample.a < 0.1) discard;
    
    // Albedo
    float3 albedo;
    float texBrightness = diffuseSample.r + diffuseSample.g + diffuseSample.b;
    if (texBrightness < 2.9) {
        albedo = diffuseSample.rgb;
    } else {
        albedo = input.color * baseColor;
    }
    
    // Normal Mapping
    float3 N;
    bool hasNormalMap = (abs(normalSample.r - normalSample.g) > 0.01 || abs(normalSample.b - 1.0) > 0.1);
    if (hasNormalMap) {
        float3 normalMap = normalSample.rgb * 2.0 - 1.0;
        float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
        N = normalize(mul(normalMap, TBN));
    } else {
        N = normalize(input.normal);
    }
    
    // PBR Parameters
    float metal = metallic;
    float rough = roughness;
    bool hasSpecMap = (specularSample.r < 0.99 || specularSample.g < 0.99);
    if (hasSpecMap) {
        metal = specularSample.b;
        rough = specularSample.g;
    }
    rough = clamp(rough, 0.04, 1.0);
    
    // Vectors
    float3 V = normalize(cameraPos - input.worldPos);
    float3 L = normalize(-lightDir);
    float3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metal);
    
    // Cook-Torrance BRDF
    float a = rough * rough;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * denom * denom + 0.0001);
    
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    float G1_V = NdotV / (NdotV * (1.0 - k) + k);
    float G1_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G1_V * G1_L;
    
    float3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    
    float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    float3 kD = (1.0 - F) * (1.0 - metal);
    float3 diffuse = kD * albedo / PI;
    
    // Shadow
    float3 shadowCoord = input.shadowCoord.xyz / input.shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;  // Flip Y for DX
    float shadow = sampleShadowPCF(shadowCoord, N, lightDir);
    
    float3 lightColor = float3(1.0, 0.98, 0.95) * 2.5;
    float3 Lo = (diffuse + specular) * NdotL * lightColor * shadow;
    
    // Ambient - use IBL or fallback to simple hemisphere
    float3 ambient;
    if (iblEnabled > 0.5) {
        // Rotate normal for environment rotation
        float3 rotatedN = rotateY(N, iblRotation);
        float3 R = reflect(-V, N);
        float3 rotatedR = rotateY(R, iblRotation);
        
        // IBL Diffuse (Irradiance)
        float3 irradiance = irradianceMap.Sample(texSampler, rotatedN).rgb;
        float3 F_ibl = fresnelSchlickRoughness(NdotV, F0, rough);
        float3 kD_ibl = (1.0 - F_ibl) * (1.0 - metal);
        float3 diffuseIBL = irradiance * albedo * kD_ibl;
        
        // IBL Specular (Prefiltered + BRDF LUT)
        float mipLevel = rough * iblMaxMip;
        float3 prefilteredColor = prefilteredMap.SampleLevel(texSampler, rotatedR, mipLevel).rgb;
        float2 brdf = brdfLUT.Sample(texSampler, float2(NdotV, rough)).rg;
        float3 specularIBL = prefilteredColor * (F_ibl * brdf.x + brdf.y);
        
        ambient = (diffuseIBL + specularIBL) * iblIntensity;
    } else {
        // Fallback: simple hemisphere ambient
        float3 skyColor = float3(0.5, 0.6, 0.8);
        float3 groundColor = float3(0.3, 0.25, 0.2);
        float3 ambientColor = lerp(groundColor, skyColor, N.y * 0.5 + 0.5);
        ambient = albedo * ambientColor * 0.25;
    }
    
    float3 color = ambient + Lo;
    
    // ACES Tone Mapping
    float a_tm = 2.51; float b_tm = 0.03; float c_tm = 2.43; float d_tm = 0.59; float e_tm = 0.14;
    color = saturate((color * (a_tm * color + b_tm)) / (color * (c_tm * color + d_tm) + e_tm));
    
    return float4(color, 1.0);
}
)";

// ===== Shadow Pass Shader (depth-only) =====
static const char* kShadowShaderSource = R"(
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
)";

// ===== Line Shader =====
static const char* kLineShaderSource = R"(
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
)";

// ===== Unlit Wireframe Shader - Direct color output, no lighting =====
// NOTE: Must declare all textures from root signature even if unused, otherwise pipeline creation fails!
static const char* kUnlitWireframeShaderSource = R"(
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
)";

// ===== Solid Mode Shader - Simple diffuse lighting, no textures =====
static const char* kSolidModeShaderSource = R"(
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
)";

// ===== Shader Loading Helper =====
static std::string loadShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";  // Silent fail - will use fallback
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Shader base path for external files
static const char* kShaderBasePath = "engine/renderer/shaders/";

// Load shader with fallback to embedded source
static std::string loadShaderWithFallback(const std::string& name, const char* embeddedSource) {
    std::string filePath = std::string(kShaderBasePath) + name;
    std::string fileSource = loadShaderFile(filePath);
    if (!fileSource.empty()) {
        std::cout << "[shader] Loaded from file: " << filePath << std::endl;
        return fileSource;
    }
    // Fallback to embedded source
    return embeddedSource ? embeddedSource : "";
}

// ===== Math Helpers =====
namespace math {
    inline void identity(float* m) {
        memset(m, 0, 64);
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    
    inline void multiply(float* out, const float* a, const float* b) {
        float tmp[16];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                tmp[i*4+j] = a[i*4+0]*b[0*4+j] + a[i*4+1]*b[1*4+j] + 
                             a[i*4+2]*b[2*4+j] + a[i*4+3]*b[3*4+j];
            }
        }
        memcpy(out, tmp, 64);
    }
    
    inline void lookAt(float* m, const float* eye, const float* at, const float* up) {
        float zAxis[3] = {eye[0]-at[0], eye[1]-at[1], eye[2]-at[2]};
        float len = sqrtf(zAxis[0]*zAxis[0] + zAxis[1]*zAxis[1] + zAxis[2]*zAxis[2]);
        zAxis[0]/=len; zAxis[1]/=len; zAxis[2]/=len;
        float xAxis[3] = {up[1]*zAxis[2]-up[2]*zAxis[1], up[2]*zAxis[0]-up[0]*zAxis[2], 
                          up[0]*zAxis[1]-up[1]*zAxis[0]};
        len = sqrtf(xAxis[0]*xAxis[0] + xAxis[1]*xAxis[1] + xAxis[2]*xAxis[2]);
        xAxis[0]/=len; xAxis[1]/=len; xAxis[2]/=len;
        float yAxis[3] = {zAxis[1]*xAxis[2]-zAxis[2]*xAxis[1], zAxis[2]*xAxis[0]-zAxis[0]*xAxis[2], 
                          zAxis[0]*xAxis[1]-zAxis[1]*xAxis[0]};
        m[0]=xAxis[0]; m[1]=yAxis[0]; m[2]=zAxis[0]; m[3]=0;
        m[4]=xAxis[1]; m[5]=yAxis[1]; m[6]=zAxis[1]; m[7]=0;
        m[8]=xAxis[2]; m[9]=yAxis[2]; m[10]=zAxis[2]; m[11]=0;
        m[12]=-(xAxis[0]*eye[0]+xAxis[1]*eye[1]+xAxis[2]*eye[2]);
        m[13]=-(yAxis[0]*eye[0]+yAxis[1]*eye[1]+yAxis[2]*eye[2]);
        m[14]=-(zAxis[0]*eye[0]+zAxis[1]*eye[1]+zAxis[2]*eye[2]); m[15]=1;
    }
    
    inline void perspective(float* m, float fov, float aspect, float nearZ, float farZ) {
        float tanHalfFov = tanf(fov / 2.0f);
        memset(m, 0, 64);
        m[0] = 1.0f / (aspect * tanHalfFov);
        m[5] = 1.0f / tanHalfFov;
        m[10] = farZ / (nearZ - farZ);
        m[11] = -1.0f;
        m[14] = (nearZ * farZ) / (nearZ - farZ);
    }
    
    inline void ortho(float* m, float left, float right, float bottom, float top, float nearZ, float farZ) {
        memset(m, 0, 64);
        m[0] = 2.0f / (right - left);
        m[5] = 2.0f / (top - bottom);
        m[10] = 1.0f / (nearZ - farZ);
        m[12] = -(right + left) / (right - left);
        m[13] = -(top + bottom) / (top - bottom);
        m[14] = nearZ / (nearZ - farZ);
        m[15] = 1.0f;
    }
    
    inline void scale(float* m, float sx, float sy, float sz) {
        memset(m, 0, 64);
        m[0] = sx; m[5] = sy; m[10] = sz; m[15] = 1.0f;
    }
    
    // 4x4 matrix inversion (general case)
    inline bool invert(float* out, const float* m) {
        float inv[16];
        
        inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + 
                 m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
        inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - 
                 m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
        inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + 
                 m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
        inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - 
                  m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
        
        inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - 
                 m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
        inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + 
                 m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
        inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - 
                 m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
        inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + 
                  m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
        
        inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] + 
                 m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
        inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] - 
                 m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
        inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] + 
                  m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
        inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] - 
                  m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
        
        inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] - 
                 m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
        inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] + 
                 m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
        inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] - 
                  m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
        inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] + 
                  m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];
        
        float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
        if (fabsf(det) < 1e-10f) return false;
        
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) out[i] = inv[i] * det;
        return true;
    }
}

// ===== DX12 GPU Mesh Storage =====
struct DX12MeshData {
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> diffuseTexture;
    ComPtr<ID3D12Resource> normalTexture;
    ComPtr<ID3D12Resource> specularTexture;
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    D3D12_INDEX_BUFFER_VIEW ibv{};
    uint32_t indexCount = 0;
    uint32_t diffuseSrvIndex = 0;
    uint32_t normalSrvIndex = 0;
    uint32_t specularSrvIndex = 0;
    float baseColor[3] = {1, 1, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    
    // GPU-side edge index buffer for original edges (LINE topology)
    // Pre-built during model loading — enables zero-CPU wireframe rendering
    ComPtr<ID3D12Resource> edgeIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW edgeIbv{};
    uint32_t edgeIndexCount = 0;
};

// ===== Renderer Implementation =====
struct UnifiedRenderer::Impl {
    // Core DX12 objects
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12Resource> renderTargets[2];
    ComPtr<ID3D12Resource> depthBuffer;
    ComPtr<ID3D12CommandAllocator> allocators[2];
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12PipelineState> linePipelineState;
    ComPtr<ID3D12PipelineState> gizmoPipelineState;  // Gizmo pipeline with always-visible depth test
    ComPtr<ID3D12PipelineState> gizmoDepthPipelineState;  // Gizmo lines with depth test + bias (for non-xray)
    ComPtr<ID3D12PipelineState> wireframePipelineState;  // Wireframe rendering pipeline
    ComPtr<ID3D12PipelineState> highlightPipelineState;  // Mesh highlight pipeline (always visible, no depth test)
    ComPtr<ID3D12PipelineState> unlitWireframePipelineState;  // Unlit wireframe - solid color, no lighting
    ComPtr<ID3D12PipelineState> overlayWireframePipelineState;  // Overlay wireframe - normal depth test, depth bias
    ComPtr<ID3D12PipelineState> culledWireframePipelineState;  // Wireframe with back-face culling (hidden line removal)
    ComPtr<ID3D12PipelineState> depthOnlyPipelineState;  // Depth pre-pass (no color writes)
    ComPtr<ID3D12PipelineState> edgeLinePipelineState;         // GPU edge rendering: LINE topology, no depth test (X-Ray ON)
    ComPtr<ID3D12PipelineState> edgeLineDepthPipelineState;    // GPU edge rendering: LINE topology, depth test (X-Ray OFF)
    ComPtr<ID3D12PipelineState> solidModePipelineState;  // Solid mode - simple lighting, no textures
    ComPtr<ID3D12Resource> constantBuffer;
    ComPtr<ID3D12Resource> defaultTexture;
    
    // Grid
    ComPtr<ID3D12Resource> gridVertexBuffer;
    ComPtr<ID3D12Resource> axisVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW gridVbv{};
    D3D12_VERTEX_BUFFER_VIEW axisVbv{};
    UINT gridVertexCount = 0;
    UINT axisVertexCount = 0;
    bool gridReady = false;
    
    // Gizmo - persistent vertex buffer to avoid per-frame allocation issues
    ComPtr<ID3D12Resource> gizmoVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW gizmoVbv{};
    static constexpr UINT kMaxGizmoVertices = 65536;  // Large buffer for thick rotation circles and mesh wireframes
    void* gizmoVbMapped = nullptr;
    
    // Edit mode wireframe - separate buffer from gizmo to avoid conflicts
    ComPtr<ID3D12Resource> editWireframeBuffer;
    D3D12_VERTEX_BUFFER_VIEW editWireframeVbv{};
    void* editWireframeMapped = nullptr;
    
    // Sync
    HANDLE fenceEvent = nullptr;
    UINT64 fenceValue = 1;
    UINT frameIndex = 0;
    UINT rtvDescSize = 0;
    UINT srvDescSize = 0;
    UINT defaultTextureSrvIndex = 0;
    UINT nextSrvIndex = 1;
    
    // Constants - Ring buffer for per-draw constants
    static constexpr UINT kMaxDrawsPerFrame = 256;
    static constexpr UINT kConstantBufferSize = sizeof(RHISceneConstants);
    static constexpr UINT kAlignedConstantSize = (kConstantBufferSize + 255) & ~255;
    RHISceneConstants constants{};
    uint8_t* constantBufferMapped = nullptr;
    UINT currentDrawIndex = 0;
    
    // Mesh Storage
    std::vector<DX12MeshData> meshStorage;
    
    // Build GPU edge index buffer for a mesh's original edges
    void buildEdgeIndexBuffer(DX12MeshData& dx12Mesh, const std::vector<OriginalEdge>& edges) {
        if (edges.empty()) return;
        std::vector<uint32_t> indices;
        indices.reserve(edges.size() * 2);
        for (const auto& e : edges) {
            indices.push_back(e.v0);
            indices.push_back(e.v1);
        }
        const UINT size = static_cast<UINT>(indices.size() * sizeof(uint32_t));
        D3D12_HEAP_PROPERTIES hp{}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bd{}; bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bd.Width = size; bd.Height = 1; bd.DepthOrArraySize = 1;
        bd.MipLevels = 1; bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        HRESULT hr = device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.edgeIndexBuffer));
        if (FAILED(hr)) return;
        void* mapped;
        dx12Mesh.edgeIndexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, indices.data(), size);
        dx12Mesh.edgeIndexBuffer->Unmap(0, nullptr);
        dx12Mesh.edgeIbv.BufferLocation = dx12Mesh.edgeIndexBuffer->GetGPUVirtualAddress();
        dx12Mesh.edgeIbv.SizeInBytes = size;
        dx12Mesh.edgeIbv.Format = DXGI_FORMAT_R32_UINT;
        dx12Mesh.edgeIndexCount = static_cast<uint32_t>(indices.size());
    }
    
    // Async Texture Loading
    // Maps async request ID to (meshIndex, textureSlot: 0=diffuse, 1=normal, 2=specular)
    std::unordered_map<uint32_t, std::pair<uint32_t, int>> pendingTextures;
    size_t asyncTexturesLoaded = 0;
    
    // Progressive texture upload queue (for smooth loading)
    struct TextureUploadJob {
        uint32_t meshIndex;
        int slot;  // 0=diffuse, 1=normal, 2=specular
        TextureData data;
    };
    std::queue<TextureUploadJob> textureUploadQueue;
    size_t totalTexturesQueued = 0;
    
    // Scene Graph Camera State
    float viewMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float projMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float cameraPos[3] = {0, 0, 0};
    bool cameraSet = false;
    
    // Shadow Mapping
    ShadowSettings shadowSettings;
    ComPtr<ID3D12Resource> shadowMap;
    ComPtr<ID3D12DescriptorHeap> shadowDsvHeap;
    ComPtr<ID3D12PipelineState> shadowPipelineState;
    UINT shadowMapSrvIndex = 0;
    float lightViewProj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool shadowMapReady = false;
    bool inShadowPass = false;
    
    // IBL (Image-Based Lighting)
    IBLSettings iblSettings;
    ComPtr<ID3D12Resource> irradianceMap;
    ComPtr<ID3D12Resource> prefilteredMap;
    ComPtr<ID3D12Resource> brdfLUT;
    UINT irradianceSrvIndex = 0;
    UINT prefilteredSrvIndex = 0;
    UINT brdfLutSrvIndex = 0;
    bool iblReady = false;
    
    // Skinned Rendering
    ComPtr<ID3D12RootSignature> skinnedRootSignature;
    ComPtr<ID3D12PipelineState> skinnedPipelineState;
    ComPtr<ID3D12Resource> boneBuffer;
    uint8_t* boneBufferMapped = nullptr;
    static constexpr UINT kMaxBones = 128;
    static constexpr UINT kBoneBufferSize = kMaxBones * 64;  // 128 * sizeof(float4x4)
    bool skinnedPipelineReady = false;
    
    // Shader Hot-Reload
    FileWatcher shaderWatcher;
    bool shaderHotReloadEnabled = false;
    bool shaderReloadPending = false;
    std::string shaderError;
    std::string materialShaderError;
    std::string shaderBasePath = "engine/renderer/shaders/";
    
    // Dynamic material shader PSOs
    std::unordered_map<void*, ComPtr<ID3D12PipelineState>> materialPSOs;
    
    // Post-Processing
    bool postProcessEnabled = true;  // Now safe with finishSceneRendering() architecture
    float frameTime = 0.0f;
    ComPtr<ID3D12Resource> hdrRenderTarget;
    ComPtr<ID3D12Resource> bloomTextures[2];  // Ping-pong buffers for blur
    ComPtr<ID3D12DescriptorHeap> postProcessRtvHeap;
    ComPtr<ID3D12DescriptorHeap> postProcessSrvHeap;
    ComPtr<ID3D12RootSignature> postProcessRootSignature;
    ComPtr<ID3D12PipelineState> postProcessPSO;
    ComPtr<ID3D12PipelineState> bloomThresholdPSO;
    ComPtr<ID3D12PipelineState> bloomBlurHPSO;
    ComPtr<ID3D12PipelineState> bloomBlurVPSO;
    ComPtr<ID3D12Resource> postProcessConstantBuffer;
    uint8_t* postProcessConstantsMapped = nullptr;
    UINT hdrRtvIndex = 0;
    UINT bloomRtvIndex[2] = {0, 0};
    UINT hdrSrvIndex = 0;
    UINT bloomSrvIndex[2] = {0, 0};
    bool postProcessReady = false;
    
    // Post-process parameters
    struct PostProcessParams {
        float bloomThreshold = 1.0f;
        float bloomIntensity = 1.0f;
        float exposure = 1.0f;
        float gamma = 2.2f;
        bool bloomEnabled = true;
        bool toneMappingEnabled = true;
    } ppParams;
    
    // State
    uint32_t width = 0;
    uint32_t height = 0;
    bool ready = false;
    
    void waitForGPU() {
        queue->Signal(fence.Get(), fenceValue);
        fence->SetEventOnCompletion(fenceValue++, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    
    void createDepthBuffer() {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC depthDesc{};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = width; depthDesc.Height = height; depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1; depthDesc.Format = DXGI_FORMAT_D32_FLOAT; depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE clearVal{}; clearVal.Format = DXGI_FORMAT_D32_FLOAT; clearVal.DepthStencil.Depth = 1.0f;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, 
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearVal, IID_PPV_ARGS(&depthBuffer));
        
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{}; dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; 
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
    
    void createShadowMap() {
        uint32_t size = shadowSettings.mapSize;
        
        // Create shadow DSV heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&shadowDsvHeap));
        
        // Create shadow map texture (depth only, with SRV for sampling)
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC shadowDesc{};
        shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        shadowDesc.Width = size; shadowDesc.Height = size; shadowDesc.DepthOrArraySize = 1;
        shadowDesc.MipLevels = 1; 
        shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;  // Typeless for both DSV and SRV
        shadowDesc.SampleDesc.Count = 1;
        shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        
        D3D12_CLEAR_VALUE clearVal{}; 
        clearVal.Format = DXGI_FORMAT_D32_FLOAT; 
        clearVal.DepthStencil.Depth = 1.0f;
        
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &shadowDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearVal, IID_PPV_ARGS(&shadowMap));
        
        // Create DSV for shadow map
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, shadowDsvHeap->GetCPUDescriptorHandleForHeapStart());
        
        // Create SRV for shadow map sampling (allocate from main srv heap)
        shadowMapSrvIndex = nextSrvIndex++;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += shadowMapSrvIndex * srvDescSize;
        device->CreateShaderResourceView(shadowMap.Get(), &srvDesc, srvHandle);
        
        shadowMapReady = true;
    }
    
    void createDefaultTexture() {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = 1; texDesc.Height = 1; texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; texDesc.SampleDesc.Count = 1;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, 
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultTexture));
        
        D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = 256; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> uploadBuf;
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf));
        uint8_t white[4] = {255, 255, 255, 255};
        void* mapped; uploadBuf->Map(0, nullptr, &mapped);
        memcpy(mapped, white, 4);
        uploadBuf->Unmap(0, nullptr);
        
        cmdList->Reset(allocators[0].Get(), nullptr);
        D3D12_TEXTURE_COPY_LOCATION dst{}, src{};
        dst.pResource = defaultTexture.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource = uploadBuf.Get(); src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);
        src.PlacedFootprint = footprint;
        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = defaultTexture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        cmdList->Close();
        ID3D12CommandList* lists[] = {cmdList.Get()};
        queue->ExecuteCommandLists(1, lists);
        waitForGPU();
        
        defaultTextureSrvIndex = nextSrvIndex++;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += defaultTextureSrvIndex * srvDescSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; 
        srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
        srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(defaultTexture.Get(), &srvViewDesc, srvHandle);
    }
    
    void createPipeline() {
        printf("[INIT] createPipeline() called\n");
        fflush(stdout);
        // Root signature: CBV + 7 SRV tables (diffuse, normal, specular, shadow, irradiance, prefiltered, brdfLUT) + 2 samplers
        D3D12_DESCRIPTOR_RANGE srvRanges[7]{};
        for (int i = 0; i < 7; i++) {
            srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            srvRanges[i].NumDescriptors = 1;
            srvRanges[i].BaseShaderRegister = i;
        }
        
        D3D12_ROOT_PARAMETER rootParams[8]{};
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; 
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        for (int i = 0; i < 7; i++) {
            rootParams[i+1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[i+1].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[i+1].DescriptorTable.pDescriptorRanges = &srvRanges[i];
            rootParams[i+1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        }
        
        // Two samplers: texture sampler (s0) and shadow comparison sampler (s1)
        D3D12_STATIC_SAMPLER_DESC samplers[2]{};
        // s0: Anisotropic texture sampler
        samplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
        samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].MaxAnisotropy = 16;
        samplers[0].ShaderRegister = 0;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        
        // s1: Shadow comparison sampler
        samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;  // Beyond shadow map = lit
        samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        samplers[1].ShaderRegister = 1;
        samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        
        D3D12_ROOT_SIGNATURE_DESC rsDesc{}; 
        rsDesc.NumParameters = 8; rsDesc.pParameters = rootParams;
        rsDesc.NumStaticSamplers = 2; rsDesc.pStaticSamplers = samplers;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), 
            IID_PPV_ARGS(&rootSignature));
        
        // Compile shaders - load from file with fallback to embedded
        ComPtr<ID3DBlob> vs, ps, errorBlob;
        UINT flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        std::string pbrSource = loadShaderWithFallback("pbr.hlsl", kPBRShaderSource);
        if (FAILED(D3DCompile(pbrSource.c_str(), pbrSource.size(), "pbr.hlsl", nullptr, nullptr, 
                              "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob))) {
            if (errorBlob) std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        if (FAILED(D3DCompile(pbrSource.c_str(), pbrSource.size(), "pbr.hlsl", nullptr, nullptr, 
                              "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob))) {
            if (errorBlob) std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        
        // Input layout
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        
        // Depth-only pre-pass pipeline (writes depth, no color, no pixel shader)
        // Used for hidden line removal in wireframe mode when X-Ray is OFF
        // CullMode=NONE: render both front and back faces to ensure complete depth coverage
        // (needed for non-manifold/single-sided geometry like clothing, hair, etc.)
        {
            // Compile a minimal depth-only VS (uses worldViewProj, not lightViewProj)
            static const char* kDepthOnlyVS = R"(
cbuffer CB : register(b0) { float4x4 worldViewProj; };
struct VSIn { float3 pos : POSITION; float3 n : NORMAL; float4 t : TANGENT; float2 uv : TEXCOORD; float3 c : COLOR; };
float4 VSMain(VSIn i) : SV_POSITION { return mul(worldViewProj, float4(i.pos, 1.0)); }
)";
            ComPtr<ID3DBlob> depthVs, depthErr;
            HRESULT hr = D3DCompile(kDepthOnlyVS, strlen(kDepthOnlyVS), "depthonly.hlsl", nullptr, nullptr,
                                    "VSMain", "vs_5_0", flags, 0, &depthVs, &depthErr);
            if (SUCCEEDED(hr) && depthVs) {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC depthPso{};
                depthPso.InputLayout = {inputLayout, _countof(inputLayout)};
                depthPso.pRootSignature = rootSignature.Get();
                depthPso.VS = {depthVs->GetBufferPointer(), depthVs->GetBufferSize()};
                // No pixel shader — depth only
                depthPso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                depthPso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // No culling — complete depth coverage
                depthPso.RasterizerState.DepthClipEnable = TRUE;
                depthPso.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;  // No color writes
                depthPso.DepthStencilState.DepthEnable = TRUE;
                depthPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
                depthPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
                depthPso.SampleMask = UINT_MAX;
                depthPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                depthPso.NumRenderTargets = 1;
                depthPso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                depthPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
                depthPso.SampleDesc.Count = 1;
                device->CreateGraphicsPipelineState(&depthPso, IID_PPV_ARGS(&depthOnlyPipelineState));
            }
        }
        
        // Shadow pass pipeline (depth-only, no pixel shader)
        ComPtr<ID3DBlob> shadowVs;
        std::string shadowSource = loadShaderWithFallback("shadow.hlsl", kShadowShaderSource);
        if (FAILED(D3DCompile(shadowSource.c_str(), shadowSource.size(), "shadow.hlsl", nullptr, nullptr, 
                              "VSMain", "vs_5_0", flags, 0, &shadowVs, &errorBlob))) {
            if (errorBlob) std::cerr << "Shadow shader error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc{};
        shadowPsoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
        shadowPsoDesc.pRootSignature = rootSignature.Get();
        shadowPsoDesc.VS = {shadowVs->GetBufferPointer(), shadowVs->GetBufferSize()};
        // No pixel shader for depth-only pass
        shadowPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        shadowPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;  // Cull front faces for shadow
        shadowPsoDesc.RasterizerState.DepthClipEnable = TRUE;
        shadowPsoDesc.RasterizerState.DepthBias = 1000;  // Small depth bias
        shadowPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        shadowPsoDesc.DepthStencilState.DepthEnable = TRUE;
        shadowPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        shadowPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        shadowPsoDesc.SampleMask = UINT_MAX;
        shadowPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        shadowPsoDesc.NumRenderTargets = 0;  // No render targets
        shadowPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        shadowPsoDesc.SampleDesc.Count = 1;
        device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&shadowPipelineState));
        
        // Constant buffer - ring buffer for per-draw constants (persistently mapped)
        D3D12_HEAP_PROPERTIES cbHeap{}; cbHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC cbDesc{}; cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.Width = kAlignedConstantSize * kMaxDrawsPerFrame * 2;  // Double buffered
        cbDesc.Height = 1; cbDesc.DepthOrArraySize = 1;
        cbDesc.MipLevels = 1; cbDesc.SampleDesc.Count = 1; cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        device->CreateCommittedResource(&cbHeap, D3D12_HEAP_FLAG_NONE, &cbDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer));
        
        // Persistently map constant buffer
        void* mapped;
        constantBuffer->Map(0, nullptr, &mapped);
        constantBufferMapped = static_cast<uint8_t*>(mapped);
        
        createLinePipeline();
        ready = true;
        std::cout << "[unified/dx12] PBR Pipeline ready" << std::endl;
    }
    
    void createLinePipeline() {
        ComPtr<ID3DBlob> vs, ps, errorBlob;
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        
        std::string lineSource = loadShaderWithFallback("line.hlsl", kLineShaderSource);
        D3DCompile(lineSource.c_str(), lineSource.size(), "line.hlsl", nullptr, nullptr, 
                   "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob);
        D3DCompile(lineSource.c_str(), lineSource.size(), "line.hlsl", nullptr, nullptr, 
                   "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob);
        
        D3D12_INPUT_ELEMENT_DESC lineLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = {lineLayout, _countof(lineLayout)};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&linePipelineState));
        
        // Create gizmo pipeline - always visible (depth test DISABLED)
        psoDesc.DepthStencilState.DepthEnable = FALSE;  // Completely disable depth test
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gizmoPipelineState));
        
        // Create depth-tested gizmo pipeline (for non-xray wireframe/selection)
        // Same as line pipeline but with LESS_EQUAL + depth bias to prevent z-fighting
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC depthGizmoPso = psoDesc;
            depthGizmoPso.DepthStencilState.DepthEnable = TRUE;
            depthGizmoPso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
            depthGizmoPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            depthGizmoPso.RasterizerState.DepthBias = -100;
            depthGizmoPso.RasterizerState.DepthBiasClamp = 0.0f;
            depthGizmoPso.RasterizerState.SlopeScaledDepthBias = -1.0f;
            device->CreateGraphicsPipelineState(&depthGizmoPso, IID_PPV_ARGS(&gizmoDepthPipelineState));
        }
        
        // Create wireframe pipeline (same as main pipeline but with D3D12_FILL_MODE_WIREFRAME)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC wirePsoDesc = psoDesc;  // Copy from current psoDesc
        wirePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;  // 必须是三角形！
        wirePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;  // WIREFRAME!
        wirePsoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
        wirePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // Show all edges
        wirePsoDesc.RasterizerState.DepthBias = -1000;  // Bring wireframe forward
        wirePsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        wirePsoDesc.RasterizerState.SlopeScaledDepthBias = -1.0f;
        wirePsoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
        wirePsoDesc.DepthStencilState.DepthEnable = TRUE;
        wirePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        wirePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        HRESULT hrWire = device->CreateGraphicsPipelineState(&wirePsoDesc, IID_PPV_ARGS(&wireframePipelineState));
        printf("[INIT] wireframePipelineState: hr=%08X, ptr=%p\n", (unsigned)hrWire, (void*)wireframePipelineState.Get());
        
        // Create highlight pipeline for selected mesh (always visible, like Maya/Blender)
        D3D12_GRAPHICS_PIPELINE_STATE_DESC highlightPsoDesc = wirePsoDesc;  // Start from wireframe
        highlightPsoDesc.DepthStencilState.DepthEnable = TRUE;
        highlightPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;  // Always pass - draw on top!
        highlightPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;  // Don't write depth
        highlightPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // No culling - show ALL wireframe lines!
        highlightPsoDesc.RasterizerState.DepthBias = 0;
        highlightPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        highlightPsoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        device->CreateGraphicsPipelineState(&highlightPsoDesc, IID_PPV_ARGS(&highlightPipelineState));
        
        // Create unlit wireframe pipeline - solid color output, no lighting (for mesh highlight)
        {
            ComPtr<ID3DBlob> unlitVs, unlitPs, unlitErr;
            std::string wireframeSource = loadShaderWithFallback("wireframe.hlsl", kUnlitWireframeShaderSource);
            HRESULT vsHr = D3DCompile(wireframeSource.c_str(), wireframeSource.size(), "wireframe.hlsl", nullptr, nullptr,
                       "VSMain", "vs_5_0", flags, 0, &unlitVs, &unlitErr);
            printf("[INIT] Unlit VS compile: hr=%08X\n", (unsigned)vsHr);
            if (unlitErr) { std::cerr << "[unlit] VS error: " << (char*)unlitErr->GetBufferPointer() << std::endl; unlitErr.Reset(); }
            
            HRESULT psHr = D3DCompile(wireframeSource.c_str(), wireframeSource.size(), "wireframe.hlsl", nullptr, nullptr,
                       "PSMain", "ps_5_0", flags, 0, &unlitPs, &unlitErr);
            printf("[INIT] Unlit PS compile: hr=%08X\n", (unsigned)psHr);
            if (unlitErr) { std::cerr << "[unlit] PS error: " << (char*)unlitErr->GetBufferPointer() << std::endl; unlitErr.Reset(); }
            
            printf("[INIT] Checking unlit shaders: unlitVs=%p, unlitPs=%p\n", (void*)unlitVs.Get(), (void*)unlitPs.Get());
            fflush(stdout);
            if (unlitVs && unlitPs) {
                printf("[INIT] Creating unlit pipeline states...\n");
                fflush(stdout);
                
                // Define input layout for unlit shader (must be local to this scope)
                D3D12_INPUT_ELEMENT_DESC unlitInputLayout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                };
                
                // Create fresh pipeline state description from scratch
                D3D12_GRAPHICS_PIPELINE_STATE_DESC unlitPsoDesc{};
                unlitPsoDesc.pRootSignature = rootSignature.Get();
                unlitPsoDesc.VS = { unlitVs->GetBufferPointer(), unlitVs->GetBufferSize() };
                unlitPsoDesc.PS = { unlitPs->GetBufferPointer(), unlitPs->GetBufferSize() };
                unlitPsoDesc.InputLayout = { unlitInputLayout, _countof(unlitInputLayout) };
                unlitPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                unlitPsoDesc.NumRenderTargets = 1;
                unlitPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                unlitPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
                unlitPsoDesc.SampleDesc.Count = 1;
                unlitPsoDesc.SampleMask = UINT_MAX;
                
                // Rasterizer for wireframe
                unlitPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
                unlitPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                unlitPsoDesc.RasterizerState.DepthClipEnable = TRUE;
                unlitPsoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
                unlitPsoDesc.RasterizerState.DepthBias = 0;
                unlitPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
                unlitPsoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
                
                // Depth state - always pass (draw on top of everything)
                unlitPsoDesc.DepthStencilState.DepthEnable = TRUE;
                unlitPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
                unlitPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                
                // No blending
                unlitPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                
                HRESULT hr1 = device->CreateGraphicsPipelineState(&unlitPsoDesc, IID_PPV_ARGS(&unlitWireframePipelineState));
                printf("[INIT] unlitWireframePipelineState: hr=%08X, ptr=%p\n", (unsigned)hr1, (void*)unlitWireframePipelineState.Get());
                
                // Create overlay wireframe pipeline - depth test + depth write + bias
                D3D12_GRAPHICS_PIPELINE_STATE_DESC overlayPsoDesc = unlitPsoDesc;
                overlayPsoDesc.DepthStencilState.DepthEnable = TRUE;
                overlayPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;  // Don't overwrite depth pre-pass
                overlayPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                overlayPsoDesc.RasterizerState.DepthBias = -1000;  // Bring wireframe forward
                HRESULT hr2 = device->CreateGraphicsPipelineState(&overlayPsoDesc, IID_PPV_ARGS(&overlayWireframePipelineState));
                printf("[INIT] overlayWireframePipelineState: hr=%08X, ptr=%p\n", (unsigned)hr2, (void*)overlayWireframePipelineState.Get());
                
                // Create culled wireframe pipeline (hidden line removal via back-face culling)
                // CullMode=BACK means only front-facing edges are drawn — zero extra cost
                D3D12_GRAPHICS_PIPELINE_STATE_DESC culledPsoDesc = overlayPsoDesc;
                culledPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
                culledPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;  // Write depth for self-occlusion
                device->CreateGraphicsPipelineState(&culledPsoDesc, IID_PPV_ARGS(&culledWireframePipelineState));
                
                // ===== GPU Edge Line Pipelines (LINE topology, full Vertex layout) =====
                // These use the unlit wireframe shader but with LINE topology for edge index buffers
                // X-Ray ON: no depth test (always visible, maximum performance)
                {
                    D3D12_GRAPHICS_PIPELINE_STATE_DESC edgePso = unlitPsoDesc;
                    edgePso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
                    edgePso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                    edgePso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                    edgePso.RasterizerState.AntialiasedLineEnable = TRUE;
                    edgePso.DepthStencilState.DepthEnable = FALSE;
                    edgePso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
                    edgePso.RasterizerState.DepthBias = 0;
                    edgePso.RasterizerState.DepthBiasClamp = 0.0f;
                    edgePso.RasterizerState.SlopeScaledDepthBias = 0.0f;
                    HRESULT hr3 = device->CreateGraphicsPipelineState(&edgePso, IID_PPV_ARGS(&edgeLinePipelineState));
                    printf("[INIT] edgeLinePipelineState: hr=%08X, ptr=%p\n", (unsigned)hr3, (void*)edgeLinePipelineState.Get());
                    
                    // X-Ray OFF: depth test enabled (hidden line removal via depth pre-pass)
                    // Negative bias pushes edges slightly closer to camera so they reliably
                    // appear on the surface they belong to (prevents z-fighting flicker)
                    edgePso.DepthStencilState.DepthEnable = TRUE;
                    edgePso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
                    edgePso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                    edgePso.RasterizerState.DepthBias = -1000;
                    edgePso.RasterizerState.DepthBiasClamp = 0.0f;
                    edgePso.RasterizerState.SlopeScaledDepthBias = -2.0f;
                    HRESULT hr4 = device->CreateGraphicsPipelineState(&edgePso, IID_PPV_ARGS(&edgeLineDepthPipelineState));
                    printf("[INIT] edgeLineDepthPipelineState: hr=%08X, ptr=%p\n", (unsigned)hr4, (void*)edgeLineDepthPipelineState.Get());
                }
            } else {
                printf("[ERROR] Unlit shader compilation failed - unlitVs=%p, unlitPs=%p\n", (void*)unlitVs.Get(), (void*)unlitPs.Get());
            }
        }
        
        // ===== Create Solid Mode Pipeline (simple lighting, no textures) =====
        {
            ComPtr<ID3DBlob> solidVs, solidPs, errors;
            std::string solidSource = loadShaderWithFallback("solid.hlsl", kSolidModeShaderSource);
            HRESULT hr1 = D3DCompile(solidSource.c_str(), solidSource.size(),
                "solid.hlsl", nullptr, nullptr, "VSMain", "vs_5_0",
                D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &solidVs, &errors);
            if (FAILED(hr1) && errors) {
                printf("[ERROR] Solid VS compile: %s\n", (char*)errors->GetBufferPointer());
            }
            
            HRESULT hr2 = D3DCompile(solidSource.c_str(), solidSource.size(),
                "solid.hlsl", nullptr, nullptr, "PSMain", "ps_5_0",
                D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &solidPs, &errors);
            if (FAILED(hr2) && errors) {
                printf("[ERROR] Solid PS compile: %s\n", (char*)errors->GetBufferPointer());
            }
            
            if (solidVs && solidPs) {
                // 定义完整的输入布局（与主PBR管线相同）
                D3D12_INPUT_ELEMENT_DESC solidInputLayout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                };
                
                // 从头创建管线描述（不依赖其他psoDesc）
                D3D12_GRAPHICS_PIPELINE_STATE_DESC solidPsoDesc{};
                solidPsoDesc.pRootSignature = rootSignature.Get();
                solidPsoDesc.VS = { solidVs->GetBufferPointer(), solidVs->GetBufferSize() };
                solidPsoDesc.PS = { solidPs->GetBufferPointer(), solidPs->GetBufferSize() };
                solidPsoDesc.InputLayout = { solidInputLayout, _countof(solidInputLayout) };
                solidPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                solidPsoDesc.NumRenderTargets = 1;
                solidPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                solidPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
                solidPsoDesc.SampleDesc.Count = 1;
                solidPsoDesc.SampleMask = UINT_MAX;
                
                // 光栅化器状态
                solidPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                solidPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
                solidPsoDesc.RasterizerState.DepthClipEnable = TRUE;
                
                // 深度模板状态
                solidPsoDesc.DepthStencilState.DepthEnable = TRUE;
                solidPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
                solidPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
                
                // 混合状态
                solidPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                
                HRESULT hr = device->CreateGraphicsPipelineState(&solidPsoDesc, IID_PPV_ARGS(&solidModePipelineState));
                printf("[INIT] solidModePipelineState: hr=%08X, ptr=%p\n", (unsigned)hr, (void*)solidModePipelineState.Get());
            } else {
                printf("[ERROR] Solid shader compilation failed\n");
            }
        }
        
        createGridData();
        createSkinnedPipeline();
    }
    
    void createSkinnedPipeline() {
        // Root signature for skinned rendering: 2 CBVs (scene + bones) + 7 SRV tables
        D3D12_DESCRIPTOR_RANGE srvRanges[7]{};
        for (int i = 0; i < 7; i++) {
            srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            srvRanges[i].NumDescriptors = 1;
            srvRanges[i].BaseShaderRegister = i;
        }
        
        D3D12_ROOT_PARAMETER rootParams[9]{};
        // Scene constants CBV (b0)
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        // Bone matrices CBV (b1)
        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[1].Descriptor.ShaderRegister = 1;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        // 7 SRV tables
        for (int i = 0; i < 7; i++) {
            rootParams[i+2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[i+2].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[i+2].DescriptorTable.pDescriptorRanges = &srvRanges[i];
            rootParams[i+2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        }
        
        D3D12_STATIC_SAMPLER_DESC samplers[2]{};
        samplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
        samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].MaxAnisotropy = 16;
        samplers[0].ShaderRegister = 0;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        
        samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplers[1].ShaderRegister = 1;
        samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
        samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        
        D3D12_ROOT_SIGNATURE_DESC rsDesc{};
        rsDesc.NumParameters = 9;
        rsDesc.pParameters = rootParams;
        rsDesc.NumStaticSamplers = 2;
        rsDesc.pStaticSamplers = samplers;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> signature, error;
        if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
            if (error) std::cerr << "[skinned] Root signature error: " << (char*)error->GetBufferPointer() << std::endl;
            return;
        }
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&skinnedRootSignature));
        
        // Load skinned shader
        std::string shaderSource = loadShaderFile(shaderBasePath + "skinned.hlsl");
        if (shaderSource.empty()) {
            std::cerr << "[skinned] Failed to load skinned.hlsl" << std::endl;
            return;
        }
        
        ComPtr<ID3DBlob> vs, ps, errorBlob;
        UINT flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        
        HRESULT hr = D3DCompile(shaderSource.c_str(), shaderSource.size(), "skinned.hlsl", nullptr, nullptr,
                                "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) std::cerr << "[skinned] VS error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        
        hr = D3DCompile(shaderSource.c_str(), shaderSource.size(), "skinned.hlsl", nullptr, nullptr,
                        "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) std::cerr << "[skinned] PS error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        
        // Skinned vertex layout
        D3D12_INPUT_ELEMENT_DESC skinnedLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = {skinnedLayout, _countof(skinnedLayout)};
        psoDesc.pRootSignature = skinnedRootSignature.Get();
        psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        
        hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&skinnedPipelineState));
        if (FAILED(hr)) {
            std::cerr << "[skinned] Failed to create PSO" << std::endl;
            return;
        }
        
        // Create bone buffer
        D3D12_HEAP_PROPERTIES cbHeap{}; cbHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC cbDesc{}; cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.Width = kBoneBufferSize;
        cbDesc.Height = 1; cbDesc.DepthOrArraySize = 1;
        cbDesc.MipLevels = 1; cbDesc.SampleDesc.Count = 1; cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        device->CreateCommittedResource(&cbHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&boneBuffer));
        
        void* mapped;
        boneBuffer->Map(0, nullptr, &mapped);
        boneBufferMapped = static_cast<uint8_t*>(mapped);
        
        // Initialize to identity matrices
        float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        for (UINT i = 0; i < kMaxBones; i++) {
            memcpy(boneBufferMapped + i * 64, identity, 64);
        }
        
        skinnedPipelineReady = true;
        std::cout << "[skinned] Skinned rendering pipeline ready" << std::endl;
    }
    
    // Recompile PBR shader from external file (for hot-reload)
    bool recompilePBRShaders() {
        std::string pbrSource = loadShaderFile(shaderBasePath + "pbr.hlsl");
        std::string shadowSource = loadShaderFile(shaderBasePath + "shadow.hlsl");
        
        // If files not found, fall back to embedded shaders
        if (pbrSource.empty()) pbrSource = kPBRShaderSource;
        if (shadowSource.empty()) shadowSource = kShadowShaderSource;
        
        ComPtr<ID3DBlob> vs, ps, shadowVs, errorBlob;
        UINT flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        
        // Compile PBR vertex shader
        HRESULT hr = D3DCompile(pbrSource.c_str(), pbrSource.size(), "pbr.hlsl", nullptr, nullptr, 
                                "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) shaderError = std::string((char*)errorBlob->GetBufferPointer());
            std::cerr << "[shader] PBR VS compile error: " << shaderError << std::endl;
            return false;
        }
        
        // Compile PBR pixel shader
        hr = D3DCompile(pbrSource.c_str(), pbrSource.size(), "pbr.hlsl", nullptr, nullptr, 
                        "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) shaderError = std::string((char*)errorBlob->GetBufferPointer());
            std::cerr << "[shader] PBR PS compile error: " << shaderError << std::endl;
            return false;
        }
        
        // Compile shadow vertex shader
        hr = D3DCompile(shadowSource.c_str(), shadowSource.size(), "shadow.hlsl", nullptr, nullptr, 
                        "VSMain", "vs_5_0", flags, 0, &shadowVs, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) shaderError = std::string((char*)errorBlob->GetBufferPointer());
            std::cerr << "[shader] Shadow VS compile error: " << shaderError << std::endl;
            return false;
        }
        
        // Wait for GPU before releasing old pipelines
        waitForGPU();
        
        // Create new PBR pipeline
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        
        ComPtr<ID3D12PipelineState> newPipelineState;
        hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&newPipelineState));
        if (FAILED(hr)) {
            shaderError = "Failed to create PBR pipeline state";
            return false;
        }
        
        // Create new shadow pipeline
        D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc{};
        shadowPsoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
        shadowPsoDesc.pRootSignature = rootSignature.Get();
        shadowPsoDesc.VS = {shadowVs->GetBufferPointer(), shadowVs->GetBufferSize()};
        shadowPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        shadowPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        shadowPsoDesc.RasterizerState.DepthClipEnable = TRUE;
        shadowPsoDesc.RasterizerState.DepthBias = 1000;
        shadowPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        shadowPsoDesc.DepthStencilState.DepthEnable = TRUE;
        shadowPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        shadowPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        shadowPsoDesc.SampleMask = UINT_MAX;
        shadowPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        shadowPsoDesc.NumRenderTargets = 0;
        shadowPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        shadowPsoDesc.SampleDesc.Count = 1;
        
        ComPtr<ID3D12PipelineState> newShadowPipelineState;
        hr = device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&newShadowPipelineState));
        if (FAILED(hr)) {
            shaderError = "Failed to create shadow pipeline state";
            return false;
        }
        
        // Swap in new pipelines
        pipelineState = std::move(newPipelineState);
        shadowPipelineState = std::move(newShadowPipelineState);
        shaderError.clear();
        
        std::cout << "[shader] Shaders recompiled successfully" << std::endl;
        return true;
    }
    
    void createGridData() {
        struct LineVertex { float pos[3]; float color[4]; };
        std::vector<LineVertex> vertices;
        
        float gridExtent = 1000.0f;
        float gridColor[4] = {0.25f, 0.25f, 0.28f, 0.4f};
        float majorColor[4] = {0.35f, 0.35f, 0.4f, 0.6f};
        
        for (int i = -100; i <= 100; i++) {
            float z = (float)i * 10.0f;
            float* col = (i % 10 == 0) ? majorColor : gridColor;
            if (i == 0) continue;
            vertices.push_back({{-gridExtent, 0, z}, {col[0], col[1], col[2], col[3]}});
            vertices.push_back({{gridExtent, 0, z}, {col[0], col[1], col[2], col[3]}});
        }
        
        for (int i = -100; i <= 100; i++) {
            float x = (float)i * 10.0f;
            float* col = (i % 10 == 0) ? majorColor : gridColor;
            if (i == 0) continue;
            vertices.push_back({{x, 0, -gridExtent}, {col[0], col[1], col[2], col[3]}});
            vertices.push_back({{x, 0, gridExtent}, {col[0], col[1], col[2], col[3]}});
        }
        
        gridVertexCount = (UINT)vertices.size();
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = vertices.size() * sizeof(LineVertex);
        bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gridVertexBuffer));
        
        void* mapped;
        gridVertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, vertices.data(), vertices.size() * sizeof(LineVertex));
        gridVertexBuffer->Unmap(0, nullptr);
        
        gridVbv.BufferLocation = gridVertexBuffer->GetGPUVirtualAddress();
        gridVbv.SizeInBytes = (UINT)(vertices.size() * sizeof(LineVertex));
        gridVbv.StrideInBytes = sizeof(LineVertex);
        
        // Axes
        std::vector<LineVertex> axisVerts;
        axisVerts.push_back({{-1.0f, 0.001f, 0}, {0.5f, 0.15f, 0.15f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.5f, 0.15f, 0.15f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.9f, 0.2f, 0.2f, 1.0f}});
        axisVerts.push_back({{1.0f, 0.001f, 0}, {0.9f, 0.2f, 0.2f, 1.0f}});
        axisVerts.push_back({{0, 0, 0}, {0.2f, 0.9f, 0.2f, 1.0f}});
        axisVerts.push_back({{0, 1.0f, 0}, {0.2f, 0.9f, 0.2f, 1.0f}});
        axisVerts.push_back({{0, 0.001f, -1.0f}, {0.15f, 0.25f, 0.5f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.15f, 0.25f, 0.5f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.2f, 0.4f, 0.9f, 1.0f}});
        axisVerts.push_back({{0, 0.001f, 1.0f}, {0.2f, 0.4f, 0.9f, 1.0f}});
        
        axisVertexCount = (UINT)axisVerts.size();
        
        bufDesc.Width = axisVerts.size() * sizeof(LineVertex);
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&axisVertexBuffer));
        
        axisVertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, axisVerts.data(), axisVerts.size() * sizeof(LineVertex));
        axisVertexBuffer->Unmap(0, nullptr);
        
        axisVbv.BufferLocation = axisVertexBuffer->GetGPUVirtualAddress();
        axisVbv.SizeInBytes = (UINT)(axisVerts.size() * sizeof(LineVertex));
        axisVbv.StrideInBytes = sizeof(LineVertex);
        
        gridReady = true;
        std::cout << "[unified/dx12] Grid ready (" << gridVertexCount << " lines)" << std::endl;
    }
    
    ComPtr<ID3D12Resource> uploadTexture(const TextureData& tex, UINT& outSrvIndex) {
        if (tex.pixels.empty()) {
            outSrvIndex = defaultTextureSrvIndex;
            return defaultTexture;
        }
        
        const UINT w = tex.width, h = tex.height;
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = w; texDesc.Height = h; texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; texDesc.SampleDesc.Count = 1;
        
        ComPtr<ID3D12Resource> texture;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, 
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
        
        D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        UINT rowPitch = (w * 4 + 255) & ~255;
        bufDesc.Width = rowPitch * h; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        ComPtr<ID3D12Resource> uploadBuf;
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf));
        
        void* mapped;
        uploadBuf->Map(0, nullptr, &mapped);
        for (UINT row = 0; row < h; row++) {
            memcpy((uint8_t*)mapped + row * rowPitch, tex.pixels.data() + row * w * 4, w * 4);
        }
        uploadBuf->Unmap(0, nullptr);
        
        waitForGPU();
        allocators[0]->Reset();
        cmdList->Reset(allocators[0].Get(), nullptr);
        
        D3D12_TEXTURE_COPY_LOCATION dst{}, src{};
        dst.pResource = texture.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource = uploadBuf.Get(); src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = w; src.PlacedFootprint.Footprint.Height = h;
        src.PlacedFootprint.Footprint.Depth = 1; src.PlacedFootprint.Footprint.RowPitch = rowPitch;
        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        cmdList->Close();
        
        ID3D12CommandList* lists[] = {cmdList.Get()};
        queue->ExecuteCommandLists(1, lists);
        waitForGPU();
        
        outSrvIndex = nextSrvIndex++;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += outSrvIndex * srvDescSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; 
        srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
        srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(texture.Get(), &srvViewDesc, srvHandle);
        
        return texture;
    }
    
    // ===== Post-Processing Infrastructure =====
    
    void createPostProcessResources() {
        // Create RTV heap for HDR target and bloom textures (3 RTVs: HDR + 2 bloom)
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 3;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&postProcessRtvHeap));
        
        // Create SRV heap for post-process textures (shader visible)
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.NumDescriptors = 4;  // HDR + 2 bloom + constants
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&postProcessSrvHeap));
        
        // Create constant buffer for post-process parameters (256 bytes aligned)
        D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC cbDesc{}; cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.Width = 256; cbDesc.Height = 1; cbDesc.DepthOrArraySize = 1; 
        cbDesc.MipLevels = 1; cbDesc.SampleDesc.Count = 1; cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&postProcessConstantBuffer));
        postProcessConstantBuffer->Map(0, nullptr, (void**)&postProcessConstantsMapped);
        
        createHDRRenderTarget();
        createPostProcessPipeline();
        
        postProcessReady = true;
        std::cout << "[post] Post-processing resources created" << std::endl;
    }
    
    void createHDRRenderTarget() {
        UINT ppRtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        UINT ppSrvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        // HDR render target (full resolution, RGBA16F)
        D3D12_RESOURCE_DESC hdrDesc{};
        hdrDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        hdrDesc.Width = width; hdrDesc.Height = height; hdrDesc.DepthOrArraySize = 1;
        hdrDesc.MipLevels = 1; hdrDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        hdrDesc.SampleDesc.Count = 1;
        hdrDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        
        D3D12_CLEAR_VALUE hdrClear{}; hdrClear.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        hdrClear.Color[0] = 0.05f; hdrClear.Color[1] = 0.05f; hdrClear.Color[2] = 0.08f; hdrClear.Color[3] = 1.0f;
        
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &hdrDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &hdrClear, IID_PPV_ARGS(&hdrRenderTarget));
        
        // Create RTV for HDR target
        D3D12_CPU_DESCRIPTOR_HANDLE hdrRtvHandle = postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_RENDER_TARGET_VIEW_DESC hdrRtvDesc{}; hdrRtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        hdrRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(hdrRenderTarget.Get(), &hdrRtvDesc, hdrRtvHandle);
        hdrRtvIndex = 0;
        
        // Create SRV for HDR target
        D3D12_CPU_DESCRIPTOR_HANDLE hdrSrvHandle = postProcessSrvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_SHADER_RESOURCE_VIEW_DESC hdrSrvDesc{};
        hdrSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        hdrSrvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        hdrSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        hdrSrvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(hdrRenderTarget.Get(), &hdrSrvDesc, hdrSrvHandle);
        hdrSrvIndex = 0;
        
        // Bloom textures (half resolution for efficiency)
        uint32_t bloomWidth = width / 2;
        uint32_t bloomHeight = height / 2;
        
        D3D12_RESOURCE_DESC bloomDesc{};
        bloomDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        bloomDesc.Width = bloomWidth; bloomDesc.Height = bloomHeight; bloomDesc.DepthOrArraySize = 1;
        bloomDesc.MipLevels = 1; bloomDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        bloomDesc.SampleDesc.Count = 1;
        bloomDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        
        D3D12_CLEAR_VALUE bloomClear{}; bloomClear.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        
        for (int i = 0; i < 2; i++) {
            device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bloomDesc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &bloomClear, IID_PPV_ARGS(&bloomTextures[i]));
            
            // RTV
            D3D12_CPU_DESCRIPTOR_HANDLE bloomRtvHandle = postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
            bloomRtvHandle.ptr += (1 + i) * ppRtvDescSize;
            D3D12_RENDER_TARGET_VIEW_DESC bloomRtvDesc{}; bloomRtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            bloomRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device->CreateRenderTargetView(bloomTextures[i].Get(), &bloomRtvDesc, bloomRtvHandle);
            bloomRtvIndex[i] = 1 + i;
            
            // SRV
            D3D12_CPU_DESCRIPTOR_HANDLE bloomSrvHandle = postProcessSrvHeap->GetCPUDescriptorHandleForHeapStart();
            bloomSrvHandle.ptr += (1 + i) * ppSrvDescSize;
            D3D12_SHADER_RESOURCE_VIEW_DESC bloomSrvDesc{};
            bloomSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            bloomSrvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            bloomSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            bloomSrvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(bloomTextures[i].Get(), &bloomSrvDesc, bloomSrvHandle);
            bloomSrvIndex[i] = 1 + i;
        }
    }
    
    void createPostProcessPipeline() {
        // Root signature: CBV (b0) + SRV table (t0, t1) + sampler
        D3D12_DESCRIPTOR_RANGE srvRange{};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 2;  // scene + bloom
        srvRange.BaseShaderRegister = 0;
        
        D3D12_ROOT_PARAMETER rootParams[2]{};
        // CBV for post-process constants
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        // SRV table
        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ShaderRegister = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        
        D3D12_ROOT_SIGNATURE_DESC rsDesc{};
        rsDesc.NumParameters = 2;
        rsDesc.pParameters = rootParams;
        rsDesc.NumStaticSamplers = 1;
        rsDesc.pStaticSamplers = &sampler;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&postProcessRootSignature));
        
        // Load and compile post-process shaders
        std::string shaderPath = shaderBasePath + "post_process.hlsl";
        std::ifstream file(shaderPath);
        std::string shaderSource;
        
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            shaderSource = buffer.str();
        } else {
            std::cerr << "[post] Failed to load post_process.hlsl, using embedded shader" << std::endl;
            shaderSource = getEmbeddedPostProcessShader();
        }
        
        // Compile shaders
        ComPtr<ID3DBlob> vsBlob, psCompositeBlob, psThresholdBlob, psBlurHBlob, psBlurVBlob;
        UINT compileFlags = 0;
        #ifdef _DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #endif
        
        D3DCompile(shaderSource.c_str(), shaderSource.size(), "post_process.hlsl", nullptr, nullptr,
            "VSMain", "vs_5_0", compileFlags, 0, &vsBlob, &error);
        if (error) std::cerr << "[post] VS error: " << (char*)error->GetBufferPointer() << std::endl;
        
        D3DCompile(shaderSource.c_str(), shaderSource.size(), "post_process.hlsl", nullptr, nullptr,
            "PSMain", "ps_5_0", compileFlags, 0, &psCompositeBlob, &error);
        if (error) std::cerr << "[post] PS error: " << (char*)error->GetBufferPointer() << std::endl;
        
        D3DCompile(shaderSource.c_str(), shaderSource.size(), "post_process.hlsl", nullptr, nullptr,
            "PSBloomThreshold", "ps_5_0", compileFlags, 0, &psThresholdBlob, &error);
        if (error) std::cerr << "[post] Threshold error: " << (char*)error->GetBufferPointer() << std::endl;
        
        D3DCompile(shaderSource.c_str(), shaderSource.size(), "post_process.hlsl", nullptr, nullptr,
            "PSBlurH", "ps_5_0", compileFlags, 0, &psBlurHBlob, &error);
        if (error) std::cerr << "[post] BlurH error: " << (char*)error->GetBufferPointer() << std::endl;
        
        D3DCompile(shaderSource.c_str(), shaderSource.size(), "post_process.hlsl", nullptr, nullptr,
            "PSBlurV", "ps_5_0", compileFlags, 0, &psBlurVBlob, &error);
        if (error) std::cerr << "[post] BlurV error: " << (char*)error->GetBufferPointer() << std::endl;
        
        if (!vsBlob || !psCompositeBlob) {
            std::cerr << "[post] Failed to compile post-process shaders" << std::endl;
            return;
        }
        
        // Create PSO for composite pass
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = postProcessRootSignature.Get();
        psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
        psoDesc.PS = {psCompositeBlob->GetBufferPointer(), psCompositeBlob->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // Output to swapchain
        psoDesc.SampleDesc.Count = 1;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&postProcessPSO));
        
        // Create PSO for bloom threshold (output to HDR format)
        if (psThresholdBlob) {
            psoDesc.PS = {psThresholdBlob->GetBufferPointer(), psThresholdBlob->GetBufferSize()};
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomThresholdPSO));
        }
        
        // Create PSO for horizontal blur
        if (psBlurHBlob) {
            psoDesc.PS = {psBlurHBlob->GetBufferPointer(), psBlurHBlob->GetBufferSize()};
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomBlurHPSO));
        }
        
        // Create PSO for vertical blur
        if (psBlurVBlob) {
            psoDesc.PS = {psBlurVBlob->GetBufferPointer(), psBlurVBlob->GetBufferSize()};
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomBlurVPSO));
        }
        
        std::cout << "[post] Post-process pipeline created" << std::endl;
    }
    
    std::string getEmbeddedPostProcessShader() {
        return R"(
cbuffer PostProcessConstants : register(b0) {
    float bloomThreshold;
    float bloomIntensity;
    float bloomRadius;
    float exposure;
    float gamma;
    float saturation;
    float contrast;
    float brightness;
    float3 liftColor;
    float vignetteIntensity;
    float3 gammaColor;
    float vignetteRadius;
    float3 gainColor;
    float chromaticAberration;
    float filmGrainIntensity;
    float filmGrainSize;
    uint enabledEffects;
    uint toneMappingMode;
    float screenWidth;
    float screenHeight;
    float time;
    float _pad;
};

Texture2D sceneTexture : register(t0);
Texture2D bloomTexture : register(t1);
SamplerState linearSampler : register(s0);

struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Full-screen triangle
VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    output.uv.y = 1.0 - output.uv.y;
    return output;
}

// ACES tone mapping
float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

// Vignette
float vignette(float2 uv, float intensity, float radius) {
    float2 center = uv - 0.5;
    float dist = length(center);
    return 1.0 - smoothstep(radius, radius + 0.5, dist) * intensity;
}

// Film grain
float grain(float2 uv, float t, float intensity) {
    float noise = frac(sin(dot(uv + t, float2(12.9898, 78.233))) * 43758.5453);
    return (noise - 0.5) * intensity;
}

// Composite pass - combines scene with bloom and applies final effects
float4 PSMain(VSOutput input) : SV_TARGET {
    float3 scene = sceneTexture.Sample(linearSampler, input.uv).rgb;
    float3 bloom = bloomTexture.Sample(linearSampler, input.uv).rgb;
    
    // Add bloom
    float3 color = scene + bloom * bloomIntensity;
    
    // Exposure
    color *= exposure;
    
    // Tone mapping (ACES)
    if (toneMappingMode > 0) {
        color = ACESFilm(color);
    }
    
    // Vignette
    if ((enabledEffects & 8) != 0) {
        color *= vignette(input.uv, vignetteIntensity, vignetteRadius);
    }
    
    // Film grain
    if ((enabledEffects & 32) != 0) {
        color += grain(input.uv, time, filmGrainIntensity);
    }
    
    // Gamma correction
    color = pow(max(color, 0.0), 1.0 / gamma);
    
    return float4(color, 1.0);
}

// Bloom threshold extraction
float4 PSBloomThreshold(VSOutput input) : SV_TARGET {
    float3 color = sceneTexture.Sample(linearSampler, input.uv).rgb;
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold) {
        return float4(color * (brightness - bloomThreshold) / brightness, 1.0);
    }
    return float4(0, 0, 0, 1);
}

// Gaussian blur weights (9-tap)
static const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

// Horizontal blur
float4 PSBlurH(VSOutput input) : SV_TARGET {
    float2 texelSize = 1.0 / float2(screenWidth * 0.5, screenHeight * 0.5);
    float3 result = sceneTexture.Sample(linearSampler, input.uv).rgb * weights[0];
    for (int i = 1; i < 5; i++) {
        result += sceneTexture.Sample(linearSampler, input.uv + float2(texelSize.x * i * bloomRadius, 0)).rgb * weights[i];
        result += sceneTexture.Sample(linearSampler, input.uv - float2(texelSize.x * i * bloomRadius, 0)).rgb * weights[i];
    }
    return float4(result, 1.0);
}

// Vertical blur
float4 PSBlurV(VSOutput input) : SV_TARGET {
    float2 texelSize = 1.0 / float2(screenWidth * 0.5, screenHeight * 0.5);
    float3 result = sceneTexture.Sample(linearSampler, input.uv).rgb * weights[0];
    for (int i = 1; i < 5; i++) {
        result += sceneTexture.Sample(linearSampler, input.uv + float2(0, texelSize.y * i * bloomRadius)).rgb * weights[i];
        result += sceneTexture.Sample(linearSampler, input.uv - float2(0, texelSize.y * i * bloomRadius)).rgb * weights[i];
    }
    return float4(result, 1.0);
}
)";
    }
    
    void resizePostProcessTargets() {
        if (!postProcessReady) return;
        
        // Release old resources
        hdrRenderTarget.Reset();
        bloomTextures[0].Reset();
        bloomTextures[1].Reset();
        
        // Recreate at new size
        createHDRRenderTarget();
    }
    
    void applyPostProcess() {
        if (!postProcessReady || !postProcessEnabled) return;
        if (!postProcessPSO || !bloomThresholdPSO || !bloomBlurHPSO || !bloomBlurVPSO) return;
        
        UINT ppRtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        UINT ppSrvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        // Set post-process descriptor heap
        ID3D12DescriptorHeap* ppHeaps[] = {postProcessSrvHeap.Get()};
        cmdList->SetDescriptorHeaps(1, ppHeaps);
        cmdList->SetGraphicsRootSignature(postProcessRootSignature.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, postProcessConstantBuffer->GetGPUVirtualAddress());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        uint32_t bloomWidth = width / 2;
        uint32_t bloomHeight = height / 2;
        
        // === Pass 1: Extract bright pixels from HDR scene ===
        {
            // Transition HDR to SRV, bloom[0] to RTV
            D3D12_RESOURCE_BARRIER barriers[2]{};
            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Transition.pResource = hdrRenderTarget.Get();
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Transition.pResource = bloomTextures[0].Get();
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(2, barriers);
            
            D3D12_CPU_DESCRIPTOR_HANDLE bloomRtv = postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
            bloomRtv.ptr += bloomRtvIndex[0] * ppRtvDescSize;
            cmdList->OMSetRenderTargets(1, &bloomRtv, FALSE, nullptr);
            float clearColor[] = {0, 0, 0, 1};
            cmdList->ClearRenderTargetView(bloomRtv, clearColor, 0, nullptr);
            
            D3D12_VIEWPORT vp{}; vp.Width = (float)bloomWidth; vp.Height = (float)bloomHeight; vp.MaxDepth = 1.0f;
            D3D12_RECT sr{}; sr.right = bloomWidth; sr.bottom = bloomHeight;
            cmdList->RSSetViewports(1, &vp);
            cmdList->RSSetScissorRects(1, &sr);
            
            cmdList->SetPipelineState(bloomThresholdPSO.Get());
            D3D12_GPU_DESCRIPTOR_HANDLE hdrSrv = postProcessSrvHeap->GetGPUDescriptorHandleForHeapStart();
            cmdList->SetGraphicsRootDescriptorTable(1, hdrSrv);
            cmdList->DrawInstanced(3, 1, 0, 0);
        }
        
        // === Pass 2: Horizontal blur ===
        {
            D3D12_RESOURCE_BARRIER barriers[2]{};
            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Transition.pResource = bloomTextures[0].Get();
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Transition.pResource = bloomTextures[1].Get();
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(2, barriers);
            
            D3D12_CPU_DESCRIPTOR_HANDLE bloomRtv = postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
            bloomRtv.ptr += bloomRtvIndex[1] * ppRtvDescSize;
            cmdList->OMSetRenderTargets(1, &bloomRtv, FALSE, nullptr);
            
            cmdList->SetPipelineState(bloomBlurHPSO.Get());
            D3D12_GPU_DESCRIPTOR_HANDLE bloom0Srv = postProcessSrvHeap->GetGPUDescriptorHandleForHeapStart();
            bloom0Srv.ptr += bloomSrvIndex[0] * ppSrvDescSize;
            cmdList->SetGraphicsRootDescriptorTable(1, bloom0Srv);
            cmdList->DrawInstanced(3, 1, 0, 0);
        }
        
        // === Pass 3: Vertical blur ===
        {
            D3D12_RESOURCE_BARRIER barriers[2]{};
            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Transition.pResource = bloomTextures[1].Get();
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[1].Transition.pResource = bloomTextures[0].Get();
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(2, barriers);
            
            D3D12_CPU_DESCRIPTOR_HANDLE bloomRtv = postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
            bloomRtv.ptr += bloomRtvIndex[0] * ppRtvDescSize;
            cmdList->OMSetRenderTargets(1, &bloomRtv, FALSE, nullptr);
            
            cmdList->SetPipelineState(bloomBlurVPSO.Get());
            D3D12_GPU_DESCRIPTOR_HANDLE bloom1Srv = postProcessSrvHeap->GetGPUDescriptorHandleForHeapStart();
            bloom1Srv.ptr += bloomSrvIndex[1] * ppSrvDescSize;
            cmdList->SetGraphicsRootDescriptorTable(1, bloom1Srv);
            cmdList->DrawInstanced(3, 1, 0, 0);
        }
        
        // === Pass 4: Final composite to swapchain ===
        {
            // Transition bloom[0] to SRV for reading
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = bloomTextures[0].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);
            
            // Set swapchain as render target
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtv.ptr += frameIndex * rtvDescSize;
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
            
            D3D12_VIEWPORT vp{}; vp.Width = (float)width; vp.Height = (float)height; vp.MaxDepth = 1.0f;
            D3D12_RECT sr{}; sr.right = width; sr.bottom = height;
            cmdList->RSSetViewports(1, &vp);
            cmdList->RSSetScissorRects(1, &sr);
            
            cmdList->SetPipelineState(postProcessPSO.Get());
            
            // Bind HDR scene and bloom as inputs
            D3D12_GPU_DESCRIPTOR_HANDLE hdrSrv = postProcessSrvHeap->GetGPUDescriptorHandleForHeapStart();
            cmdList->SetGraphicsRootDescriptorTable(1, hdrSrv);
            cmdList->DrawInstanced(3, 1, 0, 0);
            
            // Restore main SRV heap for ImGui
            ID3D12DescriptorHeap* mainHeaps[] = {srvHeap.Get()};
            cmdList->SetDescriptorHeaps(1, mainHeaps);
        }
    }
};

// ===== Public Interface =====

UnifiedRenderer::UnifiedRenderer() : impl_(std::make_unique<Impl>()) {}

UnifiedRenderer::~UnifiedRenderer() { shutdown(); }

UnifiedRenderer::UnifiedRenderer(UnifiedRenderer&&) noexcept = default;
UnifiedRenderer& UnifiedRenderer::operator=(UnifiedRenderer&&) noexcept = default;

bool UnifiedRenderer::initialize(void* windowHandle, uint32_t width, uint32_t height) {
    impl_->width = width;
    impl_->height = height;
    HWND hwnd = static_cast<HWND>(windowHandle);
    
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
    }
#endif
    
    ComPtr<IDXGIFactory6> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, 
            IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, 
                    IID_PPV_ARGS(&impl_->device)))) {
                std::wcout << L"[unified/dx12] GPU: " << desc.Description << std::endl;
                break;
            }
        }
    }
    if (!impl_->device) return false;
    
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    impl_->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&impl_->queue));
    
    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = width; scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForHwnd(impl_->queue.Get(), hwnd, &scDesc, nullptr, nullptr, &sc1);
    sc1.As(&impl_->swapchain);
    impl_->frameIndex = impl_->swapchain->GetCurrentBackBufferIndex();
    
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{}; rtvHeapDesc.NumDescriptors = 2; 
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    impl_->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&impl_->rtvHeap));
    impl_->rtvDescSize = impl_->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{}; dsvHeapDesc.NumDescriptors = 1; 
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    impl_->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&impl_->dsvHeap));
    
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{}; srvHeapDesc.NumDescriptors = 256; 
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    impl_->device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&impl_->srvHeap));
    impl_->srvDescSize = impl_->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++) {
        impl_->swapchain->GetBuffer(i, IID_PPV_ARGS(&impl_->renderTargets[i]));
        impl_->device->CreateRenderTargetView(impl_->renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += impl_->rtvDescSize;
    }
    
    for (UINT i = 0; i < 2; i++) {
        impl_->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
            IID_PPV_ARGS(&impl_->allocators[i]));
    }
    impl_->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
        impl_->allocators[0].Get(), nullptr, IID_PPV_ARGS(&impl_->cmdList));
    impl_->cmdList->Close();
    
    impl_->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&impl_->fence));
    impl_->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    
    impl_->createDepthBuffer();
    impl_->createDefaultTexture();
    impl_->createPipeline();
    impl_->createShadowMap();
    impl_->createPostProcessResources();
    
    return impl_->ready;
}

void UnifiedRenderer::shutdown() {
    if (!impl_) return;
    impl_->waitForGPU();
    // Unmap persistently mapped constant buffer
    if (impl_->constantBuffer && impl_->constantBufferMapped) {
        impl_->constantBuffer->Unmap(0, nullptr);
        impl_->constantBufferMapped = nullptr;
    }
    if (impl_->fenceEvent) CloseHandle(impl_->fenceEvent);
    impl_->meshStorage.clear();
    impl_->ready = false;
}

void UnifiedRenderer::resize(uint32_t width, uint32_t height) {
    if (!impl_ || width == 0 || height == 0) return;
    
    impl_->waitForGPU();
    impl_->width = width;
    impl_->height = height;
    
    for (UINT i = 0; i < 2; i++) impl_->renderTargets[i].Reset();
    impl_->depthBuffer.Reset();
    
    impl_->swapchain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    impl_->frameIndex = impl_->swapchain->GetCurrentBackBufferIndex();
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++) {
        impl_->swapchain->GetBuffer(i, IID_PPV_ARGS(&impl_->renderTargets[i]));
        impl_->device->CreateRenderTargetView(impl_->renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += impl_->rtvDescSize;
    }
    
    impl_->createDepthBuffer();
    impl_->resizePostProcessTargets();
}

RHIGPUMesh UnifiedRenderer::uploadMesh(const Mesh& mesh) {
    RHIGPUMesh gpu;
    gpu.name = mesh.name;  // Copy mesh name
    gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
    gpu.meshIndex = static_cast<uint32_t>(impl_->meshStorage.size());
    memcpy(gpu.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
    gpu.metallic = mesh.metallic;
    gpu.roughness = mesh.roughness;
    gpu.materialName = mesh.materialName;
    gpu.diffuseTexturePath = mesh.diffuseTexture.path;
    gpu.normalTexturePath = mesh.normalTexture.path;
    gpu.specularTexturePath = mesh.specularTexture.path;
    
    // Build original edges from originalFaces (for quad/ngon wireframe display)
    if (mesh.hasOriginalFaces && !mesh.originalFaces.empty()) {
        std::set<std::pair<uint32_t, uint32_t>> uniqueEdges;
        
        for (const auto& face : mesh.originalFaces) {
            for (size_t i = 0; i < face.vertexIndices.size(); i++) {
                uint32_t v0 = face.vertexIndices[i];
                uint32_t v1 = face.vertexIndices[(i + 1) % face.vertexIndices.size()];
                
                // Normalize edge direction for deduplication
                if (v0 > v1) std::swap(v0, v1);
                uniqueEdges.insert({v0, v1});
            }
        }
        
        for (const auto& edge : uniqueEdges) {
            gpu.originalEdges.push_back({edge.first, edge.second});
        }
        gpu.hasOriginalEdges = !gpu.originalEdges.empty();
        
        if (gpu.hasOriginalEdges) {
            std::cout << "[mesh] Built " << gpu.originalEdges.size() 
                      << " original edges from " << mesh.originalFaces.size() 
                      << " original faces" << std::endl;
        }
    }
    
    DX12MeshData dx12Mesh;
    dx12Mesh.indexCount = gpu.indexCount;
    memcpy(dx12Mesh.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
    dx12Mesh.metallic = mesh.metallic;
    dx12Mesh.roughness = mesh.roughness;
    
    const UINT vbSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));
    const UINT ibSize = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
    
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = vbSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.vertexBuffer));
    void* mapped;
    dx12Mesh.vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, mesh.vertices.data(), vbSize);
    dx12Mesh.vertexBuffer->Unmap(0, nullptr);
    dx12Mesh.vbv.BufferLocation = dx12Mesh.vertexBuffer->GetGPUVirtualAddress();
    dx12Mesh.vbv.SizeInBytes = vbSize;
    dx12Mesh.vbv.StrideInBytes = sizeof(Vertex);
    
    bufDesc.Width = ibSize;
    impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.indexBuffer));
    dx12Mesh.indexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, mesh.indices.data(), ibSize);
    dx12Mesh.indexBuffer->Unmap(0, nullptr);
    dx12Mesh.ibv.BufferLocation = dx12Mesh.indexBuffer->GetGPUVirtualAddress();
    dx12Mesh.ibv.SizeInBytes = ibSize;
    dx12Mesh.ibv.Format = DXGI_FORMAT_R32_UINT;
    
    // Build GPU edge index buffer for original edges (enables zero-CPU wireframe rendering)
    if (gpu.hasOriginalEdges) {
        impl_->buildEdgeIndexBuffer(dx12Mesh, gpu.originalEdges);
    }
    
    dx12Mesh.diffuseTexture = impl_->uploadTexture(mesh.diffuseTexture, dx12Mesh.diffuseSrvIndex);
    gpu.hasDiffuseTexture = !mesh.diffuseTexture.pixels.empty();
    dx12Mesh.normalTexture = impl_->uploadTexture(mesh.normalTexture, dx12Mesh.normalSrvIndex);
    gpu.hasNormalTexture = !mesh.normalTexture.pixels.empty();
    dx12Mesh.specularTexture = impl_->uploadTexture(mesh.specularTexture, dx12Mesh.specularSrvIndex);
    gpu.hasSpecularTexture = !mesh.specularTexture.pixels.empty();
    
    impl_->meshStorage.push_back(std::move(dx12Mesh));
    
    return gpu;
}

bool UnifiedRenderer::loadModel(const std::string& path, RHILoadedModel& outModel) {
    std::cout << "[unified/dx12] Loading model: " << path << std::endl;
    
    auto result = load_model(path);
    if (!result) return false;
    
    outModel.meshes.clear();
    outModel.textureCount = 0;
    outModel.meshStorageStartIndex = impl_->meshStorage.size();
    
    for (const auto& mesh : result->meshes) {
        outModel.meshes.push_back(uploadMesh(mesh));
        if (!mesh.diffuseTexture.pixels.empty()) outModel.textureCount++;
    }
    
    outModel.center[0] = (result->minBounds[0] + result->maxBounds[0]) / 2.0f;
    outModel.center[1] = (result->minBounds[1] + result->maxBounds[1]) / 2.0f;
    outModel.center[2] = (result->minBounds[2] + result->maxBounds[2]) / 2.0f;
    
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];
    float dz = result->maxBounds[2] - result->minBounds[2];
    outModel.radius = sqrtf(dx*dx + dy*dy + dz*dz) / 2.0f;
    
    outModel.name = path.substr(path.find_last_of("/\\") + 1);
    outModel.totalVerts = result->totalVertices;
    outModel.totalTris = result->totalTriangles;
    
    std::cout << "[unified/dx12] Model loaded: " << outModel.meshes.size() << " meshes" << std::endl;
    return true;
}

bool UnifiedRenderer::loadModelAsync(const std::string& path, RHILoadedModel& outModel) {
    std::cout << "[unified/dx12] Loading model (progressive): " << path << std::endl;
    
    // Use load_model_with_animations to get skinning data for edit mode wireframe
    auto result = load_model_with_animations(path);
    if (!result) return false;
    
    outModel.meshes.clear();
    outModel.textureCount = 0;
    outModel.meshStorageStartIndex = impl_->meshStorage.size();
    
    // Count total textures for progress tracking
    size_t totalTextures = 0;
    for (const auto& mesh : result->meshes) {
        if (!mesh.diffuseTexture.pixels.empty()) totalTextures++;
        if (!mesh.normalTexture.pixels.empty()) totalTextures++;
        if (!mesh.specularTexture.pixels.empty()) totalTextures++;
    }
    impl_->asyncTexturesLoaded = 0;
    
    for (size_t mi = 0; mi < result->meshes.size(); mi++) {
        const auto& mesh = result->meshes[mi];
        RHIGPUMesh gpu;
        gpu.name = mesh.name;  // Copy mesh name
        gpu.materialName = mesh.materialName;
        gpu.diffuseTexturePath = mesh.diffuseTexture.path;
        gpu.normalTexturePath = mesh.normalTexture.path;
        gpu.specularTexturePath = mesh.specularTexture.path;
        std::cout << "[async] Mesh " << mi << " name: '" << mesh.name 
                  << "' verts: " << mesh.vertices.size() 
                  << " hasSkinning: " << (mesh.hasSkeleton ? "yes" : "no") << std::endl;
        gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
        gpu.meshIndex = static_cast<uint32_t>(impl_->meshStorage.size());
        memcpy(gpu.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
        gpu.metallic = mesh.metallic;
        gpu.roughness = mesh.roughness;
        
        // Build original edges from originalFaces (for quad/ngon wireframe display)
        if (mesh.hasOriginalFaces && !mesh.originalFaces.empty()) {
            std::set<std::pair<uint32_t, uint32_t>> uniqueEdges;
            
            for (const auto& face : mesh.originalFaces) {
                for (size_t i = 0; i < face.vertexIndices.size(); i++) {
                    uint32_t v0 = face.vertexIndices[i];
                    uint32_t v1 = face.vertexIndices[(i + 1) % face.vertexIndices.size()];
                    
                    // Normalize edge direction for deduplication
                    if (v0 > v1) std::swap(v0, v1);
                    uniqueEdges.insert({v0, v1});
                }
            }
            
            for (const auto& edge : uniqueEdges) {
                gpu.originalEdges.push_back({edge.first, edge.second});
            }
            gpu.hasOriginalEdges = !gpu.originalEdges.empty();
            
            // Also store original faces for edit mode
            for (const auto& srcFace : mesh.originalFaces) {
                RHIGPUMesh::GPUOriginalFace gpuFace;
                gpuFace.vertexIndices = srcFace.vertexIndices;
                gpu.originalFaces.push_back(gpuFace);
            }
            gpu.hasOriginalFaces = !gpu.originalFaces.empty();
            
            if (gpu.hasOriginalEdges) {
                std::cout << "[async] Built " << gpu.originalEdges.size() 
                          << " original edges, " << gpu.originalFaces.size()
                          << " original faces for mesh " << gpu.meshIndex << std::endl;
            }
        }
        
        // Copy skinned vertices for CPU skinning in edit mode
        if (mesh.hasSkeleton && !mesh.skinnedVertices.empty()) {
            gpu.skinnedVertices = mesh.skinnedVertices;
            gpu.hasSkinning = true;
        }
        
        DX12MeshData dx12Mesh;
        dx12Mesh.indexCount = gpu.indexCount;
        memcpy(dx12Mesh.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
        dx12Mesh.metallic = mesh.metallic;
        dx12Mesh.roughness = mesh.roughness;
        
        // Upload vertex/index buffers immediately (fast)
        const UINT vbSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));
        const UINT ibSize = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = vbSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.vertexBuffer));
        void* mapped;
        dx12Mesh.vertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, mesh.vertices.data(), vbSize);
        dx12Mesh.vertexBuffer->Unmap(0, nullptr);
        dx12Mesh.vbv.BufferLocation = dx12Mesh.vertexBuffer->GetGPUVirtualAddress();
        dx12Mesh.vbv.SizeInBytes = vbSize;
        dx12Mesh.vbv.StrideInBytes = sizeof(Vertex);
        
        bufDesc.Width = ibSize;
        impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.indexBuffer));
        dx12Mesh.indexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, mesh.indices.data(), ibSize);
        dx12Mesh.indexBuffer->Unmap(0, nullptr);
        dx12Mesh.ibv.BufferLocation = dx12Mesh.indexBuffer->GetGPUVirtualAddress();
        dx12Mesh.ibv.SizeInBytes = ibSize;
        dx12Mesh.ibv.Format = DXGI_FORMAT_R32_UINT;
        
        // Progressive loading: use default textures initially, queue for background upload
        dx12Mesh.diffuseTexture = impl_->defaultTexture;
        dx12Mesh.diffuseSrvIndex = impl_->defaultTextureSrvIndex;
        dx12Mesh.normalTexture = impl_->defaultTexture;
        dx12Mesh.normalSrvIndex = impl_->defaultTextureSrvIndex;
        dx12Mesh.specularTexture = impl_->defaultTexture;
        dx12Mesh.specularSrvIndex = impl_->defaultTextureSrvIndex;
        
        uint32_t meshIdx = static_cast<uint32_t>(impl_->meshStorage.size());
        
        // Build GPU edge index buffer for original edges
        if (gpu.hasOriginalEdges) {
            impl_->buildEdgeIndexBuffer(dx12Mesh, gpu.originalEdges);
        }
        
        // Queue textures for progressive upload (will be uploaded in processAsyncTextures)
        if (!mesh.diffuseTexture.pixels.empty()) {
            impl_->textureUploadQueue.push({meshIdx, 0, mesh.diffuseTexture});
            gpu.hasDiffuseTexture = true;
            outModel.textureCount++;
        }
        if (!mesh.normalTexture.pixels.empty()) {
            impl_->textureUploadQueue.push({meshIdx, 1, mesh.normalTexture});
            gpu.hasNormalTexture = true;
        }
        if (!mesh.specularTexture.pixels.empty()) {
            impl_->textureUploadQueue.push({meshIdx, 2, mesh.specularTexture});
            gpu.hasSpecularTexture = true;
        }
        
        impl_->meshStorage.push_back(std::move(dx12Mesh));
        outModel.meshes.push_back(gpu);
    }
    
    outModel.center[0] = (result->minBounds[0] + result->maxBounds[0]) / 2.0f;
    outModel.center[1] = (result->minBounds[1] + result->maxBounds[1]) / 2.0f;
    outModel.center[2] = (result->minBounds[2] + result->maxBounds[2]) / 2.0f;
    
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];
    float dz = result->maxBounds[2] - result->minBounds[2];
    outModel.radius = sqrtf(dx*dx + dy*dy + dz*dz) / 2.0f;
    
    outModel.name = path.substr(path.find_last_of("/\\") + 1);
    outModel.totalVerts = result->totalVertices;
    outModel.totalTris = result->totalTriangles;
    
    std::cout << "[unified/dx12] Model loaded: " << outModel.meshes.size() << " meshes, "
              << outModel.textureCount << " textures" << std::endl;
    return true;
}

bool UnifiedRenderer::loadModelAsync(const std::string& path, RHILoadedModel& outModel, Model& outSourceModel) {
    std::cout << "[unified/dx12] Loading model with source data: " << path << std::endl;
    
    // Use load_model_with_animations to get full data including skeleton/animations
    auto result = load_model_with_animations(path);
    if (!result) return false;
    
    outModel.meshes.clear();
    outModel.textureCount = 0;
    outModel.meshStorageStartIndex = impl_->meshStorage.size();
    
    // Count total textures for progress tracking
    size_t totalTextures = 0;
    for (const auto& mesh : result->meshes) {
        if (!mesh.diffuseTexture.pixels.empty()) totalTextures++;
        if (!mesh.normalTexture.pixels.empty()) totalTextures++;
        if (!mesh.specularTexture.pixels.empty()) totalTextures++;
    }
    impl_->asyncTexturesLoaded = 0;
    
    for (size_t mi = 0; mi < result->meshes.size(); mi++) {
        const auto& mesh = result->meshes[mi];
        RHIGPUMesh gpu;
        gpu.name = mesh.name;
        gpu.materialName = mesh.materialName;
        gpu.diffuseTexturePath = mesh.diffuseTexture.path;
        gpu.normalTexturePath = mesh.normalTexture.path;
        gpu.specularTexturePath = mesh.specularTexture.path;
        gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
        gpu.meshIndex = static_cast<uint32_t>(impl_->meshStorage.size());
        memcpy(gpu.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
        gpu.metallic = mesh.metallic;
        gpu.roughness = mesh.roughness;
        
        // Build original edges from originalFaces
        if (mesh.hasOriginalFaces && !mesh.originalFaces.empty()) {
            std::set<std::pair<uint32_t, uint32_t>> uniqueEdges;
            for (const auto& face : mesh.originalFaces) {
                for (size_t i = 0; i < face.vertexIndices.size(); i++) {
                    uint32_t v0 = face.vertexIndices[i];
                    uint32_t v1 = face.vertexIndices[(i + 1) % face.vertexIndices.size()];
                    if (v0 > v1) std::swap(v0, v1);
                    uniqueEdges.insert({v0, v1});
                }
            }
            for (const auto& edge : uniqueEdges) {
                gpu.originalEdges.push_back({edge.first, edge.second});
            }
            gpu.hasOriginalEdges = !gpu.originalEdges.empty();
            
            for (const auto& srcFace : mesh.originalFaces) {
                RHIGPUMesh::GPUOriginalFace gpuFace;
                gpuFace.vertexIndices = srcFace.vertexIndices;
                gpu.originalFaces.push_back(gpuFace);
            }
            gpu.hasOriginalFaces = !gpu.originalFaces.empty();
        }
        
        // Copy skinned vertices for CPU skinning in edit mode
        if (mesh.hasSkeleton && !mesh.skinnedVertices.empty()) {
            gpu.skinnedVertices = mesh.skinnedVertices;
            gpu.hasSkinning = true;
        }
        
        DX12MeshData dx12Mesh;
        dx12Mesh.indexCount = gpu.indexCount;
        memcpy(dx12Mesh.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
        dx12Mesh.metallic = mesh.metallic;
        dx12Mesh.roughness = mesh.roughness;
        
        // Upload vertex/index buffers
        const UINT vbSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));
        const UINT ibSize = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = vbSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.vertexBuffer));
        void* mapped;
        dx12Mesh.vertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, mesh.vertices.data(), vbSize);
        dx12Mesh.vertexBuffer->Unmap(0, nullptr);
        dx12Mesh.vbv.BufferLocation = dx12Mesh.vertexBuffer->GetGPUVirtualAddress();
        dx12Mesh.vbv.SizeInBytes = vbSize;
        dx12Mesh.vbv.StrideInBytes = sizeof(Vertex);
        
        bufDesc.Width = ibSize;
        impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.indexBuffer));
        dx12Mesh.indexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, mesh.indices.data(), ibSize);
        dx12Mesh.indexBuffer->Unmap(0, nullptr);
        dx12Mesh.ibv.BufferLocation = dx12Mesh.indexBuffer->GetGPUVirtualAddress();
        dx12Mesh.ibv.SizeInBytes = ibSize;
        dx12Mesh.ibv.Format = DXGI_FORMAT_R32_UINT;
        
        // Use default textures initially, queue for background upload
        dx12Mesh.diffuseTexture = impl_->defaultTexture;
        dx12Mesh.diffuseSrvIndex = impl_->defaultTextureSrvIndex;
        dx12Mesh.normalTexture = impl_->defaultTexture;
        dx12Mesh.normalSrvIndex = impl_->defaultTextureSrvIndex;
        dx12Mesh.specularTexture = impl_->defaultTexture;
        dx12Mesh.specularSrvIndex = impl_->defaultTextureSrvIndex;
        
        uint32_t meshIdx = static_cast<uint32_t>(impl_->meshStorage.size());
        
        // Build GPU edge index buffer for original edges
        if (gpu.hasOriginalEdges) {
            impl_->buildEdgeIndexBuffer(dx12Mesh, gpu.originalEdges);
        }
        
        // Queue textures for progressive upload
        if (!mesh.diffuseTexture.pixels.empty()) {
            impl_->textureUploadQueue.push({meshIdx, 0, mesh.diffuseTexture});
            gpu.hasDiffuseTexture = true;
            outModel.textureCount++;
        }
        if (!mesh.normalTexture.pixels.empty()) {
            impl_->textureUploadQueue.push({meshIdx, 1, mesh.normalTexture});
            gpu.hasNormalTexture = true;
        }
        if (!mesh.specularTexture.pixels.empty()) {
            impl_->textureUploadQueue.push({meshIdx, 2, mesh.specularTexture});
            gpu.hasSpecularTexture = true;
        }
        
        impl_->meshStorage.push_back(std::move(dx12Mesh));
        outModel.meshes.push_back(gpu);
    }
    
    outModel.center[0] = (result->minBounds[0] + result->maxBounds[0]) / 2.0f;
    outModel.center[1] = (result->minBounds[1] + result->maxBounds[1]) / 2.0f;
    outModel.center[2] = (result->minBounds[2] + result->maxBounds[2]) / 2.0f;
    
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];
    float dz = result->maxBounds[2] - result->minBounds[2];
    outModel.radius = sqrtf(dx*dx + dy*dy + dz*dz) / 2.0f;
    
    outModel.name = path.substr(path.find_last_of("/\\") + 1);
    outModel.totalVerts = result->totalVertices;
    outModel.totalTris = result->totalTriangles;
    
    // Move source model data to output (skeleton, animations, etc.)
    outSourceModel = std::move(*result);
    
    std::cout << "[unified/dx12] Model loaded with source: " << outModel.meshes.size() << " meshes" << std::endl;
    return true;
}

void UnifiedRenderer::processAsyncTextures() {
    // Process progressive texture upload queue (limit uploads per frame for smooth rendering)
    const int maxUploadsPerFrame = 2;  // Upload up to 2 textures per frame
    int uploadsThisFrame = 0;
    
    while (!impl_->textureUploadQueue.empty() && uploadsThisFrame < maxUploadsPerFrame) {
        auto job = std::move(impl_->textureUploadQueue.front());
        impl_->textureUploadQueue.pop();
        
        if (job.meshIndex >= impl_->meshStorage.size()) continue;
        if (job.data.pixels.empty()) continue;
        
            UINT srvIndex;
        ComPtr<ID3D12Resource> texture = impl_->uploadTexture(job.data, srvIndex);
        
        DX12MeshData& mesh = impl_->meshStorage[job.meshIndex];
        const char* slotName = job.slot == 0 ? "diffuse" : (job.slot == 1 ? "normal" : "specular");
        
        switch (job.slot) {
                case 0:  // diffuse
                    mesh.diffuseTexture = texture;
                    mesh.diffuseSrvIndex = srvIndex;
                    break;
                case 1:  // normal
                    mesh.normalTexture = texture;
                    mesh.normalSrvIndex = srvIndex;
                    break;
                case 2:  // specular
                    mesh.specularTexture = texture;
                    mesh.specularSrvIndex = srvIndex;
                    break;
            }
        
            impl_->asyncTexturesLoaded++;
        uploadsThisFrame++;
        
        std::cout << "[progressive] Uploaded " << slotName << " (" 
                  << job.data.width << "x" << job.data.height << ") - "
                  << impl_->textureUploadQueue.size() << " remaining" << std::endl;
    }
}

float UnifiedRenderer::getAsyncLoadProgress() const {
    size_t pending = impl_->textureUploadQueue.size();
    size_t total = pending + impl_->asyncTexturesLoaded;
    if (total == 0) return 1.0f;
    return static_cast<float>(impl_->asyncTexturesLoaded) / static_cast<float>(total);
}

void UnifiedRenderer::beginFrame() {
    // Update frame time for animated effects
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    impl_->frameTime += dt;
    
    impl_->frameIndex = impl_->swapchain->GetCurrentBackBufferIndex();
    impl_->currentDrawIndex = impl_->frameIndex * Impl::kMaxDrawsPerFrame;  // Reset ring buffer offset
    impl_->allocators[impl_->frameIndex]->Reset();
    impl_->cmdList->Reset(impl_->allocators[impl_->frameIndex].Get(), nullptr);
    
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    // Transition swapchain to render target (needed for ImGui later)
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = impl_->renderTargets[impl_->frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    impl_->cmdList->ResourceBarrier(1, &barrier);
    
    float clearColor[] = {0.05f, 0.05f, 0.08f, 1.0f};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = impl_->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    
    // If post-processing is enabled and ready, render scene to HDR target
    if (impl_->postProcessEnabled && impl_->postProcessReady && impl_->hdrRenderTarget) {
        UINT ppRtvDescSize = impl_->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE hdrRtvHandle = impl_->postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
        impl_->cmdList->OMSetRenderTargets(1, &hdrRtvHandle, FALSE, &dsvHandle);
        impl_->cmdList->ClearRenderTargetView(hdrRtvHandle, clearColor, 0, nullptr);
    } else {
        // Render directly to swapchain
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += impl_->frameIndex * impl_->rtvDescSize;
        impl_->cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        impl_->cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    }
    
    impl_->cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    D3D12_VIEWPORT vp{}; vp.Width = (float)impl_->width; vp.Height = (float)impl_->height; vp.MaxDepth = 1.0f;
    D3D12_RECT sr{}; sr.right = impl_->width; sr.bottom = impl_->height;
    impl_->cmdList->RSSetViewports(1, &vp);
    impl_->cmdList->RSSetScissorRects(1, &sr);
}

void UnifiedRenderer::render(const RHILoadedModel& model, float time, float camDistMultiplier) {
    RHICameraParams cam;
    cam.yaw = time * 0.5f;
    cam.pitch = 0.3f;
    cam.distance = camDistMultiplier;
    render(model, cam);
}

void UnifiedRenderer::render(const RHILoadedModel& model, const RHICameraParams& camera) {
    if (!impl_->ready || model.meshes.empty()) return;
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->pipelineState.Get());
    
    float target[3] = {
        model.center[0] + camera.targetOffsetX,
        model.center[1] + camera.targetOffsetY,
        model.center[2] + camera.targetOffsetZ
    };
    
    float camDist = model.radius * 2.5f * camera.distance;
    float eye[3] = {
        target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist,
        target[1] + sinf(camera.pitch) * camDist,
        target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist
    };
    
    float up[3] = {0, 1, 0};
    float world[16], view[16], proj[16], wvp[16];
    math::identity(world);
    math::lookAt(view, eye, target, up);
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(proj, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = eye[0];
    impl_->constants.cameraPosAndMetal[1] = eye[1];
    impl_->constants.cameraPosAndMetal[2] = eye[2];
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        
        // Use meshIndex to find the correct DX12MeshData
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        impl_->constants.cameraPosAndMetal[3] = gpu.metallic;
        impl_->constants.baseColorAndRough[0] = gpu.baseColor[0];
        impl_->constants.baseColorAndRough[1] = gpu.baseColor[1];
        impl_->constants.baseColorAndRough[2] = gpu.baseColor[2];
        impl_->constants.baseColorAndRough[3] = gpu.roughness;
        
        // Write to ring buffer without Map/Unmap overhead
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        srv = srvGpuStart; srv.ptr += dx12Mesh.diffuseSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.normalSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.specularSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        // Shadow map (t3)
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        // IBL textures (t4, t5, t6)
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderGrid(const RHICameraParams& camera, float modelRadius) {
    if (!impl_->ready || !impl_->gridReady) return;
    
    float target[3] = {camera.targetOffsetX, camera.targetOffsetY, camera.targetOffsetZ};
    float camDist = modelRadius * 2.5f * camera.distance;
    float eye[3] = {
        target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist,
        target[1] + sinf(camera.pitch) * camDist,
        target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist
    };
    
    float up[3] = {0, 1, 0};
    float world[16], view[16], proj[16], wvp[16];
    
    math::lookAt(view, eye, target, up);
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(proj, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->linePipelineState.Get());
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    
    // Grid
    math::identity(world);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    // Write to ring buffer (no Map/Unmap overhead)
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->gridVbv);
    impl_->cmdList->DrawInstanced(impl_->gridVertexCount, 1, 0, 0);
    
    // Axes
    float axisScale = std::max(camDist * 0.3f, modelRadius * 1.5f);
    axisScale = std::max(axisScale, 10.0f);
    
    math::scale(world, axisScale, axisScale, axisScale);
    
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    // Write to ring buffer
    drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->axisVbv);
    impl_->cmdList->DrawInstanced(impl_->axisVertexCount, 1, 0, 0);
}

void UnifiedRenderer::setCamera(const RHICameraParams& camera, float sceneRadius) {
    if (!impl_->ready) return;
    
    float target[3] = {camera.targetOffsetX, camera.targetOffsetY, camera.targetOffsetZ};
    float camDist = sceneRadius * 2.5f * camera.distance;
    float eye[3] = {
        target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist,
        target[1] + sinf(camera.pitch) * camDist,
        target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist
    };
    
    float up[3] = {0, 1, 0};
    math::lookAt(impl_->viewMatrix, eye, target, up);
    
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(impl_->projMatrix, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    
    impl_->cameraPos[0] = eye[0];
    impl_->cameraPos[1] = eye[1];
    impl_->cameraPos[2] = eye[2];
    impl_->cameraSet = true;
}

void UnifiedRenderer::getViewMatrix(float* outMatrix16) const {
    if (impl_ && impl_->cameraSet) {
        memcpy(outMatrix16, impl_->viewMatrix, sizeof(float) * 16);
    }
}

void UnifiedRenderer::getProjectionMatrix(float* outMatrix16) const {
    if (impl_ && impl_->cameraSet) {
        memcpy(outMatrix16, impl_->projMatrix, sizeof(float) * 16);
    }
}

void UnifiedRenderer::renderModel(const RHILoadedModel& model, const float* worldMatrix) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->pipelineState.Get());
    
    // Set descriptor heaps for SRV access
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    // Calculate worldViewProj
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    
    // Shadow parameters
    impl_->constants.shadowParams[0] = impl_->shadowSettings.bias;
    impl_->constants.shadowParams[1] = impl_->shadowSettings.normalBias;
    impl_->constants.shadowParams[2] = impl_->shadowSettings.softness;
    impl_->constants.shadowParams[3] = impl_->shadowSettings.enabled && impl_->shadowMapReady ? 1.0f : 0.0f;
    
    // IBL parameters
    impl_->constants.iblParams[0] = impl_->iblSettings.intensity;
    impl_->constants.iblParams[1] = impl_->iblSettings.rotation;
    impl_->constants.iblParams[2] = (float)(impl_->iblSettings.prefilteredMips - 1);
    impl_->constants.iblParams[3] = impl_->iblSettings.enabled && impl_->iblReady ? 1.0f : 0.0f;
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        impl_->constants.cameraPosAndMetal[3] = gpu.metallic;
        impl_->constants.baseColorAndRough[0] = gpu.baseColor[0];
        impl_->constants.baseColorAndRough[1] = gpu.baseColor[1];
        impl_->constants.baseColorAndRough[2] = gpu.baseColor[2];
        impl_->constants.baseColorAndRough[3] = gpu.roughness;
        
        // Write to ring buffer
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        srv = srvGpuStart; srv.ptr += dx12Mesh.diffuseSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.normalSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.specularSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        // Shadow map (t3)
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        // IBL textures (t4, t5, t6)
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderModelDepthOnly(const RHILoadedModel& model, const float* worldMatrix) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (!impl_->depthOnlyPipelineState) return;
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->depthOnlyPipelineState.Get());
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    // WVP — same for ALL meshes, set once
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    // Bind dummy textures ONCE (root signature requires them even without PS)
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE defaultSrv = srvGpuStart;
    defaultSrv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
    for (int slot = 1; slot <= 7; ++slot) {
        impl_->cmdList->SetGraphicsRootDescriptorTable(slot, defaultSrv);
    }
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Draw each mesh — only VB/IB changes per mesh, everything else is shared
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderSkinnedModel(const RHILoadedModel& model, const float* worldMatrix, const float* boneMatrices) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (!impl_->skinnedPipelineReady || !impl_->skinnedPipelineState) {
        // Fall back to non-skinned rendering
        renderModel(model, worldMatrix);
        return;
    }
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->skinnedRootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->skinnedPipelineState.Get());
    
    // Set descriptor heaps for SRV access
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    // Calculate worldViewProj
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    
    // Shadow parameters
    impl_->constants.shadowParams[0] = impl_->shadowSettings.bias;
    impl_->constants.shadowParams[1] = impl_->shadowSettings.normalBias;
    impl_->constants.shadowParams[2] = impl_->shadowSettings.softness;
    impl_->constants.shadowParams[3] = impl_->shadowSettings.enabled && impl_->shadowMapReady ? 1.0f : 0.0f;
    
    // IBL parameters
    impl_->constants.iblParams[0] = impl_->iblSettings.intensity;
    impl_->constants.iblParams[1] = impl_->iblSettings.rotation;
    impl_->constants.iblParams[2] = (float)(impl_->iblSettings.prefilteredMips - 1);
    impl_->constants.iblParams[3] = impl_->iblSettings.enabled && impl_->iblReady ? 1.0f : 0.0f;
    
    // Update bone matrices buffer
    memcpy(impl_->boneBufferMapped, boneMatrices, impl_->kMaxBones * 64);
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        impl_->constants.cameraPosAndMetal[3] = gpu.metallic;
        impl_->constants.baseColorAndRough[0] = gpu.baseColor[0];
        impl_->constants.baseColorAndRough[1] = gpu.baseColor[1];
        impl_->constants.baseColorAndRough[2] = gpu.baseColor[2];
        impl_->constants.baseColorAndRough[3] = gpu.roughness;
        
        // Write scene constants to ring buffer
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);  // Scene constants (b0)
        impl_->cmdList->SetGraphicsRootConstantBufferView(1, impl_->boneBuffer->GetGPUVirtualAddress());  // Bone matrices (b1)
        impl_->currentDrawIndex++;
        
        // Textures (offset by 2 in root params)
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        srv = srvGpuStart; srv.ptr += dx12Mesh.diffuseSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.normalSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.specularSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(8, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderModelOutline(const RHILoadedModel& model, const float* worldMatrix, const float* outlineColor) {
    // TODO: Implement outline rendering (stencil-based or post-process)
    // For now, just render normally - outline requires additional render passes
    renderModel(model, worldMatrix);
}

void UnifiedRenderer::renderModelWithHighlight(const RHILoadedModel& model, const float* worldMatrix, 
                                                int highlightMeshIndex, const float* highlightColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (!impl_->unlitWireframePipelineState) return;  // Need unlit pipeline
    
    // Use UNLIT WIREFRAME pipeline - guarantees pure color output (no lighting!)
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->unlitWireframePipelineState.Get());  // UNLIT!
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    
    impl_->constants.shadowParams[0] = impl_->shadowSettings.bias;
    impl_->constants.shadowParams[1] = impl_->shadowSettings.normalBias;
    impl_->constants.shadowParams[2] = impl_->shadowSettings.softness;
    impl_->constants.shadowParams[3] = impl_->shadowSettings.enabled && impl_->shadowMapReady ? 1.0f : 0.0f;
    
    impl_->constants.iblParams[0] = impl_->iblSettings.intensity;
    impl_->constants.iblParams[1] = impl_->iblSettings.rotation;
    impl_->constants.iblParams[2] = static_cast<float>(impl_->iblSettings.prefilteredMips - 1);
    impl_->constants.iblParams[3] = impl_->iblSettings.enabled && impl_->iblReady ? 1.0f : 0.0f;
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        // Check if this mesh is highlighted - use UNLIT mode for all!
        bool isHighlighted = (static_cast<int>(i) == highlightMeshIndex);
        
        if (isHighlighted && highlightColor) {
            // Highlighted mesh = ORANGE
            impl_->constants.baseColorAndRough[0] = 1.0f;   // R
            impl_->constants.baseColorAndRough[1] = 0.5f;   // G  
            impl_->constants.baseColorAndRough[2] = 0.0f;   // B
        } else {
            // Other meshes = GRAY
            impl_->constants.baseColorAndRough[0] = 0.4f;
            impl_->constants.baseColorAndRough[1] = 0.4f;
            impl_->constants.baseColorAndRough[2] = 0.4f;
        }
        // UNLIT mode: roughness = -1.0 triggers direct color output in shader
        impl_->constants.baseColorAndRough[3] = -1.0f;
        impl_->constants.cameraPosAndMetal[3] = 0.0f;
        impl_->constants.lightDirAndFlags[3] = 0;  // No textures
        
        // Write to ring buffer
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        srv = srvGpuStart; srv.ptr += dx12Mesh.diffuseSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.normalSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        srv = srvGpuStart; srv.ptr += dx12Mesh.specularSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
        
        if (isHighlighted) {
            printf("[HIGHLIGHT] Drew %u indices for mesh %d (orange)\n", dx12Mesh.indexCount, highlightMeshIndex);
        }
    }
}

void UnifiedRenderer::renderMeshHighlight(const RHILoadedModel& model, const float* worldMatrix, 
                                           int meshIndex, const float* highlightColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    if (!impl_->highlightPipelineState) return;
    
    const auto& gpu = model.meshes[meshIndex];
    if (gpu.meshIndex >= impl_->meshStorage.size()) return;
    const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
    
    // Use highlight pipeline (wireframe, always visible)
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->highlightPipelineState.Get());
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    
    // FORCE solid orange color - disable ALL lighting/textures
    // Use a very high "emissive" style by setting base color bright and disabling everything else
    impl_->constants.baseColorAndRough[0] = highlightColor[0] * 2.0f;  // Boost brightness
    impl_->constants.baseColorAndRough[1] = highlightColor[1] * 2.0f;
    impl_->constants.baseColorAndRough[2] = highlightColor[2] * 2.0f;
    impl_->constants.baseColorAndRough[3] = 1.0f;  // Max roughness = diffuse
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    impl_->constants.cameraPosAndMetal[3] = 0.0f;  // Non-metallic
    // Point light directly at camera for uniform brightness
    impl_->constants.lightDirAndFlags[0] = 0.0f;
    impl_->constants.lightDirAndFlags[1] = 0.0f;
    impl_->constants.lightDirAndFlags[2] = 1.0f;
    impl_->constants.lightDirAndFlags[3] = 0;  // NO textures!
    impl_->constants.shadowParams[3] = 0;  // No shadows
    impl_->constants.iblParams[3] = 0;  // No IBL
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    // Set dummy white textures
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srv = srvGpuStart;
    srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
    impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
    impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
    srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
    srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
    srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
    srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
    
    // Draw the selected mesh as wireframe
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
    impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
    impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
}

void UnifiedRenderer::renderEditModeWireframe(const RHILoadedModel& model, const float* worldMatrix,
                                                int selectedMeshIndex, const float* highlightColor, const float* grayColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (!impl_->highlightPipelineState) return;
    
    // Use highlight pipeline (wireframe, always visible, no depth fighting)
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->highlightPipelineState.Get());
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    // Render ALL meshes as wireframe - selected=highlight color, others=gray
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        // Set color based on selection
        bool isSelected = (static_cast<int>(i) == selectedMeshIndex);
        const float* color = isSelected ? highlightColor : grayColor;
        
        impl_->constants.baseColorAndRough[0] = color[0];
        impl_->constants.baseColorAndRough[1] = color[1];
        impl_->constants.baseColorAndRough[2] = color[2];
        impl_->constants.baseColorAndRough[3] = -1.0f;  // Special value: UNLIT mode!
        impl_->constants.cameraPosAndMetal[3] = 0.0f;
        impl_->constants.lightDirAndFlags[3] = 0;  // No textures
        impl_->constants.shadowParams[3] = 0;
        impl_->constants.iblParams[3] = 0;
        
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        // Set dummy textures
        D3D12_GPU_DESCRIPTOR_HANDLE srv = srvGpuStart;
        srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderModelWireframeUnlit(const RHILoadedModel& model, const float* worldMatrix,
                                                 int highlightMeshIndex, const float* highlightColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (!impl_->unlitWireframePipelineState) {
        // Fallback: use regular wireframe if unlit not available
        renderModelWireframe(model, worldMatrix, highlightColor);
        return;
    }
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->unlitWireframePipelineState.Get());
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // 绑定默认纹理（root signature 需要）
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    UINT srvSize = impl_->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_GPU_DESCRIPTOR_HANDLE defaultHandle = srvGpuStart;
    defaultHandle.ptr += impl_->defaultTextureSrvIndex * srvSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(1, defaultHandle);
    impl_->cmdList->SetGraphicsRootDescriptorTable(2, defaultHandle);
    impl_->cmdList->SetGraphicsRootDescriptorTable(3, defaultHandle);
    impl_->cmdList->SetGraphicsRootDescriptorTable(4, defaultHandle);
    impl_->cmdList->SetGraphicsRootDescriptorTable(5, defaultHandle);
    impl_->cmdList->SetGraphicsRootDescriptorTable(6, defaultHandle);
    impl_->cmdList->SetGraphicsRootDescriptorTable(7, defaultHandle);
    
    // Render ALL meshes - highlight one with orange, others with gray
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        // Set color: orange for highlighted mesh, gray for others
        if (static_cast<int>(i) == highlightMeshIndex) {
            impl_->constants.baseColorAndRough[0] = highlightColor[0];  // Orange
            impl_->constants.baseColorAndRough[1] = highlightColor[1];
            impl_->constants.baseColorAndRough[2] = highlightColor[2];
        } else {
            impl_->constants.baseColorAndRough[0] = 0.4f;  // Gray
            impl_->constants.baseColorAndRough[1] = 0.4f;
            impl_->constants.baseColorAndRough[2] = 0.4f;
        }
        impl_->constants.baseColorAndRough[3] = 1.0f;
        
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderSingleMeshWireframe(const RHILoadedModel& model, const float* worldMatrix,
                                                 int meshIndex, const float* wireColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    if (!impl_->unlitWireframePipelineState) return;  // Use unlit pipeline!
    
    const auto& gpu = model.meshes[meshIndex];
    if (gpu.meshIndex >= impl_->meshStorage.size()) return;
    const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
    
    // Use UNLIT wireframe pipeline - solid color, no lighting!
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->unlitWireframePipelineState.Get());
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    // Calculate WVP with slight depth offset to ensure wireframe is visible
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    
    // Set solid wireframe color - override all material/lighting
    impl_->constants.baseColorAndRough[0] = wireColor[0];
    impl_->constants.baseColorAndRough[1] = wireColor[1];
    impl_->constants.baseColorAndRough[2] = wireColor[2];
    impl_->constants.baseColorAndRough[3] = 1.0f;  // Full roughness
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    impl_->constants.cameraPosAndMetal[3] = 0.0f;
    impl_->constants.lightDirAndFlags[0] = 0.0f;
    impl_->constants.lightDirAndFlags[1] = -1.0f;  // Light from above
    impl_->constants.lightDirAndFlags[2] = 0.0f;
    impl_->constants.lightDirAndFlags[3] = 0;  // No textures
    impl_->constants.shadowParams[3] = 0;
    impl_->constants.iblParams[3] = 0;
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    // Set dummy textures
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srv = srvGpuStart;
    srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
    impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
    impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
    srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
    srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
    srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
    srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
    
    // Draw single mesh as wireframe
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
    impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
    impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
}

void UnifiedRenderer::renderMeshWireframeOverlay(const RHILoadedModel& model, const float* worldMatrix,
                                                  int meshIndex, const float* wireColor) {
    // OVERLAY PASS: Draw wireframe on top of shaded model (like Maya/Blender mesh selection)
    // Uses wireframePipelineState with UNLIT mode (roughness = -1.0)
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    if (!impl_->wireframePipelineState) return;
    
    const auto& gpu = model.meshes[meshIndex];
    if (gpu.meshIndex >= impl_->meshStorage.size()) return;
    const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
    
    // Use overlay wireframe pipeline (UNLIT shader) if available, otherwise fall back to wireframe
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    
    // Prefer overlayWireframePipelineState (unlit shader, normal depth test with bias)
    ID3D12PipelineState* pipelineToUse = nullptr;
    if (impl_->overlayWireframePipelineState) {
        pipelineToUse = impl_->overlayWireframePipelineState.Get();
    } else if (impl_->unlitWireframePipelineState) {
        pipelineToUse = impl_->unlitWireframePipelineState.Get();
    } else {
        pipelineToUse = impl_->wireframePipelineState.Get();
    }
    impl_->cmdList->SetPipelineState(pipelineToUse);
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    // Set wireframe color (baseColorAndRough.xyz is used by unlit shader)
    impl_->constants.baseColorAndRough[0] = wireColor[0];
    impl_->constants.baseColorAndRough[1] = wireColor[1];
    impl_->constants.baseColorAndRough[2] = wireColor[2];
    impl_->constants.baseColorAndRough[3] = -1.0f;  // UNLIT mode flag (for PBR fallback)
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    impl_->constants.cameraPosAndMetal[3] = 0.0f;
    impl_->constants.lightDirAndFlags[0] = 0.0f;
    impl_->constants.lightDirAndFlags[1] = -1.0f;
    impl_->constants.lightDirAndFlags[2] = 0.0f;
    impl_->constants.lightDirAndFlags[3] = 0;  // No textures
    impl_->constants.shadowParams[3] = 0;
    impl_->constants.iblParams[3] = 0;
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    // Bind dummy textures (required by root signature)
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srv = srvGpuStart;
    srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
    impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
    impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
    srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
    srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
    srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
    srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
    impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
    
    // Draw the selected mesh as wireframe overlay
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
    impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
    impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
}

void UnifiedRenderer::renderModelWireframeCulled(const RHILoadedModel& model, const float* worldMatrix, const float* wireColor) {
    // Hidden line removal via GPU back-face culling — only front-facing edges are drawn
    // This is much faster than a depth pre-pass (zero extra geometry passes)
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    
    ID3D12PipelineState* pso = impl_->culledWireframePipelineState ? 
        impl_->culledWireframePipelineState.Get() :
        (impl_->overlayWireframePipelineState ? impl_->overlayWireframePipelineState.Get() : nullptr);
    if (!pso) return;
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(pso);
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    // Set wireframe color
    impl_->constants.baseColorAndRough[0] = wireColor[0];
    impl_->constants.baseColorAndRough[1] = wireColor[1];
    impl_->constants.baseColorAndRough[2] = wireColor[2];
    impl_->constants.baseColorAndRough[3] = -1.0f;  // UNLIT mode
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    impl_->constants.cameraPosAndMetal[3] = 0.0f;
    impl_->constants.lightDirAndFlags[3] = 0;
    impl_->constants.shadowParams[3] = 0;
    impl_->constants.iblParams[3] = 0;
    
    // Single constant buffer update for all meshes
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    // Bind dummy textures once
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srv = srvGpuStart;
    srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
    for (int slot = 1; slot <= 7; ++slot) {
        impl_->cmdList->SetGraphicsRootDescriptorTable(slot, srv);
    }
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Draw all meshes — only VB/IB changes per mesh
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderModelWireframe(const RHILoadedModel& model, const float* worldMatrix, const float* wireColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    
    // DEBUG
    static bool firstCall = true;
    if (firstCall) {
        printf("[WIREFRAME] renderModelWireframe called! wireframePipelineState=%p\n", 
               (void*)impl_->wireframePipelineState.Get());
        firstCall = false;
    }
    
    // 使用 wireframePipelineState（基于主管线，只改填充模式为线框）
    // 这样可以复用主 shader 的所有逻辑，通过标志位禁用纹理
    if (!impl_->wireframePipelineState) {
        printf("[WIREFRAME] ERROR: wireframePipelineState is NULL!\n");
        renderModel(model, worldMatrix);
        return;
    }
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->wireframePipelineState.Get());  // 主管线的线框版本
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    // 禁用纹理，使用纯色
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.lightDirAndFlags[3] = 0.0f;  // 禁用diffuse纹理
    impl_->constants.shadowParams[3] = 0.0f;     // 禁用阴影
    impl_->constants.iblParams[3] = 0.0f;        // 禁用IBL
    
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    impl_->constants.cameraPosAndMetal[3] = 0.0f;  // metallic = 0
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    // 默认线框颜色（蓝灰色）
    float defaultWireColor[3] = {0.3f, 0.5f, 0.6f};
    const float* color = wireColor ? wireColor : defaultWireColor;
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        // 设置线框颜色
        impl_->constants.baseColorAndRough[0] = color[0];
        impl_->constants.baseColorAndRough[1] = color[1];
        impl_->constants.baseColorAndRough[2] = color[2];
        impl_->constants.baseColorAndRough[3] = 1.0f;  // roughness = 1
        
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        // 绑定默认纹理（禁用纹理采样）
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        srv = srvGpuStart; srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);  // diffuse
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);  // normal
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);  // specular
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderModelSolid(const RHILoadedModel& model, const float* worldMatrix, const float* solidColor) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet) return;
    
    // DEBUG: 直接用主管线但禁用纹理，看看是否生效
    // 如果这样能显示灰色，说明问题在 solidModePipelineState
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->pipelineState.Get());  // 用主管线！
    
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    // 禁用纹理，强制使用灰色
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.lightDirAndFlags[3] = 0.0f;  // 禁用diffuse纹理
    impl_->constants.shadowParams[3] = 0.0f;     // 禁用阴影
    impl_->constants.iblParams[3] = 0.0f;        // 禁用IBL
    
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    impl_->constants.cameraPosAndMetal[3] = 0.0f;  // metallic = 0
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
    
    // 强制灰色
    float grayColor[3] = {0.6f, 0.6f, 0.65f};
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        // 强制设置灰色，覆盖材质颜色
        impl_->constants.baseColorAndRough[0] = grayColor[0];
        impl_->constants.baseColorAndRough[1] = grayColor[1];
        impl_->constants.baseColorAndRough[2] = grayColor[2];
        impl_->constants.baseColorAndRough[3] = 1.0f;  // roughness = 1
        
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        // 绑定默认纹理（禁用纹理采样）
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        srv = srvGpuStart; srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);  // diffuse
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);  // normal
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);  // specular
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::renderGizmoLines(const float* lines, uint32_t lineCount) {
    if (!impl_->ready || lineCount == 0 || !impl_->cameraSet) return;
    if (!impl_->linePipelineState) return;  // Line pipeline not ready
    
    struct LineVertex { float pos[3]; float color[4]; };
    UINT vertexCount = lineCount * 2;
    
    // Ensure we don't exceed buffer capacity
    if (vertexCount > Impl::kMaxGizmoVertices) {
        vertexCount = Impl::kMaxGizmoVertices;
        lineCount = vertexCount / 2;
    }
    
    // Create persistent gizmo vertex buffer if needed
    if (!impl_->gizmoVertexBuffer) {
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; 
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = Impl::kMaxGizmoVertices * sizeof(LineVertex);
    bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1; bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
        HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&impl_->gizmoVertexBuffer));
        if (FAILED(hr)) return;
        
        impl_->gizmoVertexBuffer->Map(0, nullptr, &impl_->gizmoVbMapped);
        
        impl_->gizmoVbv.BufferLocation = impl_->gizmoVertexBuffer->GetGPUVirtualAddress();
        impl_->gizmoVbv.SizeInBytes = Impl::kMaxGizmoVertices * sizeof(LineVertex);
        impl_->gizmoVbv.StrideInBytes = sizeof(LineVertex);
    }
    
    // Update vertex data
    LineVertex* vertices = static_cast<LineVertex*>(impl_->gizmoVbMapped);
    for (uint32_t i = 0; i < lineCount; i++) {
        const float* line = lines + i * 10;  // startXYZ, endXYZ, RGBA
        vertices[i*2] = {{line[0], line[1], line[2]}, {line[6], line[7], line[8], line[9]}};
        vertices[i*2+1] = {{line[3], line[4], line[5]}, {line[6], line[7], line[8], line[9]}};
    }
    
    // Use the gizmo pipeline (always visible, no depth test)
    if (impl_->gizmoPipelineState) {
        impl_->cmdList->SetPipelineState(impl_->gizmoPipelineState.Get());
    } else {
        // Fallback to line pipeline if gizmo pipeline not ready
        impl_->cmdList->SetPipelineState(impl_->linePipelineState.Get());
    }
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    
    // Set up constants (identity world matrix, current view/proj)
    float world[16]; math::identity(world);
    float wvp[16];
    math::multiply(wvp, world, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->gizmoVbv);
    impl_->cmdList->DrawInstanced(vertexCount, 1, 0, 0);
    
    // Restore state for ImGui
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
}

void UnifiedRenderer::renderGizmoLinesWithDepth(const float* lines, uint32_t lineCount) {
    if (!impl_->ready || lineCount == 0 || !impl_->cameraSet) return;
    if (!impl_->gizmoDepthPipelineState && !impl_->linePipelineState) return;
    
    struct LineVertex { float pos[3]; float color[4]; };
    UINT vertexCount = lineCount * 2;
    
    if (vertexCount > Impl::kMaxGizmoVertices) {
        vertexCount = Impl::kMaxGizmoVertices;
        lineCount = vertexCount / 2;
    }
    
    // Create/reuse gizmo vertex buffer
    if (!impl_->gizmoVertexBuffer) {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; 
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = Impl::kMaxGizmoVertices * sizeof(LineVertex);
        bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1; bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&impl_->gizmoVertexBuffer));
        if (FAILED(hr)) return;
        
        impl_->gizmoVertexBuffer->Map(0, nullptr, &impl_->gizmoVbMapped);
        impl_->gizmoVbv.BufferLocation = impl_->gizmoVertexBuffer->GetGPUVirtualAddress();
        impl_->gizmoVbv.SizeInBytes = Impl::kMaxGizmoVertices * sizeof(LineVertex);
        impl_->gizmoVbv.StrideInBytes = sizeof(LineVertex);
    }
    
    LineVertex* vertices = static_cast<LineVertex*>(impl_->gizmoVbMapped);
    for (uint32_t i = 0; i < lineCount; i++) {
        const float* line = lines + i * 10;
        vertices[i*2] = {{line[0], line[1], line[2]}, {line[6], line[7], line[8], line[9]}};
        vertices[i*2+1] = {{line[3], line[4], line[5]}, {line[6], line[7], line[8], line[9]}};
    }
    
    // Use depth-tested gizmo pipeline (LESS_EQUAL + depth bias to show lines on surfaces)
    ID3D12PipelineState* pso = impl_->gizmoDepthPipelineState ? 
        impl_->gizmoDepthPipelineState.Get() : impl_->linePipelineState.Get();
    impl_->cmdList->SetPipelineState(pso);
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    
    float world[16]; math::identity(world);
    float wvp[16];
    math::multiply(wvp, world, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->gizmoVbv);
    impl_->cmdList->DrawInstanced(vertexCount, 1, 0, 0);
    
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
}

void UnifiedRenderer::renderEditModeEdges(const float* vertices, uint32_t vertexCount,
                                          const uint32_t* indices, uint32_t indexCount,
                                          const float* worldMatrix, const float* color) {
    if (!impl_->ready || vertexCount == 0 || indexCount == 0 || !impl_->cameraSet) return;
    if (!impl_->linePipelineState) return;
    
    // Use gizmo pipeline (no depth test) — always visible, fast
    ID3D12PipelineState* pipelineToUse = impl_->gizmoPipelineState ?
        impl_->gizmoPipelineState.Get() : impl_->linePipelineState.Get();
    
    struct LineVertex { float pos[3]; float col[4]; };
    
    // Cap vertex count
    uint32_t maxVerts = Impl::kMaxGizmoVertices;
    uint32_t actualIndexCount = std::min(indexCount, maxVerts);
    
    // Use dedicated edit wireframe buffer (separate from gizmo)
    if (!impl_->editWireframeBuffer) {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; 
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = maxVerts * sizeof(LineVertex);
        bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1; bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&impl_->editWireframeBuffer));
        if (FAILED(hr)) return;
        
        impl_->editWireframeBuffer->Map(0, nullptr, &impl_->editWireframeMapped);
        impl_->editWireframeVbv.BufferLocation = impl_->editWireframeBuffer->GetGPUVirtualAddress();
        impl_->editWireframeVbv.SizeInBytes = maxVerts * sizeof(LineVertex);
        impl_->editWireframeVbv.StrideInBytes = sizeof(LineVertex);
    }
    
    // Fill vertex buffer with indexed lines
    LineVertex* outVerts = static_cast<LineVertex*>(impl_->editWireframeMapped);
    uint32_t outVertCount = 0;
    
    for (uint32_t i = 0; i + 1 < actualIndexCount && outVertCount + 2 <= maxVerts; i += 2) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        
        if (i0 >= vertexCount || i1 >= vertexCount) continue;
        
        const float* p0 = vertices + i0 * 3;
        const float* p1 = vertices + i1 * 3;
        
        // Transform vertices by world matrix
        float t0[3], t1[3];
        if (worldMatrix) {
            t0[0] = worldMatrix[0]*p0[0] + worldMatrix[4]*p0[1] + worldMatrix[8]*p0[2] + worldMatrix[12];
            t0[1] = worldMatrix[1]*p0[0] + worldMatrix[5]*p0[1] + worldMatrix[9]*p0[2] + worldMatrix[13];
            t0[2] = worldMatrix[2]*p0[0] + worldMatrix[6]*p0[1] + worldMatrix[10]*p0[2] + worldMatrix[14];
            
            t1[0] = worldMatrix[0]*p1[0] + worldMatrix[4]*p1[1] + worldMatrix[8]*p1[2] + worldMatrix[12];
            t1[1] = worldMatrix[1]*p1[0] + worldMatrix[5]*p1[1] + worldMatrix[9]*p1[2] + worldMatrix[13];
            t1[2] = worldMatrix[2]*p1[0] + worldMatrix[6]*p1[1] + worldMatrix[10]*p1[2] + worldMatrix[14];
        } else {
            memcpy(t0, p0, sizeof(t0));
            memcpy(t1, p1, sizeof(t1));
        }
        
        outVerts[outVertCount++] = {{t0[0], t0[1], t0[2]}, {color[0], color[1], color[2], color[3]}};
        outVerts[outVertCount++] = {{t1[0], t1[1], t1[2]}, {color[0], color[1], color[2], color[3]}};
    }
    
    if (outVertCount == 0) return;
    
    // Setup rendering
    impl_->cmdList->SetPipelineState(pipelineToUse);
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    
    // Set up constants (identity world since we pre-transformed)
    float identity[16]; math::identity(identity);
    float wvp[16];
    math::multiply(wvp, identity, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, identity, sizeof(identity));
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->editWireframeVbv);  // Use dedicated wireframe buffer
    impl_->cmdList->DrawInstanced(outVertCount, 1, 0, 0);
    
    // Restore heap
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
}

void UnifiedRenderer::renderOriginalEdges(const RHILoadedModel& model, int meshIndex,
                                          const float* worldMatrix, const float* color,
                                          bool depthTest) {
    if (!impl_->ready || !impl_->cameraSet) return;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    const auto& gpuMesh = model.meshes[meshIndex];
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    const auto& dx12Mesh = impl_->meshStorage[storageIdx];
    
    // ===== GPU-side edge rendering (fast path) =====
    // Uses pre-built edge index buffer + model's vertex buffer
    // Zero CPU per-vertex work — GPU vertex shader does all transformation
    if (dx12Mesh.edgeIndexCount > 0 && dx12Mesh.edgeIndexBuffer) {
        // Choose pipeline based on depth test mode
        ID3D12PipelineState* pso = depthTest ?
            (impl_->edgeLineDepthPipelineState ? impl_->edgeLineDepthPipelineState.Get() : nullptr) :
            (impl_->edgeLinePipelineState ? impl_->edgeLinePipelineState.Get() : nullptr);
        if (!pso) {
            // Fallback to triangle wireframe if edge line pipelines not available
            renderMeshWireframeOverlay(model, worldMatrix, meshIndex, color);
            return;
        }
        
        impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
        impl_->cmdList->SetPipelineState(pso);
        
        ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
        impl_->cmdList->SetDescriptorHeaps(1, heaps);
        
        // WVP with world matrix (GPU transforms vertices)
        float wvp[16];
        if (worldMatrix) {
            math::multiply(wvp, worldMatrix, impl_->viewMatrix);
            math::multiply(wvp, wvp, impl_->projMatrix);
        } else {
            math::multiply(wvp, impl_->viewMatrix, impl_->projMatrix);
        }
        
        memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
        if (worldMatrix) {
            memcpy(impl_->constants.world, worldMatrix, 64);
        } else {
            float identity[16]; math::identity(identity);
            memcpy(impl_->constants.world, identity, 64);
        }
        
        // Set wireframe color via baseColorAndRough (used by unlit wireframe shader)
        impl_->constants.baseColorAndRough[0] = color[0];
        impl_->constants.baseColorAndRough[1] = color[1];
        impl_->constants.baseColorAndRough[2] = color[2];
        impl_->constants.baseColorAndRough[3] = -1.0f;  // UNLIT mode flag
        impl_->constants.lightDirAndFlags[3] = 0;  // No textures
        impl_->constants.shadowParams[3] = 0;
        impl_->constants.iblParams[3] = 0;
        
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        // Bind dummy textures (required by root signature)
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE srv = srvGpuStart;
        srv.ptr += impl_->defaultTextureSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        srv = srvGpuStart; srv.ptr += impl_->shadowMapSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(4, srv);
        srv = srvGpuStart; srv.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(5, srv);
        srv = srvGpuStart; srv.ptr += impl_->prefilteredSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(6, srv);
        srv = srvGpuStart; srv.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(7, srv);
        
        // Draw: model vertex buffer + edge index buffer, LINE topology
        impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.edgeIbv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.edgeIndexCount, 1, 0, 0, 0);
        return;
    }
    
    // Fallback: no edge index buffer, use triangle wireframe overlay
    if (!gpuMesh.hasOriginalEdges || gpuMesh.originalEdges.empty()) {
        renderMeshWireframeOverlay(model, worldMatrix, meshIndex, color);
        return;
    }
    
    // Legacy fallback: should not normally be reached (edge index buffer should exist)
    renderMeshWireframeOverlay(model, worldMatrix, meshIndex, color);
}

void UnifiedRenderer::renderOriginalEdgesSkinned(const RHILoadedModel& model, int meshIndex,
                                                  const float* worldMatrix, const float* color,
                                                  const float* boneMatrices,
                                                  const std::vector<SkinnedVertex>& skinnedVertices) {
    if (!impl_->ready || !impl_->cameraSet) return;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    if (skinnedVertices.empty()) {
        // Fall back to non-skinned version
        renderOriginalEdges(model, meshIndex, worldMatrix, color);
        return;
    }
    
    const auto& gpuMesh = model.meshes[meshIndex];
    if (!gpuMesh.hasOriginalEdges || gpuMesh.originalEdges.empty()) {
        renderMeshWireframeOverlay(model, worldMatrix, meshIndex, color);
        return;
    }
    
    // Use gizmo pipeline (no depth test) — always visible, fast
    ID3D12PipelineState* pipelineToUse = impl_->gizmoPipelineState ?
        impl_->gizmoPipelineState.Get() : impl_->linePipelineState.Get();
    if (!pipelineToUse) return;
    
    struct LineVertex { float pos[3]; float col[4]; };
    uint32_t maxVerts = Impl::kMaxGizmoVertices;
    
    // Use dedicated buffer for edit wireframe (separate from gizmo buffer to avoid conflicts)
    if (!impl_->editWireframeBuffer) {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; 
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = maxVerts * sizeof(LineVertex);
        bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1; bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&impl_->editWireframeBuffer));
        if (FAILED(hr)) return;
        
        impl_->editWireframeBuffer->Map(0, nullptr, &impl_->editWireframeMapped);
        impl_->editWireframeVbv.BufferLocation = impl_->editWireframeBuffer->GetGPUVirtualAddress();
        impl_->editWireframeVbv.SizeInBytes = maxVerts * sizeof(LineVertex);
        impl_->editWireframeVbv.StrideInBytes = sizeof(LineVertex);
    }
    
    // NOTE: GPU rendering currently doesn't use skinning (vertex buffer uses Vertex, not SkinnedVertex)
    // So for consistency, we also don't apply skinning to wireframe - just use original positions
    // This ensures wireframe matches the rendered mesh
    // TODO: When GPU skinning is properly implemented, enable this path
    
    // Helper lambda to transform vertex position (NO skinning, just world matrix)
    auto skinVertex = [&](uint32_t vi, float* outPos) {
        if (vi >= skinnedVertices.size()) {
            outPos[0] = outPos[1] = outPos[2] = 0;
            return;
        }
        
        const auto& sv = skinnedVertices[vi];
        const float* pos = sv.position;
        
        // Just apply world matrix, no bone transforms (matches GPU rendering)
        if (worldMatrix) {
            outPos[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
            outPos[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
            outPos[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
        } else {
            outPos[0] = pos[0];
            outPos[1] = pos[1];
            outPos[2] = pos[2];
        }
    };
    
    // Fill edit wireframe vertex buffer (separate from gizmo)
    LineVertex* outVerts = static_cast<LineVertex*>(impl_->editWireframeMapped);
    uint32_t outVertCount = 0;
    
    // Debug: track valid/invalid edges
    static int lastDebugMesh = -1;
    int validEdges = 0, invalidEdges = 0;
    
    for (const auto& edge : gpuMesh.originalEdges) {
        if (outVertCount + 2 > maxVerts) break;
        
        // Check if indices are valid
        if (edge.v0 >= skinnedVertices.size() || edge.v1 >= skinnedVertices.size()) {
            invalidEdges++;
            continue;  // Skip invalid edges
        }
        
        float p0[3], p1[3];
        skinVertex(edge.v0, p0);
        skinVertex(edge.v1, p1);
        
        outVerts[outVertCount++] = {{p0[0], p0[1], p0[2]}, {color[0], color[1], color[2], color[3]}};
        outVerts[outVertCount++] = {{p1[0], p1[1], p1[2]}, {color[0], color[1], color[2], color[3]}};
        validEdges++;
    }
    
    // Debug output once per mesh selection
    if (lastDebugMesh != meshIndex) {
        lastDebugMesh = meshIndex;
        std::cout << "[WIREFRAME] Mesh " << meshIndex 
                  << ": validEdges=" << validEdges 
                  << ", invalidEdges=" << invalidEdges 
                  << ", skinnedVerts=" << skinnedVertices.size()
                  << ", totalEdges=" << gpuMesh.originalEdges.size() 
                  << ", outVerts=" << outVertCount << std::endl;
        
        // Print bounding box of vertices
        if (!skinnedVertices.empty()) {
            float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
            float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
            for (const auto& sv : skinnedVertices) {
                minX = std::min(minX, sv.position[0]);
                minY = std::min(minY, sv.position[1]);
                minZ = std::min(minZ, sv.position[2]);
                maxX = std::max(maxX, sv.position[0]);
                maxY = std::max(maxY, sv.position[1]);
                maxZ = std::max(maxZ, sv.position[2]);
            }
            std::cout << "  Bounds: (" << minX << "," << minY << "," << minZ 
                      << ") to (" << maxX << "," << maxY << "," << maxZ << ")" << std::endl;
        }
    }
    
    if (outVertCount == 0) return;
    
    // Render
    impl_->cmdList->SetPipelineState(pipelineToUse);
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    
    float identity[16]; math::identity(identity);
    float wvp[16];
    math::multiply(wvp, identity, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, identity, sizeof(identity));
    
    UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
    memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
    impl_->currentDrawIndex++;
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->editWireframeVbv);  // Use dedicated wireframe buffer
    impl_->cmdList->DrawInstanced(outVertCount, 1, 0, 0);
    
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
}

void UnifiedRenderer::buildEditMeshFromGPU(const RHILoadedModel& model, int meshIndex, EditMesh& outMesh) {
    outMesh.clear();
    
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    const auto& gpuMesh = model.meshes[meshIndex];
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    
    const auto& dx12Mesh = impl_->meshStorage[storageIdx];
    
    // Read vertex data from GPU buffer (keep mapped until faces are built)
    void* mappedVB = nullptr;
    D3D12_RANGE readRange = {0, dx12Mesh.vbv.SizeInBytes};
    HRESULT hr = dx12Mesh.vertexBuffer->Map(0, &readRange, &mappedVB);
    if (FAILED(hr) || !mappedVB) return;
    
    const Vertex* srcVerts = static_cast<const Vertex*>(mappedVB);
    uint32_t vertexCount = dx12Mesh.vbv.SizeInBytes / sizeof(Vertex);
    
    // Copy vertices to EditMesh
    for (uint32_t i = 0; i < vertexCount; i++) {
        outMesh.addVertex(srcVerts[i].position[0], srcVerts[i].position[1], srcVerts[i].position[2]);
    }
    
    // Add edges from originalEdges
    if (gpuMesh.hasOriginalEdges) {
        for (const auto& edge : gpuMesh.originalEdges) {
            outMesh.addEdgeIfNotExists(edge.v0, edge.v1, true);
        }
    }
    
    // Add faces from originalFaces (quads/ngons)
    // Store COMPLETE per-vertex attributes (normal, tangent, UV, color) in each Loop
    // so that undo/redo/cancel can reconstruct GPU data perfectly
    int quadCount = 0, ngonCount = 0;
    if (gpuMesh.hasOriginalFaces) {
        for (const auto& srcFace : gpuMesh.originalFaces) {
            if (srcFace.vertexIndices.size() < 3) continue;
            
            luma::EditFace face;
            for (uint32_t vi : srcFace.vertexIndices) {
                if (vi < vertexCount) {
                    luma::Loop loop;
                    loop.vertexIndex = vi;
                    // Copy ALL vertex attributes into the loop
                    loop.uv[0] = srcVerts[vi].uv[0];
                    loop.uv[1] = srcVerts[vi].uv[1];
                    loop.normal[0] = srcVerts[vi].normal[0];
                    loop.normal[1] = srcVerts[vi].normal[1];
                    loop.normal[2] = srcVerts[vi].normal[2];
                    loop.tangent[0] = srcVerts[vi].tangent[0];
                    loop.tangent[1] = srcVerts[vi].tangent[1];
                    loop.tangent[2] = srcVerts[vi].tangent[2];
                    loop.tangent[3] = srcVerts[vi].tangent[3];
                    loop.color[0] = srcVerts[vi].color[0];
                    loop.color[1] = srcVerts[vi].color[1];
                    loop.color[2] = srcVerts[vi].color[2];
                    loop.color[3] = 1.0f;
                    face.loops.push_back(loop);
                }
            }
            
            if (face.loops.size() >= 3) {
                outMesh.faces.push_back(face);
                if (face.loops.size() == 4) quadCount++;
                else if (face.loops.size() > 4) ngonCount++;
            }
        }
    }
    
    D3D12_RANGE noWrite = {0, 0};
    dx12Mesh.vertexBuffer->Unmap(0, &noWrite);
    
    std::cout << "[buildEditMeshFromGPU] Created " << outMesh.vertices.size() 
              << " verts, " << outMesh.edges.size() << " edges, "
              << outMesh.faces.size() << " faces (" << quadCount << " quads, " 
              << ngonCount << " ngons)" << std::endl;
}

void UnifiedRenderer::buildEditMeshFromGPUTriangles(const RHILoadedModel& model, int meshIndex, EditMesh& outMesh) {
    outMesh.clear();
    
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    
    const auto& dx12Mesh = impl_->meshStorage[storageIdx];
    
    // Read vertex data (keep mapped until faces are built for attribute copying)
    void* mappedVB = nullptr;
    D3D12_RANGE readRange = {0, dx12Mesh.vbv.SizeInBytes};
    HRESULT hr = dx12Mesh.vertexBuffer->Map(0, &readRange, &mappedVB);
    if (FAILED(hr) || !mappedVB) return;
    
    const Vertex* srcVerts = static_cast<const Vertex*>(mappedVB);
    uint32_t vertexCount = dx12Mesh.vbv.SizeInBytes / sizeof(Vertex);
    
    // Copy vertices
    for (uint32_t i = 0; i < vertexCount; i++) {
        outMesh.addVertex(srcVerts[i].position[0], srcVerts[i].position[1], srcVerts[i].position[2]);
    }
    
    // Read indices and create triangle faces with complete loop attributes
    void* mappedIB = nullptr;
    D3D12_RANGE ibReadRange = {0, dx12Mesh.ibv.SizeInBytes};
    hr = dx12Mesh.indexBuffer->Map(0, &ibReadRange, &mappedIB);
    if (FAILED(hr) || !mappedIB) {
        D3D12_RANGE noWrite = {0, 0};
        dx12Mesh.vertexBuffer->Unmap(0, &noWrite);
        return;
    }
    
    const uint32_t* indices = static_cast<const uint32_t*>(mappedIB);
    uint32_t indexCount = dx12Mesh.indexCount;
    
    // Create triangle faces with full vertex attributes in loops
    for (uint32_t i = 0; i + 2 < indexCount; i += 3) {
        luma::EditFace face;
        for (int j = 0; j < 3; j++) {
            uint32_t vi = indices[i + j];
            if (vi < vertexCount) {
                luma::Loop loop;
                loop.vertexIndex = vi;
                loop.uv[0] = srcVerts[vi].uv[0];
                loop.uv[1] = srcVerts[vi].uv[1];
                loop.normal[0] = srcVerts[vi].normal[0];
                loop.normal[1] = srcVerts[vi].normal[1];
                loop.normal[2] = srcVerts[vi].normal[2];
                loop.tangent[0] = srcVerts[vi].tangent[0];
                loop.tangent[1] = srcVerts[vi].tangent[1];
                loop.tangent[2] = srcVerts[vi].tangent[2];
                loop.tangent[3] = srcVerts[vi].tangent[3];
                loop.color[0] = srcVerts[vi].color[0];
                loop.color[1] = srcVerts[vi].color[1];
                loop.color[2] = srcVerts[vi].color[2];
                loop.color[3] = 1.0f;
                face.loops.push_back(loop);
            }
        }
        if (face.loops.size() == 3) {
            outMesh.faces.push_back(face);
        }
    }
    
    D3D12_RANGE noWrite = {0, 0};
    dx12Mesh.indexBuffer->Unmap(0, &noWrite);
    dx12Mesh.vertexBuffer->Unmap(0, &noWrite);
    
    std::cout << "[buildEditMeshFromGPUTriangles] Created " << outMesh.vertices.size() 
              << " verts, " << outMesh.faces.size() << " faces" << std::endl;
}

void UnifiedRenderer::updateMeshVerticesFromEditMesh(RHILoadedModel& model, int meshIndex, const EditMesh& editMesh) {
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    
    auto& dx12Mesh = impl_->meshStorage[storageIdx];
    if (!dx12Mesh.vertexBuffer) return;
    
    uint32_t gpuVertexCount = dx12Mesh.vbv.SizeInBytes / sizeof(Vertex);
    uint32_t editVertexCount = static_cast<uint32_t>(editMesh.vertices.size());
    
    // If vertex count changed (topology edit: extrude, cut, merge, etc.),
    // do a full buffer rebuild instead of just position update
    if (editVertexCount != gpuVertexCount) {
        rebuildMeshBuffersFromEditMesh(model, meshIndex, editMesh);
        return;
    }
    
    // Fast path: same vertex count — just update positions in-place
    void* mappedVB = nullptr;
    D3D12_RANGE readRange = {0, 0};  // We're writing, not reading
    HRESULT hr = dx12Mesh.vertexBuffer->Map(0, &readRange, &mappedVB);
    if (FAILED(hr) || !mappedVB) return;
    
    Vertex* dstVerts = static_cast<Vertex*>(mappedVB);
    
    // Update only positions (preserve normals, UVs, tangents, etc.)
    for (uint32_t i = 0; i < editVertexCount; i++) {
        dstVerts[i].position[0] = editMesh.vertices[i].position[0];
        dstVerts[i].position[1] = editMesh.vertices[i].position[1];
        dstVerts[i].position[2] = editMesh.vertices[i].position[2];
    }
    
    // Unmap with write range
    D3D12_RANGE writeRange = {0, editVertexCount * sizeof(Vertex)};
    dx12Mesh.vertexBuffer->Unmap(0, &writeRange);
}

void UnifiedRenderer::rebuildMeshBuffersFromEditMesh(RHILoadedModel& model, int meshIndex, const EditMesh& editMesh, bool forceFromLoops) {
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    
    auto& dx12Mesh = impl_->meshStorage[storageIdx];
    auto& gpuMesh = model.meshes[meshIndex];
    
    uint32_t vertCount = static_cast<uint32_t>(editMesh.vertices.size());
    if (vertCount == 0) return;
    
    uint32_t oldVertCount = dx12Mesh.vbv.SizeInBytes / sizeof(Vertex);
    
    // Determine the start index for vertices that need attribute computation from loops.
    // If forceFromLoops is true (undo/redo), ALL vertices need recomputation because
    // vertex ordering may have changed (e.g., removeUnusedVertices remaps indices).
    // If false (normal topology ops), only NEW vertices (beyond old count) need it.
    uint32_t recomputeFrom = forceFromLoops ? 0 : oldVertCount;
    
    std::vector<Vertex> newVertices(vertCount);
    
    if (!forceFromLoops && oldVertCount > 0 && dx12Mesh.vertexBuffer) {
        // Preserve existing GPU vertex data for vertices 0..min(old,new)-1
        void* readMapped = nullptr;
        D3D12_RANGE readRange = {0, oldVertCount * sizeof(Vertex)};
        HRESULT readHr = dx12Mesh.vertexBuffer->Map(0, &readRange, &readMapped);
        if (SUCCEEDED(readHr) && readMapped) {
            uint32_t copyCount = std::min(oldVertCount, vertCount);
            memcpy(newVertices.data(), readMapped, copyCount * sizeof(Vertex));
            D3D12_RANGE noWrite = {0, 0};
            dx12Mesh.vertexBuffer->Unmap(0, &noWrite);
        }
    }
    
    // Update ALL vertex positions from EditMesh (positions may have changed)
    for (uint32_t i = 0; i < vertCount; i++) {
        newVertices[i].position[0] = editMesh.vertices[i].position[0];
        newVertices[i].position[1] = editMesh.vertices[i].position[1];
        newVertices[i].position[2] = editMesh.vertices[i].position[2];
    }
    
    // Compute normals/UVs/tangents/colors from face loops for vertices >= recomputeFrom
    if (vertCount > recomputeFrom) {
        uint32_t recomputeCount = vertCount - recomputeFrom;
        
        // Initialize defaults
        for (uint32_t i = recomputeFrom; i < vertCount; i++) {
            newVertices[i].normal[0] = 0; newVertices[i].normal[1] = 0; newVertices[i].normal[2] = 0;
            newVertices[i].tangent[0] = 1; newVertices[i].tangent[1] = 0;
            newVertices[i].tangent[2] = 0; newVertices[i].tangent[3] = 1;
            newVertices[i].uv[0] = 0; newVertices[i].uv[1] = 0;
            newVertices[i].color[0] = 1; newVertices[i].color[1] = 1; newVertices[i].color[2] = 1;
        }
        
        // Accumulate normals and collect UV/tangent/color from face loops
        std::vector<int> normalCount(recomputeCount, 0);
        std::vector<bool> hasUV(recomputeCount, false);
        
        for (const auto& face : editMesh.faces) {
            for (const auto& loop : face.loops) {
                uint32_t vi = loop.vertexIndex;
                if (vi < recomputeFrom || vi >= vertCount) continue;
                
                uint32_t ni = vi - recomputeFrom;
                
                newVertices[vi].normal[0] += loop.normal[0];
                newVertices[vi].normal[1] += loop.normal[1];
                newVertices[vi].normal[2] += loop.normal[2];
                normalCount[ni]++;
                
                if (!hasUV[ni]) {
                    hasUV[ni] = true;
                    newVertices[vi].uv[0] = loop.uv[0];
                    newVertices[vi].uv[1] = loop.uv[1];
                    newVertices[vi].tangent[0] = loop.tangent[0];
                    newVertices[vi].tangent[1] = loop.tangent[1];
                    newVertices[vi].tangent[2] = loop.tangent[2];
                    newVertices[vi].tangent[3] = loop.tangent[3];
                    newVertices[vi].color[0] = loop.color[0];
                    newVertices[vi].color[1] = loop.color[1];
                    newVertices[vi].color[2] = loop.color[2];
                }
            }
        }
        
        // Normalize accumulated normals
        for (uint32_t i = recomputeFrom; i < vertCount; i++) {
            uint32_t ni = i - recomputeFrom;
            if (normalCount[ni] > 0) {
                float len = std::sqrt(
                    newVertices[i].normal[0] * newVertices[i].normal[0] +
                    newVertices[i].normal[1] * newVertices[i].normal[1] +
                    newVertices[i].normal[2] * newVertices[i].normal[2]);
                if (len > 1e-6f) {
                    newVertices[i].normal[0] /= len;
                    newVertices[i].normal[1] /= len;
                    newVertices[i].normal[2] /= len;
                }
            } else {
                newVertices[i].normal[1] = 1.0f; // Default up
            }
        }
    }
    
    // === 2. Triangulate EditMesh faces to build index buffer ===
    std::vector<uint32_t> newIndices;
    newIndices.reserve(editMesh.faces.size() * 3); // Rough estimate
    
    for (const auto& face : editMesh.faces) {
        if (face.loops.size() < 3) continue;
        
        // Fan triangulation: (v0, v1, v2), (v0, v2, v3), ..., (v0, vn-2, vn-1)
        uint32_t v0 = face.loops[0].vertexIndex;
        for (size_t i = 1; i + 1 < face.loops.size(); i++) {
            newIndices.push_back(v0);
            newIndices.push_back(face.loops[i].vertexIndex);
            newIndices.push_back(face.loops[i + 1].vertexIndex);
        }
    }
    
    // === 3. Recreate GPU vertex buffer ===
    const UINT vbSize = static_cast<UINT>(newVertices.size() * sizeof(Vertex));
    
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = vbSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    dx12Mesh.vertexBuffer.Reset();
    HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.vertexBuffer));
    if (FAILED(hr)) return;
    
    void* mapped = nullptr;
    dx12Mesh.vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, newVertices.data(), vbSize);
    dx12Mesh.vertexBuffer->Unmap(0, nullptr);
    dx12Mesh.vbv.BufferLocation = dx12Mesh.vertexBuffer->GetGPUVirtualAddress();
    dx12Mesh.vbv.SizeInBytes = vbSize;
    dx12Mesh.vbv.StrideInBytes = sizeof(Vertex);
    
    // === 4. Recreate GPU index buffer ===
    const UINT ibSize = static_cast<UINT>(newIndices.size() * sizeof(uint32_t));
    if (ibSize > 0) {
        bufDesc.Width = ibSize;
        dx12Mesh.indexBuffer.Reset();
        hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.indexBuffer));
        if (SUCCEEDED(hr)) {
            dx12Mesh.indexBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, newIndices.data(), ibSize);
            dx12Mesh.indexBuffer->Unmap(0, nullptr);
            dx12Mesh.ibv.BufferLocation = dx12Mesh.indexBuffer->GetGPUVirtualAddress();
            dx12Mesh.ibv.SizeInBytes = ibSize;
            dx12Mesh.ibv.Format = DXGI_FORMAT_R32_UINT;
            dx12Mesh.indexCount = static_cast<uint32_t>(newIndices.size());
        }
    }
    
    // === 5. Update RHIGPUMesh metadata ===
    gpuMesh.indexCount = static_cast<uint32_t>(newIndices.size());
    
    // === 6. Rebuild edge index buffer ===
    gpuMesh.originalEdges.clear();
    gpuMesh.originalEdges.reserve(editMesh.edges.size());
    for (const auto& edge : editMesh.edges) {
        gpuMesh.originalEdges.push_back({edge.v0, edge.v1});
    }
    gpuMesh.hasOriginalEdges = !gpuMesh.originalEdges.empty();
    
    dx12Mesh.edgeIndexBuffer.Reset();
    dx12Mesh.edgeIndexCount = 0;
    if (gpuMesh.hasOriginalEdges) {
        impl_->buildEdgeIndexBuffer(dx12Mesh, gpuMesh.originalEdges);
    }
    
    // Update skinned vertices if needed
    if (gpuMesh.hasSkinning) {
        gpuMesh.skinnedVertices.resize(vertCount);
        for (uint32_t i = 0; i < vertCount; i++) {
            gpuMesh.skinnedVertices[i].position[0] = newVertices[i].position[0];
            gpuMesh.skinnedVertices[i].position[1] = newVertices[i].position[1];
            gpuMesh.skinnedVertices[i].position[2] = newVertices[i].position[2];
            gpuMesh.skinnedVertices[i].normal[0] = newVertices[i].normal[0];
            gpuMesh.skinnedVertices[i].normal[1] = newVertices[i].normal[1];
            gpuMesh.skinnedVertices[i].normal[2] = newVertices[i].normal[2];
            gpuMesh.skinnedVertices[i].uv[0] = newVertices[i].uv[0];
            gpuMesh.skinnedVertices[i].uv[1] = newVertices[i].uv[1];
        }
    }
}

void UnifiedRenderer::rebuildEdgeIndexBuffer(RHILoadedModel& model, int meshIndex, const EditMesh& editMesh) {
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    
    auto& dx12Mesh = impl_->meshStorage[storageIdx];
    auto& gpuMesh = model.meshes[meshIndex];
    
    // Rebuild originalEdges from EditMesh edges
    gpuMesh.originalEdges.clear();
    gpuMesh.originalEdges.reserve(editMesh.edges.size());
    for (const auto& edge : editMesh.edges) {
        gpuMesh.originalEdges.push_back({edge.v0, edge.v1});
    }
    gpuMesh.hasOriginalEdges = !gpuMesh.originalEdges.empty();
    
    // Release old edge index buffer
    dx12Mesh.edgeIndexBuffer.Reset();
    dx12Mesh.edgeIndexCount = 0;
    
    // Build new GPU edge index buffer
    impl_->buildEdgeIndexBuffer(dx12Mesh, gpuMesh.originalEdges);
}

// ===== Backup/Restore GPU mesh data (for edit mode baseline) =====

MeshGPUBackup UnifiedRenderer::backupMeshGPUData(const RHILoadedModel& model, int meshIndex) {
    MeshGPUBackup backup;
    
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return backup;
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return backup;
    
    const auto& dx12Mesh = impl_->meshStorage[storageIdx];
    const auto& gpuMesh = model.meshes[meshIndex];
    
    // Read vertex buffer
    if (dx12Mesh.vertexBuffer && dx12Mesh.vbv.SizeInBytes > 0) {
        backup.vertexData.resize(dx12Mesh.vbv.SizeInBytes);
        void* mapped = nullptr;
        D3D12_RANGE readRange = {0, dx12Mesh.vbv.SizeInBytes};
        HRESULT hr = dx12Mesh.vertexBuffer->Map(0, &readRange, &mapped);
        if (SUCCEEDED(hr) && mapped) {
            memcpy(backup.vertexData.data(), mapped, dx12Mesh.vbv.SizeInBytes);
            D3D12_RANGE noWrite = {0, 0};
            dx12Mesh.vertexBuffer->Unmap(0, &noWrite);
        }
    }
    
    // Read index buffer
    if (dx12Mesh.indexBuffer && dx12Mesh.ibv.SizeInBytes > 0) {
        uint32_t indexCount = dx12Mesh.ibv.SizeInBytes / sizeof(uint32_t);
        backup.indexData.resize(indexCount);
        void* mapped = nullptr;
        D3D12_RANGE readRange = {0, dx12Mesh.ibv.SizeInBytes};
        HRESULT hr = dx12Mesh.indexBuffer->Map(0, &readRange, &mapped);
        if (SUCCEEDED(hr) && mapped) {
            memcpy(backup.indexData.data(), mapped, dx12Mesh.ibv.SizeInBytes);
            D3D12_RANGE noWrite = {0, 0};
            dx12Mesh.indexBuffer->Unmap(0, &noWrite);
        }
    }
    
    // Copy metadata
    backup.originalEdges = gpuMesh.originalEdges;
    backup.hasOriginalEdges = gpuMesh.hasOriginalEdges;
    backup.originalFaces = gpuMesh.originalFaces;
    backup.hasOriginalFaces = gpuMesh.hasOriginalFaces;
    backup.indexCount = gpuMesh.indexCount;
    backup.skinnedVertices = gpuMesh.skinnedVertices;
    backup.hasSkinning = gpuMesh.hasSkinning;
    backup.valid = true;
    
    printf("[GPU Backup] Saved mesh %d: %zu bytes VB, %zu indices\n",
           meshIndex, backup.vertexData.size(), backup.indexData.size());
    
    return backup;
}

void UnifiedRenderer::restoreMeshGPUData(RHILoadedModel& model, int meshIndex, const MeshGPUBackup& backup) {
    if (!backup.valid) return;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) return;
    
    size_t storageIdx = model.meshStorageStartIndex + meshIndex;
    if (storageIdx >= impl_->meshStorage.size()) return;
    
    auto& dx12Mesh = impl_->meshStorage[storageIdx];
    auto& gpuMesh = model.meshes[meshIndex];
    
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    // Restore vertex buffer
    if (!backup.vertexData.empty()) {
        UINT vbSize = static_cast<UINT>(backup.vertexData.size());
        bufDesc.Width = vbSize;
        
        dx12Mesh.vertexBuffer.Reset();
        HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.vertexBuffer));
        if (SUCCEEDED(hr)) {
            void* mapped = nullptr;
            dx12Mesh.vertexBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, backup.vertexData.data(), vbSize);
            dx12Mesh.vertexBuffer->Unmap(0, nullptr);
            dx12Mesh.vbv.BufferLocation = dx12Mesh.vertexBuffer->GetGPUVirtualAddress();
            dx12Mesh.vbv.SizeInBytes = vbSize;
            dx12Mesh.vbv.StrideInBytes = sizeof(Vertex);
        }
    }
    
    // Restore index buffer
    if (!backup.indexData.empty()) {
        UINT ibSize = static_cast<UINT>(backup.indexData.size() * sizeof(uint32_t));
        bufDesc.Width = ibSize;
        
        dx12Mesh.indexBuffer.Reset();
        HRESULT hr = impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&dx12Mesh.indexBuffer));
        if (SUCCEEDED(hr)) {
            void* mapped = nullptr;
            dx12Mesh.indexBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, backup.indexData.data(), ibSize);
            dx12Mesh.indexBuffer->Unmap(0, nullptr);
            dx12Mesh.ibv.BufferLocation = dx12Mesh.indexBuffer->GetGPUVirtualAddress();
            dx12Mesh.ibv.SizeInBytes = ibSize;
            dx12Mesh.ibv.Format = DXGI_FORMAT_R32_UINT;
            dx12Mesh.indexCount = static_cast<uint32_t>(backup.indexData.size());
        }
    }
    
    // Restore metadata
    gpuMesh.originalEdges = backup.originalEdges;
    gpuMesh.hasOriginalEdges = backup.hasOriginalEdges;
    gpuMesh.originalFaces = backup.originalFaces;
    gpuMesh.hasOriginalFaces = backup.hasOriginalFaces;
    gpuMesh.indexCount = backup.indexCount;
    gpuMesh.skinnedVertices = backup.skinnedVertices;
    gpuMesh.hasSkinning = backup.hasSkinning;
    
    // Rebuild edge index buffer from restored originalEdges
    dx12Mesh.edgeIndexBuffer.Reset();
    dx12Mesh.edgeIndexCount = 0;
    if (gpuMesh.hasOriginalEdges && !gpuMesh.originalEdges.empty()) {
        impl_->buildEdgeIndexBuffer(dx12Mesh, gpuMesh.originalEdges);
    }
    
    printf("[GPU Restore] Restored mesh %d: %zu bytes VB, %zu indices\n",
           meshIndex, backup.vertexData.size(), backup.indexData.size());
}

bool UnifiedRenderer::getViewProjectionInverse(float* outMatrix16) const {
    if (!impl_ || !impl_->cameraSet) return false;
    
    // Compute VP
    float vp[16];
    math::multiply(vp, impl_->viewMatrix, impl_->projMatrix);
    
    // Invert it
    math::invert(outMatrix16, vp);
    return true;
}

void UnifiedRenderer::finishSceneRendering() {
    // Apply post-processing if enabled (renders to swapchain)
    if (impl_->postProcessEnabled && impl_->postProcessReady && impl_->hdrRenderTarget) {
        impl_->applyPostProcess();
    }
    
    // Now set render target to swapchain for UI rendering
    // (applyPostProcess already did this, but we need to ensure it for non-post-process case too)
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += impl_->frameIndex * impl_->rtvDescSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = impl_->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    impl_->cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    
    // Reset viewport and scissor for UI
    D3D12_VIEWPORT vp{}; vp.Width = (float)impl_->width; vp.Height = (float)impl_->height; vp.MaxDepth = 1.0f;
    D3D12_RECT sr{}; sr.right = impl_->width; sr.bottom = impl_->height;
    impl_->cmdList->RSSetViewports(1, &vp);
    impl_->cmdList->RSSetScissorRects(1, &sr);
    
    // Restore main SRV heap for UI rendering
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
}

void UnifiedRenderer::endFrame() {
    // Transition swapchain to present
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = impl_->renderTargets[impl_->frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    impl_->cmdList->ResourceBarrier(1, &barrier);
    impl_->cmdList->Close();
    
    ID3D12CommandList* lists[] = {impl_->cmdList.Get()};
    impl_->queue->ExecuteCommandLists(1, lists);
    impl_->swapchain->Present(1, 0);
    
    impl_->queue->Signal(impl_->fence.Get(), impl_->fenceValue);
    impl_->fence->SetEventOnCompletion(impl_->fenceValue++, impl_->fenceEvent);
    WaitForSingleObject(impl_->fenceEvent, INFINITE);
}

uint32_t UnifiedRenderer::getWidth() const { return impl_ ? impl_->width : 0; }
uint32_t UnifiedRenderer::getHeight() const { return impl_ ? impl_->height : 0; }

// ===== Shadow Mapping =====
void UnifiedRenderer::setShadowSettings(const ShadowSettings& settings) {
    if (impl_) impl_->shadowSettings = settings;
}

const ShadowSettings& UnifiedRenderer::getShadowSettings() const {
    static ShadowSettings defaultSettings;
    return impl_ ? impl_->shadowSettings : defaultSettings;
}

void UnifiedRenderer::beginShadowPass(float sceneRadius, const float* sceneCenter) {
    if (!impl_->ready || !impl_->shadowMapReady || !impl_->shadowSettings.enabled) return;
    
    float center[3] = {0, 0, 0};
    if (sceneCenter) {
        center[0] = sceneCenter[0];
        center[1] = sceneCenter[1];
        center[2] = sceneCenter[2];
    }
    
    // Light direction (normalized)
    float lightDir[3] = {0.5f, -0.7f, -0.5f};
    float len = sqrtf(lightDir[0]*lightDir[0] + lightDir[1]*lightDir[1] + lightDir[2]*lightDir[2]);
    lightDir[0] /= len; lightDir[1] /= len; lightDir[2] /= len;
    
    // Calculate light view matrix (looking at scene center from light direction)
    float lightDist = sceneRadius * impl_->shadowSettings.distance / 10.0f;
    float lightPos[3] = {
        center[0] - lightDir[0] * lightDist,
        center[1] - lightDir[1] * lightDist,
        center[2] - lightDir[2] * lightDist
    };
    
    float lightView[16];
    float up[3] = {0, 1, 0};
    // Avoid gimbal lock if light is pointing straight up/down
    if (fabsf(lightDir[1]) > 0.99f) {
        up[0] = 0; up[1] = 0; up[2] = 1;
    }
    math::lookAt(lightView, lightPos, center, up);
    
    // Orthographic projection for directional light shadow
    float orthoSize = sceneRadius * 2.0f;
    float lightProj[16];
    math::ortho(lightProj, -orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, lightDist * 2.0f);
    
    // Calculate light view-projection matrix
    math::multiply(impl_->lightViewProj, lightView, lightProj);
    
    // Transition shadow map to depth write
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = impl_->shadowMap.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    impl_->cmdList->ResourceBarrier(1, &barrier);
    
    // Clear and bind shadow map
    D3D12_CPU_DESCRIPTOR_HANDLE shadowDsv = impl_->shadowDsvHeap->GetCPUDescriptorHandleForHeapStart();
    impl_->cmdList->ClearDepthStencilView(shadowDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    impl_->cmdList->OMSetRenderTargets(0, nullptr, FALSE, &shadowDsv);
    
    // Set viewport and scissor for shadow map
    D3D12_VIEWPORT viewport{};
    viewport.Width = (float)impl_->shadowSettings.mapSize;
    viewport.Height = (float)impl_->shadowSettings.mapSize;
    viewport.MaxDepth = 1.0f;
    impl_->cmdList->RSSetViewports(1, &viewport);
    
    D3D12_RECT scissor{};
    scissor.right = impl_->shadowSettings.mapSize;
    scissor.bottom = impl_->shadowSettings.mapSize;
    impl_->cmdList->RSSetScissorRects(1, &scissor);
    
    impl_->cmdList->SetPipelineState(impl_->shadowPipelineState.Get());
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    impl_->inShadowPass = true;
}

void UnifiedRenderer::renderModelShadow(const RHILoadedModel& model, const float* worldMatrix) {
    if (!impl_->ready || !impl_->inShadowPass || model.meshes.empty()) return;
    
    // Set up constants with light VP matrix
    math::identity(impl_->constants.worldViewProj);  // Not used in shadow pass
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& gpu = model.meshes[i];
        if (gpu.meshIndex >= impl_->meshStorage.size()) continue;
        const auto& dx12Mesh = impl_->meshStorage[gpu.meshIndex];
        
        // Write constants
        UINT drawOffset = impl_->currentDrawIndex * Impl::kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &impl_->constants, sizeof(impl_->constants));
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        impl_->cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

void UnifiedRenderer::endShadowPass() {
    if (!impl_->ready || !impl_->inShadowPass) return;
    
    // Transition shadow map to shader resource
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = impl_->shadowMap.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    impl_->cmdList->ResourceBarrier(1, &barrier);
    
    // Restore main render target - respect post-processing mode
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = impl_->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    
    if (impl_->postProcessEnabled && impl_->postProcessReady && impl_->hdrRenderTarget) {
        // Restore to HDR render target for post-processing
        D3D12_CPU_DESCRIPTOR_HANDLE hdrRtv = impl_->postProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();
        impl_->cmdList->OMSetRenderTargets(1, &hdrRtv, FALSE, &dsv);
    } else {
        // Restore to swapchain
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += impl_->frameIndex * impl_->rtvDescSize;
    impl_->cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    }
    
    // Restore viewport
    D3D12_VIEWPORT viewport{};
    viewport.Width = (float)impl_->width;
    viewport.Height = (float)impl_->height;
    viewport.MaxDepth = 1.0f;
    impl_->cmdList->RSSetViewports(1, &viewport);
    
    D3D12_RECT scissor{};
    scissor.right = impl_->width;
    scissor.bottom = impl_->height;
    impl_->cmdList->RSSetScissorRects(1, &scissor);
    
    impl_->inShadowPass = false;
}

// ===== IBL (Image-Based Lighting) =====
void UnifiedRenderer::setIBLSettings(const IBLSettings& settings) {
    if (impl_) impl_->iblSettings = settings;
}

const IBLSettings& UnifiedRenderer::getIBLSettings() const {
    static IBLSettings defaultSettings;
    return impl_ ? impl_->iblSettings : defaultSettings;
}

bool UnifiedRenderer::loadEnvironmentMap(const std::string& hdrPath) {
    if (!impl_ || !impl_->ready) return false;
    
    // Load HDR image
    HDRImage hdr = loadHDR(hdrPath);
    if (!hdr.isValid()) {
        std::cerr << "[ibl] Failed to load HDR: " << hdrPath << std::endl;
        return false;
    }
    
    // Convert to cubemap
    uint32_t envSize = 512;
    auto cubeFaces = equirectToCubemap(hdr, envSize);
    if (cubeFaces.empty()) {
        std::cerr << "[ibl] Failed to convert HDR to cubemap" << std::endl;
        return false;
    }
    
    // Create environment cubemap
    Cubemap envMap;
    envMap.size = envSize;
    envMap.faces = std::move(cubeFaces);
    
    // Generate IBL textures
    Cubemap irradiance = IBLGenerator::generateIrradiance(envMap, impl_->iblSettings.irradianceSize);
    Cubemap prefiltered = IBLGenerator::generatePrefiltered(envMap, impl_->iblSettings.prefilteredSize, 
                                                             impl_->iblSettings.prefilteredMips);
    BRDFLut brdfLut = IBLGenerator::generateBRDFLut(impl_->iblSettings.brdfLutSize);
    
    // Upload irradiance cubemap
    {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = irradiance.size;
        texDesc.Height = irradiance.size;
        texDesc.DepthOrArraySize = 6;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        
        impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&impl_->irradianceMap));
        
        // Upload each face
        for (int face = 0; face < 6; face++) {
            std::vector<float> rgba(irradiance.size * irradiance.size * 4);
            for (uint32_t i = 0; i < irradiance.size * irradiance.size; i++) {
                rgba[i*4 + 0] = irradiance.faces[face][i*3 + 0];
                rgba[i*4 + 1] = irradiance.faces[face][i*3 + 1];
                rgba[i*4 + 2] = irradiance.faces[face][i*3 + 2];
                rgba[i*4 + 3] = 1.0f;
            }
            
            D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
            UINT64 uploadSize = irradiance.size * irradiance.size * 16;
            D3D12_RESOURCE_DESC uploadDesc{}; uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadDesc.Width = uploadSize; uploadDesc.Height = 1; uploadDesc.DepthOrArraySize = 1;
            uploadDesc.MipLevels = 1; uploadDesc.SampleDesc.Count = 1; uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            
            ComPtr<ID3D12Resource> uploadBuffer;
            impl_->device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
            
            void* mapped; uploadBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, rgba.data(), uploadSize);
            uploadBuffer->Unmap(0, nullptr);
            
            impl_->allocators[impl_->frameIndex]->Reset();
            impl_->cmdList->Reset(impl_->allocators[impl_->frameIndex].Get(), nullptr);
            
            D3D12_TEXTURE_COPY_LOCATION dst{}; dst.pResource = impl_->irradianceMap.Get();
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = face;
            D3D12_TEXTURE_COPY_LOCATION src{}; src.pResource = uploadBuffer.Get();
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            src.PlacedFootprint.Footprint.Width = irradiance.size;
            src.PlacedFootprint.Footprint.Height = irradiance.size;
            src.PlacedFootprint.Footprint.Depth = 1;
            src.PlacedFootprint.Footprint.RowPitch = irradiance.size * 16;
            
            impl_->cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            impl_->cmdList->Close();
            ID3D12CommandList* lists[] = {impl_->cmdList.Get()};
            impl_->queue->ExecuteCommandLists(1, lists);
            impl_->waitForGPU();
        }
        
        // Transition to shader resource
        impl_->allocators[impl_->frameIndex]->Reset();
        impl_->cmdList->Reset(impl_->allocators[impl_->frameIndex].Get(), nullptr);
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = impl_->irradianceMap.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        impl_->cmdList->ResourceBarrier(1, &barrier);
        impl_->cmdList->Close();
        ID3D12CommandList* lists[] = {impl_->cmdList.Get()};
        impl_->queue->ExecuteCommandLists(1, lists);
        impl_->waitForGPU();
        
        // Create SRV
        impl_->irradianceSrvIndex = impl_->nextSrvIndex++;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.TextureCube.MipLevels = 1;
        D3D12_CPU_DESCRIPTOR_HANDLE handle = impl_->srvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += impl_->irradianceSrvIndex * impl_->srvDescSize;
        impl_->device->CreateShaderResourceView(impl_->irradianceMap.Get(), &srvDesc, handle);
    }
    
    // Upload BRDF LUT (simpler, just a 2D texture)
    {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = brdfLut.size;
        texDesc.Height = brdfLut.size;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        
        impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&impl_->brdfLUT));
        
        D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        UINT64 uploadSize = brdfLut.size * brdfLut.size * 8;
        D3D12_RESOURCE_DESC uploadDesc{}; uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Width = uploadSize; uploadDesc.Height = 1; uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1; uploadDesc.SampleDesc.Count = 1; uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        ComPtr<ID3D12Resource> uploadBuffer;
        impl_->device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
        
        void* mapped; uploadBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, brdfLut.pixels.data(), uploadSize);
        uploadBuffer->Unmap(0, nullptr);
        
        impl_->allocators[impl_->frameIndex]->Reset();
        impl_->cmdList->Reset(impl_->allocators[impl_->frameIndex].Get(), nullptr);
        
        D3D12_TEXTURE_COPY_LOCATION dst{}; dst.pResource = impl_->brdfLUT.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dst.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION src{}; src.pResource = uploadBuffer.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32G32_FLOAT;
        src.PlacedFootprint.Footprint.Width = brdfLut.size;
        src.PlacedFootprint.Footprint.Height = brdfLut.size;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = brdfLut.size * 8;
        
        impl_->cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = impl_->brdfLUT.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        impl_->cmdList->ResourceBarrier(1, &barrier);
        
        impl_->cmdList->Close();
        ID3D12CommandList* lists[] = {impl_->cmdList.Get()};
        impl_->queue->ExecuteCommandLists(1, lists);
        impl_->waitForGPU();
        
        // Create SRV
        impl_->brdfLutSrvIndex = impl_->nextSrvIndex++;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        D3D12_CPU_DESCRIPTOR_HANDLE handle = impl_->srvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += impl_->brdfLutSrvIndex * impl_->srvDescSize;
        impl_->device->CreateShaderResourceView(impl_->brdfLUT.Get(), &srvDesc, handle);
    }
    
    // For prefiltered, just use irradiance for now (full implementation would include mip levels)
    // TODO: Implement full prefiltered environment map with mip levels
    impl_->prefilteredSrvIndex = impl_->irradianceSrvIndex;  // Temporary: share with irradiance
    
    impl_->iblReady = true;
    std::cout << "[ibl] Environment map loaded: " << hdrPath << std::endl;
    return true;
}

bool UnifiedRenderer::isIBLReady() const {
    return impl_ && impl_->iblReady;
}

// ===== Shader Hot-Reload =====
void UnifiedRenderer::setShaderHotReload(bool enabled) {
    if (!impl_) return;
    
    impl_->shaderHotReloadEnabled = enabled;
    
    if (enabled) {
        // Set up file watcher for shader files
        std::string pbrPath = impl_->shaderBasePath + "pbr.hlsl";
        std::string shadowPath = impl_->shaderBasePath + "shadow.hlsl";
        
        auto reloadCallback = [this](const std::string&) {
            impl_->shaderReloadPending = true;
        };
        
        impl_->shaderWatcher.watchFile(pbrPath, reloadCallback);
        impl_->shaderWatcher.watchFile(shadowPath, reloadCallback);
        
        std::cout << "[shader] Hot-reload enabled" << std::endl;
    } else {
        impl_->shaderWatcher.unwatchAll();
        std::cout << "[shader] Hot-reload disabled" << std::endl;
    }
}

bool UnifiedRenderer::isShaderHotReloadEnabled() const {
    return impl_ && impl_->shaderHotReloadEnabled;
}

bool UnifiedRenderer::reloadShaders() {
    if (!impl_ || !impl_->ready) return false;
    return impl_->recompilePBRShaders();
}

void UnifiedRenderer::checkShaderReload() {
    if (!impl_ || !impl_->shaderHotReloadEnabled) return;
    
    // Check for file changes
    impl_->shaderWatcher.checkChanges();
    
    // If reload is pending, do it
    if (impl_->shaderReloadPending) {
        impl_->shaderReloadPending = false;
        impl_->recompilePBRShaders();
    }
}

const std::string& UnifiedRenderer::getShaderError() const {
    static std::string empty;
    return impl_ ? impl_->shaderError : empty;
}

// ===== Post-Processing =====
void UnifiedRenderer::setPostProcessEnabled(bool enabled) {
    if (impl_) impl_->postProcessEnabled = enabled;
}

bool UnifiedRenderer::isPostProcessEnabled() const {
    return impl_ && impl_->postProcessEnabled;
}

void UnifiedRenderer::setPostProcessParams(const void* constants, size_t size) {
    if (!impl_ || !constants) return;
    
    // Copy constants to the persistently mapped buffer
    if (impl_->postProcessConstantsMapped && size <= 256) {
        memcpy(impl_->postProcessConstantsMapped, constants, size);
    }
}

float UnifiedRenderer::getFrameTime() const {
    return impl_ ? impl_->frameTime : 0.0f;
}

void* UnifiedRenderer::getNativeDevice() const { return impl_ ? impl_->device.Get() : nullptr; }
void* UnifiedRenderer::getNativeQueue() const { return impl_ ? impl_->queue.Get() : nullptr; }
void* UnifiedRenderer::getNativeCommandEncoder() const { return impl_ ? impl_->cmdList.Get() : nullptr; }
void* UnifiedRenderer::getNativeSrvHeap() const { return impl_ ? impl_->srvHeap.Get() : nullptr; }

void UnifiedRenderer::waitForGPU() { if (impl_) impl_->waitForGPU(); }

// ===== Dynamic Material Shader Compilation =====

void* UnifiedRenderer::compileMaterialShader(const std::string& hlslSource,
                                              const std::string& vsEntry,
                                              const std::string& psEntry) {
    if (!impl_ || !impl_->ready) return nullptr;
    
    impl_->materialShaderError.clear();
    
    // Custom include handler for pbr_common.hlsli and procedural.hlsli
    // For now, we concatenate the includes inline
    // In production, use a D3DInclude handler
    
    // Load include files
    std::string pbrCommon = loadShaderFile(impl_->shaderBasePath + "pbr_common.hlsli");
    std::string procedural = loadShaderFile(impl_->shaderBasePath + "procedural.hlsli");
    
    // Replace #include directives with actual content
    std::string fullSource = hlslSource;
    
    // Simple include replacement
    auto replaceInclude = [](std::string& src, const std::string& inc, const std::string& content) {
        std::string pattern = "#include \"" + inc + "\"";
        size_t pos = src.find(pattern);
        if (pos != std::string::npos) {
            src.replace(pos, pattern.size(), "// --- " + inc + " ---\n" + content + "\n// --- end " + inc + " ---\n");
        }
    };
    
    replaceInclude(fullSource, "pbr_common.hlsli", pbrCommon);
    replaceInclude(fullSource, "procedural.hlsli", procedural);
    
    ComPtr<ID3DBlob> vs, ps, errorBlob;
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    
    // Compile VS
    HRESULT hr = D3DCompile(fullSource.c_str(), fullSource.size(), "material.hlsl", nullptr, nullptr,
                            vsEntry.c_str(), "vs_5_0", flags, 0, &vs, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) impl_->materialShaderError = std::string((char*)errorBlob->GetBufferPointer());
        std::cerr << "[material shader] VS compile error: " << impl_->materialShaderError << std::endl;
        return nullptr;
    }
    
    // Compile PS
    hr = D3DCompile(fullSource.c_str(), fullSource.size(), "material.hlsl", nullptr, nullptr,
                    psEntry.c_str(), "ps_5_0", flags, 0, &ps, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) impl_->materialShaderError = std::string((char*)errorBlob->GetBufferPointer());
        std::cerr << "[material shader] PS compile error: " << impl_->materialShaderError << std::endl;
        return nullptr;
    }
    
    // Create PSO (same layout as PBR pipeline)
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
    psoDesc.pRootSignature = impl_->rootSignature.Get();
    psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = impl_->postProcessEnabled ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    
    ComPtr<ID3D12PipelineState> newPSO;
    hr = impl_->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&newPSO));
    if (FAILED(hr)) {
        impl_->materialShaderError = "Failed to create PSO for material shader";
        std::cerr << "[material shader] PSO creation failed" << std::endl;
        return nullptr;
    }
    
    // Store and return raw pointer as handle
    ID3D12PipelineState* rawPtr = newPSO.Get();
    impl_->materialPSOs[rawPtr] = std::move(newPSO);
    
    std::cerr << "[material shader] Successfully compiled material shader" << std::endl;
    return rawPtr;
}

void UnifiedRenderer::renderModelWithMaterial(const RHILoadedModel& model, const float* worldMatrix,
                                               void* materialPSO,
                                               const void* materialCBData, uint32_t materialCBSize) {
    if (!impl_ || !impl_->ready || !materialPSO) return;
    
    auto psoIt = impl_->materialPSOs.find(materialPSO);
    if (psoIt == impl_->materialPSOs.end()) return;
    
    auto* cmdList = impl_->cmdList.Get();
    
    // Set the material PSO
    cmdList->SetPipelineState(psoIt->second.Get());
    cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Bind SRV heap
    ID3D12DescriptorHeap* heaps[] = { impl_->srvHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);
    
    // Render each mesh
    for (const auto& gpuMesh : model.meshes) {
        if (gpuMesh.meshIndex >= impl_->meshStorage.size()) continue;
        auto& dx12Mesh = impl_->meshStorage[gpuMesh.meshIndex];
        if (!dx12Mesh.vertexBuffer || !dx12Mesh.indexBuffer) continue;
        
        // Set constants (same as regular render)
        auto& c = impl_->constants;
        
        // Build world matrix
        if (worldMatrix) {
            memcpy(c.world, worldMatrix, 64);
        }
        
        // Build WVP
        float wvp[16];
        // ... matrices would be computed here using stored view/proj
        // For now, use the same path as renderModel
        
        // Upload constants
        UINT drawOffset = impl_->currentDrawIndex * impl_->kAlignedConstantSize;
        memcpy(impl_->constantBufferMapped + drawOffset, &c, sizeof(c));
        
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = impl_->constantBuffer->GetGPUVirtualAddress() + drawOffset;
        cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);
        impl_->currentDrawIndex++;
        
        // Bind textures
        D3D12_GPU_DESCRIPTOR_HANDLE srvStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
        
        auto bindTexture = [&](UINT slot, UINT srvIndex) {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = srvStart;
            handle.ptr += srvIndex * impl_->srvDescSize;
            cmdList->SetGraphicsRootDescriptorTable(slot, handle);
        };
        
        // Bind standard textures (diffuse, normal, specular, shadow, IBL)
        bindTexture(1, dx12Mesh.diffuseSrvIndex > 0 ? dx12Mesh.diffuseSrvIndex : impl_->defaultTextureSrvIndex);
        bindTexture(2, dx12Mesh.normalSrvIndex > 0 ? dx12Mesh.normalSrvIndex : impl_->defaultTextureSrvIndex);
        bindTexture(3, dx12Mesh.specularSrvIndex > 0 ? dx12Mesh.specularSrvIndex : impl_->defaultTextureSrvIndex);
        
        // Shadow & IBL
        if (impl_->shadowMapSrvIndex > 0) bindTexture(4, impl_->shadowMapSrvIndex);
        else bindTexture(4, impl_->defaultTextureSrvIndex);
        
        // Note: IBL textures (slots 5, 6, 7) would be bound here too
        bindTexture(5, impl_->defaultTextureSrvIndex);
        bindTexture(6, impl_->defaultTextureSrvIndex);
        bindTexture(7, impl_->defaultTextureSrvIndex);
        
        // Draw
        cmdList->IASetVertexBuffers(0, 1, &dx12Mesh.vbv);
        cmdList->IASetIndexBuffer(&dx12Mesh.ibv);
        cmdList->DrawIndexedInstanced(dx12Mesh.indexCount, 1, 0, 0, 0);
    }
}

const std::string& UnifiedRenderer::getMaterialShaderError() const {
    static std::string empty;
    return impl_ ? impl_->materialShaderError : empty;
}

void UnifiedRenderer::releaseMaterialShader(void* materialPSO) {
    if (!impl_ || !materialPSO) return;
    impl_->waitForGPU();
    impl_->materialPSOs.erase(materialPSO);
}

}  // namespace luma

#endif  // _WIN32
