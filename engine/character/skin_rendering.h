// Skin Rendering System - Subsurface Scattering (SSS)
// High-quality skin rendering with pre-integrated SSS
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <cmath>
#include <vector>
#include <array>

namespace luma {

// ============================================================================
// Skin Material Parameters
// ============================================================================

struct SkinMaterialParams {
    // === Base color ===
    Vec3 baseColor{0.85f, 0.72f, 0.65f};
    float baseColorVariation = 0.05f;
    
    // === Subsurface Scattering ===
    Vec3 subsurfaceColor{0.8f, 0.25f, 0.15f};  // Red-ish for blood
    float subsurfaceRadius = 0.01f;             // Scatter radius in world units
    float subsurfaceStrength = 0.5f;
    
    // Three-layer SSS (epidermis, dermis, subcutaneous)
    Vec3 epidermisColor{0.75f, 0.6f, 0.5f};
    Vec3 dermisColor{0.9f, 0.3f, 0.2f};        // Blood-rich layer
    Vec3 subcutaneousColor{0.95f, 0.85f, 0.7f}; // Fat layer
    
    float epidermisWeight = 0.3f;
    float dermisWeight = 0.5f;
    float subcutaneousWeight = 0.2f;
    
    // === Surface properties ===
    float roughness = 0.35f;
    float specularIntensity = 0.4f;
    float fresnelStrength = 0.04f;
    
    // === Oil/moisture layer ===
    float oilAmount = 0.3f;
    float oilRoughness = 0.1f;
    
    // === Pores ===
    float poreSize = 0.002f;
    float poreDepth = 0.1f;
    float poreDensity = 50.0f;
    
    // === Wrinkles ===
    float wrinkleDepth = 0.05f;
    float microWrinkleStrength = 0.3f;
    
    // === Blood flow ===
    float blushAmount = 0.0f;           // 0-1 for blushing effect
    Vec3 blushColor{0.9f, 0.4f, 0.4f};
    Vec3 blushRegion{0.5f, 0.6f, 0.0f}; // Face region (cheeks)
    
    // === Veins ===
    float veinVisibility = 0.2f;
    Vec3 veinColor{0.3f, 0.4f, 0.6f};
    
    // === Freckles/moles ===
    float freckleAmount = 0.0f;
    Vec3 freckleColor{0.5f, 0.35f, 0.25f};
    
    // === Ambient occlusion ===
    float aoStrength = 0.5f;
    float microAO = 0.3f;
    
    // === Translucency ===
    float translucency = 0.3f;          // Back-lit glow (ears, fingers)
    Vec3 translucencyColor{0.95f, 0.4f, 0.3f};
};

// ============================================================================
// Pre-integrated SSS Lookup Table
// ============================================================================

class SSSLookupTable {
public:
    static constexpr int LUT_SIZE = 128;
    
    // Generate pre-integrated skin BRDF lookup table
    // X axis: NdotL (wrapped)
    // Y axis: curvature
    static std::vector<Vec3> generateLUT() {
        std::vector<Vec3> lut(LUT_SIZE * LUT_SIZE);
        
        for (int y = 0; y < LUT_SIZE; y++) {
            float curvature = static_cast<float>(y) / (LUT_SIZE - 1);
            
            for (int x = 0; x < LUT_SIZE; x++) {
                float NdotL = (static_cast<float>(x) / (LUT_SIZE - 1)) * 2.0f - 1.0f;
                
                // Integrate SSS contribution
                Vec3 scatter = integrateSSS(NdotL, curvature);
                
                lut[y * LUT_SIZE + x] = scatter;
            }
        }
        
        return lut;
    }
    
