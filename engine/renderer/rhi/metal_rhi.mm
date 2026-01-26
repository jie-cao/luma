// Metal RHI Backend - Implementation
#if defined(__APPLE__)

#include "metal_rhi.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

#include <iostream>
#include <unordered_map>

namespace luma::rhi {

// ===== Metal Buffer =====
class MetalBuffer : public Buffer {
public:
    MetalBuffer(id<MTLDevice> device, const BufferDesc& desc) 
        : size_(desc.size), usage_(desc.usage), cpuAccess_(desc.cpuAccess) {
        MTLResourceOptions options = desc.cpuAccess ? 
            MTLResourceStorageModeShared : MTLResourceStorageModePrivate;
        buffer_ = [device newBufferWithLength:desc.size options:options];
        if (desc.debugName) {
            buffer_.label = [NSString stringWithUTF8String:desc.debugName];
        }
    }
    
    void* getNativeHandle() const override { return (__bridge void*)buffer_; }
    uint64_t getSize() const override { return size_; }
    BufferUsage getUsage() const override { return usage_; }
    
    void* map() override {
        if (!cpuAccess_) return nullptr;
        return [buffer_ contents];
    }
    
    void unmap() override {
        // Metal shared buffers don't need explicit unmap
    }
    
    void update(const void* data, uint64_t size, uint64_t offset) override {
        if (cpuAccess_) {
            memcpy((uint8_t*)[buffer_ contents] + offset, data, size);
        }
    }
    
    id<MTLBuffer> getBuffer() const { return buffer_; }
    
private:
    id<MTLBuffer> buffer_;
    uint64_t size_;
    BufferUsage usage_;
    bool cpuAccess_;
};

// ===== Metal Texture =====
class MetalTexture : public Texture {
public:
    MetalTexture(id<MTLDevice> device, const TextureDesc& desc) 
        : width_(desc.width), height_(desc.height), format_(desc.format) {
        MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:toMTLFormat(desc.format)
                                                                                           width:desc.width
                                                                                          height:desc.height
                                                                                       mipmapped:(desc.mipLevels > 1)];
        texDesc.usage = toMTLUsage(desc.usage);
        texDesc.storageMode = MTLStorageModePrivate;
        
        // RenderTarget and DepthStencil need special storage mode on some platforms
        if ((uint32_t)desc.usage & ((uint32_t)TextureUsage::RenderTarget | (uint32_t)TextureUsage::DepthStencil)) {
            texDesc.storageMode = MTLStorageModePrivate;
        }
        
        texture_ = [device newTextureWithDescriptor:texDesc];
        if (desc.debugName) {
            texture_.label = [NSString stringWithUTF8String:desc.debugName];
        }
    }
    
    // Wrap existing MTLTexture
    MetalTexture(id<MTLTexture> texture) 
        : texture_(texture), width_((uint32_t)texture.width), height_((uint32_t)texture.height) {
        format_ = fromMTLFormat(texture.pixelFormat);
    }
    
    void* getNativeHandle() const override { return (__bridge void*)texture_; }
    uint32_t getWidth() const override { return width_; }
    uint32_t getHeight() const override { return height_; }
    TextureFormat getFormat() const override { return format_; }
    
