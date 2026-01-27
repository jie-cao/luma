// Raycast System - Precise shape ray intersection tests
// Sphere, Box, Capsule, Plane raycasting
#pragma once

#include "physics_world.h"
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>

namespace luma {

// ===== Ray =====
#ifndef LUMA_RAY_DEFINED
#define LUMA_RAY_DEFINED
struct Ray {
    Vec3 origin;
    Vec3 direction;  // Must be normalized
    
    Ray() : origin(0, 0, 0), direction(0, 0, -1) {}
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalized()) {}
    
    Vec3 at(float t) const { return origin + direction * t; }
    Vec3 getPoint(float t) const { return at(t); }
};
#endif  // LUMA_RAY_DEFINED

// ===== Raycast Hit Result =====
struct RaycastHit {
    bool hit = false;
    float distance = std::numeric_limits<float>::max();
    Vec3 point;
    Vec3 normal;
    RigidBody* body = nullptr;
    Collider* collider = nullptr;
    
    // For sorting
    bool operator<(const RaycastHit& other) const {
        return distance < other.distance;
    }
};

// ===== Raycast Options =====
struct RaycastOptions {
    float maxDistance = 1000.0f;
    CollisionMask layerMask = CollisionLayers::All;
    bool hitTriggers = false;
    bool hitBackfaces = false;
    bool sortByDistance = true;
};

// ===== Ray-Shape Intersection Tests =====

// Ray-Sphere intersection
inline bool raycastSphere(const Ray& ray, const Vec3& center, float radius,
                          float maxDistance, RaycastHit& hit)
{
    Vec3 oc = ray.origin - center;
    
    float a = ray.direction.dot(ray.direction);
    float b = 2.0f * oc.dot(ray.direction);
    float c = oc.dot(oc) - radius * radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0) {
        return false;
    }
    
    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);
    
    // Find the nearest valid intersection
    float t;
    if (t1 > 0.0001f && t1 < maxDistance) {
        t = t1;
    } else if (t2 > 0.0001f && t2 < maxDistance) {
        t = t2;
    } else {
        return false;
    }
    
    hit.hit = true;
    hit.distance = t;
    hit.point = ray.getPoint(t);
    hit.normal = (hit.point - center).normalized();
    
    return true;
}

// Ray-Plane intersection
inline bool raycastPlane(const Ray& ray, const Vec3& planeNormal, float planeDistance,
                         float maxDistance, RaycastHit& hit, bool hitBackfaces = false)
{
    float denom = ray.direction.dot(planeNormal);
    
    // Check if ray is parallel to plane
    if (std::abs(denom) < 0.0001f) {
        return false;
    }
    
    // Check for backface
    if (!hitBackfaces && denom > 0) {
        return false;
    }
    
    float t = (planeDistance - ray.origin.dot(planeNormal)) / denom;
    
    if (t < 0.0001f || t > maxDistance) {
        return false;
    }
    
    hit.hit = true;
    hit.distance = t;
    hit.point = ray.getPoint(t);
    hit.normal = (denom < 0) ? planeNormal : planeNormal * -1.0f;
    
    return true;
}

// Ray-AABB intersection (used for broadphase and boxes)
inline bool raycastAABB(const Ray& ray, const Vec3& aabbMin, const Vec3& aabbMax,
                        float maxDistance, float& tNear, float& tFar)
{
    tNear = 0.0f;
    tFar = maxDistance;
    
    for (int i = 0; i < 3; i++) {
        float origin = (i == 0) ? ray.origin.x : (i == 1) ? ray.origin.y : ray.origin.z;
        float dir = (i == 0) ? ray.direction.x : (i == 1) ? ray.direction.y : ray.direction.z;
        float min = (i == 0) ? aabbMin.x : (i == 1) ? aabbMin.y : aabbMin.z;
        float max = (i == 0) ? aabbMax.x : (i == 1) ? aabbMax.y : aabbMax.z;
        
        if (std::abs(dir) < 0.0001f) {
            // Ray parallel to slab
            if (origin < min || origin > max) {
                return false;
            }
        } else {
            float t1 = (min - origin) / dir;
            float t2 = (max - origin) / dir;
            
            if (t1 > t2) std::swap(t1, t2);
            
            tNear = std::max(tNear, t1);
            tFar = std::min(tFar, t2);
            
            if (tNear > tFar || tFar < 0) {
                return false;
            }
        }
    }
    
    return tNear <= maxDistance;
}

