// Collision Detection - Broadphase and Narrowphase
// Implements collision detection algorithms
#pragma once

#include "physics_world.h"
#include <algorithm>
#include <cmath>

namespace luma {

// ===== SAT (Separating Axis Theorem) Helpers =====
namespace sat {
    
// Project vertices onto axis and get min/max
inline void projectOntoAxis(const Vec3* vertices, int count, const Vec3& axis, float& outMin, float& outMax) {
    outMin = outMax = vertices[0].dot(axis);
    for (int i = 1; i < count; i++) {
        float proj = vertices[i].dot(axis);
        outMin = std::min(outMin, proj);
        outMax = std::max(outMax, proj);
    }
}

// Check overlap on single axis
inline bool axisOverlap(float minA, float maxA, float minB, float maxB, float& overlap) {
    if (maxA < minB || maxB < minA) return false;
    
    overlap = std::min(maxA - minB, maxB - minA);
    return true;
}

}  // namespace sat

// ===== GJK Support Functions =====
namespace gjk {

inline Vec3 supportSphere(const Vec3& center, float radius, const Vec3& direction) {
    return center + direction.normalized() * radius;
}

inline Vec3 supportBox(const Vec3& center, const Vec3& halfExtents, const Quat& rotation, const Vec3& direction) {
    // Transform direction to local space
    Vec3 localDir = rotation.conjugate().rotate(direction);
    
    // Support in local space
    Vec3 localSupport(
        localDir.x > 0 ? halfExtents.x : -halfExtents.x,
        localDir.y > 0 ? halfExtents.y : -halfExtents.y,
        localDir.z > 0 ? halfExtents.z : -halfExtents.z
    );
    
    // Transform back to world space
    return center + rotation.rotate(localSupport);
}

}  // namespace gjk

// ===== Collision Detection Functions =====

// Sphere vs Sphere
inline bool collisionSphereSphere(
    const Vec3& posA, float radiusA,
    const Vec3& posB, float radiusB,
    CollisionInfo& info)
{
    Vec3 d = posB - posA;
    float distSq = d.lengthSquared();
    float radiusSum = radiusA + radiusB;
    
    if (distSq > radiusSum * radiusSum) return false;
    
    float dist = std::sqrt(distSq);
    
    if (dist > 0.0001f) {
        info.normal = d * (1.0f / dist);
    } else {
        info.normal = Vec3(0, 1, 0);  // Arbitrary
    }
    
    info.penetration = radiusSum - dist;
    info.contactPoint = posA + info.normal * radiusA;
    info.contactPoints[0] = info.contactPoint;
    info.contactCount = 1;
    
    return true;
}

// Sphere vs Plane
inline bool collisionSpherePlane(
    const Vec3& spherePos, float radius,
    const Vec3& planeNormal, float planeDistance,
    CollisionInfo& info)
{
    float dist = spherePos.dot(planeNormal) - planeDistance;
    
    if (dist > radius) return false;
    
    info.normal = planeNormal;
    info.penetration = radius - dist;
    info.contactPoint = spherePos - planeNormal * dist;
    info.contactPoints[0] = info.contactPoint;
    info.contactCount = 1;
    
    return true;
}

// Point vs AABB (helper)
inline Vec3 closestPointOnAABB(const Vec3& point, const Vec3& aabbMin, const Vec3& aabbMax) {
    return Vec3(
        std::max(aabbMin.x, std::min(point.x, aabbMax.x)),
        std::max(aabbMin.y, std::min(point.y, aabbMax.y)),
        std::max(aabbMin.z, std::min(point.z, aabbMax.z))
    );
}

// Sphere vs Box (AABB for simplicity, can extend to OBB)
inline bool collisionSphereBox(
    const Vec3& spherePos, float radius,
    const Vec3& boxCenter, const Vec3& boxHalfExtents, const Quat& boxRotation,
    CollisionInfo& info)
{
    // Transform sphere to box local space
    Vec3 localSpherePos = boxRotation.conjugate().rotate(spherePos - boxCenter);
    
    // Find closest point on box in local space
    Vec3 closest(
        std::max(-boxHalfExtents.x, std::min(localSpherePos.x, boxHalfExtents.x)),
        std::max(-boxHalfExtents.y, std::min(localSpherePos.y, boxHalfExtents.y)),
        std::max(-boxHalfExtents.z, std::min(localSpherePos.z, boxHalfExtents.z))
    );
    
    Vec3 diff = localSpherePos - closest;
    float distSq = diff.lengthSquared();
    
    if (distSq > radius * radius) return false;
    
    float dist = std::sqrt(distSq);
    
    // Calculate normal
    Vec3 localNormal;
    if (dist > 0.0001f) {
        localNormal = diff * (1.0f / dist);
    } else {
        // Sphere center inside box - find minimum penetration axis
        float minDist = boxHalfExtents.x - std::abs(localSpherePos.x);
        localNormal = Vec3(localSpherePos.x > 0 ? 1 : -1, 0, 0);
        
        float distY = boxHalfExtents.y - std::abs(localSpherePos.y);
        if (distY < minDist) {
            minDist = distY;
            localNormal = Vec3(0, localSpherePos.y > 0 ? 1 : -1, 0);
        }
        
        float distZ = boxHalfExtents.z - std::abs(localSpherePos.z);
        if (distZ < minDist) {
            localNormal = Vec3(0, 0, localSpherePos.z > 0 ? 1 : -1);
        }
    }
    
    // Transform back to world space
    info.normal = boxRotation.rotate(localNormal);
    info.penetration = radius - dist;
    info.contactPoint = boxCenter + boxRotation.rotate(closest);
    info.contactPoints[0] = info.contactPoint;
    info.contactCount = 1;
    
    return true;
}

// Box vs Plane
inline bool collisionBoxPlane(
    const Vec3& boxCenter, const Vec3& boxHalfExtents, const Quat& boxRotation,
    const Vec3& planeNormal, float planeDistance,
    CollisionInfo& info)
{
    // Get box corners
    Vec3 corners[8];
    for (int i = 0; i < 8; i++) {
        Vec3 corner(
            (i & 1) ? boxHalfExtents.x : -boxHalfExtents.x,
            (i & 2) ? boxHalfExtents.y : -boxHalfExtents.y,
            (i & 4) ? boxHalfExtents.z : -boxHalfExtents.z
        );
        corners[i] = boxCenter + boxRotation.rotate(corner);
    }
    
    // Find deepest corner below plane
    float maxPen = 0.0f;
    int contactCount = 0;
    
    for (int i = 0; i < 8; i++) {
        float dist = corners[i].dot(planeNormal) - planeDistance;
        if (dist < 0) {
            if (-dist > maxPen) {
                maxPen = -dist;
            }
            if (contactCount < 4) {
                info.contactPoints[contactCount++] = corners[i] - planeNormal * dist;
            }
        }
    }
    
    if (contactCount == 0) return false;
    
    info.normal = planeNormal;
    info.penetration = maxPen;
    info.contactCount = contactCount;
    
    // Average contact point
    info.contactPoint = Vec3(0, 0, 0);
    for (int i = 0; i < contactCount; i++) {
        info.contactPoint = info.contactPoint + info.contactPoints[i];
    }
    info.contactPoint = info.contactPoint * (1.0f / contactCount);
    
    return true;
}

// Box vs Box (SAT-based)
inline bool collisionBoxBox(
    const Vec3& centerA, const Vec3& halfExtentsA, const Quat& rotationA,
    const Vec3& centerB, const Vec3& halfExtentsB, const Quat& rotationB,
    CollisionInfo& info)
{
    // Get local axes
    Vec3 axesA[3] = {
        rotationA.rotate(Vec3(1, 0, 0)),
        rotationA.rotate(Vec3(0, 1, 0)),
        rotationA.rotate(Vec3(0, 0, 1))
    };
    Vec3 axesB[3] = {
        rotationB.rotate(Vec3(1, 0, 0)),
        rotationB.rotate(Vec3(0, 1, 0)),
        rotationB.rotate(Vec3(0, 0, 1))
    };
    
    // Direction between centers
    Vec3 d = centerB - centerA;
    
    // Test all 15 separating axes
    float minOverlap = 1e30f;
    Vec3 minAxis;
    
    // Test 6 face axes
    for (int i = 0; i < 3; i++) {
        float overlap;
        float projA = halfExtentsA.x * std::abs(axesA[i].dot(axesA[0])) +
                      halfExtentsA.y * std::abs(axesA[i].dot(axesA[1])) +
                      halfExtentsA.z * std::abs(axesA[i].dot(axesA[2]));
        float projB = halfExtentsB.x * std::abs(axesA[i].dot(axesB[0])) +
                      halfExtentsB.y * std::abs(axesA[i].dot(axesB[1])) +
                      halfExtentsB.z * std::abs(axesA[i].dot(axesB[2]));
        float dist = std::abs(d.dot(axesA[i]));
        
        overlap = projA + projB - dist;
        if (overlap < 0) return false;
        
        if (overlap < minOverlap) {
            minOverlap = overlap;
            minAxis = axesA[i];
            if (d.dot(minAxis) < 0) minAxis = minAxis * -1.0f;
        }
    }
    
    for (int i = 0; i < 3; i++) {
        float projA = halfExtentsA.x * std::abs(axesB[i].dot(axesA[0])) +
                      halfExtentsA.y * std::abs(axesB[i].dot(axesA[1])) +
                      halfExtentsA.z * std::abs(axesB[i].dot(axesA[2]));
        float projB = halfExtentsB.x * std::abs(axesB[i].dot(axesB[0])) +
                      halfExtentsB.y * std::abs(axesB[i].dot(axesB[1])) +
                      halfExtentsB.z * std::abs(axesB[i].dot(axesB[2]));
        float dist = std::abs(d.dot(axesB[i]));
        
        float overlap = projA + projB - dist;
        if (overlap < 0) return false;
        
        if (overlap < minOverlap) {
            minOverlap = overlap;
            minAxis = axesB[i];
            if (d.dot(minAxis) < 0) minAxis = minAxis * -1.0f;
        }
    }
    
    // Test 9 edge-edge axes
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vec3 axis = axesA[i].cross(axesB[j]);
            float len = axis.length();
            if (len < 0.0001f) continue;
            axis = axis * (1.0f / len);
            
            float projA = halfExtentsA.x * std::abs(axis.dot(axesA[0])) +
                          halfExtentsA.y * std::abs(axis.dot(axesA[1])) +
                          halfExtentsA.z * std::abs(axis.dot(axesA[2]));
            float projB = halfExtentsB.x * std::abs(axis.dot(axesB[0])) +
                          halfExtentsB.y * std::abs(axis.dot(axesB[1])) +
                          halfExtentsB.z * std::abs(axis.dot(axesB[2]));
            float dist = std::abs(d.dot(axis));
            
            float overlap = projA + projB - dist;
            if (overlap < 0) return false;
            
            if (overlap < minOverlap) {
                minOverlap = overlap;
                minAxis = axis;
                if (d.dot(minAxis) < 0) minAxis = minAxis * -1.0f;
            }
        }
    }
    
    info.normal = minAxis;
    info.penetration = minOverlap;
    
    // Calculate contact point (center of overlap region)
    info.contactPoint = (centerA + centerB) * 0.5f;
    info.contactPoints[0] = info.contactPoint;
    info.contactCount = 1;
    
    return true;
}

