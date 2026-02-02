// Water Physics - Buoyancy, drag, and water interaction physics
// Realistic floating and swimming behavior
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace luma {

// ============================================================================
// Buoyancy Point - Sample point for buoyancy calculation
// ============================================================================

struct BuoyancyPoint {
    Vec3 localPosition;     // Position relative to object center
    float radius;           // Sphere radius for volume calculation
    float forceMultiplier;  // Scale buoyancy force at this point
    
    // Runtime state
    float submergedRatio;   // 0 = above water, 1 = fully submerged
    Vec3 worldPosition;
};

// ============================================================================
// Water Physics Parameters
// ============================================================================

struct WaterPhysicsParams {
    // Water properties
    float waterDensity = 1000.0f;       // kg/m³ (pure water = 1000)
    float gravity = 9.81f;               // m/s²
    
    // Drag coefficients
    float linearDrag = 1.0f;            // Linear velocity damping
    float angularDrag = 1.0f;           // Angular velocity damping
    float surfaceDrag = 2.0f;           // Extra drag at water surface
    
    // Viscosity
    float viscosity = 0.001f;           // Water viscosity
    
    // Splash settings
    float splashVelocityThreshold = 2.0f;
    float splashForceMultiplier = 1.0f;
};

// ============================================================================
// Buoyant Object
// ============================================================================

struct BuoyantObject {
    std::string id;
    
    // Physical properties
    float mass = 1.0f;                  // kg
    float volume = 0.001f;              // m³ (for density calculation)
    Vec3 centerOfMass{0, 0, 0};
    Mat3 inertiaTensor;                 // For rotation
    
    // Buoyancy sample points
    std::vector<BuoyancyPoint> buoyancyPoints;
    
    // Transform
    Vec3 position;
    Quat rotation;
    
    // Velocity
    Vec3 linearVelocity;
    Vec3 angularVelocity;
    
    // Forces (accumulated each frame)
    Vec3 force;
    Vec3 torque;
    
    // State
    bool isFloating = false;
    float submergedVolume = 0.0f;
    float submergedRatio = 0.0f;
    
    // Configuration
    bool enableBuoyancy = true;
    bool enableDrag = true;
    bool isKinematic = false;           // If true, doesn't respond to forces
    
    float getDensity() const { return mass / std::max(volume, 0.0001f); }
    bool willFloat(float waterDensity) const { return getDensity() < waterDensity; }
};

// ============================================================================
// Buoyancy System
// ============================================================================

class BuoyancySystem {
public:
    BuoyancySystem() = default;
    
    // === Object Management ===
    
    void addObject(const BuoyantObject& obj) {
        objects_[obj.id] = obj;
        
        // Auto-generate buoyancy points if none specified
        if (objects_[obj.id].buoyancyPoints.empty()) {
            generateDefaultBuoyancyPoints(objects_[obj.id]);
        }
    }
    
    void removeObject(const std::string& id) {
        objects_.erase(id);
    }
    
    BuoyantObject* getObject(const std::string& id) {
        auto it = objects_.find(id);
        return (it != objects_.end()) ? &it->second : nullptr;
    }
    
    // === Simulation ===
    
    void simulate(float deltaTime, std::function<float(float, float)> getWaterHeight) {
        for (auto& [id, obj] : objects_) {
            if (obj.isKinematic) continue;
            
            // Reset forces
            obj.force = Vec3(0, 0, 0);
            obj.torque = Vec3(0, 0, 0);
            
            // Apply gravity
            obj.force.y -= obj.mass * params_.gravity;
            
            // Calculate buoyancy
            if (obj.enableBuoyancy) {
                calculateBuoyancy(obj, getWaterHeight);
            }
            
            // Calculate drag
            if (obj.enableDrag && obj.submergedRatio > 0.0f) {
                calculateDrag(obj);
            }
            
            // Integrate
            integrate(obj, deltaTime);
        }
    }
    
    // === Quick Buoyancy Check ===
    
    // Calculate buoyancy force for a simple sphere
    static Vec3 calculateSphereBuoyancy(const Vec3& position, float radius, float mass,
                                        float waterHeight, float waterDensity, float gravity) {
        float submergedDepth = waterHeight - (position.y - radius);
        
        if (submergedDepth <= 0.0f) {
            return Vec3(0, 0, 0);  // Above water
        }
        
        // Calculate submerged volume (sphere cap)
        float h = std::min(submergedDepth, 2.0f * radius);
        float submergedVolume = 3.14159f * h * h * (3.0f * radius - h) / 3.0f;
        
        // Buoyancy force = water density * g * displaced volume
        float buoyancyForce = waterDensity * gravity * submergedVolume;
        
        return Vec3(0, buoyancyForce, 0);
    }
    
    // === Parameters ===
    
    WaterPhysicsParams& getParams() { return params_; }
    const WaterPhysicsParams& getParams() const { return params_; }
    
private:
    void generateDefaultBuoyancyPoints(BuoyantObject& obj) {
        // Create a simple grid of buoyancy points
        float size = std::cbrt(obj.volume);  // Approximate size from volume
        float spacing = size / 2.0f;
        
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                for (int z = -1; z <= 1; z++) {
                    BuoyancyPoint bp;
                    bp.localPosition = Vec3(x * spacing, y * spacing, z * spacing);
                    bp.radius = spacing * 0.3f;
                    bp.forceMultiplier = 1.0f;
                    obj.buoyancyPoints.push_back(bp);
                }
            }
        }
    }
    
    void calculateBuoyancy(BuoyantObject& obj, 
                          std::function<float(float, float)> getWaterHeight) {
        obj.submergedVolume = 0.0f;
        float totalVolume = 0.0f;
        
        for (auto& bp : obj.buoyancyPoints) {
            // Transform to world space
            bp.worldPosition = obj.position + obj.rotation.rotate(bp.localPosition);
            
            // Get water height at this point
            float waterHeight = getWaterHeight(bp.worldPosition.x, bp.worldPosition.z);
            
            // Calculate submersion
            float depth = waterHeight - bp.worldPosition.y;
            
            if (depth > bp.radius) {
                bp.submergedRatio = 1.0f;  // Fully submerged
            } else if (depth > -bp.radius) {
                bp.submergedRatio = (depth + bp.radius) / (2.0f * bp.radius);  // Partially
            } else {
                bp.submergedRatio = 0.0f;  // Above water
            }
            
            // Calculate displaced volume for this point
            float pointVolume = (4.0f / 3.0f) * 3.14159f * bp.radius * bp.radius * bp.radius;
            float displacedVolume = pointVolume * bp.submergedRatio;
            
            obj.submergedVolume += displacedVolume;
            totalVolume += pointVolume;
            
            // Apply buoyancy force at this point
            if (bp.submergedRatio > 0.0f) {
                float buoyancyForce = params_.waterDensity * params_.gravity * 
                                      displacedVolume * bp.forceMultiplier;
                
                Vec3 forcePos = bp.worldPosition;
                Vec3 force(0, buoyancyForce, 0);
                
                // Add force
                obj.force = obj.force + force;
                
                // Add torque (force applied at offset creates rotation)
                Vec3 r = forcePos - (obj.position + obj.rotation.rotate(obj.centerOfMass));
                obj.torque = obj.torque + r.cross(force);
            }
        }
        
        obj.submergedRatio = totalVolume > 0 ? obj.submergedVolume / totalVolume : 0.0f;
        obj.isFloating = obj.submergedRatio > 0.0f && obj.submergedRatio < 1.0f;
    }
    
    void calculateDrag(BuoyantObject& obj) {
        // Linear drag
        float linearDragCoeff = params_.linearDrag * obj.submergedRatio;
        
        // Extra drag at surface
        if (obj.isFloating) {
            linearDragCoeff += params_.surfaceDrag * (1.0f - std::abs(obj.submergedRatio - 0.5f) * 2.0f);
        }
        
        // Velocity-dependent drag (quadratic)
        float speed = obj.linearVelocity.length();
        if (speed > 0.001f) {
            Vec3 dragForce = obj.linearVelocity.normalized() * (-linearDragCoeff * speed * speed);
            obj.force = obj.force + dragForce;
        }
        
        // Angular drag
        float angularDragCoeff = params_.angularDrag * obj.submergedRatio;
        float angularSpeed = obj.angularVelocity.length();
        if (angularSpeed > 0.001f) {
            Vec3 angularDrag = obj.angularVelocity.normalized() * (-angularDragCoeff * angularSpeed);
            obj.torque = obj.torque + angularDrag;
        }
    }
    
    void integrate(BuoyantObject& obj, float deltaTime) {
        // Semi-implicit Euler integration
        
        // Linear
        Vec3 acceleration = obj.force * (1.0f / obj.mass);
        obj.linearVelocity = obj.linearVelocity + acceleration * deltaTime;
        obj.position = obj.position + obj.linearVelocity * deltaTime;
        
        // Angular (simplified - assumes uniform density)
        // In production, use proper inertia tensor
        float momentOfInertia = obj.mass * std::cbrt(obj.volume) * std::cbrt(obj.volume) / 6.0f;
        Vec3 angularAccel = obj.torque * (1.0f / std::max(momentOfInertia, 0.001f));
        obj.angularVelocity = obj.angularVelocity + angularAccel * deltaTime;
        
        // Integrate rotation
        float angleSpeed = obj.angularVelocity.length();
        if (angleSpeed > 0.001f) {
            Vec3 axis = obj.angularVelocity.normalized();
            float angle = angleSpeed * deltaTime;
            Quat deltaRot = Quat::fromAxisAngle(axis, angle);
            obj.rotation = (deltaRot * obj.rotation).normalized();
        }
        
        // Damping for stability
        obj.linearVelocity = obj.linearVelocity * 0.999f;
        obj.angularVelocity = obj.angularVelocity * 0.995f;
    }
    
    std::unordered_map<std::string, BuoyantObject> objects_;
    WaterPhysicsParams params_;
};