// Ray-Box (OBB) intersection
inline bool raycastBox(const Ray& ray, const Vec3& center, const Vec3& halfExtents,
                       const Quat& rotation, float maxDistance, RaycastHit& hit)
{
    // Transform ray to box local space
    Quat invRot = rotation.conjugate();
    Vec3 localOrigin = invRot.rotate(ray.origin - center);
    Vec3 localDir = invRot.rotate(ray.direction);
    
    // Now do AABB test in local space
    float tNear, tFar;
    Vec3 aabbMin = halfExtents * -1.0f;
    Vec3 aabbMax = halfExtents;
    
    if (!raycastAABB(Ray(localOrigin, localDir), aabbMin, aabbMax, maxDistance, tNear, tFar)) {
        return false;
    }
    
    float t = (tNear > 0.0001f) ? tNear : tFar;
    if (t < 0.0001f || t > maxDistance) {
        return false;
    }
    
    hit.hit = true;
    hit.distance = t;
    hit.point = ray.getPoint(t);
    
    // Calculate normal in local space then transform
    Vec3 localHitPoint = localOrigin + localDir * t;
    Vec3 localNormal(0, 0, 0);
    
    // Find which face was hit
    float maxComp = -1.0f;
    for (int i = 0; i < 3; i++) {
        float comp = (i == 0) ? localHitPoint.x / halfExtents.x :
                     (i == 1) ? localHitPoint.y / halfExtents.y :
                                localHitPoint.z / halfExtents.z;
        
        if (std::abs(comp) > maxComp) {
            maxComp = std::abs(comp);
            localNormal = Vec3(
                (i == 0) ? (comp > 0 ? 1.0f : -1.0f) : 0.0f,
                (i == 1) ? (comp > 0 ? 1.0f : -1.0f) : 0.0f,
                (i == 2) ? (comp > 0 ? 1.0f : -1.0f) : 0.0f
            );
        }
    }
    
    hit.normal = rotation.rotate(localNormal);
    
    return true;
}

// Ray-Capsule intersection
inline bool raycastCapsule(const Ray& ray, const Vec3& center, float radius, float height,
                           const Quat& rotation, float maxDistance, RaycastHit& hit)
{
    // Capsule is defined by two hemispheres connected by a cylinder
    float halfHeight = (height - 2.0f * radius) * 0.5f;
    Vec3 up = rotation.rotate(Vec3(0, 1, 0));
    
    Vec3 p0 = center - up * halfHeight;  // Bottom hemisphere center
    Vec3 p1 = center + up * halfHeight;  // Top hemisphere center
    
    // Transform to capsule local space (capsule along Y axis)
    Quat invRot = rotation.conjugate();
    Vec3 localOrigin = invRot.rotate(ray.origin - center);
    Vec3 localDir = invRot.rotate(ray.direction);
    
    // Test infinite cylinder first
    float dx = localDir.x;
    float dz = localDir.z;
    float ox = localOrigin.x;
    float oz = localOrigin.z;
    
    float a = dx * dx + dz * dz;
    float b = 2.0f * (ox * dx + oz * dz);
    float c = ox * ox + oz * oz - radius * radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    float tMin = maxDistance + 1.0f;
    Vec3 hitNormal;
    
    // Check cylinder (if ray not parallel to cylinder axis)
    if (a > 0.0001f && discriminant >= 0) {
        float sqrtD = std::sqrt(discriminant);
        float t1 = (-b - sqrtD) / (2.0f * a);
        float t2 = (-b + sqrtD) / (2.0f * a);
        
        for (float t : {t1, t2}) {
            if (t > 0.0001f && t < tMin) {
                float y = localOrigin.y + localDir.y * t;
                if (y >= -halfHeight && y <= halfHeight) {
                    tMin = t;
                    Vec3 localHit = localOrigin + localDir * t;
                    hitNormal = rotation.rotate(Vec3(localHit.x, 0, localHit.z).normalized());
                }
            }
        }
    }
    
    // Check top hemisphere
    RaycastHit sphereHit;
    if (raycastSphere(ray, p1, radius, std::min(maxDistance, tMin), sphereHit)) {
        // Ensure we hit the top half
        Vec3 localHitOnSphere = invRot.rotate(sphereHit.point - p1);
        if (localHitOnSphere.y >= 0) {
            if (sphereHit.distance < tMin) {
                tMin = sphereHit.distance;
                hitNormal = sphereHit.normal;
            }
        }
    }
    
    // Check bottom hemisphere
    if (raycastSphere(ray, p0, radius, std::min(maxDistance, tMin), sphereHit)) {
        // Ensure we hit the bottom half
        Vec3 localHitOnSphere = invRot.rotate(sphereHit.point - p0);
        if (localHitOnSphere.y <= 0) {
            if (sphereHit.distance < tMin) {
                tMin = sphereHit.distance;
                hitNormal = sphereHit.normal;
            }
        }
    }
    
    if (tMin > maxDistance) {
        return false;
    }
    
    hit.hit = true;
    hit.distance = tMin;
    hit.point = ray.getPoint(tMin);
    hit.normal = hitNormal;
    
    return true;
}