// Capsule vs Sphere
inline bool collisionCapsuleSphere(
    const Vec3& capsulePos, float capsuleRadius, float capsuleHeight, const Quat& capsuleRot,
    const Vec3& spherePos, float sphereRadius,
    CollisionInfo& info)
{
    // Capsule line segment in world space
    float halfHeight = (capsuleHeight - 2.0f * capsuleRadius) * 0.5f;
    Vec3 capsuleUp = capsuleRot.rotate(Vec3(0, 1, 0));
    Vec3 p0 = capsulePos - capsuleUp * halfHeight;
    Vec3 p1 = capsulePos + capsuleUp * halfHeight;
    
    // Find closest point on line segment to sphere center
    Vec3 d = p1 - p0;
    float t = std::max(0.0f, std::min(1.0f, (spherePos - p0).dot(d) / d.lengthSquared()));
    Vec3 closest = p0 + d * t;
    
    // Now it's sphere vs sphere
    return collisionSphereSphere(closest, capsuleRadius, spherePos, sphereRadius, info);
}

// ===== PhysicsWorld Implementation =====

inline void PhysicsWorld::fixedStep(float dt) {
    // 1. Integrate forces
    for (auto& body : bodies_) {
        body->integrateForces(dt, settings_.gravity);
    }
    
    // 2. Broadphase
    broadphase();
    
    // 3. Narrowphase
    narrowphase();
    
    // 4. Resolve collisions
    for (int i = 0; i < settings_.velocityIterations; i++) {
        resolveCollisions();
    }
    
    // 5. Integrate velocities
    integrateVelocities(dt);
    
    // 6. Position correction
    for (int i = 0; i < settings_.positionIterations; i++) {
        for (auto& col : collisions_) {
            if (col.bodyA->getType() == RigidBodyType::Static && 
                col.bodyB->getType() == RigidBodyType::Static) continue;
            
            // Simple position correction
            float totalInvMass = col.bodyA->getInverseMass() + col.bodyB->getInverseMass();
            if (totalInvMass <= 0.0f) continue;
            
            float correction = col.penetration * 0.2f / totalInvMass;
            
            if (col.bodyA->getType() == RigidBodyType::Dynamic) {
                Vec3 posA = col.bodyA->getPosition() - col.normal * correction * col.bodyA->getInverseMass();
                col.bodyA->setPosition(posA);
            }
            if (col.bodyB->getType() == RigidBodyType::Dynamic) {
                Vec3 posB = col.bodyB->getPosition() + col.normal * correction * col.bodyB->getInverseMass();
                col.bodyB->setPosition(posB);
            }
        }
    }
    
    // 7. Update sleeping
    if (settings_.enableSleeping) {
        updateSleeping(dt);
    }
    
    // 8. Clear forces
    for (auto& body : bodies_) {
        body->clearForces();
    }
}

