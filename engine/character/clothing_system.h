// Clothing System - Complete clothing management for characters
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character_body.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <fstream>
#include <sstream>

// Forward declarations for extended clothing features
namespace luma {
    class ClothSimulation;
    class ClothingSkinningDeformer;
    struct ClothingSkinData;
    struct ClothingMaterialSet;
    enum class FabricType;
}

namespace luma {

// ============================================================================
// Clothing Categories and Slots
// ============================================================================

enum class ClothingCategory {
    Top,            // Upper body clothing
    Bottom,         // Lower body clothing
    FullBody,       // Full body clothing (dresses, suits)
    Footwear,       // Shoes, boots, sandals
    Headwear,       // Hats, helmets, hair accessories
    Eyewear,        // Glasses, goggles
    Handwear,       // Gloves
    Accessory,      // Jewelry, bags, belts
    Underwear,      // Undergarments
    Outerwear       // Jackets, coats
};

inline const char* getCategoryName(ClothingCategory cat) {
    switch (cat) {
        case ClothingCategory::Top: return "Top";
        case ClothingCategory::Bottom: return "Bottom";
        case ClothingCategory::FullBody: return "Full Body";
        case ClothingCategory::Footwear: return "Footwear";
        case ClothingCategory::Headwear: return "Headwear";
        case ClothingCategory::Eyewear: return "Eyewear";
        case ClothingCategory::Handwear: return "Handwear";
        case ClothingCategory::Accessory: return "Accessory";
        case ClothingCategory::Underwear: return "Underwear";
        case ClothingCategory::Outerwear: return "Outerwear";
    }
    return "Unknown";
}

// Slots define what a clothing item occupies
enum class ClothingSlot {
    // Upper body
    Shirt,
    Jacket,
    Vest,
    Bra,
    
    // Lower body  
    Pants,
    Shorts,
    Skirt,
    Underwear,
    
    // Full body
    Dress,
    Suit,
    Jumpsuit,
    
    // Footwear
    Shoes,
    Boots,
    Sandals,
    Socks,
    
    // Head
    Hat,
    Helmet,
    HairAccessory,
    
    // Accessories
    Glasses,
    Gloves,
    Watch,
    Necklace,
    Earrings,
    Belt,
    Bag,
    
    // Count
    SlotCount
};

// Layer order (lower = closer to body)
inline int getSlotLayer(ClothingSlot slot) {
    switch (slot) {
        case ClothingSlot::Underwear:
        case ClothingSlot::Bra:
        case ClothingSlot::Socks:
            return 0;
        case ClothingSlot::Shirt:
        case ClothingSlot::Pants:
        case ClothingSlot::Skirt:
        case ClothingSlot::Dress:
            return 1;
        case ClothingSlot::Vest:
        case ClothingSlot::Shorts:
            return 2;
        case ClothingSlot::Jacket:
        case ClothingSlot::Suit:
        case ClothingSlot::Jumpsuit:
            return 3;
        default:
            return 4;
    }
}

// ============================================================================
// Clothing Asset
// ============================================================================

struct ClothingAsset {
    // Identity
    std::string id;
    std::string name;
    std::string description;
    ClothingCategory category;
    ClothingSlot slot;
    int layer = 0;
    
    // Compatibility
    std::vector<Gender> supportedGenders;
    bool supportsAllGenders = false;
    
    // Mesh data (for the neutral/base pose)
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SkinnedVertex> skinnedVertices;
    bool isSkinned = true;
    
    // BlendShapes for body adaptation
    // These adjust the clothing mesh to fit different body types
    struct AdaptationBlendShape {
        std::string parameterName;  // e.g., "body_weight", "chest_size"
        std::vector<BlendShapeDelta> deltas;
    };
    std::vector<AdaptationBlendShape> adaptationShapes;
    
    // Material
    struct Material {
        Vec3 baseColor{1.0f, 1.0f, 1.0f};
        float roughness = 0.5f;
        float metallic = 0.0f;
        std::string diffuseTexture;
        std::string normalTexture;
        std::string roughnessTexture;
    };
    Material material;
    
