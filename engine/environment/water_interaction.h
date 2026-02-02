// Water Interaction System - Ripples, splashes, and dynamic water effects
// Interactive water surface with real-time deformation
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace luma {

// ============================================================================
// Ripple - Single ripple effect
// ============================================================================

struct Ripple {
    Vec2 position;          // World position XZ
    float birthTime;        // When ripple was created
    float amplitude;        // Initial amplitude
    float frequency;        // Wave frequency
    float speed;            // Expansion speed
    float lifetime;         // How long ripple lasts
    float decay;            // Amplitude decay rate
    
    bool isActive(float currentTime) const {
        return (currentTime - birthTime) < lifetime;
    }
    
    float getRadius(float currentTime) const {
        return (currentTime - birthTime) * speed;
    }
    
    float getAmplitude(float currentTime) const {
        float age = currentTime - birthTime;
        float t = age / lifetime;
        return amplitude * std::exp(-decay * age) * (1.0f - t * t);
    }
};

// ============================================================================
// Ripple Simulation - CPU-based ripple system
// ============================================================================

class RippleSimulation {
public:
    RippleSimulation(int resolution = 256) : resolution_(resolution) {
        // Allocate height buffers for wave equation simulation
        heightCurrent_.resize(resolution * resolution, 0.0f);
        heightPrevious_.resize(resolution * resolution, 0.0f);
        velocity_.resize(resolution * resolution, 0.0f);
    }
    
    // Add disturbance at position (normalized 0-1)
    void addDisturbance(float u, float v, float strength, float radius = 0.05f) {
        int cx = static_cast<int>(u * resolution_);
        int cy = static_cast<int>(v * resolution_);
        int r = static_cast<int>(radius * resolution_);
        
        for (int y = std::max(0, cy - r); y < std::min(resolution_, cy + r); y++) {
            for (int x = std::max(0, cx - r); x < std::min(resolution_, cx + r); x++) {
                float dx = (float)(x - cx) / r;
                float dy = (float)(y - cy) / r;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist < 1.0f) {
                    float falloff = 1.0f - dist;
                    falloff = falloff * falloff;  // Smooth falloff
                    heightCurrent_[y * resolution_ + x] += strength * falloff;
                }
            }
        }
    }
    
    // Simulate one step using wave equation
    void simulate(float deltaTime) {
        float c = waveSpeed_ * deltaTime;
        float c2 = c * c;
        float damping = 1.0f - dampingFactor_ * deltaTime;
        
        // Wave equation: d²h/dt² = c² * (d²h/dx² + d²h/dy²)
        for (int y = 1; y < resolution_ - 1; y++) {
            for (int x = 1; x < resolution_ - 1; x++) {
                int idx = y * resolution_ + x;
                
                // Laplacian (second spatial derivative)
                float laplacian = 
                    heightCurrent_[(y-1) * resolution_ + x] +
                    heightCurrent_[(y+1) * resolution_ + x] +
                    heightCurrent_[y * resolution_ + (x-1)] +
                    heightCurrent_[y * resolution_ + (x+1)] -
                    4.0f * heightCurrent_[idx];
                
                // Verlet integration
                float newHeight = 2.0f * heightCurrent_[idx] - heightPrevious_[idx] + c2 * laplacian;
                newHeight *= damping;
                
                heightPrevious_[idx] = heightCurrent_[idx];
                heightCurrent_[idx] = newHeight;
            }
        }
        
        // Boundary conditions (absorbing)
        for (int i = 0; i < resolution_; i++) {
            heightCurrent_[i] *= 0.9f;  // Top
            heightCurrent_[(resolution_-1) * resolution_ + i] *= 0.9f;  // Bottom
            heightCurrent_[i * resolution_] *= 0.9f;  // Left
            heightCurrent_[i * resolution_ + resolution_ - 1] *= 0.9f;  // Right
        }
    }
    
    // Get height at normalized position
    float getHeight(float u, float v) const {
        int x = std::clamp(static_cast<int>(u * resolution_), 0, resolution_ - 1);
        int y = std::clamp(static_cast<int>(v * resolution_), 0, resolution_ - 1);
        return heightCurrent_[y * resolution_ + x];
    }
    
    // Get normal at normalized position
    Vec3 getNormal(float u, float v) const {
        float delta = 1.0f / resolution_;
        
        float hL = getHeight(u - delta, v);
        float hR = getHeight(u + delta, v);
        float hD = getHeight(u, v - delta);
        float hU = getHeight(u, v + delta);
        
        Vec3 normal(hL - hR, 2.0f * delta * normalStrength_, hD - hU);
        return normal.normalized();
    }
    
    // Get height field for GPU upload
    const std::vector<float>& getHeightField() const { return heightCurrent_; }
    int getResolution() const { return resolution_; }
    
    // Configuration
    void setWaveSpeed(float speed) { waveSpeed_ = speed; }
    void setDamping(float damping) { dampingFactor_ = damping; }
    void setNormalStrength(float strength) { normalStrength_ = strength; }
    
    void clear() {
        std::fill(heightCurrent_.begin(), heightCurrent_.end(), 0.0f);
        std::fill(heightPrevious_.begin(), heightPrevious_.end(), 0.0f);
    }
    
private:
    int resolution_;
    std::vector<float> heightCurrent_;
    std::vector<float> heightPrevious_;
    std::vector<float> velocity_;
    
    float waveSpeed_ = 5.0f;
    float dampingFactor_ = 0.5f;
    float normalStrength_ = 1.0f;
};

