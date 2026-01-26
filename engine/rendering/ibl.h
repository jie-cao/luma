// Image-Based Lighting (IBL)
// HDR environment map processing and rendering
#pragma once

#include "../foundation/math_types.h"
#include <vector>
#include <string>
#include <cmath>
#include <memory>

namespace luma {

// ===== HDR Image =====
// Stores floating-point HDR image data
struct HDRImage {
    std::vector<float> data;  // RGBE or RGB float data
    int width = 0;
    int height = 0;
    int channels = 3;
    
    bool isValid() const { return !data.empty() && width > 0 && height > 0; }
    
    // Get pixel at coordinates
    Vec3 getPixel(int x, int y) const {
        if (!isValid()) return Vec3(0, 0, 0);
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);
        int idx = (y * width + x) * channels;
        return Vec3(data[idx], data[idx + 1], data[idx + 2]);
    }
    
    // Sample with bilinear filtering
    Vec3 sample(float u, float v) const {
        if (!isValid()) return Vec3(0, 0, 0);
        
        float x = u * (width - 1);
        float y = v * (height - 1);
        
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);
        int x1 = std::min(x0 + 1, width - 1);
        int y1 = std::min(y0 + 1, height - 1);
        
        float fx = x - x0;
        float fy = y - y0;
        
        Vec3 p00 = getPixel(x0, y0);
        Vec3 p10 = getPixel(x1, y0);
        Vec3 p01 = getPixel(x0, y1);
        Vec3 p11 = getPixel(x1, y1);
        
        Vec3 top = Vec3(
            p00.x * (1 - fx) + p10.x * fx,
            p00.y * (1 - fx) + p10.y * fx,
            p00.z * (1 - fx) + p10.z * fx
        );
        Vec3 bottom = Vec3(
            p01.x * (1 - fx) + p11.x * fx,
            p01.y * (1 - fx) + p11.y * fx,
            p01.z * (1 - fx) + p11.z * fx
        );
        
        return Vec3(
            top.x * (1 - fy) + bottom.x * fy,
            top.y * (1 - fy) + bottom.y * fy,
            top.z * (1 - fy) + bottom.z * fy
        );
    }
};

// ===== HDR Loader =====
// Loads .hdr (Radiance) and .exr files
namespace HDRLoader {

// RGBE to float conversion
inline Vec3 rgbeToFloat(uint8_t r, uint8_t g, uint8_t b, uint8_t e) {
    if (e == 0) return Vec3(0, 0, 0);
    
    float f = std::ldexp(1.0f, static_cast<int>(e) - 128 - 8);
    return Vec3(r * f, g * f, b * f);
}

// Parse simple HDR file (Radiance format)
// Note: This is a simplified parser - real implementation would need full format support
inline bool loadHDR(const std::string& filename, HDRImage& outImage) {
    // This would read the actual file - placeholder for now
    // In practice, use stb_image or a proper HDR loading library
    
    // Create a simple gradient HDR for testing
    outImage.width = 512;
    outImage.height = 256;
    outImage.channels = 3;
    outImage.data.resize(outImage.width * outImage.height * outImage.channels);
    
    for (int y = 0; y < outImage.height; y++) {
        for (int x = 0; x < outImage.width; x++) {
            int idx = (y * outImage.width + x) * 3;
            
            // Simple sky gradient
            float t = static_cast<float>(y) / outImage.height;
            float u = static_cast<float>(x) / outImage.width;
            
            // Sky color (bright blue at top, horizon glow)
            Vec3 skyTop(0.2f, 0.4f, 1.0f);
            Vec3 skyHorizon(1.0f, 0.8f, 0.6f);
            Vec3 ground(0.1f, 0.1f, 0.1f);
            
            Vec3 color;
            if (t < 0.5f) {
                // Sky
                float skyT = t * 2.0f;
                color = Vec3(
                    skyTop.x * (1 - skyT) + skyHorizon.x * skyT,
                    skyTop.y * (1 - skyT) + skyHorizon.y * skyT,
                    skyTop.z * (1 - skyT) + skyHorizon.z * skyT
                );
                
                // Add sun
                float sunAngle = u * 2.0f * 3.14159f;
                float sunElev = (0.5f - t) * 3.14159f;
                Vec3 sunDir(std::cos(sunAngle), std::sin(sunElev), std::sin(sunAngle));
                Vec3 pixelDir(std::cos(u * 2.0f * 3.14159f), 1.0f - t * 2.0f, std::sin(u * 2.0f * 3.14159f));
                float sunDot = std::max(0.0f, sunDir.dot(pixelDir.normalized()));
                float sunIntensity = std::pow(sunDot, 500.0f) * 50.0f;
                color = Vec3(
                    color.x + sunIntensity,
                    color.y + sunIntensity * 0.9f,
                    color.z + sunIntensity * 0.7f
                );
            } else {
                // Ground
                float groundT = (t - 0.5f) * 2.0f;
                color = Vec3(
                    skyHorizon.x * (1 - groundT) + ground.x * groundT,
                    skyHorizon.y * (1 - groundT) + ground.y * groundT,
                    skyHorizon.z * (1 - groundT) + ground.z * groundT
                );
            }
            
            // Apply intensity
            color = color * 2.0f;
            
            outImage.data[idx + 0] = color.x;
            outImage.data[idx + 1] = color.y;
            outImage.data[idx + 2] = color.z;
        }
    }
    
    return true;
}

}  // namespace HDRLoader

