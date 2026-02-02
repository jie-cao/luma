// Water System - Rivers, lakes, ocean with realistic rendering
// Wave simulation, reflections, refractions, foam
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <cmath>
#include <random>

namespace luma {

// ============================================================================
// Water Types
// ============================================================================

enum class WaterType {
    Lake,           // 湖泊 - calm water
    River,          // 河流 - flowing water
    Ocean,          // 海洋 - large waves
    Pond,           // 池塘 - small, still
    Waterfall,      // 瀑布 - falling water
    Stream,         // 溪流 - small flowing
    Custom
};

inline std::string waterTypeToString(WaterType type) {
    switch (type) {
        case WaterType::Lake: return "Lake";
        case WaterType::River: return "River";
        case WaterType::Ocean: return "Ocean";
        case WaterType::Pond: return "Pond";
        case WaterType::Waterfall: return "Waterfall";
        case WaterType::Stream: return "Stream";
        case WaterType::Custom: return "Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Water Parameters
// ============================================================================

struct WaterParams {
    WaterType type = WaterType::Lake;
    
    // Geometry
    float width = 100.0f;
    float length = 100.0f;
    float depth = 10.0f;
    float waterLevel = 0.0f;
    int gridResolution = 64;
    
    // Waves
    float waveAmplitude = 0.2f;
    float waveFrequency = 1.0f;
    float waveSpeed = 1.0f;
    Vec2 waveDirection{1, 0};
    int waveOctaves = 4;
    
    // Flow (for rivers)
    Vec2 flowDirection{1, 0};
    float flowSpeed = 1.0f;
    
    // Material
    Vec3 shallowColor{0.2f, 0.5f, 0.5f};
    Vec3 deepColor{0.05f, 0.15f, 0.25f};
    float colorDepthFalloff = 5.0f;
    
    float transparency = 0.8f;
    float reflectivity = 0.5f;
    float refractivity = 0.3f;
    float fresnelPower = 5.0f;
    
    float specularIntensity = 1.0f;
    float specularPower = 64.0f;
    
    // Foam
    bool enableFoam = true;
    float foamThreshold = 0.3f;
    float foamIntensity = 0.5f;
    Vec3 foamColor{0.9f, 0.95f, 1.0f};
    
    // Caustics
    bool enableCaustics = true;
    float causticScale = 1.0f;
    float causticIntensity = 0.3f;
    
    // Normal perturbation
    float normalStrength = 1.0f;
    float normalTiling = 10.0f;
};

// ============================================================================
// Water Presets
// ============================================================================

class WaterPresets {
public:
    static WaterParams getLake() {
        WaterParams p;
        p.type = WaterType::Lake;
        p.waveAmplitude = 0.1f;
        p.waveFrequency = 0.5f;
        p.waveSpeed = 0.5f;
        p.flowSpeed = 0.0f;
        p.shallowColor = Vec3(0.3f, 0.5f, 0.45f);
        p.deepColor = Vec3(0.1f, 0.2f, 0.3f);
        p.transparency = 0.85f;
        p.reflectivity = 0.6f;
        return p;
    }
    
    static WaterParams getRiver() {
        WaterParams p;
        p.type = WaterType::River;
        p.waveAmplitude = 0.15f;
        p.waveFrequency = 1.0f;
        p.waveSpeed = 1.5f;
        p.flowDirection = Vec2(1, 0);
        p.flowSpeed = 2.0f;
        p.shallowColor = Vec3(0.25f, 0.45f, 0.4f);
        p.deepColor = Vec3(0.08f, 0.18f, 0.25f);
        p.transparency = 0.75f;
        p.reflectivity = 0.4f;
        p.foamThreshold = 0.2f;
        p.foamIntensity = 0.6f;
        return p;
    }
    
    static WaterParams getOcean() {
        WaterParams p;
        p.type = WaterType::Ocean;
        p.waveAmplitude = 0.5f;
        p.waveFrequency = 0.3f;
        p.waveSpeed = 1.0f;
        p.waveOctaves = 6;
        p.shallowColor = Vec3(0.15f, 0.4f, 0.45f);
        p.deepColor = Vec3(0.02f, 0.08f, 0.15f);
        p.colorDepthFalloff = 15.0f;
        p.transparency = 0.7f;
        p.reflectivity = 0.7f;
        p.foamThreshold = 0.4f;
        p.foamIntensity = 0.8f;
        return p;
    }
    
    static WaterParams getPond() {
        WaterParams p;
        p.type = WaterType::Pond;
        p.waveAmplitude = 0.02f;
        p.waveFrequency = 2.0f;
        p.waveSpeed = 0.3f;
        p.flowSpeed = 0.0f;
        p.shallowColor = Vec3(0.25f, 0.4f, 0.35f);
        p.deepColor = Vec3(0.1f, 0.2f, 0.2f);
        p.transparency = 0.9f;
        p.reflectivity = 0.7f;
        p.enableFoam = false;
        return p;
    }
    
    static WaterParams getStream() {
        WaterParams p;
        p.type = WaterType::Stream;
        p.waveAmplitude = 0.05f;
        p.waveFrequency = 2.0f;
        p.waveSpeed = 2.0f;
        p.flowSpeed = 3.0f;
        p.shallowColor = Vec3(0.3f, 0.5f, 0.5f);
        p.deepColor = Vec3(0.15f, 0.25f, 0.3f);
        p.transparency = 0.9f;
        p.reflectivity = 0.3f;
        p.foamThreshold = 0.15f;
        return p;
    }
    
    static WaterParams getPreset(WaterType type) {
        switch (type) {
            case WaterType::Lake: return getLake();
            case WaterType::River: return getRiver();
            case WaterType::Ocean: return getOcean();
            case WaterType::Pond: return getPond();
            case WaterType::Stream: return getStream();
            default: return getLake();
        }
    }
};

// ============================================================================
// Wave Simulation
// ============================================================================

class WaveSimulation {
public:
    // Gerstner wave for realistic ocean waves
    static Vec3 gerstnerWave(const Vec2& position, float time,
                            const Vec2& direction, float amplitude,
                            float frequency, float speed, float steepness) {
        float k = 2.0f * 3.14159f * frequency;
        float w = speed * k;
        float phase = k * (direction.x * position.x + direction.y * position.y) - w * time;
        
        float s = std::sin(phase);
        float c = std::cos(phase);
        
        float q = steepness / (k * amplitude);  // Q factor
        
        return Vec3(
            q * amplitude * direction.x * c,
            amplitude * s,
            q * amplitude * direction.y * c
        );
    }
    
    // Multi-octave wave
    static Vec3 calculateWaveDisplacement(const Vec2& position, float time,
                                         const WaterParams& params) {
        Vec3 displacement(0, 0, 0);
        
        float amplitude = params.waveAmplitude;
        float frequency = params.waveFrequency;
        Vec2 direction = params.waveDirection.normalized();
        
        for (int i = 0; i < params.waveOctaves; i++) {
            // Rotate direction slightly for each octave
            float angle = i * 0.5f;
            Vec2 rotDir(
                direction.x * std::cos(angle) - direction.y * std::sin(angle),
                direction.x * std::sin(angle) + direction.y * std::cos(angle)
            );
            
            Vec3 wave = gerstnerWave(position, time, rotDir, amplitude, frequency,
                                     params.waveSpeed, 0.5f);
            displacement = displacement + wave;
            
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }
        
        return displacement;
    }
    
    // Calculate normal from wave
    static Vec3 calculateWaveNormal(const Vec2& position, float time,
                                    const WaterParams& params, float delta = 0.1f) {
        Vec3 center = calculateWaveDisplacement(position, time, params);
        Vec3 right = calculateWaveDisplacement(Vec2(position.x + delta, position.y), time, params);
        Vec3 forward = calculateWaveDisplacement(Vec2(position.x, position.y + delta), time, params);
        
        Vec3 dx = (right - center) + Vec3(delta, 0, 0);
        Vec3 dz = (forward - center) + Vec3(0, 0, delta);
        
        return dz.cross(dx).normalized();
    }
};

// ============================================================================
// Water Surface Mesh
// ============================================================================

class WaterSurface {
public:
    WaterSurface() = default;
    
    void initialize(const WaterParams& params) {
        params_ = params;
        generateMesh();
    }
    
    void update(float time) {
        currentTime_ = time;
        
        if (animateVertices_) {
            updateVertices();
        }
    }
    
    const Mesh& getMesh() const { return mesh_; }
    Mesh& getMesh() { return mesh_; }
    
    const WaterParams& getParams() const { return params_; }
    void setParams(const WaterParams& params) { 
        params_ = params;
        generateMesh();
    }
    
    // Get height at world position
    float getHeightAt(float x, float z, float time) const {
        Vec2 pos(x, z);
        Vec3 disp = WaveSimulation::calculateWaveDisplacement(pos, time, params_);
        return params_.waterLevel + disp.y;
    }
    
    // Get normal at world position
    Vec3 getNormalAt(float x, float z, float time) const {
        return WaveSimulation::calculateWaveNormal(Vec2(x, z), time, params_);
    }
    
    void setAnimateVertices(bool animate) { animateVertices_ = animate; }
    
private:
    void generateMesh() {
        mesh_.vertices.clear();
        mesh_.indices.clear();
        mesh_.name = "WaterSurface";
        
        int res = params_.gridResolution;
        float halfW = params_.width / 2.0f;
        float halfL = params_.length / 2.0f;
        
        // Generate vertices
        for (int z = 0; z <= res; z++) {
            for (int x = 0; x <= res; x++) {
                Vertex v;
                
                float u = (float)x / res;
                float vv = (float)z / res;
                
                v.position[0] = -halfW + u * params_.width;
                v.position[1] = params_.waterLevel;
                v.position[2] = -halfL + vv * params_.length;
                
                v.normal[0] = 0;
                v.normal[1] = 1;
                v.normal[2] = 0;
                
                v.uv[0] = u * params_.normalTiling;
                v.uv[1] = vv * params_.normalTiling;
                
                mesh_.vertices.push_back(v);
            }
        }
        
        // Generate indices
        for (int z = 0; z < res; z++) {
            for (int x = 0; x < res; x++) {
                uint32_t topLeft = z * (res + 1) + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * (res + 1) + x;
                uint32_t bottomRight = bottomLeft + 1;
                
                mesh_.indices.push_back(topLeft);
                mesh_.indices.push_back(bottomLeft);
                mesh_.indices.push_back(topRight);
                
                mesh_.indices.push_back(topRight);
                mesh_.indices.push_back(bottomLeft);
                mesh_.indices.push_back(bottomRight);
            }
        }
        
        // Set material properties
        mesh_.baseColor[0] = params_.shallowColor.x;
        mesh_.baseColor[1] = params_.shallowColor.y;
        mesh_.baseColor[2] = params_.shallowColor.z;
        mesh_.metallic = 0.0f;
        mesh_.roughness = 0.1f;
    }
    
    void updateVertices() {
        int res = params_.gridResolution;
        float halfW = params_.width / 2.0f;
        float halfL = params_.length / 2.0f;
        
        for (int z = 0; z <= res; z++) {
            for (int x = 0; x <= res; x++) {
                int idx = z * (res + 1) + x;
                Vertex& v = mesh_.vertices[idx];
                
                float u = (float)x / res;
                float vv = (float)z / res;
                
                float worldX = -halfW + u * params_.width;
                float worldZ = -halfL + vv * params_.length;
                
                // Calculate wave displacement
                Vec3 disp = WaveSimulation::calculateWaveDisplacement(
                    Vec2(worldX, worldZ), currentTime_, params_);
                
                v.position[0] = worldX + disp.x;
                v.position[1] = params_.waterLevel + disp.y;
                v.position[2] = worldZ + disp.z;
                
                // Calculate normal
                Vec3 normal = WaveSimulation::calculateWaveNormal(
                    Vec2(worldX, worldZ), currentTime_, params_);
                v.normal[0] = normal.x;
                v.normal[1] = normal.y;
                v.normal[2] = normal.z;
            }
        }
    }
    
    WaterParams params_;
    Mesh mesh_;
    float currentTime_ = 0.0f;
    bool animateVertices_ = true;
};

// ============================================================================
// Water Body (complete water area)
// ============================================================================

struct WaterBody {
    std::string id;
    std::string name;
    WaterParams params;
    WaterSurface surface;
    
    Vec3 position{0, 0, 0};
    float rotation = 0.0f;
    
    bool visible = true;
    bool active = true;
};

// ============================================================================
// Water System Manager
// ============================================================================

class WaterSystem {
public:
    static WaterSystem& getInstance() {
        static WaterSystem instance;
        return instance;
    }
    
    // Create water body
    WaterBody* createWaterBody(const std::string& id, WaterType type,
                              const Vec3& position, float width, float length) {
        WaterBody body;
        body.id = id;
        body.name = waterTypeToString(type);
        body.params = WaterPresets::getPreset(type);
        body.params.width = width;
        body.params.length = length;
        body.position = position;
        body.params.waterLevel = position.y;
        
        body.surface.initialize(body.params);
        
        bodies_[id] = body;
        return &bodies_[id];
    }
    
    WaterBody* getWaterBody(const std::string& id) {
        auto it = bodies_.find(id);
        return (it != bodies_.end()) ? &it->second : nullptr;
    }
    
    void removeWaterBody(const std::string& id) {
        bodies_.erase(id);
    }
    
    void update(float deltaTime) {
        time_ += deltaTime;
        
        for (auto& [id, body] : bodies_) {
            if (body.active) {
                body.surface.update(time_);
            }
        }
    }
    
    // Get water height at world position
    float getWaterHeightAt(float x, float z) const {
        float maxHeight = -1e10f;
        
        for (const auto& [id, body] : bodies_) {
            if (!body.active) continue;
            
            // Check if point is within water body bounds
            float localX = x - body.position.x;
            float localZ = z - body.position.z;
            
            if (std::abs(localX) <= body.params.width / 2 &&
                std::abs(localZ) <= body.params.length / 2) {
                float height = body.surface.getHeightAt(localX, localZ, time_);
                maxHeight = std::max(maxHeight, height);
            }
        }
        
        return maxHeight;
    }
    
    // Check if point is underwater
    bool isUnderwater(const Vec3& point) const {
        float waterHeight = getWaterHeightAt(point.x, point.z);
        return point.y < waterHeight;
    }
    
    // Get underwater depth
    float getUnderwaterDepth(const Vec3& point) const {
        float waterHeight = getWaterHeightAt(point.x, point.z);
        return std::max(0.0f, waterHeight - point.y);
    }
    
    // Get all water bodies
    const std::unordered_map<std::string, WaterBody>& getWaterBodies() const {
        return bodies_;
    }
    
    float getTime() const { return time_; }
    
private:
    WaterSystem() = default;
    
    std::unordered_map<std::string, WaterBody> bodies_;
    float time_ = 0.0f;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline WaterSystem& getWaterSystem() {
    return WaterSystem::getInstance();
}

}  // namespace luma
