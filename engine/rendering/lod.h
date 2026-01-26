// Level of Detail (LOD) System
// Automatic mesh simplification based on distance
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/unified_renderer.h"
#include <vector>
#include <string>
#include <cmath>

namespace luma {

// ===== LOD Level =====
struct LODLevel {
    float maxDistance;     // Max distance for this LOD level
    float screenSize;      // Alternative: screen-space size threshold
    uint32_t meshIndex;    // Index into mesh array
    float triangleRatio;   // Ratio of triangles compared to LOD0 (1.0 = full detail)
    
    // Quality reduction (for procedural LOD)
    float vertexReduction = 1.0f;  // 1.0 = no reduction, 0.5 = half vertices
};

// ===== LOD Group =====
// Contains multiple LOD levels for a single object
struct LODGroup {
    std::string name;
    std::vector<LODLevel> levels;
    
    // Culling settings
    float cullDistance = 1000.0f;  // Distance at which object is not rendered
    bool useCulling = true;
    
    // LOD bias (positive = use higher LOD, negative = use lower LOD)
    float lodBias = 0.0f;
    
    // Transition settings
    bool smoothTransitions = false;
    float transitionWidth = 0.1f;  // Percentage of distance for crossfade
    
    // Get LOD level for given distance
    int getLODLevel(float distance) const {
        if (levels.empty()) return 0;
        
        float biasedDistance = distance * powf(2.0f, -lodBias);
        
        for (size_t i = 0; i < levels.size(); i++) {
            if (biasedDistance <= levels[i].maxDistance) {
                return static_cast<int>(i);
            }
        }
        
        // Return last LOD if beyond all distances
        return static_cast<int>(levels.size() - 1);
    }
    
    // Get LOD level for screen-space size (in pixels)
    int getLODLevelByScreenSize(float screenSize) const {
        if (levels.empty()) return 0;
        
        for (size_t i = 0; i < levels.size(); i++) {
            if (screenSize >= levels[i].screenSize) {
                return static_cast<int>(i);
            }
        }
        
        return static_cast<int>(levels.size() - 1);
    }
    
    // Get transition blend factor (0.0 to 1.0) for smooth transitions
    float getTransitionFactor(float distance, int currentLOD) const {
        if (!smoothTransitions || currentLOD >= (int)levels.size() - 1) {
            return 1.0f;
        }
        
        float currentMax = levels[currentLOD].maxDistance;
        float transitionStart = currentMax * (1.0f - transitionWidth);
        
        if (distance < transitionStart) {
            return 1.0f;
        }
        
        return 1.0f - (distance - transitionStart) / (currentMax - transitionStart);
    }
    
    // Create default LOD group with distance-based levels
    static LODGroup createDefault(float baseDistance = 10.0f) {
        LODGroup group;
        group.name = "Default LOD";
        
        // LOD 0: Full detail (0 - baseDistance)
        LODLevel lod0;
        lod0.maxDistance = baseDistance;
        lod0.meshIndex = 0;
        lod0.triangleRatio = 1.0f;
        lod0.screenSize = 100.0f;
        group.levels.push_back(lod0);
        
        // LOD 1: 50% detail (baseDistance - 2x)
        LODLevel lod1;
        lod1.maxDistance = baseDistance * 2.0f;
        lod1.meshIndex = 1;
        lod1.triangleRatio = 0.5f;
        lod1.screenSize = 50.0f;
        group.levels.push_back(lod1);
        
        // LOD 2: 25% detail (2x - 4x)
        LODLevel lod2;
        lod2.maxDistance = baseDistance * 4.0f;
        lod2.meshIndex = 2;
        lod2.triangleRatio = 0.25f;
        lod2.screenSize = 25.0f;
        group.levels.push_back(lod2);
        
        // LOD 3: 10% detail (4x - 8x)
        LODLevel lod3;
        lod3.maxDistance = baseDistance * 8.0f;
        lod3.meshIndex = 3;
        lod3.triangleRatio = 0.1f;
        lod3.screenSize = 10.0f;
        group.levels.push_back(lod3);
        
        group.cullDistance = baseDistance * 16.0f;
        
        return group;
    }
};

// ===== LOD Selection Result =====
struct LODSelection {
    int lodLevel;
    uint32_t meshIndex;
    float blendFactor;  // 1.0 = full current LOD, 0.0 = transitioning to next
    bool culled;        // True if object should not be rendered
};

// ===== LOD Manager =====
class LODManager {
public:
    static LODManager& get() {
        static LODManager instance;
        return instance;
    }
    