// ============================================================================
// Swimming Controller - For character water movement
// ============================================================================

struct SwimmingParams {
    float swimSpeed = 3.0f;
    float diveSpeed = 2.0f;
    float surfaceSpeed = 4.0f;
    
    float acceleration = 5.0f;
    float deceleration = 3.0f;
    
    float buoyancy = 0.5f;              // Natural float tendency
    float waterDrag = 2.0f;
    
    float surfaceSnapDistance = 0.3f;   // Distance to snap to surface
    float divePressure = 1.0f;          // How hard to press to dive
    
    float staminaDrain = 0.1f;          // Per second
    float staminaRecovery = 0.2f;       // Per second (on surface)
};

class SwimmingController {
public:
    SwimmingController() = default;
    
    void update(float deltaTime, const Vec3& position, float waterSurfaceY,
               const Vec3& inputDirection, bool diveInput, bool surfaceInput) {
        
        float depth = waterSurfaceY - position.y;
        isInWater_ = depth > -0.5f;  // Consider in water if close to surface
        isSubmerged_ = depth > 0.0f;
        currentDepth_ = std::max(0.0f, depth);
        
        if (!isInWater_) {
            isSwimming_ = false;
            return;
        }
        
        isSwimming_ = true;
        
        // Calculate target velocity
        Vec3 targetVel(0, 0, 0);
        
        // Horizontal movement
        float horizInput = std::sqrt(inputDirection.x * inputDirection.x + 
                                    inputDirection.z * inputDirection.z);
        if (horizInput > 0.01f) {
            targetVel.x = inputDirection.x * params_.swimSpeed;
            targetVel.z = inputDirection.z * params_.swimSpeed;
        }
        
        // Vertical movement
        if (diveInput && !isAtSurface_) {
            targetVel.y = -params_.diveSpeed;
        } else if (surfaceInput || (!diveInput && !isSubmerged_)) {
            targetVel.y = params_.surfaceSpeed;
        } else {
            // Natural buoyancy
            targetVel.y = params_.buoyancy;
        }
        
        // Surface check
        isAtSurface_ = depth < params_.surfaceSnapDistance && depth > -0.1f;
        
        // Accelerate toward target
        Vec3 velDiff = targetVel - currentVelocity_;
        float accel = velDiff.length() > 0.01f ? params_.acceleration : params_.deceleration;
        
        currentVelocity_ = currentVelocity_ + velDiff.normalized() * 
                          std::min(velDiff.length(), accel * deltaTime);
        
        // Apply drag
        currentVelocity_ = currentVelocity_ * (1.0f - params_.waterDrag * deltaTime);
        
        // Stamina
        if (isSubmerged_) {
            stamina_ -= params_.staminaDrain * deltaTime;
            stamina_ = std::max(0.0f, stamina_);
        } else if (isAtSurface_) {
            stamina_ += params_.staminaRecovery * deltaTime;
            stamina_ = std::min(1.0f, stamina_);
        }
    }
    
