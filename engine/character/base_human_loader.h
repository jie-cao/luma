// Base Human Model Loader - Load and manage base human models with BlendShapes
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character.h"
#include "engine/character/face_template_mesh.h"
#include "engine/character/head_mesh_loader.h"
#include "engine/character/bfm_loader.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cmath>

namespace luma {

// ============================================================================
// Base Human Model Data
// ============================================================================

struct BaseHumanModel {
    std::string name;
    std::string source;                  // "makehuman", "custom", "generated"
    
    // Mesh data
    std::vector<Vertex> vertices;
    std::vector<SkinnedVertex> skinnedVertices;
    std::vector<uint32_t> indices;
    
    // BlendShape data
    BlendShapeMesh blendShapes;
    
    // Skeleton
    Skeleton skeleton;
    
    // Texture paths
    std::string diffuseTexturePath;
    std::string normalTexturePath;
    std::string specularTexturePath;
    
    // Model info
    int vertexCount = 0;
    int triangleCount = 0;
    int blendShapeCount = 0;
    int boneCount = 0;
    
    // Bounds
    Vec3 boundsMin;
    Vec3 boundsMax;
    Vec3 center;
    float radius = 1.0f;
    
    bool isValid() const {
        return !vertices.empty() && !indices.empty();
    }
};

// ============================================================================
// MakeHuman Target File Parser
// ============================================================================

// MakeHuman stores morph targets in .target files
// Format: vertex_index dx dy dz (one per line)
struct MakeHumanTarget {
    std::string name;
    std::vector<BlendShapeDelta> deltas;
    
    bool loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        // Extract name from filename
        size_t lastSlash = path.find_last_of("/\\");
        size_t lastDot = path.find_last_of('.');
        name = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            uint32_t vertexIndex;
            float dx, dy, dz;
            
            if (iss >> vertexIndex >> dx >> dy >> dz) {
                BlendShapeDelta delta;
                delta.vertexIndex = vertexIndex;
                delta.positionDelta = Vec3(dx, dy, dz);
                // Normal deltas would need to be computed
                deltas.push_back(delta);
            }
        }
        
        return !deltas.empty();
    }
};

// ============================================================================
// Base Human Loader
// ============================================================================

class BaseHumanLoader {
public:
    // Load from various sources
    
    // Load MakeHuman exported model (OBJ + targets)
    static bool loadMakeHuman(const std::string& objPath, 
                               const std::string& targetDir,
                               BaseHumanModel& outModel) {
        // Load base OBJ mesh
        if (!loadOBJ(objPath, outModel)) {
            return false;
        }
        
        outModel.source = "makehuman";
        
        // Load all .target files from directory
        // In a real implementation, you'd list the directory
        // For now, we'll load predefined targets
        
        std::vector<std::string> standardTargets = {
            // Body shape
            "body_height_increase", "body_height_decrease",
            "body_weight_increase", "body_weight_decrease",
            "body_muscle_increase", "body_muscle_decrease",
            "body_fat_increase", "body_fat_decrease",
            
            // Torso
            "torso_shoulder_width_increase", "torso_shoulder_width_decrease",
            "torso_chest_increase", "torso_chest_decrease",
            "torso_waist_increase", "torso_waist_decrease",
            "torso_hip_increase", "torso_hip_decrease",
            
            // Face shape
            "face_width_increase", "face_width_decrease",
            "face_length_increase", "face_length_decrease",
            
            // Eyes
            "eyes_size_increase", "eyes_size_decrease",
            "eyes_spacing_increase", "eyes_spacing_decrease",
            "eyes_height_increase", "eyes_height_decrease",
            
            // Nose
            "nose_length_increase", "nose_length_decrease",
            "nose_width_increase", "nose_width_decrease",
            "nose_height_increase", "nose_height_decrease",
            
            // Mouth
            "mouth_width_increase", "mouth_width_decrease",
            "lips_thickness_increase", "lips_thickness_decrease",
            
            // Chin/Jaw
            "chin_length_increase", "chin_length_decrease",
            "jaw_width_increase", "jaw_width_decrease"
        };
        
        for (const auto& targetName : standardTargets) {
            std::string targetPath = targetDir + "/" + targetName + ".target";
            MakeHumanTarget target;
            if (target.loadFromFile(targetPath)) {
                BlendShapeTarget bsTarget(target.name);
                for (const auto& delta : target.deltas) {
                    bsTarget.addDelta(delta);
                }
                outModel.blendShapes.addTarget(bsTarget);
            }
        }
        
        // Create channels from targets
        outModel.blendShapes.createChannelsFromTargets();
        
        outModel.blendShapeCount = static_cast<int>(outModel.blendShapes.getTargetCount());
        
        return true;
    }
    
    // Load generic FBX/glTF with BlendShapes
    static bool loadWithBlendShapes(const std::string& path, BaseHumanModel& outModel) {
        // This would use Assimp to load models with morph targets
        // For now, placeholder
        (void)path;
        (void)outModel;
        return false;
    }
    
    // Load simple OBJ file
    static bool loadOBJ(const std::string& path, BaseHumanModel& outModel) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        std::vector<Vec3> positions;
        std::vector<Vec3> normals;
        std::vector<Vec2> uvs;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            
            if (prefix == "v") {
                Vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (prefix == "vn") {
                Vec3 nor;
                iss >> nor.x >> nor.y >> nor.z;
                normals.push_back(nor);
            }
            else if (prefix == "vt") {
                Vec2 uv;
                iss >> uv.x >> uv.y;
                uvs.push_back(uv);
            }
            else if (prefix == "f") {
                // Parse face (handles v, v/t, v/t/n, v//n formats)
                std::string vertexStr;
                std::vector<std::array<int, 3>> faceIndices;  // pos, uv, normal
                
                while (iss >> vertexStr) {
                    std::array<int, 3> indices = {-1, -1, -1};
                    
                    size_t slash1 = vertexStr.find('/');
                    if (slash1 == std::string::npos) {
                        indices[0] = std::stoi(vertexStr) - 1;
                    } else {
                        indices[0] = std::stoi(vertexStr.substr(0, slash1)) - 1;
                        size_t slash2 = vertexStr.find('/', slash1 + 1);
                        if (slash2 == std::string::npos) {
                            if (slash1 + 1 < vertexStr.size()) {
                                indices[1] = std::stoi(vertexStr.substr(slash1 + 1)) - 1;
                            }
                        } else {
                            if (slash2 > slash1 + 1) {
                                indices[1] = std::stoi(vertexStr.substr(slash1 + 1, slash2 - slash1 - 1)) - 1;
                            }
                            if (slash2 + 1 < vertexStr.size()) {
                                indices[2] = std::stoi(vertexStr.substr(slash2 + 1)) - 1;
                            }
                        }
                    }
                    faceIndices.push_back(indices);
                }
                
                // Triangulate (fan triangulation for convex polygons)
                for (size_t i = 1; i + 1 < faceIndices.size(); i++) {
                    for (int j : {0, (int)i, (int)i + 1}) {
                        auto& idx = faceIndices[j];
                        
                        Vertex v;
                        if (idx[0] >= 0 && idx[0] < (int)positions.size()) {
                            v.position[0] = positions[idx[0]].x;
                            v.position[1] = positions[idx[0]].y;
                            v.position[2] = positions[idx[0]].z;
                        }
                        if (idx[1] >= 0 && idx[1] < (int)uvs.size()) {
                            v.uv[0] = uvs[idx[1]].x;
                            v.uv[1] = uvs[idx[1]].y;
                        }
                        if (idx[2] >= 0 && idx[2] < (int)normals.size()) {
                            v.normal[0] = normals[idx[2]].x;
                            v.normal[1] = normals[idx[2]].y;
                            v.normal[2] = normals[idx[2]].z;
                        }
                        v.color[0] = v.color[1] = v.color[2] = 1.0f;
                        
                        outModel.indices.push_back(static_cast<uint32_t>(outModel.vertices.size()));
                        outModel.vertices.push_back(v);
                    }
                }
            }
        }
        