    void upload(const void* data, uint32_t bytesPerRow) override {
        MTLRegion region = MTLRegionMake2D(0, 0, width_, height_);
        [texture_ replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
    }
    
    id<MTLTexture> getTexture() const { return texture_; }
    
    static MTLPixelFormat toMTLFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGBA8_UNorm: return MTLPixelFormatRGBA8Unorm;
            case TextureFormat::BGRA8_UNorm: return MTLPixelFormatBGRA8Unorm;
            case TextureFormat::RGBA16_Float: return MTLPixelFormatRGBA16Float;
            case TextureFormat::RGBA32_Float: return MTLPixelFormatRGBA32Float;
            case TextureFormat::Depth32_Float: return MTLPixelFormatDepth32Float;
            case TextureFormat::Depth24_Stencil8: return MTLPixelFormatDepth24Unorm_Stencil8;
            default: return MTLPixelFormatRGBA8Unorm;
        }
    }
    
    static TextureFormat fromMTLFormat(MTLPixelFormat format) {
        switch (format) {
            case MTLPixelFormatRGBA8Unorm: return TextureFormat::RGBA8_UNorm;
            case MTLPixelFormatBGRA8Unorm: return TextureFormat::BGRA8_UNorm;
            case MTLPixelFormatRGBA16Float: return TextureFormat::RGBA16_Float;
            case MTLPixelFormatRGBA32Float: return TextureFormat::RGBA32_Float;
            case MTLPixelFormatDepth32Float: return TextureFormat::Depth32_Float;
            default: return TextureFormat::Unknown;
        }
    }
    
    static MTLTextureUsage toMTLUsage(TextureUsage usage) {
        MTLTextureUsage mtlUsage = 0;
        if ((uint32_t)usage & (uint32_t)TextureUsage::ShaderRead) mtlUsage |= MTLTextureUsageShaderRead;
        if ((uint32_t)usage & (uint32_t)TextureUsage::ShaderWrite) mtlUsage |= MTLTextureUsageShaderWrite;
        if ((uint32_t)usage & (uint32_t)TextureUsage::RenderTarget) mtlUsage |= MTLTextureUsageRenderTarget;
        if ((uint32_t)usage & (uint32_t)TextureUsage::DepthStencil) mtlUsage |= MTLTextureUsageRenderTarget;
        return mtlUsage;
    }
    
private:
    id<MTLTexture> texture_;
    uint32_t width_;
    uint32_t height_;
    TextureFormat format_;
};

// ===== Metal Sampler =====
class MetalSampler : public Sampler {
public:
    MetalSampler(id<MTLDevice> device, const SamplerDesc& desc) {
        MTLSamplerDescriptor* sampDesc = [[MTLSamplerDescriptor alloc] init];
        sampDesc.minFilter = (desc.minFilter == FilterMode::Nearest) ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
        sampDesc.magFilter = (desc.magFilter == FilterMode::Nearest) ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
        sampDesc.mipFilter = (desc.mipFilter == FilterMode::Nearest) ? MTLSamplerMipFilterNearest : MTLSamplerMipFilterLinear;
        sampDesc.sAddressMode = toMTLAddressMode(desc.addressU);
        sampDesc.tAddressMode = toMTLAddressMode(desc.addressV);
        sampDesc.rAddressMode = toMTLAddressMode(desc.addressW);
        sampDesc.maxAnisotropy = desc.maxAnisotropy;
        sampler_ = [device newSamplerStateWithDescriptor:sampDesc];
    }
    
    void* getNativeHandle() const override { return (__bridge void*)sampler_; }
    id<MTLSamplerState> getSampler() const { return sampler_; }
    
    static MTLSamplerAddressMode toMTLAddressMode(AddressMode mode) {
        switch (mode) {
            case AddressMode::Repeat: return MTLSamplerAddressModeRepeat;
            case AddressMode::MirrorRepeat: return MTLSamplerAddressModeMirrorRepeat;
            case AddressMode::ClampToEdge: return MTLSamplerAddressModeClampToEdge;
            case AddressMode::ClampToBorder: return MTLSamplerAddressModeClampToBorderColor;
            default: return MTLSamplerAddressModeRepeat;
        }
    }
    
private:
    id<MTLSamplerState> sampler_;
};

// ===== Metal Shader =====
class MetalShader : public Shader {
public:
    MetalShader(id<MTLDevice> device, id<MTLLibrary> library, const ShaderDesc& desc) 
        : stage_(desc.stage) {
        NSString* entryPoint = [NSString stringWithUTF8String:desc.entryPoint];
        function_ = [library newFunctionWithName:entryPoint];
        if (!function_) {
            std::cerr << "[rhi/metal] Failed to find function: " << desc.entryPoint << std::endl;
        }
    }
    
