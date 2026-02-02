// LUMA Clothing Texture System
// Manages clothing textures, patterns, and materials
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "engine/character/clothing_system.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cmath>

namespace luma {

// ============================================================================
// Clothing Texture Types
// ============================================================================

enum class ClothingTextureType {
    Diffuse,        // Base color/albedo
    Normal,         // Normal map
    Roughness,      // Roughness/smoothness
    Metallic,       // Metallic map
    AO,             // Ambient occlusion
    Opacity,        // Alpha/transparency
    Emission        // Emissive map
};

// ============================================================================
// Fabric Types (for procedural patterns)
// ============================================================================

enum class FabricType {
    Cotton,
    Denim,
    Silk,
    Leather,
    Wool,
    Polyester,
    Velvet,
    Linen,
    Satin,
    Canvas
};

// ============================================================================
// Clothing Texture Asset
// ============================================================================

struct ClothingTextureAsset {
    std::string id;
    ClothingTextureType type;
    
    // Texture data
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 4;  // RGBA
    
    // File reference (for external textures)
    std::string filePath;
    bool isLoaded = false;
    
    // Generation parameters (for procedural textures)
    bool isProcedural = false;
    FabricType fabricType = FabricType::Cotton;
    Vec3 baseColor{1, 1, 1};
    float scale = 1.0f;
};

// ============================================================================
// Clothing Material Set
// ============================================================================

struct ClothingMaterialSet {
    std::string id;
    std::string name;
    
    // Textures by type
    std::unordered_map<ClothingTextureType, ClothingTextureAsset> textures;
    
    // PBR parameters (fallback when no texture)
    Vec3 baseColor{1, 1, 1};
    float roughness = 0.5f;
    float metallic = 0.0f;
    float opacity = 1.0f;
    
    // Fabric properties
    FabricType fabricType = FabricType::Cotton;
    bool hasPattern = false;
    
    // Apply to mesh
    void applyToMesh(Mesh& mesh) const {
        mesh.baseColor[0] = baseColor.x;
        mesh.baseColor[1] = baseColor.y;
        mesh.baseColor[2] = baseColor.z;
        mesh.roughness = roughness;
        mesh.metallic = metallic;
        
        // Apply diffuse texture if available
        auto it = textures.find(ClothingTextureType::Diffuse);
        if (it != textures.end() && it->second.isLoaded) {
            mesh.diffuseTexture.pixels = it->second.pixels;
            mesh.diffuseTexture.width = it->second.width;
            mesh.diffuseTexture.height = it->second.height;
            mesh.diffuseTexture.channels = it->second.channels;
            mesh.hasDiffuseTexture = true;
        }
        
        // Apply normal map
        it = textures.find(ClothingTextureType::Normal);
        if (it != textures.end() && it->second.isLoaded) {
            mesh.normalTexture.pixels = it->second.pixels;
            mesh.normalTexture.width = it->second.width;
            mesh.normalTexture.height = it->second.height;
            mesh.normalTexture.channels = it->second.channels;
            mesh.hasNormalTexture = true;
        }
        
        // Apply roughness/specular map
        it = textures.find(ClothingTextureType::Roughness);
        if (it != textures.end() && it->second.isLoaded) {
            mesh.specularTexture.pixels = it->second.pixels;
            mesh.specularTexture.width = it->second.width;
            mesh.specularTexture.height = it->second.height;
            mesh.specularTexture.channels = it->second.channels;
            mesh.hasSpecularTexture = true;
        }
    }
};

// ============================================================================
// Procedural Fabric Generator
// ============================================================================

class ProceduralFabricGenerator {
public:
    // Generate fabric texture based on type
    static ClothingTextureAsset generateDiffuse(FabricType type, const Vec3& color, 
                                                  int width = 512, int height = 512) {
        ClothingTextureAsset tex;
        tex.type = ClothingTextureType::Diffuse;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.isProcedural = true;
        tex.fabricType = type;
        tex.baseColor = color;
        tex.pixels.resize(width * height * 4);
        
        switch (type) {
            case FabricType::Cotton:
                generateCottonTexture(tex, color);
                break;
            case FabricType::Denim:
                generateDenimTexture(tex, color);
                break;
            case FabricType::Silk:
                generateSilkTexture(tex, color);
                break;
            case FabricType::Leather:
                generateLeatherTexture(tex, color);
                break;
            case FabricType::Wool:
                generateWoolTexture(tex, color);
                break;
            case FabricType::Velvet:
                generateVelvetTexture(tex, color);
                break;
            default:
                generateCottonTexture(tex, color);
                break;
        }
        
        tex.isLoaded = true;
        return tex;
    }
    
