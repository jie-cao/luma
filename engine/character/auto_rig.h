// Auto Rig System - Automatic skeleton binding and weight generation
// Generates skinning weights for meshes based on skeleton structure
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include "engine/renderer/mesh.h"
#include "engine/character/standard_rig.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace luma {

// ============================================================================
// Skinning Weight Data
// ============================================================================

struct SkinWeight {
    int boneIndex = -1;
    float weight = 0.0f;
    
    SkinWeight() = default;
    SkinWeight(int idx, float w) : boneIndex(idx), weight(w) {}
};

struct VertexSkinning {
    static constexpr int MAX_INFLUENCES = 4;
    
    std::array<SkinWeight, MAX_INFLUENCES> weights;
    int influenceCount = 0;
    
    void addWeight(int boneIndex, float weight) {
        if (weight < 0.001f) return;
        
        // Find slot or replace smallest
        int insertIdx = -1;
        float minWeight = weight;
        
        for (int i = 0; i < MAX_INFLUENCES; i++) {
            if (weights[i].boneIndex < 0) {
                insertIdx = i;
                break;
            }
            if (weights[i].weight < minWeight) {
                minWeight = weights[i].weight;
                insertIdx = i;
            }
        }
        
        if (insertIdx >= 0 && weight > minWeight) {
            weights[insertIdx] = SkinWeight(boneIndex, weight);
            influenceCount = std::min(influenceCount + 1, MAX_INFLUENCES);
        }
    }
    
    void normalize() {
        float total = 0.0f;
        for (int i = 0; i < influenceCount; i++) {
            total += weights[i].weight;
        }
        
        if (total > 0.001f) {
            for (int i = 0; i < influenceCount; i++) {
                weights[i].weight /= total;
            }
        } else if (influenceCount > 0) {
            weights[0].weight = 1.0f;
        }
    }
    
    void clear() {
        for (auto& w : weights) {
            w.boneIndex = -1;
            w.weight = 0.0f;
        }
        influenceCount = 0;
    }
};

// ============================================================================
// Complete Skin Data for a Mesh
// ============================================================================

struct MeshSkinData {
    std::vector<VertexSkinning> vertexWeights;
    std::unordered_map<std::string, int> boneNameToIndex;
    std::vector<Mat4> inverseBindMatrices;
    
    bool isValid() const {
        return !vertexWeights.empty() && !inverseBindMatrices.empty();
    }
    
    // Apply to mesh vertex data (for export)
    void applyToMesh(Mesh& mesh) const {
        for (size_t i = 0; i < vertexWeights.size() && i < mesh.vertices.size(); i++) {
            const auto& skin = vertexWeights[i];
            
            // Store in vertex bone indices/weights
            for (int j = 0; j < 4; j++) {
                if (j < skin.influenceCount) {
                    mesh.vertices[i].boneIndices[j] = skin.weights[j].boneIndex;
                    mesh.vertices[i].boneWeights[j] = skin.weights[j].weight;
                } else {
                    mesh.vertices[i].boneIndices[j] = 0;
                    mesh.vertices[i].boneWeights[j] = 0.0f;
                }
            }
        }
    }
};

// ============================================================================
// Auto Rig Parameters
// ============================================================================

struct AutoRigParams {
    // Weight generation method
    enum class WeightMethod {
        DistanceBased,      // Simple distance falloff
        HeatDiffusion,      // More accurate, slower
        Geodesic            // Most accurate, slowest
    };
    
    WeightMethod method = WeightMethod::DistanceBased;
    
    // Distance-based parameters
    float falloffDistance = 0.3f;   // Max distance for influence
    float falloffPower = 2.0f;      // Falloff curve (1=linear, 2=quadratic)
    
    // Smoothing
    int smoothIterations = 3;
    float smoothStrength = 0.5f;
    
    // Constraints
    int maxBonesPerVertex = 4;
    float minWeight = 0.01f;        // Weights below this are discarded
    
    // Body region hints
    bool useBodyRegions = true;     // Use anatomical hints for better weighting
};

// ============================================================================
// Bone Capsule - Simplified bone geometry for weight calculation
// ============================================================================

