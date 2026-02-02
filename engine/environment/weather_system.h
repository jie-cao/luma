// Weather System - Rain, snow, fog, and atmospheric effects
// Dynamic weather with transitions and lighting effects
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <functional>

namespace luma {

// ============================================================================
// Weather Types
// ============================================================================

enum class WeatherType {
    Clear,          // 晴天
    Cloudy,         // 多云
    Overcast,       // 阴天
    LightRain,      // 小雨
    HeavyRain,      // 大雨
    Thunderstorm,   // 雷暴
    LightSnow,      // 小雪
    HeavySnow,      // 大雪
    Blizzard,       // 暴风雪
    Fog,            // 雾
    DenseFog,       // 浓雾
    Hail,           // 冰雹
    Sandstorm,      // 沙尘暴
    Custom          // 自定义
};

inline std::string weatherTypeToString(WeatherType type) {
    switch (type) {
        case WeatherType::Clear: return "Clear";
        case WeatherType::Cloudy: return "Cloudy";
        case WeatherType::Overcast: return "Overcast";
        case WeatherType::LightRain: return "LightRain";
        case WeatherType::HeavyRain: return "HeavyRain";
        case WeatherType::Thunderstorm: return "Thunderstorm";
        case WeatherType::LightSnow: return "LightSnow";
        case WeatherType::HeavySnow: return "HeavySnow";
        case WeatherType::Blizzard: return "Blizzard";
        case WeatherType::Fog: return "Fog";
        case WeatherType::DenseFog: return "DenseFog";
        case WeatherType::Hail: return "Hail";
        case WeatherType::Sandstorm: return "Sandstorm";
        case WeatherType::Custom: return "Custom";
        default: return "Unknown";
    }
}

inline std::string weatherTypeToDisplayName(WeatherType type) {
    switch (type) {
        case WeatherType::Clear: return "晴天 Clear";
        case WeatherType::Cloudy: return "多云 Cloudy";
        case WeatherType::Overcast: return "阴天 Overcast";
        case WeatherType::LightRain: return "小雨 Light Rain";
        case WeatherType::HeavyRain: return "大雨 Heavy Rain";
        case WeatherType::Thunderstorm: return "雷暴 Thunderstorm";
        case WeatherType::LightSnow: return "小雪 Light Snow";
        case WeatherType::HeavySnow: return "大雪 Heavy Snow";
        case WeatherType::Blizzard: return "暴风雪 Blizzard";
        case WeatherType::Fog: return "雾 Fog";
        case WeatherType::DenseFog: return "浓雾 Dense Fog";
        case WeatherType::Hail: return "冰雹 Hail";
        case WeatherType::Sandstorm: return "沙尘暴 Sandstorm";
        case WeatherType::Custom: return "自定义 Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Weather Parameters
// ============================================================================

struct WeatherParams {
    WeatherType type = WeatherType::Clear;
    
    // Precipitation
    float precipitationIntensity = 0.0f;  // 0-1, controls particle density
    float precipitationSize = 1.0f;        // Particle size multiplier
    Vec3 precipitationColor{1, 1, 1};
    float windStrength = 0.0f;             // 0-1
    float windDirection = 0.0f;            // Radians
    
    // Fog
    float fogDensity = 0.0f;               // 0-1
    float fogStart = 10.0f;                // Distance where fog starts
    float fogEnd = 100.0f;                 // Distance where fog is full
    Vec3 fogColor{0.7f, 0.7f, 0.8f};
    bool useHeightFog = false;
    float fogHeightFalloff = 0.1f;
    
    // Clouds
    float cloudCoverage = 0.0f;            // 0-1
    float cloudDensity = 0.5f;
    Vec3 cloudColor{1, 1, 1};
    float cloudSpeed = 0.01f;
    
    // Lighting adjustments
    float sunIntensityMultiplier = 1.0f;
    Vec3 ambientColorMultiplier{1, 1, 1};
    float shadowIntensity = 1.0f;
    
    // Atmospheric
    float visibility = 1000.0f;            // Meters
    float humidity = 0.5f;                 // 0-1
    float temperature = 20.0f;             // Celsius (for snow/rain threshold)
    
    // Thunder/Lightning
    float lightningFrequency = 0.0f;       // Flashes per minute
    float thunderDelay = 3.0f;             // Seconds after lightning
};

// ============================================================================
// Weather Presets
// ============================================================================

class WeatherPresets {
public:
    static WeatherParams getClear() {
        WeatherParams p;
        p.type = WeatherType::Clear;
        p.cloudCoverage = 0.1f;
        p.sunIntensityMultiplier = 1.0f;
        p.visibility = 10000.0f;
        return p;
    }
    
    static WeatherParams getCloudy() {
        WeatherParams p;
        p.type = WeatherType::Cloudy;
        p.cloudCoverage = 0.6f;
        p.sunIntensityMultiplier = 0.8f;
        p.ambientColorMultiplier = Vec3(0.9f, 0.9f, 0.95f);
        p.visibility = 5000.0f;
        return p;
    }
    
    static WeatherParams getOvercast() {
        WeatherParams p;
        p.type = WeatherType::Overcast;
        p.cloudCoverage = 1.0f;
        p.cloudDensity = 0.8f;
        p.sunIntensityMultiplier = 0.4f;
        p.ambientColorMultiplier = Vec3(0.7f, 0.7f, 0.75f);
        p.shadowIntensity = 0.3f;
        p.visibility = 3000.0f;
        return p;
    }
    
    static WeatherParams getLightRain() {
        WeatherParams p;
        p.type = WeatherType::LightRain;
        p.precipitationIntensity = 0.3f;
        p.precipitationSize = 0.8f;
        p.cloudCoverage = 0.9f;
        p.sunIntensityMultiplier = 0.5f;
        p.ambientColorMultiplier = Vec3(0.7f, 0.7f, 0.75f);
        p.fogDensity = 0.1f;
        p.fogColor = Vec3(0.6f, 0.6f, 0.65f);
        p.humidity = 0.8f;
        p.visibility = 2000.0f;
        return p;
    }
    
    static WeatherParams getHeavyRain() {
        WeatherParams p;
        p.type = WeatherType::HeavyRain;
        p.precipitationIntensity = 0.8f;
        p.precipitationSize = 1.2f;
        p.windStrength = 0.4f;
        p.cloudCoverage = 1.0f;
        p.cloudDensity = 1.0f;
        p.sunIntensityMultiplier = 0.3f;
        p.ambientColorMultiplier = Vec3(0.5f, 0.5f, 0.55f);
        p.fogDensity = 0.3f;
        p.fogColor = Vec3(0.5f, 0.5f, 0.55f);
        p.shadowIntensity = 0.2f;
        p.humidity = 0.95f;
        p.visibility = 500.0f;
        return p;
    }
    
    static WeatherParams getThunderstorm() {
        WeatherParams p = getHeavyRain();
        p.type = WeatherType::Thunderstorm;
        p.precipitationIntensity = 1.0f;
        p.windStrength = 0.7f;
        p.lightningFrequency = 10.0f;
        p.sunIntensityMultiplier = 0.2f;
        p.ambientColorMultiplier = Vec3(0.3f, 0.3f, 0.4f);
        p.visibility = 300.0f;
        return p;
    }
    
    static WeatherParams getLightSnow() {
        WeatherParams p;
        p.type = WeatherType::LightSnow;
        p.precipitationIntensity = 0.3f;
        p.precipitationSize = 1.5f;
        p.precipitationColor = Vec3(1, 1, 1);
        p.cloudCoverage = 0.8f;
        p.sunIntensityMultiplier = 0.6f;
        p.ambientColorMultiplier = Vec3(0.9f, 0.9f, 1.0f);
        p.fogDensity = 0.15f;
        p.fogColor = Vec3(0.9f, 0.9f, 0.95f);
        p.temperature = -5.0f;
        p.visibility = 1500.0f;
        return p;
    }
    
    static WeatherParams getHeavySnow() {
        WeatherParams p;
        p.type = WeatherType::HeavySnow;
        p.precipitationIntensity = 0.7f;
        p.precipitationSize = 2.0f;
        p.precipitationColor = Vec3(1, 1, 1);
        p.windStrength = 0.3f;
        p.cloudCoverage = 1.0f;
        p.sunIntensityMultiplier = 0.4f;
        p.ambientColorMultiplier = Vec3(0.8f, 0.8f, 0.9f);
        p.fogDensity = 0.4f;
        p.fogColor = Vec3(0.85f, 0.85f, 0.9f);
        p.shadowIntensity = 0.3f;
        p.temperature = -10.0f;
        p.visibility = 500.0f;
        return p;
    }
    
    static WeatherParams getBlizzard() {
        WeatherParams p;
        p.type = WeatherType::Blizzard;
        p.precipitationIntensity = 1.0f;
        p.precipitationSize = 1.5f;
        p.precipitationColor = Vec3(1, 1, 1);
        p.windStrength = 0.9f;
        p.cloudCoverage = 1.0f;
        p.sunIntensityMultiplier = 0.2f;
        p.ambientColorMultiplier = Vec3(0.6f, 0.6f, 0.7f);
        p.fogDensity = 0.7f;
        p.fogColor = Vec3(0.8f, 0.8f, 0.85f);
        p.shadowIntensity = 0.1f;
        p.temperature = -20.0f;
        p.visibility = 100.0f;
        return p;
    }
    
    static WeatherParams getFog() {
        WeatherParams p;
        p.type = WeatherType::Fog;
        p.fogDensity = 0.5f;
        p.fogStart = 5.0f;
        p.fogEnd = 50.0f;
        p.fogColor = Vec3(0.8f, 0.8f, 0.82f);
        p.cloudCoverage = 0.7f;
        p.sunIntensityMultiplier = 0.5f;
        p.ambientColorMultiplier = Vec3(0.8f, 0.8f, 0.82f);
        p.shadowIntensity = 0.4f;
        p.humidity = 0.9f;
        p.visibility = 200.0f;
        return p;
    }
    
    static WeatherParams getDenseFog() {
        WeatherParams p;
        p.type = WeatherType::DenseFog;
        p.fogDensity = 0.9f;
        p.fogStart = 2.0f;
        p.fogEnd = 20.0f;
        p.fogColor = Vec3(0.75f, 0.75f, 0.78f);
        p.cloudCoverage = 1.0f;
        p.sunIntensityMultiplier = 0.3f;
        p.ambientColorMultiplier = Vec3(0.7f, 0.7f, 0.75f);
        p.shadowIntensity = 0.2f;
        p.humidity = 0.98f;
        p.visibility = 50.0f;
        return p;
    }
    
    static WeatherParams getSandstorm() {
        WeatherParams p;
        p.type = WeatherType::Sandstorm;
        p.precipitationIntensity = 0.8f;
        p.precipitationSize = 0.5f;
        p.precipitationColor = Vec3(0.8f, 0.7f, 0.5f);
        p.windStrength = 0.8f;
        p.fogDensity = 0.6f;
        p.fogColor = Vec3(0.8f, 0.7f, 0.5f);
        p.sunIntensityMultiplier = 0.4f;
        p.ambientColorMultiplier = Vec3(0.9f, 0.8f, 0.6f);
        p.visibility = 100.0f;
        return p;
    }
    
    static WeatherParams getPreset(WeatherType type) {
        switch (type) {
            case WeatherType::Clear: return getClear();
            case WeatherType::Cloudy: return getCloudy();
            case WeatherType::Overcast: return getOvercast();
            case WeatherType::LightRain: return getLightRain();
            case WeatherType::HeavyRain: return getHeavyRain();
            case WeatherType::Thunderstorm: return getThunderstorm();
            case WeatherType::LightSnow: return getLightSnow();
            case WeatherType::HeavySnow: return getHeavySnow();
            case WeatherType::Blizzard: return getBlizzard();
            case WeatherType::Fog: return getFog();
            case WeatherType::DenseFog: return getDenseFog();
            case WeatherType::Sandstorm: return getSandstorm();
            default: return getClear();
        }
    }
};

// ============================================================================
// Precipitation Particle
// ============================================================================

struct PrecipitationParticle {
    Vec3 position;
    Vec3 velocity;
    float size;
    float alpha;
    float life;
    float maxLife;
};

// ============================================================================
// Weather System
// ============================================================================

class WeatherSystem {
public:
    WeatherSystem() : rng_(std::random_device{}()) {}
    
    // === Weather Control ===
    
    void setWeather(WeatherType type, float transitionTime = 2.0f) {
        targetParams_ = WeatherPresets::getPreset(type);
        transitionDuration_ = transitionTime;
        transitionProgress_ = 0.0f;
        isTransitioning_ = true;
    }
    
    void setWeatherParams(const WeatherParams& params, float transitionTime = 2.0f) {
        targetParams_ = params;
        transitionDuration_ = transitionTime;
        transitionProgress_ = 0.0f;
        isTransitioning_ = true;
    }
    
    void setWeatherImmediate(WeatherType type) {
        currentParams_ = WeatherPresets::getPreset(type);
        targetParams_ = currentParams_;
        isTransitioning_ = false;
    }
    
    // === Update ===
    
    void update(float deltaTime, const Vec3& cameraPosition) {
        // Update transition
        if (isTransitioning_) {
            transitionProgress_ += deltaTime / transitionDuration_;
            if (transitionProgress_ >= 1.0f) {
                transitionProgress_ = 1.0f;
                currentParams_ = targetParams_;
                isTransitioning_ = false;
            } else {
                interpolateParams(transitionProgress_);
            }
        }
        
        // Update precipitation particles
        updatePrecipitation(deltaTime, cameraPosition);
        
        // Update lightning
        updateLightning(deltaTime);
        
        // Update wind
        updateWind(deltaTime);
        
        time_ += deltaTime;
    }
    
    // === Getters ===
    
    const WeatherParams& getCurrentParams() const { return currentParams_; }
    WeatherType getCurrentWeatherType() const { return currentParams_.type; }
    bool isTransitioning() const { return isTransitioning_; }
    float getTransitionProgress() const { return transitionProgress_; }
    
    // Fog parameters for rendering
    float getFogDensity() const { return currentParams_.fogDensity; }
    float getFogStart() const { return currentParams_.fogStart; }
    float getFogEnd() const { return currentParams_.fogEnd; }
    Vec3 getFogColor() const { return currentParams_.fogColor; }
    
    // Lighting multipliers
    float getSunIntensityMultiplier() const { return currentParams_.sunIntensityMultiplier; }
    Vec3 getAmbientColorMultiplier() const { return currentParams_.ambientColorMultiplier; }
    float getShadowIntensity() const { return currentParams_.shadowIntensity; }
    
    // Wind
    float getWindStrength() const { return currentParams_.windStrength; }
    float getWindDirection() const { return currentParams_.windDirection; }
    Vec3 getWindVector() const {
        float s = currentParams_.windStrength * 10.0f;  // Scale
        return Vec3(
            std::cos(currentParams_.windDirection) * s,
            0,
            std::sin(currentParams_.windDirection) * s
        );
    }
    
    // Precipitation particles
    const std::vector<PrecipitationParticle>& getParticles() const { return particles_; }
    
    // Lightning
    bool isLightningActive() const { return lightningActive_; }
    float getLightningIntensity() const { return lightningIntensity_; }
    
    // === Configuration ===
    
    void setParticleCount(int count) { maxParticles_ = count; }
    void setParticleArea(float size) { spawnAreaSize_ = size; }
    
    // === Callbacks ===
    
    std::function<void()> onLightningStrike;
    std::function<void()> onThunder;
    
private:
    void interpolateParams(float t) {
        // Smooth interpolation
        float smoothT = t * t * (3.0f - 2.0f * t);  // Smoothstep
        
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        auto lerpVec3 = [&](const Vec3& a, const Vec3& b, float t) {
            return Vec3(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t));
        };
        
        currentParams_.precipitationIntensity = lerp(
            currentParams_.precipitationIntensity, targetParams_.precipitationIntensity, smoothT);
        currentParams_.precipitationSize = lerp(
            currentParams_.precipitationSize, targetParams_.precipitationSize, smoothT);
        currentParams_.precipitationColor = lerpVec3(
            currentParams_.precipitationColor, targetParams_.precipitationColor, smoothT);
        
        currentParams_.windStrength = lerp(
            currentParams_.windStrength, targetParams_.windStrength, smoothT);
        currentParams_.windDirection = lerp(
            currentParams_.windDirection, targetParams_.windDirection, smoothT);
        
        currentParams_.fogDensity = lerp(
            currentParams_.fogDensity, targetParams_.fogDensity, smoothT);
        currentParams_.fogStart = lerp(
            currentParams_.fogStart, targetParams_.fogStart, smoothT);
        currentParams_.fogEnd = lerp(
            currentParams_.fogEnd, targetParams_.fogEnd, smoothT);
        currentParams_.fogColor = lerpVec3(
            currentParams_.fogColor, targetParams_.fogColor, smoothT);
        
        currentParams_.cloudCoverage = lerp(
            currentParams_.cloudCoverage, targetParams_.cloudCoverage, smoothT);
        currentParams_.cloudDensity = lerp(
            currentParams_.cloudDensity, targetParams_.cloudDensity, smoothT);
        
        currentParams_.sunIntensityMultiplier = lerp(
            currentParams_.sunIntensityMultiplier, targetParams_.sunIntensityMultiplier, smoothT);
        currentParams_.ambientColorMultiplier = lerpVec3(
            currentParams_.ambientColorMultiplier, targetParams_.ambientColorMultiplier, smoothT);
        currentParams_.shadowIntensity = lerp(
            currentParams_.shadowIntensity, targetParams_.shadowIntensity, smoothT);
        
        currentParams_.visibility = lerp(
            currentParams_.visibility, targetParams_.visibility, smoothT);
        currentParams_.lightningFrequency = lerp(
            currentParams_.lightningFrequency, targetParams_.lightningFrequency, smoothT);
    }
    
    void updatePrecipitation(float deltaTime, const Vec3& cameraPos) {
        if (currentParams_.precipitationIntensity < 0.01f) {
            particles_.clear();
            return;
        }
        
        // Calculate spawn rate
        int targetCount = static_cast<int>(maxParticles_ * currentParams_.precipitationIntensity);
        
        // Update existing particles
        Vec3 wind = getWindVector();
        bool isSnow = currentParams_.type == WeatherType::LightSnow ||
                      currentParams_.type == WeatherType::HeavySnow ||
                      currentParams_.type == WeatherType::Blizzard;
        
        float gravity = isSnow ? 2.0f : 15.0f;  // Snow falls slower
        
        for (auto it = particles_.begin(); it != particles_.end(); ) {
            it->position = it->position + it->velocity * deltaTime;
            it->velocity.x += wind.x * deltaTime * 0.5f;
            it->velocity.z += wind.z * deltaTime * 0.5f;
            it->velocity.y -= gravity * deltaTime;
            
            if (isSnow) {
                // Add wobble to snow
                float wobble = std::sin(time_ * 3.0f + it->position.x * 2.0f) * 0.5f;
                it->velocity.x += wobble * deltaTime;
            }
            
            it->life += deltaTime;
            
            // Remove if below ground or too old
            if (it->position.y < 0 || it->life > it->maxLife) {
                it = particles_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Spawn new particles
        while (static_cast<int>(particles_.size()) < targetCount) {
            PrecipitationParticle p;
            
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            
            p.position = Vec3(
                cameraPos.x + dist(rng_) * spawnAreaSize_,
                cameraPos.y + spawnAreaSize_ * 0.5f + dist(rng_) * 5.0f,
                cameraPos.z + dist(rng_) * spawnAreaSize_
            );
            
            if (isSnow) {
                p.velocity = Vec3(wind.x * 0.3f, -2.0f, wind.z * 0.3f);
                p.maxLife = 8.0f;
            } else {
                p.velocity = Vec3(wind.x * 0.5f, -10.0f - dist(rng_) * 5.0f, wind.z * 0.5f);
                p.maxLife = 3.0f;
            }
            
            p.size = currentParams_.precipitationSize * (0.8f + dist(rng_) * 0.4f);
            p.alpha = 0.6f + dist(rng_) * 0.4f;
            p.life = 0;
            
            particles_.push_back(p);
        }
    }
    
    void updateLightning(float deltaTime) {
        if (currentParams_.lightningFrequency < 0.01f) {
            lightningActive_ = false;
            lightningIntensity_ = 0.0f;
            return;
        }
        
        // Decay lightning
        if (lightningActive_) {
            lightningIntensity_ -= deltaTime * 5.0f;
            if (lightningIntensity_ <= 0) {
                lightningActive_ = false;
                lightningIntensity_ = 0.0f;
            }
        }
        
        // Random lightning strikes
        lightningTimer_ -= deltaTime;
        if (lightningTimer_ <= 0) {
            // Trigger lightning
            lightningActive_ = true;
            lightningIntensity_ = 1.0f;
            
            if (onLightningStrike) onLightningStrike();
            
            // Schedule thunder
            thunderTimer_ = currentParams_.thunderDelay;
            
            // Next lightning
            float avgInterval = 60.0f / currentParams_.lightningFrequency;
            std::uniform_real_distribution<float> dist(0.5f, 1.5f);
            lightningTimer_ = avgInterval * dist(rng_);
        }
        
        // Thunder sound
        if (thunderTimer_ > 0) {
            thunderTimer_ -= deltaTime;
            if (thunderTimer_ <= 0 && onThunder) {
                onThunder();
            }
        }
    }
    
    void updateWind(float deltaTime) {
        // Add some wind variation
        std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
        windVariation_ += dist(rng_) * deltaTime;
        windVariation_ *= 0.99f;  // Decay
    }
    
    // State
    WeatherParams currentParams_;
    WeatherParams targetParams_;
    bool isTransitioning_ = false;
    float transitionProgress_ = 0.0f;
    float transitionDuration_ = 2.0f;
    
    // Time
    float time_ = 0.0f;
    
    // Precipitation
    std::vector<PrecipitationParticle> particles_;
    int maxParticles_ = 5000;
    float spawnAreaSize_ = 50.0f;
    
    // Lightning
    bool lightningActive_ = false;
    float lightningIntensity_ = 0.0f;
    float lightningTimer_ = 5.0f;
    float thunderTimer_ = 0.0f;
    
    // Wind
    float windVariation_ = 0.0f;
    
    // Random
    std::mt19937 rng_;
};

// ============================================================================
// Weather Manager - Singleton
// ============================================================================

class WeatherManager {
public:
    static WeatherManager& getInstance() {
        static WeatherManager instance;
        return instance;
    }
    
    WeatherSystem& getSystem() { return system_; }
    const WeatherSystem& getSystem() const { return system_; }
    
    // Convenience methods
    void setWeather(WeatherType type, float transition = 2.0f) {
        system_.setWeather(type, transition);
    }
    
    void update(float deltaTime, const Vec3& cameraPos) {
        system_.update(deltaTime, cameraPos);
    }
    
    WeatherType getCurrentWeather() const {
        return system_.getCurrentWeatherType();
    }
    
private:
    WeatherManager() = default;
    WeatherSystem system_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline WeatherManager& getWeatherManager() {
    return WeatherManager::getInstance();
}

inline WeatherSystem& getWeatherSystem() {
    return WeatherManager::getInstance().getSystem();
}

}  // namespace luma
