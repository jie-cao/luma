// IBL Generator - Generate IBL textures from HDR environment maps
// Generates: Irradiance Map, Prefiltered Environment Map, BRDF LUT
#pragma once

#include "engine/asset/hdr_loader.h"
#include <vector>
#include <cstdint>

namespace luma {

// Cubemap data structure
struct Cubemap {
    std::vector<std::vector<float>> faces;  // 6 faces, RGB float data
    uint32_t size = 0;
    uint32_t mipLevels = 1;
    
    bool isValid() const { return size > 0 && faces.size() == 6; }
};

// BRDF LUT (2D texture, RG16F format stored as float)
struct BRDFLut {
    std::vector<float> pixels;  // RG float data (2 floats per pixel)
    uint32_t size = 0;
    
    bool isValid() const { return size > 0 && !pixels.empty(); }
};

class IBLGenerator {
public:
    // Generate irradiance cubemap from environment cubemap
    // This is a diffuse convolution for ambient lighting
    static Cubemap generateIrradiance(const Cubemap& envMap, uint32_t size = 32);
    
    // Generate prefiltered environment cubemap for specular IBL
    // Returns cubemap with mip levels for different roughness values
    static Cubemap generatePrefiltered(const Cubemap& envMap, uint32_t size = 256, uint32_t mipLevels = 5);
    
    // Generate BRDF LUT for Split-Sum approximation
    // Returns 2D texture with F0 scale and bias
    static BRDFLut generateBRDFLut(uint32_t size = 512);
    
private:
    // Importance sampling helpers
    static void importanceSampleGGX(float xi1, float xi2, float roughness, 
                                     float nx, float ny, float nz,
                                     float& hx, float& hy, float& hz);
    
    // Hammersley sequence
    static float radicalInverseVdC(uint32_t bits);
    static void hammersley(uint32_t i, uint32_t N, float& xi1, float& xi2);
    
    // Sample cubemap
    static void sampleCubemap(const Cubemap& cm, float x, float y, float z, 
                              float& r, float& g, float& b);
    
    // Geometry function for IBL
    static float geometrySchlickGGX(float NdotV, float roughness);
    static float geometrySmith(float NdotV, float NdotL, float roughness);
};

}  // namespace luma