    // Color variants
    struct ColorVariant {
        std::string name;
        Vec3 color;
    };
    std::vector<ColorVariant> colorVariants;
    bool allowCustomColor = true;
    
    // Physics (for cloth simulation)
    bool hasPhysics = false;
    float mass = 1.0f;
    float stiffness = 0.5f;
    float damping = 0.1f;
    std::vector<uint32_t> pinnedVertices;  // Vertices attached to body
    
    // Slot conflicts (items that cannot be worn together)
    std::vector<ClothingSlot> conflictingSlots;
    
    // Preview
    std::string thumbnailPath;
};

// ============================================================================
// Equipped Item State
// ============================================================================

struct EquippedClothing {
    std::string assetId;
    Vec3 color;
    int colorVariantIndex = -1;  // -1 = custom color
    
    // Adapted mesh (after applying body shape)
    std::vector<Vertex> adaptedVertices;
    bool needsAdaptation = true;
};

// ============================================================================
// Clothing Library
// ============================================================================

class ClothingLibrary {
public:
    // Singleton
    static ClothingLibrary& getInstance() {
        static ClothingLibrary instance;
        return instance;
    }
    
    // === Asset Management ===
    
    void addAsset(const ClothingAsset& asset) {
        assets_[asset.id] = asset;
        categoryIndex_[asset.category].push_back(asset.id);
        slotIndex_[asset.slot].push_back(asset.id);
    }
    
    const ClothingAsset* getAsset(const std::string& id) const {
        auto it = assets_.find(id);
        return it != assets_.end() ? &it->second : nullptr;
    }
    
    std::vector<const ClothingAsset*> getAssetsByCategory(ClothingCategory cat) const {
        std::vector<const ClothingAsset*> result;
        auto it = categoryIndex_.find(cat);
        if (it != categoryIndex_.end()) {
            for (const auto& id : it->second) {
                auto assetIt = assets_.find(id);
                if (assetIt != assets_.end()) {
                    result.push_back(&assetIt->second);
                }
            }
        }
        return result;
    }
    
    std::vector<const ClothingAsset*> getAssetsBySlot(ClothingSlot slot) const {
        std::vector<const ClothingAsset*> result;
        auto it = slotIndex_.find(slot);
        if (it != slotIndex_.end()) {
            for (const auto& id : it->second) {
                auto assetIt = assets_.find(id);
                if (assetIt != assets_.end()) {
                    result.push_back(&assetIt->second);
                }
            }
        }
        return result;
    }
    
    // Get all assets compatible with a gender
    std::vector<const ClothingAsset*> getCompatibleAssets(Gender gender) const {
        std::vector<const ClothingAsset*> result;
        for (const auto& [id, asset] : assets_) {
            if (asset.supportsAllGenders) {
                result.push_back(&asset);
            } else {
                for (Gender g : asset.supportedGenders) {
                    if (g == gender) {
                        result.push_back(&asset);
                        break;
                    }
                }
            }
        }
        return result;
    }
    
    // === Loading ===
    
    // Load clothing asset from file
    bool loadAsset(const std::string& path);
    
    // Initialize with default/sample clothing
    void initializeDefaults();
    
private:
    ClothingLibrary() = default;
    
    std::unordered_map<std::string, ClothingAsset> assets_;
    std::unordered_map<ClothingCategory, std::vector<std::string>> categoryIndex_;
    std::unordered_map<ClothingSlot, std::vector<std::string>> slotIndex_;
};

// ============================================================================
// Clothing Manager (per character)
// ============================================================================

class ClothingManager {
public:
    ClothingManager() = default;
    
    // === Equip/Unequip ===
    
    bool equip(const std::string& assetId, const Vec3& color = Vec3(1,1,1)) {
        const ClothingAsset* asset = ClothingLibrary::getInstance().getAsset(assetId);
        if (!asset) return false;
        
        // Check slot conflicts
        for (ClothingSlot conflictSlot : asset->conflictingSlots) {
            unequipSlot(conflictSlot);
        }
        
        // Unequip existing item in same slot
        unequipSlot(asset->slot);
        
        // Equip new item
        EquippedClothing equipped;
        equipped.assetId = assetId;
        equipped.color = color;
        equipped.needsAdaptation = true;
        
        equippedItems_[asset->slot] = equipped;
        dirty_ = true;
        
        return true;
    }
    