    void* getNativeHandle() const override { return (__bridge void*)function_; }
    ShaderStage getStage() const override { return stage_; }
    id<MTLFunction> getFunction() const { return function_; }
    
private:
    id<MTLFunction> function_;
    ShaderStage stage_;
};

// ===== Metal Pipeline =====
class MetalPipeline : public Pipeline {
public:
    MetalPipeline(id<MTLDevice> device, const PipelineDesc& desc) 
        : topology_(desc.topology) {
        NSError* error = nil;
        
        // Create vertex descriptor
        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
        uint32_t attrIndex = 0;
        for (const auto& attr : desc.vertexLayout.attributes) {
            vertexDesc.attributes[attrIndex].format = toMTLVertexFormat(attr.format);
            vertexDesc.attributes[attrIndex].offset = attr.offset;
            vertexDesc.attributes[attrIndex].bufferIndex = attr.bufferIndex + 1;  // +1 because 0 is constants
            attrIndex++;
        }
        vertexDesc.layouts[1].stride = desc.vertexLayout.stride;
        vertexDesc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        
        // Create pipeline descriptor
        MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        
        if (desc.vertexShader) {
            auto* mtlShader = static_cast<MetalShader*>(desc.vertexShader.get());
            pipelineDesc.vertexFunction = mtlShader->getFunction();
        }
        if (desc.fragmentShader) {
            auto* mtlShader = static_cast<MetalShader*>(desc.fragmentShader.get());
            pipelineDesc.fragmentFunction = mtlShader->getFunction();
        }
        
        pipelineDesc.vertexDescriptor = vertexDesc;
        pipelineDesc.colorAttachments[0].pixelFormat = MetalTexture::toMTLFormat(desc.colorFormat);
        
        // Blending
        if (desc.blend.enabled) {
            pipelineDesc.colorAttachments[0].blendingEnabled = YES;
            pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            pipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
            pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
            pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
            pipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        }
        pipelineDesc.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
        
        pipelineDesc.depthAttachmentPixelFormat = MetalTexture::toMTLFormat(desc.depthFormat);
        
        renderPipeline_ = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if (!renderPipeline_) {
            std::cerr << "[rhi/metal] Failed to create pipeline: " << [[error localizedDescription] UTF8String] << std::endl;
        }
        
        // Create depth stencil state
        MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthDesc.depthCompareFunction = toMTLCompareFunction(desc.depthStencil.depthCompare);
        depthDesc.depthWriteEnabled = desc.depthStencil.depthWriteEnabled;
        depthState_ = [device newDepthStencilStateWithDescriptor:depthDesc];
        
        cullMode_ = toMTLCullMode(desc.rasterizer.cullMode);
    }
    
    void* getNativeHandle() const override { return (__bridge void*)renderPipeline_; }
    PrimitiveTopology getTopology() const override { return topology_; }
    
    id<MTLRenderPipelineState> getRenderPipeline() const { return renderPipeline_; }
    id<MTLDepthStencilState> getDepthState() const { return depthState_; }
    MTLCullMode getCullMode() const { return cullMode_; }
    
    static MTLVertexFormat toMTLVertexFormat(VertexFormat format) {
        switch (format) {
            case VertexFormat::Float: return MTLVertexFormatFloat;
            case VertexFormat::Float2: return MTLVertexFormatFloat2;
            case VertexFormat::Float3: return MTLVertexFormatFloat3;
            case VertexFormat::Float4: return MTLVertexFormatFloat4;
            case VertexFormat::Int: return MTLVertexFormatInt;
            case VertexFormat::Int2: return MTLVertexFormatInt2;
            case VertexFormat::Int3: return MTLVertexFormatInt3;
            case VertexFormat::Int4: return MTLVertexFormatInt4;
            default: return MTLVertexFormatFloat3;
        }
    }
    
