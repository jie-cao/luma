// Post-Processing System
// Screen-space effects framework
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <cmath>

namespace luma {

// ===== Post-Process Effect Settings =====

// Bloom settings
struct BloomSettings {
    bool enabled = true;
    float threshold = 1.0f;       // Brightness threshold for bloom
    float intensity = 1.0f;       // Bloom intensity
    float radius = 4.0f;          // Blur radius
    int iterations = 5;           // Number of blur iterations
    float softThreshold = 0.5f;   // Soft threshold knee
};

// Tone Mapping settings
struct ToneMappingSettings {
    bool enabled = true;
    
    enum class Mode {
        None,
        Reinhard,
        ACES,
        Filmic,
        Uncharted2
    } mode = Mode::ACES;
    
    float exposure = 1.0f;        // Exposure adjustment
    float gamma = 2.2f;           // Gamma correction
    float contrast = 1.0f;        // Contrast adjustment
    float saturation = 1.0f;      // Saturation adjustment
};

// Color Grading settings
struct ColorGradingSettings {
    bool enabled = true;
    
    // Color balance (shadows, midtones, highlights)
    float shadowsR = 0.0f, shadowsG = 0.0f, shadowsB = 0.0f;
    float midtonesR = 0.0f, midtonesG = 0.0f, midtonesB = 0.0f;
    float highlightsR = 0.0f, highlightsG = 0.0f, highlightsB = 0.0f;
    
    // Lift, Gamma, Gain
    float lift[3] = {0.0f, 0.0f, 0.0f};
    float gamma_adj[3] = {1.0f, 1.0f, 1.0f};
    float gain[3] = {1.0f, 1.0f, 1.0f};
    
    // Temperature and tint
    float temperature = 0.0f;     // -1 to 1 (cool to warm)
    float tint = 0.0f;            // -1 to 1 (green to magenta)
};

// Vignette settings
struct VignetteSettings {
    bool enabled = false;
    float intensity = 0.3f;       // Vignette strength
    float smoothness = 0.5f;      // Falloff smoothness
    float roundness = 1.0f;       // Shape (1 = circular, 0 = square)
    float centerX = 0.5f;         // Center X (0-1)
    float centerY = 0.5f;         // Center Y (0-1)
};

// Chromatic Aberration settings
struct ChromaticAberrationSettings {
    bool enabled = false;
    float intensity = 0.01f;      // Aberration strength
};

// Film Grain settings
struct FilmGrainSettings {
    bool enabled = false;
    float intensity = 0.1f;       // Grain intensity
    float response = 0.8f;        // Luminance response
};

// FXAA settings
struct FXAASettings {
    bool enabled = true;
    float contrastThreshold = 0.0312f;    // Minimum contrast to apply AA
    float relativeThreshold = 0.063f;     // Relative threshold
    float subpixelBlending = 0.75f;       // Subpixel blending amount
};

// ===== Post-Process Stack =====
struct PostProcessSettings {
    BloomSettings bloom;
    ToneMappingSettings toneMapping;
    ColorGradingSettings colorGrading;
    VignetteSettings vignette;
    ChromaticAberrationSettings chromaticAberration;
    FilmGrainSettings filmGrain;
    FXAASettings fxaa;
};

// ===== Post-Process Constants (for GPU) =====
struct alignas(16) PostProcessConstants {
    // Bloom
    float bloomThreshold;
    float bloomIntensity;
    float bloomRadius;
    float bloomSoftThreshold;
    
    // Tone mapping
    float exposure;
    float gamma;
    float contrast;
    float saturation;
    
    // Color grading
    float lift[4];           // RGB + padding
    float gamma_adj[4];      // RGB + padding
    float gain[4];           // RGB + padding
    float temperature;
    float tint;
    float padding1[2];
    
    // Vignette
    float vignetteIntensity;
    float vignetteSmoothness;
    float vignetteRoundness;
    float vignettePadding;
    float vignetteCenter[4]; // XY + padding
    
    // Chromatic aberration
    float chromaIntensity;
    float padding2[3];
    
    // Film grain
    float grainIntensity;
    float grainResponse;
    float grainTime;         // For animation
    float padding3;
    