// ============================================================================
// Analytical Ripple System (for point ripples)
// ============================================================================

class AnalyticalRippleSystem {
public:
    void addRipple(const Vec2& position, float amplitude = 0.1f, 
                   float speed = 2.0f, float lifetime = 3.0f) {
        Ripple r;
        r.position = position;
        r.birthTime = currentTime_;
        r.amplitude = amplitude;
        r.frequency = 8.0f;
        r.speed = speed;
        r.lifetime = lifetime;
        r.decay = 2.0f;
        
        ripples_.push_back(r);
    }
    
    void update(float deltaTime) {
        currentTime_ += deltaTime;
        
        // Remove dead ripples
        ripples_.erase(
            std::remove_if(ripples_.begin(), ripples_.end(),
                [this](const Ripple& r) { return !r.isActive(currentTime_); }),
            ripples_.end());
    }
    
    // Calculate total displacement at a point
    float getHeightAt(const Vec2& pos) const {
        float height = 0.0f;
        
        for (const auto& r : ripples_) {
            if (!r.isActive(currentTime_)) continue;
            
            float dx = pos.x - r.position.x;
            float dy = pos.y - r.position.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            float radius = r.getRadius(currentTime_);
            float ringWidth = 2.0f;  // Width of the ripple ring
            
            // Distance from the expanding ring
            float ringDist = std::abs(dist - radius);
            
            if (ringDist < ringWidth) {
                float amp = r.getAmplitude(currentTime_);
                float phase = dist * r.frequency - currentTime_ * r.speed * r.frequency;
                float wave = std::sin(phase);
                
                // Falloff from ring center
                float falloff = 1.0f - ringDist / ringWidth;
                falloff = falloff * falloff;
                
                height += amp * wave * falloff;
            }
        }
        
        return height;
    }
    
    // Calculate normal at a point
    Vec3 getNormalAt(const Vec2& pos, float delta = 0.1f) const {
        float hL = getHeightAt(Vec2(pos.x - delta, pos.y));
        float hR = getHeightAt(Vec2(pos.x + delta, pos.y));
        float hD = getHeightAt(Vec2(pos.x, pos.y - delta));
        float hU = getHeightAt(Vec2(pos.x, pos.y + delta));
        
        Vec3 normal(hL - hR, 2.0f * delta, hD - hU);
        return normal.normalized();
    }
    
    const std::vector<Ripple>& getRipples() const { return ripples_; }
    int getActiveRippleCount() const { return static_cast<int>(ripples_.size()); }
    
    void clear() { ripples_.clear(); }
    
private:
    std::vector<Ripple> ripples_;
    float currentTime_ = 0.0f;
};

// ============================================================================
// Water Interaction Manager
// ============================================================================

struct WaterInteractor {
    std::string id;
    Vec3 position;
    Vec3 velocity;
    float radius;
    float lastWaterHeight;
    bool wasInWater;
    float submergedDepth;
    
    // Interaction settings
    float rippleStrength = 0.1f;
    float splashThreshold = 1.0f;  // Velocity threshold for splash
    float movementRippleInterval = 0.2f;  // Seconds between movement ripples
    float lastRippleTime = 0.0f;
};

class WaterInteractionManager {
public:
    static WaterInteractionManager& getInstance() {
        static WaterInteractionManager instance;
        return instance;
    }
    
    // Register an interactor (character, object, etc.)
    void registerInteractor(const std::string& id, float radius = 0.5f) {
        WaterInteractor interactor;
        interactor.id = id;
        interactor.radius = radius;
        interactor.wasInWater = false;
        interactor.submergedDepth = 0.0f;
        interactors_[id] = interactor;
    }
    
    void unregisterInteractor(const std::string& id) {
        interactors_.erase(id);
    }
    