    // Global LOD settings
    void setGlobalLODBias(float bias) { globalBias_ = bias; }
    float getGlobalLODBias() const { return globalBias_; }
    
    void setMaxLODLevel(int maxLevel) { maxLODLevel_ = maxLevel; }
    int getMaxLODLevel() const { return maxLODLevel_; }
    
    void setForceLODLevel(int level) { forcedLOD_ = level; }
    int getForcedLODLevel() const { return forcedLOD_; }
    void clearForcedLOD() { forcedLOD_ = -1; }
    
    // Quality presets
    enum class Quality { Low, Medium, High, Ultra };
    void setQuality(Quality quality) {
        switch (quality) {
            case Quality::Low:
                globalBias_ = -2.0f;
                maxLODLevel_ = 2;
                break;
            case Quality::Medium:
                globalBias_ = -1.0f;
                maxLODLevel_ = 3;
                break;
            case Quality::High:
                globalBias_ = 0.0f;
                maxLODLevel_ = 4;
                break;
            case Quality::Ultra:
                globalBias_ = 1.0f;
                maxLODLevel_ = 10;
                break;
        }
    }
    
    // Calculate LOD selection for an object
    LODSelection selectLOD(const LODGroup& group, 
                            const Vec3& objectPosition,
                            const Vec3& cameraPosition) const {
        LODSelection result;
        result.culled = false;
        result.blendFactor = 1.0f;
        
        // Calculate distance
        float dx = objectPosition.x - cameraPosition.x;
        float dy = objectPosition.y - cameraPosition.y;
        float dz = objectPosition.z - cameraPosition.z;
        float distance = sqrtf(dx*dx + dy*dy + dz*dz);
        
        // Check culling
        if (group.useCulling && distance > group.cullDistance) {
            result.culled = true;
            result.lodLevel = -1;
            result.meshIndex = 0;
            return result;
        }
        
        // Check forced LOD
        if (forcedLOD_ >= 0) {
            result.lodLevel = forcedLOD_;
            result.meshIndex = group.levels.empty() ? 0 : 
                group.levels[std::min((size_t)forcedLOD_, group.levels.size() - 1)].meshIndex;
            return result;
        }
        
        // Apply global bias
        float biasedDistance = distance * powf(2.0f, -globalBias_);
        
        // Get LOD level
        result.lodLevel = group.getLODLevel(biasedDistance);
        
        // Clamp to max LOD level
        if (result.lodLevel > maxLODLevel_) {
            result.lodLevel = maxLODLevel_;
        }
        
        // Clamp to available levels
        if (result.lodLevel >= (int)group.levels.size()) {
            result.lodLevel = (int)group.levels.size() - 1;
        }
        
        result.meshIndex = group.levels[result.lodLevel].meshIndex;
        
        // Calculate transition factor
        if (group.smoothTransitions) {
            result.blendFactor = group.getTransitionFactor(biasedDistance, result.lodLevel);
        }
        
        return result;
    }
    
    // Calculate screen-space size for LOD selection
    float calculateScreenSize(const Vec3& objectPosition,
                               float objectRadius,
                               const Vec3& cameraPosition,
                               float fovY,
                               float screenHeight) const {
        float dx = objectPosition.x - cameraPosition.x;
        float dy = objectPosition.y - cameraPosition.y;
        float dz = objectPosition.z - cameraPosition.z;
        float distance = sqrtf(dx*dx + dy*dy + dz*dz);
        
        if (distance < 0.001f) distance = 0.001f;
        
        // Project radius to screen space
        float projectedSize = (objectRadius / distance) * (screenHeight / (2.0f * tanf(fovY * 0.5f)));
        
        return projectedSize;
    }
    
    // Statistics
    struct Stats {
        size_t totalObjects = 0;
        size_t lodDistribution[8] = {0};  // Count per LOD level
        size_t culledByDistance = 0;
    };
    
    void resetStats() { stats_ = {}; }
    
    void recordSelection(const LODSelection& selection) {
        stats_.totalObjects++;
        if (selection.culled) {
            stats_.culledByDistance++;
        } else if (selection.lodLevel >= 0 && selection.lodLevel < 8) {
            stats_.lodDistribution[selection.lodLevel]++;
        }
    }
    
    const Stats& getStats() const { return stats_; }
    
private:
    LODManager() = default;
    
    float globalBias_ = 0.0f;
    int maxLODLevel_ = 10;
    int forcedLOD_ = -1;
    Stats stats_;
};

// Global accessor
inline LODManager& getLODManager() {
    return LODManager::get();
}

}  // namespace luma
