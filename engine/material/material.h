// PBR Material System
// Supports standard PBR workflow with metallic-roughness model
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace luma {

// Forward declaration
struct RHITexture;

// ===== Material Texture Slots =====
enum class TextureSlot : uint8_t {
    Albedo = 0,
    Normal,
    MetallicRoughness,
    Occlusion,
    Emissive,
    Height,
    Count
};

constexpr size_t TEXTURE_SLOT_COUNT = static_cast<size_t>(TextureSlot::Count);

// ===== PBR Material =====
struct Material {
    // Identity
    std::string name = "Default Material";
    uint32_t id = 0;
    
    // Base Color / Albedo
    Vec3 baseColor{1.0f, 1.0f, 1.0f};
    float alpha = 1.0f;
    
    // PBR Properties
    float metallic = 0.0f;      // 0 = dielectric, 1 = metal
    float roughness = 0.5f;     // 0 = smooth/mirror, 1 = rough/diffuse
    float ao = 1.0f;            // Ambient occlusion multiplier
    
    // Emissive
    Vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f;
    
    // Additional properties
    float normalStrength = 1.0f;
    float heightScale = 0.05f;
    float ior = 1.5f;           // Index of refraction for dielectrics
    
    // Texture handles (platform-specific, managed externally)
    void* textures[TEXTURE_SLOT_COUNT] = {nullptr};
    std::string texturePaths[TEXTURE_SLOT_COUNT];
    
    // Rendering flags
    bool twoSided = false;
    bool alphaBlend = false;
    bool alphaCutoff = false;
    float alphaCutoffValue = 0.5f;
    
    // Check if material has specific texture
    bool hasTexture(TextureSlot slot) const {
        return textures[static_cast<size_t>(slot)] != nullptr;
    }
    
    // Get texture slot name for UI
    static const char* getSlotName(TextureSlot slot) {
        switch (slot) {
            case TextureSlot::Albedo: return "Albedo";
            case TextureSlot::Normal: return "Normal";
            case TextureSlot::MetallicRoughness: return "Metallic/Roughness";
            case TextureSlot::Occlusion: return "Ambient Occlusion";
            case TextureSlot::Emissive: return "Emissive";
            case TextureSlot::Height: return "Height/Displacement";
            default: return "Unknown";
        }
    }
    
    // Default material presets
    static Material createDefault() {
        Material mat;
        mat.name = "Default";
        return mat;
    }
    
    static Material createMetal(const Vec3& color = {0.9f, 0.9f, 0.9f}) {
        Material mat;
        mat.name = "Metal";
        mat.baseColor = color;
        mat.metallic = 1.0f;
        mat.roughness = 0.3f;
        return mat;
    }
    
    static Material createRubber(const Vec3& color = {0.1f, 0.1f, 0.1f}) {
        Material mat;
        mat.name = "Rubber";
        mat.baseColor = color;
        mat.metallic = 0.0f;
        mat.roughness = 0.9f;
        return mat;
    }
    
    static Material createPlastic(const Vec3& color = {0.8f, 0.2f, 0.2f}) {
        Material mat;
        mat.name = "Plastic";
        mat.baseColor = color;
        mat.metallic = 0.0f;
        mat.roughness = 0.4f;
        return mat;
    }
    
    static Material createGold() {
        Material mat;
        mat.name = "Gold";
        mat.baseColor = {1.0f, 0.766f, 0.336f};
        mat.metallic = 1.0f;
        mat.roughness = 0.1f;
        return mat;
    }
    
    static Material createSilver() {
        Material mat;
        mat.name = "Silver";
        mat.baseColor = {0.972f, 0.960f, 0.915f};
        mat.metallic = 1.0f;
        mat.roughness = 0.15f;
        return mat;
    }
    
    static Material createCopper() {
        Material mat;
        mat.name = "Copper";
        mat.baseColor = {0.955f, 0.637f, 0.538f};
        mat.metallic = 1.0f;
        mat.roughness = 0.25f;
        return mat;
    }
    
    static Material createGlass() {
        Material mat;
        mat.name = "Glass";
        mat.baseColor = {1.0f, 1.0f, 1.0f};
        mat.alpha = 0.2f;
        mat.metallic = 0.0f;
        mat.roughness = 0.1f;
        mat.ior = 1.52f;
        mat.alphaBlend = true;
        return mat;
    }
    
    static Material createEmissive(const Vec3& color = {1.0f, 0.5f, 0.0f}, float intensity = 5.0f) {
        Material mat;
        mat.name = "Emissive";
        mat.baseColor = color;
        mat.emissiveColor = color;
        mat.emissiveIntensity = intensity;
        mat.metallic = 0.0f;
        mat.roughness = 0.5f;
        return mat;
    }
};

// ===== Material Library =====
// Manages material instances and presets
class MaterialLibrary {
public:
    static MaterialLibrary& get() {
        static MaterialLibrary instance;
        return instance;
    }
    
    // Create a new material
    std::shared_ptr<Material> createMaterial(const std::string& name) {
        auto mat = std::make_shared<Material>();
        mat->name = name;
        mat->id = nextId_++;
        materials_[mat->id] = mat;
        return mat;
    }
    
    // Get material by ID
    std::shared_ptr<Material> getMaterial(uint32_t id) {
        auto it = materials_.find(id);
        return it != materials_.end() ? it->second : nullptr;
    }
    
    // Get material by name
    std::shared_ptr<Material> findByName(const std::string& name) {
        for (auto& [id, mat] : materials_) {
            if (mat->name == name) return mat;
        }
        return nullptr;
    }
    
    // Register preset materials
    void registerPresets() {
        auto defMat = std::make_shared<Material>(Material::createDefault());
        defMat->id = nextId_++;
        materials_[defMat->id] = defMat;
        presets_["Default"] = defMat;
        
        auto gold = std::make_shared<Material>(Material::createGold());
        gold->id = nextId_++;
        materials_[gold->id] = gold;
        presets_["Gold"] = gold;
        
        auto silver = std::make_shared<Material>(Material::createSilver());
        silver->id = nextId_++;
        materials_[silver->id] = silver;
        presets_["Silver"] = silver;
        
        auto copper = std::make_shared<Material>(Material::createCopper());
        copper->id = nextId_++;
        materials_[copper->id] = copper;
        presets_["Copper"] = copper;
        
        auto plastic = std::make_shared<Material>(Material::createPlastic());
        plastic->id = nextId_++;
        materials_[plastic->id] = plastic;
        presets_["Plastic"] = plastic;
        
        auto rubber = std::make_shared<Material>(Material::createRubber());
        rubber->id = nextId_++;
        materials_[rubber->id] = rubber;
        presets_["Rubber"] = rubber;
        
        auto glass = std::make_shared<Material>(Material::createGlass());
        glass->id = nextId_++;
        materials_[glass->id] = glass;
        presets_["Glass"] = glass;
        
        auto emissive = std::make_shared<Material>(Material::createEmissive());
        emissive->id = nextId_++;
        materials_[emissive->id] = emissive;
        presets_["Emissive"] = emissive;
    }
    
    // Get all presets
    const std::unordered_map<std::string, std::shared_ptr<Material>>& getPresets() const {
        return presets_;
    }
    
    // Get all materials
    const std::unordered_map<uint32_t, std::shared_ptr<Material>>& getAllMaterials() const {
        return materials_;
    }
    
    // Duplicate a material
    std::shared_ptr<Material> duplicateMaterial(const Material& source) {
        auto mat = std::make_shared<Material>(source);
        mat->id = nextId_++;
        mat->name = source.name + " (Copy)";
        materials_[mat->id] = mat;
        return mat;
    }
    
    // Remove a material
    void removeMaterial(uint32_t id) {
        materials_.erase(id);
    }
    
private:
    MaterialLibrary() {
        registerPresets();
    }
    
    std::unordered_map<uint32_t, std::shared_ptr<Material>> materials_;
    std::unordered_map<std::string, std::shared_ptr<Material>> presets_;
    uint32_t nextId_ = 1;
};

// ===== Legacy compatibility =====
struct MaterialData {
    std::unordered_map<std::string, std::string> parameters;
    int variant{0};
};

}  // namespace luma
