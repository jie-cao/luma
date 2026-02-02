// Water Effects - Splashes, foam, shore effects, caustics
// Visual effects for water interaction
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

namespace luma {

// ============================================================================
// Splash Particle
// ============================================================================

struct SplashParticle {
    Vec3 position;
    Vec3 velocity;
    float size;
    float alpha;
    float life;
    float maxLife;
    
    enum class Type {
        Droplet,    // Water droplet flying up
        Spray,      // Fine mist
        Ring,       // Expanding ring on surface
        Splash      // Main splash shape
    };
    Type type = Type::Droplet;
    
    bool isAlive() const { return life < maxLife; }
    float getLifeRatio() const { return life / maxLife; }
};

// ============================================================================
// Foam Particle (on water surface)
// ============================================================================

struct FoamParticle {
    Vec2 position;      // XZ position on water
    float size;
    float alpha;
    float life;
    float maxLife;
    float rotation;
    float rotationSpeed;
};

// ============================================================================
// Shore Wave
// ============================================================================

struct ShoreWave {
    Vec2 startPoint;
    Vec2 endPoint;
    float progress;     // 0-1 along the shore
    float amplitude;
    float speed;
    float width;
    float foam;
};

// ============================================================================
// Splash Effect System
// ============================================================================

class SplashEffectSystem {
public:
    SplashEffectSystem() : rng_(std::random_device{}()) {}
    
    // Create splash at position with given strength (0-1)
    void createSplash(const Vec3& position, float strength, float waterHeight) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        
        int particleCount = static_cast<int>(20 + strength * 80);
        
        for (int i = 0; i < particleCount; i++) {
            SplashParticle p;
            
            // Random angle
            float angle = dist01(rng_) * 6.28318f;
            float speed = (0.5f + dist01(rng_)) * strength * 8.0f;
            float upSpeed = (1.0f + dist01(rng_)) * strength * 6.0f;
            
            // Spawn slightly above water
            p.position = position;
            p.position.y = waterHeight + dist01(rng_) * 0.1f;
            
            // Random direction with upward bias
            p.velocity = Vec3(
                std::cos(angle) * speed,
                upSpeed,
                std::sin(angle) * speed
            );
            
            p.size = 0.02f + dist01(rng_) * 0.06f * strength;
            p.alpha = 0.8f;
            p.life = 0.0f;
            p.maxLife = 0.5f + dist01(rng_) * 0.5f;
            p.type = SplashParticle::Type::Droplet;
            
            particles_.push_back(p);
        }
        
        // Add spray particles (finer mist)
        int sprayCount = static_cast<int>(strength * 50);
        for (int i = 0; i < sprayCount; i++) {
            SplashParticle p;
            
            float angle = dist01(rng_) * 6.28318f;
            float speed = dist01(rng_) * strength * 4.0f;
            
            p.position = position;
            p.position.y = waterHeight;
            
            p.velocity = Vec3(
                std::cos(angle) * speed,
                2.0f + dist01(rng_) * 3.0f * strength,
                std::sin(angle) * speed
            );
            
            p.size = 0.01f + dist01(rng_) * 0.02f;
            p.alpha = 0.5f;
            p.life = 0.0f;
            p.maxLife = 0.3f + dist01(rng_) * 0.3f;
            p.type = SplashParticle::Type::Spray;
            
            particles_.push_back(p);
        }
        
        // Add expanding ring
        SplashParticle ring;
        ring.position = position;
        ring.position.y = waterHeight;
        ring.velocity = Vec3(0, 0, 0);
        ring.size = 0.1f;
        ring.alpha = 0.8f * strength;
        ring.life = 0.0f;
        ring.maxLife = 1.0f;
        ring.type = SplashParticle::Type::Ring;
        particles_.push_back(ring);
    }
    
    // Create small ripple splash (walking through water)
    void createRippleSplash(const Vec3& position, float waterHeight, float speed) {
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        
        int count = static_cast<int>(5 + speed * 10);
        
        for (int i = 0; i < count; i++) {
            SplashParticle p;
            
            float angle = dist01(rng_) * 6.28318f;
            float s = dist01(rng_) * speed * 2.0f;
            
            p.position = position;
            p.position.y = waterHeight;
            
            p.velocity = Vec3(
                std::cos(angle) * s,
                0.5f + dist01(rng_) * speed,
                std::sin(angle) * s
            );
            
            p.size = 0.01f + dist01(rng_) * 0.02f;
            p.alpha = 0.6f;
            p.life = 0.0f;
            p.maxLife = 0.3f;
            p.type = SplashParticle::Type::Droplet;
            
            particles_.push_back(p);
        }
    }
    