    void unequip(const std::string& assetId) {
        for (auto it = equippedItems_.begin(); it != equippedItems_.end(); ) {
            if (it->second.assetId == assetId) {
                it = equippedItems_.erase(it);
                dirty_ = true;
            } else {
                ++it;
            }
        }
    }
    
    void unequipSlot(ClothingSlot slot) {
        auto it = equippedItems_.find(slot);
        if (it != equippedItems_.end()) {
            equippedItems_.erase(it);
            dirty_ = true;
        }
    }
    
    void unequipAll() {
        equippedItems_.clear();
        dirty_ = true;
    }
    
    // === Query ===
    
    bool isEquipped(const std::string& assetId) const {
        for (const auto& [slot, item] : equippedItems_) {
            if (item.assetId == assetId) return true;
        }
        return false;
    }
    
    const EquippedClothing* getEquippedInSlot(ClothingSlot slot) const {
        auto it = equippedItems_.find(slot);
        return it != equippedItems_.end() ? &it->second : nullptr;
    }
    
    std::vector<std::pair<ClothingSlot, const EquippedClothing*>> getAllEquipped() const {
        std::vector<std::pair<ClothingSlot, const EquippedClothing*>> result;
        for (const auto& [slot, item] : equippedItems_) {
            result.push_back({slot, &item});
        }
        // Sort by layer
        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return getSlotLayer(a.first) < getSlotLayer(b.first);
        });
        return result;
    }
    
    // === Customization ===
    
    void setColor(ClothingSlot slot, const Vec3& color) {
        auto it = equippedItems_.find(slot);
        if (it != equippedItems_.end()) {
            it->second.color = color;
            it->second.colorVariantIndex = -1;  // Custom
            dirty_ = true;
        }
    }
    
    void setColorVariant(ClothingSlot slot, int variantIndex) {
        auto it = equippedItems_.find(slot);
        if (it != equippedItems_.end()) {
            const ClothingAsset* asset = ClothingLibrary::getInstance().getAsset(it->second.assetId);
            if (asset && variantIndex >= 0 && variantIndex < (int)asset->colorVariants.size()) {
                it->second.color = asset->colorVariants[variantIndex].color;
                it->second.colorVariantIndex = variantIndex;
                dirty_ = true;
            }
        }
    }
    
    // === Body Adaptation ===
    
    // Adapt all clothing to current body shape
    void adaptToBody(const BodyMeasurements& bodyMeasurements) {
        for (auto& [slot, item] : equippedItems_) {
            if (item.needsAdaptation) {
                adaptClothing(item, bodyMeasurements);
            }
        }
    }
    
    // Force re-adaptation
    void markNeedsAdaptation() {
        for (auto& [slot, item] : equippedItems_) {
            item.needsAdaptation = true;
        }
        dirty_ = true;
    }
    
    // === State ===
    
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }
    
    // === Rendering ===
    
    // Get mesh for equipped item (after adaptation)
    const std::vector<Vertex>& getAdaptedMesh(ClothingSlot slot) const {
        static std::vector<Vertex> empty;
        auto it = equippedItems_.find(slot);
        return it != equippedItems_.end() ? it->second.adaptedVertices : empty;
    }
    
    // Get all clothing meshes for rendering
    std::vector<Mesh> getClothingMeshes() const {
        std::vector<Mesh> meshes;
        auto equipped = getAllEquipped();
        
        for (const auto& [slot, item] : equipped) {
            const ClothingAsset* asset = ClothingLibrary::getInstance().getAsset(item->assetId);
            if (!asset) continue;
            
            Mesh mesh;
            mesh.vertices = item->adaptedVertices.empty() ? asset->vertices : item->adaptedVertices;
            mesh.indices = asset->indices;
            mesh.baseColor[0] = item->color.x;
            mesh.baseColor[1] = item->color.y;
            mesh.baseColor[2] = item->color.z;
            mesh.roughness = asset->material.roughness;
            mesh.metallic = asset->material.metallic;
            
            meshes.push_back(mesh);
        }
        
        return meshes;
    }
    
    // === Physics Simulation ===
    
    // Enable cloth physics for a slot
    void enablePhysics(ClothingSlot slot, bool enable = true) {
        physicsEnabled_[slot] = enable;
    }
    
    bool isPhysicsEnabled(ClothingSlot slot) const {
        auto it = physicsEnabled_.find(slot);
        return it != physicsEnabled_.end() && it->second;
    }
    
    // Update cloth simulation (call each frame)
    void updatePhysics(float deltaTime) {
        // Physics simulation is handled by ClothSimulation class
        // This is called from the render loop
        physicsTime_ += deltaTime;
    }
    
    // === Skeletal Skinning ===
    
    // Enable skinning for a slot
    void enableSkinning(ClothingSlot slot, bool enable = true) {
        skinningEnabled_[slot] = enable;
    }
    
    bool isSkinningEnabled(ClothingSlot slot) const {
        auto it = skinningEnabled_.find(slot);
        return it != skinningEnabled_.end() && it->second;
    }
    
    // Update skinned clothing with bone matrices
    void updateSkinning(ClothingSlot slot, const std::vector<Mat4>& boneMatrices) {
        auto it = equippedItems_.find(slot);
        if (it == equippedItems_.end()) return;
        if (!isSkinningEnabled(slot)) return;
        
        // Skinning is handled by ClothingSkinningDeformer
        // Store bone matrices for this frame
        currentBoneMatrices_[slot] = boneMatrices;
        dirty_ = true;
    }
    
    // === Texture/Material ===
    
    // Set fabric type for clothing (regenerates textures)
    void setFabricType(ClothingSlot slot, int fabricTypeIndex) {
        fabricTypes_[slot] = fabricTypeIndex;
        dirty_ = true;
    }
    
    int getFabricType(ClothingSlot slot) const {
        auto it = fabricTypes_.find(slot);
        return it != fabricTypes_.end() ? it->second : 0;
    }
    
