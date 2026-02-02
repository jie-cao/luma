// LUMA Character Texture System
// Manages skin, eye, and detail textures for character creation
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>

namespace luma {

// ============================================================================
// Texture Types
// ============================================================================

enum class CharacterTextureType {
    // Skin textures
    SkinDiffuse,        // Base color/albedo
    SkinNormal,         // Surface detail normals
    SkinRoughness,      // Roughness/glossiness
    SkinSSS,            // Subsurface scattering mask
    
    // Face detail
    FaceDetail,         // Pores, wrinkles overlay
    
    // Eye textures
    EyeIris,            // Iris pattern
    EyeSclera,          // Eye white
    EyeNormal,          // Cornea/iris normals
    
    // Other
    Lips,               // Lip color/detail
    Eyebrows,           // Eyebrow alpha mask
    Eyelashes,          // Eyelash alpha mask
    
    // Overlays
    Makeup,             // Makeup layer
    Tattoo,             // Tattoo overlay
    Freckles,           // Freckles overlay
    Wrinkles            // Age wrinkles overlay
};

// ============================================================================
// Texture Asset
// ============================================================================

struct CharacterTextureAsset {
    std::string id;
    std::string name;
    CharacterTextureType type;
    
    // Texture data
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 4;  // RGBA
    
    // Metadata
    std::string category;       // "Caucasian", "Asian", "African", etc.
    std::string ageGroup;       // "Young", "Middle", "Elderly"
    std::string gender;         // "Male", "Female", "Neutral"
    
    // Adjustment parameters
    bool supportsColorAdjust = true;
    bool supportsBlending = true;
    
    bool isValid() const { return !pixels.empty() && width > 0 && height > 0; }
};

// ============================================================================
// Skin Texture Parameters
// ============================================================================

struct SkinTextureParams {
    // Base skin tone
    Vec3 baseColor{0.85f, 0.65f, 0.5f};     // RGB
    float saturation = 1.0f;
    float brightness = 1.0f;
    
    // Subsurface scattering
    Vec3 sssColor{0.9f, 0.3f, 0.2f};        // Reddish for blood
    float sssIntensity = 0.3f;
    float sssRadius = 0.02f;
    
    // Surface properties
    float roughness = 0.5f;
    float specularIntensity = 0.3f;
    
    // Detail overlays
    float poreIntensity = 0.5f;
    float wrinkleIntensity = 0.0f;          // 0 = young, 1 = elderly
    float freckleIntensity = 0.0f;
    Vec3 freckleColor{0.6f, 0.4f, 0.3f};
    
    // Variation
    float skinVariation = 0.1f;             // Color variation across skin
    
    // Presets by ethnicity
    static SkinTextureParams caucasian() {
        SkinTextureParams p;
        p.baseColor = Vec3(0.9f, 0.75f, 0.65f);
        p.sssColor = Vec3(0.95f, 0.4f, 0.3f);
        return p;
    }
    
    static SkinTextureParams asian() {
        SkinTextureParams p;
        p.baseColor = Vec3(0.95f, 0.82f, 0.7f);
        p.sssColor = Vec3(0.9f, 0.35f, 0.25f);
        return p;
    }
    
    static SkinTextureParams african() {
        SkinTextureParams p;
        p.baseColor = Vec3(0.45f, 0.3f, 0.2f);
        p.sssColor = Vec3(0.6f, 0.25f, 0.15f);
        p.sssIntensity = 0.2f;  // Less visible SSS on darker skin
        return p;
    }
    
    static SkinTextureParams latino() {
        SkinTextureParams p;
        p.baseColor = Vec3(0.75f, 0.55f, 0.4f);
        p.sssColor = Vec3(0.85f, 0.35f, 0.25f);
        return p;
    }
    
