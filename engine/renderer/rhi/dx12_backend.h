#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <array>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "engine/renderer/rhi/rhi.h"

namespace luma::rhi {

class Dx12Backend : public Backend {
public:
    Dx12Backend(const NativeWindow& window);
    ~Dx12Backend() override;

    void render_clear(float r, float g, float b) override;
    void present() override;
    void transition_backbuffer(ResourceState before, ResourceState after) override;
    void bind_material_params(const std::unordered_map<std::string, std::string>& params) override;

private:
    static constexpr UINT kFrameCount = 2;

    void init(const NativeWindow& window);
    void populate_command_list(float r, float g, float b);
    void wait_for_gpu();

    HWND hwnd_{};
    UINT width_{1280};
    UINT height_{720};

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    UINT rtv_descriptor_size_{0};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kFrameCount> render_targets_;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, kFrameCount> command_allocators_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    UINT64 fence_value_{0};
    HANDLE fence_event_{nullptr};
    UINT frame_index_{0};
    ResourceState backbuffer_state_{ResourceState::Present};

    std::unordered_map<std::string, std::string> bound_params_;
    Microsoft::WRL::ComPtr<ID3D12Resource> param_buffer_;
    size_t param_buffer_capacity_{0};

    void ensure_param_buffer(size_t bytes);
    void upload_params();
};

}  // namespace luma::rhi

#endif  // _WIN32