struct BoneCapsule {
    int boneIndex = -1;
    std::string boneName;
    Vec3 start;                     // Start position (bone position)
    Vec3 end;                       // End position (child bone position or estimated)
    float radius = 0.05f;           // Influence radius
    
    // Calculate distance from point to capsule
    float distanceToPoint(const Vec3& point) const {
        Vec3 ab = end - start;
        float t = std::clamp(ab.dot(point - start) / ab.dot(ab), 0.0f, 1.0f);
        Vec3 closest = start + ab * t;
        return (point - closest).length();
    }
    
    // Calculate weight based on distance
    float calculateWeight(const Vec3& point, float falloffPower) const {
        float dist = distanceToPoint(point);
        if (dist >= radius * 2.0f) return 0.0f;
        
        float normalizedDist = dist / (radius * 2.0f);
        return std::pow(1.0f - normalizedDist, falloffPower);
    }
};

// ============================================================================
// Auto Rig Generator
// ============================================================================

class AutoRigGenerator {
public:
    AutoRigGenerator() = default;
    
    // === Main Generation Functions ===
    
    // Generate skin weights for a mesh based on skeleton
    static MeshSkinData generateWeights(
        const Mesh& mesh,
        const Skeleton& skeleton,
        const AutoRigParams& params = {})
    {
        MeshSkinData skinData;
        
        // Build bone capsules for distance calculation
        std::vector<BoneCapsule> capsules = buildBoneCapsules(skeleton, params);
        
        // Calculate inverse bind matrices
        skinData.inverseBindMatrices.resize(skeleton.getBoneCount());
        std::vector<Mat4> worldMatrices(skeleton.getBoneCount());
        computeWorldMatrices(skeleton, worldMatrices.data());
        
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            skinData.inverseBindMatrices[i] = worldMatrices[i].inverse();
            skinData.boneNameToIndex[skeleton.getBoneName(i)] = i;
        }
        
        // Generate weights for each vertex
        skinData.vertexWeights.resize(mesh.vertices.size());
        
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            Vec3 vertexPos(
                mesh.vertices[i].position[0],
                mesh.vertices[i].position[1],
                mesh.vertices[i].position[2]
            );
            
            skinData.vertexWeights[i] = calculateVertexWeights(
                vertexPos, capsules, params);
        }
        
        // Smooth weights
        if (params.smoothIterations > 0) {
            smoothWeights(skinData, mesh, params);
        }
        
        return skinData;
    }
    
    // Generate a complete standard humanoid rig for a mesh
    static MeshSkinData generateHumanoidRig(
        Mesh& mesh,
        float characterHeight,
        const AutoRigParams& params = {})
    {
        // Create standard humanoid skeleton
        HumanoidRigParams rigParams;
        rigParams.height = characterHeight;
        rigParams.includeFingers = true;
        rigParams.includeToes = true;
        rigParams.includeFaceBones = true;
        
        Skeleton skeleton = StandardHumanoidRig::createSkeleton(rigParams);
        
        // Generate weights
        MeshSkinData skinData = generateWeights(mesh, skeleton, params);
        
        // Apply weights to mesh
        skinData.applyToMesh(mesh);
        
        return skinData;
    }
    
    // === Utility Functions ===
    
    // Estimate bone radii based on mesh
    static void estimateBoneRadii(
        std::vector<BoneCapsule>& capsules,
        const Mesh& mesh)
    {
        for (auto& capsule : capsules) {
            // Find vertices closest to this bone
            std::vector<float> distances;
            
            for (const auto& vertex : mesh.vertices) {
                Vec3 pos(vertex.position[0], vertex.position[1], vertex.position[2]);
                float dist = capsule.distanceToPoint(pos);
                distances.push_back(dist);
            }
            
            // Sort and take percentile for radius estimation
            std::sort(distances.begin(), distances.end());
            size_t idx = std::min(
                static_cast<size_t>(distances.size() * 0.2f),
                distances.size() - 1);
            
            capsule.radius = std::max(0.01f, distances[idx]);
        }
    }
    
    // Validate that all vertices have valid weights
    static bool validateWeights(const MeshSkinData& skinData) {
        for (const auto& vw : skinData.vertexWeights) {
            if (vw.influenceCount == 0) {
                return false;  // Unweighted vertex
            }
            
            float total = 0.0f;
            for (int i = 0; i < vw.influenceCount; i++) {
                total += vw.weights[i].weight;
            }
            
            if (std::abs(total - 1.0f) > 0.01f) {
                return false;  // Unnormalized
            }
        }
        return true;
    }
    
