// HDR Image Loader - Load Radiance HDR (.hdr) files
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace luma {

// HDR image data (floating point RGB)
struct HDRImage {
    std::vector<float> pixels;  // RGB float data (3 floats per pixel)
    uint32_t width = 0;
    uint32_t height = 0;
    
    bool isValid() const { return width > 0 && height > 0 && !pixels.empty(); }
    
    // Get pixel at (x, y)
    void getPixel(uint32_t x, uint32_t y, float& r, float& g, float& b) const {
        if (x >= width || y >= height) { r = g = b = 0; return; }
        size_t idx = (y * width + x) * 3;
        r = pixels[idx];
        g = pixels[idx + 1];
        b = pixels[idx + 2];
    }
    
    // Sample with bilinear filtering (u, v in [0, 1])
    void sample(float u, float v, float& r, float& g, float& b) const {
        // Wrap coordinates
        u = u - floorf(u);
        v = v - floorf(v);
        
        float fx = u * (width - 1);
        float fy = v * (height - 1);
        
        uint32_t x0 = (uint32_t)fx;
        uint32_t y0 = (uint32_t)fy;
        uint32_t x1 = (x0 + 1) % width;
        uint32_t y1 = (y0 + 1) % height;
        
        float wx = fx - x0;
        float wy = fy - y0;
        
        float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
        getPixel(x0, y0, r00, g00, b00);
        getPixel(x1, y0, r10, g10, b10);
        getPixel(x0, y1, r01, g01, b01);
        getPixel(x1, y1, r11, g11, b11);
        
        r = (r00 * (1-wx) + r10 * wx) * (1-wy) + (r01 * (1-wx) + r11 * wx) * wy;
        g = (g00 * (1-wx) + g10 * wx) * (1-wy) + (g01 * (1-wx) + g11 * wx) * wy;
        b = (b00 * (1-wx) + b10 * wx) * (1-wy) + (b01 * (1-wx) + b11 * wx) * wy;
    }
};

// Load HDR file (Radiance .hdr format)
// Returns empty image on failure
HDRImage loadHDR(const std::string& path);

// Convert equirectangular HDR to cubemap faces
// faceSize: resolution of each cubemap face
// Returns 6 faces: +X, -X, +Y, -Y, +Z, -Z
std::vector<std::vector<float>> equirectToCubemap(const HDRImage& hdr, uint32_t faceSize);

}  // namespace luma