// ===== Physics Raycaster =====
class PhysicsRaycaster {
public:
    // Single raycast - returns closest hit
    static RaycastHit raycast(const PhysicsWorld& world, const Ray& ray,
                              const RaycastOptions& options = RaycastOptions())
    {
        RaycastHit closestHit;
        
        for (const auto& body : world.getBodies()) {
            Collider* collider = body->getCollider();
            if (!collider) continue;
            
            // Check layer mask
            if (!(collider->getLayer() & options.layerMask)) continue;
            
            // Check trigger setting
            if (collider->isTrigger() && !options.hitTriggers) continue;
            
            // Broadphase: AABB test first
            AABB aabb = body->getAABB();
            float tNear, tFar;
            if (!raycastAABB(ray, aabb.min, aabb.max, options.maxDistance, tNear, tFar)) {
                continue;
            }
            
            // Narrowphase: precise shape test
            RaycastHit hit;
            Vec3 pos = body->getPosition() + body->getRotation().rotate(collider->getOffset());
            Quat rot = body->getRotation() * collider->getRotation();
            
            bool didHit = false;
            
            switch (collider->getType()) {
                case ColliderType::Sphere:
                    didHit = raycastSphere(ray, pos, collider->asSphere().radius,
                                          options.maxDistance, hit);
                    break;
                    
                case ColliderType::Box:
                    didHit = raycastBox(ray, pos, collider->asBox().halfExtents, rot,
                                       options.maxDistance, hit);
                    break;
                    
                case ColliderType::Capsule:
                    didHit = raycastCapsule(ray, pos, collider->asCapsule().radius,
                                           collider->asCapsule().height, rot,
                                           options.maxDistance, hit);
                    break;
                    
                case ColliderType::Plane:
                    didHit = raycastPlane(ray, collider->asPlane().normal,
                                         collider->asPlane().distance,
                                         options.maxDistance, hit, options.hitBackfaces);
                    break;
                    
                default:
                    break;
            }
            
            if (didHit && hit.distance < closestHit.distance) {
                closestHit = hit;
                closestHit.body = body.get();
                closestHit.collider = collider;
            }
        }
        
        return closestHit;
    }
    
    // Multi raycast - returns all hits
    static std::vector<RaycastHit> raycastAll(const PhysicsWorld& world, const Ray& ray,
                                               const RaycastOptions& options = RaycastOptions())
    {
        std::vector<RaycastHit> hits;
        
        for (const auto& body : world.getBodies()) {
            Collider* collider = body->getCollider();
            if (!collider) continue;
            
            if (!(collider->getLayer() & options.layerMask)) continue;
            if (collider->isTrigger() && !options.hitTriggers) continue;
            
            AABB aabb = body->getAABB();
            float tNear, tFar;
            if (!raycastAABB(ray, aabb.min, aabb.max, options.maxDistance, tNear, tFar)) {
                continue;
            }
            
            RaycastHit hit;
            Vec3 pos = body->getPosition() + body->getRotation().rotate(collider->getOffset());
            Quat rot = body->getRotation() * collider->getRotation();
            
            bool didHit = false;
            
            switch (collider->getType()) {
                case ColliderType::Sphere:
                    didHit = raycastSphere(ray, pos, collider->asSphere().radius,
                                          options.maxDistance, hit);
                    break;
                case ColliderType::Box:
                    didHit = raycastBox(ray, pos, collider->asBox().halfExtents, rot,
                                       options.maxDistance, hit);
                    break;
                case ColliderType::Capsule:
                    didHit = raycastCapsule(ray, pos, collider->asCapsule().radius,
                                           collider->asCapsule().height, rot,
                                           options.maxDistance, hit);
                    break;
                case ColliderType::Plane:
                    didHit = raycastPlane(ray, collider->asPlane().normal,
                                         collider->asPlane().distance,
                                         options.maxDistance, hit, options.hitBackfaces);
                    break;
                default:
                    break;
            }
            
            if (didHit) {
                hit.body = body.get();
                hit.collider = collider;
                hits.push_back(hit);
            }
        }
        
        if (options.sortByDistance) {
            std::sort(hits.begin(), hits.end());
        }
        
        return hits;
    }
    
