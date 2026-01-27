// Terrain Generator - Procedural terrain generation
// Perlin noise, fractal noise, hydraulic erosion
#pragma once

#include "terrain.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace luma {

// ===== Perlin Noise =====
class PerlinNoise {
public:
    PerlinNoise(uint32_t seed = 0) {
        // Initialize permutation table
        for (int i = 0; i < 256; i++) {
            p_[i] = i;
        }
        
        // Shuffle
        std::mt19937 rng(seed);
        for (int i = 255; i > 0; i--) {
            std::uniform_int_distribution<int> dist(0, i);
            int j = dist(rng);
            std::swap(p_[i], p_[j]);
        }
        
        // Duplicate
        for (int i = 0; i < 256; i++) {
            p_[256 + i] = p_[i];
        }
    }
    
    float noise(float x, float y) const {
        // Find unit grid cell
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        
        // Relative position in cell
        x -= std::floor(x);
        y -= std::floor(y);
        
        // Fade curves
        float u = fade(x);
        float v = fade(y);
        
        // Hash coordinates of corners
        int A = p_[X] + Y;
        int B = p_[X + 1] + Y;
        
        // Blend results
        return lerp(v, 
            lerp(u, grad(p_[A], x, y), grad(p_[B], x - 1, y)),
            lerp(u, grad(p_[A + 1], x, y - 1), grad(p_[B + 1], x - 1, y - 1))
        );
    }
    
    float noise3D(float x, float y, float z) const {
        int X = (int)std::floor(x) & 255;
        int Y = (int)std::floor(y) & 255;
        int Z = (int)std::floor(z) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);
        
        float u = fade(x);
        float v = fade(y);
        float w = fade(z);
        
        int A = p_[X] + Y;
        int AA = p_[A] + Z;
        int AB = p_[A + 1] + Z;
        int B = p_[X + 1] + Y;
        int BA = p_[B] + Z;
        int BB = p_[B + 1] + Z;
        
        return lerp(w,
            lerp(v,
                lerp(u, grad3D(p_[AA], x, y, z), grad3D(p_[BA], x - 1, y, z)),
                lerp(u, grad3D(p_[AB], x, y - 1, z), grad3D(p_[BB], x - 1, y - 1, z))),
            lerp(v,
                lerp(u, grad3D(p_[AA + 1], x, y, z - 1), grad3D(p_[BA + 1], x - 1, y, z - 1)),
                lerp(u, grad3D(p_[AB + 1], x, y - 1, z - 1), grad3D(p_[BB + 1], x - 1, y - 1, z - 1)))
        );
    }
    
private:
    static float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    static float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }
    
    static float grad(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
    
    static float grad3D(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }
    
    int p_[512];
};

// ===== Fractal Noise Settings =====
struct FractalNoiseSettings {
    int octaves = 6;
    float frequency = 0.005f;
    float amplitude = 1.0f;
    float lacunarity = 2.0f;     // Frequency multiplier per octave
    float persistence = 0.5f;    // Amplitude multiplier per octave
    float exponent = 1.0f;       // Power curve
    bool ridged = false;         // Ridged noise
    float ridgeOffset = 1.0f;
};

// ===== Fractal Noise Generator =====
class FractalNoise {
public:
    FractalNoise(uint32_t seed = 0) : perlin_(seed) {}
    
    float generate(float x, float y, const FractalNoiseSettings& settings) const {
        float value = 0.0f;
        float frequency = settings.frequency;
        float amplitude = settings.amplitude;
        float maxValue = 0.0f;
        
        for (int i = 0; i < settings.octaves; i++) {
            float n = perlin_.noise(x * frequency, y * frequency);
            
            if (settings.ridged) {
                n = settings.ridgeOffset - std::abs(n);
                n = n * n;
            }
            
            value += n * amplitude;
            maxValue += amplitude;
            
            frequency *= settings.lacunarity;
            amplitude *= settings.persistence;
        }
        
        value = value / maxValue;  // Normalize to [-1, 1]
        value = (value + 1.0f) * 0.5f;  // Normalize to [0, 1]
        
        if (settings.exponent != 1.0f) {
            value = std::pow(value, settings.exponent);
        }
        
        return value;
    }
    
private:
    PerlinNoise perlin_;
};

