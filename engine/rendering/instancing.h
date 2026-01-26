// GPU Instancing System
// Efficient rendering of multiple instances of the same mesh
#pragma once

#include "engine/foundation/math_types.h"
#include "culling.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace luma {

// ===== Instance Data =====
// Per-instance data sent to GPU
struct InstanceData {
    Mat4 worldMatrix;
    Mat4 normalMatrix;  // Inverse transpose for correct normal transformation
    Vec3 color;         // Instance color tint
    float padding;
    
    // Additional per-instance data
    uint32_t materialId;
    uint32_t flags;     // Custom flags (e.g., selected, highlighted)
    float lodBlend;     // For smooth LOD transitions
    float padding2;
    
    static InstanceData fromTransform(const Mat4& world) {
        InstanceData data;
        data.worldMatrix = world;
        // Normal matrix would be inverse-transpose, but for uniform scale
        // we can just use the world matrix (transpose handles the rest in shader)
        data.normalMatrix = world;
        data.color = {1, 1, 1};
        data.padding = 0;
        data.materialId = 0;
        data.flags = 0;
        data.lodBlend = 1.0f;
        data.padding2 = 0;
        return data;
    }
};

// ===== Instance Batch =====
// A collection of instances sharing the same mesh
struct InstanceBatch {
    uint32_t meshId;
    uint32_t materialId;
    std::vector<InstanceData> instances;
    
    // Bounding volume for the entire batch (used for batch-level culling)
    BoundingSphere batchBounds;
    
    // GPU buffer handles (platform-specific)
    void* instanceBuffer = nullptr;
    size_t instanceBufferSize = 0;
    bool bufferDirty = true;
    
    void addInstance(const InstanceData& data) {
        instances.push_back(data);
        bufferDirty = true;
    }
    
    void clear() {
        instances.clear();
        bufferDirty = true;
    }
    
    size_t getInstanceCount() const { return instances.size(); }
    
    // Update batch bounding volume
    void updateBounds() {
        if (instances.empty()) {
            batchBounds = BoundingSphere();
            return;
        }
        
        // Calculate AABB encompassing all instances
        Vec3 min = {1e30f, 1e30f, 1e30f};
        Vec3 max = {-1e30f, -1e30f, -1e30f};
        
        for (const auto& instance : instances) {
            Vec3 pos = {
                instance.worldMatrix.m[12],
                instance.worldMatrix.m[13],
                instance.worldMatrix.m[14]
            };
            min.x = fminf(min.x, pos.x);
            min.y = fminf(min.y, pos.y);
            min.z = fminf(min.z, pos.z);
            max.x = fmaxf(max.x, pos.x);
            max.y = fmaxf(max.y, pos.y);
            max.z = fmaxf(max.z, pos.z);
        }
        
        batchBounds = BoundingSphere::fromMinMax(min, max);
    }
};

// ===== Instance Group =====
// Groups batches by mesh+material for efficient rendering
struct InstanceGroup {
    std::string name;
    uint32_t meshId;
    uint32_t materialId;
    
    std::vector<InstanceData> visibleInstances;  // After culling
    size_t totalInstances = 0;
    size_t visibleCount = 0;
    
    // GPU buffer for visible instances
    void* gpuBuffer = nullptr;
    size_t gpuBufferCapacity = 0;
};

// ===== Instancing Manager =====
class InstancingManager {
public:
    static InstancingManager& get() {
        static InstancingManager instance;
        return instance;
    }
    
    // Begin a new frame
    void beginFrame() {
        for (auto& [key, batch] : batches_) {
            batch.clear();
        }
        frameStats_ = {};
    }
    
    // Add an instance to be rendered
    void addInstance(uint32_t meshId, uint32_t materialId, const InstanceData& data) {
        uint64_t key = makeKey(meshId, materialId);
        auto& batch = batches_[key];
        batch.meshId = meshId;
        batch.materialId = materialId;
        batch.addInstance(data);
        frameStats_.totalInstances++;
    }
    
