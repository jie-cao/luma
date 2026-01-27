// Physics Constraints - Joints and constraints between rigid bodies
// Distance, Hinge, Ball-Socket, Slider, Fixed
#pragma once

#include "physics_world.h"
#include <cmath>

namespace luma {

// ===== Constraint Types =====
enum class ConstraintType {
    Distance,       // Fixed distance between two points
    BallSocket,     // 3 DOF rotational joint
    Hinge,          // 1 DOF rotational joint (door hinge)
    Slider,         // 1 DOF translational joint
    Fixed,          // 0 DOF - locks bodies together
    Spring,         // Soft distance constraint
    Cone            // Cone-limited ball socket
};

// ===== Base Constraint =====
class Constraint {
public:
    Constraint(RigidBody* bodyA, RigidBody* bodyB)
        : bodyA_(bodyA), bodyB_(bodyB), enabled_(true), breakForce_(0.0f) {}
    
    virtual ~Constraint() = default;
    
    virtual ConstraintType getType() const = 0;
    virtual void solve(float dt) = 0;
    
    RigidBody* getBodyA() { return bodyA_; }
    RigidBody* getBodyB() { return bodyB_; }
    RigidBody* getBodyA() const { return bodyA_; }
    RigidBody* getBodyB() const { return bodyB_; }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Break force (0 = unbreakable)
    void setBreakForce(float force) { breakForce_ = force; }
    float getBreakForce() const { return breakForce_; }
    
    bool isBroken() const { return broken_; }
    
protected:
    RigidBody* bodyA_;
    RigidBody* bodyB_;
    bool enabled_;
    float breakForce_;
    bool broken_ = false;
    float appliedForce_ = 0.0f;
};

// ===== Distance Constraint =====
// Maintains fixed distance between two anchor points
class DistanceConstraint : public Constraint {
public:
    DistanceConstraint(RigidBody* bodyA, RigidBody* bodyB,
                       const Vec3& anchorA, const Vec3& anchorB,
                       float distance = -1.0f)
        : Constraint(bodyA, bodyB),
          localAnchorA_(anchorA), localAnchorB_(anchorB)
    {
        if (distance < 0.0f) {
            // Calculate initial distance
            Vec3 worldA = bodyA->getPosition() + bodyA->getRotation().rotate(anchorA);
            Vec3 worldB = bodyB->getPosition() + bodyB->getRotation().rotate(anchorB);
            distance_ = (worldB - worldA).length();
        } else {
            distance_ = distance;
        }
    }
    
    ConstraintType getType() const override { return ConstraintType::Distance; }
    
    void setDistance(float d) { distance_ = d; }
    float getDistance() const { return distance_; }
    
    void solve(float dt) override {
        if (!enabled_ || broken_) return;
        
        Vec3 worldA = bodyA_->getPosition() + bodyA_->getRotation().rotate(localAnchorA_);
        Vec3 worldB = bodyB_->getPosition() + bodyB_->getRotation().rotate(localAnchorB_);
        
        Vec3 delta = worldB - worldA;
        float currentDist = delta.length();
        
        if (currentDist < 0.0001f) return;
        
        Vec3 normal = delta * (1.0f / currentDist);
        float error = currentDist - distance_;
        
        // Calculate relative velocity
        Vec3 relVel = bodyB_->getLinearVelocity() - bodyA_->getLinearVelocity();
        float relVelNormal = relVel.dot(normal);
        
        // Calculate correction
        float invMassSum = bodyA_->getInverseMass() + bodyB_->getInverseMass();
        if (invMassSum <= 0.0f) return;
        
        float biasFactor = 0.2f / dt;
        float bias = biasFactor * error;
        
        float lambda = -(relVelNormal + bias) / invMassSum;
        
        Vec3 impulse = normal * lambda;
        
        appliedForce_ = std::abs(lambda) / dt;
        if (breakForce_ > 0.0f && appliedForce_ > breakForce_) {
            broken_ = true;
            return;
        }
        
        if (bodyA_->getType() == RigidBodyType::Dynamic) {
            bodyA_->addImpulseAtPoint(impulse * -1.0f, worldA);
        }
        if (bodyB_->getType() == RigidBodyType::Dynamic) {
            bodyB_->addImpulseAtPoint(impulse, worldB);
        }
    }
    
private:
    Vec3 localAnchorA_;
    Vec3 localAnchorB_;
    float distance_;
};

// ===== Ball Socket Constraint (Point-to-Point) =====
// Allows free rotation around a shared pivot
class BallSocketConstraint : public Constraint {
public:
    BallSocketConstraint(RigidBody* bodyA, RigidBody* bodyB,
                         const Vec3& pivot)
        : Constraint(bodyA, bodyB)
    {
        // Convert world pivot to local space
        localAnchorA_ = bodyA->getRotation().conjugate().rotate(pivot - bodyA->getPosition());
        localAnchorB_ = bodyB->getRotation().conjugate().rotate(pivot - bodyB->getPosition());
    }
    
