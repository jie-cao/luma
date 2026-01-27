// Particle Modules - Modular behavior system
// Force fields, turbulence, color over lifetime, etc.
#pragma once

#include "particle.h"
#include <functional>
#include <cmath>

namespace luma {

// ===== Gradient Key =====
template<typename T>
struct GradientKey {
    float time;  // 0 to 1
    T value;
    
    GradientKey() : time(0), value(T{}) {}
    GradientKey(float t, T v) : time(t), value(v) {}
};

// ===== Color Gradient =====
class ColorGradient {
public:
    ColorGradient() {
        // Default white to white
        keys_.push_back({0.0f, Vec4(1, 1, 1, 1)});
        keys_.push_back({1.0f, Vec4(1, 1, 1, 1)});
    }
    
    void addKey(float time, const Vec4& color) {
        keys_.push_back({time, color});
        sortKeys();
    }
    
    void clearKeys() {
        keys_.clear();
    }
    
    Vec4 evaluate(float t) const {
        if (keys_.empty()) return Vec4(1, 1, 1, 1);
        if (keys_.size() == 1) return keys_[0].value;
        
        t = std::max(0.0f, std::min(1.0f, t));
        
        // Find surrounding keys
        size_t i = 0;
        for (; i < keys_.size() - 1; i++) {
            if (keys_[i + 1].time >= t) break;
        }
        
        if (i >= keys_.size() - 1) return keys_.back().value;
        
        float localT = (t - keys_[i].time) / (keys_[i + 1].time - keys_[i].time);
        const Vec4& a = keys_[i].value;
        const Vec4& b = keys_[i + 1].value;
        
        return Vec4(
            a.x + (b.x - a.x) * localT,
            a.y + (b.y - a.y) * localT,
            a.z + (b.z - a.z) * localT,
            a.w + (b.w - a.w) * localT
        );
    }
    
    const std::vector<GradientKey<Vec4>>& getKeys() const { return keys_; }
    
private:
    void sortKeys() {
        std::sort(keys_.begin(), keys_.end(), 
            [](const auto& a, const auto& b) { return a.time < b.time; });
    }
    
    std::vector<GradientKey<Vec4>> keys_;
};

// ===== Float Curve =====
class FloatCurve {
public:
    FloatCurve(float constantValue = 1.0f) {
        keys_.push_back({0.0f, constantValue});
        keys_.push_back({1.0f, constantValue});
    }
    
    void addKey(float time, float value) {
        keys_.push_back({time, value});
        sortKeys();
    }
    
    void clearKeys() { keys_.clear(); }
    
    float evaluate(float t) const {
        if (keys_.empty()) return 1.0f;
        if (keys_.size() == 1) return keys_[0].value;
        
        t = std::max(0.0f, std::min(1.0f, t));
        
        size_t i = 0;
        for (; i < keys_.size() - 1; i++) {
            if (keys_[i + 1].time >= t) break;
        }
        
        if (i >= keys_.size() - 1) return keys_.back().value;
        
        float localT = (t - keys_[i].time) / (keys_[i + 1].time - keys_[i].time);
        return keys_[i].value + (keys_[i + 1].value - keys_[i].value) * localT;
    }
    
    const std::vector<GradientKey<float>>& getKeys() const { return keys_; }
    
private:
    void sortKeys() {
        std::sort(keys_.begin(), keys_.end(),
            [](const auto& a, const auto& b) { return a.time < b.time; });
    }
    
    std::vector<GradientKey<float>> keys_;
};

// ===== Particle Module Base =====
class ParticleModule {
public:
    virtual ~ParticleModule() = default;
    
    virtual void onParticleSpawn(Particle& p) {}
    virtual void update(Particle& p, float dt) {}
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    virtual const char* getName() const = 0;
    
protected:
    bool enabled_ = true;
};

// ===== Color Over Lifetime =====
class ColorOverLifetimeModule : public ParticleModule {
public:
    ColorGradient gradient;
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        p.color = gradient.evaluate(p.age);
    }
    
    const char* getName() const override { return "Color Over Lifetime"; }
};

// ===== Size Over Lifetime =====
class SizeOverLifetimeModule : public ParticleModule {
public:
    FloatCurve curve = FloatCurve(1.0f);
    float multiplier = 1.0f;
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        float sizeMult = curve.evaluate(p.age) * multiplier;
        p.size = p.startSize * sizeMult;
    }
    
    const char* getName() const override { return "Size Over Lifetime"; }
};