    static SkinTextureParams middleEastern() {
        SkinTextureParams p;
        p.baseColor = Vec3(0.8f, 0.6f, 0.45f);
        p.sssColor = Vec3(0.88f, 0.38f, 0.28f);
        return p;
    }
};

// ============================================================================
// Eye Texture Parameters
// ============================================================================

struct EyeTextureParams {
    // Iris
    Vec3 irisColor{0.4f, 0.25f, 0.15f};     // Brown default
    Vec3 irisRingColor{0.3f, 0.2f, 0.1f};   // Outer ring
    float irisSize = 0.5f;
    float pupilSize = 0.3f;
    float irisDetail = 0.7f;                 // Pattern complexity
    
    // Sclera (eye white)
    Vec3 scleraColor{0.95f, 0.93f, 0.9f};
    float scleraVeins = 0.1f;               // Blood vessel visibility
    
    // Reflection/wetness
    float wetness = 0.8f;
    float corneaBump = 0.3f;
    
    // Presets
    static EyeTextureParams blue() {
        EyeTextureParams p;
        p.irisColor = Vec3(0.3f, 0.5f, 0.8f);
        p.irisRingColor = Vec3(0.2f, 0.35f, 0.6f);
        return p;
    }
    
    static EyeTextureParams green() {
        EyeTextureParams p;
        p.irisColor = Vec3(0.35f, 0.55f, 0.35f);
        p.irisRingColor = Vec3(0.25f, 0.4f, 0.25f);
        return p;
    }
    
    static EyeTextureParams brown() {
        EyeTextureParams p;
        p.irisColor = Vec3(0.4f, 0.25f, 0.15f);
        p.irisRingColor = Vec3(0.3f, 0.18f, 0.1f);
        return p;
    }
    
    static EyeTextureParams hazel() {
        EyeTextureParams p;
        p.irisColor = Vec3(0.5f, 0.4f, 0.25f);
        p.irisRingColor = Vec3(0.35f, 0.3f, 0.15f);
        return p;
    }
    
    static EyeTextureParams gray() {
        EyeTextureParams p;
        p.irisColor = Vec3(0.5f, 0.55f, 0.6f);
        p.irisRingColor = Vec3(0.4f, 0.45f, 0.5f);
        return p;
    }
};

// ============================================================================
// Lip Texture Parameters
// ============================================================================

struct LipTextureParams {
    Vec3 color{0.75f, 0.45f, 0.45f};
    float saturation = 1.0f;
    float glossiness = 0.4f;
    float chappedAmount = 0.0f;             // 0 = smooth, 1 = very chapped
    
    static LipTextureParams natural() {
        return LipTextureParams();
    }
    
    static LipTextureParams pale() {
        LipTextureParams p;
        p.color = Vec3(0.7f, 0.55f, 0.55f);
        p.saturation = 0.7f;
        return p;
    }
    
    static LipTextureParams dark() {
        LipTextureParams p;
        p.color = Vec3(0.5f, 0.25f, 0.25f);
        return p;
    }
};

// ============================================================================
// Procedural Texture Generator
// ============================================================================

class ProceduralTextureGenerator {
public:
    // Generate skin diffuse texture
    static TextureData generateSkinDiffuse(const SkinTextureParams& params, 
                                           int width = 1024, int height = 1024) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4);
        
        // Noise for variation
        auto noise = [](float x, float y, float scale) -> float {
            float fx = x * scale;
            float fy = y * scale;
            int ix = static_cast<int>(fx) & 255;
            int iy = static_cast<int>(fy) & 255;
            // Simple hash-based noise
            int h = (ix * 374761393 + iy * 668265263) ^ (ix * 1274126177);
            return static_cast<float>(h & 0xFFFF) / 65535.0f;
        };
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float u = static_cast<float>(x) / width;
                float v = static_cast<float>(y) / height;
                
                // Base color with variation
                float variation = (noise(u, v, 20.0f) - 0.5f) * params.skinVariation;
                
                Vec3 color = params.baseColor;
                color.x = std::clamp(color.x + variation, 0.0f, 1.0f);
                color.y = std::clamp(color.y + variation * 0.8f, 0.0f, 1.0f);
                color.z = std::clamp(color.z + variation * 0.6f, 0.0f, 1.0f);
                
