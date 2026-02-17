// Landmark-based Mesh Deformer
// Deforms a standard topology head mesh based on detected facial landmarks
// Uses RBF (Radial Basis Function) interpolation to propagate landmark displacements to all vertices
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace luma {

// ============================================================================
// Landmark Vertex Mapping
// ============================================================================

struct LandmarkMapping {
    int landmarkIndex;          // iBUG 68 landmark index (0-67)
    int vertexIndex;            // Corresponding vertex index in the mesh
    float weight;               // Weight for this mapping (usually 1.0)
    
    LandmarkMapping() : landmarkIndex(-1), vertexIndex(-1), weight(1.0f) {}
    LandmarkMapping(int lm, int v, float w = 1.0f) : landmarkIndex(lm), vertexIndex(v), weight(w) {}
};

// ============================================================================
// RBF Kernel Functions
// ============================================================================

namespace RBFKernel {
    // Thin Plate Spline: r^2 * log(r)
    inline float thinPlateSpline(float r) {
        if (r < 0.0001f) return 0.0f;
        return r * r * std::log(r);
    }
    
    // Gaussian: exp(-r^2 / (2 * sigma^2))
    inline float gaussian(float r, float sigma = 0.1f) {
        return std::exp(-(r * r) / (2.0f * sigma * sigma));
    }
    
    // Multiquadric: sqrt(r^2 + c^2)
    inline float multiquadric(float r, float c = 0.1f) {
        return std::sqrt(r * r + c * c);
    }
    
    // Inverse Multiquadric: 1 / sqrt(r^2 + c^2)
    inline float inverseMultiquadric(float r, float c = 0.1f) {
        return 1.0f / std::sqrt(r * r + c * c);
    }
    
    // Wendland C2 (compact support): (1-r)^4 * (4r + 1) for r < 1, else 0
    inline float wendlandC2(float r, float radius = 0.2f) {
        float nr = r / radius;
        if (nr >= 1.0f) return 0.0f;
        float t = 1.0f - nr;
        return t * t * t * t * (4.0f * nr + 1.0f);
    }
}

// ============================================================================
// Landmark Deformer
// ============================================================================

class LandmarkDeformer {
public:
    // Load landmark-to-vertex mapping from JSON file
    bool loadMapping(const std::string& jsonPath) {
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            printf("[LandmarkDeformer] Failed to open mapping file: %s\n", jsonPath.c_str());
            return false;
        }
        
        mappings_.clear();
        
        // Simple JSON parsing (assumes specific format from auto_landmark_estimator.py)
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        
        // Find landmarks array
        size_t landmarksPos = content.find("\"landmarks\"");
        if (landmarksPos == std::string::npos) {
            printf("[LandmarkDeformer] No 'landmarks' array found in JSON\n");
            return false;
        }
        
        // Parse each landmark entry
        size_t pos = landmarksPos;
        while ((pos = content.find("\"id\":", pos)) != std::string::npos) {
            // Extract id
            size_t idStart = pos + 5;
            size_t idEnd = content.find_first_of(",}", idStart);
            int id = std::stoi(content.substr(idStart, idEnd - idStart));
            
            // Find vertices array for this landmark
            size_t verticesPos = content.find("\"vertices\":", pos);
            if (verticesPos == std::string::npos || verticesPos > pos + 200) {
                pos = idEnd;
                continue;
            }
            
            // Extract first vertex index
            size_t bracketStart = content.find("[", verticesPos);
            size_t bracketEnd = content.find("]", bracketStart);
            if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                std::string verticesStr = content.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                
                // Parse first number
                size_t numEnd = verticesStr.find_first_of(",]");
                if (numEnd == std::string::npos) numEnd = verticesStr.length();
                
                std::string numStr = verticesStr.substr(0, numEnd);
                // Remove whitespace
                numStr.erase(std::remove_if(numStr.begin(), numStr.end(), ::isspace), numStr.end());
                
                if (!numStr.empty()) {
                    int vertexIdx = std::stoi(numStr);
                    mappings_.push_back(LandmarkMapping(id, vertexIdx, 1.0f));
                }
            }
            
            pos = idEnd;
        }
        
        printf("[LandmarkDeformer] Loaded %zu landmark mappings\n", mappings_.size());
        return mappings_.size() > 0;
    }
    
    // Set the base mesh (undeformed reference)
    // Also loads reference positions from OBJ file to handle vertex index mismatch
    void setBaseMesh(const std::vector<Vertex>& vertices, const std::string& objPath = "") {
        baseVertices_ = vertices;
        
        // Compute mesh bounds first
        meshMin_ = Vec3(1e30f, 1e30f, 1e30f);
        meshMax_ = Vec3(-1e30f, -1e30f, -1e30f);
        for (const auto& v : vertices) {
            meshMin_.x = std::min(meshMin_.x, v.position[0]);
            meshMin_.y = std::min(meshMin_.y, v.position[1]);
            meshMin_.z = std::min(meshMin_.z, v.position[2]);
            meshMax_.x = std::max(meshMax_.x, v.position[0]);
            meshMax_.y = std::max(meshMax_.y, v.position[1]);
            meshMax_.z = std::max(meshMax_.z, v.position[2]);
        }
        meshCenter_ = Vec3(
            (meshMin_.x + meshMax_.x) * 0.5f,
            (meshMin_.y + meshMax_.y) * 0.5f,
            (meshMin_.z + meshMax_.z) * 0.5f
        );
        meshScale_ = std::max({meshMax_.x - meshMin_.x, meshMax_.y - meshMin_.y, meshMax_.z - meshMin_.z});
        
        // Try to load reference OBJ to get original vertex positions
        std::vector<Vec3> objPositions;
        if (!objPath.empty()) {
            loadObjPositions(objPath, objPositions);
        }
        
        // Compute transform from OBJ space to mesh space
        // The OBJ and BIN may have different scales/centers due to normalization
        Vec3 objCenter(0, 0, 0), objMin(1e30f, 1e30f, 1e30f), objMax(-1e30f, -1e30f, -1e30f);
        for (const auto& p : objPositions) {
            objMin.x = std::min(objMin.x, p.x); objMax.x = std::max(objMax.x, p.x);
            objMin.y = std::min(objMin.y, p.y); objMax.y = std::max(objMax.y, p.y);
            objMin.z = std::min(objMin.z, p.z); objMax.z = std::max(objMax.z, p.z);
        }
        objCenter = Vec3((objMin.x + objMax.x) * 0.5f, (objMin.y + objMax.y) * 0.5f, (objMin.z + objMax.z) * 0.5f);
        float objScale = std::max({objMax.x - objMin.x, objMax.y - objMin.y, objMax.z - objMin.z});
        
        printf("[LandmarkDeformer] OBJ bounds: [%.3f,%.3f] x [%.3f,%.3f] x [%.3f,%.3f], scale=%.4f\n",
               objMin.x, objMax.x, objMin.y, objMax.y, objMin.z, objMax.z, objScale);
        printf("[LandmarkDeformer] Mesh bounds: [%.3f,%.3f] x [%.3f,%.3f] x [%.3f,%.3f], scale=%.4f\n",
               meshMin_.x, meshMax_.x, meshMin_.y, meshMax_.y, meshMin_.z, meshMax_.z, meshScale_);
        
        // Transform OBJ positions to mesh space
        float scaleRatio = (objScale > 0.0001f) ? meshScale_ / objScale : 1.0f;
        std::vector<Vec3> transformedObjPositions;
        for (const auto& p : objPositions) {
            Vec3 transformed;
            transformed.x = (p.x - objCenter.x) * scaleRatio + meshCenter_.x;
            transformed.y = (p.y - objCenter.y) * scaleRatio + meshCenter_.y;
            transformed.z = (p.z - objCenter.z) * scaleRatio + meshCenter_.z;
            transformedObjPositions.push_back(transformed);
        }
        
        // Extract positions of landmark vertices
        landmarkBasePositions_.clear();
        landmarkBasePositions_.resize(68, Vec3(0, 0, 0));
        landmarkVertexIndices_.clear();
        landmarkVertexIndices_.resize(68, -1);
        
        int foundCount = 0;
        for (const auto& m : mappings_) {
            if (m.landmarkIndex < 0 || m.landmarkIndex >= 68) continue;
            
            Vec3 targetPos;
            bool hasTarget = false;
            
            // If we have transformed OBJ positions, use them as reference
            if (!transformedObjPositions.empty() && m.vertexIndex >= 0 && m.vertexIndex < (int)transformedObjPositions.size()) {
                targetPos = transformedObjPositions[m.vertexIndex];
                hasTarget = true;
            }
            
            if (hasTarget) {
                // Find closest vertex in the actual mesh
                int bestIdx = -1;
                float bestDist = 1e30f;
                for (size_t i = 0; i < vertices.size(); i++) {
                    float dx = vertices[i].position[0] - targetPos.x;
                    float dy = vertices[i].position[1] - targetPos.y;
                    float dz = vertices[i].position[2] - targetPos.z;
                    float dist = dx*dx + dy*dy + dz*dz;
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx = (int)i;
                    }
                }
                
                // Use a more generous threshold (0.5% of mesh scale)
                float threshold = meshScale_ * 0.005f;
                if (bestIdx >= 0 && bestDist < threshold * threshold) {
                    landmarkBasePositions_[m.landmarkIndex] = Vec3(
                        vertices[bestIdx].position[0],
                        vertices[bestIdx].position[1],
                        vertices[bestIdx].position[2]
                    );
                    landmarkVertexIndices_[m.landmarkIndex] = bestIdx;
                    foundCount++;
                }
            } else if (m.vertexIndex >= 0 && m.vertexIndex < (int)vertices.size()) {
                // Direct index lookup (if mesh matches)
                const auto& v = vertices[m.vertexIndex];
                landmarkBasePositions_[m.landmarkIndex] = Vec3(v.position[0], v.position[1], v.position[2]);
                landmarkVertexIndices_[m.landmarkIndex] = m.vertexIndex;
                foundCount++;
            }
        }
        
        printf("[LandmarkDeformer] Base mesh set: %zu vertices, scale=%.4f, landmarks found: %d/68\n", 
               vertices.size(), meshScale_, foundCount);
    }
    
