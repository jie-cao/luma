// Unified Renderer Implementation (Metal)
// Complete PBR rendering with Cook-Torrance BRDF

#if defined(__APPLE__)

#include "unified_renderer.h"
#include "engine/asset/model_loader.h"
#include "engine/asset/async_texture_loader.h"
#include "engine/asset/hdr_loader.h"
#include "engine/renderer/ibl_generator.h"
#include "engine/util/file_watcher.h"
#include <fstream>
#include <sstream>
#include <chrono>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace luma {

// ===== Math Helpers =====
namespace math {
    inline void identity(float* m) {
        memset(m, 0, 64);
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    
    inline void multiply(float* out, const float* a, const float* b) {
        float tmp[16];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                tmp[i*4+j] = a[i*4+0]*b[0*4+j] + a[i*4+1]*b[1*4+j] + 
                             a[i*4+2]*b[2*4+j] + a[i*4+3]*b[3*4+j];
            }
        }
        memcpy(out, tmp, 64);
    }
    
    inline void lookAt(float* m, const float* eye, const float* at, const float* up) {
        float zAxis[3] = {eye[0]-at[0], eye[1]-at[1], eye[2]-at[2]};
        float len = sqrtf(zAxis[0]*zAxis[0] + zAxis[1]*zAxis[1] + zAxis[2]*zAxis[2]);
        zAxis[0]/=len; zAxis[1]/=len; zAxis[2]/=len;
        float xAxis[3] = {up[1]*zAxis[2]-up[2]*zAxis[1], up[2]*zAxis[0]-up[0]*zAxis[2], 
                          up[0]*zAxis[1]-up[1]*zAxis[0]};
        len = sqrtf(xAxis[0]*xAxis[0] + xAxis[1]*xAxis[1] + xAxis[2]*xAxis[2]);
        xAxis[0]/=len; xAxis[1]/=len; xAxis[2]/=len;
        float yAxis[3] = {zAxis[1]*xAxis[2]-zAxis[2]*xAxis[1], zAxis[2]*xAxis[0]-zAxis[0]*xAxis[2], 
                          zAxis[0]*xAxis[1]-zAxis[1]*xAxis[0]};
        m[0]=xAxis[0]; m[1]=yAxis[0]; m[2]=zAxis[0]; m[3]=0;
        m[4]=xAxis[1]; m[5]=yAxis[1]; m[6]=zAxis[1]; m[7]=0;
        m[8]=xAxis[2]; m[9]=yAxis[2]; m[10]=zAxis[2]; m[11]=0;
        m[12]=-(xAxis[0]*eye[0]+xAxis[1]*eye[1]+xAxis[2]*eye[2]);
        m[13]=-(yAxis[0]*eye[0]+yAxis[1]*eye[1]+yAxis[2]*eye[2]);
        m[14]=-(zAxis[0]*eye[0]+zAxis[1]*eye[1]+zAxis[2]*eye[2]); m[15]=1;
    }
    
    inline void perspective(float* m, float fov, float aspect, float nearZ, float farZ) {
        float tanHalfFov = tanf(fov / 2.0f);
        memset(m, 0, 64);
        m[0] = 1.0f / (aspect * tanHalfFov);
        m[5] = 1.0f / tanHalfFov;
        m[10] = farZ / (nearZ - farZ);
        m[11] = -1.0f;
        m[14] = (nearZ * farZ) / (nearZ - farZ);
    }
    
    inline void ortho(float* m, float left, float right, float bottom, float top, float nearZ, float farZ) {
        memset(m, 0, 64);
        m[0] = 2.0f / (right - left);
        m[5] = 2.0f / (top - bottom);
        m[10] = 1.0f / (nearZ - farZ);
        m[12] = -(right + left) / (right - left);
        m[13] = -(top + bottom) / (top - bottom);
        m[14] = nearZ / (nearZ - farZ);
        m[15] = 1.0f;
    }
    
    inline void scale(float* m, float sx, float sy, float sz) {
        memset(m, 0, 64);
        m[0] = sx; m[5] = sy; m[10] = sz; m[15] = 1.0f;
    }
    
    // 4x4 matrix inversion (general case)
    inline bool invert(float* out, const float* m) {
        float inv[16];
        
        inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + 
                 m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
        inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - 
                 m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
        inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + 
                 m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
        inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - 
                  m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
        
        inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - 
                 m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
        inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + 
                 m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
        inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - 
                 m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
        inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + 
                  m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
        
        inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] + 
                 m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
        inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] - 
                 m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
        inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] + 
                  m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
        inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] - 
                  m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
        
        inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] - 
                 m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
        inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] + 
                 m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
        inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] - 
                  m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
        inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] + 
                  m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];
        
        float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
        if (fabsf(det) < 1e-10f) return false;
        
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) out[i] = inv[i] * det;
        return true;
    }
}

// ===== Metal GPU Mesh Storage =====
struct MetalMeshData {
    id<MTLBuffer> vertexBuffer;
    id<MTLBuffer> indexBuffer;
    id<MTLTexture> diffuseTexture;
    id<MTLTexture> normalTexture;
    id<MTLTexture> specularTexture;
    uint32_t indexCount;
    float baseColor[3];
    float metallic;
    float roughness;
};

// ===== Renderer Implementation =====
struct UnifiedRenderer::Impl {
    // Metal Core
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    CAMetalLayer* metalLayer = nil;
    
    // Pipelines
    id<MTLLibrary> shaderLibrary = nil;
    id<MTLRenderPipelineState> pbrPipeline = nil;
    id<MTLRenderPipelineState> linePipeline = nil;
    id<MTLDepthStencilState> depthState = nil;
    id<MTLDepthStencilState> lineDepthState = nil;
    id<MTLSamplerState> samplerState = nil;
    
    // Resources
    id<MTLBuffer> constantBuffer = nil;
    id<MTLTexture> depthTexture = nil;
    id<MTLTexture> defaultTexture = nil;
    
    // Grid
    id<MTLBuffer> gridVertexBuffer = nil;
    id<MTLBuffer> axisVertexBuffer = nil;
    uint32_t gridVertexCount = 0;
    uint32_t axisVertexCount = 0;
    
    // Mesh Storage
    std::vector<MetalMeshData> meshStorage;
    
    // Async Texture Loading
    // Maps async request ID to (meshIndex, textureSlot: 0=diffuse, 1=normal, 2=specular)
    std::unordered_map<uint32_t, std::pair<uint32_t, int>> pendingTextures;
    size_t asyncTexturesLoaded = 0;
    
    // Frame State
    id<CAMetalDrawable> currentDrawable = nil;
    id<MTLCommandBuffer> currentCommandBuffer = nil;
    id<MTLRenderCommandEncoder> currentEncoder = nil;
    
    // Constants
    RHISceneConstants constants;
    
    // Scene Graph Camera State
    float viewMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float projMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    float cameraPos[3] = {0, 0, 0};
    bool cameraSet = false;
    
    // Shadow Mapping
    ShadowSettings shadowSettings;
    id<MTLTexture> shadowMap = nil;
    id<MTLRenderPipelineState> shadowPipeline = nil;
    id<MTLDepthStencilState> shadowDepthState = nil;
    id<MTLSamplerState> shadowSampler = nil;
    float lightViewProj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool shadowMapReady = false;
    bool inShadowPass = false;
    
    // IBL (Image-Based Lighting)
    IBLSettings iblSettings;
    id<MTLTexture> irradianceMap = nil;
    id<MTLTexture> prefilteredMap = nil;
    id<MTLTexture> brdfLUT = nil;
    bool iblReady = false;
    
    // Shader Hot-Reload
    FileWatcher shaderWatcher;
    bool shaderHotReloadEnabled = false;
    bool shaderReloadPending = false;
    std::string shaderError;
    std::string shaderBasePath = "engine/renderer/shaders/";
    
    // Post-Processing
    bool postProcessEnabled = true;
    float frameTime = 0.0f;
    id<MTLTexture> hdrRenderTarget = nil;
    id<MTLTexture> bloomTextures[2] = {nil, nil};
    id<MTLRenderPipelineState> postProcessPipeline = nil;
    id<MTLRenderPipelineState> bloomThresholdPipeline = nil;
    id<MTLRenderPipelineState> bloomBlurHPipeline = nil;
    id<MTLRenderPipelineState> bloomBlurVPipeline = nil;
    id<MTLBuffer> postProcessConstantBuffer = nil;
    bool postProcessReady = false;
    
    // Skinned Rendering
    id<MTLRenderPipelineState> skinnedPipeline = nil;
    id<MTLBuffer> boneBuffer = nil;
    static constexpr uint32_t kMaxBones = 128;
    bool skinnedPipelineReady = false;
    
    // State
    uint32_t width = 0;
    uint32_t height = 0;
    bool ready = false;
    
    bool initDevice() {
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            std::cerr << "[unified/metal] Failed to create Metal device" << std::endl;
            return false;
        }
        std::cout << "[unified/metal] Device: " << [[device name] UTF8String] << std::endl;
        
