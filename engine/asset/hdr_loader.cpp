// HDR Image Loader Implementation
// Supports Radiance HDR (.hdr) format

#include "hdr_loader.h"
#include <fstream>
#include <cstring>
#include <cmath>
#include <iostream>

namespace luma {

// RGBE to float RGB conversion
static void rgbeToFloat(uint8_t r, uint8_t g, uint8_t b, uint8_t e, float& fr, float& fg, float& fb) {
    if (e == 0) {
        fr = fg = fb = 0.0f;
    } else {
        float f = ldexpf(1.0f, (int)e - 128 - 8);
        fr = r * f;
        fg = g * f;
        fb = b * f;
    }
}

HDRImage loadHDR(const std::string& path) {
    HDRImage result;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[hdr] Failed to open: " << path << std::endl;
        return result;
    }
    
    // Read header
    std::string line;
    bool foundFormat = false;
    
    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") break;
        if (line.find("#?RADIANCE") != std::string::npos || 
            line.find("#?RGBE") != std::string::npos) {
            foundFormat = true;
        }
        if (line.find("FORMAT=32-bit_rle_rgbe") != std::string::npos) {
            foundFormat = true;
        }
    }
    
    if (!foundFormat) {
        std::cerr << "[hdr] Invalid HDR format: " << path << std::endl;
        return result;
    }
    
    // Read resolution line
    std::getline(file, line);
    int width = 0, height = 0;
    if (sscanf(line.c_str(), "-Y %d +X %d", &height, &width) != 2) {
        if (sscanf(line.c_str(), "+Y %d +X %d", &height, &width) != 2) {
            std::cerr << "[hdr] Failed to parse resolution: " << line << std::endl;
            return result;
        }
    }
    
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) {
        std::cerr << "[hdr] Invalid resolution: " << width << "x" << height << std::endl;
        return result;
    }
    
    result.width = width;
    result.height = height;
    result.pixels.resize(width * height * 3);
    
    // Read scanlines
    std::vector<uint8_t> scanline(width * 4);
    
    for (int y = 0; y < height; y++) {
        // Check for new RLE format
        uint8_t header[4];
        file.read(reinterpret_cast<char*>(header), 4);
        
        if (header[0] == 2 && header[1] == 2 && ((header[2] << 8) | header[3]) == width) {
            // New RLE format - read each channel separately
            for (int ch = 0; ch < 4; ch++) {
                int i = 0;
                while (i < width) {
                    uint8_t code;
                    file.read(reinterpret_cast<char*>(&code), 1);
                    
                    if (code > 128) {
                        // Run
                        int count = code - 128;
                        uint8_t value;
                        file.read(reinterpret_cast<char*>(&value), 1);
                        for (int j = 0; j < count && i < width; j++, i++) {
                            scanline[i * 4 + ch] = value;
                        }
                    } else {
                        // Literal
                        int count = code;
                        for (int j = 0; j < count && i < width; j++, i++) {
                            file.read(reinterpret_cast<char*>(&scanline[i * 4 + ch]), 1);
                        }
                    }
                }
            }
        } else {
            // Old format or uncompressed - put header back and read raw
            scanline[0] = header[0];
            scanline[1] = header[1];
            scanline[2] = header[2];
            scanline[3] = header[3];
            file.read(reinterpret_cast<char*>(scanline.data() + 4), (width - 1) * 4);
        }
        
        // Convert RGBE to float RGB
        for (int x = 0; x < width; x++) {
            float r, g, b;
            rgbeToFloat(scanline[x*4], scanline[x*4+1], scanline[x*4+2], scanline[x*4+3], r, g, b);
            
            size_t idx = (y * width + x) * 3;
            result.pixels[idx] = r;
            result.pixels[idx + 1] = g;
            result.pixels[idx + 2] = b;
        }
    }
    
    std::cout << "[hdr] Loaded: " << path << " (" << width << "x" << height << ")" << std::endl;
    return result;
}

// Direction from cubemap face and UV
static void getCubeDirection(int face, float u, float v, float& x, float& y, float& z) {
    // Convert u,v from [0,1] to [-1,1]
    float uc = 2.0f * u - 1.0f;
    float vc = 2.0f * v - 1.0f;
    
    switch (face) {
        case 0: x =  1.0f; y = -vc;   z = -uc;   break;  // +X
        case 1: x = -1.0f; y = -vc;   z =  uc;   break;  // -X
        case 2: x =  uc;   y =  1.0f; z =  vc;   break;  // +Y
        case 3: x =  uc;   y = -1.0f; z = -vc;   break;  // -Y
        case 4: x =  uc;   y = -vc;   z =  1.0f; break;  // +Z
        case 5: x = -uc;   y = -vc;   z = -1.0f; break;  // -Z
    }
    
    // Normalize
    float len = sqrtf(x*x + y*y + z*z);
    x /= len; y /= len; z /= len;
}

// Convert 3D direction to equirectangular UV
static void directionToEquirect(float x, float y, float z, float& u, float& v) {
    // Spherical coordinates
    float theta = atan2f(z, x);  // Azimuth
    float phi = asinf(y);        // Elevation
    
    u = (theta + 3.14159265f) / (2.0f * 3.14159265f);
    v = (phi + 3.14159265f / 2.0f) / 3.14159265f;
}

std::vector<std::vector<float>> equirectToCubemap(const HDRImage& hdr, uint32_t faceSize) {
    std::vector<std::vector<float>> faces(6);
    
    if (!hdr.isValid()) return faces;
    
    for (int face = 0; face < 6; face++) {
        faces[face].resize(faceSize * faceSize * 3);
        
        for (uint32_t y = 0; y < faceSize; y++) {
            for (uint32_t x = 0; x < faceSize; x++) {
                float u = (x + 0.5f) / faceSize;
                float v = (y + 0.5f) / faceSize;
                
                float dx, dy, dz;
                getCubeDirection(face, u, v, dx, dy, dz);
                
                float eu, ev;
                directionToEquirect(dx, dy, dz, eu, ev);
                
                float r, g, b;
                hdr.sample(eu, ev, r, g, b);
                
                size_t idx = (y * faceSize + x) * 3;
                faces[face][idx] = r;
                faces[face][idx + 1] = g;
                faces[face][idx + 2] = b;
            }
        }
    }
    
    return faces;
}

}  // namespace luma
