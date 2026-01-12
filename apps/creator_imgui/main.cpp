#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <cmath>
#include <cstring>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>

#include "engine/engine_facade.h"
#include "engine/scene/scene.h"
#include "engine/actions/action.h"
#include "engine/renderer/mesh.h"
#include "engine/asset/model_loader.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "comdlg32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using Microsoft::WRL::ComPtr;

inline void CheckHR(HRESULT hr, const char* where) {
    if (FAILED(hr)) {
        std::cerr << "[luma] DX12 failure at " << where << " hr=0x" << std::hex << hr << std::dec << std::endl;
        std::terminate();
    }
}

static void EnableDebugLayer() {
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
    }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
        case WM_SIZE: return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ===== Math helpers =====
namespace math {
    void identity(float* m) {
        std::memset(m, 0, 16 * sizeof(float));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    void perspective(float* m, float fovY, float aspect, float nearZ, float farZ) {
        std::memset(m, 0, 16 * sizeof(float));
        float yScale = 1.0f / tanf(fovY * 0.5f);
        m[0] = yScale / aspect;
        m[5] = yScale;
        m[10] = farZ / (farZ - nearZ);
        m[11] = 1.0f;
        m[14] = -nearZ * farZ / (farZ - nearZ);
    }
    void lookAt(float* m, const float* eye, const float* at, const float* up) {
        float zAxis[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
        float len = sqrtf(zAxis[0]*zAxis[0] + zAxis[1]*zAxis[1] + zAxis[2]*zAxis[2]);
        if (len < 0.0001f) len = 1.0f;
        zAxis[0] /= len; zAxis[1] /= len; zAxis[2] /= len;
        float xAxis[3] = {up[1]*zAxis[2] - up[2]*zAxis[1], up[2]*zAxis[0] - up[0]*zAxis[2], up[0]*zAxis[1] - up[1]*zAxis[0]};
        len = sqrtf(xAxis[0]*xAxis[0] + xAxis[1]*xAxis[1] + xAxis[2]*xAxis[2]);
        if (len < 0.0001f) { xAxis[0] = 1; len = 1.0f; }
        xAxis[0] /= len; xAxis[1] /= len; xAxis[2] /= len;
        float yAxis[3] = {zAxis[1]*xAxis[2] - zAxis[2]*xAxis[1], zAxis[2]*xAxis[0] - zAxis[0]*xAxis[2], zAxis[0]*xAxis[1] - zAxis[1]*xAxis[0]};
        m[0] = xAxis[0]; m[1] = yAxis[0]; m[2] = zAxis[0]; m[3] = 0;
        m[4] = xAxis[1]; m[5] = yAxis[1]; m[6] = zAxis[1]; m[7] = 0;
        m[8] = xAxis[2]; m[9] = yAxis[2]; m[10] = zAxis[2]; m[11] = 0;
        m[12] = -(xAxis[0]*eye[0] + xAxis[1]*eye[1] + xAxis[2]*eye[2]);
        m[13] = -(yAxis[0]*eye[0] + yAxis[1]*eye[1] + yAxis[2]*eye[2]);
        m[14] = -(zAxis[0]*eye[0] + zAxis[1]*eye[1] + zAxis[2]*eye[2]);
        m[15] = 1;
    }
    void multiply(float* out, const float* a, const float* b) {
        float tmp[16];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                tmp[i*4+j] = a[i*4+0]*b[0*4+j] + a[i*4+1]*b[1*4+j] + a[i*4+2]*b[2*4+j] + a[i*4+3]*b[3*4+j];
        std::memcpy(out, tmp, sizeof(tmp));
    }
}

// ===== PBR Shader with Normal Mapping =====
const char* kShaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;   // 64 bytes
    float4x4 world;           // 64 bytes
    float4 lightDirAndFlags;  // xyz = lightDir, w = texture flags (packed)
    float4 cameraPosAndMetal; // xyz = cameraPos, w = metallic
    float4 baseColorAndRough; // xyz = baseColor, w = roughness
};

#define lightDir lightDirAndFlags.xyz
#define texFlags lightDirAndFlags.w
#define cameraPos cameraPosAndMetal.xyz
#define metallic cameraPosAndMetal.w
#define baseColor baseColorAndRough.xyz
#define roughness baseColorAndRough.w

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);
SamplerState texSampler : register(s0);

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 uv : TEXCOORD4;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.worldPos = mul(world, float4(input.position, 1.0)).xyz;
    output.normal = normalize(mul((float3x3)world, input.normal));
    output.tangent = normalize(mul((float3x3)world, input.tangent.xyz));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Sample all textures
    float4 diffuseSample = diffuseTexture.Sample(texSampler, input.uv);
    float4 normalSample = normalTexture.Sample(texSampler, input.uv);
    float4 specularSample = specularTexture.Sample(texSampler, input.uv);
    
    // Alpha test
    if (diffuseSample.a < 0.1) {
        discard;
    }
    
    // Albedo - use texture if it has color data, otherwise use material color
    float3 albedo;
    float texBrightness = diffuseSample.r + diffuseSample.g + diffuseSample.b;
    if (texBrightness < 2.9) {  // Not pure white (has actual texture)
        albedo = diffuseSample.rgb;
    } else {
        // No texture (white default) - use vertex color * baseColor
        albedo = input.color * baseColor;
    }
    
    // Normal mapping - check if normal map has actual data (not flat white)
    float3 N;
    bool hasNormalMap = (abs(normalSample.r - normalSample.g) > 0.01 || 
                         abs(normalSample.b - 1.0) > 0.1);  // Normal maps are usually blueish
    if (hasNormalMap) {
        float3 normalMap = normalSample.rgb * 2.0 - 1.0;
        float3x3 TBN = float3x3(
            normalize(input.tangent),
            normalize(input.bitangent),
            normalize(input.normal)
        );
        N = normalize(mul(normalMap, TBN));
    } else {
        N = normalize(input.normal);
    }
    
    // PBR parameters - use texture if available, else material values
    float metal = metallic;
    float rough = roughness;
    bool hasSpecMap = (specularSample.r < 0.99 || specularSample.g < 0.99 || specularSample.b < 0.99);
    if (hasSpecMap) {
        // ORM convention: R=AO, G=Roughness, B=Metallic
        metal = specularSample.b;
        rough = specularSample.g;
    }
    rough = max(rough, 0.04);
    
    // Vectors
    float3 V = normalize(cameraPos - input.worldPos);
    float3 L = normalize(-lightDir);
    float3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // F0 - base reflectivity
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metal);
    
    // Cook-Torrance BRDF
    // D - GGX Distribution
    float a = rough * rough;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    float D = a2 / (3.14159 * denom * denom + 0.0001);
    
    // G - Smith's Geometry function
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    float G1_V = NdotV / (NdotV * (1.0 - k) + k);
    float G1_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G1_V * G1_L;
    
    // F - Fresnel-Schlick
    float3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    
    // Specular BRDF
    float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    
    // Diffuse (energy conservation)
    float3 kD = (1.0 - F) * (1.0 - metal);
    float3 diffuse = kD * albedo / 3.14159;
    
    // Direct lighting (warm sunlight)
    float3 lightColor = float3(1.0, 0.98, 0.95) * 2.5;
    float3 Lo = (diffuse + specular) * NdotL * lightColor;
    
    // Ambient (sky light approximation)
    float3 ambient = albedo * 0.2;
    
    // Final color
    float3 color = ambient + Lo;
    
    // Tone mapping (ACES approximation)
    color = color * (color + 0.5) / (color * (color + 0.55) + 0.5);
    
    return float4(color, 1.0);
}
)";

