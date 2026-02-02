// Decal System - Tattoos, scars, makeup, body paint
// Project textures onto character mesh surface
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

namespace luma {

// ============================================================================
// Decal Types
// ============================================================================

enum class DecalType {
    Tattoo,         // 纹身
    Scar,           // 伤疤
    Birthmark,      // 胎记
    Makeup,         // 妆容
    BodyPaint,      // 身体彩绘
    FacePaint,      // 脸部彩绘
    Wound,          // 伤口
    Dirt,           // 污渍
    Sweat,          // 汗水
    Blood,          // 血迹
    Freckles,       // 雀斑
    Wrinkles,       // 皱纹
    Custom          // 自定义
};

inline std::string decalTypeToString(DecalType type) {
    switch (type) {
        case DecalType::Tattoo: return "Tattoo";
        case DecalType::Scar: return "Scar";
        case DecalType::Birthmark: return "Birthmark";
        case DecalType::Makeup: return "Makeup";
        case DecalType::BodyPaint: return "BodyPaint";
        case DecalType::FacePaint: return "FacePaint";
        case DecalType::Wound: return "Wound";
        case DecalType::Dirt: return "Dirt";
        case DecalType::Sweat: return "Sweat";
        case DecalType::Blood: return "Blood";
        case DecalType::Freckles: return "Freckles";
        case DecalType::Wrinkles: return "Wrinkles";
        case DecalType::Custom: return "Custom";
        default: return "Unknown";
    }
}

inline std::string decalTypeToDisplayName(DecalType type) {
    switch (type) {
        case DecalType::Tattoo: return "纹身 Tattoo";
        case DecalType::Scar: return "伤疤 Scar";
        case DecalType::Birthmark: return "胎记 Birthmark";
        case DecalType::Makeup: return "妆容 Makeup";
        case DecalType::BodyPaint: return "彩绘 BodyPaint";
        case DecalType::FacePaint: return "脸绘 FacePaint";
        case DecalType::Wound: return "伤口 Wound";
        case DecalType::Dirt: return "污渍 Dirt";
        case DecalType::Sweat: return "汗水 Sweat";
        case DecalType::Blood: return "血迹 Blood";
        case DecalType::Freckles: return "雀斑 Freckles";
        case DecalType::Wrinkles: return "皱纹 Wrinkles";
        case DecalType::Custom: return "自定义 Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Body Region for Decal Placement
// ============================================================================

enum class BodyRegion {
    Face,
    FaceLeft,
    FaceRight,
    Forehead,
    Cheek,
    Neck,
    Chest,
    Back,
    Stomach,
    LeftUpperArm,
    RightUpperArm,
    LeftLowerArm,
    RightLowerArm,
    LeftHand,
    RightHand,
    LeftUpperLeg,
    RightUpperLeg,
    LeftLowerLeg,
    RightLowerLeg,
    FullBody,
    Custom
};

inline std::string bodyRegionToString(BodyRegion region) {
    switch (region) {
        case BodyRegion::Face: return "Face";
        case BodyRegion::FaceLeft: return "FaceLeft";
        case BodyRegion::FaceRight: return "FaceRight";
        case BodyRegion::Forehead: return "Forehead";
        case BodyRegion::Cheek: return "Cheek";
        case BodyRegion::Neck: return "Neck";
        case BodyRegion::Chest: return "Chest";
        case BodyRegion::Back: return "Back";
        case BodyRegion::Stomach: return "Stomach";
        case BodyRegion::LeftUpperArm: return "LeftUpperArm";
        case BodyRegion::RightUpperArm: return "RightUpperArm";
        case BodyRegion::LeftLowerArm: return "LeftLowerArm";
        case BodyRegion::RightLowerArm: return "RightLowerArm";
        case BodyRegion::LeftHand: return "LeftHand";
        case BodyRegion::RightHand: return "RightHand";
        case BodyRegion::LeftUpperLeg: return "LeftUpperLeg";
        case BodyRegion::RightUpperLeg: return "RightUpperLeg";
        case BodyRegion::LeftLowerLeg: return "LeftLowerLeg";
        case BodyRegion::RightLowerLeg: return "RightLowerLeg";
        case BodyRegion::FullBody: return "FullBody";
        case BodyRegion::Custom: return "Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Decal Asset
// ============================================================================

struct DecalAsset {
    std::string id;
    std::string name;
    std::string nameCN;
    std::string description;
    DecalType type = DecalType::Custom;
    
    // Texture
    std::string texturePath;
    TextureData texture;
    bool textureLoaded = false;
    
    // Normal map (for scars, wounds that affect surface)
    std::string normalMapPath;
    TextureData normalMap;
    bool hasNormalMap = false;
    
    // Default placement
    BodyRegion defaultRegion = BodyRegion::Custom;
    Vec2 defaultUVCenter{0.5f, 0.5f};
    Vec2 defaultUVSize{0.1f, 0.1f};
    float defaultRotation = 0.0f;
    
    // Material properties
    Vec3 defaultColor{1, 1, 1};
    float defaultOpacity = 1.0f;
    bool allowColorCustomization = true;
    
    // Blend mode
    enum class BlendMode {
        Normal,     // Standard alpha blend
        Multiply,   // Darken (good for dirt, shadows)
        Overlay,    // Mix (good for tattoos)
        Additive    // Lighten (good for glow effects)
    };
    BlendMode blendMode = BlendMode::Normal;
    
    // Surface properties (for scars, wounds)
    float roughnessModifier = 0.0f;  // Add to base roughness
    float metallicModifier = 0.0f;
    float bumpStrength = 0.0f;
    
    // Tags
    std::vector<std::string> tags;
    std::vector<std::string> compatibleStyles;
    
    // Thumbnail
    std::string thumbnailPath;
};

// ============================================================================
// Applied Decal Instance
// ============================================================================

struct AppliedDecal {
    std::string assetId;
    const DecalAsset* asset = nullptr;
    
    // Placement in UV space
    Vec2 uvCenter{0.5f, 0.5f};
    Vec2 uvSize{0.1f, 0.1f};
    float rotation = 0.0f;  // Radians
    
    // Appearance
    Vec3 color{1, 1, 1};
    float opacity = 1.0f;
    
    // Region hint (for easier positioning)
    BodyRegion region = BodyRegion::Custom;
    
    // Visibility
    bool visible = true;
    
    // Layer order (higher = on top)
    int layer = 0;
};

// ============================================================================
// Procedural Decal Generator
// ============================================================================

class ProceduralDecalGenerator {
public:
    // Generate simple tattoo patterns
    static TextureData generateTribalPattern(int size = 256) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4, 0);
        
        // Simple tribal-like pattern with curves
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size - 0.5f;
                float v = (float)y / size - 0.5f;
                
                // Distance from center
                float dist = std::sqrt(u * u + v * v);
                
                // Angle
                float angle = std::atan2(v, u);
                
                // Create spiky pattern
                float pattern = std::sin(angle * 6) * 0.3f + 0.2f;
                pattern += std::sin(angle * 3) * 0.15f;
                
                float alpha = 0.0f;
                if (dist < pattern && dist > pattern - 0.08f) {
                    alpha = 1.0f;
                }
                
                // Smooth edges
                float edgeDist = std::abs(dist - (pattern - 0.04f));
                if (edgeDist < 0.04f) {
                    alpha = 1.0f - edgeDist / 0.04f;
                }
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = 0;      // R
                tex.pixels[idx + 1] = 0;      // G
                tex.pixels[idx + 2] = 0;      // B
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 255);  // A
            }
        }
        
        return tex;
    }
    
