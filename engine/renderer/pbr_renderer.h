// PBR Renderer - High-level rendering interface
#pragma once

#include <memory>
#include <vector>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include "engine/renderer/mesh.h"

namespace luma {

// Forward declarations
struct RenderContext;

// GPU-side mesh representation
struct GPUMesh {
#ifdef _WIN32
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> diffuseTexture;
    ComPtr<ID3D12Resource> normalTexture;
    ComPtr<ID3D12Resource> specularTexture;
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    D3D12_INDEX_BUFFER_VIEW ibv{};
#endif
    uint32_t indexCount = 0;
    bool hasDiffuseTexture = false;
    bool hasNormalTexture = false;
    bool hasSpecularTexture = false;
    uint32_t diffuseSrvIndex = 0;
    uint32_t normalSrvIndex = 0;
    uint32_t specularSrvIndex = 0;
    
    // PBR parameters
    float baseColor[3] = {1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
};

// Scene constants for shader
struct alignas(256) SceneConstants {
    float worldViewProj[16];
    float world[16];
    float lightDirAndFlags[4];   // xyz = lightDir, w = flags
    float cameraPosAndMetal[4];  // xyz = cameraPos, w = metallic
    float baseColorAndRough[4];  // xyz = baseColor, w = roughness
};

// Loaded model state
struct LoadedModel {
    std::vector<GPUMesh> meshes;
    float center[3] = {0, 0, 0};
    float radius = 1.0f;
    std::string name = "Untitled";
    size_t totalVerts = 0;
    size_t totalTris = 0;
    int textureCount = 0;
};

// PBR Renderer class (Pimpl pattern)
class PBRRenderer {
public:
    PBRRenderer();
    ~PBRRenderer();
    
    // Non-copyable, movable
    PBRRenderer(const PBRRenderer&) = delete;
    PBRRenderer& operator=(const PBRRenderer&) = delete;
    PBRRenderer(PBRRenderer&&) noexcept;
    PBRRenderer& operator=(PBRRenderer&&) noexcept;
    
    // Initialize renderer with window handle
    bool initialize(void* windowHandle, uint32_t width, uint32_t height);
    
    // Resize viewport
    void resize(uint32_t width, uint32_t height);
    
    // Upload mesh to GPU
    GPUMesh uploadMesh(const Mesh& mesh);
    
    // Load model (convenience function)
    bool loadModel(const std::string& path, LoadedModel& outModel);
    
    // Begin frame
    void beginFrame();
    
    // Render model (simple orbit camera)
    void render(const LoadedModel& model, float time, float camDistMultiplier);
    
    // Render model with full camera control
    struct CameraParams {
        float yaw = 0.0f;           // Horizontal rotation (radians)
        float pitch = 0.0f;         // Vertical rotation (radians)
        float distance = 2.5f;      // Distance multiplier
        float targetOffsetX = 0.0f; // Target point offset X
        float targetOffsetY = 0.0f; // Target point offset Y
        float targetOffsetZ = 0.0f; // Target point offset Z
    };
    void render(const LoadedModel& model, const CameraParams& camera);
    
    // Render ground grid and axes
    void renderGrid(const CameraParams& camera, float gridSize = 10.0f);
    
    // End frame and present
    void endFrame();
    
    // Get render context for ImGui
    void* getDevice() const;
    void* getCommandList() const;
    void* getSrvHeap() const;
    uint32_t getSrvDescriptorSize() const;
    
    // Wait for GPU
    void waitForGPU();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace luma