    ConstraintType getType() const override { return ConstraintType::BallSocket; }
    
    void solve(float dt) override {
        if (!enabled_ || broken_) return;
        
        Vec3 worldA = bodyA_->getPosition() + bodyA_->getRotation().rotate(localAnchorA_);
        Vec3 worldB = bodyB_->getPosition() + bodyB_->getRotation().rotate(localAnchorB_);
        
        Vec3 error = worldB - worldA;
        float errorLen = error.length();
        
        if (errorLen < 0.0001f) return;
        
        // Calculate relative velocity at anchor points
        Vec3 velA = bodyA_->getLinearVelocity();
        Vec3 velB = bodyB_->getLinearVelocity();
        Vec3 relVel = velB - velA;
        
        // Solve for each axis
        float invMassSum = bodyA_->getInverseMass() + bodyB_->getInverseMass();
        if (invMassSum <= 0.0f) return;
        
        float biasFactor = 0.2f / dt;
        Vec3 bias = error * biasFactor;
        
        Vec3 impulse = (relVel * -1.0f - bias) * (1.0f / invMassSum);
        
        appliedForce_ = impulse.length() / dt;
        if (breakForce_ > 0.0f && appliedForce_ > breakForce_) {
            broken_ = true;
            return;
        }
        
        if (bodyA_->getType() == RigidBodyType::Dynamic) {
            bodyA_->addImpulseAtPoint(impulse, worldA);
        }
        if (bodyB_->getType() == RigidBodyType::Dynamic) {
            bodyB_->addImpulseAtPoint(impulse * -1.0f, worldB);
        }
    }
    
private:
    Vec3 localAnchorA_;
    Vec3 localAnchorB_;
};

// ===== Hinge Constraint =====
// Allows rotation only around one axis
class HingeConstraint : public Constraint {
public:
    HingeConstraint(RigidBody* bodyA, RigidBody* bodyB,
                    const Vec3& pivot, const Vec3& axis)
        : Constraint(bodyA, bodyB),
          useLimits_(false), minAngle_(0.0f), maxAngle_(0.0f)
    {
        localAnchorA_ = bodyA->getRotation().conjugate().rotate(pivot - bodyA->getPosition());
        localAnchorB_ = bodyB->getRotation().conjugate().rotate(pivot - bodyB->getPosition());
        localAxisA_ = bodyA->getRotation().conjugate().rotate(axis.normalized());
        localAxisB_ = bodyB->getRotation().conjugate().rotate(axis.normalized());
    }
    
    ConstraintType getType() const override { return ConstraintType::Hinge; }
    
    void setLimits(float minAngle, float maxAngle) {
        useLimits_ = true;
        minAngle_ = minAngle;
        maxAngle_ = maxAngle;
    }
    
    void setMotor(float targetVelocity, float maxTorque) {
        useMotor_ = true;
        motorTargetVelocity_ = targetVelocity;
        motorMaxTorque_ = maxTorque;
    }
    
    void disableMotor() { useMotor_ = false; }
    