// ===== Hydraulic Erosion Settings =====
struct ErosionSettings {
    int iterations = 50000;
    int maxLifetime = 30;
    
    float inertia = 0.05f;
    float sedimentCapacityFactor = 4.0f;
    float minSedimentCapacity = 0.01f;
    float depositSpeed = 0.3f;
    float erodeSpeed = 0.3f;
    float evaporateSpeed = 0.01f;
    float gravity = 4.0f;
    
    int erosionRadius = 3;
    float initialWaterVolume = 1.0f;
    float initialSpeed = 1.0f;
};

// ===== Hydraulic Erosion =====
class HydraulicErosion {
public:
    HydraulicErosion(uint32_t seed = 0) : rng_(seed) {}
    
    void erode(Heightmap& heightmap, const ErosionSettings& settings) {
        int width = heightmap.getWidth();
        int height = heightmap.getHeight();
        
        // Precompute erosion brush indices and weights
        std::vector<std::vector<int>> brushIndices;
        std::vector<std::vector<float>> brushWeights;
        initializeBrush(width, height, settings.erosionRadius, brushIndices, brushWeights);
        
        std::uniform_real_distribution<float> distX(0.0f, (float)(width - 1));
        std::uniform_real_distribution<float> distY(0.0f, (float)(height - 1));
        
        for (int iteration = 0; iteration < settings.iterations; iteration++) {
            // Create water droplet at random position
            float posX = distX(rng_);
            float posY = distY(rng_);
            float dirX = 0.0f;
            float dirY = 0.0f;
            float speed = settings.initialSpeed;
            float water = settings.initialWaterVolume;
            float sediment = 0.0f;
            
            for (int lifetime = 0; lifetime < settings.maxLifetime; lifetime++) {
                int nodeX = (int)posX;
                int nodeY = (int)posY;
                int dropletIndex = nodeY * width + nodeX;
                
                // Calculate droplet offset within cell
                float cellOffsetX = posX - nodeX;
                float cellOffsetY = posY - nodeY;
                
                // Calculate gradient and height
                HeightAndGradient hg = calculateHeightAndGradient(heightmap, posX, posY);
                
                // Update direction with inertia
                dirX = dirX * settings.inertia - hg.gradientX * (1.0f - settings.inertia);
                dirY = dirY * settings.inertia - hg.gradientY * (1.0f - settings.inertia);
                
                // Normalize direction
                float len = std::sqrt(dirX * dirX + dirY * dirY);
                if (len > 0.0001f) {
                    dirX /= len;
                    dirY /= len;
                }
                
                // Update position
                posX += dirX;
                posY += dirY;
                
                // Stop if outside bounds or not moving
                if ((dirX == 0 && dirY == 0) ||
                    posX < 0 || posX >= width - 1 ||
                    posY < 0 || posY >= height - 1) {
                    break;
                }
                
                // Calculate new height and delta height
                float newHeight = calculateHeightAndGradient(heightmap, posX, posY).height;
                float deltaHeight = newHeight - hg.height;
                
                // Calculate sediment capacity
                float sedimentCapacity = std::max(-deltaHeight * speed * water * settings.sedimentCapacityFactor,
                                                   settings.minSedimentCapacity);
                
                // Deposit or erode
                if (sediment > sedimentCapacity || deltaHeight > 0) {
                    // Deposit sediment
                    float amountToDeposit = (deltaHeight > 0) ? 
                        std::min(deltaHeight, sediment) :
                        (sediment - sedimentCapacity) * settings.depositSpeed;
                    
                    sediment -= amountToDeposit;
                    
                    // Deposit to the four nodes of the current cell
                    heightmap.setHeight(nodeX, nodeY, 
                        heightmap.getHeight(nodeX, nodeY) + amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY));
                    heightmap.setHeight(nodeX + 1, nodeY,
                        heightmap.getHeight(nodeX + 1, nodeY) + amountToDeposit * cellOffsetX * (1 - cellOffsetY));
                    heightmap.setHeight(nodeX, nodeY + 1,
                        heightmap.getHeight(nodeX, nodeY + 1) + amountToDeposit * (1 - cellOffsetX) * cellOffsetY);
                    heightmap.setHeight(nodeX + 1, nodeY + 1,
                        heightmap.getHeight(nodeX + 1, nodeY + 1) + amountToDeposit * cellOffsetX * cellOffsetY);
                } else {
                    // Erode
                    float amountToErode = std::min((sedimentCapacity - sediment) * settings.erodeSpeed,
                                                    -deltaHeight);
                    
                    // Use brush for erosion
                    for (size_t i = 0; i < brushIndices[dropletIndex].size(); i++) {
                        int erodeIndex = brushIndices[dropletIndex][i];
                        float weight = brushWeights[dropletIndex][i];
                        
                        int ex = erodeIndex % width;
                        int ey = erodeIndex / width;
                        
                        float weightedErode = amountToErode * weight;
                        float deltaSediment = std::min(heightmap.getHeight(ex, ey), weightedErode);
                        
                        heightmap.setHeight(ex, ey, heightmap.getHeight(ex, ey) - deltaSediment);
                        sediment += deltaSediment;
                    }
                }
                
                // Update speed and water
                speed = std::sqrt(speed * speed + deltaHeight * settings.gravity);
                water *= (1.0f - settings.evaporateSpeed);
            }
        }
    }
    
