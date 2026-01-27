// Reflection Probe System - Environment reflections
// Cubemap-based reflection probes with box/sphere influence
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <algorithm>

namespace luma {

// ===== Reflection Probe Shape =====
enum class ReflectionProbeShape {
    Box,
    Sphere
};

// ===== Reflection Probe Settings =====
struct ReflectionProbeSettings {
    // Cubemap
    int resolution = 256;           // Per-face resolution
    int mipLevels = 7;              // Mip chain for roughness
    bool hdr = true;
    
    // Rendering
    float nearClip = 0.1f;
    float farClip = 100.0f;
    int layerMask = 0xFFFFFFFF;     // Which layers to render
    
    // Update
    bool realtime = false;          // vs baked
    int refreshRate = 0;            // 0 = every frame, N = every N frames
    float timeSlice = 0.0f;         // For progressive updates
    
    // Quality
    bool boxProjection = true;       // Use parallax correction
    float blendDistance = 1.0f;      // Distance for blending between probes
};

// ===== Reflection Probe =====
class ReflectionProbe {
public:
    ReflectionProbe() : id_(nextId_++) {}
    
    uint32_t getId() const { return id_; }
    
    // Name
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }
    
    // Position
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    // Shape and bounds
    void setShape(ReflectionProbeShape shape) { shape_ = shape; }
    ReflectionProbeShape getShape() const { return shape_; }
    
    // Box bounds (for box shape)
    void setBoxSize(const Vec3& size) { boxSize_ = size; }
    Vec3 getBoxSize() const { return boxSize_; }
    
    void setBoxOffset(const Vec3& offset) { boxOffset_ = offset; }
    Vec3 getBoxOffset() const { return boxOffset_; }
    
    // Sphere radius (for sphere shape)
    void setSphereRadius(float radius) { sphereRadius_ = radius; }
    float getSphereRadius() const { return sphereRadius_; }
    
    // Influence
    void setInfluenceRadius(float radius) { influenceRadius_ = radius; }
    float getInfluenceRadius() const { return influenceRadius_; }
    
    // Settings
    void setSettings(const ReflectionProbeSettings& settings) { settings_ = settings; }
    ReflectionProbeSettings& getSettings() { return settings_; }
    const ReflectionProbeSettings& getSettings() const { return settings_; }
    
    // Priority (higher = more important)
    void setPriority(int priority) { priority_ = priority; }
    int getPriority() const { return priority_; }
    
    // Intensity
    void setIntensity(float intensity) { intensity_ = intensity; }
    float getIntensity() const { return intensity_; }
    
    // Check if a point is inside the influence volume
    bool containsPoint(const Vec3& point) const {
        if (shape_ == ReflectionProbeShape::Sphere) {
            Vec3 delta = point - position_;
            return delta.length() <= influenceRadius_;
        } else {
            Vec3 min = position_ + boxOffset_ - boxSize_ * 0.5f;
            Vec3 max = position_ + boxOffset_ + boxSize_ * 0.5f;
            return point.x >= min.x && point.x <= max.x &&
                   point.y >= min.y && point.y <= max.y &&
                   point.z >= min.z && point.z <= max.z;
        }
    }
    
    // Calculate blend weight for a point
    float calculateBlendWeight(const Vec3& point) const {
        if (!containsPoint(point)) return 0.0f;
        
        if (shape_ == ReflectionProbeShape::Sphere) {
            Vec3 delta = point - position_;
            float dist = delta.length();
            float blend = 1.0f - (dist / influenceRadius_);
            return std::max(0.0f, std::min(1.0f, blend));
        } else {
            Vec3 halfSize = boxSize_ * 0.5f;
            Vec3 local = point - (position_ + boxOffset_);
            
            // Calculate distance to each face
            float dx = halfSize.x - std::abs(local.x);
            float dy = halfSize.y - std::abs(local.y);
            float dz = halfSize.z - std::abs(local.z);
            
            float minDist = std::min(std::min(dx, dy), dz);
            float blend = minDist / settings_.blendDistance;
            return std::max(0.0f, std::min(1.0f, blend));
        }
    }
    
    // Box projection correction for parallax
    Vec3 boxProjectReflection(const Vec3& position, const Vec3& reflectionDir) const {
        if (!settings_.boxProjection) return reflectionDir;
        
        Vec3 boxMin = position_ + boxOffset_ - boxSize_ * 0.5f;
        Vec3 boxMax = position_ + boxOffset_ + boxSize_ * 0.5f;
        
        // Find intersection with box
        Vec3 firstPlaneIntersect = (boxMax - position) / reflectionDir;
        Vec3 secondPlaneIntersect = (boxMin - position) / reflectionDir;
        
        Vec3 furthestPlane;
        furthestPlane.x = std::max(firstPlaneIntersect.x, secondPlaneIntersect.x);
        furthestPlane.y = std::max(firstPlaneIntersect.y, secondPlaneIntersect.y);
        furthestPlane.z = std::max(firstPlaneIntersect.z, secondPlaneIntersect.z);
        
        float dist = std::min(std::min(furthestPlane.x, furthestPlane.y), furthestPlane.z);
        
        Vec3 intersectionPos = position + reflectionDir * dist;
        return (intersectionPos - position_).normalized();
    }
    