    // FXAA
    float fxaaContrastThreshold;
    float fxaaRelativeThreshold;
    float fxaaSubpixelBlending;
    float padding4;
    
    // Screen info
    float screenWidth;
    float screenHeight;
    float invScreenWidth;
    float invScreenHeight;
    
    // Flags
    uint32_t enabledEffects;  // Bit flags for enabled effects
    uint32_t toneMappingMode;
    float padding5[2];
};

// Effect enable flags
constexpr uint32_t PP_BLOOM = 1 << 0;
constexpr uint32_t PP_TONEMAPPING = 1 << 1;
constexpr uint32_t PP_COLORGRADING = 1 << 2;
constexpr uint32_t PP_VIGNETTE = 1 << 3;
constexpr uint32_t PP_CHROMATIC = 1 << 4;
constexpr uint32_t PP_FILMGRAIN = 1 << 5;
constexpr uint32_t PP_FXAA = 1 << 6;

// Fill constants from settings
inline void fillPostProcessConstants(PostProcessConstants& c, const PostProcessSettings& s, 
                                      uint32_t width, uint32_t height, float time) {
    // Bloom
    c.bloomThreshold = s.bloom.threshold;
    c.bloomIntensity = s.bloom.intensity;
    c.bloomRadius = s.bloom.radius;
    c.bloomSoftThreshold = s.bloom.softThreshold;
    
    // Tone mapping
    c.exposure = s.toneMapping.exposure;
    c.gamma = s.toneMapping.gamma;
    c.contrast = s.toneMapping.contrast;
    c.saturation = s.toneMapping.saturation;
    
    // Color grading
    c.lift[0] = s.colorGrading.lift[0];
    c.lift[1] = s.colorGrading.lift[1];
    c.lift[2] = s.colorGrading.lift[2];
    c.lift[3] = 0.0f;
    c.gamma_adj[0] = s.colorGrading.gamma_adj[0];
    c.gamma_adj[1] = s.colorGrading.gamma_adj[1];
    c.gamma_adj[2] = s.colorGrading.gamma_adj[2];
    c.gamma_adj[3] = 0.0f;
    c.gain[0] = s.colorGrading.gain[0];
    c.gain[1] = s.colorGrading.gain[1];
    c.gain[2] = s.colorGrading.gain[2];
    c.gain[3] = 0.0f;
    c.temperature = s.colorGrading.temperature;
    c.tint = s.colorGrading.tint;
    
    // Vignette
    c.vignetteIntensity = s.vignette.intensity;
    c.vignetteSmoothness = s.vignette.smoothness;
    c.vignetteRoundness = s.vignette.roundness;
    c.vignetteCenter[0] = s.vignette.centerX;
    c.vignetteCenter[1] = s.vignette.centerY;
    
    // Chromatic aberration
    c.chromaIntensity = s.chromaticAberration.intensity;
    
    // Film grain
    c.grainIntensity = s.filmGrain.intensity;
    c.grainResponse = s.filmGrain.response;
    c.grainTime = time;
    
    // FXAA
    c.fxaaContrastThreshold = s.fxaa.contrastThreshold;
    c.fxaaRelativeThreshold = s.fxaa.relativeThreshold;
    c.fxaaSubpixelBlending = s.fxaa.subpixelBlending;
    
    // Screen info
    c.screenWidth = (float)width;
    c.screenHeight = (float)height;
    c.invScreenWidth = 1.0f / (float)width;
    c.invScreenHeight = 1.0f / (float)height;
    
    // Flags
    c.enabledEffects = 0;
    if (s.bloom.enabled) c.enabledEffects |= PP_BLOOM;
    if (s.toneMapping.enabled) c.enabledEffects |= PP_TONEMAPPING;
    if (s.colorGrading.enabled) c.enabledEffects |= PP_COLORGRADING;
    if (s.vignette.enabled) c.enabledEffects |= PP_VIGNETTE;
    if (s.chromaticAberration.enabled) c.enabledEffects |= PP_CHROMATIC;
    if (s.filmGrain.enabled) c.enabledEffects |= PP_FILMGRAIN;
    if (s.fxaa.enabled) c.enabledEffects |= PP_FXAA;
    
    c.toneMappingMode = (uint32_t)s.toneMapping.mode;
}

}  // namespace luma