    // Sample the LUT
    static Vec3 sample(const std::vector<Vec3>& lut, float NdotL, float curvature) {
        float u = (NdotL * 0.5f + 0.5f) * (LUT_SIZE - 1);
        float v = std::clamp(curvature, 0.0f, 1.0f) * (LUT_SIZE - 1);
        
        int x0 = std::clamp(static_cast<int>(u), 0, LUT_SIZE - 1);
        int y0 = std::clamp(static_cast<int>(v), 0, LUT_SIZE - 1);
        int x1 = std::min(x0 + 1, LUT_SIZE - 1);
        int y1 = std::min(y0 + 1, LUT_SIZE - 1);
        
        float fx = u - x0;
        float fy = v - y0;
        
        Vec3 v00 = lut[y0 * LUT_SIZE + x0];
        Vec3 v10 = lut[y0 * LUT_SIZE + x1];
        Vec3 v01 = lut[y1 * LUT_SIZE + x0];
        Vec3 v11 = lut[y1 * LUT_SIZE + x1];
        
        Vec3 v0 = v00 * (1 - fx) + v10 * fx;
        Vec3 v1 = v01 * (1 - fx) + v11 * fx;
        
        return v0 * (1 - fy) + v1 * fy;
    }
    
private:
    static Vec3 integrateSSS(float NdotL, float curvature) {
        // Gaussian profiles for skin layers
        // Based on "A Practical Model for Subsurface Light Transport" (Jensen et al.)
        
        Vec3 result(0, 0, 0);
        
        // Profile widths (in normalized space)
        float sigmaR = 0.0484f + curvature * 0.187f;
        float sigmaG = 0.0187f + curvature * 0.0821f;
        float sigmaB = 0.0051f + curvature * 0.0216f;
        
        // Wrap lighting with SSS
        float wrapR = gaussian(NdotL, sigmaR);
        float wrapG = gaussian(NdotL, sigmaG);
        float wrapB = gaussian(NdotL, sigmaB);
        
        // Apply skin color tint
        result.x = std::clamp(wrapR * 1.0f, 0.0f, 1.0f);
        result.y = std::clamp(wrapG * 0.8f, 0.0f, 1.0f);
        result.z = std::clamp(wrapB * 0.6f, 0.0f, 1.0f);
        
        return result;
    }
    
    static float gaussian(float x, float sigma) {
        float invSigma = 1.0f / (sigma + 0.001f);
        return std::exp(-x * x * invSigma * invSigma * 0.5f);
    }
};

// ============================================================================
// Skin Shader - Advanced skin rendering
// ============================================================================

class SkinShader {
public:
    // Initialize with pre-computed LUT
    void initialize() {
        sssLUT_ = SSSLookupTable::generateLUT();
    }
    
    // Main skin shading function
    Vec3 shade(const Vec3& position,
               const Vec3& normal,
               const Vec3& viewDir,
               const Vec3& lightDir,
               const Vec3& lightColor,
               float curvature,                 // Surface curvature
               float thickness,                 // Mesh thickness for translucency
               const SkinMaterialParams& params) {
        
        Vec3 result(0, 0, 0);
        
        // === Diffuse with SSS ===
        float NdotL = normal.dot(lightDir);
        
        // Sample pre-integrated SSS
        Vec3 sss = SSSLookupTable::sample(sssLUT_, NdotL, curvature);
        
        // Apply skin layer colors
        Vec3 diffuse = params.baseColor;
        diffuse = diffuse * (sss.x * params.epidermisWeight +
                            sss.y * params.dermisWeight +
                            sss.z * params.subcutaneousWeight);
        
        // Scale by subsurface strength
        diffuse = diffuse * params.subsurfaceStrength + 
                  params.baseColor * (1.0f - params.subsurfaceStrength) * std::max(0.0f, NdotL);
        
        result = result + diffuse;
        
        // === Specular ===
        Vec3 halfVec = (viewDir + lightDir).normalized();
        float NdotH = std::max(0.0f, normal.dot(halfVec));
        float NdotV = std::max(0.0f, normal.dot(viewDir));
        
        // Dual-lobe specular (skin + oil)
        float spec1 = pow5Specular(NdotH, params.roughness);
        float spec2 = pow5Specular(NdotH, params.oilRoughness) * params.oilAmount;
        
        // Fresnel
        float fresnel = schlickFresnel(NdotV, params.fresnelStrength);
        
        Vec3 specular = Vec3(1, 1, 1) * (spec1 + spec2) * fresnel * params.specularIntensity;
        result = result + specular;
        
        // === Translucency (backlit) ===
        if (params.translucency > 0 && thickness < 0.1f) {
            float translucent = computeTranslucency(normal, viewDir, lightDir, thickness);
            result = result + params.translucencyColor * translucent * params.translucency;
        }
        
        // === Apply light color ===
        result.x *= lightColor.x;
        result.y *= lightColor.y;
        result.z *= lightColor.z;
        
        return result;
    }
    