        // Calculate bounds
        outModel.boundsMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        outModel.boundsMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        for (const auto& v : outModel.vertices) {
            outModel.boundsMin.x = std::min(outModel.boundsMin.x, v.position[0]);
            outModel.boundsMin.y = std::min(outModel.boundsMin.y, v.position[1]);
            outModel.boundsMin.z = std::min(outModel.boundsMin.z, v.position[2]);
            outModel.boundsMax.x = std::max(outModel.boundsMax.x, v.position[0]);
            outModel.boundsMax.y = std::max(outModel.boundsMax.y, v.position[1]);
            outModel.boundsMax.z = std::max(outModel.boundsMax.z, v.position[2]);
        }
        
        outModel.center = Vec3(
            (outModel.boundsMin.x + outModel.boundsMax.x) * 0.5f,
            (outModel.boundsMin.y + outModel.boundsMax.y) * 0.5f,
            (outModel.boundsMin.z + outModel.boundsMax.z) * 0.5f
        );
        
        Vec3 extent(
            outModel.boundsMax.x - outModel.boundsMin.x,
            outModel.boundsMax.y - outModel.boundsMin.y,
            outModel.boundsMax.z - outModel.boundsMin.z
        );
        outModel.radius = extent.length() * 0.5f;
        
        outModel.vertexCount = static_cast<int>(outModel.vertices.size());
        outModel.triangleCount = static_cast<int>(outModel.indices.size() / 3);
        
        // Extract name from path
        size_t lastSlash = path.find_last_of("/\\");
        size_t lastDot = path.find_last_of('.');
        outModel.name = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
        
        return !outModel.vertices.empty();
    }
    
    // Initialize skeleton for MakeHuman model
    static void initializeMakeHumanSkeleton(Skeleton& skeleton) {
        // MakeHuman default skeleton structure
        int root = skeleton.addBone("Root", -1);
        int hips = skeleton.addBone("Hips", root);
        
        // Spine
        int spine = skeleton.addBone("Spine", hips);
        int spine1 = skeleton.addBone("Spine1", spine);
        int spine2 = skeleton.addBone("Spine2", spine1);
        int spine3 = skeleton.addBone("Spine3", spine2);
        
        // Neck and head
        int neck = skeleton.addBone("Neck", spine3);
        int neck1 = skeleton.addBone("Neck1", neck);
        int head = skeleton.addBone("Head", neck1);
        
        // Left arm
        int lClavicle = skeleton.addBone("LeftClavicle", spine3);
        int lShoulder = skeleton.addBone("LeftShoulder", lClavicle);
        int lElbow = skeleton.addBone("LeftElbow", lShoulder);
        int lWrist = skeleton.addBone("LeftWrist", lElbow);
        
        // Left hand
        int lHand = skeleton.addBone("LeftHand", lWrist);
        skeleton.addBone("LeftThumb1", lHand);
        skeleton.addBone("LeftIndex1", lHand);
        skeleton.addBone("LeftMiddle1", lHand);
        skeleton.addBone("LeftRing1", lHand);
        skeleton.addBone("LeftPinky1", lHand);
        
        // Right arm
        int rClavicle = skeleton.addBone("RightClavicle", spine3);
        int rShoulder = skeleton.addBone("RightShoulder", rClavicle);
        int rElbow = skeleton.addBone("RightElbow", rShoulder);
        int rWrist = skeleton.addBone("RightWrist", rElbow);
        
        // Right hand
        int rHand = skeleton.addBone("RightHand", rWrist);
        skeleton.addBone("RightThumb1", rHand);
        skeleton.addBone("RightIndex1", rHand);
        skeleton.addBone("RightMiddle1", rHand);
        skeleton.addBone("RightRing1", rHand);
        skeleton.addBone("RightPinky1", rHand);
        
        // Left leg
        int lHip = skeleton.addBone("LeftHip", hips);
        int lKnee = skeleton.addBone("LeftKnee", lHip);
        int lAnkle = skeleton.addBone("LeftAnkle", lKnee);
        int lFoot = skeleton.addBone("LeftFoot", lAnkle);
        skeleton.addBone("LeftToe", lFoot);
        
        // Right leg
        int rHip = skeleton.addBone("RightHip", hips);
        int rKnee = skeleton.addBone("RightKnee", rHip);
        int rAnkle = skeleton.addBone("RightAnkle", rKnee);
        int rFoot = skeleton.addBone("RightFoot", rAnkle);
        skeleton.addBone("RightToe", rFoot);
        
        // Face bones (simplified)
        skeleton.addBone("Jaw", head);
        skeleton.addBone("LeftEye", head);
        skeleton.addBone("RightEye", head);
        
        // Suppress unused warnings
        (void)neck1;
    }
};

// ============================================================================
// Procedural Human Model Generator (for testing without external assets)
// ============================================================================

struct ProceduralHumanParams {
    int bodySubdivisions = 8;       // Subdivisions around body
    int heightSegments = 20;        // Vertical segments
    float height = 1.8f;            // Total height in meters
    bool generateBlendShapes = true;
    bool generateSkeleton = true;
};

class ProceduralHumanGenerator {
public:
    using GeneratorParams = ProceduralHumanParams;
    
