// MakeHuman Integration - Load and use MakeHuman assets
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/base_human_loader.h"
#include "engine/animation/skeleton.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace luma {

// ============================================================================
// MakeHuman Asset Paths
// ============================================================================

struct MakeHumanPaths {
    // Default paths relative to assets folder
    static constexpr const char* BaseModelPath = "assets/makehuman/base.obj";
    static constexpr const char* TargetsPath = "assets/makehuman/targets/";
    static constexpr const char* SkeletonPath = "assets/makehuman/skeleton/";
    static constexpr const char* TexturesPath = "assets/makehuman/textures/";
    
    // MakeHuman target file categories
    static constexpr const char* MacroTargets = "macrodetails/";
    static constexpr const char* GenderTargets = "macrodetails/Gender/";
    static constexpr const char* AgeTargets = "macrodetails/Age/";
    static constexpr const char* BodyTargets = "body/";
    static constexpr const char* FaceTargets = "face/";
    static constexpr const char* HeadTargets = "head/";
    static constexpr const char* EyeTargets = "eyes/";
    static constexpr const char* NoseTargets = "nose/";
    static constexpr const char* MouthTargets = "mouth/";
    static constexpr const char* EarTargets = "ears/";
};

// ============================================================================
// MakeHuman Target File Parser
// ============================================================================

class MakeHumanTargetLoader {
public:
    // Load a .target file (MakeHuman's BlendShape format)
    // Format: vertex_index dx dy dz
    static bool loadTarget(const std::string& path, BlendShapeTarget& outTarget, const std::string& name = "") {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        outTarget.name = name.empty() ? extractTargetName(path) : name;
        outTarget.deltas.clear();
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            uint32_t vertexIndex;
            float dx, dy, dz;
            
            if (iss >> vertexIndex >> dx >> dy >> dz) {
                // MakeHuman uses Y-up, same as our convention
                BlendShapeDelta delta;
                delta.vertexIndex = vertexIndex;
                delta.positionDelta = Vec3(dx, dy, dz);
                delta.normalDelta = Vec3(0, 0, 0);  // Will be computed later
                outTarget.deltas.push_back(delta);
            }
        }
        
        return !outTarget.deltas.empty();
    }
    
    // Load all targets from a directory
    static std::vector<BlendShapeTarget> loadTargetsFromDirectory(const std::string& directory) {
        std::vector<BlendShapeTarget> targets;
        
        if (!std::filesystem::exists(directory)) {
            return targets;
        }
        
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".target") {
                    BlendShapeTarget target;
                    if (loadTarget(entry.path().string(), target)) {
                        targets.push_back(target);
                    }
                }
            }
        }
        
        return targets;
    }
    
private:
    static std::string extractTargetName(const std::string& path) {
        // Get filename without extension
        std::filesystem::path p(path);
        return p.stem().string();
    }
};

// ============================================================================
// MakeHuman Skeleton Mapping
// ============================================================================

struct MakeHumanSkeletonMapping {
    // MakeHuman default skeleton bone names
    static const std::vector<std::string>& getBoneNames() {
        static std::vector<std::string> names = {
            "root",
            "pelvis",
            "spine01", "spine02", "spine03",
            "clavicle_l", "upperarm_l", "forearm_l", "hand_l",
            "clavicle_r", "upperarm_r", "forearm_r", "hand_r",
            "neck", "head",
            "thigh_l", "shin_l", "foot_l", "toe_l",
            "thigh_r", "shin_r", "foot_r", "toe_r",
            // Fingers (optional)
            "thumb01_l", "thumb02_l", "thumb03_l",
            "index01_l", "index02_l", "index03_l",
            "middle01_l", "middle02_l", "middle03_l",
            "ring01_l", "ring02_l", "ring03_l",
            "pinky01_l", "pinky02_l", "pinky03_l",
            "thumb01_r", "thumb02_r", "thumb03_r",
            "index01_r", "index02_r", "index03_r",
            "middle01_r", "middle02_r", "middle03_r",
            "ring01_r", "ring02_r", "ring03_r",
            "pinky01_r", "pinky02_r", "pinky03_r"
        };
        return names;
    }
    