    // Update interactor position and check for water interaction
    void updateInteractor(const std::string& id, const Vec3& position, 
                         const Vec3& velocity, float waterHeight) {
        auto it = interactors_.find(id);
        if (it == interactors_.end()) return;
        
        WaterInteractor& interactor = it->second;
        Vec3 prevPos = interactor.position;
        bool wasInWater = interactor.wasInWater;
        
        interactor.position = position;
        interactor.velocity = velocity;
        interactor.lastWaterHeight = waterHeight;
        
        // Check if in water
        bool isInWater = position.y < waterHeight;
        interactor.wasInWater = isInWater;
        interactor.submergedDepth = isInWater ? (waterHeight - position.y) : 0.0f;
        
        // Entry/exit detection
        if (isInWater && !wasInWater) {
            // Entering water - create splash
            float speed = velocity.length();
            if (speed > interactor.splashThreshold) {
                onWaterEntry(interactor, speed);
            } else {
                // Gentle entry - just ripple
                createRipple(Vec2(position.x, position.z), interactor.rippleStrength * 2.0f);
            }
        } else if (!isInWater && wasInWater) {
            // Exiting water
            float speed = velocity.length();
            if (speed > interactor.splashThreshold * 0.5f) {
                onWaterExit(interactor, speed);
            }
        } else if (isInWater) {
            // Moving in water - create movement ripples
            float horizontalSpeed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
            if (horizontalSpeed > 0.1f && 
                currentTime_ - interactor.lastRippleTime > interactor.movementRippleInterval) {
                float strength = std::min(horizontalSpeed * 0.05f, 0.1f);
                createRipple(Vec2(position.x, position.z), strength);
                interactor.lastRippleTime = currentTime_;
            }
        }
    }
    
    // Update all systems
    void update(float deltaTime) {
        currentTime_ += deltaTime;
        
        // Update grid-based simulation
        rippleSim_.simulate(deltaTime);
        
        // Update analytical ripples
        analyticalRipples_.update(deltaTime);
    }
    
    // Create ripple at world position
    void createRipple(const Vec2& worldPos, float strength = 0.1f) {
        // Add to analytical system
        analyticalRipples_.addRipple(worldPos, strength);
        
        // Also add to grid simulation if within bounds
        // (would need to convert world pos to UV)
    }
    
    // Create ripple in grid simulation (UV coordinates)
    void createGridRipple(float u, float v, float strength = 0.1f) {
        rippleSim_.addDisturbance(u, v, strength);
    }
    
    // Get combined height displacement at world position
    float getHeightDisplacement(const Vec2& worldPos) const {
        return analyticalRipples_.getHeightAt(worldPos);
    }
    
    // Get combined normal at world position
    Vec3 getNormal(const Vec2& worldPos) const {
        return analyticalRipples_.getNormalAt(worldPos);
    }
    
    // Get grid simulation
    RippleSimulation& getGridSimulation() { return rippleSim_; }
    const RippleSimulation& getGridSimulation() const { return rippleSim_; }
    
    // Get analytical ripples
    AnalyticalRippleSystem& getAnalyticalRipples() { return analyticalRipples_; }
    const AnalyticalRippleSystem& getAnalyticalRipples() const { return analyticalRipples_; }
    
    // Callbacks
    std::function<void(const Vec3& pos, float strength)> onSplash;
    std::function<void(const Vec3& pos)> onBubbles;
    
private:
    WaterInteractionManager() = default;
    
    void onWaterEntry(WaterInteractor& interactor, float speed) {
        float splashStrength = std::min(speed * 0.2f, 1.0f);
        
        // Create big ripple
        createRipple(Vec2(interactor.position.x, interactor.position.z), 
                    splashStrength * 0.5f);
        
        // Trigger splash effect
        if (onSplash) {
            onSplash(interactor.position, splashStrength);
        }
        
        // Trigger bubbles
        if (onBubbles) {
            onBubbles(interactor.position);
        }
    }
    
    void onWaterExit(WaterInteractor& interactor, float speed) {
        float strength = std::min(speed * 0.1f, 0.5f);
        createRipple(Vec2(interactor.position.x, interactor.position.z), strength);
        
        // Smaller splash on exit
        if (onSplash) {
            onSplash(interactor.position, strength * 0.5f);
        }
    }
    
    std::unordered_map<std::string, WaterInteractor> interactors_;
    RippleSimulation rippleSim_{256};
    AnalyticalRippleSystem analyticalRipples_;
    float currentTime_ = 0.0f;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline WaterInteractionManager& getWaterInteraction() {
    return WaterInteractionManager::getInstance();
}

}  // namespace luma
