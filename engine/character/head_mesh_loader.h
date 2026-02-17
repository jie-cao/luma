// Head Mesh Loader - Load standard topology head mesh for MetaHuman-style character creation
// Loads binary format exported from convert_head.py
#pragma once

#include "engine/renderer/mesh.h"
#include "engine/foundation/math_types.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>

namespace luma {

class HeadMeshLoader {
public:
    struct HeadMesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        // Neck boundary info for body stitching
        int neckBoundaryStart = 0;
        int neckBoundaryCount = 0;
        
        // Mesh bounds
        Vec3 minBounds;
        Vec3 maxBounds;
        Vec3 center;
        
        bool loaded = false;
    };
    
    // Load head mesh from binary file
    static bool load(const std::string& path, HeadMesh& mesh) {
        printf("[HeadMesh] Loading from: %s\n", path.c_str());
        
        FILE* fp = fopen(path.c_str(), "rb");
        if (!fp) {
            printf("[HeadMesh] Failed to open file\n");
            return false;
        }
        
        // Read header
        uint32_t vertexCount, indexCount, neckStart, neckCount;
        fread(&vertexCount, sizeof(uint32_t), 1, fp);
        fread(&indexCount, sizeof(uint32_t), 1, fp);
        fread(&neckStart, sizeof(uint32_t), 1, fp);
        fread(&neckCount, sizeof(uint32_t), 1, fp);
        
        printf("[HeadMesh] Header: %u vertices, %u indices, neck at %u (%u verts)\n",
               vertexCount, indexCount, neckStart, neckCount);
        
        mesh.neckBoundaryStart = neckStart;
        mesh.neckBoundaryCount = neckCount;
        
        // Allocate
        mesh.vertices.resize(vertexCount);
        mesh.indices.resize(indexCount);
        
        // Read positions
        std::vector<float> positions(vertexCount * 3);
        fread(positions.data(), sizeof(float), vertexCount * 3, fp);
        
        // Read normals
        std::vector<float> normals(vertexCount * 3);
        fread(normals.data(), sizeof(float), vertexCount * 3, fp);
        
        // Read UVs
        std::vector<float> uvs(vertexCount * 2);
        fread(uvs.data(), sizeof(float), vertexCount * 2, fp);
        
        // Read indices
        fread(mesh.indices.data(), sizeof(uint32_t), indexCount, fp);
        
        fclose(fp);
        
        // Build vertices and compute bounds
        mesh.minBounds = Vec3(1e30f, 1e30f, 1e30f);
        mesh.maxBounds = Vec3(-1e30f, -1e30f, -1e30f);
        
        for (uint32_t i = 0; i < vertexCount; i++) {
            Vertex& v = mesh.vertices[i];
            
            v.position[0] = positions[i * 3 + 0];
            v.position[1] = positions[i * 3 + 1];
            v.position[2] = positions[i * 3 + 2];
            
            v.normal[0] = normals[i * 3 + 0];
            v.normal[1] = normals[i * 3 + 1];
            v.normal[2] = normals[i * 3 + 2];
            
            v.uv[0] = uvs[i * 2 + 0];
            v.uv[1] = uvs[i * 2 + 1];
            
            // Default white color (skin tone from material)
            v.color[0] = 1.0f;
            v.color[1] = 1.0f;
            v.color[2] = 1.0f;
            
            // Update bounds
            mesh.minBounds.x = std::min(mesh.minBounds.x, v.position[0]);
            mesh.minBounds.y = std::min(mesh.minBounds.y, v.position[1]);
            mesh.minBounds.z = std::min(mesh.minBounds.z, v.position[2]);
            mesh.maxBounds.x = std::max(mesh.maxBounds.x, v.position[0]);
            mesh.maxBounds.y = std::max(mesh.maxBounds.y, v.position[1]);
            mesh.maxBounds.z = std::max(mesh.maxBounds.z, v.position[2]);
        }
        
        mesh.center = Vec3(
            (mesh.minBounds.x + mesh.maxBounds.x) * 0.5f,
            (mesh.minBounds.y + mesh.maxBounds.y) * 0.5f,
            (mesh.minBounds.z + mesh.maxBounds.z) * 0.5f
        );
        
        mesh.loaded = true;
        
        printf("[HeadMesh] Loaded: %u vertices, %u triangles\n",
               vertexCount, indexCount / 3);
        printf("[HeadMesh] Bounds: [%.3f,%.3f] x [%.3f,%.3f] x [%.3f,%.3f]\n",
               mesh.minBounds.x, mesh.maxBounds.x,
               mesh.minBounds.y, mesh.maxBounds.y,
               mesh.minBounds.z, mesh.maxBounds.z);
        
        return true;
    }
    
