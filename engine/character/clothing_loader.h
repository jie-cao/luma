// LUMA Clothing Loader
// Loads clothing assets from external 3D model files (OBJ, glTF, FBX)
#pragma once

#include "engine/character/clothing_system.h"
#include "engine/asset/model_loader.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <optional>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace luma {

// ============================================================================
// Clothing Metadata (loaded from sidecar JSON file)
// ============================================================================

struct ClothingMetadata {
    std::string id;
    std::string name;
    std::string description;
    ClothingCategory category = ClothingCategory::Top;
    ClothingSlot slot = ClothingSlot::Shirt;
    
    // Gender compatibility
    bool supportsAllGenders = true;
    std::vector<Gender> supportedGenders;
    
    // Material overrides
    float roughness = 0.5f;
    float metallic = 0.0f;
    
    // Color variants
    std::vector<std::pair<std::string, Vec3>> colorVariants;
    bool allowCustomColor = true;
    
    // Physics settings
    bool hasPhysics = false;
    float mass = 1.0f;
    float stiffness = 0.5f;
    float damping = 0.1f;
    
    // Adaptation parameters
    std::vector<std::string> adaptationBones;  // Bones that affect this clothing
    
    // Slot conflicts
    std::vector<ClothingSlot> conflictingSlots;
    
    // Parse from JSON-like simple format
    static ClothingMetadata parse(const std::string& content);
};

// ============================================================================
// Clothing Loader
// ============================================================================

class ClothingLoader {
public:
    // Load clothing asset from model file
    // Looks for sidecar .meta file for metadata
    static std::optional<ClothingAsset> loadFromFile(const std::string& modelPath) {
        // Load the 3D model
        auto model = load_model(modelPath);
        if (!model || model->meshes.empty()) {
            return std::nullopt;
        }
        
        ClothingAsset asset;
        
        // Try to load metadata
        std::string metaPath = modelPath + ".meta";
        ClothingMetadata meta = loadMetadata(metaPath);
        
        // Use filename as ID if not in metadata
        std::filesystem::path p(modelPath);
        if (meta.id.empty()) {
            meta.id = p.stem().string();
        }
        if (meta.name.empty()) {
            meta.name = meta.id;
        }
        
        // Fill asset from metadata
        asset.id = meta.id;
        asset.name = meta.name;
        asset.description = meta.description;
        asset.category = meta.category;
        asset.slot = meta.slot;
        asset.supportsAllGenders = meta.supportsAllGenders;
        asset.supportedGenders = meta.supportedGenders;
        asset.allowCustomColor = meta.allowCustomColor;
        asset.hasPhysics = meta.hasPhysics;
        asset.mass = meta.mass;
        asset.stiffness = meta.stiffness;
        asset.damping = meta.damping;
        asset.conflictingSlots = meta.conflictingSlots;
        
        // Material
        asset.material.roughness = meta.roughness;
        asset.material.metallic = meta.metallic;
        
        // Color variants
        for (const auto& [name, color] : meta.colorVariants) {
            asset.colorVariants.push_back({name, color});
        }
        
        // Merge all meshes from the model
        for (const auto& mesh : model->meshes) {
            // Append vertices
            size_t baseIdx = asset.vertices.size();
            for (const auto& v : mesh.vertices) {
                asset.vertices.push_back(v);
            }
            
            // Append indices with offset
            for (uint32_t idx : mesh.indices) {
                asset.indices.push_back(static_cast<uint32_t>(baseIdx) + idx);
            }
            
            // Copy material from first mesh with valid base color
            if (mesh.baseColor[0] > 0.01f || mesh.baseColor[1] > 0.01f || mesh.baseColor[2] > 0.01f) {
                asset.material.baseColor = Vec3(mesh.baseColor[0], mesh.baseColor[1], mesh.baseColor[2]);
            }
            
            // Copy texture paths
            if (!mesh.diffuseTexture.path.empty() && asset.material.diffuseTexture.empty()) {
                asset.material.diffuseTexture = mesh.diffuseTexture.path;
            }
            if (!mesh.normalTexture.path.empty() && asset.material.normalTexture.empty()) {
                asset.material.normalTexture = mesh.normalTexture.path;
            }
        }
        
        // Handle skinned vertices if model has skeleton
        if (model->hasSkeleton()) {
            asset.isSkinned = true;
            
            // Copy skinned vertex data
            for (const auto& mesh : model->meshes) {
                for (const auto& sv : mesh.skinnedVertices) {
                    asset.skinnedVertices.push_back(sv);
                }
            }
        }
        
        // Generate default adaptation shapes if not skinned
        if (!asset.isSkinned && asset.vertices.size() > 0) {
            generateDefaultAdaptationShapes(asset);
        }
        
        return asset;
    }
    
