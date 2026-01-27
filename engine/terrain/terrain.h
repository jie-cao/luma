// Terrain System - Core terrain data structures
// Heightmap-based terrain with chunking, LOD, and multi-layer materials
#pragma once

#include "engine/foundation/math_types.h"
#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>

namespace luma {

// ===== Heightmap =====
class Heightmap {
public:
    Heightmap() : width_(0), height_(0) {}
    Heightmap(int width, int height) : width_(width), height_(height), data_(width * height, 0.0f) {}
    
    void resize(int width, int height) {
        width_ = width;
        height_ = height;
        data_.resize(width * height, 0.0f);
    }
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    float getHeight(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0.0f;
        return data_[y * width_ + x];
    }
    
    void setHeight(int x, int y, float h) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        data_[y * width_ + x] = h;
    }
    
    // Bilinear interpolation for smooth sampling
    float sampleBilinear(float u, float v) const {
        float x = u * (width_ - 1);
        float y = v * (height_ - 1);
        
        int x0 = (int)std::floor(x);
        int y0 = (int)std::floor(y);
        int x1 = std::min(x0 + 1, width_ - 1);
        int y1 = std::min(y0 + 1, height_ - 1);
        
        float fx = x - x0;
        float fy = y - y0;
        
        float h00 = getHeight(x0, y0);
        float h10 = getHeight(x1, y0);
        float h01 = getHeight(x0, y1);
        float h11 = getHeight(x1, y1);
        
        float h0 = h00 + (h10 - h00) * fx;
        float h1 = h01 + (h11 - h01) * fx;
        
        return h0 + (h1 - h0) * fy;
    }
    
    // Get normal at position
    Vec3 getNormal(int x, int y, float cellSize = 1.0f) const {
        float hL = getHeight(x - 1, y);
        float hR = getHeight(x + 1, y);
        float hD = getHeight(x, y - 1);
        float hU = getHeight(x, y + 1);
        
        Vec3 normal(
            (hL - hR) / (2.0f * cellSize),
            1.0f,
            (hD - hU) / (2.0f * cellSize)
        );
        
        return normal.normalized();
    }
    
    // Fill from raw data
    void setData(const float* data, int width, int height) {
        resize(width, height);
        std::copy(data, data + width * height, data_.begin());
    }
    
    const float* getData() const { return data_.data(); }
    float* getData() { return data_.data(); }
    
    // Find min/max heights
    void getMinMax(float& minH, float& maxH) const {
        if (data_.empty()) {
            minH = maxH = 0.0f;
            return;
        }
        minH = maxH = data_[0];
        for (float h : data_) {
            minH = std::min(minH, h);
            maxH = std::max(maxH, h);
        }
    }
    
    // Normalize heights to 0-1 range
    void normalize() {
        float minH, maxH;
        getMinMax(minH, maxH);
        float range = maxH - minH;
        if (range < 0.0001f) return;
        
        for (float& h : data_) {
            h = (h - minH) / range;
        }
    }
    
private:
    int width_;
    int height_;
    std::vector<float> data_;
};

// ===== Splatmap (Texture weight map) =====
class Splatmap {
public:
    static constexpr int MAX_LAYERS = 4;
    
    Splatmap() : width_(0), height_(0) {}
    Splatmap(int width, int height) : width_(width), height_(height) {
        for (int i = 0; i < MAX_LAYERS; i++) {
            weights_[i].resize(width * height, i == 0 ? 1.0f : 0.0f);
        }
    }
    
    void resize(int width, int height) {
        width_ = width;
        height_ = height;
        for (int i = 0; i < MAX_LAYERS; i++) {
            weights_[i].resize(width * height, i == 0 ? 1.0f : 0.0f);
        }
    }
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    float getWeight(int layer, int x, int y) const {
        if (layer < 0 || layer >= MAX_LAYERS) return 0.0f;
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0.0f;
        return weights_[layer][y * width_ + x];
    }
    
    void setWeight(int layer, int x, int y, float weight) {
        if (layer < 0 || layer >= MAX_LAYERS) return;
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        weights_[layer][y * width_ + x] = std::max(0.0f, std::min(1.0f, weight));
    }
    
    // Normalize weights at a point so they sum to 1
    void normalizeAt(int x, int y) {
        float sum = 0.0f;
        for (int i = 0; i < MAX_LAYERS; i++) {
            sum += getWeight(i, x, y);
        }
        if (sum < 0.0001f) {
            setWeight(0, x, y, 1.0f);
            return;
        }
        for (int i = 0; i < MAX_LAYERS; i++) {
            setWeight(i, x, y, getWeight(i, x, y) / sum);
        }
    }
    
