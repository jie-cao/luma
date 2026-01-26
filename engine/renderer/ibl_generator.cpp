// IBL Generator Implementation
// CPU-based IBL texture generation

#include "ibl_generator.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace luma {

static const float PI = 3.14159265359f;

// Van der Corput radical inverse
float IBLGenerator::radicalInverseVdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

void IBLGenerator::hammersley(uint32_t i, uint32_t N, float& xi1, float& xi2) {
    xi1 = float(i) / float(N);
    xi2 = radicalInverseVdC(i);
}

// Importance sample GGX distribution
void IBLGenerator::importanceSampleGGX(float xi1, float xi2, float roughness,
                                        float nx, float ny, float nz,
                                        float& hx, float& hy, float& hz) {
    float a = roughness * roughness;
    
    float phi = 2.0f * PI * xi1;
    float cosTheta = sqrtf((1.0f - xi2) / (1.0f + (a*a - 1.0f) * xi2));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    
    // Tangent space half vector
    float hx_t = cosf(phi) * sinTheta;
    float hy_t = sinf(phi) * sinTheta;
    float hz_t = cosTheta;
    
    // Create tangent space basis
    float upx = fabsf(nz) < 0.999f ? 0.0f : 1.0f;
    float upy = fabsf(nz) < 0.999f ? 0.0f : 0.0f;
    float upz = fabsf(nz) < 0.999f ? 1.0f : 0.0f;
    
    // tangent = normalize(cross(up, N))
    float tx = upy * nz - upz * ny;
    float ty = upz * nx - upx * nz;
    float tz = upx * ny - upy * nx;
    float tlen = sqrtf(tx*tx + ty*ty + tz*tz);
    tx /= tlen; ty /= tlen; tz /= tlen;
    
    // bitangent = cross(N, tangent)
    float bx = ny * tz - nz * ty;
    float by = nz * tx - nx * tz;
    float bz = nx * ty - ny * tx;
    
    // Transform to world space
    hx = tx * hx_t + bx * hy_t + nx * hz_t;
    hy = ty * hx_t + by * hy_t + ny * hz_t;
    hz = tz * hx_t + bz * hy_t + nz * hz_t;
    
    // Normalize
    float hlen = sqrtf(hx*hx + hy*hy + hz*hz);
    hx /= hlen; hy /= hlen; hz /= hlen;
}

// Sample cubemap in direction (x, y, z)
void IBLGenerator::sampleCubemap(const Cubemap& cm, float x, float y, float z,
                                  float& r, float& g, float& b) {
    // Find which face and UV
    float absX = fabsf(x), absY = fabsf(y), absZ = fabsf(z);
    int face;
    float u, v, ma;
    
    if (absX >= absY && absX >= absZ) {
        ma = absX;
        if (x > 0) { face = 0; u = -z; v = -y; }  // +X
        else       { face = 1; u =  z; v = -y; }  // -X
    } else if (absY >= absX && absY >= absZ) {
        ma = absY;
        if (y > 0) { face = 2; u =  x; v =  z; }  // +Y
        else       { face = 3; u =  x; v = -z; }  // -Y
    } else {
        ma = absZ;
        if (z > 0) { face = 4; u =  x; v = -y; }  // +Z
        else       { face = 5; u = -x; v = -y; }  // -Z
    }
    
    // Convert to [0, 1] UV
    u = 0.5f * (u / ma + 1.0f);
    v = 0.5f * (v / ma + 1.0f);
    
    // Clamp and sample
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));
    
    uint32_t px = std::min((uint32_t)(u * cm.size), cm.size - 1);
    uint32_t py = std::min((uint32_t)(v * cm.size), cm.size - 1);
    
    size_t idx = (py * cm.size + px) * 3;
    r = cm.faces[face][idx];
    g = cm.faces[face][idx + 1];
    b = cm.faces[face][idx + 2];
}

// Geometry function for IBL
float IBLGenerator::geometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float IBLGenerator::geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// Direction from cubemap face and UV
static void getCubeDirection(int face, float u, float v, float& x, float& y, float& z) {
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
    
    float len = sqrtf(x*x + y*y + z*z);
    x /= len; y /= len; z /= len;
}

