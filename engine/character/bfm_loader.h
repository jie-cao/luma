// BFM (Basel Face Model) Loader
// Loads the BFM model exported from 3DDFA_V2 for realistic face reconstruction
#pragma once

#include "engine/renderer/mesh.h"
#include "engine/character/blend_shape.h"
#include "engine/foundation/math_types.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <direct.h>  // for _getcwd on Windows

namespace luma {

class BFMLoader {
public:
    struct BFMModel {
        std::vector<float> meanFace;      // (N*3) mean face vertices
        std::vector<float> shapeBasis;    // (N*3 x 40) shape basis
        std::vector<float> expBasis;      // (N*3 x 10) expression basis
        std::vector<uint32_t> triangles;  // (M*3) triangle indices
        std::vector<int32_t> keypoints;   // 68 landmark vertex indices
        
        int numVertices = 0;
        int numTriangles = 0;
        int shapeDims = 40;
        int expDims = 10;
        
        bool loaded = false;
    };
    
    // Normalize path separators for Windows
    static std::string normalizePath(const std::string& path) {
        std::string result = path;
        for (char& c : result) {
            if (c == '/') c = '\\';
        }
        return result;
    }
    
    // Helper to open file with path fallback
    static FILE* openBinaryFile(const std::string& basePath) {
        FILE* fp = fopen(basePath.c_str(), "rb");
        if (fp) return fp;
        
        // Try with backslashes
        std::string altPath = normalizePath(basePath);
        fp = fopen(altPath.c_str(), "rb");
        return fp;
    }
    
    // Load BFM model from binary files
    static bool load(const std::string& modelDir, BFMModel& model) {
        // Print current working directory for debugging
        char cwd[1024];
        if (_getcwd(cwd, sizeof(cwd))) {
            printf("[BFM] Current working directory: %s\n", cwd);
        }
        printf("[BFM] Loading from directory: '%s'\n", modelDir.c_str());
        
        // Build paths
        std::string meanPath = modelDir + "/bfm_mean_face.bin";
        std::string shapePath = modelDir + "/bfm_shape_basis.bin";
        std::string expPath = modelDir + "/bfm_exp_basis.bin";
        std::string triPath = modelDir + "/bfm_triangles.bin";
        std::string kpPath = modelDir + "/bfm_keypoints.bin";
        
        // Load mean face
        {
            printf("[BFM] Opening: %s\n", meanPath.c_str());
            FILE* fp = openBinaryFile(meanPath);
            if (!fp) {
                printf("[BFM] Failed to open mean face file\n");
                return false;
            }
            
            uint32_t numVerts;
            fread(&numVerts, sizeof(uint32_t), 1, fp);
            model.numVertices = numVerts;
            
            model.meanFace.resize(numVerts * 3);
            fread(model.meanFace.data(), sizeof(float), numVerts * 3, fp);
            fclose(fp);
            
            printf("[BFM] Loaded mean face: %d vertices\n", numVerts);
        }
        
        // Load shape basis
        {
            FILE* fp = openBinaryFile(shapePath);
            if (!fp) {
                printf("[BFM] Failed to open shape basis file\n");
                return false;
            }
            
            uint32_t rows, cols;
            fread(&rows, sizeof(uint32_t), 1, fp);
            fread(&cols, sizeof(uint32_t), 1, fp);
            model.shapeDims = cols;
            
            model.shapeBasis.resize(rows * cols);
            fread(model.shapeBasis.data(), sizeof(float), rows * cols, fp);
            fclose(fp);
            
            printf("[BFM] Loaded shape basis: %d x %d\n", rows, cols);
        }
        
        // Load expression basis
        {
            FILE* fp = openBinaryFile(expPath);
            if (!fp) {
                printf("[BFM] Failed to open expression basis file\n");
                return false;
            }
            
            uint32_t rows, cols;
            fread(&rows, sizeof(uint32_t), 1, fp);
            fread(&cols, sizeof(uint32_t), 1, fp);
            model.expDims = cols;
            
            model.expBasis.resize(rows * cols);
            fread(model.expBasis.data(), sizeof(float), rows * cols, fp);
            fclose(fp);
            
            printf("[BFM] Loaded expression basis: %d x %d\n", rows, cols);
        }
        
        // Load triangles
        {
            FILE* fp = openBinaryFile(triPath);
            if (!fp) {
                printf("[BFM] Failed to open triangles file\n");
                return false;
            }
            
            uint32_t numTris;
            fread(&numTris, sizeof(uint32_t), 1, fp);
            model.numTriangles = numTris;
            
            model.triangles.resize(numTris * 3);
            fread(model.triangles.data(), sizeof(uint32_t), numTris * 3, fp);
            fclose(fp);
            
            printf("[BFM] Loaded triangles: %d\n", numTris);
        }
        
        // Load keypoints (optional)
        {
            FILE* fp = openBinaryFile(kpPath);
            if (fp) {
                uint32_t numKp;
                fread(&numKp, sizeof(uint32_t), 1, fp);
                
                model.keypoints.resize(numKp);
                fread(model.keypoints.data(), sizeof(int32_t), numKp, fp);
                fclose(fp);
                
                printf("[BFM] Loaded keypoints: %d\n", numKp);
            }
        }
        
        model.loaded = true;
        return true;
    }
    
