// Minimal RHI interface; DX12 implementation provided for Windows.
#pragma once

#include <memory>
#include <unordered_map>
#include <string>

#include "engine/renderer/rhi/rhi_types.h"

namespace luma::rhi {

class Backend {
public:
    virtual ~Backend() = default;
    virtual void render_clear(float r, float g, float b) = 0;
    virtual void present() = 0;

    // Optional: resize swapchain if needed.
    virtual void resize(uint32_t /*width*/, uint32_t /*height*/) {}

    // Optional: backend-specific transition for the backbuffer.
    virtual void transition_backbuffer(ResourceState /*before*/, ResourceState /*after*/) {}

    // Optional: bind material parameters (name -> value string) for debug/CB updates.
    virtual void bind_material_params(const std::unordered_map<std::string, std::string>& /*params*/) {}
};

std::unique_ptr<Backend> create_dx12_backend(const NativeWindow& window);
std::unique_ptr<Backend> create_metal_backend(const NativeWindow& window);
std::unique_ptr<Backend> create_vulkan_backend(const NativeWindow& window);

}  // namespace luma::rhi

