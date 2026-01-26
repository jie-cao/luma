// Advanced Shadow System
// Cascaded Shadow Maps (CSM), PCSS, Contact Hardening Shadows
#pragma once

#include "../foundation/math_types.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace luma {

// ===== Shadow Quality Settings =====
enum class ShadowQuality {
    Low,        // 1 cascade, 1024x1024, basic PCF
    Medium,     // 2 cascades, 2048x2048, PCF
    High,       // 4 cascades, 2048x2048, PCSS
    Ultra       // 4 cascades, 4096x4096, PCSS + contact hardening
};

// ===== Cascaded Shadow Map Settings =====
struct CSMSettings {
    // Cascade configuration
    int numCascades = 4;                    // 1-4 cascades
    std::array<float, 4> cascadeSplits = {0.1f, 0.25f, 0.5f, 1.0f};  // Normalized distances
    
    // Shadow map resolution
    int shadowMapSize = 2048;               // Per-cascade resolution
    
    // Depth bias
    float constantBias = 0.005f;            // Constant depth bias
    float slopeBias = 1.5f;                 // Slope-scaled bias
    float normalBias = 0.02f;               // Normal offset bias
    
    // Filtering
    int pcfSamples = 16;                    // PCF filter samples
    float filterRadius = 2.0f;              // Filter radius in texels
    
    // Cascade blending
    float cascadeBlendWidth = 0.1f;         // Blend zone between cascades
    bool stabilizeCascades = true;          // Reduce shadow swimming
    
    // Performance
    bool cullBackFaces = true;              // Cull back faces when rendering shadow map
    float maxShadowDistance = 100.0f;       // Max distance for shadows
};

// ===== PCSS Settings =====
struct PCSSSettings {
    // Light size (affects penumbra)
    float lightSize = 0.02f;                // Normalized light size (0-1)
    
    // Blocker search
    int blockerSearchSamples = 16;          // Samples for blocker search
    float blockerSearchRadius = 0.01f;      // Search radius
    
    // Penumbra filtering
    int penumbraSamples = 32;               // Samples for soft shadow
    float minPenumbraSize = 0.5f;           // Minimum penumbra (texels)
    float maxPenumbraSize = 32.0f;          // Maximum penumbra (texels)
    
    // Contact hardening
    bool enableContactHardening = true;
    float contactHardeningScale = 1.0f;
};

// ===== Cascade Data =====
struct ShadowCascade {
    Mat4 viewMatrix;
    Mat4 projectionMatrix;
    Mat4 viewProjectionMatrix;
    
    float nearPlane = 0.0f;
    float farPlane = 1.0f;
    float splitDistance = 0.0f;
    
    // Bounding sphere for culling
    Vec3 boundingSphereCenter;
    float boundingSphereRadius = 0.0f;
    
    // Texel size for stable shadows
    float texelSize = 0.0f;
};

// ===== Shadow Frustum =====
struct ShadowFrustum {
    Vec3 corners[8];  // View frustum corners in world space
    Vec3 center;
    float radius;
    
    // Calculate from camera matrices
    void calculateFromCamera(const Mat4& invViewProj, float nearSplit, float farSplit) {
        // NDC corners
        const Vec3 ndcCorners[8] = {
            Vec3(-1, -1, nearSplit),  // Near bottom-left
            Vec3( 1, -1, nearSplit),  // Near bottom-right
            Vec3(-1,  1, nearSplit),  // Near top-left
            Vec3( 1,  1, nearSplit),  // Near top-right
            Vec3(-1, -1, farSplit),   // Far bottom-left
            Vec3( 1, -1, farSplit),   // Far bottom-right
            Vec3(-1,  1, farSplit),   // Far top-left
            Vec3( 1,  1, farSplit)    // Far top-right
        };
        
        center = Vec3(0, 0, 0);
        
        for (int i = 0; i < 8; i++) {
            // Transform from NDC to world space
            float x = ndcCorners[i].x;
            float y = ndcCorners[i].y;
            float z = ndcCorners[i].z;
            
            float wx = invViewProj.m[0] * x + invViewProj.m[4] * y + 
                       invViewProj.m[8] * z + invViewProj.m[12];
            float wy = invViewProj.m[1] * x + invViewProj.m[5] * y + 
                       invViewProj.m[9] * z + invViewProj.m[13];
            float wz = invViewProj.m[2] * x + invViewProj.m[6] * y + 
                       invViewProj.m[10] * z + invViewProj.m[14];
            float ww = invViewProj.m[3] * x + invViewProj.m[7] * y + 
                       invViewProj.m[11] * z + invViewProj.m[15];
            
            corners[i] = Vec3(wx / ww, wy / ww, wz / ww);
            center = center + corners[i];
        }
        
        center = center * (1.0f / 8.0f);
        
        // Calculate bounding sphere radius
        radius = 0.0f;
        for (int i = 0; i < 8; i++) {
            float dist = (corners[i] - center).length();
            radius = std::max(radius, dist);
        }
    }
};