    // Generate a parametric human model with high-quality face
    // MetaHuman style architecture:
    // 1. First try to load standard topology head mesh (head_base.bin)
    // 2. If not available, fall back to procedural generation
    // Face only first, body added later
    static BaseHumanModel generate(const GeneratorParams& params = GeneratorParams(),
                                    const std::string& modelDir = "",
                                    bool faceOnly = true) {
        BaseHumanModel model;
        model.name = "ProceduralHuman";
        model.source = "generated";

        float H = params.height;
        float headHeight = 0.23f;  // Standard head height in meters

        int bodyVertCount = 0;

        // Only generate body if requested (MetaHuman style: face first)
        if (!faceOnly) {
            generateBody(model, params);
            bodyVertCount = (int)model.vertices.size();
        }

        // Try to load standard topology head mesh first (MetaHuman style)
        bool usedStandardHead = false;
        std::string headMeshPath;
        if (!modelDir.empty()) {
            headMeshPath = modelDir + "/head_base.bin";
            usedStandardHead = generateHeadFromStandardMesh(model, headMeshPath, headHeight, bodyVertCount);
        }

        // If standard head not available, use FaceTemplateMesh (which has its own fallback)
        if (!usedStandardHead) {
            printf("[ProceduralHuman] Standard head mesh not found, using FaceTemplateMesh\n");
            
            // FaceTemplateMesh will try to load head_base.bin itself, or fall back to procedural
            auto faceMesh = FaceTemplateMesh::generate(headMeshPath, headHeight);

            for (const auto& fv : faceMesh.vertices) {
                model.vertices.push_back(fv);
            }

            for (uint32_t idx : faceMesh.indices) {
                model.indices.push_back(idx + bodyVertCount);
            }

            // Copy face blend shapes
            size_t faceTargets = faceMesh.blendShapes.getTargetCount();
            for (size_t ti = 0; ti < faceTargets; ti++) {
                const BlendShapeTarget* srcTarget = faceMesh.blendShapes.getTarget((int)ti);
                if (!srcTarget) continue;

                BlendShapeTarget newTarget(srcTarget->name);
                for (const auto& delta : srcTarget->deltas) {
                    BlendShapeDelta newDelta = delta;
                    newDelta.vertexIndex += bodyVertCount;
                    newTarget.addDelta(newDelta);
                }
                int newTargetIdx = model.blendShapes.addTarget(newTarget);
                model.blendShapes.createChannel(srcTarget->name, newTargetIdx);
            }
        }

        // Update model info
        model.vertexCount = (int)model.vertices.size();
        model.triangleCount = (int)model.indices.size() / 3;
        model.blendShapeCount = model.blendShapes.getChannelCount();

        // Calculate bounds
        model.boundsMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        model.boundsMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (const auto& v : model.vertices) {
            model.boundsMin.x = std::min(model.boundsMin.x, v.position[0]);
            model.boundsMin.y = std::min(model.boundsMin.y, v.position[1]);
            model.boundsMin.z = std::min(model.boundsMin.z, v.position[2]);
            model.boundsMax.x = std::max(model.boundsMax.x, v.position[0]);
            model.boundsMax.y = std::max(model.boundsMax.y, v.position[1]);
            model.boundsMax.z = std::max(model.boundsMax.z, v.position[2]);
        }
        model.center = Vec3(
            (model.boundsMin.x + model.boundsMax.x) * 0.5f,
            (model.boundsMin.y + model.boundsMax.y) * 0.5f,
            (model.boundsMin.z + model.boundsMax.z) * 0.5f
        );
        Vec3 extent(
            model.boundsMax.x - model.boundsMin.x,
            model.boundsMax.y - model.boundsMin.y,
            model.boundsMax.z - model.boundsMin.z
        );
        model.radius = extent.length() * 0.5f;

        // Generate skeleton (minimal for face-only)
        if (params.generateSkeleton) {
            if (faceOnly) {
                // Just head bone for face-only mode
                model.skeleton.addBone("Root", -1);
                model.skeleton.addBone("Head", 0);
            } else {
                BaseHumanLoader::initializeMakeHumanSkeleton(model.skeleton);
            }
            model.boneCount = model.skeleton.getBoneCount();
        }

        printf("[ProceduralHuman] Generated: %d verts, %d triangles, %d blend shapes (standardHead: %s, faceOnly: %s)\n",
               model.vertexCount, model.triangleCount, model.blendShapeCount,
               usedStandardHead ? "yes" : "no", faceOnly ? "yes" : "no");

        return model;
    }

    // Generate head from standard topology mesh (MetaHuman style)
    static bool generateHeadFromStandardMesh(BaseHumanModel& model, const std::string& headMeshPath,
                                              float headHeight, int bodyVertCount) {
        // Use FaceTemplateMesh to load and generate BlendShapes for standard head
        auto headMesh = FaceTemplateMesh::generate(headMeshPath, headHeight);
        
        // Check if it actually loaded the standard mesh (not fallback)
        // We can tell by checking if neckBoundaryCount > 0 (standard mesh has neck boundary)
        if (headMesh.neckBoundaryCount == 0) {
            // This was a fallback to procedural, not the standard mesh
            return false;
        }
        
        printf("[ProceduralHuman] Using standard topology head mesh\n");
        
        // Add vertices
        for (const auto& v : headMesh.vertices) {
            model.vertices.push_back(v);
        }

        // Add indices with offset
        for (uint32_t idx : headMesh.indices) {
            model.indices.push_back(idx + bodyVertCount);
        }

        // Copy blend shapes
        size_t numTargets = headMesh.blendShapes.getTargetCount();
        for (size_t ti = 0; ti < numTargets; ti++) {
            const BlendShapeTarget* srcTarget = headMesh.blendShapes.getTarget((int)ti);
            if (!srcTarget) continue;

            BlendShapeTarget newTarget(srcTarget->name);
            for (const auto& delta : srcTarget->deltas) {
                BlendShapeDelta newDelta = delta;
                newDelta.vertexIndex += bodyVertCount;
                newTarget.addDelta(newDelta);
            }
            int newTargetIdx = model.blendShapes.addTarget(newTarget);
            model.blendShapes.createChannel(srcTarget->name, newTargetIdx);
        }

        printf("[ProceduralHuman] Standard head: %zu verts, %zu tris, %zu blend shapes\n",
               headMesh.vertices.size(), headMesh.indices.size() / 3, numTargets);

        return true;
    }

    // Legacy: Generate complete head from BFM model (face + procedural skull)
    // Kept for backward compatibility but no longer the primary method
    static bool generateFaceFromBFM(BaseHumanModel& model, const std::string& modelDir,
                                     float headHeight, int bodyVertCount) {
        BFMLoader::BFMModel bfm;
        if (!BFMLoader::load(modelDir, bfm)) {
            printf("[ProceduralHuman] BFM model not found\n");
            return false;
        }

        // Use neutral coefficients (mean face)
        std::vector<float> shapeCoeffs(40, 0.0f);
        std::vector<float> expCoeffs(10, 0.0f);

        // Generate complete head mesh (BFM face + procedural skull)
        std::vector<Vertex> headVerts;
        std::vector<uint32_t> headIndices;
        BFMLoader::toMeshWithHead(bfm, shapeCoeffs, expCoeffs, headVerts, headIndices, headHeight);

        // Add vertices
        for (auto& v : headVerts) {
            model.vertices.push_back(v);
        }

        for (uint32_t idx : headIndices) {
            model.indices.push_back(idx + bodyVertCount);
        }

        printf("[ProceduralHuman] Using BFM head: %zu verts, %zu tris\n",
               headVerts.size(), headIndices.size() / 3);

        return true;
    }
    
private:
    struct BodyProfile {
        float height;       // Normalized height (0 = feet, 1 = top of head)
        float radiusX;      // Horizontal radius (side to side)
        float radiusZ;      // Depth radius (front to back)
        float offsetX;      // Horizontal offset
        float offsetZ;      // Depth offset
    };
    