private:
    // Build capsule representations for all bones
    static std::vector<BoneCapsule> buildBoneCapsules(
        const Skeleton& skeleton,
        const AutoRigParams& params)
    {
        std::vector<BoneCapsule> capsules;
        
        // Compute world-space bone positions
        std::vector<Mat4> worldMatrices(skeleton.getBoneCount());
        computeWorldMatrices(skeleton, worldMatrices.data());
        
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* bone = skeleton.getBone(i);
            if (!bone) continue;
            
            BoneCapsule capsule;
            capsule.boneIndex = i;
            capsule.boneName = bone->name;
            
            // Start at bone position
            capsule.start = Vec3(
                worldMatrices[i].m[12],
                worldMatrices[i].m[13],
                worldMatrices[i].m[14]
            );
            
            // End at first child bone, or estimate
            bool foundChild = false;
            for (int j = 0; j < skeleton.getBoneCount(); j++) {
                const Bone* child = skeleton.getBone(j);
                if (child && child->parentIndex == i) {
                    capsule.end = Vec3(
                        worldMatrices[j].m[12],
                        worldMatrices[j].m[13],
                        worldMatrices[j].m[14]
                    );
                    foundChild = true;
                    break;
                }
            }
            
            if (!foundChild) {
                // Estimate end position
                capsule.end = capsule.start + Vec3(0, -params.falloffDistance * 0.5f, 0);
            }
            
            // Set radius based on bone type
            capsule.radius = estimateBoneRadius(bone->name, params.falloffDistance);
            
            capsules.push_back(capsule);
        }
        
        return capsules;
    }
    
    // Estimate bone radius based on name
    static float estimateBoneRadius(const std::string& boneName, float defaultRadius) {
        // Larger for torso bones
        if (boneName.find("spine") != std::string::npos ||
            boneName.find("chest") != std::string::npos ||
            boneName.find("hips") != std::string::npos) {
            return defaultRadius * 1.5f;
        }
        
        // Smaller for fingers
        if (boneName.find("thumb") != std::string::npos ||
            boneName.find("index") != std::string::npos ||
            boneName.find("middle") != std::string::npos ||
            boneName.find("ring") != std::string::npos ||
            boneName.find("pinky") != std::string::npos) {
            return defaultRadius * 0.3f;
        }
        
        // Medium for limbs
        if (boneName.find("arm") != std::string::npos ||
            boneName.find("leg") != std::string::npos) {
            return defaultRadius * 0.8f;
        }
        
        // Head
        if (boneName == "head") {
            return defaultRadius * 1.2f;
        }
        
        return defaultRadius;
    }
    
    // Compute world matrices for all bones
    static void computeWorldMatrices(const Skeleton& skeleton, Mat4* outMatrices) {
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* bone = skeleton.getBone(i);
            if (!bone) {
                outMatrices[i] = Mat4::identity();
                continue;
            }
            
            Mat4 localMatrix = bone->getLocalMatrix();
            
            if (bone->parentIndex >= 0 && bone->parentIndex < i) {
                outMatrices[i] = outMatrices[bone->parentIndex] * localMatrix;
            } else {
                outMatrices[i] = localMatrix;
            }
        }
    }
    
    // Calculate weights for a single vertex
    static VertexSkinning calculateVertexWeights(
        const Vec3& position,
        const std::vector<BoneCapsule>& capsules,
        const AutoRigParams& params)
    {
        VertexSkinning skinning;
        
        // Calculate weight contribution from each bone
        std::vector<std::pair<int, float>> boneWeights;
        
        for (const auto& capsule : capsules) {
            float weight = capsule.calculateWeight(position, params.falloffPower);
            if (weight > params.minWeight) {
                boneWeights.push_back({capsule.boneIndex, weight});
            }
        }
        
        // Sort by weight (highest first)
        std::sort(boneWeights.begin(), boneWeights.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Take top N weights
        int count = std::min(static_cast<int>(boneWeights.size()), params.maxBonesPerVertex);
        for (int i = 0; i < count; i++) {
            skinning.addWeight(boneWeights[i].first, boneWeights[i].second);
        }
        
        // Normalize
        skinning.normalize();
        
        // Fallback: assign to nearest bone if no weights
        if (skinning.influenceCount == 0 && !capsules.empty()) {
            float minDist = FLT_MAX;
            int nearestBone = 0;
            
            for (const auto& capsule : capsules) {
                float dist = capsule.distanceToPoint(position);
                if (dist < minDist) {
                    minDist = dist;
                    nearestBone = capsule.boneIndex;
                }
            }
            
            skinning.addWeight(nearestBone, 1.0f);
        }
        
        return skinning;
    }
    
    // Smooth weights across mesh connectivity
    static void smoothWeights(
        MeshSkinData& skinData,
        const Mesh& mesh,
        const AutoRigParams& params)
    {
        // Build adjacency list from triangles
        std::vector<std::vector<size_t>> adjacency(mesh.vertices.size());
        
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            uint32_t a = mesh.indices[i];
            uint32_t b = mesh.indices[i + 1];
            uint32_t c = mesh.indices[i + 2];
            
            adjacency[a].push_back(b);
            adjacency[a].push_back(c);
            adjacency[b].push_back(a);
            adjacency[b].push_back(c);
            adjacency[c].push_back(a);
            adjacency[c].push_back(b);
        }
        
        // Remove duplicates in adjacency
        for (auto& adj : adjacency) {
            std::sort(adj.begin(), adj.end());
            adj.erase(std::unique(adj.begin(), adj.end()), adj.end());
        }
        
        // Smooth iterations
        for (int iter = 0; iter < params.smoothIterations; iter++) {
            std::vector<VertexSkinning> smoothed = skinData.vertexWeights;
            
            for (size_t i = 0; i < mesh.vertices.size(); i++) {
                if (adjacency[i].empty()) continue;
                
                // Accumulate neighbor weights
                std::unordered_map<int, float> accumulated;
                
                // Add own weight
                float ownFactor = 1.0f - params.smoothStrength;
                for (int j = 0; j < skinData.vertexWeights[i].influenceCount; j++) {
                    accumulated[skinData.vertexWeights[i].weights[j].boneIndex] +=
                        skinData.vertexWeights[i].weights[j].weight * ownFactor;
                }
                
                // Add neighbor weights
                float neighborFactor = params.smoothStrength / adjacency[i].size();
                for (size_t ni : adjacency[i]) {
                    for (int j = 0; j < skinData.vertexWeights[ni].influenceCount; j++) {
                        accumulated[skinData.vertexWeights[ni].weights[j].boneIndex] +=
                            skinData.vertexWeights[ni].weights[j].weight * neighborFactor;
                    }
                }
                
                // Rebuild vertex skinning from accumulated
                smoothed[i].clear();
                
                std::vector<std::pair<int, float>> sorted(accumulated.begin(), accumulated.end());
                std::sort(sorted.begin(), sorted.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                
                for (int j = 0; j < std::min(params.maxBonesPerVertex, (int)sorted.size()); j++) {
                    smoothed[i].addWeight(sorted[j].first, sorted[j].second);
                }
                smoothed[i].normalize();
            }
            
            skinData.vertexWeights = std::move(smoothed);
        }
    }
};