inline void PhysicsWorld::broadphase() {
    broadphasePairs_.clear();
    
    // Simple O(n²) check - can be optimized with spatial partitioning
    for (size_t i = 0; i < bodies_.size(); i++) {
        if (!bodies_[i]->getCollider()) continue;
        
        AABB aabbA = bodies_[i]->getAABB();
        
        for (size_t j = i + 1; j < bodies_.size(); j++) {
            if (!bodies_[j]->getCollider()) continue;
            
            // Skip static vs static
            if (bodies_[i]->getType() == RigidBodyType::Static &&
                bodies_[j]->getType() == RigidBodyType::Static) continue;
            
            // Check collision layers
            if (!bodies_[i]->getCollider()->canCollideWith(*bodies_[j]->getCollider())) continue;
            
            AABB aabbB = bodies_[j]->getAABB();
            
            if (aabbA.intersects(aabbB)) {
                broadphasePairs_.push_back({(uint32_t)i, (uint32_t)j});
            }
        }
    }
}

inline void PhysicsWorld::narrowphase() {
    collisions_.clear();
    
    for (const auto& pair : broadphasePairs_) {
        RigidBody* bodyA = bodies_[pair.first].get();
        RigidBody* bodyB = bodies_[pair.second].get();
        
        CollisionInfo info;
        info.bodyA = bodyA;
        info.bodyB = bodyB;
        
        Collider* colA = bodyA->getCollider();
        Collider* colB = bodyB->getCollider();
        
        bool collision = false;
        
        // Dispatch based on collider types
        ColliderType typeA = colA->getType();
        ColliderType typeB = colB->getType();
        
        if (typeA == ColliderType::Sphere && typeB == ColliderType::Sphere) {
            collision = collisionSphereSphere(
                bodyA->getPosition() + bodyA->getRotation().rotate(colA->getOffset()),
                colA->asSphere().radius,
                bodyB->getPosition() + bodyB->getRotation().rotate(colB->getOffset()),
                colB->asSphere().radius,
                info
            );
        }
        else if (typeA == ColliderType::Sphere && typeB == ColliderType::Box) {
            collision = collisionSphereBox(
                bodyA->getPosition() + bodyA->getRotation().rotate(colA->getOffset()),
                colA->asSphere().radius,
                bodyB->getPosition() + bodyB->getRotation().rotate(colB->getOffset()),
                colB->asBox().halfExtents,
                bodyB->getRotation() * colB->getRotation(),
                info
            );
        }
        else if (typeA == ColliderType::Box && typeB == ColliderType::Sphere) {
            collision = collisionSphereBox(
                bodyB->getPosition() + bodyB->getRotation().rotate(colB->getOffset()),
                colB->asSphere().radius,
                bodyA->getPosition() + bodyA->getRotation().rotate(colA->getOffset()),
                colA->asBox().halfExtents,
                bodyA->getRotation() * colA->getRotation(),
                info
            );
            if (collision) {
                std::swap(info.bodyA, info.bodyB);
                info.normal = info.normal * -1.0f;
            }
        }
        else if (typeA == ColliderType::Box && typeB == ColliderType::Box) {
            collision = collisionBoxBox(
                bodyA->getPosition() + bodyA->getRotation().rotate(colA->getOffset()),
                colA->asBox().halfExtents,
                bodyA->getRotation() * colA->getRotation(),
                bodyB->getPosition() + bodyB->getRotation().rotate(colB->getOffset()),
                colB->asBox().halfExtents,
                bodyB->getRotation() * colB->getRotation(),
                info
            );
        }
        else if (typeA == ColliderType::Sphere && typeB == ColliderType::Plane) {
            collision = collisionSpherePlane(
                bodyA->getPosition() + bodyA->getRotation().rotate(colA->getOffset()),
                colA->asSphere().radius,
                colB->asPlane().normal,
                colB->asPlane().distance,
                info
            );
        }
        else if (typeA == ColliderType::Plane && typeB == ColliderType::Sphere) {
            collision = collisionSpherePlane(
                bodyB->getPosition() + bodyB->getRotation().rotate(colB->getOffset()),
                colB->asSphere().radius,
                colA->asPlane().normal,
                colA->asPlane().distance,
                info
            );
            if (collision) {
                std::swap(info.bodyA, info.bodyB);
                info.normal = info.normal * -1.0f;
            }
        }
        else if (typeA == ColliderType::Box && typeB == ColliderType::Plane) {
            collision = collisionBoxPlane(
                bodyA->getPosition() + bodyA->getRotation().rotate(colA->getOffset()),
                colA->asBox().halfExtents,
                bodyA->getRotation() * colA->getRotation(),
                colB->asPlane().normal,
                colB->asPlane().distance,
                info
            );
        }
        else if (typeA == ColliderType::Plane && typeB == ColliderType::Box) {
            collision = collisionBoxPlane(
                bodyB->getPosition() + bodyB->getRotation().rotate(colB->getOffset()),
                colB->asBox().halfExtents,
                bodyB->getRotation() * colB->getRotation(),
                colA->asPlane().normal,
                colA->asPlane().distance,
                info
            );
            if (collision) {
                std::swap(info.bodyA, info.bodyB);
                info.normal = info.normal * -1.0f;
            }
        }
        
        if (collision) {
            // Handle triggers
            bool isTrigger = colA->isTrigger() || colB->isTrigger();
            uint64_t triggerKey = ((uint64_t)bodyA->getId() << 32) | bodyB->getId();
            
            if (isTrigger) {
                if (activeTriggers_.find(triggerKey) == activeTriggers_.end()) {
                    activeTriggers_.insert(triggerKey);
                    if (triggerEnterCallback_) {
                        triggerEnterCallback_(bodyA, bodyB);
                    }
                }
            } else {
                collisions_.push_back(info);
                if (collisionCallback_) {
                    collisionCallback_(info);
                }
            }
        }
    }
    
    // Check for trigger exits
    std::vector<uint64_t> toRemove;
    for (uint64_t key : activeTriggers_) {
        uint32_t idA = (uint32_t)(key >> 32);
        uint32_t idB = (uint32_t)(key & 0xFFFFFFFF);
        
        bool stillOverlapping = false;
        for (const auto& pair : broadphasePairs_) {
            if ((bodies_[pair.first]->getId() == idA && bodies_[pair.second]->getId() == idB) ||
                (bodies_[pair.first]->getId() == idB && bodies_[pair.second]->getId() == idA)) {
                stillOverlapping = true;
                break;
            }
        }
        
        if (!stillOverlapping) {
            toRemove.push_back(key);
        }
    }
    
    for (uint64_t key : toRemove) {
        activeTriggers_.erase(key);
        if (triggerExitCallback_) {
            // Find bodies by ID
            RigidBody* bodyA = nullptr;
            RigidBody* bodyB = nullptr;
            uint32_t idA = (uint32_t)(key >> 32);
            uint32_t idB = (uint32_t)(key & 0xFFFFFFFF);
            
            for (auto& body : bodies_) {
                if (body->getId() == idA) bodyA = body.get();
                if (body->getId() == idB) bodyB = body.get();
            }
            
            if (bodyA && bodyB) {
                triggerExitCallback_(bodyA, bodyB);
            }
        }
    }
}

