// Physics World - Core Physics System
// Rigid body dynamics, collision detection, and constraints
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace luma {

// Forward declarations
class RigidBody;
class Collider;
struct CollisionInfo;
class PhysicsWorld;

// ===== Physics Settings =====
struct PhysicsSettings {
    Vec3 gravity = {0.0f, -9.81f, 0.0f};
    int velocityIterations = 8;
    int positionIterations = 3;
    float fixedTimeStep = 1.0f / 60.0f;
    float maxDeltaTime = 0.25f;  // Max frame time to prevent spiral of death
    bool enableSleeping = true;
    float sleepThreshold = 0.01f;
    float sleepTime = 0.5f;
    float defaultRestitution = 0.3f;
    float defaultFriction = 0.5f;
    float linearDamping = 0.01f;
    float angularDamping = 0.05f;
};

// ===== Collision Layers =====
using CollisionMask = uint32_t;

namespace CollisionLayers {
    constexpr CollisionMask Default = 1 << 0;
    constexpr CollisionMask Static = 1 << 1;
    constexpr CollisionMask Dynamic = 1 << 2;
    constexpr CollisionMask Kinematic = 1 << 3;
    constexpr CollisionMask Trigger = 1 << 4;
    constexpr CollisionMask Player = 1 << 5;
    constexpr CollisionMask Enemy = 1 << 6;
    constexpr CollisionMask Projectile = 1 << 7;
    constexpr CollisionMask All = 0xFFFFFFFF;
}

// ===== Rigid Body Type =====
enum class RigidBodyType {
    Static,     // Doesn't move, infinite mass
    Dynamic,    // Affected by forces and collisions
    Kinematic   // Moved by code, affects dynamic bodies
};

// ===== Collider Shape Type =====
enum class ColliderType {
    Sphere,
    Box,
    Capsule,
    Plane,
    Mesh,       // Convex mesh
    Compound    // Multiple shapes
};

// ===== AABB for Broadphase =====
#ifndef LUMA_AABB_DEFINED
#define LUMA_AABB_DEFINED
struct AABB {
    Vec3 min;
    Vec3 max;
    
    AABB() : min(0, 0, 0), max(0, 0, 0) {}
    AABB(const Vec3& minPt, const Vec3& maxPt) : min(minPt), max(maxPt) {}
    
    Vec3 getCenter() const {
        return Vec3(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f
        );
    }
    
    Vec3 getExtents() const {
        return Vec3(
            (max.x - min.x) * 0.5f,
            (max.y - min.y) * 0.5f,
            (max.z - min.z) * 0.5f
        );
    }
    
    void expand(const Vec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }
    
    void expand(float margin) {
        min.x -= margin; min.y -= margin; min.z -= margin;
        max.x += margin; max.y += margin; max.z += margin;
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
};
#endif  // LUMA_AABB_DEFINED

// ===== Collision Info =====
struct CollisionInfo {
    RigidBody* bodyA = nullptr;
    RigidBody* bodyB = nullptr;
    Vec3 contactPoint;
    Vec3 normal;        // Points from A to B
    float penetration = 0.0f;
    
    // Multiple contact points for stable stacking
    Vec3 contactPoints[4];
    int contactCount = 0;
};

// ===== Collider Shape Data =====
struct SphereShape {
    float radius = 0.5f;
};

struct BoxShape {
    Vec3 halfExtents = {0.5f, 0.5f, 0.5f};
};

struct CapsuleShape {
    float radius = 0.25f;
    float height = 1.0f;  // Total height including caps
};

struct PlaneShape {
    Vec3 normal = {0, 1, 0};
    float distance = 0.0f;
};

struct MeshShape {
    std::vector<Vec3> vertices;
    std::vector<uint32_t> indices;
    // Computed on initialization
    std::vector<Vec3> normals;
    AABB bounds;
};

// ===== Collider =====
class Collider {
public:
    Collider(ColliderType type = ColliderType::Box)
        : type_(type), offset_(0, 0, 0), rotation_(Quat::identity()),
          isTrigger_(false), layer_(CollisionLayers::Default),
          mask_(CollisionLayers::All) {}
    
    ColliderType getType() const { return type_; }
    
    // Shape data
    SphereShape& asSphere() { return sphere_; }
    const SphereShape& asSphere() const { return sphere_; }
    
    BoxShape& asBox() { return box_; }
    const BoxShape& asBox() const { return box_; }
    
    CapsuleShape& asCapsule() { return capsule_; }
    const CapsuleShape& asCapsule() const { return capsule_; }
    
    PlaneShape& asPlane() { return plane_; }
    const PlaneShape& asPlane() const { return plane_; }
    
    MeshShape& asMesh() { return mesh_; }
    const MeshShape& asMesh() const { return mesh_; }
    
    // Transform
    void setOffset(const Vec3& offset) { offset_ = offset; }
    Vec3 getOffset() const { return offset_; }
    
    void setRotation(const Quat& rot) { rotation_ = rot; }
    Quat getRotation() const { return rotation_; }
    
    // Trigger (no physics response, only events)
    void setTrigger(bool trigger) { isTrigger_ = trigger; }
    bool isTrigger() const { return isTrigger_; }
    
    // Collision filtering
    void setLayer(CollisionMask layer) { layer_ = layer; }
    CollisionMask getLayer() const { return layer_; }
    
    void setMask(CollisionMask mask) { mask_ = mask; }
    CollisionMask getMask() const { return mask_; }
    
    bool canCollideWith(const Collider& other) const {
        return (layer_ & other.mask_) && (other.layer_ & mask_);
    }
    
    // Compute AABB in world space
    AABB computeAABB(const Vec3& position, const Quat& rotation) const {
        Vec3 worldOffset = rotation.rotate(offset_);
        Vec3 center = position + worldOffset;
        
        switch (type_) {
            case ColliderType::Sphere: {
                float r = sphere_.radius;
                return AABB(
                    Vec3(center.x - r, center.y - r, center.z - r),
                    Vec3(center.x + r, center.y + r, center.z + r)
                );
            }
            case ColliderType::Box: {
                // Rotated box AABB (conservative)
                Quat totalRot = rotation * rotation_;
                Vec3 corners[8];
                Vec3 h = box_.halfExtents;
                
                corners[0] = totalRot.rotate(Vec3(-h.x, -h.y, -h.z));
                corners[1] = totalRot.rotate(Vec3( h.x, -h.y, -h.z));
                corners[2] = totalRot.rotate(Vec3(-h.x,  h.y, -h.z));
                corners[3] = totalRot.rotate(Vec3( h.x,  h.y, -h.z));
                corners[4] = totalRot.rotate(Vec3(-h.x, -h.y,  h.z));
                corners[5] = totalRot.rotate(Vec3( h.x, -h.y,  h.z));
                corners[6] = totalRot.rotate(Vec3(-h.x,  h.y,  h.z));
                corners[7] = totalRot.rotate(Vec3( h.x,  h.y,  h.z));
                
                AABB aabb(center, center);
                for (int i = 0; i < 8; i++) {
                    aabb.expand(center + corners[i]);
                }
                return aabb;
            }
            case ColliderType::Capsule: {
                float r = capsule_.radius;
                float h = capsule_.height * 0.5f;
                return AABB(
                    Vec3(center.x - r, center.y - h, center.z - r),
                    Vec3(center.x + r, center.y + h, center.z + r)
                );
            }
            case ColliderType::Plane: {
                // Infinite plane - use large AABB
                float inf = 1000000.0f;
                if (std::abs(plane_.normal.y) > 0.9f) {
                    return AABB(
                        Vec3(-inf, -0.01f, -inf),
                        Vec3(inf, 0.01f, inf)
                    );
                }
                return AABB(Vec3(-inf, -inf, -inf), Vec3(inf, inf, inf));
            }
            default:
                return AABB(center, center);
        }
    }
    
private:
    ColliderType type_;
    Vec3 offset_;
    Quat rotation_;
    bool isTrigger_;
    CollisionMask layer_;
    CollisionMask mask_;
    
    // Shape data (union-like, only one is valid based on type_)
    SphereShape sphere_;
    BoxShape box_;
    CapsuleShape capsule_;
    PlaneShape plane_;
    MeshShape mesh_;
};

// ===== Rigid Body =====
class RigidBody {
public:
    RigidBody(RigidBodyType type = RigidBodyType::Dynamic)
        : type_(type), mass_(1.0f), invMass_(1.0f),
          position_(0, 0, 0), rotation_(Quat::identity()),
          linearVelocity_(0, 0, 0), angularVelocity_(0, 0, 0),
          force_(0, 0, 0), torque_(0, 0, 0),
          restitution_(0.3f), friction_(0.5f),
          linearDamping_(0.01f), angularDamping_(0.05f),
          isSleeping_(false), sleepTimer_(0.0f),
          userData_(nullptr), id_(nextId_++) {}
    
    // Type
    RigidBodyType getType() const { return type_; }
    void setType(RigidBodyType type) {
        type_ = type;
        if (type_ == RigidBodyType::Static) {
            invMass_ = 0.0f;
            invInertiaTensor_ = Mat3();
        }
    }
    
    // Mass
    void setMass(float mass) {
        mass_ = mass;
        invMass_ = (type_ == RigidBodyType::Static || mass <= 0.0f) ? 0.0f : 1.0f / mass;
        computeInertiaTensor();
    }
    float getMass() const { return mass_; }
    float getInverseMass() const { return invMass_; }
    
    // Transform
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    void setRotation(const Quat& rot) { rotation_ = rot.normalized(); }
    Quat getRotation() const { return rotation_; }
    
    // Velocity
    void setLinearVelocity(const Vec3& vel) { linearVelocity_ = vel; }
    Vec3 getLinearVelocity() const { return linearVelocity_; }
    
    void setAngularVelocity(const Vec3& vel) { angularVelocity_ = vel; }
    Vec3 getAngularVelocity() const { return angularVelocity_; }
    
    // Forces
    void addForce(const Vec3& force) {
        force_ = force_ + force;
        wakeUp();
    }
    
    void addForceAtPoint(const Vec3& force, const Vec3& point) {
        force_ = force_ + force;
        Vec3 r = point - position_;
        torque_ = torque_ + r.cross(force);
        wakeUp();
    }
    
    void addTorque(const Vec3& torque) {
        torque_ = torque_ + torque;
        wakeUp();
    }
    
    void addImpulse(const Vec3& impulse) {
        if (type_ == RigidBodyType::Static) return;
        linearVelocity_ = linearVelocity_ + impulse * invMass_;
        wakeUp();
    }
    
    void addImpulseAtPoint(const Vec3& impulse, const Vec3& point) {
        if (type_ == RigidBodyType::Static) return;
        linearVelocity_ = linearVelocity_ + impulse * invMass_;
        Vec3 r = point - position_;
        angularVelocity_ = angularVelocity_ + invInertiaTensor_ * r.cross(impulse);
        wakeUp();
    }
    
    void clearForces() {
        force_ = Vec3(0, 0, 0);
        torque_ = Vec3(0, 0, 0);
    }
    
    // Material
    void setRestitution(float r) { restitution_ = std::max(0.0f, std::min(1.0f, r)); }
    float getRestitution() const { return restitution_; }
    
    void setFriction(float f) { friction_ = std::max(0.0f, f); }
    float getFriction() const { return friction_; }
    
    // Damping
    void setLinearDamping(float d) { linearDamping_ = d; }
    float getLinearDamping() const { return linearDamping_; }
    
    void setAngularDamping(float d) { angularDamping_ = d; }
    float getAngularDamping() const { return angularDamping_; }
    
    // Sleeping
    bool isSleeping() const { return isSleeping_; }
    void wakeUp() { isSleeping_ = false; sleepTimer_ = 0.0f; }
    void putToSleep() { isSleeping_ = true; linearVelocity_ = angularVelocity_ = Vec3(0, 0, 0); }
    
    // Collider
    void setCollider(std::shared_ptr<Collider> collider) { 
        collider_ = collider;
        computeInertiaTensor();
    }
    Collider* getCollider() { return collider_.get(); }
    const Collider* getCollider() const { return collider_.get(); }
    
    // User data
    void setUserData(void* data) { userData_ = data; }
    void* getUserData() const { return userData_; }
    
    uint32_t getId() const { return id_; }
    
    // AABB
    AABB getAABB() const {
        if (collider_) {
            return collider_->computeAABB(position_, rotation_);
        }
        return AABB(position_, position_);
    }
    
    // Integrate
    void integrateForces(float dt, const Vec3& gravity) {
        if (type_ != RigidBodyType::Dynamic || isSleeping_) return;
        
        // Apply gravity
        Vec3 acceleration = gravity + force_ * invMass_;
        linearVelocity_ = linearVelocity_ + acceleration * dt;
        
        // Angular
        Vec3 angularAccel = invInertiaTensor_ * torque_;
        angularVelocity_ = angularVelocity_ + angularAccel * dt;
    }
    
    void integrateVelocity(float dt) {
        if (type_ == RigidBodyType::Static || isSleeping_) return;
        
        // Apply damping
        linearVelocity_ = linearVelocity_ * (1.0f - linearDamping_);
        angularVelocity_ = angularVelocity_ * (1.0f - angularDamping_);
        
        // Integrate position
        position_ = position_ + linearVelocity_ * dt;
        
        // Integrate rotation
        if (angularVelocity_.lengthSquared() > 0.0001f) {
            float angle = angularVelocity_.length() * dt;
            Vec3 axis = angularVelocity_.normalized();
            Quat deltaRot = Quat::fromAxisAngle(axis, angle);
            rotation_ = (deltaRot * rotation_).normalized();
        }
    }
    
    void updateSleeping(float dt, float threshold, float sleepTime) {
        if (type_ != RigidBodyType::Dynamic) return;
        
        float energy = linearVelocity_.lengthSquared() + angularVelocity_.lengthSquared();
        if (energy < threshold * threshold) {
            sleepTimer_ += dt;
            if (sleepTimer_ >= sleepTime) {
                putToSleep();
            }
        } else {
            sleepTimer_ = 0.0f;
        }
    }
    
    // Inertia tensor (simplified - assumes box shape for now)
    const Mat3& getInverseInertiaTensor() const { return invInertiaTensor_; }
    
private:
    void computeInertiaTensor() {
        if (type_ == RigidBodyType::Static || invMass_ == 0.0f) {
            invInertiaTensor_ = Mat3();
            return;
        }
        
        // Default box inertia
        float w = 1.0f, h = 1.0f, d = 1.0f;
        if (collider_) {
            switch (collider_->getType()) {
                case ColliderType::Box:
                    w = collider_->asBox().halfExtents.x * 2.0f;
                    h = collider_->asBox().halfExtents.y * 2.0f;
                    d = collider_->asBox().halfExtents.z * 2.0f;
                    break;
                case ColliderType::Sphere: {
                    float r = collider_->asSphere().radius;
                    float i = 2.0f / 5.0f * mass_ * r * r;
                    invInertiaTensor_ = Mat3::identity() * (1.0f / i);
                    return;
                }
                default:
                    break;
            }
        }
        
        // Box inertia tensor
        float ix = mass_ * (h * h + d * d) / 12.0f;
        float iy = mass_ * (w * w + d * d) / 12.0f;
        float iz = mass_ * (w * w + h * h) / 12.0f;
        
        invInertiaTensor_(0, 0) = 1.0f / ix;
        invInertiaTensor_(1, 1) = 1.0f / iy;
        invInertiaTensor_(2, 2) = 1.0f / iz;
    }
    
    RigidBodyType type_;
    float mass_;
    float invMass_;
    
    Vec3 position_;
    Quat rotation_;
    Vec3 linearVelocity_;
    Vec3 angularVelocity_;
    Vec3 force_;
    Vec3 torque_;
    
    float restitution_;
    float friction_;
    float linearDamping_;
    float angularDamping_;
    
    bool isSleeping_;
    float sleepTimer_;
    
    Mat3 invInertiaTensor_;
    std::shared_ptr<Collider> collider_;
    
    void* userData_;
    uint32_t id_;
    static inline uint32_t nextId_ = 0;
};

// ===== Collision Callback =====
using CollisionCallback = std::function<void(const CollisionInfo&)>;
using TriggerCallback = std::function<void(RigidBody*, RigidBody*)>;

// ===== Physics World =====
class PhysicsWorld {
public:
    PhysicsWorld() = default;
    
    void setSettings(const PhysicsSettings& settings) { settings_ = settings; }
    PhysicsSettings& getSettings() { return settings_; }
    const PhysicsSettings& getSettings() const { return settings_; }
    
    // Body management
    RigidBody* createBody(RigidBodyType type = RigidBodyType::Dynamic) {
        bodies_.push_back(std::make_unique<RigidBody>(type));
        return bodies_.back().get();
    }
    
    void destroyBody(RigidBody* body) {
        bodies_.erase(
            std::remove_if(bodies_.begin(), bodies_.end(),
                [body](const auto& b) { return b.get() == body; }),
            bodies_.end()
        );
    }
    
    const std::vector<std::unique_ptr<RigidBody>>& getBodies() const { return bodies_; }
    
    // Step simulation
    void step(float dt) {
        // Clamp delta time
        dt = std::min(dt, settings_.maxDeltaTime);
        
        // Fixed timestep accumulator
        accumulator_ += dt;
        
        while (accumulator_ >= settings_.fixedTimeStep) {
            fixedStep(settings_.fixedTimeStep);
            accumulator_ -= settings_.fixedTimeStep;
        }
    }
    
    // Callbacks
    void setCollisionCallback(CollisionCallback callback) { collisionCallback_ = callback; }
    void setTriggerEnterCallback(TriggerCallback callback) { triggerEnterCallback_ = callback; }
    void setTriggerExitCallback(TriggerCallback callback) { triggerExitCallback_ = callback; }
    
    // Queries
    RigidBody* raycast(const Vec3& origin, const Vec3& direction, float maxDistance,
                       Vec3* hitPoint = nullptr, Vec3* hitNormal = nullptr);
    
    std::vector<RigidBody*> queryAABB(const AABB& aabb);
    std::vector<RigidBody*> querySphere(const Vec3& center, float radius);
    
    // Debug
    const std::vector<CollisionInfo>& getCollisions() const { return collisions_; }
    size_t getBodyCount() const { return bodies_.size(); }
    
    // Clear
    void clear() {
        bodies_.clear();
        collisions_.clear();
        activeTriggers_.clear();
    }
    
private:
    void fixedStep(float dt);
    void broadphase();
    void narrowphase();
    void resolveCollisions();
    void integrateVelocities(float dt);
    void updateSleeping(float dt);
    
    // Collision detection helpers
    bool sphereVsSphere(const RigidBody* a, const RigidBody* b, CollisionInfo& info);
    bool sphereVsBox(const RigidBody* sphere, const RigidBody* box, CollisionInfo& info);
    bool boxVsBox(const RigidBody* a, const RigidBody* b, CollisionInfo& info);
    bool sphereVsPlane(const RigidBody* sphere, const RigidBody* plane, CollisionInfo& info);
    bool boxVsPlane(const RigidBody* box, const RigidBody* plane, CollisionInfo& info);
    
    PhysicsSettings settings_;
    std::vector<std::unique_ptr<RigidBody>> bodies_;
    std::vector<CollisionInfo> collisions_;
    std::vector<std::pair<uint32_t, uint32_t>> broadphasePairs_;
    
    // Trigger tracking
    std::unordered_set<uint64_t> activeTriggers_;
    
    // Callbacks
    CollisionCallback collisionCallback_;
    TriggerCallback triggerEnterCallback_;
    TriggerCallback triggerExitCallback_;
    
    float accumulator_ = 0.0f;
};

// ===== Global Physics World Accessor =====
inline PhysicsWorld& getPhysicsWorld() {
    static PhysicsWorld world;
    return world;
}

}  // namespace luma
