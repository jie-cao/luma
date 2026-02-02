// Standard Rig System - Industry-compatible skeleton and facial rig definitions
// Supports: Mixamo, Unity Humanoid, VRM, Unreal Engine Mannequin
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include "engine/character/blend_shape.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <functional>

namespace luma {

// ============================================================================
// Rig Standard Enum - Different industry standards
// ============================================================================

enum class RigStandard {
    Luma,           // Our internal standard (superset)
    Mixamo,         // Adobe Mixamo naming convention
    UnityHumanoid,  // Unity's Avatar system
    VRM,            // VRM 1.0 standard for VTuber
    UnrealMannequin,// Unreal Engine mannequin
    BVH,            // BioVision Hierarchy (mocap)
    FBX_HumanIK     // Autodesk HumanIK
};

// ============================================================================
// Standard Bone Names - Our canonical bone naming
// ============================================================================

namespace StandardBones {
    // Root
    constexpr const char* Root = "root";
    
    // Spine (bottom to top)
    constexpr const char* Hips = "hips";
    constexpr const char* Spine = "spine";
    constexpr const char* Spine1 = "spine1";
    constexpr const char* Spine2 = "spine2";
    constexpr const char* Chest = "chest";
    constexpr const char* UpperChest = "upperChest";
    constexpr const char* Neck = "neck";
    constexpr const char* Head = "head";
    
    // Left Arm
    constexpr const char* LeftShoulder = "shoulder_L";
    constexpr const char* LeftUpperArm = "upperArm_L";
    constexpr const char* LeftLowerArm = "lowerArm_L";
    constexpr const char* LeftHand = "hand_L";
    
    // Right Arm
    constexpr const char* RightShoulder = "shoulder_R";
    constexpr const char* RightUpperArm = "upperArm_R";
    constexpr const char* RightLowerArm = "lowerArm_R";
    constexpr const char* RightHand = "hand_R";
    
    // Left Leg
    constexpr const char* LeftUpperLeg = "upperLeg_L";
    constexpr const char* LeftLowerLeg = "lowerLeg_L";
    constexpr const char* LeftFoot = "foot_L";
    constexpr const char* LeftToes = "toes_L";
    
    // Right Leg
    constexpr const char* RightUpperLeg = "upperLeg_R";
    constexpr const char* RightLowerLeg = "lowerLeg_R";
    constexpr const char* RightFoot = "foot_R";
    constexpr const char* RightToes = "toes_R";
    
    // Left Hand Fingers
    constexpr const char* LeftThumb1 = "thumb1_L";
    constexpr const char* LeftThumb2 = "thumb2_L";
    constexpr const char* LeftThumb3 = "thumb3_L";
    constexpr const char* LeftIndex1 = "index1_L";
    constexpr const char* LeftIndex2 = "index2_L";
    constexpr const char* LeftIndex3 = "index3_L";
    constexpr const char* LeftMiddle1 = "middle1_L";
    constexpr const char* LeftMiddle2 = "middle2_L";
    constexpr const char* LeftMiddle3 = "middle3_L";
    constexpr const char* LeftRing1 = "ring1_L";
    constexpr const char* LeftRing2 = "ring2_L";
    constexpr const char* LeftRing3 = "ring3_L";
    constexpr const char* LeftPinky1 = "pinky1_L";
    constexpr const char* LeftPinky2 = "pinky2_L";
    constexpr const char* LeftPinky3 = "pinky3_L";
    
    // Right Hand Fingers
    constexpr const char* RightThumb1 = "thumb1_R";
    constexpr const char* RightThumb2 = "thumb2_R";
    constexpr const char* RightThumb3 = "thumb3_R";
    constexpr const char* RightIndex1 = "index1_R";
    constexpr const char* RightIndex2 = "index2_R";
    constexpr const char* RightIndex3 = "index3_R";
    constexpr const char* RightMiddle1 = "middle1_R";
    constexpr const char* RightMiddle2 = "middle2_R";
    constexpr const char* RightMiddle3 = "middle3_R";
    constexpr const char* RightRing1 = "ring1_R";
    constexpr const char* RightRing2 = "ring2_R";
    constexpr const char* RightRing3 = "ring3_R";
    constexpr const char* RightPinky1 = "pinky1_R";
    constexpr const char* RightPinky2 = "pinky2_R";
    constexpr const char* RightPinky3 = "pinky3_R";
    
    // Face bones (optional, for bone-based facial animation)
    constexpr const char* Jaw = "jaw";
    constexpr const char* LeftEye = "eye_L";
    constexpr const char* RightEye = "eye_R";
    constexpr const char* LeftEyebrow = "eyebrow_L";
    constexpr const char* RightEyebrow = "eyebrow_R";
    constexpr const char* LeftEyelidUpper = "eyelidUpper_L";
    constexpr const char* LeftEyelidLower = "eyelidLower_L";
    constexpr const char* RightEyelidUpper = "eyelidUpper_R";
    constexpr const char* RightEyelidLower = "eyelidLower_R";
    constexpr const char* Tongue = "tongue";
    constexpr const char* Tongue1 = "tongue1";
    constexpr const char* Tongue2 = "tongue2";
    
