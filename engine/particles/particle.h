// Particle System - Core Data Structures
// High-performance GPU-accelerated particle system
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>
#include <cmath>
#include <random>

namespace luma {

// ===== Single Particle =====
struct Particle {
    Vec3 position;
    Vec3 velocity;
    Vec4 color;         // RGBA with alpha
    Vec4 startColor;    // Initial color for interpolation
    Vec4 endColor;      // Final color for interpolation
    float size;
    float startSize;
    float endSize;
    float rotation;     // Radians
    float angularVelocity;
    float life;         // Remaining life (seconds)
    float maxLife;      // Initial life
    float age;          // 0 to 1 normalized age
    
    // Custom data for modules
    Vec3 customVec;
    float customFloat;
    
    bool isAlive() const { return life > 0.0f; }
    
    void update(float dt) {
        if (!isAlive()) return;
        
        life -= dt;
        age = 1.0f - (life / maxLife);
        
        // Basic update
        position = position + velocity * dt;
        rotation += angularVelocity * dt;
        
        // Interpolate size and color
        size = startSize + (endSize - startSize) * age;
        color.x = startColor.x + (endColor.x - startColor.x) * age;
        color.y = startColor.y + (endColor.y - startColor.y) * age;
        color.z = startColor.z + (endColor.z - startColor.z) * age;
        color.w = startColor.w + (endColor.w - startColor.w) * age;
    }
};

// ===== Emission Shape =====
enum class EmissionShape {
    Point,
    Sphere,
    Hemisphere,
    Cone,
    Box,
    Circle,
    Edge,
    Mesh  // Emit from mesh surface (future)
};

struct EmissionShapeParams {
    EmissionShape shape = EmissionShape::Point;
    
    // Sphere/Hemisphere
    float radius = 1.0f;
    float radiusThickness = 0.0f;  // 0 = surface, 1 = full volume
    
    // Cone
    float coneAngle = 45.0f;       // Degrees
    float coneRadius = 1.0f;
    float coneLength = 5.0f;
    
    // Box
    Vec3 boxSize = {1.0f, 1.0f, 1.0f};
    
    // Circle/Edge
    float arcAngle = 360.0f;       // Degrees
    
    // Emission direction
    bool randomizeDirection = false;
    float directionalSpread = 0.0f;  // 0 = straight, 1 = hemisphere
};

// ===== Burst =====
struct ParticleBurst {
    float time = 0.0f;      // Time after emitter start
    int minCount = 10;
    int maxCount = 10;
    int cycles = 1;         // -1 = infinite
    float interval = 1.0f;  // Interval between cycles
    float probability = 1.0f;
    
    // Internal state
    int cyclesDone = 0;
    float lastBurstTime = -1000.0f;
};

// ===== Value Range (for random ranges) =====
template<typename T>
struct ValueRange {
    T min;
    T max;
    
    ValueRange() : min(T{}), max(T{}) {}
    ValueRange(T value) : min(value), max(value) {}
    ValueRange(T minVal, T maxVal) : min(minVal), max(maxVal) {}
    
    T evaluate(float t) const {
        return min + (max - min) * t;
    }
};

using FloatRange = ValueRange<float>;
using Vec3Range = ValueRange<Vec3>;
using Vec4Range = ValueRange<Vec4>;

// ===== Emitter Settings =====
struct ParticleEmitterSettings {
    // Emission
    float emissionRate = 10.0f;     // Particles per second
    int maxParticles = 1000;
    bool looping = true;
    float duration = 5.0f;          // Duration if not looping
    float startDelay = 0.0f;
    
    // Shape
    EmissionShapeParams shape;
    
    // Initial values
    FloatRange startLife = {1.0f, 2.0f};
    FloatRange startSpeed = {1.0f, 3.0f};
    FloatRange startSize = {0.1f, 0.3f};
    FloatRange endSize = {0.0f, 0.1f};
    FloatRange startRotation = {0.0f, 360.0f};
    FloatRange angularVelocity = {0.0f, 0.0f};
    
    // Colors
    Vec4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vec4 endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    bool useColorGradient = false;
    
    // Physics
    Vec3 gravity = {0.0f, -9.81f, 0.0f};
    float gravityMultiplier = 0.0f;  // 0 = no gravity
    float drag = 0.0f;               // Air resistance
    
    // Rendering
    bool billboard = true;           // Face camera
    bool stretchWithVelocity = false;
    float velocityStretch = 0.0f;
    int sortMode = 0;                // 0=none, 1=by distance, 2=by age
    
