#include "engine/renderer/rhi/rhi.h"

#include <memory>

namespace luma::rhi {

std::unique_ptr<Backend> create_vulkan_backend(const NativeWindow&) {
    // Stub for Vulkan backend (Android/Desktop). Real impl will create instance/device/swapchain.
    return nullptr;
}

}  // namespace luma::rhi

