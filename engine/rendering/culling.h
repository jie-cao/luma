// Culling Systems - Frustum Culling, Occlusion Culling
// Performance optimization for rendering large scenes
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <functional>

namespace luma {

// ===== Bounding Volumes =====

struct BoundingSphere {
    Vec3 center{0, 0, 0};
    float radius = 1.0f;
    
    // Create from min/max points
    static BoundingSphere fromMinMax(const Vec3& minPt, const Vec3& maxPt) {
        BoundingSphere sphere;
        sphere.center = {
            (minPt.x + maxPt.x) * 0.5f,
            (minPt.y + maxPt.y) * 0.5f,
            (minPt.z + maxPt.z) * 0.5f
        };
        float dx = maxPt.x - minPt.x;
        float dy = maxPt.y - minPt.y;
        float dz = maxPt.z - minPt.z;
        sphere.radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
        return sphere;
    }
    
    // Transform sphere by matrix
    BoundingSphere transformed(const Mat4& matrix) const {
        BoundingSphere result;
        // Transform center
        float w = matrix.m[3] * center.x + matrix.m[7] * center.y + 
                  matrix.m[11] * center.z + matrix.m[15];
        if (fabsf(w) < 0.0001f) w = 1.0f;
        result.center.x = (matrix.m[0] * center.x + matrix.m[4] * center.y + 
                          matrix.m[8] * center.z + matrix.m[12]) / w;
        result.center.y = (matrix.m[1] * center.x + matrix.m[5] * center.y + 
                          matrix.m[9] * center.z + matrix.m[13]) / w;
        result.center.z = (matrix.m[2] * center.x + matrix.m[6] * center.y + 
                          matrix.m[10] * center.z + matrix.m[14]) / w;
        
        // Scale radius by largest scale factor
        float sx = sqrtf(matrix.m[0]*matrix.m[0] + matrix.m[1]*matrix.m[1] + matrix.m[2]*matrix.m[2]);
        float sy = sqrtf(matrix.m[4]*matrix.m[4] + matrix.m[5]*matrix.m[5] + matrix.m[6]*matrix.m[6]);
        float sz = sqrtf(matrix.m[8]*matrix.m[8] + matrix.m[9]*matrix.m[9] + matrix.m[10]*matrix.m[10]);
        result.radius = radius * std::max({sx, sy, sz});
        
        return result;
    }
};

// ===== Frustum =====

struct Plane {
    Vec3 normal{0, 1, 0};
    float distance = 0.0f;
    
    // Distance from point to plane (positive = in front)
    float distanceToPoint(const Vec3& point) const {
        return normal.x * point.x + normal.y * point.y + normal.z * point.z + distance;
    }
    
    // Normalize the plane
    void normalize() {
        float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        if (len > 0.0001f) {
            normal.x /= len;
            normal.y /= len;
            normal.z /= len;
            distance /= len;
        }
    }
};

class Frustum {
public:
    enum PlaneIndex {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        PlaneCount
    };
    
    Plane planes[PlaneCount];
    
    // Extract frustum planes from view-projection matrix
    void extractFromMatrix(const Mat4& viewProj) {
        // Left plane
        planes[Left].normal.x = viewProj.m[3] + viewProj.m[0];
        planes[Left].normal.y = viewProj.m[7] + viewProj.m[4];
        planes[Left].normal.z = viewProj.m[11] + viewProj.m[8];
        planes[Left].distance = viewProj.m[15] + viewProj.m[12];
        
        // Right plane
        planes[Right].normal.x = viewProj.m[3] - viewProj.m[0];
        planes[Right].normal.y = viewProj.m[7] - viewProj.m[4];
        planes[Right].normal.z = viewProj.m[11] - viewProj.m[8];
        planes[Right].distance = viewProj.m[15] - viewProj.m[12];
        
        // Bottom plane
        planes[Bottom].normal.x = viewProj.m[3] + viewProj.m[1];
        planes[Bottom].normal.y = viewProj.m[7] + viewProj.m[5];
        planes[Bottom].normal.z = viewProj.m[11] + viewProj.m[9];
        planes[Bottom].distance = viewProj.m[15] + viewProj.m[13];
        
        // Top plane
        planes[Top].normal.x = viewProj.m[3] - viewProj.m[1];
        planes[Top].normal.y = viewProj.m[7] - viewProj.m[5];
        planes[Top].normal.z = viewProj.m[11] - viewProj.m[9];
        planes[Top].distance = viewProj.m[15] - viewProj.m[13];
        
        // Near plane
        planes[Near].normal.x = viewProj.m[3] + viewProj.m[2];
        planes[Near].normal.y = viewProj.m[7] + viewProj.m[6];
        planes[Near].normal.z = viewProj.m[11] + viewProj.m[10];
        planes[Near].distance = viewProj.m[15] + viewProj.m[14];
        
        // Far plane
        planes[Far].normal.x = viewProj.m[3] - viewProj.m[2];
        planes[Far].normal.y = viewProj.m[7] - viewProj.m[6];
        planes[Far].normal.z = viewProj.m[11] - viewProj.m[10];
        planes[Far].distance = viewProj.m[15] - viewProj.m[14];
        
        // Normalize all planes
        for (int i = 0; i < PlaneCount; i++) {
            planes[i].normalize();
        }
    }
    
