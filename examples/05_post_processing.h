// Example 05: Post-Processing Effects
// Demonstrates bloom, tone mapping, SSAO, and other effects
#pragma once

#include "engine/renderer/post_process.h"
#include "engine/rendering/ssao.h"
#include "engine/rendering/ssr.h"
#include "engine/rendering/volumetrics.h"
#include <iostream>

namespace luma {
namespace examples {

// ============================================
// Example 1: Basic Post-Processing Setup
// ============================================

inline void example_BasicPostProcess() {
    PostProcessSettings pp;
    
    // === Tone Mapping ===
    pp.toneMapping.enabled = true;
    pp.toneMapping.exposure = 1.0f;      // Brightness adjustment
    pp.toneMapping.gamma = 2.2f;         // Standard gamma
    pp.toneMapping.method = ToneMappingMethod::ACES;  // Film-like response
    
    // === Bloom ===
    pp.bloom.enabled = true;
    pp.bloom.threshold = 1.0f;           // Brightness threshold for bloom
    pp.bloom.intensity = 0.5f;           // Bloom strength
    pp.bloom.radius = 0.5f;              // Bloom spread
    
    // === Anti-Aliasing ===
    pp.fxaa.enabled = true;
    
    std::cout << "Post-Processing Configuration:\n";
    std::cout << "  Tone Mapping: " << (pp.toneMapping.enabled ? "ON" : "OFF") << "\n";
    std::cout << "    Exposure: " << pp.toneMapping.exposure << "\n";
    std::cout << "  Bloom: " << (pp.bloom.enabled ? "ON" : "OFF") << "\n";
    std::cout << "    Threshold: " << pp.bloom.threshold << "\n";
    std::cout << "    Intensity: " << pp.bloom.intensity << "\n";
    std::cout << "  FXAA: " << (pp.fxaa.enabled ? "ON" : "OFF") << "\n";
}

// ============================================
// Example 2: Tone Mapping Comparison
// ============================================

inline void example_ToneMappingMethods() {
    std::cout << "Tone Mapping Methods:\n\n";
    
    std::cout << "1. LINEAR (No tone mapping)\n";
    std::cout << "   - Direct HDR to LDR conversion\n";
    std::cout << "   - Can cause clipping in bright areas\n\n";
    
    std::cout << "2. REINHARD\n";
    std::cout << "   - Simple, natural-looking\n";
    std::cout << "   - Formula: color / (1 + color)\n";
    std::cout << "   - Preserves color ratios\n\n";
    
    std::cout << "3. ACES (Academy Color Encoding System)\n";
    std::cout << "   - Film industry standard\n";
    std::cout << "   - Rich contrast, film-like look\n";
    std::cout << "   - Slightly desaturates bright areas\n\n";
    
    std::cout << "4. UNCHARTED2 (Filmic)\n";
    std::cout << "   - Used in Uncharted 2 game\n";
    std::cout << "   - Good for games with high contrast\n";
    std::cout << "   - Toe and shoulder curve\n\n";
    
    // Example settings for each
    PostProcessSettings pp;
    
    // Reinhard for outdoor scenes
    pp.toneMapping.method = ToneMappingMethod::Reinhard;
    pp.toneMapping.exposure = 1.0f;
    
    // ACES for cinematic look
    pp.toneMapping.method = ToneMappingMethod::ACES;
    pp.toneMapping.exposure = 0.8f;  // Slightly darker for film look
}

// ============================================
// Example 3: Bloom Configuration
// ============================================

inline void example_BloomSettings() {
    PostProcessSettings pp;
    
    // === Subtle Bloom (Realistic) ===
    pp.bloom.enabled = true;
    pp.bloom.threshold = 1.5f;    // Only very bright pixels bloom
    pp.bloom.intensity = 0.3f;    // Subtle effect
    pp.bloom.radius = 0.3f;       // Tight glow
    
    std::cout << "Subtle Bloom (Realistic):\n";
    std::cout << "  Threshold: 1.5, Intensity: 0.3, Radius: 0.3\n\n";
    
    // === Strong Bloom (Dreamy) ===
    pp.bloom.threshold = 0.8f;    // More pixels contribute to bloom
    pp.bloom.intensity = 1.0f;    // Strong effect
    pp.bloom.radius = 0.8f;       // Wide spread
    
    std::cout << "Strong Bloom (Dreamy):\n";
    std::cout << "  Threshold: 0.8, Intensity: 1.0, Radius: 0.8\n\n";
    
    // === Sci-Fi Bloom ===
    pp.bloom.threshold = 1.0f;
    pp.bloom.intensity = 0.7f;
    pp.bloom.radius = 0.5f;
    // Would also enable chromatic aberration for full effect
    
    std::cout << "Sci-Fi Bloom:\n";
    std::cout << "  Threshold: 1.0, Intensity: 0.7, Radius: 0.5\n";
}

// ============================================
// Example 4: SSAO Configuration
// ============================================

inline void example_SSAOSettings() {
    SSAOEffect ssao;
    
    // === Default Quality ===
    ssao.settings = SSAOPresets::medium();
    std::cout << "Medium SSAO:\n";
    std::cout << "  Samples: " << ssao.settings.sampleCount << "\n";
    std::cout << "  Radius: " << ssao.settings.radius << "\n";
    std::cout << "  Intensity: " << ssao.settings.intensity << "\n";
    std::cout << "  Half Resolution: " << (ssao.settings.halfResolution ? "Yes" : "No") << "\n\n";
    
    // === High Quality ===
    ssao.settings = SSAOPresets::high();
    std::cout << "High SSAO:\n";
    std::cout << "  Samples: " << ssao.settings.sampleCount << "\n";
    std::cout << "  Half Resolution: " << (ssao.settings.halfResolution ? "Yes" : "No") << "\n\n";
    
    // === Custom Settings ===
    ssao.settings.sampleCount = 48;
    ssao.settings.radius = 0.75f;        // Larger radius for bigger occlusion
    ssao.settings.bias = 0.02f;          // Reduce self-occlusion
    ssao.settings.intensity = 1.5f;      // Stronger effect
    ssao.settings.power = 2.5f;          // More contrast
    ssao.settings.enableBlur = true;
    ssao.settings.blurPasses = 2;
    
    std::cout << "Custom SSAO:\n";
    std::cout << "  Samples: " << ssao.settings.sampleCount << "\n";
    std::cout << "  Radius: " << ssao.settings.radius << "\n";
    std::cout << "  Intensity: " << ssao.settings.intensity << "\n";
}

// ============================================
// Example 5: Screen Space Reflections
// ============================================

inline void example_SSRSettings() {
    SSRSettings ssr;
    
    // === Performance Settings ===
    ssr = SSRPresets::low();
    std::cout << "Low Quality SSR:\n";
    std::cout << "  Max Steps: " << ssr.maxSteps << "\n";
    std::cout << "  Half Resolution: " << (ssr.halfResolution ? "Yes" : "No") << "\n\n";
    
    // === Quality Settings ===
    ssr = SSRPresets::high();
    std::cout << "High Quality SSR:\n";
    std::cout << "  Max Steps: " << ssr.maxSteps << "\n";
    std::cout << "  Binary Search Steps: " << ssr.binarySearchSteps << "\n";
    std::cout << "  Half Resolution: " << (ssr.halfResolution ? "Yes" : "No") << "\n\n";
    
    // === Custom for Wet Surfaces ===
    ssr.maxSteps = 128;
    ssr.thickness = 0.3f;           // Thinner comparison for sharper reflections
    ssr.maxDistance = 50.0f;        // Shorter range
    ssr.roughnessThreshold = 0.3f;  // Only very smooth surfaces
    ssr.fadeStart = 0.7f;           // Start fading earlier
    
    std::cout << "Wet Surface SSR:\n";
    std::cout << "  Max Steps: " << ssr.maxSteps << "\n";
    std::cout << "  Roughness Threshold: " << ssr.roughnessThreshold << "\n";
}

// ============================================
// Example 6: Volumetric Effects
// ============================================

inline void example_VolumetricEffects() {
    // === Light Fog ===
    VolumetricFog lightFog;
    lightFog.settings = VolumetricPresets::lightFog();
    std::cout << "Light Fog:\n";
    std::cout << "  Density: " << lightFog.settings.density << "\n";
    std::cout << "  Steps: " << lightFog.settings.steps << "\n\n";
    
    // === Dense Fog ===
    VolumetricFog denseFog;
    denseFog.settings = VolumetricPresets::denseFog();
    std::cout << "Dense Fog:\n";
    std::cout << "  Density: " << denseFog.settings.density << "\n\n";
    
    // === Ground Fog (Low-lying) ===
    VolumetricFog groundFog;
    groundFog.settings = VolumetricPresets::groundFog();
    std::cout << "Ground Fog:\n";
    std::cout << "  Density: " << groundFog.settings.density << "\n";
    std::cout << "  Height Falloff: " << groundFog.settings.heightFalloff << "\n\n";
    
    // === Custom Underwater ===
    VolumetricFogSettings underwater;
    underwater.density = 0.1f;
    underwater.albedo = Vec3(0.2f, 0.4f, 0.5f);  // Blue-green tint
    underwater.scattering = 0.8f;
    underwater.absorption = 0.2f;
    underwater.anisotropy = 0.3f;
    underwater.maxDistance = 50.0f;
    
    std::cout << "Underwater Fog:\n";
    std::cout << "  Color: Blue-green\n";
    std::cout << "  High scattering, low absorption\n";
}

// ============================================
// Example 7: God Rays
// ============================================

inline void example_GodRays() {
    GodRaySettings godRays;
    
    // Configure for sun through clouds
    godRays.lightPosition = Vec3(100, 50, 100);  // Far sun position
    godRays.lightColor = Vec3(1.0f, 0.95f, 0.8f);
    
    godRays.samples = 100;
    godRays.density = 1.0f;
    godRays.weight = 0.01f;   // Per-sample contribution
    godRays.decay = 0.97f;    // How quickly rays fade
    godRays.exposure = 1.0f;
    
    std::cout << "God Rays (Sun through clouds):\n";
    std::cout << "  Samples: " << godRays.samples << "\n";
    std::cout << "  Decay: " << godRays.decay << "\n";
    std::cout << "  Exposure: " << godRays.exposure << "\n";
}

// ============================================
// Example 8: Atmospheric Scattering
// ============================================

inline void example_Atmosphere() {
    // === Earth Atmosphere ===
    AtmosphericScattering earthAtmo;
    earthAtmo.settings = VolumetricPresets::earth();
    std::cout << "Earth Atmosphere:\n";
    std::cout << "  Planet Radius: " << earthAtmo.settings.planetRadius << "m\n";
    std::cout << "  Atmosphere Height: " << (earthAtmo.settings.atmosphereRadius - earthAtmo.settings.planetRadius) << "m\n\n";
    
    // === Mars Atmosphere ===
    AtmosphericScattering marsAtmo;
    marsAtmo.settings = VolumetricPresets::mars();
    std::cout << "Mars Atmosphere:\n";
    std::cout << "  More Mie scattering (dusty)\n";
    std::cout << "  Orange-red sky color\n\n";
    
    // Calculate sky color for a view direction
    Vec3 viewDir(0, 0.5f, 1);  // Looking at horizon
    Vec3 cameraPos(0, 1.8f, 0); // Human eye height
    
    // In real usage:
    // Vec3 skyColor = earthAtmo.calculateSkyColor(viewDir, cameraPos);
    std::cout << "Sky color calculated based on view direction and sun position\n";
}

// ============================================
// Example 9: Complete Post-Process Stack
// ============================================

inline void example_FullPostProcessStack() {
    std::cout << "=== Recommended Post-Process Stack ===\n\n";
    
    std::cout << "1. SSAO (Ambient Occlusion)\n";
    std::cout << "   - Adds depth and grounding to scene\n";
    std::cout << "   - Apply before lighting\n\n";
    
    std::cout << "2. SSR (Screen Space Reflections)\n";
    std::cout << "   - Adds reflections to smooth surfaces\n";
    std::cout << "   - Blend with IBL fallback\n\n";
    
    std::cout << "3. Volumetric Fog / God Rays\n";
    std::cout << "   - Atmospheric depth\n";
    std::cout << "   - Apply after lighting\n\n";
    
    std::cout << "4. Bloom\n";
    std::cout << "   - Bright area glow\n";
    std::cout << "   - After HDR lighting\n\n";
    
    std::cout << "5. Tone Mapping\n";
    std::cout << "   - HDR to LDR conversion\n";
    std::cout << "   - Apply near end of pipeline\n\n";
    
    std::cout << "6. Color Grading (if available)\n";
    std::cout << "   - Artistic color adjustment\n";
    std::cout << "   - After tone mapping\n\n";
    
    std::cout << "7. FXAA (Anti-aliasing)\n";
    std::cout << "   - Smooth jagged edges\n";
    std::cout << "   - Apply last\n";
}

}  // namespace examples
}  // namespace luma