    // Generate scar texture
    static TextureData generateScar(int size = 256, float length = 0.6f, float width = 0.08f) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4, 0);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size - 0.5f;
                float v = (float)y / size - 0.5f;
                
                // Distance from center line
                float distFromLine = std::abs(u);
                
                // Length check
                if (std::abs(v) > length / 2) continue;
                
                // Jagged edges
                float jagged = std::sin(v * 40) * 0.01f;
                float adjustedWidth = width / 2 + jagged;
                
                float alpha = 0.0f;
                if (distFromLine < adjustedWidth) {
                    alpha = 1.0f - distFromLine / adjustedWidth;
                    alpha = alpha * alpha;  // Sharper falloff
                }
                
                // Scar color (pinkish)
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(200 * alpha);
                tex.pixels[idx + 1] = static_cast<uint8_t>(150 * alpha);
                tex.pixels[idx + 2] = static_cast<uint8_t>(150 * alpha);
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 200);
            }
        }
        
        return tex;
    }
    
    // Generate freckles
    static TextureData generateFreckles(int size = 256, int count = 50) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4, 0);
        
        // Random freckle positions
        std::vector<Vec3> freckles;  // x, y, radius
        unsigned int seed = 12345;
        
        for (int i = 0; i < count; i++) {
            seed = seed * 1103515245 + 12345;
            float x = (float)(seed % 1000) / 1000.0f;
            seed = seed * 1103515245 + 12345;
            float y = (float)(seed % 1000) / 1000.0f;
            seed = seed * 1103515245 + 12345;
            float r = 0.005f + (float)(seed % 100) / 100.0f * 0.015f;
            
            freckles.push_back(Vec3(x, y, r));
        }
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size;
                float v = (float)y / size;
                
                float alpha = 0.0f;
                
                for (const auto& f : freckles) {
                    float dx = u - f.x;
                    float dy = v - f.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    
                    if (dist < f.z) {
                        float falloff = 1.0f - dist / f.z;
                        alpha = std::max(alpha, falloff * falloff);
                    }
                }
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(120);  // Brown color
                tex.pixels[idx + 1] = static_cast<uint8_t>(80);
                tex.pixels[idx + 2] = static_cast<uint8_t>(50);
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 180);
            }
        }
        
        return tex;
    }
    
    // Generate makeup (lipstick, eyeshadow base)
    static TextureData generateMakeupBase(int size = 256, Vec3 color = Vec3(0.8f, 0.2f, 0.2f)) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4, 0);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size - 0.5f;
                float v = (float)y / size - 0.5f;
                
                float dist = std::sqrt(u * u + v * v);
                
                float alpha = 0.0f;
                if (dist < 0.4f) {
                    alpha = 1.0f - dist / 0.4f;
                    alpha = std::sqrt(alpha);  // Softer falloff
                }
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(color.x * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(color.y * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(color.z * 255);
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 200);
            }
        }
        
        return tex;
    }
    
    // Generate wound texture
    static TextureData generateWound(int size = 256) {
        TextureData tex;
        tex.width = size;
        tex.height = size;
        tex.channels = 4;
        tex.pixels.resize(size * size * 4, 0);
        
        unsigned int seed = 54321;
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size - 0.5f;
                float v = (float)y / size - 0.5f;
                
                // Elongated shape
                float distX = std::abs(u) / 0.3f;
                float distY = std::abs(v) / 0.15f;
                float dist = std::sqrt(distX * distX + distY * distY);
                
                float alpha = 0.0f;
                if (dist < 1.0f) {
                    alpha = 1.0f - dist;
                    
                    // Add noise for irregular edges
                    seed = seed * 1103515245 + 12345;
                    float noise = (float)(seed % 100) / 100.0f * 0.3f;
                    alpha *= (0.7f + noise);
                }
                
                // Gradient from red center to dark edges
                float r = 0.4f + 0.4f * (1.0f - dist);
                float g = 0.1f + 0.15f * (1.0f - dist);
                float b = 0.1f + 0.1f * (1.0f - dist);
                
                int idx = (y * size + x) * 4;
                tex.pixels[idx + 0] = static_cast<uint8_t>(r * 255);
                tex.pixels[idx + 1] = static_cast<uint8_t>(g * 255);
                tex.pixels[idx + 2] = static_cast<uint8_t>(b * 255);
                tex.pixels[idx + 3] = static_cast<uint8_t>(alpha * 255);
            }
        }
        
        return tex;
    }
};

