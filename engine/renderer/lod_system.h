// LOD System - Level of Detail management
// Automatic mesh switching based on camera distance
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace luma {

// ============================================================================
// LOD Level Definition
// ============================================================================

struct LODLevel {
    int level = 0;              // 0 = highest detail
    float screenSize = 0;       // Screen size threshold (0-1)
    float distance = 0;         // Distance threshold
    
    // Mesh data
    std::shared_ptr<Mesh> mesh;
    
    // Quality metrics
    int vertexCount = 0;
    int triangleCount = 0;
    float reductionPercent = 0; // % reduction from LOD0
    
    // Optional shadow-only mesh (lower quality for shadow maps)
    std::shared_ptr<Mesh> shadowMesh;
};

// ============================================================================
// LOD Group - Collection of LOD levels for an object
// ============================================================================

class LODGroup {
public:
    LODGroup() = default;
    explicit LODGroup(const std::string& name) : name_(name) {}
    
    // Add LOD level
    void addLevel(const LODLevel& level) {
        levels_.push_back(level);
        // Sort by level number
        std::sort(levels_.begin(), levels_.end(), 
                  [](const LODLevel& a, const LODLevel& b) {
                      return a.level < b.level;
                  });
    }
    
    // Get LOD level by index
    LODLevel* getLevel(int index) {
        if (index >= 0 && index < static_cast<int>(levels_.size())) {
            return &levels_[index];
        }
        return nullptr;
    }
    
    // Get appropriate LOD for distance
    int selectLODByDistance(float distance) const {
        for (size_t i = 0; i < levels_.size(); i++) {
            if (distance < levels_[i].distance) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(levels_.size()) - 1;
    }
    
    // Get appropriate LOD for screen size (0-1)
    int selectLODByScreenSize(float screenSize) const {
        for (size_t i = 0; i < levels_.size(); i++) {
            if (screenSize > levels_[i].screenSize) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(levels_.size()) - 1;
    }
    
    // Get mesh for LOD level
    Mesh* getMesh(int lodLevel) const {
        if (lodLevel >= 0 && lodLevel < static_cast<int>(levels_.size())) {
            return levels_[lodLevel].mesh.get();
        }
        return nullptr;
    }
    
    // Get shadow mesh (uses lower LOD if available)
    Mesh* getShadowMesh(int lodLevel) const {
        if (lodLevel >= 0 && lodLevel < static_cast<int>(levels_.size())) {
            if (levels_[lodLevel].shadowMesh) {
                return levels_[lodLevel].shadowMesh.get();
            }
            // Fall back to regular mesh
            return levels_[lodLevel].mesh.get();
        }
        return nullptr;
    }
    
    const std::string& getName() const { return name_; }
    int getLevelCount() const { return static_cast<int>(levels_.size()); }
    const std::vector<LODLevel>& getLevels() const { return levels_; }
    
    // Bounds (calculated from LOD0)
    void setBounds(const Vec3& center, float radius) {
        boundsCenter_ = center;
        boundsRadius_ = radius;
    }
    
    Vec3 getBoundsCenter() const { return boundsCenter_; }
    float getBoundsRadius() const { return boundsRadius_; }
    
    // LOD bias (adjust selection)
    void setLODBias(float bias) { lodBias_ = bias; }
    float getLODBias() const { return lodBias_; }
    
    // Fade transition
    void setFadeDuration(float seconds) { fadeDuration_ = seconds; }
    float getFadeDuration() const { return fadeDuration_; }
    
private:
    std::string name_;
    std::vector<LODLevel> levels_;
    
    Vec3 boundsCenter_{0, 0, 0};
    float boundsRadius_ = 1.0f;
    
    float lodBias_ = 0;          // Negative = higher quality, positive = lower
    float fadeDuration_ = 0.2f;  // Cross-fade duration
};

// ============================================================================
// LOD Generator - Auto-generate LOD meshes
// ============================================================================

struct LODGenerationSettings {
    int numLevels = 4;                  // Number of LOD levels to generate
    
    // Reduction targets for each level (% of original)
    std::vector<float> reductionTargets = {1.0f, 0.5f, 0.25f, 0.1f};
    
    // Distance thresholds
    std::vector<float> distanceThresholds = {0, 10, 25, 50};
    
    // Screen size thresholds
    std::vector<float> screenSizeThresholds = {1.0f, 0.5f, 0.25f, 0.1f};
    
    // Quality settings
    bool preserveUVs = true;
    bool preserveNormals = true;
    bool preserveBorders = true;
    float targetError = 0.01f;
    
    // Shadow LOD
    bool generateShadowLOD = true;
    float shadowLODReduction = 0.5f;  // Additional reduction for shadows
};

class LODGenerator {
public:
    // Generate LOD group from a source mesh
    static LODGroup generate(const Mesh& sourceMesh, 
                             const LODGenerationSettings& settings = {}) {
        LODGroup group;
        
        for (int i = 0; i < settings.numLevels; i++) {
            LODLevel level;
            level.level = i;
            
            if (i < static_cast<int>(settings.distanceThresholds.size())) {
                level.distance = settings.distanceThresholds[i];
            }
            
            if (i < static_cast<int>(settings.screenSizeThresholds.size())) {
                level.screenSize = settings.screenSizeThresholds[i];
            }
            
            float reduction = 1.0f;
            if (i < static_cast<int>(settings.reductionTargets.size())) {
                reduction = settings.reductionTargets[i];
            }
            
            // Generate simplified mesh
            auto simplifiedMesh = std::make_shared<Mesh>();
            if (i == 0) {
                // LOD0 is the original mesh
                *simplifiedMesh = sourceMesh;
            } else {
                // Simplify mesh
                *simplifiedMesh = simplifyMesh(sourceMesh, reduction, settings);
            }
            
            level.mesh = simplifiedMesh;
            level.vertexCount = static_cast<int>(simplifiedMesh->vertices.size());
            level.triangleCount = static_cast<int>(simplifiedMesh->indices.size() / 3);
            level.reductionPercent = 1.0f - static_cast<float>(level.vertexCount) / 
                                     sourceMesh.vertices.size();
            
            // Generate shadow mesh if requested
            if (settings.generateShadowLOD && i > 0) {
                float shadowReduction = reduction * settings.shadowLODReduction;
                level.shadowMesh = std::make_shared<Mesh>();
                *level.shadowMesh = simplifyMesh(sourceMesh, shadowReduction, settings);
            }
            
            group.addLevel(level);
        }
        
        // Calculate bounds from LOD0
        if (!sourceMesh.vertices.empty()) {
            Vec3 minP = sourceMesh.vertices[0].position;
            Vec3 maxP = minP;
            
            for (const auto& v : sourceMesh.vertices) {
                minP.x = std::min(minP.x, v.position.x);
                minP.y = std::min(minP.y, v.position.y);
                minP.z = std::min(minP.z, v.position.z);
                maxP.x = std::max(maxP.x, v.position.x);
                maxP.y = std::max(maxP.y, v.position.y);
                maxP.z = std::max(maxP.z, v.position.z);
            }
            
            Vec3 center = (minP + maxP) * 0.5f;
            float radius = (maxP - minP).length() * 0.5f;
            group.setBounds(center, radius);
        }
        
        return group;
    }
    
private:
    // Simplified mesh decimation using edge collapse
    static Mesh simplifyMesh(const Mesh& source, float targetRatio,
                             const LODGenerationSettings& settings) {
        if (targetRatio >= 1.0f) {
            return source;
        }
        
        Mesh result = source;
        
        int targetVertices = static_cast<int>(source.vertices.size() * targetRatio);
        targetVertices = std::max(targetVertices, 4);  // Minimum 4 vertices
        
        // Simple vertex clustering decimation
        // (A real implementation would use QEM or similar)
        
        float gridSize = estimateGridSize(source, targetVertices);
        
        // Cluster vertices
        std::unordered_map<uint64_t, std::vector<int>> clusters;
        std::unordered_map<uint64_t, Vertex> clusterRepresentatives;
        
        for (size_t i = 0; i < source.vertices.size(); i++) {
            const Vec3& p = source.vertices[i].position;
            int gx = static_cast<int>(std::floor(p.x / gridSize));
            int gy = static_cast<int>(std::floor(p.y / gridSize));
            int gz = static_cast<int>(std::floor(p.z / gridSize));
            
            uint64_t key = hashGridCell(gx, gy, gz);
            clusters[key].push_back(static_cast<int>(i));
        }
        
        // Create representative vertices
        result.vertices.clear();
        std::unordered_map<uint64_t, int> clusterToVertex;
        
        for (auto& [key, indices] : clusters) {
            Vertex representative = source.vertices[indices[0]];
            
            // Average position and attributes
            Vec3 avgPos{0, 0, 0};
            Vec3 avgNormal{0, 0, 0};
            float avgU = 0, avgV = 0;
            
            for (int idx : indices) {
                const auto& v = source.vertices[idx];
                avgPos = avgPos + v.position;
                avgNormal = avgNormal + v.normal;
                avgU += v.texCoord0.x;
                avgV += v.texCoord0.y;
            }
            
            float count = static_cast<float>(indices.size());
            representative.position = avgPos * (1.0f / count);
            representative.normal = avgNormal.normalized();
            
            if (settings.preserveUVs) {
                representative.texCoord0 = {avgU / count, avgV / count};
            }
            
            clusterToVertex[key] = static_cast<int>(result.vertices.size());
            result.vertices.push_back(representative);
        }
        
        // Remap indices
        result.indices.clear();
        
        for (size_t i = 0; i < source.indices.size(); i += 3) {
            int i0 = source.indices[i];
            int i1 = source.indices[i + 1];
            int i2 = source.indices[i + 2];
            
            // Find cluster for each vertex
            auto findCluster = [&](int idx) -> uint64_t {
                const Vec3& p = source.vertices[idx].position;
                int gx = static_cast<int>(std::floor(p.x / gridSize));
                int gy = static_cast<int>(std::floor(p.y / gridSize));
                int gz = static_cast<int>(std::floor(p.z / gridSize));
                return hashGridCell(gx, gy, gz);
            };
            
            uint64_t c0 = findCluster(i0);
            uint64_t c1 = findCluster(i1);
            uint64_t c2 = findCluster(i2);
            
            // Skip degenerate triangles
            if (c0 == c1 || c1 == c2 || c0 == c2) continue;
            
            result.indices.push_back(clusterToVertex[c0]);
            result.indices.push_back(clusterToVertex[c1]);
            result.indices.push_back(clusterToVertex[c2]);
        }
        
        return result;
    }
    
    static float estimateGridSize(const Mesh& mesh, int targetVertices) {
        if (mesh.vertices.empty()) return 1.0f;
        
        // Calculate bounding box
        Vec3 minP = mesh.vertices[0].position;
        Vec3 maxP = minP;
        
        for (const auto& v : mesh.vertices) {
            minP.x = std::min(minP.x, v.position.x);
            minP.y = std::min(minP.y, v.position.y);
            minP.z = std::min(minP.z, v.position.z);
            maxP.x = std::max(maxP.x, v.position.x);
            maxP.y = std::max(maxP.y, v.position.y);
            maxP.z = std::max(maxP.z, v.position.z);
        }
        
        Vec3 size = maxP - minP;
        float volume = size.x * size.y * size.z;
        
        // Estimate grid size to achieve target vertex count
        float cellsNeeded = static_cast<float>(targetVertices);
        float cellSize = std::pow(volume / cellsNeeded, 1.0f / 3.0f);
        
        return std::max(cellSize, 0.001f);
    }
    
    static uint64_t hashGridCell(int x, int y, int z) {
        return (static_cast<uint64_t>(x + 10000) << 40) |
               (static_cast<uint64_t>(y + 10000) << 20) |
               (static_cast<uint64_t>(z + 10000));
    }
};

// ============================================================================
// LOD Instance - Runtime LOD state for an object
// ============================================================================

struct LODInstance {
    LODGroup* group = nullptr;
    
    int currentLOD = 0;
    int targetLOD = 0;
    float fadeProgress = 1.0f;  // 0 = fading, 1 = complete
    
    Vec3 worldPosition{0, 0, 0};
    float worldScale = 1.0f;
    
    bool forceLOD = false;
    int forcedLODLevel = 0;
    
    // Update LOD selection
    void update(const Vec3& cameraPosition, float deltaTime) {
        if (!group || forceLOD) {
            currentLOD = forcedLODLevel;
            return;
        }
        
        // Calculate distance
        float distance = (worldPosition - cameraPosition).length();
        distance /= worldScale;  // Adjust for scale
        distance += group->getLODBias();
        
        // Select LOD
        int newLOD = group->selectLODByDistance(distance);
        
        if (newLOD != targetLOD) {
            targetLOD = newLOD;
            fadeProgress = 0;
        }
        
        // Update fade
        if (fadeProgress < 1.0f) {
            float fadeDuration = group->getFadeDuration();
            if (fadeDuration > 0) {
                fadeProgress += deltaTime / fadeDuration;
                fadeProgress = std::min(fadeProgress, 1.0f);
            } else {
                fadeProgress = 1.0f;
            }
            
            if (fadeProgress >= 1.0f) {
                currentLOD = targetLOD;
            }
        }
    }
    
    bool isFading() const { return fadeProgress < 1.0f; }
    
    // Get interpolated mesh for cross-fade rendering
    std::pair<Mesh*, Mesh*> getFadeMeshes() const {
        if (!group) return {nullptr, nullptr};
        
        Mesh* current = group->getMesh(currentLOD);
        Mesh* target = group->getMesh(targetLOD);
        
        return {current, target};
    }
};

// ============================================================================
// LOD Manager - Global LOD management
// ============================================================================

class LODManager {
public:
    static LODManager& getInstance() {
        static LODManager instance;
        return instance;
    }
    
    void initialize() {
        initialized_ = true;
    }
    
    // Register LOD group
    void registerGroup(const std::string& id, const LODGroup& group) {
        groups_[id] = group;
    }
    
    // Get LOD group
    LODGroup* getGroup(const std::string& id) {
        auto it = groups_.find(id);
        return (it != groups_.end()) ? &it->second : nullptr;
    }
    
    // Create instance
    LODInstance createInstance(const std::string& groupId) {
        LODInstance instance;
        instance.group = getGroup(groupId);
        return instance;
    }
    
    // Update all instances
    void update(const Vec3& cameraPosition, float deltaTime) {
        cameraPosition_ = cameraPosition;
        
        for (auto& instance : instances_) {
            instance->update(cameraPosition, deltaTime);
        }
    }
    
    // Register instance for automatic updates
    void trackInstance(LODInstance* instance) {
        instances_.push_back(instance);
    }
    
    void untrackInstance(LODInstance* instance) {
        instances_.erase(std::remove(instances_.begin(), instances_.end(), instance),
                        instances_.end());
    }
    
    // Settings
    void setGlobalLODBias(float bias) { globalLODBias_ = bias; }
    float getGlobalLODBias() const { return globalLODBias_; }
    
    void setMaxLODLevel(int level) { maxLODLevel_ = level; }
    int getMaxLODLevel() const { return maxLODLevel_; }
    
    void setLODDistanceScale(float scale) { lodDistanceScale_ = scale; }
    float getLODDistanceScale() const { return lodDistanceScale_; }
    
    // Statistics
    struct Statistics {
        int totalGroups = 0;
        int totalInstances = 0;
        int lodTransitions = 0;
        std::array<int, 8> instancesPerLOD{};
    };
    
    Statistics getStatistics() const {
        Statistics stats;
        stats.totalGroups = static_cast<int>(groups_.size());
        stats.totalInstances = static_cast<int>(instances_.size());
        
        for (const auto* instance : instances_) {
            if (instance->currentLOD < 8) {
                stats.instancesPerLOD[instance->currentLOD]++;
            }
            if (instance->isFading()) {
                stats.lodTransitions++;
            }
        }
        
        return stats;
    }
    
private:
    LODManager() = default;
    
    std::unordered_map<std::string, LODGroup> groups_;
    std::vector<LODInstance*> instances_;
    
    Vec3 cameraPosition_{0, 0, 0};
    float globalLODBias_ = 0;
    int maxLODLevel_ = 7;
    float lodDistanceScale_ = 1.0f;
    
    bool initialized_ = false;
};

inline LODManager& getLODManager() {
    return LODManager::getInstance();
}

}  // namespace luma
