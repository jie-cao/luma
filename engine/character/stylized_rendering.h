// Stylized Rendering System - Anime/Cartoon style rendering for characters
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <cmath>

namespace luma {

// ============================================================================
// Rendering Style
// ============================================================================

enum class RenderingStyle {
    Realistic,      // Standard PBR
    Anime,          // Anime/manga cel-shading
    Cartoon,        // Western cartoon style
    Painterly,      // Painterly/artistic
    Pixel,          // Pixel art style
    Sketch          // Pencil sketch effect
};

inline const char* getStyleName(RenderingStyle style) {
    switch (style) {
        case RenderingStyle::Realistic: return "Realistic";
        case RenderingStyle::Anime: return "Anime";
        case RenderingStyle::Cartoon: return "Cartoon";
        case RenderingStyle::Painterly: return "Painterly";
        case RenderingStyle::Pixel: return "Pixel Art";
        case RenderingStyle::Sketch: return "Sketch";
    }
    return "Unknown";
}

// ============================================================================
// Cel Shading / Toon Shading Parameters
// ============================================================================

struct CelShadingParams {
    // Number of shading bands (2-5 typical)
    int shadingBands = 3;
    
    // Thresholds for each band (normalized 0-1)
    // For 3 bands: [dark threshold, mid threshold]
    float bandThresholds[4] = {0.2f, 0.5f, 0.8f, 1.0f};
    
    // Colors for each band (multiplied with base color)
    float bandIntensities[5] = {0.3f, 0.6f, 0.85f, 1.0f, 1.0f};
    
    // Specular highlight
    bool enableSpecular = true;
    float specularSize = 0.95f;        // Higher = smaller highlight
    float specularIntensity = 0.8f;
    
    // Fresnel/rim lighting
    bool enableRimLight = true;
    float rimPower = 3.0f;
    float rimIntensity = 0.4f;
    Vec3 rimColor{1.0f, 1.0f, 1.0f};
    
    // Color adjustments
    float saturationBoost = 1.1f;
    float shadowHue = 0.0f;     // Hue shift for shadows (0 = no shift, >0 = warmer, <0 = cooler)
    
    // Apply cel shading to a diffuse value
    float applyBands(float diffuse) const {
        for (int i = 0; i < shadingBands; i++) {
            if (diffuse < bandThresholds[i]) {
                return bandIntensities[i];
            }
        }
        return bandIntensities[shadingBands - 1];
    }
};

// ============================================================================
// Outline Parameters
// ============================================================================

struct OutlineParams {
    bool enabled = true;
    
    // Outline method
    enum class Method {
        BackFaceExpansion,    // Expand back faces along normals
        PostProcess,          // Screen-space edge detection
        Both                  // Combined for best quality
    };
    Method method = Method::BackFaceExpansion;
    
    // Back-face expansion settings
    float thickness = 0.003f;    // World-space thickness
    float minThickness = 1.0f;   // Minimum screen-space pixels
    float maxThickness = 5.0f;   // Maximum screen-space pixels
    
    // Distance-based scaling
    bool scaleWithDistance = true;
    float referenceDistance = 5.0f;
    
    // Color
    Vec3 color{0.1f, 0.1f, 0.15f};
    float colorIntensity = 1.0f;
    
    // Color from surface
    bool deriveColorFromSurface = false;
    float surfaceColorDarkening = 0.3f;
    
    // Edge detection settings (for post-process)
    float depthThreshold = 0.01f;
    float normalThreshold = 0.5f;
    float colorThreshold = 0.1f;
};

// ============================================================================
// Anime-specific Features
// ============================================================================

struct AnimeFeatures {
    // Hair highlight (specular band)
    bool hairHighlight = true;
    float hairHighlightWidth = 0.2f;
    float hairHighlightIntensity = 0.6f;
    Vec3 hairHighlightColor{1.0f, 1.0f, 1.0f};
    
    // Eye highlight
    bool eyeHighlight = true;
    float eyeHighlightSize = 0.15f;
    Vec3 eyeHighlightColor{1.0f, 1.0f, 1.0f};
    
    // Blush effect
    bool blush = false;
    float blushIntensity = 0.3f;
    Vec3 blushColor{1.0f, 0.6f, 0.6f};
    
    // Shadow color shift (anime shadows often have a color tint)
    Vec3 shadowTint{0.9f, 0.85f, 1.0f};  // Slightly purple
    