private:
    struct HeightAndGradient {
        float height;
        float gradientX;
        float gradientY;
    };
    
    HeightAndGradient calculateHeightAndGradient(const Heightmap& heightmap, float posX, float posY) {
        int coordX = (int)posX;
        int coordY = (int)posY;
        
        float x = posX - coordX;
        float y = posY - coordY;
        
        int width = heightmap.getWidth();
        int height = heightmap.getHeight();
        
        // Clamp coordinates
        coordX = std::max(0, std::min(coordX, width - 2));
        coordY = std::max(0, std::min(coordY, height - 2));
        
        float h00 = heightmap.getHeight(coordX, coordY);
        float h10 = heightmap.getHeight(coordX + 1, coordY);
        float h01 = heightmap.getHeight(coordX, coordY + 1);
        float h11 = heightmap.getHeight(coordX + 1, coordY + 1);
        
        HeightAndGradient result;
        result.gradientX = (h10 - h00) * (1 - y) + (h11 - h01) * y;
        result.gradientY = (h01 - h00) * (1 - x) + (h11 - h10) * x;
        result.height = h00 * (1 - x) * (1 - y) + h10 * x * (1 - y) + h01 * (1 - x) * y + h11 * x * y;
        
        return result;
    }
    
    void initializeBrush(int width, int height, int radius,
                         std::vector<std::vector<int>>& brushIndices,
                         std::vector<std::vector<float>>& brushWeights)
    {
        brushIndices.resize(width * height);
        brushWeights.resize(width * height);
        
        std::vector<int> xOffsets;
        std::vector<int> yOffsets;
        std::vector<float> weights;
        
        float weightSum = 0;
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                float dist = std::sqrt((float)(x * x + y * y));
                if (dist <= radius) {
                    xOffsets.push_back(x);
                    yOffsets.push_back(y);
                    float weight = 1.0f - dist / radius;
                    weightSum += weight;
                    weights.push_back(weight);
                }
            }
        }
        
        // Normalize weights
        for (float& w : weights) {
            w /= weightSum;
        }
        
        // Build brush for each position
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = y * width + x;
                
                for (size_t i = 0; i < xOffsets.size(); i++) {
                    int bx = x + xOffsets[i];
                    int by = y + yOffsets[i];
                    
                    if (bx >= 0 && bx < width && by >= 0 && by < height) {
                        brushIndices[index].push_back(by * width + bx);
                        brushWeights[index].push_back(weights[i]);
                    }
                }
            }
        }
    }
    
    std::mt19937 rng_;
};

