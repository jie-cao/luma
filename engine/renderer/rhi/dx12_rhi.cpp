// DX12 RHI Backend - Implementation
#if defined(_WIN32)

#include "dx12_rhi.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <iostream>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace luma::rhi {

// Helper macro
inline void ThrowIfFailed(HRESULT hr, const char* msg) {
    if (FAILED(hr)) {
        std::cerr << "[rhi/dx12] " << msg << " (HRESULT: 0x" << std::hex << hr << ")" << std::endl;
    }
}

// ===== DX12 Buffer =====
class DX12Buffer : public Buffer {
public:
    DX12Buffer(ID3D12Device* device, const BufferDesc& desc) 
        : size_(desc.size), usage_(desc.usage), cpuAccess_(desc.cpuAccess) {
        
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = desc.cpuAccess ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = desc.size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        D3D12_RESOURCE_STATES initialState = desc.cpuAccess ? 
            D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
        
        ThrowIfFailed(
            device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                           initialState, nullptr, IID_PPV_ARGS(&resource_)),
            "CreateCommittedResource (Buffer)"
        );
        
        if (desc.debugName) {
            std::wstring wname(desc.debugName, desc.debugName + strlen(desc.debugName));
            resource_->SetName(wname.c_str());
        }
    }
    
    void* getNativeHandle() const override { return resource_.Get(); }
    uint64_t getSize() const override { return size_; }
    BufferUsage getUsage() const override { return usage_; }
    
    void* map() override {
        if (!cpuAccess_ || mapped_) return mapped_;
        D3D12_RANGE readRange = {0, 0};
        ThrowIfFailed(resource_->Map(0, &readRange, &mapped_), "Buffer Map");
        return mapped_;
    }
    
    void unmap() override {
        if (mapped_) {
            resource_->Unmap(0, nullptr);
            mapped_ = nullptr;
        }
    }
    
    void update(const void* data, uint64_t size, uint64_t offset) override {
        if (cpuAccess_) {
            void* dest = map();
            if (dest) {
                memcpy((uint8_t*)dest + offset, data, size);
                unmap();
            }
        }
    }
    
    ID3D12Resource* getResource() const { return resource_.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const { return resource_->GetGPUVirtualAddress(); }
    
private:
    ComPtr<ID3D12Resource> resource_;
    uint64_t size_;
    BufferUsage usage_;
    bool cpuAccess_;
    void* mapped_ = nullptr;
};

// ===== DX12 Texture =====
class DX12Texture : public Texture {
public:
    DX12Texture(ID3D12Device* device, const TextureDesc& desc) 
        : width_(desc.width), height_(desc.height), format_(desc.format) {
        
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = desc.width;
        resourceDesc.Height = desc.height;
        resourceDesc.DepthOrArraySize = desc.arrayLayers;
        resourceDesc.MipLevels = desc.mipLevels;
        resourceDesc.Format = toDXGIFormat(desc.format);
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = toDX12ResourceFlags(desc.usage);
        
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
        D3D12_CLEAR_VALUE* clearValue = nullptr;
        D3D12_CLEAR_VALUE depthClear = {};
        
        if ((uint32_t)desc.usage & (uint32_t)TextureUsage::DepthStencil) {
            depthClear.Format = resourceDesc.Format;
            depthClear.DepthStencil.Depth = 1.0f;
            depthClear.DepthStencil.Stencil = 0;
            clearValue = &depthClear;
            initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        
        ThrowIfFailed(
            device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                           initialState, clearValue, IID_PPV_ARGS(&resource_)),
            "CreateCommittedResource (Texture)"
        );
        
        if (desc.debugName) {
            std::wstring wname(desc.debugName, desc.debugName + strlen(desc.debugName));
            resource_->SetName(wname.c_str());
        }
    }
    
    // Wrap existing resource (for swapchain)
    DX12Texture(ID3D12Resource* resource, TextureFormat format) 
        : format_(format) {
        resource_.Attach(resource);
        resource->AddRef();  // We're taking a reference
        
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        width_ = (uint32_t)desc.Width;
        height_ = desc.Height;
    }
    
