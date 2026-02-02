// Underwater Effects - Post-processing, scattering, bubbles
// Immersive underwater experience
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <cmath>
#include <random>

namespace luma {

// ============================================================================
// Underwater Visual Parameters
// ============================================================================

struct UnderwaterParams {
    // Fog/Scattering
    Vec3 waterColor{0.1f, 0.3f, 0.4f};
    Vec3 scatterColor{0.2f, 0.4f, 0.5f};
    float fogDensity = 0.05f;
    float fogStart = 0.0f;
    float fogEnd = 50.0f;
    
    // Color absorption (red light absorbed first)
    Vec3 absorptionCoefficients{0.4f, 0.1f, 0.05f};  // RGB absorption rates
    float maxAbsorptionDepth = 30.0f;
    
    // Caustics (light patterns on surfaces)
    float causticIntensity = 0.5f;
    float causticScale = 1.0f;
    float causticSpeed = 1.0f;
    
    // Distortion
    float distortionStrength = 0.02f;
    float distortionSpeed = 1.0f;
    float distortionScale = 10.0f;
    
    // God rays
    bool enableGodRays = true;
    float godRayIntensity = 0.3f;
    float godRayDecay = 0.95f;
    int godRaySamples = 32;
    
    // Blur
    float depthBlurStrength = 0.5f;
    float depthBlurStart = 10.0f;
    float depthBlurEnd = 50.0f;
    
    // Vignette
    float vignetteStrength = 0.3f;
    float vignetteRadius = 0.7f;
    
    // Particles
    bool enableParticles = true;
    int particleCount = 200;
    float particleSize = 0.02f;
};

// ============================================================================
// Underwater Bubble
// ============================================================================

struct UnderwaterBubble {
    Vec3 position;
    Vec3 velocity;
    float size;
    float alpha;
    float wobblePhase;
    float lifetime;
    float age;
    
    bool isAlive() const { return age < lifetime; }
};

// ============================================================================
// Floating Particle (dust, plankton)
// ============================================================================

struct FloatingParticle {
    Vec3 position;
    Vec3 velocity;
    float size;
    float alpha;
    float phase;  // For gentle movement
};

// ============================================================================
// Underwater Effect System
// ============================================================================

class UnderwaterEffectSystem {
public:
    UnderwaterEffectSystem() : rng_(std::random_device{}()) {}
    
    // === State ===
    
    void setUnderwater(bool underwater, float depth = 0.0f) {
        wasUnderwater_ = isUnderwater_;
        isUnderwater_ = underwater;
        currentDepth_ = depth;
        
        if (underwater && !wasUnderwater_) {
            onEnterWater();
        } else if (!underwater && wasUnderwater_) {
            onExitWater();
        }
    }
    
    bool isUnderwater() const { return isUnderwater_; }
    float getCurrentDepth() const { return currentDepth_; }
    
    // === Update ===
    
    void update(float deltaTime, const Vec3& cameraPosition) {
        time_ += deltaTime;
        cameraPos_ = cameraPosition;
        
        if (isUnderwater_) {
            updateBubbles(deltaTime);
            updateFloatingParticles(deltaTime, cameraPosition);
            updateCaustics(deltaTime);
        }
    }
    
    // === Bubbles ===
    
    void spawnBubble(const Vec3& position, float size = 0.05f) {
        UnderwaterBubble bubble;
        bubble.position = position;
        
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        bubble.velocity = Vec3(dist(rng_) * 0.2f, 1.0f + dist(rng_) * 0.3f, dist(rng_) * 0.2f);
        bubble.size = size * (0.5f + std::abs(dist(rng_)) * 0.5f);
        bubble.alpha = 0.6f;
        bubble.wobblePhase = dist(rng_) * 6.28f;
        bubble.lifetime = 3.0f + dist(rng_) * 2.0f;
        bubble.age = 0.0f;
        
        bubbles_.push_back(bubble);
    }
    
    void spawnBubbleBurst(const Vec3& position, int count = 10) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (int i = 0; i < count; i++) {
            Vec3 offset(dist(rng_) * 0.3f, dist(rng_) * 0.2f, dist(rng_) * 0.3f);
            float size = 0.02f + std::abs(dist(rng_)) * 0.08f;
            spawnBubble(position + offset, size);
        }
    }
    
    const std::vector<UnderwaterBubble>& getBubbles() const { return bubbles_; }
    
    // === Floating Particles ===
    
    const std::vector<FloatingParticle>& getFloatingParticles() const { return particles_; }
    
    // === Caustics ===
    
