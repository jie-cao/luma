// Day/Night Cycle System - Dynamic skybox and lighting
// Realistic sun/moon movement, atmospheric scattering, stars
#pragma once

#include "engine/foundation/math_types.h"
#include <cmath>
#include <functional>

namespace luma {

// ============================================================================
// Time of Day
// ============================================================================

enum class TimeOfDay {
    Dawn,           // 黎明 (5:00 - 7:00)
    Morning,        // 上午 (7:00 - 11:00)
    Noon,           // 正午 (11:00 - 13:00)
    Afternoon,      // 下午 (13:00 - 17:00)
    Dusk,           // 黄昏 (17:00 - 19:00)
    Evening,        // 傍晚 (19:00 - 21:00)
    Night,          // 夜晚 (21:00 - 5:00)
};

inline std::string timeOfDayToString(TimeOfDay tod) {
    switch (tod) {
        case TimeOfDay::Dawn: return "Dawn";
        case TimeOfDay::Morning: return "Morning";
        case TimeOfDay::Noon: return "Noon";
        case TimeOfDay::Afternoon: return "Afternoon";
        case TimeOfDay::Dusk: return "Dusk";
        case TimeOfDay::Evening: return "Evening";
        case TimeOfDay::Night: return "Night";
        default: return "Unknown";
    }
}

inline std::string timeOfDayToDisplayName(TimeOfDay tod) {
    switch (tod) {
        case TimeOfDay::Dawn: return "黎明 Dawn";
        case TimeOfDay::Morning: return "上午 Morning";
        case TimeOfDay::Noon: return "正午 Noon";
        case TimeOfDay::Afternoon: return "下午 Afternoon";
        case TimeOfDay::Dusk: return "黄昏 Dusk";
        case TimeOfDay::Evening: return "傍晚 Evening";
        case TimeOfDay::Night: return "夜晚 Night";
        default: return "Unknown";
    }
}

// ============================================================================
// Sky Colors for different times
// ============================================================================

struct SkyGradient {
    Vec3 zenithColor;       // Color at top of sky
    Vec3 horizonColor;      // Color at horizon
    Vec3 groundColor;       // Color below horizon (for ground reflection)
    
    static SkyGradient lerp(const SkyGradient& a, const SkyGradient& b, float t) {
        SkyGradient result;
        result.zenithColor = a.zenithColor.lerp(b.zenithColor, t);
        result.horizonColor = a.horizonColor.lerp(b.horizonColor, t);
        result.groundColor = a.groundColor.lerp(b.groundColor, t);
        return result;
    }
};

// ============================================================================
// Day/Night Parameters
// ============================================================================

struct DayNightParams {
    // Time (0-24 hours)
    float timeOfDay = 12.0f;
    
    // Sun
    Vec3 sunDirection;
    Vec3 sunColor;
    float sunIntensity = 1.0f;
    float sunSize = 0.02f;  // Angular size
    
    // Moon
    Vec3 moonDirection;
    Vec3 moonColor;
    float moonIntensity = 0.1f;
    float moonPhase = 0.5f;  // 0=new, 0.5=full, 1=new
    float moonSize = 0.015f;
    
    // Sky
    SkyGradient skyGradient;
    float atmosphericDensity = 1.0f;
    
    // Stars
    float starVisibility = 0.0f;  // 0=invisible, 1=fully visible
    float starTwinkle = 0.0f;
    
    // Ambient
    Vec3 ambientColor;
    float ambientIntensity = 0.3f;
    
    // Fog (time-based)
    Vec3 fogColor;
    float fogDensity = 0.0f;
};

// ============================================================================
// Day/Night Cycle System
// ============================================================================

class DayNightCycle {
public:
    DayNightCycle() {
        initializeSkyColors();
    }
    
    // === Time Control ===
    
    void setTime(float hours) {
        currentTime_ = std::fmod(hours, 24.0f);
        if (currentTime_ < 0) currentTime_ += 24.0f;
        updateParameters();
    }
    
    void setTimeNormalized(float t) {
        setTime(t * 24.0f);
    }
    
    float getTime() const { return currentTime_; }
    float getTimeNormalized() const { return currentTime_ / 24.0f; }
    
    std::string getTimeString() const {
        int hours = static_cast<int>(currentTime_);
        int minutes = static_cast<int>((currentTime_ - hours) * 60);
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", hours, minutes);
        return buf;
    }
    
    TimeOfDay getTimeOfDay() const {
        if (currentTime_ >= 5.0f && currentTime_ < 7.0f) return TimeOfDay::Dawn;
        if (currentTime_ >= 7.0f && currentTime_ < 11.0f) return TimeOfDay::Morning;
        if (currentTime_ >= 11.0f && currentTime_ < 13.0f) return TimeOfDay::Noon;
        if (currentTime_ >= 13.0f && currentTime_ < 17.0f) return TimeOfDay::Afternoon;
        if (currentTime_ >= 17.0f && currentTime_ < 19.0f) return TimeOfDay::Dusk;
        if (currentTime_ >= 19.0f && currentTime_ < 21.0f) return TimeOfDay::Evening;
        return TimeOfDay::Night;
    }
    
