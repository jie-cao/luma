// Render Optimizer - Unified performance optimization system
// Integrates culling, LOD, and instancing for optimal rendering
#pragma once

#include "culling.h"
#include "lod.h"
#include "instancing.h"
#include "engine/scene/entity.h"
#include <functional>

namespace luma {

// ===== Render Object =====
// Represents an object to be rendered with all optimization data
struct RenderObject {
    uint32_t entityId;
    uint32_t meshId;
    uint32_t materialId;
    
    Mat4 worldMatrix;
    BoundingSphere worldBounds;
    
    // LOD data
    int lodLevel = 0;
    float lodBlend = 1.0f;
    
    // Flags
    bool visible = true;
    bool castsShadow = true;
    bool receiveShadow = true;
    bool isStatic = false;
    
    // Sorting key for front-to-back or back-to-front
    float sortDistance = 0.0f;
};

// ===== Render Queue =====
// Sorted collection of render objects
class RenderQueue {
public:
    enum class SortMode {
        None,
        FrontToBack,    // For opaque objects (minimize overdraw)
        BackToFront,    // For transparent objects (correct blending)
        ByMaterial,     // Minimize state changes
        ByMesh          // Maximize instancing
    };
    
    void clear() {
        objects_.clear();
    }
    
    void add(const RenderObject& obj) {
        objects_.push_back(obj);
    }
    
    void sort(SortMode mode, const Vec3& cameraPosition) {
        switch (mode) {
            case SortMode::FrontToBack:
                sortByDistance(cameraPosition, true);
                break;
            case SortMode::BackToFront:
                sortByDistance(cameraPosition, false);
                break;
            case SortMode::ByMaterial:
                sortByMaterial();
                break;
            case SortMode::ByMesh:
                sortByMesh();
                break;
            default:
                break;
        }
    }
    
    const std::vector<RenderObject>& getObjects() const { return objects_; }
    size_t size() const { return objects_.size(); }
    
private:
    void sortByDistance(const Vec3& cameraPos, bool frontToBack) {
        for (auto& obj : objects_) {
            float dx = obj.worldBounds.center.x - cameraPos.x;
            float dy = obj.worldBounds.center.y - cameraPos.y;
            float dz = obj.worldBounds.center.z - cameraPos.z;
            obj.sortDistance = dx*dx + dy*dy + dz*dz;
        }
        
        if (frontToBack) {
            std::sort(objects_.begin(), objects_.end(),
                [](const RenderObject& a, const RenderObject& b) {
                    return a.sortDistance < b.sortDistance;
                });
        } else {
            std::sort(objects_.begin(), objects_.end(),
                [](const RenderObject& a, const RenderObject& b) {
                    return a.sortDistance > b.sortDistance;
                });
        }
    }
    
    void sortByMaterial() {
        std::sort(objects_.begin(), objects_.end(),
            [](const RenderObject& a, const RenderObject& b) {
                return a.materialId < b.materialId;
            });
    }
    
    void sortByMesh() {
        std::sort(objects_.begin(), objects_.end(),
            [](const RenderObject& a, const RenderObject& b) {
                if (a.meshId != b.meshId) return a.meshId < b.meshId;
                return a.materialId < b.materialId;
            });
    }
    
    std::vector<RenderObject> objects_;
};

// ===== Render Optimizer =====
class RenderOptimizer {
public:
    static RenderOptimizer& get() {
        static RenderOptimizer instance;
        return instance;
    }
    
    // Configuration
    struct Config {
        bool enableFrustumCulling = true;
        bool enableLOD = true;
        bool enableInstancing = true;
        bool enableOcclusionCulling = false;  // Requires GPU support
        
        // LOD settings
        float lodBias = 0.0f;
        int maxLODLevel = 4;
        
        // Instancing settings
        size_t minInstancesForBatch = 2;
        size_t maxInstancesPerBatch = 1024;
        
        // Debug
        bool showCullingStats = false;
        bool freezeCulling = false;  // Don't update frustum (for debugging)
    };
    
    Config& getConfig() { return config_; }
    const Config& getConfig() const { return config_; }
    
    // Begin frame - update frustum and reset stats
    void beginFrame(const Mat4& viewMatrix, const Mat4& projMatrix, const Vec3& cameraPos) {
        cameraPosition_ = cameraPos;
        
        if (!config_.freezeCulling) {
            Mat4 viewProj = projMatrix * viewMatrix;
            getCullingSystem().beginFrame(viewProj);
        }
        
        getInstancingManager().beginFrame();
        getLODManager().resetStats();
        
        opaqueQueue_.clear();
        transparentQueue_.clear();
        
        frameStats_ = {};
    }
    
