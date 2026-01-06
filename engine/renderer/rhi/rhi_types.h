// Minimal RHI-agnostic types.
#pragma once

#include <cstdint>
#include <optional>

namespace luma::rhi {

struct SwapchainDesc {
    uint32_t width{1280};
    uint32_t height{720};
};

struct NativeWindow {
    void* handle{nullptr};  // HWND on Windows
    uint32_t width{1280};
    uint32_t height{720};
};

enum class BackendType {
    DX12,
    Metal,
    Vulkan
};

enum class TextureFormat {
    Unknown,
    RGBA8_UNorm,
    BGRA8_UNorm
};

enum class TextureUsage {
    ColorAttachment,
    Present
};

enum class ResourceState {
    Undefined,
    ColorAttachment,
    Present
};

}  // namespace luma::rhi