// ===== Constant buffer =====
struct alignas(256) SceneConstants {
    float worldViewProj[16];      // 64 bytes
    float world[16];              // 64 bytes
    float lightDirAndTex[4];      // xyz = lightDir, w = hasTexture
    float cameraPosAndMetal[4];   // xyz = cameraPos, w = metallic
    float baseColorAndRough[4];   // xyz = baseColor, w = roughness
};

// ===== GPU Mesh with texture =====
struct MeshGPU {
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    ComPtr<ID3D12Resource> diffuseTexture;
    ComPtr<ID3D12Resource> normalTexture;
    ComPtr<ID3D12Resource> specularTexture;
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    D3D12_INDEX_BUFFER_VIEW ibv{};
    UINT indexCount{0};
    bool hasDiffuseTexture{false};
    bool hasNormalTexture{false};
    bool hasSpecularTexture{false};
    UINT diffuseSrvIndex{0};
    UINT normalSrvIndex{0};
    UINT specularSrvIndex{0};
    
    // PBR parameters per mesh
    float baseColor[3] = {1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
};

// ===== DX Context =====
struct DxContext {
    HWND hwnd{};
    UINT width{1280}, height{720};
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;  // For textures + ImGui
    std::vector<ComPtr<ID3D12Resource>> renderTargets;
    ComPtr<ID3D12Resource> depthStencil;
    std::vector<ComPtr<ID3D12CommandAllocator>> allocators;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue{0};
    HANDLE fenceEvent{nullptr};
    UINT frameIndex{0};
    UINT rtvDescriptorSize{0};
    UINT srvDescriptorSize{0};
    UINT nextSrvIndex{1};  // 0 is reserved for ImGui font
    
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12Resource> constantBuffer;
    SceneConstants sceneConstants{};
    bool pipelineReady{false};
    
