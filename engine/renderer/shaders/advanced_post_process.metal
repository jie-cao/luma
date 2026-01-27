// Advanced Post-Processing Shaders for Metal
// SSAO, SSR, Volumetric Fog, God Rays

#include <metal_stdlib>
using namespace metal;

// ===== Common Structures =====

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Fullscreen triangle vertex shader
vertex VertexOut fullscreenVertex(uint vertexID [[vertex_id]]) {
    VertexOut out;
    // Generate fullscreen triangle
    out.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    out.position = float4(out.texCoord * float2(2, -2) + float2(-1, 1), 0, 1);
    return out;
}

// ===== SSAO =====

struct SSAOUniforms {
    float4x4 projection;
    float4x4 invProjection;
    float4 samples[64];  // Hemisphere samples
    float2 noiseScale;
    float radius;
    float bias;
    float intensity;
    float power;
    int sampleCount;
    float maxDistance;
};

// Reconstruct view-space position from depth
float3 reconstructViewPos(float2 uv, float depth, float4x4 invProj) {
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 viewPos = invProj * clipPos;
    return viewPos.xyz / viewPos.w;
}

fragment float4 ssaoFragment(
    VertexOut in [[stage_in]],
    texture2d<float> depthTexture [[texture(0)]],
    texture2d<float> normalTexture [[texture(1)]],
    texture2d<float> noiseTexture [[texture(2)]],
    constant SSAOUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler samp(filter::nearest, address::clamp_to_edge);
    constexpr sampler noiseSamp(filter::nearest, address::repeat);
    
    float depth = depthTexture.sample(samp, in.texCoord).r;
    if (depth >= 1.0) return float4(1.0);  // Sky
    
    float3 viewPos = reconstructViewPos(in.texCoord, depth, uniforms.invProjection);
    float3 normal = normalTexture.sample(samp, in.texCoord).xyz * 2.0 - 1.0;
    
    // Random rotation from noise texture
    float3 randomVec = noiseTexture.sample(noiseSamp, in.texCoord * uniforms.noiseScale).xyz * 2.0 - 1.0;
    
    // Create TBN matrix
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    
    // Occlusion calculation
    float occlusion = 0.0;
    for (int i = 0; i < uniforms.sampleCount; i++) {
        // Get sample position in view space
        float3 sampleVec = TBN * uniforms.samples[i].xyz;
        float3 samplePos = viewPos + sampleVec * uniforms.radius;
        
        // Project to screen
        float4 offset = uniforms.projection * float4(samplePos, 1.0);
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        offset.y = 1.0 - offset.y;
        
        // Sample depth at this position
        float sampleDepth = depthTexture.sample(samp, offset.xy).r;
        float3 sampleViewPos = reconstructViewPos(offset.xy, sampleDepth, uniforms.invProjection);
        
        // Range check and occlusion
        float rangeCheck = smoothstep(0.0, 1.0, uniforms.radius / abs(viewPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + uniforms.bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(uniforms.sampleCount));
    occlusion = pow(occlusion, uniforms.power) * uniforms.intensity;
    
    return float4(occlusion, occlusion, occlusion, 1.0);
}

// SSAO Blur
fragment float4 ssaoBlurFragment(
    VertexOut in [[stage_in]],
    texture2d<float> ssaoTexture [[texture(0)]]
) {
    constexpr sampler samp(filter::linear, address::clamp_to_edge);
    
    float2 texelSize = 1.0 / float2(ssaoTexture.get_width(), ssaoTexture.get_height());
    float result = 0.0;
    
    // 4x4 blur
    for (int x = -2; x < 2; x++) {
        for (int y = -2; y < 2; y++) {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += ssaoTexture.sample(samp, in.texCoord + offset).r;
        }
    }
    
    result /= 16.0;
    return float4(result, result, result, 1.0);
}

// ===== SSR (Screen Space Reflections) =====

struct SSRUniforms {
    float4x4 projection;
    float4x4 invProjection;
    float4x4 view;
    int maxSteps;
    int binarySearchSteps;
    float maxDistance;
    float thickness;
    float roughnessThreshold;
    float edgeFade;
    float fadeStart;
    float fadeEnd;
};

fragment float4 ssrFragment(
    VertexOut in [[stage_in]],
    texture2d<float> colorTexture [[texture(0)]],
    texture2d<float> depthTexture [[texture(1)]],
    texture2d<float> normalTexture [[texture(2)]],
    texture2d<float> roughnessTexture [[texture(3)]],
    constant SSRUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler samp(filter::linear, address::clamp_to_edge);
    
    float roughness = roughnessTexture.sample(samp, in.texCoord).r;
    if (roughness > uniforms.roughnessThreshold) {
        return float4(0.0);  // Too rough for SSR
    }
    
    float depth = depthTexture.sample(samp, in.texCoord).r;
    if (depth >= 1.0) return float4(0.0);
    
    float3 viewPos = reconstructViewPos(in.texCoord, depth, uniforms.invProjection);
    float3 normal = normalTexture.sample(samp, in.texCoord).xyz * 2.0 - 1.0;
    
    // Reflect view direction
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, normal);
    
    // Ray march in view space
    float3 rayPos = viewPos;
    float stepSize = uniforms.maxDistance / float(uniforms.maxSteps);
    
    for (int i = 0; i < uniforms.maxSteps; i++) {
        rayPos += reflectDir * stepSize;
        
        // Project to screen
        float4 projPos = uniforms.projection * float4(rayPos, 1.0);
        projPos.xy /= projPos.w;
        float2 uv = projPos.xy * 0.5 + 0.5;
        uv.y = 1.0 - uv.y;
        
        // Out of screen?
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) break;
        
        float sampleDepth = depthTexture.sample(samp, uv).r;
        float3 samplePos = reconstructViewPos(uv, sampleDepth, uniforms.invProjection);
        
        float depthDiff = rayPos.z - samplePos.z;
        
        if (depthDiff > 0.0 && depthDiff < uniforms.thickness) {
            // Hit! Binary search refinement
            float3 hitPos = rayPos;
            for (int j = 0; j < uniforms.binarySearchSteps; j++) {
                stepSize *= 0.5;
                float4 midProj = uniforms.projection * float4(hitPos, 1.0);
                float2 midUV = midProj.xy / midProj.w * 0.5 + 0.5;
                midUV.y = 1.0 - midUV.y;
                
                float midDepth = depthTexture.sample(samp, midUV).r;
                float3 midSample = reconstructViewPos(midUV, midDepth, uniforms.invProjection);
                
                if (hitPos.z > midSample.z) {
                    hitPos -= reflectDir * stepSize;
                } else {
                    hitPos += reflectDir * stepSize;
                }
            }
            
            float4 finalProj = uniforms.projection * float4(hitPos, 1.0);
            float2 finalUV = finalProj.xy / finalProj.w * 0.5 + 0.5;
            finalUV.y = 1.0 - finalUV.y;
            
            // Edge fade
            float edgeFadeX = smoothstep(0.0, uniforms.edgeFade, finalUV.x) * 
                             smoothstep(1.0, 1.0 - uniforms.edgeFade, finalUV.x);
            float edgeFadeY = smoothstep(0.0, uniforms.edgeFade, finalUV.y) * 
                             smoothstep(1.0, 1.0 - uniforms.edgeFade, finalUV.y);
            float fade = edgeFadeX * edgeFadeY;
            
            // Distance fade
            float dist = length(hitPos - viewPos) / uniforms.maxDistance;
            fade *= 1.0 - smoothstep(uniforms.fadeStart, uniforms.fadeEnd, dist);
            
            // Roughness fade
            fade *= 1.0 - roughness / uniforms.roughnessThreshold;
            
            float3 reflectedColor = colorTexture.sample(samp, finalUV).rgb;
            return float4(reflectedColor, fade);
        }
    }
    
    return float4(0.0);
}

