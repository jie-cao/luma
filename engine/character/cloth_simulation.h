// LUMA Cloth Simulation System
// Simple mass-spring cloth physics for clothing
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <unordered_set>
#include <cmath>
#include <algorithm>

namespace luma {

// ============================================================================
// Cloth Particle
// ============================================================================

struct ClothParticle {
    Vec3 position;
    Vec3 previousPosition;
    Vec3 velocity;
    Vec3 acceleration;
    
    float mass = 1.0f;
    bool pinned = false;  // Fixed in place
    
    // Collision shape (for body collision)
    float collisionRadius = 0.01f;
    
    ClothParticle() = default;
    ClothParticle(const Vec3& pos, float m = 1.0f) 
        : position(pos), previousPosition(pos), velocity(0,0,0), acceleration(0,0,0), mass(m) {}
};

// ============================================================================
// Spring Constraint
// ============================================================================

struct ClothSpring {
    uint32_t p1, p2;      // Particle indices
    float restLength;      // Original length
    float stiffness;       // Spring constant (0-1)
    
    enum Type {
        Structural,   // Direct neighbors
        Shear,        // Diagonal neighbors  
        Bend          // Skip one neighbor (for bending resistance)
    } type = Structural;
    
    ClothSpring() = default;
    ClothSpring(uint32_t a, uint32_t b, float len, float stiff, Type t = Structural)
        : p1(a), p2(b), restLength(len), stiffness(stiff), type(t) {}
};

// ============================================================================
// Collision Sphere (for body parts)
// ============================================================================

struct CollisionSphere {
    Vec3 center;
    float radius;
    
    CollisionSphere() : center(0,0,0), radius(0.1f) {}
    CollisionSphere(const Vec3& c, float r) : center(c), radius(r) {}
};

// ============================================================================
// Collision Capsule (for limbs)
// ============================================================================

struct CollisionCapsule {
    Vec3 start;
    Vec3 end;
    float radius;
    
    CollisionCapsule() : start(0,0,0), end(0,1,0), radius(0.05f) {}
    CollisionCapsule(const Vec3& s, const Vec3& e, float r) : start(s), end(e), radius(r) {}
};

// ============================================================================
// Cloth Simulation Settings
// ============================================================================

struct ClothSettings {
    // Physics
    Vec3 gravity{0, -9.81f, 0};
    float damping = 0.98f;           // Velocity damping
    float airResistance = 0.02f;     // Air drag
    
    // Solver
    int constraintIterations = 8;    // More = stiffer but slower
    float timestep = 1.0f / 60.0f;   // Fixed timestep
    
    // Material
    float structuralStiffness = 0.9f;
    float shearStiffness = 0.7f;
    float bendStiffness = 0.3f;
    
    // Collision
    float collisionMargin = 0.005f;  // Extra distance from collision surfaces
    float collisionFriction = 0.5f;
    
    // Limits
    float maxVelocity = 10.0f;       // Clamp velocity
    float maxStretch = 1.1f;         // Maximum spring stretch ratio
};

// ============================================================================
// Cloth Simulation
// ============================================================================

class ClothSimulation {
public:
    ClothSimulation() = default;
    
    // Initialize from mesh
    void initialize(const std::vector<Vertex>& vertices,
                   const std::vector<uint32_t>& indices,
                   const std::vector<uint32_t>& pinnedVertices = {}) {
        particles_.clear();
        springs_.clear();
        
        // Create particles from vertices
        for (const auto& v : vertices) {
            ClothParticle p(Vec3(v.position[0], v.position[1], v.position[2]));
            particles_.push_back(p);
        }
        
        // Mark pinned particles
        for (uint32_t idx : pinnedVertices) {
            if (idx < particles_.size()) {
                particles_[idx].pinned = true;
            }
        }
        
        // Build springs from mesh topology
        buildSpringsFromMesh(indices);
        
        initialized_ = true;
    }
    
    // Set pinned vertices (e.g., collar, waistband)
    void setPinnedVertices(const std::vector<uint32_t>& indices) {
        for (auto& p : particles_) {
            p.pinned = false;
        }
        for (uint32_t idx : indices) {
            if (idx < particles_.size()) {
                particles_[idx].pinned = true;
            }
        }
    }
    
    // Update pinned positions (for animation)
    void updatePinnedPositions(const std::vector<Vertex>& animatedVertices) {
        for (size_t i = 0; i < particles_.size() && i < animatedVertices.size(); i++) {
            if (particles_[i].pinned) {
                particles_[i].position = Vec3(
                    animatedVertices[i].position[0],
                    animatedVertices[i].position[1],
                    animatedVertices[i].position[2]
                );
                particles_[i].previousPosition = particles_[i].position;
                particles_[i].velocity = Vec3(0, 0, 0);
            }
        }
    }
    