                // Apply saturation
                float gray = color.x * 0.299f + color.y * 0.587f + color.z * 0.114f;
                color.x = gray + (color.x - gray) * params.saturation;
                color.y = gray + (color.y - gray) * params.saturation;
                color.z = gray + (color.z - gray) * params.saturation;
                
                // Apply brightness
                color = color * params.brightness;
                
                // Freckles
                if (params.freckleIntensity > 0.0f) {
                    float freckle = noise(u, v, 100.0f);
                    if (freckle > (1.0f - params.freckleIntensity * 0.1f)) {
                        float blend = (freckle - (1.0f - params.freckleIntensity * 0.1f)) * 10.0f;
                        blend = std::clamp(blend, 0.0f, params.freckleIntensity);
                        color = Vec3::lerp(color, params.freckleColor, blend);
                    }
                }
                
                // Store pixel
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 1] = static_cast<uint8_t>(std::clamp(color.y * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 2] = static_cast<uint8_t>(std::clamp(color.z * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate skin normal map
    static TextureData generateSkinNormal(const SkinTextureParams& params,
                                          int width = 1024, int height = 1024) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4);
        
        // Generate height map first for pores
        std::vector<float> heightMap(width * height);
        
        auto noise = [](float x, float y, float scale, int seed) -> float {
            float fx = x * scale;
            float fy = y * scale;
            int ix = static_cast<int>(fx);
            int iy = static_cast<int>(fy);
            int h = (ix * 374761393 + iy * 668265263 + seed) ^ (ix * 1274126177);
            return static_cast<float>(h & 0xFFFF) / 65535.0f;
        };
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float u = static_cast<float>(x) / width;
                float v = static_cast<float>(y) / height;
                
                float h = 0.0f;
                
                // Multi-octave noise for pores
                h += noise(u, v, 50.0f, 0) * 0.5f * params.poreIntensity;
                h += noise(u, v, 100.0f, 1) * 0.3f * params.poreIntensity;
                h += noise(u, v, 200.0f, 2) * 0.2f * params.poreIntensity;
                
                // Wrinkle patterns (simplified)
                if (params.wrinkleIntensity > 0.0f) {
                    float wrinkle = noise(u, v, 10.0f, 3);
                    h += wrinkle * 0.3f * params.wrinkleIntensity;
                }
                
                heightMap[y * width + x] = h;
            }
        }
        
        // Convert height map to normal map
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int x1 = (x + 1) % width;
                int y1 = (y + 1) % height;
                int x0 = (x - 1 + width) % width;
                int y0 = (y - 1 + height) % height;
                
                float dX = heightMap[y * width + x1] - heightMap[y * width + x0];
                float dY = heightMap[y1 * width + x] - heightMap[y0 * width + x];
                
                Vec3 normal(-dX * 2.0f, -dY * 2.0f, 1.0f);
                normal = normal.normalized();
                
                // Convert to 0-255 range (normal map encoding)
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>((normal.x * 0.5f + 0.5f) * 255.0f);
                tex.pixels[idx + 1] = static_cast<uint8_t>((normal.y * 0.5f + 0.5f) * 255.0f);
                tex.pixels[idx + 2] = static_cast<uint8_t>((normal.z * 0.5f + 0.5f) * 255.0f);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate skin roughness map
    static TextureData generateSkinRoughness(const SkinTextureParams& params,
                                             int width = 512, int height = 512) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4);
        
        auto noise = [](float x, float y, float scale) -> float {
            int ix = static_cast<int>(x * scale) & 255;
            int iy = static_cast<int>(y * scale) & 255;
            int h = (ix * 374761393 + iy * 668265263) ^ (ix * 1274126177);
            return static_cast<float>(h & 0xFFFF) / 65535.0f;
        };
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float u = static_cast<float>(x) / width;
                float v = static_cast<float>(y) / height;
                
                // Base roughness with variation
                float rough = params.roughness;
                rough += (noise(u, v, 30.0f) - 0.5f) * 0.1f;
                
                // Oilier areas (T-zone simulation would need UV mapping)
                rough = std::clamp(rough, 0.1f, 0.9f);
                
                uint8_t r = static_cast<uint8_t>(rough * 255.0f);
                
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = r;
                tex.pixels[idx + 1] = r;
                tex.pixels[idx + 2] = r;
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate eye iris texture
    static TextureData generateIrisTexture(const EyeTextureParams& params,
                                           int size = 512) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        float center = size / 2.0f;
        float irisRadius = size * params.irisSize * 0.5f;
        float pupilRadius = irisRadius * params.pupilSize;
        
        auto noise = [](float x, float y, float scale, int seed) -> float {
            int ix = static_cast<int>(x * scale);
            int iy = static_cast<int>(y * scale);
            int h = (ix * 374761393 + iy * 668265263 + seed) ^ (ix * 1274126177);
            return static_cast<float>(h & 0xFFFF) / 65535.0f;
        };
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float dx = x - center;
                float dy = y - center;
                float dist = std::sqrt(dx * dx + dy * dy);
                float angle = std::atan2(dy, dx);
                
                Vec3 color(0, 0, 0);
                float alpha = 0.0f;
                
                if (dist < pupilRadius) {
                    // Pupil - black
                    color = Vec3(0.02f, 0.02f, 0.02f);
                    alpha = 1.0f;
                }
                else if (dist < irisRadius) {
                    // Iris
                    float t = (dist - pupilRadius) / (irisRadius - pupilRadius);
                    
                    // Radial fibers
                    float fiber = std::sin(angle * 60.0f) * 0.5f + 0.5f;
                    fiber *= params.irisDetail;
                    
                    // Color gradient from inner to outer
                    color = Vec3::lerp(params.irisColor, params.irisRingColor, t * 0.7f);
                    
                    // Add fiber pattern
                    float fiberNoise = noise(angle * 10.0f, t, 5.0f, 42);
                    color = color * (0.8f + fiber * 0.2f + fiberNoise * 0.1f);
                    
                    // Limbal ring (darker outer edge)
                    if (t > 0.85f) {
                        float ring = (t - 0.85f) / 0.15f;
                        color = Vec3::lerp(color, params.irisRingColor * 0.5f, ring * 0.5f);
                    }
                    
                    alpha = 1.0f;
                }
                else {
                    // Outside iris - transparent
                    alpha = 0.0f;
                }
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 1] = static_cast<uint8_t>(std::clamp(color.y * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 2] = static_cast<uint8_t>(std::clamp(color.z * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 255.0f);
            }
        }
        
        return tex;
    }
    
    // Generate sclera (eye white) texture
    static TextureData generateScleraTexture(const EyeTextureParams& params,
                                             int size = 256) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        auto noise = [](float x, float y, float scale) -> float {
            int ix = static_cast<int>(x * scale) & 255;
            int iy = static_cast<int>(y * scale) & 255;
            int h = (ix * 374761393 + iy * 668265263) ^ (ix * 1274126177);
            return static_cast<float>(h & 0xFFFF) / 65535.0f;
        };
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = static_cast<float>(x) / size;
                float v = static_cast<float>(y) / size;
                
                Vec3 color = params.scleraColor;
                
                // Blood vessels
                if (params.scleraVeins > 0.0f) {
                    float vein = noise(u, v, 30.0f);
                    if (vein > 0.9f) {
                        float intensity = (vein - 0.9f) * 10.0f * params.scleraVeins;
                        color = Vec3::lerp(color, Vec3(0.9f, 0.7f, 0.7f), intensity);
                    }
                }
                
                // Slight variation
                float variation = (noise(u, v, 10.0f) - 0.5f) * 0.02f;
                color.x = std::clamp(color.x + variation, 0.0f, 1.0f);
                color.y = std::clamp(color.y + variation, 0.0f, 1.0f);
                color.z = std::clamp(color.z + variation, 0.0f, 1.0f);
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(color.x * 255.0f);
                tex.pixels[idx + 1] = static_cast<uint8_t>(color.y * 255.0f);
                tex.pixels[idx + 2] = static_cast<uint8_t>(color.z * 255.0f);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // Generate lip texture
    static TextureData generateLipTexture(const LipTextureParams& params,
                                          int width = 256, int height = 128) {
        TextureData tex;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.pixels.resize(width * height * 4);
        
        auto noise = [](float x, float y, float scale) -> float {
            int ix = static_cast<int>(x * scale) & 255;
            int iy = static_cast<int>(y * scale) & 255;
            int h = (ix * 374761393 + iy * 668265263) ^ (ix * 1274126177);
            return static_cast<float>(h & 0xFFFF) / 65535.0f;
        };
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float u = static_cast<float>(x) / width;
                float v = static_cast<float>(y) / height;
                
                Vec3 color = params.color;
                
                // Vertical lines (lip texture)
                float lines = std::sin(u * 100.0f) * 0.5f + 0.5f;
                color = color * (0.95f + lines * 0.05f);
                
                // Chapped texture
                if (params.chappedAmount > 0.0f) {
                    float chap = noise(u, v, 50.0f);
                    if (chap > (1.0f - params.chappedAmount * 0.3f)) {
                        color = color * 0.9f;
                    }
                }
                
                // Apply saturation
                float gray = color.x * 0.299f + color.y * 0.587f + color.z * 0.114f;
                color.x = gray + (color.x - gray) * params.saturation;
                color.y = gray + (color.y - gray) * params.saturation;
                color.z = gray + (color.z - gray) * params.saturation;
                
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 1] = static_cast<uint8_t>(std::clamp(color.y * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 2] = static_cast<uint8_t>(std::clamp(color.z * 255.0f, 0.0f, 255.0f));
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
};

