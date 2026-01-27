// Global Illumination System
// Combines light probes, reflection probes, and baking
#pragma once

#include "spherical_harmonics.h"
#include "light_probe.h"
#include "reflection_probe.h"
#include <functional>
#include <random>

namespace luma {

// ===== GI Settings =====
struct GISettings {
    // Light probes
    bool lightProbesEnabled = true;
    float lightProbeIntensity = 1.0f;
    int lightProbeSamples = 64;  // Samples per probe for baking
    
    // Reflection probes
    bool reflectionProbesEnabled = true;
    float reflectionProbeIntensity = 1.0f;
    
    // Ambient
    Vec3 ambientSkyColor = {0.5f, 0.7f, 1.0f};
    Vec3 ambientGroundColor = {0.2f, 0.15f, 0.1f};
    float ambientIntensity = 0.3f;
    
    // Baking
    int bounces = 2;           // Number of light bounces
    int raysPerSample = 32;    // Rays per sample direction
    float rayLength = 100.0f;  // Maximum ray distance
};

// ===== Bake Job =====
struct BakeJob {
    enum class Type {
        LightProbe,
        ReflectionProbe
    };
    
    Type type;
    void* probe;
    int progress;
    int total;
    bool complete;
};

// ===== Ray Trace Result (for baking) =====
struct RayTraceResult {
    bool hit;
    Vec3 position;
    Vec3 normal;
    Vec3 albedo;
    float distance;
};

// ===== GI System =====
class GISystem {
public:
    using RayTraceCallback = std::function<RayTraceResult(const Vec3& origin, const Vec3& direction, float maxDist)>;
    
    // Light info for baking (defined early for forward references)
    struct LightInfo {
        enum class Type { Directional, Point, Spot };
        Type type = Type::Directional;
        Vec3 position;
        Vec3 direction;
        Vec3 color;
        float intensity = 1.0f;
        float range = 10.0f;
        float spotAngle = 45.0f;
    };
    
    GISystem() = default;
    
    // Settings
    void setSettings(const GISettings& settings) { settings_ = settings; }
    GISettings& getSettings() { return settings_; }
    const GISettings& getSettings() const { return settings_; }
    
    // Light probe grid
    void initializeLightProbeGrid(const Vec3& min, const Vec3& max, int resX, int resY, int resZ) {
        lightProbeGrid_.initialize(min, max, resX, resY, resZ);
        gridInitialized_ = true;
    }
    
    LightProbeGrid& getLightProbeGrid() { return lightProbeGrid_; }
    const LightProbeGrid& getLightProbeGrid() const { return lightProbeGrid_; }
    bool hasLightProbeGrid() const { return gridInitialized_; }
    
    // Light probe groups (for non-grid probes)
    LightProbeGroup& addLightProbeGroup(const std::string& name = "LightProbeGroup") {
        lightProbeGroups_.push_back(std::make_unique<LightProbeGroup>(name));
        return *lightProbeGroups_.back();
    }
    
    void removeLightProbeGroup(LightProbeGroup* group) {
        lightProbeGroups_.erase(
            std::remove_if(lightProbeGroups_.begin(), lightProbeGroups_.end(),
                [group](const auto& g) { return g.get() == group; }),
            lightProbeGroups_.end()
        );
    }
    
    const std::vector<std::unique_ptr<LightProbeGroup>>& getLightProbeGroups() const {
        return lightProbeGroups_;
    }
    
    // Reflection probe manager access
    ReflectionProbeManager& getReflectionProbeManager() {
        return luma::getReflectionProbeManager();
    }
    
    // Sample GI at a position
    Vec3 sampleIndirectDiffuse(const Vec3& position, const Vec3& normal) const {
        if (!settings_.lightProbesEnabled) {
            return getAmbientSH().evaluateIrradiance(normal) * settings_.ambientIntensity;
        }
        
        // Sample from grid if available
        if (gridInitialized_) {
            SHCoefficients sh = lightProbeGrid_.sampleSH(position);
            return sh.evaluateIrradiance(normal) * settings_.lightProbeIntensity;
        }
        
        // Sample from groups
        SHCoefficients combinedSH;
        float totalWeight = 0.0f;
        
        for (const auto& group : lightProbeGroups_) {
            SHCoefficients groupSH = group->interpolateSH(position);
            combinedSH.add(groupSH);
            totalWeight += 1.0f;
        }
        
        if (totalWeight > 0.0f) {
            combinedSH.scale(1.0f / totalWeight);
            return combinedSH.evaluateIrradiance(normal) * settings_.lightProbeIntensity;
        }
        
        // Fallback to ambient
        return getAmbientSH().evaluateIrradiance(normal) * settings_.ambientIntensity;
    }
    
    // Get ambient SH
    SHCoefficients getAmbientSH() const {
        return SHCoefficients::fromSkyGradient(
            settings_.ambientSkyColor,
            settings_.ambientGroundColor
        );
    }
    
    // ===== Baking =====
    
    // Set ray trace callback for baking
    void setRayTraceCallback(RayTraceCallback callback) {
        rayTraceCallback_ = callback;
    }
    
    // Bake a single light probe
    void bakeLightProbe(LightProbe& probe, const std::vector<LightInfo>& lights) {
        if (!rayTraceCallback_) {
            // Use simple sky gradient if no ray trace
            probe.setSHCoefficients(getAmbientSH());
            probe.setDirty(false);
            probe.setValid(true);
            return;
        }
        
        auto samples = SHSampleGenerator::generateSamples(settings_.lightProbeSamples);
        SHCoefficients sh;
        
        for (const auto& sample : samples) {
            Vec3 radiance = traceRadiance(probe.getPosition(), sample.direction, 0);
            
            for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
                sh.coefficients[i] = sh.coefficients[i] + radiance * sample.basis[i] * sample.solidAngle;
            }
        }
        