    // Load clothing from OBJ file directly (simpler format)
    static std::optional<ClothingAsset> loadFromOBJ(const std::string& objPath) {
        return loadFromFile(objPath);  // Assimp handles OBJ
    }
    
    // Load clothing from glTF file
    static std::optional<ClothingAsset> loadFromGLTF(const std::string& gltfPath) {
        return loadFromFile(gltfPath);  // Assimp handles glTF
    }
    
    // Load clothing from FBX file
    static std::optional<ClothingAsset> loadFromFBX(const std::string& fbxPath) {
        return loadFromFile(fbxPath);  // Assimp handles FBX
    }
    
    // Load and register clothing to library
    static bool loadAndRegister(const std::string& modelPath) {
        auto asset = loadFromFile(modelPath);
        if (asset) {
            ClothingLibrary::getInstance().addAsset(*asset);
            return true;
        }
        return false;
    }
    
    // Load all clothing from a directory
    static int loadDirectory(const std::string& directoryPath) {
        int loaded = 0;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
                if (!entry.is_regular_file()) continue;
                
                std::string ext = entry.path().extension().string();
                // Convert to lowercase
                for (char& c : ext) c = std::tolower(c);
                
                // Check if it's a supported model format
                if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || 
                    ext == ".fbx" || ext == ".dae") {
                    if (loadAndRegister(entry.path().string())) {
                        loaded++;
                    }
                }
            }
        } catch (const std::exception&) {
            // Directory doesn't exist or access error
        }
        
        return loaded;
    }
    
    // Get supported file extensions
    static std::vector<std::string> getSupportedExtensions() {
        return {".obj", ".gltf", ".glb", ".fbx", ".dae", ".3ds"};
    }
    