    void update(float deltaTime, float waterHeight) {
        for (auto& p : particles_) {
            p.life += deltaTime;
            
            if (p.type == SplashParticle::Type::Ring) {
                // Rings expand outward
                p.size += deltaTime * 3.0f;
                p.alpha = (1.0f - p.getLifeRatio()) * 0.8f;
            } else {
                // Droplets and spray follow physics
                p.velocity.y -= 9.81f * deltaTime;  // Gravity
                p.position = p.position + p.velocity * deltaTime;
                
                // Air resistance
                p.velocity = p.velocity * (1.0f - 2.0f * deltaTime);
                
                // Fade out
                float t = p.getLifeRatio();
                p.alpha = 0.8f * (1.0f - t * t);
                
                // If droplet hits water, spawn small ripple
                if (p.position.y < waterHeight && p.velocity.y < 0) {
                    p.velocity.y = 0;
                    p.position.y = waterHeight;
                    p.life = p.maxLife;  // Kill particle
                }
            }
        }
        
        // Remove dead particles
        particles_.erase(
            std::remove_if(particles_.begin(), particles_.end(),
                [](const SplashParticle& p) { return !p.isAlive(); }),
            particles_.end());
    }
    
    const std::vector<SplashParticle>& getParticles() const { return particles_; }
    
    void clear() { particles_.clear(); }
    
private:
    std::vector<SplashParticle> particles_;
    std::mt19937 rng_;
};

// ============================================================================
// Foam System (surface foam patches)
// ============================================================================

class FoamSystem {
public:
    FoamSystem() : rng_(std::random_device{}()) {}
    
    // Add foam at position
    void addFoam(const Vec2& position, float intensity = 1.0f) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        
        int count = static_cast<int>(5 + intensity * 15);
        
        for (int i = 0; i < count; i++) {
            FoamParticle f;
            f.position = position + Vec2(dist(rng_) * 0.5f, dist(rng_) * 0.5f);
            f.size = 0.1f + dist01(rng_) * 0.3f * intensity;
            f.alpha = 0.5f + dist01(rng_) * 0.5f;
            f.life = 0.0f;
            f.maxLife = 2.0f + dist01(rng_) * 3.0f;
            f.rotation = dist01(rng_) * 6.28318f;
            f.rotationSpeed = dist(rng_) * 0.5f;
            
            foam_.push_back(f);
        }
    }
    
    void update(float deltaTime, const Vec2& flowDirection, float flowSpeed) {
        for (auto& f : foam_) {
            f.life += deltaTime;
            
            // Move with water flow
            f.position = f.position + flowDirection * flowSpeed * deltaTime;
            
            // Rotate
            f.rotation += f.rotationSpeed * deltaTime;
            
            // Fade
            float t = f.life / f.maxLife;
            if (t > 0.7f) {
                f.alpha = (1.0f - (t - 0.7f) / 0.3f) * 0.5f;
            }
            
            // Shrink at end
            if (t > 0.8f) {
                f.size *= 0.95f;
            }
        }
        
        // Remove dead foam
        foam_.erase(
            std::remove_if(foam_.begin(), foam_.end(),
                [](const FoamParticle& f) { return f.life >= f.maxLife; }),
            foam_.end());
    }
    
    const std::vector<FoamParticle>& getFoam() const { return foam_; }
    
    void clear() { foam_.clear(); }
    
private:
    std::vector<FoamParticle> foam_;
    std::mt19937 rng_;
};

// ============================================================================
// Shore Effect System
// ============================================================================

class ShoreEffectSystem {
public:
    ShoreEffectSystem() : rng_(std::random_device{}()) {}
    
    // Define shore line (series of points)
    void setShoreLine(const std::vector<Vec2>& points) {
        shorePoints_ = points;
    }
    
    // Update shore waves
    void update(float deltaTime) {
        time_ += deltaTime;
        
        // Update existing waves
        for (auto& wave : waves_) {
            wave.progress += wave.speed * deltaTime;
            
            // Foam increases as wave approaches shore
            wave.foam = std::min(1.0f, wave.progress * 2.0f);
            
            // Amplitude decreases
            wave.amplitude *= (1.0f - deltaTime * 0.5f);
        }
        
        // Remove completed waves
        waves_.erase(
            std::remove_if(waves_.begin(), waves_.end(),
                [](const ShoreWave& w) { return w.progress > 1.0f || w.amplitude < 0.01f; }),
            waves_.end());
        
        // Spawn new waves periodically
        if (time_ - lastWaveTime_ > waveInterval_) {
            spawnWave();
            lastWaveTime_ = time_;
        }
    }
    