    Vec3 getVelocity() const { return currentVelocity_; }
    bool isSwimming() const { return isSwimming_; }
    bool isSubmerged() const { return isSubmerged_; }
    bool isAtSurface() const { return isAtSurface_; }
    float getStamina() const { return stamina_; }
    float getCurrentDepth() const { return currentDepth_; }
    
    SwimmingParams& getParams() { return params_; }
    
private:
    SwimmingParams params_;
    
    Vec3 currentVelocity_;
    bool isInWater_ = false;
    bool isSwimming_ = false;
    bool isSubmerged_ = false;
    bool isAtSurface_ = false;
    float currentDepth_ = 0.0f;
    float stamina_ = 1.0f;
};

// ============================================================================
// Water Physics Manager - Singleton
// ============================================================================

class WaterPhysicsManager {
public:
    static WaterPhysicsManager& getInstance() {
        static WaterPhysicsManager instance;
        return instance;
    }
    
    BuoyancySystem& getBuoyancy() { return buoyancy_; }
    const BuoyancySystem& getBuoyancy() const { return buoyancy_; }
    
    void update(float deltaTime, std::function<float(float, float)> getWaterHeight) {
        buoyancy_.simulate(deltaTime, getWaterHeight);
    }
    
private:
    WaterPhysicsManager() = default;
    BuoyancySystem buoyancy_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline WaterPhysicsManager& getWaterPhysics() {
    return WaterPhysicsManager::getInstance();
}

inline BuoyancySystem& getBuoyancySystem() {
    return WaterPhysicsManager::getInstance().getBuoyancy();
}

}  // namespace luma
