// Spherical Harmonics - L2 (9 coefficients per channel)
// Used for encoding/decoding diffuse irradiance
#pragma once

#include "engine/foundation/math_types.h"
#include <cmath>
#include <array>

namespace luma {

// ===== SH Constants =====
// L2 spherical harmonics has 9 coefficients (0,0), (1,-1), (1,0), (1,1), (2,-2), (2,-1), (2,0), (2,1), (2,2)
constexpr int SH_COEFFICIENT_COUNT = 9;

// Pre-computed constants for SH
namespace SHConstants {
    // Normalization constants
    constexpr float kC0 = 0.282095f;     // Y_00
    constexpr float kC1 = 0.488603f;     // Y_1m
    constexpr float kC2 = 1.092548f;     // Y_2,-2, Y_2,2
    constexpr float kC3 = 0.315392f;     // Y_20
    constexpr float kC4 = 0.546274f;     // Y_2,-1, Y_2,1
    
    // For irradiance conversion (Ramamoorthi & Hanrahan)
    constexpr float kA0 = 3.141593f;     // π
    constexpr float kA1 = 2.094395f;     // 2π/3
    constexpr float kA2 = 0.785398f;     // π/4
    
    // Combined coefficients for irradiance
    constexpr float kIrr0 = kA0 * kC0;
    constexpr float kIrr1 = kA1 * kC1;
    constexpr float kIrr2_02 = kA2 * kC2;
    constexpr float kIrr2_20 = kA2 * kC3;
    constexpr float kIrr2_11 = kA2 * kC4;
}

// ===== SH Coefficients (RGB) =====
struct SHCoefficients {
    Vec3 coefficients[SH_COEFFICIENT_COUNT];
    
    SHCoefficients() {
        for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
            coefficients[i] = Vec3(0, 0, 0);
        }
    }
    
    // Add radiance sample
    void addSample(const Vec3& direction, const Vec3& radiance) {
        float basis[SH_COEFFICIENT_COUNT];
        evaluateBasis(direction, basis);
        
        for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
            coefficients[i] = coefficients[i] + radiance * basis[i];
        }
    }
    
    // Scale coefficients
    void scale(float s) {
        for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
            coefficients[i] = coefficients[i] * s;
        }
    }
    
    // Add another SH
    void add(const SHCoefficients& other) {
        for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
            coefficients[i] = coefficients[i] + other.coefficients[i];
        }
    }
    
    // Lerp between two SH
    static SHCoefficients lerp(const SHCoefficients& a, const SHCoefficients& b, float t) {
        SHCoefficients result;
        for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
            result.coefficients[i] = a.coefficients[i] * (1.0f - t) + b.coefficients[i] * t;
        }
        return result;
    }
    
    // Evaluate irradiance for a normal direction
    Vec3 evaluateIrradiance(const Vec3& normal) const {
        using namespace SHConstants;
        
        float x = normal.x;
        float y = normal.y;
        float z = normal.z;
        
        Vec3 irradiance(0, 0, 0);
        
        // L0
        irradiance = irradiance + coefficients[0] * kIrr0;
        
        // L1
        irradiance = irradiance + coefficients[1] * (kIrr1 * y);
        irradiance = irradiance + coefficients[2] * (kIrr1 * z);
        irradiance = irradiance + coefficients[3] * (kIrr1 * x);
        
        // L2
        irradiance = irradiance + coefficients[4] * (kIrr2_02 * x * y);
        irradiance = irradiance + coefficients[5] * (kIrr2_11 * y * z);
        irradiance = irradiance + coefficients[6] * (kIrr2_20 * (3.0f * z * z - 1.0f));
        irradiance = irradiance + coefficients[7] * (kIrr2_11 * x * z);
        irradiance = irradiance + coefficients[8] * (kIrr2_02 * (x * x - y * y));
        
        return irradiance;
    }
    
    // Evaluate SH basis functions for a direction
    static void evaluateBasis(const Vec3& dir, float* basis) {
        using namespace SHConstants;
        
        float x = dir.x;
        float y = dir.y;
        float z = dir.z;
        
        // L0
        basis[0] = kC0;
        
        // L1
        basis[1] = kC1 * y;
        basis[2] = kC1 * z;
        basis[3] = kC1 * x;
        
        // L2
        basis[4] = kC2 * x * y;
        basis[5] = kC4 * y * z;
        basis[6] = kC3 * (3.0f * z * z - 1.0f);
        basis[7] = kC4 * x * z;
        basis[8] = kC2 * 0.5f * (x * x - y * y);
    }
    
    // Create SH from a single directional light
    static SHCoefficients fromDirectionalLight(const Vec3& direction, const Vec3& color) {
        SHCoefficients sh;
        
        // Add samples in a cone around the light direction
        // For a directional light, we approximate with the main direction
        sh.addSample(direction, color);
        
        return sh;
    }
    
    // Create ambient SH (constant color from all directions)
    static SHCoefficients fromAmbient(const Vec3& color) {
        SHCoefficients sh;
        sh.coefficients[0] = color * (1.0f / SHConstants::kC0);
        return sh;
    }
    
    // Create sky gradient SH (sky color at top, ground color at bottom)
    static SHCoefficients fromSkyGradient(const Vec3& skyColor, const Vec3& groundColor) {
        SHCoefficients sh;
        
        Vec3 average = (skyColor + groundColor) * 0.5f;
        Vec3 diff = (skyColor - groundColor) * 0.5f;
        
        // L0 - average
        sh.coefficients[0] = average * (1.0f / SHConstants::kC0);
        
        // L1 - gradient (Y direction)
        sh.coefficients[2] = diff * (1.0f / SHConstants::kC1);
        
        return sh;
    }
};

// ===== SH Sample Generator =====
// Generates uniformly distributed sample directions for SH projection
class SHSampleGenerator {
public:
    struct Sample {
        Vec3 direction;
        float basis[SH_COEFFICIENT_COUNT];
        float solidAngle;
    };
    
    // Generate samples using spherical fibonacci
    static std::vector<Sample> generateSamples(int count) {
        std::vector<Sample> samples;
        samples.reserve(count);
        
        float goldenRatio = (1.0f + std::sqrt(5.0f)) / 2.0f;
        float angleIncrement = 2.0f * 3.14159265f / goldenRatio;
        
        float solidAngle = 4.0f * 3.14159265f / count;
        
        for (int i = 0; i < count; i++) {
            float t = (float)i / count;
            float inclination = std::acos(1.0f - 2.0f * t);
            float azimuth = angleIncrement * i;
            
            Sample sample;
            sample.direction.x = std::sin(inclination) * std::cos(azimuth);
            sample.direction.y = std::cos(inclination);
            sample.direction.z = std::sin(inclination) * std::sin(azimuth);
            sample.solidAngle = solidAngle;
            
            SHCoefficients::evaluateBasis(sample.direction, sample.basis);
            
            samples.push_back(sample);
        }
        
        return samples;
    }
};

// ===== SH GPU Data (for shader upload) =====
struct SHGPUData {
    float coefficients[SH_COEFFICIENT_COUNT * 4];  // Padded for GPU alignment
    
    void fromSHCoefficients(const SHCoefficients& sh) {
        for (int i = 0; i < SH_COEFFICIENT_COUNT; i++) {
            coefficients[i * 4 + 0] = sh.coefficients[i].x;
            coefficients[i * 4 + 1] = sh.coefficients[i].y;
            coefficients[i * 4 + 2] = sh.coefficients[i].z;
            coefficients[i * 4 + 3] = 0.0f;  // Padding
        }
    }
};

}  // namespace luma