    // Get wave height at shore position (0-1 along shore)
    float getWaveHeight(float shorePosition) const {
        float height = 0.0f;
        
        for (const auto& wave : waves_) {
            // Check if this wave affects this position
            float dist = std::abs(shorePosition - wave.progress);
            if (dist < wave.width) {
                float factor = 1.0f - dist / wave.width;
                factor = factor * factor;  // Smooth falloff
                
                // Wave shape (builds up then crashes)
                float waveShape = std::sin(wave.progress * 3.14159f);
                height += wave.amplitude * factor * waveShape;
            }
        }
        
        return height;
    }
    
    // Get foam amount at position
    float getFoamAmount(const Vec2& position) const {
        float foam = 0.0f;
        
        for (const auto& wave : waves_) {
            // Simple distance check from wave position
            Vec2 wavePos = wave.startPoint.lerp(wave.endPoint, wave.progress);
            float dist = (position - wavePos).length();
            
            if (dist < wave.width * 5.0f) {
                float factor = 1.0f - dist / (wave.width * 5.0f);
                foam = std::max(foam, wave.foam * factor);
            }
        }
        
        return foam;
    }
    
    const std::vector<ShoreWave>& getWaves() const { return waves_; }
    
    void setWaveInterval(float interval) { waveInterval_ = interval; }
    
private:
    void spawnWave() {
        if (shorePoints_.size() < 2) return;
        
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        
        ShoreWave wave;
        
        // Pick random segment of shore
        int segIdx = static_cast<int>(dist01(rng_) * (shorePoints_.size() - 1));
        wave.startPoint = shorePoints_[segIdx];
        wave.endPoint = shorePoints_[segIdx + 1];
        
        wave.progress = 0.0f;
        wave.amplitude = 0.1f + dist01(rng_) * 0.2f;
        wave.speed = 0.3f + dist01(rng_) * 0.2f;
        wave.width = 0.1f + dist01(rng_) * 0.1f;
        wave.foam = 0.0f;
        
        waves_.push_back(wave);
    }
    
    std::vector<Vec2> shorePoints_;
    std::vector<ShoreWave> waves_;
    float time_ = 0.0f;
    float lastWaveTime_ = 0.0f;
    float waveInterval_ = 3.0f;
    std::mt19937 rng_;
};

// ============================================================================
// Dynamic Caustics Generator
// ============================================================================

class CausticsGenerator {
public:
    // Generate caustic texture data
    static std::vector<float> generateCausticsTexture(int size, float time, float scale = 1.0f) {
        std::vector<float> data(size * size);
        
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float u = (float)x / size;
                float v = (float)y / size;
                
                // Multiple overlapping patterns
                float c1 = std::sin(u * 20.0f * scale + time) * std::sin(v * 20.0f * scale + time * 0.7f);
                float c2 = std::sin(u * 15.0f * scale - time * 0.5f) * std::sin(v * 18.0f * scale + time * 1.1f);
                float c3 = std::sin((u + v) * 12.0f * scale + time * 0.3f);
                
                // Voronoi-like pattern for more realistic caustics
                float voronoi = voronoiNoise(u * 5.0f * scale, v * 5.0f * scale, time * 0.2f);
                
                float value = (c1 + c2 + c3) / 3.0f * 0.5f + voronoi * 0.5f;
                value = value * 0.5f + 0.5f;  // Normalize to 0-1
                
                // Sharpen for caustic look
                value = std::pow(value, 2.0f);
                
                data[y * size + x] = value;
            }
        }
        
        return data;
    }
    
    // Get caustic intensity at world position
    static float getCausticIntensity(const Vec3& worldPos, float time, 
                                     float waterSurfaceY, float scale = 1.0f) {
        if (worldPos.y > waterSurfaceY) return 0.0f;
        
        float depth = waterSurfaceY - worldPos.y;
        
        // Multiple sine waves
        float c1 = std::sin(worldPos.x * scale + time) * std::sin(worldPos.z * scale * 1.3f + time * 0.7f);
        float c2 = std::sin(worldPos.x * scale * 0.7f - time * 0.5f) * std::sin(worldPos.z * scale + time * 1.1f);
        
        float caustic = (c1 + c2) * 0.5f + 0.5f;
        caustic = caustic * caustic;  // Sharpen
        
        // Depth falloff
        float depthFalloff = std::exp(-depth * 0.1f);
        
        return caustic * depthFalloff;
    }
    