        // Add direct light contribution
        for (const auto& light : lights) {
            addDirectLightToSH(sh, light, probe.getPosition());
        }
        
        probe.setSHCoefficients(sh);
        probe.setDirty(false);
        probe.setValid(true);
    }
    
    // Bake all light probes in grid
    void bakeAllLightProbes(const std::vector<LightInfo>& lights,
                            std::function<void(int, int)> progressCallback = nullptr)
    {
        if (!gridInitialized_) return;
        
        auto& probes = lightProbeGrid_.getProbes();
        int total = (int)probes.size();
        
        for (int i = 0; i < total; i++) {
            bakeLightProbe(probes[i], lights);
            
            if (progressCallback) {
                progressCallback(i + 1, total);
            }
        }
    }
    
    // Bake all light probe groups
    void bakeAllLightProbeGroups(const std::vector<LightInfo>& lights) {
        for (auto& group : lightProbeGroups_) {
            for (auto& probe : group->getProbes()) {
                bakeLightProbe(*probe, lights);
            }
        }
    }
    
    // Clear all baked data
    void clearBakedData() {
        if (gridInitialized_) {
            for (auto& probe : lightProbeGrid_.getProbes()) {
                probe.setDirty(true);
                probe.setValid(false);
            }
        }
        
        for (auto& group : lightProbeGroups_) {
            group->markAllDirty();
        }
    }
    
    // ===== GPU Data Export =====
    
    // Export SH data for GPU (all grid probes)
    struct GPUProbeData {
        std::vector<SHGPUData> shData;
        Vec3 gridMin;
        Vec3 gridMax;
        Vec3 gridSize;
        int resX, resY, resZ;
    };
    
    GPUProbeData exportGPUData() const {
        GPUProbeData data;
        
        if (gridInitialized_) {
            data.gridMin = lightProbeGrid_.getMinBounds();
            data.gridMax = lightProbeGrid_.getMaxBounds();
            data.gridSize = lightProbeGrid_.getCellSize();
            data.resX = lightProbeGrid_.getResolutionX();
            data.resY = lightProbeGrid_.getResolutionY();
            data.resZ = lightProbeGrid_.getResolutionZ();
            
            const auto& probes = lightProbeGrid_.getProbes();
            data.shData.resize(probes.size());
            
            for (size_t i = 0; i < probes.size(); i++) {
                data.shData[i].fromSHCoefficients(probes[i].getSHCoefficients());
            }
        }
        
        return data;
    }
    
private:
    // Trace radiance for a ray (recursive for bounces)
    Vec3 traceRadiance(const Vec3& origin, const Vec3& direction, int bounce) {
        if (bounce >= settings_.bounces || !rayTraceCallback_) {
            // Return sky color based on direction
            float t = direction.y * 0.5f + 0.5f;
            return settings_.ambientGroundColor * (1.0f - t) + settings_.ambientSkyColor * t;
        }
        
        RayTraceResult hit = rayTraceCallback_(origin, direction, settings_.rayLength);
        
        if (!hit.hit) {
            // Sky
            float t = direction.y * 0.5f + 0.5f;
            return settings_.ambientGroundColor * (1.0f - t) + settings_.ambientSkyColor * t;
        }
        
        // Compute bounced radiance
        Vec3 bounceDir = randomHemisphereDirection(hit.normal);
        Vec3 bounceRadiance = traceRadiance(hit.position + hit.normal * 0.001f, bounceDir, bounce + 1);
        
        // Lambertian BRDF
        float cosTheta = std::max(0.0f, hit.normal.dot(bounceDir));
        return hit.albedo * bounceRadiance * cosTheta;
    }
    
    // Add direct light contribution to SH
    void addDirectLightToSH(SHCoefficients& sh, const LightInfo& light, const Vec3& probePos) {
        if (light.type == LightInfo::Type::Directional) {
            // Check shadow (simplified - no actual shadow test here)
            SHCoefficients directSH = SHCoefficients::fromDirectionalLight(
                light.direction * -1.0f,  // Towards light
                light.color * light.intensity
            );
            sh.add(directSH);
        }
        // Point/spot lights would require more complex handling
    }
    
    // Random hemisphere direction (for diffuse bounces)
    Vec3 randomHemisphereDirection(const Vec3& normal) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        float u1 = dist(rng_);
        float u2 = dist(rng_);
        
        float r = std::sqrt(u1);
        float theta = 2.0f * 3.14159265f * u2;
        
        float x = r * std::cos(theta);
        float y = r * std::sin(theta);
        float z = std::sqrt(1.0f - u1);
        
        // Transform to normal's hemisphere
        Vec3 tangent, bitangent;
        if (std::abs(normal.x) > 0.9f) {
            tangent = normal.cross(Vec3(0, 1, 0)).normalized();
        } else {
            tangent = normal.cross(Vec3(1, 0, 0)).normalized();
        }
        bitangent = normal.cross(tangent);
        
        return tangent * x + bitangent * y + normal * z;
    }
    
    GISettings settings_;
    LightProbeGrid lightProbeGrid_;
    std::vector<std::unique_ptr<LightProbeGroup>> lightProbeGroups_;
    bool gridInitialized_ = false;
    
    RayTraceCallback rayTraceCallback_;
    std::mt19937 rng_{42};
};

// ===== Global GI System =====
inline GISystem& getGISystem() {
    static GISystem system;
    return system;
}

}  // namespace luma