    // Parent indices (-1 for root)
    static const std::vector<int>& getBoneParents() {
        static std::vector<int> parents = {
            -1,  // root
            0,   // pelvis -> root
            1, 2, 3,  // spine chain -> pelvis
            3, 5, 6, 7,  // left arm
            3, 9, 10, 11,  // right arm
            3, 13,  // neck, head
            1, 15, 16, 17,  // left leg
            1, 19, 20, 21,  // right leg
            // Finger parents (all relative to hands)
            8, 23, 24,  // left thumb
            8, 26, 27,  // left index
            8, 29, 30,  // left middle
            8, 32, 33,  // left ring
            8, 35, 36,  // left pinky
            12, 38, 39,  // right thumb
            12, 41, 42,  // right index
            12, 44, 45,  // right middle
            12, 47, 48,  // right ring
            12, 50, 51   // right pinky
        };
        return parents;
    }
};

// ============================================================================
// MakeHuman Model Loader
// ============================================================================

class MakeHumanLoader {
public:
    // Load complete MakeHuman model with targets
    static bool loadModel(const std::string& basePath, BaseHumanModel& outModel) {
        std::string modelDir = basePath;
        if (modelDir.back() != '/' && modelDir.back() != '\\') {
            modelDir += '/';
        }
        
        // Load base mesh
        std::string objPath = modelDir + "base.obj";
        if (!loadOBJ(objPath, outModel)) {
            // Try alternative path
            objPath = modelDir + "makehuman_base.obj";
            if (!loadOBJ(objPath, outModel)) {
                return false;
            }
        }
        
        // Load targets
        std::string targetsDir = modelDir + "targets/";
        if (std::filesystem::exists(targetsDir)) {
            auto targets = MakeHumanTargetLoader::loadTargetsFromDirectory(targetsDir);
            for (const auto& target : targets) {
                outModel.blendShapes.addTarget(target);
                
                // Create channel for each target
                BlendShapeChannel channel;
                channel.name = target.name;
                channel.targetIndices.push_back(static_cast<int>(outModel.blendShapes.getTargetCount()) - 1);
                channel.targetWeights.push_back(1.0f);
                channel.defaultWeight = 0.0f;
                channel.minWeight = -1.0f;
                channel.maxWeight = 1.0f;
                outModel.blendShapes.addChannel(channel);
            }
        }
        
        // Initialize skeleton
        initializeMakeHumanSkeleton(outModel.skeleton);
        
        outModel.name = "MakeHuman";
        outModel.source = "MakeHuman";
        
        return true;
    }
    
    // Load OBJ file
    static bool loadOBJ(const std::string& path, BaseHumanModel& outModel) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        std::vector<Vec3> positions;
        std::vector<Vec3> normals;
        std::vector<Vec2> texCoords;
        
