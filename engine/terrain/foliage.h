// Foliage System - Grass, plants, trees with GPU instancing
// Density maps, LOD, wind animation
#pragma once

#include "terrain.h"
#include <random>
#include <algorithm>

namespace luma {

// ===== Foliage Instance =====
struct FoliageInstance {
    Vec3 position;
    float rotation;     // Y-axis rotation in radians
    float scale;
    Vec4 color;         // Tint variation
    float windPhase;    // For wind animation offset
};

// ===== Foliage Type =====
enum class FoliageType {
    Grass,
    Bush,
    Tree,
    Rock,
    Flower
};

// ===== Foliage Layer Settings =====
struct FoliageLayerSettings {
    std::string name = "Grass";
    FoliageType type = FoliageType::Grass;
    
    // Density
    float density = 10.0f;          // Instances per square unit
    float densityVariation = 0.3f;  // Random density variation
    
    // Transform
    float minScale = 0.8f;
    float maxScale = 1.2f;
    float minRotation = 0.0f;       // Radians
    float maxRotation = 6.28318f;   // Full rotation
    
    // Color variation
    Vec3 baseColor = {0.3f, 0.5f, 0.2f};
    Vec3 colorVariation = {0.1f, 0.1f, 0.05f};
    
    // Placement constraints
    float minHeight = 0.0f;         // Normalized terrain height
    float maxHeight = 0.7f;
    float minSlope = 0.0f;          // 0 = flat, 1 = vertical
    float maxSlope = 0.3f;
    int terrainLayer = 0;           // Which splatmap layer (e.g., grass layer)
    float layerThreshold = 0.5f;    // Minimum weight for placement
    
    // LOD
    float lodDistance[3] = {30.0f, 60.0f, 100.0f};
    float cullDistance = 150.0f;
    
    // Wind
    float windStrength = 1.0f;
    float windFrequency = 1.0f;
    
    // Mesh
    std::string meshPath;           // Path to mesh file (for trees/rocks)
    bool billboard = true;          // Use billboard for grass
    float width = 0.1f;             // For billboards
    float height = 0.3f;            // For billboards
};

// ===== Foliage Patch (chunk of instances) =====
class FoliagePatch {
public:
    FoliagePatch(int patchX, int patchZ, float patchSize)
        : patchX_(patchX), patchZ_(patchZ), patchSize_(patchSize),
          lodLevel_(0), visible_(true) {}
    
    int getPatchX() const { return patchX_; }
    int getPatchZ() const { return patchZ_; }
    float getPatchSize() const { return patchSize_; }
    
    void setLODLevel(int lod) { lodLevel_ = lod; }
    int getLODLevel() const { return lodLevel_; }
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    
    std::vector<FoliageInstance>& getInstances() { return instances_; }
    const std::vector<FoliageInstance>& getInstances() const { return instances_; }
    
    size_t getInstanceCount() const { return instances_.size(); }
    
    // GPU buffer handles
    uint32_t gpuBufferIndex = 0;
    bool gpuBufferValid = false;
    
private:
    int patchX_;
    int patchZ_;
    float patchSize_;
    int lodLevel_;
    bool visible_;
    std::vector<FoliageInstance> instances_;
};

// ===== Foliage Layer =====
class FoliageLayer {
public:
    FoliageLayer() = default;
    
    void initialize(const FoliageLayerSettings& settings, float terrainSize, int patchesPerSide) {
        settings_ = settings;
        float patchSize = terrainSize / patchesPerSide;
        
        patches_.clear();
        for (int z = 0; z < patchesPerSide; z++) {
            for (int x = 0; x < patchesPerSide; x++) {
                patches_.push_back(std::make_unique<FoliagePatch>(x, z, patchSize));
            }
        }
        
        patchesPerSide_ = patchesPerSide;
        terrainSize_ = terrainSize;
    }
    
    const FoliageLayerSettings& getSettings() const { return settings_; }
    FoliageLayerSettings& getSettings() { return settings_; }
    
    // Generate instances based on terrain
    void generateInstances(const Terrain& terrain, uint32_t seed = 0) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> distUnit(0.0f, 1.0f);
        
        float halfSize = terrainSize_ * 0.5f;
        float patchSize = terrainSize_ / patchesPerSide_;
        
        const Heightmap& heightmap = terrain.getHeightmap();
        const Splatmap& splatmap = terrain.getSplatmap();
        
