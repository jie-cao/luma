// LUMA PBR Shader (HLSL)
// Cook-Torrance BRDF with GGX, Smith, Fresnel-Schlick
// Shadow Mapping with PCF + IBL

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
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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
float sampleShadowPCF(float3 shadowCoord, float3 normal, float3 lightDir) {
    if (shadowEnabled < 0.5) return 1.0;
    
    // Apply bias
    float NdotL = max(dot(normal, -lightDir), 0.0);
    float bias = shadowBias + shadowNormalBias * (1.0 - NdotL);
    float depth = shadowCoord.z - bias;
    
    // PCF 3x3
    float shadow = 0.0;
    float2 texelSize = shadowSoftness / 2048.0;  // Assuming 2048 shadow map
    
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