// ===== Volumetric Fog =====

struct FogUniforms {
    float4x4 invViewProj;
    float3 cameraPos;
    float density;
    float3 fogColor;
    float scattering;
    float absorption;
    float heightFalloff;
    float startHeight;
    float3 lightDir;
    float3 lightColor;
    float lightIntensity;
    int steps;
    float maxDistance;
    float anisotropy;
};

// Henyey-Greenstein phase function
float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

fragment float4 volumetricFogFragment(
    VertexOut in [[stage_in]],
    texture2d<float> depthTexture [[texture(0)]],
    texture2d<float> colorTexture [[texture(1)]],
    constant FogUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler samp(filter::linear, address::clamp_to_edge);
    
    float depth = depthTexture.sample(samp, in.texCoord).r;
    float3 sceneColor = colorTexture.sample(samp, in.texCoord).rgb;
    
    // Reconstruct world position
    float4 clipPos = float4(in.texCoord * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 worldPos = uniforms.invViewProj * clipPos;
    worldPos.xyz /= worldPos.w;
    
    float3 rayDir = normalize(worldPos.xyz - uniforms.cameraPos);
    float rayLength = min(length(worldPos.xyz - uniforms.cameraPos), uniforms.maxDistance);
    
    if (depth >= 1.0) {
        rayLength = uniforms.maxDistance;  // Sky
    }
    
    // Ray march through fog
    float stepSize = rayLength / float(uniforms.steps);
    float3 rayPos = uniforms.cameraPos;
    
    float transmittance = 1.0;
    float3 inScattered = float3(0.0);
    
    for (int i = 0; i < uniforms.steps; i++) {
        rayPos += rayDir * stepSize;
        
        // Height-based density
        float heightDensity = exp(-(rayPos.y - uniforms.startHeight) * uniforms.heightFalloff);
        float localDensity = uniforms.density * heightDensity;
        
        // Extinction
        float extinction = localDensity * (uniforms.scattering + uniforms.absorption);
        float stepTransmittance = exp(-extinction * stepSize);
        
        // In-scattering
        float cosTheta = dot(rayDir, -uniforms.lightDir);
        float phase = henyeyGreenstein(cosTheta, uniforms.anisotropy);
        
        float3 lightContrib = uniforms.lightColor * uniforms.lightIntensity * phase;
        float3 ambient = uniforms.fogColor * 0.1;
        
        inScattered += (lightContrib + ambient) * localDensity * uniforms.scattering * 
                       transmittance * stepSize;
        
        transmittance *= stepTransmittance;
        
        if (transmittance < 0.01) break;
    }
    
    float3 finalColor = sceneColor * transmittance + inScattered;
    return float4(finalColor, 1.0);
}

// ===== God Rays =====

struct GodRayUniforms {
    float2 lightScreenPos;  // Light position in screen space
    float3 lightColor;
    int samples;
    float density;
    float weight;
    float decay;
    float exposure;
};

fragment float4 godRaysFragment(
    VertexOut in [[stage_in]],
    texture2d<float> occlusionTexture [[texture(0)]],
    constant GodRayUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler samp(filter::linear, address::clamp_to_edge);
    
    float2 deltaTexCoord = in.texCoord - uniforms.lightScreenPos;
    deltaTexCoord *= 1.0 / float(uniforms.samples) * uniforms.density;
    
    float2 uv = in.texCoord;
    float illumination = 0.0;
    float decay = 1.0;
    
    for (int i = 0; i < uniforms.samples; i++) {
        uv -= deltaTexCoord;
        float sampleOcclusion = occlusionTexture.sample(samp, uv).r;
        sampleOcclusion *= decay * uniforms.weight;
        illumination += sampleOcclusion;
        decay *= uniforms.decay;
    }
    
    illumination *= uniforms.exposure;
    return float4(uniforms.lightColor * illumination, illumination);
}

// ===== CSM Shadow Sampling =====

struct CSMUniforms {
    float4x4 cascadeViewProj[4];
    float4 cascadeSplits;  // Split depths
    int cascadeCount;
    float bias;
    float normalBias;
};

// Sample shadow with cascade selection
float sampleCSMShadow(
    float3 worldPos,
    float viewDepth,
    texture2d_array<float> shadowMaps,
    constant CSMUniforms& csm,
    sampler shadowSampler
) {
    // Select cascade
    int cascade = 0;
    for (int i = 0; i < csm.cascadeCount - 1; i++) {
        if (viewDepth > csm.cascadeSplits[i]) {
            cascade = i + 1;
        }
    }
    
    // Transform to shadow space
    float4 shadowCoord = csm.cascadeViewProj[cascade] * float4(worldPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;
    
    // Sample shadow map
    float shadowDepth = shadowMaps.sample(shadowSampler, shadowCoord.xy, cascade).r;
    float bias = csm.bias * (1.0 + cascade * 0.5);  // Increase bias for far cascades
    
    return shadowCoord.z - bias > shadowDepth ? 0.0 : 1.0;
}

// PCF shadow sampling
float samplePCFShadow(
    float3 worldPos,
    float viewDepth,
    texture2d_array<float> shadowMaps,
    constant CSMUniforms& csm,
    sampler shadowSampler,
    int pcfSize
) {
    int cascade = 0;
    for (int i = 0; i < csm.cascadeCount - 1; i++) {
        if (viewDepth > csm.cascadeSplits[i]) cascade = i + 1;
    }
    
    float4 shadowCoord = csm.cascadeViewProj[cascade] * float4(worldPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;
    
    float2 texelSize = 1.0 / float2(shadowMaps.get_width(), shadowMaps.get_height());
    float shadow = 0.0;
    float bias = csm.bias * (1.0 + cascade * 0.5);
    
    int halfSize = pcfSize / 2;
    for (int x = -halfSize; x <= halfSize; x++) {
        for (int y = -halfSize; y <= halfSize; y++) {
            float2 offset = float2(x, y) * texelSize;
            float shadowDepth = shadowMaps.sample(shadowSampler, shadowCoord.xy + offset, cascade).r;
            shadow += shadowCoord.z - bias > shadowDepth ? 0.0 : 1.0;
        }
    }
    
    return shadow / float((pcfSize + 1) * (pcfSize + 1));
}

// ===== PCSS (Percentage Closer Soft Shadows) =====

// Poisson disk for sampling
constant float2 poissonDisk[32] = {
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790),
    float2(-0.44451212, -0.66390866), float2(0.61730189, 0.51043266),
    float2(-0.66895759, -0.17807587), float2(0.86330044, -0.22605498),
    float2(-0.20007161, 0.54386254), float2(0.41423202, 0.06827564),
    float2(-0.56014967, 0.57406288), float2(0.09614027, 0.36759713),
    float2(-0.79538119, 0.09636255), float2(0.29794776, -0.72069478),
    float2(-0.46975991, 0.84387243), float2(0.76454329, -0.56489450),
    float2(-0.99114108, -0.03125694), float2(0.51352793, 0.85799646),
    float2(-0.32897124, 0.13207649), float2(0.03151475, -0.55953668)
};

struct PCSSUniforms {
    float4x4 lightViewProj;
    float lightSize;
    float nearPlane;
    int blockerSearchSamples;
    int pcfSamples;
};

float findBlockerDistance(
    float2 uv, float receiverDepth,
    texture2d<float> shadowMap,
    sampler shadowSampler,
    float searchRadius,
    int samples
) {
    float blockerSum = 0.0;
    int blockerCount = 0;
    
    for (int i = 0; i < samples; i++) {
        float2 offset = poissonDisk[i % 32] * searchRadius;
        float shadowDepth = shadowMap.sample(shadowSampler, uv + offset).r;
        
        if (shadowDepth < receiverDepth) {
            blockerSum += shadowDepth;
            blockerCount++;
        }
    }
    
    if (blockerCount == 0) return -1.0;  // No blockers
    return blockerSum / float(blockerCount);
}

float pcssFilter(
    float2 uv, float receiverDepth,
    texture2d<float> shadowMap,
    sampler shadowSampler,
    float filterRadius,
    int samples,
    float bias
) {
    float shadow = 0.0;
    
    for (int i = 0; i < samples; i++) {
        float2 offset = poissonDisk[i % 32] * filterRadius;
        float shadowDepth = shadowMap.sample(shadowSampler, uv + offset).r;
        shadow += receiverDepth - bias > shadowDepth ? 0.0 : 1.0;
    }
    
    return shadow / float(samples);
}

fragment float4 pcssShadowFragment(
    VertexOut in [[stage_in]],
    texture2d<float> shadowMap [[texture(0)]],
    texture2d<float> depthTexture [[texture(1)]],
    constant PCSSUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler shadowSampler(filter::linear, address::clamp_to_edge, compare_func::less);
    constexpr sampler samp(filter::nearest, address::clamp_to_edge);
    
    float depth = depthTexture.sample(samp, in.texCoord).r;
    if (depth >= 1.0) return float4(1.0);
    
    // Would need world position from G-buffer
    // For now, return simple shadow
    float shadow = shadowMap.sample(shadowSampler, in.texCoord).r;
    return float4(shadow, shadow, shadow, 1.0);
}
