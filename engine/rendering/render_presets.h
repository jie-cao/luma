// Render Presets - Quality settings and high-quality output
// One-click rendering quality and final output configuration
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <functional>

namespace luma {

// ============================================================================
// Quality Levels
// ============================================================================

enum class RenderQuality {
    Preview,        // Fast preview (低质量快速预览)
    Draft,          // Draft quality (草稿)
    Standard,       // Standard quality (标准)
    High,           // High quality (高质量)
    Ultra,          // Ultra/cinematic (超高质量)
    Custom          // User-defined (自定义)
};

inline std::string renderQualityToString(RenderQuality q) {
    switch (q) {
        case RenderQuality::Preview: return "Preview";
        case RenderQuality::Draft: return "Draft";
        case RenderQuality::Standard: return "Standard";
        case RenderQuality::High: return "High";
        case RenderQuality::Ultra: return "Ultra";
        case RenderQuality::Custom: return "Custom";
        default: return "Unknown";
    }
}

inline std::string renderQualityToStringCN(RenderQuality q) {
    switch (q) {
        case RenderQuality::Preview: return "预览";
        case RenderQuality::Draft: return "草稿";
        case RenderQuality::Standard: return "标准";
        case RenderQuality::High: return "高质量";
        case RenderQuality::Ultra: return "超高";
        case RenderQuality::Custom: return "自定义";
        default: return "未知";
    }
}

// ============================================================================
// Render Settings
// ============================================================================

struct RenderSettings {
    // === Resolution ===
    int width = 1920;
    int height = 1080;
    float renderScale = 1.0f;      // Internal resolution multiplier
    
    // === Anti-aliasing ===
    enum class AAMethod { None, FXAA, SMAA, TAA, MSAA4x, MSAA8x };
    AAMethod antiAliasing = AAMethod::TAA;
    float taaSharpness = 0.5f;
    
    // === Shadows ===
    bool shadowsEnabled = true;
    int shadowMapSize = 2048;       // 512, 1024, 2048, 4096
    int shadowCascades = 4;         // 1-4
    float shadowDistance = 100.0f;
    float shadowBias = 0.001f;
    bool softShadows = true;
    int pcfSamples = 16;            // PCF filter samples
    
    // === Ambient Occlusion ===
    bool aoEnabled = true;
    enum class AOMethod { None, SSAO, HBAO, GTAO };
    AOMethod aoMethod = AOMethod::SSAO;
    float aoRadius = 0.5f;
    float aoIntensity = 1.0f;
    int aoSamples = 16;
    
    // === Screen Space Reflections ===
    bool ssrEnabled = true;
    int ssrMaxSteps = 64;
    float ssrThickness = 0.1f;
    float ssrMaxDistance = 100.0f;
    
    // === Global Illumination ===
    bool giEnabled = false;
    int giSamples = 32;
    int giBounces = 2;
    
    // === Volumetrics ===
    bool volumetricsEnabled = false;
    int volumetricSamples = 32;
    float volumetricDensity = 0.01f;
    
    // === Post Processing ===
    bool bloomEnabled = true;
    float bloomIntensity = 0.3f;
    float bloomThreshold = 1.0f;
    
    bool dofEnabled = false;
    float dofFocusDistance = 5.0f;
    float dofAperture = 2.8f;
    
    bool motionBlurEnabled = false;
    float motionBlurAmount = 0.5f;
    
    bool chromaticAberrationEnabled = false;
    float chromaticAberrationIntensity = 0.1f;
    
    bool vignetteEnabled = false;
    float vignetteIntensity = 0.3f;
    
    bool filmGrainEnabled = false;
    float filmGrainIntensity = 0.05f;
    
    // === Tone Mapping ===
    enum class ToneMapper { None, Reinhard, ACES, Filmic, AgX };
    ToneMapper toneMapper = ToneMapper::ACES;
    float exposure = 1.0f;
    float gamma = 2.2f;
    
    // === Material Quality ===
    int textureAnisotropy = 8;      // 1, 2, 4, 8, 16
    bool normalMapsEnabled = true;
    bool parallaxMappingEnabled = true;
    bool sssEnabled = true;         // Subsurface scattering
    
    // === Hair ===
    bool hairShadows = true;
    int hairStrandDetail = 2;       // 0=cards, 1=low, 2=high
    
    // === Transparency ===
    enum class TransparencyMethod { AlphaBlend, OIT };
    TransparencyMethod transparency = TransparencyMethod::AlphaBlend;
};

// ============================================================================
// Render Preset Definitions
// ============================================================================

class RenderPresets {
public:
    static RenderSettings getPreview() {
        RenderSettings s;
        s.renderScale = 0.5f;
        s.antiAliasing = RenderSettings::AAMethod::FXAA;
        s.shadowMapSize = 512;
        s.shadowCascades = 2;
        s.softShadows = false;
        s.pcfSamples = 4;
        s.aoEnabled = false;
        s.ssrEnabled = false;
        s.giEnabled = false;
        s.volumetricsEnabled = false;
        s.bloomEnabled = false;
        s.dofEnabled = false;
        s.motionBlurEnabled = false;
        s.textureAnisotropy = 2;
        s.parallaxMappingEnabled = false;
        s.sssEnabled = false;
        s.hairStrandDetail = 0;
        return s;
    }
    