    // Set collision bodies
    void setCollisionSpheres(const std::vector<CollisionSphere>& spheres) {
        collisionSpheres_ = spheres;
    }
    
    void setCollisionCapsules(const std::vector<CollisionCapsule>& capsules) {
        collisionCapsules_ = capsules;
    }
    
    // Simulate one step
    void step(float dt) {
        if (!initialized_ || particles_.empty()) return;
        
        dt = std::min(dt, settings_.timestep * 4);  // Clamp large dt
        
        // Accumulate substeps for stability
        float remaining = dt;
        while (remaining > 0.0001f) {
            float substep = std::min(remaining, settings_.timestep);
            simulateSubstep(substep);
            remaining -= substep;
        }
    }
    
    // Apply simulation results back to mesh
    void applyToMesh(std::vector<Vertex>& vertices) const {
        for (size_t i = 0; i < vertices.size() && i < particles_.size(); i++) {
            vertices[i].position[0] = particles_[i].position.x;
            vertices[i].position[1] = particles_[i].position.y;
            vertices[i].position[2] = particles_[i].position.z;
        }
        
        // Recalculate normals
        recalculateNormals(vertices);
    }
    
    // Reset to initial state
    void reset(const std::vector<Vertex>& originalVertices) {
        for (size_t i = 0; i < particles_.size() && i < originalVertices.size(); i++) {
            particles_[i].position = Vec3(
                originalVertices[i].position[0],
                originalVertices[i].position[1],
                originalVertices[i].position[2]
            );
            particles_[i].previousPosition = particles_[i].position;
            particles_[i].velocity = Vec3(0, 0, 0);
            particles_[i].acceleration = Vec3(0, 0, 0);
        }
    }
    
    // Settings
    ClothSettings& getSettings() { return settings_; }
    const ClothSettings& getSettings() const { return settings_; }
    
    // Debug info
    size_t getParticleCount() const { return particles_.size(); }
    size_t getSpringCount() const { return springs_.size(); }
    
private:
    std::vector<ClothParticle> particles_;
    std::vector<ClothSpring> springs_;
    std::vector<CollisionSphere> collisionSpheres_;
    std::vector<CollisionCapsule> collisionCapsules_;
    ClothSettings settings_;
    bool initialized_ = false;
    
    void buildSpringsFromMesh(const std::vector<uint32_t>& indices) {
        std::unordered_set<uint64_t> addedSprings;
        
        auto addSpring = [&](uint32_t a, uint32_t b, ClothSpring::Type type, float stiffness) {
            if (a == b) return;
            if (a > b) std::swap(a, b);
            
            uint64_t key = (uint64_t(a) << 32) | b;
            if (addedSprings.count(key)) return;
            addedSprings.insert(key);
            
            float length = (particles_[a].position - particles_[b].position).length();
            springs_.push_back(ClothSpring(a, b, length, stiffness, type));
        };
        
        // Build structural springs from triangles
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            // Structural springs (edges)
            addSpring(i0, i1, ClothSpring::Structural, settings_.structuralStiffness);
            addSpring(i1, i2, ClothSpring::Structural, settings_.structuralStiffness);
            addSpring(i2, i0, ClothSpring::Structural, settings_.structuralStiffness);
        }
        
        // Build shear and bend springs from vertex adjacency
        // Simplified: add springs between vertices sharing a triangle
        std::vector<std::unordered_set<uint32_t>> adjacency(particles_.size());
        
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            adjacency[i0].insert(i1);
            adjacency[i0].insert(i2);
            adjacency[i1].insert(i0);
            adjacency[i1].insert(i2);
            adjacency[i2].insert(i0);
            adjacency[i2].insert(i1);
        }
        