    // Generate normal map for fabric
    static ClothingTextureAsset generateNormal(FabricType type, int width = 512, int height = 512) {
        ClothingTextureAsset tex;
        tex.type = ClothingTextureType::Normal;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.isProcedural = true;
        tex.fabricType = type;
        tex.pixels.resize(width * height * 4);
        
        switch (type) {
            case FabricType::Denim:
                generateDenimNormal(tex);
                break;
            case FabricType::Leather:
                generateLeatherNormal(tex);
                break;
            case FabricType::Wool:
                generateWoolNormal(tex);
                break;
            default:
                generateGenericFabricNormal(tex);
                break;
        }
        
        tex.isLoaded = true;
        return tex;
    }
    
    // Generate roughness map
    static ClothingTextureAsset generateRoughness(FabricType type, int width = 512, int height = 512) {
        ClothingTextureAsset tex;
        tex.type = ClothingTextureType::Roughness;
        tex.width = width;
        tex.height = height;
        tex.channels = 4;
        tex.isProcedural = true;
        tex.fabricType = type;
        tex.pixels.resize(width * height * 4);
        
        float baseRoughness = getFabricRoughness(type);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float noise = fbmNoise(x * 0.02f, y * 0.02f, 3) * 0.1f;
                float r = std::clamp(baseRoughness + noise, 0.0f, 1.0f);
                
                uint8_t val = static_cast<uint8_t>(r * 255);
                int idx = (y * width + x) * 4;
                tex.pixels[idx + 0] = val;
                tex.pixels[idx + 1] = val;
                tex.pixels[idx + 2] = val;
                tex.pixels[idx + 3] = 255;
            }
        }
        
        tex.isLoaded = true;
        return tex;
    }
    
    // Get fabric roughness
    static float getFabricRoughness(FabricType type) {
        switch (type) {
            case FabricType::Silk:    return 0.2f;
            case FabricType::Satin:   return 0.25f;
            case FabricType::Leather: return 0.4f;
            case FabricType::Cotton:  return 0.7f;
            case FabricType::Denim:   return 0.75f;
            case FabricType::Wool:    return 0.8f;
            case FabricType::Velvet:  return 0.85f;
            case FabricType::Canvas:  return 0.9f;
            default:                  return 0.5f;
        }
    }
    