inline void PhysicsWorld::resolveCollisions() {
    for (auto& col : collisions_) {
        RigidBody* a = col.bodyA;
        RigidBody* b = col.bodyB;
        
        if (a->isSleeping() && b->isSleeping()) continue;
        
        // Calculate relative velocity
        Vec3 relVel = b->getLinearVelocity() - a->getLinearVelocity();
        float velAlongNormal = relVel.dot(col.normal);
        
        // Don't resolve if velocities are separating
        if (velAlongNormal > 0) continue;
        
        // Calculate restitution
        float e = std::min(a->getRestitution(), b->getRestitution());
        
        // Calculate impulse magnitude
        float invMassSum = a->getInverseMass() + b->getInverseMass();
        if (invMassSum <= 0.0f) continue;
        
        float j = -(1.0f + e) * velAlongNormal / invMassSum;
        
        // Apply impulse
        Vec3 impulse = col.normal * j;
        
        if (a->getType() == RigidBodyType::Dynamic) {
            a->addImpulse(impulse * -1.0f);
        }
        if (b->getType() == RigidBodyType::Dynamic) {
            b->addImpulse(impulse);
        }
        
        // Friction
        Vec3 tangent = relVel - col.normal * velAlongNormal;
        float tangentLen = tangent.length();
        if (tangentLen > 0.0001f) {
            tangent = tangent * (1.0f / tangentLen);
            
            float friction = std::sqrt(a->getFriction() * b->getFriction());
            float jt = -relVel.dot(tangent) / invMassSum;
            
            // Coulomb friction
            Vec3 frictionImpulse;
            if (std::abs(jt) < j * friction) {
                frictionImpulse = tangent * jt;
            } else {
                frictionImpulse = tangent * (-j * friction);
            }
            
            if (a->getType() == RigidBodyType::Dynamic) {
                a->addImpulse(frictionImpulse * -1.0f);
            }
            if (b->getType() == RigidBodyType::Dynamic) {
                b->addImpulse(frictionImpulse);
            }
        }
    }
}