// ===== Cubemap Face =====
enum class CubemapFace {
    PositiveX = 0,  // Right
    NegativeX = 1,  // Left
    PositiveY = 2,  // Top
    NegativeZ = 3,  // Front (note: some conventions swap Y/Z)
    NegativeY = 4,  // Bottom
    PositiveZ = 5   // Back
};

// ===== Environment Map =====
// Stores processed environment data for IBL
class EnvironmentMap {
public:
    // Source HDR image
    HDRImage sourceHDR;
    
    // Cubemap resolution
    int cubemapSize = 512;
    
    // Prefiltered mip levels for specular IBL
    int prefilteredMipLevels = 5;
    
    // Irradiance map size (for diffuse IBL)
    int irradianceSize = 32;
    
    // Processed data
    std::vector<float> cubemapData[6];           // 6 faces
    std::vector<std::vector<float>> prefilteredData[6];  // 6 faces x mip levels
    std::vector<float> irradianceData[6];       // 6 faces
    
    // BRDF LUT (split-sum approximation)
    std::vector<float> brdfLUT;
    int brdfLUTSize = 512;
    
    bool initialized = false;
    
    // Convert equirectangular to cubemap
    void convertToCubemap() {
        if (!sourceHDR.isValid()) return;
        
        for (int face = 0; face < 6; face++) {
            cubemapData[face].resize(cubemapSize * cubemapSize * 3);
            
            for (int y = 0; y < cubemapSize; y++) {
                for (int x = 0; x < cubemapSize; x++) {
                    // Get direction for this pixel
                    Vec3 dir = getCubemapDirection(static_cast<CubemapFace>(face), x, y, cubemapSize);
                    
                    // Convert to equirectangular UV
                    float u = 0.5f + std::atan2(dir.z, dir.x) / (2.0f * 3.14159265f);
                    float v = 0.5f - std::asin(std::clamp(dir.y, -1.0f, 1.0f)) / 3.14159265f;
                    
                    // Sample HDR
                    Vec3 color = sourceHDR.sample(u, v);
                    
                    int idx = (y * cubemapSize + x) * 3;
                    cubemapData[face][idx + 0] = color.x;
                    cubemapData[face][idx + 1] = color.y;
                    cubemapData[face][idx + 2] = color.z;
                }
            }
        }
    }
    
    // Generate irradiance map (diffuse IBL)
    void generateIrradianceMap() {
        for (int face = 0; face < 6; face++) {
            irradianceData[face].resize(irradianceSize * irradianceSize * 3);
            
            for (int y = 0; y < irradianceSize; y++) {
                for (int x = 0; x < irradianceSize; x++) {
                    Vec3 normal = getCubemapDirection(static_cast<CubemapFace>(face), x, y, irradianceSize);
                    
                    // Convolve hemisphere
                    Vec3 irradiance = convolveIrradiance(normal);
                    
                    int idx = (y * irradianceSize + x) * 3;
                    irradianceData[face][idx + 0] = irradiance.x;
                    irradianceData[face][idx + 1] = irradiance.y;
                    irradianceData[face][idx + 2] = irradiance.z;
                }
            }
        }
    }
    
    // Generate prefiltered environment map (specular IBL)
    void generatePrefilteredMap() {
        for (int face = 0; face < 6; face++) {
            prefilteredData[face].resize(prefilteredMipLevels);
            
            for (int mip = 0; mip < prefilteredMipLevels; mip++) {
                int mipSize = cubemapSize >> mip;
                if (mipSize < 1) mipSize = 1;
                
                prefilteredData[face][mip].resize(mipSize * mipSize * 3);
                
                float roughness = static_cast<float>(mip) / static_cast<float>(prefilteredMipLevels - 1);
                
                for (int y = 0; y < mipSize; y++) {
                    for (int x = 0; x < mipSize; x++) {
                        Vec3 normal = getCubemapDirection(static_cast<CubemapFace>(face), x, y, mipSize);
                        
                        Vec3 color = prefilterEnvironment(normal, roughness);
                        
                        int idx = (y * mipSize + x) * 3;
                        prefilteredData[face][mip][idx + 0] = color.x;
                        prefilteredData[face][mip][idx + 1] = color.y;
                        prefilteredData[face][mip][idx + 2] = color.z;
                    }
                }
            }
        }
    }
    
