// PBR Renderer Implementation (DX12)
#include "pbr_renderer.h"
#include "engine/asset/model_loader.h"

#include <iostream>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <d3dcompiler.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace luma {

// ===== PBR Shader =====
static const char* kPBRShaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4 lightDirAndFlags;
    float4 cameraPosAndMetal;
    float4 baseColorAndRough;
};

#define lightDir lightDirAndFlags.xyz
#define cameraPos cameraPosAndMetal.xyz
#define metallic cameraPosAndMetal.w
#define baseColor baseColorAndRough.xyz
#define roughness baseColorAndRough.w

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);
SamplerState texSampler : register(s0);

static const float PI = 3.14159265359;

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
    // Sample textures
    float4 diffuseSample = diffuseTexture.Sample(texSampler, input.uv);
    float4 normalSample = normalTexture.Sample(texSampler, input.uv);
    float4 specularSample = specularTexture.Sample(texSampler, input.uv);
    
    // Alpha test
    if (diffuseSample.a < 0.1) discard;
    
    // === Albedo ===
    float3 albedo;
    float texBrightness = diffuseSample.r + diffuseSample.g + diffuseSample.b;
    if (texBrightness < 2.9) {
        albedo = diffuseSample.rgb;
    } else {
        albedo = input.color * baseColor;
    }
    
    // === Normal Mapping ===
    float3 N;
    bool hasNormalMap = (abs(normalSample.r - normalSample.g) > 0.01 || 
                         abs(normalSample.b - 1.0) > 0.1);
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
    
    // === PBR Parameters ===
    float metal = metallic;
    float rough = roughness;
    bool hasSpecMap = (specularSample.r < 0.99 || specularSample.g < 0.99);
    if (hasSpecMap) {
        metal = specularSample.b;
        rough = specularSample.g;
    }
    rough = clamp(rough, 0.04, 1.0);
    
    // === Vectors ===
    float3 V = normalize(cameraPos - input.worldPos);
    float3 L = normalize(-lightDir);
    float3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // === F0 ===
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metal);
    
    // === Cook-Torrance BRDF ===
    // D - GGX
    float a = rough * rough;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * denom * denom + 0.0001);
    
    // G - Smith
    float k = (rough + 1.0) * (rough + 1.0) / 8.0;
    float G1_V = NdotV / (NdotV * (1.0 - k) + k);
    float G1_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G1_V * G1_L;
    
    // F - Fresnel
    float3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    
    // Specular
    float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    
    // Diffuse
    float3 kD = (1.0 - F) * (1.0 - metal);
    float3 diffuse = kD * albedo / PI;
    
    // === Direct Lighting ===
    float3 lightColor = float3(1.0, 0.98, 0.95) * 2.5;
    float3 Lo = (diffuse + specular) * NdotL * lightColor;
    
    // === Ambient (simple hemisphere) ===
    float3 skyColor = float3(0.5, 0.6, 0.8);
    float3 groundColor = float3(0.3, 0.25, 0.2);
    float3 ambientColor = lerp(groundColor, skyColor, N.y * 0.5 + 0.5);
    float3 ambient = albedo * ambientColor * 0.25;
    
    // === Final ===
    float3 color = ambient + Lo;
    
    // Tone mapping (ACES)
    float a_tm = 2.51; float b_tm = 0.03; float c_tm = 2.43; float d_tm = 0.59; float e_tm = 0.14;
    color = saturate((color * (a_tm * color + b_tm)) / (color * (c_tm * color + d_tm) + e_tm));
    
    return float4(color, 1.0);
}
)";

// ===== Line Shader (for grid and axes) =====
static const char* kLineShaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float4 unused1;
    float4 unused2;
    float4 unused3;
};

struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.color;
}
)";