    // Get caustic intensity at a world position (for surface lighting)
    float getCausticIntensity(const Vec3& worldPos) const {
        if (!isUnderwater_ && worldPos.y > waterSurfaceY_) return 0.0f;
        
        // Animated caustic pattern using multiple sine waves
        float scale = params_.causticScale;
        float t = time_ * params_.causticSpeed;
        
        float c1 = std::sin(worldPos.x * scale + t) * std::sin(worldPos.z * scale * 1.3f + t * 0.7f);
        float c2 = std::sin(worldPos.x * scale * 0.7f - t * 0.5f) * std::sin(worldPos.z * scale + t * 1.1f);
        float c3 = std::sin((worldPos.x + worldPos.z) * scale * 0.5f + t * 0.3f);
        
        float caustic = (c1 + c2 + c3) / 3.0f;
        caustic = caustic * 0.5f + 0.5f;  // Normalize to 0-1
        caustic = caustic * caustic;  // Sharpen
        
        // Depth falloff
        float depthFactor = 1.0f - std::min(1.0f, (waterSurfaceY_ - worldPos.y) / 20.0f);
        
        return caustic * params_.causticIntensity * depthFactor;
    }
    
    // === Post-Processing Parameters ===
    
    // Get fog color with depth-based absorption
    Vec3 getFogColor() const {
        if (!isUnderwater_) return Vec3(1, 1, 1);
        
        // Deeper = more blue, less red
        Vec3 color = params_.waterColor;
        float depthFactor = std::min(1.0f, currentDepth_ / params_.maxAbsorptionDepth);
        
        // Apply absorption
        color.x *= 1.0f - params_.absorptionCoefficients.x * depthFactor;
        color.y *= 1.0f - params_.absorptionCoefficients.y * depthFactor;
        color.z *= 1.0f - params_.absorptionCoefficients.z * depthFactor;
        
        return color;
    }
    
    float getFogDensity() const {
        if (!isUnderwater_) return 0.0f;
        return params_.fogDensity * (1.0f + currentDepth_ * 0.02f);
    }
    
    // Get distortion offset for screen UV
    Vec2 getDistortionOffset(const Vec2& screenUV) const {
        if (!isUnderwater_) return Vec2(0, 0);
        
        float t = time_ * params_.distortionSpeed;
        float scale = params_.distortionScale;
        
        float dx = std::sin(screenUV.y * scale + t) * std::sin(screenUV.x * scale * 0.7f + t * 0.5f);
        float dy = std::sin(screenUV.x * scale + t * 0.7f) * std::sin(screenUV.y * scale * 1.3f + t);
        
        return Vec2(dx, dy) * params_.distortionStrength;
    }
    
    // Get vignette factor (0 = full dark, 1 = no vignette)
    float getVignetteFactor(const Vec2& screenUV) const {
        if (!isUnderwater_) return 1.0f;
        
        float dx = screenUV.x - 0.5f;
        float dy = screenUV.y - 0.5f;
        float dist = std::sqrt(dx * dx + dy * dy) * 2.0f;
        
        float vignette = 1.0f - std::smoothstep(params_.vignetteRadius, 1.0f, dist) * params_.vignetteStrength;
        return vignette;
    }
    
    // Get depth blur amount
    float getDepthBlur(float viewDistance) const {
        if (!isUnderwater_) return 0.0f;
        
        if (viewDistance < params_.depthBlurStart) return 0.0f;
        if (viewDistance > params_.depthBlurEnd) return params_.depthBlurStrength;
        
        float t = (viewDistance - params_.depthBlurStart) / (params_.depthBlurEnd - params_.depthBlurStart);
        return t * params_.depthBlurStrength;
    }
    
    // === Parameters ===
    
    UnderwaterParams& getParams() { return params_; }
    const UnderwaterParams& getParams() const { return params_; }
    
    void setWaterSurfaceY(float y) { waterSurfaceY_ = y; }
    
    // === Callbacks ===
    
