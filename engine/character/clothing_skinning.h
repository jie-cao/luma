// LUMA Clothing Skinning System
// Handles skeletal deformation of clothing meshes
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>

namespace luma {

// ============================================================================
// Bone Weight
// ============================================================================

struct BoneWeight {
    int boneIndex = -1;
    float weight = 0.0f;
    
    BoneWeight() = default;
    BoneWeight(int idx, float w) : boneIndex(idx), weight(w) {}
};

// ============================================================================
// Vertex Skin Data
// ============================================================================

struct VertexSkinData {
    // Up to 4 bone influences per vertex
    BoneWeight weights[4];
    int weightCount = 0;
    
    void addWeight(int boneIndex, float weight) {
        if (weight < 0.001f) return;
        
        // Find slot or replace smallest
        int minIdx = -1;
        float minWeight = weight;
        
        for (int i = 0; i < 4; i++) {
            if (weights[i].boneIndex == -1) {
                weights[i] = BoneWeight(boneIndex, weight);
                weightCount = std::max(weightCount, i + 1);
                return;
            }
            if (weights[i].weight < minWeight) {
                minWeight = weights[i].weight;
                minIdx = i;
            }
        }
        
        // Replace smallest if new weight is larger
        if (minIdx >= 0 && weight > minWeight) {
            weights[minIdx] = BoneWeight(boneIndex, weight);
        }
    }
    
    void normalize() {
        float total = 0.0f;
        for (int i = 0; i < weightCount; i++) {
            total += weights[i].weight;
        }
        if (total > 0.001f) {
            for (int i = 0; i < weightCount; i++) {
                weights[i].weight /= total;
            }
        }
    }
};

// ============================================================================
// Clothing Skin Data (per clothing item)
// ============================================================================

struct ClothingSkinData {
    std::string clothingId;
    std::vector<VertexSkinData> vertexWeights;
    
    // Bone name to index mapping (may differ from character skeleton)
    std::unordered_map<std::string, int> boneNameToIndex;
    
    // Bind pose matrices (inverse bind pose)
    std::vector<Mat4> inverseBindMatrices;
    
    bool isValid() const { return !vertexWeights.empty(); }
};

// ============================================================================
// Automatic Weight Generator
// ============================================================================

class AutoWeightGenerator {
public:
    // Generate weights based on distance to bones
    static ClothingSkinData generateWeights(const std::vector<Vertex>& vertices,
                                            const Skeleton& skeleton,
                                            float maxInfluenceDistance = 0.3f) {
        ClothingSkinData skinData;
        skinData.vertexWeights.resize(vertices.size());
        
        int boneCount = skeleton.getBoneCount();
        if (boneCount == 0) return skinData;
        
        // Get bone positions
        std::vector<Vec3> bonePositions(boneCount);
        std::vector<Vec3> boneEnds(boneCount);
        
        for (int i = 0; i < boneCount; i++) {
            Mat4 globalMat = skeleton.getGlobalMatrix(i);
            bonePositions[i] = Vec3(globalMat.m[12], globalMat.m[13], globalMat.m[14]);
            
            // Estimate bone end from child or parent
            int childIdx = findFirstChild(skeleton, i);
            if (childIdx >= 0) {
                Mat4 childMat = skeleton.getGlobalMatrix(childIdx);
                boneEnds[i] = Vec3(childMat.m[12], childMat.m[13], childMat.m[14]);
            } else {
                // Extend in local Y direction
                boneEnds[i] = bonePositions[i] + Vec3(0, 0.1f, 0);
            }
            
            // Map bone name to index
            skinData.boneNameToIndex[skeleton.getBoneName(i)] = i;
        }
        
        // Calculate inverse bind matrices
        skinData.inverseBindMatrices.resize(boneCount);
        for (int i = 0; i < boneCount; i++) {
            Mat4 bindMat = skeleton.getGlobalMatrix(i);
            skinData.inverseBindMatrices[i] = bindMat.inverse();
        }
        
        // Calculate weights for each vertex
        for (size_t vi = 0; vi < vertices.size(); vi++) {
            Vec3 pos(vertices[vi].position[0], vertices[vi].position[1], vertices[vi].position[2]);
            
            for (int bi = 0; bi < boneCount; bi++) {
                // Distance from vertex to bone segment
                float dist = distanceToSegment(pos, bonePositions[bi], boneEnds[bi]);
                
                if (dist < maxInfluenceDistance) {
                    // Linear falloff
                    float weight = 1.0f - (dist / maxInfluenceDistance);
                    weight = weight * weight;  // Quadratic falloff for smoother blending
                    
                    skinData.vertexWeights[vi].addWeight(bi, weight);
                }
            }
            
            skinData.vertexWeights[vi].normalize();
            
            // Ensure at least one bone has influence
            if (skinData.vertexWeights[vi].weightCount == 0) {
                // Find closest bone
                int closestBone = 0;
                float closestDist = FLT_MAX;
                
                for (int bi = 0; bi < boneCount; bi++) {
                    float dist = distanceToSegment(pos, bonePositions[bi], boneEnds[bi]);
                    if (dist < closestDist) {
                        closestDist = dist;
                        closestBone = bi;
                    }
                }
                
                skinData.vertexWeights[vi].addWeight(closestBone, 1.0f);
            }
        }
        
        return skinData;
    }
    