    // === Auto Advance ===
    
    void setAutoAdvance(bool enabled, float speed = 1.0f) {
        autoAdvance_ = enabled;
        timeSpeed_ = speed;  // 1.0 = 1 real second = 1 game minute
    }
    
    void update(float deltaTime) {
        if (autoAdvance_) {
            // Convert real seconds to game hours
            float gameMinutes = deltaTime * timeSpeed_;
            float gameHours = gameMinutes / 60.0f;
            setTime(currentTime_ + gameHours);
        }
    }
    
    // === Parameters ===
    
    const DayNightParams& getParams() const { return params_; }
    
    // Sun
    Vec3 getSunDirection() const { return params_.sunDirection; }
    Vec3 getSunColor() const { return params_.sunColor; }
    float getSunIntensity() const { return params_.sunIntensity; }
    
    // Moon
    Vec3 getMoonDirection() const { return params_.moonDirection; }
    Vec3 getMoonColor() const { return params_.moonColor; }
    float getMoonIntensity() const { return params_.moonIntensity; }
    void setMoonPhase(float phase) { moonPhase_ = std::fmod(phase, 1.0f); }
    
    // Sky
    const SkyGradient& getSkyGradient() const { return params_.skyGradient; }
    Vec3 getZenithColor() const { return params_.skyGradient.zenithColor; }
    Vec3 getHorizonColor() const { return params_.skyGradient.horizonColor; }
    
    // Stars
    float getStarVisibility() const { return params_.starVisibility; }
    
    // Ambient
    Vec3 getAmbientColor() const { return params_.ambientColor; }
    float getAmbientIntensity() const { return params_.ambientIntensity; }
    
    // === Configuration ===
    
    void setLatitude(float lat) { latitude_ = lat; updateParameters(); }
    float getLatitude() const { return latitude_; }
    
    void setDayOfYear(int day) { dayOfYear_ = day % 365; updateParameters(); }
    int getDayOfYear() const { return dayOfYear_; }
    
    // === Callbacks ===
    
    std::function<void(TimeOfDay)> onTimeOfDayChanged;
    std::function<void()> onSunrise;
    std::function<void()> onSunset;
    
private:
    void initializeSkyColors() {
        // Dawn (5:00-7:00)
        skyColors_[0].zenithColor = Vec3(0.2f, 0.3f, 0.5f);
        skyColors_[0].horizonColor = Vec3(0.9f, 0.6f, 0.4f);
        skyColors_[0].groundColor = Vec3(0.15f, 0.1f, 0.1f);
        
        // Morning (7:00-11:00)
        skyColors_[1].zenithColor = Vec3(0.3f, 0.5f, 0.8f);
        skyColors_[1].horizonColor = Vec3(0.7f, 0.8f, 0.9f);
        skyColors_[1].groundColor = Vec3(0.2f, 0.25f, 0.2f);
        
        // Noon (11:00-13:00)
        skyColors_[2].zenithColor = Vec3(0.2f, 0.4f, 0.9f);
        skyColors_[2].horizonColor = Vec3(0.6f, 0.75f, 0.95f);
        skyColors_[2].groundColor = Vec3(0.25f, 0.3f, 0.25f);
        
        // Afternoon (13:00-17:00)
        skyColors_[3].zenithColor = Vec3(0.25f, 0.45f, 0.85f);
        skyColors_[3].horizonColor = Vec3(0.65f, 0.75f, 0.9f);
        skyColors_[3].groundColor = Vec3(0.25f, 0.28f, 0.22f);
        
        // Dusk (17:00-19:00)
        skyColors_[4].zenithColor = Vec3(0.3f, 0.35f, 0.6f);
        skyColors_[4].horizonColor = Vec3(0.95f, 0.5f, 0.3f);
        skyColors_[4].groundColor = Vec3(0.2f, 0.15f, 0.12f);
        
        // Evening (19:00-21:00)
        skyColors_[5].zenithColor = Vec3(0.1f, 0.12f, 0.25f);
        skyColors_[5].horizonColor = Vec3(0.4f, 0.25f, 0.3f);
        skyColors_[5].groundColor = Vec3(0.08f, 0.06f, 0.08f);
        
        // Night (21:00-5:00)
        skyColors_[6].zenithColor = Vec3(0.02f, 0.02f, 0.05f);
        skyColors_[6].horizonColor = Vec3(0.05f, 0.05f, 0.1f);
        skyColors_[6].groundColor = Vec3(0.02f, 0.02f, 0.03f);
    }
    