// ============================================================================
// Rig Exporter - Export rigged mesh to standard formats
// ============================================================================

class RigExporter {
public:
    // Export bone weights for Unity/Blender import
    static std::string exportToJson(
        const MeshSkinData& skinData,
        const Skeleton& skeleton)
    {
        std::string json = "{\n";
        
        // Bones
        json += "  \"bones\": [\n";
        for (int i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* bone = skeleton.getBone(i);
            if (!bone) continue;
            
            json += "    {";
            json += "\"name\": \"" + bone->name + "\", ";
            json += "\"parent\": " + std::to_string(bone->parentIndex) + ", ";
            json += "\"position\": [" + 
                std::to_string(bone->localPosition.x) + ", " +
                std::to_string(bone->localPosition.y) + ", " +
                std::to_string(bone->localPosition.z) + "]";
            json += "}";
            if (i < skeleton.getBoneCount() - 1) json += ",";
            json += "\n";
        }
        json += "  ],\n";
        
        // Weights (sparse format)
        json += "  \"vertexWeights\": [\n";
        for (size_t i = 0; i < skinData.vertexWeights.size(); i++) {
            const auto& vw = skinData.vertexWeights[i];
            
            json += "    {\"v\": " + std::to_string(i) + ", \"w\": [";
            for (int j = 0; j < vw.influenceCount; j++) {
                json += "[" + std::to_string(vw.weights[j].boneIndex) + ", " +
                    std::to_string(vw.weights[j].weight) + "]";
                if (j < vw.influenceCount - 1) json += ", ";
            }
            json += "]}";
            if (i < skinData.vertexWeights.size() - 1) json += ",";
            json += "\n";
        }
        json += "  ]\n";
        
        json += "}\n";
        return json;
    }
    
