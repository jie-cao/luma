// Metal RHI Backend - Header
#pragma once

#if defined(__APPLE__)

#include "rhi.h"

namespace luma::rhi {

// Factory function to create Metal device
DeviceHandle createMetalDevice();

}  // namespace luma::rhi

#endif  // __APPLE__
