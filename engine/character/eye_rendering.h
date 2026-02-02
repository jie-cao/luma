// Eye Rendering System - Realistic eye rendering with refraction
// High-quality eye rendering with parallax, wetness, and caustics
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <cmath>
#include <vector>

namespace luma {

// ============================================================================
// Eye Anatomy Layers
// ============================================================================

enum class EyeLayer {
    Sclera,         // White of eye
    Limbus,         // Dark ring around iris
    Iris,           // Colored part
    Pupil,          // Black center
    Cornea,         // Clear outer layer
    Wetness         // Tear film layer
};

// ============================================================================
// Eye Material Parameters
// ============================================================================

struct EyeMaterialParams {
    // === Sclera (white) ===
    Vec3 scleraColor{0.95f, 0.93f, 0.91f};
    float scleraRoughness = 0.3f;
    Vec3 scleraVeinColor{0.8f, 0.3f, 0.2f};
    float scleraVeinIntensity = 0.2f;
    float scleraSubsurface = 0.3f;
    
    // === Limbus (iris edge ring) ===
    Vec3 limbusColor{0.15f, 0.12f, 0.1f};
    float limbusWidth = 0.05f;
    float limbusFalloff = 2.0f;
    
    // === Iris ===
    Vec3 irisInnerColor{0.4f, 0.6f, 0.8f};    // Inner iris color
    Vec3 irisOuterColor{0.2f, 0.4f, 0.6f};    // Outer iris color
    float irisRadius = 0.35f;
    float irisRoughness = 0.8f;
    float irisDepth = 0.02f;                   // Parallax depth
    
    // Iris pattern
    float irisPatternFrequency = 20.0f;
    float irisPatternStrength = 0.3f;
    float irisCryptFrequency = 8.0f;
    float irisCryptStrength = 0.2f;
    
    // === Pupil ===
    Vec3 pupilColor{0.02f, 0.02f, 0.02f};
    float pupilRadius = 0.15f;
    float pupilSharpness = 50.0f;
    
    // === Cornea ===
    float corneaIOR = 1.376f;                 // Index of refraction
    float corneaBulge = 0.03f;                // How much cornea bulges over iris
    float corneaRoughness = 0.0f;
    
    // === Wetness (tear film) ===
    float wetnessAmount = 0.5f;
    float wetnessRoughness = 0.02f;
    Vec3 wetnessSpecularColor{1, 1, 1};
    
    // === Reflections ===
    float environmentReflection = 0.5f;
    float specularIntensity = 1.0f;
    
    // === Caustics ===
    float causticStrength = 0.3f;
    float causticScale = 5.0f;
    
    // === Animation ===
    float pupilDilation = 0.0f;               // -1 to 1 (constrict to dilate)
};

// ============================================================================
// Eye Mesh Generator
// ============================================================================

class EyeMeshGenerator {
public:
    // Generate anatomically correct eye mesh with multiple layers
    static Mesh generateEyeMesh(float radius = 0.012f,      // ~12mm eye radius
                                int segments = 64,
                                int rings = 32) {
        Mesh mesh;
        
        // Generate sphere with UVs suitable for eye
        for (int ring = 0; ring <= rings; ring++) {
            float v = static_cast<float>(ring) / rings;
            float phi = v * 3.14159f;
            
            for (int seg = 0; seg <= segments; seg++) {
                float u = static_cast<float>(seg) / segments;
                float theta = u * 3.14159f * 2.0f;
                
                Vertex vert;
                
                // Position on sphere
                vert.position.x = radius * std::sin(phi) * std::cos(theta);
                vert.position.y = radius * std::sin(phi) * std::sin(theta);
                vert.position.z = radius * std::cos(phi);
                
                // Normal
                vert.normal = vert.position.normalized();
                
                // UV for front-facing projection (looking at +Z)
                float nx = vert.normal.x;
                float ny = vert.normal.y;
                float nz = vert.normal.z;
                
                // Cylindrical projection for UVs
                vert.texCoord.x = 0.5f + std::atan2(nx, nz) / (2.0f * 3.14159f);
                vert.texCoord.y = 0.5f - ny * 0.5f;
                
                // Tangent
                vert.tangent = Vec3(-std::sin(theta), 0, std::cos(theta));
                
                mesh.vertices.push_back(vert);
            }
        }
        
        // Generate indices
        for (int ring = 0; ring < rings; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                int current = ring * (segments + 1) + seg;
                int next = current + segments + 1;
                
                mesh.indices.push_back(current);
                mesh.indices.push_back(next);
                mesh.indices.push_back(current + 1);
                
                mesh.indices.push_back(current + 1);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }
        
        return mesh;
    }
    