    // Get all required bones for humanoid rig
    inline std::vector<std::string> getRequiredHumanoidBones() {
        return {
            Hips, Spine, Chest, Neck, Head,
            LeftUpperArm, LeftLowerArm, LeftHand,
            RightUpperArm, RightLowerArm, RightHand,
            LeftUpperLeg, LeftLowerLeg, LeftFoot,
            RightUpperLeg, RightLowerLeg, RightFoot
        };
    }
    
    // Get optional bones
    inline std::vector<std::string> getOptionalHumanoidBones() {
        return {
            Root, Spine1, Spine2, UpperChest,
            LeftShoulder, RightShoulder,
            LeftToes, RightToes,
            Jaw, LeftEye, RightEye
        };
    }
    
    // Get finger bones
    inline std::vector<std::string> getFingerBones(bool left) {
        if (left) {
            return {
                LeftThumb1, LeftThumb2, LeftThumb3,
                LeftIndex1, LeftIndex2, LeftIndex3,
                LeftMiddle1, LeftMiddle2, LeftMiddle3,
                LeftRing1, LeftRing2, LeftRing3,
                LeftPinky1, LeftPinky2, LeftPinky3
            };
        } else {
            return {
                RightThumb1, RightThumb2, RightThumb3,
                RightIndex1, RightIndex2, RightIndex3,
                RightMiddle1, RightMiddle2, RightMiddle3,
                RightRing1, RightRing2, RightRing3,
                RightPinky1, RightPinky2, RightPinky3
            };
        }
    }
}

// ============================================================================
// Bone Mapping - Maps between different rig standards
// ============================================================================

struct BoneMapping {
    std::string lumaBone;       // Our standard name
    std::string mixamoBone;     // Mixamo name
    std::string unityBone;      // Unity Humanoid name
    std::string vrmBone;        // VRM bone name
    std::string unrealBone;     // Unreal name
    
    // Get name for specific standard
    std::string getNameForStandard(RigStandard standard) const {
        switch (standard) {
            case RigStandard::Mixamo: return mixamoBone;
            case RigStandard::UnityHumanoid: return unityBone;
            case RigStandard::VRM: return vrmBone;
            case RigStandard::UnrealMannequin: return unrealBone;
            default: return lumaBone;
        }
    }
};

// ============================================================================
// Standard Bone Mapping Table
// ============================================================================

class BoneMappingTable {
public:
    static BoneMappingTable& getInstance() {
        static BoneMappingTable instance;
        return instance;
    }
    
    const BoneMapping* getMapping(const std::string& lumaBone) const {
        auto it = mappings_.find(lumaBone);
        return (it != mappings_.end()) ? &it->second : nullptr;
    }
    
    // Convert bone name from one standard to another
    std::string convertBoneName(const std::string& name, 
                                RigStandard fromStandard,
                                RigStandard toStandard) const {
        // Find the luma name first
        std::string lumaName = name;
        if (fromStandard != RigStandard::Luma) {
            lumaName = findLumaName(name, fromStandard);
        }
        
        if (lumaName.empty()) return name;  // Unknown bone
        
        auto it = mappings_.find(lumaName);
        if (it == mappings_.end()) return name;
        
        return it->second.getNameForStandard(toStandard);
    }
    
    // Find our standard name from external name
    std::string findLumaName(const std::string& externalName, RigStandard standard) const {
        for (const auto& [luma, mapping] : mappings_) {
            std::string extName = mapping.getNameForStandard(standard);
            if (extName == externalName) {
                return luma;
            }
        }
        return "";
    }
    
private:
    BoneMappingTable() {
        initializeMappings();
    }
    