// ===== Velocity Over Lifetime =====
class VelocityOverLifetimeModule : public ParticleModule {
public:
    Vec3 linear = {0, 0, 0};
    Vec3 orbital = {0, 0, 0};   // Orbital velocity around emitter center
    Vec3 radial = {0, 0, 0};   // Radial (outward from emitter)
    FloatCurve speedMultiplier = FloatCurve(1.0f);
    
    Vec3 emitterPosition = {0, 0, 0};  // Set by emitter
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        float speedMult = speedMultiplier.evaluate(p.age);
        
        // Linear velocity
        p.velocity = p.velocity + linear * dt;
        
        // Radial velocity
        if (radial.lengthSquared() > 0.0001f) {
            Vec3 toParticle = p.position - emitterPosition;
            float dist = toParticle.length();
            if (dist > 0.0001f) {
                Vec3 radialDir = toParticle * (1.0f / dist);
                p.velocity = p.velocity + Vec3(
                    radialDir.x * radial.x,
                    radialDir.y * radial.y,
                    radialDir.z * radial.z
                ) * dt;
            }
        }
        
        // Orbital velocity
        if (orbital.lengthSquared() > 0.0001f) {
            Vec3 toParticle = p.position - emitterPosition;
            
            // Rotate around Y axis
            if (std::abs(orbital.y) > 0.0001f) {
                float angle = orbital.y * dt;
                float cosA = cosf(angle);
                float sinA = sinf(angle);
                float newX = toParticle.x * cosA - toParticle.z * sinA;
                float newZ = toParticle.x * sinA + toParticle.z * cosA;
                p.position.x = emitterPosition.x + newX;
                p.position.z = emitterPosition.z + newZ;
            }
            
            // Similar for X and Z axes...
        }
        
        // Apply speed multiplier
        p.velocity = p.velocity * speedMult;
    }
    
    const char* getName() const override { return "Velocity Over Lifetime"; }
};

// ===== Force Field =====
enum class ForceFieldType {
    Directional,
    Point,
    Vortex,
    Turbulence
};

class ForceFieldModule : public ParticleModule {
public:
    ForceFieldType type = ForceFieldType::Directional;
    Vec3 position = {0, 0, 0};
    Vec3 direction = {0, 1, 0};  // For directional
    float strength = 1.0f;
    float radius = 10.0f;        // Influence radius for point/vortex
    float falloff = 1.0f;        // 0 = linear, 1 = quadratic
    
    // Turbulence specific
    float frequency = 1.0f;
    float amplitude = 1.0f;
    int octaves = 3;
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        Vec3 force = {0, 0, 0};
        
        switch (type) {
            case ForceFieldType::Directional:
                force = direction * strength;
                break;
                
            case ForceFieldType::Point: {
                Vec3 toCenter = position - p.position;
                float dist = toCenter.length();
                if (dist > 0.0001f && dist < radius) {
                    float atten = 1.0f - powf(dist / radius, falloff);
                    force = toCenter.normalized() * strength * atten;
                }
                break;
            }
            
            case ForceFieldType::Vortex: {
                Vec3 toCenter = position - p.position;
                toCenter.y = 0;  // XZ plane vortex
                float dist = toCenter.length();
                if (dist > 0.0001f && dist < radius) {
                    float atten = 1.0f - powf(dist / radius, falloff);
                    // Perpendicular to center direction
                    force.x = -toCenter.z;
                    force.z = toCenter.x;
                    force = force.normalized() * strength * atten;
                }
                break;
            }
            
            case ForceFieldType::Turbulence: {
                // Simple noise-based turbulence
                float t = p.age * frequency;
                force.x = sinf(t * 1.7f + p.position.x) * amplitude;
                force.y = sinf(t * 2.3f + p.position.y) * amplitude;
                force.z = sinf(t * 1.9f + p.position.z) * amplitude;
                force = force * strength;
                break;
            }
        }
        
        p.velocity = p.velocity + force * dt;
    }
    
    const char* getName() const override { return "Force Field"; }
};