// ===== Cascaded Shadow Map System =====
class CascadedShadowMap {
public:
    CSMSettings settings;
    std::vector<ShadowCascade> cascades;
    
    CascadedShadowMap() {
        cascades.resize(4);
    }
    
    // Update cascade matrices for given camera and light
    void update(
        const Mat4& cameraView,
        const Mat4& cameraProj,
        const Vec3& lightDirection,
        float cameraNear,
        float cameraFar
    ) {
        // Calculate inverse view-projection for frustum corners
        Mat4 viewProj = multiplyMatrices(cameraProj, cameraView);
        Mat4 invViewProj = invertMatrix(viewProj);
        
        // Calculate cascade split distances
        std::vector<float> splitDistances(settings.numCascades + 1);
        splitDistances[0] = cameraNear;
        
        for (int i = 1; i <= settings.numCascades; i++) {
            float p = static_cast<float>(i) / settings.numCascades;
            
            // Logarithmic split (better for large ranges)
            float log = cameraNear * std::pow(cameraFar / cameraNear, p);
            // Linear split
            float lin = cameraNear + (cameraFar - cameraNear) * p;
            // Blend between logarithmic and linear
            float lambda = 0.5f;
            splitDistances[i] = lambda * log + (1.0f - lambda) * lin;
            
            // Apply manual split if provided
            if (i <= static_cast<int>(settings.cascadeSplits.size())) {
                splitDistances[i] = cameraNear + 
                    settings.cascadeSplits[i - 1] * (cameraFar - cameraNear);
            }
        }
        
        // Calculate each cascade
        for (int i = 0; i < settings.numCascades; i++) {
            updateCascade(
                i,
                invViewProj,
                lightDirection,
                splitDistances[i],
                splitDistances[i + 1],
                cameraNear,
                cameraFar
            );
        }
    }
    
    // Get cascade index for a given view-space depth
    int getCascadeIndex(float viewSpaceDepth) const {
        for (int i = 0; i < settings.numCascades; i++) {
            if (viewSpaceDepth < cascades[i].splitDistance) {
                return i;
            }
        }
        return settings.numCascades - 1;
    }
    