    // Generate cornea bulge mesh (overlay on eye)
    static Mesh generateCorneaMesh(float eyeRadius = 0.012f,
                                    float corneaRadius = 0.008f,
                                    float bulgeHeight = 0.002f,
                                    int segments = 32) {
        Mesh mesh;
        
        // Cornea is a dome over the front of the eye
        for (int ring = 0; ring <= segments / 2; ring++) {
            float v = static_cast<float>(ring) / (segments / 2);
            float phi = v * 3.14159f * 0.5f;  // Half sphere
            
            for (int seg = 0; seg <= segments; seg++) {
                float u = static_cast<float>(seg) / segments;
                float theta = u * 3.14159f * 2.0f;
                
                Vertex vert;
                
                // Dome shape
                float r = corneaRadius * std::sin(phi);
                float z = eyeRadius + bulgeHeight * std::cos(phi);
                
                vert.position.x = r * std::cos(theta);
                vert.position.y = r * std::sin(theta);
                vert.position.z = z;
                
                // Normal pointing outward from dome
                vert.normal = Vec3(
                    std::sin(phi) * std::cos(theta),
                    std::sin(phi) * std::sin(theta),
                    std::cos(phi)
                ).normalized();
                
                vert.texCoord.x = u;
                vert.texCoord.y = v;
                
                mesh.vertices.push_back(vert);
            }
        }
        
        // Generate indices
        int vertsPerRing = segments + 1;
        for (int ring = 0; ring < segments / 2; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                int current = ring * vertsPerRing + seg;
                int next = current + vertsPerRing;
                
                mesh.indices.push_back(current);
                mesh.indices.push_back(next);
                mesh.indices.push_back(current + 1);
                
                mesh.indices.push_back(current + 1);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }
        
        return mesh;
    }
};

// ============================================================================
// Eye Texture Generator - Procedural eye textures
// ============================================================================

class EyeTextureGenerator {
public:
    // Generate complete iris texture
    static TextureData generateIrisTexture(int size = 512,
                                            const EyeMaterialParams& params = {}) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        float center = size / 2.0f;
        float irisPixelRadius = params.irisRadius * size * 0.5f;
        float pupilPixelRadius = params.pupilRadius * size * 0.5f;
        float limbusInner = irisPixelRadius * (1.0f - params.limbusWidth);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = x - center;
                float dy = y - center;
                float dist = std::sqrt(dx * dx + dy * dy);
                float angle = std::atan2(dy, dx);
                
                Vec3 color;
                float alpha = 1.0f;
                
