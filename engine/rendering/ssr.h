// Screen Space Reflections (SSR)
// Ray-marched reflections using the depth buffer
#pragma once

// Prevent Windows macros from interfering
#ifdef _WIN32
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#endif

#include "../foundation/math_types.h"
#include <cmath>
#include <algorithm>
#include <functional>

namespace luma {

// ===== SSR Settings =====
struct SSRSettings {
    // Quality
    int maxSteps = 64;              // Max ray march steps
    int binarySearchSteps = 8;      // Binary search refinement steps
    float maxDistance = 100.0f;     // Max reflection distance (world units)
    float thickness = 0.5f;         // Depth comparison thickness
    
    // Performance
    bool halfResolution = true;     // Trace at half resolution
    float stride = 1.0f;            // Initial step size multiplier
    float strideMultiplier = 1.05f; // Step size increase per iteration
    
    // Blending
    float fadeStart = 0.8f;         // Start fading at screen edges (0-1)
    float fadeEnd = 1.0f;           // Fully faded at screen edges
    float roughnessThreshold = 0.5f;// Skip reflections above this roughness
    
    // Debug
    bool showOnlyReflections = false;
    bool showRayMarchSteps = false;
};

// ===== SSR Ray March Result =====
struct SSRHit {
    bool hit = false;
    Vec3 hitPosition;       // World space
    Vec3 screenUV;          // Screen UV of hit point
    float confidence = 0.0f;// Reflection confidence (0-1)
    int steps = 0;          // Steps taken to find hit
};

// ===== SSR Algorithm =====
class SSRTracer {
public:
    SSRSettings settings;
    
    // Trace a single reflection ray
    // viewDir: view-space ray direction
    // viewPos: view-space ray origin
    // Returns: screen UV of hit, or invalid if no hit
    SSRHit trace(
        const Vec3& viewPos,
        const Vec3& viewDir,
        const Mat4& projection,
        int screenWidth,
        int screenHeight,
        // Depth buffer sample function: (u, v) -> linearDepth
        std::function<float(float, float)> sampleDepth
    ) const {
        SSRHit result;
        
        // Early out for rays facing camera
        if (viewDir.z > 0.0f) {
            return result;
        }
        
        // Project start point to screen
        Vec3 startScreen = projectToScreen(viewPos, projection, screenWidth, screenHeight);
        
        // Calculate end point
        Vec3 endPos = viewPos + viewDir * settings.maxDistance;
        Vec3 endScreen = projectToScreen(endPos, projection, screenWidth, screenHeight);
        
        // Screen-space ray direction
        Vec3 rayDir = endScreen - startScreen;
        float rayLength = rayDir.length();
        
        if (rayLength < 0.001f) {
            return result;
        }
        
        rayDir = rayDir * (1.0f / rayLength);
        
        // Ray march in screen space
        float stepSize = settings.stride;
        Vec3 currentPos = startScreen;
        float traveled = 0.0f;
        
        for (int i = 0; i < settings.maxSteps; i++) {
            result.steps = i + 1;
            
            // Advance ray
            currentPos = currentPos + rayDir * stepSize;
            traveled += stepSize;
            
            // Check bounds
            if (currentPos.x < 0.0f || currentPos.x > 1.0f ||
                currentPos.y < 0.0f || currentPos.y > 1.0f ||
                currentPos.z < 0.0f || currentPos.z > 1.0f) {
                break;
            }
            
            // Check max distance
            if (traveled > rayLength) {
                break;
            }
            
            // Sample depth buffer
            float sceneDepth = sampleDepth(currentPos.x, currentPos.y);
            
            // Compare depths
            float rayDepth = linearizeDepth(currentPos.z, projection);
            float depthDiff = rayDepth - sceneDepth;
            
            // Hit test
            if (depthDiff > 0.0f && depthDiff < settings.thickness) {
                // Binary search refinement
                Vec3 hitPos = binarySearch(
                    currentPos - rayDir * stepSize,
                    currentPos,
                    projection,
                    sampleDepth
                );
                
                // Calculate confidence
                result.hit = true;
                result.screenUV = hitPos;
                result.confidence = calculateConfidence(hitPos, depthDiff);
                
                return result;
            }
            
            // Increase step size (hierarchical tracing)
            stepSize *= settings.strideMultiplier;
        }
        
        return result;
    }
    
private:
    // Project view-space point to screen space (0-1)
    Vec3 projectToScreen(const Vec3& viewPos, const Mat4& projection,
                         int width, int height) const {
        // Apply projection
        float x = viewPos.x * projection.m[0] + viewPos.z * projection.m[8];
        float y = viewPos.y * projection.m[5] + viewPos.z * projection.m[9];
        float z = viewPos.z * projection.m[10] + projection.m[14];
        float w = viewPos.z * projection.m[11];
        
        if (std::abs(w) < 0.0001f) w = 0.0001f;
        
        // Perspective divide and screen transform
        return Vec3(
            (x / w) * 0.5f + 0.5f,
            (y / w) * 0.5f + 0.5f,
            (z / w) * 0.5f + 0.5f
        );
    }
    