// ===== Noise Module =====
class NoiseModule : public ParticleModule {
public:
    float strength = 1.0f;
    float frequency = 1.0f;
    float scrollSpeed = 0.5f;
    int octaves = 2;
    bool damping = false;
    float dampingStrength = 1.0f;
    
    // Position offset vs velocity
    bool positionMode = false;  // false = velocity, true = position
    
    // Internal time accumulator
    float time = 0.0f;
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        time += scrollSpeed * dt;
        
        // Generate noise based on position and time
        float noiseX = generateNoise(p.position.x * frequency + time, octaves);
        float noiseY = generateNoise(p.position.y * frequency + time * 1.3f, octaves);
        float noiseZ = generateNoise(p.position.z * frequency + time * 0.7f, octaves);
        
        Vec3 noise = {noiseX, noiseY, noiseZ};
        noise = noise * strength;
        
        // Apply damping based on particle age
        if (damping) {
            float dampFactor = 1.0f - p.age * dampingStrength;
            dampFactor = std::max(0.0f, dampFactor);
            noise = noise * dampFactor;
        }
        
        if (positionMode) {
            p.position = p.position + noise * dt;
        } else {
            p.velocity = p.velocity + noise * dt;
        }
    }
    
    const char* getName() const override { return "Noise"; }
    
private:
    // Simple noise function (could be replaced with proper Perlin/Simplex)
    float generateNoise(float x, int octaves) {
        float result = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float maxAmp = 0.0f;
        
        for (int i = 0; i < octaves; i++) {
            result += sinf(x * freq) * amp;
            maxAmp += amp;
            amp *= 0.5f;
            freq *= 2.0f;
        }
        
        return result / maxAmp;
    }
};

// ===== Rotation Over Lifetime =====
class RotationOverLifetimeModule : public ParticleModule {
public:
    FloatCurve angularVelocity = FloatCurve(0.0f);
    float multiplier = 1.0f;
    bool separateAxes = false;  // 3D rotation (future)
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        float angVel = angularVelocity.evaluate(p.age) * multiplier;
        p.rotation += angVel * dt;
    }
    
    const char* getName() const override { return "Rotation Over Lifetime"; }
};

// ===== Limit Velocity =====
class LimitVelocityModule : public ParticleModule {
public:
    float maxSpeed = 10.0f;
    float damping = 0.5f;  // How quickly it slows down when exceeding
    bool separateAxes = false;
    Vec3 maxVelocity = {10, 10, 10};
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        if (separateAxes) {
            if (std::abs(p.velocity.x) > maxVelocity.x) {
                p.velocity.x *= 1.0f - damping * dt;
            }
            if (std::abs(p.velocity.y) > maxVelocity.y) {
                p.velocity.y *= 1.0f - damping * dt;
            }
            if (std::abs(p.velocity.z) > maxVelocity.z) {
                p.velocity.z *= 1.0f - damping * dt;
            }
        } else {
            float speed = p.velocity.length();
            if (speed > maxSpeed) {
                float factor = 1.0f - damping * dt;
                p.velocity = p.velocity * factor;
            }
        }
    }
    
    const char* getName() const override { return "Limit Velocity"; }
};

// ===== Collision Module =====
class CollisionModule : public ParticleModule {
public:
    bool bounce = true;
    float bounciness = 0.5f;
    float lifetimeLoss = 0.1f;  // Fraction of life lost on collision
    
    // Simple ground plane collision
    float groundY = 0.0f;
    bool useGroundPlane = true;
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        if (useGroundPlane && p.position.y < groundY) {
            if (bounce) {
                p.position.y = groundY;
                p.velocity.y = -p.velocity.y * bounciness;
                
                // Apply friction to horizontal velocity
                p.velocity.x *= 0.9f;
                p.velocity.z *= 0.9f;
            } else {
                p.life = 0.0f;  // Kill particle
            }
            
            // Lose some life
            p.life -= p.maxLife * lifetimeLoss;
        }
    }
    
    const char* getName() const override { return "Collision"; }
};

// ===== Sub Emitter Module =====
class SubEmitterModule : public ParticleModule {
public:
    enum class TriggerType {
        Birth,
        Death,
        Collision,
        Manual
    };
    
    TriggerType trigger = TriggerType::Death;
    ParticleEmitterSettings subEmitterSettings;
    int inheritVelocity = 1;  // 0=none, 1=partial, 2=full
    float velocityScale = 0.5f;
    
