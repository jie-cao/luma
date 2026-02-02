// GPU Instancing - Efficient batch rendering for many identical objects
// Reduces draw calls by rendering multiple instances in a single call
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace luma {

// ============================================================================
// Instance Data - Per-instance transform and properties
// ============================================================================

struct InstanceData {
    Mat4 transform;             // Model matrix
    Vec4 color{1, 1, 1, 1};     // Instance color/tint
    Vec4 customData{0, 0, 0, 0}; // User-defined data
    
    // For culling
    Vec3 boundingCenter{0, 0, 0};
    float boundingRadius = 1.0f;
    
    // Flags
    bool visible = true;
    int lodLevel = 0;
    
    static InstanceData fromPosition(const Vec3& pos) {
        InstanceData data;
        data.transform = Mat4::translation(pos);
        data.boundingCenter = pos;
        return data;
    }
    
    static InstanceData fromTransform(const Vec3& pos, const Quat& rot, const Vec3& scale) {
        InstanceData data;
        Mat4 t = Mat4::translation(pos);
        Mat4 r = rot.toMatrix();
        Mat4 s = Mat4::scale(scale);
        data.transform = t * r * s;
        data.boundingCenter = pos;
        data.boundingRadius = std::max({scale.x, scale.y, scale.z});
        return data;
    }
};

// ============================================================================
// Instance Batch - Collection of instances sharing the same mesh/material
// ============================================================================

class InstanceBatch {
public:
    InstanceBatch() = default;
    InstanceBatch(const std::string& meshId, const std::string& materialId)
        : meshId_(meshId), materialId_(materialId) {}
    
    // Instance management
    int addInstance(const InstanceData& data) {
        int index = static_cast<int>(instances_.size());
        instances_.push_back(data);
        dirty_ = true;
        return index;
    }
    
    void removeInstance(int index) {
        if (index >= 0 && index < static_cast<int>(instances_.size())) {
            instances_.erase(instances_.begin() + index);
            dirty_ = true;
        }
    }
    
    void updateInstance(int index, const InstanceData& data) {
        if (index >= 0 && index < static_cast<int>(instances_.size())) {
            instances_[index] = data;
            dirty_ = true;
        }
    }
    
    InstanceData* getInstance(int index) {
        if (index >= 0 && index < static_cast<int>(instances_.size())) {
            return &instances_[index];
        }
        return nullptr;
    }
    
    void clear() {
        instances_.clear();
        dirty_ = true;
    }
    
    // Bulk operations
    void addInstances(const std::vector<InstanceData>& data) {
        instances_.insert(instances_.end(), data.begin(), data.end());
        dirty_ = true;
    }
    
    void setInstances(const std::vector<InstanceData>& data) {
        instances_ = data;
        dirty_ = true;
    }
    
    // Accessors
    const std::string& getMeshId() const { return meshId_; }
    const std::string& getMaterialId() const { return materialId_; }
    
    int getInstanceCount() const { return static_cast<int>(instances_.size()); }
    int getVisibleCount() const { return visibleCount_; }
    
    const std::vector<InstanceData>& getInstances() const { return instances_; }
    std::vector<InstanceData>& getInstances() { return instances_; }
    
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }
    
    // Culling
    void performCulling(const Mat4& viewProj, const Vec3& cameraPos) {
        visibleCount_ = 0;
        culledInstances_.clear();
        
        for (auto& instance : instances_) {
            if (!instance.visible) continue;
            
            // Simple frustum culling using bounding sphere
            Vec4 center4(instance.boundingCenter.x, instance.boundingCenter.y,
                        instance.boundingCenter.z, 1.0f);
            Vec4 projected = viewProj * center4;
            
            // Check if inside frustum (simplified)
            float w = projected.w;
            if (w > 0) {
                float x = projected.x / w;
                float y = projected.y / w;
                
                // Screen bounds check with radius
                float screenRadius = instance.boundingRadius / w * 2.0f;
                
                if (x + screenRadius > -1 && x - screenRadius < 1 &&
                    y + screenRadius > -1 && y - screenRadius < 1) {
                    culledInstances_.push_back(instance);
                    visibleCount_++;
                }
            }
        }
    }
    
    const std::vector<InstanceData>& getCulledInstances() const {
        return culledInstances_;
    }
    
    // Get instance buffer data for GPU
    std::vector<Mat4> getTransformBuffer() const {
        std::vector<Mat4> buffer;
        buffer.reserve(culledInstances_.empty() ? instances_.size() : culledInstances_.size());
        
        const auto& source = culledInstances_.empty() ? instances_ : culledInstances_;
        for (const auto& instance : source) {
            buffer.push_back(instance.transform);
        }
        
        return buffer;
    }
    
    std::vector<Vec4> getColorBuffer() const {
        std::vector<Vec4> buffer;
        buffer.reserve(culledInstances_.empty() ? instances_.size() : culledInstances_.size());
        
        const auto& source = culledInstances_.empty() ? instances_ : culledInstances_;
        for (const auto& instance : source) {
            buffer.push_back(instance.color);
        }
        
        return buffer;
    }
    
private:
    std::string meshId_;
    std::string materialId_;
    
    std::vector<InstanceData> instances_;
    std::vector<InstanceData> culledInstances_;
    
    int visibleCount_ = 0;
    bool dirty_ = true;
};