    // GPU handles
    uint32_t gpuCubemapHandle = 0;
    bool gpuCubemapValid = false;
    
    // State
    bool isDirty() const { return dirty_; }
    void setDirty(bool dirty) { dirty_ = dirty; }
    
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
private:
    uint32_t id_;
    static inline uint32_t nextId_ = 0;
    
    std::string name_ = "ReflectionProbe";
    Vec3 position_ = {0, 0, 0};
    
    ReflectionProbeShape shape_ = ReflectionProbeShape::Box;
    Vec3 boxSize_ = {10, 10, 10};
    Vec3 boxOffset_ = {0, 0, 0};
    float sphereRadius_ = 10.0f;
    float influenceRadius_ = 10.0f;
    
    ReflectionProbeSettings settings_;
    int priority_ = 0;
    float intensity_ = 1.0f;
    
    bool dirty_ = true;
    bool enabled_ = true;
};

// ===== Reflection Probe Manager =====
class ReflectionProbeManager {
public:
    ReflectionProbeManager() {
        // Create default skybox probe
        auto skyProbe = createProbe("Skybox");
        skyProbe->setShape(ReflectionProbeShape::Sphere);
        skyProbe->setSphereRadius(10000.0f);
        skyProbe->setInfluenceRadius(10000.0f);
        skyProbe->setPriority(-1000);  // Lowest priority
    }
    
    ReflectionProbe* createProbe(const std::string& name = "ReflectionProbe") {
        probes_.push_back(std::make_unique<ReflectionProbe>());
        probes_.back()->setName(name);
        return probes_.back().get();
    }
    
    void removeProbe(ReflectionProbe* probe) {
        probes_.erase(
            std::remove_if(probes_.begin(), probes_.end(),
                [probe](const auto& p) { return p.get() == probe; }),
            probes_.end()
        );
    }
    
    void clear() {
        probes_.clear();
    }
    
    // Find probes affecting a point, sorted by priority and blend weight
    struct ProbeBlend {
        ReflectionProbe* probe;
        float weight;
    };
    
    std::vector<ProbeBlend> findProbesForPoint(const Vec3& point, int maxCount = 2) const {
        std::vector<ProbeBlend> affecting;
        
        for (const auto& probe : probes_) {
            if (!probe->isEnabled()) continue;
            
            float weight = probe->calculateBlendWeight(point);
            if (weight > 0.0f) {
                affecting.push_back({probe.get(), weight});
            }
        }
        
        // Sort by priority (descending), then by weight (descending)
        std::sort(affecting.begin(), affecting.end(),
            [](const ProbeBlend& a, const ProbeBlend& b) {
                if (a.probe->getPriority() != b.probe->getPriority()) {
                    return a.probe->getPriority() > b.probe->getPriority();
                }
                return a.weight > b.weight;
            });
        
        // Limit count and renormalize weights
        if (affecting.size() > (size_t)maxCount) {
            affecting.resize(maxCount);
        }
        
        float totalWeight = 0.0f;
        for (const auto& pb : affecting) {
            totalWeight += pb.weight;
        }
        
        if (totalWeight > 0.0f) {
            for (auto& pb : affecting) {
                pb.weight /= totalWeight;
            }
        }
        
        return affecting;
    }
    
    // Get all probes that need updating
    std::vector<ReflectionProbe*> getDirtyProbes() const {
        std::vector<ReflectionProbe*> dirty;
        for (const auto& probe : probes_) {
            if (probe->isDirty() && probe->isEnabled()) {
                dirty.push_back(probe.get());
            }
        }
        return dirty;
    }
    
    // Mark all probes as dirty
    void markAllDirty() {
        for (auto& probe : probes_) {
            probe->setDirty(true);
        }
    }
    
    // Accessors
    const std::vector<std::unique_ptr<ReflectionProbe>>& getProbes() const { return probes_; }
    std::vector<std::unique_ptr<ReflectionProbe>>& getProbes() { return probes_; }
    size_t getProbeCount() const { return probes_.size(); }
    
    ReflectionProbe* getProbeByName(const std::string& name) {
        for (auto& probe : probes_) {
            if (probe->getName() == name) return probe.get();
        }
        return nullptr;
    }
    
private:
    std::vector<std::unique_ptr<ReflectionProbe>> probes_;
};

// ===== Global Manager =====
inline ReflectionProbeManager& getReflectionProbeManager() {
    static ReflectionProbeManager manager;
    return manager;
}

}  // namespace luma