    void initializeMappings() {
        // Format: {luma, mixamo, unity, vrm, unreal}
        
        // Spine chain
        addMapping({"hips", "mixamorig:Hips", "Hips", "hips", "pelvis"});
        addMapping({"spine", "mixamorig:Spine", "Spine", "spine", "spine_01"});
        addMapping({"spine1", "mixamorig:Spine1", "Spine", "spine", "spine_02"});
        addMapping({"spine2", "mixamorig:Spine2", "Chest", "chest", "spine_03"});
        addMapping({"chest", "mixamorig:Spine2", "Chest", "chest", "spine_03"});
        addMapping({"upperChest", "mixamorig:Spine3", "UpperChest", "upperChest", "spine_04"});
        addMapping({"neck", "mixamorig:Neck", "Neck", "neck", "neck_01"});
        addMapping({"head", "mixamorig:Head", "Head", "head", "head"});
        
        // Left Arm
        addMapping({"shoulder_L", "mixamorig:LeftShoulder", "LeftShoulder", "leftShoulder", "clavicle_l"});
        addMapping({"upperArm_L", "mixamorig:LeftArm", "LeftUpperArm", "leftUpperArm", "upperarm_l"});
        addMapping({"lowerArm_L", "mixamorig:LeftForeArm", "LeftLowerArm", "leftLowerArm", "lowerarm_l"});
        addMapping({"hand_L", "mixamorig:LeftHand", "LeftHand", "leftHand", "hand_l"});
        
        // Right Arm
        addMapping({"shoulder_R", "mixamorig:RightShoulder", "RightShoulder", "rightShoulder", "clavicle_r"});
        addMapping({"upperArm_R", "mixamorig:RightArm", "RightUpperArm", "rightUpperArm", "upperarm_r"});
        addMapping({"lowerArm_R", "mixamorig:RightForeArm", "RightLowerArm", "rightLowerArm", "lowerarm_r"});
        addMapping({"hand_R", "mixamorig:RightHand", "RightHand", "rightHand", "hand_r"});
        
        // Left Leg
        addMapping({"upperLeg_L", "mixamorig:LeftUpLeg", "LeftUpperLeg", "leftUpperLeg", "thigh_l"});
        addMapping({"lowerLeg_L", "mixamorig:LeftLeg", "LeftLowerLeg", "leftLowerLeg", "calf_l"});
        addMapping({"foot_L", "mixamorig:LeftFoot", "LeftFoot", "leftFoot", "foot_l"});
        addMapping({"toes_L", "mixamorig:LeftToeBase", "LeftToes", "leftToes", "ball_l"});
        
        // Right Leg
        addMapping({"upperLeg_R", "mixamorig:RightUpLeg", "RightUpperLeg", "rightUpperLeg", "thigh_r"});
        addMapping({"lowerLeg_R", "mixamorig:RightLeg", "RightLowerLeg", "rightLowerLeg", "calf_r"});
        addMapping({"foot_R", "mixamorig:RightFoot", "RightFoot", "rightFoot", "foot_r"});
        addMapping({"toes_R", "mixamorig:RightToeBase", "RightToes", "rightToes", "ball_r"});
        
        // Left Fingers
        addMapping({"thumb1_L", "mixamorig:LeftHandThumb1", "Left Thumb Proximal", "leftThumbProximal", "thumb_01_l"});
        addMapping({"thumb2_L", "mixamorig:LeftHandThumb2", "Left Thumb Intermediate", "leftThumbIntermediate", "thumb_02_l"});
        addMapping({"thumb3_L", "mixamorig:LeftHandThumb3", "Left Thumb Distal", "leftThumbDistal", "thumb_03_l"});
        addMapping({"index1_L", "mixamorig:LeftHandIndex1", "Left Index Proximal", "leftIndexProximal", "index_01_l"});
        addMapping({"index2_L", "mixamorig:LeftHandIndex2", "Left Index Intermediate", "leftIndexIntermediate", "index_02_l"});
        addMapping({"index3_L", "mixamorig:LeftHandIndex3", "Left Index Distal", "leftIndexDistal", "index_03_l"});
        addMapping({"middle1_L", "mixamorig:LeftHandMiddle1", "Left Middle Proximal", "leftMiddleProximal", "middle_01_l"});
        addMapping({"middle2_L", "mixamorig:LeftHandMiddle2", "Left Middle Intermediate", "leftMiddleIntermediate", "middle_02_l"});
        addMapping({"middle3_L", "mixamorig:LeftHandMiddle3", "Left Middle Distal", "leftMiddleDistal", "middle_03_l"});
        addMapping({"ring1_L", "mixamorig:LeftHandRing1", "Left Ring Proximal", "leftRingProximal", "ring_01_l"});
        addMapping({"ring2_L", "mixamorig:LeftHandRing2", "Left Ring Intermediate", "leftRingIntermediate", "ring_02_l"});
        addMapping({"ring3_L", "mixamorig:LeftHandRing3", "Left Ring Distal", "leftRingDistal", "ring_03_l"});
        addMapping({"pinky1_L", "mixamorig:LeftHandPinky1", "Left Little Proximal", "leftLittleProximal", "pinky_01_l"});
        addMapping({"pinky2_L", "mixamorig:LeftHandPinky2", "Left Little Intermediate", "leftLittleIntermediate", "pinky_02_l"});
        addMapping({"pinky3_L", "mixamorig:LeftHandPinky3", "Left Little Distal", "leftLittleDistal", "pinky_03_l"});
        
        // Right Fingers
        addMapping({"thumb1_R", "mixamorig:RightHandThumb1", "Right Thumb Proximal", "rightThumbProximal", "thumb_01_r"});
        addMapping({"thumb2_R", "mixamorig:RightHandThumb2", "Right Thumb Intermediate", "rightThumbIntermediate", "thumb_02_r"});
        addMapping({"thumb3_R", "mixamorig:RightHandThumb3", "Right Thumb Distal", "rightThumbDistal", "thumb_03_r"});
        addMapping({"index1_R", "mixamorig:RightHandIndex1", "Right Index Proximal", "rightIndexProximal", "index_01_r"});
        addMapping({"index2_R", "mixamorig:RightHandIndex2", "Right Index Intermediate", "rightIndexIntermediate", "index_02_r"});
        addMapping({"index3_R", "mixamorig:RightHandIndex3", "Right Index Distal", "rightIndexDistal", "index_03_r"});
        addMapping({"middle1_R", "mixamorig:RightHandMiddle1", "Right Middle Proximal", "rightMiddleProximal", "middle_01_r"});
        addMapping({"middle2_R", "mixamorig:RightHandMiddle2", "Right Middle Intermediate", "rightMiddleIntermediate", "middle_02_r"});
        addMapping({"middle3_R", "mixamorig:RightHandMiddle3", "Right Middle Distal", "rightMiddleDistal", "middle_03_r"});
        addMapping({"ring1_R", "mixamorig:RightHandRing1", "Right Ring Proximal", "rightRingProximal", "ring_01_r"});
        addMapping({"ring2_R", "mixamorig:RightHandRing2", "Right Ring Intermediate", "rightRingIntermediate", "ring_02_r"});
        addMapping({"ring3_R", "mixamorig:RightHandRing3", "Right Ring Distal", "rightRingDistal", "ring_03_r"});
        addMapping({"pinky1_R", "mixamorig:RightHandPinky1", "Right Little Proximal", "rightLittleProximal", "pinky_01_r"});
        addMapping({"pinky2_R", "mixamorig:RightHandPinky2", "Right Little Intermediate", "rightLittleIntermediate", "pinky_02_r"});
        addMapping({"pinky3_R", "mixamorig:RightHandPinky3", "Right Little Distal", "rightLittleDistal", "pinky_03_r"});
        
        // Face
        addMapping({"jaw", "mixamorig:Jaw", "Jaw", "jaw", "jaw"});
        addMapping({"eye_L", "mixamorig:LeftEye", "LeftEye", "leftEye", "eye_l"});
        addMapping({"eye_R", "mixamorig:RightEye", "RightEye", "rightEye", "eye_r"});
    }
    