    static float smoothstep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
    // Head coordinate helpers - features defined in head-local space (0=chin, 1=crown)
    // This ensures correct proportions regardless of body height mapping
    static constexpr float HEAD_BOTTOM = 0.86f;  // chin position (matches neck top profile)
    static constexpr float HEAD_TOP    = 1.0f;    // crown position
    static constexpr float HEAD_RANGE  = HEAD_TOP - HEAD_BOTTOM; // 0.14
    
    // Convert head-local Y (0=chin, 1=crown) to body-global normalized Y
    static float headY(float localY) { return HEAD_BOTTOM + localY * HEAD_RANGE; }
    
    // Facial landmark positions in head-local coordinates
    static constexpr float HL_CHIN        = 0.0f;
    static constexpr float HL_LOWER_LIP   = 0.08f;
    static constexpr float HL_MOUTH       = 0.12f;
    static constexpr float HL_UPPER_LIP   = 0.16f;
    static constexpr float HL_PHILTRUM    = 0.20f;
    static constexpr float HL_NOSE_BASE   = 0.25f;
    static constexpr float HL_NOSE_TIP    = 0.28f;
    static constexpr float HL_NOSE_BRIDGE = 0.33f;
    static constexpr float HL_EYE_LEVEL   = 0.40f;
    static constexpr float HL_BROW        = 0.50f;
    static constexpr float HL_FOREHEAD    = 0.62f;
    static constexpr float HL_CROWN       = 0.85f;
    