    std::function<void()> onEnterWaterCallback;
    std::function<void()> onExitWaterCallback;
    
private:
    float smoothstep(float edge0, float edge1, float x) const {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
    void onEnterWater() {
        // Spawn initial bubbles
        spawnBubbleBurst(cameraPos_ + Vec3(0, -0.5f, 0), 20);
        
        // Initialize floating particles
        initializeFloatingParticles();
        
        if (onEnterWaterCallback) onEnterWaterCallback();
    }
    
    void onExitWater() {
        if (onExitWaterCallback) onExitWaterCallback();
    }
    
    void updateBubbles(float deltaTime) {
        for (auto& bubble : bubbles_) {
            bubble.age += deltaTime;
            
            // Rise with wobble
            float wobble = std::sin(bubble.wobblePhase + time_ * 3.0f) * 0.3f;
            bubble.position.x += wobble * deltaTime;
            bubble.position = bubble.position + bubble.velocity * deltaTime;
            
            // Slow down as they rise
            bubble.velocity.y *= 0.99f;
            
            // Fade out
            float lifeRatio = bubble.age / bubble.lifetime;
            bubble.alpha = 0.6f * (1.0f - lifeRatio * lifeRatio);
            
            // Pop if reaching surface
            if (bubble.position.y > waterSurfaceY_) {
                bubble.age = bubble.lifetime;  // Mark for removal
            }
        }
        
        // Remove dead bubbles
        bubbles_.erase(
            std::remove_if(bubbles_.begin(), bubbles_.end(),
                [](const UnderwaterBubble& b) { return !b.isAlive(); }),
            bubbles_.end());
        
        // Occasional ambient bubbles from camera
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(rng_) < deltaTime * 2.0f) {
            Vec3 offset((dist(rng_) - 0.5f) * 3.0f, -1.0f, (dist(rng_) - 0.5f) * 3.0f);
            spawnBubble(cameraPos_ + offset, 0.01f + dist(rng_) * 0.02f);
        }
    }
    
    void initializeFloatingParticles() {
        particles_.clear();
        particles_.reserve(params_.particleCount);
        
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (int i = 0; i < params_.particleCount; i++) {
            FloatingParticle p;
            p.position = cameraPos_ + Vec3(
                dist(rng_) * 10.0f,
                dist(rng_) * 5.0f,
                dist(rng_) * 10.0f
            );
            p.velocity = Vec3(dist(rng_) * 0.1f, dist(rng_) * 0.05f, dist(rng_) * 0.1f);
            p.size = params_.particleSize * (0.5f + std::abs(dist(rng_)) * 0.5f);
            p.alpha = 0.3f + std::abs(dist(rng_)) * 0.3f;
            p.phase = dist(rng_) * 6.28f;
            
            particles_.push_back(p);
        }
    }
    
    void updateFloatingParticles(float deltaTime, const Vec3& cameraPos) {
        float range = 15.0f;
        
        for (auto& p : particles_) {
            // Gentle drifting motion
            float drift = std::sin(p.phase + time_ * 0.5f) * 0.1f;
            p.position.x += (p.velocity.x + drift) * deltaTime;
            p.position.y += p.velocity.y * deltaTime;
            p.position.z += (p.velocity.z + drift * 0.7f) * deltaTime;
            
            // Wrap around camera
            Vec3 toCamera = p.position - cameraPos;
            if (toCamera.x > range) p.position.x -= range * 2;
            if (toCamera.x < -range) p.position.x += range * 2;
            if (toCamera.y > range * 0.5f) p.position.y -= range;
            if (toCamera.y < -range * 0.5f) p.position.y += range;
            if (toCamera.z > range) p.position.z -= range * 2;
            if (toCamera.z < -range) p.position.z += range * 2;
        }
    }
    
    void updateCaustics(float deltaTime) {
        // Caustics are calculated on-demand via getCausticIntensity()
    }
    
    // State
    bool isUnderwater_ = false;
    bool wasUnderwater_ = false;
    float currentDepth_ = 0.0f;
    float waterSurfaceY_ = 0.0f;
    float time_ = 0.0f;
    Vec3 cameraPos_;
    
    // Parameters
    UnderwaterParams params_;
    
    // Particles
    std::vector<UnderwaterBubble> bubbles_;
    std::vector<FloatingParticle> particles_;
    
    // Random
    std::mt19937 rng_;
};

// ============================================================================
// Underwater Manager - Singleton
// ============================================================================

class UnderwaterManager {
public:
    static UnderwaterManager& getInstance() {
        static UnderwaterManager instance;
        return instance;
    }
    
    UnderwaterEffectSystem& getEffects() { return effects_; }
    const UnderwaterEffectSystem& getEffects() const { return effects_; }
    
    void update(float deltaTime, const Vec3& cameraPos, float waterSurfaceY) {
        effects_.setWaterSurfaceY(waterSurfaceY);
        
        bool underwater = cameraPos.y < waterSurfaceY;
        float depth = underwater ? (waterSurfaceY - cameraPos.y) : 0.0f;
        
        effects_.setUnderwater(underwater, depth);
        effects_.update(deltaTime, cameraPos);
    }
    
private:
    UnderwaterManager() = default;
    UnderwaterEffectSystem effects_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline UnderwaterManager& getUnderwaterManager() {
    return UnderwaterManager::getInstance();
}

inline UnderwaterEffectSystem& getUnderwaterEffects() {
    return UnderwaterManager::getInstance().getEffects();
}

}  // namespace luma