// ============================================================================
// Character Texture Set
// ============================================================================

struct CharacterTextureSet {
    // Skin
    TextureData skinDiffuse;
    TextureData skinNormal;
    TextureData skinRoughness;
    
    // Eyes
    TextureData irisLeft;
    TextureData irisRight;
    TextureData sclera;
    
    // Face details
    TextureData lips;
    
    // Overlays (optional)
    TextureData freckles;
    TextureData wrinkles;
    TextureData makeup;
    
    // Parameters used to generate
    SkinTextureParams skinParams;
    EyeTextureParams eyeParams;
    LipTextureParams lipParams;
    
    bool isGenerated = false;
};

// ============================================================================
// Character Texture Manager
// ============================================================================

class CharacterTextureManager {
public:
    static CharacterTextureManager& getInstance() {
        static CharacterTextureManager instance;
        return instance;
    }
    
    // Generate complete texture set
    CharacterTextureSet generateTextureSet(const SkinTextureParams& skin,
                                           const EyeTextureParams& eyes,
                                           const LipTextureParams& lips,
                                           int resolution = 1024) {
        CharacterTextureSet set;
        set.skinParams = skin;
        set.eyeParams = eyes;
        set.lipParams = lips;
        
        // Generate skin textures
        set.skinDiffuse = ProceduralTextureGenerator::generateSkinDiffuse(skin, resolution, resolution);
        set.skinNormal = ProceduralTextureGenerator::generateSkinNormal(skin, resolution, resolution);
        set.skinRoughness = ProceduralTextureGenerator::generateSkinRoughness(skin, resolution / 2, resolution / 2);
        
        // Generate eye textures
        set.irisLeft = ProceduralTextureGenerator::generateIrisTexture(eyes, 512);
        set.irisRight = set.irisLeft;  // Same for both by default
        set.sclera = ProceduralTextureGenerator::generateScleraTexture(eyes, 256);
        
        // Generate lip texture
        set.lips = ProceduralTextureGenerator::generateLipTexture(lips, 256, 128);
        
        set.isGenerated = true;
        return set;
    }
    