private:
    std::unordered_map<ClothingSlot, EquippedClothing> equippedItems_;
    bool dirty_ = false;
    
    // Physics state
    std::unordered_map<ClothingSlot, bool> physicsEnabled_;
    float physicsTime_ = 0.0f;
    
    // Skinning state
    std::unordered_map<ClothingSlot, bool> skinningEnabled_;
    std::unordered_map<ClothingSlot, std::vector<Mat4>> currentBoneMatrices_;
    
    // Material state
    std::unordered_map<ClothingSlot, int> fabricTypes_;
    
    void adaptClothing(EquippedClothing& item, const BodyMeasurements& body) {
        const ClothingAsset* asset = ClothingLibrary::getInstance().getAsset(item.assetId);
        if (!asset) return;
        
        // Start with base mesh
        item.adaptedVertices = asset->vertices;
        
        // Apply adaptation blend shapes based on body measurements
        for (const auto& shape : asset->adaptationShapes) {
            float weight = 0.0f;
            
            // Map body parameter to weight
            if (shape.parameterName == "body_weight") {
                weight = body.weight - 0.5f;  // Center around 0.5
            } else if (shape.parameterName == "body_height") {
                weight = body.height - 0.5f;
            } else if (shape.parameterName == "chest_size") {
                weight = body.chestSize - 0.5f;
            } else if (shape.parameterName == "waist_size") {
                weight = body.waistSize - 0.5f;
            } else if (shape.parameterName == "hip_width") {
                weight = body.hipWidth - 0.5f;
            } else if (shape.parameterName == "shoulder_width") {
                weight = body.shoulderWidth - 0.5f;
            } else if (shape.parameterName == "bust_size") {
                weight = body.bustSize - 0.5f;
            }
            
            // Apply deltas
            if (std::abs(weight) > 0.01f) {
                for (const auto& delta : shape.deltas) {
                    if (delta.vertexIndex < item.adaptedVertices.size()) {
                        item.adaptedVertices[delta.vertexIndex].position[0] += delta.positionDelta.x * weight;
                        item.adaptedVertices[delta.vertexIndex].position[1] += delta.positionDelta.y * weight;
                        item.adaptedVertices[delta.vertexIndex].position[2] += delta.positionDelta.z * weight;
                        
                        item.adaptedVertices[delta.vertexIndex].normal[0] += delta.normalDelta.x * weight;
                        item.adaptedVertices[delta.vertexIndex].normal[1] += delta.normalDelta.y * weight;
                        item.adaptedVertices[delta.vertexIndex].normal[2] += delta.normalDelta.z * weight;
                    }
                }
            }
        }
        
        // Normalize normals
        for (auto& v : item.adaptedVertices) {
            float len = std::sqrt(v.normal[0]*v.normal[0] + v.normal[1]*v.normal[1] + v.normal[2]*v.normal[2]);
            if (len > 0.001f) {
                v.normal[0] /= len;
                v.normal[1] /= len;
                v.normal[2] /= len;
            }
        }
        
        item.needsAdaptation = false;
    }
};