// ===== Math helpers =====
namespace math {
    inline void identity(float* m) {
        memset(m, 0, 64);
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    inline void multiply(float* out, const float* a, const float* b) {
        float tmp[16];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                tmp[i*4+j] = a[i*4+0]*b[0*4+j] + a[i*4+1]*b[1*4+j] + a[i*4+2]*b[2*4+j] + a[i*4+3]*b[3*4+j];
            }
        }
        memcpy(out, tmp, 64);
    }
    inline void lookAt(float* m, const float* eye, const float* at, const float* up) {
        float zAxis[3] = {eye[0]-at[0], eye[1]-at[1], eye[2]-at[2]};
        float len = sqrtf(zAxis[0]*zAxis[0] + zAxis[1]*zAxis[1] + zAxis[2]*zAxis[2]);
        zAxis[0]/=len; zAxis[1]/=len; zAxis[2]/=len;
        float xAxis[3] = {up[1]*zAxis[2]-up[2]*zAxis[1], up[2]*zAxis[0]-up[0]*zAxis[2], up[0]*zAxis[1]-up[1]*zAxis[0]};
        len = sqrtf(xAxis[0]*xAxis[0] + xAxis[1]*xAxis[1] + xAxis[2]*xAxis[2]);
        xAxis[0]/=len; xAxis[1]/=len; xAxis[2]/=len;
        float yAxis[3] = {zAxis[1]*xAxis[2]-zAxis[2]*xAxis[1], zAxis[2]*xAxis[0]-zAxis[0]*xAxis[2], zAxis[0]*xAxis[1]-zAxis[1]*xAxis[0]};
        m[0]=xAxis[0]; m[1]=yAxis[0]; m[2]=zAxis[0]; m[3]=0;
        m[4]=xAxis[1]; m[5]=yAxis[1]; m[6]=zAxis[1]; m[7]=0;
        m[8]=xAxis[2]; m[9]=yAxis[2]; m[10]=zAxis[2]; m[11]=0;
        m[12]=-(xAxis[0]*eye[0]+xAxis[1]*eye[1]+xAxis[2]*eye[2]);
        m[13]=-(yAxis[0]*eye[0]+yAxis[1]*eye[1]+yAxis[2]*eye[2]);
        m[14]=-(zAxis[0]*eye[0]+zAxis[1]*eye[1]+zAxis[2]*eye[2]); m[15]=1;
    }
    inline void perspective(float* m, float fov, float aspect, float nearZ, float farZ) {
        float tanHalfFov = tanf(fov / 2.0f);
        memset(m, 0, 64);
        m[0] = 1.0f / (aspect * tanHalfFov);
        m[5] = 1.0f / tanHalfFov;
        m[10] = farZ / (nearZ - farZ);
        m[11] = -1.0f;
        m[14] = (nearZ * farZ) / (nearZ - farZ);
    }
}

// ===== Renderer implementation =====
struct PBRRenderer::Impl {
    // Core DX12 objects
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ComPtr<ID3D12Resource> renderTargets[2];
    ComPtr<ID3D12Resource> depthBuffer;
    ComPtr<ID3D12CommandAllocator> allocators[2];
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12Resource> constantBuffer;
    ComPtr<ID3D12Resource> defaultTexture;
    HANDLE fenceEvent = nullptr;
    UINT64 fenceValue = 1;
    UINT frameIndex = 0;
    UINT rtvDescSize = 0;
    UINT srvDescSize = 0;
    UINT width = 0;
    UINT height = 0;
    UINT defaultTextureSrvIndex = 0;
    UINT nextSrvIndex = 1;  // Reserve 0 for ImGui font
    SceneConstants constants{};
    bool ready = false;
    
    // Grid rendering
    ComPtr<ID3D12PipelineState> linePipelineState;
    ComPtr<ID3D12Resource> gridVertexBuffer;
    ComPtr<ID3D12Resource> axisVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW gridVbv{};
    D3D12_VERTEX_BUFFER_VIEW axisVbv{};
    UINT gridVertexCount = 0;
    UINT axisVertexCount = 0;
    bool gridReady = false;