    void addMapping(const BoneMapping& mapping) {
        mappings_[mapping.lumaBone] = mapping;
    }
    
    std::unordered_map<std::string, BoneMapping> mappings_;
};

// ============================================================================
// Standard Humanoid Rig Generator
// ============================================================================

struct HumanoidRigParams {
    float height = 1.8f;
    float armSpan = 1.0f;      // Relative to height
    float legRatio = 0.5f;     // Leg length ratio to height
    float shoulderWidth = 0.3f; // Relative to height
    float hipWidth = 0.2f;
    bool includeFingers = true;
    bool includeToes = true;
    bool includeFaceBones = true;
};

class StandardHumanoidRig {
public:
    // Create a standard humanoid skeleton
    static Skeleton createSkeleton(const HumanoidRigParams& params = {}) {
        Skeleton skeleton;
        float h = params.height;
        
        // === Root ===
        int root = skeleton.addBone(StandardBones::Root, -1);
        skeleton.setBoneLocalTransform(root, Vec3(0, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // === Spine Chain ===
        int hips = skeleton.addBone(StandardBones::Hips, root);
        skeleton.setBoneLocalTransform(hips, Vec3(0, h * 0.53f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine = skeleton.addBone(StandardBones::Spine, hips);
        skeleton.setBoneLocalTransform(spine, Vec3(0, h * 0.06f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine1 = skeleton.addBone(StandardBones::Spine1, spine);
        skeleton.setBoneLocalTransform(spine1, Vec3(0, h * 0.06f, 0), Quat(), Vec3(1, 1, 1));
        
        int spine2 = skeleton.addBone(StandardBones::Spine2, spine1);
        skeleton.setBoneLocalTransform(spine2, Vec3(0, h * 0.06f, 0), Quat(), Vec3(1, 1, 1));
        
        int chest = skeleton.addBone(StandardBones::Chest, spine2);
        skeleton.setBoneLocalTransform(chest, Vec3(0, h * 0.05f, 0), Quat(), Vec3(1, 1, 1));
        
        int neck = skeleton.addBone(StandardBones::Neck, chest);
        skeleton.setBoneLocalTransform(neck, Vec3(0, h * 0.06f, 0), Quat(), Vec3(1, 1, 1));
        
        int head = skeleton.addBone(StandardBones::Head, neck);
        skeleton.setBoneLocalTransform(head, Vec3(0, h * 0.04f, 0), Quat(), Vec3(1, 1, 1));
        
        // === Left Arm ===
        float shoulderOffset = h * params.shoulderWidth * 0.5f;
        
        int leftShoulder = skeleton.addBone(StandardBones::LeftShoulder, chest);
        skeleton.setBoneLocalTransform(leftShoulder, Vec3(-shoulderOffset * 0.3f, h * 0.02f, 0), Quat(), Vec3(1, 1, 1));
        
        int leftUpperArm = skeleton.addBone(StandardBones::LeftUpperArm, leftShoulder);
        skeleton.setBoneLocalTransform(leftUpperArm, Vec3(-shoulderOffset * 0.7f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int leftLowerArm = skeleton.addBone(StandardBones::LeftLowerArm, leftUpperArm);
        skeleton.setBoneLocalTransform(leftLowerArm, Vec3(-h * 0.15f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int leftHand = skeleton.addBone(StandardBones::LeftHand, leftLowerArm);
        skeleton.setBoneLocalTransform(leftHand, Vec3(-h * 0.13f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // === Right Arm ===
        int rightShoulder = skeleton.addBone(StandardBones::RightShoulder, chest);
        skeleton.setBoneLocalTransform(rightShoulder, Vec3(shoulderOffset * 0.3f, h * 0.02f, 0), Quat(), Vec3(1, 1, 1));
        
        int rightUpperArm = skeleton.addBone(StandardBones::RightUpperArm, rightShoulder);
        skeleton.setBoneLocalTransform(rightUpperArm, Vec3(shoulderOffset * 0.7f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int rightLowerArm = skeleton.addBone(StandardBones::RightLowerArm, rightUpperArm);
        skeleton.setBoneLocalTransform(rightLowerArm, Vec3(h * 0.15f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int rightHand = skeleton.addBone(StandardBones::RightHand, rightLowerArm);
        skeleton.setBoneLocalTransform(rightHand, Vec3(h * 0.13f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // === Left Leg ===
        float hipOffset = h * params.hipWidth * 0.5f;
        
        int leftUpperLeg = skeleton.addBone(StandardBones::LeftUpperLeg, hips);
        skeleton.setBoneLocalTransform(leftUpperLeg, Vec3(-hipOffset, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int leftLowerLeg = skeleton.addBone(StandardBones::LeftLowerLeg, leftUpperLeg);
        skeleton.setBoneLocalTransform(leftLowerLeg, Vec3(0, -h * 0.24f, 0), Quat(), Vec3(1, 1, 1));
        
        int leftFoot = skeleton.addBone(StandardBones::LeftFoot, leftLowerLeg);
        skeleton.setBoneLocalTransform(leftFoot, Vec3(0, -h * 0.24f, 0), Quat(), Vec3(1, 1, 1));
        
        if (params.includeToes) {
            int leftToes = skeleton.addBone(StandardBones::LeftToes, leftFoot);
            skeleton.setBoneLocalTransform(leftToes, Vec3(0, -h * 0.02f, h * 0.08f), Quat(), Vec3(1, 1, 1));
        }
        
        // === Right Leg ===
        int rightUpperLeg = skeleton.addBone(StandardBones::RightUpperLeg, hips);
        skeleton.setBoneLocalTransform(rightUpperLeg, Vec3(hipOffset, 0, 0), Quat(), Vec3(1, 1, 1));
        
        int rightLowerLeg = skeleton.addBone(StandardBones::RightLowerLeg, rightUpperLeg);
        skeleton.setBoneLocalTransform(rightLowerLeg, Vec3(0, -h * 0.24f, 0), Quat(), Vec3(1, 1, 1));
        
        int rightFoot = skeleton.addBone(StandardBones::RightFoot, rightLowerLeg);
        skeleton.setBoneLocalTransform(rightFoot, Vec3(0, -h * 0.24f, 0), Quat(), Vec3(1, 1, 1));
        
        if (params.includeToes) {
            int rightToes = skeleton.addBone(StandardBones::RightToes, rightFoot);
            skeleton.setBoneLocalTransform(rightToes, Vec3(0, -h * 0.02f, h * 0.08f), Quat(), Vec3(1, 1, 1));
        }
        
        // === Fingers ===
        if (params.includeFingers) {
            addFingers(skeleton, leftHand, h, true);
            addFingers(skeleton, rightHand, h, false);
        }
        
        // === Face Bones ===
        if (params.includeFaceBones) {
            addFaceBones(skeleton, head, h);
        }
        
        return skeleton;
    }
    
private:
    static void addFingers(Skeleton& skeleton, int handBone, float height, bool isLeft) {
        float h = height;
        float sign = isLeft ? -1.0f : 1.0f;
        
        const char* thumb1 = isLeft ? StandardBones::LeftThumb1 : StandardBones::RightThumb1;
        const char* thumb2 = isLeft ? StandardBones::LeftThumb2 : StandardBones::RightThumb2;
        const char* thumb3 = isLeft ? StandardBones::LeftThumb3 : StandardBones::RightThumb3;
        const char* index1 = isLeft ? StandardBones::LeftIndex1 : StandardBones::RightIndex1;
        const char* index2 = isLeft ? StandardBones::LeftIndex2 : StandardBones::RightIndex2;
        const char* index3 = isLeft ? StandardBones::LeftIndex3 : StandardBones::RightIndex3;
        const char* middle1 = isLeft ? StandardBones::LeftMiddle1 : StandardBones::RightMiddle1;
        const char* middle2 = isLeft ? StandardBones::LeftMiddle2 : StandardBones::RightMiddle2;
        const char* middle3 = isLeft ? StandardBones::LeftMiddle3 : StandardBones::RightMiddle3;
        const char* ring1 = isLeft ? StandardBones::LeftRing1 : StandardBones::RightRing1;
        const char* ring2 = isLeft ? StandardBones::LeftRing2 : StandardBones::RightRing2;
        const char* ring3 = isLeft ? StandardBones::LeftRing3 : StandardBones::RightRing3;
        const char* pinky1 = isLeft ? StandardBones::LeftPinky1 : StandardBones::RightPinky1;
        const char* pinky2 = isLeft ? StandardBones::LeftPinky2 : StandardBones::RightPinky2;
        const char* pinky3 = isLeft ? StandardBones::LeftPinky3 : StandardBones::RightPinky3;
        
        // Thumb (different angle)
        int t1 = skeleton.addBone(thumb1, handBone);
        skeleton.setBoneLocalTransform(t1, Vec3(sign * h * 0.015f, -h * 0.01f, h * 0.015f), Quat(), Vec3(1, 1, 1));
        int t2 = skeleton.addBone(thumb2, t1);
        skeleton.setBoneLocalTransform(t2, Vec3(sign * h * 0.018f, 0, h * 0.01f), Quat(), Vec3(1, 1, 1));
        int t3 = skeleton.addBone(thumb3, t2);
        skeleton.setBoneLocalTransform(t3, Vec3(sign * h * 0.015f, 0, h * 0.008f), Quat(), Vec3(1, 1, 1));
        
        // Index finger
        int i1 = skeleton.addBone(index1, handBone);
        skeleton.setBoneLocalTransform(i1, Vec3(sign * h * 0.045f, 0, h * 0.01f), Quat(), Vec3(1, 1, 1));
        int i2 = skeleton.addBone(index2, i1);
        skeleton.setBoneLocalTransform(i2, Vec3(sign * h * 0.022f, 0, 0), Quat(), Vec3(1, 1, 1));
        int i3 = skeleton.addBone(index3, i2);
        skeleton.setBoneLocalTransform(i3, Vec3(sign * h * 0.015f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Middle finger
        int m1 = skeleton.addBone(middle1, handBone);
        skeleton.setBoneLocalTransform(m1, Vec3(sign * h * 0.048f, 0, 0), Quat(), Vec3(1, 1, 1));
        int m2 = skeleton.addBone(middle2, m1);
        skeleton.setBoneLocalTransform(m2, Vec3(sign * h * 0.025f, 0, 0), Quat(), Vec3(1, 1, 1));
        int m3 = skeleton.addBone(middle3, m2);
        skeleton.setBoneLocalTransform(m3, Vec3(sign * h * 0.018f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Ring finger
        int r1 = skeleton.addBone(ring1, handBone);
        skeleton.setBoneLocalTransform(r1, Vec3(sign * h * 0.045f, 0, -h * 0.008f), Quat(), Vec3(1, 1, 1));
        int r2 = skeleton.addBone(ring2, r1);
        skeleton.setBoneLocalTransform(r2, Vec3(sign * h * 0.022f, 0, 0), Quat(), Vec3(1, 1, 1));
        int r3 = skeleton.addBone(ring3, r2);
        skeleton.setBoneLocalTransform(r3, Vec3(sign * h * 0.015f, 0, 0), Quat(), Vec3(1, 1, 1));
        
        // Pinky finger
        int p1 = skeleton.addBone(pinky1, handBone);
        skeleton.setBoneLocalTransform(p1, Vec3(sign * h * 0.04f, 0, -h * 0.015f), Quat(), Vec3(1, 1, 1));
        int p2 = skeleton.addBone(pinky2, p1);
        skeleton.setBoneLocalTransform(p2, Vec3(sign * h * 0.018f, 0, 0), Quat(), Vec3(1, 1, 1));
        int p3 = skeleton.addBone(pinky3, p2);
        skeleton.setBoneLocalTransform(p3, Vec3(sign * h * 0.012f, 0, 0), Quat(), Vec3(1, 1, 1));
    }
    
    static void addFaceBones(Skeleton& skeleton, int headBone, float height) {
        float h = height;
        
        // Jaw
        int jaw = skeleton.addBone(StandardBones::Jaw, headBone);
        skeleton.setBoneLocalTransform(jaw, Vec3(0, h * 0.03f, h * 0.04f), Quat(), Vec3(1, 1, 1));
        
        // Eyes
        int leftEye = skeleton.addBone(StandardBones::LeftEye, headBone);
        skeleton.setBoneLocalTransform(leftEye, Vec3(-h * 0.02f, h * 0.06f, h * 0.05f), Quat(), Vec3(1, 1, 1));
        
        int rightEye = skeleton.addBone(StandardBones::RightEye, headBone);
        skeleton.setBoneLocalTransform(rightEye, Vec3(h * 0.02f, h * 0.06f, h * 0.05f), Quat(), Vec3(1, 1, 1));
        
        // Eyebrows
        int leftBrow = skeleton.addBone(StandardBones::LeftEyebrow, headBone);
        skeleton.setBoneLocalTransform(leftBrow, Vec3(-h * 0.025f, h * 0.075f, h * 0.045f), Quat(), Vec3(1, 1, 1));
        
        int rightBrow = skeleton.addBone(StandardBones::RightEyebrow, headBone);
        skeleton.setBoneLocalTransform(rightBrow, Vec3(h * 0.025f, h * 0.075f, h * 0.045f), Quat(), Vec3(1, 1, 1));
        
        // Eyelids
        int leftUpperLid = skeleton.addBone(StandardBones::LeftEyelidUpper, leftEye);
        skeleton.setBoneLocalTransform(leftUpperLid, Vec3(0, h * 0.008f, h * 0.01f), Quat(), Vec3(1, 1, 1));
        
        int leftLowerLid = skeleton.addBone(StandardBones::LeftEyelidLower, leftEye);
        skeleton.setBoneLocalTransform(leftLowerLid, Vec3(0, -h * 0.005f, h * 0.01f), Quat(), Vec3(1, 1, 1));
        
        int rightUpperLid = skeleton.addBone(StandardBones::RightEyelidUpper, rightEye);
        skeleton.setBoneLocalTransform(rightUpperLid, Vec3(0, h * 0.008f, h * 0.01f), Quat(), Vec3(1, 1, 1));
        
        int rightLowerLid = skeleton.addBone(StandardBones::RightEyelidLower, rightEye);
        skeleton.setBoneLocalTransform(rightLowerLid, Vec3(0, -h * 0.005f, h * 0.01f), Quat(), Vec3(1, 1, 1));
        
        // Tongue
        int tongue = skeleton.addBone(StandardBones::Tongue, jaw);
        skeleton.setBoneLocalTransform(tongue, Vec3(0, h * 0.005f, h * 0.02f), Quat(), Vec3(1, 1, 1));
        
        int tongue1 = skeleton.addBone(StandardBones::Tongue1, tongue);
        skeleton.setBoneLocalTransform(tongue1, Vec3(0, 0, h * 0.015f), Quat(), Vec3(1, 1, 1));
        
        int tongue2 = skeleton.addBone(StandardBones::Tongue2, tongue1);
        skeleton.setBoneLocalTransform(tongue2, Vec3(0, 0, h * 0.012f), Quat(), Vec3(1, 1, 1));
    }
};

// ============================================================================
// Rig Validator - Check if a skeleton is valid for animation retargeting
// ============================================================================

struct RigValidationResult {
    bool isValid = false;
    bool isHumanoid = false;
    std::vector<std::string> missingRequiredBones;
    std::vector<std::string> missingOptionalBones;
    std::vector<std::string> warnings;
    RigStandard detectedStandard = RigStandard::Luma;
    
    std::string getSummary() const {
        std::string result;
        result += "Valid: " + std::string(isValid ? "Yes" : "No") + "\n";
        result += "Humanoid: " + std::string(isHumanoid ? "Yes" : "No") + "\n";
        
        if (!missingRequiredBones.empty()) {
            result += "Missing required bones:\n";
            for (const auto& b : missingRequiredBones) {
                result += "  - " + b + "\n";
            }
        }
        
        if (!warnings.empty()) {
            result += "Warnings:\n";
            for (const auto& w : warnings) {
                result += "  - " + w + "\n";
            }
        }
        
        return result;
    }
};

class RigValidator {
public:
    static RigValidationResult validate(const Skeleton& skeleton, RigStandard targetStandard = RigStandard::Luma) {
        RigValidationResult result;
        auto& mapping = BoneMappingTable::getInstance();
        
        // Check for required humanoid bones
        auto requiredBones = StandardBones::getRequiredHumanoidBones();
        for (const auto& boneName : requiredBones) {
            // Try to find with our naming
            int idx = skeleton.findBoneByName(boneName);
            
            // Also try common alternative names
            if (idx < 0) {
                std::string mixamoName = mapping.convertBoneName(boneName, RigStandard::Luma, RigStandard::Mixamo);
                idx = skeleton.findBoneByName(mixamoName);
            }
            
            if (idx < 0) {
                result.missingRequiredBones.push_back(boneName);
            }
        }
        
        // Check optional bones
        auto optionalBones = StandardBones::getOptionalHumanoidBones();
        for (const auto& boneName : optionalBones) {
            int idx = skeleton.findBoneByName(boneName);
            if (idx < 0) {
                std::string mixamoName = mapping.convertBoneName(boneName, RigStandard::Luma, RigStandard::Mixamo);
                idx = skeleton.findBoneByName(mixamoName);
            }
            if (idx < 0) {
                result.missingOptionalBones.push_back(boneName);
            }
        }
        
        // Determine validity
        result.isHumanoid = result.missingRequiredBones.empty();
        result.isValid = result.isHumanoid;
        
        // Detect standard
        result.detectedStandard = detectRigStandard(skeleton);
        
        // Add warnings
        if (skeleton.getBoneCount() < 15) {
            result.warnings.push_back("Low bone count - may lack detail");
        }
        
        if (skeleton.getBoneCount() > 200) {
            result.warnings.push_back("High bone count - may impact performance");
        }
        
        return result;
    }
    
    static RigStandard detectRigStandard(const Skeleton& skeleton) {
        // Check for Mixamo naming
        if (skeleton.findBoneByName("mixamorig:Hips") >= 0 ||
            skeleton.findBoneByName("mixamorig:Spine") >= 0) {
            return RigStandard::Mixamo;
        }
        
        // Check for Unreal naming
        if (skeleton.findBoneByName("pelvis") >= 0 &&
            skeleton.findBoneByName("spine_01") >= 0) {
            return RigStandard::UnrealMannequin;
        }
        
        // Check for VRM naming
        if (skeleton.findBoneByName("leftUpperArm") >= 0 ||
            skeleton.findBoneByName("rightUpperArm") >= 0) {
            return RigStandard::VRM;
        }
        
        // Default to our standard
        return RigStandard::Luma;
    }
};

// ============================================================================
// Skeleton Converter - Convert between rig standards
// ============================================================================

class SkeletonConverter {
public:
    // Convert skeleton bone names to target standard
    static Skeleton convertToStandard(const Skeleton& source, RigStandard targetStandard) {
        Skeleton result;
        auto& mapping = BoneMappingTable::getInstance();
        
        RigStandard sourceStandard = RigValidator::detectRigStandard(source);
        
        for (int i = 0; i < source.getBoneCount(); i++) {
            const Bone* srcBone = source.getBone(i);
            if (!srcBone) continue;
            
            // Convert bone name
            std::string newName = mapping.convertBoneName(
                srcBone->name, sourceStandard, targetStandard);
            
            // Find parent (by index, preserved)
            int parentIndex = srcBone->parentIndex;
            
            int newIndex = result.addBone(newName, parentIndex);
            
            // Copy transforms
            result.setBoneLocalTransform(
                newIndex,
                srcBone->localPosition,
                srcBone->localRotation,
                srcBone->localScale
            );
            
            result.setInverseBindMatrix(newIndex, srcBone->inverseBindMatrix);
        }
        
        return result;
    }
    
    // Create a mapping from source skeleton to target skeleton for animation retargeting
    static std::unordered_map<int, int> createRetargetMap(
        const Skeleton& source, 
        const Skeleton& target)
    {
        std::unordered_map<int, int> mapping;
        auto& boneMapping = BoneMappingTable::getInstance();
        
        RigStandard sourceStd = RigValidator::detectRigStandard(source);
        RigStandard targetStd = RigValidator::detectRigStandard(target);
        
        for (int i = 0; i < source.getBoneCount(); i++) {
            const Bone* srcBone = source.getBone(i);
            if (!srcBone) continue;
            
            // Convert source bone name to Luma standard
            std::string lumaName = boneMapping.findLumaName(srcBone->name, sourceStd);
            if (lumaName.empty()) lumaName = srcBone->name;
            
            // Convert Luma name to target standard
            std::string targetName = boneMapping.convertBoneName(lumaName, RigStandard::Luma, targetStd);
            
            // Find in target skeleton
            int targetIdx = target.findBoneByName(targetName);
            if (targetIdx < 0) {
                // Try with Luma name directly
                targetIdx = target.findBoneByName(lumaName);
            }
            
            if (targetIdx >= 0) {
                mapping[i] = targetIdx;
            }
        }
        
        return mapping;
    }
};

}  // namespace luma