    void updateParameters() {
        TimeOfDay prevTod = lastTimeOfDay_;
        TimeOfDay currentTod = getTimeOfDay();
        
        // Notify time of day change
        if (currentTod != prevTod && onTimeOfDayChanged) {
            onTimeOfDayChanged(currentTod);
        }
        
        // Check for sunrise/sunset
        if (prevTod == TimeOfDay::Night && currentTod == TimeOfDay::Dawn && onSunrise) {
            onSunrise();
        }
        if (prevTod == TimeOfDay::Dusk && currentTod == TimeOfDay::Evening && onSunset) {
            onSunset();
        }
        
        lastTimeOfDay_ = currentTod;
        
        // Calculate sun position
        updateSunPosition();
        
        // Calculate moon position
        updateMoonPosition();
        
        // Calculate sky colors
        updateSkyColors();
        
        // Calculate ambient
        updateAmbient();
        
        // Calculate star visibility
        updateStars();
        
        params_.timeOfDay = currentTime_;
    }
    
    void updateSunPosition() {
        // Simplified sun position calculation
        // Full accuracy would use astronomical calculations
        
        float hourAngle = (currentTime_ - 12.0f) * 15.0f * (3.14159f / 180.0f);  // 15 degrees per hour
        
        // Seasonal variation (simplified)
        float declination = 23.45f * std::sin((dayOfYear_ - 81) * 2.0f * 3.14159f / 365.0f);
        declination *= (3.14159f / 180.0f);
        
        float latRad = latitude_ * (3.14159f / 180.0f);
        
        // Solar altitude
        float sinAlt = std::sin(latRad) * std::sin(declination) +
                       std::cos(latRad) * std::cos(declination) * std::cos(hourAngle);
        float altitude = std::asin(sinAlt);
        
        // Solar azimuth
        float cosAz = (std::sin(declination) - std::sin(latRad) * sinAlt) /
                      (std::cos(latRad) * std::cos(altitude));
        cosAz = std::clamp(cosAz, -1.0f, 1.0f);
        float azimuth = std::acos(cosAz);
        if (hourAngle > 0) azimuth = 2.0f * 3.14159f - azimuth;
        
        // Convert to direction vector
        params_.sunDirection = Vec3(
            std::cos(altitude) * std::sin(azimuth),
            std::sin(altitude),
            std::cos(altitude) * std::cos(azimuth)
        ).normalized();
        
        // Sun color based on altitude
        float altDeg = altitude * (180.0f / 3.14159f);
        
        if (altDeg < -10.0f) {
            // Sun below horizon
            params_.sunColor = Vec3(0, 0, 0);
            params_.sunIntensity = 0.0f;
        } else if (altDeg < 0.0f) {
            // Twilight
            float t = (altDeg + 10.0f) / 10.0f;
            params_.sunColor = Vec3(1.0f, 0.4f * t, 0.2f * t);
            params_.sunIntensity = t * 0.3f;
        } else if (altDeg < 10.0f) {
            // Sunrise/sunset - warm colors
            float t = altDeg / 10.0f;
            params_.sunColor = Vec3(1.0f, 0.6f + 0.3f * t, 0.4f + 0.4f * t);
            params_.sunIntensity = 0.3f + 0.5f * t;
        } else if (altDeg < 30.0f) {
            // Low sun
            float t = (altDeg - 10.0f) / 20.0f;
            params_.sunColor = Vec3(1.0f, 0.9f + 0.1f * t, 0.8f + 0.2f * t);
            params_.sunIntensity = 0.8f + 0.2f * t;
        } else {
            // High sun - white light
            params_.sunColor = Vec3(1.0f, 0.98f, 0.95f);
            params_.sunIntensity = 1.0f;
        }
    }
    
    void updateMoonPosition() {
        // Moon is roughly opposite the sun (simplified)
        float moonHourOffset = 12.0f + (moonPhase_ - 0.5f) * 24.0f;
        float moonTime = std::fmod(currentTime_ + moonHourOffset, 24.0f);
        
        float hourAngle = (moonTime - 12.0f) * 15.0f * (3.14159f / 180.0f);
        float latRad = latitude_ * (3.14159f / 180.0f);
        
        // Simplified position
        float altitude = std::cos(hourAngle) * (3.14159f / 3.0f);  // Max 60 degrees
        float azimuth = hourAngle + 3.14159f / 2.0f;
        
        params_.moonDirection = Vec3(
            std::cos(altitude) * std::sin(azimuth),
            std::sin(altitude),
            std::cos(altitude) * std::cos(azimuth)
        ).normalized();
        
        // Moon color and intensity
        params_.moonColor = Vec3(0.7f, 0.7f, 0.8f);
        
        // Moon intensity based on phase and altitude
        float phaseIntensity = 0.5f + 0.5f * std::cos((moonPhase_ - 0.5f) * 2.0f * 3.14159f);
        float altIntensity = std::max(0.0f, params_.moonDirection.y);
        params_.moonIntensity = phaseIntensity * altIntensity * 0.15f;
        params_.moonPhase = moonPhase_;
    }
    