// ============================================================================
// Instanced Mesh - Mesh with instancing support
// ============================================================================

struct InstancedMesh {
    std::shared_ptr<Mesh> mesh;
    std::string materialId;
    
    // Instance buffer (GPU side)
    bool bufferDirty = true;
    int maxInstances = 1000;
    
    // Batches for different materials/variations
    std::vector<InstanceBatch> batches;
};

// ============================================================================
// Instance Manager - Global instancing management
// ============================================================================

class InstanceManager {
public:
    static InstanceManager& getInstance() {
        static InstanceManager instance;
        return instance;
    }
    
    void initialize() {
        initialized_ = true;
    }
    
    // Create or get a batch
    InstanceBatch* createBatch(const std::string& batchId,
                               const std::string& meshId,
                               const std::string& materialId) {
        batches_[batchId] = InstanceBatch(meshId, materialId);
        return &batches_[batchId];
    }
    
    InstanceBatch* getBatch(const std::string& batchId) {
        auto it = batches_.find(batchId);
        return (it != batches_.end()) ? &it->second : nullptr;
    }
    
    void removeBatch(const std::string& batchId) {
        batches_.erase(batchId);
    }
    
    // Update all batches
    void update(const Mat4& viewProj, const Vec3& cameraPos) {
        viewProj_ = viewProj;
        cameraPos_ = cameraPos;
        
        if (autoCulling_) {
            for (auto& [id, batch] : batches_) {
                batch.performCulling(viewProj, cameraPos);
            }
        }
    }
    
    // Get all batches for rendering
    std::vector<InstanceBatch*> getBatches() {
        std::vector<InstanceBatch*> result;
        for (auto& [id, batch] : batches_) {
            if (batch.getInstanceCount() > 0) {
                result.push_back(&batch);
            }
        }
        return result;
    }
    
    // Settings
    void setAutoCulling(bool enabled) { autoCulling_ = enabled; }
    bool getAutoCulling() const { return autoCulling_; }
    
    void setMaxInstancesPerBatch(int max) { maxInstancesPerBatch_ = max; }
    int getMaxInstancesPerBatch() const { return maxInstancesPerBatch_; }
    
    // Statistics
    struct Statistics {
        int totalBatches = 0;
        int totalInstances = 0;
        int visibleInstances = 0;
        int drawCalls = 0;
        int trianglesRendered = 0;
    };
    
    Statistics getStatistics() const {
        Statistics stats;
        stats.totalBatches = static_cast<int>(batches_.size());
        
        for (const auto& [id, batch] : batches_) {
            stats.totalInstances += batch.getInstanceCount();
            stats.visibleInstances += batch.getVisibleCount();
            if (batch.getVisibleCount() > 0) {
                stats.drawCalls++;
            }
        }
        
        return stats;
    }
    
    // Quick helpers
    void scatter(const std::string& batchId, const Vec3& center, float radius,
                 int count, float minScale = 0.8f, float maxScale = 1.2f) {
        auto* batch = getBatch(batchId);
        if (!batch) return;
        
        for (int i = 0; i < count; i++) {
            // Random position in disk
            float angle = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
            float dist = std::sqrt(static_cast<float>(rand()) / RAND_MAX) * radius;
            
            Vec3 pos = center;
            pos.x += std::cos(angle) * dist;
            pos.z += std::sin(angle) * dist;
            
            // Random rotation and scale
            float rotY = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
            float scale = minScale + static_cast<float>(rand()) / RAND_MAX * (maxScale - minScale);
            
            Quat rot = Quat::fromAxisAngle(Vec3(0, 1, 0), rotY);
            
            batch->addInstance(InstanceData::fromTransform(pos, rot, Vec3(scale, scale, scale)));
        }
    }
    