    static MTLCompareFunction toMTLCompareFunction(CompareFunction func) {
        switch (func) {
            case CompareFunction::Never: return MTLCompareFunctionNever;
            case CompareFunction::Less: return MTLCompareFunctionLess;
            case CompareFunction::Equal: return MTLCompareFunctionEqual;
            case CompareFunction::LessEqual: return MTLCompareFunctionLessEqual;
            case CompareFunction::Greater: return MTLCompareFunctionGreater;
            case CompareFunction::NotEqual: return MTLCompareFunctionNotEqual;
            case CompareFunction::GreaterEqual: return MTLCompareFunctionGreaterEqual;
            case CompareFunction::Always: return MTLCompareFunctionAlways;
            default: return MTLCompareFunctionLess;
        }
    }
    
    static MTLCullMode toMTLCullMode(CullMode mode) {
        switch (mode) {
            case CullMode::None: return MTLCullModeNone;
            case CullMode::Front: return MTLCullModeFront;
            case CullMode::Back: return MTLCullModeBack;
            default: return MTLCullModeNone;
        }
    }
    
    static MTLPrimitiveType toMTLPrimitiveType(PrimitiveTopology topology) {
        switch (topology) {
            case PrimitiveTopology::TriangleList: return MTLPrimitiveTypeTriangle;
            case PrimitiveTopology::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
            case PrimitiveTopology::LineList: return MTLPrimitiveTypeLine;
            case PrimitiveTopology::LineStrip: return MTLPrimitiveTypeLineStrip;
            case PrimitiveTopology::PointList: return MTLPrimitiveTypePoint;
            default: return MTLPrimitiveTypeTriangle;
        }
    }
    
private:
    id<MTLRenderPipelineState> renderPipeline_;
    id<MTLDepthStencilState> depthState_;
    MTLCullMode cullMode_;
    PrimitiveTopology topology_;
};

// ===== Metal Swapchain =====
class MetalSwapchain : public Swapchain {
public:
    MetalSwapchain(id<MTLDevice> device, const SwapchainDesc& desc) 
        : width_(desc.window.width), height_(desc.window.height), format_(desc.format) {
        metalLayer_ = [CAMetalLayer layer];
        metalLayer_.device = device;
        metalLayer_.pixelFormat = MetalTexture::toMTLFormat(format_);
        metalLayer_.framebufferOnly = NO;  // Allow read-back for ImGui
        metalLayer_.drawableSize = CGSizeMake(width_, height_);
        
#if TARGET_OS_OSX
        NSView* view = (__bridge NSView*)desc.window.handle;
        [view setWantsLayer:YES];
        [view setLayer:metalLayer_];
        metalLayer_.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
#else
        UIView* view = (__bridge UIView*)desc.window.handle;
        [view.layer addSublayer:metalLayer_];
        metalLayer_.frame = view.bounds;
        metalLayer_.contentsScale = [[UIScreen mainScreen] scale];
#endif
    }
    
    TextureHandle getCurrentTexture() override {
        currentDrawable_ = [metalLayer_ nextDrawable];
        if (!currentDrawable_) return nullptr;
        return std::make_shared<MetalTexture>(currentDrawable_.texture);
    }
    
    uint32_t getCurrentIndex() const override { return 0; }  // Metal handles this internally
    
    void present() override {
        // Present is handled in command buffer commit
    }
    
    void resize(uint32_t width, uint32_t height) override {
        width_ = width;
        height_ = height;
        metalLayer_.drawableSize = CGSizeMake(width, height);
    }
    
    uint32_t getWidth() const override { return width_; }
    uint32_t getHeight() const override { return height_; }
    TextureFormat getFormat() const override { return format_; }
    