private:
    // Load vertex positions from OBJ file
    void loadObjPositions(const std::string& path, std::vector<Vec3>& positions) {
        std::ifstream file(path);
        if (!file.is_open()) {
            printf("[LandmarkDeformer] Could not open OBJ: %s\n", path.c_str());
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.size() > 2 && line[0] == 'v' && line[1] == ' ') {
                std::istringstream iss(line.substr(2));
                float x, y, z;
                if (iss >> x >> y >> z) {
                    positions.push_back(Vec3(x, y, z));
                }
            }
        }
        printf("[LandmarkDeformer] Loaded %zu positions from OBJ\n", positions.size());
    }
    
public:
    
    // Deform mesh based on detected 2D landmarks
    // landmarks2D: 68 points in normalized image coordinates (0-1)
    // imageAspect: width/height of the source image
    void deformFromLandmarks2D(const std::vector<Vec2>& landmarks2D, 
                                float imageAspect,
                                std::vector<Vertex>& outVertices) {
        if (baseVertices_.empty() || mappings_.empty()) {
            printf("[LandmarkDeformer] No base mesh or mappings loaded\n");
            return;
        }
        
        if (landmarks2D.size() < 68) {
            printf("[LandmarkDeformer] Need 68 landmarks, got %zu\n", landmarks2D.size());
            return;
        }
        
        // Step 1: Compute alignment transform (Procrustes analysis)
        // Find the best scale, rotation, and translation to align detected landmarks to mesh landmarks
        
        // Collect valid landmark pairs
        std::vector<Vec2> detected2D;
        std::vector<Vec2> mesh2D;  // Project mesh landmarks to 2D (XY plane)
        std::vector<int> validIndices;
        
        for (const auto& m : mappings_) {
            if (m.landmarkIndex < 0 || m.landmarkIndex >= 68) continue;
            if (m.landmarkIndex >= (int)landmarkBasePositions_.size()) continue;
            
            const Vec3& basePos = landmarkBasePositions_[m.landmarkIndex];
            if (basePos.x == 0 && basePos.y == 0 && basePos.z == 0) continue;
            
            detected2D.push_back(landmarks2D[m.landmarkIndex]);
            mesh2D.push_back(Vec2(basePos.x, basePos.y));  // Use XY as 2D
            validIndices.push_back(m.landmarkIndex);
        }
        
        if (validIndices.empty()) {
            printf("[LandmarkDeformer] No valid landmark pairs\n");
            return;
        }
        
        printf("[LandmarkDeformer] Aligning %zu landmark pairs\n", validIndices.size());
        
        // Compute centroids
        Vec2 centroidDetected(0, 0), centroidMesh(0, 0);
        for (size_t i = 0; i < detected2D.size(); i++) {
            centroidDetected.x += detected2D[i].x;
            centroidDetected.y += detected2D[i].y;
            centroidMesh.x += mesh2D[i].x;
            centroidMesh.y += mesh2D[i].y;
        }
        centroidDetected.x /= detected2D.size();
        centroidDetected.y /= detected2D.size();
        centroidMesh.x /= mesh2D.size();
        centroidMesh.y /= mesh2D.size();
        
        // Compute scale: ratio of average distances from centroid
        float avgDistDetected = 0, avgDistMesh = 0;
        for (size_t i = 0; i < detected2D.size(); i++) {
            float dx = detected2D[i].x - centroidDetected.x;
            float dy = detected2D[i].y - centroidDetected.y;
            avgDistDetected += std::sqrt(dx*dx + dy*dy);
            
            dx = mesh2D[i].x - centroidMesh.x;
            dy = mesh2D[i].y - centroidMesh.y;
            avgDistMesh += std::sqrt(dx*dx + dy*dy);
        }
        avgDistDetected /= detected2D.size();
        avgDistMesh /= mesh2D.size();
        
        float scale = (avgDistDetected > 0.0001f) ? avgDistMesh / avgDistDetected : 1.0f;
        
        printf("[LandmarkDeformer] Scale factor: %.4f (detected spread: %.4f, mesh spread: %.4f)\n",
               scale, avgDistDetected, avgDistMesh);
        
        // Step 2: Compute relative displacements
        // Instead of absolute positions, we compute how much each landmark deviates
        // from its expected position relative to the centroid
        std::vector<Vec3> displacements(68, Vec3(0, 0, 0));
        std::vector<bool> validLandmark(68, false);
        
        // Deformation strength - controls how much the detected face shape affects the mesh
        // 1.0 = full deformation, 0.5 = half, etc.
        const float deformStrength = 0.3f;  // Conservative to avoid extreme deformation
        
        for (size_t i = 0; i < validIndices.size(); i++) {
            int lmIdx = validIndices[i];
            
            // Detected landmark relative to detected centroid (in normalized image space)
            float detRelX = detected2D[i].x - centroidDetected.x;
            float detRelY = -(detected2D[i].y - centroidDetected.y);  // Flip Y
            
            // Mesh landmark relative to mesh centroid
            float meshRelX = mesh2D[i].x - centroidMesh.x;
            float meshRelY = mesh2D[i].y - centroidMesh.y;
            
            // Scale detected relative position to mesh space
            float scaledDetX = detRelX * scale;
            float scaledDetY = detRelY * scale;
            
            // Displacement is the difference between where the landmark IS (in photo)
            // and where it SHOULD BE (in standard mesh), both relative to centroid
            float dispX = (scaledDetX - meshRelX) * deformStrength;
            float dispY = (scaledDetY - meshRelY) * deformStrength;
            
            displacements[lmIdx] = Vec3(dispX, dispY, 0);
            validLandmark[lmIdx] = true;
        }
        
        printf("[LandmarkDeformer] Deformation strength: %.1f%%\n", deformStrength * 100.0f);
        
        // Debug: print some displacement info
        float maxDisp = 0.0f;
        int validCount = 0;
        for (int i = 0; i < 68; i++) {
            if (validLandmark[i]) {
                validCount++;
                float d = displacements[i].length();
                if (d > maxDisp) maxDisp = d;
            }
        }
        printf("[LandmarkDeformer] Valid landmarks: %d, Max displacement: %.4f\n", validCount, maxDisp);
        
        // Print a few key displacements
        if (validLandmark[8]) {
            printf("[LandmarkDeformer] Chin (8): disp=(%.4f, %.4f, %.4f)\n",
                   displacements[8].x, displacements[8].y, displacements[8].z);
        }
        if (validLandmark[30]) {
            printf("[LandmarkDeformer] Nose tip (30): disp=(%.4f, %.4f, %.4f)\n",
                   displacements[30].x, displacements[30].y, displacements[30].z);
        }
        if (validLandmark[36]) {
            printf("[LandmarkDeformer] Left eye (36): disp=(%.4f, %.4f, %.4f)\n",
                   displacements[36].x, displacements[36].y, displacements[36].z);
        }
        
        // Step 3: Apply RBF interpolation to propagate displacements
        outVertices = baseVertices_;
        applyRBFDeformation(displacements, validLandmark, outVertices);
        
        printf("[LandmarkDeformer] Deformation applied to %zu vertices\n", outVertices.size());
    }
    
    // Deform mesh based on detected 3D landmarks (from 3DDFA or similar)
    void deformFromLandmarks3D(const std::vector<Vec3>& landmarks3D,
                                std::vector<Vertex>& outVertices) {
        if (baseVertices_.empty() || mappings_.empty()) {
            printf("[LandmarkDeformer] No base mesh or mappings loaded\n");
            return;
        }
        
        if (landmarks3D.size() < 68) {
            printf("[LandmarkDeformer] Need 68 landmarks, got %zu\n", landmarks3D.size());
            return;
        }
        
        // Calculate displacements from base positions
        std::vector<Vec3> displacements(68);
        std::vector<bool> validLandmark(68, false);
        
        for (const auto& m : mappings_) {
            if (m.landmarkIndex < 0 || m.landmarkIndex >= 68) continue;
            
            const Vec3& target = landmarks3D[m.landmarkIndex];
            const Vec3& basePos = landmarkBasePositions_[m.landmarkIndex];
            
            displacements[m.landmarkIndex] = Vec3(
                target.x - basePos.x,
                target.y - basePos.y,
                target.z - basePos.z
            );
            validLandmark[m.landmarkIndex] = true;
        }
        
        // Apply RBF interpolation
        outVertices = baseVertices_;
        applyRBFDeformation(displacements, validLandmark, outVertices);
    }
    
    // Get the landmark vertex indices for visualization
    const std::vector<LandmarkMapping>& getMappings() const { return mappings_; }
    
    // Get base landmark positions
    const std::vector<Vec3>& getLandmarkBasePositions() const { return landmarkBasePositions_; }