    const float* getLayerData(int layer) const {
        if (layer < 0 || layer >= MAX_LAYERS) return nullptr;
        return weights_[layer].data();
    }
    
private:
    int width_;
    int height_;
    std::vector<float> weights_[MAX_LAYERS];
};

// ===== Terrain Layer (Material) =====
struct TerrainLayer {
    std::string name = "Layer";
    std::string diffuseTexture;
    std::string normalTexture;
    
    Vec3 tint = {1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.8f;
    
    float tileScale = 10.0f;  // UV tiling
    
    // Height-based blending
    float minHeight = 0.0f;   // Min height for this layer
    float maxHeight = 1.0f;   // Max height for this layer
    float blendSharpness = 1.0f;
    
    // Slope-based blending
    float minSlope = 0.0f;    // 0 = flat, 1 = vertical
    float maxSlope = 1.0f;
    float slopeBlendSharpness = 1.0f;
};

// ===== Terrain Chunk Mesh Data =====
struct TerrainMeshData {
    std::vector<float> vertices;   // Position (3) + Normal (3) + UV (2) + Splat UV (2) = 10 floats per vertex
    std::vector<uint32_t> indices;
    int vertexCount = 0;
    int triangleCount = 0;
};

// ===== Terrain Chunk =====
class TerrainChunk {
public:
    TerrainChunk(int chunkX, int chunkZ, int resolution)
        : chunkX_(chunkX), chunkZ_(chunkZ), resolution_(resolution), lodLevel_(0) {}
    
    int getChunkX() const { return chunkX_; }
    int getChunkZ() const { return chunkZ_; }
    int getResolution() const { return resolution_; }
    int getLODLevel() const { return lodLevel_; }
    
    void setLODLevel(int lod) { lodLevel_ = lod; }
    
    // Generate mesh from heightmap section
    void generateMesh(const Heightmap& heightmap, float chunkSize, float heightScale,
                      int startX, int startY, int sampleStep = 1);
    
    const TerrainMeshData& getMeshData() const { return meshData_; }
    TerrainMeshData& getMeshData() { return meshData_; }
    
    // GPU resource handles (set by renderer)
    uint32_t gpuMeshIndex = 0;
    bool gpuMeshValid = false;
    
private:
    int chunkX_;
    int chunkZ_;
    int resolution_;
    int lodLevel_;
    TerrainMeshData meshData_;
};

// ===== Terrain Settings =====
struct TerrainSettings {
    int heightmapResolution = 513;  // Should be 2^n + 1
    float terrainSize = 256.0f;     // World units
    float heightScale = 50.0f;      // Maximum height
    
    int chunkResolution = 33;       // Vertices per chunk side
    int chunksPerSide = 8;          // Number of chunks per axis
    
    int lodLevels = 4;
    float lodDistances[4] = {50.0f, 100.0f, 200.0f, 400.0f};
    
    std::vector<TerrainLayer> layers;
};

// ===== Terrain =====
class Terrain {
public:
    Terrain() = default;
    
    void initialize(const TerrainSettings& settings) {
        settings_ = settings;
        heightmap_.resize(settings.heightmapResolution, settings.heightmapResolution);
        splatmap_.resize(settings.heightmapResolution, settings.heightmapResolution);
        
        // Create chunks
        chunks_.clear();
        int chunkRes = settings.chunkResolution;
        for (int z = 0; z < settings.chunksPerSide; z++) {
            for (int x = 0; x < settings.chunksPerSide; x++) {
                chunks_.push_back(std::make_unique<TerrainChunk>(x, z, chunkRes));
            }
        }
        
        // Add default layers
        if (settings_.layers.empty()) {
            TerrainLayer grass;
            grass.name = "Grass";
            grass.tint = {0.3f, 0.5f, 0.2f};
            grass.roughness = 0.9f;
            settings_.layers.push_back(grass);
            
            TerrainLayer rock;
            rock.name = "Rock";
            rock.tint = {0.5f, 0.5f, 0.5f};
            rock.roughness = 0.7f;
            rock.minSlope = 0.5f;
            settings_.layers.push_back(rock);
            
            TerrainLayer sand;
            sand.name = "Sand";
            sand.tint = {0.9f, 0.8f, 0.6f};
            sand.roughness = 0.95f;
            sand.maxHeight = 0.1f;
            settings_.layers.push_back(sand);
            
            TerrainLayer snow;
            snow.name = "Snow";
            snow.tint = {0.95f, 0.95f, 1.0f};
            snow.roughness = 0.3f;
            snow.minHeight = 0.7f;
            settings_.layers.push_back(snow);
        }
    }
    