    void waitForGPU() {
        queue->Signal(fence.Get(), fenceValue);
        fence->SetEventOnCompletion(fenceValue++, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    
    void createDefaultTexture() {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = 1; texDesc.Height = 1; texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; texDesc.SampleDesc.Count = 1;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultTexture));
        
        D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = 256; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> uploadBuf;
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf));
        uint8_t white[4] = {255, 255, 255, 255};
        void* mapped; uploadBuf->Map(0, nullptr, &mapped);
        memcpy(mapped, white, 4);
        uploadBuf->Unmap(0, nullptr);
        
        cmdList->Reset(allocators[0].Get(), nullptr);
        D3D12_TEXTURE_COPY_LOCATION dst{}, src{};
        dst.pResource = defaultTexture.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource = uploadBuf.Get(); src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);
        src.PlacedFootprint = footprint;
        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = defaultTexture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        cmdList->Close();
        ID3D12CommandList* lists[] = {cmdList.Get()};
        queue->ExecuteCommandLists(1, lists);
        waitForGPU();

        defaultTextureSrvIndex = nextSrvIndex++;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += defaultTextureSrvIndex * srvDescSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(defaultTexture.Get(), &srvViewDesc, srvHandle);
    }
    
    void createPipeline() {
        // Root signature
        D3D12_DESCRIPTOR_RANGE srvRanges[3]{};
        srvRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; srvRanges[0].NumDescriptors = 1; srvRanges[0].BaseShaderRegister = 0;
        srvRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; srvRanges[1].NumDescriptors = 1; srvRanges[1].BaseShaderRegister = 1;
        srvRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; srvRanges[2].NumDescriptors = 1; srvRanges[2].BaseShaderRegister = 2;

        D3D12_ROOT_PARAMETER rootParams[4]{};
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        for (int i = 0; i < 3; i++) {
            rootParams[i+1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[i+1].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[i+1].DescriptorTable.pDescriptorRanges = &srvRanges[i];
            rootParams[i+1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        }
        
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MaxAnisotropy = 16;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        
        D3D12_ROOT_SIGNATURE_DESC rsDesc{}; rsDesc.NumParameters = 4; rsDesc.pParameters = rootParams;
        rsDesc.NumStaticSamplers = 1; rsDesc.pStaticSamplers = &sampler;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        
        // Compile shaders
        ComPtr<ID3DBlob> vs, ps, errorBlob;
        UINT flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        if (FAILED(D3DCompile(kPBRShaderSource, strlen(kPBRShaderSource), "pbr.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob))) {
            if (errorBlob) std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        if (FAILED(D3DCompile(kPBRShaderSource, strlen(kPBRShaderSource), "pbr.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob))) {
            if (errorBlob) std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        
        // Input layout
        // Vertex layout: position[3], normal[3], tangent[4], uv[2], color[3]
        // Offsets:       0,           12,        24,         40,    48
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
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
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        
        // Constant buffer
        D3D12_HEAP_PROPERTIES cbHeap{}; cbHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC cbDesc{}; cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbDesc.Width = sizeof(SceneConstants); cbDesc.Height = 1; cbDesc.DepthOrArraySize = 1;
        cbDesc.MipLevels = 1; cbDesc.SampleDesc.Count = 1; cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        device->CreateCommittedResource(&cbHeap, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer));
        
        // Create line pipeline for grid rendering
        createLinePipeline();
        
        ready = true;
        std::cout << "[luma] PBR Pipeline ready" << std::endl;
    }
    
    void createLinePipeline() {
        ComPtr<ID3DBlob> vs, ps, errorBlob;
        UINT flags = 0;
        #ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #endif
        
        if (FAILED(D3DCompile(kLineShaderSource, strlen(kLineShaderSource), "line.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", flags, 0, &vs, &errorBlob))) {
            if (errorBlob) std::cerr << "[luma] Line VS error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        if (FAILED(D3DCompile(kLineShaderSource, strlen(kLineShaderSource), "line.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", flags, 0, &ps, &errorBlob))) {
            if (errorBlob) std::cerr << "[luma] Line PS error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            return;
        }
        
        // Line vertex: position[3], color[4]
        D3D12_INPUT_ELEMENT_DESC lineLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = {lineLayout, _countof(lineLayout)};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
        psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Don't write depth for grid
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&linePipelineState));
        
        // Create grid vertex buffer
        createGridVertexBuffer(10.0f, 20);
    }
    
    void createGridVertexBuffer(float size, int divisions) {
        struct LineVertex { float pos[3]; float color[4]; };
        std::vector<LineVertex> vertices;
        
        // Create a very large grid (1000 units each direction)
        // Major lines every 100 units, minor lines every 10 units
        float gridExtent = 1000.0f;
        float gridColor[4] = {0.25f, 0.25f, 0.28f, 0.4f};     // Minor lines (every 10)
        float majorColor[4] = {0.35f, 0.35f, 0.4f, 0.6f};     // Major lines (every 100)
        
        // Grid lines parallel to X axis (along Z) - every 10 units
        for (int i = -100; i <= 100; i++) {
            float z = (float)i * 10.0f;  // Spacing of 10 units
            float* col = (i % 10 == 0) ? majorColor : gridColor;
            if (i == 0) continue;  // Skip center line (will be drawn as axis)
            vertices.push_back({{-gridExtent, 0, z}, {col[0], col[1], col[2], col[3]}});
            vertices.push_back({{gridExtent, 0, z}, {col[0], col[1], col[2], col[3]}});
        }
        
        // Grid lines parallel to Z axis (along X) - every 10 units
        for (int i = -100; i <= 100; i++) {
            float x = (float)i * 10.0f;  // Spacing of 10 units
            float* col = (i % 10 == 0) ? majorColor : gridColor;
            if (i == 0) continue;  // Skip center line (will be drawn as axis)
            vertices.push_back({{x, 0, -gridExtent}, {col[0], col[1], col[2], col[3]}});
            vertices.push_back({{x, 0, gridExtent}, {col[0], col[1], col[2], col[3]}});
        }
        
        gridVertexCount = (UINT)vertices.size();
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = vertices.size() * sizeof(LineVertex);
        bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gridVertexBuffer));
        
        void* mapped;
        gridVertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, vertices.data(), vertices.size() * sizeof(LineVertex));
        gridVertexBuffer->Unmap(0, nullptr);
        
        gridVbv.BufferLocation = gridVertexBuffer->GetGPUVirtualAddress();
        gridVbv.SizeInBytes = (UINT)(vertices.size() * sizeof(LineVertex));
        gridVbv.StrideInBytes = sizeof(LineVertex);
        
        // Create axis vertex buffer separately
        createAxisVertexBuffer();
        
        gridReady = true;
        std::cout << "[luma] Grid ready (" << gridVertexCount << " lines)" << std::endl;
    }
    
    void createAxisVertexBuffer() {
        struct LineVertex { float pos[3]; float color[4]; };
        std::vector<LineVertex> vertices;
        
        // Axis lines - unit length (will be scaled in shader)
        // X axis - Red (positive and negative)
        vertices.push_back({{-1.0f, 0.001f, 0}, {0.5f, 0.15f, 0.15f, 0.8f}});  // Negative X (darker)
        vertices.push_back({{0, 0.001f, 0}, {0.5f, 0.15f, 0.15f, 0.8f}});
        vertices.push_back({{0, 0.001f, 0}, {0.9f, 0.2f, 0.2f, 1.0f}});        // Positive X (bright)
        vertices.push_back({{1.0f, 0.001f, 0}, {0.9f, 0.2f, 0.2f, 1.0f}});
        
        // Y axis - Green
        vertices.push_back({{0, 0, 0}, {0.2f, 0.9f, 0.2f, 1.0f}});
        vertices.push_back({{0, 1.0f, 0}, {0.2f, 0.9f, 0.2f, 1.0f}});
        
        // Z axis - Blue (positive and negative)
        vertices.push_back({{0, 0.001f, -1.0f}, {0.15f, 0.25f, 0.5f, 0.8f}});  // Negative Z (darker)
        vertices.push_back({{0, 0.001f, 0}, {0.15f, 0.25f, 0.5f, 0.8f}});
        vertices.push_back({{0, 0.001f, 0}, {0.2f, 0.4f, 0.9f, 1.0f}});        // Positive Z (bright)
        vertices.push_back({{0, 0.001f, 1.0f}, {0.2f, 0.4f, 0.9f, 1.0f}});
        
        axisVertexCount = (UINT)vertices.size();
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = vertices.size() * sizeof(LineVertex);
        bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, 
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&axisVertexBuffer));
        
        void* mapped;
        axisVertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, vertices.data(), vertices.size() * sizeof(LineVertex));
        axisVertexBuffer->Unmap(0, nullptr);
        
        axisVbv.BufferLocation = axisVertexBuffer->GetGPUVirtualAddress();
        axisVbv.SizeInBytes = (UINT)(vertices.size() * sizeof(LineVertex));
        axisVbv.StrideInBytes = sizeof(LineVertex);
    }
    
    ComPtr<ID3D12Resource> uploadTexture(const TextureData& tex, UINT& outSrvIndex) {
        if (tex.pixels.empty()) {
            outSrvIndex = defaultTextureSrvIndex;
            return defaultTexture;
        }
        
        const UINT w = tex.width, h = tex.height;
        
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texDesc{};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = w; texDesc.Height = h; texDesc.DepthOrArraySize = 1; texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; texDesc.SampleDesc.Count = 1;
        
        ComPtr<ID3D12Resource> texture;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
        
        D3D12_HEAP_PROPERTIES uploadHeap{}; uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        UINT rowPitch = (w * 4 + 255) & ~255;
        bufDesc.Width = rowPitch * h; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
        bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        ComPtr<ID3D12Resource> uploadBuf;
        device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf));
        
        void* mapped;
        uploadBuf->Map(0, nullptr, &mapped);
        for (UINT row = 0; row < h; row++) {
            memcpy((uint8_t*)mapped + row * rowPitch, tex.pixels.data() + row * w * 4, w * 4);
        }
        uploadBuf->Unmap(0, nullptr);
        
        // Wait for any pending GPU work before using command list for upload
        waitForGPU();
        allocators[0]->Reset();
        cmdList->Reset(allocators[0].Get(), nullptr);
        
        D3D12_TEXTURE_COPY_LOCATION dst{}, src{};
        dst.pResource = texture.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.pResource = uploadBuf.Get(); src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = w; src.PlacedFootprint.Footprint.Height = h;
        src.PlacedFootprint.Footprint.Depth = 1; src.PlacedFootprint.Footprint.RowPitch = rowPitch;
        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        cmdList->Close();
        
        ID3D12CommandList* lists[] = {cmdList.Get()};
        queue->ExecuteCommandLists(1, lists);
        waitForGPU();
        
        outSrvIndex = nextSrvIndex++;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandle.ptr += outSrvIndex * srvDescSize;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc{}; srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(texture.Get(), &srvViewDesc, srvHandle);
        
        return texture;
    }
    
    void createDepthBuffer() {
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC depthDesc{};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = width; depthDesc.Height = height; depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1; depthDesc.Format = DXGI_FORMAT_D32_FLOAT; depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE clearVal{}; clearVal.Format = DXGI_FORMAT_D32_FLOAT; clearVal.DepthStencil.Depth = 1.0f;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearVal, IID_PPV_ARGS(&depthBuffer));
        
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{}; dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
};