        // Bend springs (connect vertices two edges apart)
        for (size_t i = 0; i < particles_.size(); i++) {
            for (uint32_t j : adjacency[i]) {
                for (uint32_t k : adjacency[j]) {
                    if (k != i && !adjacency[i].count(k)) {
                        addSpring(static_cast<uint32_t>(i), k, 
                                  ClothSpring::Bend, settings_.bendStiffness);
                    }
                }
            }
        }
    }
    
    void simulateSubstep(float dt) {
        // Apply forces
        for (auto& p : particles_) {
            if (p.pinned) continue;
            
            // Gravity
            p.acceleration = settings_.gravity;
            
            // Air resistance
            p.acceleration = p.acceleration - p.velocity * settings_.airResistance;
        }
        
        // Integrate (Verlet integration)
        for (auto& p : particles_) {
            if (p.pinned) continue;
            
            Vec3 newPos = p.position * 2.0f - p.previousPosition + p.acceleration * dt * dt;
            p.previousPosition = p.position;
            p.position = newPos;
            
            // Calculate velocity (for damping and collision response)
            p.velocity = (p.position - p.previousPosition) / dt;
            
            // Damping
            p.velocity = p.velocity * settings_.damping;
            
            // Clamp velocity
            float speed = p.velocity.length();
            if (speed > settings_.maxVelocity) {
                p.velocity = p.velocity * (settings_.maxVelocity / speed);
            }
        }
        
        // Solve constraints
        for (int iter = 0; iter < settings_.constraintIterations; iter++) {
            // Spring constraints
            for (const auto& spring : springs_) {
                solveSpringConstraint(spring);
            }
            
            // Collision constraints
            for (auto& p : particles_) {
                if (p.pinned) continue;
                
                for (const auto& sphere : collisionSpheres_) {
                    solveSphereCollision(p, sphere);
                }
                
                for (const auto& capsule : collisionCapsules_) {
                    solveCapsuleCollision(p, capsule);
                }
            }
        }
    }
    
    void solveSpringConstraint(const ClothSpring& spring) {
        ClothParticle& p1 = particles_[spring.p1];
        ClothParticle& p2 = particles_[spring.p2];
        
        Vec3 delta = p2.position - p1.position;
        float currentLength = delta.length();
        
        if (currentLength < 0.0001f) return;
        
        // Calculate correction
        float diff = (currentLength - spring.restLength) / currentLength;
        
        // Clamp stretch
        float stretchRatio = currentLength / spring.restLength;
        if (stretchRatio > settings_.maxStretch) {
            diff = (currentLength - spring.restLength * settings_.maxStretch) / currentLength;
        }
        
        Vec3 correction = delta * diff * spring.stiffness * 0.5f;
        
        // Apply correction based on mass
        if (!p1.pinned && !p2.pinned) {
            float totalMass = p1.mass + p2.mass;
            p1.position = p1.position + correction * (p2.mass / totalMass);
            p2.position = p2.position - correction * (p1.mass / totalMass);
        } else if (!p1.pinned) {
            p1.position = p1.position + correction * 2.0f;
        } else if (!p2.pinned) {
            p2.position = p2.position - correction * 2.0f;
        }
    }
    
    void solveSphereCollision(ClothParticle& p, const CollisionSphere& sphere) {
        Vec3 diff = p.position - sphere.center;
        float dist = diff.length();
        float minDist = sphere.radius + settings_.collisionMargin;
        
        if (dist < minDist && dist > 0.0001f) {
            // Push out
            Vec3 normal = diff / dist;
            p.position = sphere.center + normal * minDist;
            
            // Apply friction
            Vec3 velocity = p.position - p.previousPosition;
            Vec3 normalVel = normal * (velocity.x * normal.x + velocity.y * normal.y + velocity.z * normal.z);
            Vec3 tangentVel = velocity - normalVel;
            
            p.previousPosition = p.position - tangentVel * settings_.collisionFriction;
        }
    }
    
    void solveCapsuleCollision(ClothParticle& p, const CollisionCapsule& capsule) {
        // Find closest point on capsule axis
        Vec3 axis = capsule.end - capsule.start;
        float axisLengthSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
        
        if (axisLengthSq < 0.0001f) {
            // Degenerate capsule, treat as sphere
            CollisionSphere sphere(capsule.start, capsule.radius);
            solveSphereCollision(p, sphere);
            return;
        }
        
        Vec3 toParticle = p.position - capsule.start;
        float t = (toParticle.x * axis.x + toParticle.y * axis.y + toParticle.z * axis.z) / axisLengthSq;
        t = std::clamp(t, 0.0f, 1.0f);
        
        Vec3 closestPoint = capsule.start + axis * t;
        Vec3 diff = p.position - closestPoint;
        float dist = diff.length();
        float minDist = capsule.radius + settings_.collisionMargin;
        
        if (dist < minDist && dist > 0.0001f) {
            Vec3 normal = diff / dist;
            p.position = closestPoint + normal * minDist;
            
            // Friction
            Vec3 velocity = p.position - p.previousPosition;
            Vec3 normalVel = normal * (velocity.x * normal.x + velocity.y * normal.y + velocity.z * normal.z);
            Vec3 tangentVel = velocity - normalVel;
            
            p.previousPosition = p.position - tangentVel * settings_.collisionFriction;
        }
    }
    
    void recalculateNormals(std::vector<Vertex>& vertices) const {
        // Reset normals
        for (auto& v : vertices) {
            v.normal[0] = v.normal[1] = v.normal[2] = 0;
        }
        
        // This is a simplified version - in practice you'd use the index buffer
        // to calculate face normals and accumulate them
        for (auto& v : vertices) {
            // Simple approximation: normal points away from center
            float len = std::sqrt(v.position[0]*v.position[0] + 
                                   v.position[2]*v.position[2]);
            if (len > 0.001f) {
                v.normal[0] = v.position[0] / len * 0.5f;
                v.normal[1] = 0.5f;
                v.normal[2] = v.position[2] / len * 0.5f;
            } else {
                v.normal[0] = 0;
                v.normal[1] = 1;
                v.normal[2] = 0;
            }
        }
    }
};