    // Default white texture for meshes without texture
    ComPtr<ID3D12Resource> defaultTexture;
    UINT defaultTextureSrvIndex{0};
};

struct ModelState {
    std::vector<MeshGPU> meshes;
    float center[3] = {0, 0, 0};
    float radius = 1.0f;
    std::string name = "Default Cube";
    size_t totalVerts = 0;
    size_t totalTris = 0;
    int textureCount = 0;
};

static void CreateDevice(DxContext& ctx) {
    EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;
    CheckHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc; adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ctx.device)))) break;
    }
    CheckHR(ctx.device ? S_OK : E_FAIL, "D3D12CreateDevice");
    D3D12_COMMAND_QUEUE_DESC qdesc{}; qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CheckHR(ctx.device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&ctx.queue)), "CreateCommandQueue");
}

static void CreateSwapchain(DxContext& ctx) {
    ComPtr<IDXGIFactory6> factory;
    CheckHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = 2; desc.Width = ctx.width; desc.Height = ctx.height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; desc.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> sc1;
    CheckHR(factory->CreateSwapChainForHwnd(ctx.queue.Get(), ctx.hwnd, &desc, nullptr, nullptr, &sc1), "CreateSwapChainForHwnd");
    CheckHR(sc1.As(&ctx.swapchain), "Swapchain As");
    ctx.frameIndex = ctx.swapchain->GetCurrentBackBufferIndex();
}

static void CreateHeaps(DxContext& ctx) {
    // RTV
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{}; rtvDesc.NumDescriptors = 2; rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    CheckHR(ctx.device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&ctx.rtvHeap)), "RTV Heap");
    ctx.rtvDescriptorSize = ctx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    ctx.renderTargets.resize(2);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++) {
        CheckHR(ctx.swapchain->GetBuffer(i, IID_PPV_ARGS(&ctx.renderTargets[i])), "GetBuffer RTV");
        ctx.device->CreateRenderTargetView(ctx.renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += ctx.rtvDescriptorSize;
    }
    // DSV
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{}; dsvDesc.NumDescriptors = 1; dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    CheckHR(ctx.device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&ctx.dsvHeap)), "DSV Heap");
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC depthDesc{};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = ctx.width; depthDesc.Height = ctx.height;
    depthDesc.DepthOrArraySize = 1; depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT; depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE clearValue{}; clearValue.Format = DXGI_FORMAT_D32_FLOAT; clearValue.DepthStencil.Depth = 1.0f;
    CheckHR(ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&ctx.depthStencil)), "Depth");
    ctx.device->CreateDepthStencilView(ctx.depthStencil.Get(), nullptr, ctx.dsvHeap->GetCPUDescriptorHandleForHeapStart());
    // SRV heap (for textures + ImGui)
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{}; srvDesc.NumDescriptors = 256;  // Room for many textures
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CheckHR(ctx.device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&ctx.srvHeap)), "SRV Heap");
    ctx.srvDescriptorSize = ctx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

static void CreateCommands(DxContext& ctx) {
    ctx.allocators.resize(2);
    for (auto& alloc : ctx.allocators)
        CheckHR(ctx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)), "Allocator");
    CheckHR(ctx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, ctx.allocators[0].Get(), nullptr, IID_PPV_ARGS(&ctx.cmdList)), "CmdList");
    ctx.cmdList->Close();
    CheckHR(ctx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx.fence)), "Fence");
    ctx.fenceValue = 1;
    ctx.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