    // Linearize depth from NDC
    float linearizeDepth(float ndcDepth, const Mat4& projection) const {
        float near = -projection.m[14] / (projection.m[10] - 1.0f);
        float far = -projection.m[14] / (projection.m[10] + 1.0f);
        return (2.0f * near * far) / (far + near - ndcDepth * (far - near));
    }
    
    // Binary search for precise hit point
    Vec3 binarySearch(Vec3 start, Vec3 end, const Mat4& projection,
                      std::function<float(float, float)> sampleDepth) const {
        Vec3 mid = start;
        
        for (int i = 0; i < settings.binarySearchSteps; i++) {
            mid = Vec3(
                (start.x + end.x) * 0.5f,
                (start.y + end.y) * 0.5f,
                (start.z + end.z) * 0.5f
            );
            
            float sceneDepth = sampleDepth(mid.x, mid.y);
            float rayDepth = linearizeDepth(mid.z, projection);
            
            if (rayDepth > sceneDepth) {
                end = mid;
            } else {
                start = mid;
            }
        }
        
        return mid;
    }
    
    // Calculate reflection confidence
    float calculateConfidence(const Vec3& screenUV, float depthDiff) const {
        float confidence = 1.0f;
        
        // Fade at screen edges
        float edgeFade = 1.0f;
        float edgeX = std::min(screenUV.x, 1.0f - screenUV.x);
        float edgeY = std::min(screenUV.y, 1.0f - screenUV.y);
        float edge = std::min(edgeX, edgeY);
        
        if (edge < settings.fadeEnd) {
            edgeFade = smoothstep(0.0f, settings.fadeEnd - settings.fadeStart, 
                                  edge - settings.fadeStart);
        }
        confidence *= edgeFade;
        
        // Fade based on depth difference
        float depthFade = 1.0f - std::min(depthDiff / settings.thickness, 1.0f);
        confidence *= depthFade;
        
        return confidence;
    }
    