    // Update specific texture
    void updateSkinTexture(CharacterTextureSet& set, const SkinTextureParams& params, int resolution = 1024) {
        set.skinParams = params;
        set.skinDiffuse = ProceduralTextureGenerator::generateSkinDiffuse(params, resolution, resolution);
        set.skinNormal = ProceduralTextureGenerator::generateSkinNormal(params, resolution, resolution);
        set.skinRoughness = ProceduralTextureGenerator::generateSkinRoughness(params, resolution / 2, resolution / 2);
    }
    
    void updateEyeTexture(CharacterTextureSet& set, const EyeTextureParams& params) {
        set.eyeParams = params;
        set.irisLeft = ProceduralTextureGenerator::generateIrisTexture(params, 512);
        set.irisRight = set.irisLeft;
        set.sclera = ProceduralTextureGenerator::generateScleraTexture(params, 256);
    }
    
    void updateLipTexture(CharacterTextureSet& set, const LipTextureParams& params) {
        set.lipParams = params;
        set.lips = ProceduralTextureGenerator::generateLipTexture(params, 256, 128);
    }
    
    // Asset library
    void addTextureAsset(const CharacterTextureAsset& asset) {
        textureAssets_[asset.id] = asset;
    }
    
    const CharacterTextureAsset* getTextureAsset(const std::string& id) const {
        auto it = textureAssets_.find(id);
        return it != textureAssets_.end() ? &it->second : nullptr;
    }
    