    void solve(float dt) override {
        if (!enabled_ || broken_) return;
        
        // First solve ball socket part
        Vec3 worldA = bodyA_->getPosition() + bodyA_->getRotation().rotate(localAnchorA_);
        Vec3 worldB = bodyB_->getPosition() + bodyB_->getRotation().rotate(localAnchorB_);
        
        Vec3 error = worldB - worldA;
        
        float invMassSum = bodyA_->getInverseMass() + bodyB_->getInverseMass();
        if (invMassSum > 0.0f) {
            Vec3 velA = bodyA_->getLinearVelocity();
            Vec3 velB = bodyB_->getLinearVelocity();
            Vec3 relVel = velB - velA;
            
            float biasFactor = 0.2f / dt;
            Vec3 bias = error * biasFactor;
            
            Vec3 impulse = (relVel * -1.0f - bias) * (1.0f / invMassSum);
            
            if (bodyA_->getType() == RigidBodyType::Dynamic) {
                bodyA_->addImpulseAtPoint(impulse, worldA);
            }
            if (bodyB_->getType() == RigidBodyType::Dynamic) {
                bodyB_->addImpulseAtPoint(impulse * -1.0f, worldB);
            }
        }
        
        // Constrain rotation to hinge axis
        Vec3 worldAxisA = bodyA_->getRotation().rotate(localAxisA_);
        Vec3 worldAxisB = bodyB_->getRotation().rotate(localAxisB_);
        
        // Find perpendicular axes that should align
        Vec3 perp1A, perp2A;
        if (std::abs(worldAxisA.y) < 0.9f) {
            perp1A = worldAxisA.cross(Vec3(0, 1, 0)).normalized();
        } else {
            perp1A = worldAxisA.cross(Vec3(1, 0, 0)).normalized();
        }
        perp2A = worldAxisA.cross(perp1A);
        
        // Angular constraint to keep axes aligned
        Vec3 angularError = worldAxisA.cross(worldAxisB);
        float angularErrorLen = angularError.length();
        
        if (angularErrorLen > 0.001f) {
            // Apply angular correction
            Vec3 angularImpulse = angularError * (0.2f / dt);
            
            if (bodyA_->getType() == RigidBodyType::Dynamic) {
                bodyA_->addTorque(angularImpulse * -1.0f);
            }
            if (bodyB_->getType() == RigidBodyType::Dynamic) {
                bodyB_->addTorque(angularImpulse);
            }
        }
        
        // Motor
        if (useMotor_) {
            Vec3 relAngVel = bodyB_->getAngularVelocity() - bodyA_->getAngularVelocity();
            float hingeVel = relAngVel.dot(worldAxisA);
            float velError = motorTargetVelocity_ - hingeVel;
            
            float torque = std::max(-motorMaxTorque_, std::min(motorMaxTorque_, velError * 10.0f));
            Vec3 motorTorque = worldAxisA * torque;
            
            if (bodyA_->getType() == RigidBodyType::Dynamic) {
                bodyA_->addTorque(motorTorque * -1.0f);
            }
            if (bodyB_->getType() == RigidBodyType::Dynamic) {
                bodyB_->addTorque(motorTorque);
            }
        }
    }
    
private:
    Vec3 localAnchorA_;
    Vec3 localAnchorB_;
    Vec3 localAxisA_;
    Vec3 localAxisB_;
    
    bool useLimits_;
    float minAngle_;
    float maxAngle_;
    
    bool useMotor_ = false;
    float motorTargetVelocity_ = 0.0f;
    float motorMaxTorque_ = 0.0f;
};

// ===== Slider Constraint =====
// Allows movement only along one axis
class SliderConstraint : public Constraint {
public:
    SliderConstraint(RigidBody* bodyA, RigidBody* bodyB,
                     const Vec3& axis)
        : Constraint(bodyA, bodyB),
          useLimits_(false), minDistance_(0.0f), maxDistance_(0.0f)
    {
        localAxis_ = bodyA->getRotation().conjugate().rotate(axis.normalized());
        
        Vec3 delta = bodyB->getPosition() - bodyA->getPosition();
        initialDistance_ = delta.dot(axis.normalized());
    }
    
    ConstraintType getType() const override { return ConstraintType::Slider; }
    
    void setLimits(float minDist, float maxDist) {
        useLimits_ = true;
        minDistance_ = minDist;
        maxDistance_ = maxDist;
    }
    