    // Get facial region mask for a vertex (used for BlendShape generation)
    // Returns a value 0-1 indicating how much this vertex belongs to each region
    struct RegionWeights {
        float forehead = 0;
        float eyes = 0;
        float nose = 0;
        float mouth = 0;
        float chin = 0;
        float jaw = 0;
        float cheeks = 0;
        float ears = 0;
        float skull = 0;  // Back of head
    };
    
    static RegionWeights getRegionWeights(const Vertex& v, const HeadMesh& mesh) {
        RegionWeights w;
        
        // Normalize Y position (0 = bottom, 1 = top)
        float height = mesh.maxBounds.y - mesh.minBounds.y;
        float normalizedY = (v.position[1] - mesh.minBounds.y) / height;
        
        // Normalize Z position (0 = back, 1 = front)
        float depth = mesh.maxBounds.z - mesh.minBounds.z;
        float normalizedZ = (v.position[2] - mesh.minBounds.z) / depth;
        
        // Normalize X position (-1 = left, 1 = right)
        float width = mesh.maxBounds.x - mesh.minBounds.x;
        float normalizedX = (v.position[0] - mesh.center.x) / (width * 0.5f);
        
        // Determine if front or back of head
        bool isFront = normalizedZ > 0.3f;
        
        if (!isFront) {
            w.skull = 1.0f;
            return w;
        }
        
        // Front face regions based on Y position
        if (normalizedY > 0.75f) {
            w.forehead = 1.0f;
        } else if (normalizedY > 0.55f) {
            // Eye region
            float eyeWeight = 1.0f - std::abs(normalizedY - 0.65f) / 0.1f;
            w.eyes = std::max(0.0f, eyeWeight);
            w.forehead = 1.0f - w.eyes;
        } else if (normalizedY > 0.4f) {
            // Nose region
            float centerWeight = 1.0f - std::abs(normalizedX);
            w.nose = centerWeight;
            w.cheeks = 1.0f - centerWeight;
        } else if (normalizedY > 0.25f) {
            // Mouth region
            float centerWeight = 1.0f - std::abs(normalizedX) * 0.7f;
            w.mouth = centerWeight;
            w.cheeks = (1.0f - centerWeight) * 0.5f;
            w.jaw = (1.0f - centerWeight) * 0.5f;
        } else {
            // Chin and jaw
            float centerWeight = 1.0f - std::abs(normalizedX);
            w.chin = centerWeight;
            w.jaw = 1.0f - centerWeight;
        }
        
        // Ear detection (side of head)
        if (std::abs(normalizedX) > 0.7f && normalizedY > 0.35f && normalizedY < 0.7f) {
            float earWeight = (std::abs(normalizedX) - 0.7f) / 0.3f;
            w.ears = earWeight;
            // Reduce other weights
            float scale = 1.0f - earWeight;
            w.eyes *= scale;
            w.cheeks *= scale;
        }
        
        return w;
    }
    
    // Compute normals from triangle data
    static void recomputeNormals(HeadMesh& mesh) {
        // Reset normals
        for (auto& v : mesh.vertices) {
            v.normal[0] = v.normal[1] = v.normal[2] = 0;
        }
        
        // Accumulate face normals
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];
            
            Vertex& v0 = mesh.vertices[i0];
            Vertex& v1 = mesh.vertices[i1];
            Vertex& v2 = mesh.vertices[i2];
            
            // Edge vectors
            float e1x = v1.position[0] - v0.position[0];
            float e1y = v1.position[1] - v0.position[1];
            float e1z = v1.position[2] - v0.position[2];
            
            float e2x = v2.position[0] - v0.position[0];
            float e2y = v2.position[1] - v0.position[1];
            float e2z = v2.position[2] - v0.position[2];
            
            // Cross product
            float nx = e1y * e2z - e1z * e2y;
            float ny = e1z * e2x - e1x * e2z;
            float nz = e1x * e2y - e1y * e2x;
            
            // Add to vertex normals
            v0.normal[0] += nx; v0.normal[1] += ny; v0.normal[2] += nz;
            v1.normal[0] += nx; v1.normal[1] += ny; v1.normal[2] += nz;
            v2.normal[0] += nx; v2.normal[1] += ny; v2.normal[2] += nz;
        }
        
        // Normalize
        for (auto& v : mesh.vertices) {
            float len = std::sqrt(v.normal[0]*v.normal[0] + 
                                  v.normal[1]*v.normal[1] + 
                                  v.normal[2]*v.normal[2]);
            if (len > 0.0001f) {
                v.normal[0] /= len;
                v.normal[1] /= len;
                v.normal[2] /= len;
            }
        }
    }
};

} // namespace luma