    // Generate weights using heat diffusion (more accurate but slower)
    static ClothingSkinData generateWeightsHeatDiffusion(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const Skeleton& skeleton,
        int iterations = 10) {
        
        // Start with distance-based weights
        ClothingSkinData skinData = generateWeights(vertices, skeleton);
        
        // Build vertex adjacency
        std::vector<std::vector<uint32_t>> adjacency(vertices.size());
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            adjacency[i0].push_back(i1);
            adjacency[i0].push_back(i2);
            adjacency[i1].push_back(i0);
            adjacency[i1].push_back(i2);
            adjacency[i2].push_back(i0);
            adjacency[i2].push_back(i1);
        }
        
        // Heat diffusion iterations
        int boneCount = skeleton.getBoneCount();
        std::vector<std::vector<float>> weights(vertices.size(), std::vector<float>(boneCount, 0.0f));
        
        // Initialize from distance-based weights
        for (size_t vi = 0; vi < vertices.size(); vi++) {
            for (int wi = 0; wi < skinData.vertexWeights[vi].weightCount; wi++) {
                int bi = skinData.vertexWeights[vi].weights[wi].boneIndex;
                float w = skinData.vertexWeights[vi].weights[wi].weight;
                if (bi >= 0 && bi < boneCount) {
                    weights[vi][bi] = w;
                }
            }
        }
        
        // Diffuse weights
        std::vector<std::vector<float>> newWeights = weights;
        
        for (int iter = 0; iter < iterations; iter++) {
            for (size_t vi = 0; vi < vertices.size(); vi++) {
                if (adjacency[vi].empty()) continue;
                
                for (int bi = 0; bi < boneCount; bi++) {
                    float sum = weights[vi][bi];
                    for (uint32_t neighbor : adjacency[vi]) {
                        sum += weights[neighbor][bi];
                    }
                    newWeights[vi][bi] = sum / (adjacency[vi].size() + 1);
                }
            }
            std::swap(weights, newWeights);
        }
        
        // Convert back to skin data format
        for (size_t vi = 0; vi < vertices.size(); vi++) {
            skinData.vertexWeights[vi] = VertexSkinData();
            
            for (int bi = 0; bi < boneCount; bi++) {
                if (weights[vi][bi] > 0.01f) {
                    skinData.vertexWeights[vi].addWeight(bi, weights[vi][bi]);
                }
            }
            
            skinData.vertexWeights[vi].normalize();
        }
        
        return skinData;
    }
    
private:
    static int findFirstChild(const Skeleton& skeleton, int parentIndex) {
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            if (skeleton.getBone(i)->parentIndex == parentIndex) {
                return i;
            }
        }
        return -1;
    }
    
    static float distanceToSegment(const Vec3& point, const Vec3& segStart, const Vec3& segEnd) {
        Vec3 seg = segEnd - segStart;
        float segLengthSq = seg.x * seg.x + seg.y * seg.y + seg.z * seg.z;
        
        if (segLengthSq < 0.0001f) {
            Vec3 diff = point - segStart;
            return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        }
        
        Vec3 toPoint = point - segStart;
        float t = (toPoint.x * seg.x + toPoint.y * seg.y + toPoint.z * seg.z) / segLengthSq;
        t = std::clamp(t, 0.0f, 1.0f);
        
        Vec3 closest = segStart + seg * t;
        Vec3 diff = point - closest;
        
        return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    }
};

// ============================================================================
// Clothing Skinning Deformer
// ============================================================================

class ClothingSkinningDeformer {
public:
    ClothingSkinningDeformer() = default;
    
    // Initialize with skin data
    void initialize(const ClothingSkinData& skinData) {
        skinData_ = skinData;
        initialized_ = true;
    }
    