    static RenderSettings getDraft() {
        RenderSettings s;
        s.renderScale = 0.75f;
        s.antiAliasing = RenderSettings::AAMethod::FXAA;
        s.shadowMapSize = 1024;
        s.shadowCascades = 3;
        s.softShadows = true;
        s.pcfSamples = 8;
        s.aoEnabled = true;
        s.aoMethod = RenderSettings::AOMethod::SSAO;
        s.aoSamples = 8;
        s.ssrEnabled = false;
        s.giEnabled = false;
        s.volumetricsEnabled = false;
        s.bloomEnabled = true;
        s.bloomIntensity = 0.2f;
        s.dofEnabled = false;
        s.textureAnisotropy = 4;
        s.sssEnabled = true;
        s.hairStrandDetail = 1;
        return s;
    }
    
    static RenderSettings getStandard() {
        RenderSettings s;
        s.renderScale = 1.0f;
        s.antiAliasing = RenderSettings::AAMethod::TAA;
        s.shadowMapSize = 2048;
        s.shadowCascades = 4;
        s.softShadows = true;
        s.pcfSamples = 16;
        s.aoEnabled = true;
        s.aoMethod = RenderSettings::AOMethod::SSAO;
        s.aoSamples = 16;
        s.ssrEnabled = true;
        s.ssrMaxSteps = 32;
        s.giEnabled = false;
        s.volumetricsEnabled = false;
        s.bloomEnabled = true;
        s.textureAnisotropy = 8;
        s.sssEnabled = true;
        s.hairStrandDetail = 2;
        return s;
    }
    
    static RenderSettings getHigh() {
        RenderSettings s;
        s.renderScale = 1.0f;
        s.antiAliasing = RenderSettings::AAMethod::TAA;
        s.shadowMapSize = 4096;
        s.shadowCascades = 4;
        s.softShadows = true;
        s.pcfSamples = 32;
        s.aoEnabled = true;
        s.aoMethod = RenderSettings::AOMethod::HBAO;
        s.aoSamples = 32;
        s.ssrEnabled = true;
        s.ssrMaxSteps = 64;
        s.giEnabled = true;
        s.giSamples = 16;
        s.giBounces = 1;
        s.volumetricsEnabled = true;
        s.volumetricSamples = 16;
        s.bloomEnabled = true;
        s.dofEnabled = true;
        s.textureAnisotropy = 16;
        s.sssEnabled = true;
        s.hairStrandDetail = 2;
        s.hairShadows = true;
        return s;
    }
    
    static RenderSettings getUltra() {
        RenderSettings s;
        s.renderScale = 1.5f;  // Super sampling
        s.antiAliasing = RenderSettings::AAMethod::MSAA8x;
        s.shadowMapSize = 4096;
        s.shadowCascades = 4;
        s.softShadows = true;
        s.pcfSamples = 64;
        s.aoEnabled = true;
        s.aoMethod = RenderSettings::AOMethod::GTAO;
        s.aoSamples = 64;
        s.ssrEnabled = true;
        s.ssrMaxSteps = 128;
        s.giEnabled = true;
        s.giSamples = 64;
        s.giBounces = 3;
        s.volumetricsEnabled = true;
        s.volumetricSamples = 64;
        s.bloomEnabled = true;
        s.dofEnabled = true;
        s.motionBlurEnabled = true;
        s.textureAnisotropy = 16;
        s.sssEnabled = true;
        s.hairStrandDetail = 2;
        s.hairShadows = true;
        s.transparency = RenderSettings::TransparencyMethod::OIT;
        return s;
    }
    
    static RenderSettings getPreset(RenderQuality quality) {
        switch (quality) {
            case RenderQuality::Preview: return getPreview();
            case RenderQuality::Draft: return getDraft();
            case RenderQuality::Standard: return getStandard();
            case RenderQuality::High: return getHigh();
            case RenderQuality::Ultra: return getUltra();
            default: return getStandard();
        }
    }
};

// ============================================================================
// High Quality Output Settings
// ============================================================================

struct OutputSettings {
    // === Resolution ===
    int width = 1920;
    int height = 1080;
    
    // Common presets
    enum class Resolution {
        HD_720p,        // 1280x720
        FullHD_1080p,   // 1920x1080
        QHD_1440p,      // 2560x1440
        UHD_4K,         // 3840x2160
        UHD_8K,         // 7680x4320
        Square_1K,      // 1024x1024
        Square_2K,      // 2048x2048
        Square_4K,      // 4096x4096
        Portrait_2x3,   // 2000x3000
        Custom
    };
    
    Resolution preset = Resolution::FullHD_1080p;
    
    // === Format ===
    enum class ImageFormat { PNG, JPG, EXR, TGA, BMP };
    ImageFormat format = ImageFormat::PNG;
    int jpgQuality = 95;  // 0-100 for JPG
    