private:
    static ClothingMetadata loadMetadata(const std::string& metaPath) {
        ClothingMetadata meta;
        
        std::ifstream file(metaPath);
        if (!file.is_open()) {
            return meta;  // Return defaults
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Simple key=value format
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // Trim whitespace
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
            
            if (key == "id") meta.id = value;
            else if (key == "name") meta.name = value;
            else if (key == "description") meta.description = value;
            else if (key == "category") meta.category = parseCategory(value);
            else if (key == "slot") meta.slot = parseSlot(value);
            else if (key == "roughness") meta.roughness = std::stof(value);
            else if (key == "metallic") meta.metallic = std::stof(value);
            else if (key == "physics") meta.hasPhysics = (value == "true" || value == "1");
            else if (key == "mass") meta.mass = std::stof(value);
            else if (key == "stiffness") meta.stiffness = std::stof(value);
            else if (key == "damping") meta.damping = std::stof(value);
            else if (key == "all_genders") meta.supportsAllGenders = (value == "true" || value == "1");
        }
        
        return meta;
    }
    
    static ClothingCategory parseCategory(const std::string& s) {
        if (s == "top") return ClothingCategory::Top;
        if (s == "bottom") return ClothingCategory::Bottom;
        if (s == "fullbody") return ClothingCategory::FullBody;
        if (s == "footwear") return ClothingCategory::Footwear;
        if (s == "headwear") return ClothingCategory::Headwear;
        if (s == "eyewear") return ClothingCategory::Eyewear;
        if (s == "handwear") return ClothingCategory::Handwear;
        if (s == "accessory") return ClothingCategory::Accessory;
        if (s == "underwear") return ClothingCategory::Underwear;
        if (s == "outerwear") return ClothingCategory::Outerwear;
        return ClothingCategory::Top;
    }
    
    static ClothingSlot parseSlot(const std::string& s) {
        if (s == "shirt") return ClothingSlot::Shirt;
        if (s == "jacket") return ClothingSlot::Jacket;
        if (s == "vest") return ClothingSlot::Vest;
        if (s == "pants") return ClothingSlot::Pants;
        if (s == "shorts") return ClothingSlot::Shorts;
        if (s == "skirt") return ClothingSlot::Skirt;
        if (s == "dress") return ClothingSlot::Dress;
        if (s == "shoes") return ClothingSlot::Shoes;
        if (s == "boots") return ClothingSlot::Boots;
        if (s == "hat") return ClothingSlot::Hat;
        if (s == "glasses") return ClothingSlot::Glasses;
        if (s == "gloves") return ClothingSlot::Gloves;
        return ClothingSlot::Shirt;
    }
    
    static void generateDefaultAdaptationShapes(ClothingAsset& asset) {
        // Analyze mesh to determine which adaptation shapes to create
        float minY = 1000.0f, maxY = -1000.0f;
        for (const auto& v : asset.vertices) {
            minY = std::min(minY, v.position[1]);
            maxY = std::max(maxY, v.position[1]);
        }
        
        // Weight adaptation (scales X and Z)
        {
            ClothingAsset::AdaptationBlendShape shape;
            shape.parameterName = "body_weight";
            
            for (size_t i = 0; i < asset.vertices.size(); i++) {
                BlendShapeDelta delta;
                delta.vertexIndex = static_cast<uint32_t>(i);
                float x = asset.vertices[i].position[0];
                float z = asset.vertices[i].position[2];
                delta.positionDelta = Vec3(x * 0.15f, 0, z * 0.15f);
                delta.normalDelta = Vec3(0, 0, 0);
                shape.deltas.push_back(delta);
            }
            
            asset.adaptationShapes.push_back(shape);
        }
        
        // Height adaptation (scales Y)
        {
            ClothingAsset::AdaptationBlendShape shape;
            shape.parameterName = "body_height";
            
            for (size_t i = 0; i < asset.vertices.size(); i++) {
                BlendShapeDelta delta;
                delta.vertexIndex = static_cast<uint32_t>(i);
                float y = asset.vertices[i].position[1];
                float normalizedY = (y - minY) / (maxY - minY + 0.001f);
                delta.positionDelta = Vec3(0, normalizedY * 0.1f, 0);
                delta.normalDelta = Vec3(0, 0, 0);
                shape.deltas.push_back(delta);
            }
            
            asset.adaptationShapes.push_back(shape);
        }
        
        // Chest adaptation (for upper body clothing)
        if (asset.category == ClothingCategory::Top || asset.category == ClothingCategory::FullBody) {
            ClothingAsset::AdaptationBlendShape shape;
            shape.parameterName = "chest_size";
            
            float midY = (minY + maxY) * 0.5f;
            float chestY = midY + (maxY - minY) * 0.2f;
            
            for (size_t i = 0; i < asset.vertices.size(); i++) {
                float y = asset.vertices[i].position[1];
                // Only affect chest area
                float chestInfluence = std::max(0.0f, 1.0f - std::abs(y - chestY) / 0.2f);
                
                if (chestInfluence > 0.01f) {
                    BlendShapeDelta delta;
                    delta.vertexIndex = static_cast<uint32_t>(i);
                    float x = asset.vertices[i].position[0];
                    float z = asset.vertices[i].position[2];
                    delta.positionDelta = Vec3(x * 0.1f * chestInfluence, 
                                               0.02f * chestInfluence, 
                                               z * 0.15f * chestInfluence);
                    delta.normalDelta = Vec3(0, 0, 0);
                    shape.deltas.push_back(delta);
                }
            }
            
            asset.adaptationShapes.push_back(shape);
        }
        
        // Hip adaptation (for lower body clothing)
        if (asset.category == ClothingCategory::Bottom || asset.category == ClothingCategory::FullBody) {
            ClothingAsset::AdaptationBlendShape shape;
            shape.parameterName = "hip_width";
            
            float midY = (minY + maxY) * 0.5f;
            float hipY = midY - (maxY - minY) * 0.1f;
            
            for (size_t i = 0; i < asset.vertices.size(); i++) {
                float y = asset.vertices[i].position[1];
                float hipInfluence = std::max(0.0f, 1.0f - std::abs(y - hipY) / 0.15f);
                
                if (hipInfluence > 0.01f) {
                    BlendShapeDelta delta;
                    delta.vertexIndex = static_cast<uint32_t>(i);
                    float x = asset.vertices[i].position[0];
                    delta.positionDelta = Vec3(x * 0.12f * hipInfluence, 0, 0);
                    delta.normalDelta = Vec3(0, 0, 0);
                    shape.deltas.push_back(delta);
                }
            }
            
            asset.adaptationShapes.push_back(shape);
        }
    }
};

// ============================================================================
// Update ClothingLibrary::loadAsset implementation
// ============================================================================

inline bool ClothingLibrary::loadAsset(const std::string& path) {
    return ClothingLoader::loadAndRegister(path);
}

}  // namespace luma
