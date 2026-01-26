// RHI Device - Abstract device and command buffer interfaces
#pragma once

#include "rhi_types.h"
#include "rhi_resources.h"
#include <functional>

namespace luma::rhi {

// ===== Command Buffer =====
class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;
    
    // Begin/End
    virtual void begin() = 0;
    virtual void end() = 0;
    
    // Render Pass
    virtual void beginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void endRenderPass() = 0;
    
    // Pipeline State
    virtual void setPipeline(const PipelineHandle& pipeline) = 0;
    virtual void setViewport(const Viewport& viewport) = 0;
    virtual void setScissor(const Scissor& scissor) = 0;
    
    // Resource Binding
    virtual void setVertexBuffer(uint32_t slot, const BufferHandle& buffer, uint64_t offset = 0) = 0;
    virtual void setIndexBuffer(const BufferHandle& buffer, IndexType type, uint64_t offset = 0) = 0;
    virtual void setConstantBuffer(uint32_t slot, const BufferHandle& buffer, ShaderStage stage = ShaderStage::Vertex) = 0;
    virtual void setTexture(uint32_t slot, const TextureHandle& texture, ShaderStage stage = ShaderStage::Fragment) = 0;
    virtual void setSampler(uint32_t slot, const SamplerHandle& sampler, ShaderStage stage = ShaderStage::Fragment) = 0;
    
    // Draw Commands
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
    
    // Copy Commands
    virtual void copyBufferToTexture(const BufferHandle& buffer, const TextureHandle& texture) = 0;
};

using CommandBufferHandle = std::unique_ptr<CommandBuffer>;

// ===== Device =====
class Device {
public:
    virtual ~Device() = default;
    
    // Device Info
    virtual BackendType getBackendType() const = 0;
    virtual std::string getDeviceName() const = 0;
    
    // Resource Creation
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual SamplerHandle createSampler(const SamplerDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
    
    // Swapchain
    virtual SwapchainHandle createSwapchain(const SwapchainDesc& desc) = 0;
    
    // Command Buffer
    virtual CommandBufferHandle createCommandBuffer() = 0;
    
    // Submission
    virtual void submit(CommandBuffer* cmdBuffer) = 0;
    virtual void waitIdle() = 0;
    
    // Frame synchronization
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    // Native handle access (for ImGui integration etc.)
    virtual void* getNativeDevice() const = 0;
    virtual void* getNativeQueue() const = 0;
};

using DeviceHandle = std::unique_ptr<Device>;

// ===== Factory Functions =====
DeviceHandle createDevice(BackendType type);
DeviceHandle createDX12Device();
DeviceHandle createMetalDevice();
DeviceHandle createVulkanDevice();

}  // namespace luma::rhi