PBRRenderer::PBRRenderer() = default;

PBRRenderer::~PBRRenderer() {
    if (impl_) {
        impl_->waitForGPU();
        if (impl_->fenceEvent) CloseHandle(impl_->fenceEvent);
    }
}

PBRRenderer::PBRRenderer(PBRRenderer&&) noexcept = default;
PBRRenderer& PBRRenderer::operator=(PBRRenderer&&) noexcept = default;

bool PBRRenderer::initialize(void* windowHandle, uint32_t width, uint32_t height) {
    impl_ = std::make_unique<Impl>();
    impl_->width = width;
    impl_->height = height;
    
    HWND hwnd = static_cast<HWND>(windowHandle);
    
    // Enable debug layer
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
    }
#endif
    
    // Create factory and find GPU
    ComPtr<IDXGIFactory6> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&impl_->device)))) {
                std::wcout << L"[luma] GPU: " << desc.Description << std::endl;
                break;
            }
        }
    }
    if (!impl_->device) return false;
    
    // Command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    impl_->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&impl_->queue));
    
    // Swapchain
    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = width; scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForHwnd(impl_->queue.Get(), hwnd, &scDesc, nullptr, nullptr, &sc1);
    sc1.As(&impl_->swapchain);
    impl_->frameIndex = impl_->swapchain->GetCurrentBackBufferIndex();
    
    // Descriptor heaps
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{}; rtvHeapDesc.NumDescriptors = 2; rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    impl_->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&impl_->rtvHeap));
    impl_->rtvDescSize = impl_->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{}; dsvHeapDesc.NumDescriptors = 1; dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    impl_->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&impl_->dsvHeap));
    
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{}; srvHeapDesc.NumDescriptors = 256; srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    impl_->device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&impl_->srvHeap));
    impl_->srvDescSize = impl_->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++) {
        impl_->swapchain->GetBuffer(i, IID_PPV_ARGS(&impl_->renderTargets[i]));
        impl_->device->CreateRenderTargetView(impl_->renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += impl_->rtvDescSize;
    }
    
    // Command allocators and list
    for (UINT i = 0; i < 2; i++) {
        impl_->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&impl_->allocators[i]));
    }
    impl_->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, impl_->allocators[0].Get(), nullptr, IID_PPV_ARGS(&impl_->cmdList));
    impl_->cmdList->Close();
    
    // Fence
    impl_->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&impl_->fence));
    impl_->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    
    // Depth buffer
    impl_->createDepthBuffer();
    
    // Default texture and pipeline
    impl_->createDefaultTexture();
    impl_->createPipeline();
    
    return true;
}