static void CreateDefaultTexture(DxContext& ctx) {
    // Create 1x1 white texture (neutral for multiplication)
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC texDesc{};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = 1; texDesc.Height = 1; texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; texDesc.SampleDesc.Count = 1;
    CheckHR(ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&ctx.defaultTexture)), "DefaultTex");
    
    // Upload white pixel (neutral for texture * baseColor)
    D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = 256; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> uploadBuf;
    CheckHR(ctx.device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf)), "UploadBuf");
    uint8_t white[4] = {255, 255, 255, 255};  // White = neutral for multiplication
    void* mapped; uploadBuf->Map(0, nullptr, &mapped);
    memcpy(mapped, white, 4);
    uploadBuf->Unmap(0, nullptr);
    
    // Copy
    ctx.allocators[0]->Reset();
    ctx.cmdList->Reset(ctx.allocators[0].Get(), nullptr);
    D3D12_TEXTURE_COPY_LOCATION dst{}; dst.pResource = ctx.defaultTexture.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION src{}; src.pResource = uploadBuf.Get(); src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = 1; src.PlacedFootprint.Footprint.Height = 1;
    src.PlacedFootprint.Footprint.Depth = 1; src.PlacedFootprint.Footprint.RowPitch = 256;
    ctx.cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = ctx.defaultTexture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ctx.cmdList->ResourceBarrier(1, &barrier);
    ctx.cmdList->Close();
    ID3D12CommandList* lists[] = {ctx.cmdList.Get()};
    ctx.queue->ExecuteCommandLists(1, lists);
    ctx.queue->Signal(ctx.fence.Get(), ctx.fenceValue);
    ctx.fence->SetEventOnCompletion(ctx.fenceValue++, ctx.fenceEvent);
    WaitForSingleObject(ctx.fenceEvent, INFINITE);
    
    // Create SRV
    ctx.defaultTextureSrvIndex = ctx.nextSrvIndex++;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = ctx.srvHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += ctx.defaultTextureSrvIndex * ctx.srvDescriptorSize;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvViewDesc.Texture2D.MipLevels = 1;
    ctx.device->CreateShaderResourceView(ctx.defaultTexture.Get(), &srvViewDesc, srvHandle);
}

static void CreatePipeline(DxContext& ctx) {
    // Root signature: CBV(b0), SRV table (t0, t1, t2), Sampler(s0)
    D3D12_DESCRIPTOR_RANGE srvRange{}; srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 3; srvRange.BaseShaderRegister = 0;  // t0, t1, t2
    D3D12_ROOT_PARAMETER rootParams[2]{};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1; rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    D3D12_STATIC_SAMPLER_DESC sampler{}; 
    sampler.Filter = D3D12_FILTER_ANISOTROPIC;  // High quality anisotropic filtering
    sampler.MaxAnisotropy = 16;  // Maximum anisotropy
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    D3D12_ROOT_SIGNATURE_DESC rsDesc{}; rsDesc.NumParameters = 2; rsDesc.pParameters = rootParams;
    rsDesc.NumStaticSamplers = 1; rsDesc.pStaticSamplers = &sampler;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr) && error) std::cerr << (char*)error->GetBufferPointer() << std::endl;
    CheckHR(hr, "SerializeRS");
    CheckHR(ctx.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&ctx.rootSignature)), "CreateRS");
    
    // Compile shaders
    ComPtr<ID3DBlob> vs, ps, errorBlob;
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    hr = D3DCompile(kShaderSource, strlen(kShaderSource), "shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob);
    if (FAILED(hr) && errorBlob) std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
    CheckHR(hr, "Compile VS");
    hr = D3DCompile(kShaderSource, strlen(kShaderSource), "shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob);
    if (FAILED(hr) && errorBlob) std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
    CheckHR(hr, "Compile PS");
    
    // Input layout: position(12) + normal(12) + tangent(16) + uv(8) + color(12) = 60 bytes
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
    psoDesc.pRootSignature = ctx.rootSignature.Get();
    psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // Disable culling to show all faces
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    CheckHR(ctx.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&ctx.pipelineState)), "CreatePSO");
    
    // Constant buffer
    D3D12_HEAP_PROPERTIES cbHeap{}; cbHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC cbDesc{}; cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = sizeof(SceneConstants); cbDesc.Height = 1; cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1; cbDesc.SampleDesc.Count = 1; cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    CheckHR(ctx.device->CreateCommittedResource(&cbHeap, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ctx.constantBuffer)), "CB");
    
    ctx.pipelineReady = true;
    std::cout << "[luma] Pipeline with textures ready" << std::endl;
}

