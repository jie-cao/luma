#ifdef _WIN32
#include "dx12_backend.h"

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "engine/foundation/log.h"

using Microsoft::WRL::ComPtr;

namespace {

inline void ThrowIfFailed(HRESULT hr, std::string_view where) {
    if (FAILED(hr)) {
        std::string msg = std::string("DX12 failure at ") + std::string(where);
        MessageBoxA(nullptr, msg.c_str(), "luma_dx12_backend", MB_OK | MB_ICONERROR);
        std::terminate();
    }
}

}  // namespace

namespace luma::rhi {

Dx12Backend::Dx12Backend(const NativeWindow& window) {
    init(window);
    luma::log_info("DX12 backend initialized");
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
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtv_heap_)), "CreateDescriptorHeap");
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
}

void Dx12Backend::render_clear(float r, float g, float b) {
    // Ensure correct state
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
    // For now we only support Present <-> ColorAttachment.
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
                      &heapProps,
                      D3D12_HEAP_FLAG_NONE,
                      &desc,
                      D3D12_RESOURCE_STATE_GENERIC_READ,
                      nullptr,
                      IID_PPV_ARGS(&param_buffer_)),
                  "CreateCommittedResource param buffer");
}

void Dx12Backend::populate_command_list(float r, float g, float b) {
    ThrowIfFailed(command_allocators_[frame_index_]->Reset(), "Allocator Reset");
    ThrowIfFailed(command_list_->Reset(command_allocators_[frame_index_].Get(), nullptr), "CommandList Reset");

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
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
    // Flatten values to floats; non-float values are treated as 0.
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