void PBRRenderer::resize(uint32_t width, uint32_t height) {
    if (!impl_ || width == 0 || height == 0) return;
    
    impl_->waitForGPU();
    impl_->width = width;
    impl_->height = height;
    
    for (UINT i = 0; i < 2; i++) impl_->renderTargets[i].Reset();
    impl_->depthBuffer.Reset();
    
    impl_->swapchain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    impl_->frameIndex = impl_->swapchain->GetCurrentBackBufferIndex();
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++) {
        impl_->swapchain->GetBuffer(i, IID_PPV_ARGS(&impl_->renderTargets[i]));
        impl_->device->CreateRenderTargetView(impl_->renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += impl_->rtvDescSize;
    }
    
    impl_->createDepthBuffer();
}

GPUMesh PBRRenderer::uploadMesh(const Mesh& mesh) {
    GPUMesh gpu;
    gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
    memcpy(gpu.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
    gpu.metallic = mesh.metallic;
    gpu.roughness = mesh.roughness;
    
    const UINT vbSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));
    const UINT ibSize = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
    
    // Vertex buffer
    D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc{}; bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = vbSize; bufDesc.Height = 1; bufDesc.DepthOrArraySize = 1; bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1; bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu.vertexBuffer));
    void* mapped;
    gpu.vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, mesh.vertices.data(), vbSize);
    gpu.vertexBuffer->Unmap(0, nullptr);
    gpu.vbv.BufferLocation = gpu.vertexBuffer->GetGPUVirtualAddress();
    gpu.vbv.SizeInBytes = vbSize;
    gpu.vbv.StrideInBytes = sizeof(Vertex);
    
    // Index buffer
    bufDesc.Width = ibSize;
    impl_->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&gpu.indexBuffer));
    gpu.indexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, mesh.indices.data(), ibSize);
    gpu.indexBuffer->Unmap(0, nullptr);
    gpu.ibv.BufferLocation = gpu.indexBuffer->GetGPUVirtualAddress();
    gpu.ibv.SizeInBytes = ibSize;
    gpu.ibv.Format = DXGI_FORMAT_R32_UINT;
    
    // Textures
    gpu.diffuseTexture = impl_->uploadTexture(mesh.diffuseTexture, gpu.diffuseSrvIndex);
    gpu.hasDiffuseTexture = !mesh.diffuseTexture.pixels.empty();
    gpu.normalTexture = impl_->uploadTexture(mesh.normalTexture, gpu.normalSrvIndex);
    gpu.hasNormalTexture = !mesh.normalTexture.pixels.empty();
    gpu.specularTexture = impl_->uploadTexture(mesh.specularTexture, gpu.specularSrvIndex);
    gpu.hasSpecularTexture = !mesh.specularTexture.pixels.empty();
    
    return gpu;
}

