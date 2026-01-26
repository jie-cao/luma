// Multi-Light System
// Supports directional, point, and spot lights
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace luma {

// ===== Light Types =====
enum class LightType : uint8_t {
    Directional = 0,
    Point,
    Spot
};

// ===== Light Base =====
struct Light {
    // Identity
    std::string name = "Light";
    uint32_t id = 0;
    bool enabled = true;
    
    // Type
    LightType type = LightType::Directional;
    
    // Common properties
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    
    // Transform (position for point/spot, direction for directional)
    Vec3 position{0.0f, 5.0f, 0.0f};
    Vec3 direction{0.0f, -1.0f, 0.0f};  // Normalized direction
    
    // Point/Spot light attenuation
    float range = 10.0f;           // Max effective distance
    float constantAtten = 1.0f;    // Constant attenuation factor
    float linearAtten = 0.09f;     // Linear attenuation factor
    float quadraticAtten = 0.032f; // Quadratic attenuation factor
    
    // Spot light cone
    float innerConeAngle = 25.0f;  // Degrees - full intensity
    float outerConeAngle = 35.0f;  // Degrees - falloff to zero
    
    // Shadow settings
    bool castShadows = true;
    float shadowBias = 0.005f;
    float shadowNormalBias = 0.02f;
    int shadowMapSize = 1024;
    
    // Soft shadows
    float shadowSoftness = 1.0f;
    int shadowPCFSamples = 16;
    
    // Visualization
    bool showGizmo = true;
    
    // Get light type name for UI
    static const char* getTypeName(LightType type) {
        switch (type) {
            case LightType::Directional: return "Directional";
            case LightType::Point: return "Point";
            case LightType::Spot: return "Spot";
            default: return "Unknown";
        }
    }
    
    // Calculate attenuation for point/spot lights
    float calculateAttenuation(float distance) const {
        if (type == LightType::Directional) return 1.0f;
        if (distance > range) return 0.0f;
        
        float atten = 1.0f / (constantAtten + 
                              linearAtten * distance + 
                              quadraticAtten * distance * distance);
        
        // Smooth falloff at range boundary
        float rangeFactor = 1.0f - (distance / range);
        rangeFactor = rangeFactor * rangeFactor;
        
        return atten * rangeFactor;
    }
    
    // Calculate spot light cone factor
    float calculateSpotFactor(const Vec3& toLight) const {
        if (type != LightType::Spot) return 1.0f;
        
        // toLight should be normalized and pointing FROM the surface TO the light
        // direction points FROM the light
        float cosAngle = -(toLight.x * direction.x + 
                          toLight.y * direction.y + 
                          toLight.z * direction.z);
        
        float innerCos = cosf(innerConeAngle * 0.0174533f);
        float outerCos = cosf(outerConeAngle * 0.0174533f);
        
        if (cosAngle > innerCos) return 1.0f;
        if (cosAngle < outerCos) return 0.0f;
        
        // Smooth interpolation
        float t = (cosAngle - outerCos) / (innerCos - outerCos);
        return t * t;  // Quadratic falloff
    }
    
    // Create preset lights
    static Light createDirectional(const Vec3& dir = {0.5f, -1.0f, 0.3f},
                                    const Vec3& col = {1.0f, 0.98f, 0.95f},
                                    float intensity = 1.0f) {
        Light light;
        light.name = "Directional Light";
        light.type = LightType::Directional;
        // Normalize direction
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        light.direction = {dir.x/len, dir.y/len, dir.z/len};
        light.color = col;
        light.intensity = intensity;
        return light;
    }
    
    static Light createPoint(const Vec3& pos = {0, 3, 0},
                              const Vec3& col = {1, 1, 1},
                              float intensity = 1.0f,
                              float range = 10.0f) {
        Light light;
        light.name = "Point Light";
        light.type = LightType::Point;
        light.position = pos;
        light.color = col;
        light.intensity = intensity;
        light.range = range;
        return light;
    }
    
    static Light createSpot(const Vec3& pos = {0, 5, 0},
                             const Vec3& dir = {0, -1, 0},
                             const Vec3& col = {1, 1, 1},
                             float intensity = 1.0f,
                             float innerAngle = 25.0f,
                             float outerAngle = 35.0f) {
        Light light;
        light.name = "Spot Light";
        light.type = LightType::Spot;
        light.position = pos;
        // Normalize direction
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        light.direction = {dir.x/len, dir.y/len, dir.z/len};
        light.color = col;
        light.intensity = intensity;
        light.innerConeAngle = innerAngle;
        light.outerConeAngle = outerAngle;
        return light;
    }
};

// ===== Ambient Light Settings =====
struct AmbientLight {
    Vec3 color{0.1f, 0.1f, 0.15f};
    float intensity = 0.3f;
    
    // IBL (Image-Based Lighting)
    bool useIBL = false;
    std::string environmentMap;
    float iblIntensity = 1.0f;
};

// ===== Light Manager =====
// Manages all lights in the scene
class LightManager {
public:
    static constexpr size_t MAX_LIGHTS = 16;
    
    static LightManager& get() {
        static LightManager instance;
        return instance;
    }
    
    // Add a light
    Light* addLight(LightType type = LightType::Point) {
        if (lights_.size() >= MAX_LIGHTS) return nullptr;
        
        Light light;
        light.id = nextId_++;
        light.type = type;
        
        switch (type) {
            case LightType::Directional:
                light = Light::createDirectional();
                break;
            case LightType::Point:
                light = Light::createPoint();
                break;
            case LightType::Spot:
                light = Light::createSpot();
                break;
        }
        light.id = nextId_ - 1;  // Use the ID we assigned
        
        lights_.push_back(std::make_unique<Light>(light));
        return lights_.back().get();
    }
    
    // Remove a light
    void removeLight(uint32_t id) {
        for (auto it = lights_.begin(); it != lights_.end(); ++it) {
            if ((*it)->id == id) {
                lights_.erase(it);
                return;
            }
        }
    }
    
    // Get light by ID
    Light* getLight(uint32_t id) {
        for (auto& light : lights_) {
            if (light->id == id) return light.get();
        }
        return nullptr;
    }
    
    // Get all lights
    const std::vector<std::unique_ptr<Light>>& getLights() const { return lights_; }
    
    // Get lights by type
    std::vector<Light*> getLightsByType(LightType type) {
        std::vector<Light*> result;
        for (auto& light : lights_) {
            if (light->type == type && light->enabled) {
                result.push_back(light.get());
            }
        }
        return result;
    }
    
    // Get primary directional light (for main shadow)
    Light* getPrimaryDirectional() {
        for (auto& light : lights_) {
            if (light->type == LightType::Directional && light->enabled) {
                return light.get();
            }
        }
        return nullptr;
    }
    
    // Count enabled lights
    size_t getEnabledLightCount() const {
        size_t count = 0;
        for (auto& light : lights_) {
            if (light->enabled) count++;
        }
        return count;
    }
    
    // Ambient light settings
    AmbientLight& getAmbient() { return ambient_; }
    const AmbientLight& getAmbient() const { return ambient_; }
    
    // Pack light data for GPU constant buffer
    // Returns data suitable for uploading to a structured buffer
    struct GPULightData {
        float position[4];     // xyz = position, w = type
        float direction[4];    // xyz = direction, w = range
        float color[4];        // rgb = color, a = intensity
        float params[4];       // x = innerCone, y = outerCone, z = shadowBias, w = enabled
    };
    
    void packLightData(GPULightData* outData, size_t& outCount) {
        outCount = 0;
        for (auto& light : lights_) {
            if (!light->enabled) continue;
            if (outCount >= MAX_LIGHTS) break;
            
            GPULightData& data = outData[outCount++];
            
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.position[3] = static_cast<float>(light->type);
            
            data.direction[0] = light->direction.x;
            data.direction[1] = light->direction.y;
            data.direction[2] = light->direction.z;
            data.direction[3] = light->range;
            
            data.color[0] = light->color.x;
            data.color[1] = light->color.y;
            data.color[2] = light->color.z;
            data.color[3] = light->intensity;
            
            data.params[0] = light->innerConeAngle;
            data.params[1] = light->outerConeAngle;
            data.params[2] = light->shadowBias;
            data.params[3] = light->castShadows ? 1.0f : 0.0f;
        }
    }
    
    // Clear all lights
    void clear() {
        lights_.clear();
        nextId_ = 1;
    }
    
    // Initialize with default lights
    void initializeDefaults() {
        clear();
        
        // Add a default directional light
        auto* sun = addLight(LightType::Directional);
        sun->name = "Sun";
        sun->direction = {0.5f, -0.8f, 0.3f};
        sun->color = {1.0f, 0.98f, 0.95f};
        sun->intensity = 1.0f;
    }
    
private:
    LightManager() {
        initializeDefaults();
    }
    
    std::vector<std::unique_ptr<Light>> lights_;
    AmbientLight ambient_;
    uint32_t nextId_ = 1;
};

// ===== Global accessor =====
inline LightManager& getLightManager() {
    return LightManager::get();
}

}  // namespace luma