                if (dist < pupilPixelRadius) {
                    // Pupil
                    float edge = smoothstep(pupilPixelRadius - 2.0f, pupilPixelRadius, dist);
                    color = params.pupilColor.lerp(params.irisInnerColor, edge);
                } else if (dist < irisPixelRadius) {
                    // Iris
                    float t = (dist - pupilPixelRadius) / (irisPixelRadius - pupilPixelRadius);
                    
                    // Base color gradient
                    color = params.irisInnerColor.lerp(params.irisOuterColor, t);
                    
                    // Radial fibers (collarette pattern)
                    float fibers = std::sin(angle * params.irisPatternFrequency);
                    fibers = fibers * 0.5f + 0.5f;
                    fibers *= params.irisPatternStrength * (1.0f - t * 0.5f);
                    
                    // Crypts (darker regions)
                    float cryptAngle = angle + noise1D(t * 10.0f) * 0.5f;
                    float crypts = std::sin(cryptAngle * params.irisCryptFrequency);
                    crypts = std::max(0.0f, crypts);
                    crypts *= params.irisCryptStrength;
                    
                    // Furrows (concentric rings)
                    float furrows = std::sin(t * 30.0f);
                    furrows = furrows * 0.5f + 0.5f;
                    furrows *= 0.1f;
                    
                    // Combine
                    float brightness = 1.0f + fibers - crypts + furrows;
                    color = color * brightness;
                    
                    // Limbus darkening at edge
                    if (dist > limbusInner) {
                        float limbusT = (dist - limbusInner) / (irisPixelRadius - limbusInner);
                        limbusT = std::pow(limbusT, params.limbusFalloff);
                        color = color.lerp(params.limbusColor, limbusT * 0.7f);
                    }
                } else {
                    // Outside iris - transparent
                    color = Vec3(0, 0, 0);
                    alpha = 0.0f;
                }
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(std::clamp(color.x, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(std::clamp(color.y, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(std::clamp(color.z, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 255);
            }
        }
        
        return tex;
    }
    
    // Generate sclera (white) texture with veins
    static TextureData generateScleraTexture(int size = 512,
                                              const EyeMaterialParams& params = {}) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = static_cast<float>(x) / size;
                float v = static_cast<float>(y) / size;
                
                Vec3 color = params.scleraColor;
                
                // Add subtle variation
                float variation = noise2D(u * 10.0f, v * 10.0f) * 0.05f;
                color = color * (1.0f + variation);
                
                // Blood vessels (veins)
                if (params.scleraVeinIntensity > 0) {
                    float veinNoise = 0;
                    
                    // Main veins
                    for (int i = 0; i < 5; i++) {
                        float angle = i * 0.7f + 0.3f;
                        float veinX = u * std::cos(angle) + v * std::sin(angle);
                        float veinPattern = std::sin(veinX * 20.0f + noise2D(u * 5, v * 5) * 5.0f);
                        veinPattern = std::pow(std::max(0.0f, veinPattern), 8.0f);
                        veinNoise += veinPattern * 0.3f;
                    }
                    
                    // Capillaries
                    float capillaries = noise2D(u * 50.0f, v * 50.0f);
                    capillaries = std::pow(std::max(0.0f, capillaries - 0.3f), 2.0f) * 0.5f;
                    
                    float totalVein = (veinNoise + capillaries) * params.scleraVeinIntensity;
                    color = color.lerp(params.scleraVeinColor, totalVein);
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
    
    // Generate iris normal map for parallax
    static TextureData generateIrisNormalMap(int size = 512,
                                              const EyeMaterialParams& params = {}) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        float center = size / 2.0f;
        float irisPixelRadius = params.irisRadius * size * 0.5f;
        float pupilPixelRadius = params.pupilRadius * size * 0.5f;
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = x - center;
                float dy = y - center;
                float dist = std::sqrt(dx * dx + dy * dy);
                float angle = std::atan2(dy, dx);
                
                Vec3 normal(0, 0, 1);  // Default flat
                
                if (dist > pupilPixelRadius && dist < irisPixelRadius) {
                    float t = (dist - pupilPixelRadius) / (irisPixelRadius - pupilPixelRadius);
                    
                    // Radial fibers create height variation
                    float fiberHeight = std::sin(angle * params.irisPatternFrequency);
                    float cryptHeight = -std::cos(angle * params.irisCryptFrequency * 0.5f) * 0.5f;
                    
                    float totalHeight = (fiberHeight + cryptHeight) * 0.3f * (1.0f - t);
                    
                    // Calculate normal from height
                    float dhdx = std::cos(angle * params.irisPatternFrequency) * params.irisPatternFrequency * 0.3f;
                    float dhdy = std::sin(angle * params.irisCryptFrequency * 0.5f) * params.irisCryptFrequency * 0.15f;
                    
                    normal = Vec3(-dhdx * 0.5f, -dhdy * 0.5f, 1.0f).normalized();
                }
                
                // Pack normal to 0-255
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>((normal.x * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>((normal.y * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>((normal.z * 0.5f + 0.5f) * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate caustic pattern texture
    static TextureData generateCausticTexture(int size = 256) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 1;
        tex.pixels.resize(size * size);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = static_cast<float>(x) / size;
                float v = static_cast<float>(y) / size;
                
                // Voronoi-based caustic pattern
                float caustic = 0;
                for (int i = 0; i < 3; i++) {
                    float scale = 5.0f * (i + 1);
                    float n = voronoiNoise(u * scale, v * scale);
                    caustic += n / (i + 1);
                }
                
                caustic = std::pow(caustic, 2.0f);
                
                tex.pixels[y * size + x] = static_cast<uint8_t>(std::clamp(caustic, 0.0f, 1.0f) * 255);
            }
        }
        
        return tex;
    }
    
private:
    static float smoothstep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
    static float noise1D(float x) {
        int xi = static_cast<int>(std::floor(x)) & 255;
        float xf = x - std::floor(x);
        float u = xf * xf * (3.0f - 2.0f * xf);
        return lerp(hash(xi), hash(xi + 1), u) * 2.0f - 1.0f;
    }
    
    static float noise2D(float x, float y) {
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
};

// ============================================================================
// Eye Shader - Physically based eye rendering
// ============================================================================

class EyeShader {
public:
    // Compute eye shading with refraction
    static Vec3 shade(const Vec3& position,
                      const Vec3& normal,
                      const Vec3& viewDir,
                      const Vec3& lightDir,
                      const Vec3& lightColor,
                      const Vec2& uv,
                      const EyeMaterialParams& params,
                      float irisDistance) {  // Distance from iris center
        
        Vec3 result(0, 0, 0);
        
        // Determine which layer we're shading
        float pupilRadius = params.pupilRadius * (1.0f + params.pupilDilation * 0.3f);
        
        if (irisDistance < pupilRadius) {
            // Pupil - very dark, slight reflection
            result = params.pupilColor;
            
        } else if (irisDistance < params.irisRadius) {
            // Iris layer with refraction
            
            // Parallax offset for iris depth
            Vec3 viewTangent = viewDir - normal * viewDir.dot(normal);
            Vec2 parallaxOffset;
            parallaxOffset.x = viewTangent.x * params.irisDepth;
            parallaxOffset.y = viewTangent.y * params.irisDepth;
            
            // Iris color with gradient
            float t = (irisDistance - pupilRadius) / (params.irisRadius - pupilRadius);
            Vec3 irisColor = params.irisInnerColor.lerp(params.irisOuterColor, t);
            
            // Add iris pattern (would sample from texture in real implementation)
            float pattern = std::sin(std::atan2(uv.y - 0.5f, uv.x - 0.5f) * params.irisPatternFrequency);
            pattern = pattern * 0.5f + 0.5f;
            irisColor = irisColor * (1.0f + pattern * params.irisPatternStrength);
            
            result = irisColor;
            
            // Caustics from cornea refraction
            float caustic = computeCaustic(position, lightDir, params);
            result = result + Vec3(1, 1, 1) * caustic * params.causticStrength;
            
        } else {
            // Sclera
            result = params.scleraColor;
            
            // Subsurface scattering approximation
            float sss = computeSSS(normal, lightDir, viewDir);
            result = result + Vec3(0.8f, 0.3f, 0.2f) * sss * params.scleraSubsurface;
        }
        
        // === Cornea specular reflection ===
        Vec3 halfVec = (viewDir + lightDir).normalized();
        float NdotH = std::max(0.0f, normal.dot(halfVec));
        
        // Fresnel
        float fresnel = fresnelSchlick(std::max(0.0f, viewDir.dot(normal)), params.corneaIOR);
        
        // Specular (very sharp for wet cornea)
        float specPower = 1.0f / (params.corneaRoughness * params.corneaRoughness + 0.001f);
        float specular = std::pow(NdotH, specPower) * fresnel;
        
        result = result + params.wetnessSpecularColor * specular * params.specularIntensity;
        
        // === Wetness layer ===
        if (params.wetnessAmount > 0) {
            // Additional sharp highlight for tear film
            float wetnessSpec = std::pow(NdotH, 500.0f) * params.wetnessAmount;
            result = result + Vec3(1, 1, 1) * wetnessSpec;
        }
        
        // === Environment reflection ===
        Vec3 reflectDir = normal * 2.0f * normal.dot(viewDir) - viewDir;
        // Would sample environment map here
        Vec3 envColor = sampleFakeEnvironment(reflectDir);
        result = result + envColor * fresnel * params.environmentReflection;
        
        // Apply light color
        result.x *= lightColor.x;
        result.y *= lightColor.y;
        result.z *= lightColor.z;
        
        return result;
    }
    
private:
    static float fresnelSchlick(float cosTheta, float ior) {
        float r0 = (1.0f - ior) / (1.0f + ior);
        r0 = r0 * r0;
        return r0 + (1.0f - r0) * std::pow(1.0f - cosTheta, 5.0f);
    }
    
    static float computeCaustic(const Vec3& pos, const Vec3& lightDir, 
                                 const EyeMaterialParams& params) {
        // Simple caustic approximation based on light direction
        float caustic = std::sin(pos.x * params.causticScale * 10.0f + lightDir.x * 5.0f);
        caustic *= std::sin(pos.y * params.causticScale * 10.0f + lightDir.y * 5.0f);
        caustic = (caustic + 1.0f) * 0.5f;
        return caustic * caustic;
    }
    
    static float computeSSS(const Vec3& normal, const Vec3& lightDir, const Vec3& viewDir) {
        // Simple wrap lighting for SSS approximation
        float NdotL = normal.dot(lightDir);
        float wrap = (NdotL + 0.5f) / 1.5f;
        wrap = std::max(0.0f, wrap);
        
        // Back scattering
        float scatter = std::max(0.0f, -normal.dot(lightDir)) * 0.3f;
        
        return wrap * 0.5f + scatter;
    }
    
    static Vec3 sampleFakeEnvironment(const Vec3& dir) {
        // Simple gradient environment
        float t = dir.y * 0.5f + 0.5f;
        Vec3 skyColor(0.5f, 0.7f, 0.9f);
        Vec3 groundColor(0.3f, 0.25f, 0.2f);
        return groundColor.lerp(skyColor, t) * 0.3f;
    }
};

// ============================================================================
// Eye Presets - Common eye color/style presets
// ============================================================================

class EyePresets {
public:
    static EyeMaterialParams getBrownEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.35f, 0.2f, 0.1f);
        p.irisOuterColor = Vec3(0.2f, 0.12f, 0.05f);
        p.limbusColor = Vec3(0.1f, 0.08f, 0.05f);
        return p;
    }
    
    static EyeMaterialParams getBlueEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.3f, 0.5f, 0.8f);
        p.irisOuterColor = Vec3(0.15f, 0.3f, 0.6f);
        p.limbusColor = Vec3(0.1f, 0.15f, 0.25f);
        return p;
    }
    
    static EyeMaterialParams getGreenEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.3f, 0.55f, 0.35f);
        p.irisOuterColor = Vec3(0.2f, 0.4f, 0.25f);
        p.limbusColor = Vec3(0.1f, 0.15f, 0.1f);
        return p;
    }
    
