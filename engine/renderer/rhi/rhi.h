// RHI - Render Hardware Interface
// Unified abstraction layer for DX12, Metal, and Vulkan
#pragma once

#include "rhi_types.h"
#include "rhi_resources.h"
#include "rhi_device.h"

namespace luma::rhi {

// ===== Utility Functions =====

// Get the appropriate backend for the current platform
inline BackendType getDefaultBackend() {
#if defined(_WIN32)
    return BackendType::DX12;
#elif defined(__APPLE__)
    return BackendType::Metal;
#else
    return BackendType::Vulkan;
#endif
}

// Get format size in bytes
inline uint32_t getFormatSize(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8_UNorm:
        case TextureFormat::BGRA8_UNorm:
            return 4;
        case TextureFormat::RGBA16_Float:
            return 8;
        case TextureFormat::RGBA32_Float:
            return 16;
        case TextureFormat::Depth32_Float:
            return 4;
        case TextureFormat::Depth24_Stencil8:
            return 4;
        default:
            return 0;
    }
}

// Get vertex format size in bytes
inline uint32_t getVertexFormatSize(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float:
        case VertexFormat::Int:
            return 4;
        case VertexFormat::Float2:
        case VertexFormat::Int2:
            return 8;
        case VertexFormat::Float3:
        case VertexFormat::Int3:
            return 12;
        case VertexFormat::Float4:
        case VertexFormat::Int4:
            return 16;
        default:
            return 0;
    }
}

// ===== Helper: Create standard vertex layout for PBR meshes =====
inline VertexLayout createPBRVertexLayout() {
    VertexLayout layout;
    layout.stride = 60;  // sizeof(Vertex)
    
    // position: float3 at offset 0
    layout.attributes.push_back({"POSITION", 0, VertexFormat::Float3, 0, 0});
    // normal: float3 at offset 12
    layout.attributes.push_back({"NORMAL", 1, VertexFormat::Float3, 12, 0});
    // tangent: float4 at offset 24
    layout.attributes.push_back({"TANGENT", 2, VertexFormat::Float4, 24, 0});
    // uv: float2 at offset 40
    layout.attributes.push_back({"TEXCOORD", 3, VertexFormat::Float2, 40, 0});
    // color: float3 at offset 48
    layout.attributes.push_back({"COLOR", 4, VertexFormat::Float3, 48, 0});
    
    return layout;
}

// ===== Helper: Create line vertex layout =====
inline VertexLayout createLineVertexLayout() {
    VertexLayout layout;
    layout.stride = 28;  // float3 position + float4 color
    
    layout.attributes.push_back({"POSITION", 0, VertexFormat::Float3, 0, 0});
    layout.attributes.push_back({"COLOR", 1, VertexFormat::Float4, 12, 0});
    
    return layout;
}

}  // namespace luma::rhi