// ============================================================================
// Body Collision Generator
// ============================================================================

class BodyCollisionGenerator {
public:
    // Generate collision shapes from body parameters
    static std::vector<CollisionSphere> generateFromBody(float height = 1.8f,
                                                          float weight = 0.5f) {
        std::vector<CollisionSphere> spheres;
        
        float scale = height / 1.8f;
        float widthScale = 1.0f + (weight - 0.5f) * 0.3f;
        
        // Head
        spheres.push_back(CollisionSphere(Vec3(0, 1.6f * scale, 0), 0.1f * scale));
        
        // Neck
        spheres.push_back(CollisionSphere(Vec3(0, 1.45f * scale, 0), 0.05f * scale));
        
        // Upper torso
        spheres.push_back(CollisionSphere(Vec3(0, 1.3f * scale, 0), 0.15f * scale * widthScale));
        
        // Chest
        spheres.push_back(CollisionSphere(Vec3(0, 1.15f * scale, 0), 0.14f * scale * widthScale));
        
        // Mid torso
        spheres.push_back(CollisionSphere(Vec3(0, 1.0f * scale, 0), 0.12f * scale * widthScale));
        
        // Waist
        spheres.push_back(CollisionSphere(Vec3(0, 0.85f * scale, 0), 0.11f * scale * widthScale));
        
        // Hips
        spheres.push_back(CollisionSphere(Vec3(0, 0.75f * scale, 0), 0.13f * scale * widthScale));
        
        // Upper legs
        spheres.push_back(CollisionSphere(Vec3(-0.08f, 0.55f * scale, 0), 0.07f * scale * widthScale));
        spheres.push_back(CollisionSphere(Vec3(0.08f, 0.55f * scale, 0), 0.07f * scale * widthScale));
        
        // Lower legs
        spheres.push_back(CollisionSphere(Vec3(-0.08f, 0.3f * scale, 0), 0.05f * scale));
        spheres.push_back(CollisionSphere(Vec3(0.08f, 0.3f * scale, 0), 0.05f * scale));
        
        // Upper arms
        spheres.push_back(CollisionSphere(Vec3(-0.25f * widthScale, 1.25f * scale, 0), 0.04f * scale));
        spheres.push_back(CollisionSphere(Vec3(0.25f * widthScale, 1.25f * scale, 0), 0.04f * scale));
        
        return spheres;
    }
    
    static std::vector<CollisionCapsule> generateCapsulesFromBody(float height = 1.8f,
                                                                   float weight = 0.5f) {
        std::vector<CollisionCapsule> capsules;
        
        float scale = height / 1.8f;
        float widthScale = 1.0f + (weight - 0.5f) * 0.3f;
        
        // Torso
        capsules.push_back(CollisionCapsule(
            Vec3(0, 0.75f * scale, 0),
            Vec3(0, 1.35f * scale, 0),
            0.12f * scale * widthScale
        ));
        
        // Left leg
        capsules.push_back(CollisionCapsule(
            Vec3(-0.08f, 0.1f * scale, 0),
            Vec3(-0.08f, 0.7f * scale, 0),
            0.06f * scale * widthScale
        ));
        
        // Right leg
        capsules.push_back(CollisionCapsule(
            Vec3(0.08f, 0.1f * scale, 0),
            Vec3(0.08f, 0.7f * scale, 0),
            0.06f * scale * widthScale
        ));
        
        // Left arm
        capsules.push_back(CollisionCapsule(
            Vec3(-0.2f * widthScale, 1.25f * scale, 0),
            Vec3(-0.45f * widthScale, 1.1f * scale, 0),
            0.035f * scale
        ));
        
        // Right arm
        capsules.push_back(CollisionCapsule(
            Vec3(0.2f * widthScale, 1.25f * scale, 0),
            Vec3(0.45f * widthScale, 1.1f * scale, 0),
            0.035f * scale
        ));
        
        return capsules;
    }
};

}  // namespace luma