    // Sphere cast (swept sphere)
    static RaycastHit sphereCast(const PhysicsWorld& world, const Ray& ray, float radius,
                                  const RaycastOptions& options = RaycastOptions())
    {
        RaycastHit closestHit;
        
        for (const auto& body : world.getBodies()) {
            Collider* collider = body->getCollider();
            if (!collider) continue;
            
            if (!(collider->getLayer() & options.layerMask)) continue;
            if (collider->isTrigger() && !options.hitTriggers) continue;
            
            // Expand AABB by sphere radius
            AABB aabb = body->getAABB();
            aabb.min.x -= radius; aabb.min.y -= radius; aabb.min.z -= radius;
            aabb.max.x += radius; aabb.max.y += radius; aabb.max.z += radius;
            
            float tNear, tFar;
            if (!raycastAABB(ray, aabb.min, aabb.max, options.maxDistance, tNear, tFar)) {
                continue;
            }
            
            RaycastHit hit;
            Vec3 pos = body->getPosition() + body->getRotation().rotate(collider->getOffset());
            
            bool didHit = false;
            
            // Sphere cast is equivalent to raycasting against expanded shapes
            switch (collider->getType()) {
                case ColliderType::Sphere: {
                    float expandedRadius = collider->asSphere().radius + radius;
                    didHit = raycastSphere(ray, pos, expandedRadius, options.maxDistance, hit);
                    if (didHit) {
                        // Adjust hit point to surface of swept sphere
                        hit.point = hit.point - hit.normal * radius;
                    }
                    break;
                }
                case ColliderType::Box: {
                    // Minkowski sum approximation - expand box and round corners
                    Vec3 expandedHalfExtents = collider->asBox().halfExtents;
                    expandedHalfExtents.x += radius;
                    expandedHalfExtents.y += radius;
                    expandedHalfExtents.z += radius;
                    
                    Quat rot = body->getRotation() * collider->getRotation();
                    didHit = raycastBox(ray, pos, expandedHalfExtents, rot, options.maxDistance, hit);
                    if (didHit) {
                        hit.point = hit.point - hit.normal * radius;
                    }
                    break;
                }
                case ColliderType::Capsule: {
                    float expandedRadius = collider->asCapsule().radius + radius;
                    Quat rot = body->getRotation() * collider->getRotation();
                    didHit = raycastCapsule(ray, pos, expandedRadius,
                                           collider->asCapsule().height + 2.0f * radius,
                                           rot, options.maxDistance, hit);
                    if (didHit) {
                        hit.point = hit.point - hit.normal * radius;
                    }
                    break;
                }
                case ColliderType::Plane: {
                    // Offset plane by radius
                    float expandedDistance = collider->asPlane().distance + radius;
                    didHit = raycastPlane(ray, collider->asPlane().normal, expandedDistance,
                                         options.maxDistance, hit, options.hitBackfaces);
                    break;
                }
                default:
                    break;
            }
            
            if (didHit && hit.distance < closestHit.distance) {
                closestHit = hit;
                closestHit.body = body.get();
                closestHit.collider = collider;
            }
        }
        
        return closestHit;
    }
    
    // Box cast (swept box)
    static RaycastHit boxCast(const PhysicsWorld& world, const Ray& ray,
                               const Vec3& halfExtents, const Quat& orientation,
                               const RaycastOptions& options = RaycastOptions())
    {
        // Simplified: use sphere cast with bounding sphere radius
        float boundingRadius = halfExtents.length();
        return sphereCast(world, ray, boundingRadius, options);
    }
};

// ===== Convenience Functions =====

inline RaycastHit physicsRaycast(const Vec3& origin, const Vec3& direction,
                                  float maxDistance = 1000.0f,
                                  CollisionMask layerMask = CollisionLayers::All)
{
    RaycastOptions options;
    options.maxDistance = maxDistance;
    options.layerMask = layerMask;
    
    return PhysicsRaycaster::raycast(getPhysicsWorld(), Ray(origin, direction), options);
}

inline std::vector<RaycastHit> physicsRaycastAll(const Vec3& origin, const Vec3& direction,
                                                  float maxDistance = 1000.0f,
                                                  CollisionMask layerMask = CollisionLayers::All)
{
    RaycastOptions options;
    options.maxDistance = maxDistance;
    options.layerMask = layerMask;
    
    return PhysicsRaycaster::raycastAll(getPhysicsWorld(), Ray(origin, direction), options);
}

inline RaycastHit physicsSphereCast(const Vec3& origin, const Vec3& direction, float radius,
                                     float maxDistance = 1000.0f,
                                     CollisionMask layerMask = CollisionLayers::All)
{
    RaycastOptions options;
    options.maxDistance = maxDistance;
    options.layerMask = layerMask;
    
    return PhysicsRaycaster::sphereCast(getPhysicsWorld(), Ray(origin, direction), radius, options);
}

}  // namespace luma
