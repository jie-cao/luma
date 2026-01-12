#ifdef _WIN32
#include "dx12_backend.h"

#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "engine/foundation/log.h"

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace {

inline void ThrowIfFailed(HRESULT hr, std::string_view where) {
    if (FAILED(hr)) {
        std::string msg = std::string("DX12 failure at ") + std::string(where) + " hr=0x" + std::to_string(hr);
        MessageBoxA(nullptr, msg.c_str(), "luma_dx12_backend", MB_OK | MB_ICONERROR);
        std::terminate();
    }
}

// Simple matrix math
void identity(float* m) {
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void perspective(float* m, float fovY, float aspect, float nearZ, float farZ) {
    std::memset(m, 0, 16 * sizeof(float));
    float yScale = 1.0f / tanf(fovY * 0.5f);
    float xScale = yScale / aspect;
    m[0] = xScale;
    m[5] = yScale;
    m[10] = farZ / (farZ - nearZ);
    m[11] = 1.0f;
    m[14] = -nearZ * farZ / (farZ - nearZ);
}

void lookAt(float* m, const float* eye, const float* at, const float* up) {
    float zAxis[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
    float len = sqrtf(zAxis[0]*zAxis[0] + zAxis[1]*zAxis[1] + zAxis[2]*zAxis[2]);
    zAxis[0] /= len; zAxis[1] /= len; zAxis[2] /= len;
    
    float xAxis[3] = {
        up[1]*zAxis[2] - up[2]*zAxis[1],
        up[2]*zAxis[0] - up[0]*zAxis[2],
        up[0]*zAxis[1] - up[1]*zAxis[0]
    };
    len = sqrtf(xAxis[0]*xAxis[0] + xAxis[1]*xAxis[1] + xAxis[2]*xAxis[2]);
    xAxis[0] /= len; xAxis[1] /= len; xAxis[2] /= len;
    
    float yAxis[3] = {
        zAxis[1]*xAxis[2] - zAxis[2]*xAxis[1],
        zAxis[2]*xAxis[0] - zAxis[0]*xAxis[2],
        zAxis[0]*xAxis[1] - zAxis[1]*xAxis[0]
    };
    
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
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp[i*4+j] = a[i*4+0]*b[0*4+j] + a[i*4+1]*b[1*4+j] + a[i*4+2]*b[2*4+j] + a[i*4+3]*b[3*4+j];
        }
    }
    std::memcpy(out, tmp, sizeof(tmp));
}

void rotateY(float* m, float angle) {
    identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c; m[2] = -s;
    m[8] = s; m[10] = c;
}

// Embedded shader source
const char* kShaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    float4x4 worldViewProj;
    float4x4 world;
    float3 lightDir;
    float _pad0;
    float3 cameraPos;
    float _pad1;
    float3 baseColor;
    float metallic;
    float roughness;
    float3 _pad2;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.worldPos = mul(world, float4(input.position, 1.0)).xyz;
    output.normal = mul((float3x3)world, input.normal);
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir);
    float3 V = normalize(cameraPos - input.worldPos);
    float3 H = normalize(L + V);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    
    float3 diffuse = input.color * baseColor * NdotL;
    float shininess = (1.0 - roughness) * 128.0 + 1.0;
    float spec = pow(NdotH, shininess);
    float3 specular = float3(1, 1, 1) * spec * (1.0 - roughness) * metallic;
    float3 ambient = input.color * baseColor * 0.15;
    
    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, 1.0);
}
)";

}  // namespace

namespace luma::rhi {

Dx12Backend::Dx12Backend(const NativeWindow& window) {
    init(window);
    init_pipeline();
    luma::log_info("DX12 backend initialized with PBR pipeline");
}

Dx12Backend::~Dx12Backend() {
    if (fence_event_) {
        wait_for_gpu();
        CloseHandle(fence_event_);
    }
}

void Dx12Backend::init(const NativeWindow& window) {
    hwnd_ = static_cast<HWND>(window.handle);
    width_ = window.width;
    height_ = window.height;

    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
        }
    }