    // Compute facial feature offsets for head vertices
    static Vec3 getFacialOffset(float angle, float nY, float H) {
        // Convert global nY to head-local coordinate
        if (nY < HEAD_BOTTOM || nY > HEAD_TOP) return Vec3(0, 0, 0);
        float hLocal = (nY - HEAD_BOTTOM) / HEAD_RANGE; // 0=chin, 1=crown
        
        float sinA = std::sin(angle);
        float cosA = std::cos(angle);
        float absCos = std::abs(cosA);
        float front = std::max(0.0f, sinA);
        
        Vec3 off(0, 0, 0);
        if (front <= 0.01f && absCos < 0.92f) return off;
        
        // Feature scale = head height (about 23cm for 1.75m person)
        float headH = H * HEAD_RANGE;
        float S = headH; // features scale relative to head size
        
        // --- NOSE ---
        // Bridge
        {
            float hF = smoothstep(0.06f, 0.0f, std::abs(hLocal - HL_NOSE_BRIDGE));
            float aF = smoothstep(0.22f, 0.0f, absCos) * smoothstep(0.0f, 0.8f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += S * 0.08f * amt;
        }
        // Tip (strongest protrusion)
        {
            float hF = smoothstep(0.04f, 0.0f, std::abs(hLocal - HL_NOSE_TIP));
            float aF = smoothstep(0.25f, 0.0f, absCos) * smoothstep(0.0f, 0.7f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += S * 0.12f * amt;
        }
        // Nose base / nostrils (wider)
        {
            float hF = smoothstep(0.03f, 0.0f, std::abs(hLocal - HL_NOSE_BASE));
            float cosTarget = 0.2f;
            float aF = smoothstep(0.15f, 0.0f, std::abs(absCos - cosTarget)) *
                       smoothstep(0.0f, 0.5f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) {
                off.z += S * 0.05f * amt;
                off.x += (cosA > 0 ? 1.0f : -1.0f) * S * 0.02f * amt;
            }
        }
        
        // --- EYE SOCKETS (deep inward depression) ---
        {
            float hF = smoothstep(0.05f, 0.0f, std::abs(hLocal - HL_EYE_LEVEL));
            float eyeCosL = -0.40f;
            float eyeCosR = 0.40f;
            float eyeAngR = 0.22f;
            float sinReq = smoothstep(0.0f, 0.3f, sinA);
            
            float distL = std::abs(cosA - eyeCosL);
            float amtL = smoothstep(eyeAngR, 0.0f, distL) * hF * sinReq;
            if (amtL > 0.001f) {
                off.x += cosA * (-S * 0.06f * amtL);
                off.z += sinA * (-S * 0.06f * amtL);
            }
            
            float distR = std::abs(cosA - eyeCosR);
            float amtR = smoothstep(eyeAngR, 0.0f, distR) * hF * sinReq;
            if (amtR > 0.001f) {
                off.x += cosA * (-S * 0.06f * amtR);
                off.z += sinA * (-S * 0.06f * amtR);
            }
        }
        
        // --- BROW RIDGE (forward push above eyes) ---
        {
            float hF = smoothstep(0.04f, 0.0f, std::abs(hLocal - HL_BROW));
            float aF = smoothstep(0.0f, 0.4f, sinA) * smoothstep(0.72f, 0.0f, absCos);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += S * 0.03f * amt;
        }
        
        // --- CHEEKBONES (outward bumps) ---
        {
            float cheekLocal = (HL_EYE_LEVEL + HL_NOSE_BRIDGE) * 0.5f;
            float hF = smoothstep(0.06f, 0.0f, std::abs(hLocal - cheekLocal));
            float cosTarget = 0.58f;
            float aF = smoothstep(0.22f, 0.0f, std::abs(absCos - cosTarget)) *
                       smoothstep(0.0f, 0.2f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) {
                off.x += (cosA > 0 ? 1.0f : -1.0f) * S * 0.03f * amt;
                off.z += S * 0.015f * amt;
            }
        }
        
        // --- MOUTH GROOVE ---
        {
            float hF = smoothstep(0.025f, 0.0f, std::abs(hLocal - HL_MOUTH));
            float aF = smoothstep(0.4f, 0.0f, absCos) * smoothstep(0.0f, 0.5f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += -S * 0.025f * amt;
        }
        
        // --- UPPER LIP ---
        {
            float hF = smoothstep(0.025f, 0.0f, std::abs(hLocal - HL_UPPER_LIP));
            float aF = smoothstep(0.35f, 0.0f, absCos) * smoothstep(0.0f, 0.55f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += S * 0.02f * amt;
        }
        
        // --- LOWER LIP ---
        {
            float hF = smoothstep(0.025f, 0.0f, std::abs(hLocal - HL_LOWER_LIP));
            float aF = smoothstep(0.35f, 0.0f, absCos) * smoothstep(0.0f, 0.55f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += S * 0.018f * amt;
        }
        
        // --- CHIN ---
        {
            float hF = smoothstep(0.05f, 0.0f, std::abs(hLocal - HL_CHIN * 0.5f + 0.02f));
            float aF = smoothstep(0.3f, 0.0f, absCos) * smoothstep(0.0f, 0.4f, sinA);
            float amt = hF * aF;
            if (amt > 0.001f) off.z += S * 0.04f * amt;
        }
        
        // --- EARS ---
        {
            float earLocal = (HL_EYE_LEVEL + HL_NOSE_BASE) * 0.5f;
            float hF = smoothstep(0.08f, 0.0f, std::abs(hLocal - earLocal));
            float aF = smoothstep(0.0f, 0.9f, absCos) * smoothstep(0.4f, 0.0f, std::abs(sinA));
            float amt = hF * aF;
            if (amt > 0.001f) {
                off.x += (cosA > 0 ? 1.0f : -1.0f) * S * 0.05f * amt;
            }
        }
        
        // --- FRONT FACE FLATTENING (makes the face more planar, less cylindrical) ---
        {
            if (hLocal > 0.0f && hLocal < 0.65f && sinA > 0.5f) {
                // Flatten the front of the face (reduce curvature)
                float flatAmt = smoothstep(0.0f, 0.85f, sinA) * smoothstep(0.65f, 0.0f, hLocal);
                // Push front vertices slightly inward to create a flatter face plane
                float inward = -S * 0.015f * flatAmt * (1.0f - absCos);
                off.z += inward;
            }
        }
        
        return off;
    }
    
    // Generate body mesh (feet to neck top). Head is generated separately by FaceTemplateMesh.
    static void generateBody(BaseHumanModel& model, const GeneratorParams& params) {
        const float PI = 3.14159265f;

        // Body profiles: feet to neck top (head is generated separately)
        std::vector<BodyProfile> profiles = {
            {0.00f, 0.08f, 0.08f, 0.0f, 0.0f},   // Feet
            {0.05f, 0.07f, 0.08f, 0.0f, 0.0f},   // Ankles
            {0.25f, 0.10f, 0.10f, 0.0f, 0.0f},   // Calves
            {0.30f, 0.12f, 0.11f, 0.0f, 0.0f},   // Knees
            {0.45f, 0.14f, 0.12f, 0.0f, 0.0f},   // Thighs
            {0.50f, 0.18f, 0.14f, 0.0f, 0.0f},   // Hips
            {0.55f, 0.16f, 0.12f, 0.0f, 0.0f},   // Waist
            {0.62f, 0.18f, 0.13f, 0.0f, 0.0f},   // Chest
            {0.70f, 0.20f, 0.12f, 0.0f, 0.0f},   // Shoulders
            {0.76f, 0.07f, 0.07f, 0.0f, 0.0f},   // Neck base
            {0.82f, 0.065f,0.065f,0.0f, 0.0f},   // Neck mid
            {0.86f, 0.06f, 0.065f,0.0f, 0.003f}, // Neck top
        };

        int numProfiles = static_cast<int>(profiles.size());
        int numSlices = params.bodySubdivisions;
        float H = params.height;

        model.boundsMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        model.boundsMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (int p = 0; p < numProfiles; p++) {
            const BodyProfile& profile = profiles[p];
            float y = profile.height * H;

            for (int s = 0; s < numSlices; s++) {
                float angle = (float)s / numSlices * 2.0f * PI;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                float x = profile.radiusX * cosA + profile.offsetX;
                float z = profile.radiusZ * sinA + profile.offsetZ;

                Vertex v = {};
                v.position[0] = x;
                v.position[1] = y;
                v.position[2] = z;

                float nLen = std::sqrt(cosA * cosA + sinA * sinA);
                v.normal[0] = (nLen > 0.001f) ? cosA / nLen : 0.0f;
                v.normal[1] = 0.0f;
                v.normal[2] = (nLen > 0.001f) ? sinA / nLen : 0.0f;

                v.uv[0] = (float)s / numSlices;
                v.uv[1] = profile.height;

                // White vertex color: actual skin tone comes from material baseColor
                v.color[0] = 1.0f;
                v.color[1] = 1.0f;
                v.color[2] = 1.0f;

                v.tangent[0] = -sinA;
                v.tangent[1] = 0.0f;
                v.tangent[2] = cosA;
                v.tangent[3] = 1.0f;

                model.vertices.push_back(v);

                model.boundsMin.x = std::min(model.boundsMin.x, v.position[0]);
                model.boundsMin.y = std::min(model.boundsMin.y, v.position[1]);
                model.boundsMin.z = std::min(model.boundsMin.z, v.position[2]);
                model.boundsMax.x = std::max(model.boundsMax.x, v.position[0]);
                model.boundsMax.y = std::max(model.boundsMax.y, v.position[1]);
                model.boundsMax.z = std::max(model.boundsMax.z, v.position[2]);
            }
        }

        for (int p = 0; p < numProfiles - 1; p++) {
            for (int s = 0; s < numSlices; s++) {
                int current = p * numSlices + s;
                int next = p * numSlices + (s + 1) % numSlices;
                int above = (p + 1) * numSlices + s;
                int aboveNext = (p + 1) * numSlices + (s + 1) % numSlices;

                // CCW winding for outward-facing normals
                model.indices.push_back(current);
                model.indices.push_back(above);
                model.indices.push_back(next);

                model.indices.push_back(next);
                model.indices.push_back(above);
                model.indices.push_back(aboveNext);
            }
        }

        recomputeNormals(model);

        model.center = Vec3(
            (model.boundsMin.x + model.boundsMax.x) * 0.5f,
            (model.boundsMin.y + model.boundsMax.y) * 0.5f,
            (model.boundsMin.z + model.boundsMax.z) * 0.5f
        );
        Vec3 extent(
            model.boundsMax.x - model.boundsMin.x,
            model.boundsMax.y - model.boundsMin.y,
            model.boundsMax.z - model.boundsMin.z
        );
        model.radius = extent.length() * 0.5f;
        model.vertexCount = static_cast<int>(model.vertices.size());
        model.triangleCount = static_cast<int>(model.indices.size() / 3);
    }

    // Generate body-only blend shapes (e.g. body_weight) excluding the face
    static void generateBodyBlendShapes(BaseHumanModel& model,
                                         const GeneratorParams& params,
                                         int bodyVertCount) {
        float H = params.height;

        BlendShapeTarget target("body_weight");
        for (int i = 0; i < bodyVertCount; i++) {
            float nY = model.vertices[i].position[1] / H;
            if (nY > HEAD_BOTTOM) continue;
            float scale = 0.15f;
            Vec3 delta(model.vertices[i].position[0] * scale,
                       0,
                       model.vertices[i].position[2] * scale);
            if (delta.length() > 0.0001f) {
                BlendShapeDelta d;
                d.vertexIndex = (uint32_t)i;
                d.positionDelta = delta;
                d.normalDelta = Vec3(0, 0, 0);
                target.addDelta(d);
            }
        }
        int targetIdx = model.blendShapes.addTarget(target);
        model.blendShapes.createChannel("body_weight", targetIdx);
    }
    
    static void recomputeNormals(BaseHumanModel& model) {
        // Reset all normals to zero
        for (auto& v : model.vertices) {
            v.normal[0] = v.normal[1] = v.normal[2] = 0.0f;
        }
        // Accumulate face normals
        for (size_t i = 0; i + 2 < model.indices.size(); i += 3) {
            uint32_t i0 = model.indices[i];
            uint32_t i1 = model.indices[i + 1];
            uint32_t i2 = model.indices[i + 2];
            Vec3 p0(model.vertices[i0].position[0], model.vertices[i0].position[1], model.vertices[i0].position[2]);
            Vec3 p1(model.vertices[i1].position[0], model.vertices[i1].position[1], model.vertices[i1].position[2]);
            Vec3 p2(model.vertices[i2].position[0], model.vertices[i2].position[1], model.vertices[i2].position[2]);
            Vec3 e1 = p1 - p0;
            Vec3 e2 = p2 - p0;
            Vec3 fn = e1.cross(e2);
            for (uint32_t idx : {i0, i1, i2}) {
                model.vertices[idx].normal[0] += fn.x;
                model.vertices[idx].normal[1] += fn.y;
                model.vertices[idx].normal[2] += fn.z;
            }
        }
        // Normalize
        for (auto& v : model.vertices) {
            float len = std::sqrt(v.normal[0]*v.normal[0] + v.normal[1]*v.normal[1] + v.normal[2]*v.normal[2]);
            if (len > 0.0001f) {
                v.normal[0] /= len; v.normal[1] /= len; v.normal[2] /= len;
            }
        }
    }
    
    // Helper to add a blend shape channel matching CharacterFace naming convention
    static void addFaceBlendShape(BaseHumanModel& model, const std::string& channelName,
                                   float H, int numSlices,
                                   std::function<Vec3(float x, float y, float z, float nY, float angle)> deltaFn) {
        BlendShapeTarget target(channelName);
        for (size_t i = 0; i < model.vertices.size(); i++) {
            float vx = model.vertices[i].position[0];
            float vy = model.vertices[i].position[1];
            float vz = model.vertices[i].position[2];
            float nY = vy / H;
            
            // Compute angle from vertex position (reconstruct from x,z)
            float angle = std::atan2(vz, vx);
            if (angle < 0) angle += 2.0f * 3.14159265f;
            
            Vec3 delta = deltaFn(vx, vy, vz, nY, angle);
            if (delta.length() > 0.0001f) {
                target.addDelta(BlendShapeDelta(static_cast<uint32_t>(i), delta));
            }
        }
        if (!target.deltas.empty()) {
            int idx = model.blendShapes.addTarget(target);
            BlendShapeChannel ch(channelName);
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
    }
    
    static void generateBlendShapes(BaseHumanModel& model, const GeneratorParams& params) {
        int numSlices = params.bodySubdivisions;
        float H = params.height;
        float headH = H * HEAD_RANGE;
        float S = headH; // features scale relative to head size
        const float PI = 3.14159265f;
        (void)numSlices;
        
        // Head region in global normalized Y
        float headMin = HEAD_BOTTOM;
        float headMax = HEAD_TOP - 0.02f; // exclude very top
        
        auto isHead = [&](float nY) { return nY >= headMin && nY <= headMax; };
        // Convert global nY to head-local (0=chin, 1=crown)
        auto toHeadLocal = [&](float nY) { return (nY - HEAD_BOTTOM) / HEAD_RANGE; };
        
        // ================================================================
        // BODY BLEND SHAPES
        // ================================================================
        
        // Body height
        addFaceBlendShape(model, "body_height", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                return Vec3(0, nY * 0.1f * H, 0);
            });
        
        // Body weight
        addFaceBlendShape(model, "body_weight", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (nY > HEAD_BOTTOM) return Vec3(0,0,0); // don't scale head
                return Vec3(x * 0.2f, 0, z * 0.2f);
            });
        
        // Shoulder width
        addFaceBlendShape(model, "shoulder_width", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (nY < 0.65f || nY > 0.75f) return Vec3(0,0,0);
                float inf = 1.0f - std::abs(nY - 0.7f) / 0.05f;
                inf = std::max(0.0f, inf);
                return Vec3(x * 0.15f * inf, 0, 0);
            });
        
        // ================================================================
        // FACE BLEND SHAPES - all use head-local coordinates
        // Names match CharacterFace::setupDefaultMappings()
        // ================================================================
        
        // Helper: check if in head and get head-local Y
        #define BS_HEAD_CHECK \
            if (!isHead(nY)) return Vec3(0,0,0); \
            float hL = toHeadLocal(nY); \
            float cosA = std::cos(a); float sinA = std::sin(a); \
            float absCos = std::abs(cosA);
        
        addFaceBlendShape(model, "faceWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (!isHead(nY)) return Vec3(0,0,0);
                return Vec3(x * 0.15f, 0, 0);
            });
        
        addFaceBlendShape(model, "faceLength", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (!isHead(nY)) return Vec3(0,0,0);
                float hL = toHeadLocal(nY);
                float center = 0.4f;
                return Vec3(0, (hL - center) * S * 0.15f, 0);
            });
        
        addFaceBlendShape(model, "faceRoundness", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (hL > 0.25f && hL < 0.5f) {
                    float f = smoothstep(0.12f, 0.0f, std::abs(hL - 0.37f));
                    return Vec3(x * 0.08f * f, 0, z * 0.04f * f);
                }
                if (hL < 0.12f) {
                    float f = smoothstep(0.06f, 0.0f, hL);
                    return Vec3(-x * 0.06f * f, 0, 0);
                }
                return Vec3(0,0,0);
            });
        
        addFaceBlendShape(model, "eyeSize", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float hF = smoothstep(0.06f, 0.0f, std::abs(hL - HL_EYE_LEVEL));
                float eyeD = std::min(std::abs(cosA - 0.40f), std::abs(cosA + 0.40f));
                if (eyeD > 0.25f) return Vec3(0,0,0);
                float amt = smoothstep(0.25f, 0.0f, eyeD) * hF;
                return Vec3(cosA * (-S * 0.04f * amt), 0, sinA * (-S * 0.04f * amt));
            });
        
