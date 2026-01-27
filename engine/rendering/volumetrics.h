// Volumetric Effects
// Volumetric lighting, fog, and god rays
#pragma once

#include "../foundation/math_types.h"
#include <cmath>
#include <algorithm>

namespace luma {

// ===== Volumetric Fog Settings =====
struct VolumetricFogSettings {
    // Fog parameters
    float density = 0.02f;          // Base fog density
    float heightFalloff = 0.1f;     // How quickly fog thins with height
    float heightOffset = 0.0f;      // Base height for fog
    Vec3 albedo{0.9f, 0.9f, 0.95f}; // Fog color (scattering albedo)
    
    // Extinction
    float scattering = 0.5f;        // In-scattering coefficient
    float absorption = 0.1f;        // Absorption coefficient
    
    // Lighting
    float ambientIntensity = 0.2f;  // Ambient fog illumination
    float lightIntensity = 1.0f;    // Directional light contribution
    float anisotropy = 0.6f;        // Henyey-Greenstein phase function g (-1 to 1)
    
    // Quality
    int steps = 64;                 // Ray march steps
    float maxDistance = 200.0f;     // Max fog distance
    bool temporalReprojection = true;
};

// ===== Volumetric Light Settings =====
struct VolumetricLightSettings {
    // Ray march
    int samples = 32;               // Samples per ray
    float maxDistance = 100.0f;     // Max ray distance
    
    // Appearance
    float intensity = 1.0f;         // Light shaft intensity
    float decay = 0.95f;            // Intensity decay per sample
    float exposure = 0.3f;          // Post-process exposure
    float weight = 0.5f;            // Blend weight
    
    // Quality
    bool halfResolution = true;
    int blurPasses = 2;
    
    // Debug
    bool showOnlyShafts = false;
};

// ===== God Ray Settings =====
struct GodRaySettings {
    Vec3 lightPosition;             // Light source position (can be sun)
    Vec3 lightColor{1.0f, 0.95f, 0.8f};
    
    int samples = 100;              // Number of samples
    float density = 1.0f;           // Ray density
    float weight = 0.01f;           // Sample weight
    float decay = 0.97f;            // Decay factor
    float exposure = 1.0f;          // Final exposure
    
    // Screen-space light position (computed)
    Vec3 screenLightPos;
};

// ===== Atmospheric Scattering =====
struct AtmosphereSettings {
    // Planet
    float planetRadius = 6371000.0f;    // Earth radius in meters
    float atmosphereRadius = 6471000.0f;// Atmosphere outer radius
    
    // Rayleigh scattering (small particles - blue sky)
    Vec3 rayleighCoeff{5.8e-6f, 13.5e-6f, 33.1e-6f};  // RGB coefficients
    float rayleighScale = 8000.0f;      // Scale height
    
    // Mie scattering (larger particles - haze/sun glow)
    float mieCoeff = 21e-6f;
    float mieScale = 1200.0f;
    float mieAnisotropy = 0.758f;       // Phase function g
    
    // Sun
    Vec3 sunDirection{0.0f, 1.0f, 0.0f};
    Vec3 sunColor{1.0f, 1.0f, 1.0f};
    float sunIntensity = 20.0f;
    
    // Quality
    int viewSamples = 16;
    int lightSamples = 8;
};

// ===== Phase Functions =====
namespace PhaseFunction {

// Henyey-Greenstein phase function
// g: anisotropy parameter (-1 = back scatter, 0 = isotropic, 1 = forward scatter)
inline float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * 3.14159265f * std::pow(denom, 1.5f));
}

// Rayleigh phase function (for small particles)
inline float rayleigh(float cosTheta) {
    return (3.0f / (16.0f * 3.14159265f)) * (1.0f + cosTheta * cosTheta);
}

// Mie phase function (Cornette-Shanks approximation)
inline float mie(float cosTheta, float g) {
    float g2 = g * g;
    float num = 3.0f * (1.0f - g2) * (1.0f + cosTheta * cosTheta);
    float denom = 2.0f * (2.0f + g2) * std::pow(1.0f + g2 - 2.0f * g * cosTheta, 1.5f);
    return num / (8.0f * 3.14159265f * denom);
}

}  // namespace PhaseFunction

// ===== Volumetric Fog =====
class VolumetricFog {
public:
    VolumetricFogSettings settings;
    
    // Calculate fog density at a point
    float getDensity(const Vec3& worldPos) const {
        float heightDensity = std::exp(-(worldPos.y - settings.heightOffset) * settings.heightFalloff);
        return settings.density * heightDensity;
    }
    