    CAMetalLayer* getMetalLayer() const { return metalLayer_; }
    id<CAMetalDrawable> getCurrentDrawable() const { return currentDrawable_; }
    
private:
    CAMetalLayer* metalLayer_;
    id<CAMetalDrawable> currentDrawable_;
    uint32_t width_;
    uint32_t height_;
    TextureFormat format_;
};

// ===== Metal Command Buffer =====
class MetalCommandBuffer : public CommandBuffer {
public:
    MetalCommandBuffer(id<MTLCommandQueue> queue) : queue_(queue) {}
    
    void begin() override {
        commandBuffer_ = [queue_ commandBuffer];
    }
    
    void end() override {
        // Finalized on submit
    }
    
    void beginRenderPass(const RenderPassDesc& desc) override {
        MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
        
        // Color attachments
        for (size_t i = 0; i < desc.colorAttachments.size(); i++) {
            const auto& attach = desc.colorAttachments[i];
            if (attach.texture) {
                auto* mtlTex = static_cast<MetalTexture*>(attach.texture.get());
                passDesc.colorAttachments[i].texture = mtlTex->getTexture();
                passDesc.colorAttachments[i].loadAction = toMTLLoadAction(attach.loadAction);
                passDesc.colorAttachments[i].storeAction = toMTLStoreAction(attach.storeAction);
                passDesc.colorAttachments[i].clearColor = MTLClearColorMake(
                    attach.clearColor[0], attach.clearColor[1], attach.clearColor[2], attach.clearColor[3]);
            }
        }
        
        // Depth attachment
        if (desc.depthAttachment.texture) {
            auto* mtlTex = static_cast<MetalTexture*>(desc.depthAttachment.texture.get());
            passDesc.depthAttachment.texture = mtlTex->getTexture();
            passDesc.depthAttachment.loadAction = toMTLLoadAction(desc.depthAttachment.loadAction);
            passDesc.depthAttachment.storeAction = toMTLStoreAction(desc.depthAttachment.storeAction);
            passDesc.depthAttachment.clearDepth = desc.depthAttachment.clearDepth;
        }
        
        encoder_ = [commandBuffer_ renderCommandEncoderWithDescriptor:passDesc];
    }
    
    void endRenderPass() override {
        [encoder_ endEncoding];
        encoder_ = nil;
    }
    
    void setPipeline(const PipelineHandle& pipeline) override {
        currentPipeline_ = static_cast<MetalPipeline*>(pipeline.get());
        [encoder_ setRenderPipelineState:currentPipeline_->getRenderPipeline()];
        [encoder_ setDepthStencilState:currentPipeline_->getDepthState()];
        [encoder_ setCullMode:currentPipeline_->getCullMode()];
    }
    
    void setViewport(const Viewport& viewport) override {
        MTLViewport vp = {viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth};
        [encoder_ setViewport:vp];
    }
    
    void setScissor(const Scissor& scissor) override {
        MTLScissorRect rect = {(NSUInteger)scissor.x, (NSUInteger)scissor.y, scissor.width, scissor.height};
        [encoder_ setScissorRect:rect];
    }
    
    void setVertexBuffer(uint32_t slot, const BufferHandle& buffer, uint64_t offset) override {
        auto* mtlBuffer = static_cast<MetalBuffer*>(buffer.get());
        [encoder_ setVertexBuffer:mtlBuffer->getBuffer() offset:offset atIndex:slot + 1];  // +1 because 0 is constants
    }
    
    void setIndexBuffer(const BufferHandle& buffer, IndexType type, uint64_t offset) override {
        currentIndexBuffer_ = static_cast<MetalBuffer*>(buffer.get());
        currentIndexType_ = (type == IndexType::UInt16) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
        currentIndexOffset_ = offset;
    }
    
