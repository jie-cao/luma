// Picking System - Ray casting and object selection
#pragma once

#include "entity.h"
#include "scene_graph.h"
#include <limits>
#include <cmath>

namespace luma {

// ===== Ray =====
#ifndef LUMA_RAY_DEFINED
#define LUMA_RAY_DEFINED
struct Ray {
    Vec3 origin;
    Vec3 direction;  // Should be normalized
    
    Ray() = default;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalized()) {}
    
    Vec3 at(float t) const {
        return origin + direction * t;
    }
    
    Vec3 getPoint(float t) const {  // Alias for at()
        return at(t);
    }
};
#endif  // LUMA_RAY_DEFINED

// ===== Axis-Aligned Bounding Box =====
#ifndef LUMA_AABB_DEFINED
#define LUMA_AABB_DEFINED
struct AABB {
    Vec3 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vec3 max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    
    AABB() = default;
    AABB(const Vec3& minPt, const Vec3& maxPt) : min(minPt), max(maxPt) {}
    
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
    
    Vec3 center() const {
        return Vec3((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f);
    }
    
    Vec3 size() const {
        return Vec3(max.x - min.x, max.y - min.y, max.z - min.z);
    }
    
    void expand(const Vec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }
    
    void expand(const AABB& other) {
        expand(other.min);
        expand(other.max);
    }
    
    bool intersects(const AABB& other) const {
        if (max.x < other.min.x || min.x > other.max.x) return false;
        if (max.y < other.min.y || min.y > other.max.y) return false;
        if (max.z < other.min.z || min.z > other.max.z) return false;
        return true;
    }
    
    bool contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    // Transform AABB by matrix (results in larger AABB)
    AABB transformed(const Mat4& m) const {
        AABB result;
        
        // Transform all 8 corners
        Vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {min.x, max.y, max.z},
            {max.x, max.y, max.z}
        };
        
        for (int i = 0; i < 8; i++) {
            result.expand(m.transformPoint(corners[i]));
        }
        
        return result;
    }
};
#endif  // LUMA_AABB_DEFINED

// ===== Ray-AABB Intersection =====
// Returns true if ray intersects AABB, and sets t to hit distance
inline bool rayAABBIntersect(const Ray& ray, const AABB& aabb, float& tMin, float& tMax) {
    tMin = 0.0f;
    tMax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; i++) {
        float origin = (&ray.origin.x)[i];
        float dir = (&ray.direction.x)[i];
        float bmin = (&aabb.min.x)[i];
        float bmax = (&aabb.max.x)[i];
        
        if (std::abs(dir) < 1e-8f) {
            // Ray parallel to slab
            if (origin < bmin || origin > bmax) {
                return false;
            }
        } else {
            float invDir = 1.0f / dir;
            float t1 = (bmin - origin) * invDir;
            float t2 = (bmax - origin) * invDir;
            
            if (t1 > t2) std::swap(t1, t2);
            
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            
            if (tMin > tMax) {
                return false;
            }
        }
    }
    
    return tMin >= 0.0f || tMax >= 0.0f;
}

// Simple overload that just returns hit/miss
inline bool rayAABBIntersect(const Ray& ray, const AABB& aabb) {
    float tMin, tMax;
    return rayAABBIntersect(ray, aabb, tMin, tMax);
}

// ===== Picking Helpers =====

// Get AABB for entity's model (in local space)
inline AABB getEntityLocalAABB(const Entity* entity) {
    if (!entity || !entity->hasModel) {
        return AABB(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
    }
    
    // Use model's center and radius to create AABB
    const RHILoadedModel& model = entity->model;
    Vec3 center(model.center[0], model.center[1], model.center[2]);
    float r = model.radius;
    
    return AABB(
        Vec3(center.x - r, center.y - r, center.z - r),
        Vec3(center.x + r, center.y + r, center.z + r)
    );
}

// Get world-space AABB for entity
inline AABB getEntityWorldAABB(const Entity* entity) {
    AABB localAABB = getEntityLocalAABB(entity);
    return localAABB.transformed(entity->worldMatrix);
}

// ===== Camera Ray Generation =====

// Create ray from screen coordinates (normalized device coordinates: -1 to 1)
// viewProjInverse is the inverse of view * projection matrix
inline Ray screenToWorldRay(float ndcX, float ndcY, const float* viewProjInverse) {
    // Near plane point
    Vec3 nearPoint;
    {
        float w = viewProjInverse[3] * ndcX + viewProjInverse[7] * ndcY + viewProjInverse[11] * (-1.0f) + viewProjInverse[15];
        nearPoint.x = (viewProjInverse[0] * ndcX + viewProjInverse[4] * ndcY + viewProjInverse[8] * (-1.0f) + viewProjInverse[12]) / w;
        nearPoint.y = (viewProjInverse[1] * ndcX + viewProjInverse[5] * ndcY + viewProjInverse[9] * (-1.0f) + viewProjInverse[13]) / w;
        nearPoint.z = (viewProjInverse[2] * ndcX + viewProjInverse[6] * ndcY + viewProjInverse[10] * (-1.0f) + viewProjInverse[14]) / w;
    }
    
    // Far plane point
    Vec3 farPoint;
    {
        float w = viewProjInverse[3] * ndcX + viewProjInverse[7] * ndcY + viewProjInverse[11] * 1.0f + viewProjInverse[15];
        farPoint.x = (viewProjInverse[0] * ndcX + viewProjInverse[4] * ndcY + viewProjInverse[8] * 1.0f + viewProjInverse[12]) / w;
        farPoint.y = (viewProjInverse[1] * ndcX + viewProjInverse[5] * ndcY + viewProjInverse[9] * 1.0f + viewProjInverse[13]) / w;
        farPoint.z = (viewProjInverse[2] * ndcX + viewProjInverse[6] * ndcY + viewProjInverse[10] * 1.0f + viewProjInverse[14]) / w;
    }
    
    return Ray(nearPoint, farPoint - nearPoint);
}

// Convert pixel coordinates to NDC
inline void pixelToNDC(float pixelX, float pixelY, uint32_t screenWidth, uint32_t screenHeight, float& ndcX, float& ndcY) {
    ndcX = (2.0f * pixelX / screenWidth) - 1.0f;
    ndcY = 1.0f - (2.0f * pixelY / screenHeight);  // Y is flipped
}

// ===== Picking Result =====
struct PickResult {
    Entity* entity = nullptr;
    float distance = std::numeric_limits<float>::max();
    Vec3 hitPoint;
    
    bool hit() const { return entity != nullptr; }
};

// ===== Scene Picking =====
// Pick the closest entity at screen position
inline PickResult pickEntity(SceneGraph& scene, const Ray& ray) {
    PickResult result;
    
    scene.traverse([&](Entity* entity) {
        if (!entity->enabled) return;
        
        AABB worldAABB = getEntityWorldAABB(entity);
        float tMin, tMax;
        
        if (rayAABBIntersect(ray, worldAABB, tMin, tMax)) {
            float hitDist = (tMin >= 0) ? tMin : tMax;
            if (hitDist < result.distance) {
                result.entity = entity;
                result.distance = hitDist;
                result.hitPoint = ray.at(hitDist);
            }
        }
    });
    
    return result;
}

}  // namespace luma