#endif
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2");
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)), "D3D12CreateDevice");

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&command_queue_)), "CreateCommandQueue");

    DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
    swapDesc.BufferCount = kFrameCount;
    swapDesc.Width = width_;
    swapDesc.Height = height_;
    swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(factory_->CreateSwapChainForHwnd(
                      command_queue_.Get(), hwnd_, &swapDesc, nullptr, nullptr, &swapChain1),
                  "CreateSwapChainForHwnd");
    ThrowIfFailed(swapChain1.As(&swap_chain_), "SwapChain3");
    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = kFrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtv_heap_)), "CreateDescriptorHeap RTV");
    rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT n = 0; n < kFrameCount; n++) {
        ThrowIfFailed(swap_chain_->GetBuffer(n, IID_PPV_ARGS(&render_targets_[n])), "GetBuffer");
        device_->CreateRenderTargetView(render_targets_[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtv_descriptor_size_;
        ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS(&command_allocators_[n])),
                      "CreateCommandAllocator");
    }

    ThrowIfFailed(device_->CreateCommandList(
                      0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocators_[frame_index_].Get(), nullptr,
                      IID_PPV_ARGS(&command_list_)),
                  "CreateCommandList");
    command_list_->Close();

    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence");
    fence_value_ = 1;
    fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    create_depth_stencil();
}

void Dx12Backend::create_depth_stencil() {
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsv_heap_)), "CreateDescriptorHeap DSV");

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width_;
    depthDesc.Height = height_;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue, IID_PPV_ARGS(&depth_stencil_)), "CreateCommittedResource depth");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device_->CreateDepthStencilView(depth_stencil_.Get(), &dsvDesc, dsv_heap_->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Backend::init_pipeline() {
    // Root signature: one CBV at b0
    D3D12_ROOT_PARAMETER rootParams[1] = {};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = rootParams;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr)) {
        if (error) {
            MessageBoxA(nullptr, (char*)error->GetBufferPointer(), "Root Signature Error", MB_OK);
        }
        ThrowIfFailed(hr, "D3D12SerializeRootSignature");
    }
    ThrowIfFailed(device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                               IID_PPV_ARGS(&root_signature_)), "CreateRootSignature");

    // Compile shaders
    ComPtr<ID3DBlob> vertexShader, pixelShader, errorBlob;
    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    hr = D3DCompile(kShaderSource, strlen(kShaderSource), "basic.hlsl", nullptr, nullptr,
                    "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "VS Compile Error", MB_OK);
        ThrowIfFailed(hr, "D3DCompile VS");
    }

    hr = D3DCompile(kShaderSource, strlen(kShaderSource), "basic.hlsl", nullptr, nullptr,
                    "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "PS Compile Error", MB_OK);
        ThrowIfFailed(hr, "D3DCompile PS");
    }

    // Input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
    psoDesc.pRootSignature = root_signature_.Get();
    psoDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    psoDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
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

    ThrowIfFailed(device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_state_)), "CreateGraphicsPipelineState");

    // Constant buffer
    D3D12_HEAP_PROPERTIES cbHeapProps = {};
    cbHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = sizeof(SceneConstants);
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device_->CreateCommittedResource(
        &cbHeapProps, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&constant_buffer_)), "CreateCommittedResource CB");

    pipeline_ready_ = true;
    luma::log_info("DX12 PBR pipeline ready");
}

MeshGPU Dx12Backend::create_mesh(const Mesh& mesh) {
    MeshGPU gpu;
    gpu.index_count = static_cast<UINT>(mesh.indices.size());

    const UINT vbSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));
    const UINT ibSize = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = vbSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&gpu.vertex_buffer)), "CreateCommittedResource VB");

    bufDesc.Width = ibSize;
    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&gpu.index_buffer)), "CreateCommittedResource IB");

    // Upload vertex data
    void* mapped = nullptr;
    D3D12_RANGE readRange = {0, 0};
    ThrowIfFailed(gpu.vertex_buffer->Map(0, &readRange, &mapped), "VB Map");
    std::memcpy(mapped, mesh.vertices.data(), vbSize);
    gpu.vertex_buffer->Unmap(0, nullptr);

    ThrowIfFailed(gpu.index_buffer->Map(0, &readRange, &mapped), "IB Map");
    std::memcpy(mapped, mesh.indices.data(), ibSize);
    gpu.index_buffer->Unmap(0, nullptr);

    gpu.vbv.BufferLocation = gpu.vertex_buffer->GetGPUVirtualAddress();
    gpu.vbv.SizeInBytes = vbSize;
    gpu.vbv.StrideInBytes = sizeof(Vertex);

    gpu.ibv.BufferLocation = gpu.index_buffer->GetGPUVirtualAddress();
    gpu.ibv.SizeInBytes = ibSize;
    gpu.ibv.Format = DXGI_FORMAT_R32_UINT;

    return gpu;
}

