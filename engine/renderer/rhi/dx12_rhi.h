// DX12 RHI Backend - Header
#pragma once

#if defined(_WIN32)

#include "rhi.h"

namespace luma::rhi {

// Factory function to create DX12 device
DeviceHandle createDX12Device();

}  // namespace luma::rhi

#endif  // _WIN32