    static float smoothstep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
};

// ===== SSR Shaders =====
namespace SSRShaders {

inline const char* vertexShader = R"(
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

vertex VertexOut ssrVertex(uint vertexID [[vertex_id]]) {
    VertexOut out;
    out.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    out.position = float4(out.texCoord * 2.0 - 1.0, 0.0, 1.0);
    out.texCoord.y = 1.0 - out.texCoord.y;
    return out;
}
)";

inline const char* fragmentShader = R"(
struct SSRUniforms {
    float4x4 projection;
    float4x4 invProjection;
    float4x4 view;
    int maxSteps;
    int binarySearchSteps;
    float maxDistance;
    float thickness;
    float stride;
    float strideMultiplier;
    float fadeStart;
    float fadeEnd;
    float roughnessThreshold;
};

float3 reconstructViewPosition(float2 texCoord, float depth, float4x4 invProjection) {
    float4 clipPos = float4(texCoord * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 viewPos = invProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

float3 reconstructNormal(texture2d<float> normalTexture, sampler s, float2 texCoord) {
    float3 normal = normalTexture.sample(s, texCoord).xyz;
    return normalize(normal * 2.0 - 1.0);
}

float4 projectToScreen(float3 viewPos, float4x4 projection) {
    float4 clipPos = projection * float4(viewPos, 1.0);
    clipPos.xyz /= clipPos.w;
    clipPos.xy = clipPos.xy * 0.5 + 0.5;
    clipPos.y = 1.0 - clipPos.y;
    return clipPos;
}

fragment float4 ssrFragment(
    VertexOut in [[stage_in]],
    texture2d<float> depthTexture [[texture(0)]],
    texture2d<float> normalTexture [[texture(1)]],
    texture2d<float> colorTexture [[texture(2)]],
    texture2d<float> roughnessTexture [[texture(3)]],
    constant SSRUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float depth = depthTexture.sample(s, in.texCoord).r;
    if (depth >= 1.0) {
        return float4(0.0);
    }
    
    // Check roughness
    float roughness = roughnessTexture.sample(s, in.texCoord).r;
    if (roughness > uniforms.roughnessThreshold) {
        return float4(0.0);
    }
    
    // Reconstruct view position and normal
    float3 viewPos = reconstructViewPosition(in.texCoord, depth, uniforms.invProjection);
    float3 normal = reconstructNormal(normalTexture, s, in.texCoord);
    
    // Calculate reflection direction
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, normal);
    
    // Ray march
    float3 currentPos = viewPos;
    float stepSize = uniforms.stride;
    
    for (int i = 0; i < uniforms.maxSteps; i++) {
        currentPos += reflectDir * stepSize;
        
        float4 screenPos = projectToScreen(currentPos, uniforms.projection);
        
        // Check bounds
        if (screenPos.x < 0.0 || screenPos.x > 1.0 ||
            screenPos.y < 0.0 || screenPos.y > 1.0 ||
            screenPos.z < 0.0 || screenPos.z > 1.0) {
            break;
        }
        
        float sceneDepth = depthTexture.sample(s, screenPos.xy).r;
        float3 scenePos = reconstructViewPosition(screenPos.xy, sceneDepth, uniforms.invProjection);
        
        float rayDepth = -currentPos.z;
        float sceneLinearDepth = -scenePos.z;
        float diff = rayDepth - sceneLinearDepth;
        
        if (diff > 0.0 && diff < uniforms.thickness) {
            // Hit - sample color
            float3 color = colorTexture.sample(s, screenPos.xy).rgb;
            
            // Edge fade
            float edgeX = min(screenPos.x, 1.0 - screenPos.x);
            float edgeY = min(screenPos.y, 1.0 - screenPos.y);
            float edge = min(edgeX, edgeY);
            float edgeFade = smoothstep(0.0, uniforms.fadeEnd - uniforms.fadeStart, edge - uniforms.fadeStart);
            
            // Roughness fade
            float roughFade = 1.0 - roughness / uniforms.roughnessThreshold;
            
            float confidence = edgeFade * roughFade;
            
            return float4(color * confidence, confidence);
        }
        
        stepSize *= uniforms.strideMultiplier;
    }
    
    return float4(0.0);
}
)";

// Blur/denoise shader
inline const char* blurShader = R"(
fragment float4 ssrBlurFragment(
    VertexOut in [[stage_in]],
    texture2d<float> ssrTexture [[texture(0)]],
    texture2d<float> depthTexture [[texture(1)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float2 texelSize = 1.0 / float2(ssrTexture.get_width(), ssrTexture.get_height());
    
    float4 center = ssrTexture.sample(s, in.texCoord);
    float centerDepth = depthTexture.sample(s, in.texCoord).r;
    
    float4 result = center;
    float totalWeight = 1.0;
    
    // 3x3 bilateral blur
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (x == 0 && y == 0) continue;
            
            float2 offset = float2(x, y) * texelSize;
            float4 sample = ssrTexture.sample(s, in.texCoord + offset);
            float sampleDepth = depthTexture.sample(s, in.texCoord + offset).r;
            
            float depthWeight = exp(-abs(centerDepth - sampleDepth) * 100.0);
            float weight = depthWeight;
            
            result += sample * weight;
            totalWeight += weight;
        }
    }
    
    return result / totalWeight;
}
)";

// Composite shader
inline const char* compositeShader = R"(
fragment float4 ssrCompositeFragment(
    VertexOut in [[stage_in]],
    texture2d<float> sceneTexture [[texture(0)]],
    texture2d<float> ssrTexture [[texture(1)]],
    texture2d<float> roughnessTexture [[texture(2)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float4 sceneColor = sceneTexture.sample(s, in.texCoord);
    float4 ssrColor = ssrTexture.sample(s, in.texCoord);
    float roughness = roughnessTexture.sample(s, in.texCoord).r;
    
    // Blend based on confidence and roughness
    float fresnel = pow(1.0 - max(0.0, dot(normalize(float3(in.texCoord * 2.0 - 1.0, 1.0)), float3(0, 0, 1))), 2.0);
    float blend = ssrColor.a * (1.0 - roughness) * fresnel;
    
    return float4(mix(sceneColor.rgb, ssrColor.rgb, blend), sceneColor.a);
}
)";

}  // namespace SSRShaders

// ===== SSR Quality Presets =====
namespace SSRPresets {

inline SSRSettings low() {
    SSRSettings s;
    s.maxSteps = 32;
    s.halfResolution = true;
    s.stride = 2.0f;
    return s;
}

inline SSRSettings medium() {
    SSRSettings s;
    s.maxSteps = 64;
    s.halfResolution = true;
    s.stride = 1.0f;
    return s;
}

inline SSRSettings high() {
    SSRSettings s;
    s.maxSteps = 128;
    s.halfResolution = false;
    s.stride = 0.5f;
    return s;
}

}  // namespace SSRPresets

}  // namespace luma