    void solve(float dt) override {
        if (!enabled_ || broken_) return;
        
        Vec3 worldAxis = bodyA_->getRotation().rotate(localAxis_);
        Vec3 delta = bodyB_->getPosition() - bodyA_->getPosition();
        
        // Project delta perpendicular to slider axis
        float alongAxis = delta.dot(worldAxis);
        Vec3 perp = delta - worldAxis * alongAxis;
        
        // Constrain perpendicular movement
        if (perp.lengthSquared() > 0.0001f) {
            float invMassSum = bodyA_->getInverseMass() + bodyB_->getInverseMass();
            if (invMassSum > 0.0f) {
                Vec3 correction = perp * (0.2f / dt);
                
                if (bodyA_->getType() == RigidBodyType::Dynamic) {
                    bodyA_->addImpulse(correction * bodyA_->getInverseMass());
                }
                if (bodyB_->getType() == RigidBodyType::Dynamic) {
                    bodyB_->addImpulse(correction * -1.0f * bodyB_->getInverseMass());
                }
            }
        }
        
        // Apply limits
        if (useLimits_) {
            if (alongAxis < minDistance_) {
                Vec3 push = worldAxis * (minDistance_ - alongAxis) * 0.5f;
                if (bodyA_->getType() == RigidBodyType::Dynamic) {
                    bodyA_->setPosition(bodyA_->getPosition() - push);
                }
                if (bodyB_->getType() == RigidBodyType::Dynamic) {
                    bodyB_->setPosition(bodyB_->getPosition() + push);
                }
            } else if (alongAxis > maxDistance_) {
                Vec3 push = worldAxis * (alongAxis - maxDistance_) * 0.5f;
                if (bodyA_->getType() == RigidBodyType::Dynamic) {
                    bodyA_->setPosition(bodyA_->getPosition() + push);
                }
                if (bodyB_->getType() == RigidBodyType::Dynamic) {
                    bodyB_->setPosition(bodyB_->getPosition() - push);
                }
            }
        }
        
        // Constrain rotation (keep aligned)
        Vec3 angVelA = bodyA_->getAngularVelocity();
        Vec3 angVelB = bodyB_->getAngularVelocity();
        Vec3 relAngVel = angVelB - angVelA;
        
        // Remove rotation perpendicular to slider axis
        Vec3 perpAngVel = relAngVel - worldAxis * relAngVel.dot(worldAxis);
        
        if (perpAngVel.lengthSquared() > 0.0001f) {
            if (bodyA_->getType() == RigidBodyType::Dynamic) {
                bodyA_->addTorque(perpAngVel * 0.5f);
            }
            if (bodyB_->getType() == RigidBodyType::Dynamic) {
                bodyB_->addTorque(perpAngVel * -0.5f);
            }
        }
    }
    
private:
    Vec3 localAxis_;
    float initialDistance_;
    
    bool useLimits_;
    float minDistance_;
    float maxDistance_;
};

// ===== Fixed Constraint =====
// Locks bodies together (weld)
class FixedConstraint : public Constraint {
public:
    FixedConstraint(RigidBody* bodyA, RigidBody* bodyB)
        : Constraint(bodyA, bodyB)
    {
        Vec3 delta = bodyB->getPosition() - bodyA->getPosition();
        localOffset_ = bodyA->getRotation().conjugate().rotate(delta);
        relativeRotation_ = bodyA->getRotation().conjugate() * bodyB->getRotation();
    }
    
    ConstraintType getType() const override { return ConstraintType::Fixed; }
    
    void solve(float dt) override {
        if (!enabled_ || broken_) return;
        
        // Position constraint
        Vec3 targetPos = bodyA_->getPosition() + bodyA_->getRotation().rotate(localOffset_);
        Vec3 error = targetPos - bodyB_->getPosition();
        
        float invMassSum = bodyA_->getInverseMass() + bodyB_->getInverseMass();
        if (invMassSum > 0.0f) {
            Vec3 correction = error * (0.3f / dt);
            
            if (bodyA_->getType() == RigidBodyType::Dynamic) {
                bodyA_->addImpulse(correction * -1.0f * bodyA_->getInverseMass());
            }
            if (bodyB_->getType() == RigidBodyType::Dynamic) {
                bodyB_->addImpulse(correction * bodyB_->getInverseMass());
            }
        }
        
        // Rotation constraint
        Quat targetRot = bodyA_->getRotation() * relativeRotation_;
        Quat currentRot = bodyB_->getRotation();
        Quat rotError = targetRot * currentRot.conjugate();
        
        // Convert quaternion error to axis-angle
        Vec3 axis;
        float angle;
        float sinHalfAngle = std::sqrt(rotError.x*rotError.x + rotError.y*rotError.y + rotError.z*rotError.z);
        if (sinHalfAngle > 0.001f) {
            axis = Vec3(rotError.x, rotError.y, rotError.z) * (1.0f / sinHalfAngle);
            angle = 2.0f * std::atan2(sinHalfAngle, rotError.w);
            
            Vec3 angularCorrection = axis * angle * 0.3f / dt;
            
            if (bodyA_->getType() == RigidBodyType::Dynamic) {
                bodyA_->addTorque(angularCorrection * -1.0f);
            }
            if (bodyB_->getType() == RigidBodyType::Dynamic) {
                bodyB_->addTorque(angularCorrection);
            }
        }
    }
    
private:
    Vec3 localOffset_;
    Quat relativeRotation_;
};

// ===== Spring Constraint =====
// Soft distance constraint with damping
class SpringConstraint : public Constraint {
public:
    SpringConstraint(RigidBody* bodyA, RigidBody* bodyB,
                     const Vec3& anchorA, const Vec3& anchorB,
                     float restLength = -1.0f,
                     float stiffness = 100.0f, float damping = 10.0f)
        : Constraint(bodyA, bodyB),
          localAnchorA_(anchorA), localAnchorB_(anchorB),
          stiffness_(stiffness), damping_(damping)
    {
        if (restLength < 0.0f) {
            Vec3 worldA = bodyA->getPosition() + bodyA->getRotation().rotate(anchorA);
            Vec3 worldB = bodyB->getPosition() + bodyB->getRotation().rotate(anchorB);
            restLength_ = (worldB - worldA).length();
        } else {
            restLength_ = restLength;
        }
    }
    