    void updateSkyColors() {
        // Determine which sky colors to blend between
        int index1, index2;
        float blendFactor;
        
        if (currentTime_ < 5.0f) {
            index1 = 6; index2 = 6; blendFactor = 0.0f;  // Night
        } else if (currentTime_ < 7.0f) {
            index1 = 6; index2 = 0; blendFactor = (currentTime_ - 5.0f) / 2.0f;  // Night to Dawn
        } else if (currentTime_ < 11.0f) {
            index1 = 0; index2 = 1; blendFactor = (currentTime_ - 7.0f) / 4.0f;  // Dawn to Morning
        } else if (currentTime_ < 13.0f) {
            index1 = 1; index2 = 2; blendFactor = (currentTime_ - 11.0f) / 2.0f;  // Morning to Noon
        } else if (currentTime_ < 17.0f) {
            index1 = 2; index2 = 3; blendFactor = (currentTime_ - 13.0f) / 4.0f;  // Noon to Afternoon
        } else if (currentTime_ < 19.0f) {
            index1 = 3; index2 = 4; blendFactor = (currentTime_ - 17.0f) / 2.0f;  // Afternoon to Dusk
        } else if (currentTime_ < 21.0f) {
            index1 = 4; index2 = 5; blendFactor = (currentTime_ - 19.0f) / 2.0f;  // Dusk to Evening
        } else {
            index1 = 5; index2 = 6; blendFactor = (currentTime_ - 21.0f) / 8.0f;  // Evening to Night
            if (blendFactor > 1.0f) blendFactor = 1.0f;
        }
        
        params_.skyGradient = SkyGradient::lerp(skyColors_[index1], skyColors_[index2], blendFactor);
        
        // Fog color matches horizon
        params_.fogColor = params_.skyGradient.horizonColor;
    }
    
    void updateAmbient() {
        // Ambient light is a blend of sky color and ground bounce
        float sunContrib = std::max(0.0f, params_.sunDirection.y);
        float moonContrib = params_.moonIntensity;
        
        Vec3 skyAmbient = (params_.skyGradient.zenithColor + params_.skyGradient.horizonColor) * 0.5f;
        
        params_.ambientColor = skyAmbient * 0.3f + Vec3(0.1f, 0.1f, 0.15f);
        params_.ambientIntensity = 0.15f + sunContrib * 0.25f + moonContrib * 0.05f;
    }
    
    void updateStars() {
        // Stars visible when sun is below horizon
        float sunAlt = params_.sunDirection.y;
        
        if (sunAlt < -0.1f) {
            params_.starVisibility = std::min(1.0f, (-sunAlt - 0.1f) * 5.0f);
        } else {
            params_.starVisibility = 0.0f;
        }
        
        // Twinkle effect
        params_.starTwinkle = params_.starVisibility * 0.3f;
    }
    
    // State
    float currentTime_ = 12.0f;  // Hours (0-24)
    TimeOfDay lastTimeOfDay_ = TimeOfDay::Noon;
    
    // Auto advance
    bool autoAdvance_ = false;
    float timeSpeed_ = 1.0f;  // Game minutes per real second
    
    // Location
    float latitude_ = 40.0f;   // Degrees (positive = north)
    int dayOfYear_ = 172;      // Summer solstice
    
    // Moon
    float moonPhase_ = 0.5f;   // 0=new, 0.5=full
    
    // Sky colors for each time of day
    SkyGradient skyColors_[7];
    
    // Current parameters
    DayNightParams params_;
};

// ============================================================================
// Day/Night Manager - Singleton
// ============================================================================

class DayNightManager {
public:
    static DayNightManager& getInstance() {
        static DayNightManager instance;
        return instance;
    }
    
    DayNightCycle& getCycle() { return cycle_; }
    const DayNightCycle& getCycle() const { return cycle_; }
    
    // Convenience methods
    void setTime(float hours) { cycle_.setTime(hours); }
    float getTime() const { return cycle_.getTime(); }
    
    void update(float deltaTime) { cycle_.update(deltaTime); }
    
    Vec3 getSunDirection() const { return cycle_.getSunDirection(); }
    Vec3 getSunColor() const { return cycle_.getSunColor(); }
    
private:
    DayNightManager() = default;
    DayNightCycle cycle_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline DayNightManager& getDayNightManager() {
    return DayNightManager::getInstance();
}

inline DayNightCycle& getDayNightCycle() {
    return DayNightManager::getInstance().getCycle();
}

}  // namespace luma