    static EyeMaterialParams getHazelEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.5f, 0.4f, 0.2f);
        p.irisOuterColor = Vec3(0.25f, 0.35f, 0.25f);
        p.limbusColor = Vec3(0.12f, 0.1f, 0.08f);
        return p;
    }
    
    static EyeMaterialParams getGrayEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.5f, 0.52f, 0.55f);
        p.irisOuterColor = Vec3(0.35f, 0.37f, 0.4f);
        p.limbusColor = Vec3(0.15f, 0.15f, 0.18f);
        return p;
    }
    
    static EyeMaterialParams getAmberEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.8f, 0.55f, 0.2f);
        p.irisOuterColor = Vec3(0.5f, 0.35f, 0.15f);
        p.limbusColor = Vec3(0.2f, 0.12f, 0.05f);
        return p;
    }
    
    // Stylized/Fantasy
    static EyeMaterialParams getRedEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.9f, 0.2f, 0.15f);
        p.irisOuterColor = Vec3(0.5f, 0.1f, 0.08f);
        p.limbusColor = Vec3(0.2f, 0.05f, 0.05f);
        return p;
    }
    
    static EyeMaterialParams getVioletEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(0.6f, 0.3f, 0.8f);
        p.irisOuterColor = Vec3(0.35f, 0.15f, 0.5f);
        p.limbusColor = Vec3(0.15f, 0.08f, 0.2f);
        return p;
    }
    
    static EyeMaterialParams getGoldEye() {
        EyeMaterialParams p;
        p.irisInnerColor = Vec3(1.0f, 0.85f, 0.3f);
        p.irisOuterColor = Vec3(0.7f, 0.5f, 0.15f);
        p.limbusColor = Vec3(0.3f, 0.2f, 0.05f);
        return p;
    }
    
    // Anime style
    static EyeMaterialParams getAnimeEye(const Vec3& mainColor) {
        EyeMaterialParams p;
        p.irisInnerColor = mainColor * 1.2f;
        p.irisOuterColor = mainColor * 0.6f;
        p.limbusColor = mainColor * 0.2f;
        p.irisPatternStrength = 0.1f;  // Less detail for anime
        p.irisCryptStrength = 0.05f;
        p.specularIntensity = 2.0f;    // Big highlights
        p.pupilRadius = 0.2f;          // Larger pupils
        return p;
    }
};