    // Get cascade blend factor (0 = current, 1 = next)
    float getCascadeBlendFactor(float viewSpaceDepth, int cascadeIndex) const {
        if (cascadeIndex >= settings.numCascades - 1) return 0.0f;
        
        float currentSplit = cascades[cascadeIndex].splitDistance;
        float nextSplit = cascades[cascadeIndex + 1].splitDistance;
        float blendStart = currentSplit - settings.cascadeBlendWidth * (nextSplit - currentSplit);
        
        if (viewSpaceDepth < blendStart) return 0.0f;
        return (viewSpaceDepth - blendStart) / (currentSplit - blendStart);
    }
    
private:
    void updateCascade(
        int cascadeIndex,
        const Mat4& invViewProj,
        const Vec3& lightDir,
        float nearSplit,
        float farSplit,
        float cameraNear,
        float cameraFar
    ) {
        ShadowCascade& cascade = cascades[cascadeIndex];
        
        // Calculate frustum for this cascade
        float nearNDC = (nearSplit - cameraNear) / (cameraFar - cameraNear) * 2.0f - 1.0f;
        float farNDC = (farSplit - cameraNear) / (cameraFar - cameraNear) * 2.0f - 1.0f;
        
        ShadowFrustum frustum;
        frustum.calculateFromCamera(invViewProj, nearNDC, farNDC);
        
        cascade.splitDistance = farSplit;
        cascade.boundingSphereCenter = frustum.center;
        cascade.boundingSphereRadius = frustum.radius;
        
        // Create light view matrix (looking along light direction)
        Vec3 lightUp = std::abs(lightDir.y) < 0.99f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        Vec3 lightRight = lightUp.cross(lightDir).normalized();
        lightUp = lightDir.cross(lightRight).normalized();
        
        // Position light to encompass frustum
        Vec3 lightPos = frustum.center - lightDir * frustum.radius * 2.0f;
        
        cascade.viewMatrix = lookAtMatrix(lightPos, frustum.center, lightUp);
        
        // Calculate tight orthographic projection
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        
        for (int i = 0; i < 8; i++) {
            Vec3 corner = transformPoint(cascade.viewMatrix, frustum.corners[i]);
            minX = std::min(minX, corner.x);
            maxX = std::max(maxX, corner.x);
            minY = std::min(minY, corner.y);
            maxY = std::max(maxY, corner.y);
            minZ = std::min(minZ, corner.z);
            maxZ = std::max(maxZ, corner.z);
        }
        
        // Stabilize shadow map (prevent swimming)
        if (settings.stabilizeCascades) {
            float texelSize = (maxX - minX) / settings.shadowMapSize;
            minX = std::floor(minX / texelSize) * texelSize;
            maxX = std::floor(maxX / texelSize) * texelSize;
            minY = std::floor(minY / texelSize) * texelSize;
            maxY = std::floor(maxY / texelSize) * texelSize;
            cascade.texelSize = texelSize;
        }
        
        cascade.projectionMatrix = orthographicMatrix(minX, maxX, minY, maxY, minZ - 10.0f, maxZ + 10.0f);
        cascade.viewProjectionMatrix = multiplyMatrices(cascade.projectionMatrix, cascade.viewMatrix);
        cascade.nearPlane = minZ;
        cascade.farPlane = maxZ;
    }
    