    void grid(const std::string& batchId, const Vec3& origin,
              int countX, int countZ, float spacing) {
        auto* batch = getBatch(batchId);
        if (!batch) return;
        
        for (int x = 0; x < countX; x++) {
            for (int z = 0; z < countZ; z++) {
                Vec3 pos = origin;
                pos.x += x * spacing;
                pos.z += z * spacing;
                
                batch->addInstance(InstanceData::fromPosition(pos));
            }
        }
    }
    
private:
    InstanceManager() = default;
    
    std::unordered_map<std::string, InstanceBatch> batches_;
    
    Mat4 viewProj_;
    Vec3 cameraPos_;
    
    bool autoCulling_ = true;
    int maxInstancesPerBatch_ = 10000;
    
    bool initialized_ = false;
};

inline InstanceManager& getInstanceManager() {
    return InstanceManager::getInstance();
}

// ============================================================================
// Vegetation Instancing - Specialized for foliage
// ============================================================================

struct VegetationInstance {
    Vec3 position;
    float rotation = 0;         // Y-axis rotation
    float scale = 1.0f;
    Vec3 tint{1, 1, 1};
    float windPhase = 0;        // For wind animation
    float health = 1.0f;        // For seasonal effects
};

class VegetationInstancer {
public:
    // Add vegetation type
    void addType(const std::string& typeId, const std::string& meshId,
                 float density = 1.0f, float minScale = 0.8f, float maxScale = 1.2f) {
        VegetationType type;
        type.meshId = meshId;
        type.density = density;
        type.minScale = minScale;
        type.maxScale = maxScale;
        types_[typeId] = type;
    }
    
    // Generate instances for an area
    void generateForArea(const std::string& typeId, const Vec3& min, const Vec3& max,
                         std::function<float(float x, float z)> heightmap = nullptr,
                         std::function<float(float x, float z)> densityMap = nullptr) {
        auto it = types_.find(typeId);
        if (it == types_.end()) return;
        
        auto& type = it->second;
        
        // Calculate instance count
        float area = (max.x - min.x) * (max.z - min.z);
        int count = static_cast<int>(area * type.density);
        
        type.instances.clear();
        type.instances.reserve(count);
        
        for (int i = 0; i < count; i++) {
            float x = min.x + static_cast<float>(rand()) / RAND_MAX * (max.x - min.x);
            float z = min.z + static_cast<float>(rand()) / RAND_MAX * (max.z - min.z);
            
            // Check density map
            if (densityMap) {
                float d = densityMap(x, z);
                if (static_cast<float>(rand()) / RAND_MAX > d) continue;
            }
            
            VegetationInstance instance;
            instance.position.x = x;
            instance.position.z = z;
            instance.position.y = heightmap ? heightmap(x, z) : 0;
            
            instance.rotation = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
            instance.scale = type.minScale + static_cast<float>(rand()) / RAND_MAX * 
                            (type.maxScale - type.minScale);
            instance.windPhase = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
            
            // Slight color variation
            float variation = 0.9f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
            instance.tint = Vec3(variation, variation, variation);
            
            type.instances.push_back(instance);
        }
    }
    
    // Get instances for a type
    const std::vector<VegetationInstance>& getInstances(const std::string& typeId) const {
        static std::vector<VegetationInstance> empty;
        auto it = types_.find(typeId);
        return (it != types_.end()) ? it->second.instances : empty;
    }
    
    // Convert to InstanceData for rendering
    std::vector<InstanceData> getInstanceData(const std::string& typeId, float time = 0) const {
        std::vector<InstanceData> result;
        
        auto it = types_.find(typeId);
        if (it == types_.end()) return result;
        
        for (const auto& veg : it->second.instances) {
            InstanceData data;
            
            // Apply wind animation
            float windOffset = std::sin(time * 2.0f + veg.windPhase) * 0.05f * veg.scale;
            Vec3 pos = veg.position;
            pos.x += windOffset;
            
            Quat rot = Quat::fromAxisAngle(Vec3(0, 1, 0), veg.rotation);
            Vec3 scale(veg.scale, veg.scale, veg.scale);
            
            data = InstanceData::fromTransform(pos, rot, scale);
            data.color = Vec4(veg.tint.x, veg.tint.y, veg.tint.z, 1.0f);
            data.customData = Vec4(veg.windPhase, veg.health, 0, 0);
            
            result.push_back(data);
        }
        
        return result;
    }
    
private:
    struct VegetationType {
        std::string meshId;
        float density = 1.0f;
        float minScale = 0.8f;
        float maxScale = 1.2f;
        std::vector<VegetationInstance> instances;
    };
    
    std::unordered_map<std::string, VegetationType> types_;
};

}  // namespace luma