    void setConstantBuffer(uint32_t slot, const BufferHandle& buffer, ShaderStage stage) override {
        auto* mtlBuffer = static_cast<MetalBuffer*>(buffer.get());
        if (stage == ShaderStage::Vertex || stage == ShaderStage::Compute) {
            [encoder_ setVertexBuffer:mtlBuffer->getBuffer() offset:0 atIndex:slot];
        }
        if (stage == ShaderStage::Fragment || stage == ShaderStage::Compute) {
            [encoder_ setFragmentBuffer:mtlBuffer->getBuffer() offset:0 atIndex:slot];
        }
    }
    
    void setTexture(uint32_t slot, const TextureHandle& texture, ShaderStage stage) override {
        auto* mtlTex = static_cast<MetalTexture*>(texture.get());
        if (stage == ShaderStage::Fragment) {
            [encoder_ setFragmentTexture:mtlTex->getTexture() atIndex:slot];
        } else if (stage == ShaderStage::Vertex) {
            [encoder_ setVertexTexture:mtlTex->getTexture() atIndex:slot];
        }
    }
    
    void setSampler(uint32_t slot, const SamplerHandle& sampler, ShaderStage stage) override {
        auto* mtlSampler = static_cast<MetalSampler*>(sampler.get());
        if (stage == ShaderStage::Fragment) {
            [encoder_ setFragmentSamplerState:mtlSampler->getSampler() atIndex:slot];
        } else if (stage == ShaderStage::Vertex) {
            [encoder_ setVertexSamplerState:mtlSampler->getSampler() atIndex:slot];
        }
    }
    
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override {
        MTLPrimitiveType primType = MetalPipeline::toMTLPrimitiveType(currentPipeline_->getTopology());
        [encoder_ drawPrimitives:primType vertexStart:firstVertex vertexCount:vertexCount instanceCount:instanceCount baseInstance:firstInstance];
    }
    
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override {
        MTLPrimitiveType primType = MetalPipeline::toMTLPrimitiveType(currentPipeline_->getTopology());
        [encoder_ drawIndexedPrimitives:primType
                             indexCount:indexCount
                              indexType:currentIndexType_
                            indexBuffer:currentIndexBuffer_->getBuffer()
                      indexBufferOffset:currentIndexOffset_ + firstIndex * (currentIndexType_ == MTLIndexTypeUInt16 ? 2 : 4)
                          instanceCount:instanceCount
                             baseVertex:vertexOffset
                           baseInstance:firstInstance];
    }
    
    void copyBufferToTexture(const BufferHandle& buffer, const TextureHandle& texture) override {
        // Would need blit encoder - simplified for now
    }
    
    id<MTLCommandBuffer> getCommandBuffer() const { return commandBuffer_; }
    id<MTLRenderCommandEncoder> getEncoder() const { return encoder_; }
    
    static MTLLoadAction toMTLLoadAction(LoadAction action) {
        switch (action) {
            case LoadAction::Load: return MTLLoadActionLoad;
            case LoadAction::Clear: return MTLLoadActionClear;
            case LoadAction::DontCare: return MTLLoadActionDontCare;
            default: return MTLLoadActionClear;
        }
    }
    
    static MTLStoreAction toMTLStoreAction(StoreAction action) {
        switch (action) {
            case StoreAction::Store: return MTLStoreActionStore;
            case StoreAction::DontCare: return MTLStoreActionDontCare;
            default: return MTLStoreActionStore;
        }
    }
    
private:
    id<MTLCommandQueue> queue_;
    id<MTLCommandBuffer> commandBuffer_;
    id<MTLRenderCommandEncoder> encoder_;
    MetalPipeline* currentPipeline_ = nullptr;
    MetalBuffer* currentIndexBuffer_ = nullptr;
    MTLIndexType currentIndexType_ = MTLIndexTypeUInt32;
    uint64_t currentIndexOffset_ = 0;
};

// ===== Metal Device =====
class MetalDevice : public Device {
public:
    MetalDevice() {
        device_ = MTLCreateSystemDefaultDevice();
        if (!device_) {
            std::cerr << "[rhi/metal] Failed to create Metal device" << std::endl;
            return;
        }
        queue_ = [device_ newCommandQueue];
        std::cout << "[rhi/metal] Device: " << [[device_ name] UTF8String] << std::endl;
    }
    