    // Calculate in-scattering for a ray segment
    Vec3 calculateInScattering(
        const Vec3& rayStart,
        const Vec3& rayEnd,
        const Vec3& lightDir,
        const Vec3& lightColor
    ) const {
        Vec3 rayDir = (rayEnd - rayStart).normalized();
        float rayLength = (rayEnd - rayStart).length();
        float stepSize = rayLength / settings.steps;
        
        Vec3 inScattering(0, 0, 0);
        float transmittance = 1.0f;
        
        float cosTheta = rayDir.dot(lightDir);
        float phase = PhaseFunction::henyeyGreenstein(cosTheta, settings.anisotropy);
        
        for (int i = 0; i < settings.steps; i++) {
            float t = (i + 0.5f) / settings.steps;
            Vec3 samplePos = rayStart + rayDir * (t * rayLength);
            
            float density = getDensity(samplePos);
            float extinction = (settings.scattering + settings.absorption) * density;
            
            // Beer-Lambert law for segment transmittance
            float segmentTransmittance = std::exp(-extinction * stepSize);
            
            // In-scattering contribution
            Vec3 scatterColor = lightColor * settings.lightIntensity * phase +
                               settings.albedo * settings.ambientIntensity;
            
            float scatterFactor = settings.scattering * density * transmittance * 
                                  (1.0f - segmentTransmittance) / std::max(extinction, 0.0001f);
            Vec3 segmentScattering = scatterColor * scatterFactor;
            
            inScattering = inScattering + segmentScattering;
            transmittance *= segmentTransmittance;
            
            // Early termination
            if (transmittance < 0.01f) break;
        }
        
        return inScattering;
    }
    
    // Get final transmittance along ray
    float getTransmittance(const Vec3& rayStart, const Vec3& rayEnd) const {
        float rayLength = (rayEnd - rayStart).length();
        Vec3 rayDir = (rayEnd - rayStart).normalized();
        float stepSize = rayLength / settings.steps;
        
        float transmittance = 1.0f;
        
        for (int i = 0; i < settings.steps; i++) {
            float t = (i + 0.5f) / settings.steps;
            Vec3 samplePos = rayStart + rayDir * (t * rayLength);
            
            float density = getDensity(samplePos);
            float extinction = (settings.scattering + settings.absorption) * density;
            
            transmittance *= std::exp(-extinction * stepSize);
            
            if (transmittance < 0.001f) break;
        }
        
        return transmittance;
    }
};

// ===== God Rays =====
class GodRays {
public:
    GodRaySettings settings;
    
    // Compute screen-space light position
    void updateScreenPosition(const Mat4& viewProjection, int screenWidth, int screenHeight) {
        // Project light position to screen
        Vec3 pos = settings.lightPosition;
        
        // If light is directional (sun), use a far point in that direction
        float x = pos.x * viewProjection.m[0] + pos.y * viewProjection.m[4] + 
                  pos.z * viewProjection.m[8] + viewProjection.m[12];
        float y = pos.x * viewProjection.m[1] + pos.y * viewProjection.m[5] + 
                  pos.z * viewProjection.m[9] + viewProjection.m[13];
        float w = pos.x * viewProjection.m[3] + pos.y * viewProjection.m[7] + 
                  pos.z * viewProjection.m[11] + viewProjection.m[15];
        
        if (w > 0.0f) {
            settings.screenLightPos.x = (x / w) * 0.5f + 0.5f;
            settings.screenLightPos.y = (y / w) * 0.5f + 0.5f;
            settings.screenLightPos.z = 1.0f;  // Visible
        } else {
            settings.screenLightPos.z = 0.0f;  // Behind camera
        }
    }
};

// ===== Atmospheric Scattering =====
class AtmosphericScattering {
public:
    AtmosphereSettings settings;
    
    // Calculate optical depth along a ray
    float opticalDepthRayleigh(const Vec3& rayOrigin, const Vec3& rayDir, float rayLength) const {
        float opticalDepth = 0.0f;
        float stepSize = rayLength / settings.lightSamples;
        
        for (int i = 0; i < settings.lightSamples; i++) {
            float t = (i + 0.5f) * stepSize;
            Vec3 samplePos = rayOrigin + rayDir * t;
            float altitude = samplePos.length() - settings.planetRadius;
            
            opticalDepth += std::exp(-altitude / settings.rayleighScale) * stepSize;
        }
        
        return opticalDepth;
    }
    
    float opticalDepthMie(const Vec3& rayOrigin, const Vec3& rayDir, float rayLength) const {
        float opticalDepth = 0.0f;
        float stepSize = rayLength / settings.lightSamples;
        
        for (int i = 0; i < settings.lightSamples; i++) {
            float t = (i + 0.5f) * stepSize;
            Vec3 samplePos = rayOrigin + rayDir * t;
            float altitude = samplePos.length() - settings.planetRadius;
            
            opticalDepth += std::exp(-altitude / settings.mieScale) * stepSize;
        }
        
        return opticalDepth;
    }
    