bool PBRRenderer::loadModel(const std::string& path, LoadedModel& outModel) {
    std::cout << "[luma] Loading model via renderer: " << path << std::endl;
    
    auto result = load_model(path);
    if (!result) {
        std::cerr << "[luma] Failed to load model" << std::endl;
        return false;
    }
    
    std::cout << "[luma] Creating GPU meshes..." << std::endl;
    outModel.meshes.clear();
    outModel.textureCount = 0;
    
    for (size_t i = 0; i < result->meshes.size(); ++i) {
        const auto& mesh = result->meshes[i];
        std::cout << "[luma] Uploading mesh " << i << " (" << mesh.vertices.size() << " verts)" << std::endl;
        outModel.meshes.push_back(uploadMesh(mesh));
        if (!mesh.diffuseTexture.pixels.empty()) outModel.textureCount++;
    }
    
    outModel.center[0] = (result->minBounds[0] + result->maxBounds[0]) / 2.0f;
    outModel.center[1] = (result->minBounds[1] + result->maxBounds[1]) / 2.0f;
    outModel.center[2] = (result->minBounds[2] + result->maxBounds[2]) / 2.0f;
    
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];
    float dz = result->maxBounds[2] - result->minBounds[2];
    outModel.radius = sqrtf(dx*dx + dy*dy + dz*dz) / 2.0f;
    
    outModel.name = path.substr(path.find_last_of("/\\") + 1);
    outModel.totalVerts = result->totalVertices;
    outModel.totalTris = result->totalTriangles;
    
    std::cout << "[luma] Model loaded: " << outModel.name << " (" << outModel.meshes.size() << " meshes)" << std::endl;
    return true;
}