    // Add instance from transform
    void addInstance(uint32_t meshId, uint32_t materialId, const Mat4& worldMatrix) {
        addInstance(meshId, materialId, InstanceData::fromTransform(worldMatrix));
    }
    
    // Get all batches for rendering
    const std::unordered_map<uint64_t, InstanceBatch>& getBatches() const {
        return batches_;
    }
    
    // Get batch count
    size_t getBatchCount() const { return batches_.size(); }
    
    // Perform frustum culling on all batches
    void cullInstances(const FrustumCuller& culler, float objectRadius = 1.0f) {
        culledBatches_.clear();
        frameStats_.visibleInstances = 0;
        frameStats_.culledInstances = 0;
        
        for (auto& [key, batch] : batches_) {
            InstanceBatch culledBatch;
            culledBatch.meshId = batch.meshId;
            culledBatch.materialId = batch.materialId;
            
            for (const auto& instance : batch.instances) {
                // Get instance position from world matrix
                Vec3 pos = {
                    instance.worldMatrix.m[12],
                    instance.worldMatrix.m[13],
                    instance.worldMatrix.m[14]
                };
                
                BoundingSphere bounds;
                bounds.center = pos;
                bounds.radius = objectRadius;
                
                if (culler.isVisible(bounds)) {
                    culledBatch.instances.push_back(instance);
                    frameStats_.visibleInstances++;
                } else {
                    frameStats_.culledInstances++;
                }
            }
            
            if (!culledBatch.instances.empty()) {
                culledBatches_[key] = std::move(culledBatch);
            }
        }
    }
    
    // Get culled batches for rendering
    const std::unordered_map<uint64_t, InstanceBatch>& getCulledBatches() const {
        return culledBatches_;
    }
    
    // Settings
    void setMaxInstancesPerBatch(size_t max) { maxInstancesPerBatch_ = max; }
    size_t getMaxInstancesPerBatch() const { return maxInstancesPerBatch_; }
    
    void setMinInstancesForBatching(size_t min) { minInstancesForBatching_ = min; }
    size_t getMinInstancesForBatching() const { return minInstancesForBatching_; }
    
    // Statistics
    struct Stats {
        size_t totalInstances = 0;
        size_t visibleInstances = 0;
        size_t culledInstances = 0;
        size_t batchCount = 0;
        size_t drawCalls = 0;  // After batching
    };
    
    const Stats& getStats() const { return frameStats_; }
    
    // Calculate draw call savings
    float getDrawCallReduction() const {
        if (frameStats_.visibleInstances == 0) return 0.0f;
        // Without instancing: 1 draw call per instance
        // With instancing: 1 draw call per batch
        return 1.0f - (float)culledBatches_.size() / frameStats_.visibleInstances;
    }
    
private:
    InstancingManager() = default;
    
    uint64_t makeKey(uint32_t meshId, uint32_t materialId) const {
        return ((uint64_t)meshId << 32) | materialId;
    }
    
    std::unordered_map<uint64_t, InstanceBatch> batches_;
    std::unordered_map<uint64_t, InstanceBatch> culledBatches_;
    
    size_t maxInstancesPerBatch_ = 1024;
    size_t minInstancesForBatching_ = 2;
    
    Stats frameStats_;
};

// Global accessor
inline InstancingManager& getInstancingManager() {
    return InstancingManager::get();
}

// ===== Indirect Drawing Support =====
// For GPU-driven rendering

struct IndirectDrawCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  baseVertex;
    uint32_t firstInstance;
};

struct IndirectDrawBatch {
    std::vector<IndirectDrawCommand> commands;
    std::vector<InstanceData> instanceData;
    
    // GPU buffers (platform-specific handles)
    void* commandBuffer = nullptr;
    void* instanceBuffer = nullptr;
    
    size_t getDrawCount() const { return commands.size(); }
    size_t getTotalInstances() const { return instanceData.size(); }
};

}  // namespace luma