// ============================================================================
// Procedural Clothing Generator
// ============================================================================

class ProceduralClothingGenerator {
public:
    // Generate basic T-shirt
    static ClothingAsset generateTShirt(const std::string& id, const Vec3& color) {
        ClothingAsset asset;
        asset.id = id;
        asset.name = "T-Shirt";
        asset.description = "Basic T-Shirt";
        asset.category = ClothingCategory::Top;
        asset.slot = ClothingSlot::Shirt;
        asset.supportsAllGenders = true;
        asset.material.baseColor = color;
        asset.allowCustomColor = true;
        
        // Generate simple T-shirt mesh (simplified tube shape)
        generateTubeMesh(asset.vertices, asset.indices,
                        0.3f,   // Top radius
                        0.35f,  // Bottom radius
                        0.6f,   // Length
                        16,     // Segments
                        0.9f,   // Y offset (start height)
                        0.3f);  // Y end
        
        // Add sleeves
        addSleeveMesh(asset.vertices, asset.indices, true, 0.08f, 0.2f, 0.75f);
        addSleeveMesh(asset.vertices, asset.indices, false, 0.08f, 0.2f, 0.75f);
        
        // Color variants
        asset.colorVariants = {
            {"White", Vec3(0.95f, 0.95f, 0.95f)},
            {"Black", Vec3(0.1f, 0.1f, 0.1f)},
            {"Navy", Vec3(0.1f, 0.15f, 0.3f)},
            {"Red", Vec3(0.8f, 0.15f, 0.15f)},
            {"Gray", Vec3(0.5f, 0.5f, 0.5f)}
        };
        
        // Adaptation shapes
        addBodyAdaptationShapes(asset);
        
        return asset;
    }
    
    // Generate basic pants
    static ClothingAsset generatePants(const std::string& id, const Vec3& color) {
        ClothingAsset asset;
        asset.id = id;
        asset.name = "Pants";
        asset.description = "Basic Pants";
        asset.category = ClothingCategory::Bottom;
        asset.slot = ClothingSlot::Pants;
        asset.supportsAllGenders = true;
        asset.material.baseColor = color;
        asset.allowCustomColor = true;
        
        // Generate pants mesh (two leg tubes + waist)
        // Waist section
        generateTubeMesh(asset.vertices, asset.indices,
                        0.25f,  // Top radius (waist)
                        0.22f,  // Bottom radius
                        0.15f,  // Length
                        16,     // Segments
                        0.5f,   // Y start
                        0.35f); // Y end
        
        // Left leg
        generateTubeMesh(asset.vertices, asset.indices,
                        0.12f,  // Top radius
                        0.09f,  // Bottom radius
                        0.5f,   // Length
                        12,     // Segments
                        0.35f,  // Y start
                        -0.15f, // Y end
                        -0.08f);// X offset
        
        // Right leg
        generateTubeMesh(asset.vertices, asset.indices,
                        0.12f,  // Top radius
                        0.09f,  // Bottom radius
                        0.5f,   // Length
                        12,     // Segments
                        0.35f,  // Y start
                        -0.15f, // Y end
                        0.08f); // X offset
        
        asset.colorVariants = {
            {"Blue Jeans", Vec3(0.2f, 0.3f, 0.5f)},
            {"Black", Vec3(0.1f, 0.1f, 0.1f)},
            {"Khaki", Vec3(0.76f, 0.69f, 0.57f)},
            {"Gray", Vec3(0.4f, 0.4f, 0.4f)}
        };
        
        addBodyAdaptationShapes(asset);
        
        return asset;
    }
    
