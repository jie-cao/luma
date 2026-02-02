// Pose Editor - Manual bone manipulation for posing characters
// Interactive skeleton editing with IK support
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace luma {

// ============================================================================
// Bone Transform Data
// ============================================================================

struct BonePoseData {
    std::string boneName;
    Vec3 localPosition;     // Offset from bind pose
    Quat localRotation;     // Rotation from bind pose
    Vec3 localScale{1, 1, 1};
    
    bool positionModified = false;
    bool rotationModified = false;
    bool scaleModified = false;
    
    void reset() {
        localPosition = Vec3(0, 0, 0);
        localRotation = Quat::identity();
        localScale = Vec3(1, 1, 1);
        positionModified = false;
        rotationModified = false;
        scaleModified = false;
    }
};

// ============================================================================
// Pose - Collection of bone transforms
// ============================================================================

struct Pose {
    std::string name;
    std::string nameCN;
    std::string description;
    std::string category;
    
    std::unordered_map<std::string, BonePoseData> boneData;
    
    // Metadata
    float timestamp = 0.0f;
    std::string author;
    std::vector<std::string> tags;
    
    // Get bone data (creates if not exists)
    BonePoseData& getBone(const std::string& name) {
        if (boneData.find(name) == boneData.end()) {
            BonePoseData data;
            data.boneName = name;
            boneData[name] = data;
        }
        return boneData[name];
    }
    
    // Check if pose has bone
    bool hasBone(const std::string& name) const {
        return boneData.find(name) != boneData.end();
    }
    
    // Clear all modifications
    void clear() {
        boneData.clear();
    }
    
    // Count modified bones
    int getModifiedBoneCount() const {
        int count = 0;
        for (const auto& [name, data] : boneData) {
            if (data.rotationModified || data.positionModified || data.scaleModified) {
                count++;
            }
        }
        return count;
    }
};

// ============================================================================
// Pose Library - Built-in and custom poses
// ============================================================================

class PoseLibrary {
public:
    static PoseLibrary& getInstance() {
        static PoseLibrary instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // === Reference Poses ===
        addPose(createTPose());
        addPose(createAPose());
        addPose(createRelaxed());
        
        // === Standing Poses ===
        addPose(createStandingNeutral());
        addPose(createStandingHeroic());
        addPose(createStandingCasual());
        addPose(createContrapposto());
        
        // === Action Poses ===
        addPose(createFightingStance());
        addPose(createRunning());
        addPose(createJumping());
        addPose(createPunching());
        addPose(createKicking());
        
        // === Sitting Poses ===
        addPose(createSitting());
        addPose(createSittingCrossLegged());
        addPose(createKneeling());
        
        // === Gesture Poses ===
        addPose(createWaving());
        addPose(createPointing());
        addPose(createThinking());
        addPose(createArmsRaised());
        addPose(createArmsCrossed());
        
        initialized_ = true;
    }
    
    const Pose* getPose(const std::string& name) const {
        auto it = poses_.find(name);
        return (it != poses_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getPoseNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : poses_) {
            names.push_back(name);
        }
        return names;
    }
    
    std::vector<const Pose*> getPosesByCategory(const std::string& category) const {
        std::vector<const Pose*> result;
        for (const auto& [name, pose] : poses_) {
            if (pose.category == category) {
                result.push_back(&pose);
            }
        }
        return result;
    }
    
    std::vector<std::string> getCategories() const {
        return {"Reference", "Standing", "Action", "Sitting", "Gesture"};
    }
    
    void addPose(const Pose& pose) {
        poses_[pose.name] = pose;
    }
    
    void removePose(const std::string& name) {
        poses_.erase(name);
    }
    
private:
    PoseLibrary() { initialize(); }
    
    // === Reference Poses ===
    
    Pose createTPose() {
        Pose p;
        p.name = "t_pose";
        p.nameCN = "T字姿势";
        p.category = "Reference";
        p.description = "Standard T-Pose for rigging reference";
        
        // Arms straight out
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0, 0, -1.57f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0, 0, 1.57f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    Pose createAPose() {
        Pose p;
        p.name = "a_pose";
        p.nameCN = "A字姿势";
        p.category = "Reference";
        p.description = "A-Pose for animation reference";
        
        // Arms at 45 degrees
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0, 0, -0.78f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0, 0, 0.78f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    Pose createRelaxed() {
        Pose p;
        p.name = "relaxed";
        p.nameCN = "放松";
        p.category = "Reference";
        p.description = "Natural relaxed pose";
        
        // Arms down, slight bend
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.1f, 0.1f, -0.2f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 0.1f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.1f, -0.1f, 0.2f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -0.1f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    // === Standing Poses ===
    
    Pose createStandingNeutral() {
        Pose p;
        p.name = "standing_neutral";
        p.nameCN = "站立中立";
        p.category = "Standing";
        p.description = "Neutral standing pose";
        
        // Slight arm relaxation
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.05f, 0.1f, -0.15f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.05f, -0.1f, 0.15f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    Pose createStandingHeroic() {
        Pose p;
        p.name = "standing_heroic";
        p.nameCN = "英雄站姿";
        p.category = "Standing";
        p.description = "Heroic power pose";
        
        // Chest out, hands on hips
        p.getBone("Spine").localRotation = Quat::fromEuler(-0.1f, 0, 0);
        p.getBone("Spine").rotationModified = true;
        
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.3f, 0.5f, -0.5f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 1.2f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.3f, -0.5f, 0.5f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -1.2f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    Pose createStandingCasual() {
        Pose p;
        p.name = "standing_casual";
        p.nameCN = "随意站姿";
        p.category = "Standing";
        p.description = "Casual relaxed standing";
        
        // Weight on one leg
        p.getBone("Hips").localRotation = Quat::fromEuler(0, 0, 0.05f);
        p.getBone("Hips").rotationModified = true;
        
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(0, 0, -0.1f);
        p.getBone("LeftUpperLeg").rotationModified = true;
        
        return p;
    }
    
    Pose createContrapposto() {
        Pose p;
        p.name = "contrapposto";
        p.nameCN = "对立式";
        p.category = "Standing";
        p.description = "Classical contrapposto pose";
        
        p.getBone("Hips").localRotation = Quat::fromEuler(0, 0.1f, 0.08f);
        p.getBone("Hips").rotationModified = true;
        p.getBone("Spine").localRotation = Quat::fromEuler(0, -0.05f, -0.05f);
        p.getBone("Spine").rotationModified = true;
        
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(0.1f, 0, 0);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(0.2f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        return p;
    }
    
    // === Action Poses ===
    
    Pose createFightingStance() {
        Pose p;
        p.name = "fighting_stance";
        p.nameCN = "战斗姿势";
        p.category = "Action";
        p.description = "Ready to fight stance";
        
        // Legs apart, fists up
        p.getBone("Hips").localRotation = Quat::fromEuler(0, 0.3f, 0);
        p.getBone("Hips").rotationModified = true;
        
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(0, 0, -0.2f);
        p.getBone("LeftUpperLeg").rotationModified = true;
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(0.1f, 0, 0.1f);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(0.3f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.5f, 0.3f, -0.8f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 1.8f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.8f, -0.3f, 0.5f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -1.5f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    Pose createRunning() {
        Pose p;
        p.name = "running";
        p.nameCN = "奔跑";
        p.category = "Action";
        p.description = "Mid-run pose";
        
        // Left leg forward, right leg back
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(-0.8f, 0, 0);
        p.getBone("LeftUpperLeg").rotationModified = true;
        p.getBone("LeftLowerLeg").localRotation = Quat::fromEuler(0.5f, 0, 0);
        p.getBone("LeftLowerLeg").rotationModified = true;
        
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(0.5f, 0, 0);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(1.2f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        // Arms swinging opposite
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.6f, 0, -0.3f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 0.8f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(-0.5f, 0, 0.3f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -1.0f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        p.getBone("Spine").localRotation = Quat::fromEuler(-0.1f, 0, 0);
        p.getBone("Spine").rotationModified = true;
        
        return p;
    }
    
    Pose createJumping() {
        Pose p;
        p.name = "jumping";
        p.nameCN = "跳跃";
        p.category = "Action";
        p.description = "Mid-jump pose";
        
        // Legs tucked
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(-0.5f, 0, -0.2f);
        p.getBone("LeftUpperLeg").rotationModified = true;
        p.getBone("LeftLowerLeg").localRotation = Quat::fromEuler(0.8f, 0, 0);
        p.getBone("LeftLowerLeg").rotationModified = true;
        
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(-0.5f, 0, 0.2f);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(0.8f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        // Arms up
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(-1.2f, 0, -0.5f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(-1.2f, 0, 0.5f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    Pose createPunching() {
        Pose p;
        p.name = "punching";
        p.nameCN = "出拳";
        p.category = "Action";
        p.description = "Right punch extended";
        
        p.getBone("Hips").localRotation = Quat::fromEuler(0, -0.3f, 0);
        p.getBone("Hips").rotationModified = true;
        p.getBone("Spine").localRotation = Quat::fromEuler(0, -0.2f, 0);
        p.getBone("Spine").rotationModified = true;
        
        // Right arm punching forward
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(1.2f, -0.3f, 0.3f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -0.2f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        // Left arm pulled back
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.5f, 0.5f, -0.5f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 1.5f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        return p;
    }
    
    Pose createKicking() {
        Pose p;
        p.name = "kicking";
        p.nameCN = "踢腿";
        p.category = "Action";
        p.description = "High kick pose";
        
        // Right leg kicking high
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(-1.5f, 0, 0.1f);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(0.3f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        // Left leg supporting
        p.getBone("LeftLowerLeg").localRotation = Quat::fromEuler(0.2f, 0, 0);
        p.getBone("LeftLowerLeg").rotationModified = true;
        
        // Arms for balance
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.5f, 0.3f, -0.8f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.3f, -0.5f, 0.5f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    // === Sitting Poses ===
    
    Pose createSitting() {
        Pose p;
        p.name = "sitting";
        p.nameCN = "坐姿";
        p.category = "Sitting";
        p.description = "Basic sitting pose";
        
        // Legs bent at 90 degrees
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(-1.57f, 0, -0.1f);
        p.getBone("LeftUpperLeg").rotationModified = true;
        p.getBone("LeftLowerLeg").localRotation = Quat::fromEuler(1.57f, 0, 0);
        p.getBone("LeftLowerLeg").rotationModified = true;
        
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(-1.57f, 0, 0.1f);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(1.57f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        // Hands on knees
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.3f, 0.2f, -0.3f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 0.5f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.3f, -0.2f, 0.3f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -0.5f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    Pose createSittingCrossLegged() {
        Pose p;
        p.name = "sitting_cross_legged";
        p.nameCN = "盘腿坐";
        p.category = "Sitting";
        p.description = "Cross-legged meditation pose";
        
        // Cross-legged
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(-1.2f, 0.5f, -0.8f);
        p.getBone("LeftUpperLeg").rotationModified = true;
        p.getBone("LeftLowerLeg").localRotation = Quat::fromEuler(2.0f, 0, 0);
        p.getBone("LeftLowerLeg").rotationModified = true;
        
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(-1.2f, -0.5f, 0.8f);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(2.0f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        // Hands on knees
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.2f, 0.3f, -0.4f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.2f, -0.3f, 0.4f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    Pose createKneeling() {
        Pose p;
        p.name = "kneeling";
        p.nameCN = "跪姿";
        p.category = "Sitting";
        p.description = "Kneeling pose";
        
        p.getBone("LeftUpperLeg").localRotation = Quat::fromEuler(-1.57f, 0, 0);
        p.getBone("LeftUpperLeg").rotationModified = true;
        p.getBone("LeftLowerLeg").localRotation = Quat::fromEuler(2.8f, 0, 0);
        p.getBone("LeftLowerLeg").rotationModified = true;
        
        p.getBone("RightUpperLeg").localRotation = Quat::fromEuler(-1.57f, 0, 0);
        p.getBone("RightUpperLeg").rotationModified = true;
        p.getBone("RightLowerLeg").localRotation = Quat::fromEuler(2.8f, 0, 0);
        p.getBone("RightLowerLeg").rotationModified = true;
        
        return p;
    }
    
    // === Gesture Poses ===
    
    Pose createWaving() {
        Pose p;
        p.name = "waving";
        p.nameCN = "挥手";
        p.category = "Gesture";
        p.description = "Friendly wave";
        
        // Right arm raised waving
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(-1.8f, 0, 0.3f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -0.8f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    Pose createPointing() {
        Pose p;
        p.name = "pointing";
        p.nameCN = "指向";
        p.category = "Gesture";
        p.description = "Pointing forward";
        
        // Right arm pointing forward
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(1.3f, -0.2f, 0.3f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -0.1f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    Pose createThinking() {
        Pose p;
        p.name = "thinking";
        p.nameCN = "思考";
        p.category = "Gesture";
        p.description = "Contemplative thinking pose";
        
        // Hand on chin
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.8f, -0.3f, 0.5f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0, -2.0f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        // Left arm across body
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.3f, 0.5f, -0.3f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, 0, 1.2f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        // Head tilted
        p.getBone("Head").localRotation = Quat::fromEuler(0.1f, 0, 0.1f);
        p.getBone("Head").rotationModified = true;
        
        return p;
    }
    
    Pose createArmsRaised() {
        Pose p;
        p.name = "arms_raised";
        p.nameCN = "举手";
        p.category = "Gesture";
        p.description = "Both arms raised up";
        
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(-2.5f, 0, -0.3f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(-2.5f, 0, 0.3f);
        p.getBone("RightUpperArm").rotationModified = true;
        
        return p;
    }
    
    Pose createArmsCrossed() {
        Pose p;
        p.name = "arms_crossed";
        p.nameCN = "双臂交叉";
        p.category = "Gesture";
        p.description = "Arms folded across chest";
        
        p.getBone("LeftUpperArm").localRotation = Quat::fromEuler(0.5f, 0.8f, -0.5f);
        p.getBone("LeftUpperArm").rotationModified = true;
        p.getBone("LeftLowerArm").localRotation = Quat::fromEuler(0, -0.3f, 1.8f);
        p.getBone("LeftLowerArm").rotationModified = true;
        
        p.getBone("RightUpperArm").localRotation = Quat::fromEuler(0.5f, -0.8f, 0.5f);
        p.getBone("RightUpperArm").rotationModified = true;
        p.getBone("RightLowerArm").localRotation = Quat::fromEuler(0, 0.3f, -1.8f);
        p.getBone("RightLowerArm").rotationModified = true;
        
        return p;
    }
    
    std::unordered_map<std::string, Pose> poses_;
    bool initialized_ = false;
};

// ============================================================================
// Pose Editor - Interactive bone manipulation
// ============================================================================

class PoseEditor {
public:
    PoseEditor() = default;
    
    // Set target skeleton
    void setSkeleton(Skeleton* skeleton) {
        skeleton_ = skeleton;
        if (skeleton_) {
            buildBoneList();
        }
    }
    
    // === Selection ===
    
    void selectBone(int boneIndex) {
        selectedBoneIndex_ = boneIndex;
        if (skeleton_ && boneIndex >= 0) {
            selectedBoneName_ = skeleton_->getBone(boneIndex)->name;
        }
    }
    
    void selectBone(const std::string& boneName) {
        if (!skeleton_) return;
        selectedBoneIndex_ = skeleton_->findBoneByName(boneName);
        if (selectedBoneIndex_ >= 0) {
            selectedBoneName_ = boneName;
        }
    }
    
    void clearSelection() {
        selectedBoneIndex_ = -1;
        selectedBoneName_.clear();
    }
    
    int getSelectedBoneIndex() const { return selectedBoneIndex_; }
    const std::string& getSelectedBoneName() const { return selectedBoneName_; }
    
    // === Manipulation ===
    
    void rotateBone(const std::string& boneName, const Vec3& eulerDelta) {
        if (!skeleton_) return;
        
        Quat deltaRot = Quat::fromEuler(eulerDelta.x, eulerDelta.y, eulerDelta.z);
        
        auto& bone = currentPose_.getBone(boneName);
        bone.localRotation = (deltaRot * bone.localRotation).normalized();
        bone.rotationModified = true;
        
        applyPoseToSkeleton();
    }
    
    void setBoneRotation(const std::string& boneName, const Quat& rotation) {
        auto& bone = currentPose_.getBone(boneName);
        bone.localRotation = rotation;
        bone.rotationModified = true;
        
        applyPoseToSkeleton();
    }
    
    void setBoneRotationEuler(const std::string& boneName, const Vec3& euler) {
        setBoneRotation(boneName, Quat::fromEuler(euler.x, euler.y, euler.z));
    }
    
    void resetBone(const std::string& boneName) {
        if (currentPose_.hasBone(boneName)) {
            currentPose_.getBone(boneName).reset();
            applyPoseToSkeleton();
        }
    }
    
    void resetAllBones() {
        currentPose_.clear();
        applyPoseToSkeleton();
    }
    
    // === Pose Management ===
    
    void applyPose(const Pose& pose) {
        currentPose_ = pose;
        applyPoseToSkeleton();
    }
    
    void applyPose(const std::string& poseName) {
        auto* pose = PoseLibrary::getInstance().getPose(poseName);
        if (pose) {
            applyPose(*pose);
        }
    }
    
    const Pose& getCurrentPose() const { return currentPose_; }
    Pose& getCurrentPose() { return currentPose_; }
    
    void savePoseToLibrary(const std::string& name, const std::string& nameCN = "",
                          const std::string& category = "Custom") {
        Pose pose = currentPose_;
        pose.name = name;
        pose.nameCN = nameCN.empty() ? name : nameCN;
        pose.category = category;
        PoseLibrary::getInstance().addPose(pose);
    }
    
    // === Mirror ===
    
    void mirrorPose() {
        Pose mirrored;
        mirrored.name = currentPose_.name + "_mirrored";
        
        for (const auto& [boneName, data] : currentPose_.boneData) {
            std::string mirrorName = getMirroredBoneName(boneName);
            
            BonePoseData mirrorData = data;
            mirrorData.boneName = mirrorName;
            
            // Mirror rotation (flip X and Z rotations)
            Vec3 euler = mirrorData.localRotation.toEuler();
            euler.x = -euler.x;
            euler.z = -euler.z;
            mirrorData.localRotation = Quat::fromEuler(euler.x, euler.y, euler.z);
            
            // Mirror position
            mirrorData.localPosition.x = -mirrorData.localPosition.x;
            
            mirrored.boneData[mirrorName] = mirrorData;
        }
        
        currentPose_ = mirrored;
        applyPoseToSkeleton();
    }
    
    void copyLeftToRight() {
        for (const auto& [boneName, data] : currentPose_.boneData) {
            if (boneName.find("Left") != std::string::npos) {
                std::string rightName = boneName;
                size_t pos = rightName.find("Left");
                rightName.replace(pos, 4, "Right");
                
                BonePoseData rightData = data;
                rightData.boneName = rightName;
                
                // Mirror the rotation
                Vec3 euler = rightData.localRotation.toEuler();
                euler.y = -euler.y;
                euler.z = -euler.z;
                rightData.localRotation = Quat::fromEuler(euler.x, euler.y, euler.z);
                
                currentPose_.boneData[rightName] = rightData;
            }
        }
        applyPoseToSkeleton();
    }
    
    void copyRightToLeft() {
        for (const auto& [boneName, data] : currentPose_.boneData) {
            if (boneName.find("Right") != std::string::npos) {
                std::string leftName = boneName;
                size_t pos = leftName.find("Right");
                leftName.replace(pos, 5, "Left");
                
                BonePoseData leftData = data;
                leftData.boneName = leftName;
                
                Vec3 euler = leftData.localRotation.toEuler();
                euler.y = -euler.y;
                euler.z = -euler.z;
                leftData.localRotation = Quat::fromEuler(euler.x, euler.y, euler.z);
                
                currentPose_.boneData[leftName] = leftData;
            }
        }
        applyPoseToSkeleton();
    }
    
    // === Bone List ===
    
    const std::vector<std::string>& getBoneNames() const { return boneNames_; }
    const std::vector<std::string>& getBoneCategories() const { return boneCategories_; }
    
    std::vector<std::string> getBonesByCategory(const std::string& category) const {
        std::vector<std::string> result;
        for (const auto& name : boneNames_) {
            if (getBoneCategory(name) == category) {
                result.push_back(name);
            }
        }
        return result;
    }
    
    // === Settings ===
    
    enum class RotationSpace { Local, World };
    RotationSpace rotationSpace = RotationSpace::Local;
    
    float rotationSensitivity = 0.01f;
    bool showBoneGizmos = true;
    bool showBoneNames = false;
    
private:
    void buildBoneList() {
        boneNames_.clear();
        if (!skeleton_) return;
        
        for (int i = 0; i < skeleton_->getBoneCount(); i++) {
            boneNames_.push_back(skeleton_->getBone(i)->name);
        }
        
        boneCategories_ = {"Spine", "LeftArm", "RightArm", "LeftLeg", "RightLeg", "Head", "Other"};
    }
    
    std::string getBoneCategory(const std::string& boneName) const {
        if (boneName.find("Spine") != std::string::npos || 
            boneName.find("Hips") != std::string::npos ||
            boneName.find("Chest") != std::string::npos) {
            return "Spine";
        }
        if (boneName.find("Left") != std::string::npos) {
            if (boneName.find("Arm") != std::string::npos || 
                boneName.find("Hand") != std::string::npos ||
                boneName.find("Shoulder") != std::string::npos) {
                return "LeftArm";
            }
            if (boneName.find("Leg") != std::string::npos || 
                boneName.find("Foot") != std::string::npos) {
                return "LeftLeg";
            }
        }
        if (boneName.find("Right") != std::string::npos) {
            if (boneName.find("Arm") != std::string::npos || 
                boneName.find("Hand") != std::string::npos ||
                boneName.find("Shoulder") != std::string::npos) {
                return "RightArm";
            }
            if (boneName.find("Leg") != std::string::npos || 
                boneName.find("Foot") != std::string::npos) {
                return "RightLeg";
            }
        }
        if (boneName.find("Head") != std::string::npos || 
            boneName.find("Neck") != std::string::npos) {
            return "Head";
        }
        return "Other";
    }
    
    std::string getMirroredBoneName(const std::string& name) const {
        std::string result = name;
        
        size_t leftPos = result.find("Left");
        if (leftPos != std::string::npos) {
            result.replace(leftPos, 4, "Right");
            return result;
        }
        
        size_t rightPos = result.find("Right");
        if (rightPos != std::string::npos) {
            result.replace(rightPos, 5, "Left");
            return result;
        }
        
        return result;  // Center bone, return as-is
    }
    
    void applyPoseToSkeleton() {
        if (!skeleton_) return;
        
        for (const auto& [boneName, data] : currentPose_.boneData) {
            int boneIdx = skeleton_->findBoneByName(boneName);
            if (boneIdx >= 0) {
                Bone* bone = skeleton_->getBone(boneIdx);
                if (bone) {
                    if (data.rotationModified) {
                        bone->localRotation = data.localRotation;
                    }
                    if (data.positionModified) {
                        bone->localPosition = bone->localPosition + data.localPosition;
                    }
                    if (data.scaleModified) {
                        bone->localScale = data.localScale;
                    }
                }
            }
        }
        
        skeleton_->updateMatrices();
    }
    
    Skeleton* skeleton_ = nullptr;
    Pose currentPose_;
    
    int selectedBoneIndex_ = -1;
    std::string selectedBoneName_;
    
    std::vector<std::string> boneNames_;
    std::vector<std::string> boneCategories_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline PoseLibrary& getPoseLibrary() {
    return PoseLibrary::getInstance();
}

}  // namespace luma