private:
    // Simple noise function
    static float hash(float x, float y) {
        float h = x * 12.9898f + y * 78.233f;
        return std::fmod(std::sin(h) * 43758.5453f, 1.0f);
    }
    
    static float smoothNoise(float x, float y) {
        int ix = static_cast<int>(std::floor(x));
        int iy = static_cast<int>(std::floor(y));
        float fx = x - ix;
        float fy = y - iy;
        
        float a = hash(ix, iy);
        float b = hash(ix + 1, iy);
        float c = hash(ix, iy + 1);
        float d = hash(ix + 1, iy + 1);
        
        fx = fx * fx * (3.0f - 2.0f * fx);
        fy = fy * fy * (3.0f - 2.0f * fy);
        
        return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
    }
    
    static float fbmNoise(float x, float y, int octaves) {
        float value = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;
        
        for (int i = 0; i < octaves; i++) {
            value += smoothNoise(x * frequency, y * frequency) * amplitude;
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }
        
        return value;
    }
    
    static void generateCottonTexture(ClothingTextureAsset& tex, const Vec3& color) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Cotton weave pattern
                float weaveX = std::sin(x * 0.5f) * 0.5f + 0.5f;
                float weaveY = std::sin(y * 0.5f) * 0.5f + 0.5f;
                float weave = (weaveX + weaveY) * 0.5f;
                
                // Add noise for fiber texture
                float noise = fbmNoise(x * 0.1f, y * 0.1f, 4) * 0.15f;
                
                float brightness = 0.85f + weave * 0.1f + noise;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte(color.x * brightness * 255);
                tex.pixels[idx + 1] = clampByte(color.y * brightness * 255);
                tex.pixels[idx + 2] = clampByte(color.z * brightness * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateDenimTexture(ClothingTextureAsset& tex, const Vec3& color) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Denim diagonal twill weave
                int pattern = ((x + y) / 2) % 4;
                float weave = (pattern < 2) ? 0.9f : 1.0f;
                
                // Add thread variation
                float threadNoise = fbmNoise(x * 0.3f, y * 0.3f, 3) * 0.1f;
                
                // Subtle white threads showing through
                float whiteThread = (pattern == 0 && (x % 8 < 2)) ? 0.1f : 0.0f;
                
                float brightness = weave + threadNoise;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte((color.x * brightness + whiteThread) * 255);
                tex.pixels[idx + 1] = clampByte((color.y * brightness + whiteThread) * 255);
                tex.pixels[idx + 2] = clampByte((color.z * brightness + whiteThread) * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateSilkTexture(ClothingTextureAsset& tex, const Vec3& color) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Silk sheen effect
                float sheen = std::sin((x + y) * 0.02f) * 0.15f + 0.85f;
                float noise = fbmNoise(x * 0.05f, y * 0.05f, 2) * 0.05f;
                
                float brightness = sheen + noise;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte(color.x * brightness * 255);
                tex.pixels[idx + 1] = clampByte(color.y * brightness * 255);
                tex.pixels[idx + 2] = clampByte(color.z * brightness * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateLeatherTexture(ClothingTextureAsset& tex, const Vec3& color) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Leather grain pattern
                float grain = fbmNoise(x * 0.08f, y * 0.08f, 5);
                
                // Pore-like details
                float pores = fbmNoise(x * 0.3f, y * 0.3f, 2) * 0.5f;
                pores = std::pow(pores, 3.0f) * 0.3f;
                
                float brightness = 0.8f + grain * 0.2f - pores;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte(color.x * brightness * 255);
                tex.pixels[idx + 1] = clampByte(color.y * brightness * 255);
                tex.pixels[idx + 2] = clampByte(color.z * brightness * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateWoolTexture(ClothingTextureAsset& tex, const Vec3& color) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Wool fiber pattern
                float fiber = fbmNoise(x * 0.15f, y * 0.15f, 4);
                
                // Knit pattern
                float knitX = std::sin(x * 0.3f) * 0.5f + 0.5f;
                float knitY = std::sin(y * 0.2f) * 0.5f + 0.5f;
                float knit = (knitX * knitY) * 0.1f;
                
                float brightness = 0.75f + fiber * 0.2f + knit;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte(color.x * brightness * 255);
                tex.pixels[idx + 1] = clampByte(color.y * brightness * 255);
                tex.pixels[idx + 2] = clampByte(color.z * brightness * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateVelvetTexture(ClothingTextureAsset& tex, const Vec3& color) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Velvet pile direction effect
                float pile = fbmNoise(x * 0.04f, y * 0.04f, 3);
                float direction = std::sin(pile * 6.28f) * 0.2f;
                
                float brightness = 0.7f + direction + pile * 0.1f;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte(color.x * brightness * 255);
                tex.pixels[idx + 1] = clampByte(color.y * brightness * 255);
                tex.pixels[idx + 2] = clampByte(color.z * brightness * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateDenimNormal(ClothingTextureAsset& tex) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Twill weave normal
                int pattern = ((x + y) / 2) % 4;
                float nx = (pattern < 2) ? 0.1f : -0.1f;
                float ny = (pattern % 2 == 0) ? 0.1f : -0.1f;
                
                float noise = fbmNoise(x * 0.2f, y * 0.2f, 2) * 0.1f;
                nx += noise;
                ny += noise;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte((nx + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 1] = clampByte((ny + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 2] = 255;  // Z always up
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateLeatherNormal(ClothingTextureAsset& tex) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                // Sample heights for normal calculation
                float h = fbmNoise(x * 0.08f, y * 0.08f, 5);
                float hx = fbmNoise((x + 1) * 0.08f, y * 0.08f, 5);
                float hy = fbmNoise(x * 0.08f, (y + 1) * 0.08f, 5);
                
                float nx = (h - hx) * 2.0f;
                float ny = (h - hy) * 2.0f;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte((nx + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 1] = clampByte((ny + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 2] = 255;
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateWoolNormal(ClothingTextureAsset& tex) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                float fiber = fbmNoise(x * 0.15f, y * 0.15f, 4);
                float fiberX = fbmNoise((x + 1) * 0.15f, y * 0.15f, 4);
                float fiberY = fbmNoise(x * 0.15f, (y + 1) * 0.15f, 4);
                
                float nx = (fiber - fiberX) * 1.5f;
                float ny = (fiber - fiberY) * 1.5f;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte((nx + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 1] = clampByte((ny + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 2] = 255;
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static void generateGenericFabricNormal(ClothingTextureAsset& tex) {
        for (int y = 0; y < tex.height; y++) {
            for (int x = 0; x < tex.width; x++) {
                float noise = fbmNoise(x * 0.1f, y * 0.1f, 3);
                float noiseX = fbmNoise((x + 1) * 0.1f, y * 0.1f, 3);
                float noiseY = fbmNoise(x * 0.1f, (y + 1) * 0.1f, 3);
                
                float nx = (noise - noiseX) * 0.5f;
                float ny = (noise - noiseY) * 0.5f;
                
                int idx = (y * tex.width + x) * 4;
                tex.pixels[idx + 0] = clampByte((nx + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 1] = clampByte((ny + 1.0f) * 0.5f * 255);
                tex.pixels[idx + 2] = 255;
                tex.pixels[idx + 3] = 255;
            }
        }
    }
    
    static uint8_t clampByte(float v) {
        return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
    }
};

// ============================================================================
// Clothing Texture Manager
// ============================================================================

class ClothingTextureManager {
public:
    static ClothingTextureManager& getInstance() {
        static ClothingTextureManager instance;
        return instance;
    }
    
    // Generate material set for fabric type and color
    ClothingMaterialSet generateMaterialSet(FabricType type, const Vec3& color, 
                                            const std::string& id = "",
                                            int resolution = 512) {
        ClothingMaterialSet set;
        set.id = id.empty() ? generateId(type, color) : id;
        set.name = getFabricName(type);
        set.fabricType = type;
        set.baseColor = color;
        set.roughness = ProceduralFabricGenerator::getFabricRoughness(type);
        set.metallic = 0.0f;
        
        // Generate textures
        set.textures[ClothingTextureType::Diffuse] = 
            ProceduralFabricGenerator::generateDiffuse(type, color, resolution, resolution);
        set.textures[ClothingTextureType::Normal] = 
            ProceduralFabricGenerator::generateNormal(type, resolution, resolution);
        set.textures[ClothingTextureType::Roughness] = 
            ProceduralFabricGenerator::generateRoughness(type, resolution, resolution);
        
        // Cache the material set
        materialSets_[set.id] = set;
        
        return set;
    }
    
    // Get cached material set
    const ClothingMaterialSet* getMaterialSet(const std::string& id) const {
        auto it = materialSets_.find(id);
        return it != materialSets_.end() ? &it->second : nullptr;
    }
    
    // Load texture from file
    bool loadTexture(const std::string& path, ClothingTextureAsset& outTexture) {
        // This would use stb_image or similar
        // For now, return false to indicate file loading not implemented
        outTexture.filePath = path;
        return false;
    }
    
    // Get fabric name
    static const char* getFabricName(FabricType type) {
        switch (type) {
            case FabricType::Cotton:    return "Cotton";
            case FabricType::Denim:     return "Denim";
            case FabricType::Silk:      return "Silk";
            case FabricType::Leather:   return "Leather";
            case FabricType::Wool:      return "Wool";
            case FabricType::Polyester: return "Polyester";
            case FabricType::Velvet:    return "Velvet";
            case FabricType::Linen:     return "Linen";
            case FabricType::Satin:     return "Satin";
            case FabricType::Canvas:    return "Canvas";
            default:                    return "Unknown";
        }
    }
    
    // Get all fabric types
    static std::vector<FabricType> getAllFabricTypes() {
        return {
            FabricType::Cotton, FabricType::Denim, FabricType::Silk,
            FabricType::Leather, FabricType::Wool, FabricType::Polyester,
            FabricType::Velvet, FabricType::Linen, FabricType::Satin,
            FabricType::Canvas
        };
    }
    
private:
    ClothingTextureManager() = default;
    
    std::unordered_map<std::string, ClothingMaterialSet> materialSets_;
    
    std::string generateId(FabricType type, const Vec3& color) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s_%02x%02x%02x",
                 getFabricName(type),
                 int(color.x * 255), int(color.y * 255), int(color.z * 255));
        return buf;
    }
};

// Convenience function
inline ClothingTextureManager& getClothingTextureManager() {
    return ClothingTextureManager::getInstance();
}

}  // namespace luma