private:
    // Apply RBF interpolation to propagate landmark displacements to all vertices
    void applyRBFDeformation(const std::vector<Vec3>& displacements,
                              const std::vector<bool>& validLandmark,
                              std::vector<Vertex>& vertices) {
        // Collect valid control points
        std::vector<Vec3> controlPoints;
        std::vector<Vec3> controlDisplacements;
        
        for (size_t i = 0; i < 68; i++) {
            if (validLandmark[i]) {
                controlPoints.push_back(landmarkBasePositions_[i]);
                controlDisplacements.push_back(displacements[i]);
            }
        }
        
        if (controlPoints.empty()) {
            printf("[LandmarkDeformer] No valid control points\n");
            return;
        }
        
        printf("[LandmarkDeformer] RBF with %zu control points\n", controlPoints.size());
        
        // For each vertex, compute weighted sum of control point displacements
        // Using Wendland C2 kernel for locality (compact support)
        float influenceRadius = meshScale_ * 0.3f;  // 30% of mesh size
        
        for (auto& v : vertices) {
            Vec3 pos(v.position[0], v.position[1], v.position[2]);
            Vec3 totalDisp(0, 0, 0);
            float totalWeight = 0.0f;
            
            for (size_t i = 0; i < controlPoints.size(); i++) {
                float dist = (pos - controlPoints[i]).length();
                float weight = RBFKernel::wendlandC2(dist, influenceRadius);
                
                if (weight > 0.0001f) {
                    totalDisp.x += controlDisplacements[i].x * weight;
                    totalDisp.y += controlDisplacements[i].y * weight;
                    totalDisp.z += controlDisplacements[i].z * weight;
                    totalWeight += weight;
                }
            }
            
            if (totalWeight > 0.0001f) {
                v.position[0] += totalDisp.x / totalWeight;
                v.position[1] += totalDisp.y / totalWeight;
                v.position[2] += totalDisp.z / totalWeight;
            }
        }
    }
    
    std::vector<LandmarkMapping> mappings_;
    std::vector<Vertex> baseVertices_;
    std::vector<Vec3> landmarkBasePositions_;
    std::vector<int> landmarkVertexIndices_;  // Actual vertex indices in the mesh
    
    Vec3 meshMin_, meshMax_, meshCenter_;
    float meshScale_ = 1.0f;
};

