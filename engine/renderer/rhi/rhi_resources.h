// RHI Resources - Abstract resource interfaces
#pragma once

#include "rhi_types.h"

namespace luma::rhi {

// ===== Base Resource =====
class Resource {
public:
    virtual ~Resource() = default;
    virtual void* getNativeHandle() const = 0;
};

// ===== Buffer =====
class Buffer : public Resource {
public:
    virtual ~Buffer() = default;
    
    virtual uint64_t getSize() const = 0;
    virtual BufferUsage getUsage() const = 0;
    
    // CPU access (only if created with cpuAccess = true)
    virtual void* map() = 0;
    virtual void unmap() = 0;
    
    // Update data (convenience method)
    virtual void update(const void* data, uint64_t size, uint64_t offset = 0) = 0;
};

// ===== Texture =====
class Texture : public Resource {
public:
    virtual ~Texture() = default;
    
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
    
    // Upload texture data
    virtual void upload(const void* data, uint32_t bytesPerRow) = 0;
};

// ===== Sampler =====
class Sampler : public Resource {
public:
    virtual ~Sampler() = default;
};

// ===== Shader =====
class Shader : public Resource {
public:
    virtual ~Shader() = default;
    virtual ShaderStage getStage() const = 0;
};

// ===== Pipeline =====
class Pipeline : public Resource {
public:
    virtual ~Pipeline() = default;
    virtual PrimitiveTopology getTopology() const = 0;
};

// ===== Swapchain =====
class Swapchain {
public:
    virtual ~Swapchain() = default;
    
    virtual TextureHandle getCurrentTexture() = 0;
    virtual uint32_t getCurrentIndex() const = 0;
    virtual void present() = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;
    
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
};

using SwapchainHandle = std::unique_ptr<Swapchain>;

}  // namespace luma::rhi