    // Compute curvature from mesh (for pre-computation)
    static std::vector<float> computeCurvature(const Mesh& mesh) {
        std::vector<float> curvature(mesh.vertices.size(), 0.0f);
        
        // Simple curvature approximation using neighbor normals
        std::vector<std::vector<int>> adjacency(mesh.vertices.size());
        
        // Build adjacency
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            int i0 = mesh.indices[i];
            int i1 = mesh.indices[i + 1];
            int i2 = mesh.indices[i + 2];
            
            adjacency[i0].push_back(i1);
            adjacency[i0].push_back(i2);
            adjacency[i1].push_back(i0);
            adjacency[i1].push_back(i2);
            adjacency[i2].push_back(i0);
            adjacency[i2].push_back(i1);
        }
        
        // Compute curvature
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            const Vec3& n = mesh.vertices[i].normal;
            float totalCurve = 0;
            int count = 0;
            
            for (int neighbor : adjacency[i]) {
                const Vec3& nn = mesh.vertices[neighbor].normal;
                float dot = n.dot(nn);
                totalCurve += 1.0f - dot;  // Higher when normals differ
                count++;
            }
            
            if (count > 0) {
                curvature[i] = totalCurve / count;
            }
        }
        
        return curvature;
    }
    
    // Compute thickness for translucency
    static std::vector<float> computeThickness(const Mesh& mesh, int numRays = 16) {
        std::vector<float> thickness(mesh.vertices.size(), 1.0f);
        
        // Simple approximation: shoot rays from each vertex along -normal
        // In production, use ray tracing or baking
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            const Vec3& pos = mesh.vertices[i].position;
            const Vec3& normal = mesh.vertices[i].normal;
            
            // Find nearest backface intersection
            float minDist = 1.0f;  // Max thickness
            
            for (size_t j = 0; j < mesh.indices.size(); j += 3) {
                int i0 = mesh.indices[j];
                int i1 = mesh.indices[j + 1];
                int i2 = mesh.indices[j + 2];
                
                const Vec3& v0 = mesh.vertices[i0].position;
                const Vec3& v1 = mesh.vertices[i1].position;
                const Vec3& v2 = mesh.vertices[i2].position;
                
                // Simple ray-triangle test (Möller–Trumbore)
                float t = rayTriangleIntersect(pos, normal * -1.0f, v0, v1, v2);
                if (t > 0.001f && t < minDist) {
                    minDist = t;
                }
            }
            
            thickness[i] = minDist;
        }
        
        return thickness;
    }
    
private:
    std::vector<Vec3> sssLUT_;
    
    static float pow5Specular(float NdotH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
        return a2 / (3.14159f * d * d + 0.0001f);
    }
    
    static float schlickFresnel(float NdotV, float f0) {
        return f0 + (1.0f - f0) * std::pow(1.0f - NdotV, 5.0f);
    }
    
    static float computeTranslucency(const Vec3& normal, const Vec3& viewDir,
                                      const Vec3& lightDir, float thickness) {
        // Forward scattering approximation
        Vec3 scatterDir = (lightDir + normal * 0.5f).normalized();
        float scatter = std::max(0.0f, viewDir.dot(scatterDir * -1.0f));
        scatter = std::pow(scatter, 2.0f);
        
        // Thinner = more translucent
        float thicknessFactor = std::exp(-thickness * 20.0f);
        
        return scatter * thicknessFactor;
    }
    
    static float rayTriangleIntersect(const Vec3& origin, const Vec3& dir,
                                       const Vec3& v0, const Vec3& v1, const Vec3& v2) {
        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        Vec3 h = dir.cross(e2);
        float a = e1.dot(h);
        
        if (std::abs(a) < 0.0001f) return -1.0f;
        
        float f = 1.0f / a;
        Vec3 s = origin - v0;
        float u = f * s.dot(h);
        
        if (u < 0.0f || u > 1.0f) return -1.0f;
        
        Vec3 q = s.cross(e1);
        float v = f * dir.dot(q);
        
        if (v < 0.0f || u + v > 1.0f) return -1.0f;
        
        return f * e2.dot(q);
    }
};

// ============================================================================
// Skin Texture Generator - Procedural skin details
// ============================================================================