private:
    static float voronoiNoise(float x, float y, float time) {
        float minDist = 1.0f;
        
        int ix = static_cast<int>(std::floor(x));
        int iy = static_cast<int>(std::floor(y));
        
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                // Pseudo-random point in cell
                float px = ix + dx + hash(ix + dx, iy + dy) * 0.8f + 0.1f;
                float py = iy + dy + hash(iy + dy, ix + dx) * 0.8f + 0.1f;
                
                // Animate point
                px += std::sin(time + hash(ix + dx, iy + dy) * 10.0f) * 0.1f;
                py += std::cos(time * 0.7f + hash(iy + dy, ix + dx) * 10.0f) * 0.1f;
                
                float dist = std::sqrt((x - px) * (x - px) + (y - py) * (y - py));
                minDist = std::min(minDist, dist);
            }
        }
        
        return minDist;
    }
    
    static float hash(int x, int y) {
        int n = x + y * 57;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f) * 0.5f + 0.5f;
    }
};

// ============================================================================
// Wet Surface Effect
// ============================================================================

struct WetSurface {
    Vec3 position;
    float radius;
    float wetness;      // 0-1
    float drySpeed;     // How fast it dries
    float age;
};

class WetSurfaceSystem {
public:
    void addWetSpot(const Vec3& position, float radius, float intensity = 1.0f) {
        WetSurface wet;
        wet.position = position;
        wet.radius = radius;
        wet.wetness = intensity;
        wet.drySpeed = 0.1f;
        wet.age = 0.0f;
        
        wetSpots_.push_back(wet);
    }
    
    void update(float deltaTime) {
        for (auto& wet : wetSpots_) {
            wet.age += deltaTime;
            wet.wetness -= wet.drySpeed * deltaTime;
        }
        
        // Remove dried spots
        wetSpots_.erase(
            std::remove_if(wetSpots_.begin(), wetSpots_.end(),
                [](const WetSurface& w) { return w.wetness <= 0.0f; }),
            wetSpots_.end());
    }
    
    // Get wetness at position
    float getWetness(const Vec3& position) const {
        float wetness = 0.0f;
        
        for (const auto& wet : wetSpots_) {
            Vec3 diff = position - wet.position;
            float dist = std::sqrt(diff.x * diff.x + diff.z * diff.z);
            
            if (dist < wet.radius) {
                float factor = 1.0f - dist / wet.radius;
                factor = factor * factor;
                wetness = std::max(wetness, wet.wetness * factor);
            }
        }
        
        return wetness;
    }
    
    const std::vector<WetSurface>& getWetSpots() const { return wetSpots_; }
    
private:
    std::vector<WetSurface> wetSpots_;
};

// ============================================================================
// Water Effects Manager
// ============================================================================

class WaterEffectsManager {
public:
    static WaterEffectsManager& getInstance() {
        static WaterEffectsManager instance;
        return instance;
    }
    
    SplashEffectSystem& getSplash() { return splash_; }
    FoamSystem& getFoam() { return foam_; }
    ShoreEffectSystem& getShore() { return shore_; }
    WetSurfaceSystem& getWetSurface() { return wetSurface_; }
    
    void update(float deltaTime, float waterHeight, const Vec2& flowDirection, float flowSpeed) {
        splash_.update(deltaTime, waterHeight);
        foam_.update(deltaTime, flowDirection, flowSpeed);
        shore_.update(deltaTime);
        wetSurface_.update(deltaTime);
    }
    
    // Convenience: Create splash with foam
    void createSplashWithFoam(const Vec3& position, float strength, float waterHeight) {
        splash_.createSplash(position, strength, waterHeight);
        foam_.addFoam(Vec2(position.x, position.z), strength);
    }
    
private:
    WaterEffectsManager() = default;
    
    SplashEffectSystem splash_;
    FoamSystem foam_;
    ShoreEffectSystem shore_;
    WetSurfaceSystem wetSurface_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline WaterEffectsManager& getWaterEffects() {
    return WaterEffectsManager::getInstance();
}

}  // namespace luma