        struct Face {
            int v[3];
            int t[3];
            int n[3];
        };
        std::vector<Face> faces;
        
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
                Vec3 n;
                iss >> n.x >> n.y >> n.z;
                normals.push_back(n);
            }
            else if (prefix == "vt") {
                Vec2 uv;
                iss >> uv.x >> uv.y;
                texCoords.push_back(uv);
            }
            else if (prefix == "f") {
                Face face;
                for (int i = 0; i < 3; i++) {
                    std::string vertStr;
                    iss >> vertStr;
                    
                    // Parse v/t/n format
                    int v = 0, t = 0, n = 0;
                    sscanf(vertStr.c_str(), "%d/%d/%d", &v, &t, &n);
                    
                    if (v == 0) {
                        sscanf(vertStr.c_str(), "%d//%d", &v, &n);
                    }
                    if (v == 0) {
                        sscanf(vertStr.c_str(), "%d", &v);
                    }
                    
                    // OBJ is 1-indexed
                    face.v[i] = v - 1;
                    face.t[i] = t - 1;
                    face.n[i] = n - 1;
                }
                faces.push_back(face);
            }
        }
        
        if (positions.empty()) {
            return false;
        }
        
        // Build vertex buffer
        outModel.vertices.clear();
        outModel.indices.clear();
        
        // Simple approach: each face vertex becomes a unique vertex
        // (A more sophisticated approach would use index sharing)
        for (const auto& face : faces) {
            for (int i = 0; i < 3; i++) {
                Vertex v;
                
                if (face.v[i] >= 0 && face.v[i] < (int)positions.size()) {
                    v.position[0] = positions[face.v[i]].x;
                    v.position[1] = positions[face.v[i]].y;
                    v.position[2] = positions[face.v[i]].z;
                }
                
                if (face.n[i] >= 0 && face.n[i] < (int)normals.size()) {
                    v.normal[0] = normals[face.n[i]].x;
                    v.normal[1] = normals[face.n[i]].y;
                    v.normal[2] = normals[face.n[i]].z;
                }
                
                if (face.t[i] >= 0 && face.t[i] < (int)texCoords.size()) {
                    v.texCoord[0] = texCoords[face.t[i]].x;
                    v.texCoord[1] = texCoords[face.t[i]].y;
                }
                
                outModel.indices.push_back(static_cast<uint32_t>(outModel.vertices.size()));
                outModel.vertices.push_back(v);
            }
        }
        
        return true;
    }
    
    // Initialize MakeHuman skeleton
    static void initializeMakeHumanSkeleton(Skeleton& skeleton) {
        skeleton = Skeleton();
        
        const auto& names = MakeHumanSkeletonMapping::getBoneNames();
        const auto& parents = MakeHumanSkeletonMapping::getBoneParents();
        
        // Default bind pose positions (approximate)
        struct BoneDefaults {
            Vec3 position;
            Quat rotation;
        };
        
        std::vector<BoneDefaults> defaults(names.size());
        
        // Root and pelvis
        defaults[0] = {{0, 0, 0}, Quat::identity()};
        defaults[1] = {{0, 0.95f, 0}, Quat::identity()};
        
        // Spine
        defaults[2] = {{0, 1.0f, 0}, Quat::identity()};
        defaults[3] = {{0, 1.1f, 0}, Quat::identity()};
        defaults[4] = {{0, 1.2f, 0}, Quat::identity()};
        
        // Left arm
        defaults[5] = {{-0.15f, 1.35f, 0}, Quat::identity()};
        defaults[6] = {{-0.25f, 1.35f, 0}, Quat::fromEuler(0, 0, 1.57f)};
        defaults[7] = {{-0.5f, 1.35f, 0}, Quat::identity()};
        defaults[8] = {{-0.75f, 1.35f, 0}, Quat::identity()};
        
        // Right arm
        defaults[9] = {{0.15f, 1.35f, 0}, Quat::identity()};
        defaults[10] = {{0.25f, 1.35f, 0}, Quat::fromEuler(0, 0, -1.57f)};
        defaults[11] = {{0.5f, 1.35f, 0}, Quat::identity()};
        defaults[12] = {{0.75f, 1.35f, 0}, Quat::identity()};
        
        // Neck and head
        defaults[13] = {{0, 1.4f, 0}, Quat::identity()};
        defaults[14] = {{0, 1.55f, 0}, Quat::identity()};
        
        // Left leg
        defaults[15] = {{-0.1f, 0.9f, 0}, Quat::identity()};
        defaults[16] = {{-0.1f, 0.5f, 0}, Quat::identity()};
        defaults[17] = {{-0.1f, 0.05f, 0}, Quat::identity()};
        defaults[18] = {{-0.1f, 0.0f, 0.1f}, Quat::identity()};
        
        // Right leg
        defaults[19] = {{0.1f, 0.9f, 0}, Quat::identity()};
        defaults[20] = {{0.1f, 0.5f, 0}, Quat::identity()};
        defaults[21] = {{0.1f, 0.05f, 0}, Quat::identity()};
        defaults[22] = {{0.1f, 0.0f, 0.1f}, Quat::identity()};
        
        // Add bones to skeleton and set local transforms
        int numBones = std::min(names.size(), std::min(parents.size(), defaults.size()));
        for (int i = 0; i < numBones; i++) {
            int parentIndex = (parents[i] >= 0 && parents[i] < i) ? parents[i] : -1;
            int boneIndex = skeleton.addBone(names[i], parentIndex);
            if (boneIndex >= 0) {
                skeleton.setBoneLocalTransform(boneIndex, defaults[i].position, defaults[i].rotation, Vec3(1, 1, 1));
            }
        }
    }
};