void PBRRenderer::beginFrame() {
    impl_->frameIndex = impl_->swapchain->GetCurrentBackBufferIndex();
    impl_->allocators[impl_->frameIndex]->Reset();
    impl_->cmdList->Reset(impl_->allocators[impl_->frameIndex].Get(), nullptr);
    
    ID3D12DescriptorHeap* heaps[] = {impl_->srvHeap.Get()};
    impl_->cmdList->SetDescriptorHeaps(1, heaps);
    
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = impl_->renderTargets[impl_->frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    impl_->cmdList->ResourceBarrier(1, &barrier);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = impl_->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += impl_->frameIndex * impl_->rtvDescSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = impl_->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    impl_->cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    
    float clearColor[] = {0.05f, 0.05f, 0.08f, 1.0f};
    impl_->cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    impl_->cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    D3D12_VIEWPORT vp{}; vp.Width = (float)impl_->width; vp.Height = (float)impl_->height; vp.MaxDepth = 1.0f;
    D3D12_RECT sr{}; sr.right = impl_->width; sr.bottom = impl_->height;
    impl_->cmdList->RSSetViewports(1, &vp);
    impl_->cmdList->RSSetScissorRects(1, &sr);
}

void PBRRenderer::render(const LoadedModel& model, float time, float camDistMultiplier) {
    // Legacy render - convert to CameraParams
    CameraParams cam;
    cam.yaw = time * 0.5f;
    cam.pitch = 0.3f;
    cam.distance = camDistMultiplier;
    cam.targetOffsetX = cam.targetOffsetY = cam.targetOffsetZ = 0.0f;
    render(model, cam);
}

void PBRRenderer::render(const LoadedModel& model, const CameraParams& camera) {
    if (!impl_->ready || model.meshes.empty()) return;
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->pipelineState.Get());
    
    // Calculate target point (model center + pan offset)
    float target[3];
    target[0] = model.center[0] + camera.targetOffsetX;
    target[1] = model.center[1] + camera.targetOffsetY;
    target[2] = model.center[2] + camera.targetOffsetZ;
    
    // Calculate camera position orbiting around target
    float camDist = model.radius * 2.5f * camera.distance;
    float eye[3];
    eye[0] = target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist;
    eye[1] = target[1] + sinf(camera.pitch) * camDist;
    eye[2] = target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist;
    
    float at[3] = {target[0], target[1], target[2]};
    float up[3] = {0, 1, 0};
    
    float world[16], view[16], proj[16], wvp[16];
    math::identity(world);
    math::lookAt(view, eye, at, up);
    // Dynamic near/far planes based on camera distance
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(proj, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    // Fixed light direction: from upper-left-front (like sun at 10am)
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    
    impl_->constants.cameraPosAndMetal[0] = eye[0];
    impl_->constants.cameraPosAndMetal[1] = eye[1];
    impl_->constants.cameraPosAndMetal[2] = eye[2];
    
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, impl_->constantBuffer->GetGPUVirtualAddress());
    
    for (const auto& mesh : model.meshes) {
        impl_->constants.cameraPosAndMetal[3] = mesh.metallic;
        impl_->constants.baseColorAndRough[0] = mesh.baseColor[0];
        impl_->constants.baseColorAndRough[1] = mesh.baseColor[1];
        impl_->constants.baseColorAndRough[2] = mesh.baseColor[2];
        impl_->constants.baseColorAndRough[3] = mesh.roughness;
        
        void* mapped;
        impl_->constantBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, &impl_->constants, sizeof(impl_->constants));
        impl_->constantBuffer->Unmap(0, nullptr);
        
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = impl_->srvHeap->GetGPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE srv;
        
        srv = srvGpuStart; srv.ptr += mesh.diffuseSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(1, srv);
        srv = srvGpuStart; srv.ptr += mesh.normalSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(2, srv);
        srv = srvGpuStart; srv.ptr += mesh.specularSrvIndex * impl_->srvDescSize;
        impl_->cmdList->SetGraphicsRootDescriptorTable(3, srv);
        
        impl_->cmdList->IASetVertexBuffers(0, 1, &mesh.vbv);
        impl_->cmdList->IASetIndexBuffer(&mesh.ibv);
        impl_->cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
    }
}

