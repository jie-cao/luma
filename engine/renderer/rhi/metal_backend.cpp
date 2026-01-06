#include "engine/renderer/rhi/rhi.h"

#include <memory>

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IOS
#import <Metal/Metal.hpp>
#import <QuartzCore/CAMetalLayer.h>

namespace {

class MetalBackend final : public luma::rhi::Backend {
public:
    explicit MetalBackend(const luma::rhi::NativeWindow& window) {
        device_ = MTL::CreateSystemDefaultDevice();
        layer_ = [CAMetalLayer layer];
        layer_.device = device_;
        layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer_.framebufferOnly = YES;
        layer_.drawableSize = CGSizeMake(window.width, window.height);
        layer_.maximumDrawableCount = 2;
        // On macOS/iOS you would attach layer_ to NSView/UIView here using window.handle.
    }

    void render_clear(float r, float g, float b) override {
        if (!layer_) return;
        auto drawable = [layer_ nextDrawable];
        if (!drawable) return;
        auto commandQueue = device_->newCommandQueue();
        auto commandBuffer = commandQueue->commandBuffer();
        MTL::RenderPassDescriptor* desc = MTL::RenderPassDescriptor::renderPassDescriptor();
        auto colorAtt = desc->colorAttachments()->object(0);
        colorAtt->setClearColor(MTL::ClearColor::Make(r, g, b, 1.0));
        colorAtt->setLoadAction(MTL::LoadActionClear);
        colorAtt->setStoreAction(MTL::StoreActionStore);
        colorAtt->setTexture(drawable.texture);

        auto encoder = commandBuffer->renderCommandEncoder(desc);
        encoder->endEncoding();
        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();
    }

    void present() override {
        // Present is done in render_clear via commandBuffer->presentDrawable
    }

private:
    MTL::Device* device_{nullptr};
    CAMetalLayer* layer_{nullptr};
};

}  // namespace
#endif  // TARGET_OS_OSX || TARGET_OS_IOS
#endif  // __APPLE__

namespace luma::rhi {

std::unique_ptr<Backend> create_metal_backend(const NativeWindow& window) {
#if defined(__APPLE__) && (TARGET_OS_OSX || TARGET_OS_IOS)
    return std::make_unique<MetalBackend>(window);
#else
    return nullptr;
#endif
}

}  // namespace luma::rhi