    // Test if sphere is inside frustum
    bool containsSphere(const BoundingSphere& sphere) const {
        for (int i = 0; i < PlaneCount; i++) {
            float dist = planes[i].distanceToPoint(sphere.center);
            if (dist < -sphere.radius) {
                return false;  // Completely outside this plane
            }
        }
        return true;
    }
    
    // Test if point is inside frustum
    bool containsPoint(const Vec3& point) const {
        for (int i = 0; i < PlaneCount; i++) {
            if (planes[i].distanceToPoint(point) < 0) {
                return false;
            }
        }
        return true;
    }
};

// ===== Frustum Culler =====

struct CullResult {
    size_t totalObjects = 0;
    size_t visibleObjects = 0;
    size_t culledObjects = 0;
    
    float cullingRatio() const {
        return totalObjects > 0 ? (float)culledObjects / totalObjects : 0.0f;
    }
};

class FrustumCuller {
public:
    // Update frustum from view-projection matrix
    void updateFrustum(const Mat4& viewProj) {
        frustum_.extractFromMatrix(viewProj);
    }
    
    // Test visibility of a bounding sphere
    bool isVisible(const BoundingSphere& sphere) const {
        return frustum_.containsSphere(sphere);
    }
    
    // Get the frustum for custom tests
    const Frustum& getFrustum() const { return frustum_; }
    
    // Cull a list of bounding spheres, return indices of visible objects
    template<typename T>
    CullResult cull(const std::vector<T>& objects,
                    std::function<BoundingSphere(const T&)> getBounds,
                    std::vector<size_t>& outVisibleIndices) const {
        CullResult result;
        result.totalObjects = objects.size();
        outVisibleIndices.clear();
        outVisibleIndices.reserve(objects.size());
        
        for (size_t i = 0; i < objects.size(); i++) {
            BoundingSphere bounds = getBounds(objects[i]);
            if (frustum_.containsSphere(bounds)) {
                outVisibleIndices.push_back(i);
                result.visibleObjects++;
            } else {
                result.culledObjects++;
            }
        }
        
        return result;
    }
    
private:
    Frustum frustum_;
};

// ===== Occlusion Query Helper =====
// Note: Actual GPU queries are platform-specific, this is a CPU-side helper

struct OcclusionQueryResult {
    uint32_t objectId;
    bool visible;
    uint32_t pixelCount;  // Number of pixels that passed depth test
};

class OcclusionCuller {
public:
    // Enable/disable occlusion culling
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Set minimum pixel threshold for visibility
    void setPixelThreshold(uint32_t threshold) { pixelThreshold_ = threshold; }
    uint32_t getPixelThreshold() const { return pixelThreshold_; }
    
    // Process occlusion query results (called after GPU queries complete)
    void processResults(const std::vector<OcclusionQueryResult>& results) {
        visibleObjects_.clear();
        for (const auto& result : results) {
            if (result.visible && result.pixelCount >= pixelThreshold_) {
                visibleObjects_.push_back(result.objectId);
            }
        }
    }
    
    // Check if object was visible in previous frame
    bool wasVisible(uint32_t objectId) const {
        for (uint32_t id : visibleObjects_) {
            if (id == objectId) return true;
        }
        return false;
    }
    
    // Get statistics
    size_t getVisibleCount() const { return visibleObjects_.size(); }
    
private:
    bool enabled_ = false;
    uint32_t pixelThreshold_ = 1;
    std::vector<uint32_t> visibleObjects_;
};

// ===== Combined Culling System =====

class CullingSystem {
public:
    static CullingSystem& get() {
        static CullingSystem instance;
        return instance;
    }
    
    // Update for new frame
    void beginFrame(const Mat4& viewProj) {
        frustumCuller_.updateFrustum(viewProj);
        stats_ = {};
    }
    
    // Get cullers
    FrustumCuller& getFrustumCuller() { return frustumCuller_; }
    OcclusionCuller& getOcclusionCuller() { return occlusionCuller_; }
    
    // Quick visibility test
    bool isVisible(const BoundingSphere& worldBounds) {
        stats_.totalObjects++;
        
        // Frustum cull first (cheapest)
        if (!frustumCuller_.isVisible(worldBounds)) {
            stats_.frustumCulled++;
            return false;
        }
        
        stats_.visibleObjects++;
        return true;
    }
    
    // Get frame statistics
    struct Stats {
        size_t totalObjects = 0;
        size_t visibleObjects = 0;
        size_t frustumCulled = 0;
        size_t occlusionCulled = 0;
    };
    
    const Stats& getStats() const { return stats_; }
    
private:
    CullingSystem() = default;
    
    FrustumCuller frustumCuller_;
    OcclusionCuller occlusionCuller_;
    Stats stats_;
};

// Global accessor
inline CullingSystem& getCullingSystem() {
    return CullingSystem::get();
}

}  // namespace luma