inline void PhysicsWorld::integrateVelocities(float dt) {
    for (auto& body : bodies_) {
        body->integrateVelocity(dt);
    }
}

inline void PhysicsWorld::updateSleeping(float dt) {
    for (auto& body : bodies_) {
        body->updateSleeping(dt, settings_.sleepThreshold, settings_.sleepTime);
    }
}

// ===== Queries =====

inline RigidBody* PhysicsWorld::raycast(const Vec3& origin, const Vec3& direction, float maxDistance,
                                        Vec3* hitPoint, Vec3* hitNormal) {
    Vec3 dir = direction.normalized();
    RigidBody* closest = nullptr;
    float closestDist = maxDistance;
    
    for (auto& body : bodies_) {
        Collider* col = body->getCollider();
        if (!col) continue;
        
        Vec3 pos = body->getPosition() + body->getRotation().rotate(col->getOffset());
        
        // Simple sphere raycast for all shapes (conservative)
        float radius = 0.5f;
        switch (col->getType()) {
            case ColliderType::Sphere:
                radius = col->asSphere().radius;
                break;
            case ColliderType::Box:
                radius = col->asBox().halfExtents.length();
                break;
            default:
                break;
        }
        
        Vec3 oc = origin - pos;
        float b = oc.dot(dir);
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b * b - c;
        
        if (discriminant > 0) {
            float t = -b - std::sqrt(discriminant);
            if (t > 0 && t < closestDist) {
                closestDist = t;
                closest = body.get();
                
                if (hitPoint) *hitPoint = origin + dir * t;
                if (hitNormal) {
                    Vec3 hp = origin + dir * t;
                    *hitNormal = (hp - pos).normalized();
                }
            }
        }
    }
    
    return closest;
}