    // Texture
    int textureRows = 1;
    int textureCols = 1;
    bool animateTexture = false;
    float textureAnimSpeed = 10.0f;
    
    // Bursts
    std::vector<ParticleBurst> bursts;
    
    // World/Local space
    bool worldSpace = true;
};

// ===== Particle Pool (for efficient memory) =====
class ParticlePool {
public:
    ParticlePool(size_t maxParticles = 10000)
        : maxSize_(maxParticles), particles_(maxParticles), aliveCount_(0) {}
    
    Particle* spawn() {
        if (aliveCount_ >= maxSize_) return nullptr;
        
        // Find first dead particle
        for (size_t i = 0; i < maxSize_; i++) {
            if (!particles_[i].isAlive()) {
                aliveCount_++;
                return &particles_[i];
            }
        }
        return nullptr;
    }
    
    void update(float dt) {
        aliveCount_ = 0;
        for (auto& p : particles_) {
            if (p.isAlive()) {
                p.update(dt);
                if (p.isAlive()) aliveCount_++;
            }
        }
    }
    
    void clear() {
        for (auto& p : particles_) {
            p.life = 0.0f;
        }
        aliveCount_ = 0;
    }
    
    const std::vector<Particle>& getParticles() const { return particles_; }
    std::vector<Particle>& getParticles() { return particles_; }
    size_t getAliveCount() const { return aliveCount_; }
    size_t getMaxSize() const { return maxSize_; }
    
private:
    size_t maxSize_;
    std::vector<Particle> particles_;
    size_t aliveCount_;
};

// ===== Particle Emitter =====
class ParticleEmitter {
public:
    ParticleEmitter() : pool_(10000), rng_(std::random_device{}()) {}
    
    void setSettings(const ParticleEmitterSettings& settings) {
        settings_ = settings;
        pool_ = ParticlePool(settings.maxParticles);
    }
    
    const ParticleEmitterSettings& getSettings() const { return settings_; }
    ParticleEmitterSettings& getSettings() { return settings_; }
    
    void setPosition(const Vec3& pos) { position_ = pos; }
    Vec3 getPosition() const { return position_; }
    
    void setRotation(const Quat& rot) { rotation_ = rot; }
    Quat getRotation() const { return rotation_; }
    
    void play() {
        playing_ = true;
        time_ = 0.0f;
        emissionAccumulator_ = 0.0f;
        for (auto& burst : settings_.bursts) {
            burst.cyclesDone = 0;
            burst.lastBurstTime = -1000.0f;
        }
    }
    
    void stop(bool clearParticles = false) {
        playing_ = false;
        if (clearParticles) {
            pool_.clear();
        }
    }
    
    void pause() { playing_ = false; }
    void resume() { playing_ = true; }
    
    bool isPlaying() const { return playing_; }
    bool isAlive() const { return playing_ || pool_.getAliveCount() > 0; }
    
    void update(float dt) {
        if (!playing_ && pool_.getAliveCount() == 0) return;
        
        // Update existing particles
        pool_.update(dt);
        
        // Apply gravity and drag
        for (auto& p : pool_.getParticles()) {
            if (!p.isAlive()) continue;
            
            // Gravity
            if (settings_.gravityMultiplier > 0.0f) {
                p.velocity = p.velocity + settings_.gravity * settings_.gravityMultiplier * dt;
            }
            
            // Drag
            if (settings_.drag > 0.0f) {
                p.velocity = p.velocity * (1.0f - settings_.drag * dt);
            }
        }
        
        if (!playing_) return;
        
        time_ += dt;
        
        // Check duration
        if (!settings_.looping && time_ > settings_.duration) {
            playing_ = false;
            return;
        }
        
        // Check start delay
        if (time_ < settings_.startDelay) return;
        
        // Emit particles based on rate
        emissionAccumulator_ += settings_.emissionRate * dt;
        while (emissionAccumulator_ >= 1.0f) {
            emitParticle();
            emissionAccumulator_ -= 1.0f;
        }
        
        // Process bursts
        float effectiveTime = time_ - settings_.startDelay;
        for (auto& burst : settings_.bursts) {
            if (burst.cycles >= 0 && burst.cyclesDone >= burst.cycles) continue;
            
            float burstTime = burst.time + burst.cyclesDone * burst.interval;
            if (effectiveTime >= burstTime && burst.lastBurstTime < burstTime) {
                // Check probability
                std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
                if (probDist(rng_) <= burst.probability) {
                    // Emit burst
                    std::uniform_int_distribution<int> countDist(burst.minCount, burst.maxCount);
                    int count = countDist(rng_);
                    for (int i = 0; i < count; i++) {
                        emitParticle();
                    }
                }
                burst.lastBurstTime = burstTime;
                burst.cyclesDone++;
            }
        }
    }
    