    // === Transparency ===
    bool transparentBackground = false;
    Vec3 backgroundColor{0.2f, 0.2f, 0.2f};
    
    // === Multi-sample ===
    int samples = 1;      // For accumulation rendering
    
    // === Sequence ===
    bool isSequence = false;
    int startFrame = 0;
    int endFrame = 100;
    int frameRate = 30;
    
    // === Post-render ===
    bool addWatermark = false;
    std::string watermarkText;
    
    static void applyResolutionPreset(OutputSettings& settings, Resolution preset) {
        switch (preset) {
            case Resolution::HD_720p:
                settings.width = 1280; settings.height = 720; break;
            case Resolution::FullHD_1080p:
                settings.width = 1920; settings.height = 1080; break;
            case Resolution::QHD_1440p:
                settings.width = 2560; settings.height = 1440; break;
            case Resolution::UHD_4K:
                settings.width = 3840; settings.height = 2160; break;
            case Resolution::UHD_8K:
                settings.width = 7680; settings.height = 4320; break;
            case Resolution::Square_1K:
                settings.width = 1024; settings.height = 1024; break;
            case Resolution::Square_2K:
                settings.width = 2048; settings.height = 2048; break;
            case Resolution::Square_4K:
                settings.width = 4096; settings.height = 4096; break;
            case Resolution::Portrait_2x3:
                settings.width = 2000; settings.height = 3000; break;
            default: break;
        }
        settings.preset = preset;
    }
};

// ============================================================================
// Render Output Manager
// ============================================================================

class RenderOutputManager {
public:
    static RenderOutputManager& getInstance() {
        static RenderOutputManager instance;
        return instance;
    }
    
    // === Quality Management ===
    
    void setQuality(RenderQuality quality) {
        currentQuality_ = quality;
        if (quality != RenderQuality::Custom) {
            currentSettings_ = RenderPresets::getPreset(quality);
        }
        if (onSettingsChanged_) onSettingsChanged_();
    }
    
    RenderQuality getQuality() const { return currentQuality_; }
    RenderSettings& getSettings() { return currentSettings_; }
    const RenderSettings& getSettings() const { return currentSettings_; }
    
    // === Screenshot ===
    
    void captureScreenshot(const std::string& path, const OutputSettings& settings) {
        // Store settings for the render callback
        pendingCapture_ = true;
        capturePath_ = path;
        captureSettings_ = settings;
    }
    
    bool hasPendingCapture() const { return pendingCapture_; }
    
    void completePendingCapture() {
        pendingCapture_ = false;
    }
    
    const std::string& getCapturePath() const { return capturePath_; }
    const OutputSettings& getCaptureSettings() const { return captureSettings_; }
    
    // === High Quality Render ===
    
    void renderHighQuality(const std::string& path, 
                           const OutputSettings& output,
                           std::function<void(float progress)> progressCallback = nullptr,
                           std::function<void(bool success)> completionCallback = nullptr) {
        
        // Save current settings
        RenderSettings savedSettings = currentSettings_;
        RenderQuality savedQuality = currentQuality_;
        
        // Apply ultra settings temporarily
        currentSettings_ = RenderPresets::getUltra();
        currentSettings_.width = output.width;
        currentSettings_.height = output.height;
        
        if (onSettingsChanged_) onSettingsChanged_();
        
        // Start render
        isRendering_ = true;
        renderProgress_ = 0.0f;
        
        // In a real implementation, this would be async
        // For now, just capture a screenshot
        captureScreenshot(path, output);
        
        if (progressCallback) progressCallback(1.0f);
        
        // Restore settings
        currentSettings_ = savedSettings;
        currentQuality_ = savedQuality;
        isRendering_ = false;
        
        if (onSettingsChanged_) onSettingsChanged_();
        if (completionCallback) completionCallback(true);
    }
    
    bool isRendering() const { return isRendering_; }
    float getRenderProgress() const { return renderProgress_; }
    
    // === Callbacks ===
    
    void setOnSettingsChanged(std::function<void()> callback) {
        onSettingsChanged_ = callback;
    }
    
    // === Quick Actions ===
    
    void quickPreview() { setQuality(RenderQuality::Preview); }
    void quickStandard() { setQuality(RenderQuality::Standard); }
    void quickHigh() { setQuality(RenderQuality::High); }
    
private:
    RenderOutputManager() {
        currentSettings_ = RenderPresets::getStandard();
    }
    
    RenderQuality currentQuality_ = RenderQuality::Standard;
    RenderSettings currentSettings_;
    
    bool pendingCapture_ = false;
    std::string capturePath_;
    OutputSettings captureSettings_;
    
    bool isRendering_ = false;
    float renderProgress_ = 0.0f;
    
    std::function<void()> onSettingsChanged_;
};

// ============================================================================
// Convenience
// ============================================================================

inline RenderOutputManager& getRenderOutput() {
    return RenderOutputManager::getInstance();
}

}  // namespace luma