inline std::vector<RigidBody*> PhysicsWorld::queryAABB(const AABB& aabb) {
    std::vector<RigidBody*> result;
    
    for (auto& body : bodies_) {
        if (body->getAABB().intersects(aabb)) {
            result.push_back(body.get());
        }
    }
    
    return result;
}

inline std::vector<RigidBody*> PhysicsWorld::querySphere(const Vec3& center, float radius) {
    std::vector<RigidBody*> result;
    
    AABB queryAABB(
        Vec3(center.x - radius, center.y - radius, center.z - radius),
        Vec3(center.x + radius, center.y + radius, center.z + radius)
    );
    
    for (auto& body : bodies_) {
        if (!body->getAABB().intersects(queryAABB)) continue;
        
        Vec3 bodyPos = body->getPosition();
        float distSq = (bodyPos - center).lengthSquared();
        
        // Add body radius estimate
        float bodyRadius = 0.5f;
        if (body->getCollider()) {
            switch (body->getCollider()->getType()) {
                case ColliderType::Sphere:
                    bodyRadius = body->getCollider()->asSphere().radius;
                    break;
                case ColliderType::Box:
                    bodyRadius = body->getCollider()->asBox().halfExtents.length();
                    break;
                default:
                    break;
            }
        }
        
        float totalRadius = radius + bodyRadius;
        if (distSq <= totalRadius * totalRadius) {
            result.push_back(body.get());
        }
    }
    
    return result;
}

}  // namespace luma
