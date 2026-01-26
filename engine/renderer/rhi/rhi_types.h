// RHI Types - Platform-agnostic rendering types
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace luma::rhi {

// ===== Forward Declarations =====
class Device;
class CommandBuffer;
class Buffer;
class Texture;
class Sampler;
class Shader;
class Pipeline;
class RenderPass;

// ===== Handle Types =====
using BufferHandle = std::shared_ptr<Buffer>;
using TextureHandle = std::shared_ptr<Texture>;
using SamplerHandle = std::shared_ptr<Sampler>;
using ShaderHandle = std::shared_ptr<Shader>;
using PipelineHandle = std::shared_ptr<Pipeline>;

// ===== Enums =====

enum class BackendType {
    DX12,
    Metal,
    Vulkan
};

enum class BufferUsage : uint32_t {
    Vertex      = 1 << 0,
    Index       = 1 << 1,
    Constant    = 1 << 2,
    Storage     = 1 << 3,
    CopySrc     = 1 << 4,
    CopyDst     = 1 << 5,
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(BufferUsage a, BufferUsage b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

enum class TextureFormat {
    Unknown,
    RGBA8_UNorm,
    BGRA8_UNorm,
    RGBA16_Float,
    RGBA32_Float,
    Depth32_Float,
    Depth24_Stencil8,
};

enum class TextureUsage : uint32_t {
    ShaderRead      = 1 << 0,
    ShaderWrite     = 1 << 1,
    RenderTarget    = 1 << 2,
    DepthStencil    = 1 << 3,
    CopySrc         = 1 << 4,
    CopyDst         = 1 << 5,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute
};

enum class VertexFormat {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
};

enum class PrimitiveTopology {
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

enum class CullMode {
    None,
    Front,
    Back,
};

enum class CompareFunction {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

enum class LoadAction {
    Load,
    Clear,
    DontCare,
};

enum class StoreAction {
    Store,
    DontCare,
};

enum class FilterMode {
    Nearest,
    Linear,
};

enum class AddressMode {
    Repeat,
    MirrorRepeat,
    ClampToEdge,
    ClampToBorder,
};

enum class IndexType {
    UInt16,
    UInt32,
};

// ===== Descriptors =====

struct NativeWindow {
    void* handle = nullptr;
    uint32_t width = 1280;
    uint32_t height = 720;
};

struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::Vertex;
    bool cpuAccess = false;  // Allow CPU read/write
    const char* debugName = nullptr;
};

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    TextureFormat format = TextureFormat::RGBA8_UNorm;
    TextureUsage usage = TextureUsage::ShaderRead;
    const char* debugName = nullptr;
};

struct SamplerDesc {
    FilterMode minFilter = FilterMode::Linear;
    FilterMode magFilter = FilterMode::Linear;
    FilterMode mipFilter = FilterMode::Linear;
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    uint32_t maxAnisotropy = 1;
};

struct VertexAttribute {
    const char* semantic = nullptr;  // "POSITION", "NORMAL", etc.
    uint32_t location = 0;
    VertexFormat format = VertexFormat::Float3;
    uint32_t offset = 0;
    uint32_t bufferIndex = 0;
};

struct VertexLayout {
    std::vector<VertexAttribute> attributes;
    uint32_t stride = 0;
};

struct ShaderDesc {
    ShaderStage stage = ShaderStage::Vertex;
    const void* code = nullptr;
    size_t codeSize = 0;
    const char* entryPoint = "main";
    const char* debugName = nullptr;
};

struct BlendState {
    bool enabled = false;
    // Simplified - can be expanded later
};

struct DepthStencilState {
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    CompareFunction depthCompare = CompareFunction::Less;
};

struct RasterizerState {
    CullMode cullMode = CullMode::Back;
    bool wireframe = false;
};

struct PipelineDesc {
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    VertexLayout vertexLayout;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    BlendState blend;
    DepthStencilState depthStencil;
    RasterizerState rasterizer;
    TextureFormat colorFormat = TextureFormat::BGRA8_UNorm;
    TextureFormat depthFormat = TextureFormat::Depth32_Float;
    const char* debugName = nullptr;
};

struct ColorAttachment {
    TextureHandle texture;
    LoadAction loadAction = LoadAction::Clear;
    StoreAction storeAction = StoreAction::Store;
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct DepthAttachment {
    TextureHandle texture;
    LoadAction loadAction = LoadAction::Clear;
    StoreAction storeAction = StoreAction::DontCare;
    float clearDepth = 1.0f;
};

struct RenderPassDesc {
    std::vector<ColorAttachment> colorAttachments;
    DepthAttachment depthAttachment;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct Scissor {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

// ===== Swapchain =====

struct SwapchainDesc {
    NativeWindow window;
    uint32_t bufferCount = 2;
    TextureFormat format = TextureFormat::BGRA8_UNorm;
    bool vsync = true;
};

}  // namespace luma::rhi