// ===== Terrain Generator =====
class TerrainGenerator {
public:
    TerrainGenerator(uint32_t seed = 0) : fractal_(seed), erosion_(seed), seed_(seed) {}
    
    void setSeed(uint32_t seed) {
        seed_ = seed;
        fractal_ = FractalNoise(seed);
        erosion_ = HydraulicErosion(seed);
    }
    
    // Generate heightmap from fractal noise
    void generateFromNoise(Heightmap& heightmap, const FractalNoiseSettings& settings) {
        int w = heightmap.getWidth();
        int h = heightmap.getHeight();
        
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float value = fractal_.generate((float)x, (float)y, settings);
                heightmap.setHeight(x, y, value);
            }
        }
    }
    
    // Apply hydraulic erosion
    void applyErosion(Heightmap& heightmap, const ErosionSettings& settings) {
        erosion_.erode(heightmap, settings);
    }
    
    // Generate complete terrain
    void generate(Terrain& terrain, const FractalNoiseSettings& noiseSettings,
                  const ErosionSettings& erosionSettings, bool applyErosionPass = true)
    {
        generateFromNoise(terrain.getHeightmap(), noiseSettings);
        
        if (applyErosionPass) {
            applyErosion(terrain.getHeightmap(), erosionSettings);
        }
        
        terrain.getHeightmap().normalize();
        terrain.autoGenerateSplatmap();
        terrain.rebuildMeshes();
    }
    
    // Preset: Flat terrain
    static FractalNoiseSettings presetFlat() {
        FractalNoiseSettings settings;
        settings.octaves = 4;
        settings.frequency = 0.01f;
        settings.amplitude = 0.1f;
        settings.persistence = 0.3f;
        return settings;
    }
    
    // Preset: Rolling hills
    static FractalNoiseSettings presetHills() {
        FractalNoiseSettings settings;
        settings.octaves = 5;
        settings.frequency = 0.005f;
        settings.amplitude = 0.5f;
        settings.persistence = 0.5f;
        settings.exponent = 1.2f;
        return settings;
    }
    
    // Preset: Mountains
    static FractalNoiseSettings presetMountains() {
        FractalNoiseSettings settings;
        settings.octaves = 8;
        settings.frequency = 0.003f;
        settings.amplitude = 1.0f;
        settings.persistence = 0.6f;
        settings.exponent = 1.5f;
        settings.ridged = true;
        settings.ridgeOffset = 1.0f;
        return settings;
    }
    
    // Preset: Islands
    static FractalNoiseSettings presetIslands() {
        FractalNoiseSettings settings;
        settings.octaves = 6;
        settings.frequency = 0.004f;
        settings.amplitude = 0.7f;
        settings.persistence = 0.45f;
        settings.exponent = 2.0f;
        return settings;
    }
    
    // Preset: Canyon
    static FractalNoiseSettings presetCanyon() {
        FractalNoiseSettings settings;
        settings.octaves = 5;
        settings.frequency = 0.006f;
        settings.amplitude = 0.8f;
        settings.persistence = 0.55f;
        settings.ridged = true;
        settings.ridgeOffset = 0.8f;
        settings.exponent = 0.8f;
        return settings;
    }
    
    // Erosion preset: Light
    static ErosionSettings erosionLight() {
        ErosionSettings settings;
        settings.iterations = 10000;
        return settings;
    }
    
    // Erosion preset: Medium
    static ErosionSettings erosionMedium() {
        ErosionSettings settings;
        settings.iterations = 50000;
        return settings;
    }
    
    // Erosion preset: Heavy
    static ErosionSettings erosionHeavy() {
        ErosionSettings settings;
        settings.iterations = 200000;
        settings.erodeSpeed = 0.5f;
        return settings;
    }
    
private:
    FractalNoise fractal_;
    HydraulicErosion erosion_;
    uint32_t seed_;
};

// ===== Global Generator =====
inline TerrainGenerator& getTerrainGenerator() {
    static TerrainGenerator generator;
    return generator;
}

}  // namespace luma