    void* getNativeHandle() const override { return resource_.Get(); }
    uint32_t getWidth() const override { return width_; }
    uint32_t getHeight() const override { return height_; }
    TextureFormat getFormat() const override { return format_; }
    
    void upload(const void* data, uint32_t bytesPerRow) override {
        // Would need staging buffer and command list - simplified for now
    }
    
    ID3D12Resource* getResource() const { return resource_.Get(); }
    
    static DXGI_FORMAT toDXGIFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGBA8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::BGRA8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::RGBA16_Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case TextureFormat::RGBA32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case TextureFormat::Depth32_Float: return DXGI_FORMAT_D32_FLOAT;
            case TextureFormat::Depth24_Stencil8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
            default: return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }
    
    static TextureFormat fromDXGIFormat(DXGI_FORMAT format) {
        switch (format) {
            case DXGI_FORMAT_R8G8B8A8_UNORM: return TextureFormat::RGBA8_UNorm;
            case DXGI_FORMAT_B8G8R8A8_UNORM: return TextureFormat::BGRA8_UNorm;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return TextureFormat::RGBA16_Float;
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return TextureFormat::RGBA32_Float;
            case DXGI_FORMAT_D32_FLOAT: return TextureFormat::Depth32_Float;
            default: return TextureFormat::Unknown;
        }
    }
    
    static D3D12_RESOURCE_FLAGS toDX12ResourceFlags(TextureUsage usage) {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if ((uint32_t)usage & (uint32_t)TextureUsage::RenderTarget) 
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if ((uint32_t)usage & (uint32_t)TextureUsage::DepthStencil)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if ((uint32_t)usage & (uint32_t)TextureUsage::ShaderWrite)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        return flags;
    }
    
private:
    ComPtr<ID3D12Resource> resource_;
    uint32_t width_;
    uint32_t height_;
    TextureFormat format_;
};

// ===== DX12 Sampler =====
class DX12Sampler : public Sampler {
public:
    DX12Sampler(const SamplerDesc& desc) : desc_(desc) {
        // Samplers are created via descriptor heap in DX12
        // Store the description for later use
    }
    
    void* getNativeHandle() const override { return nullptr; }
    const SamplerDesc& getDesc() const { return desc_; }
    
    D3D12_SAMPLER_DESC toD3D12Desc() const {
        D3D12_SAMPLER_DESC d = {};
        d.Filter = D3D12_FILTER_ANISOTROPIC;
        d.AddressU = toD3D12AddressMode(desc_.addressU);
        d.AddressV = toD3D12AddressMode(desc_.addressV);
        d.AddressW = toD3D12AddressMode(desc_.addressW);
        d.MaxAnisotropy = desc_.maxAnisotropy;
        d.MaxLOD = D3D12_FLOAT32_MAX;
        return d;
    }
    
    static D3D12_TEXTURE_ADDRESS_MODE toD3D12AddressMode(AddressMode mode) {
        switch (mode) {
            case AddressMode::Repeat: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case AddressMode::MirrorRepeat: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            case AddressMode::ClampToEdge: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case AddressMode::ClampToBorder: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }
    
private:
    SamplerDesc desc_;
};

// ===== DX12 Shader =====
class DX12Shader : public Shader {
public:
    DX12Shader(const ShaderDesc& desc, ComPtr<ID3DBlob> blob) 
        : stage_(desc.stage), blob_(blob) {}
    
    void* getNativeHandle() const override { return blob_.Get(); }
    ShaderStage getStage() const override { return stage_; }
    