        commandQueue = [device newCommandQueue];
        return true;
    }
    
    void setupMetalLayer(void* windowHandle) {
        metalLayer = [CAMetalLayer layer];
        metalLayer.device = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.framebufferOnly = NO;  // Allow ImGui read
        metalLayer.drawableSize = CGSizeMake(width, height);
        
#if TARGET_OS_OSX
        NSView* view = (__bridge NSView*)windowHandle;
        [view setWantsLayer:YES];
        [view setLayer:metalLayer];
        metalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
#else
        UIView* view = (__bridge UIView*)windowHandle;
        [view.layer addSublayer:metalLayer];
        metalLayer.frame = view.bounds;
        metalLayer.contentsScale = [[UIScreen mainScreen] scale];
#endif
    }
    
    void createDepthTexture() {
        MTLTextureDescriptor* desc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                         width:width
                                        height:height
                                     mipmapped:NO];
        desc.usage = MTLTextureUsageRenderTarget;
        desc.storageMode = MTLStorageModePrivate;
        depthTexture = [device newTextureWithDescriptor:desc];
    }
    
    void createDefaultTexture() {
        MTLTextureDescriptor* desc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                         width:1
                                        height:1
                                     mipmapped:NO];
        desc.usage = MTLTextureUsageShaderRead;
        defaultTexture = [device newTextureWithDescriptor:desc];
        
        uint8_t white[4] = {255, 255, 255, 255};
        MTLRegion region = MTLRegionMake2D(0, 0, 1, 1);
        [defaultTexture replaceRegion:region mipmapLevel:0 withBytes:white bytesPerRow:4];
    }
    
    void createSampler() {
        MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
        desc.minFilter = MTLSamplerMinMagFilterLinear;
        desc.magFilter = MTLSamplerMinMagFilterLinear;
        desc.mipFilter = MTLSamplerMipFilterLinear;
        desc.sAddressMode = MTLSamplerAddressModeRepeat;
        desc.tAddressMode = MTLSamplerAddressModeRepeat;
        desc.maxAnisotropy = 16;
        samplerState = [device newSamplerStateWithDescriptor:desc];
    }
    
    bool loadShaders() {
        NSError* error = nil;
        NSString* shaderSource = nil;
        
        NSArray* paths = @[
            [[NSBundle mainBundle] pathForResource:@"pbr" ofType:@"metal"],
            @"engine/renderer/shaders/pbr.metal",
            [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:
                @"../../../engine/renderer/shaders/pbr.metal"],
        ];
        
        for (NSString* path in paths) {
            if (path && [[NSFileManager defaultManager] fileExistsAtPath:path]) {
                shaderSource = [NSString stringWithContentsOfFile:path 
                                                        encoding:NSUTF8StringEncoding 
                                                           error:&error];
                if (shaderSource) {
                    std::cout << "[unified/metal] Loaded shader: " << [path UTF8String] << std::endl;
                    break;
                }
            }
        }
        
        if (!shaderSource) {
            std::cerr << "[unified/metal] Failed to load shader source" << std::endl;
            return false;
        }
        
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        shaderLibrary = [device newLibraryWithSource:shaderSource options:options error:&error];
        
        if (!shaderLibrary) {
            std::cerr << "[unified/metal] Shader compile error: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool createPipelines() {
        NSError* error = nil;
        
        // PBR Pipeline
        id<MTLFunction> pbrVS = [shaderLibrary newFunctionWithName:@"pbrVertexMain"];
        id<MTLFunction> pbrFS = [shaderLibrary newFunctionWithName:@"pbrFragmentMain"];
        
        if (!pbrVS || !pbrFS) {
            std::cerr << "[unified/metal] Failed to find PBR shader functions" << std::endl;
            return false;
        }
        
        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
        // position: float3 at offset 0
        vertexDesc.attributes[0].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[0].offset = 0;
        vertexDesc.attributes[0].bufferIndex = 1;
        // normal: float3 at offset 12
        vertexDesc.attributes[1].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[1].offset = 12;
        vertexDesc.attributes[1].bufferIndex = 1;
        // tangent: float4 at offset 24
        vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
        vertexDesc.attributes[2].offset = 24;
        vertexDesc.attributes[2].bufferIndex = 1;
        // uv: float2 at offset 40
        vertexDesc.attributes[3].format = MTLVertexFormatFloat2;
        vertexDesc.attributes[3].offset = 40;
        vertexDesc.attributes[3].bufferIndex = 1;
        // color: float3 at offset 48
        vertexDesc.attributes[4].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[4].offset = 48;
        vertexDesc.attributes[4].bufferIndex = 1;
        // Layout
        vertexDesc.layouts[1].stride = sizeof(Vertex);
        vertexDesc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        
        MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipeDesc.vertexFunction = pbrVS;
        pipeDesc.fragmentFunction = pbrFS;
        pipeDesc.vertexDescriptor = vertexDesc;
        pipeDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipeDesc.colorAttachments[0].blendingEnabled = YES;
        pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipeDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        
        pbrPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
        if (!pbrPipeline) {
            std::cerr << "[unified/metal] PBR pipeline error: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return false;
        }
        
        // Depth state
        MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthDesc.depthCompareFunction = MTLCompareFunctionLess;
        depthDesc.depthWriteEnabled = YES;
        depthState = [device newDepthStencilStateWithDescriptor:depthDesc];
        
        // Line Pipeline
        id<MTLFunction> lineVS = [shaderLibrary newFunctionWithName:@"lineVertexMain"];
        id<MTLFunction> lineFS = [shaderLibrary newFunctionWithName:@"lineFragmentMain"];
        
        if (!lineVS || !lineFS) {
            std::cerr << "[unified/metal] Failed to find line shader functions" << std::endl;
            return false;
        }
        
        MTLVertexDescriptor* lineVertexDesc = [[MTLVertexDescriptor alloc] init];
        lineVertexDesc.attributes[0].format = MTLVertexFormatFloat3;
        lineVertexDesc.attributes[0].offset = 0;
        lineVertexDesc.attributes[0].bufferIndex = 1;
        lineVertexDesc.attributes[1].format = MTLVertexFormatFloat4;
        lineVertexDesc.attributes[1].offset = 12;
        lineVertexDesc.attributes[1].bufferIndex = 1;
        lineVertexDesc.layouts[1].stride = 28;
        lineVertexDesc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        
        pipeDesc.vertexFunction = lineVS;
        pipeDesc.fragmentFunction = lineFS;
        pipeDesc.vertexDescriptor = lineVertexDesc;
        
        linePipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
        
        // Line depth state (no write)
        depthDesc.depthWriteEnabled = NO;
        lineDepthState = [device newDepthStencilStateWithDescriptor:depthDesc];
        
        // Shadow Pipeline
        id<MTLFunction> shadowVS = [shaderLibrary newFunctionWithName:@"shadowVertexMain"];
        if (shadowVS) {
            MTLRenderPipelineDescriptor* shadowPipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
            shadowPipeDesc.vertexFunction = shadowVS;
            shadowPipeDesc.fragmentFunction = nil;  // Depth-only pass
            shadowPipeDesc.vertexDescriptor = vertexDesc;  // Same as PBR
            shadowPipeDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
            
            shadowPipeline = [device newRenderPipelineStateWithDescriptor:shadowPipeDesc error:&error];
            if (!shadowPipeline) {
                std::cerr << "[unified/metal] Shadow pipeline error: " 
                          << [[error localizedDescription] UTF8String] << std::endl;
            }
            
            // Shadow depth state
            MTLDepthStencilDescriptor* shadowDepthDesc = [[MTLDepthStencilDescriptor alloc] init];
            shadowDepthDesc.depthCompareFunction = MTLCompareFunctionLess;
            shadowDepthDesc.depthWriteEnabled = YES;
            shadowDepthState = [device newDepthStencilStateWithDescriptor:shadowDepthDesc];
        }
        
        // Create skinned pipeline
        createSkinnedPipeline();
        
        return true;
    }
    
    void createSkinnedPipeline() {
        // Try to load skinned shader
        NSArray* paths = @[
            [NSString stringWithUTF8String:(shaderBasePath + "skinned.metal").c_str()],
            @"engine/renderer/shaders/skinned.metal",
        ];
        
        NSString* shaderSource = nil;
        for (NSString* path in paths) {
            shaderSource = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
            if (shaderSource) break;
        }
        
        if (!shaderSource) {
            std::cerr << "[skinned] Failed to load skinned.metal" << std::endl;
            return;
        }
        
        NSError* error = nil;
        id<MTLLibrary> skinnedLibrary = [device newLibraryWithSource:shaderSource options:nil error:&error];
        if (!skinnedLibrary) {
            std::cerr << "[skinned] Compile error: " << [[error localizedDescription] UTF8String] << std::endl;
            return;
        }
        
        id<MTLFunction> skinnedVS = [skinnedLibrary newFunctionWithName:@"skinnedVertexMain"];
        id<MTLFunction> skinnedFS = [skinnedLibrary newFunctionWithName:@"skinnedFragmentMain"];
        
        if (!skinnedVS || !skinnedFS) {
            std::cerr << "[skinned] Failed to find skinned shader functions" << std::endl;
            return;
        }
        
        // Skinned vertex descriptor
        MTLVertexDescriptor* skinnedVertexDesc = [[MTLVertexDescriptor alloc] init];
        // position: float3 at offset 0
        skinnedVertexDesc.attributes[0].format = MTLVertexFormatFloat3;
        skinnedVertexDesc.attributes[0].offset = 0;
        skinnedVertexDesc.attributes[0].bufferIndex = 1;
        // normal: float3 at offset 12
        skinnedVertexDesc.attributes[1].format = MTLVertexFormatFloat3;
        skinnedVertexDesc.attributes[1].offset = 12;
        skinnedVertexDesc.attributes[1].bufferIndex = 1;
        // tangent: float4 at offset 24
        skinnedVertexDesc.attributes[2].format = MTLVertexFormatFloat4;
        skinnedVertexDesc.attributes[2].offset = 24;
        skinnedVertexDesc.attributes[2].bufferIndex = 1;
        // uv: float2 at offset 40
        skinnedVertexDesc.attributes[3].format = MTLVertexFormatFloat2;
        skinnedVertexDesc.attributes[3].offset = 40;
        skinnedVertexDesc.attributes[3].bufferIndex = 1;
        // color: float3 at offset 48
        skinnedVertexDesc.attributes[4].format = MTLVertexFormatFloat3;
        skinnedVertexDesc.attributes[4].offset = 48;
        skinnedVertexDesc.attributes[4].bufferIndex = 1;
        // boneIndices: uint4 at offset 60
        skinnedVertexDesc.attributes[5].format = MTLVertexFormatUInt4;
        skinnedVertexDesc.attributes[5].offset = 60;
        skinnedVertexDesc.attributes[5].bufferIndex = 1;
        // boneWeights: float4 at offset 76
        skinnedVertexDesc.attributes[6].format = MTLVertexFormatFloat4;
        skinnedVertexDesc.attributes[6].offset = 76;
        skinnedVertexDesc.attributes[6].bufferIndex = 1;
        // Layout (SkinnedVertex size = 92 bytes)
        skinnedVertexDesc.layouts[1].stride = 92;
        skinnedVertexDesc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
        
        MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipeDesc.vertexFunction = skinnedVS;
        pipeDesc.fragmentFunction = skinnedFS;
        pipeDesc.vertexDescriptor = skinnedVertexDesc;
        pipeDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipeDesc.colorAttachments[0].blendingEnabled = YES;
        pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipeDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        
        skinnedPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
        if (!skinnedPipeline) {
            std::cerr << "[skinned] Pipeline error: " << [[error localizedDescription] UTF8String] << std::endl;
            return;
        }
        
        // Create bone buffer (128 bones * 64 bytes per matrix)
        boneBuffer = [device newBufferWithLength:kMaxBones * 64 options:MTLResourceStorageModeShared];
        
        // Initialize to identity matrices
        float* matrices = (float*)[boneBuffer contents];
        for (uint32_t i = 0; i < kMaxBones; i++) {
            float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            memcpy(matrices + i * 16, identity, 64);
        }
        
        skinnedPipelineReady = true;
        std::cout << "[skinned] Skinned rendering pipeline ready (Metal)" << std::endl;
    }
    
    void createShadowMap() {
        uint32_t size = shadowSettings.mapSize;
        
        MTLTextureDescriptor* shadowDesc = [[MTLTextureDescriptor alloc] init];
        shadowDesc.textureType = MTLTextureType2D;
        shadowDesc.width = size;
        shadowDesc.height = size;
        shadowDesc.pixelFormat = MTLPixelFormatDepth32Float;
        shadowDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        shadowDesc.storageMode = MTLStorageModePrivate;
        
        shadowMap = [device newTextureWithDescriptor:shadowDesc];
        
        // Shadow sampler
        MTLSamplerDescriptor* samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.sAddressMode = MTLSamplerAddressModeClampToBorderColor;
        samplerDesc.tAddressMode = MTLSamplerAddressModeClampToBorderColor;
        samplerDesc.borderColor = MTLSamplerBorderColorOpaqueWhite;
        samplerDesc.compareFunction = MTLCompareFunctionLessEqual;
        shadowSampler = [device newSamplerStateWithDescriptor:samplerDesc];
        
        shadowMapReady = shadowMap && shadowSampler;
    }
    
    // Recompile shaders from external file (for hot-reload)
    bool recompileShaders() {
        NSError* error = nil;
        NSString* shaderSource = nil;
        
        // Try to load shader from external file
        NSArray* paths = @[
            [NSString stringWithUTF8String:(shaderBasePath + "pbr.metal").c_str()],
            @"engine/renderer/shaders/pbr.metal",
        ];
        
        for (NSString* path in paths) {
            if (path && [[NSFileManager defaultManager] fileExistsAtPath:path]) {
                shaderSource = [NSString stringWithContentsOfFile:path 
                                                        encoding:NSUTF8StringEncoding 
                                                           error:&error];
                if (shaderSource) {
                    std::cout << "[shader] Loading from: " << [path UTF8String] << std::endl;
                    break;
                }
            }
        }
        
        if (!shaderSource) {
            shaderError = "Failed to load shader source file";
            std::cerr << "[shader] " << shaderError << std::endl;
            return false;
        }
        
        // Compile new library
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        id<MTLLibrary> newLibrary = [device newLibraryWithSource:shaderSource options:options error:&error];
        
        if (!newLibrary) {
            shaderError = [[error localizedDescription] UTF8String];
            std::cerr << "[shader] Compile error: " << shaderError << std::endl;
            return false;
        }
        
        // Get new shader functions
        id<MTLFunction> newVS = [newLibrary newFunctionWithName:@"pbrVertexMain"];
        id<MTLFunction> newFS = [newLibrary newFunctionWithName:@"pbrFragmentMain"];
        id<MTLFunction> newShadowVS = [newLibrary newFunctionWithName:@"shadowVertexMain"];
        
        if (!newVS || !newFS) {
            shaderError = "Failed to get shader functions from library";
            std::cerr << "[shader] " << shaderError << std::endl;
            return false;
        }
        
        // Create new PBR pipeline
        MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
        vertexDesc.attributes[0].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[0].offset = 0;
        vertexDesc.attributes[0].bufferIndex = 0;
        vertexDesc.attributes[1].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[1].offset = 12;
        vertexDesc.attributes[1].bufferIndex = 0;
        vertexDesc.attributes[2].format = MTLVertexFormatFloat4;
        vertexDesc.attributes[2].offset = 24;
        vertexDesc.attributes[2].bufferIndex = 0;
        vertexDesc.attributes[3].format = MTLVertexFormatFloat2;
        vertexDesc.attributes[3].offset = 40;
        vertexDesc.attributes[3].bufferIndex = 0;
        vertexDesc.attributes[4].format = MTLVertexFormatFloat3;
        vertexDesc.attributes[4].offset = 48;
        vertexDesc.attributes[4].bufferIndex = 0;
        vertexDesc.layouts[0].stride = 60;
        vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        
        MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipeDesc.vertexFunction = newVS;
        pipeDesc.fragmentFunction = newFS;
        pipeDesc.vertexDescriptor = vertexDesc;
        pipeDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pipeDesc.colorAttachments[0].blendingEnabled = YES;
        pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipeDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        
        id<MTLRenderPipelineState> newPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
        if (!newPipeline) {
            shaderError = [[error localizedDescription] UTF8String];
            std::cerr << "[shader] Pipeline error: " << shaderError << std::endl;
            return false;
        }
        
        // Create new shadow pipeline (if shadow VS available)
        id<MTLRenderPipelineState> newShadowPipeline = nil;
        if (newShadowVS) {
            MTLRenderPipelineDescriptor* shadowPipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
            shadowPipeDesc.vertexFunction = newShadowVS;
            shadowPipeDesc.fragmentFunction = nil;
            shadowPipeDesc.vertexDescriptor = vertexDesc;
            shadowPipeDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
            
            newShadowPipeline = [device newRenderPipelineStateWithDescriptor:shadowPipeDesc error:&error];
        }
        
        // Swap in new pipelines
        shaderLibrary = newLibrary;
        pbrPipeline = newPipeline;
        if (newShadowPipeline) shadowPipeline = newShadowPipeline;
        shaderError.clear();
        
        std::cout << "[shader] Shaders recompiled successfully" << std::endl;
        return true;
    }
    
    void createGridData() {
        struct LineVertex { float pos[3]; float color[4]; };
        std::vector<LineVertex> vertices;
        
        float gridExtent = 1000.0f;
        float gridColor[4] = {0.25f, 0.25f, 0.28f, 0.4f};
        float majorColor[4] = {0.35f, 0.35f, 0.4f, 0.6f};
        
        for (int i = -100; i <= 100; i++) {
            float z = (float)i * 10.0f;
            float* col = (i % 10 == 0) ? majorColor : gridColor;
            if (i == 0) continue;
            vertices.push_back({{-gridExtent, 0, z}, {col[0], col[1], col[2], col[3]}});
            vertices.push_back({{gridExtent, 0, z}, {col[0], col[1], col[2], col[3]}});
        }
        
        for (int i = -100; i <= 100; i++) {
            float x = (float)i * 10.0f;
            float* col = (i % 10 == 0) ? majorColor : gridColor;
            if (i == 0) continue;
            vertices.push_back({{x, 0, -gridExtent}, {col[0], col[1], col[2], col[3]}});
            vertices.push_back({{x, 0, gridExtent}, {col[0], col[1], col[2], col[3]}});
        }
        
        gridVertexCount = (uint32_t)vertices.size();
        gridVertexBuffer = [device newBufferWithBytes:vertices.data()
                                               length:vertices.size() * sizeof(LineVertex)
                                              options:MTLResourceStorageModeShared];
        
        // Axis vertices
        std::vector<LineVertex> axisVerts;
        // X axis - Red
        axisVerts.push_back({{-1.0f, 0.001f, 0}, {0.5f, 0.15f, 0.15f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.5f, 0.15f, 0.15f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.9f, 0.2f, 0.2f, 1.0f}});
        axisVerts.push_back({{1.0f, 0.001f, 0}, {0.9f, 0.2f, 0.2f, 1.0f}});
        // Y axis - Green
        axisVerts.push_back({{0, 0, 0}, {0.2f, 0.9f, 0.2f, 1.0f}});
        axisVerts.push_back({{0, 1.0f, 0}, {0.2f, 0.9f, 0.2f, 1.0f}});
        // Z axis - Blue
        axisVerts.push_back({{0, 0.001f, -1.0f}, {0.15f, 0.25f, 0.5f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.15f, 0.25f, 0.5f, 0.8f}});
        axisVerts.push_back({{0, 0.001f, 0}, {0.2f, 0.4f, 0.9f, 1.0f}});
        axisVerts.push_back({{0, 0.001f, 1.0f}, {0.2f, 0.4f, 0.9f, 1.0f}});
        
        axisVertexCount = (uint32_t)axisVerts.size();
        axisVertexBuffer = [device newBufferWithBytes:axisVerts.data()
                                               length:axisVerts.size() * sizeof(LineVertex)
                                              options:MTLResourceStorageModeShared];
        
        std::cout << "[unified/metal] Grid ready (" << gridVertexCount << " vertices)" << std::endl;
    }
    
    id<MTLTexture> uploadTexture(const TextureData& tex, const char* name) {
        if (tex.pixels.empty()) {
            return defaultTexture;
        }
        
        MTLTextureDescriptor* desc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                         width:tex.width
                                        height:tex.height
                                     mipmapped:NO];
        desc.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
        
        MTLRegion region = MTLRegionMake2D(0, 0, tex.width, tex.height);
        [texture replaceRegion:region mipmapLevel:0 withBytes:tex.pixels.data() 
                   bytesPerRow:tex.width * 4];
        
        std::cout << "[unified/metal] " << name << ": " << tex.width << "x" << tex.height << std::endl;
        return texture;
    }
    
    // ===== Post-Processing =====
    
    void createPostProcessResources() {
        createHDRRenderTarget();
        createPostProcessPipelines();
        
        // Constant buffer for post-process parameters (256 bytes)
        postProcessConstantBuffer = [device newBufferWithLength:256 options:MTLResourceStorageModeShared];
        
        postProcessReady = true;
        std::cout << "[post] Post-processing resources created (Metal)" << std::endl;
    }
    
    void createHDRRenderTarget() {
        // HDR render target (RGBA16F)
        MTLTextureDescriptor* hdrDesc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                         width:width
                                        height:height
                                     mipmapped:NO];
        hdrDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        hdrDesc.storageMode = MTLStorageModePrivate;
        hdrRenderTarget = [device newTextureWithDescriptor:hdrDesc];
        
        // Bloom textures (half resolution)
        uint32_t bloomWidth = width / 2;
        uint32_t bloomHeight = height / 2;
        MTLTextureDescriptor* bloomDesc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                         width:bloomWidth
                                        height:bloomHeight
                                     mipmapped:NO];
        bloomDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        bloomDesc.storageMode = MTLStorageModePrivate;
        
        bloomTextures[0] = [device newTextureWithDescriptor:bloomDesc];
        bloomTextures[1] = [device newTextureWithDescriptor:bloomDesc];
    }
    
    void createPostProcessPipelines() {
        // Compile post-process shaders
        NSString* shaderPath = [NSString stringWithUTF8String:(shaderBasePath + "post_process.metal").c_str()];
        NSString* source = [NSString stringWithContentsOfFile:shaderPath 
                                                     encoding:NSUTF8StringEncoding error:nil];
        
        if (!source) {
            std::cout << "[post] Using embedded Metal post-process shader" << std::endl;
            source = getEmbeddedPostProcessShaderMetal();
        }
        
        NSError* error = nil;
        id<MTLLibrary> ppLibrary = [device newLibraryWithSource:source options:nil error:&error];
        if (!ppLibrary) {
            std::cerr << "[post] Failed to compile post-process shaders: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return;
        }
        
        id<MTLFunction> vsFunc = [ppLibrary newFunctionWithName:@"postProcessVertex"];
        id<MTLFunction> psComposite = [ppLibrary newFunctionWithName:@"postProcessFragment"];
        id<MTLFunction> psThreshold = [ppLibrary newFunctionWithName:@"bloomThresholdFragment"];
        id<MTLFunction> psBlurH = [ppLibrary newFunctionWithName:@"blurHFragment"];
        id<MTLFunction> psBlurV = [ppLibrary newFunctionWithName:@"blurVFragment"];
        
        if (!vsFunc || !psComposite) {
            std::cerr << "[post] Failed to find post-process shader functions" << std::endl;
            return;
        }
        
        // Composite pipeline (output to swapchain BGRA8)
        MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipeDesc.vertexFunction = vsFunc;
        pipeDesc.fragmentFunction = psComposite;
        pipeDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        postProcessPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
        
        // Bloom threshold pipeline (output to RGBA16F)
        if (psThreshold) {
            pipeDesc.fragmentFunction = psThreshold;
            pipeDesc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA16Float;
            bloomThresholdPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:nil];
        }
        
        // Blur H pipeline
        if (psBlurH) {
            pipeDesc.fragmentFunction = psBlurH;
            bloomBlurHPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:nil];
        }
        
        // Blur V pipeline
        if (psBlurV) {
            pipeDesc.fragmentFunction = psBlurV;
            bloomBlurVPipeline = [device newRenderPipelineStateWithDescriptor:pipeDesc error:nil];
        }
        
        std::cout << "[post] Post-process pipelines created (Metal)" << std::endl;
    }
    
    NSString* getEmbeddedPostProcessShaderMetal() {
        return @R"(
#include <metal_stdlib>
using namespace metal;

struct PostProcessConstants {
    float bloomThreshold;
    float bloomIntensity;
    float bloomRadius;
    float exposure;
    float gamma;
    float saturation;
    float contrast;
    float brightness;
    float3 liftColor;
    float vignetteIntensity;
    float3 gammaColor;
    float vignetteRadius;
    float3 gainColor;
    float chromaticAberration;
    float filmGrainIntensity;
    float filmGrainSize;
    uint enabledEffects;
    uint toneMappingMode;
    float screenWidth;
    float screenHeight;
    float time;
    float _pad;
};

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

// Full-screen triangle
vertex VertexOut postProcessVertex(uint vertexID [[vertex_id]]) {
    VertexOut out;
    out.uv = float2((vertexID << 1) & 2, vertexID & 2);
    out.position = float4(out.uv * 2.0 - 1.0, 0.0, 1.0);
    out.uv.y = 1.0 - out.uv.y;
    return out;
}

// ACES tone mapping
float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

float vignette(float2 uv, float intensity, float radius) {
    float2 center = uv - 0.5;
    float dist = length(center);
    return 1.0 - smoothstep(radius, radius + 0.5, dist) * intensity;
}

float grain(float2 uv, float t, float intensity) {
    float noise = fract(sin(dot(uv + t, float2(12.9898, 78.233))) * 43758.5453);
    return (noise - 0.5) * intensity;
}

// Composite pass
fragment float4 postProcessFragment(VertexOut in [[stage_in]],
                                    constant PostProcessConstants& params [[buffer(0)]],
                                    texture2d<float> sceneTexture [[texture(0)]],
                                    texture2d<float> bloomTexture [[texture(1)]],
                                    sampler linearSampler [[sampler(0)]]) {
    float3 scene = sceneTexture.sample(linearSampler, in.uv).rgb;
    float3 bloom = bloomTexture.sample(linearSampler, in.uv).rgb;
    
    float3 color = scene + bloom * params.bloomIntensity;
    color *= params.exposure;
    
    if (params.toneMappingMode > 0) {
        color = ACESFilm(color);
    }
    
    if ((params.enabledEffects & 8) != 0) {
        color *= vignette(in.uv, params.vignetteIntensity, params.vignetteRadius);
    }
    
    if ((params.enabledEffects & 32) != 0) {
        color += grain(in.uv, params.time, params.filmGrainIntensity);
    }
    
    color = pow(max(color, 0.0), 1.0 / params.gamma);
    return float4(color, 1.0);
}

// Bloom threshold
fragment float4 bloomThresholdFragment(VertexOut in [[stage_in]],
                                       constant PostProcessConstants& params [[buffer(0)]],
                                       texture2d<float> sceneTexture [[texture(0)]],
                                       sampler linearSampler [[sampler(0)]]) {
    float3 color = sceneTexture.sample(linearSampler, in.uv).rgb;
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    if (brightness > params.bloomThreshold) {
        return float4(color * (brightness - params.bloomThreshold) / brightness, 1.0);
    }
    return float4(0, 0, 0, 1);
}

constant float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

// Horizontal blur
fragment float4 blurHFragment(VertexOut in [[stage_in]],
                              constant PostProcessConstants& params [[buffer(0)]],
                              texture2d<float> tex [[texture(0)]],
                              sampler linearSampler [[sampler(0)]]) {
    float2 texelSize = 1.0 / float2(params.screenWidth * 0.5, params.screenHeight * 0.5);
    float3 result = tex.sample(linearSampler, in.uv).rgb * weights[0];
    for (int i = 1; i < 5; i++) {
        result += tex.sample(linearSampler, in.uv + float2(texelSize.x * i * params.bloomRadius, 0)).rgb * weights[i];
        result += tex.sample(linearSampler, in.uv - float2(texelSize.x * i * params.bloomRadius, 0)).rgb * weights[i];
    }
    return float4(result, 1.0);
}

// Vertical blur
fragment float4 blurVFragment(VertexOut in [[stage_in]],
                              constant PostProcessConstants& params [[buffer(0)]],
                              texture2d<float> tex [[texture(0)]],
                              sampler linearSampler [[sampler(0)]]) {
    float2 texelSize = 1.0 / float2(params.screenWidth * 0.5, params.screenHeight * 0.5);
    float3 result = tex.sample(linearSampler, in.uv).rgb * weights[0];
    for (int i = 1; i < 5; i++) {
        result += tex.sample(linearSampler, in.uv + float2(0, texelSize.y * i * params.bloomRadius)).rgb * weights[i];
        result += tex.sample(linearSampler, in.uv - float2(0, texelSize.y * i * params.bloomRadius)).rgb * weights[i];
    }
    return float4(result, 1.0);
}
)";
    }
    
    void resizePostProcessTargets() {
        if (!postProcessReady) return;
        
        hdrRenderTarget = nil;
        bloomTextures[0] = nil;
        bloomTextures[1] = nil;
        
        createHDRRenderTarget();
    }
    
    void applyPostProcess(id<MTLCommandBuffer> commandBuffer, id<CAMetalDrawable> drawable) {
        if (!postProcessReady || !postProcessEnabled) return;
        if (!postProcessPipeline || !bloomThresholdPipeline || !bloomBlurHPipeline || !bloomBlurVPipeline) return;
        
        uint32_t bloomWidth = width / 2;
        uint32_t bloomHeight = height / 2;
        
        // Pass 1: Extract bright pixels
        {
            MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = bloomTextures[0];
            passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
            
            id<MTLRenderCommandEncoder> enc = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [enc setRenderPipelineState:bloomThresholdPipeline];
            [enc setFragmentBuffer:postProcessConstantBuffer offset:0 atIndex:0];
            [enc setFragmentTexture:hdrRenderTarget atIndex:0];
            [enc setFragmentSamplerState:samplerState atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [enc endEncoding];
        }
        
        // Pass 2: Horizontal blur
        {
            MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = bloomTextures[1];
            passDesc.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            
            id<MTLRenderCommandEncoder> enc = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [enc setRenderPipelineState:bloomBlurHPipeline];
            [enc setFragmentBuffer:postProcessConstantBuffer offset:0 atIndex:0];
            [enc setFragmentTexture:bloomTextures[0] atIndex:0];
            [enc setFragmentSamplerState:samplerState atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [enc endEncoding];
        }
        
        // Pass 3: Vertical blur
        {
            MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = bloomTextures[0];
            passDesc.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            
            id<MTLRenderCommandEncoder> enc = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [enc setRenderPipelineState:bloomBlurVPipeline];
            [enc setFragmentBuffer:postProcessConstantBuffer offset:0 atIndex:0];
            [enc setFragmentTexture:bloomTextures[1] atIndex:0];
            [enc setFragmentSamplerState:samplerState atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [enc endEncoding];
        }
        
        // Pass 4: Final composite to swapchain
        {
            MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = drawable.texture;
            passDesc.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            
            id<MTLRenderCommandEncoder> enc = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [enc setRenderPipelineState:postProcessPipeline];
            [enc setFragmentBuffer:postProcessConstantBuffer offset:0 atIndex:0];
            [enc setFragmentTexture:hdrRenderTarget atIndex:0];
            [enc setFragmentTexture:bloomTextures[0] atIndex:1];
            [enc setFragmentSamplerState:samplerState atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [enc endEncoding];
        }
    }
};

// ===== Public Interface =====

UnifiedRenderer::UnifiedRenderer() : impl_(std::make_unique<Impl>()) {}

UnifiedRenderer::~UnifiedRenderer() { shutdown(); }

UnifiedRenderer::UnifiedRenderer(UnifiedRenderer&&) noexcept = default;
UnifiedRenderer& UnifiedRenderer::operator=(UnifiedRenderer&&) noexcept = default;

bool UnifiedRenderer::initialize(void* windowHandle, uint32_t width, uint32_t height) {
    impl_->width = width;
    impl_->height = height;
    
    if (!impl_->initDevice()) return false;
    impl_->setupMetalLayer(windowHandle);
    impl_->createDepthTexture();
    impl_->createDefaultTexture();
    impl_->createSampler();
    
    if (!impl_->loadShaders()) return false;
    if (!impl_->createPipelines()) return false;
    
    impl_->constantBuffer = [impl_->device newBufferWithLength:sizeof(RHISceneConstants)
                                                       options:MTLResourceStorageModeShared];
    
    impl_->createGridData();
    impl_->createShadowMap();
    impl_->createPostProcessResources();
    impl_->ready = true;
    
    std::cout << "[unified/metal] Renderer initialized" << std::endl;
    return true;
}

void UnifiedRenderer::shutdown() {
    if (!impl_) return;
    waitForGPU();
    impl_->meshStorage.clear();
    impl_->ready = false;
}

void UnifiedRenderer::resize(uint32_t width, uint32_t height) {
    if (!impl_ || width == 0 || height == 0) return;
    impl_->width = width;
    impl_->height = height;
    impl_->metalLayer.drawableSize = CGSizeMake(width, height);
    impl_->createDepthTexture();
    impl_->resizePostProcessTargets();
}

RHIGPUMesh UnifiedRenderer::uploadMesh(const Mesh& mesh) {
    RHIGPUMesh gpu;
    gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
    gpu.meshIndex = static_cast<uint32_t>(impl_->meshStorage.size());
    memcpy(gpu.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
    gpu.metallic = mesh.metallic;
    gpu.roughness = mesh.roughness;
    
    MetalMeshData metalMesh;
    metalMesh.indexCount = gpu.indexCount;
    memcpy(metalMesh.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
    metalMesh.metallic = mesh.metallic;
    metalMesh.roughness = mesh.roughness;
    
    metalMesh.vertexBuffer = [impl_->device newBufferWithBytes:mesh.vertices.data()
                                                        length:mesh.vertices.size() * sizeof(Vertex)
                                                       options:MTLResourceStorageModeShared];
    
    metalMesh.indexBuffer = [impl_->device newBufferWithBytes:mesh.indices.data()
                                                       length:mesh.indices.size() * sizeof(uint32_t)
                                                      options:MTLResourceStorageModeShared];
    
    metalMesh.diffuseTexture = impl_->uploadTexture(mesh.diffuseTexture, "diffuse");
    metalMesh.normalTexture = impl_->uploadTexture(mesh.normalTexture, "normal");
    metalMesh.specularTexture = impl_->uploadTexture(mesh.specularTexture, "specular");
    
    gpu.hasDiffuseTexture = !mesh.diffuseTexture.pixels.empty();
    gpu.hasNormalTexture = !mesh.normalTexture.pixels.empty();
    gpu.hasSpecularTexture = !mesh.specularTexture.pixels.empty();
    
    impl_->meshStorage.push_back(metalMesh);
    
    return gpu;
}

bool UnifiedRenderer::loadModel(const std::string& path, RHILoadedModel& outModel) {
    std::cout << "[unified/metal] Loading model: " << path << std::endl;
    
    auto result = load_model(path);
    if (!result) {
        std::cerr << "[unified/metal] Failed to load model" << std::endl;
        return false;
    }
    
    outModel.meshes.clear();
    outModel.textureCount = 0;
    outModel.meshStorageStartIndex = impl_->meshStorage.size();
    
    for (size_t i = 0; i < result->meshes.size(); ++i) {
        const auto& mesh = result->meshes[i];
        auto gpuMesh = uploadMesh(mesh);
        outModel.meshes.push_back(gpuMesh);
        if (!mesh.diffuseTexture.pixels.empty()) outModel.textureCount++;
    }
    
    outModel.center[0] = (result->minBounds[0] + result->maxBounds[0]) / 2.0f;
    outModel.center[1] = (result->minBounds[1] + result->maxBounds[1]) / 2.0f;
    outModel.center[2] = (result->minBounds[2] + result->maxBounds[2]) / 2.0f;
    
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];  // Fixed: was maxBounds[1] - maxBounds[1]
    float dz = result->maxBounds[2] - result->minBounds[2];
    outModel.radius = sqrtf(dx*dx + dy*dy + dz*dz) / 2.0f;
    
    outModel.name = path.substr(path.find_last_of("/\\") + 1);
    outModel.totalVerts = result->totalVertices;
    outModel.totalTris = result->totalTriangles;
    
    std::cout << "[unified/metal] Model loaded: " << outModel.meshes.size() << " meshes" << std::endl;
    return true;
}

bool UnifiedRenderer::loadModelAsync(const std::string& path, RHILoadedModel& outModel) {
    std::cout << "[unified/metal] Loading model (async): " << path << std::endl;
    
    auto result = load_model(path);
    if (!result) {
        std::cerr << "[unified/metal] Failed to load model" << std::endl;
        return false;
    }
    
    outModel.meshes.clear();
    outModel.textureCount = 0;
    outModel.meshStorageStartIndex = impl_->meshStorage.size();
    
    auto& asyncLoader = getAsyncTextureLoader();
    
    for (size_t i = 0; i < result->meshes.size(); ++i) {
        const auto& mesh = result->meshes[i];
        
        RHIGPUMesh gpu;
        gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
        gpu.meshIndex = static_cast<uint32_t>(impl_->meshStorage.size());
        memcpy(gpu.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
        gpu.metallic = mesh.metallic;
        gpu.roughness = mesh.roughness;
        
        MetalMeshData metalMesh;
        metalMesh.indexCount = gpu.indexCount;
        memcpy(metalMesh.baseColor, mesh.baseColor, sizeof(mesh.baseColor));
        metalMesh.metallic = mesh.metallic;
        metalMesh.roughness = mesh.roughness;
        
        // Upload vertex/index buffers immediately (fast)
        metalMesh.vertexBuffer = [impl_->device newBufferWithBytes:mesh.vertices.data()
                                                            length:mesh.vertices.size() * sizeof(Vertex)
                                                           options:MTLResourceStorageModeShared];
        
        metalMesh.indexBuffer = [impl_->device newBufferWithBytes:mesh.indices.data()
                                                           length:mesh.indices.size() * sizeof(uint32_t)
                                                          options:MTLResourceStorageModeShared];
        
        // Use default textures initially
        metalMesh.diffuseTexture = impl_->defaultTexture;
        metalMesh.normalTexture = impl_->defaultTexture;
        metalMesh.specularTexture = impl_->defaultTexture;
        
        uint32_t meshIdx = static_cast<uint32_t>(impl_->meshStorage.size());
        
        // Queue textures for async loading
        if (!mesh.diffuseTexture.pixels.empty()) {
            uint32_t reqId = asyncLoader.loadTextureFromMemory(mesh.diffuseTexture.pixels,
                mesh.diffuseTexture.path.empty() ? "diffuse" : mesh.diffuseTexture.path);
            impl_->pendingTextures[reqId] = {meshIdx, 0};  // slot 0 = diffuse
            gpu.hasDiffuseTexture = true;
            outModel.textureCount++;
        }
        if (!mesh.normalTexture.pixels.empty()) {
            uint32_t reqId = asyncLoader.loadTextureFromMemory(mesh.normalTexture.pixels,
                mesh.normalTexture.path.empty() ? "normal" : mesh.normalTexture.path);
            impl_->pendingTextures[reqId] = {meshIdx, 1};  // slot 1 = normal
            gpu.hasNormalTexture = true;
        }
        if (!mesh.specularTexture.pixels.empty()) {
            uint32_t reqId = asyncLoader.loadTextureFromMemory(mesh.specularTexture.pixels,
                mesh.specularTexture.path.empty() ? "specular" : mesh.specularTexture.path);
            impl_->pendingTextures[reqId] = {meshIdx, 2};  // slot 2 = specular
            gpu.hasSpecularTexture = true;
        }
        
        impl_->meshStorage.push_back(metalMesh);
        outModel.meshes.push_back(gpu);
    }
    
    outModel.center[0] = (result->minBounds[0] + result->maxBounds[0]) / 2.0f;
    outModel.center[1] = (result->minBounds[1] + result->maxBounds[1]) / 2.0f;
    outModel.center[2] = (result->minBounds[2] + result->maxBounds[2]) / 2.0f;
    
    float dx = result->maxBounds[0] - result->minBounds[0];
    float dy = result->maxBounds[1] - result->minBounds[1];
    float dz = result->maxBounds[2] - result->minBounds[2];
    outModel.radius = sqrtf(dx*dx + dy*dy + dz*dz) / 2.0f;
    
    outModel.name = path.substr(path.find_last_of("/\\") + 1);
    outModel.totalVerts = result->totalVertices;
    outModel.totalTris = result->totalTriangles;
    
    std::cout << "[unified/metal] Model queued (async): " << outModel.meshes.size() << " meshes, "
              << impl_->pendingTextures.size() << " textures pending" << std::endl;
    return true;
}

void UnifiedRenderer::processAsyncTextures() {
    auto& asyncLoader = getAsyncTextureLoader();
    
    auto completed = asyncLoader.getCompletedTextures();
    for (auto& result : completed) {
        auto it = impl_->pendingTextures.find(result.id);
        if (it == impl_->pendingTextures.end()) continue;
        
        uint32_t meshIdx = it->second.first;
        int slot = it->second.second;
        
        if (meshIdx >= impl_->meshStorage.size()) {
            impl_->pendingTextures.erase(it);
            continue;
        }
        
        if (result.success && !result.data.pixels.empty()) {
            id<MTLTexture> texture = impl_->uploadTexture(result.data, "async");
            
            MetalMeshData& mesh = impl_->meshStorage[meshIdx];
            switch (slot) {
                case 0:  // diffuse
                    mesh.diffuseTexture = texture;
                    break;
                case 1:  // normal
                    mesh.normalTexture = texture;
                    break;
                case 2:  // specular
                    mesh.specularTexture = texture;
                    break;
            }
            impl_->asyncTexturesLoaded++;
        }
        
        impl_->pendingTextures.erase(it);
    }
}

float UnifiedRenderer::getAsyncLoadProgress() const {
    size_t pending = impl_->pendingTextures.size();
    size_t total = pending + impl_->asyncTexturesLoaded;
    if (total == 0) return 1.0f;
    return static_cast<float>(impl_->asyncTexturesLoaded) / static_cast<float>(total);
}

void UnifiedRenderer::beginFrame() {
    // Update frame time for animated effects
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    impl_->frameTime += dt;
    
    impl_->currentDrawable = [impl_->metalLayer nextDrawable];
    if (!impl_->currentDrawable) return;
    
    impl_->currentCommandBuffer = [impl_->commandQueue commandBuffer];
    
    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    
    // If post-processing is enabled, render scene to HDR target
    if (impl_->postProcessEnabled && impl_->postProcessReady && impl_->hdrRenderTarget) {
        passDesc.colorAttachments[0].texture = impl_->hdrRenderTarget;
    } else {
        passDesc.colorAttachments[0].texture = impl_->currentDrawable.texture;
    }
    
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.05, 0.05, 0.08, 1.0);
    passDesc.depthAttachment.texture = impl_->depthTexture;
    passDesc.depthAttachment.loadAction = MTLLoadActionClear;
    passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
    passDesc.depthAttachment.clearDepth = 1.0;
    
    impl_->currentEncoder = [impl_->currentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
    
    MTLViewport viewport = {0, 0, (double)impl_->width, (double)impl_->height, 0.0, 1.0};
    [impl_->currentEncoder setViewport:viewport];
}

void UnifiedRenderer::render(const RHILoadedModel& model, float time, float camDistMultiplier) {
    RHICameraParams cam;
    cam.yaw = time * 0.5f;
    cam.pitch = 0.3f;
    cam.distance = camDistMultiplier;
    render(model, cam);
}

void UnifiedRenderer::render(const RHILoadedModel& model, const RHICameraParams& camera) {
    if (!impl_->ready || model.meshes.empty() || !impl_->currentEncoder) return;
    
    [impl_->currentEncoder setRenderPipelineState:impl_->pbrPipeline];
    [impl_->currentEncoder setDepthStencilState:impl_->depthState];
    [impl_->currentEncoder setFragmentSamplerState:impl_->samplerState atIndex:0];
    
    // Camera calculation
    float target[3] = {
        model.center[0] + camera.targetOffsetX,
        model.center[1] + camera.targetOffsetY,
        model.center[2] + camera.targetOffsetZ
    };
    
    float camDist = model.radius * 2.5f * camera.distance;
    float eye[3] = {
        target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist,
        target[1] + sinf(camera.pitch) * camDist,
        target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist
    };
    
    float up[3] = {0, 1, 0};
    float world[16], view[16], proj[16], wvp[16];
    math::identity(world);
    math::lookAt(view, eye, target, up);
    
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(proj, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = eye[0];
    impl_->constants.cameraPosAndMetal[1] = eye[1];
    impl_->constants.cameraPosAndMetal[2] = eye[2];
    
    // Render meshes using stored indices
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const RHIGPUMesh& gpuMesh = model.meshes[i];
        
        // Use meshIndex to find the correct MetalMeshData
        if (gpuMesh.meshIndex >= impl_->meshStorage.size()) continue;
        const MetalMeshData& metalMesh = impl_->meshStorage[gpuMesh.meshIndex];
        
        impl_->constants.cameraPosAndMetal[3] = gpuMesh.metallic;
        impl_->constants.baseColorAndRough[0] = gpuMesh.baseColor[0];
        impl_->constants.baseColorAndRough[1] = gpuMesh.baseColor[1];
        impl_->constants.baseColorAndRough[2] = gpuMesh.baseColor[2];
        impl_->constants.baseColorAndRough[3] = gpuMesh.roughness;
        
        // Texture flags
        uint32_t texFlags = 0;
        if (gpuMesh.hasDiffuseTexture) texFlags |= 1;
        if (gpuMesh.hasNormalTexture) texFlags |= 2;
        if (gpuMesh.hasSpecularTexture) texFlags |= 4;
        impl_->constants.lightDirAndFlags[3] = *reinterpret_cast<float*>(&texFlags);
        
        memcpy([impl_->constantBuffer contents], &impl_->constants, sizeof(impl_->constants));
        
        [impl_->currentEncoder setVertexBuffer:impl_->constantBuffer offset:0 atIndex:0];
        [impl_->currentEncoder setFragmentBuffer:impl_->constantBuffer offset:0 atIndex:0];
        [impl_->currentEncoder setVertexBuffer:metalMesh.vertexBuffer offset:0 atIndex:1];
        
        [impl_->currentEncoder setFragmentTexture:metalMesh.diffuseTexture atIndex:0];
        [impl_->currentEncoder setFragmentTexture:metalMesh.normalTexture atIndex:1];
        [impl_->currentEncoder setFragmentTexture:metalMesh.specularTexture atIndex:2];
        
        [impl_->currentEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                          indexCount:metalMesh.indexCount
                                           indexType:MTLIndexTypeUInt32
                                         indexBuffer:metalMesh.indexBuffer
                                   indexBufferOffset:0];
    }
}

void UnifiedRenderer::renderGrid(const RHICameraParams& camera, float modelRadius) {
    if (!impl_->ready || !impl_->currentEncoder) return;
    
    [impl_->currentEncoder setRenderPipelineState:impl_->linePipeline];
    [impl_->currentEncoder setDepthStencilState:impl_->lineDepthState];
    
    float target[3] = {camera.targetOffsetX, camera.targetOffsetY, camera.targetOffsetZ};
    float camDist = modelRadius * 2.5f * camera.distance;
    float eye[3] = {
        target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist,
        target[1] + sinf(camera.pitch) * camDist,
        target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist
    };
    
    float up[3] = {0, 1, 0};
    float world[16], view[16], proj[16], wvp[16];
    
    math::lookAt(view, eye, target, up);
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(proj, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    
    // Grid (identity transform)
    math::identity(world);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    memcpy([impl_->constantBuffer contents], &impl_->constants, sizeof(impl_->constants));
    
    [impl_->currentEncoder setVertexBuffer:impl_->constantBuffer offset:0 atIndex:0];
    [impl_->currentEncoder setVertexBuffer:impl_->gridVertexBuffer offset:0 atIndex:1];
    [impl_->currentEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:impl_->gridVertexCount];
    
    // Axes (scaled)
    float axisScale = std::max(camDist * 0.3f, modelRadius * 1.5f);
    axisScale = std::max(axisScale, 10.0f);
    
    math::scale(world, axisScale, axisScale, axisScale);
    math::multiply(wvp, world, view);
    math::multiply(wvp, wvp, proj);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    memcpy([impl_->constantBuffer contents], &impl_->constants, sizeof(impl_->constants));
    
    [impl_->currentEncoder setVertexBuffer:impl_->constantBuffer offset:0 atIndex:0];
    [impl_->currentEncoder setVertexBuffer:impl_->axisVertexBuffer offset:0 atIndex:1];
    [impl_->currentEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:impl_->axisVertexCount];
}

void UnifiedRenderer::setCamera(const RHICameraParams& camera, float sceneRadius) {
    if (!impl_->ready) return;
    
    float target[3] = {camera.targetOffsetX, camera.targetOffsetY, camera.targetOffsetZ};
    float camDist = sceneRadius * 2.5f * camera.distance;
    float eye[3] = {
        target[0] + sinf(camera.yaw) * cosf(camera.pitch) * camDist,
        target[1] + sinf(camera.pitch) * camDist,
        target[2] + cosf(camera.yaw) * cosf(camera.pitch) * camDist
    };
    
    float up[3] = {0, 1, 0};
    math::lookAt(impl_->viewMatrix, eye, target, up);
    
    float nearPlane = std::max(0.01f, camDist * 0.001f);
    float farPlane = std::max(10000.0f, camDist * 10.0f);
    math::perspective(impl_->projMatrix, 3.14159f / 4.0f, (float)impl_->width / impl_->height, nearPlane, farPlane);
    
    impl_->cameraPos[0] = eye[0];
    impl_->cameraPos[1] = eye[1];
    impl_->cameraPos[2] = eye[2];
    impl_->cameraSet = true;
}

void UnifiedRenderer::renderModel(const RHILoadedModel& model, const float* worldMatrix) {
    if (!impl_->ready || model.meshes.empty() || !impl_->currentEncoder || !impl_->cameraSet) return;
    
    [impl_->currentEncoder setRenderPipelineState:impl_->pbrPipeline];
    [impl_->currentEncoder setDepthStencilState:impl_->depthState];
    [impl_->currentEncoder setFragmentSamplerState:impl_->samplerState atIndex:0];
    [impl_->currentEncoder setFragmentSamplerState:impl_->shadowSampler atIndex:1];
    
    // Calculate worldViewProj
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    
    // Shadow parameters
    impl_->constants.shadowParams[0] = impl_->shadowSettings.bias;
    impl_->constants.shadowParams[1] = impl_->shadowSettings.normalBias;
    impl_->constants.shadowParams[2] = impl_->shadowSettings.softness;
    impl_->constants.shadowParams[3] = impl_->shadowSettings.enabled && impl_->shadowMapReady ? 1.0f : 0.0f;
    
    // IBL parameters
    impl_->constants.iblParams[0] = impl_->iblSettings.intensity;
    impl_->constants.iblParams[1] = impl_->iblSettings.rotation;
    impl_->constants.iblParams[2] = (float)(impl_->iblSettings.prefilteredMips - 1);
    impl_->constants.iblParams[3] = impl_->iblSettings.enabled && impl_->iblReady ? 1.0f : 0.0f;
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const RHIGPUMesh& gpuMesh = model.meshes[i];
        
        if (gpuMesh.meshIndex >= impl_->meshStorage.size()) continue;
        const MetalMeshData& metalMesh = impl_->meshStorage[gpuMesh.meshIndex];
        
        impl_->constants.cameraPosAndMetal[3] = gpuMesh.metallic;
        impl_->constants.baseColorAndRough[0] = gpuMesh.baseColor[0];
        impl_->constants.baseColorAndRough[1] = gpuMesh.baseColor[1];
        impl_->constants.baseColorAndRough[2] = gpuMesh.baseColor[2];
        impl_->constants.baseColorAndRough[3] = gpuMesh.roughness;
        
        // Texture flags
        uint32_t texFlags = 0;
        if (gpuMesh.hasDiffuseTexture) texFlags |= 1;
        if (gpuMesh.hasNormalTexture) texFlags |= 2;
        if (gpuMesh.hasSpecularTexture) texFlags |= 4;
        impl_->constants.lightDirAndFlags[3] = *reinterpret_cast<float*>(&texFlags);
        
        memcpy([impl_->constantBuffer contents], &impl_->constants, sizeof(impl_->constants));
        
        [impl_->currentEncoder setVertexBuffer:impl_->constantBuffer offset:0 atIndex:0];
        [impl_->currentEncoder setFragmentBuffer:impl_->constantBuffer offset:0 atIndex:0];
        [impl_->currentEncoder setVertexBuffer:metalMesh.vertexBuffer offset:0 atIndex:1];
        
        [impl_->currentEncoder setFragmentTexture:metalMesh.diffuseTexture atIndex:0];
        [impl_->currentEncoder setFragmentTexture:metalMesh.normalTexture atIndex:1];
        [impl_->currentEncoder setFragmentTexture:metalMesh.specularTexture atIndex:2];
        [impl_->currentEncoder setFragmentTexture:impl_->shadowMap atIndex:3];
        [impl_->currentEncoder setFragmentTexture:impl_->irradianceMap atIndex:4];
        [impl_->currentEncoder setFragmentTexture:impl_->prefilteredMap atIndex:5];
        [impl_->currentEncoder setFragmentTexture:impl_->brdfLUT atIndex:6];
        
        [impl_->currentEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                          indexCount:metalMesh.indexCount
                                           indexType:MTLIndexTypeUInt32
                                         indexBuffer:metalMesh.indexBuffer
                                   indexBufferOffset:0];
    }
}

void UnifiedRenderer::renderSkinnedModel(const RHILoadedModel& model, const float* worldMatrix, const float* boneMatrices) {
    if (!impl_->ready || model.meshes.empty() || !impl_->cameraSet || !impl_->currentEncoder) return;
    
    if (!impl_->skinnedPipelineReady || !impl_->skinnedPipeline) {
        // Fall back to non-skinned rendering
        renderModel(model, worldMatrix);
        return;
    }
    
    [impl_->currentEncoder setRenderPipelineState:impl_->skinnedPipeline];
    [impl_->currentEncoder setDepthStencilState:impl_->depthState];
    [impl_->currentEncoder setFragmentSamplerState:impl_->samplerState atIndex:0];
    if (impl_->shadowSampler) {
        [impl_->currentEncoder setFragmentSamplerState:impl_->shadowSampler atIndex:1];
    }
    
    // Calculate worldViewProj
    float wvp[16];
    math::multiply(wvp, worldMatrix, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    
    impl_->constants.lightDirAndFlags[0] = 0.5f;
    impl_->constants.lightDirAndFlags[1] = -0.7f;
    impl_->constants.lightDirAndFlags[2] = -0.5f;
    impl_->constants.cameraPosAndMetal[0] = impl_->cameraPos[0];
    impl_->constants.cameraPosAndMetal[1] = impl_->cameraPos[1];
    impl_->constants.cameraPosAndMetal[2] = impl_->cameraPos[2];
    
    // Shadow parameters
    impl_->constants.shadowParams[0] = impl_->shadowSettings.bias;
    impl_->constants.shadowParams[1] = impl_->shadowSettings.normalBias;
    impl_->constants.shadowParams[2] = impl_->shadowSettings.softness;
    impl_->constants.shadowParams[3] = impl_->shadowSettings.enabled && impl_->shadowMapReady ? 1.0f : 0.0f;
    
    // IBL parameters
    impl_->constants.iblParams[0] = impl_->iblSettings.intensity;
    impl_->constants.iblParams[1] = impl_->iblSettings.rotation;
    impl_->constants.iblParams[2] = (float)(impl_->iblSettings.prefilteredMips - 1);
    impl_->constants.iblParams[3] = impl_->iblSettings.enabled && impl_->iblReady ? 1.0f : 0.0f;
    
    // Update bone matrices buffer
    memcpy([impl_->boneBuffer contents], boneMatrices, impl_->kMaxBones * 64);
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const RHIGPUMesh& gpuMesh = model.meshes[i];
        
        if (gpuMesh.meshIndex >= impl_->meshStorage.size()) continue;
        const MetalMeshData& metalMesh = impl_->meshStorage[gpuMesh.meshIndex];
        
        impl_->constants.cameraPosAndMetal[3] = gpuMesh.metallic;
        impl_->constants.baseColorAndRough[0] = gpuMesh.baseColor[0];
        impl_->constants.baseColorAndRough[1] = gpuMesh.baseColor[1];
        impl_->constants.baseColorAndRough[2] = gpuMesh.baseColor[2];
        impl_->constants.baseColorAndRough[3] = gpuMesh.roughness;
        
        memcpy([impl_->constantBuffer contents], &impl_->constants, sizeof(impl_->constants));
        
        [impl_->currentEncoder setVertexBuffer:impl_->constantBuffer offset:0 atIndex:0];
        [impl_->currentEncoder setVertexBuffer:metalMesh.vertexBuffer offset:0 atIndex:1];
        [impl_->currentEncoder setVertexBuffer:impl_->boneBuffer offset:0 atIndex:2];  // Bone matrices
        [impl_->currentEncoder setFragmentBuffer:impl_->constantBuffer offset:0 atIndex:0];
        
        [impl_->currentEncoder setFragmentTexture:metalMesh.diffuseTexture atIndex:0];
        [impl_->currentEncoder setFragmentTexture:metalMesh.normalTexture atIndex:1];
        [impl_->currentEncoder setFragmentTexture:metalMesh.specularTexture atIndex:2];
        [impl_->currentEncoder setFragmentTexture:impl_->shadowMap atIndex:3];
        [impl_->currentEncoder setFragmentTexture:impl_->irradianceMap atIndex:4];
        [impl_->currentEncoder setFragmentTexture:impl_->prefilteredMap atIndex:5];
        [impl_->currentEncoder setFragmentTexture:impl_->brdfLUT atIndex:6];
        
        [impl_->currentEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                          indexCount:metalMesh.indexCount
                                           indexType:MTLIndexTypeUInt32
                                         indexBuffer:metalMesh.indexBuffer
                                   indexBufferOffset:0];
    }
}

void UnifiedRenderer::renderModelOutline(const RHILoadedModel& model, const float* worldMatrix, const float* outlineColor) {
    // TODO: Implement outline rendering (stencil-based or post-process)
    // For now, just render normally
    renderModel(model, worldMatrix);
}

void UnifiedRenderer::renderGizmoLines(const float* lines, uint32_t lineCount) {
    if (!impl_->ready || lineCount == 0 || !impl_->cameraSet || !impl_->currentEncoder) return;
    
    // Use the line pipeline
    [impl_->currentEncoder setRenderPipelineState:impl_->linePipeline];
    [impl_->currentEncoder setDepthStencilState:impl_->lineDepthState];
    
    // Create temporary vertex buffer for gizmo lines
    struct LineVertex { float pos[3]; float color[4]; };
    std::vector<LineVertex> vertices;
    vertices.reserve(lineCount * 2);
    
    for (uint32_t i = 0; i < lineCount; i++) {
        const float* line = lines + i * 10;  // startXYZ, endXYZ, RGBA
        vertices.push_back({{line[0], line[1], line[2]}, {line[6], line[7], line[8], line[9]}});
        vertices.push_back({{line[3], line[4], line[5]}, {line[6], line[7], line[8], line[9]}});
    }
    
    // Create temporary buffer
    id<MTLBuffer> tempVB = [impl_->device newBufferWithBytes:vertices.data()
                                                     length:vertices.size() * sizeof(LineVertex)
                                                    options:MTLResourceStorageModeShared];
    
    // Set up constants (identity world matrix, current view/proj)
    float world[16]; math::identity(world);
    float wvp[16];
    math::multiply(wvp, world, impl_->viewMatrix);
    math::multiply(wvp, wvp, impl_->projMatrix);
    
    memcpy(impl_->constants.worldViewProj, wvp, sizeof(wvp));
    memcpy(impl_->constants.world, world, sizeof(world));
    
    [impl_->currentEncoder setVertexBytes:&impl_->constants length:sizeof(impl_->constants) atIndex:1];
    [impl_->currentEncoder setVertexBuffer:tempVB offset:0 atIndex:0];
    [impl_->currentEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:vertices.size()];
}

bool UnifiedRenderer::getViewProjectionInverse(float* outMatrix16) const {
    if (!impl_ || !impl_->cameraSet) return false;
    
    // Compute VP
    float vp[16];
    math::multiply(vp, impl_->viewMatrix, impl_->projMatrix);
    
    // Invert it
    math::invert(outMatrix16, vp);
    return true;
}

void UnifiedRenderer::endFrame() {
    if (!impl_->currentEncoder || !impl_->currentCommandBuffer || !impl_->currentDrawable) return;
    
    [impl_->currentEncoder endEncoding];
    impl_->currentEncoder = nil;
    
    // Apply post-processing if enabled (renders HDR scene to swapchain)
    if (impl_->postProcessEnabled && impl_->postProcessReady && impl_->hdrRenderTarget) {
        impl_->applyPostProcess(impl_->currentCommandBuffer, impl_->currentDrawable);
    }
    
    [impl_->currentCommandBuffer presentDrawable:impl_->currentDrawable];
    [impl_->currentCommandBuffer commit];
    
    impl_->currentCommandBuffer = nil;
    impl_->currentDrawable = nil;
}

uint32_t UnifiedRenderer::getWidth() const { return impl_ ? impl_->width : 0; }
uint32_t UnifiedRenderer::getHeight() const { return impl_ ? impl_->height : 0; }

// ===== Shadow Mapping =====
void UnifiedRenderer::setShadowSettings(const ShadowSettings& settings) {
    if (impl_) impl_->shadowSettings = settings;
}

const ShadowSettings& UnifiedRenderer::getShadowSettings() const {
    static ShadowSettings defaultSettings;
    return impl_ ? impl_->shadowSettings : defaultSettings;
}

void UnifiedRenderer::beginShadowPass(float sceneRadius, const float* sceneCenter) {
    if (!impl_->ready || !impl_->shadowMapReady || !impl_->shadowSettings.enabled) return;
    if (!impl_->shadowPipeline) return;
    
    float center[3] = {0, 0, 0};
    if (sceneCenter) {
        center[0] = sceneCenter[0];
        center[1] = sceneCenter[1];
        center[2] = sceneCenter[2];
    }
    
    // Light direction (normalized)
    float lightDir[3] = {0.5f, -0.7f, -0.5f};
    float len = sqrtf(lightDir[0]*lightDir[0] + lightDir[1]*lightDir[1] + lightDir[2]*lightDir[2]);
    lightDir[0] /= len; lightDir[1] /= len; lightDir[2] /= len;
    
    // Calculate light view matrix
    float lightDist = sceneRadius * impl_->shadowSettings.distance / 10.0f;
    float lightPos[3] = {
        center[0] - lightDir[0] * lightDist,
        center[1] - lightDir[1] * lightDist,
        center[2] - lightDir[2] * lightDist
    };
    
    float lightView[16];
    float up[3] = {0, 1, 0};
    if (fabsf(lightDir[1]) > 0.99f) {
        up[0] = 0; up[1] = 0; up[2] = 1;
    }
    math::lookAt(lightView, lightPos, center, up);
    
    // Orthographic projection
    float orthoSize = sceneRadius * 2.0f;
    float lightProj[16];
    math::ortho(lightProj, -orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, lightDist * 2.0f);
    
    math::multiply(impl_->lightViewProj, lightView, lightProj);
    
    // Create shadow render pass
    MTLRenderPassDescriptor* passDesc = [[MTLRenderPassDescriptor alloc] init];
    passDesc.depthAttachment.texture = impl_->shadowMap;
    passDesc.depthAttachment.loadAction = MTLLoadActionClear;
    passDesc.depthAttachment.storeAction = MTLStoreActionStore;
    passDesc.depthAttachment.clearDepth = 1.0;
    
    // End main encoder if active
    if (impl_->currentEncoder) {
        [impl_->currentEncoder endEncoding];
    }
    
    impl_->currentEncoder = [impl_->currentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
    [impl_->currentEncoder setRenderPipelineState:impl_->shadowPipeline];
    [impl_->currentEncoder setDepthStencilState:impl_->shadowDepthState];
    [impl_->currentEncoder setCullMode:MTLCullModeBack];
    [impl_->currentEncoder setDepthBias:0.005 slopeScale:1.0 clamp:0.0];
    
    MTLViewport viewport = {0, 0, (double)impl_->shadowSettings.mapSize, (double)impl_->shadowSettings.mapSize, 0, 1};
    [impl_->currentEncoder setViewport:viewport];
    
    impl_->inShadowPass = true;
}

void UnifiedRenderer::renderModelShadow(const RHILoadedModel& model, const float* worldMatrix) {
    if (!impl_->ready || !impl_->inShadowPass || model.meshes.empty()) return;
    
    // Set up constants with light VP matrix
    memcpy(impl_->constants.world, worldMatrix, 64);
    memcpy(impl_->constants.lightViewProj, impl_->lightViewProj, 64);
    memcpy([impl_->constantBuffer contents], &impl_->constants, sizeof(impl_->constants));
    
    [impl_->currentEncoder setVertexBuffer:impl_->constantBuffer offset:0 atIndex:0];
    
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const RHIGPUMesh& gpuMesh = model.meshes[i];
        if (gpuMesh.meshIndex >= impl_->meshStorage.size()) continue;
        const MetalMeshData& metalMesh = impl_->meshStorage[gpuMesh.meshIndex];
        
        [impl_->currentEncoder setVertexBuffer:metalMesh.vertexBuffer offset:0 atIndex:1];
        [impl_->currentEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                          indexCount:metalMesh.indexCount
                                           indexType:MTLIndexTypeUInt32
                                         indexBuffer:metalMesh.indexBuffer
                                   indexBufferOffset:0];
    }
}

void UnifiedRenderer::endShadowPass() {
    if (!impl_->ready || !impl_->inShadowPass) return;
    
    [impl_->currentEncoder endEncoding];
    impl_->inShadowPass = false;
    
    // Restart main render pass
    MTLRenderPassDescriptor* passDesc = [[MTLRenderPassDescriptor alloc] init];
    passDesc.colorAttachments[0].texture = impl_->currentDrawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;  // Keep existing content
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.depthAttachment.texture = impl_->depthTexture;
    passDesc.depthAttachment.loadAction = MTLLoadActionLoad;
    passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
    
    impl_->currentEncoder = [impl_->currentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
    
    MTLViewport viewport = {0, 0, (double)impl_->width, (double)impl_->height, 0, 1};
    [impl_->currentEncoder setViewport:viewport];
}

// ===== IBL (Image-Based Lighting) =====
void UnifiedRenderer::setIBLSettings(const IBLSettings& settings) {
    if (impl_) impl_->iblSettings = settings;
}

const IBLSettings& UnifiedRenderer::getIBLSettings() const {
    static IBLSettings defaultSettings;
    return impl_ ? impl_->iblSettings : defaultSettings;
}

bool UnifiedRenderer::loadEnvironmentMap(const std::string& hdrPath) {
    if (!impl_ || !impl_->ready) return false;
    
    // Load HDR image
    HDRImage hdr = loadHDR(hdrPath);
    if (!hdr.isValid()) {
        std::cerr << "[ibl] Failed to load HDR: " << hdrPath << std::endl;
        return false;
    }
    
    // Convert to cubemap
    uint32_t envSize = 512;
    auto cubeFaces = equirectToCubemap(hdr, envSize);
    if (cubeFaces.empty()) {
        std::cerr << "[ibl] Failed to convert HDR to cubemap" << std::endl;
        return false;
    }
    
    // Create environment cubemap
    Cubemap envMap;
    envMap.size = envSize;
    envMap.faces = std::move(cubeFaces);
    
    // Generate IBL textures
    Cubemap irradiance = IBLGenerator::generateIrradiance(envMap, impl_->iblSettings.irradianceSize);
    Cubemap prefiltered = IBLGenerator::generatePrefiltered(envMap, impl_->iblSettings.prefilteredSize, 
                                                             impl_->iblSettings.prefilteredMips);
    BRDFLut brdfLut = IBLGenerator::generateBRDFLut(impl_->iblSettings.brdfLutSize);
    
    // Create irradiance cubemap texture
    {
        MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
        desc.textureType = MTLTextureTypeCube;
        desc.width = irradiance.size;
        desc.height = irradiance.size;
        desc.pixelFormat = MTLPixelFormatRGBA32Float;
        desc.mipmapLevelCount = 1;
        desc.usage = MTLTextureUsageShaderRead;
        
        impl_->irradianceMap = [impl_->device newTextureWithDescriptor:desc];
        
        for (int face = 0; face < 6; face++) {
            std::vector<float> rgba(irradiance.size * irradiance.size * 4);
            for (uint32_t i = 0; i < irradiance.size * irradiance.size; i++) {
                rgba[i*4 + 0] = irradiance.faces[face][i*3 + 0];
                rgba[i*4 + 1] = irradiance.faces[face][i*3 + 1];
                rgba[i*4 + 2] = irradiance.faces[face][i*3 + 2];
                rgba[i*4 + 3] = 1.0f;
            }
            
            MTLRegion region = MTLRegionMake2D(0, 0, irradiance.size, irradiance.size);
            [impl_->irradianceMap replaceRegion:region 
                                      mipmapLevel:0 
                                            slice:face 
                                        withBytes:rgba.data() 
                                      bytesPerRow:irradiance.size * 16 
                                    bytesPerImage:rgba.size() * sizeof(float)];
        }
    }
    
    // Create BRDF LUT texture
    {
        MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
        desc.textureType = MTLTextureType2D;
        desc.width = brdfLut.size;
        desc.height = brdfLut.size;
        desc.pixelFormat = MTLPixelFormatRG32Float;
        desc.mipmapLevelCount = 1;
        desc.usage = MTLTextureUsageShaderRead;
        
        impl_->brdfLUT = [impl_->device newTextureWithDescriptor:desc];
        
        MTLRegion region = MTLRegionMake2D(0, 0, brdfLut.size, brdfLut.size);
        [impl_->brdfLUT replaceRegion:region 
                          mipmapLevel:0 
                            withBytes:brdfLut.pixels.data() 
                          bytesPerRow:brdfLut.size * 8];
    }
    
    // For prefiltered, just use irradiance for now (simplified)
    impl_->prefilteredMap = impl_->irradianceMap;
    
    impl_->iblReady = true;
    std::cout << "[ibl] Environment map loaded: " << hdrPath << std::endl;
    return true;
}

bool UnifiedRenderer::isIBLReady() const {
    return impl_ && impl_->iblReady;
}

// ===== Shader Hot-Reload =====
void UnifiedRenderer::setShaderHotReload(bool enabled) {
    if (!impl_) return;
    
    impl_->shaderHotReloadEnabled = enabled;
    
    if (enabled) {
        // Set up file watcher for shader files
        std::string metalPath = impl_->shaderBasePath + "pbr.metal";
        
        auto reloadCallback = [this](const std::string&) {
            impl_->shaderReloadPending = true;
        };
        
        impl_->shaderWatcher.watchFile(metalPath, reloadCallback);
        
        std::cout << "[shader] Hot-reload enabled" << std::endl;
    } else {
        impl_->shaderWatcher.unwatchAll();
        std::cout << "[shader] Hot-reload disabled" << std::endl;
    }
}

bool UnifiedRenderer::isShaderHotReloadEnabled() const {
    return impl_ && impl_->shaderHotReloadEnabled;
}

bool UnifiedRenderer::reloadShaders() {
    if (!impl_ || !impl_->ready) return false;
    return impl_->recompileShaders();
}

void UnifiedRenderer::checkShaderReload() {
    if (!impl_ || !impl_->shaderHotReloadEnabled) return;
    
    // Check for file changes
    impl_->shaderWatcher.checkChanges();
    
    // If reload is pending, do it
    if (impl_->shaderReloadPending) {
        impl_->shaderReloadPending = false;
        impl_->recompileShaders();
    }
}

const std::string& UnifiedRenderer::getShaderError() const {
    static std::string empty;
    return impl_ ? impl_->shaderError : empty;
}

// ===== Post-Processing =====
void UnifiedRenderer::setPostProcessEnabled(bool enabled) {
    if (impl_) impl_->postProcessEnabled = enabled;
}

bool UnifiedRenderer::isPostProcessEnabled() const {
    return impl_ && impl_->postProcessEnabled;
}

void UnifiedRenderer::setPostProcessParams(const void* constants, size_t size) {
    if (!impl_ || !constants) return;
    
    // Copy constants to the Metal buffer
    if (impl_->postProcessConstantBuffer && size <= 256) {
        memcpy([impl_->postProcessConstantBuffer contents], constants, size);
    }
}

float UnifiedRenderer::getFrameTime() const {
    return impl_ ? impl_->frameTime : 0.0f;
}

void* UnifiedRenderer::getNativeDevice() const { 
    return impl_ ? (__bridge void*)impl_->device : nullptr; 
}

void* UnifiedRenderer::getNativeQueue() const { 
    return impl_ ? (__bridge void*)impl_->commandQueue : nullptr; 
}

void* UnifiedRenderer::getNativeCommandEncoder() const {
    return impl_ ? (__bridge void*)impl_->currentEncoder : nullptr;
}

void UnifiedRenderer::waitForGPU() {
    if (impl_ && impl_->commandQueue) {
        id<MTLCommandBuffer> cmd = [impl_->commandQueue commandBuffer];
        [cmd commit];
        [cmd waitUntilCompleted];
    }
}

}  // namespace luma

#endif  // __APPLE__