    const TerrainSettings& getSettings() const { return settings_; }
    TerrainSettings& getSettings() { return settings_; }
    
    Heightmap& getHeightmap() { return heightmap_; }
    const Heightmap& getHeightmap() const { return heightmap_; }
    
    Splatmap& getSplatmap() { return splatmap_; }
    const Splatmap& getSplatmap() const { return splatmap_; }
    
    // Get height at world position
    float getHeightAt(float worldX, float worldZ) const {
        float halfSize = settings_.terrainSize * 0.5f;
        float u = (worldX + halfSize) / settings_.terrainSize;
        float v = (worldZ + halfSize) / settings_.terrainSize;
        
        if (u < 0 || u > 1 || v < 0 || v > 1) return 0.0f;
        
        return heightmap_.sampleBilinear(u, v) * settings_.heightScale;
    }
    
    // Get normal at world position
    Vec3 getNormalAt(float worldX, float worldZ) const {
        float halfSize = settings_.terrainSize * 0.5f;
        float u = (worldX + halfSize) / settings_.terrainSize;
        float v = (worldZ + halfSize) / settings_.terrainSize;
        
        if (u < 0 || u > 1 || v < 0 || v > 1) return Vec3(0, 1, 0);
        
        int x = (int)(u * (heightmap_.getWidth() - 1));
        int y = (int)(v * (heightmap_.getHeight() - 1));
        
        float cellSize = settings_.terrainSize / heightmap_.getWidth();
        return heightmap_.getNormal(x, y, cellSize / settings_.heightScale);
    }
    
    // Update LOD based on camera position
    void updateLOD(const Vec3& cameraPos) {
        float chunkSize = settings_.terrainSize / settings_.chunksPerSide;
        float halfSize = settings_.terrainSize * 0.5f;
        
        for (auto& chunk : chunks_) {
            float chunkCenterX = chunk->getChunkX() * chunkSize - halfSize + chunkSize * 0.5f;
            float chunkCenterZ = chunk->getChunkZ() * chunkSize - halfSize + chunkSize * 0.5f;
            
            float dist = std::sqrt(
                (cameraPos.x - chunkCenterX) * (cameraPos.x - chunkCenterX) +
                (cameraPos.z - chunkCenterZ) * (cameraPos.z - chunkCenterZ)
            );
            
            int lod = settings_.lodLevels - 1;
            for (int i = 0; i < settings_.lodLevels; i++) {
                if (dist < settings_.lodDistances[i]) {
                    lod = i;
                    break;
                }
            }
            
            if (chunk->getLODLevel() != lod) {
                chunk->setLODLevel(lod);
                chunk->gpuMeshValid = false;  // Need to regenerate
            }
        }
    }
    
    // Rebuild all chunk meshes
    void rebuildMeshes() {
        float chunkSize = settings_.terrainSize / settings_.chunksPerSide;
        int hmRes = settings_.heightmapResolution;
        int samplesPerChunk = (hmRes - 1) / settings_.chunksPerSide;
        
        for (auto& chunk : chunks_) {
            int startX = chunk->getChunkX() * samplesPerChunk;
            int startY = chunk->getChunkZ() * samplesPerChunk;
            int sampleStep = 1 << chunk->getLODLevel();  // 1, 2, 4, 8 for LOD 0-3
            
            chunk->generateMesh(heightmap_, chunkSize, settings_.heightScale, startX, startY, sampleStep);
            chunk->gpuMeshValid = false;
        }
    }
    