    // Gradient mapping for skin
    bool skinGradient = true;
    Vec3 skinHighlightColor{1.0f, 0.95f, 0.9f};
    Vec3 skinShadowColor{0.9f, 0.75f, 0.75f};
};

// ============================================================================
// Stylized Rendering Settings
// ============================================================================

struct StylizedRenderingSettings {
    RenderingStyle style = RenderingStyle::Realistic;
    
    // Main parameters
    CelShadingParams celShading;
    OutlineParams outline;
    AnimeFeatures anime;
    
    // Global adjustments
    float stylizationStrength = 1.0f;   // 0 = realistic, 1 = full stylization
    float colorVibrancy = 1.0f;         // Overall color saturation
    float contrastBoost = 1.0f;
    
    // Apply preset for common styles
    void applyPreset(RenderingStyle preset) {
        style = preset;
        
        switch (preset) {
            case RenderingStyle::Anime:
                celShading.shadingBands = 3;
                celShading.bandThresholds[0] = 0.3f;
                celShading.bandThresholds[1] = 0.6f;
                celShading.bandIntensities[0] = 0.4f;
                celShading.bandIntensities[1] = 0.75f;
                celShading.bandIntensities[2] = 1.0f;
                celShading.enableRimLight = true;
                celShading.rimPower = 2.5f;
                celShading.rimIntensity = 0.5f;
                
                outline.enabled = true;
                outline.thickness = 0.004f;
                outline.color = Vec3(0.1f, 0.1f, 0.15f);
                
                anime.hairHighlight = true;
                anime.eyeHighlight = true;
                anime.skinGradient = true;
                stylizationStrength = 1.0f;
                colorVibrancy = 1.15f;
                break;
                
            case RenderingStyle::Cartoon:
                celShading.shadingBands = 2;
                celShading.bandThresholds[0] = 0.45f;
                celShading.bandIntensities[0] = 0.5f;
                celShading.bandIntensities[1] = 1.0f;
                celShading.enableRimLight = false;
                celShading.enableSpecular = true;
                celShading.specularSize = 0.9f;
                
                outline.enabled = true;
                outline.thickness = 0.006f;
                outline.color = Vec3(0.05f, 0.05f, 0.1f);
                
                anime.hairHighlight = false;
                anime.skinGradient = false;
                stylizationStrength = 1.0f;
                colorVibrancy = 1.3f;
                contrastBoost = 1.1f;
                break;
                
            case RenderingStyle::Painterly:
                celShading.shadingBands = 5;
                celShading.bandThresholds[0] = 0.15f;
                celShading.bandThresholds[1] = 0.35f;
                celShading.bandThresholds[2] = 0.55f;
                celShading.bandThresholds[3] = 0.75f;
                celShading.enableRimLight = true;
                celShading.rimPower = 4.0f;
                celShading.saturationBoost = 1.2f;
                
                outline.enabled = false;
                
                stylizationStrength = 0.8f;
                colorVibrancy = 1.2f;
                break;
                
            case RenderingStyle::Sketch:
                celShading.shadingBands = 2;
                celShading.enableSpecular = false;
                celShading.enableRimLight = false;
                
                outline.enabled = true;
                outline.thickness = 0.002f;
                outline.color = Vec3(0.2f, 0.2f, 0.2f);
                outline.method = OutlineParams::Method::Both;
                
                stylizationStrength = 1.0f;
                colorVibrancy = 0.3f;  // Desaturated
                break;
                
            case RenderingStyle::Realistic:
            default:
                // Reset to defaults
                celShading = CelShadingParams();
                celShading.shadingBands = 1;  // No banding
                outline.enabled = false;
                anime = AnimeFeatures();
                anime.hairHighlight = false;
                anime.eyeHighlight = false;
                anime.skinGradient = false;
                stylizationStrength = 0.0f;
                colorVibrancy = 1.0f;
                contrastBoost = 1.0f;
                break;
        }
    }
};

// ============================================================================
// Stylized Shader Data (for GPU upload)
// ============================================================================

struct StylizedShaderConstants {
    // Cel shading
    float bandThresholds[4];
    float bandIntensities[4];
    int shadingBands;
    float specularSize;
    float specularIntensity;
    float padding1;
    
    // Rim light
    float rimPower;
    float rimIntensity;
    float rimColor[3];
    float padding2;
    
    // Outline
    float outlineThickness;
    float outlineColor[3];
    
    // Style
    float stylizationStrength;
    float colorVibrancy;
    float contrastBoost;
    float padding3;
    