// Generate irradiance map (diffuse convolution)
Cubemap IBLGenerator::generateIrradiance(const Cubemap& envMap, uint32_t size) {
    Cubemap result;
    if (!envMap.isValid()) return result;
    
    result.size = size;
    result.mipLevels = 1;
    result.faces.resize(6);
    
    const uint32_t SAMPLE_COUNT = 2048;
    
    std::cout << "[ibl] Generating irradiance map (" << size << "x" << size << ")..." << std::endl;
    
    for (int face = 0; face < 6; face++) {
        result.faces[face].resize(size * size * 3);
        
        for (uint32_t y = 0; y < size; y++) {
            for (uint32_t x = 0; x < size; x++) {
                float nx, ny, nz;
                getCubeDirection(face, (x + 0.5f) / size, (y + 0.5f) / size, nx, ny, nz);
                
                // Hemisphere integration
                float irradR = 0, irradG = 0, irradB = 0;
                float totalWeight = 0;
                
                // Create basis
                float upX = fabsf(ny) < 0.999f ? 0.0f : 1.0f;
                float upY = fabsf(ny) < 0.999f ? 1.0f : 0.0f;
                float upZ = 0.0f;
                
                // right = cross(up, normal)
                float rightX = upY * nz - upZ * ny;
                float rightY = upZ * nx - upX * nz;
                float rightZ = upX * ny - upY * nx;
                float rightLen = sqrtf(rightX*rightX + rightY*rightY + rightZ*rightZ);
                rightX /= rightLen; rightY /= rightLen; rightZ /= rightLen;
                
                // up = cross(normal, right)
                upX = ny * rightZ - nz * rightY;
                upY = nz * rightX - nx * rightZ;
                upZ = nx * rightY - ny * rightX;
                
                // Sample hemisphere
                for (uint32_t i = 0; i < SAMPLE_COUNT; i++) {
                    float xi1, xi2;
                    hammersley(i, SAMPLE_COUNT, xi1, xi2);
                    
                    // Cosine-weighted sampling
                    float phi = 2.0f * PI * xi1;
                    float cosTheta = sqrtf(1.0f - xi2);
                    float sinTheta = sqrtf(xi2);
                    
                    // Tangent space to world
                    float sx = sinTheta * cosf(phi);
                    float sy = sinTheta * sinf(phi);
                    float sz = cosTheta;
                    
                    float wx = sx * rightX + sy * upX + sz * nx;
                    float wy = sx * rightY + sy * upY + sz * ny;
                    float wz = sx * rightZ + sy * upZ + sz * nz;
                    
                    float sr, sg, sb;
                    sampleCubemap(envMap, wx, wy, wz, sr, sg, sb);
                    
                    irradR += sr;
                    irradG += sg;
                    irradB += sb;
                    totalWeight += 1.0f;
                }
                
                // Average and apply PI factor
                float invWeight = PI / totalWeight;
                
                size_t idx = (y * size + x) * 3;
                result.faces[face][idx] = irradR * invWeight;
                result.faces[face][idx + 1] = irradG * invWeight;
                result.faces[face][idx + 2] = irradB * invWeight;
            }
        }
    }
    
    std::cout << "[ibl] Irradiance map generated" << std::endl;
    return result;
}