    // Auto-generate splatmap from height/slope
    void autoGenerateSplatmap() {
        int w = heightmap_.getWidth();
        int h = heightmap_.getHeight();
        float cellSize = settings_.terrainSize / w;
        
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float height = heightmap_.getHeight(x, y);
                Vec3 normal = heightmap_.getNormal(x, y, cellSize);
                float slope = 1.0f - normal.y;  // 0 = flat, 1 = vertical
                
                // Calculate weight for each layer
                for (size_t i = 0; i < settings_.layers.size() && i < Splatmap::MAX_LAYERS; i++) {
                    const auto& layer = settings_.layers[i];
                    
                    float heightWeight = 1.0f;
                    if (height < layer.minHeight) {
                        heightWeight = std::exp(-(layer.minHeight - height) * layer.blendSharpness * 10.0f);
                    } else if (height > layer.maxHeight) {
                        heightWeight = std::exp(-(height - layer.maxHeight) * layer.blendSharpness * 10.0f);
                    }
                    
                    float slopeWeight = 1.0f;
                    if (slope < layer.minSlope) {
                        slopeWeight = std::exp(-(layer.minSlope - slope) * layer.slopeBlendSharpness * 10.0f);
                    } else if (slope > layer.maxSlope) {
                        slopeWeight = std::exp(-(slope - layer.maxSlope) * layer.slopeBlendSharpness * 10.0f);
                    }
                    
                    splatmap_.setWeight((int)i, x, y, heightWeight * slopeWeight);
                }
                
                splatmap_.normalizeAt(x, y);
            }
        }
    }
    
    const std::vector<std::unique_ptr<TerrainChunk>>& getChunks() const { return chunks_; }
    std::vector<std::unique_ptr<TerrainChunk>>& getChunks() { return chunks_; }
    
    size_t getChunkCount() const { return chunks_.size(); }
    
private:
    TerrainSettings settings_;
    Heightmap heightmap_;
    Splatmap splatmap_;
    std::vector<std::unique_ptr<TerrainChunk>> chunks_;
};

// ===== Terrain Chunk Mesh Generation =====
inline void TerrainChunk::generateMesh(const Heightmap& heightmap, float chunkSize, float heightScale,
                                        int startX, int startY, int sampleStep)
{
    int hmWidth = heightmap.getWidth();
    int hmHeight = heightmap.getHeight();
    
    // Calculate actual resolution after LOD
    int vertsPerSide = ((resolution_ - 1) / sampleStep) + 1;
    float cellSize = chunkSize / (vertsPerSide - 1);
    
    meshData_.vertices.clear();
    meshData_.indices.clear();
    
    // Generate vertices
    for (int z = 0; z < vertsPerSide; z++) {
        for (int x = 0; x < vertsPerSide; x++) {
            int hmX = startX + x * sampleStep;
            int hmY = startY + z * sampleStep;
            
            // Clamp to heightmap bounds
            hmX = std::min(hmX, hmWidth - 1);
            hmY = std::min(hmY, hmHeight - 1);
            
            float h = heightmap.getHeight(hmX, hmY) * heightScale;
            Vec3 normal = heightmap.getNormal(hmX, hmY, cellSize / heightScale);
            
            // Position
            float posX = x * cellSize;
            float posY = h;
            float posZ = z * cellSize;
            
            // UV for texture tiling
            float u = (float)x / (vertsPerSide - 1);
            float v = (float)z / (vertsPerSide - 1);
            
            // Splat UV (for splatmap sampling)
            float splatU = (float)hmX / (hmWidth - 1);
            float splatV = (float)hmY / (hmHeight - 1);
            
            // Push vertex data
            meshData_.vertices.push_back(posX);
            meshData_.vertices.push_back(posY);
            meshData_.vertices.push_back(posZ);
            meshData_.vertices.push_back(normal.x);
            meshData_.vertices.push_back(normal.y);
            meshData_.vertices.push_back(normal.z);
            meshData_.vertices.push_back(u);
            meshData_.vertices.push_back(v);
            meshData_.vertices.push_back(splatU);
            meshData_.vertices.push_back(splatV);
        }
    }
    
    // Generate indices
    for (int z = 0; z < vertsPerSide - 1; z++) {
        for (int x = 0; x < vertsPerSide - 1; x++) {
            uint32_t topLeft = z * vertsPerSide + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * vertsPerSide + x;
            uint32_t bottomRight = bottomLeft + 1;
            
            // First triangle
            meshData_.indices.push_back(topLeft);
            meshData_.indices.push_back(bottomLeft);
            meshData_.indices.push_back(topRight);
            
            // Second triangle
            meshData_.indices.push_back(topRight);
            meshData_.indices.push_back(bottomLeft);
            meshData_.indices.push_back(bottomRight);
        }
    }
    
    meshData_.vertexCount = vertsPerSide * vertsPerSide;
    meshData_.triangleCount = (vertsPerSide - 1) * (vertsPerSide - 1) * 2;
}

// ===== Global Terrain Accessor =====
inline Terrain& getTerrain() {
    static Terrain terrain;
    return terrain;
}

}  // namespace luma