// ============================================================================
// MakeHuman Asset Manager
// ============================================================================

class MakeHumanAssetManager {
public:
    static MakeHumanAssetManager& getInstance() {
        static MakeHumanAssetManager instance;
        return instance;
    }
    
    // Set base path for MakeHuman assets
    void setAssetPath(const std::string& path) {
        assetPath_ = path;
    }
    
    const std::string& getAssetPath() const {
        return assetPath_;
    }
    
    // Check if MakeHuman assets are available
    bool hasAssets() const {
        return std::filesystem::exists(assetPath_) &&
               std::filesystem::exists(assetPath_ + "/base.obj");
    }
    
    // Get available target categories
    std::vector<std::string> getTargetCategories() const {
        std::vector<std::string> categories;
        std::string targetsPath = assetPath_ + "/targets/";
        
        if (std::filesystem::exists(targetsPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(targetsPath)) {
                if (entry.is_directory()) {
                    categories.push_back(entry.path().filename().string());
                }
            }
        }
        
        return categories;
    }
    
    // Load model from assets
    bool loadModel(BaseHumanModel& outModel) {
        return MakeHumanLoader::loadModel(assetPath_, outModel);
    }
    
    // Load specific targets by category
    std::vector<BlendShapeTarget> loadTargetCategory(const std::string& category) {
        std::string categoryPath = assetPath_ + "/targets/" + category + "/";
        return MakeHumanTargetLoader::loadTargetsFromDirectory(categoryPath);
    }
    
private:
    MakeHumanAssetManager() {
        assetPath_ = "assets/makehuman";
    }
    
    std::string assetPath_;
};

// ============================================================================
// MakeHuman Setup Instructions
// ============================================================================

inline std::string getMakeHumanSetupInstructions() {
    return R"(
=== MakeHuman Asset Setup ===

To use high-quality MakeHuman assets:

1. Download MakeHuman:
   - Visit: http://www.makehumancommunity.org/
   - Download MakeHuman (free, CC0 licensed)

2. Export base mesh:
   - Open MakeHuman
   - Create a neutral character (no modifications)
   - Export as OBJ format
   - Save as: assets/makehuman/base.obj

3. Copy target files:
   - Locate MakeHuman's data folder:
     - Windows: %USERPROFILE%/Documents/makehuman/v1py3/data/
     - macOS: ~/Documents/makehuman/v1py3/data/
     - Linux: ~/.makehuman/v1py3/data/
   - Copy the 'targets' folder to: assets/makehuman/targets/

4. Directory structure:
   assets/makehuman/
   ├── base.obj
   ├── targets/
   │   ├── macrodetails/
   │   ├── body/
   │   ├── face/
   │   └── ...
   └── textures/ (optional)

5. Restart the application

Note: MakeHuman assets are CC0 licensed (public domain).
)";
}

// ============================================================================
// Convenience Functions
// ============================================================================

inline MakeHumanAssetManager& getMakeHumanAssets() {
    return MakeHumanAssetManager::getInstance();
}

} // namespace luma