// Generate prefiltered environment map for specular IBL
Cubemap IBLGenerator::generatePrefiltered(const Cubemap& envMap, uint32_t size, uint32_t mipLevels) {
    Cubemap result;
    if (!envMap.isValid()) return result;
    
    // For prefiltered map, we store all mip levels in a single large array per face
    // Each mip level corresponds to a different roughness
    result.size = size;
    result.mipLevels = mipLevels;
    result.faces.resize(6);
    
    const uint32_t SAMPLE_COUNT = 1024;
    
    std::cout << "[ibl] Generating prefiltered env map (" << size << "x" << size 
              << ", " << mipLevels << " mips)..." << std::endl;
    
    // Calculate total size needed (all mip levels)
    size_t totalPixels = 0;
    for (uint32_t mip = 0; mip < mipLevels; mip++) {
        uint32_t mipSize = size >> mip;
        if (mipSize < 1) mipSize = 1;
        totalPixels += mipSize * mipSize;
    }
    
    for (int face = 0; face < 6; face++) {
        result.faces[face].resize(totalPixels * 3);
    }
    
    size_t pixelOffset = 0;
    for (uint32_t mip = 0; mip < mipLevels; mip++) {
        uint32_t mipSize = size >> mip;
        if (mipSize < 1) mipSize = 1;
        
        float roughness = (float)mip / (float)(mipLevels - 1);
        
        for (int face = 0; face < 6; face++) {
            for (uint32_t y = 0; y < mipSize; y++) {
                for (uint32_t x = 0; x < mipSize; x++) {
                    float nx, ny, nz;
                    getCubeDirection(face, (x + 0.5f) / mipSize, (y + 0.5f) / mipSize, nx, ny, nz);
                    
                    // View = Normal (reflection assumes V = R = N)
                    float vx = nx, vy = ny, vz = nz;
                    
                    float prefilteredR = 0, prefilteredG = 0, prefilteredB = 0;
                    float totalWeight = 0;
                    
                    for (uint32_t i = 0; i < SAMPLE_COUNT; i++) {
                        float xi1, xi2;
                        hammersley(i, SAMPLE_COUNT, xi1, xi2);
                        
                        float hx, hy, hz;
                        importanceSampleGGX(xi1, xi2, roughness, nx, ny, nz, hx, hy, hz);
                        
                        // L = 2 * dot(V, H) * H - V
                        float VdotH = vx*hx + vy*hy + vz*hz;
                        float lx = 2.0f * VdotH * hx - vx;
                        float ly = 2.0f * VdotH * hy - vy;
                        float lz = 2.0f * VdotH * hz - vz;
                        
                        float NdotL = nx*lx + ny*ly + nz*lz;
                        if (NdotL > 0.0f) {
                            float sr, sg, sb;
                            sampleCubemap(envMap, lx, ly, lz, sr, sg, sb);
                            
                            prefilteredR += sr * NdotL;
                            prefilteredG += sg * NdotL;
                            prefilteredB += sb * NdotL;
                            totalWeight += NdotL;
                        }
                    }
                    
                    if (totalWeight > 0) {
                        prefilteredR /= totalWeight;
                        prefilteredG /= totalWeight;
                        prefilteredB /= totalWeight;
                    }
                    
                    size_t idx = (pixelOffset + y * mipSize + x) * 3;
                    result.faces[face][idx] = prefilteredR;
                    result.faces[face][idx + 1] = prefilteredG;
                    result.faces[face][idx + 2] = prefilteredB;
                }
            }
        }
        
        pixelOffset += mipSize * mipSize;
    }
    
    std::cout << "[ibl] Prefiltered env map generated" << std::endl;
    return result;
}

// Generate BRDF LUT
BRDFLut IBLGenerator::generateBRDFLut(uint32_t size) {
    BRDFLut result;
    result.size = size;
    result.pixels.resize(size * size * 2);
    
    const uint32_t SAMPLE_COUNT = 1024;
    
    std::cout << "[ibl] Generating BRDF LUT (" << size << "x" << size << ")..." << std::endl;
    
    for (uint32_t y = 0; y < size; y++) {
        float roughness = (y + 0.5f) / size;
        
        for (uint32_t x = 0; x < size; x++) {
            float NdotV = (x + 0.5f) / size;
            NdotV = std::max(NdotV, 0.001f);  // Avoid division by zero
            
            // View vector
            float vx = sqrtf(1.0f - NdotV * NdotV);  // sin(theta)
            float vy = 0.0f;
            float vz = NdotV;  // cos(theta)
            
            // Normal
            float nx = 0.0f, ny = 0.0f, nz = 1.0f;
            
            float A = 0.0f;  // Scale
            float B = 0.0f;  // Bias
            
            for (uint32_t i = 0; i < SAMPLE_COUNT; i++) {
                float xi1, xi2;
                hammersley(i, SAMPLE_COUNT, xi1, xi2);
                
                float hx, hy, hz;
                importanceSampleGGX(xi1, xi2, roughness, nx, ny, nz, hx, hy, hz);
                
                // L = 2 * dot(V, H) * H - V
                float VdotH = vx*hx + vy*hy + vz*hz;
                VdotH = std::max(VdotH, 0.0f);
                
                float lx = 2.0f * VdotH * hx - vx;
                float ly = 2.0f * VdotH * hy - vy;
                float lz = 2.0f * VdotH * hz - vz;
                
                float NdotL = std::max(lz, 0.0f);
                float NdotH = std::max(hz, 0.0f);
                
                if (NdotL > 0.0f) {
                    float G = geometrySmith(NdotV, NdotL, roughness);
                    float G_Vis = (G * VdotH) / (NdotH * NdotV + 0.0001f);
                    float Fc = powf(1.0f - VdotH, 5.0f);
                    
                    A += (1.0f - Fc) * G_Vis;
                    B += Fc * G_Vis;
                }
            }
            
            A /= (float)SAMPLE_COUNT;
            B /= (float)SAMPLE_COUNT;
            
            size_t idx = (y * size + x) * 2;
            result.pixels[idx] = A;
            result.pixels[idx + 1] = B;
        }
    }
    
    std::cout << "[ibl] BRDF LUT generated" << std::endl;
    return result;
}

}  // namespace luma