    // Ray-sphere intersection
    bool raySphereIntersect(const Vec3& origin, const Vec3& dir, float radius,
                           float& t0, float& t1) const {
        float a = dir.dot(dir);
        float b = 2.0f * origin.dot(dir);
        float c = origin.dot(origin) - radius * radius;
        float discriminant = b * b - 4.0f * a * c;
        
        if (discriminant < 0.0f) return false;
        
        float sqrtD = std::sqrt(discriminant);
        t0 = (-b - sqrtD) / (2.0f * a);
        t1 = (-b + sqrtD) / (2.0f * a);
        
        return true;
    }
    
    // Calculate sky color for a view direction
    Vec3 calculateSkyColor(const Vec3& viewDir, const Vec3& cameraPos) const {
        Vec3 rayOrigin = cameraPos + Vec3(0, settings.planetRadius, 0);
        Vec3 rayDir = viewDir.normalized();
        
        float t0, t1;
        if (!raySphereIntersect(rayOrigin, rayDir, settings.atmosphereRadius, t0, t1)) {
            return Vec3(0, 0, 0);  // No atmosphere intersection
        }
        
        t0 = std::max(t0, 0.0f);
        float rayLength = t1 - t0;
        float stepSize = rayLength / settings.viewSamples;
        
        Vec3 totalRayleigh(0, 0, 0);
        Vec3 totalMie(0, 0, 0);
        float opticalDepthRayleighAcc = 0.0f;
        float opticalDepthMieAcc = 0.0f;
        
        float cosTheta = rayDir.dot(settings.sunDirection);
        float phaseR = PhaseFunction::rayleigh(cosTheta);
        float phaseM = PhaseFunction::mie(cosTheta, settings.mieAnisotropy);
        
        for (int i = 0; i < settings.viewSamples; i++) {
            float t = t0 + (i + 0.5f) * stepSize;
            Vec3 samplePos = rayOrigin + rayDir * t;
            float altitude = samplePos.length() - settings.planetRadius;
            
            // Local density
            float densityR = std::exp(-altitude / settings.rayleighScale);
            float densityM = std::exp(-altitude / settings.mieScale);
            
            opticalDepthRayleighAcc += densityR * stepSize;
            opticalDepthMieAcc += densityM * stepSize;
            
            // Light ray to sun
            float lt0, lt1;
            raySphereIntersect(samplePos, settings.sunDirection, settings.atmosphereRadius, lt0, lt1);
            float lightRayLength = lt1;
            
            float lightOpticalDepthR = opticalDepthRayleigh(samplePos, settings.sunDirection, lightRayLength);
            float lightOpticalDepthM = opticalDepthMie(samplePos, settings.sunDirection, lightRayLength);
            
            // Transmittance
            Vec3 tau = settings.rayleighCoeff * (opticalDepthRayleighAcc + lightOpticalDepthR) +
                       Vec3(settings.mieCoeff, settings.mieCoeff, settings.mieCoeff) * 1.1f * 
                       (opticalDepthMieAcc + lightOpticalDepthM);
            
            Vec3 transmittance(
                std::exp(-tau.x),
                std::exp(-tau.y),
                std::exp(-tau.z)
            );
            
            totalRayleigh = totalRayleigh + transmittance * densityR * stepSize;
            totalMie = totalMie + transmittance * densityM * stepSize;
        }
        
        Vec3 color(
            (totalRayleigh.x * settings.rayleighCoeff.x * phaseR + totalMie.x * settings.mieCoeff * phaseM) * settings.sunIntensity * settings.sunColor.x,
            (totalRayleigh.y * settings.rayleighCoeff.y * phaseR + totalMie.y * settings.mieCoeff * phaseM) * settings.sunIntensity * settings.sunColor.y,
            (totalRayleigh.z * settings.rayleighCoeff.z * phaseR + totalMie.z * settings.mieCoeff * phaseM) * settings.sunIntensity * settings.sunColor.z
        );
        
        return color;
    }
};

// ===== Volumetric Shaders =====
namespace VolumetricShaders {

inline const char* fogFragmentShader = R"(
struct FogUniforms {
    float4x4 invViewProjection;
    float3 cameraPosition;
    float3 lightDirection;
    float3 lightColor;
    float3 fogAlbedo;
    float density;
    float heightFalloff;
    float heightOffset;
    float scattering;
    float absorption;
    float anisotropy;
    float ambientIntensity;
    float lightIntensity;
    float maxDistance;
    int steps;
};

float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(denom, 1.5));
}