        for (auto& patch : patches_) {
            patch->getInstances().clear();
            
            float patchMinX = patch->getPatchX() * patchSize - halfSize;
            float patchMinZ = patch->getPatchZ() * patchSize - halfSize;
            
            // Calculate number of potential instances in patch
            float area = patchSize * patchSize;
            int potentialInstances = (int)(area * settings_.density);
            
            for (int i = 0; i < potentialInstances; i++) {
                // Random position within patch
                float localX = distUnit(rng) * patchSize;
                float localZ = distUnit(rng) * patchSize;
                float worldX = patchMinX + localX;
                float worldZ = patchMinZ + localZ;
                
                // Get terrain info at this position
                float height = terrain.getHeightAt(worldX, worldZ);
                float normalizedHeight = height / terrain.getSettings().heightScale;
                Vec3 normal = terrain.getNormalAt(worldX, worldZ);
                float slope = 1.0f - normal.y;
                
                // Check placement constraints
                if (normalizedHeight < settings_.minHeight || normalizedHeight > settings_.maxHeight) continue;
                if (slope < settings_.minSlope || slope > settings_.maxSlope) continue;
                
                // Check splatmap layer
                float u = (worldX + halfSize) / terrainSize_;
                float v = (worldZ + halfSize) / terrainSize_;
                int splatX = (int)(u * (splatmap.getWidth() - 1));
                int splatY = (int)(v * (splatmap.getHeight() - 1));
                float layerWeight = splatmap.getWeight(settings_.terrainLayer, splatX, splatY);
                if (layerWeight < settings_.layerThreshold) continue;
                
                // Density variation
                float densityRoll = distUnit(rng);
                if (densityRoll > 1.0f - settings_.densityVariation) continue;
                
                // Create instance
                FoliageInstance instance;
                instance.position = Vec3(worldX, height, worldZ);
                instance.rotation = settings_.minRotation + distUnit(rng) * (settings_.maxRotation - settings_.minRotation);
                instance.scale = settings_.minScale + distUnit(rng) * (settings_.maxScale - settings_.minScale);
                
                // Color variation
                instance.color = Vec4(
                    settings_.baseColor.x + (distUnit(rng) - 0.5f) * 2.0f * settings_.colorVariation.x,
                    settings_.baseColor.y + (distUnit(rng) - 0.5f) * 2.0f * settings_.colorVariation.y,
                    settings_.baseColor.z + (distUnit(rng) - 0.5f) * 2.0f * settings_.colorVariation.z,
                    1.0f
                );
                
                instance.windPhase = distUnit(rng) * 6.28318f;
                
                patch->getInstances().push_back(instance);
            }
            
            patch->gpuBufferValid = false;
        }
        
        // Count total instances
        totalInstances_ = 0;
        for (const auto& patch : patches_) {
            totalInstances_ += patch->getInstanceCount();
        }
    }
    
    // Update visibility and LOD based on camera
    void updateLOD(const Vec3& cameraPos) {
        float halfSize = terrainSize_ * 0.5f;
        float patchSize = terrainSize_ / patchesPerSide_;
        
        for (auto& patch : patches_) {
            float patchCenterX = patch->getPatchX() * patchSize - halfSize + patchSize * 0.5f;
            float patchCenterZ = patch->getPatchZ() * patchSize - halfSize + patchSize * 0.5f;
            
            float dist = std::sqrt(
                (cameraPos.x - patchCenterX) * (cameraPos.x - patchCenterX) +
                (cameraPos.z - patchCenterZ) * (cameraPos.z - patchCenterZ)
            );
            
            // Cull far patches
            if (dist > settings_.cullDistance) {
                patch->setVisible(false);
                continue;
            }
            
            patch->setVisible(true);
            
            // Determine LOD level
            int lod = 2;
            if (dist < settings_.lodDistance[0]) lod = 0;
            else if (dist < settings_.lodDistance[1]) lod = 1;
            
            patch->setLODLevel(lod);
        }
    }
    
    const std::vector<std::unique_ptr<FoliagePatch>>& getPatches() const { return patches_; }
    std::vector<std::unique_ptr<FoliagePatch>>& getPatches() { return patches_; }
    
    size_t getTotalInstances() const { return totalInstances_; }
    
    // Get visible instance count
    size_t getVisibleInstances() const {
        size_t count = 0;
        for (const auto& patch : patches_) {
            if (patch->isVisible()) {
                count += patch->getInstanceCount();
            }
        }
        return count;
    }
    
private:
    FoliageLayerSettings settings_;
    std::vector<std::unique_ptr<FoliagePatch>> patches_;
    int patchesPerSide_ = 0;
    float terrainSize_ = 0.0f;
    size_t totalInstances_ = 0;
};

// ===== Foliage System =====
class FoliageSystem {
public:
    FoliageSystem() = default;
    
    void initialize(float terrainSize, int patchesPerSide = 16) {
        terrainSize_ = terrainSize;
        patchesPerSide_ = patchesPerSide;
    }
    
    FoliageLayer& addLayer(const FoliageLayerSettings& settings) {
        layers_.push_back(std::make_unique<FoliageLayer>());
        layers_.back()->initialize(settings, terrainSize_, patchesPerSide_);
        return *layers_.back();
    }
    
    void removeLayer(size_t index) {
        if (index < layers_.size()) {
            layers_.erase(layers_.begin() + index);
        }
    }
    