    // Helper: multiply two matrices
    Mat4 multiplyMatrices(const Mat4& a, const Mat4& b) const {
        Mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i * 4 + j] = 
                    a.m[i * 4 + 0] * b.m[0 * 4 + j] +
                    a.m[i * 4 + 1] * b.m[1 * 4 + j] +
                    a.m[i * 4 + 2] * b.m[2 * 4 + j] +
                    a.m[i * 4 + 3] * b.m[3 * 4 + j];
            }
        }
        return result;
    }
    
    // Helper: look-at matrix
    Mat4 lookAtMatrix(const Vec3& eye, const Vec3& target, const Vec3& up) const {
        Vec3 f = (target - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        Vec3 u = r.cross(f);
        
        Mat4 m;
        m.m[0] = r.x;  m.m[4] = r.y;  m.m[8]  = r.z;  m.m[12] = -r.dot(eye);
        m.m[1] = u.x;  m.m[5] = u.y;  m.m[9]  = u.z;  m.m[13] = -u.dot(eye);
        m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z; m.m[14] = f.dot(eye);
        m.m[3] = 0;    m.m[7] = 0;    m.m[11] = 0;    m.m[15] = 1;
        return m;
    }
    
    // Helper: orthographic projection
    Mat4 orthographicMatrix(float left, float right, float bottom, float top, 
                           float near, float far) const {
        Mat4 m;
        m.m[0] = 2.0f / (right - left);
        m.m[5] = 2.0f / (top - bottom);
        m.m[10] = -2.0f / (far - near);
        m.m[12] = -(right + left) / (right - left);
        m.m[13] = -(top + bottom) / (top - bottom);
        m.m[14] = -(far + near) / (far - near);
        m.m[15] = 1.0f;
        return m;
    }
    
    // Helper: transform point by matrix
    Vec3 transformPoint(const Mat4& m, const Vec3& p) const {
        float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
        return Vec3(
            (m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12]) / w,
            (m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13]) / w,
            (m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]) / w
        );
    }
    
    // Helper: matrix inverse (simplified for view matrices)
    Mat4 invertMatrix(const Mat4& m) const {
        // For a view-projection matrix, we use the general inverse formula
        // This is a simplified version - full implementation would be more complex
        Mat4 inv;
        
        float det;
        int i;
        
        inv.m[0] = m.m[5] * m.m[10] * m.m[15] - m.m[5] * m.m[11] * m.m[14] - 
                   m.m[9] * m.m[6] * m.m[15] + m.m[9] * m.m[7] * m.m[14] +
                   m.m[13] * m.m[6] * m.m[11] - m.m[13] * m.m[7] * m.m[10];
        
        inv.m[4] = -m.m[4] * m.m[10] * m.m[15] + m.m[4] * m.m[11] * m.m[14] + 
                    m.m[8] * m.m[6] * m.m[15] - m.m[8] * m.m[7] * m.m[14] - 
                    m.m[12] * m.m[6] * m.m[11] + m.m[12] * m.m[7] * m.m[10];
        
        inv.m[8] = m.m[4] * m.m[9] * m.m[15] - m.m[4] * m.m[11] * m.m[13] - 
                   m.m[8] * m.m[5] * m.m[15] + m.m[8] * m.m[7] * m.m[13] + 
                   m.m[12] * m.m[5] * m.m[11] - m.m[12] * m.m[7] * m.m[9];
        
        inv.m[12] = -m.m[4] * m.m[9] * m.m[14] + m.m[4] * m.m[10] * m.m[13] +
                     m.m[8] * m.m[5] * m.m[14] - m.m[8] * m.m[6] * m.m[13] - 
                     m.m[12] * m.m[5] * m.m[10] + m.m[12] * m.m[6] * m.m[9];
        
        inv.m[1] = -m.m[1] * m.m[10] * m.m[15] + m.m[1] * m.m[11] * m.m[14] + 
                    m.m[9] * m.m[2] * m.m[15] - m.m[9] * m.m[3] * m.m[14] - 
                    m.m[13] * m.m[2] * m.m[11] + m.m[13] * m.m[3] * m.m[10];
        
        inv.m[5] = m.m[0] * m.m[10] * m.m[15] - m.m[0] * m.m[11] * m.m[14] - 
                   m.m[8] * m.m[2] * m.m[15] + m.m[8] * m.m[3] * m.m[14] + 
                   m.m[12] * m.m[2] * m.m[11] - m.m[12] * m.m[3] * m.m[10];
        
        inv.m[9] = -m.m[0] * m.m[9] * m.m[15] + m.m[0] * m.m[11] * m.m[13] + 
                    m.m[8] * m.m[1] * m.m[15] - m.m[8] * m.m[3] * m.m[13] - 
                    m.m[12] * m.m[1] * m.m[11] + m.m[12] * m.m[3] * m.m[9];
        
        inv.m[13] = m.m[0] * m.m[9] * m.m[14] - m.m[0] * m.m[10] * m.m[13] - 
                    m.m[8] * m.m[1] * m.m[14] + m.m[8] * m.m[2] * m.m[13] + 
                    m.m[12] * m.m[1] * m.m[10] - m.m[12] * m.m[2] * m.m[9];
        
        inv.m[2] = m.m[1] * m.m[6] * m.m[15] - m.m[1] * m.m[7] * m.m[14] - 
                   m.m[5] * m.m[2] * m.m[15] + m.m[5] * m.m[3] * m.m[14] + 
                   m.m[13] * m.m[2] * m.m[7] - m.m[13] * m.m[3] * m.m[6];
        
        inv.m[6] = -m.m[0] * m.m[6] * m.m[15] + m.m[0] * m.m[7] * m.m[14] + 
                    m.m[4] * m.m[2] * m.m[15] - m.m[4] * m.m[3] * m.m[14] - 
                    m.m[12] * m.m[2] * m.m[7] + m.m[12] * m.m[3] * m.m[6];
        
        inv.m[10] = m.m[0] * m.m[5] * m.m[15] - m.m[0] * m.m[7] * m.m[13] - 
                    m.m[4] * m.m[1] * m.m[15] + m.m[4] * m.m[3] * m.m[13] + 
                    m.m[12] * m.m[1] * m.m[7] - m.m[12] * m.m[3] * m.m[5];
        
        inv.m[14] = -m.m[0] * m.m[5] * m.m[14] + m.m[0] * m.m[6] * m.m[13] + 
                     m.m[4] * m.m[1] * m.m[14] - m.m[4] * m.m[2] * m.m[13] - 
                     m.m[12] * m.m[1] * m.m[6] + m.m[12] * m.m[2] * m.m[5];
        
        inv.m[3] = -m.m[1] * m.m[6] * m.m[11] + m.m[1] * m.m[7] * m.m[10] + 
                    m.m[5] * m.m[2] * m.m[11] - m.m[5] * m.m[3] * m.m[10] - 
                    m.m[9] * m.m[2] * m.m[7] + m.m[9] * m.m[3] * m.m[6];
        
        inv.m[7] = m.m[0] * m.m[6] * m.m[11] - m.m[0] * m.m[7] * m.m[10] - 
                   m.m[4] * m.m[2] * m.m[11] + m.m[4] * m.m[3] * m.m[10] + 
                   m.m[8] * m.m[2] * m.m[7] - m.m[8] * m.m[3] * m.m[6];
        
        inv.m[11] = -m.m[0] * m.m[5] * m.m[11] + m.m[0] * m.m[7] * m.m[9] + 
                     m.m[4] * m.m[1] * m.m[11] - m.m[4] * m.m[3] * m.m[9] - 
                     m.m[8] * m.m[1] * m.m[7] + m.m[8] * m.m[3] * m.m[5];
        
        inv.m[15] = m.m[0] * m.m[5] * m.m[10] - m.m[0] * m.m[6] * m.m[9] - 
                    m.m[4] * m.m[1] * m.m[10] + m.m[4] * m.m[2] * m.m[9] + 
                    m.m[8] * m.m[1] * m.m[6] - m.m[8] * m.m[2] * m.m[5];
        
        det = m.m[0] * inv.m[0] + m.m[1] * inv.m[4] + m.m[2] * inv.m[8] + m.m[3] * inv.m[12];
        
        if (std::abs(det) < 0.0001f) {
            return Mat4::identity();
        }
        
        det = 1.0f / det;
        
        for (i = 0; i < 16; i++) {
            inv.m[i] *= det;
        }
        
        return inv;
    }
};