    // Reconstruct face from 3DMM coefficients
    // face = mean + shape_basis @ shape_coeffs + exp_basis @ exp_coeffs
    static std::vector<float> reconstructFace(
        const BFMModel& model,
        const std::vector<float>& shapeCoeffs,  // 40 shape coefficients
        const std::vector<float>& expCoeffs     // 10 expression coefficients
    ) {
        int n = model.numVertices * 3;
        std::vector<float> vertices(n);
        
        // Start with mean face
        for (int i = 0; i < n; i++) {
            vertices[i] = model.meanFace[i];
        }
        
        // Add shape deformation: shape_basis @ shape_coeffs
        int numShape = std::min((int)shapeCoeffs.size(), model.shapeDims);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < numShape; j++) {
                vertices[i] += model.shapeBasis[i * model.shapeDims + j] * shapeCoeffs[j];
            }
        }
        
        // Add expression deformation: exp_basis @ exp_coeffs
        int numExp = std::min((int)expCoeffs.size(), model.expDims);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < numExp; j++) {
                vertices[i] += model.expBasis[i * model.expDims + j] * expCoeffs[j];
            }
        }
        
        return vertices;
    }
    
    // Convert BFM model to renderable mesh with complete head
    // Generates back of head procedurally and merges with BFM face
    static void toMeshWithHead(
        const BFMModel& model,
        const std::vector<float>& shapeCoeffs,
        const std::vector<float>& expCoeffs,
        std::vector<Vertex>& outVertices,
        std::vector<uint32_t>& outIndices,
        float scale = 1.0f
    ) {
        // First generate the BFM face
        std::vector<Vertex> faceVerts;
        std::vector<uint32_t> faceIndices;
        toMesh(model, shapeCoeffs, expCoeffs, faceVerts, faceIndices, scale);
        
        // Generate back of head (skull) as ellipsoid
        std::vector<Vertex> skullVerts;
        std::vector<uint32_t> skullIndices;
        generateSkullMesh(skullVerts, skullIndices, scale);
        
        // Merge: face first, then skull
        outVertices = faceVerts;
        outIndices = faceIndices;
        
        uint32_t faceVertCount = (uint32_t)faceVerts.size();
        for (const auto& v : skullVerts) {
            outVertices.push_back(v);
        }
        for (uint32_t idx : skullIndices) {
            outIndices.push_back(idx + faceVertCount);
        }
        
        printf("[BFM] Combined mesh: %zu face + %zu skull = %zu total verts\n",
               faceVerts.size(), skullVerts.size(), outVertices.size());
    }
    
    // Generate back of head (skull) mesh
    static void generateSkullMesh(std::vector<Vertex>& verts, std::vector<uint32_t>& indices, float scale) {
        const float PI = 3.14159265f;
        
        // Skull dimensions (relative to scale which is head height)
        float headWidth = scale * 0.65f;
        float headDepth = scale * 0.75f;
        float headHeight = scale;
        
        int numLat = 20;  // latitude (top to bottom)
        int numLon = 24;  // longitude (around, but only back half)
        
        // Generate vertices for back half of head (from side to side, excluding front)
        for (int lat = 0; lat <= numLat; lat++) {
            float t = (float)lat / numLat;  // 0 = bottom, 1 = top
            float y = (t - 0.5f) * headHeight;  // centered at origin
            
            // Radius varies with height
            float radius;
            if (t < 0.15f) {
                radius = 0.5f + t * 2.5f;  // neck transition
            } else if (t < 0.5f) {
                radius = 0.875f + (t - 0.15f) * 0.36f;  // widening
            } else if (t < 0.85f) {
                radius = 1.0f - (t - 0.5f) * 0.3f;  // narrowing toward top
            } else {
                radius = 0.895f - (t - 0.85f) * 2.5f;  // top of head
            }
            radius = std::max(0.1f, radius);
            
            for (int lon = 0; lon <= numLon; lon++) {
                // Only generate back half: from -90 to +90 degrees (back)
                // Front is 0 degrees, back is 180 degrees
                float u = (float)lon / numLon;
                float angle = PI * 0.5f + u * PI;  // 90 to 270 degrees (back half)
                
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                
                float x = radius * headWidth * 0.5f * cosA;
                float z = radius * headDepth * 0.5f * sinA;
                
                Vertex v = {};
                v.position[0] = x;
                v.position[1] = y;
                v.position[2] = z;
                
                // Normal pointing outward
                v.normal[0] = cosA;
                v.normal[1] = 0;
                v.normal[2] = sinA;
                
                // UV
                v.uv[0] = u;
                v.uv[1] = t;
                
                // Skin color
                v.color[0] = 1.0f;
                v.color[1] = 1.0f;
                v.color[2] = 1.0f;
                
                verts.push_back(v);
            }
        }
        
        // Generate indices
        for (int lat = 0; lat < numLat; lat++) {
            for (int lon = 0; lon < numLon; lon++) {
                int curr = lat * (numLon + 1) + lon;
                int next = curr + numLon + 1;
                
                // Two triangles per quad - back faces outward
                indices.push_back(curr);
                indices.push_back(next);
                indices.push_back(curr + 1);
                
                indices.push_back(curr + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    }
    
    // Convert BFM model to renderable mesh (face only)
    static void toMesh(
        const BFMModel& model,
        const std::vector<float>& shapeCoeffs,
        const std::vector<float>& expCoeffs,
        std::vector<Vertex>& outVertices,
        std::vector<uint32_t>& outIndices,
        float scale = 1.0f
    ) {
        // Reconstruct face vertices
        std::vector<float> verts = reconstructFace(model, shapeCoeffs, expCoeffs);
        
        // Find bounds for normalization
        float minX = 1e30f, maxX = -1e30f;
        float minY = 1e30f, maxY = -1e30f;
        float minZ = 1e30f, maxZ = -1e30f;
        
        for (int i = 0; i < model.numVertices; i++) {
            float x = verts[i * 3 + 0];
            float y = verts[i * 3 + 1];
            float z = verts[i * 3 + 2];
            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
            minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
        }
        
        // Center and scale
        float cx = (minX + maxX) * 0.5f;
        float cy = (minY + maxY) * 0.5f;
        float cz = (minZ + maxZ) * 0.5f;
        float range = std::max({maxX - minX, maxY - minY, maxZ - minZ});
        float normScale = scale / range;
        
        printf("[BFM] Bounds: X[%.1f, %.1f] Y[%.1f, %.1f] Z[%.1f, %.1f]\n",
               minX, maxX, minY, maxY, minZ, maxZ);
        printf("[BFM] Center: (%.1f, %.1f, %.1f), Range: %.1f, Scale: %.6f\n",
               cx, cy, cz, range, normScale);
        
        // Create vertices
        outVertices.resize(model.numVertices);
        for (int i = 0; i < model.numVertices; i++) {
            Vertex& v = outVertices[i];
            
            // BFM coordinate system: X-right, Y-up, Z-forward (towards viewer)
            // Our system: X-right, Y-up, camera looks at -Z direction
            // The face should face the camera (towards -Z), so we negate both X and Z
            // This is equivalent to rotating 180 degrees around Y axis
            float x = -(verts[i * 3 + 0] - cx) * normScale;  // Negate X (flip horizontally)
            float y = (verts[i * 3 + 1] - cy) * normScale;   // Y stays same
            float z = -(verts[i * 3 + 2] - cz) * normScale;  // Negate Z
            
            v.position[0] = x;
            v.position[1] = y;
            v.position[2] = z;
            
            // Placeholder normal (will be computed later)
            v.normal[0] = 0;
            v.normal[1] = 0;
            v.normal[2] = 1;
            
            // UV (simple spherical mapping for now)
            float theta = std::atan2(x, -z);  // angle around Y
            float phi = std::asin(std::clamp(y / scale, -1.0f, 1.0f));  // angle from equator
            v.uv[0] = (theta / 3.14159f + 1.0f) * 0.5f;
            v.uv[1] = (phi / 1.5708f + 1.0f) * 0.5f;
            
            // White color (skin tone from material/texture)
            v.color[0] = 1.0f;
            v.color[1] = 1.0f;
            v.color[2] = 1.0f;
        }
        
        // Copy indices (BFM uses 0-indexed triangles)
        outIndices = model.triangles;
        
        // Compute normals
        computeNormals(outVertices, outIndices);
        
        printf("[BFM] Created mesh: %d vertices, %d triangles\n",
               (int)outVertices.size(), (int)outIndices.size() / 3);
    }
    
    // Compute smooth normals
    static void computeNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        // Reset normals
        for (auto& v : vertices) {
            v.normal[0] = v.normal[1] = v.normal[2] = 0;
        }
        
        // Accumulate face normals
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
                continue;
            }
            
            float* p0 = vertices[i0].position;
            float* p1 = vertices[i1].position;
            float* p2 = vertices[i2].position;
            
            // Edge vectors
            float e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
            float e2x = p2[0] - p0[0], e2y = p2[1] - p0[1], e2z = p2[2] - p0[2];
            
            // Cross product
            float nx = e1y * e2z - e1z * e2y;
            float ny = e1z * e2x - e1x * e2z;
            float nz = e1x * e2y - e1y * e2x;
            
            // Add to vertex normals (area-weighted)
            vertices[i0].normal[0] += nx; vertices[i0].normal[1] += ny; vertices[i0].normal[2] += nz;
            vertices[i1].normal[0] += nx; vertices[i1].normal[1] += ny; vertices[i1].normal[2] += nz;
            vertices[i2].normal[0] += nx; vertices[i2].normal[1] += ny; vertices[i2].normal[2] += nz;
        }
        
        // Normalize
        for (auto& v : vertices) {
            float len = std::sqrt(v.normal[0]*v.normal[0] + v.normal[1]*v.normal[1] + v.normal[2]*v.normal[2]);
            if (len > 0.0001f) {
                v.normal[0] /= len;
                v.normal[1] /= len;
                v.normal[2] /= len;
            } else {
                v.normal[0] = 0;
                v.normal[1] = 0;
                v.normal[2] = 1;
            }
        }
    }
};

} // namespace luma