    // Deform vertices using bone matrices
    void deform(const std::vector<Vertex>& restPose,
                const std::vector<Mat4>& boneMatrices,
                std::vector<Vertex>& outVertices) const {
        
        if (!initialized_) {
            outVertices = restPose;
            return;
        }
        
        outVertices.resize(restPose.size());
        
        for (size_t i = 0; i < restPose.size(); i++) {
            const Vertex& src = restPose[i];
            Vertex& dst = outVertices[i];
            
            Vec3 pos(src.position[0], src.position[1], src.position[2]);
            Vec3 normal(src.normal[0], src.normal[1], src.normal[2]);
            
            Vec3 skinnedPos(0, 0, 0);
            Vec3 skinnedNormal(0, 0, 0);
            
            const VertexSkinData& weights = skinData_.vertexWeights[i];
            
            for (int w = 0; w < weights.weightCount; w++) {
                int boneIdx = weights.weights[w].boneIndex;
                float weight = weights.weights[w].weight;
                
                if (boneIdx < 0 || boneIdx >= (int)boneMatrices.size()) continue;
                if (boneIdx >= (int)skinData_.inverseBindMatrices.size()) continue;
                
                // Get skinning matrix: boneMatrix * inverseBindMatrix
                Mat4 skinMatrix = boneMatrices[boneIdx] * skinData_.inverseBindMatrices[boneIdx];
                
                // Transform position
                Vec3 transformedPos = skinMatrix.transformPoint(pos);
                skinnedPos = skinnedPos + transformedPos * weight;
                
                // Transform normal (use upper 3x3)
                Vec3 transformedNormal = skinMatrix.transformDirection(normal);
                skinnedNormal = skinnedNormal + transformedNormal * weight;
            }
            
            // Normalize normal
            float normalLen = std::sqrt(skinnedNormal.x * skinnedNormal.x + 
                                         skinnedNormal.y * skinnedNormal.y + 
                                         skinnedNormal.z * skinnedNormal.z);
            if (normalLen > 0.001f) {
                skinnedNormal = skinnedNormal * (1.0f / normalLen);
            }
            
            // Copy result
            dst = src;
            dst.position[0] = skinnedPos.x;
            dst.position[1] = skinnedPos.y;
            dst.position[2] = skinnedPos.z;
            dst.normal[0] = skinnedNormal.x;
            dst.normal[1] = skinnedNormal.y;
            dst.normal[2] = skinnedNormal.z;
        }
    }
    
    // Deform in place
    void deformInPlace(std::vector<Vertex>& vertices,
                       const std::vector<Mat4>& boneMatrices) const {
        std::vector<Vertex> result;
        deform(vertices, boneMatrices, result);
        vertices = std::move(result);
    }
    
    const ClothingSkinData& getSkinData() const { return skinData_; }
    bool isInitialized() const { return initialized_; }
    
private:
    ClothingSkinData skinData_;
    bool initialized_ = false;
};

// ============================================================================
// Clothing Skinning Manager
// ============================================================================

class ClothingSkinningManager {
public:
    static ClothingSkinningManager& getInstance() {
        static ClothingSkinningManager instance;
        return instance;
    }
    
    // Generate and cache skin data for clothing
    const ClothingSkinData* getOrGenerateSkinData(
        const std::string& clothingId,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const Skeleton& skeleton,
        bool useHeatDiffusion = false) {
        
        // Check cache
        auto it = skinDataCache_.find(clothingId);
        if (it != skinDataCache_.end()) {
            return &it->second;
        }
        
        // Generate new skin data
        ClothingSkinData skinData;
        if (useHeatDiffusion) {
            skinData = AutoWeightGenerator::generateWeightsHeatDiffusion(
                vertices, indices, skeleton);
        } else {
            skinData = AutoWeightGenerator::generateWeights(vertices, skeleton);
        }
        
        skinData.clothingId = clothingId;
        skinDataCache_[clothingId] = std::move(skinData);
        
        return &skinDataCache_[clothingId];
    }
    
    // Get cached skin data
    const ClothingSkinData* getSkinData(const std::string& clothingId) const {
        auto it = skinDataCache_.find(clothingId);
        return it != skinDataCache_.end() ? &it->second : nullptr;
    }
    
    // Clear cache
    void clearCache() {
        skinDataCache_.clear();
    }
    
    void removeSkinData(const std::string& clothingId) {
        skinDataCache_.erase(clothingId);
    }
    
private:
    ClothingSkinningManager() = default;
    
    std::unordered_map<std::string, ClothingSkinData> skinDataCache_;
};

// Convenience function
inline ClothingSkinningManager& getClothingSkinningManager() {
    return ClothingSkinningManager::getInstance();
}

}  // namespace luma