class SkinTextureGenerator {
public:
    // Generate skin detail normal map (pores, micro-wrinkles)
    static TextureData generateSkinDetailNormal(int size = 1024,
                                                 const SkinMaterialParams& params = {}) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = static_cast<float>(x) / size;
                float v = static_cast<float>(y) / size;
                
                Vec3 normal(0, 0, 1);
                
                // Pores
                float poreNoise = voronoiNoise(u * params.poreDensity, v * params.poreDensity);
                poreNoise = std::pow(poreNoise, 0.5f) * params.poreDepth;
                
                // Micro wrinkles
                float wrinkle1 = std::sin(u * 100.0f + perlinNoise(u * 10, v * 10) * 5.0f);
                float wrinkle2 = std::sin(v * 100.0f + perlinNoise(u * 10 + 50, v * 10 + 50) * 5.0f);
                float wrinkles = (wrinkle1 + wrinkle2) * 0.5f * params.microWrinkleStrength;
                
                // Combine into normal
                float dx = (poreNoise + wrinkles) * 0.1f;
                float dy = (poreNoise + wrinkles) * 0.1f;
                
                normal = Vec3(-dx, -dy, 1.0f).normalized();
                
                // Pack to 0-255
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>((normal.x * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>((normal.y * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>((normal.z * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate subsurface color map
    static TextureData generateSubsurfaceMap(int size = 512,
                                              const SkinMaterialParams& params = {}) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = static_cast<float>(x) / size;
                float v = static_cast<float>(y) / size;
                
                // Base subsurface color with variation
                Vec3 color = params.subsurfaceColor;
                
                // Add some organic variation
                float variation = perlinNoise(u * 5, v * 5) * 0.2f;
                color = color * (1.0f + variation);
                
                // Veins (branching patterns)
                if (params.veinVisibility > 0) {
                    float veinPattern = 0;
                    for (int i = 0; i < 3; i++) {
                        float scale = 10.0f * (i + 1);
                        float vein = std::abs(std::sin(u * scale + perlinNoise(u * scale, v * scale) * 2.0f));
                        vein = std::pow(vein, 10.0f);
                        veinPattern += vein * 0.3f / (i + 1);
                    }
                    color = color.lerp(params.veinColor, veinPattern * params.veinVisibility);
                }
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(std::clamp(color.x, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(std::clamp(color.y, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(std::clamp(color.z, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate freckle/mole map
    static TextureData generateFreckleMap(int size = 512, float density = 0.1f,
                                           const Vec3& freckleColor = Vec3(0.5f, 0.35f, 0.25f)) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4, 0);
        
        // Generate random freckle positions
        int numFreckles = static_cast<int>(density * size * size / 100);
        
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> distPos(0, size - 1);
        std::uniform_real_distribution<float> distSize(1.0f, 4.0f);
        std::uniform_real_distribution<float> distAlpha(0.3f, 0.8f);
        
        for (int i = 0; i < numFreckles; i++) {
            int cx = distPos(rng);
            int cy = distPos(rng);
            float radius = distSize(rng);
            float alpha = distAlpha(rng);
            
            // Draw freckle
            for (int dy = -static_cast<int>(radius) - 1; dy <= static_cast<int>(radius) + 1; dy++) {
                for (int dx = -static_cast<int>(radius) - 1; dx <= static_cast<int>(radius) + 1; dx++) {
                    int px = cx + dx;
                    int py = cy + dy;
                    
                    if (px < 0 || px >= size || py < 0 || py >= size) continue;
                    
                    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (dist > radius) continue;
                    
                    float falloff = 1.0f - dist / radius;
                    falloff = falloff * falloff;
                    
                    int idx = (py * size + px) * 4;
                    float blend = falloff * alpha;
                    
                    tex.pixels[idx + 0] = std::max(tex.pixels[idx + 0],
                        static_cast<uint8_t>(freckleColor.x * blend * 255));
                    tex.pixels[idx + 1] = std::max(tex.pixels[idx + 1],
                        static_cast<uint8_t>(freckleColor.y * blend * 255));
                    tex.pixels[idx + 2] = std::max(tex.pixels[idx + 2],
                        static_cast<uint8_t>(freckleColor.z * blend * 255));
                    tex.pixels[idx + 3] = std::max(tex.pixels[idx + 3],
                        static_cast<uint8_t>(blend * 255));
                }
            }
        }
        
        return tex;
    }
    
    // Generate specular/oil map
    static TextureData generateSpecularMap(int size = 512,
                                            const SkinMaterialParams& params = {}) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 1;
        tex.pixels.resize(size * size);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = static_cast<float>(x) / size;
                float v = static_cast<float>(y) / size;
                
                // Base specular
                float spec = params.specularIntensity;
                
                // Oil variation
                float oil = perlinNoise(u * 20, v * 20) * 0.5f + 0.5f;
                oil = oil * params.oilAmount;
                
                // Higher specular in oily areas (T-zone, etc.)
                spec += oil * 0.3f;
                
                // Reduce in pore areas
                float pores = voronoiNoise(u * params.poreDensity, v * params.poreDensity);
                spec *= 0.8f + pores * 0.2f;
                
                tex.pixels[y * size + x] = static_cast<uint8_t>(std::clamp(spec, 0.0f, 1.0f) * 255);
            }
        }
        
        return tex;
    }
    
private:
    static float perlinNoise(float x, float y) {
        int xi = static_cast<int>(std::floor(x)) & 255;
        int yi = static_cast<int>(std::floor(y)) & 255;
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        
        float u = xf * xf * (3.0f - 2.0f * xf);
        float v = yf * yf * (3.0f - 2.0f * yf);
        
        float a = hash(xi + hash(yi));
        float b = hash(xi + 1 + hash(yi));
        float c = hash(xi + hash(yi + 1));
        float d = hash(xi + 1 + hash(yi + 1));
        
        return lerp(lerp(a, b, u), lerp(c, d, u), v);
    }
    
    static float voronoiNoise(float x, float y) {
        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        float xf = x - xi;
        float yf = y - yi;
        
        float minDist = 1.0f;
        
        for (int j = -1; j <= 1; j++) {
            for (int i = -1; i <= 1; i++) {
                float cellX = i + hash((xi + i) * 127 + (yi + j) * 311) - xf;
                float cellY = j + hash((xi + i) * 269 + (yi + j) * 183) - yf;
                float dist = cellX * cellX + cellY * cellY;
                minDist = std::min(minDist, dist);
            }
        }
        
        return std::sqrt(minDist);
    }
    
    static float hash(int n) {
        n = (n << 13) ^ n;
        return ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483648.0f;
    }
    
    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    std::mt19937 rng_{42};
};

// ============================================================================
// Skin Tone Presets
// ============================================================================

class SkinPresets {
public:
    static SkinMaterialParams getFairSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.92f, 0.82f, 0.78f);
        p.subsurfaceColor = Vec3(0.85f, 0.3f, 0.2f);
        p.translucencyColor = Vec3(0.95f, 0.5f, 0.4f);
        p.fresnelStrength = 0.05f;
        return p;
    }
    
    static SkinMaterialParams getLightSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.88f, 0.75f, 0.68f);
        p.subsurfaceColor = Vec3(0.8f, 0.28f, 0.18f);
        p.translucencyColor = Vec3(0.9f, 0.45f, 0.35f);
        return p;
    }
    
    static SkinMaterialParams getMediumSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.78f, 0.62f, 0.52f);
        p.subsurfaceColor = Vec3(0.7f, 0.25f, 0.15f);
        p.translucencyColor = Vec3(0.8f, 0.4f, 0.3f);
        p.subsurfaceStrength = 0.4f;
        return p;
    }
    
    static SkinMaterialParams getOliveSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.72f, 0.58f, 0.45f);
        p.subsurfaceColor = Vec3(0.65f, 0.22f, 0.12f);
        p.translucencyColor = Vec3(0.75f, 0.35f, 0.25f);
        p.subsurfaceStrength = 0.35f;
        return p;
    }
    
    static SkinMaterialParams getTanSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.65f, 0.48f, 0.38f);
        p.subsurfaceColor = Vec3(0.55f, 0.2f, 0.1f);
        p.translucencyColor = Vec3(0.65f, 0.3f, 0.2f);
        p.subsurfaceStrength = 0.3f;
        return p;
    }
    
    static SkinMaterialParams getBrownSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.52f, 0.38f, 0.3f);
        p.subsurfaceColor = Vec3(0.45f, 0.18f, 0.1f);
        p.translucencyColor = Vec3(0.55f, 0.25f, 0.18f);
        p.subsurfaceStrength = 0.25f;
        return p;
    }
    
    static SkinMaterialParams getDarkSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.38f, 0.28f, 0.22f);
        p.subsurfaceColor = Vec3(0.35f, 0.15f, 0.08f);
        p.translucencyColor = Vec3(0.45f, 0.2f, 0.12f);
        p.subsurfaceStrength = 0.2f;
        p.specularIntensity = 0.5f;  // More visible specular on dark skin
        return p;
    }
    
    static SkinMaterialParams getDeepSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.25f, 0.18f, 0.15f);
        p.subsurfaceColor = Vec3(0.25f, 0.1f, 0.05f);
        p.translucencyColor = Vec3(0.35f, 0.15f, 0.1f);
        p.subsurfaceStrength = 0.15f;
        p.specularIntensity = 0.55f;
        return p;
    }
    
    // Stylized
    static SkinMaterialParams getAnimeSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.98f, 0.92f, 0.88f);
        p.subsurfaceStrength = 0.1f;  // Less SSS for flat look
        p.roughness = 0.5f;
        p.specularIntensity = 0.2f;
        p.poreDepth = 0.0f;          // No pores
        p.microWrinkleStrength = 0.0f;
        return p;
    }
    
    static SkinMaterialParams getZombieSkin() {
        SkinMaterialParams p;
        p.baseColor = Vec3(0.5f, 0.55f, 0.45f);  // Greenish
        p.subsurfaceColor = Vec3(0.3f, 0.35f, 0.25f);
        p.subsurfaceStrength = 0.2f;
        p.veinVisibility = 0.5f;
        p.veinColor = Vec3(0.2f, 0.25f, 0.15f);
        p.roughness = 0.7f;
        return p;
    }
    
    static SkinMaterialParams getAlienSkin(const Vec3& tint) {
        SkinMaterialParams p;
        p.baseColor = tint;
        p.subsurfaceColor = tint * 0.5f;
        p.translucencyColor = tint * 1.2f;
        p.subsurfaceStrength = 0.6f;
        p.poreDepth = 0.0f;
        return p;
    }
};