void Dx12Backend::begin_scene(float time) {
    if (!pipeline_ready_) return;

    ThrowIfFailed(command_allocators_[frame_index_]->Reset(), "Allocator Reset");
    ThrowIfFailed(command_list_->Reset(command_allocators_[frame_index_].Get(), pipeline_state_.Get()), "CommandList Reset");

    // Transition to render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = render_targets_[frame_index_].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list_->ResourceBarrier(1, &barrier);

    // Set render targets
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(frame_index_) * rtv_descriptor_size_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
    command_list_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Clear
    const float clearColor[] = {0.1f, 0.1f, 0.15f, 1.0f};
    command_list_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    command_list_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set viewport and scissor
    D3D12_VIEWPORT viewport = {0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, 1.0f};
    D3D12_RECT scissor = {0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
    command_list_->RSSetViewports(1, &viewport);
    command_list_->RSSetScissorRects(1, &scissor);

    // Set root signature
    command_list_->SetGraphicsRootSignature(root_signature_.Get());

    // Update constant buffer
    float world[16], view[16], proj[16], wvp[16];
    rotateY(world, time);
    
    float eye[3] = {0.0f, 1.5f, -3.0f};
    float at[3] = {0.0f, 0.0f, 0.0f};
    float up[3] = {0.0f, 1.0f, 0.0f};
    lookAt(view, eye, at, up);
    
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    perspective(proj, 3.14159f / 4.0f, aspect, 0.1f, 100.0f);
    
    multiply(wvp, world, view);
    multiply(wvp, wvp, proj);

    std::memcpy(scene_constants_.worldViewProj, wvp, sizeof(wvp));
    std::memcpy(scene_constants_.world, world, sizeof(world));
    scene_constants_.lightDir[0] = -0.5f;
    scene_constants_.lightDir[1] = -1.0f;
    scene_constants_.lightDir[2] = 0.5f;
    scene_constants_.cameraPos[0] = eye[0];
    scene_constants_.cameraPos[1] = eye[1];
    scene_constants_.cameraPos[2] = eye[2];
    scene_constants_.baseColor[0] = 1.0f;
    scene_constants_.baseColor[1] = 1.0f;
    scene_constants_.baseColor[2] = 1.0f;
    scene_constants_.metallic = 0.3f;
    scene_constants_.roughness = 0.5f;

    void* mapped = nullptr;
    D3D12_RANGE readRange = {0, 0};
    ThrowIfFailed(constant_buffer_->Map(0, &readRange, &mapped), "CB Map");
    std::memcpy(mapped, &scene_constants_, sizeof(scene_constants_));
    constant_buffer_->Unmap(0, nullptr);

    command_list_->SetGraphicsRootConstantBufferView(0, constant_buffer_->GetGPUVirtualAddress());
    command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Dx12Backend::draw_mesh(const MeshGPU& mesh) {
    if (!pipeline_ready_) return;
    command_list_->IASetVertexBuffers(0, 1, &mesh.vbv);
    command_list_->IASetIndexBuffer(&mesh.ibv);
    command_list_->DrawIndexedInstanced(mesh.index_count, 1, 0, 0, 0);
}

void Dx12Backend::end_scene() {
    if (!pipeline_ready_) return;

    // Transition to present
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = render_targets_[frame_index_].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list_->ResourceBarrier(1, &barrier);

    ThrowIfFailed(command_list_->Close(), "CommandList Close");

    ID3D12CommandList* ppCommandLists[] = {command_list_.Get()};
    command_queue_->ExecuteCommandLists(1, ppCommandLists);
}

void Dx12Backend::render_clear(float r, float g, float b) {
    populate_command_list(r, g, b);
    ID3D12CommandList* ppCommandLists[] = {command_list_.Get()};
    command_queue_->ExecuteCommandLists(1, ppCommandLists);
}

void Dx12Backend::present() {
    ThrowIfFailed(swap_chain_->Present(1, 0), "Present");
    wait_for_gpu();
    backbuffer_state_ = ResourceState::Present;
}

void Dx12Backend::transition_backbuffer(ResourceState before, ResourceState after) {
    if (before == after) return;
    if (before == ResourceState::Present && after == ResourceState::ColorAttachment) {
        backbuffer_state_ = ResourceState::ColorAttachment;
    } else if (before == ResourceState::ColorAttachment && after == ResourceState::Present) {
        backbuffer_state_ = ResourceState::Present;
    }
}

void Dx12Backend::bind_material_params(const std::unordered_map<std::string, std::string>& params) {
    bound_params_ = params;
    upload_params();
}

void Dx12Backend::ensure_param_buffer(size_t bytes) {
    if (param_buffer_ && param_buffer_capacity_ >= bytes) return;
    param_buffer_.Reset();
    param_buffer_capacity_ = bytes;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = bytes;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device_->CreateCommittedResource(
                      &heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
                      nullptr, IID_PPV_ARGS(&param_buffer_)),
                  "CreateCommittedResource param buffer");
}

void Dx12Backend::populate_command_list(float r, float g, float b) {
    ThrowIfFailed(command_allocators_[frame_index_]->Reset(), "Allocator Reset");
    ThrowIfFailed(command_list_->Reset(command_allocators_[frame_index_].Get(), nullptr), "CommandList Reset");

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = render_targets_[frame_index_].Get();
    barrier.Transition.StateBefore = (backbuffer_state_ == ResourceState::Present)
                                         ? D3D12_RESOURCE_STATE_PRESENT
                                         : D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list_->ResourceBarrier(1, &barrier);
    backbuffer_state_ = ResourceState::ColorAttachment;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(frame_index_) * rtv_descriptor_size_;
    FLOAT clearColor[] = {r, g, b, 1.0f};
    command_list_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    command_list_->ResourceBarrier(1, &barrier);
    backbuffer_state_ = ResourceState::Present;

    ThrowIfFailed(command_list_->Close(), "CommandList Close");
}

void Dx12Backend::upload_params() {
    std::vector<float> flat;
    flat.reserve(bound_params_.size());
    for (const auto& kv : bound_params_) {
        try {
            flat.push_back(std::stof(kv.second));
        } catch (...) {
            flat.push_back(0.0f);
        }
    }
    if (flat.empty()) {
        param_buffer_.Reset();
        param_buffer_capacity_ = 0;
        return;
    }
    const size_t bytes = flat.size() * sizeof(float);
    ensure_param_buffer(bytes);
    void* mapped = nullptr;
    D3D12_RANGE range{0, 0};
    ThrowIfFailed(param_buffer_->Map(0, &range, &mapped), "Param buffer map");
    std::memcpy(mapped, flat.data(), bytes);
    param_buffer_->Unmap(0, nullptr);
}

void Dx12Backend::wait_for_gpu() {
    const UINT64 fenceToWaitFor = fence_value_;
    ThrowIfFailed(command_queue_->Signal(fence_.Get(), fenceToWaitFor), "Fence Signal");
    fence_value_++;
    if (fence_->GetCompletedValue() < fenceToWaitFor) {
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceToWaitFor, fence_event_), "SetEventOnCompletion");
        WaitForSingleObject(fence_event_, INFINITE);
    }
    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
}

std::unique_ptr<Backend> create_dx12_backend(const NativeWindow& window) {
    return std::make_unique<Dx12Backend>(window);
}

}  // namespace luma::rhi

#endif  // _WIN32