    // Anime features
    float hairHighlightWidth;
    float hairHighlightIntensity;
    float shadowTint[3];
    float padding4;
    
    // Fill from settings
    void fillFromSettings(const StylizedRenderingSettings& settings) {
        // Cel shading
        for (int i = 0; i < 4; i++) {
            bandThresholds[i] = settings.celShading.bandThresholds[i];
            bandIntensities[i] = settings.celShading.bandIntensities[i];
        }
        shadingBands = settings.celShading.shadingBands;
        specularSize = settings.celShading.specularSize;
        specularIntensity = settings.celShading.specularIntensity;
        
        // Rim light
        rimPower = settings.celShading.rimPower;
        rimIntensity = settings.celShading.enableRimLight ? settings.celShading.rimIntensity : 0.0f;
        rimColor[0] = settings.celShading.rimColor.x;
        rimColor[1] = settings.celShading.rimColor.y;
        rimColor[2] = settings.celShading.rimColor.z;
        
        // Outline
        outlineThickness = settings.outline.enabled ? settings.outline.thickness : 0.0f;
        outlineColor[0] = settings.outline.color.x;
        outlineColor[1] = settings.outline.color.y;
        outlineColor[2] = settings.outline.color.z;
        
        // Style
        stylizationStrength = settings.stylizationStrength;
        colorVibrancy = settings.colorVibrancy;
        contrastBoost = settings.contrastBoost;
        
        // Anime
        hairHighlightWidth = settings.anime.hairHighlight ? settings.anime.hairHighlightWidth : 0.0f;
        hairHighlightIntensity = settings.anime.hairHighlightIntensity;
        shadowTint[0] = settings.anime.shadowTint.x;
        shadowTint[1] = settings.anime.shadowTint.y;
        shadowTint[2] = settings.anime.shadowTint.z;
    }
};

// ============================================================================
// Stylized Rendering Manager
// ============================================================================

class StylizedRenderingManager {
public:
    static StylizedRenderingManager& getInstance() {
        static StylizedRenderingManager instance;
        return instance;
    }
    
    // === Settings ===
    
    StylizedRenderingSettings& getSettings() { return settings_; }
    const StylizedRenderingSettings& getSettings() const { return settings_; }
    
    void setStyle(RenderingStyle style) {
        settings_.applyPreset(style);
    }
    
    RenderingStyle getStyle() const {
        return settings_.style;
    }
    
    // === Shader Constants ===
    
    StylizedShaderConstants getShaderConstants() const {
        StylizedShaderConstants constants;
        constants.fillFromSettings(settings_);
        return constants;
    }
    
    // === CPU Shading (for preview/testing) ===
    
    // Apply stylized shading to a color
    Vec3 applyShading(const Vec3& baseColor, float nDotL, float nDotV, float nDotH) const {
        if (settings_.stylizationStrength < 0.01f) {
            // Realistic - just return with basic diffuse
            return baseColor * std::max(0.2f, nDotL);
        }
        
        // Cel shading
        float shadedIntensity = settings_.celShading.applyBands(nDotL * 0.5f + 0.5f);
        
        // Apply shadow tint
        Vec3 shadowColor = baseColor * Vec3(
            settings_.anime.shadowTint.x,
            settings_.anime.shadowTint.y,
            settings_.anime.shadowTint.z
        );
        
        // Blend between shadow and lit based on intensity
        Vec3 result = Vec3::lerp(shadowColor, baseColor, shadedIntensity);
        
        // Specular
        if (settings_.celShading.enableSpecular && nDotH > settings_.celShading.specularSize) {
            result = result + Vec3(1.0f, 1.0f, 1.0f) * settings_.celShading.specularIntensity;
        }
        
        // Rim light
        if (settings_.celShading.enableRimLight) {
            float rim = std::pow(1.0f - nDotV, settings_.celShading.rimPower);
            result = result + settings_.celShading.rimColor * rim * settings_.celShading.rimIntensity;
        }
        
        // Apply vibrancy
        if (settings_.colorVibrancy != 1.0f) {
            float gray = result.x * 0.299f + result.y * 0.587f + result.z * 0.114f;
            result = Vec3::lerp(Vec3(gray, gray, gray), result, settings_.colorVibrancy);
        }
        
        return result;
    }
    
private:
    StylizedRenderingManager() = default;
    
    StylizedRenderingSettings settings_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline StylizedRenderingManager& getStylizedRenderer() {
    return StylizedRenderingManager::getInstance();
}

} // namespace luma
