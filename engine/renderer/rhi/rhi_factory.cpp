// RHI Factory - Device creation
#include "rhi_device.h"

#if defined(_WIN32)
#include "dx12_rhi.h"
#endif

#if defined(__APPLE__)
// Metal factory declared in metal_rhi.h
namespace luma::rhi {
DeviceHandle createMetalDevice();
}
#endif

namespace luma::rhi {

DeviceHandle createDevice(BackendType type) {
    switch (type) {
#if defined(_WIN32)
        case BackendType::DX12:
            return createDX12Device();
#endif
            
#if defined(__APPLE__)
        case BackendType::Metal:
            return createMetalDevice();
#endif
            
        case BackendType::Vulkan:
            // TODO: Implement Vulkan backend
            return nullptr;
            
        default:
            return nullptr;
    }
}

// Platform-specific default device creation
DeviceHandle createDefaultDevice() {
#if defined(_WIN32)
    return createDX12Device();
#elif defined(__APPLE__)
    return createMetalDevice();
#else
    // Linux - try Vulkan
    return createDevice(BackendType::Vulkan);
#endif
}

}  // namespace luma::rhi