// ============================================================================
// Decal Library
// ============================================================================

class DecalLibrary {
public:
    static DecalLibrary& getInstance() {
        static DecalLibrary instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // === Tattoos ===
        {
            DecalAsset asset;
            asset.id = "tattoo_tribal_01";
            asset.name = "Tribal Pattern";
            asset.nameCN = "部落图腾";
            asset.type = DecalType::Tattoo;
            asset.defaultRegion = BodyRegion::LeftUpperArm;
            asset.defaultUVSize = Vec2(0.15f, 0.15f);
            asset.texture = ProceduralDecalGenerator::generateTribalPattern(256);
            asset.textureLoaded = true;
            asset.blendMode = DecalAsset::BlendMode::Multiply;
            asset.tags = {"tribal", "arm", "traditional"};
            addAsset(asset);
        }
        
        // === Scars ===
        {
            DecalAsset asset;
            asset.id = "scar_slash_01";
            asset.name = "Slash Scar";
            asset.nameCN = "刀疤";
            asset.type = DecalType::Scar;
            asset.defaultRegion = BodyRegion::Face;
            asset.defaultUVSize = Vec2(0.08f, 0.15f);
            asset.texture = ProceduralDecalGenerator::generateScar(256);
            asset.textureLoaded = true;
            asset.roughnessModifier = 0.2f;
            asset.bumpStrength = 0.3f;
            asset.tags = {"scar", "face", "battle"};
            addAsset(asset);
        }
        
        // === Birthmarks/Freckles ===
        {
            DecalAsset asset;
            asset.id = "freckles_01";
            asset.name = "Freckles";
            asset.nameCN = "雀斑";
            asset.type = DecalType::Freckles;
            asset.defaultRegion = BodyRegion::Face;
            asset.defaultUVSize = Vec2(0.3f, 0.2f);
            asset.texture = ProceduralDecalGenerator::generateFreckles(256, 60);
            asset.textureLoaded = true;
            asset.blendMode = DecalAsset::BlendMode::Multiply;
            asset.tags = {"freckles", "face", "cute"};
            addAsset(asset);
        }
        
        // === Makeup ===
        {
            DecalAsset asset;
            asset.id = "makeup_blush";
            asset.name = "Blush";
            asset.nameCN = "腮红";
            asset.type = DecalType::Makeup;
            asset.defaultRegion = BodyRegion::Cheek;
            asset.defaultUVSize = Vec2(0.08f, 0.06f);
            asset.texture = ProceduralDecalGenerator::generateMakeupBase(256, Vec3(0.9f, 0.5f, 0.5f));
            asset.textureLoaded = true;
            asset.defaultOpacity = 0.5f;
            asset.allowColorCustomization = true;
            asset.tags = {"makeup", "blush", "cheek"};
            addAsset(asset);
        }
        
        {
            DecalAsset asset;
            asset.id = "makeup_lipstick";
            asset.name = "Lipstick";
            asset.nameCN = "口红";
            asset.type = DecalType::Makeup;
            asset.defaultRegion = BodyRegion::Face;
            asset.defaultUVSize = Vec2(0.06f, 0.03f);
            asset.defaultUVCenter = Vec2(0.5f, 0.35f);  // Mouth area
            asset.texture = ProceduralDecalGenerator::generateMakeupBase(256, Vec3(0.8f, 0.15f, 0.2f));
            asset.textureLoaded = true;
            asset.allowColorCustomization = true;
            asset.tags = {"makeup", "lips", "lipstick"};
            addAsset(asset);
        }
        
        // === Wounds ===
        {
            DecalAsset asset;
            asset.id = "wound_scratch";
            asset.name = "Scratch Wound";
            asset.nameCN = "抓伤";
            asset.type = DecalType::Wound;
            asset.defaultRegion = BodyRegion::Chest;
            asset.defaultUVSize = Vec2(0.1f, 0.05f);
            asset.texture = ProceduralDecalGenerator::generateWound(256);
            asset.textureLoaded = true;
            asset.roughnessModifier = 0.3f;
            asset.tags = {"wound", "battle", "scratch"};
            addAsset(asset);
        }
        
        initialized_ = true;
    }
    