// ============================================================================
// Photo-to-Mesh Pipeline
// ============================================================================

class PhotoToMeshPipeline {
public:
    struct Result {
        bool success = false;
        std::vector<Vertex> deformedVertices;
        std::vector<Vec2> detectedLandmarks;
        std::string errorMessage;
    };
    
    PhotoToMeshPipeline() = default;
    
    // Initialize with base mesh and landmark mapping
    bool initialize(const std::vector<Vertex>& baseMesh,
                    const std::string& landmarkMappingPath) {
        deformer_.setBaseMesh(baseMesh);
        return deformer_.loadMapping(landmarkMappingPath);
    }
    
    // Process a photo and deform the mesh
    // landmarks: 68 2D points detected from the photo (normalized 0-1)
    Result processLandmarks(const std::vector<Vec2>& landmarks, float imageAspect = 1.0f) {
        Result result;
        
        if (landmarks.size() < 68) {
            result.errorMessage = "Need 68 landmarks";
            return result;
        }
        
        result.detectedLandmarks = landmarks;
        deformer_.deformFromLandmarks2D(landmarks, imageAspect, result.deformedVertices);
        result.success = !result.deformedVertices.empty();
        
        return result;
    }
    
    // Get the deformer for direct access
    LandmarkDeformer& getDeformer() { return deformer_; }
    
private:
    LandmarkDeformer deformer_;
};

} // namespace luma