    void generateAll(const Terrain& terrain, uint32_t seed = 0) {
        for (size_t i = 0; i < layers_.size(); i++) {
            layers_[i]->generateInstances(terrain, seed + (uint32_t)i * 12345);
        }
    }
    
    void updateLOD(const Vec3& cameraPos) {
        for (auto& layer : layers_) {
            layer->updateLOD(cameraPos);
        }
    }
    
    const std::vector<std::unique_ptr<FoliageLayer>>& getLayers() const { return layers_; }
    std::vector<std::unique_ptr<FoliageLayer>>& getLayers() { return layers_; }
    
    size_t getLayerCount() const { return layers_.size(); }
    
    size_t getTotalInstances() const {
        size_t count = 0;
        for (const auto& layer : layers_) {
            count += layer->getTotalInstances();
        }
        return count;
    }
    
    size_t getVisibleInstances() const {
        size_t count = 0;
        for (const auto& layer : layers_) {
            count += layer->getVisibleInstances();
        }
        return count;
    }
    
    void clear() {
        layers_.clear();
    }
    
    // Preset: Default grass
    static FoliageLayerSettings presetGrass() {
        FoliageLayerSettings settings;
        settings.name = "Grass";
        settings.type = FoliageType::Grass;
        settings.density = 20.0f;
        settings.minScale = 0.7f;
        settings.maxScale = 1.3f;
        settings.baseColor = {0.3f, 0.55f, 0.2f};
        settings.colorVariation = {0.1f, 0.15f, 0.05f};
        settings.maxSlope = 0.4f;
        settings.maxHeight = 0.6f;
        settings.width = 0.08f;
        settings.height = 0.25f;
        settings.windStrength = 1.0f;
        return settings;
    }
    
    // Preset: Tall grass
    static FoliageLayerSettings presetTallGrass() {
        FoliageLayerSettings settings;
        settings.name = "Tall Grass";
        settings.type = FoliageType::Grass;
        settings.density = 5.0f;
        settings.minScale = 1.0f;
        settings.maxScale = 1.5f;
        settings.baseColor = {0.25f, 0.45f, 0.15f};
        settings.colorVariation = {0.08f, 0.12f, 0.05f};
        settings.maxSlope = 0.3f;
        settings.maxHeight = 0.5f;
        settings.width = 0.12f;
        settings.height = 0.5f;
        settings.windStrength = 1.5f;
        return settings;
    }
    
    // Preset: Flowers
    static FoliageLayerSettings presetFlowers() {
        FoliageLayerSettings settings;
        settings.name = "Flowers";
        settings.type = FoliageType::Flower;
        settings.density = 2.0f;
        settings.minScale = 0.6f;
        settings.maxScale = 1.0f;
        settings.baseColor = {0.9f, 0.7f, 0.3f};
        settings.colorVariation = {0.3f, 0.3f, 0.2f};
        settings.maxSlope = 0.25f;
        settings.maxHeight = 0.4f;
        settings.width = 0.1f;
        settings.height = 0.2f;
        settings.windStrength = 0.8f;
        return settings;
    }
    
    // Preset: Rocks
    static FoliageLayerSettings presetRocks() {
        FoliageLayerSettings settings;
        settings.name = "Rocks";
        settings.type = FoliageType::Rock;
        settings.density = 0.5f;
        settings.minScale = 0.5f;
        settings.maxScale = 2.0f;
        settings.baseColor = {0.5f, 0.5f, 0.5f};
        settings.colorVariation = {0.1f, 0.1f, 0.1f};
        settings.minSlope = 0.2f;
        settings.maxSlope = 0.8f;
        settings.billboard = false;
        settings.windStrength = 0.0f;
        settings.cullDistance = 200.0f;
        return settings;
    }
    
    // Preset: Trees
    static FoliageLayerSettings presetTrees() {
        FoliageLayerSettings settings;
        settings.name = "Trees";
        settings.type = FoliageType::Tree;
        settings.density = 0.1f;
        settings.minScale = 0.8f;
        settings.maxScale = 1.4f;
        settings.baseColor = {0.2f, 0.35f, 0.15f};
        settings.colorVariation = {0.05f, 0.1f, 0.05f};
        settings.maxSlope = 0.35f;
        settings.minHeight = 0.1f;
        settings.maxHeight = 0.5f;
        settings.billboard = false;
        settings.windStrength = 0.3f;
        settings.cullDistance = 500.0f;
        settings.lodDistance[0] = 100.0f;
        settings.lodDistance[1] = 200.0f;
        settings.lodDistance[2] = 350.0f;
        return settings;
    }
    
private:
    std::vector<std::unique_ptr<FoliageLayer>> layers_;
    float terrainSize_ = 256.0f;
    int patchesPerSide_ = 16;
};

// ===== Global Foliage System =====
inline FoliageSystem& getFoliageSystem() {
    static FoliageSystem system;
    return system;
}

}  // namespace luma