    const DecalAsset* getAsset(const std::string& id) const {
        auto it = assets_.find(id);
        return (it != assets_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getAssetIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : assets_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    std::vector<const DecalAsset*> getAssetsByType(DecalType type) const {
        std::vector<const DecalAsset*> result;
        for (const auto& [id, asset] : assets_) {
            if (asset.type == type) {
                result.push_back(&asset);
            }
        }
        return result;
    }
    
    void addAsset(const DecalAsset& asset) {
        assets_[asset.id] = asset;
    }
    
private:
    DecalLibrary() { initialize(); }
    
    std::unordered_map<std::string, DecalAsset> assets_;
    bool initialized_ = false;
};

// ============================================================================
// Decal Manager - Per character
// ============================================================================

class DecalManager {
public:
    DecalManager() = default;
    
    // Apply decal
    bool applyDecal(const std::string& assetId, const Vec2& uvCenter, const Vec2& uvSize, float rotation = 0.0f) {
        auto* asset = DecalLibrary::getInstance().getAsset(assetId);
        if (!asset) return false;
        
        AppliedDecal decal;
        decal.assetId = assetId;
        decal.asset = asset;
        decal.uvCenter = uvCenter;
        decal.uvSize = uvSize;
        decal.rotation = rotation;
        decal.color = asset->defaultColor;
        decal.opacity = asset->defaultOpacity;
        decal.region = asset->defaultRegion;
        decal.layer = static_cast<int>(decals_.size());
        
        decals_.push_back(decal);
        return true;
    }
    
    // Apply decal with default placement
    bool applyDecal(const std::string& assetId) {
        auto* asset = DecalLibrary::getInstance().getAsset(assetId);
        if (!asset) return false;
        
        return applyDecal(assetId, asset->defaultUVCenter, asset->defaultUVSize, asset->defaultRotation);
    }
    
    // Remove decal by index
    bool removeDecal(int index) {
        if (index < 0 || index >= static_cast<int>(decals_.size())) return false;
        decals_.erase(decals_.begin() + index);
        return true;
    }
    
    // Remove all decals of a type
    void removeDecalsByType(DecalType type) {
        decals_.erase(
            std::remove_if(decals_.begin(), decals_.end(),
                [&](const AppliedDecal& d) { return d.asset && d.asset->type == type; }),
            decals_.end());
    }
    
    // Clear all decals
    void clearAll() {
        decals_.clear();
    }
    
    // Get all applied decals
    const std::vector<AppliedDecal>& getDecals() const {
        return decals_;
    }
    
    std::vector<AppliedDecal>& getDecals() {
        return decals_;
    }
    
    // Bake decals into a texture
    TextureData bakeDecalsToTexture(int width, int height) const {
        TextureData result;
        result.width = width;
        result.height = height;
        result.channels = 4;
        result.pixels.resize(width * height * 4, 0);
        
        // Sort decals by layer
        std::vector<const AppliedDecal*> sorted;
        for (const auto& d : decals_) {
            if (d.visible) sorted.push_back(&d);
        }
        std::sort(sorted.begin(), sorted.end(),
            [](const AppliedDecal* a, const AppliedDecal* b) { return a->layer < b->layer; });
        
        // Composite each decal
        for (const auto* decal : sorted) {
            if (!decal->asset || !decal->asset->textureLoaded) continue;
            
            const TextureData& src = decal->asset->texture;
            
            // Calculate UV bounds
            float minU = decal->uvCenter.x - decal->uvSize.x / 2;
            float maxU = decal->uvCenter.x + decal->uvSize.x / 2;
            float minV = decal->uvCenter.y - decal->uvSize.y / 2;
            float maxV = decal->uvCenter.y + decal->uvSize.y / 2;
            
            // Rasterize
            int startX = static_cast<int>(minU * width);
            int endX = static_cast<int>(maxU * width);
            int startY = static_cast<int>(minV * height);
            int endY = static_cast<int>(maxV * height);
            
            for (int y = std::max(0, startY); y < std::min(height, endY); y++) {
                for (int x = std::max(0, startX); x < std::min(width, endX); x++) {
                    // Calculate source UV
                    float u = (float)(x - startX) / (endX - startX);
                    float v = (float)(y - startY) / (endY - startY);
                    
                    // Apply rotation around center
                    if (std::abs(decal->rotation) > 0.001f) {
                        float cu = u - 0.5f;
                        float cv = v - 0.5f;
                        float cosR = std::cos(-decal->rotation);
                        float sinR = std::sin(-decal->rotation);
                        u = cu * cosR - cv * sinR + 0.5f;
                        v = cu * sinR + cv * cosR + 0.5f;
                    }
                    
                    // Skip if outside source texture
                    if (u < 0 || u >= 1 || v < 0 || v >= 1) continue;
                    
                    // Sample source
                    int srcX = static_cast<int>(u * src.width);
                    int srcY = static_cast<int>(v * src.height);
                    srcX = std::clamp(srcX, 0, src.width - 1);
                    srcY = std::clamp(srcY, 0, src.height - 1);
                    
                    int srcIdx = (srcY * src.width + srcX) * src.channels;
                    int dstIdx = (y * width + x) * 4;
                    
                    // Get source color
                    float sr = src.pixels[srcIdx] / 255.0f * decal->color.x;
                    float sg = src.pixels[srcIdx + 1] / 255.0f * decal->color.y;
                    float sb = src.pixels[srcIdx + 2] / 255.0f * decal->color.z;
                    float sa = (src.channels == 4 ? src.pixels[srcIdx + 3] / 255.0f : 1.0f) * decal->opacity;
                    
                    // Get dest color
                    float dr = result.pixels[dstIdx] / 255.0f;
                    float dg = result.pixels[dstIdx + 1] / 255.0f;
                    float db = result.pixels[dstIdx + 2] / 255.0f;
                    float da = result.pixels[dstIdx + 3] / 255.0f;
                    
                    // Blend based on mode
                    float or_, og, ob, oa;
                    
                    switch (decal->asset->blendMode) {
                        case DecalAsset::BlendMode::Multiply:
                            or_ = dr * sr;
                            og = dg * sg;
                            ob = db * sb;
                            oa = std::max(da, sa);
                            break;
                            
                        case DecalAsset::BlendMode::Overlay:
                            or_ = dr < 0.5f ? 2 * dr * sr : 1 - 2 * (1 - dr) * (1 - sr);
                            og = dg < 0.5f ? 2 * dg * sg : 1 - 2 * (1 - dg) * (1 - sg);
                            ob = db < 0.5f ? 2 * db * sb : 1 - 2 * (1 - db) * (1 - sb);
                            oa = std::max(da, sa);
                            break;
                            
                        case DecalAsset::BlendMode::Additive:
                            or_ = std::min(dr + sr * sa, 1.0f);
                            og = std::min(dg + sg * sa, 1.0f);
                            ob = std::min(db + sb * sa, 1.0f);
                            oa = std::max(da, sa);
                            break;
                            
                        default:  // Normal blend
                            or_ = sr * sa + dr * (1 - sa);
                            og = sg * sa + dg * (1 - sa);
                            ob = sb * sa + db * (1 - sa);
                            oa = sa + da * (1 - sa);
                            break;
                    }
                    
                    result.pixels[dstIdx] = static_cast<uint8_t>(or_ * 255);
                    result.pixels[dstIdx + 1] = static_cast<uint8_t>(og * 255);
                    result.pixels[dstIdx + 2] = static_cast<uint8_t>(ob * 255);
                    result.pixels[dstIdx + 3] = static_cast<uint8_t>(oa * 255);
                }
            }
        }
        
        return result;
    }
    
private:
    std::vector<AppliedDecal> decals_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline DecalLibrary& getDecalLibrary() {
    return DecalLibrary::getInstance();
}

}  // namespace luma