    // Callback to create sub-particles (set by parent system)
    std::function<void(const Vec3&, const Vec3&)> onEmit;
    
    void onParticleSpawn(Particle& p) override {
        if (!enabled_ || trigger != TriggerType::Birth) return;
        if (onEmit) {
            onEmit(p.position, p.velocity * velocityScale);
        }
    }
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        // Check for death trigger
        if (trigger == TriggerType::Death && p.life <= 0.0f && p.life + dt > 0.0f) {
            if (onEmit) {
                onEmit(p.position, p.velocity * velocityScale);
            }
        }
    }
    
    const char* getName() const override { return "Sub Emitter"; }
};

// ===== Trail Module (data for trail rendering) =====
struct TrailPoint {
    Vec3 position;
    float width;
    Vec4 color;
    float age;
};

class TrailModule : public ParticleModule {
public:
    float lifetime = 1.0f;
    float minVertexDistance = 0.1f;
    float width = 0.1f;
    ColorGradient colorOverLifetime;
    FloatCurve widthOverLifetime = FloatCurve(1.0f);
    
    bool textureMode = false;
    int textureMode_value = 0;  // 0=stretch, 1=tile
    
    // Trail points stored per-particle (simplified - in reality would be per-particle storage)
    std::vector<TrailPoint> trailPoints;
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        // Add trail point if moved enough
        if (trailPoints.empty() || 
            (trailPoints.back().position - p.position).length() > minVertexDistance) {
            
            TrailPoint point;
            point.position = p.position;
            point.width = width * widthOverLifetime.evaluate(0.0f);
            point.color = colorOverLifetime.evaluate(0.0f);
            point.age = 0.0f;
            trailPoints.push_back(point);
        }
        
        // Update and remove old trail points
        for (auto& tp : trailPoints) {
            tp.age += dt;
            float normalizedAge = tp.age / lifetime;
            tp.width = width * widthOverLifetime.evaluate(normalizedAge);
            tp.color = colorOverLifetime.evaluate(normalizedAge);
        }
        
        // Remove expired points
        trailPoints.erase(
            std::remove_if(trailPoints.begin(), trailPoints.end(),
                [this](const TrailPoint& tp) { return tp.age >= lifetime; }),
            trailPoints.end()
        );
    }
    
    const char* getName() const override { return "Trail"; }
};

// ===== Texture Sheet Animation =====
class TextureSheetModule : public ParticleModule {
public:
    int tilesX = 4;
    int tilesY = 4;
    
    enum class AnimationMode {
        WholeSheet,     // Animate through all tiles
        SingleRow,      // Animate through one row
        Random          // Random tile per particle
    };
    AnimationMode mode = AnimationMode::WholeSheet;
    
    int rowIndex = 0;   // For SingleRow mode
    float frameRate = 10.0f;
    bool randomStartFrame = false;
    
    // Output: current frame index (stored in particle customFloat)
    void onParticleSpawn(Particle& p) override {
        if (!enabled_) return;
        
        if (randomStartFrame || mode == AnimationMode::Random) {
            p.customFloat = static_cast<float>(rand() % (tilesX * tilesY));
        } else {
            p.customFloat = 0.0f;
        }
    }
    
    void update(Particle& p, float dt) override {
        if (!enabled_) return;
        
        if (mode == AnimationMode::Random) return;  // Keep initial random frame
        
        int totalFrames = (mode == AnimationMode::SingleRow) ? tilesX : (tilesX * tilesY);
        
        // Update frame based on age
        float frame = p.age * frameRate;
        p.customFloat = fmodf(frame, static_cast<float>(totalFrames));
        
        if (mode == AnimationMode::SingleRow) {
            p.customFloat += rowIndex * tilesX;
        }
    }
    
    // Get UV offset for current frame
    void getUVOffset(float frame, float& uOffset, float& vOffset, float& uScale, float& vScale) const {
        int frameInt = static_cast<int>(frame) % (tilesX * tilesY);
        int col = frameInt % tilesX;
        int row = frameInt / tilesX;
        
        uScale = 1.0f / tilesX;
        vScale = 1.0f / tilesY;
        uOffset = col * uScale;
        vOffset = row * vScale;
    }
    
    const char* getName() const override { return "Texture Sheet Animation"; }
};

}  // namespace luma
