// Screen Space Ambient Occlusion (SSAO)
// High-quality ambient occlusion post-processing effect
#pragma once

#include "../foundation/math_types.h"
#include <vector>
#include <random>
#include <cmath>

namespace luma {

// ===== SSAO Settings =====
struct SSAOSettings {
    // Quality
    int sampleCount = 32;           // Number of samples per pixel (16-64)
    float radius = 0.5f;            // Sampling radius in world units
    float bias = 0.025f;            // Depth bias to prevent self-occlusion
    
    // Intensity
    float intensity = 1.0f;         // AO strength (0-2)
    float power = 2.0f;             // Contrast adjustment
    
    // Blur
    bool enableBlur = true;
    int blurPasses = 2;             // Number of blur passes (1-3)
    float blurSharpness = 4.0f;     // Edge-aware blur sharpness
    
    // Performance
    bool halfResolution = true;     // Compute at half resolution
    float maxDistance = 100.0f;     // Max distance for AO (far objects skip)
    
    // Debug
    bool showOnlyAO = false;        // Debug: show AO buffer only
};

// ===== SSAO Sample Kernel =====
// Pre-computed hemisphere sampling kernel
class SSAOKernel {
public:
    static constexpr int MAX_SAMPLES = 64;
    
    Vec3 samples[MAX_SAMPLES];
    int sampleCount = 32;
    
    SSAOKernel() {
        generateKernel(32);
    }
    
    void generateKernel(int count) {
        sampleCount = std::min(count, MAX_SAMPLES);
        
        std::mt19937 rng(42);  // Fixed seed for deterministic results
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < sampleCount; i++) {
            // Generate sample on unit hemisphere (cosine-weighted)
            float xi1 = dist(rng);
            float xi2 = dist(rng);
            
            // Cosine-weighted hemisphere sampling
            float phi = 2.0f * 3.14159265f * xi1;
            float cosTheta = std::sqrt(1.0f - xi2);
            float sinTheta = std::sqrt(xi2);
            
            Vec3 sample(
                sinTheta * std::cos(phi),
                sinTheta * std::sin(phi),
                cosTheta  // Z is up (towards normal)
            );
            
            // Scale sample to distribute more samples closer to origin
            float scale = static_cast<float>(i) / static_cast<float>(sampleCount);
            scale = lerp(0.1f, 1.0f, scale * scale);  // Quadratic distribution
            
            samples[i] = sample * scale;
        }
    }
    
private:
    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
};

// ===== SSAO Noise Texture =====
// 4x4 rotation texture for randomizing samples
class SSAONoise {
public:
    static constexpr int NOISE_SIZE = 4;
    static constexpr int NOISE_PIXELS = NOISE_SIZE * NOISE_SIZE;
    
    Vec3 noise[NOISE_PIXELS];
    
    SSAONoise() {
        generateNoise();
    }
    
    void generateNoise() {
        std::mt19937 rng(12345);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < NOISE_PIXELS; i++) {
            // Random rotation vectors in tangent space
            float angle = dist(rng) * 2.0f * 3.14159265f;
            noise[i] = Vec3(
                std::cos(angle),
                std::sin(angle),
                0.0f
            );
        }
    }
    
    // Get noise vector for pixel
    Vec3 getNoise(int x, int y) const {
        int idx = (y % NOISE_SIZE) * NOISE_SIZE + (x % NOISE_SIZE);
        return noise[idx];
    }
};

// ===== SSAO Shader Code =====
// HLSL/Metal compatible shader source
namespace SSAOShaders {

// Vertex shader (full-screen quad)
inline const char* vertexShader = R"(
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut ssaoVertex(uint vertexID [[vertex_id]]) {
    VertexOut out;
    
    // Full-screen triangle
    out.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    out.position = float4(out.texCoord * 2.0 - 1.0, 0.0, 1.0);
    out.texCoord.y = 1.0 - out.texCoord.y;
    
    return out;
}
)";