// ============================================================================
// Eye Controller - Animation and dynamics
// ============================================================================

class EyeController {
public:
    // Look at target
    void lookAt(const Vec3& targetWorld, const Vec3& eyePosition) {
        Vec3 direction = (targetWorld - eyePosition).normalized();
        
        // Calculate rotation angles
        gazeYaw_ = std::atan2(direction.x, direction.z);
        gazePitch_ = std::asin(direction.y);
        
        // Clamp to realistic eye movement range
        gazeYaw_ = std::clamp(gazeYaw_, -0.6f, 0.6f);      // ~35 degrees
        gazePitch_ = std::clamp(gazePitch_, -0.4f, 0.5f);  // ~25 degrees down, 30 up
    }
    
    // Get eye rotation quaternion
    Quat getEyeRotation() const {
        return Quat::fromEuler(gazePitch_, gazeYaw_, 0);
    }
    
    // Pupil dilation based on light
    float computePupilDilation(float ambientLight) {
        // Brighter = more constricted (-1), darker = more dilated (+1)
        return 1.0f - ambientLight * 2.0f;
    }
    
    // Blink animation
    float getBlinkAmount(float time) {
        // Automatic blink every ~4 seconds
        float blinkPeriod = 4.0f + std::sin(time * 0.3f) * 1.0f;
        float blinkTime = std::fmod(time, blinkPeriod);
        
        if (blinkTime < 0.15f) {
            // Close
            return std::sin(blinkTime / 0.15f * 3.14159f * 0.5f);
        } else if (blinkTime < 0.3f) {
            // Open
            return std::cos((blinkTime - 0.15f) / 0.15f * 3.14159f * 0.5f);
        }
        return 0.0f;
    }
    
    // Microsaccades (tiny eye movements)
    Vec2 getMicrosaccade(float time) {
        float x = std::sin(time * 7.3f) * 0.002f + std::sin(time * 11.7f) * 0.001f;
        float y = std::sin(time * 5.1f) * 0.002f + std::sin(time * 13.3f) * 0.001f;
        return Vec2(x, y);
    }
    
private:
    float gazeYaw_ = 0;
    float gazePitch_ = 0;
};

}  // namespace luma