    const ParticlePool& getPool() const { return pool_; }
    size_t getParticleCount() const { return pool_.getAliveCount(); }
    
private:
    void emitParticle() {
        Particle* p = pool_.spawn();
        if (!p) return;
        
        // Position based on shape
        Vec3 localPos, direction;
        getEmissionPoint(localPos, direction);
        
        if (settings_.worldSpace) {
            p->position = position_ + localPos;
        } else {
            p->position = localPos;
        }
        
        // Velocity
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float speed = settings_.startSpeed.evaluate(dist(rng_));
        p->velocity = direction * speed;
        
        // Life
        p->life = settings_.startLife.evaluate(dist(rng_));
        p->maxLife = p->life;
        p->age = 0.0f;
        
        // Size
        p->startSize = settings_.startSize.evaluate(dist(rng_));
        p->endSize = settings_.endSize.evaluate(dist(rng_));
        p->size = p->startSize;
        
        // Color
        p->startColor = settings_.startColor;
        p->endColor = settings_.endColor;
        p->color = p->startColor;
        
        // Rotation
        p->rotation = settings_.startRotation.evaluate(dist(rng_)) * 3.14159f / 180.0f;
        p->angularVelocity = settings_.angularVelocity.evaluate(dist(rng_)) * 3.14159f / 180.0f;
    }
    
    void getEmissionPoint(Vec3& position, Vec3& direction) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
        
        const auto& shape = settings_.shape;
        
        switch (shape.shape) {
            case EmissionShape::Point:
                position = Vec3(0, 0, 0);
                direction = getRandomDirection();
                break;
                
            case EmissionShape::Sphere: {
                float theta = angleDist(rng_);
                float phi = acosf(2.0f * dist(rng_) - 1.0f);
                float r = shape.radius;
                if (shape.radiusThickness > 0.0f) {
                    r *= 1.0f - shape.radiusThickness * dist(rng_);
                }
                
                position.x = r * sinf(phi) * cosf(theta);
                position.y = r * cosf(phi);
                position.z = r * sinf(phi) * sinf(theta);
                
                if (shape.randomizeDirection) {
                    direction = getRandomDirection();
                } else {
                    direction = position.normalized();
                }
                break;
            }
            
            case EmissionShape::Hemisphere: {
                float theta = angleDist(rng_);
                float phi = acosf(dist(rng_));  // Only upper hemisphere
                float r = shape.radius;
                if (shape.radiusThickness > 0.0f) {
                    r *= 1.0f - shape.radiusThickness * dist(rng_);
                }
                
                position.x = r * sinf(phi) * cosf(theta);
                position.y = r * cosf(phi);
                position.z = r * sinf(phi) * sinf(theta);
                
                if (shape.randomizeDirection) {
                    direction = getRandomDirection();
                } else {
                    direction = position.normalized();
                }
                break;
            }
            
            case EmissionShape::Cone: {
                float theta = angleDist(rng_);
                float maxAngle = shape.coneAngle * 3.14159f / 180.0f;
                float phi = maxAngle * sqrtf(dist(rng_));
                
                float height = shape.coneLength * dist(rng_);
                float radius = height * tanf(phi);
                
                position.x = radius * cosf(theta);
                position.y = height;
                position.z = radius * sinf(theta);
                
                direction.x = sinf(phi) * cosf(theta);
                direction.y = cosf(phi);
                direction.z = sinf(phi) * sinf(theta);
                break;
            }
            
            case EmissionShape::Box: {
                position.x = (dist(rng_) - 0.5f) * shape.boxSize.x;
                position.y = (dist(rng_) - 0.5f) * shape.boxSize.y;
                position.z = (dist(rng_) - 0.5f) * shape.boxSize.z;
                direction = Vec3(0, 1, 0);  // Default up
                if (shape.randomizeDirection) {
                    direction = getRandomDirection();
                }
                break;
            }
            
            case EmissionShape::Circle: {
                float theta = shape.arcAngle * 3.14159f / 180.0f * dist(rng_);
                float r = shape.radius * sqrtf(dist(rng_));
                
                position.x = r * cosf(theta);
                position.y = 0;
                position.z = r * sinf(theta);
                
                direction = Vec3(0, 1, 0);
                if (shape.randomizeDirection) {
                    direction = getRandomDirection();
                }
                break;
            }
            
            case EmissionShape::Edge: {
                float t = dist(rng_);
                position.x = (t - 0.5f) * shape.radius * 2.0f;
                position.y = 0;
                position.z = 0;
                direction = Vec3(0, 1, 0);
                break;
            }
            
            default:
                position = Vec3(0, 0, 0);
                direction = Vec3(0, 1, 0);
                break;
        }
        