        addFaceBlendShape(model, "eyeSpacing", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float hF = smoothstep(0.05f, 0.0f, std::abs(hL - HL_EYE_LEVEL));
                float dL = std::abs(cosA + 0.40f);
                if (dL < 0.22f) return Vec3(-S * 0.03f * smoothstep(0.22f, 0.0f, dL) * hF, 0, 0);
                float dR = std::abs(cosA - 0.40f);
                if (dR < 0.22f) return Vec3(S * 0.03f * smoothstep(0.22f, 0.0f, dR) * hF, 0, 0);
                return Vec3(0,0,0);
            });
        
        addFaceBlendShape(model, "eyeDepth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float hF = smoothstep(0.05f, 0.0f, std::abs(hL - HL_EYE_LEVEL));
                float eyeD = std::min(std::abs(cosA - 0.40f), std::abs(cosA + 0.40f));
                if (eyeD > 0.22f) return Vec3(0,0,0);
                float amt = smoothstep(0.22f, 0.0f, eyeD) * hF;
                return Vec3(cosA * (-S * 0.03f * amt), 0, sinA * (-S * 0.03f * amt));
            });
        
        addFaceBlendShape(model, "eyeHeight", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float eyeD = std::min(std::abs(cosA - 0.40f), std::abs(cosA + 0.40f));
                if (eyeD > 0.25f) return Vec3(0,0,0);
                return Vec3(0, S * 0.03f * smoothstep(0.25f, 0.0f, eyeD), 0);
            });
        
        addFaceBlendShape(model, "eyeAngle", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float hF = smoothstep(0.05f, 0.0f, std::abs(hL - HL_EYE_LEVEL));
                float dL = cosA + 0.40f, dR = cosA - 0.40f;
                float yOff = 0;
                if (std::abs(dL) < 0.22f) yOff = dL * S * 0.02f * hF;
                if (std::abs(dR) < 0.22f) yOff = -dR * S * 0.02f * hF;
                return (std::abs(yOff) > 0.0001f) ? Vec3(0, yOff, 0) : Vec3(0,0,0);
            });
        
        addFaceBlendShape(model, "browHeight", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float f = smoothstep(0.04f, 0.0f, std::abs(hL - HL_BROW)) * sinA;
                return Vec3(0, S * 0.025f * f, 0);
            });
        
        addFaceBlendShape(model, "browAngle", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float f = smoothstep(0.04f, 0.0f, std::abs(hL - HL_BROW)) * sinA;
                return Vec3(0, absCos * S * 0.015f * f, 0);
            });
        
        addFaceBlendShape(model, "browThickness", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.3f) return Vec3(0,0,0);
                float f = smoothstep(0.04f, 0.0f, std::abs(hL - HL_BROW)) * sinA;
                return Vec3(0, 0, S * 0.02f * f);
            });
        
        addFaceBlendShape(model, "noseLength", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.65f || absCos > 0.3f) return Vec3(0,0,0);
                if (hL < HL_NOSE_BASE - 0.03f || hL > HL_NOSE_BRIDGE + 0.03f) return Vec3(0,0,0);
                float f = smoothstep(0.3f, 0.0f, absCos) * smoothstep(0.0f, 0.75f, sinA);
                return Vec3(0, -(hL - HL_NOSE_TIP) * S * 0.2f * f, 0);
            });
        
        addFaceBlendShape(model, "noseWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.5f || absCos > 0.35f) return Vec3(0,0,0);
                float hF = smoothstep(0.04f, 0.0f, std::abs(hL - HL_NOSE_BASE));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.02f * hF, 0, 0);
            });
        
        addFaceBlendShape(model, "noseHeight", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.7f || absCos > 0.25f) return Vec3(0,0,0);
                float hF = smoothstep(0.05f, 0.0f, std::abs(hL - HL_NOSE_TIP));
                float f = smoothstep(0.25f, 0.0f, absCos) * smoothstep(0.0f, 0.8f, sinA) * hF;
                return Vec3(0, 0, S * 0.06f * f);
            });
        
        addFaceBlendShape(model, "noseBridge", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.75f) return Vec3(0,0,0);
                float hF = smoothstep(0.04f, 0.0f, std::abs(hL - HL_NOSE_BRIDGE));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.015f * hF, 0, 0);
            });
        
        addFaceBlendShape(model, "noseTip", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.7f || absCos > 0.25f) return Vec3(0,0,0);
                float hF = smoothstep(0.03f, 0.0f, std::abs(hL - HL_NOSE_TIP));
                float f = smoothstep(0.25f, 0.0f, absCos) * hF;
                return Vec3(0, 0, S * 0.04f * f);
            });
        
        addFaceBlendShape(model, "nostrilWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.4f) return Vec3(0,0,0);
                if (absCos < 0.1f || absCos > 0.35f) return Vec3(0,0,0);
                float hF = smoothstep(0.025f, 0.0f, std::abs(hL - HL_NOSE_BASE));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.02f * hF, 0, 0);
            });
        
        addFaceBlendShape(model, "mouthWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.45f || absCos > 0.45f) return Vec3(0,0,0);
                float hF = smoothstep(0.04f, 0.0f, std::abs(hL - HL_MOUTH));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.025f * hF, 0, 0);
            });
        
        addFaceBlendShape(model, "upperLipThickness", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.5f || absCos > 0.35f) return Vec3(0,0,0);
                float f = smoothstep(0.35f, 0.0f, absCos) * smoothstep(0.0f, 0.55f, sinA);
                float hF = smoothstep(0.03f, 0.0f, std::abs(hL - HL_UPPER_LIP));
                return Vec3(0, 0, S * 0.02f * f * hF);
            });
        
        addFaceBlendShape(model, "lowerLipThickness", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.5f || absCos > 0.35f) return Vec3(0,0,0);
                float f = smoothstep(0.35f, 0.0f, absCos) * smoothstep(0.0f, 0.55f, sinA);
                float hF = smoothstep(0.03f, 0.0f, std::abs(hL - HL_LOWER_LIP));
                return Vec3(0, 0, S * 0.02f * f * hF);
            });
        
        addFaceBlendShape(model, "lipProtrusion", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (sinA < 0.5f || absCos > 0.35f) return Vec3(0,0,0);
                float hF = smoothstep(0.05f, 0.0f, std::abs(hL - HL_MOUTH));
                float f = smoothstep(0.35f, 0.0f, absCos) * sinA * hF;
                return Vec3(0, 0, S * 0.03f * f);
            });
        
        addFaceBlendShape(model, "chinLength", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (hL > 0.08f) return Vec3(0,0,0);
                float f = smoothstep(0.04f, 0.0f, hL) * std::max(0.0f, sinA);
                return Vec3(0, -S * 0.03f * f, 0);
            });
        
        addFaceBlendShape(model, "chinWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (!isHead(nY)) return Vec3(0,0,0);
                float hL = toHeadLocal(nY);
                if (hL > 0.08f) return Vec3(0,0,0);
                return Vec3(x * 0.1f * smoothstep(0.04f, 0.0f, hL), 0, 0);
            });
        
        addFaceBlendShape(model, "chinProtrusion", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (hL > 0.06f || sinA < 0.35f || absCos > 0.35f) return Vec3(0,0,0);
                float f = smoothstep(0.04f, 0.0f, hL) * smoothstep(0.35f, 0.0f, absCos) * sinA;
                return Vec3(0, 0, S * 0.035f * f);
            });
        
        addFaceBlendShape(model, "jawWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (!isHead(nY)) return Vec3(0,0,0);
                float hL = toHeadLocal(nY);
                if (hL > 0.2f) return Vec3(0,0,0);
                float f = smoothstep(0.1f, 0.0f, std::abs(hL - 0.08f));
                return Vec3(x * 0.12f * f, 0, 0);
            });
        
        addFaceBlendShape(model, "jawAngle", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (hL > 0.2f) return Vec3(0,0,0);
                if (absCos < 0.35f || absCos > 0.8f) return Vec3(0,0,0);
                float f = smoothstep(0.08f, 0.0f, std::abs(hL - 0.08f));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.02f * f, -S * 0.01f * f, 0);
            });
        
        addFaceBlendShape(model, "cheekboneProminence", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                float cheekHL = (HL_EYE_LEVEL + HL_NOSE_BRIDGE) * 0.5f;
                if (absCos < 0.3f || absCos > 0.75f || sinA < 0.15f) return Vec3(0,0,0);
                float hF = smoothstep(0.07f, 0.0f, std::abs(hL - cheekHL));
                float aF = smoothstep(0.22f, 0.0f, std::abs(absCos - 0.55f));
                float f = hF * aF;
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.03f * f, 0, S * 0.012f * f);
            });
        
        addFaceBlendShape(model, "cheekFullness", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (hL < 0.1f || hL > 0.4f) return Vec3(0,0,0);
                if (absCos < 0.2f || absCos > 0.7f || sinA < 0.1f) return Vec3(0,0,0);
                float hF = smoothstep(0.1f, 0.0f, std::abs(hL - 0.25f));
                float aF = smoothstep(0.22f, 0.0f, std::abs(absCos - 0.45f));
                float f = hF * aF;
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.025f * f, 0, S * 0.015f * f);
            });
        
        addFaceBlendShape(model, "foreheadHeight", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                if (hL < 0.55f || hL > 0.85f) return Vec3(0,0,0);
                if (sinA < 0.2f) return Vec3(0,0,0);
                float f = smoothstep(0.55f, 0.85f, hL) * sinA;
                return Vec3(0, S * 0.02f * f, 0);
            });
        
        addFaceBlendShape(model, "foreheadWidth", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                if (!isHead(nY)) return Vec3(0,0,0);
                float hL = toHeadLocal(nY);
                if (hL < 0.55f || hL > 0.75f) return Vec3(0,0,0);
                float f = smoothstep(0.1f, 0.0f, std::abs(hL - 0.65f));
                return Vec3(x * 0.1f * f, 0, 0);
            });
        
        addFaceBlendShape(model, "earSize", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                float earHL = (HL_EYE_LEVEL + HL_NOSE_BASE) * 0.5f;
                if (absCos < 0.85f || std::abs(sinA) > 0.4f) return Vec3(0,0,0);
                float hF = smoothstep(0.08f, 0.0f, std::abs(hL - earHL));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.035f * hF, 0, 0);
            });
        
        addFaceBlendShape(model, "earAngle", H, numSlices,
            [&](float x, float y, float z, float nY, float a) -> Vec3 {
                BS_HEAD_CHECK;
                float earHL = (HL_EYE_LEVEL + HL_NOSE_BASE) * 0.5f;
                if (absCos < 0.82f || std::abs(sinA) > 0.45f) return Vec3(0,0,0);
                float hF = smoothstep(0.08f, 0.0f, std::abs(hL - earHL));
                float sign = cosA > 0 ? 1.0f : -1.0f;
                return Vec3(sign * S * 0.05f * hF, 0, 0);
            });
        
        #undef BS_HEAD_CHECK
        
        model.blendShapeCount = static_cast<int>(model.blendShapes.getTargetCount());
    }
};

