// Unified Renderer - Cross-platform PBR renderer
// Supports DX12 (Windows) and Metal (macOS/iOS)
#pragma once

#include "engine/renderer/mesh.h"
#include "engine/rendering/ssao.h"
#include "engine/rendering/ssr.h"
#include "engine/rendering/volumetrics.h"
#include "engine/rendering/advanced_shadows.h"
#include <string>
#include <vector>
#include <memory>

namespace luma {

// ===== GPU Mesh =====
// Platform-agnostic mesh representation
// Actual GPU resources are stored internally by the renderer
struct RHIGPUMesh {
    uint32_t indexCount = 0;
    uint32_t meshIndex = 0;  // Internal index for renderer's mesh storage
    
    // Texture presence flags
    bool hasDiffuseTexture = false;
    bool hasNormalTexture = false;
    bool hasSpecularTexture = false;
    
    // PBR parameters
    float baseColor[3] = {1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
};

// ===== Loaded Model =====
struct RHILoadedModel {
    std::vector<RHIGPUMesh> meshes;
    float center[3] = {0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
    std::string name = "Untitled";
    std::string debugName;  // Full path for serialization
    size_t totalVerts = 0;
    size_t totalTris = 0;
    int textureCount = 0;
    size_t meshStorageStartIndex = 0;  // Internal: starting index in renderer's storage
};

// ===== Camera Parameters =====
struct RHICameraParams {
    float yaw = 0.0f;           // Horizontal rotation (radians)
    float pitch = 0.0f;         // Vertical rotation (radians)
    float distance = 1.0f;      // Distance multiplier
    float targetOffsetX = 0.0f;
    float targetOffsetY = 0.0f;
    float targetOffsetZ = 0.0f;
};

// ===== Shadow Settings =====
struct ShadowSettings {
    bool enabled = true;            // Shadow mapping enabled
    uint32_t mapSize = 2048;        // Shadow map resolution
    float bias = 0.005f;            // Depth bias to prevent acne
    float normalBias = 0.02f;       // Normal-based bias for grazing angles
    int pcfSamples = 3;             // PCF kernel size (1, 2, 3 = 3x3, 5x5, 7x7)
    float softness = 1.0f;          // PCF sampling spread
    float distance = 50.0f;         // Shadow distance from scene center
};

// ===== IBL Settings =====
struct IBLSettings {
    bool enabled = true;
    float intensity = 1.0f;         // Environment light intensity
    float rotation = 0.0f;          // Environment rotation (radians)
    uint32_t irradianceSize = 32;   // Irradiance cubemap size
    uint32_t prefilteredSize = 256; // Prefiltered env map size
    uint32_t prefilteredMips = 5;   // Mip levels for roughness
    uint32_t brdfLutSize = 512;     // BRDF LUT size
};

// ===== Scene Constants (must match shader) =====
struct alignas(256) RHISceneConstants {
    float worldViewProj[16];
    float world[16];
    float lightViewProj[16];         // Light's view-projection matrix for shadows
    float lightDirAndFlags[4];       // xyz = lightDir, w = flags (texture bits)
    float cameraPosAndMetal[4];      // xyz = cameraPos, w = metallic
    float baseColorAndRough[4];      // xyz = baseColor, w = roughness
    float shadowParams[4];           // x = bias, y = normalBias, z = softness, w = enabled
    float iblParams[4];              // x = intensity, y = rotation, z = maxMipLevel, w = enabled
};

// ===== Unified Renderer =====
class UnifiedRenderer {
public:
    UnifiedRenderer();
    ~UnifiedRenderer();
    
    // Non-copyable, movable
    UnifiedRenderer(const UnifiedRenderer&) = delete;
    UnifiedRenderer& operator=(const UnifiedRenderer&) = delete;
    UnifiedRenderer(UnifiedRenderer&&) noexcept;
    UnifiedRenderer& operator=(UnifiedRenderer&&) noexcept;
    
    // === Initialization ===
    bool initialize(void* windowHandle, uint32_t width, uint32_t height);
    void shutdown();
    void resize(uint32_t width, uint32_t height);
    
    // === Resource Management ===
    RHIGPUMesh uploadMesh(const Mesh& mesh);
    bool loadModel(const std::string& path, RHILoadedModel& outModel);
    
    // Async model loading - geometry loads immediately, textures load in background
    bool loadModelAsync(const std::string& path, RHILoadedModel& outModel);
    
    // Get async loading progress (0.0 - 1.0)
    float getAsyncLoadProgress() const;
    
    // === Frame Rendering ===
    void beginFrame();
    void endFrame();
    
    // === Async Texture Loading ===
    // Call once per frame to process completed async texture uploads
    void processAsyncTextures();
    
    // === Render Operations ===
    void render(const RHILoadedModel& model, float time, float camDistMultiplier);
    void render(const RHILoadedModel& model, const RHICameraParams& camera);
    void renderGrid(const RHICameraParams& camera, float modelRadius);
    
    // === Scene Graph Rendering ===
    // Set camera for subsequent render calls
    void setCamera(const RHICameraParams& camera, float sceneRadius);
    