    D3D12_SHADER_BYTECODE getBytecode() const {
        return {blob_->GetBufferPointer(), blob_->GetBufferSize()};
    }
    
private:
    ShaderStage stage_;
    ComPtr<ID3DBlob> blob_;
};

// ===== DX12 Pipeline =====
class DX12Pipeline : public Pipeline {
public:
    DX12Pipeline(ID3D12Device* device, const PipelineDesc& desc, 
                 ID3D12RootSignature* rootSignature) 
        : topology_(desc.topology) {
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature;
        
        if (desc.vertexShader) {
            auto* dx12Shader = static_cast<DX12Shader*>(desc.vertexShader.get());
            psoDesc.VS = dx12Shader->getBytecode();
        }
        if (desc.fragmentShader) {
            auto* dx12Shader = static_cast<DX12Shader*>(desc.fragmentShader.get());
            psoDesc.PS = dx12Shader->getBytecode();
        }
        
        // Input layout
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
        for (const auto& attr : desc.vertexLayout.attributes) {
            D3D12_INPUT_ELEMENT_DESC element = {};
            element.SemanticName = attr.semantic;
            element.SemanticIndex = 0;
            element.Format = toDXGIFormat(attr.format);
            element.InputSlot = attr.bufferIndex;
            element.AlignedByteOffset = attr.offset;
            element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            inputElements.push_back(element);
        }
        psoDesc.InputLayout = {inputElements.data(), (UINT)inputElements.size()};
        
        // Rasterizer state
        psoDesc.RasterizerState.FillMode = desc.rasterizer.wireframe ? 
            D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = toCullMode(desc.rasterizer.cullMode);
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        
        // Blend state
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        if (desc.blend.enabled) {
            psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        }
        
        // Depth stencil state
        psoDesc.DepthStencilState.DepthEnable = desc.depthStencil.depthTestEnabled;
        psoDesc.DepthStencilState.DepthWriteMask = desc.depthStencil.depthWriteEnabled ?
            D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = toCompareFunc(desc.depthStencil.depthCompare);
        
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = toTopologyType(desc.topology);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DX12Texture::toDXGIFormat(desc.colorFormat);
        psoDesc.DSVFormat = DX12Texture::toDXGIFormat(desc.depthFormat);
        psoDesc.SampleDesc.Count = 1;
        
        ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_)),
            "CreateGraphicsPipelineState"
        );
    }
    
    void* getNativeHandle() const override { return pipelineState_.Get(); }
    PrimitiveTopology getTopology() const override { return topology_; }
    ID3D12PipelineState* getPipelineState() const { return pipelineState_.Get(); }
    
    static DXGI_FORMAT toDXGIFormat(VertexFormat format) {
        switch (format) {
            case VertexFormat::Float: return DXGI_FORMAT_R32_FLOAT;
            case VertexFormat::Float2: return DXGI_FORMAT_R32G32_FLOAT;
            case VertexFormat::Float3: return DXGI_FORMAT_R32G32B32_FLOAT;
            case VertexFormat::Float4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case VertexFormat::Int: return DXGI_FORMAT_R32_SINT;
            case VertexFormat::Int2: return DXGI_FORMAT_R32G32_SINT;
            case VertexFormat::Int3: return DXGI_FORMAT_R32G32B32_SINT;
            case VertexFormat::Int4: return DXGI_FORMAT_R32G32B32A32_SINT;
            default: return DXGI_FORMAT_R32G32B32_FLOAT;
        }
    }
    
    static D3D12_CULL_MODE toCullMode(CullMode mode) {
        switch (mode) {
            case CullMode::None: return D3D12_CULL_MODE_NONE;
            case CullMode::Front: return D3D12_CULL_MODE_FRONT;
            case CullMode::Back: return D3D12_CULL_MODE_BACK;
            default: return D3D12_CULL_MODE_NONE;
        }
    }
    
    static D3D12_COMPARISON_FUNC toCompareFunc(CompareFunction func) {
        switch (func) {
            case CompareFunction::Never: return D3D12_COMPARISON_FUNC_NEVER;
            case CompareFunction::Less: return D3D12_COMPARISON_FUNC_LESS;
            case CompareFunction::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
            case CompareFunction::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case CompareFunction::Greater: return D3D12_COMPARISON_FUNC_GREATER;
            case CompareFunction::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case CompareFunction::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case CompareFunction::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
            default: return D3D12_COMPARISON_FUNC_LESS;
        }
    }
    
    static D3D12_PRIMITIVE_TOPOLOGY_TYPE toTopologyType(PrimitiveTopology topology) {
        switch (topology) {
            case PrimitiveTopology::TriangleList:
            case PrimitiveTopology::TriangleStrip:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            case PrimitiveTopology::LineList:
            case PrimitiveTopology::LineStrip:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            case PrimitiveTopology::PointList:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            default:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        }
    }
    
    static D3D_PRIMITIVE_TOPOLOGY toPrimitiveTopology(PrimitiveTopology topology) {
        switch (topology) {
            case PrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case PrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            case PrimitiveTopology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case PrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
    }
    
private:
    ComPtr<ID3D12PipelineState> pipelineState_;
    PrimitiveTopology topology_;
};