static ComPtr<ID3D12Resource> UploadTexture(DxContext& ctx, const luma::TextureData& tex, UINT& outSrvIndex) {
    if (tex.pixels.empty()) return nullptr;
    
    // Create texture resource
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC texDesc{}; texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = tex.width; texDesc.Height = tex.height;
    texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; texDesc.SampleDesc.Count = 1;
    ComPtr<ID3D12Resource> texture;
    CheckHR(ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)), "Texture");
    
    // Upload buffer
    UINT64 uploadSize;
    ctx.device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);
    D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = uploadSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> uploadBuf;
    CheckHR(ctx.device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf)), "UploadBuf");
    
    // Map and copy
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    ctx.device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);
    void* mapped;
    uploadBuf->Map(0, nullptr, &mapped);
    for (UINT y = 0; y < (UINT)tex.height; y++) {
        memcpy((uint8_t*)mapped + y * footprint.Footprint.RowPitch, tex.pixels.data() + y * tex.width * 4, tex.width * 4);
    }
    uploadBuf->Unmap(0, nullptr);
    
    // Copy command
    ctx.allocators[0]->Reset();
    ctx.cmdList->Reset(ctx.allocators[0].Get(), nullptr);
    D3D12_TEXTURE_COPY_LOCATION dst{}; dst.pResource = texture.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION src{}; src.pResource = uploadBuf.Get(); src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = footprint;
    ctx.cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ctx.cmdList->ResourceBarrier(1, &barrier);
    ctx.cmdList->Close();
    ID3D12CommandList* lists[] = {ctx.cmdList.Get()};
    ctx.queue->ExecuteCommandLists(1, lists);
    ctx.queue->Signal(ctx.fence.Get(), ctx.fenceValue);
    ctx.fence->SetEventOnCompletion(ctx.fenceValue++, ctx.fenceEvent);
    WaitForSingleObject(ctx.fenceEvent, INFINITE);
    
    // Create SRV
    outSrvIndex = ctx.nextSrvIndex++;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = ctx.srvHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += outSrvIndex * ctx.srvDescriptorSize;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvViewDesc.Texture2D.MipLevels = 1;
    ctx.device->CreateShaderResourceView(texture.Get(), &srvViewDesc, srvHandle);
    
    return texture;
}

static MeshGPU CreateMeshGPU(DxContext& ctx, const luma::Mesh& mesh) {
    MeshGPU gpu;
    gpu.indexCount = static_cast<UINT>(mesh.indices.size());
    const UINT vbSize = static_cast<UINT>(mesh.vertices.size() * sizeof(luma::Vertex));
    const UINT ibSize = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
    
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = vbSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    CheckHR(ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu.vertexBuffer)), "VB");
    bufDesc.Width = ibSize;
    CheckHR(ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu.indexBuffer)), "IB");
    
    void* mapped;
    gpu.vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, mesh.vertices.data(), vbSize);
    gpu.vertexBuffer->Unmap(0, nullptr);
    gpu.indexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, mesh.indices.data(), ibSize);
    gpu.indexBuffer->Unmap(0, nullptr);
    
    gpu.vbv.BufferLocation = gpu.vertexBuffer->GetGPUVirtualAddress();
    gpu.vbv.SizeInBytes = vbSize; gpu.vbv.StrideInBytes = sizeof(luma::Vertex);
    gpu.ibv.BufferLocation = gpu.indexBuffer->GetGPUVirtualAddress();
    gpu.ibv.SizeInBytes = ibSize; gpu.ibv.Format = DXGI_FORMAT_R32_UINT;
    
    // Copy PBR parameters
    gpu.baseColor[0] = mesh.baseColor[0];
    gpu.baseColor[1] = mesh.baseColor[1];
    gpu.baseColor[2] = mesh.baseColor[2];
    gpu.metallic = mesh.metallic;
    gpu.roughness = mesh.roughness;
    
    // Allocate 3 consecutive SRV slots for this mesh (diffuse, normal, specular)
    UINT baseSrvIndex = ctx.nextSrvIndex;
    ctx.nextSrvIndex += 3;
    
    gpu.diffuseSrvIndex = baseSrvIndex;
    gpu.normalSrvIndex = baseSrvIndex + 1;
    gpu.specularSrvIndex = baseSrvIndex + 2;
    
    // Helper to create SRV at specific index
    auto createSrvAt = [&](ID3D12Resource* texture, UINT index) {
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = ctx.srvHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += index * ctx.srvDescriptorSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; 
        srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
        srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Texture2D.MipLevels = 1;
        ctx.device->CreateShaderResourceView(texture, &srvViewDesc, srvHandle);
    };
    
    // Upload diffuse texture
    if (mesh.hasDiffuseTexture) {
        UINT tempIndex;  // We'll use our pre-allocated index instead
        gpu.diffuseTexture = UploadTexture(ctx, mesh.diffuseTexture, tempIndex);
        gpu.hasDiffuseTexture = (gpu.diffuseTexture != nullptr);
        if (gpu.hasDiffuseTexture) {
            createSrvAt(gpu.diffuseTexture.Get(), gpu.diffuseSrvIndex);
            std::cout << "[gpu] Diffuse texture -> srvIndex=" << gpu.diffuseSrvIndex << std::endl;
        }
    }
    if (!gpu.hasDiffuseTexture) {
        createSrvAt(ctx.defaultTexture.Get(), gpu.diffuseSrvIndex);
    }
    
    // Upload normal texture
    if (mesh.hasNormalTexture) {
        UINT tempIndex;
        gpu.normalTexture = UploadTexture(ctx, mesh.normalTexture, tempIndex);
        gpu.hasNormalTexture = (gpu.normalTexture != nullptr);
        if (gpu.hasNormalTexture) {
            createSrvAt(gpu.normalTexture.Get(), gpu.normalSrvIndex);
            std::cout << "[gpu] Normal texture -> srvIndex=" << gpu.normalSrvIndex << std::endl;
        }
    }
    if (!gpu.hasNormalTexture) {
        createSrvAt(ctx.defaultTexture.Get(), gpu.normalSrvIndex);
    }
    
    // Upload specular texture
    if (mesh.hasSpecularTexture) {
        UINT tempIndex;
        gpu.specularTexture = UploadTexture(ctx, mesh.specularTexture, tempIndex);
        gpu.hasSpecularTexture = (gpu.specularTexture != nullptr);
        if (gpu.hasSpecularTexture) {
            createSrvAt(gpu.specularTexture.Get(), gpu.specularSrvIndex);
            std::cout << "[gpu] Specular texture -> srvIndex=" << gpu.specularSrvIndex << std::endl;
        }
    }
    if (!gpu.hasSpecularTexture) {
        createSrvAt(ctx.defaultTexture.Get(), gpu.specularSrvIndex);
    }
    
    return gpu;
}