    BackendType getBackendType() const override { return BackendType::Metal; }
    
    std::string getDeviceName() const override {
        return [[device_ name] UTF8String];
    }
    
    BufferHandle createBuffer(const BufferDesc& desc) override {
        return std::make_shared<MetalBuffer>(device_, desc);
    }
    
    TextureHandle createTexture(const TextureDesc& desc) override {
        return std::make_shared<MetalTexture>(device_, desc);
    }
    
    SamplerHandle createSampler(const SamplerDesc& desc) override {
        return std::make_shared<MetalSampler>(device_, desc);
    }
    
    ShaderHandle createShader(const ShaderDesc& desc) override {
        // For Metal, we need the library to be set first via compileShaderLibrary
        if (!shaderLibrary_) {
            std::cerr << "[rhi/metal] No shader library compiled" << std::endl;
            return nullptr;
        }
        return std::make_shared<MetalShader>(device_, shaderLibrary_, desc);
    }
    
    PipelineHandle createPipeline(const PipelineDesc& desc) override {
        return std::make_shared<MetalPipeline>(device_, desc);
    }
    
    SwapchainHandle createSwapchain(const SwapchainDesc& desc) override {
        return std::make_unique<MetalSwapchain>(device_, desc);
    }
    
    CommandBufferHandle createCommandBuffer() override {
        return std::make_unique<MetalCommandBuffer>(queue_);
    }
    
    void submit(CommandBuffer* cmdBuffer) override {
        auto* mtlCmd = static_cast<MetalCommandBuffer*>(cmdBuffer);
        [mtlCmd->getCommandBuffer() commit];
    }
    
    void submitAndPresent(CommandBuffer* cmdBuffer, Swapchain* swapchain) {
        auto* mtlCmd = static_cast<MetalCommandBuffer*>(cmdBuffer);
        auto* mtlSwap = static_cast<MetalSwapchain*>(swapchain);
        
        if (mtlSwap->getCurrentDrawable()) {
            [mtlCmd->getCommandBuffer() presentDrawable:mtlSwap->getCurrentDrawable()];
        }
        [mtlCmd->getCommandBuffer() commit];
    }
    
    void waitIdle() override {
        // Create a temporary command buffer and wait for it
        id<MTLCommandBuffer> cmdBuffer = [queue_ commandBuffer];
        [cmdBuffer commit];
        [cmdBuffer waitUntilCompleted];
    }
    
    void beginFrame() override {
        // Metal doesn't need explicit frame begin
    }
    
    void endFrame() override {
        // Metal doesn't need explicit frame end
    }
    
    void* getNativeDevice() const override { return (__bridge void*)device_; }
    void* getNativeQueue() const override { return (__bridge void*)queue_; }
    
    // Metal-specific: compile shader library from source
    bool compileShaderLibrary(const std::string& source) {
        NSError* error = nil;
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        
        NSString* sourceStr = [NSString stringWithUTF8String:source.c_str()];
        shaderLibrary_ = [device_ newLibraryWithSource:sourceStr options:options error:&error];
        
        if (!shaderLibrary_) {
            std::cerr << "[rhi/metal] Failed to compile shader library: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return false;
        }
        
        std::cout << "[rhi/metal] Shader library compiled successfully" << std::endl;
        return true;
    }
    
    id<MTLLibrary> getShaderLibrary() const { return shaderLibrary_; }
    
private:
    id<MTLDevice> device_;
    id<MTLCommandQueue> queue_;
    id<MTLLibrary> shaderLibrary_;
};

// ===== Factory Function =====
DeviceHandle createMetalDevice() {
    return std::make_unique<MetalDevice>();
}

}  // namespace luma::rhi

#endif  // __APPLE__
