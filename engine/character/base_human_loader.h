// Base Human Model Loader - Load and manage base human models with BlendShapes
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character.h"
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
    
    // Generate a simple parametric human model
    static BaseHumanModel generate(const GeneratorParams& params = GeneratorParams()) {
        BaseHumanModel model;
        model.name = "ProceduralHuman";
        model.source = "generated";
        
        // Generate body as a series of elliptical cross-sections
        generateBody(model, params);
        
        // Generate blend shapes
        if (params.generateBlendShapes) {
            generateBlendShapes(model, params);
        }
        
        // Generate skeleton
        if (params.generateSkeleton) {
            BaseHumanLoader::initializeMakeHumanSkeleton(model.skeleton);
            model.boneCount = model.skeleton.getBoneCount();
        }
        
        return model;
    }
    
private:
    // Body profile at different heights (normalized 0-1)
    struct BodyProfile {
        float height;       // Normalized height (0 = feet, 1 = top of head)
        float radiusX;      // Horizontal radius (side to side)
        float radiusZ;      // Depth radius (front to back)
        float offsetX;      // Horizontal offset
        float offsetZ;      // Depth offset
    };
    
    static void generateBody(BaseHumanModel& model, const GeneratorParams& params) {
        // Define body profile (simplified human shape)
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
            {0.75f, 0.08f, 0.08f, 0.0f, 0.0f},   // Neck
            {0.80f, 0.10f, 0.11f, 0.0f, 0.0f},   // Head bottom
            {0.90f, 0.11f, 0.12f, 0.0f, 0.0f},   // Head middle
            {0.97f, 0.09f, 0.10f, 0.0f, 0.0f},   // Head top
            {1.00f, 0.02f, 0.02f, 0.0f, 0.0f}    // Crown
        };
        
        int numProfiles = static_cast<int>(profiles.size());
        int numSlices = params.bodySubdivisions;
        
        model.boundsMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        model.boundsMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        // Generate vertices
        for (int p = 0; p < numProfiles; p++) {
            const BodyProfile& profile = profiles[p];
            float y = profile.height * params.height;
            
            for (int s = 0; s < numSlices; s++) {
                float angle = (float)s / numSlices * 2.0f * 3.14159f;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                
                float x = profile.radiusX * cosA + profile.offsetX;
                float z = profile.radiusZ * sinA + profile.offsetZ;
                
                Vertex v;
                v.position[0] = x;
                v.position[1] = y;
                v.position[2] = z;
                
                // Normal (approximate)
                v.normal[0] = cosA;
                v.normal[1] = 0.0f;
                v.normal[2] = sinA;
                
                // UV
                v.uv[0] = (float)s / numSlices;
                v.uv[1] = profile.height;
                
                // Color (skin tone placeholder)
                v.color[0] = 0.85f;
                v.color[1] = 0.65f;
                v.color[2] = 0.5f;
                
                // Tangent
                v.tangent[0] = -sinA;
                v.tangent[1] = 0.0f;
                v.tangent[2] = cosA;
                v.tangent[3] = 1.0f;
                
                model.vertices.push_back(v);
                
                // Update bounds
                model.boundsMin.x = std::min(model.boundsMin.x, x);
                model.boundsMin.y = std::min(model.boundsMin.y, y);
                model.boundsMin.z = std::min(model.boundsMin.z, z);
                model.boundsMax.x = std::max(model.boundsMax.x, x);
                model.boundsMax.y = std::max(model.boundsMax.y, y);
                model.boundsMax.z = std::max(model.boundsMax.z, z);
            }
        }
        
        // Generate indices (connect profiles)
        for (int p = 0; p < numProfiles - 1; p++) {
            for (int s = 0; s < numSlices; s++) {
                int current = p * numSlices + s;
                int next = p * numSlices + (s + 1) % numSlices;
                int above = (p + 1) * numSlices + s;
                int aboveNext = (p + 1) * numSlices + (s + 1) % numSlices;
                
                // Triangle 1
                model.indices.push_back(current);
                model.indices.push_back(next);
                model.indices.push_back(above);
                
                // Triangle 2
                model.indices.push_back(next);
                model.indices.push_back(aboveNext);
                model.indices.push_back(above);
            }
        }
        
        // Calculate center and radius
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
    
    static void generateBlendShapes(BaseHumanModel& model, const GeneratorParams& params) {
        int numSlices = params.bodySubdivisions;
        float height = params.height;
        
        // Height increase/decrease
        {
            BlendShapeTarget heightInc("body_height");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float normalizedY = model.vertices[i].position[1] / height;
                float delta = normalizedY * 0.1f * height;  // 10% height change
                heightInc.addDelta(BlendShapeDelta(
                    static_cast<uint32_t>(i),
                    Vec3(0, delta, 0)
                ));
            }
            int idx = model.blendShapes.addTarget(heightInc);
            BlendShapeChannel ch("body_height");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Weight (horizontal scale)
        {
            BlendShapeTarget weightInc("body_weight");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float x = model.vertices[i].position[0];
                float z = model.vertices[i].position[2];
                weightInc.addDelta(BlendShapeDelta(
                    static_cast<uint32_t>(i),
                    Vec3(x * 0.2f, 0, z * 0.2f)  // 20% scale change
                ));
            }
            int idx = model.blendShapes.addTarget(weightInc);
            BlendShapeChannel ch("body_weight");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Shoulder width
        {
            BlendShapeTarget shoulderWidth("shoulder_width");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float y = model.vertices[i].position[1];
                float normalizedY = y / height;
                
                // Only affect shoulder area (0.65 - 0.75)
                if (normalizedY >= 0.65f && normalizedY <= 0.75f) {
                    float x = model.vertices[i].position[0];
                    float influence = 1.0f - std::abs(normalizedY - 0.7f) / 0.05f;
                    influence = std::max(0.0f, influence);
                    shoulderWidth.addDelta(BlendShapeDelta(
                        static_cast<uint32_t>(i),
                        Vec3(x * 0.15f * influence, 0, 0)
                    ));
                }
            }
            int idx = model.blendShapes.addTarget(shoulderWidth);
            BlendShapeChannel ch("shoulder_width");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Chest size
        {
            BlendShapeTarget chestSize("chest_size");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float y = model.vertices[i].position[1];
                float z = model.vertices[i].position[2];
                float normalizedY = y / height;
                
                // Only affect chest area (0.58 - 0.68) and front
                if (normalizedY >= 0.58f && normalizedY <= 0.68f && z > 0) {
                    float influence = 1.0f - std::abs(normalizedY - 0.63f) / 0.05f;
                    influence = std::max(0.0f, influence);
                    chestSize.addDelta(BlendShapeDelta(
                        static_cast<uint32_t>(i),
                        Vec3(0, 0, z * 0.2f * influence)
                    ));
                }
            }
            int idx = model.blendShapes.addTarget(chestSize);
            BlendShapeChannel ch("chest_size");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Waist size
        {
            BlendShapeTarget waistSize("waist_size");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float y = model.vertices[i].position[1];
                float x = model.vertices[i].position[0];
                float z = model.vertices[i].position[2];
                float normalizedY = y / height;
                
                // Waist area (0.52 - 0.58)
                if (normalizedY >= 0.52f && normalizedY <= 0.58f) {
                    float influence = 1.0f - std::abs(normalizedY - 0.55f) / 0.03f;
                    influence = std::max(0.0f, influence);
                    waistSize.addDelta(BlendShapeDelta(
                        static_cast<uint32_t>(i),
                        Vec3(x * 0.15f * influence, 0, z * 0.15f * influence)
                    ));
                }
            }
            int idx = model.blendShapes.addTarget(waistSize);
            BlendShapeChannel ch("waist_size");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Hip width
        {
            BlendShapeTarget hipWidth("hip_width");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float y = model.vertices[i].position[1];
                float x = model.vertices[i].position[0];
                float normalizedY = y / height;
                
                // Hip area (0.45 - 0.52)
                if (normalizedY >= 0.45f && normalizedY <= 0.52f) {
                    float influence = 1.0f - std::abs(normalizedY - 0.485f) / 0.035f;
                    influence = std::max(0.0f, influence);
                    hipWidth.addDelta(BlendShapeDelta(
                        static_cast<uint32_t>(i),
                        Vec3(x * 0.15f * influence, 0, 0)
                    ));
                }
            }
            int idx = model.blendShapes.addTarget(hipWidth);
            BlendShapeChannel ch("hip_width");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Face width (head area)
        {
            BlendShapeTarget faceWidth("face_width");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float y = model.vertices[i].position[1];
                float x = model.vertices[i].position[0];
                float normalizedY = y / height;
                
                // Head area (0.78 - 0.97)
                if (normalizedY >= 0.78f && normalizedY <= 0.97f) {
                    faceWidth.addDelta(BlendShapeDelta(
                        static_cast<uint32_t>(i),
                        Vec3(x * 0.1f, 0, 0)
                    ));
                }
            }
            int idx = model.blendShapes.addTarget(faceWidth);
            BlendShapeChannel ch("face_width");
            ch.minWeight = -1.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        // Muscularity (affects arms and legs)
        {
            BlendShapeTarget muscle("muscularity");
            for (size_t i = 0; i < model.vertices.size(); i++) {
                float y = model.vertices[i].position[1];
                float x = model.vertices[i].position[0];
                float z = model.vertices[i].position[2];
                float normalizedY = y / height;
                
                // Arms and legs have more pronounced muscle effect
                bool isLimb = (normalizedY < 0.5f) ||  // Legs
                              (normalizedY > 0.65f && normalizedY < 0.75f);  // Shoulders/arms
                
                if (isLimb) {
                    muscle.addDelta(BlendShapeDelta(
                        static_cast<uint32_t>(i),
                        Vec3(x * 0.1f, 0, z * 0.1f)
                    ));
                }
            }
            int idx = model.blendShapes.addTarget(muscle);
            BlendShapeChannel ch("muscularity");
            ch.minWeight = 0.0f;
            ch.maxWeight = 1.0f;
            ch.addTarget(idx, 1.0f);
            model.blendShapes.addChannel(ch);
        }
        
        model.blendShapeCount = static_cast<int>(model.blendShapes.getTargetCount());
        
        (void)numSlices;  // Suppress unused warning
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
    void initializeDefaults() {
        // Add a procedural human for testing
        ProceduralHumanGenerator::GeneratorParams params;
        params.bodySubdivisions = 16;
        params.heightSegments = 30;
        params.height = 1.75f;
        
        BaseHumanModel proceduralModel = ProceduralHumanGenerator::generate(params);
        addModel("procedural_human", proceduralModel);
    }
    
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
};

} // namespace luma