        // Apply directional spread
        if (shape.directionalSpread > 0.0f) {
            Vec3 randomDir = getRandomDirection();
            direction.x = direction.x + randomDir.x * shape.directionalSpread;
            direction.y = direction.y + randomDir.y * shape.directionalSpread;
            direction.z = direction.z + randomDir.z * shape.directionalSpread;
            direction = direction.normalized();
        }
    }
    
    Vec3 getRandomDirection() {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        Vec3 dir;
        do {
            dir.x = dist(rng_);
            dir.y = dist(rng_);
            dir.z = dist(rng_);
        } while (dir.lengthSquared() > 1.0f || dir.lengthSquared() < 0.0001f);
        return dir.normalized();
    }
    
    ParticleEmitterSettings settings_;
    ParticlePool pool_;
    Vec3 position_ = {0, 0, 0};
    Quat rotation_ = Quat::identity();
    
    bool playing_ = false;
    float time_ = 0.0f;
    float emissionAccumulator_ = 0.0f;
    
    std::mt19937 rng_;
};

// ===== Particle System (manages multiple emitters) =====
class ParticleSystem {
public:
    ParticleSystem() = default;
    
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }
    
    ParticleEmitter& addEmitter() {
        emitters_.push_back(std::make_unique<ParticleEmitter>());
        return *emitters_.back();
    }
    
    void removeEmitter(size_t index) {
        if (index < emitters_.size()) {
            emitters_.erase(emitters_.begin() + index);
        }
    }
    
    size_t getEmitterCount() const { return emitters_.size(); }
    
    ParticleEmitter* getEmitter(size_t index) {
        return index < emitters_.size() ? emitters_[index].get() : nullptr;
    }
    
    const ParticleEmitter* getEmitter(size_t index) const {
        return index < emitters_.size() ? emitters_[index].get() : nullptr;
    }
    
    void setPosition(const Vec3& pos) {
        position_ = pos;
        for (auto& e : emitters_) {
            e->setPosition(pos);
        }
    }
    Vec3 getPosition() const { return position_; }
    
    void play() {
        for (auto& e : emitters_) {
            e->play();
        }
    }
    
    void stop(bool clear = false) {
        for (auto& e : emitters_) {
            e->stop(clear);
        }
    }
    
    void pause() {
        for (auto& e : emitters_) {
            e->pause();
        }
    }
    
    void update(float dt) {
        for (auto& e : emitters_) {
            e->update(dt);
        }
    }
    
    bool isAlive() const {
        for (const auto& e : emitters_) {
            if (e->isAlive()) return true;
        }
        return false;
    }
    
    size_t getTotalParticleCount() const {
        size_t count = 0;
        for (const auto& e : emitters_) {
            count += e->getParticleCount();
        }
        return count;
    }
    
private:
    std::string name_ = "Particle System";
    Vec3 position_ = {0, 0, 0};
    std::vector<std::unique_ptr<ParticleEmitter>> emitters_;
};

// ===== Global Particle Manager =====
class ParticleManager {
public:
    static ParticleManager& get() {
        static ParticleManager instance;
        return instance;
    }
    
    ParticleSystem* createSystem(const std::string& name = "Particle System") {
        systems_.push_back(std::make_unique<ParticleSystem>());
        systems_.back()->setName(name);
        return systems_.back().get();
    }
    
    void destroySystem(ParticleSystem* system) {
        for (auto it = systems_.begin(); it != systems_.end(); ++it) {
            if (it->get() == system) {
                systems_.erase(it);
                return;
            }
        }
    }
    
    void update(float dt) {
        for (auto& sys : systems_) {
            sys->update(dt);
        }
        
        // Remove dead systems (optional, could keep them)
    }
    
    void clear() {
        systems_.clear();
    }
    
    const std::vector<std::unique_ptr<ParticleSystem>>& getSystems() const {
        return systems_;
    }
    
    size_t getTotalParticleCount() const {
        size_t count = 0;
        for (const auto& sys : systems_) {
            count += sys->getTotalParticleCount();
        }
        return count;
    }
    
private:
    ParticleManager() = default;
    std::vector<std::unique_ptr<ParticleSystem>> systems_;
};

// Helper function
inline ParticleManager& getParticleManager() {
    return ParticleManager::get();
}

}  // namespace luma