// ===== PCSS (Percentage Closer Soft Shadows) =====
class PCSShadows {
public:
    PCSSSettings settings;
    
    // Poisson disk samples for shadow filtering
    static constexpr int MAX_SAMPLES = 64;
    Vec3 poissonDisk[MAX_SAMPLES];
    int sampleCount = 32;
    
    PCSShadows() {
        generatePoissonDisk();
    }
    
    void generatePoissonDisk() {
        // Pre-computed Poisson disk distribution
        const float samples[64][2] = {
            {-0.94201624f, -0.39906216f}, {0.94558609f, -0.76890725f},
            {-0.094184101f, -0.92938870f}, {0.34495938f, 0.29387760f},
            {-0.91588581f, 0.45771432f}, {-0.81544232f, -0.87912464f},
            {-0.38277543f, 0.27676845f}, {0.97484398f, 0.75648379f},
            {0.44323325f, -0.97511554f}, {0.53742981f, -0.47373420f},
            {-0.26496911f, -0.41893023f}, {0.79197514f, 0.19090188f},
            {-0.24188840f, 0.99706507f}, {-0.81409955f, 0.91437590f},
            {0.19984126f, 0.78641367f}, {0.14383161f, -0.14100790f},
            {-0.44451493f, -0.94792867f}, {0.69757803f, 0.45741895f},
            {-0.67885357f, 0.65068054f}, {0.48769018f, 0.95898765f},
            {-0.98986587f, -0.06762656f}, {0.95856935f, -0.04012432f},
            {-0.56899232f, -0.65874276f}, {0.18176234f, 0.43654876f},
            {-0.34567546f, 0.76543876f}, {0.65476543f, -0.23456789f},
            {-0.76543210f, -0.12345678f}, {0.23456789f, -0.87654321f},
            {-0.12345678f, 0.54321098f}, {0.87654321f, 0.12345678f},
            {-0.54321098f, -0.76543210f}, {0.43210987f, 0.65432109f},
            // ... more samples would go here
        };
        
        for (int i = 0; i < MAX_SAMPLES && i < 32; i++) {
            poissonDisk[i] = Vec3(samples[i][0], samples[i][1], 0.0f);
        }
    }
    
