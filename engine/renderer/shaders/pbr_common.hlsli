// LUMA PBR Common Functions
// Shared PBR lighting code used by both static and generated material shaders
// Extracted from pbr.hlsl for reuse in node-generated shaders

#ifndef PBR_COMMON_HLSLI
#define PBR_COMMON_HLSLI

static const float PI_PBR = 3.14159265359;

// ===== Fresnel =====

// Fresnel-Schlick approximation
float3 fresnelSchlick_PBR(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel-Schlick with roughness (for IBL)
float3 fresnelSchlickRoughness_PBR(float cosTheta, float3 F0, float r) {
    return F0 + (max(float3(1.0 - r, 1.0 - r, 1.0 - r), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// ===== Normal Distribution Function (NDF) =====

// GGX/Trowbridge-Reitz NDF
float NDF_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI_PBR * denom * denom + 0.0001);
}

// ===== Geometry Function =====

// Smith's Schlick-GGX
float G_SchlickGGX(float NdotX, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

// ===== Rotate direction around Y axis =====
float3 rotateY_PBR(float3 v, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return float3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

// ===== PCF Shadow Sampling =====
float sampleShadowPCF_PBR(
    Texture2D shadowMapTex,
    SamplerComparisonState shadowSamp,
    float3 shadowCoord, float3 normal, float3 lDir,
    float sBias, float sNormalBias, float sSoftness, float sEnabled)
{
    if (sEnabled < 0.5) return 1.0;
    
    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 ||
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        shadowCoord.z < 0.0 || shadowCoord.z > 1.0) {
        return 1.0;
    }
    
    float NdotL = max(dot(normal, -lDir), 0.0);
    float bias = sBias + sNormalBias * (1.0 - NdotL);
    float depth = shadowCoord.z - bias;
    
    float shadow = 0.0;
    float2 texelSize = sSoftness / 2048.0;
    
    [unroll]
    for (int x = -1; x <= 1; x++) {
        [unroll]
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMapTex.SampleCmpLevelZero(shadowSamp, shadowCoord.xy + offset, depth);
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

// ===== Main PBR Lighting =====
// Complete PBR lighting calculation used by generated material shaders
//
// Parameters:
//   albedo        - Base color (RGB)
//   metallic      - Metallic factor [0,1]
//   roughness     - Roughness factor [0,1]
//   N             - World-space normal (normalized)
//   worldPos      - World-space position
//   camPos        - Camera position
//   lDir          - Light direction (unnormalized, from object to light source)
//   shadowCoord   - Shadow map coordinates
//   ao            - Ambient occlusion [0,1]
//   emissive      - Emissive color (RGB)
//   alphaVal      - Alpha / opacity
//   shadowMapTex  - Shadow map texture
//   shadowSamp    - Shadow comparison sampler
//   irradianceMapTex - IBL irradiance cubemap
//   prefilteredMapTex - IBL prefiltered env cubemap
//   brdfLUTTex    - BRDF integration LUT
//   texSamp       - Standard texture sampler
//   sParams       - Shadow parameters (bias, normalBias, softness, enabled)
//   iParams       - IBL parameters (intensity, rotation, maxMip, enabled)
//
float4 PBRLighting(
    float3 albedo,
    float metal,
    float rough,
    float3 N,
    float3 worldPos,
    float3 camPos,
    float3 lDir,
    float3 shadowCoord,
    float ao,
    float3 emissive,
    float alphaVal,
    Texture2D shadowMapTex,
    SamplerComparisonState shadowSamp,
    TextureCube irradianceMapTex,
    TextureCube prefilteredMapTex,
    Texture2D brdfLUTTex,
    SamplerState texSamp,
    float4 sParams,
    float4 iParams)
{
    rough = clamp(rough, 0.04, 1.0);
    
    float3 V = normalize(camPos - worldPos);
    float3 L = normalize(-lDir);
    float3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metal);
    
    // Cook-Torrance BRDF
    float D = NDF_GGX(NdotH, rough);
    float G = G_Smith(NdotV, NdotL, rough);
    float3 F = fresnelSchlick_PBR(HdotV, F0);
    
    float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    float3 kD = (1.0 - F) * (1.0 - metal);
    float3 diffuse = kD * albedo / PI_PBR;
    
    // Shadow
    float shadow = sampleShadowPCF_PBR(shadowMapTex, shadowSamp, shadowCoord, N, lDir,
                                         sParams.x, sParams.y, sParams.z, sParams.w);
    
    float3 lightColor = float3(1.0, 0.98, 0.95) * 2.5;
    float3 Lo = (diffuse + specular) * NdotL * lightColor * shadow;
    
    // Ambient - IBL or fallback hemisphere
    float3 ambient;
    if (iParams.w > 0.5) {
        float3 rotatedN = rotateY_PBR(N, iParams.y);
        float3 R = reflect(-V, N);
        float3 rotatedR = rotateY_PBR(R, iParams.y);
        
        float3 irradiance = irradianceMapTex.Sample(texSamp, rotatedN).rgb;
        float3 F_ibl = fresnelSchlickRoughness_PBR(NdotV, F0, rough);
        float3 kD_ibl = (1.0 - F_ibl) * (1.0 - metal);
        float3 diffuseIBL = irradiance * albedo * kD_ibl;
        
        float mipLevel = rough * iParams.z;
        float3 prefilteredColor = prefilteredMapTex.SampleLevel(texSamp, rotatedR, mipLevel).rgb;
        float2 brdf = brdfLUTTex.Sample(texSamp, float2(NdotV, rough)).rg;
        float3 specularIBL = prefilteredColor * (F_ibl * brdf.x + brdf.y);
        
        ambient = (diffuseIBL + specularIBL) * iParams.x;
    } else {
        float3 skyColor = float3(0.5, 0.6, 0.8);
        float3 groundColor = float3(0.3, 0.25, 0.2);
        float3 ambientColor = lerp(groundColor, skyColor, N.y * 0.5 + 0.5);
        ambient = albedo * ambientColor * 0.25;
    }
    
    float3 color = (ambient + Lo) * ao + emissive;
    
    // ACES Tone Mapping
    float a_tm = 2.51; float b_tm = 0.03; float c_tm = 2.43; float d_tm = 0.59; float e_tm = 0.14;
    color = saturate((color * (a_tm * color + b_tm)) / (color * (c_tm * color + d_tm) + e_tm));
    
    return float4(color, alphaVal);
}

#endif // PBR_COMMON_HLSLI