float getFogDensity(float3 pos, constant FogUniforms& u) {
    float heightDensity = exp(-(pos.y - u.heightOffset) * u.heightFalloff);
    return u.density * heightDensity;
}

fragment float4 volumetricFogFragment(
    VertexOut in [[stage_in]],
    texture2d<float> depthTexture [[texture(0)]],
    texture2d<float> sceneTexture [[texture(1)]],
    constant FogUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float4 sceneColor = sceneTexture.sample(s, in.texCoord);
    float depth = depthTexture.sample(s, in.texCoord).r;
    
    // Reconstruct world position
    float4 clipPos = float4(in.texCoord * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 worldPos = uniforms.invViewProjection * clipPos;
    worldPos /= worldPos.w;
    
    float3 rayStart = uniforms.cameraPosition;
    float3 rayEnd = worldPos.xyz;
    float3 rayDir = normalize(rayEnd - rayStart);
    float rayLength = min(length(rayEnd - rayStart), uniforms.maxDistance);
    
    float stepSize = rayLength / float(uniforms.steps);
    float3 inScattering = float3(0.0);
    float transmittance = 1.0;
    
    float cosTheta = dot(rayDir, uniforms.lightDirection);
    float phase = henyeyGreenstein(cosTheta, uniforms.anisotropy);
    
    for (int i = 0; i < uniforms.steps; i++) {
        float t = (float(i) + 0.5) / float(uniforms.steps);
        float3 samplePos = rayStart + rayDir * (t * rayLength);
        
        float density = getFogDensity(samplePos, uniforms);
        float extinction = (uniforms.scattering + uniforms.absorption) * density;
        
        float segmentTransmittance = exp(-extinction * stepSize);
        
        float3 scatterColor = uniforms.lightColor * uniforms.lightIntensity * phase +
                             uniforms.fogAlbedo * uniforms.ambientIntensity;
        
        float3 segmentScattering = scatterColor * uniforms.scattering * density *
                                   transmittance * (1.0 - segmentTransmittance) /
                                   max(extinction, 0.0001);
        
        inScattering += segmentScattering;
        transmittance *= segmentTransmittance;
        
        if (transmittance < 0.01) break;
    }
    
    float3 finalColor = sceneColor.rgb * transmittance + inScattering;
    return float4(finalColor, sceneColor.a);
}
)";

inline const char* godRayFragmentShader = R"(
struct GodRayUniforms {
    float2 lightScreenPos;
    float density;
    float weight;
    float decay;
    float exposure;
    int samples;
};

fragment float4 godRayFragment(
    VertexOut in [[stage_in]],
    texture2d<float> occlusionTexture [[texture(0)]],
    constant GodRayUniforms& uniforms [[buffer(0)]]
) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    
    float2 deltaTexCoord = (in.texCoord - uniforms.lightScreenPos) * uniforms.density / float(uniforms.samples);
    float2 texCoord = in.texCoord;
    float illuminationDecay = 1.0;
    float4 color = float4(0.0);
    
    for (int i = 0; i < uniforms.samples; i++) {
        texCoord -= deltaTexCoord;
        float4 sample = occlusionTexture.sample(s, texCoord);
        sample *= illuminationDecay * uniforms.weight;
        color += sample;
        illuminationDecay *= uniforms.decay;
    }
    
    return color * uniforms.exposure;
}
)";

}  // namespace VolumetricShaders

// ===== Presets =====
namespace VolumetricPresets {

inline VolumetricFogSettings lightFog() {
    VolumetricFogSettings s;
    s.density = 0.01f;
    s.heightFalloff = 0.05f;
    s.steps = 32;
    return s;
}

inline VolumetricFogSettings denseFog() {
    VolumetricFogSettings s;
    s.density = 0.05f;
    s.heightFalloff = 0.02f;
    s.steps = 64;
    return s;
}

inline VolumetricFogSettings groundFog() {
    VolumetricFogSettings s;
    s.density = 0.1f;
    s.heightFalloff = 0.2f;
    s.heightOffset = 0.0f;
    s.steps = 48;
    return s;
}

inline AtmosphereSettings earth() {
    return AtmosphereSettings();  // Default is Earth-like
}

inline AtmosphereSettings mars() {
    AtmosphereSettings s;
    s.rayleighCoeff = Vec3(19.918e-6f, 13.57e-6f, 5.75e-6f);  // More red/orange
    s.mieCoeff = 50e-6f;  // More dust
    s.sunIntensity = 15.0f;
    return s;
}

}  // namespace VolumetricPresets

}  // namespace luma