    // Generate skirt
    static ClothingAsset generateSkirt(const std::string& id, const Vec3& color) {
        ClothingAsset asset;
        asset.id = id;
        asset.name = "Skirt";
        asset.description = "Basic Skirt";
        asset.category = ClothingCategory::Bottom;
        asset.slot = ClothingSlot::Skirt;
        asset.supportedGenders = {Gender::Female};
        asset.material.baseColor = color;
        asset.allowCustomColor = true;
        
        // Cone-like shape for skirt
        generateConeMesh(asset.vertices, asset.indices,
                        0.2f,   // Top radius (waist)
                        0.35f,  // Bottom radius
                        0.35f,  // Length
                        24,     // Segments
                        0.5f,   // Y start
                        0.15f); // Y end
        
        asset.colorVariants = {
            {"Black", Vec3(0.1f, 0.1f, 0.1f)},
            {"Navy", Vec3(0.1f, 0.15f, 0.3f)},
            {"Red", Vec3(0.7f, 0.15f, 0.15f)},
            {"Pink", Vec3(0.9f, 0.6f, 0.7f)}
        };
        
        addBodyAdaptationShapes(asset);
        
        return asset;
    }
    
    // Generate shoes
    static ClothingAsset generateShoes(const std::string& id, const Vec3& color) {
        ClothingAsset asset;
        asset.id = id;
        asset.name = "Shoes";
        asset.description = "Basic Shoes";
        asset.category = ClothingCategory::Footwear;
        asset.slot = ClothingSlot::Shoes;
        asset.supportsAllGenders = true;
        asset.material.baseColor = color;
        asset.material.roughness = 0.4f;
        asset.allowCustomColor = true;
        
        // Simple shoe shapes
        generateShoeMesh(asset.vertices, asset.indices, -0.08f);  // Left
        generateShoeMesh(asset.vertices, asset.indices, 0.08f);   // Right
        
        asset.colorVariants = {
            {"Black", Vec3(0.1f, 0.1f, 0.1f)},
            {"Brown", Vec3(0.4f, 0.25f, 0.15f)},
            {"White", Vec3(0.9f, 0.9f, 0.9f)}
        };
        
        return asset;
    }
    
private:
    static void generateTubeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                float topRadius, float bottomRadius, float height,
                                int segments, float yStart, float yEnd, float xOffset = 0.0f) {
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
        
        // Generate vertices
        for (int ring = 0; ring <= 1; ring++) {
            float t = static_cast<float>(ring);
            float radius = topRadius + (bottomRadius - topRadius) * t;
            float y = yStart + (yEnd - yStart) * t;
            
            for (int seg = 0; seg < segments; seg++) {
                float angle = (2.0f * 3.14159f * seg) / segments;
                float x = std::cos(angle) * radius + xOffset;
                float z = std::sin(angle) * radius;
                
                Vertex v;
                v.position[0] = x;
                v.position[1] = y;
                v.position[2] = z;
                
                // Normal pointing outward
                v.normal[0] = std::cos(angle);
                v.normal[1] = 0.0f;
                v.normal[2] = std::sin(angle);
                
                // UV
                v.uv[0] = static_cast<float>(seg) / segments;
                v.uv[1] = t;
                
                vertices.push_back(v);
            }
        }
        
        // Generate indices
        for (int seg = 0; seg < segments; seg++) {
            int next = (seg + 1) % segments;
            
            uint32_t i0 = baseIndex + seg;
            uint32_t i1 = baseIndex + next;
            uint32_t i2 = baseIndex + segments + seg;
            uint32_t i3 = baseIndex + segments + next;
            
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    
    static void generateConeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                float topRadius, float bottomRadius, float height,
                                int segments, float yStart, float yEnd) {
        // Similar to tube but with more variation in radius
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
        
        int rings = 4;  // More rings for smoother shape
        for (int ring = 0; ring <= rings; ring++) {
            float t = static_cast<float>(ring) / rings;
            // Curve the radius for a more natural skirt shape
            float radius = topRadius + (bottomRadius - topRadius) * std::pow(t, 0.7f);
            float y = yStart + (yEnd - yStart) * t;
            
            for (int seg = 0; seg < segments; seg++) {
                float angle = (2.0f * 3.14159f * seg) / segments;
                float x = std::cos(angle) * radius;
                float z = std::sin(angle) * radius;
                
                Vertex v;
                v.position[0] = x;
                v.position[1] = y;
                v.position[2] = z;
                
                v.normal[0] = std::cos(angle) * 0.5f;
                v.normal[1] = 0.5f;
                v.normal[2] = std::sin(angle) * 0.5f;
                
                v.uv[0] = static_cast<float>(seg) / segments;
                v.uv[1] = t;
                
                vertices.push_back(v);
            }
        }
        
        // Generate indices for all rings
        for (int ring = 0; ring < rings; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                int next = (seg + 1) % segments;
                
                uint32_t i0 = baseIndex + ring * segments + seg;
                uint32_t i1 = baseIndex + ring * segments + next;
                uint32_t i2 = baseIndex + (ring + 1) * segments + seg;
                uint32_t i3 = baseIndex + (ring + 1) * segments + next;
                
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);
                
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
    }
    
    static void addSleeveMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                             bool left, float radius, float length, float yPos) {
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
        float xSign = left ? -1.0f : 1.0f;
        float xStart = xSign * 0.28f;  // Shoulder position
        
        int segments = 8;
        int rings = 2;
        
        for (int ring = 0; ring <= rings; ring++) {
            float t = static_cast<float>(ring) / rings;
            float r = radius * (1.0f - t * 0.2f);
            float x = xStart + xSign * length * t;
            float y = yPos - t * 0.05f;  // Slight downward slope
            
            for (int seg = 0; seg < segments; seg++) {
                float angle = (2.0f * 3.14159f * seg) / segments;
                
                Vertex v;
                v.position[0] = x;
                v.position[1] = y + std::cos(angle) * r;
                v.position[2] = std::sin(angle) * r;
                
                v.normal[0] = xSign * 0.3f;
                v.normal[1] = std::cos(angle) * 0.7f;
                v.normal[2] = std::sin(angle) * 0.7f;
                
                v.uv[0] = static_cast<float>(seg) / segments;
                v.uv[1] = t;
                
                vertices.push_back(v);
            }
        }
        
        for (int ring = 0; ring < rings; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                int next = (seg + 1) % segments;
                
                uint32_t i0 = baseIndex + ring * segments + seg;
                uint32_t i1 = baseIndex + ring * segments + next;
                uint32_t i2 = baseIndex + (ring + 1) * segments + seg;
                uint32_t i3 = baseIndex + (ring + 1) * segments + next;
                
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);
                
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
    }
    
    static void generateShoeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                                float xOffset) {
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
        
        // Simple box-like shoe
        float w = 0.05f, h = 0.04f, l = 0.12f;
        float y = -0.2f;  // At foot level
        float z = 0.02f;  // Forward
        
        Vertex verts[8];
        // Bottom
        verts[0] = {{xOffset - w, y, z - l}, {0, -1, 0}, {0, 0}};
        verts[1] = {{xOffset + w, y, z - l}, {0, -1, 0}, {1, 0}};
        verts[2] = {{xOffset + w, y, z + l}, {0, -1, 0}, {1, 1}};
        verts[3] = {{xOffset - w, y, z + l}, {0, -1, 0}, {0, 1}};
        // Top
        verts[4] = {{xOffset - w, y + h, z - l}, {0, 1, 0}, {0, 0}};
        verts[5] = {{xOffset + w, y + h, z - l}, {0, 1, 0}, {1, 0}};
        verts[6] = {{xOffset + w, y + h, z + l * 0.5f}, {0, 1, 0}, {1, 0.75f}};
        verts[7] = {{xOffset - w, y + h, z + l * 0.5f}, {0, 1, 0}, {0, 0.75f}};
        
        for (int i = 0; i < 8; i++) {
            vertices.push_back(verts[i]);
        }
        
        // Indices for a simple box (6 faces, 12 triangles)
        uint32_t boxIndices[] = {
            0, 1, 2, 0, 2, 3,  // Bottom
            4, 6, 5, 4, 7, 6,  // Top
            0, 4, 5, 0, 5, 1,  // Back
            2, 6, 7, 2, 7, 3,  // Front
            0, 3, 7, 0, 7, 4,  // Left
            1, 5, 6, 1, 6, 2   // Right
        };
        
        for (uint32_t idx : boxIndices) {
            indices.push_back(baseIndex + idx);
        }
    }
    
    static void addBodyAdaptationShapes(ClothingAsset& asset) {
        // Add basic adaptation shapes for body size changes
        // These would be more sophisticated in a real implementation
        
        ClothingAsset::AdaptationBlendShape weightShape;
        weightShape.parameterName = "body_weight";
        // Add deltas for all vertices (simplified: just scale)
        for (size_t i = 0; i < asset.vertices.size(); i++) {
            BlendShapeDelta delta;
            delta.vertexIndex = static_cast<uint32_t>(i);
            float x = asset.vertices[i].position[0];
            float z = asset.vertices[i].position[2];
            delta.positionDelta = Vec3(x * 0.15f, 0, z * 0.15f);
            delta.normalDelta = Vec3(0, 0, 0);
            weightShape.deltas.push_back(delta);
        }
        asset.adaptationShapes.push_back(weightShape);
        
        ClothingAsset::AdaptationBlendShape chestShape;
        chestShape.parameterName = "chest_size";
        for (size_t i = 0; i < asset.vertices.size(); i++) {
            if (asset.vertices[i].position[1] > 0.6f) {  // Chest area
                BlendShapeDelta delta;
                delta.vertexIndex = static_cast<uint32_t>(i);
                float x = asset.vertices[i].position[0];
                float z = asset.vertices[i].position[2];
                delta.positionDelta = Vec3(x * 0.1f, 0.02f, z * 0.15f);
                delta.normalDelta = Vec3(0, 0, 0);
                chestShape.deltas.push_back(delta);
            }
        }
        asset.adaptationShapes.push_back(chestShape);
    }
};

