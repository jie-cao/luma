// Simplified RenderGraph placeholder; supports recording clear passes and executing via RHI.
#pragma once

#include <memory>
#include <vector>
#include <utility>

#include "engine/renderer/rhi/rhi.h"

namespace luma::render_graph {

struct ClearPass {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    ResourceHandle target;
};

struct ResourceHandle {
    int id{-1};
};

struct ResourceDesc {
    uint32_t width{0};
    uint32_t height{0};
    rhi::TextureFormat format{rhi::TextureFormat::RGBA8_UNorm};
    rhi::TextureUsage usage{rhi::TextureUsage::ColorAttachment};
};

struct Barrier {
    ResourceHandle resource;
    enum class State {
        Undefined,
        Present,
        ColorAttachment
    } before{State::Undefined}, after{State::Undefined};
};

class RenderGraph {
public:
    explicit RenderGraph(std::shared_ptr<rhi::Backend> backend)
        : backend_(std::move(backend)) {}

    void set_material_params(std::unordered_map<std::string, std::string> params) {
        material_params_ = std::move(params);
    }

    ResourceHandle create_resource(const ResourceDesc& desc) {
        ResourceHandle h;
        h.id = static_cast<int>(resources_.size());
        resources_.push_back(desc);
        if (static_cast<size_t>(h.id) >= states_.size()) {
            states_.push_back(Barrier::State::Undefined);
        }
        return h;
    }

    void add_clear_pass(const ClearPass& pass) { clear_passes_.push_back(pass); }
    void add_barrier(const Barrier& barrier) { barriers_.push_back(barrier); }

    // Convenience: enqueue single clear.
    void clear(float r, float g, float b) { add_clear_pass(ClearPass{r, g, b}); }

    void execute() {
        if (!backend_) return;
        // Translate simple backbuffer barriers.
        backend_->transition_backbuffer(rhi::ResourceState::Present, rhi::ResourceState::ColorAttachment);
        for (const auto& b : barriers_) {
            if (b.resource.id >= 0 && static_cast<size_t>(b.resource.id) < states_.size()) {
                states_[b.resource.id] = b.after;
            }
            if (b.resource.id == -1) {
                backend_->transition_backbuffer(
                    static_cast<rhi::ResourceState>(b.before),
                    static_cast<rhi::ResourceState>(b.after));
            }
        }
        barriers_.clear();
        backend_->bind_material_params(material_params_);
        for (const auto& pass : clear_passes_) {
            backend_->render_clear(pass.r, pass.g, pass.b);
        }
        clear_passes_.clear();
        backend_->transition_backbuffer(rhi::ResourceState::ColorAttachment, rhi::ResourceState::Present);
    }

    void present() { if (backend_) backend_->present(); }

private:
    std::shared_ptr<rhi::Backend> backend_;
    std::vector<ClearPass> clear_passes_;
    std::vector<ResourceDesc> resources_;
    std::vector<Barrier::State> states_;
    std::vector<Barrier> barriers_;
    std::unordered_map<std::string, std::string> material_params_;
};

}  // namespace luma::render_graph