    // Process an entity and add to appropriate render queue
    void processEntity(Entity* entity, const LODGroup* lodGroup = nullptr) {
        if (!entity || !entity->enabled || !entity->hasModel) {
            return;
        }
        
        frameStats_.totalEntities++;
        
        // Create bounding sphere from entity
        BoundingSphere bounds;
        bounds.center = entity->getWorldPosition();
        bounds.radius = entity->model.radius;
        
        // Transform bounds by world matrix
        bounds = bounds.transformed(entity->worldMatrix);
        
        // Frustum culling
        if (config_.enableFrustumCulling) {
            if (!getCullingSystem().isVisible(bounds)) {
                frameStats_.frustumCulled++;
                return;
            }
        }
        
        // LOD selection
        int lodLevel = 0;
        float lodBlend = 1.0f;
        
        if (config_.enableLOD && lodGroup) {
            auto& lodMgr = getLODManager();
            auto selection = lodMgr.selectLOD(*lodGroup, bounds.center, cameraPosition_);
            
            if (selection.culled) {
                frameStats_.lodCulled++;
                return;
            }
            
            lodLevel = selection.lodLevel;
            lodBlend = selection.blendFactor;
            lodMgr.recordSelection(selection);
        }
        
        // Create render object
        RenderObject obj;
        obj.entityId = entity->id;
        obj.meshId = 0;  // TODO: Get from model
        obj.materialId = entity->material ? entity->material->id : 0;
        obj.worldMatrix = entity->worldMatrix;
        obj.worldBounds = bounds;
        obj.lodLevel = lodLevel;
        obj.lodBlend = lodBlend;
        obj.visible = true;
        
        // Determine if transparent
        bool isTransparent = entity->material && 
            (entity->material->alphaBlend || entity->material->alpha < 1.0f);
        
        if (isTransparent) {
            transparentQueue_.add(obj);
        } else {
            opaqueQueue_.add(obj);
        }
        
        // Add to instancing if enabled
        if (config_.enableInstancing) {
            InstanceData instanceData = InstanceData::fromTransform(entity->worldMatrix);
            instanceData.materialId = obj.materialId;
            instanceData.lodBlend = lodBlend;
            
            getInstancingManager().addInstance(obj.meshId, obj.materialId, instanceData);
        }
        
        frameStats_.visibleEntities++;
    }
    
    // Finalize frame - sort queues, batch instances
    void endFrame() {
        // Sort opaque objects front-to-back for early-z
        opaqueQueue_.sort(RenderQueue::SortMode::FrontToBack, cameraPosition_);
        
        // Sort transparent objects back-to-front for correct blending
        transparentQueue_.sort(RenderQueue::SortMode::BackToFront, cameraPosition_);
        
        // Perform instancing culling
        if (config_.enableInstancing) {
            getInstancingManager().cullInstances(
                getCullingSystem().getFrustumCuller(), 1.0f);
        }
        
        // Calculate statistics
        frameStats_.opaqueObjects = opaqueQueue_.size();
        frameStats_.transparentObjects = transparentQueue_.size();
        
        auto& instStats = getInstancingManager().getStats();
        frameStats_.instancedDrawCalls = instStats.batchCount;
        frameStats_.drawCallSavings = getInstancingManager().getDrawCallReduction();
    }
    
    // Get render queues
    const RenderQueue& getOpaqueQueue() const { return opaqueQueue_; }
    const RenderQueue& getTransparentQueue() const { return transparentQueue_; }
    
    // Statistics
    struct FrameStats {
        size_t totalEntities = 0;
        size_t visibleEntities = 0;
        size_t frustumCulled = 0;
        size_t lodCulled = 0;
        size_t occlusionCulled = 0;
        
        size_t opaqueObjects = 0;
        size_t transparentObjects = 0;
        
        size_t instancedDrawCalls = 0;
        float drawCallSavings = 0.0f;
    };
    
    const FrameStats& getFrameStats() const { return frameStats_; }
    
    // Get combined statistics string
    std::string getStatsString() const {
        char buf[512];
        snprintf(buf, sizeof(buf),
            "Entities: %zu visible / %zu total\n"
            "Culled: %zu frustum, %zu LOD, %zu occlusion\n"
            "Opaque: %zu, Transparent: %zu\n"
            "Draw call savings: %.1f%%",
            frameStats_.visibleEntities, frameStats_.totalEntities,
            frameStats_.frustumCulled, frameStats_.lodCulled, frameStats_.occlusionCulled,
            frameStats_.opaqueObjects, frameStats_.transparentObjects,
            frameStats_.drawCallSavings * 100.0f);
        return std::string(buf);
    }
    
private:
    RenderOptimizer() = default;
    
    Config config_;
    Vec3 cameraPosition_{0, 0, 0};
    
    RenderQueue opaqueQueue_;
    RenderQueue transparentQueue_;
    
    FrameStats frameStats_;
};

// Global accessor
inline RenderOptimizer& getRenderOptimizer() {
    return RenderOptimizer::get();
}

}  // namespace luma