// ============================================================================
// Initialize Default Clothing Library
// ============================================================================

inline void ClothingLibrary::initializeDefaults() {
    // T-Shirts
    addAsset(ProceduralClothingGenerator::generateTShirt("tshirt_white", Vec3(0.95f, 0.95f, 0.95f)));
    addAsset(ProceduralClothingGenerator::generateTShirt("tshirt_black", Vec3(0.1f, 0.1f, 0.1f)));
    addAsset(ProceduralClothingGenerator::generateTShirt("tshirt_red", Vec3(0.8f, 0.15f, 0.15f)));
    addAsset(ProceduralClothingGenerator::generateTShirt("tshirt_blue", Vec3(0.2f, 0.3f, 0.7f)));
    
    // Pants
    addAsset(ProceduralClothingGenerator::generatePants("pants_jeans", Vec3(0.2f, 0.3f, 0.5f)));
    addAsset(ProceduralClothingGenerator::generatePants("pants_black", Vec3(0.1f, 0.1f, 0.1f)));
    addAsset(ProceduralClothingGenerator::generatePants("pants_khaki", Vec3(0.76f, 0.69f, 0.57f)));
    
    // Skirts
    addAsset(ProceduralClothingGenerator::generateSkirt("skirt_black", Vec3(0.1f, 0.1f, 0.1f)));
    addAsset(ProceduralClothingGenerator::generateSkirt("skirt_red", Vec3(0.7f, 0.15f, 0.15f)));
    
    // Shoes
    addAsset(ProceduralClothingGenerator::generateShoes("shoes_black", Vec3(0.1f, 0.1f, 0.1f)));
    addAsset(ProceduralClothingGenerator::generateShoes("shoes_brown", Vec3(0.4f, 0.25f, 0.15f)));
}

inline bool ClothingLibrary::loadAsset(const std::string& path) {
    // TODO: Implement loading from file
    // Would support OBJ, glTF, or custom format
    return false;
}

} // namespace luma