    ConstraintType getType() const override { return ConstraintType::Spring; }
    
    void setStiffness(float k) { stiffness_ = k; }
    float getStiffness() const { return stiffness_; }
    
    void setDamping(float d) { damping_ = d; }
    float getDamping() const { return damping_; }
    
    void setRestLength(float len) { restLength_ = len; }
    float getRestLength() const { return restLength_; }
    
    void solve(float dt) override {
        if (!enabled_ || broken_) return;
        
        Vec3 worldA = bodyA_->getPosition() + bodyA_->getRotation().rotate(localAnchorA_);
        Vec3 worldB = bodyB_->getPosition() + bodyB_->getRotation().rotate(localAnchorB_);
        
        Vec3 delta = worldB - worldA;
        float currentLength = delta.length();
        
        if (currentLength < 0.0001f) return;
        
        Vec3 normal = delta * (1.0f / currentLength);
        float displacement = currentLength - restLength_;
        
        // Spring force: F = -k * x
        float springForce = stiffness_ * displacement;
        
        // Damping force: F = -c * v
        Vec3 relVel = bodyB_->getLinearVelocity() - bodyA_->getLinearVelocity();
        float dampingForce = damping_ * relVel.dot(normal);
        
        float totalForce = springForce + dampingForce;
        Vec3 force = normal * totalForce;
        
        appliedForce_ = std::abs(totalForce);
        if (breakForce_ > 0.0f && appliedForce_ > breakForce_) {
            broken_ = true;
            return;
        }
        
        if (bodyA_->getType() == RigidBodyType::Dynamic) {
            bodyA_->addForceAtPoint(force, worldA);
        }
        if (bodyB_->getType() == RigidBodyType::Dynamic) {
            bodyB_->addForceAtPoint(force * -1.0f, worldB);
        }
    }
    
private:
    Vec3 localAnchorA_;
    Vec3 localAnchorB_;
    float restLength_;
    float stiffness_;
    float damping_;
};

// ===== Constraint Manager =====
class ConstraintManager {
public:
    template<typename T, typename... Args>
    T* createConstraint(Args&&... args) {
        auto constraint = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = constraint.get();
        constraints_.push_back(std::move(constraint));
        return ptr;
    }
    
    void destroyConstraint(Constraint* constraint) {
        constraints_.erase(
            std::remove_if(constraints_.begin(), constraints_.end(),
                [constraint](const auto& c) { return c.get() == constraint; }),
            constraints_.end()
        );
    }
    
    void solveConstraints(float dt) {
        for (auto& constraint : constraints_) {
            if (constraint->isEnabled() && !constraint->isBroken()) {
                constraint->solve(dt);
            }
        }
    }
    
    // Remove broken constraints
    void cleanupBroken() {
        constraints_.erase(
            std::remove_if(constraints_.begin(), constraints_.end(),
                [](const auto& c) { return c->isBroken(); }),
            constraints_.end()
        );
    }
    
    const std::vector<std::unique_ptr<Constraint>>& getConstraints() const { 
        return constraints_; 
    }
    
    void clear() { constraints_.clear(); }
    
private:
    std::vector<std::unique_ptr<Constraint>> constraints_;
};

// ===== Global Constraint Manager =====
inline ConstraintManager& getConstraintManager() {
    static ConstraintManager manager;
    return manager;
}

}  // namespace luma