// ===== DX12 Swapchain =====
class DX12Swapchain : public Swapchain {
public:
    static constexpr UINT BufferCount = 2;
    
    DX12Swapchain(ID3D12Device* device, IDXGIFactory6* factory, 
                  ID3D12CommandQueue* queue, const SwapchainDesc& desc) 
        : width_(desc.window.width), height_(desc.window.height), 
          format_(desc.format), device_(device) {
        
        HWND hwnd = static_cast<HWND>(desc.window.handle);
        
        DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
        swapDesc.Width = desc.window.width;
        swapDesc.Height = desc.window.height;
        swapDesc.Format = DX12Texture::toDXGIFormat(desc.format);
        swapDesc.SampleDesc.Count = 1;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferCount = BufferCount;
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        
        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(
            factory->CreateSwapChainForHwnd(queue, hwnd, &swapDesc, nullptr, nullptr, &swapChain1),
            "CreateSwapChainForHwnd"
        );
        swapChain1.As(&swapChain_);
        
        currentIndex_ = swapChain_->GetCurrentBackBufferIndex();
        
        // Create RTV heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = BufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ThrowIfFailed(
            device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)),
            "CreateDescriptorHeap (RTV)"
        );
        rtvDescSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        // Create RTVs
        createRenderTargets();
    }
    
    TextureHandle getCurrentTexture() override {
        currentIndex_ = swapChain_->GetCurrentBackBufferIndex();
        return renderTargets_[currentIndex_];
    }
    
    uint32_t getCurrentIndex() const override { return currentIndex_; }
    
    void present() override {
        swapChain_->Present(1, 0);
    }
    
    void resize(uint32_t width, uint32_t height) override {
        if (width == 0 || height == 0) return;
        width_ = width;
        height_ = height;
        
        for (auto& rt : renderTargets_) rt.reset();
        
        swapChain_->ResizeBuffers(BufferCount, width, height, 
                                  DX12Texture::toDXGIFormat(format_), 0);
        currentIndex_ = swapChain_->GetCurrentBackBufferIndex();
        
        createRenderTargets();
    }
    
    uint32_t getWidth() const override { return width_; }
    uint32_t getHeight() const override { return height_; }
    TextureFormat getFormat() const override { return format_; }
    
    D3D12_CPU_DESCRIPTOR_HANDLE getRTV(uint32_t index) const {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * rtvDescSize_;
        return handle;
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRTV() const {
        return getRTV(currentIndex_);
    }
    
    ID3D12Resource* getCurrentResource() const {
        return static_cast<DX12Texture*>(renderTargets_[currentIndex_].get())->getResource();
    }
    
private:
    void createRenderTargets() {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        
        for (UINT i = 0; i < BufferCount; i++) {
            ComPtr<ID3D12Resource> buffer;
            swapChain_->GetBuffer(i, IID_PPV_ARGS(&buffer));
            device_->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle);
            
            renderTargets_[i] = std::make_shared<DX12Texture>(buffer.Get(), format_);
            
            rtvHandle.ptr += rtvDescSize_;
        }
    }
    
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    ID3D12Device* device_;
    TextureHandle renderTargets_[BufferCount];
    uint32_t rtvDescSize_;
    uint32_t currentIndex_;
    uint32_t width_;
    uint32_t height_;
    TextureFormat format_;
};