// SSAO fragment shader
inline const char* fragmentShader = R"(
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
float3 reconstructPosition(float2 texCoord, float depth, float4x4 invProjection) {
    float4 clipPos = float4(texCoord * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 viewPos = invProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

// Get view-space normal from depth buffer
float3 reconstructNormal(texture2d<float> depthTexture, sampler s, float2 texCoord, float2 texelSize, float4x4 invProjection) {
    float depth = depthTexture.sample(s, texCoord).r;
    float depthL = depthTexture.sample(s, texCoord + float2(-texelSize.x, 0)).r;
    float depthR = depthTexture.sample(s, texCoord + float2(texelSize.x, 0)).r;
    float depthT = depthTexture.sample(s, texCoord + float2(0, -texelSize.y)).r;
    float depthB = depthTexture.sample(s, texCoord + float2(0, texelSize.y)).r;
    
    float3 pos = reconstructPosition(texCoord, depth, invProjection);
    float3 posL = reconstructPosition(texCoord + float2(-texelSize.x, 0), depthL, invProjection);
    float3 posR = reconstructPosition(texCoord + float2(texelSize.x, 0), depthR, invProjection);
    float3 posT = reconstructPosition(texCoord + float2(0, -texelSize.y), depthT, invProjection);
    float3 posB = reconstructPosition(texCoord + float2(0, texelSize.y), depthB, invProjection);
    
    float3 dx = posR - posL;
    float3 dy = posB - posT;
    
    return normalize(cross(dx, dy));
}

fragment float4 ssaoFragment(
    VertexOut in [[stage_in]],
    texture2d<float> depthTexture [[texture(0)]],
    texture2d<float> noiseTexture [[texture(1)]],
    constant SSAOUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler s(filter::nearest, address::clamp_to_edge);
    constexpr sampler noiseSampler(filter::nearest, address::repeat);
    
    float2 texelSize = 1.0 / float2(depthTexture.get_width(), depthTexture.get_height());
    
    // Sample depth and reconstruct position
    float depth = depthTexture.sample(s, in.texCoord).r;
    
    // Skip far pixels
    if (depth >= 1.0) {
        return float4(1.0);
    }
    
    float3 position = reconstructPosition(in.texCoord, depth, uniforms.invProjection);
    
    // Skip pixels beyond max distance
    if (-position.z > uniforms.maxDistance) {
        return float4(1.0);
    }
    
    // Get normal
    float3 normal = reconstructNormal(depthTexture, s, in.texCoord, texelSize, uniforms.invProjection);
    
    // Get noise vector
    float2 noiseCoord = in.texCoord * uniforms.noiseScale;
    float3 randomVec = noiseTexture.sample(noiseSampler, noiseCoord).xyz * 2.0 - 1.0;
    
    // Create TBN matrix
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    
    // Accumulate occlusion
    float occlusion = 0.0;
    
    for (int i = 0; i < uniforms.sampleCount; i++) {
        // Get sample position
        float3 sampleDir = TBN * uniforms.samples[i].xyz;
        float3 samplePos = position + sampleDir * uniforms.radius;
        
        // Project sample to screen space
        float4 offset = uniforms.projection * float4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        offset.y = 1.0 - offset.y;
        
        // Sample depth at projected position
        float sampleDepth = depthTexture.sample(s, offset.xy).r;
        float3 sampleActualPos = reconstructPosition(offset.xy, sampleDepth, uniforms.invProjection);
        
        // Range check
        float rangeCheck = smoothstep(0.0, 1.0, uniforms.radius / abs(position.z - sampleActualPos.z));
        
        // Occlusion test
        occlusion += (sampleActualPos.z >= samplePos.z + uniforms.bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(uniforms.sampleCount));
    occlusion = pow(occlusion, uniforms.power) * uniforms.intensity;
    
    return float4(occlusion);
}
)";

// Bilateral blur shader
inline const char* blurShader = R"(
struct BlurUniforms {
    float2 direction;  // (1,0) for horizontal, (0,1) for vertical
    float sharpness;
    float depthThreshold;
};

fragment float4 ssaoBlurFragment(
    VertexOut in [[stage_in]],
    texture2d<float> aoTexture [[texture(0)]],
    texture2d<float> depthTexture [[texture(1)]],
    constant BlurUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float2 texelSize = 1.0 / float2(aoTexture.get_width(), aoTexture.get_height());
    
    float centerDepth = depthTexture.sample(s, in.texCoord).r;
    float centerAO = aoTexture.sample(s, in.texCoord).r;
    
    // Gaussian weights
    const float weights[5] = { 0.0625, 0.25, 0.375, 0.25, 0.0625 };
    
    float result = centerAO * weights[2];
    float totalWeight = weights[2];
    
    for (int i = -2; i <= 2; i++) {
        if (i == 0) continue;
        
        float2 offset = uniforms.direction * texelSize * float(i);
        float sampleDepth = depthTexture.sample(s, in.texCoord + offset).r;
        float sampleAO = aoTexture.sample(s, in.texCoord + offset).r;
        
        // Edge-aware weight based on depth difference
        float depthDiff = abs(centerDepth - sampleDepth);
        float edgeWeight = exp(-depthDiff * uniforms.sharpness);
        
        float weight = weights[i + 2] * edgeWeight;
        result += sampleAO * weight;
        totalWeight += weight;
    }
    
    return float4(result / totalWeight);
}
)";

// Apply AO to scene
inline const char* applyShader = R"(
fragment float4 ssaoApplyFragment(
    VertexOut in [[stage_in]],
    texture2d<float> sceneTexture [[texture(0)]],
    texture2d<float> aoTexture [[texture(1)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float4 color = sceneTexture.sample(s, in.texCoord);
    float ao = aoTexture.sample(s, in.texCoord).r;
    
    // Apply AO to ambient/indirect lighting
    color.rgb *= ao;
    
    return color;
}
)";

}  // namespace SSAOShaders

// ===== SSAO Effect =====
// Main SSAO class
class SSAOEffect {
public:
    SSAOSettings settings;
    SSAOKernel kernel;
    SSAONoise noise;
    
    // GPU resources (to be initialized by renderer)
    bool initialized = false;
    
    // Update kernel when sample count changes
    void updateSettings(const SSAOSettings& newSettings) {
        if (settings.sampleCount != newSettings.sampleCount) {
            kernel.generateKernel(newSettings.sampleCount);
        }
        settings = newSettings;
    }
    
    // Get uniform data for shader
    struct SSAOUniforms {
        Mat4 projection;
        Mat4 invProjection;
        Vec3 samples[SSAOKernel::MAX_SAMPLES];
        float noiseScaleX, noiseScaleY;
        float radius;
        float bias;
        float intensity;
        float power;
        int sampleCount;
        float maxDistance;
        float padding[2];  // Alignment
    };
    
    SSAOUniforms getUniforms(const Mat4& projection, int screenWidth, int screenHeight) const {
        SSAOUniforms uniforms;
        uniforms.projection = projection;
        // Note: Would need proper matrix inverse
        uniforms.invProjection = Mat4::identity();
        
        for (int i = 0; i < kernel.sampleCount; i++) {
            uniforms.samples[i] = kernel.samples[i];
        }
        
        uniforms.noiseScaleX = static_cast<float>(screenWidth) / SSAONoise::NOISE_SIZE;
        uniforms.noiseScaleY = static_cast<float>(screenHeight) / SSAONoise::NOISE_SIZE;
        uniforms.radius = settings.radius;
        uniforms.bias = settings.bias;
        uniforms.intensity = settings.intensity;
        uniforms.power = settings.power;
        uniforms.sampleCount = kernel.sampleCount;
        uniforms.maxDistance = settings.maxDistance;
        
        return uniforms;
    }
    
    struct BlurUniforms {
        float directionX, directionY;
        float sharpness;
        float depthThreshold;
    };
    
    BlurUniforms getBlurUniforms(bool horizontal) const {
        BlurUniforms uniforms;
        uniforms.directionX = horizontal ? 1.0f : 0.0f;
        uniforms.directionY = horizontal ? 0.0f : 1.0f;
        uniforms.sharpness = settings.blurSharpness;
        uniforms.depthThreshold = 0.001f;
        return uniforms;
    }
};

// ===== SSAO Quality Presets =====
namespace SSAOPresets {

inline SSAOSettings low() {
    SSAOSettings s;
    s.sampleCount = 16;
    s.halfResolution = true;
    s.blurPasses = 1;
    return s;
}

inline SSAOSettings medium() {
    SSAOSettings s;
    s.sampleCount = 32;
    s.halfResolution = true;
    s.blurPasses = 2;
    return s;
}

inline SSAOSettings high() {
    SSAOSettings s;
    s.sampleCount = 64;
    s.halfResolution = false;
    s.blurPasses = 2;
    return s;
}

inline SSAOSettings ultra() {
    SSAOSettings s;
    s.sampleCount = 64;
    s.halfResolution = false;
    s.blurPasses = 3;
    s.radius = 0.75f;
    return s;
}

}  // namespace SSAOPresets

}  // namespace luma