static void WaitForGPU(DxContext& ctx) {
    const UINT64 fv = ctx.fenceValue++;
    ctx.queue->Signal(ctx.fence.Get(), fv);
    if (ctx.fence->GetCompletedValue() < fv) {
        ctx.fence->SetEventOnCompletion(fv, ctx.fenceEvent);
        WaitForSingleObject(ctx.fenceEvent, INFINITE);
    }
    ctx.frameIndex = ctx.swapchain->GetCurrentBackBufferIndex();
}

static void Render(DxContext& ctx, ModelState& model, float time, float camDist) {
    auto& alloc = ctx.allocators[ctx.frameIndex];
    CheckHR(alloc->Reset(), "Allocator Reset");
    CheckHR(ctx.cmdList->Reset(alloc.Get(), ctx.pipelineState.Get()), "CmdList Reset");
    
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = ctx.renderTargets[ctx.frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ctx.cmdList->ResourceBarrier(1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += static_cast<SIZE_T>(ctx.frameIndex) * ctx.rtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = ctx.dsvHeap->GetCPUDescriptorHandleForHeapStart();
    ctx.cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    
    const float clearColor[] = {0.05f, 0.05f, 0.08f, 1.0f};
    ctx.cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    ctx.cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    D3D12_VIEWPORT viewport = {0, 0, (float)ctx.width, (float)ctx.height, 0.0f, 1.0f};
    D3D12_RECT scissor = {0, 0, (LONG)ctx.width, (LONG)ctx.height};
    ctx.cmdList->RSSetViewports(1, &viewport);
    ctx.cmdList->RSSetScissorRects(1, &scissor);
    
    ID3D12DescriptorHeap* heaps[] = {ctx.srvHeap.Get()};
    ctx.cmdList->SetDescriptorHeaps(1, heaps);
    
    if (ctx.pipelineReady && !model.meshes.empty()) {
        ctx.cmdList->SetGraphicsRootSignature(ctx.rootSignature.Get());
        
        // Camera
        float eye[3] = {model.center[0] + sinf(time * 0.5f) * camDist, model.center[1] + camDist * 0.4f, model.center[2] + cosf(time * 0.5f) * camDist};
        float at[3] = {model.center[0], model.center[1], model.center[2]};
        float up[3] = {0, 1, 0};
        float world[16], view[16], proj[16], wvp[16];
        math::identity(world);
        math::lookAt(view, eye, at, up);
        math::perspective(proj, 3.14159f / 4.0f, (float)ctx.width / ctx.height, 0.01f, 1000.0f);
        math::multiply(wvp, world, view);
        math::multiply(wvp, wvp, proj);
        
        memcpy(ctx.sceneConstants.worldViewProj, wvp, sizeof(wvp));
        memcpy(ctx.sceneConstants.world, world, sizeof(world));
        // lightDirAndTex: xyz = lightDir, w = hasTexture (set per mesh)
        ctx.sceneConstants.lightDirAndTex[0] = -0.5f;
        ctx.sceneConstants.lightDirAndTex[1] = -1.0f;
        ctx.sceneConstants.lightDirAndTex[2] = 0.5f;
        // cameraPosAndMetal: xyz = cameraPos, w = metallic (per mesh)
        ctx.sceneConstants.cameraPosAndMetal[0] = eye[0];
        ctx.sceneConstants.cameraPosAndMetal[1] = eye[1];
        ctx.sceneConstants.cameraPosAndMetal[2] = eye[2];
        
        ctx.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx.cmdList->SetGraphicsRootConstantBufferView(0, ctx.constantBuffer->GetGPUVirtualAddress());
        
        static bool firstFrame = true;
        for (size_t i = 0; i < model.meshes.size(); ++i) {
            const auto& mesh = model.meshes[i];
            
            // Pack texture flags: bit 0 = diffuse, bit 1 = normal, bit 2 = specular
            int texFlags = 0;
            if (mesh.hasDiffuseTexture) texFlags |= 1;
            if (mesh.hasNormalTexture) texFlags |= 2;
            if (mesh.hasSpecularTexture) texFlags |= 4;
            
            // Set per-mesh material parameters
            ctx.sceneConstants.lightDirAndTex[3] = (float)texFlags;
            ctx.sceneConstants.cameraPosAndMetal[3] = mesh.metallic;
            ctx.sceneConstants.baseColorAndRough[0] = mesh.baseColor[0];
            ctx.sceneConstants.baseColorAndRough[1] = mesh.baseColor[1];
            ctx.sceneConstants.baseColorAndRough[2] = mesh.baseColor[2];
            ctx.sceneConstants.baseColorAndRough[3] = mesh.roughness;
            
            if (firstFrame) {
                std::cout << "[render] Mesh " << i << ": texFlags=" << texFlags 
                          << ", metallic=" << mesh.metallic << ", roughness=" << mesh.roughness << std::endl;
            }
            void* mapped;
            ctx.constantBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, &ctx.sceneConstants, sizeof(ctx.sceneConstants));
            ctx.constantBuffer->Unmap(0, nullptr);
            
            // Bind textures - use diffuse SRV as base (they should be consecutive)
            D3D12_GPU_DESCRIPTOR_HANDLE srvGpu = ctx.srvHeap->GetGPUDescriptorHandleForHeapStart();
            srvGpu.ptr += mesh.diffuseSrvIndex * ctx.srvDescriptorSize;
            ctx.cmdList->SetGraphicsRootDescriptorTable(1, srvGpu);
            
            ctx.cmdList->IASetVertexBuffers(0, 1, &mesh.vbv);
            ctx.cmdList->IASetIndexBuffer(&mesh.ibv);
            ctx.cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
        }
        firstFrame = false;
    }
    
    // ImGui
    if (ImDrawData* dd = ImGui::GetDrawData()) {
        ImGui_ImplDX12_RenderDrawData(dd, ctx.cmdList.Get());
    }
    
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    ctx.cmdList->ResourceBarrier(1, &barrier);
    CheckHR(ctx.cmdList->Close(), "CmdList Close");
    
    ID3D12CommandList* lists[] = {ctx.cmdList.Get()};
    ctx.queue->ExecuteCommandLists(1, lists);
    CheckHR(ctx.swapchain->Present(1, 0), "Present");
    WaitForGPU(ctx);
}

static std::string OpenFileDialog(HWND hwnd) {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn{}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = luma::get_file_filter();
    ofn.lpstrFile = filename; ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return filename;
    return "";
}

static void LoadModel(DxContext& ctx, ModelState& state, const std::string& path) {
    auto result = luma::load_model(path);
    if (!result) return;
    
    state.meshes.clear();
    state.textureCount = 0;
    std::cout << "[luma] Creating GPU meshes..." << std::endl;
    for (size_t i = 0; i < result->meshes.size(); ++i) {
        const auto& mesh = result->meshes[i];
        std::cout << "[luma] Mesh " << i << ": hasDiffuseTexture=" << mesh.hasDiffuseTexture 
                  << ", vertices=" << mesh.vertices.size() 
                  << ", indices=" << mesh.indices.size() << std::endl;
        state.meshes.push_back(CreateMeshGPU(ctx, mesh));
        if (mesh.hasDiffuseTexture) state.textureCount++;
    }
    
    state.center[0] = (result->minBounds[0] + result->maxBounds[0]) * 0.5f;
    state.center[1] = (result->minBounds[1] + result->maxBounds[1]) * 0.5f;
    state.center[2] = (result->minBounds[2] + result->maxBounds[2]) * 0.5f;
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];
    float dz = result->maxBounds[2] - result->minBounds[2];
    state.radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
    if (state.radius < 0.01f) state.radius = 1.0f;
    
    state.name = result->name;
    state.totalVerts = result->totalVertices;
    state.totalTris = result->totalTriangles;
    
    std::cout << "[luma] Model loaded: " << state.name << " (" << state.meshes.size() << " meshes)" << std::endl;
    for (size_t i = 0; i < state.meshes.size(); ++i) {
        const auto& m = state.meshes[i];
        std::cout << "[luma] GPU Mesh " << i << ": diffuse=" << m.hasDiffuseTexture 
                  << ", normal=" << m.hasNormalTexture 
                  << ", specular=" << m.hasSpecularTexture << std::endl;
    }
}