    // Calculate penumbra size based on blocker distance
    float calculatePenumbraSize(float receiverDepth, float blockerDepth) const {
        if (blockerDepth >= receiverDepth) return settings.minPenumbraSize;
        
        float penumbra = settings.lightSize * (receiverDepth - blockerDepth) / blockerDepth;
        return std::clamp(penumbra * settings.contactHardeningScale,
                         settings.minPenumbraSize, settings.maxPenumbraSize);
    }
};

// ===== Shadow Map Shader Code =====
namespace ShadowShaders {

// Shadow map generation vertex shader
inline const char* shadowMapVertexShader = R"(
struct ShadowUniforms {
    float4x4 lightViewProjection;
    float2 depthBias;
};

struct VertexIn {
    float3 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[position]];
    float depth;
};

vertex VertexOut shadowMapVertex(
    VertexIn in [[stage_in]],
    constant ShadowUniforms& uniforms [[buffer(1)]],
    constant float4x4& modelMatrix [[buffer(2)]]
) {
    VertexOut out;
    float4 worldPos = modelMatrix * float4(in.position, 1.0);
    out.position = uniforms.lightViewProjection * worldPos;
    out.depth = out.position.z / out.position.w;
    return out;
}

fragment float4 shadowMapFragment(VertexOut in [[stage_in]]) {
    return float4(in.depth, in.depth * in.depth, 0.0, 1.0);  // Store depth and depth^2 for VSM
}
)";

// PCF shadow sampling
inline const char* pcfShadowShader = R"(
float sampleShadowPCF(
    texture2d<float> shadowMap,
    sampler shadowSampler,
    float3 shadowCoord,
    float2 texelSize,
    float bias,
    int samples
) {
    float shadow = 0.0;
    float currentDepth = shadowCoord.z - bias;
    
    // 4x4 PCF
    for (int x = -2; x <= 1; x++) {
        for (int y = -2; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            float closestDepth = shadowMap.sample(shadowSampler, shadowCoord.xy + offset).r;
            shadow += currentDepth > closestDepth ? 1.0 : 0.0;
        }
    }
    
    return shadow / 16.0;
}
)";

// PCSS shadow sampling
inline const char* pcssShadowShader = R"(
// Poisson disk samples (declared in buffer)
constant float2 poissonDisk[32] = {
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790),
    float2(-0.44451493, -0.94792867), float2(0.69757803, 0.45741895),
    float2(-0.67885357, 0.65068054), float2(0.48769018, 0.95898765),
    float2(-0.98986587, -0.06762656), float2(0.95856935, -0.04012432),
    float2(-0.56899232, -0.65874276), float2(0.18176234, 0.43654876),
    float2(-0.34567546, 0.76543876), float2(0.65476543, -0.23456789),
    float2(-0.76543210, -0.12345678), float2(0.23456789, -0.87654321),
    float2(-0.12345678, 0.54321098), float2(0.87654321, 0.12345678),
    float2(-0.54321098, -0.76543210), float2(0.43210987, 0.65432109)
};

// Blocker search
float findBlockerDepth(
    texture2d<float> shadowMap,
    sampler shadowSampler,
    float3 shadowCoord,
    float searchRadius,
    int samples
) {
    float blockerSum = 0.0;
    float blockerCount = 0.0;
    float receiverDepth = shadowCoord.z;
    
    for (int i = 0; i < samples; i++) {
        float2 offset = poissonDisk[i] * searchRadius;
        float shadowDepth = shadowMap.sample(shadowSampler, shadowCoord.xy + offset).r;
        
        if (shadowDepth < receiverDepth) {
            blockerSum += shadowDepth;
            blockerCount += 1.0;
        }
    }
    
    if (blockerCount < 1.0) return -1.0;  // No blockers
    return blockerSum / blockerCount;
}

