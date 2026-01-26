// LUMA Skinned Mesh Shader for Metal
// PBR with GPU Skinning support

#include <metal_stdlib>
using namespace metal;

#define MAX_BONES 128

constant float PI = 3.14159265359;

// ===== Uniform Buffers =====
struct SceneConstants {
    float4x4 worldViewProj;
    float4x4 world;
    float4x4 lightViewProj;
    float4 lightDirAndFlags;
    float4 cameraPosAndMetal;
    float4 baseColorAndRough;
    float4 shadowParams;
    float4 iblParams;
};

struct BoneMatrices {
    float4x4 bones[MAX_BONES];
};

// ===== Vertex Structures =====
struct SkinnedVertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float4 tangent  [[attribute(2)]];
    float2 uv       [[attribute(3)]];
    float3 color    [[attribute(4)]];
    uint4 boneIndices [[attribute(5)]];
    float4 boneWeights [[attribute(6)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float2 uv;
    float3 color;
    float4 shadowCoord;
};

// ===== Skinning Functions =====
float3 skinPosition(float3 pos, uint4 boneIdx, float4 weights, constant BoneMatrices& bones) {
    float3 result = float3(0, 0, 0);
    
    for (int i = 0; i < 4; i++) {
        if (weights[i] > 0.0) {
            result += (bones.bones[boneIdx[i]] * float4(pos, 1.0)).xyz * weights[i];
        }
    }
    
    return result;
}

float3 skinDirection(float3 dir, uint4 boneIdx, float4 weights, constant BoneMatrices& bones) {
    float3 result = float3(0, 0, 0);
    
    for (int i = 0; i < 4; i++) {
        if (weights[i] > 0.0) {
            float3x3 bone = float3x3(bones.bones[boneIdx[i]][0].xyz,
                                      bones.bones[boneIdx[i]][1].xyz,
                                      bones.bones[boneIdx[i]][2].xyz);
            result += (bone * dir) * weights[i];
        }
    }
    
    return normalize(result);
}

// ===== Vertex Shader =====
vertex VertexOut skinnedVertexMain(
    SkinnedVertexIn in [[stage_in]],
    constant SceneConstants& uniforms [[buffer(1)]],
    constant BoneMatrices& bones [[buffer(2)]]
) {
    VertexOut out;
    
    // Apply skinning
    float3 skinnedPos = skinPosition(in.position, in.boneIndices, in.boneWeights, bones);
    float3 skinnedNormal = skinDirection(in.normal, in.boneIndices, in.boneWeights, bones);
    float3 skinnedTangent = skinDirection(in.tangent.xyz, in.boneIndices, in.boneWeights, bones);
    
    // Transform to world space
    float4 worldPos = uniforms.world * float4(skinnedPos, 1.0);
    
    out.position = uniforms.worldViewProj * float4(skinnedPos, 1.0);
    out.worldPos = worldPos.xyz;
    out.normal = normalize((float3x3(uniforms.world[0].xyz, uniforms.world[1].xyz, uniforms.world[2].xyz)) * skinnedNormal);
    out.tangent = normalize((float3x3(uniforms.world[0].xyz, uniforms.world[1].xyz, uniforms.world[2].xyz)) * skinnedTangent);
    out.bitangent = cross(out.normal, out.tangent) * in.tangent.w;
    out.uv = in.uv;
    out.color = in.color;
    out.shadowCoord = uniforms.lightViewProj * worldPos;
    
    return out;
}

// Helper functions
float3 rotateY(float3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return float3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float sampleShadowPCF(float3 shadowCoord, float3 normal, float3 lightDir, 
                      float shadowBias, float shadowNormalBias, float shadowSoftness, float shadowEnabled,
                      depth2d<float> shadowMap, sampler shadowSampler) {
    if (shadowEnabled < 0.5) return 1.0;
    
    float NdotL = max(dot(normal, -lightDir), 0.0);
    float bias = shadowBias + shadowNormalBias * (1.0 - NdotL);
    float depth = shadowCoord.z - bias;
    
    float shadow = 0.0;
    float2 texelSize = shadowSoftness / 2048.0;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.sample_compare(shadowSampler, shadowCoord.xy + offset, depth);
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

// ===== Fragment Shader =====
fragment float4 skinnedFragmentMain(
    VertexOut in [[stage_in]],
    constant SceneConstants& uniforms [[buffer(0)]],
    texture2d<float> diffuseTexture [[texture(0)]],
    texture2d<float> normalTexture [[texture(1)]],
    texture2d<float> specularTexture [[texture(2)]],
    depth2d<float> shadowMap [[texture(3)]],
    texturecube<float> irradianceMap [[texture(4)]],
    texturecube<float> prefilteredMap [[texture(5)]],
    texture2d<float> brdfLUT [[texture(6)]],
    sampler texSampler [[sampler(0)]],
    sampler shadowSampler [[sampler(1)]]
) {
    // Extract uniforms
    float3 lightDir = uniforms.lightDirAndFlags.xyz;
    float3 cameraPos = uniforms.cameraPosAndMetal.xyz;
    float metallic = uniforms.cameraPosAndMetal.w;
    float3 baseColor = uniforms.baseColorAndRough.xyz;
    float roughness = uniforms.baseColorAndRough.w;
    float iblIntensity = uniforms.iblParams.x;
    float iblRotation = uniforms.iblParams.y;
    float iblMaxMip = uniforms.iblParams.z;
    float iblEnabled = uniforms.iblParams.w;
    
    // Sample textures
    float4 diffuseSample = diffuseTexture.sample(texSampler, in.uv);
    float4 normalSample = normalTexture.sample(texSampler, in.uv);
    float4 specularSample = specularTexture.sample(texSampler, in.uv);
    
    if (diffuseSample.a < 0.1) discard_fragment();
    
    // Albedo
    float3 albedo;
    float texBrightness = diffuseSample.r + diffuseSample.g + diffuseSample.b;
    if (texBrightness < 2.9) {
        albedo = diffuseSample.rgb;
    } else {
        albedo = in.color * baseColor;
    }
    
    // Normal
    float3 N;
    bool hasNormalMap = (abs(normalSample.r - normalSample.g) > 0.01 || abs(normalSample.b - 1.0) > 0.1);
    if (hasNormalMap) {
        float3 normalMap = normalSample.rgb * 2.0 - 1.0;
        float3x3 TBN = float3x3(normalize(in.tangent), normalize(in.bitangent), normalize(in.normal));
        N = normalize(TBN * normalMap);
    } else {
        N = normalize(in.normal);
    }
    
    // PBR Parameters
    float metal = metallic;
    float rough = roughness;
    bool hasSpecMap = (specularSample.r < 0.99 || specularSample.g < 0.99);
    if (hasSpecMap) {
        metal = specularSample.b;
        rough = specularSample.g;
    }
    rough = clamp(rough, 0.04f, 1.0f);
    
    // Vectors
    float3 V = normalize(cameraPos - in.worldPos);
    float3 L = normalize(-lightDir);
    float3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    float3 F0 = mix(float3(0.04), albedo, metal);
    
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
    float3 shadowCoord = in.shadowCoord.xyz / in.shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;
    float shadow = sampleShadowPCF(shadowCoord, N, lightDir, 
                                    uniforms.shadowParams.x, uniforms.shadowParams.y,
                                    uniforms.shadowParams.z, uniforms.shadowParams.w,
                                    shadowMap, shadowSampler);
    
    float3 lightColor = float3(1.0, 0.98, 0.95) * 2.5;
    float3 Lo = (diffuse + specular) * NdotL * lightColor * shadow;
    
    // Ambient
    float3 ambient;
    if (iblEnabled > 0.5) {
        float3 rotatedN = rotateY(N, iblRotation);
        float3 R = reflect(-V, N);
        float3 rotatedR = rotateY(R, iblRotation);
        
        float3 irradiance = irradianceMap.sample(texSampler, rotatedN).rgb;
        float3 F_ibl = fresnelSchlickRoughness(NdotV, F0, rough);
        float3 kD_ibl = (1.0 - F_ibl) * (1.0 - metal);
        float3 diffuseIBL = irradiance * albedo * kD_ibl;
        
        float mipLevel = rough * iblMaxMip;
        float3 prefilteredColor = prefilteredMap.sample(texSampler, rotatedR, level(mipLevel)).rgb;
        float2 brdf = brdfLUT.sample(texSampler, float2(NdotV, rough)).rg;
        float3 specularIBL = prefilteredColor * (F_ibl * brdf.x + brdf.y);
        
        ambient = (diffuseIBL + specularIBL) * iblIntensity;
    } else {
        float3 skyColor = float3(0.5, 0.6, 0.8);
        float3 groundColor = float3(0.3, 0.25, 0.2);
        float3 ambientColor = mix(groundColor, skyColor, N.y * 0.5 + 0.5);
        ambient = albedo * ambientColor * 0.25;
    }
    
    float3 color = ambient + Lo;
    
    // ACES Tone Mapping
    float a_tm = 2.51;
    float b_tm = 0.03;
    float c_tm = 2.43;
    float d_tm = 0.59;
    float e_tm = 0.14;
    color = saturate((color * (a_tm * color + b_tm)) / (color * (c_tm * color + d_tm) + e_tm));
    
    return float4(color, 1.0);
}
