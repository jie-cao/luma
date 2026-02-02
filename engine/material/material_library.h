// Material Library - Extended PBR material presets and textures
// Rich material collection with procedural generation
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace luma {

// ============================================================================
// Material Category
// ============================================================================

enum class MaterialCategory {
    Metal,          // 金属
    Wood,           // 木材
    Stone,          // 石材
    Fabric,         // 布料
    Plastic,        // 塑料
    Glass,          // 玻璃
    Organic,        // 有机物
    Gemstone,       // 宝石
    Ceramic,        // 陶瓷
    Liquid,         // 液体
    Emissive,       // 发光
    Stylized,       // 风格化
    Custom          // 自定义
};

inline std::string materialCategoryToString(MaterialCategory cat) {
    switch (cat) {
        case MaterialCategory::Metal: return "Metal";
        case MaterialCategory::Wood: return "Wood";
        case MaterialCategory::Stone: return "Stone";
        case MaterialCategory::Fabric: return "Fabric";
        case MaterialCategory::Plastic: return "Plastic";
        case MaterialCategory::Glass: return "Glass";
        case MaterialCategory::Organic: return "Organic";
        case MaterialCategory::Gemstone: return "Gemstone";
        case MaterialCategory::Ceramic: return "Ceramic";
        case MaterialCategory::Liquid: return "Liquid";
        case MaterialCategory::Emissive: return "Emissive";
        case MaterialCategory::Stylized: return "Stylized";
        case MaterialCategory::Custom: return "Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Extended PBR Material
// ============================================================================

struct PBRMaterialPreset {
    std::string id;
    std::string name;
    std::string nameCN;
    std::string description;
    MaterialCategory category = MaterialCategory::Custom;
    
    // Base PBR
    Vec3 baseColor{1, 1, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    
    // Additional properties
    Vec3 emissiveColor{0, 0, 0};
    float emissiveIntensity = 0.0f;
    
    float opacity = 1.0f;
    float ior = 1.5f;              // Index of refraction
    
    float anisotropy = 0.0f;       // Anisotropic reflection
    float anisotropyRotation = 0.0f;
    
    float clearcoat = 0.0f;        // Clear coat layer
    float clearcoatRoughness = 0.0f;
    
    float sheen = 0.0f;            // Velvet/fabric sheen
    Vec3 sheenColor{1, 1, 1};
    
    float subsurface = 0.0f;       // SSS amount
    Vec3 subsurfaceColor{1, 0.5f, 0.3f};
    float subsurfaceRadius = 1.0f;
    
    float transmission = 0.0f;     // Glass/liquid transmission
    float transmissionRoughness = 0.0f;
    
    // Texture flags
    bool hasAlbedoTexture = false;
    bool hasNormalTexture = false;
    bool hasRoughnessTexture = false;
    bool hasMetallicTexture = false;
    bool hasAOTexture = false;
    bool hasEmissiveTexture = false;
    
    // Normal map strength
    float normalStrength = 1.0f;
    
    // Tags for filtering
    std::vector<std::string> tags;
};

// ============================================================================
// Procedural Texture Generator
// ============================================================================

class ProceduralTextureGenerator {
public:
    // === Noise Functions ===
    
    static float noise2D(float x, float y) {
        int xi = static_cast<int>(std::floor(x)) & 255;
        int yi = static_cast<int>(std::floor(y)) & 255;
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        
        float u = fade(xf);
        float v = fade(yf);
        
        int aa = perm[perm[xi] + yi];
        int ab = perm[perm[xi] + yi + 1];
        int ba = perm[perm[xi + 1] + yi];
        int bb = perm[perm[xi + 1] + yi + 1];
        
        float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
        float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);
        
        return (lerp(x1, x2, v) + 1.0f) / 2.0f;
    }
    
    static float fractalNoise(float x, float y, int octaves = 4, float persistence = 0.5f) {
        float total = 0;
        float frequency = 1;
        float amplitude = 1;
        float maxValue = 0;
        
        for (int i = 0; i < octaves; i++) {
            total += noise2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2;
        }
        
        return total / maxValue;
    }
    
    // === Metal Textures ===
    
    static TextureData generateBrushedMetal(int size = 512, const Vec3& baseColor = Vec3(0.8f, 0.8f, 0.85f)) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // Horizontal brush strokes
                float streaks = noise2D(x * 0.1f, y * 0.01f);
                float detail = fractalNoise(x * 0.5f, y * 0.5f, 3) * 0.1f;
                
                float value = streaks * 0.8f + detail + 0.2f;
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(std::clamp(baseColor.x * value, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(std::clamp(baseColor.y * value, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(std::clamp(baseColor.z * value, 0.0f, 1.0f) * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    static TextureData generateRustMetal(int size = 512) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float n1 = fractalNoise(x * 0.02f, y * 0.02f, 4);
                float n2 = fractalNoise(x * 0.05f + 100, y * 0.05f + 100, 3);
                
                // Metal base
                Vec3 metalColor(0.5f, 0.5f, 0.55f);
                // Rust color
                Vec3 rustColor(0.6f, 0.25f, 0.1f);
                
                float rustAmount = std::clamp(n1 * n2 * 2.0f - 0.3f, 0.0f, 1.0f);
                Vec3 color = metalColor.lerp(rustColor, rustAmount);
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(color.x * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(color.y * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(color.z * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // === Wood Textures ===
    
    static TextureData generateWoodGrain(int size = 512, const Vec3& lightColor = Vec3(0.8f, 0.6f, 0.4f),
                                         const Vec3& darkColor = Vec3(0.4f, 0.25f, 0.1f)) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size;
                float v = (float)y / size;
                
                // Wood rings
                float distFromCenter = std::sqrt((u - 0.5f) * (u - 0.5f) * 0.1f + v * v);
                float rings = std::sin(distFromCenter * 50.0f + fractalNoise(u * 10, v * 2, 3) * 5.0f);
                rings = (rings + 1.0f) / 2.0f;
                
                // Add variation
                float variation = fractalNoise(u * 5, v * 20, 4) * 0.3f;
                rings = std::clamp(rings + variation, 0.0f, 1.0f);
                
                Vec3 color = darkColor.lerp(lightColor, rings);
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(color.x * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(color.y * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(color.z * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // === Stone Textures ===
    
    static TextureData generateMarble(int size = 512, const Vec3& baseColor = Vec3(0.95f, 0.95f, 0.95f),
                                      const Vec3& veinColor = Vec3(0.3f, 0.3f, 0.35f)) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size;
                float v = (float)y / size;
                
                // Marble veins using turbulence
                float vein = std::sin(u * 10.0f + fractalNoise(u * 5, v * 5, 5) * 10.0f);
                vein = std::pow(std::abs(vein), 0.5f);
                
                Vec3 color = baseColor.lerp(veinColor, vein * 0.5f);
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(color.x * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(color.y * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(color.z * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    static TextureData generateGranite(int size = 512) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // Mix of different colored spots
                float n1 = fractalNoise(x * 0.1f, y * 0.1f, 4);
                float n2 = fractalNoise(x * 0.2f + 50, y * 0.2f + 50, 3);
                float n3 = fractalNoise(x * 0.3f + 100, y * 0.3f + 100, 2);
                
                Vec3 gray(0.5f, 0.5f, 0.5f);
                Vec3 dark(0.2f, 0.2f, 0.22f);
                Vec3 light(0.8f, 0.78f, 0.75f);
                
                Vec3 color = gray;
                color = color.lerp(dark, std::clamp(n1 * 2 - 0.5f, 0.0f, 1.0f) * 0.5f);
                color = color.lerp(light, std::clamp(n2 * 2 - 0.5f, 0.0f, 1.0f) * 0.3f);
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(color.x * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(color.y * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(color.z * 255);
                tex.pixels[idx + 3] = 255;
            }
        }
        
        return tex;
    }
    
    // === Normal Map Generation ===
    
    static TextureData generateNormalFromHeight(const TextureData& heightMap, float strength = 1.0f) {
        TextureData normal;
        normal.width = heightMap.width;
        normal.height = heightMap.height;
        normal.channels = 4;
        normal.pixels.resize(normal.width * normal.height * 4);
        
        for (int y = 0; y < normal.height; y++) {
            for (int x = 0; x < normal.width; x++) {
                // Sample heights
                float h = getHeight(heightMap, x, y);
                float hL = getHeight(heightMap, x - 1, y);
                float hR = getHeight(heightMap, x + 1, y);
                float hU = getHeight(heightMap, x, y - 1);
                float hD = getHeight(heightMap, x, y + 1);
                
                // Calculate normal
                Vec3 n;
                n.x = (hL - hR) * strength;
                n.y = (hU - hD) * strength;
                n.z = 1.0f;
                n = n.normalized();
                
                // Pack to 0-255
                int idx = (y * normal.width + x) * 4;
                normal.pixels[idx + 0] = static_cast<uint8_t>((n.x * 0.5f + 0.5f) * 255);
                normal.pixels[idx + 1] = static_cast<uint8_t>((n.y * 0.5f + 0.5f) * 255);
                normal.pixels[idx + 2] = static_cast<uint8_t>((n.z * 0.5f + 0.5f) * 255);
                normal.pixels[idx + 3] = 255;
            }
        }
        
        return normal;
    }
    
    // === Roughness Maps ===
    
    static TextureData generateRoughnessMap(int size, float baseRoughness = 0.5f, float variation = 0.2f) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 1;
        tex.pixels.resize(size * size);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float n = fractalNoise(x * 0.05f, y * 0.05f, 3);
                float value = baseRoughness + (n - 0.5f) * variation * 2.0f;
                value = std::clamp(value, 0.0f, 1.0f);
                
                tex.pixels[y * size + x] = static_cast<uint8_t>(value * 255);
            }
        }
        
        return tex;
    }
    
private:
    static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static float lerp(float a, float b, float t) { return a + t * (b - a); }
    
    static float grad(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
    
    static float getHeight(const TextureData& tex, int x, int y) {
        x = std::clamp(x, 0, tex.width - 1);
        y = std::clamp(y, 0, tex.height - 1);
        int idx = (y * tex.width + x) * tex.channels;
        return tex.pixels[idx] / 255.0f;
    }
    
    // Permutation table
    static constexpr int perm[512] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
        // Repeat
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
};

// ============================================================================
// Material Library
// ============================================================================

class MaterialLibrary {
public:
    static MaterialLibrary& getInstance() {
        static MaterialLibrary instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // === Metals ===
        addPreset(createGold());
        addPreset(createSilver());
        addPreset(createCopper());
        addPreset(createBronze());
        addPreset(createIron());
        addPreset(createSteel());
        addPreset(createBrushedSteel());
        addPreset(createChrome());
        addPreset(createRustyMetal());
        addPreset(createAluminum());
        
        // === Woods ===
        addPreset(createOak());
        addPreset(createWalnut());
        addPreset(createPine());
        addPreset(createCherry());
        addPreset(createEbony());
        addPreset(createBamboo());
        
        // === Stones ===
        addPreset(createMarble());
        addPreset(createGranite());
        addPreset(createSlate());
        addPreset(createSandstone());
        addPreset(createConcrete());
        addPreset(createBrick());
        
        // === Fabrics ===
        addPreset(createCotton());
        addPreset(createSilk());
        addPreset(createVelvet());
        addPreset(createLeather());
        addPreset(createDenim());
        addPreset(createWool());
        
        // === Plastics ===
        addPreset(createGlossyPlastic());
        addPreset(createMattePlastic());
        addPreset(createRubber());
        addPreset(createSilicone());
        
        // === Glass ===
        addPreset(createClearGlass());
        addPreset(createFrostedGlass());
        addPreset(createColoredGlass());
        
        // === Gemstones ===
        addPreset(createDiamond());
        addPreset(createRuby());
        addPreset(createEmerald());
        addPreset(createSapphire());
        addPreset(createAmethyst());
        addPreset(createJade());
        
        // === Organic ===
        addPreset(createSkin());
        addPreset(createHair());
        addPreset(createEye());
        addPreset(createNail());
        
        // === Ceramic ===
        addPreset(createPorcelain());
        addPreset(createTerracotta());
        addPreset(createGlazedCeramic());
        
        // === Emissive ===
        addPreset(createNeonRed());
        addPreset(createNeonBlue());
        addPreset(createNeonGreen());
        addPreset(createLava());
        addPreset(createHologram());
        
        // === Stylized ===
        addPreset(createToon());
        addPreset(createWatercolor());
        addPreset(createCelShaded());
        
        initialized_ = true;
    }
    
    const PBRMaterialPreset* getPreset(const std::string& id) const {
        auto it = presets_.find(id);
        return (it != presets_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getPresetIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : presets_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    std::vector<const PBRMaterialPreset*> getPresetsByCategory(MaterialCategory category) const {
        std::vector<const PBRMaterialPreset*> result;
        for (const auto& [id, preset] : presets_) {
            if (preset.category == category) {
                result.push_back(&preset);
            }
        }
        return result;
    }
    
    std::vector<MaterialCategory> getCategories() const {
        return {
            MaterialCategory::Metal,
            MaterialCategory::Wood,
            MaterialCategory::Stone,
            MaterialCategory::Fabric,
            MaterialCategory::Plastic,
            MaterialCategory::Glass,
            MaterialCategory::Gemstone,
            MaterialCategory::Organic,
            MaterialCategory::Ceramic,
            MaterialCategory::Emissive,
            MaterialCategory::Stylized
        };
    }
    
    void addPreset(const PBRMaterialPreset& preset) {
        presets_[preset.id] = preset;
    }
    
private:
    MaterialLibrary() { initialize(); }
    
    // === Metal Presets ===
    
    PBRMaterialPreset createGold() {
        PBRMaterialPreset m;
        m.id = "gold";
        m.name = "Gold";
        m.nameCN = "黄金";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(1.0f, 0.766f, 0.336f);
        m.metallic = 1.0f;
        m.roughness = 0.1f;
        m.tags = {"metal", "precious", "shiny"};
        return m;
    }
    
    PBRMaterialPreset createSilver() {
        PBRMaterialPreset m;
        m.id = "silver";
        m.name = "Silver";
        m.nameCN = "白银";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.972f, 0.960f, 0.915f);
        m.metallic = 1.0f;
        m.roughness = 0.15f;
        m.tags = {"metal", "precious", "shiny"};
        return m;
    }
    
    PBRMaterialPreset createCopper() {
        PBRMaterialPreset m;
        m.id = "copper";
        m.name = "Copper";
        m.nameCN = "铜";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.955f, 0.637f, 0.538f);
        m.metallic = 1.0f;
        m.roughness = 0.25f;
        m.tags = {"metal", "warm"};
        return m;
    }
    
    PBRMaterialPreset createBronze() {
        PBRMaterialPreset m;
        m.id = "bronze";
        m.name = "Bronze";
        m.nameCN = "青铜";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.714f, 0.428f, 0.181f);
        m.metallic = 1.0f;
        m.roughness = 0.4f;
        m.tags = {"metal", "antique"};
        return m;
    }
    
    PBRMaterialPreset createIron() {
        PBRMaterialPreset m;
        m.id = "iron";
        m.name = "Iron";
        m.nameCN = "铁";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.56f, 0.57f, 0.58f);
        m.metallic = 1.0f;
        m.roughness = 0.5f;
        m.tags = {"metal", "industrial"};
        return m;
    }
    
    PBRMaterialPreset createSteel() {
        PBRMaterialPreset m;
        m.id = "steel";
        m.name = "Steel";
        m.nameCN = "钢";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.77f, 0.78f, 0.78f);
        m.metallic = 1.0f;
        m.roughness = 0.35f;
        m.tags = {"metal", "industrial"};
        return m;
    }
    
    PBRMaterialPreset createBrushedSteel() {
        PBRMaterialPreset m;
        m.id = "brushed_steel";
        m.name = "Brushed Steel";
        m.nameCN = "拉丝钢";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.75f, 0.76f, 0.77f);
        m.metallic = 1.0f;
        m.roughness = 0.4f;
        m.anisotropy = 0.8f;
        m.tags = {"metal", "industrial", "anisotropic"};
        return m;
    }
    
    PBRMaterialPreset createChrome() {
        PBRMaterialPreset m;
        m.id = "chrome";
        m.name = "Chrome";
        m.nameCN = "铬";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.55f, 0.55f, 0.55f);
        m.metallic = 1.0f;
        m.roughness = 0.05f;
        m.tags = {"metal", "mirror", "shiny"};
        return m;
    }
    
    PBRMaterialPreset createRustyMetal() {
        PBRMaterialPreset m;
        m.id = "rusty_metal";
        m.name = "Rusty Metal";
        m.nameCN = "锈铁";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.5f, 0.35f, 0.25f);
        m.metallic = 0.6f;
        m.roughness = 0.8f;
        m.tags = {"metal", "worn", "aged"};
        return m;
    }
    
    PBRMaterialPreset createAluminum() {
        PBRMaterialPreset m;
        m.id = "aluminum";
        m.name = "Aluminum";
        m.nameCN = "铝";
        m.category = MaterialCategory::Metal;
        m.baseColor = Vec3(0.91f, 0.92f, 0.92f);
        m.metallic = 1.0f;
        m.roughness = 0.3f;
        m.tags = {"metal", "light"};
        return m;
    }
    
    // === Wood Presets ===
    
    PBRMaterialPreset createOak() {
        PBRMaterialPreset m;
        m.id = "oak";
        m.name = "Oak";
        m.nameCN = "橡木";
        m.category = MaterialCategory::Wood;
        m.baseColor = Vec3(0.65f, 0.5f, 0.35f);
        m.metallic = 0.0f;
        m.roughness = 0.6f;
        m.tags = {"wood", "natural"};
        return m;
    }
    
    PBRMaterialPreset createWalnut() {
        PBRMaterialPreset m;
        m.id = "walnut";
        m.name = "Walnut";
        m.nameCN = "胡桃木";
        m.category = MaterialCategory::Wood;
        m.baseColor = Vec3(0.4f, 0.28f, 0.18f);
        m.metallic = 0.0f;
        m.roughness = 0.55f;
        m.tags = {"wood", "dark", "natural"};
        return m;
    }
    
    PBRMaterialPreset createPine() {
        PBRMaterialPreset m;
        m.id = "pine";
        m.name = "Pine";
        m.nameCN = "松木";
        m.category = MaterialCategory::Wood;
        m.baseColor = Vec3(0.85f, 0.7f, 0.5f);
        m.metallic = 0.0f;
        m.roughness = 0.65f;
        m.tags = {"wood", "light", "natural"};
        return m;
    }
    
    PBRMaterialPreset createCherry() {
        PBRMaterialPreset m;
        m.id = "cherry";
        m.name = "Cherry";
        m.nameCN = "樱桃木";
        m.category = MaterialCategory::Wood;
        m.baseColor = Vec3(0.6f, 0.35f, 0.25f);
        m.metallic = 0.0f;
        m.roughness = 0.5f;
        m.tags = {"wood", "warm", "natural"};
        return m;
    }
    
    PBRMaterialPreset createEbony() {
        PBRMaterialPreset m;
        m.id = "ebony";
        m.name = "Ebony";
        m.nameCN = "乌木";
        m.category = MaterialCategory::Wood;
        m.baseColor = Vec3(0.15f, 0.12f, 0.1f);
        m.metallic = 0.0f;
        m.roughness = 0.4f;
        m.tags = {"wood", "dark", "exotic"};
        return m;
    }
    
    PBRMaterialPreset createBamboo() {
        PBRMaterialPreset m;
        m.id = "bamboo";
        m.name = "Bamboo";
        m.nameCN = "竹";
        m.category = MaterialCategory::Wood;
        m.baseColor = Vec3(0.9f, 0.85f, 0.65f);
        m.metallic = 0.0f;
        m.roughness = 0.55f;
        m.tags = {"wood", "asian", "natural"};
        return m;
    }
    
    // === Stone Presets ===
    
    PBRMaterialPreset createMarble() {
        PBRMaterialPreset m;
        m.id = "marble";
        m.name = "Marble";
        m.nameCN = "大理石";
        m.category = MaterialCategory::Stone;
        m.baseColor = Vec3(0.95f, 0.95f, 0.95f);
        m.metallic = 0.0f;
        m.roughness = 0.2f;
        m.tags = {"stone", "polished", "elegant"};
        return m;
    }
    
    PBRMaterialPreset createGranite() {
        PBRMaterialPreset m;
        m.id = "granite";
        m.name = "Granite";
        m.nameCN = "花岗岩";
        m.category = MaterialCategory::Stone;
        m.baseColor = Vec3(0.5f, 0.5f, 0.52f);
        m.metallic = 0.0f;
        m.roughness = 0.5f;
        m.tags = {"stone", "speckled"};
        return m;
    }
    
    PBRMaterialPreset createSlate() {
        PBRMaterialPreset m;
        m.id = "slate";
        m.name = "Slate";
        m.nameCN = "板岩";
        m.category = MaterialCategory::Stone;
        m.baseColor = Vec3(0.35f, 0.38f, 0.4f);
        m.metallic = 0.0f;
        m.roughness = 0.7f;
        m.tags = {"stone", "layered"};
        return m;
    }
    
    PBRMaterialPreset createSandstone() {
        PBRMaterialPreset m;
        m.id = "sandstone";
        m.name = "Sandstone";
        m.nameCN = "砂岩";
        m.category = MaterialCategory::Stone;
        m.baseColor = Vec3(0.85f, 0.75f, 0.6f);
        m.metallic = 0.0f;
        m.roughness = 0.8f;
        m.tags = {"stone", "sandy"};
        return m;
    }
    
    PBRMaterialPreset createConcrete() {
        PBRMaterialPreset m;
        m.id = "concrete";
        m.name = "Concrete";
        m.nameCN = "混凝土";
        m.category = MaterialCategory::Stone;
        m.baseColor = Vec3(0.6f, 0.6f, 0.6f);
        m.metallic = 0.0f;
        m.roughness = 0.9f;
        m.tags = {"stone", "industrial", "urban"};
        return m;
    }
    
    PBRMaterialPreset createBrick() {
        PBRMaterialPreset m;
        m.id = "brick";
        m.name = "Brick";
        m.nameCN = "砖";
        m.category = MaterialCategory::Stone;
        m.baseColor = Vec3(0.7f, 0.35f, 0.25f);
        m.metallic = 0.0f;
        m.roughness = 0.85f;
        m.tags = {"stone", "building"};
        return m;
    }
    
    // === Fabric Presets ===
    
    PBRMaterialPreset createCotton() {
        PBRMaterialPreset m;
        m.id = "cotton";
        m.name = "Cotton";
        m.nameCN = "棉布";
        m.category = MaterialCategory::Fabric;
        m.baseColor = Vec3(0.95f, 0.95f, 0.95f);
        m.metallic = 0.0f;
        m.roughness = 0.8f;
        m.sheen = 0.3f;
        m.tags = {"fabric", "soft"};
        return m;
    }
    
    PBRMaterialPreset createSilk() {
        PBRMaterialPreset m;
        m.id = "silk";
        m.name = "Silk";
        m.nameCN = "丝绸";
        m.category = MaterialCategory::Fabric;
        m.baseColor = Vec3(0.95f, 0.92f, 0.88f);
        m.metallic = 0.0f;
        m.roughness = 0.3f;
        m.sheen = 0.8f;
        m.anisotropy = 0.5f;
        m.tags = {"fabric", "luxurious", "shiny"};
        return m;
    }
    
    PBRMaterialPreset createVelvet() {
        PBRMaterialPreset m;
        m.id = "velvet";
        m.name = "Velvet";
        m.nameCN = "天鹅绒";
        m.category = MaterialCategory::Fabric;
        m.baseColor = Vec3(0.5f, 0.1f, 0.15f);
        m.metallic = 0.0f;
        m.roughness = 0.9f;
        m.sheen = 1.0f;
        m.sheenColor = Vec3(1, 0.8f, 0.85f);
        m.tags = {"fabric", "luxurious", "soft"};
        return m;
    }
    
    PBRMaterialPreset createLeather() {
        PBRMaterialPreset m;
        m.id = "leather";
        m.name = "Leather";
        m.nameCN = "皮革";
        m.category = MaterialCategory::Fabric;
        m.baseColor = Vec3(0.35f, 0.22f, 0.12f);
        m.metallic = 0.0f;
        m.roughness = 0.6f;
        m.tags = {"fabric", "animal", "natural"};
        return m;
    }
    
    PBRMaterialPreset createDenim() {
        PBRMaterialPreset m;
        m.id = "denim";
        m.name = "Denim";
        m.nameCN = "牛仔布";
        m.category = MaterialCategory::Fabric;
        m.baseColor = Vec3(0.2f, 0.3f, 0.5f);
        m.metallic = 0.0f;
        m.roughness = 0.85f;
        m.tags = {"fabric", "casual"};
        return m;
    }
    
    PBRMaterialPreset createWool() {
        PBRMaterialPreset m;
        m.id = "wool";
        m.name = "Wool";
        m.nameCN = "羊毛";
        m.category = MaterialCategory::Fabric;
        m.baseColor = Vec3(0.9f, 0.88f, 0.82f);
        m.metallic = 0.0f;
        m.roughness = 0.95f;
        m.subsurface = 0.2f;
        m.tags = {"fabric", "warm", "fuzzy"};
        return m;
    }
    
    // === Plastic Presets ===
    
    PBRMaterialPreset createGlossyPlastic() {
        PBRMaterialPreset m;
        m.id = "glossy_plastic";
        m.name = "Glossy Plastic";
        m.nameCN = "亮面塑料";
        m.category = MaterialCategory::Plastic;
        m.baseColor = Vec3(0.8f, 0.1f, 0.1f);
        m.metallic = 0.0f;
        m.roughness = 0.1f;
        m.tags = {"plastic", "shiny"};
        return m;
    }
    
    PBRMaterialPreset createMattePlastic() {
        PBRMaterialPreset m;
        m.id = "matte_plastic";
        m.name = "Matte Plastic";
        m.nameCN = "哑光塑料";
        m.category = MaterialCategory::Plastic;
        m.baseColor = Vec3(0.3f, 0.3f, 0.35f);
        m.metallic = 0.0f;
        m.roughness = 0.7f;
        m.tags = {"plastic", "matte"};
        return m;
    }
    
    PBRMaterialPreset createRubber() {
        PBRMaterialPreset m;
        m.id = "rubber";
        m.name = "Rubber";
        m.nameCN = "橡胶";
        m.category = MaterialCategory::Plastic;
        m.baseColor = Vec3(0.1f, 0.1f, 0.1f);
        m.metallic = 0.0f;
        m.roughness = 0.9f;
        m.tags = {"plastic", "soft", "matte"};
        return m;
    }
    
    PBRMaterialPreset createSilicone() {
        PBRMaterialPreset m;
        m.id = "silicone";
        m.name = "Silicone";
        m.nameCN = "硅胶";
        m.category = MaterialCategory::Plastic;
        m.baseColor = Vec3(0.85f, 0.85f, 0.88f);
        m.metallic = 0.0f;
        m.roughness = 0.5f;
        m.subsurface = 0.3f;
        m.tags = {"plastic", "translucent"};
        return m;
    }
    
    // === Glass Presets ===
    
    PBRMaterialPreset createClearGlass() {
        PBRMaterialPreset m;
        m.id = "clear_glass";
        m.name = "Clear Glass";
        m.nameCN = "透明玻璃";
        m.category = MaterialCategory::Glass;
        m.baseColor = Vec3(1, 1, 1);
        m.metallic = 0.0f;
        m.roughness = 0.0f;
        m.transmission = 1.0f;
        m.ior = 1.5f;
        m.tags = {"glass", "transparent"};
        return m;
    }
    
    PBRMaterialPreset createFrostedGlass() {
        PBRMaterialPreset m;
        m.id = "frosted_glass";
        m.name = "Frosted Glass";
        m.nameCN = "磨砂玻璃";
        m.category = MaterialCategory::Glass;
        m.baseColor = Vec3(0.95f, 0.95f, 0.98f);
        m.metallic = 0.0f;
        m.roughness = 0.5f;
        m.transmission = 0.9f;
        m.transmissionRoughness = 0.5f;
        m.ior = 1.5f;
        m.tags = {"glass", "translucent"};
        return m;
    }
    
    PBRMaterialPreset createColoredGlass() {
        PBRMaterialPreset m;
        m.id = "colored_glass";
        m.name = "Colored Glass";
        m.nameCN = "彩色玻璃";
        m.category = MaterialCategory::Glass;
        m.baseColor = Vec3(0.2f, 0.5f, 0.8f);
        m.metallic = 0.0f;
        m.roughness = 0.0f;
        m.transmission = 0.95f;
        m.ior = 1.5f;
        m.tags = {"glass", "colorful"};
        return m;
    }
    
    // === Gemstone Presets ===
    
    PBRMaterialPreset createDiamond() {
        PBRMaterialPreset m;
        m.id = "diamond";
        m.name = "Diamond";
        m.nameCN = "钻石";
        m.category = MaterialCategory::Gemstone;
        m.baseColor = Vec3(1, 1, 1);
        m.metallic = 0.0f;
        m.roughness = 0.0f;
        m.transmission = 1.0f;
        m.ior = 2.42f;
        m.tags = {"gemstone", "precious", "brilliant"};
        return m;
    }
    
    PBRMaterialPreset createRuby() {
        PBRMaterialPreset m;
        m.id = "ruby";
        m.name = "Ruby";
        m.nameCN = "红宝石";
        m.category = MaterialCategory::Gemstone;
        m.baseColor = Vec3(0.9f, 0.1f, 0.15f);
        m.metallic = 0.0f;
        m.roughness = 0.05f;
        m.transmission = 0.8f;
        m.ior = 1.77f;
        m.tags = {"gemstone", "red"};
        return m;
    }
    
    PBRMaterialPreset createEmerald() {
        PBRMaterialPreset m;
        m.id = "emerald";
        m.name = "Emerald";
        m.nameCN = "祖母绿";
        m.category = MaterialCategory::Gemstone;
        m.baseColor = Vec3(0.1f, 0.7f, 0.3f);
        m.metallic = 0.0f;
        m.roughness = 0.05f;
        m.transmission = 0.75f;
        m.ior = 1.58f;
        m.tags = {"gemstone", "green"};
        return m;
    }
    
    PBRMaterialPreset createSapphire() {
        PBRMaterialPreset m;
        m.id = "sapphire";
        m.name = "Sapphire";
        m.nameCN = "蓝宝石";
        m.category = MaterialCategory::Gemstone;
        m.baseColor = Vec3(0.1f, 0.2f, 0.8f);
        m.metallic = 0.0f;
        m.roughness = 0.05f;
        m.transmission = 0.85f;
        m.ior = 1.77f;
        m.tags = {"gemstone", "blue"};
        return m;
    }
    
    PBRMaterialPreset createAmethyst() {
        PBRMaterialPreset m;
        m.id = "amethyst";
        m.name = "Amethyst";
        m.nameCN = "紫水晶";
        m.category = MaterialCategory::Gemstone;
        m.baseColor = Vec3(0.6f, 0.3f, 0.8f);
        m.metallic = 0.0f;
        m.roughness = 0.05f;
        m.transmission = 0.9f;
        m.ior = 1.54f;
        m.tags = {"gemstone", "purple"};
        return m;
    }
    
    PBRMaterialPreset createJade() {
        PBRMaterialPreset m;
        m.id = "jade";
        m.name = "Jade";
        m.nameCN = "翡翠";
        m.category = MaterialCategory::Gemstone;
        m.baseColor = Vec3(0.2f, 0.6f, 0.35f);
        m.metallic = 0.0f;
        m.roughness = 0.2f;
        m.subsurface = 0.4f;
        m.subsurfaceColor = Vec3(0.3f, 0.8f, 0.4f);
        m.ior = 1.65f;
        m.tags = {"gemstone", "green", "asian"};
        return m;
    }
    
    // === Organic Presets ===
    
    PBRMaterialPreset createSkin() {
        PBRMaterialPreset m;
        m.id = "skin";
        m.name = "Skin";
        m.nameCN = "皮肤";
        m.category = MaterialCategory::Organic;
        m.baseColor = Vec3(0.9f, 0.75f, 0.65f);
        m.metallic = 0.0f;
        m.roughness = 0.5f;
        m.subsurface = 0.5f;
        m.subsurfaceColor = Vec3(0.8f, 0.3f, 0.2f);
        m.subsurfaceRadius = 1.0f;
        m.tags = {"organic", "human"};
        return m;
    }
    
    PBRMaterialPreset createHair() {
        PBRMaterialPreset m;
        m.id = "hair";
        m.name = "Hair";
        m.nameCN = "头发";
        m.category = MaterialCategory::Organic;
        m.baseColor = Vec3(0.15f, 0.1f, 0.08f);
        m.metallic = 0.0f;
        m.roughness = 0.4f;
        m.anisotropy = 0.9f;
        m.tags = {"organic", "human"};
        return m;
    }
    
    PBRMaterialPreset createEye() {
        PBRMaterialPreset m;
        m.id = "eye";
        m.name = "Eye";
        m.nameCN = "眼睛";
        m.category = MaterialCategory::Organic;
        m.baseColor = Vec3(0.2f, 0.4f, 0.6f);
        m.metallic = 0.0f;
        m.roughness = 0.0f;
        m.clearcoat = 1.0f;
        m.clearcoatRoughness = 0.0f;
        m.tags = {"organic", "human"};
        return m;
    }
    
    PBRMaterialPreset createNail() {
        PBRMaterialPreset m;
        m.id = "nail";
        m.name = "Nail";
        m.nameCN = "指甲";
        m.category = MaterialCategory::Organic;
        m.baseColor = Vec3(0.95f, 0.85f, 0.8f);
        m.metallic = 0.0f;
        m.roughness = 0.3f;
        m.subsurface = 0.2f;
        m.tags = {"organic", "human"};
        return m;
    }
    
    // === Ceramic Presets ===
    
    PBRMaterialPreset createPorcelain() {
        PBRMaterialPreset m;
        m.id = "porcelain";
        m.name = "Porcelain";
        m.nameCN = "瓷器";
        m.category = MaterialCategory::Ceramic;
        m.baseColor = Vec3(0.98f, 0.97f, 0.95f);
        m.metallic = 0.0f;
        m.roughness = 0.15f;
        m.subsurface = 0.3f;
        m.tags = {"ceramic", "elegant"};
        return m;
    }
    
    PBRMaterialPreset createTerracotta() {
        PBRMaterialPreset m;
        m.id = "terracotta";
        m.name = "Terracotta";
        m.nameCN = "赤陶";
        m.category = MaterialCategory::Ceramic;
        m.baseColor = Vec3(0.8f, 0.45f, 0.3f);
        m.metallic = 0.0f;
        m.roughness = 0.85f;
        m.tags = {"ceramic", "rustic"};
        return m;
    }
    
    PBRMaterialPreset createGlazedCeramic() {
        PBRMaterialPreset m;
        m.id = "glazed_ceramic";
        m.name = "Glazed Ceramic";
        m.nameCN = "釉面陶瓷";
        m.category = MaterialCategory::Ceramic;
        m.baseColor = Vec3(0.2f, 0.4f, 0.7f);
        m.metallic = 0.0f;
        m.roughness = 0.1f;
        m.clearcoat = 0.8f;
        m.tags = {"ceramic", "shiny"};
        return m;
    }
    
    // === Emissive Presets ===
    
    PBRMaterialPreset createNeonRed() {
        PBRMaterialPreset m;
        m.id = "neon_red";
        m.name = "Neon Red";
        m.nameCN = "霓虹红";
        m.category = MaterialCategory::Emissive;
        m.baseColor = Vec3(1.0f, 0.1f, 0.1f);
        m.metallic = 0.0f;
        m.roughness = 0.3f;
        m.emissiveColor = Vec3(1.0f, 0.0f, 0.0f);
        m.emissiveIntensity = 5.0f;
        m.tags = {"emissive", "neon", "red"};
        return m;
    }
    
    PBRMaterialPreset createNeonBlue() {
        PBRMaterialPreset m;
        m.id = "neon_blue";
        m.name = "Neon Blue";
        m.nameCN = "霓虹蓝";
        m.category = MaterialCategory::Emissive;
        m.baseColor = Vec3(0.1f, 0.3f, 1.0f);
        m.metallic = 0.0f;
        m.roughness = 0.3f;
        m.emissiveColor = Vec3(0.0f, 0.3f, 1.0f);
        m.emissiveIntensity = 5.0f;
        m.tags = {"emissive", "neon", "blue"};
        return m;
    }
    
    PBRMaterialPreset createNeonGreen() {
        PBRMaterialPreset m;
        m.id = "neon_green";
        m.name = "Neon Green";
        m.nameCN = "霓虹绿";
        m.category = MaterialCategory::Emissive;
        m.baseColor = Vec3(0.1f, 1.0f, 0.3f);
        m.metallic = 0.0f;
        m.roughness = 0.3f;
        m.emissiveColor = Vec3(0.0f, 1.0f, 0.2f);
        m.emissiveIntensity = 5.0f;
        m.tags = {"emissive", "neon", "green"};
        return m;
    }
    
    PBRMaterialPreset createLava() {
        PBRMaterialPreset m;
        m.id = "lava";
        m.name = "Lava";
        m.nameCN = "熔岩";
        m.category = MaterialCategory::Emissive;
        m.baseColor = Vec3(0.3f, 0.1f, 0.05f);
        m.metallic = 0.0f;
        m.roughness = 0.8f;
        m.emissiveColor = Vec3(1.0f, 0.3f, 0.0f);
        m.emissiveIntensity = 3.0f;
        m.tags = {"emissive", "hot", "volcanic"};
        return m;
    }
    
    PBRMaterialPreset createHologram() {
        PBRMaterialPreset m;
        m.id = "hologram";
        m.name = "Hologram";
        m.nameCN = "全息";
        m.category = MaterialCategory::Emissive;
        m.baseColor = Vec3(0.3f, 0.8f, 1.0f);
        m.metallic = 0.0f;
        m.roughness = 0.0f;
        m.opacity = 0.5f;
        m.emissiveColor = Vec3(0.3f, 0.8f, 1.0f);
        m.emissiveIntensity = 2.0f;
        m.tags = {"emissive", "scifi", "transparent"};
        return m;
    }
    
    // === Stylized Presets ===
    
    PBRMaterialPreset createToon() {
        PBRMaterialPreset m;
        m.id = "toon";
        m.name = "Toon";
        m.nameCN = "卡通";
        m.category = MaterialCategory::Stylized;
        m.baseColor = Vec3(1.0f, 0.6f, 0.4f);
        m.metallic = 0.0f;
        m.roughness = 1.0f;
        m.description = "Flat cartoon shading";
        m.tags = {"stylized", "cartoon", "flat"};
        return m;
    }
    
    PBRMaterialPreset createWatercolor() {
        PBRMaterialPreset m;
        m.id = "watercolor";
        m.name = "Watercolor";
        m.nameCN = "水彩";
        m.category = MaterialCategory::Stylized;
        m.baseColor = Vec3(0.8f, 0.9f, 0.95f);
        m.metallic = 0.0f;
        m.roughness = 0.9f;
        m.description = "Soft watercolor look";
        m.tags = {"stylized", "artistic", "soft"};
        return m;
    }
    
    PBRMaterialPreset createCelShaded() {
        PBRMaterialPreset m;
        m.id = "cel_shaded";
        m.name = "Cel Shaded";
        m.nameCN = "赛璐璐";
        m.category = MaterialCategory::Stylized;
        m.baseColor = Vec3(0.95f, 0.8f, 0.7f);
        m.metallic = 0.0f;
        m.roughness = 1.0f;
        m.description = "Anime-style cel shading";
        m.tags = {"stylized", "anime", "cel"};
        return m;
    }
    
    std::unordered_map<std::string, PBRMaterialPreset> presets_;
    bool initialized_ = false;
};

// ============================================================================
// Convenience
// ============================================================================

inline MaterialLibrary& getMaterialLibrary() {
    return MaterialLibrary::getInstance();
}

}  // namespace luma