// ============================================================================
// Base Human Model Library
// ============================================================================

class BaseHumanModelLibrary {
public:
    // Singleton access
    static BaseHumanModelLibrary& getInstance() {
        static BaseHumanModelLibrary instance;
        return instance;
    }
    
    // Add a model to the library
    void addModel(const std::string& id, const BaseHumanModel& model) {
        models_[id] = model;
    }
    
    // Get a model by ID
    const BaseHumanModel* getModel(const std::string& id) const {
        auto it = models_.find(id);
        return (it != models_.end()) ? &it->second : nullptr;
    }
    
    BaseHumanModel* getModel(const std::string& id) {
        auto it = models_.find(id);
        return (it != models_.end()) ? &it->second : nullptr;
    }
    
    // List all available models
    std::vector<std::string> getModelIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : models_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    // Generate and add default procedural model
    // MetaHuman style: face only first, body added later
    void initializeDefaults(const std::string& modelDir = "", bool faceOnly = true) {
        printf("[ModelLibrary] initializeDefaults called with modelDir='%s', faceOnly=%d\n", 
               modelDir.c_str(), faceOnly);
        
        // If model directory changed or no models exist, regenerate
        if (modelDir != modelDirectory_ || models_.find("procedural_human") == models_.end()) {
            modelDirectory_ = modelDir;
            
            // Remove old model if exists
            models_.erase("procedural_human");
            
            ProceduralHumanGenerator::GeneratorParams params;
            params.bodySubdivisions = 48;
            params.heightSegments = 30;
            params.height = 1.75f;
            
            // Pass model directory to use BFM if available, face only mode
            BaseHumanModel proceduralModel = ProceduralHumanGenerator::generate(params, modelDir, faceOnly);
            addModel("procedural_human", proceduralModel);
        } else {
            printf("[ModelLibrary] Using cached model (modelDir unchanged)\n");
        }
    }
    
    const std::string& getModelDirectory() const { return modelDirectory_; }
    
    // Load MakeHuman model if available
    bool loadMakeHumanModel(const std::string& objPath, const std::string& targetDir) {
        BaseHumanModel model;
        if (BaseHumanLoader::loadMakeHuman(objPath, targetDir, model)) {
            addModel("makehuman_" + model.name, model);
            return true;
        }
        return false;
    }
    
private:
    BaseHumanModelLibrary() = default;
    std::unordered_map<std::string, BaseHumanModel> models_;
    std::string modelDirectory_;
};

} // namespace luma