    // Generate BRDF LUT
    void generateBRDFLUT() {
        brdfLUT.resize(brdfLUTSize * brdfLUTSize * 2);  // RG only
        
        for (int y = 0; y < brdfLUTSize; y++) {
            for (int x = 0; x < brdfLUTSize; x++) {
                float NdotV = static_cast<float>(x + 1) / brdfLUTSize;
                float roughness = static_cast<float>(y + 1) / brdfLUTSize;
                
                Vec3 V(std::sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);
                
                float A = 0.0f;
                float B = 0.0f;
                const int numSamples = 1024;
                
                for (int i = 0; i < numSamples; i++) {
                    // Importance sample GGX
                    float xi1 = static_cast<float>(i) / numSamples;
                    float xi2 = static_cast<float>((i * 7) % numSamples) / numSamples;
                    
                    Vec3 H = importanceSampleGGX(xi1, xi2, roughness);
                    Vec3 L = Vec3(
                        2.0f * V.dot(H) * H.x - V.x,
                        2.0f * V.dot(H) * H.y - V.y,
                        2.0f * V.dot(H) * H.z - V.z
                    );
                    
                    float NdotL = std::max(L.z, 0.0f);
                    float NdotH = std::max(H.z, 0.0f);
                    float VdotH = std::max(V.dot(H), 0.0f);
                    
                    if (NdotL > 0.0f) {
                        float G = geometrySmith(NdotV, NdotL, roughness);
                        float G_Vis = (G * VdotH) / (NdotH * NdotV);
                        float Fc = std::pow(1.0f - VdotH, 5.0f);
                        
                        A += (1.0f - Fc) * G_Vis;
                        B += Fc * G_Vis;
                    }
                }
                
                A /= numSamples;
                B /= numSamples;
                
                int idx = (y * brdfLUTSize + x) * 2;
                brdfLUT[idx + 0] = A;
                brdfLUT[idx + 1] = B;
            }
        }
    }
    
    // Full processing pipeline
    void process() {
        if (!sourceHDR.isValid()) return;
        
        convertToCubemap();
        generateIrradianceMap();
        generatePrefilteredMap();
        generateBRDFLUT();
        
        initialized = true;
    }
    
private:
    // Get direction vector for cubemap pixel
    Vec3 getCubemapDirection(CubemapFace face, int x, int y, int size) const {
        float u = (static_cast<float>(x) + 0.5f) / size * 2.0f - 1.0f;
        float v = (static_cast<float>(y) + 0.5f) / size * 2.0f - 1.0f;
        
        Vec3 dir;
        switch (face) {
            case CubemapFace::PositiveX: dir = Vec3( 1, -v, -u); break;
            case CubemapFace::NegativeX: dir = Vec3(-1, -v,  u); break;
            case CubemapFace::PositiveY: dir = Vec3( u,  1,  v); break;
            case CubemapFace::NegativeY: dir = Vec3( u, -1, -v); break;
            case CubemapFace::PositiveZ: dir = Vec3( u, -v,  1); break;
            case CubemapFace::NegativeZ: dir = Vec3(-u, -v, -1); break;
        }
        
        return dir.normalized();
    }
    
    // Sample cubemap by direction
    Vec3 sampleCubemap(const Vec3& dir) const {
        // Find face and UV
        float absX = std::abs(dir.x);
        float absY = std::abs(dir.y);
        float absZ = std::abs(dir.z);
        
        int face;
        float u, v, ma;
        
        if (absX >= absY && absX >= absZ) {
            ma = absX;
            if (dir.x > 0) { face = 0; u = -dir.z; v = -dir.y; }
            else           { face = 1; u =  dir.z; v = -dir.y; }
        } else if (absY >= absX && absY >= absZ) {
            ma = absY;
            if (dir.y > 0) { face = 2; u =  dir.x; v =  dir.z; }
            else           { face = 4; u =  dir.x; v = -dir.z; }
        } else {
            ma = absZ;
            if (dir.z > 0) { face = 5; u =  dir.x; v = -dir.y; }
            else           { face = 3; u = -dir.x; v = -dir.y; }
        }
        
        u = 0.5f * (u / ma + 1.0f);
        v = 0.5f * (v / ma + 1.0f);
        
        // Sample from cubemap data
        int x = static_cast<int>(u * (cubemapSize - 1));
        int y = static_cast<int>(v * (cubemapSize - 1));
        x = std::clamp(x, 0, cubemapSize - 1);
        y = std::clamp(y, 0, cubemapSize - 1);
        
        if (cubemapData[face].empty()) return Vec3(0, 0, 0);
        
        int idx = (y * cubemapSize + x) * 3;
        return Vec3(cubemapData[face][idx], cubemapData[face][idx + 1], cubemapData[face][idx + 2]);
    }
    