    // Render model with explicit world transform matrix (16 floats, column-major)
    void renderModel(const RHILoadedModel& model, const float* worldMatrix);
    
    // Render skinned model with bone matrices
    // boneMatrices: array of MAX_BONES (128) 4x4 matrices
    void renderSkinnedModel(const RHILoadedModel& model, const float* worldMatrix, const float* boneMatrices);
    
    // Render model with selection outline (for selected objects)
    void renderModelOutline(const RHILoadedModel& model, const float* worldMatrix, const float* outlineColor);
    
    // Render line list for gizmos (pairs of points, RGBA color per line)
    // lines: array of {startX, startY, startZ, endX, endY, endZ, r, g, b, a} per line
    void renderGizmoLines(const float* lines, uint32_t lineCount);
    
    // === Shadow Mapping ===
    // Configure shadow settings
    void setShadowSettings(const ShadowSettings& settings);
    const ShadowSettings& getShadowSettings() const;
    
    // Begin shadow pass - renders to shadow map from light's perspective
    void beginShadowPass(float sceneRadius, const float* sceneCenter = nullptr);
    
    // Render model to shadow map (depth only)
    void renderModelShadow(const RHILoadedModel& model, const float* worldMatrix);
    
    // End shadow pass and prepare for main rendering
    void endShadowPass();
    
    // === Image-Based Lighting (IBL) ===
    // Configure IBL settings
    void setIBLSettings(const IBLSettings& settings);
    const IBLSettings& getIBLSettings() const;
    
    // Load HDR environment map and generate IBL textures
    // Returns true if successful
    bool loadEnvironmentMap(const std::string& hdrPath);
    
    // Check if IBL is ready (environment loaded and textures generated)
    bool isIBLReady() const;
    
    // === Shader Hot-Reload ===
    // Enable/disable shader hot-reload (watches shader files for changes)
    void setShaderHotReload(bool enabled);
    bool isShaderHotReloadEnabled() const;
    
    // Manually trigger shader reload (returns true if successful)
    bool reloadShaders();
    
    // Check for shader file changes and reload if needed (call once per frame)
    void checkShaderReload();
    
    // Get last shader compilation error (empty if no error)
    const std::string& getShaderError() const;
    
    // === Post-Processing ===
    // Enable/disable post-processing pipeline
    void setPostProcessEnabled(bool enabled);
    bool isPostProcessEnabled() const;
    
    // Set post-process parameters (bloom, tone mapping, etc.)
    // Constants structure defined in post_process.h
    void setPostProcessParams(const void* constants, size_t size);
    
    // Get current frame time for animated effects (film grain, etc.)
    float getFrameTime() const;
    
    // === Advanced Post-Processing ===
    
    // SSAO (Screen Space Ambient Occlusion)
    void setSSAOEnabled(bool enabled);
    bool isSSAOEnabled() const;
    void setSSAOSettings(const SSAOSettings& settings);
    const SSAOSettings& getSSAOSettings() const;
    
    // SSR (Screen Space Reflections)
    void setSSREnabled(bool enabled);
    bool isSSREnabled() const;
    void setSSRSettings(const SSRSettings& settings);
    const SSRSettings& getSSRSettings() const;
    
    // Volumetric Effects
    void setVolumetricFogEnabled(bool enabled);
    bool isVolumetricFogEnabled() const;
    void setVolumetricFogSettings(const VolumetricFogSettings& settings);
    const VolumetricFogSettings& getVolumetricFogSettings() const;
    
    void setGodRaysEnabled(bool enabled);
    bool isGodRaysEnabled() const;
    void setGodRaysSettings(const GodRaySettings& settings);
    const GodRaySettings& getGodRaysSettings() const;
    
    // === Advanced Shadows ===
    
    // Cascaded Shadow Maps
    void setCSMEnabled(bool enabled);
    bool isCSMEnabled() const;
    void setCSMSettings(const CSMSettings& settings);
    const CSMSettings& getCSMSettings() const;
    
    // PCSS (Percentage Closer Soft Shadows)
    void setPCSSEnabled(bool enabled);
    bool isPCSSEnabled() const;
    void setPCSSSettings(int blockerSamples, int pcfSamples, float lightSize);
    
    // Update CSM for current frame (call before shadow passes)
    void updateCSM(const float* cameraView, const float* cameraProj, 
                   const float* lightDirection, float cameraNear, float cameraFar);
    
    // === Accessors ===
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    
    // Get inverse view-projection matrix (for picking/ray generation)
    // Returns false if not available
    bool getViewProjectionInverse(float* outMatrix16) const;
    
    // Post-processing / UI rendering separation
    // Call after all 3D rendering, before UI rendering
    // This applies post-processing (if enabled) and switches render target to swapchain
    void finishSceneRendering();
    
    // Native handle access (for ImGui integration)
    void* getNativeDevice() const;
    void* getNativeQueue() const;
    void* getNativeCommandEncoder() const;  // For ImGui rendering
    void* getNativeSrvHeap() const;         // For ImGui font texture
    
    void waitForGPU();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace luma