static int RunApp() {
    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    luma::EngineFacade engine;
    
    WNDCLASSEXW wc{sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"LumaImGui", nullptr};
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Luma Creator - PBR Model Viewer", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    
    DxContext ctx; ctx.hwnd = hwnd;
    RECT rc; GetClientRect(hwnd, &rc);
    ctx.width = rc.right - rc.left; ctx.height = rc.bottom - rc.top;
    
    CreateDevice(ctx);
    CreateSwapchain(ctx);
    CreateHeaps(ctx);
    CreateCommands(ctx);
    CreateDefaultTexture(ctx);
    CreatePipeline(ctx);
    
    // Initialize default material parameters
    ctx.sceneConstants.cameraPosAndMetal[3] = 0.0f;   // metallic
    ctx.sceneConstants.baseColorAndRough[3] = 0.5f;   // roughness
    
    // Default cube
    ModelState model;
    luma::Mesh cube = luma::create_cube();
    model.meshes.push_back(CreateMeshGPU(ctx, cube));
    model.totalVerts = cube.vertices.size();
    model.totalTris = cube.indices.size() / 3;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(ctx.device.Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, ctx.srvHeap.Get(),
        ctx.srvHeap->GetCPUDescriptorHandleForHeapStart(), ctx.srvHeap->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplDX12_CreateDeviceObjects();
    
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    float camDistMult = 2.5f;
    bool autoRotate = true;
    float manualTime = 0.0f;
    
    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            continue;
        }
        
        float elapsed = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startTime).count();
        float time = autoRotate ? elapsed : manualTime;
        
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        if (ImGui::Begin("Model")) {
            ImGui::Text("File: %s", model.name.c_str());
            ImGui::Separator();
            ImGui::Text("Meshes: %zu", model.meshes.size());
            ImGui::Text("Vertices: %zu", model.totalVerts);
            ImGui::Text("Triangles: %zu", model.totalTris);
            ImGui::Text("Textures: %d", model.textureCount);
            ImGui::Separator();
            if (ImGui::Button("Open Model...")) {
                std::string path = OpenFileDialog(hwnd);
                if (!path.empty()) LoadModel(ctx, model, path);
            }
            if (ImGui::Button("Reset to Cube")) {
                model.meshes.clear();
                luma::Mesh cube = luma::create_cube();
                model.meshes.push_back(CreateMeshGPU(ctx, cube));
                model.center[0] = model.center[1] = model.center[2] = 0;
                model.radius = 1.0f;
                model.name = "Default Cube";
                model.totalVerts = 24; model.totalTris = 12;
                model.textureCount = 0;
            }
        }
        ImGui::End();
        
        if (ImGui::Begin("Camera")) {
            ImGui::Checkbox("Auto Rotate", &autoRotate);
            if (!autoRotate) ImGui::SliderFloat("Rotation", &manualTime, 0, 20);
            ImGui::SliderFloat("Distance", &camDistMult, 0.5f, 10.0f);
        }
        ImGui::End();
        
        if (ImGui::Begin("Materials")) {
            ImGui::Text("Materials are loaded from model.");
            if (!model.meshes.empty()) {
                for (size_t i = 0; i < model.meshes.size(); ++i) {
                    const auto& m = model.meshes[i];
                    ImGui::Text("Mesh %zu: M=%.2f R=%.2f", i, m.metallic, m.roughness);
                }
            }
        }
        ImGui::End();
        
        ImGui::Render();
        Render(ctx, model, time, model.radius * camDistMult);
    }
    
    WaitForGPU(ctx);
    CloseHandle(ctx.fenceEvent);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

static LONG WINAPI CrashFilter(_In_ struct _EXCEPTION_POINTERS* info) {
    DWORD code = info ? info->ExceptionRecord->ExceptionCode : 0;
    std::cerr << "[luma] SEH exception. Code=0x" << std::hex << code << std::dec << std::endl;
    MessageBoxA(nullptr, ("SEH exception code=0x" + std::to_string(code)).c_str(), "Crash", MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main() {
    SetUnhandledExceptionFilter(CrashFilter);
    try { return RunApp(); }
    catch (const std::exception& e) {
        std::cerr << "[luma] Exception: " << e.what() << std::endl;
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}
#else
int main() { return 0; }
#endif