// ===== DX12 Command Buffer =====
class DX12CommandBuffer : public CommandBuffer {
public:
    DX12CommandBuffer(ID3D12Device* device, ID3D12CommandAllocator* allocator,
                      ID3D12RootSignature* rootSignature) 
        : allocator_(allocator), rootSignature_(rootSignature) {
        ThrowIfFailed(
            device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                     allocator, nullptr, IID_PPV_ARGS(&cmdList_)),
            "CreateCommandList"
        );
        cmdList_->Close();  // Start in closed state
    }
    
    void begin() override {
        allocator_->Reset();
        cmdList_->Reset(allocator_, nullptr);
        cmdList_->SetGraphicsRootSignature(rootSignature_);
    }
    
    void end() override {
        cmdList_->Close();
    }
    
    void beginRenderPass(const RenderPassDesc& desc) override {
        // Simplified: assumes single color attachment
        if (!desc.colorAttachments.empty() && desc.colorAttachments[0].texture) {
            auto* dx12Tex = static_cast<DX12Texture*>(desc.colorAttachments[0].texture.get());
            
            // Transition to render target
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = dx12Tex->getResource();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList_->ResourceBarrier(1, &barrier);
            
            // Would need external RTV handle - simplified
        }
    }
    
    void endRenderPass() override {
        // Transition back to present would go here
    }
    
    void setPipeline(const PipelineHandle& pipeline) override {
        currentPipeline_ = static_cast<DX12Pipeline*>(pipeline.get());
        cmdList_->SetPipelineState(currentPipeline_->getPipelineState());
        cmdList_->IASetPrimitiveTopology(DX12Pipeline::toPrimitiveTopology(currentPipeline_->getTopology()));
    }
    
    void setViewport(const Viewport& viewport) override {
        D3D12_VIEWPORT vp = {viewport.x, viewport.y, viewport.width, viewport.height, 
                            viewport.minDepth, viewport.maxDepth};
        cmdList_->RSSetViewports(1, &vp);
    }
    
    void setScissor(const Scissor& scissor) override {
        D3D12_RECT rect = {scissor.x, scissor.y, 
                          (LONG)(scissor.x + scissor.width), 
                          (LONG)(scissor.y + scissor.height)};
        cmdList_->RSSetScissorRects(1, &rect);
    }
    
    void setVertexBuffer(uint32_t slot, const BufferHandle& buffer, uint64_t offset) override {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer.get());
        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = dx12Buffer->getGPUAddress() + offset;
        vbv.SizeInBytes = (UINT)(dx12Buffer->getSize() - offset);
        vbv.StrideInBytes = 60;  // TODO: Get from pipeline
        cmdList_->IASetVertexBuffers(slot, 1, &vbv);
    }
    
    void setIndexBuffer(const BufferHandle& buffer, IndexType type, uint64_t offset) override {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer.get());
        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = dx12Buffer->getGPUAddress() + offset;
        ibv.SizeInBytes = (UINT)(dx12Buffer->getSize() - offset);
        ibv.Format = (type == IndexType::UInt16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        cmdList_->IASetIndexBuffer(&ibv);
    }
    
    void setConstantBuffer(uint32_t slot, const BufferHandle& buffer, ShaderStage stage) override {
        auto* dx12Buffer = static_cast<DX12Buffer*>(buffer.get());
        cmdList_->SetGraphicsRootConstantBufferView(slot, dx12Buffer->getGPUAddress());
    }
    
    void setTexture(uint32_t slot, const TextureHandle& texture, ShaderStage stage) override {
        // Would need SRV descriptor - simplified
    }
    
    void setSampler(uint32_t slot, const SamplerHandle& sampler, ShaderStage stage) override {
        // Would need sampler descriptor - simplified
    }
    
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override {
        cmdList_->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override {
        cmdList_->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    
    void copyBufferToTexture(const BufferHandle& buffer, const TextureHandle& texture) override {
        // Would need copy command - simplified
    }
    
    ID3D12GraphicsCommandList* getCommandList() const { return cmdList_.Get(); }
    
private:
    ComPtr<ID3D12GraphicsCommandList> cmdList_;
    ID3D12CommandAllocator* allocator_;
    ID3D12RootSignature* rootSignature_;
    DX12Pipeline* currentPipeline_ = nullptr;
};

// ===== DX12 Device =====
class DX12Device : public Device {
public:
    DX12Device() {
        // Enable debug layer in debug builds
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
        }
#endif
        
        // Create factory
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2");
        
        // Find adapter
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, 
                                                             IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)))) {
                    char name[256];
                    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, 256, nullptr, nullptr);
                    deviceName_ = name;
                    std::cout << "[rhi/dx12] Device: " << deviceName_ << std::endl;
                    break;
                }
            }
        }
        
        if (!device_) {
            std::cerr << "[rhi/dx12] Failed to create DX12 device" << std::endl;
            return;
        }
        
        // Create command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue_)), "CreateCommandQueue");
        
        // Create command allocator
        ThrowIfFailed(
            device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_)),
            "CreateCommandAllocator"
        );
        
        // Create fence
        ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence");
        fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        
        // Create root signature
        createRootSignature();
    }
    
    ~DX12Device() {
        waitIdle();
        if (fenceEvent_) CloseHandle(fenceEvent_);
    }
    
    BackendType getBackendType() const override { return BackendType::DX12; }
    std::string getDeviceName() const override { return deviceName_; }
    
    BufferHandle createBuffer(const BufferDesc& desc) override {
        return std::make_shared<DX12Buffer>(device_.Get(), desc);
    }
    
    TextureHandle createTexture(const TextureDesc& desc) override {
        return std::make_shared<DX12Texture>(device_.Get(), desc);
    }
    
    SamplerHandle createSampler(const SamplerDesc& desc) override {
        return std::make_shared<DX12Sampler>(desc);
    }
    
    ShaderHandle createShader(const ShaderDesc& desc) override {
        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> error;
        
        const char* target = (desc.stage == ShaderStage::Vertex) ? "vs_5_0" : "ps_5_0";
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        
        HRESULT hr = D3DCompile(desc.code, desc.codeSize, desc.debugName, nullptr, nullptr,
                                desc.entryPoint, target, flags, 0, &blob, &error);
        if (FAILED(hr)) {
            if (error) {
                std::cerr << "[rhi/dx12] Shader compile error: " << (char*)error->GetBufferPointer() << std::endl;
            }
            return nullptr;
        }
        
        return std::make_shared<DX12Shader>(desc, blob);
    }
    
    PipelineHandle createPipeline(const PipelineDesc& desc) override {
        return std::make_shared<DX12Pipeline>(device_.Get(), desc, rootSignature_.Get());
    }
    
    SwapchainHandle createSwapchain(const SwapchainDesc& desc) override {
        return std::make_unique<DX12Swapchain>(device_.Get(), factory_.Get(), queue_.Get(), desc);
    }
    
    CommandBufferHandle createCommandBuffer() override {
        return std::make_unique<DX12CommandBuffer>(device_.Get(), allocator_.Get(), rootSignature_.Get());
    }
    
    void submit(CommandBuffer* cmdBuffer) override {
        auto* dx12Cmd = static_cast<DX12CommandBuffer*>(cmdBuffer);
        ID3D12CommandList* lists[] = {dx12Cmd->getCommandList()};
        queue_->ExecuteCommandLists(1, lists);
    }
    
    void waitIdle() override {
        fenceValue_++;
        queue_->Signal(fence_.Get(), fenceValue_);
        if (fence_->GetCompletedValue() < fenceValue_) {
            fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
    }
    
    void beginFrame() override {}
    void endFrame() override {}
    
    void* getNativeDevice() const override { return device_.Get(); }
    void* getNativeQueue() const override { return queue_.Get(); }
    
    ID3D12RootSignature* getRootSignature() const { return rootSignature_.Get(); }
    
private:
    void createRootSignature() {
        // Simple root signature: CBV at slot 0
        D3D12_ROOT_PARAMETER rootParams[1] = {};
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        
        D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.NumParameters = 1;
        rsDesc.pParameters = rootParams;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ComPtr<ID3DBlob> signature, error;
        D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), 
                                    IID_PPV_ARGS(&rootSignature_));
    }
    
    ComPtr<IDXGIFactory6> factory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12CommandAllocator> allocator_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<ID3D12RootSignature> rootSignature_;
    HANDLE fenceEvent_ = nullptr;
    UINT64 fenceValue_ = 0;
    std::string deviceName_;
};

// ===== Factory Function =====
DeviceHandle createDX12Device() {
    return std::make_unique<DX12Device>();
}

}  // namespace luma::rhi

#endif  // _WIN32