    // Convolve irradiance (diffuse)
    Vec3 convolveIrradiance(const Vec3& normal) const {
        Vec3 irradiance(0, 0, 0);
        
        // Build tangent space
        Vec3 up = std::abs(normal.y) < 0.999f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        Vec3 tangent = up.cross(normal).normalized();
        Vec3 bitangent = normal.cross(tangent);
        
        const int numSamples = 64;
        float sampleDelta = 0.1f;
        int count = 0;
        
        for (float phi = 0.0f; phi < 2.0f * 3.14159f; phi += sampleDelta) {
            for (float theta = 0.0f; theta < 0.5f * 3.14159f; theta += sampleDelta) {
                // Spherical to cartesian
                Vec3 sampleDir(
                    std::sin(theta) * std::cos(phi),
                    std::sin(theta) * std::sin(phi),
                    std::cos(theta)
                );
                
                // Transform to world space
                Vec3 worldDir(
                    sampleDir.x * tangent.x + sampleDir.y * bitangent.x + sampleDir.z * normal.x,
                    sampleDir.x * tangent.y + sampleDir.y * bitangent.y + sampleDir.z * normal.y,
                    sampleDir.x * tangent.z + sampleDir.y * bitangent.z + sampleDir.z * normal.z
                );
                
                Vec3 sample = sampleCubemap(worldDir);
                irradiance = irradiance + sample * std::cos(theta) * std::sin(theta);
                count++;
            }
        }
        
        irradiance = irradiance * (3.14159f / count);
        return irradiance;
    }
    
    // Prefilter for specular (roughness-based)
    Vec3 prefilterEnvironment(const Vec3& N, float roughness) const {
        Vec3 R = N;
        Vec3 V = R;
        
        Vec3 prefilteredColor(0, 0, 0);
        float totalWeight = 0.0f;
        
        const int numSamples = 256;
        
        for (int i = 0; i < numSamples; i++) {
            float xi1 = static_cast<float>(i) / numSamples;
            float xi2 = static_cast<float>((i * 7) % numSamples) / numSamples;
            
            Vec3 H = importanceSampleGGX(xi1, xi2, roughness, N);
            Vec3 L = Vec3(
                2.0f * V.dot(H) * H.x - V.x,
                2.0f * V.dot(H) * H.y - V.y,
                2.0f * V.dot(H) * H.z - V.z
            );
            
            float NdotL = std::max(N.dot(L), 0.0f);
            if (NdotL > 0.0f) {
                Vec3 sample = sampleCubemap(L);
                prefilteredColor = prefilteredColor + sample * NdotL;
                totalWeight += NdotL;
            }
        }
        
        if (totalWeight > 0.0f) {
            prefilteredColor = prefilteredColor * (1.0f / totalWeight);
        }
        
        return prefilteredColor;
    }
    
    // GGX importance sampling
    Vec3 importanceSampleGGX(float xi1, float xi2, float roughness, const Vec3& N = Vec3(0, 0, 1)) const {
        float a = roughness * roughness;
        
        float phi = 2.0f * 3.14159f * xi1;
        float cosTheta = std::sqrt((1.0f - xi2) / (1.0f + (a * a - 1.0f) * xi2));
        float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
        
        Vec3 H(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
        
        // Transform to world space if N provided
        if (std::abs(N.x) > 0.001f || std::abs(N.y) > 0.001f || std::abs(N.z - 1.0f) > 0.001f) {
            Vec3 up = std::abs(N.z) < 0.999f ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
            Vec3 tangent = up.cross(N).normalized();
            Vec3 bitangent = N.cross(tangent);
            
            return Vec3(
                H.x * tangent.x + H.y * bitangent.x + H.z * N.x,
                H.x * tangent.y + H.y * bitangent.y + H.z * N.y,
                H.x * tangent.z + H.y * bitangent.z + H.z * N.z
            ).normalized();
        }
        
        return H;
    }
    
    // Smith geometry function
    float geometrySmith(float NdotV, float NdotL, float roughness) const {
        float a = roughness;
        float k = (a * a) / 2.0f;
        
        float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
        float ggx2 = NdotL / (NdotL * (1.0f - k) + k);
        
        return ggx1 * ggx2;
    }
};

// ===== IBL Settings =====
struct IBLSettings {
    float diffuseIntensity = 1.0f;
    float specularIntensity = 1.0f;
    float exposure = 1.0f;
    bool enabled = true;
};

}  // namespace luma