    // Get rig compatibility info for different standards
    static std::string getCompatibilityReport(
        const Skeleton& skeleton,
        RigStandard targetStandard)
    {
        auto result = RigValidator::validate(skeleton, targetStandard);
        return result.getSummary();
    }
};

// ============================================================================
// Character Rig Manager - High-level rig management
// ============================================================================

class CharacterRigManager {
public:
    CharacterRigManager() = default;
    
    // Create complete rigged character
    struct RigResult {
        Skeleton skeleton;
        MeshSkinData skinData;
        bool success = false;
        std::string errorMessage;
    };
    
    static RigResult createRiggedCharacter(
        Mesh& mesh,
        float height = 1.8f,
        RigStandard standard = RigStandard::Luma)
    {
        RigResult result;
        
        // Create skeleton
        HumanoidRigParams params;
        params.height = height;
        params.includeFingers = true;
        params.includeToes = true;
        params.includeFaceBones = true;
        
        result.skeleton = StandardHumanoidRig::createSkeleton(params);
        
        // Convert to target standard if needed
        if (standard != RigStandard::Luma) {
            result.skeleton = SkeletonConverter::convertToStandard(result.skeleton, standard);
        }
        
        // Generate weights
        AutoRigParams rigParams;
        rigParams.falloffDistance = height * 0.15f;
        rigParams.smoothIterations = 3;
        
        result.skinData = AutoRigGenerator::generateWeights(mesh, result.skeleton, rigParams);
        
        // Apply to mesh
        result.skinData.applyToMesh(mesh);
        
        // Validate
        result.success = AutoRigGenerator::validateWeights(result.skinData);
        if (!result.success) {
            result.errorMessage = "Some vertices have invalid weights";
        }
        
        return result;
    }
    
    // Apply existing rig from another mesh/skeleton
    static bool applyRigFromReference(
        Mesh& targetMesh,
        const Mesh& referenceMesh,
        const Skeleton& referenceSkeleton,
        const MeshSkinData& referenceSkinData)
    {
        // Transfer weights by nearest vertex
        MeshSkinData newSkinData;
        newSkinData.inverseBindMatrices = referenceSkinData.inverseBindMatrices;
        newSkinData.boneNameToIndex = referenceSkinData.boneNameToIndex;
        newSkinData.vertexWeights.resize(targetMesh.vertices.size());
        
        for (size_t i = 0; i < targetMesh.vertices.size(); i++) {
            Vec3 targetPos(
                targetMesh.vertices[i].position[0],
                targetMesh.vertices[i].position[1],
                targetMesh.vertices[i].position[2]
            );
            
            // Find nearest reference vertex
            float minDist = FLT_MAX;
            size_t nearestIdx = 0;
            
            for (size_t j = 0; j < referenceMesh.vertices.size(); j++) {
                Vec3 refPos(
                    referenceMesh.vertices[j].position[0],
                    referenceMesh.vertices[j].position[1],
                    referenceMesh.vertices[j].position[2]
                );
                
                float dist = (targetPos - refPos).length();
                if (dist < minDist) {
                    minDist = dist;
                    nearestIdx = j;
                }
            }
            
            // Copy weights from nearest
            newSkinData.vertexWeights[i] = referenceSkinData.vertexWeights[nearestIdx];
        }
        
        newSkinData.applyToMesh(targetMesh);
        return true;
    }
};

}  // namespace luma