    std::vector<std::string> getAssetsByType(CharacterTextureType type) const {
        std::vector<std::string> result;
        for (const auto& [id, asset] : textureAssets_) {
            if (asset.type == type) {
                result.push_back(id);
            }
        }
        return result;
    }
    
    // Presets
    CharacterTextureSet createPreset(const std::string& presetName) {
        SkinTextureParams skin;
        EyeTextureParams eyes;
        LipTextureParams lips;
        
        if (presetName == "caucasian_male") {
            skin = SkinTextureParams::caucasian();
            skin.poreIntensity = 0.6f;
            eyes = EyeTextureParams::blue();
            lips = LipTextureParams::natural();
        }
        else if (presetName == "caucasian_female") {
            skin = SkinTextureParams::caucasian();
            skin.poreIntensity = 0.3f;
            eyes = EyeTextureParams::green();
            lips = LipTextureParams::natural();
            lips.glossiness = 0.5f;
        }
        else if (presetName == "asian_male") {
            skin = SkinTextureParams::asian();
            skin.poreIntensity = 0.5f;
            eyes = EyeTextureParams::brown();
            lips = LipTextureParams::natural();
        }
        else if (presetName == "asian_female") {
            skin = SkinTextureParams::asian();
            skin.poreIntensity = 0.25f;
            eyes = EyeTextureParams::brown();
            lips = LipTextureParams::natural();
            lips.glossiness = 0.5f;
        }
        else if (presetName == "african_male") {
            skin = SkinTextureParams::african();
            skin.poreIntensity = 0.5f;
            eyes = EyeTextureParams::brown();
            lips = LipTextureParams::dark();
        }
        else if (presetName == "african_female") {
            skin = SkinTextureParams::african();
            skin.poreIntensity = 0.3f;
            eyes = EyeTextureParams::brown();
            lips = LipTextureParams::dark();
            lips.glossiness = 0.5f;
        }
        else {
            // Default
            skin = SkinTextureParams::caucasian();
            eyes = EyeTextureParams::brown();
            lips = LipTextureParams::natural();
        }
        
        return generateTextureSet(skin, eyes, lips);
    }
    
    std::vector<std::string> getPresetNames() const {
        return {
            "caucasian_male",
            "caucasian_female", 
            "asian_male",
            "asian_female",
            "african_male",
            "african_female",
            "latino_male",
            "latino_female"
        };
    }

private:
    CharacterTextureManager() = default;
    std::unordered_map<std::string, CharacterTextureAsset> textureAssets_;
};

// Convenience function
inline CharacterTextureManager& getTextureManager() {
    return CharacterTextureManager::getInstance();
}

}  // namespace luma