// PCSS main function
float sampleShadowPCSS(
    texture2d<float> shadowMap,
    sampler shadowSampler,
    float3 shadowCoord,
    float2 texelSize,
    float lightSize,
    float bias
) {
    // Step 1: Blocker search
    float searchRadius = lightSize * shadowCoord.z;
    float blockerDepth = findBlockerDepth(shadowMap, shadowSampler, shadowCoord, searchRadius, 16);
    
    if (blockerDepth < 0.0) {
        return 0.0;  // No shadow
    }
    
    // Step 2: Calculate penumbra size
    float penumbra = lightSize * (shadowCoord.z - blockerDepth) / blockerDepth;
    penumbra = clamp(penumbra, texelSize.x, texelSize.x * 32.0);
    
    // Step 3: PCF with variable filter size
    float shadow = 0.0;
    float currentDepth = shadowCoord.z - bias;
    
    for (int i = 0; i < 32; i++) {
        float2 offset = poissonDisk[i] * penumbra;
        float closestDepth = shadowMap.sample(shadowSampler, shadowCoord.xy + offset).r;
        shadow += currentDepth > closestDepth ? 1.0 : 0.0;
    }
    
    return shadow / 32.0;
}
)";

// CSM shadow sampling with cascade selection
inline const char* csmShadowShader = R"(
struct CSMUniforms {
    float4x4 cascadeViewProjections[4];
    float4 cascadeSplits;  // View-space split distances
    float4 shadowParams;   // x: bias, y: normalBias, z: texelSize, w: cascadeCount
};

int selectCascade(float viewDepth, float4 splits, int cascadeCount) {
    for (int i = 0; i < cascadeCount; i++) {
        if (viewDepth < splits[i]) {
            return i;
        }
    }
    return cascadeCount - 1;
}

float sampleCascadedShadow(
    texture2d_array<float> shadowMapArray,
    sampler shadowSampler,
    float3 worldPos,
    float viewDepth,
    float3 normal,
    constant CSMUniforms& csm
) {
    int cascadeCount = int(csm.shadowParams.w);
    int cascade = selectCascade(viewDepth, csm.cascadeSplits, cascadeCount);
    
    // Transform to shadow space
    float4 shadowPos = csm.cascadeViewProjections[cascade] * float4(worldPos, 1.0);
    shadowPos.xyz /= shadowPos.w;
    shadowPos.xy = shadowPos.xy * 0.5 + 0.5;
    shadowPos.y = 1.0 - shadowPos.y;
    
    // Normal bias
    float bias = csm.shadowParams.x + csm.shadowParams.y * (1.0 - dot(normal, float3(0, 1, 0)));
    
    // Sample shadow map
    float2 texelSize = float2(csm.shadowParams.z);
    float shadow = 0.0;
    
    // 3x3 PCF
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            float depth = shadowMapArray.sample(shadowSampler, shadowPos.xy + offset, cascade).r;
            shadow += shadowPos.z - bias > depth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    // Cascade edge fading
    float2 edge = abs(shadowPos.xy * 2.0 - 1.0);
    float edgeFade = 1.0 - smoothstep(0.9, 1.0, max(edge.x, edge.y));
    
    return shadow * edgeFade;
}
)";

}  // namespace ShadowShaders

// ===== Shadow Quality Presets =====
namespace ShadowPresets {

inline CSMSettings low() {
    CSMSettings s;
    s.numCascades = 1;
    s.shadowMapSize = 1024;
    s.pcfSamples = 4;
    return s;
}

inline CSMSettings medium() {
    CSMSettings s;
    s.numCascades = 2;
    s.shadowMapSize = 2048;
    s.pcfSamples = 9;
    return s;
}

inline CSMSettings high() {
    CSMSettings s;
    s.numCascades = 4;
    s.shadowMapSize = 2048;
    s.pcfSamples = 16;
    return s;
}

inline CSMSettings ultra() {
    CSMSettings s;
    s.numCascades = 4;
    s.shadowMapSize = 4096;
    s.pcfSamples = 32;
    s.stabilizeCascades = true;
    return s;
}

inline PCSSSettings softShadows() {
    PCSSSettings s;
    s.lightSize = 0.02f;
    s.enableContactHardening = true;
    return s;
}

inline PCSSSettings verysSoftShadows() {
    PCSSSettings s;
    s.lightSize = 0.05f;
    s.penumbraSamples = 64;
    s.enableContactHardening = true;
    return s;
}

}  // namespace ShadowPresets

}  // namespace luma