// ============================================================================
// Skin Manager - Combines all skin rendering components
// ============================================================================

class SkinManager {
public:
    static SkinManager& getInstance() {
        static SkinManager instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        skinShader_.initialize();
        
        // Generate default textures
        SkinMaterialParams defaultParams;
        detailNormalMap_ = SkinTextureGenerator::generateSkinDetailNormal(1024, defaultParams);
        subsurfaceMap_ = SkinTextureGenerator::generateSubsurfaceMap(512, defaultParams);
        specularMap_ = SkinTextureGenerator::generateSpecularMap(512, defaultParams);
        
        initialized_ = true;
    }
    
    SkinShader& getShader() { return skinShader_; }
    
    const TextureData& getDetailNormalMap() const { return detailNormalMap_; }
    const TextureData& getSubsurfaceMap() const { return subsurfaceMap_; }
    const TextureData& getSpecularMap() const { return specularMap_; }
    
    // Pre-compute mesh-specific data
    void prepareMesh(const Mesh& mesh) {
        meshCurvature_ = SkinShader::computeCurvature(mesh);
        meshThickness_ = SkinShader::computeThickness(mesh);
    }
    
    float getCurvature(int vertexIndex) const {
        if (vertexIndex < 0 || vertexIndex >= static_cast<int>(meshCurvature_.size())) {
            return 0.5f;
        }
        return meshCurvature_[vertexIndex];
    }
    
    float getThickness(int vertexIndex) const {
        if (vertexIndex < 0 || vertexIndex >= static_cast<int>(meshThickness_.size())) {
            return 1.0f;
        }
        return meshThickness_[vertexIndex];
    }
    
private:
    SkinManager() = default;
    
    bool initialized_ = false;
    SkinShader skinShader_;
    
    TextureData detailNormalMap_;
    TextureData subsurfaceMap_;
    TextureData specularMap_;
    
    std::vector<float> meshCurvature_;
    std::vector<float> meshThickness_;
};

}  // namespace luma