void PBRRenderer::renderGrid(const CameraParams& camera, float modelRadius) {
    if (!impl_->ready || !impl_->gridReady) return;
    
    // Calculate camera position
    float target[3] = {camera.targetOffsetX, camera.targetOffsetY, camera.targetOffsetZ};
    float camDist = modelRadius * 2.5f * camera.distance;
    float eye[3];
    eye[0] = target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist;
    eye[1] = target[1] + sinf(camera.pitch) * camDist;
    eye[2] = target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist;
    
    float up[3] = {0, 1, 0};
    float world[16], view[16], proj[16], wvp[16];
    
    math::lookAt(view, eye, target, up);
    // Dynamic near/far planes based on camera distance
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(proj, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    
    impl_->cmdList->SetGraphicsRootSignature(impl_->rootSignature.Get());
    impl_->cmdList->SetPipelineState(impl_->linePipelineState.Get());
    impl_->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    
    // 1. Render grid (identity transform - fixed in world space)
    math::identity(world);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    void* mapped;
    impl_->constantBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, &impl_->constants, sizeof(impl_->constants));
    impl_->constantBuffer->Unmap(0, nullptr);
    
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, impl_->constantBuffer->GetGPUVirtualAddress());
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->gridVbv);
    impl_->cmdList->DrawInstanced(impl_->gridVertexCount, 1, 0, 0);
    
    // 2. Render axes (scaled based on view distance for visibility)
    // Axes should be visible regardless of zoom level
    float axisScale = std::max(camDist * 0.3f, modelRadius * 1.5f);
    axisScale = std::max(axisScale, 10.0f);  // Minimum 10 units
    
    memset(world, 0, sizeof(world));
    world[0] = axisScale;
    world[5] = axisScale;
    world[10] = axisScale;
    world[15] = 1.0f;
    
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    impl_->constantBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, &impl_->constants, sizeof(impl_->constants));
    impl_->constantBuffer->Unmap(0, nullptr);
    
    impl_->cmdList->SetGraphicsRootConstantBufferView(0, impl_->constantBuffer->GetGPUVirtualAddress());
    impl_->cmdList->IASetVertexBuffers(0, 1, &impl_->axisVbv);
    impl_->cmdList->DrawInstanced(impl_->axisVertexCount, 1, 0, 0);
}

void PBRRenderer::endFrame() {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = impl_->renderTargets[impl_->frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    impl_->cmdList->ResourceBarrier(1, &barrier);
    impl_->cmdList->Close();
    
    ID3D12CommandList* lists[] = {impl_->cmdList.Get()};
    impl_->queue->ExecuteCommandLists(1, lists);
    impl_->swapchain->Present(1, 0);
    
    impl_->queue->Signal(impl_->fence.Get(), impl_->fenceValue);
    impl_->fence->SetEventOnCompletion(impl_->fenceValue++, impl_->fenceEvent);
    WaitForSingleObject(impl_->fenceEvent, INFINITE);
}

void* PBRRenderer::getDevice() const { return impl_->device.Get(); }
void* PBRRenderer::getCommandList() const { return impl_->cmdList.Get(); }
void* PBRRenderer::getSrvHeap() const { return impl_->srvHeap.Get(); }
uint32_t PBRRenderer::getSrvDescriptorSize() const { return impl_->srvDescSize; }

void PBRRenderer::waitForGPU() { if (impl_) impl_->waitForGPU(); }

}  // namespace luma

#else  // Non-Windows stub

namespace luma {
PBRRenderer::PBRRenderer() = default;
PBRRenderer::~PBRRenderer() = default;
PBRRenderer::PBRRenderer(PBRRenderer&&) noexcept = default;
PBRRenderer& PBRRenderer::operator=(PBRRenderer&&) noexcept = default;
bool PBRRenderer::initialize(void*, uint32_t, uint32_t) { return false; }
void PBRRenderer::resize(uint32_t, uint32_t) {}
GPUMesh PBRRenderer::uploadMesh(const Mesh&) { return {}; }
bool PBRRenderer::loadModel(const std::string&, LoadedModel&) { return false; }
void PBRRenderer::beginFrame() {}
void PBRRenderer::render(const LoadedModel&, float, float) {}
void PBRRenderer::render(const LoadedModel&, const CameraParams&) {}
void PBRRenderer::renderGrid(const CameraParams&, float) {}
void PBRRenderer::endFrame() {}
void* PBRRenderer::getDevice() const { return nullptr; }
void* PBRRenderer::getCommandList() const { return nullptr; }
void* PBRRenderer::getSrvHeap() const { return nullptr; }
uint32_t PBRRenderer::getSrvDescriptorSize() const { return 0; }
void PBRRenderer::waitForGPU() {}
}

#endif
