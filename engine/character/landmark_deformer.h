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

// Helper function for smooth interpolation
inline float smoothstep(float edge0, float edge1, float x) {
    if (edge0 == edge1) return (x < edge0) ? 0.0f : 1.0f;
    float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

// ============================================================================
// 2D Procrustes Analysis
// Finds optimal scale, rotation, and translation to align source points to target points
// Minimizes: ||target - (scale * R * source + translation)||^2
// ============================================================================

struct Procrustes2DResult {
    float scale;        // Uniform scale factor
    float rotation;     // Rotation angle in radians
    Vec2 translation;   // Translation vector
    float error;        // Mean squared error after alignment
    
    Procrustes2DResult() : scale(1.0f), rotation(0.0f), translation(0.0f, 0.0f), error(0.0f) {}
};

// Solve 2D Procrustes alignment: target ≈ scale * R(theta) * source + t
inline Procrustes2DResult solveProcrustes2D(const std::vector<Vec2>& source, 
                                            const std::vector<Vec2>& target) {
    Procrustes2DResult result;
    
    if (source.size() != target.size() || source.size() < 3) {
        printf("[Procrustes2D] Invalid input: source=%zu, target=%zu\n", source.size(), target.size());
        return result;
    }
    
    int n = (int)source.size();
    
    // Step 1: Compute centroids
    Vec2 srcCentroid(0, 0), tgtCentroid(0, 0);
    for (int i = 0; i < n; i++) {
        srcCentroid.x += source[i].x;
        srcCentroid.y += source[i].y;
        tgtCentroid.x += target[i].x;
        tgtCentroid.y += target[i].y;
    }
    srcCentroid.x /= n;
    srcCentroid.y /= n;
    tgtCentroid.x /= n;
    tgtCentroid.y /= n;
    
    // Step 2: Center the points
    std::vector<Vec2> srcCentered(n), tgtCentered(n);
    for (int i = 0; i < n; i++) {
        srcCentered[i].x = source[i].x - srcCentroid.x;
        srcCentered[i].y = source[i].y - srcCentroid.y;
        tgtCentered[i].x = target[i].x - tgtCentroid.x;
        tgtCentered[i].y = target[i].y - tgtCentroid.y;
    }
    
    // Step 3: Compute the 2x2 covariance matrix H = src^T * tgt
    // For 2D rotation, we need: H = [a b; c d]
    // where a = sum(sx*tx), b = sum(sx*ty), c = sum(sy*tx), d = sum(sy*ty)
    float a = 0, b = 0, c = 0, d = 0;
    for (int i = 0; i < n; i++) {
        a += srcCentered[i].x * tgtCentered[i].x;
        b += srcCentered[i].x * tgtCentered[i].y;
        c += srcCentered[i].y * tgtCentered[i].x;
        d += srcCentered[i].y * tgtCentered[i].y;
    }
    
    // Step 4: Compute rotation angle using atan2
    // For 2D, the optimal rotation is: theta = atan2(c - b, a + d)
    // This comes from the SVD of the 2x2 matrix
    result.rotation = std::atan2(c - b, a + d);
    
    float cosTheta = std::cos(result.rotation);
    float sinTheta = std::sin(result.rotation);
    
    // Step 5: Compute scale
    // scale = sum(tgt . R*src) / sum(src . src)
    float numerator = 0, denominator = 0;
    for (int i = 0; i < n; i++) {
        // Rotate source point
        float rx = cosTheta * srcCentered[i].x - sinTheta * srcCentered[i].y;
        float ry = sinTheta * srcCentered[i].x + cosTheta * srcCentered[i].y;
        
        numerator += tgtCentered[i].x * rx + tgtCentered[i].y * ry;
        denominator += srcCentered[i].x * srcCentered[i].x + srcCentered[i].y * srcCentered[i].y;
    }
    
    result.scale = (denominator > 1e-10f) ? numerator / denominator : 1.0f;
    
    // Step 6: Compute translation
    // t = tgtCentroid - scale * R * srcCentroid
    float rxc = cosTheta * srcCentroid.x - sinTheta * srcCentroid.y;
    float ryc = sinTheta * srcCentroid.x + cosTheta * srcCentroid.y;
    result.translation.x = tgtCentroid.x - result.scale * rxc;
    result.translation.y = tgtCentroid.y - result.scale * ryc;
    
    // Step 7: Compute alignment error
    float totalError = 0;
    for (int i = 0; i < n; i++) {
        float rx = cosTheta * source[i].x - sinTheta * source[i].y;
        float ry = sinTheta * source[i].x + cosTheta * source[i].y;
        float px = result.scale * rx + result.translation.x;
        float py = result.scale * ry + result.translation.y;
        float dx = px - target[i].x;
        float dy = py - target[i].y;
        totalError += dx * dx + dy * dy;
    }
    result.error = totalError / n;
    
    return result;
}

// Apply 2D transform: result = scale * R(theta) * point + translation
inline Vec2 applyTransform2D(const Vec2& point, float scale, float rotation, const Vec2& translation) {
    float cosTheta = std::cos(rotation);
    float sinTheta = std::sin(rotation);
    float rx = cosTheta * point.x - sinTheta * point.y;
    float ry = sinTheta * point.x + cosTheta * point.y;
    return Vec2(scale * rx + translation.x, scale * ry + translation.y);
}

// Inverse 2D transform for displacement: converts displacement in target space back to source space
// If forward is: p' = s * R * p + t
// Then displacement inverse is: d_src = R^(-1) * d_tgt / s
inline Vec2 inverseTransformDisplacement2D(const Vec2& displacement, float scale, float rotation) {
    if (std::abs(scale) < 1e-10f) return Vec2(0, 0);
    float cosTheta = std::cos(-rotation);  // Inverse rotation
    float sinTheta = std::sin(-rotation);
    float dx = displacement.x / scale;
    float dy = displacement.y / scale;
    return Vec2(cosTheta * dx - sinTheta * dy, sinTheta * dx + cosTheta * dy);
}

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
                
                // Always use the closest vertex (no threshold - OBJ and BIN should have same topology)
                if (bestIdx >= 0) {
                    landmarkBasePositions_[m.landmarkIndex] = Vec3(
                        vertices[bestIdx].position[0],
                        vertices[bestIdx].position[1],
                        vertices[bestIdx].position[2]
                    );
                    landmarkVertexIndices_[m.landmarkIndex] = bestIdx;
                    foundCount++;
                    
                    // Debug: print distance for eye landmarks
                    if (m.landmarkIndex == 36 || m.landmarkIndex == 45) {
                        printf("[LandmarkDeformer] Landmark %d: OBJ idx %d -> BIN idx %d, dist=%.6f\n",
                               m.landmarkIndex, m.vertexIndex, bestIdx, std::sqrt(bestDist));
                        printf("  OBJ transformed: (%.4f, %.4f, %.4f)\n", targetPos.x, targetPos.y, targetPos.z);
                        printf("  BIN vertex:      (%.4f, %.4f, %.4f)\n", 
                               vertices[bestIdx].position[0], vertices[bestIdx].position[1], vertices[bestIdx].position[2]);
                    }
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
        
        // Debug: print some landmark positions
        if (foundCount > 0) {
            printf("[LandmarkDeformer] Sample landmark positions:\n");
            int samples[] = {8, 30, 36, 45, 48, 54};  // chin, nose tip, eyes, mouth corners
            for (int idx : samples) {
                if (landmarkVertexIndices_[idx] >= 0) {
                    const Vec3& p = landmarkBasePositions_[idx];
                    // Find original mapping to show JSON vertex index
                    int jsonVertexIdx = -1;
                    for (const auto& m : mappings_) {
                        if (m.landmarkIndex == idx) {
                            jsonVertexIdx = m.vertexIndex;
                            break;
                        }
                    }
                    printf("  Landmark %d: pos=(%.4f, %.4f, %.4f), vertex=%d (JSON: %d)\n",
                           idx, p.x, p.y, p.z, landmarkVertexIndices_[idx], jsonVertexIdx);
                } else {
                    printf("  Landmark %d: NOT FOUND\n", idx);
                }
            }
        }
        
        // Check if OBJ positions were loaded correctly
        if (!transformedObjPositions.empty()) {
            printf("[LandmarkDeformer] OBJ vertex count: %zu, checking eye landmarks:\n", transformedObjPositions.size());
            for (const auto& m : mappings_) {
                if (m.landmarkIndex == 36 || m.landmarkIndex == 45) {
                    if (m.vertexIndex >= 0 && m.vertexIndex < (int)transformedObjPositions.size()) {
                        const Vec3& objPos = transformedObjPositions[m.vertexIndex];
                        printf("  Landmark %d: OBJ vertex %d -> transformed pos (%.4f, %.4f, %.4f)\n",
                               m.landmarkIndex, m.vertexIndex, objPos.x, objPos.y, objPos.z);
                    }
                }
            }
        }
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
    
    // Deform mesh based on detected 2D landmarks using Procrustes alignment
    // landmarks2D: 68 points normalized to face bounding box (0-1)
    // faceAspect: face_bbox.width / face_bbox.height
    void deformFromLandmarks2D(const std::vector<Vec2>& landmarks2D, 
                                float faceAspect,
                                std::vector<Vertex>& outVertices) {
        if (baseVertices_.empty() || mappings_.empty()) {
            printf("[LandmarkDeformer] No base mesh or mappings loaded\n");
            return;
        }
        
        if (landmarks2D.size() < 68) {
            printf("[LandmarkDeformer] Need 68 landmarks, got %zu\n", landmarks2D.size());
            return;
        }
        
        if (landmarkBasePositions_.size() < 68) {
            printf("[LandmarkDeformer] Mesh landmarks not initialized (have %zu)\n", landmarkBasePositions_.size());
            return;
        }
        
        printf("[LandmarkDeformer] === Direct Normalized Deformation ===\n");
        printf("[LandmarkDeformer] Face aspect ratio: %.3f\n", faceAspect);
        
        // Debug: print sample detected landmarks (normalized to face bbox 0-1)
        printf("[LandmarkDeformer] Sample photo landmarks (normalized to face bbox 0-1):\n");
        printf("  Chin (8): (%.3f, %.3f)\n", landmarks2D[8].x, landmarks2D[8].y);
        printf("  Nose tip (30): (%.3f, %.3f)\n", landmarks2D[30].x, landmarks2D[30].y);
        printf("  Left eye outer (36): (%.3f, %.3f)\n", landmarks2D[36].x, landmarks2D[36].y);
        printf("  Right eye outer (45): (%.3f, %.3f)\n", landmarks2D[45].x, landmarks2D[45].y);
        
        // =====================================================================
        // Step 1: Extract 3D mesh landmark positions and project to 2D (XY plane)
        // =====================================================================
        std::vector<Vec3> mesh3D(68);
        std::vector<Vec2> mesh2D(68);
        
        for (int i = 0; i < 68; i++) {
            mesh3D[i] = landmarkBasePositions_[i];
            mesh2D[i] = Vec2(mesh3D[i].x, mesh3D[i].y);  // Orthographic projection: just take XY
        }
        
        // =====================================================================
        // Step 2: Compute bounding boxes and normalize both point sets to 0-1
        // =====================================================================
        
        // Mesh 2D bounding box (from projected landmarks)
        Vec2 meshLmMin(1e30f, 1e30f), meshLmMax(-1e30f, -1e30f);
        for (int i = 0; i < 68; i++) {
            meshLmMin.x = std::min(meshLmMin.x, mesh2D[i].x);
            meshLmMin.y = std::min(meshLmMin.y, mesh2D[i].y);
            meshLmMax.x = std::max(meshLmMax.x, mesh2D[i].x);
            meshLmMax.y = std::max(meshLmMax.y, mesh2D[i].y);
        }
        float meshLmW = meshLmMax.x - meshLmMin.x;
        float meshLmH = meshLmMax.y - meshLmMin.y;
        
        printf("[LandmarkDeformer] Mesh landmark bbox: (%.4f,%.4f)-(%.4f,%.4f), size: %.4f x %.4f\n",
               meshLmMin.x, meshLmMin.y, meshLmMax.x, meshLmMax.y, meshLmW, meshLmH);
        
        // =====================================================================
        // Step 3: Align photo landmarks to mesh landmarks using key reference points
        // Use eyes and nose as anchors for alignment
        // =====================================================================
        
        // Get key reference points from mesh (in mesh coordinates)
        Vec2 meshLeftEye = mesh2D[36];   // Left eye outer corner
        Vec2 meshRightEye = mesh2D[45];  // Right eye outer corner
        Vec2 meshNose = mesh2D[30];      // Nose tip
        
        // Calculate mesh eye center and eye distance
        Vec2 meshEyeCenter((meshLeftEye.x + meshRightEye.x) / 2.0f, 
                           (meshLeftEye.y + meshRightEye.y) / 2.0f);
        float meshEyeDist = std::sqrt((meshRightEye.x - meshLeftEye.x) * (meshRightEye.x - meshLeftEye.x) +
                                      (meshRightEye.y - meshLeftEye.y) * (meshRightEye.y - meshLeftEye.y));
        
        // Get key reference points from photo (normalized 0-1, Y flipped)
        Vec2 photoLeftEye(landmarks2D[36].x, 1.0f - landmarks2D[36].y);
        Vec2 photoRightEye(landmarks2D[45].x, 1.0f - landmarks2D[45].y);
        Vec2 photoNose(landmarks2D[30].x, 1.0f - landmarks2D[30].y);
        
        // Calculate photo eye center and eye distance
        Vec2 photoEyeCenter((photoLeftEye.x + photoRightEye.x) / 2.0f,
                            (photoLeftEye.y + photoRightEye.y) / 2.0f);
        float photoEyeDist = std::sqrt((photoRightEye.x - photoLeftEye.x) * (photoRightEye.x - photoLeftEye.x) +
                                       (photoRightEye.y - photoLeftEye.y) * (photoRightEye.y - photoLeftEye.y));
        
        // Calculate scale factor: mesh eye distance / photo eye distance
        float scaleFactor = (photoEyeDist > 1e-6f) ? meshEyeDist / photoEyeDist : 1.0f;
        
        printf("[LandmarkDeformer] Eye distance: mesh=%.4f, photo=%.4f, scale=%.4f\n",
               meshEyeDist, photoEyeDist, scaleFactor);
        printf("[LandmarkDeformer] Eye center: mesh=(%.4f,%.4f), photo=(%.4f,%.4f)\n",
               meshEyeCenter.x, meshEyeCenter.y, photoEyeCenter.x, photoEyeCenter.y);
        
        // =====================================================================
        // Step 4: Transform photo landmarks to mesh space and compute displacements
        // Transform: scale around photo eye center, then translate to mesh eye center
        // =====================================================================
        std::vector<Vec3> delta3D(68);
        
        printf("[LandmarkDeformer] Sample 2D displacements:\n");
        for (int i = 0; i < 68; i++) {
            // Photo landmark (Y flipped)
            Vec2 photoLm(landmarks2D[i].x, 1.0f - landmarks2D[i].y);
            
            // Transform photo landmark to mesh space:
            // 1. Translate so photo eye center is at origin
            // 2. Scale by scaleFactor
            // 3. Translate to mesh eye center
            Vec2 transformedPhoto;
            transformedPhoto.x = (photoLm.x - photoEyeCenter.x) * scaleFactor + meshEyeCenter.x;
            transformedPhoto.y = (photoLm.y - photoEyeCenter.y) * scaleFactor + meshEyeCenter.y;
            
            // Displacement = where photo wants the point - where mesh has it
            Vec2 delta2D;
            delta2D.x = transformedPhoto.x - mesh2D[i].x;
            delta2D.y = transformedPhoto.y - mesh2D[i].y;
            
            // Apply deform strength
            delta3D[i].x = delta2D.x * deformStrength_;
            delta3D[i].y = delta2D.y * deformStrength_;
            delta3D[i].z = 0.0f;  // Cannot recover depth from 2D
            
            // Debug output for key landmarks
            if (i == 8 || i == 30 || i == 36 || i == 45 || i == 48 || i == 54) {
                printf("  Landmark %d: photo=(%.3f,%.3f) -> transformed=(%.4f,%.4f), mesh=(%.4f,%.4f), delta=(%.4f,%.4f)\n",
                       i, photoLm.x, photoLm.y, transformedPhoto.x, transformedPhoto.y, 
                       mesh2D[i].x, mesh2D[i].y, delta2D.x, delta2D.y);
            }
        }
        
        printf("[LandmarkDeformer] Sample 3D displacements (strength=%.0f%%):\n", deformStrength_ * 100.0f);
        printf("  Chin (8): delta3D=(%.5f, %.5f, %.5f)\n", delta3D[8].x, delta3D[8].y, delta3D[8].z);
        printf("  Nose (30): delta3D=(%.5f, %.5f, %.5f)\n", delta3D[30].x, delta3D[30].y, delta3D[30].z);
        printf("  Left eye (36): delta3D=(%.5f, %.5f, %.5f)\n", delta3D[36].x, delta3D[36].y, delta3D[36].z);
        
        // =====================================================================
        // Step 5: Apply RBF interpolation to propagate landmark displacements to all vertices
        // =====================================================================
        applyRBFDeformation(delta3D, outVertices);
        
        printf("[LandmarkDeformer] Direct normalized deformation complete: %zu vertices\n", outVertices.size());
    }
    
    // Apply RBF-based deformation to propagate landmark displacements to all mesh vertices
    void applyRBFDeformation(const std::vector<Vec3>& landmarkDeltas, std::vector<Vertex>& outVertices) {
        outVertices = baseVertices_;
        
        if (landmarkDeltas.size() < 68) {
            printf("[LandmarkDeformer] Not enough landmark deltas\n");
            return;
        }
        
        // Collect control points (landmarks with their 3D positions and displacements)
        struct ControlPoint {
            Vec3 position;
            Vec3 displacement;
        };
        std::vector<ControlPoint> controlPoints;
        
        for (int i = 0; i < 68; i++) {
            if (landmarkBasePositions_[i].x == 0 && landmarkBasePositions_[i].y == 0 && landmarkBasePositions_[i].z == 0) {
                continue;  // Skip invalid landmarks
            }
            ControlPoint cp;
            cp.position = landmarkBasePositions_[i];
            cp.displacement = landmarkDeltas[i];
            controlPoints.push_back(cp);
        }
        
        if (controlPoints.empty()) {
            printf("[LandmarkDeformer] No valid control points\n");
            return;
        }
        
        printf("[LandmarkDeformer] RBF interpolation with %zu control points\n", controlPoints.size());
        
        // Define face region for deformation (don't deform back of head, neck, etc.)
        Vec3 faceMin(1e30f, 1e30f, 1e30f), faceMax(-1e30f, -1e30f, -1e30f);
        for (const auto& cp : controlPoints) {
            faceMin.x = std::min(faceMin.x, cp.position.x);
            faceMin.y = std::min(faceMin.y, cp.position.y);
            faceMin.z = std::min(faceMin.z, cp.position.z);
            faceMax.x = std::max(faceMax.x, cp.position.x);
            faceMax.y = std::max(faceMax.y, cp.position.y);
            faceMax.z = std::max(faceMax.z, cp.position.z);
        }
        
        // IMPORTANT: Only expand in X and Y directions, NOT in Z (depth)
        // We want to deform only the FRONT of the face, not the back of the head
        float margin = meshScale_ * 0.1f;
        faceMin.x -= margin; 
        faceMin.y -= margin;
        // faceMin.z stays as-is (don't expand backwards!)
        faceMax.x += margin; 
        faceMax.y += margin; 
        faceMax.z += margin * 0.5f;  // Only slightly forward
        
        printf("[LandmarkDeformer] Face region Z range: %.4f to %.4f\n", faceMin.z, faceMax.z);
        
        // RBF influence radius
        float influenceRadius = meshScale_ * influenceRadiusFactor_;
        
        // Calculate the average Z of landmarks (face front plane)
        float avgLandmarkZ = 0;
        for (const auto& cp : controlPoints) {
            avgLandmarkZ += cp.position.z;
        }
        avgLandmarkZ /= controlPoints.size();
        
        // Only deform vertices that are in FRONT of (or at) the landmark plane
        // This prevents back-of-head deformation
        float zThreshold = avgLandmarkZ - meshScale_ * 0.05f;  // Small tolerance behind landmarks
        
        printf("[LandmarkDeformer] Avg landmark Z: %.4f, Z threshold: %.4f\n", avgLandmarkZ, zThreshold);
        
        int deformedCount = 0;
        int skippedBack = 0;
        for (auto& v : outVertices) {
            Vec3 pos(v.position[0], v.position[1], v.position[2]);
            
            // Skip vertices outside face region (XY bounds)
            if (pos.x < faceMin.x || pos.x > faceMax.x ||
                pos.y < faceMin.y || pos.y > faceMax.y) {
                continue;
            }
            
            // CRITICAL: Skip vertices BEHIND the face (back of head)
            // Only deform vertices that are in front of (Z >= threshold)
            if (pos.z < zThreshold) {
                skippedBack++;
                continue;
            }
            
            // Compute weighted displacement using RBF
            Vec3 totalDisp(0, 0, 0);
            float totalWeight = 0;
            
            for (const auto& cp : controlPoints) {
                float dx = pos.x - cp.position.x;
                float dy = pos.y - cp.position.y;
                float dz = pos.z - cp.position.z;
                float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                
                // Use Wendland C2 kernel (compact support)
                float weight = RBFKernel::wendlandC2(dist, influenceRadius);
                
                if (weight > 1e-6f) {
                    totalDisp.x += weight * cp.displacement.x;
                    totalDisp.y += weight * cp.displacement.y;
                    totalDisp.z += weight * cp.displacement.z;
                    totalWeight += weight;
                }
            }
            
            if (totalWeight > 1e-6f) {
                v.position[0] += totalDisp.x / totalWeight;
                v.position[1] += totalDisp.y / totalWeight;
                v.position[2] += totalDisp.z / totalWeight;
                deformedCount++;
            }
        }
        
        printf("[LandmarkDeformer] Skipped %d back-of-head vertices\n", skippedBack);
        
        printf("[LandmarkDeformer] RBF deformation applied to %d vertices\n", deformedCount);
    }
    
    // Get the landmark vertex indices for visualization
    const std::vector<LandmarkMapping>& getMappings() const { return mappings_; }
    
    // Get base landmark positions (for visualization)
    const std::vector<Vec3>& getLandmarkBasePositions() const { return landmarkBasePositions_; }
    
    // Get current landmark positions from deformed mesh
    std::vector<Vec3> getCurrentLandmarkPositions(const std::vector<Vertex>& vertices) const {
        std::vector<Vec3> positions(68, Vec3(0, 0, 0));
        for (const auto& m : mappings_) {
            if (m.landmarkIndex >= 0 && m.landmarkIndex < 68) {
                int vidx = landmarkVertexIndices_[m.landmarkIndex];
                if (vidx >= 0 && vidx < (int)vertices.size()) {
                    positions[m.landmarkIndex] = Vec3(
                        vertices[vidx].position[0],
                        vertices[vidx].position[1],
                        vertices[vidx].position[2]
                    );
                }
            }
        }
        return positions;
    }

private:
    
    std::vector<LandmarkMapping> mappings_;
    std::vector<Vertex> baseVertices_;
    std::vector<Vec3> landmarkBasePositions_;
    std::vector<int> landmarkVertexIndices_;  // Actual vertex indices in the mesh
    
    Vec3 meshMin_, meshMax_, meshCenter_;
    float meshScale_ = 1.0f;
    
    // Configurable parameters
    float deformStrength_ = 0.8f;         // How much to apply detected shape (0-1)
    float influenceRadiusFactor_ = 0.5f;  // RBF influence radius as factor of mesh size
    
public:
    // Get landmark vertex indices (after OBJ->BIN conversion)
    const std::vector<int>& getLandmarkVertexIndices() const { return landmarkVertexIndices_; }
    
    // Parameter setters
    void setDeformStrength(float s) { deformStrength_ = s; }
    float getDeformStrength() const { return deformStrength_; }
    
    void setInfluenceRadius(float r) { influenceRadiusFactor_ = r; }
    float getInfluenceRadius() const { return influenceRadiusFactor_; }
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
