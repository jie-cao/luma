// Animation Retargeting System - Apply animations to different characters
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include "engine/animation/animation.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace luma {

// ============================================================================
// Standard Humanoid Bone Names
// ============================================================================

struct HumanoidBoneNames {
    // Root
    static constexpr const char* Hips = "Hips";
    
    // Spine
    static constexpr const char* Spine = "Spine";
    static constexpr const char* Spine1 = "Spine1";
    static constexpr const char* Spine2 = "Spine2";
    static constexpr const char* Chest = "Chest";
    static constexpr const char* Neck = "Neck";
    static constexpr const char* Head = "Head";
    
    // Left Arm
    static constexpr const char* LeftShoulder = "LeftShoulder";
    static constexpr const char* LeftArm = "LeftArm";
    static constexpr const char* LeftForeArm = "LeftForeArm";
    static constexpr const char* LeftHand = "LeftHand";
    
    // Right Arm
    static constexpr const char* RightShoulder = "RightShoulder";
    static constexpr const char* RightArm = "RightArm";
    static constexpr const char* RightForeArm = "RightForeArm";
    static constexpr const char* RightHand = "RightHand";
    
    // Left Leg
    static constexpr const char* LeftUpLeg = "LeftUpLeg";
    static constexpr const char* LeftLeg = "LeftLeg";
    static constexpr const char* LeftFoot = "LeftFoot";
    static constexpr const char* LeftToeBase = "LeftToeBase";
    
    // Right Leg
    static constexpr const char* RightUpLeg = "RightUpLeg";
    static constexpr const char* RightLeg = "RightLeg";
    static constexpr const char* RightFoot = "RightFoot";
    static constexpr const char* RightToeBase = "RightToeBase";
    
    // Fingers (optional)
    static constexpr const char* LeftThumb1 = "LeftHandThumb1";
    static constexpr const char* LeftIndex1 = "LeftHandIndex1";
    static constexpr const char* LeftMiddle1 = "LeftHandMiddle1";
    static constexpr const char* LeftRing1 = "LeftHandRing1";
    static constexpr const char* LeftPinky1 = "LeftHandPinky1";
    
    static constexpr const char* RightThumb1 = "RightHandThumb1";
    static constexpr const char* RightIndex1 = "RightHandIndex1";
    static constexpr const char* RightMiddle1 = "RightHandMiddle1";
    static constexpr const char* RightRing1 = "RightHandRing1";
    static constexpr const char* RightPinky1 = "RightHandPinky1";
};

// ============================================================================
// Humanoid Bone Mapping
// ============================================================================

struct HumanoidBoneMapping {
    // Maps source skeleton bone names to humanoid standard names
    std::unordered_map<std::string, std::string> sourceToHumanoid;
    
    // Maps humanoid standard names to target skeleton bone indices
    std::unordered_map<std::string, int> humanoidToTargetIndex;
    
    // Common name variations to try when auto-mapping
    static std::vector<std::string> getNameVariations(const std::string& humanoidName) {
        std::vector<std::string> variations;
        variations.push_back(humanoidName);
        
        // Add common variations
        if (humanoidName == "Hips") {
            variations.push_back("pelvis");
            variations.push_back("hip");
            variations.push_back("root");
        } else if (humanoidName == "Spine") {
            variations.push_back("spine01");
            variations.push_back("spine_01");
        } else if (humanoidName == "LeftArm") {
            variations.push_back("upperarm_l");
            variations.push_back("upper_arm_l");
            variations.push_back("l_upperarm");
        } else if (humanoidName == "LeftForeArm") {
            variations.push_back("forearm_l");
            variations.push_back("lowerarm_l");
            variations.push_back("l_forearm");
        }
        // Add more variations as needed
        
        // Also try lowercase
        for (size_t i = 0; i < variations.size(); i++) {
            std::string lower = variations[i];
            for (char& c : lower) c = tolower(c);
            if (lower != variations[i]) {
                variations.push_back(lower);
            }
        }
        
        return variations;
    }
};

// ============================================================================
// Animation Pose
// ============================================================================

struct AnimationPose {
    std::string name;
    std::string description;
    std::string category;  // "idle", "action", "expression", etc.
    
    // Bone rotations (quaternion) keyed by humanoid bone name
    std::unordered_map<std::string, Quat> boneRotations;
    
    // Root position offset
    Vec3 rootPosition{0, 0, 0};
    
    // Thumbnail for UI
    std::string thumbnailPath;
    
    // Apply this pose with a blend weight
    void apply(Skeleton& skeleton, const HumanoidBoneMapping& mapping, float weight = 1.0f) const {
        for (const auto& [humanoidBone, rotation] : boneRotations) {
            auto it = mapping.humanoidToTargetIndex.find(humanoidBone);
            if (it != mapping.humanoidToTargetIndex.end()) {
                int boneIndex = it->second;
                Bone* bone = skeleton.getBone(boneIndex);
                if (bone) {
                    if (weight >= 0.999f) {
                        bone->localRotation = rotation;
                    } else {
                        // Blend with current rotation
                        Quat current = bone->localRotation;
                        Quat blended = Quat::slerp(current, rotation, weight);
                        bone->localRotation = blended;
                    }
                }
            }
        }
    }
};

// ============================================================================
// Pose Library
// ============================================================================

class PoseLibrary {
public:
    static PoseLibrary& getInstance() {
        static PoseLibrary instance;
        return instance;
    }
    
    // === Pose Management ===
    
    void addPose(const AnimationPose& pose) {
        poses_[pose.name] = pose;
        categoryIndex_[pose.category].push_back(pose.name);
    }
    
    const AnimationPose* getPose(const std::string& name) const {
        auto it = poses_.find(name);
        return it != poses_.end() ? &it->second : nullptr;
    }
    
    std::vector<const AnimationPose*> getPosesByCategory(const std::string& category) const {
        std::vector<const AnimationPose*> result;
        auto it = categoryIndex_.find(category);
        if (it != categoryIndex_.end()) {
            for (const auto& name : it->second) {
                auto poseIt = poses_.find(name);
                if (poseIt != poses_.end()) {
                    result.push_back(&poseIt->second);
                }
            }
        }
        return result;
    }
    
    std::vector<std::string> getCategories() const {
        std::vector<std::string> cats;
        for (const auto& [cat, _] : categoryIndex_) {
            cats.push_back(cat);
        }
        return cats;
    }
    
    // === Default Poses ===
    
    void initializeDefaults() {
        // T-Pose (default bind pose)
        AnimationPose tPose;
        tPose.name = "t_pose";
        tPose.description = "Standard T-Pose";
        tPose.category = "bind";
        tPose.boneRotations[HumanoidBoneNames::Hips] = Quat::identity();
        tPose.boneRotations[HumanoidBoneNames::Spine] = Quat::identity();
        tPose.boneRotations[HumanoidBoneNames::LeftArm] = Quat::identity();
        tPose.boneRotations[HumanoidBoneNames::RightArm] = Quat::identity();
        addPose(tPose);
        
        // A-Pose
        AnimationPose aPose;
        aPose.name = "a_pose";
        aPose.description = "A-Pose (arms at 45 degrees)";
        aPose.category = "bind";
        aPose.boneRotations[HumanoidBoneNames::Hips] = Quat::identity();
        aPose.boneRotations[HumanoidBoneNames::LeftArm] = Quat::fromEuler(0, 0, 0.785f);  // 45 deg
        aPose.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0, 0, -0.785f);
        addPose(aPose);
        
        // Idle Pose
        AnimationPose idle;
        idle.name = "idle";
        idle.description = "Relaxed standing pose";
        idle.category = "idle";
        idle.boneRotations[HumanoidBoneNames::Hips] = Quat::identity();
        idle.boneRotations[HumanoidBoneNames::Spine] = Quat::fromEuler(0.03f, 0, 0);
        idle.boneRotations[HumanoidBoneNames::LeftArm] = Quat::fromEuler(0.1f, 0, 0.3f);
        idle.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0.1f, 0, -0.3f);
        idle.boneRotations[HumanoidBoneNames::LeftForeArm] = Quat::fromEuler(0, 0, 0.2f);
        idle.boneRotations[HumanoidBoneNames::RightForeArm] = Quat::fromEuler(0, 0, -0.2f);
        addPose(idle);
        
        // Hands on Hips
        AnimationPose handsOnHips;
        handsOnHips.name = "hands_on_hips";
        handsOnHips.description = "Hands resting on hips";
        handsOnHips.category = "idle";
        handsOnHips.boneRotations[HumanoidBoneNames::Hips] = Quat::identity();
        handsOnHips.boneRotations[HumanoidBoneNames::LeftArm] = Quat::fromEuler(0.4f, 0.3f, 0.6f);
        handsOnHips.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0.4f, -0.3f, -0.6f);
        handsOnHips.boneRotations[HumanoidBoneNames::LeftForeArm] = Quat::fromEuler(0, 0, 1.2f);
        handsOnHips.boneRotations[HumanoidBoneNames::RightForeArm] = Quat::fromEuler(0, 0, -1.2f);
        addPose(handsOnHips);
        
        // Arms Crossed
        AnimationPose armsCrossed;
        armsCrossed.name = "arms_crossed";
        armsCrossed.description = "Arms crossed in front";
        armsCrossed.category = "idle";
        armsCrossed.boneRotations[HumanoidBoneNames::Spine] = Quat::fromEuler(0.05f, 0, 0);
        armsCrossed.boneRotations[HumanoidBoneNames::LeftArm] = Quat::fromEuler(0.5f, 0.7f, 0.4f);
        armsCrossed.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0.5f, -0.7f, -0.4f);
        armsCrossed.boneRotations[HumanoidBoneNames::LeftForeArm] = Quat::fromEuler(0, 0, 1.8f);
        armsCrossed.boneRotations[HumanoidBoneNames::RightForeArm] = Quat::fromEuler(0, 0, -1.8f);
        addPose(armsCrossed);
        
        // Wave
        AnimationPose wave;
        wave.name = "wave";
        wave.description = "Waving gesture";
        wave.category = "action";
        wave.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0, 0, -2.5f);
        wave.boneRotations[HumanoidBoneNames::RightForeArm] = Quat::fromEuler(0, 0, -0.5f);
        addPose(wave);
        
        // Thinking
        AnimationPose thinking;
        thinking.name = "thinking";
        thinking.description = "Thoughtful pose";
        thinking.category = "action";
        thinking.boneRotations[HumanoidBoneNames::Head] = Quat::fromEuler(0.15f, -0.1f, 0);
        thinking.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0.6f, 0.4f, -0.2f);
        thinking.boneRotations[HumanoidBoneNames::RightForeArm] = Quat::fromEuler(0, 0, -2.0f);
        addPose(thinking);
        
        // Walking (base frame)
        AnimationPose walkBase;
        walkBase.name = "walk_base";
        walkBase.description = "Base walking pose";
        walkBase.category = "locomotion";
        walkBase.boneRotations[HumanoidBoneNames::LeftUpLeg] = Quat::fromEuler(-0.3f, 0, 0);
        walkBase.boneRotations[HumanoidBoneNames::RightUpLeg] = Quat::fromEuler(0.3f, 0, 0);
        walkBase.boneRotations[HumanoidBoneNames::LeftLeg] = Quat::fromEuler(0.2f, 0, 0);
        walkBase.boneRotations[HumanoidBoneNames::RightLeg] = Quat::fromEuler(0.1f, 0, 0);
        walkBase.boneRotations[HumanoidBoneNames::LeftArm] = Quat::fromEuler(0.2f, 0, 0.2f);
        walkBase.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(-0.2f, 0, -0.2f);
        addPose(walkBase);
        
        // Sitting
        AnimationPose sitting;
        sitting.name = "sitting";
        sitting.description = "Seated pose";
        sitting.category = "seated";
        sitting.rootPosition = Vec3(0, -0.4f, 0);
        sitting.boneRotations[HumanoidBoneNames::LeftUpLeg] = Quat::fromEuler(-1.57f, 0, 0.1f);
        sitting.boneRotations[HumanoidBoneNames::RightUpLeg] = Quat::fromEuler(-1.57f, 0, -0.1f);
        sitting.boneRotations[HumanoidBoneNames::LeftLeg] = Quat::fromEuler(1.57f, 0, 0);
        sitting.boneRotations[HumanoidBoneNames::RightLeg] = Quat::fromEuler(1.57f, 0, 0);
        sitting.boneRotations[HumanoidBoneNames::LeftArm] = Quat::fromEuler(0.3f, 0, 0.3f);
        sitting.boneRotations[HumanoidBoneNames::RightArm] = Quat::fromEuler(0.3f, 0, -0.3f);
        addPose(sitting);
    }
    
private:
    PoseLibrary() = default;
    
    std::unordered_map<std::string, AnimationPose> poses_;
    std::unordered_map<std::string, std::vector<std::string>> categoryIndex_;
};

// ============================================================================
// Animation Clip for Retargeting
// ============================================================================

struct RetargetableClip {
    std::string name;
    std::string description;
    std::string category;
    float duration = 1.0f;
    bool looping = true;
    
    // Keyframes for each humanoid bone
    struct BoneTrack {
        std::string humanoidBoneName;
        std::vector<float> times;
        std::vector<Quat> rotations;
        std::vector<Vec3> positions;  // Only for root bone
    };
    std::vector<BoneTrack> tracks;
    
    // Sample animation at time t
    AnimationPose sample(float time) const {
        AnimationPose pose;
        pose.name = name + "_sample";
        
        float t = looping ? fmod(time, duration) : std::min(time, duration);
        
        for (const auto& track : tracks) {
            if (track.times.empty()) continue;
            
            // Find keyframe
            size_t keyIndex = 0;
            for (size_t i = 0; i < track.times.size() - 1; i++) {
                if (t >= track.times[i] && t < track.times[i + 1]) {
                    keyIndex = i;
                    break;
                }
            }
            
            // Interpolate
            if (keyIndex < track.times.size() - 1 && !track.rotations.empty()) {
                float blend = (t - track.times[keyIndex]) / 
                             (track.times[keyIndex + 1] - track.times[keyIndex]);
                
                Quat rotation = Quat::slerp(
                    track.rotations[keyIndex],
                    track.rotations[keyIndex + 1],
                    blend
                );
                pose.boneRotations[track.humanoidBoneName] = rotation;
            }
        }
        
        return pose;
    }
};

// ============================================================================
// Animation Library
// ============================================================================

class AnimationLibrary {
public:
    static AnimationLibrary& getInstance() {
        static AnimationLibrary instance;
        return instance;
    }
    
    void addClip(const RetargetableClip& clip) {
        clips_[clip.name] = clip;
        categoryIndex_[clip.category].push_back(clip.name);
    }
    
    const RetargetableClip* getClip(const std::string& name) const {
        auto it = clips_.find(name);
        return it != clips_.end() ? &it->second : nullptr;
    }
    
    std::vector<const RetargetableClip*> getClipsByCategory(const std::string& category) const {
        std::vector<const RetargetableClip*> result;
        auto it = categoryIndex_.find(category);
        if (it != categoryIndex_.end()) {
            for (const auto& name : it->second) {
                auto clipIt = clips_.find(name);
                if (clipIt != clips_.end()) {
                    result.push_back(&clipIt->second);
                }
            }
        }
        return result;
    }
    
    void initializeDefaults() {
        // Simple idle breathing animation
        RetargetableClip idleBreathing;
        idleBreathing.name = "idle_breathing";
        idleBreathing.description = "Subtle breathing idle";
        idleBreathing.category = "idle";
        idleBreathing.duration = 3.0f;
        idleBreathing.looping = true;
        
        RetargetableClip::BoneTrack spineTrack;
        spineTrack.humanoidBoneName = HumanoidBoneNames::Spine;
        spineTrack.times = {0.0f, 1.5f, 3.0f};
        spineTrack.rotations = {
            Quat::fromEuler(0.02f, 0, 0),
            Quat::fromEuler(0.04f, 0, 0),
            Quat::fromEuler(0.02f, 0, 0)
        };
        idleBreathing.tracks.push_back(spineTrack);
        
        addClip(idleBreathing);
        
        // Simple wave animation
        RetargetableClip waveAnim;
        waveAnim.name = "wave_animation";
        waveAnim.description = "Waving hand";
        waveAnim.category = "gesture";
        waveAnim.duration = 2.0f;
        waveAnim.looping = false;
        
        RetargetableClip::BoneTrack armTrack;
        armTrack.humanoidBoneName = HumanoidBoneNames::RightArm;
        armTrack.times = {0.0f, 0.3f, 0.5f, 2.0f};
        armTrack.rotations = {
            Quat::fromEuler(0.1f, 0, -0.2f),
            Quat::fromEuler(0, 0, -2.5f),
            Quat::fromEuler(0, 0, -2.5f),
            Quat::fromEuler(0.1f, 0, -0.2f)
        };
        waveAnim.tracks.push_back(armTrack);
        
        RetargetableClip::BoneTrack forearmTrack;
        forearmTrack.humanoidBoneName = HumanoidBoneNames::RightForeArm;
        forearmTrack.times = {0.0f, 0.3f, 0.5f, 0.7f, 0.9f, 1.1f, 1.3f, 1.5f, 2.0f};
        forearmTrack.rotations = {
            Quat::fromEuler(0, 0, -0.2f),
            Quat::fromEuler(0, 0, -0.5f),
            Quat::fromEuler(0.2f, 0, -0.3f),
            Quat::fromEuler(-0.2f, 0, -0.5f),
            Quat::fromEuler(0.2f, 0, -0.3f),
            Quat::fromEuler(-0.2f, 0, -0.5f),
            Quat::fromEuler(0.2f, 0, -0.3f),
            Quat::fromEuler(-0.2f, 0, -0.5f),
            Quat::fromEuler(0, 0, -0.2f)
        };
        waveAnim.tracks.push_back(forearmTrack);
        
        addClip(waveAnim);
    }
    
private:
    AnimationLibrary() = default;
    
    std::unordered_map<std::string, RetargetableClip> clips_;
    std::unordered_map<std::string, std::vector<std::string>> categoryIndex_;
};

// ============================================================================
// Animation Retargeter
// ============================================================================

class CharacterAnimationRetargeter {
public:
    CharacterAnimationRetargeter() = default;
    
    // Setup with target skeleton (simplified interface)
    void setup(Skeleton& targetSkeleton) {
        setupMapping(targetSkeleton, targetSkeleton);
    }
    
    // Setup mapping from source skeleton to target
    void setupMapping(const Skeleton& sourceSkeleton, Skeleton& targetSkeleton) {
        mapping_ = HumanoidBoneMapping();
        
        // Auto-map based on bone names
        for (int i = 0; i < targetSkeleton.getBoneCount(); i++) {
            std::string boneName = targetSkeleton.getBoneName(i);
            
            // Try to match to humanoid standard
            const char* humanoidNames[] = {
                HumanoidBoneNames::Hips,
                HumanoidBoneNames::Spine,
                HumanoidBoneNames::Spine1,
                HumanoidBoneNames::Chest,
                HumanoidBoneNames::Neck,
                HumanoidBoneNames::Head,
                HumanoidBoneNames::LeftShoulder,
                HumanoidBoneNames::LeftArm,
                HumanoidBoneNames::LeftForeArm,
                HumanoidBoneNames::LeftHand,
                HumanoidBoneNames::RightShoulder,
                HumanoidBoneNames::RightArm,
                HumanoidBoneNames::RightForeArm,
                HumanoidBoneNames::RightHand,
                HumanoidBoneNames::LeftUpLeg,
                HumanoidBoneNames::LeftLeg,
                HumanoidBoneNames::LeftFoot,
                HumanoidBoneNames::RightUpLeg,
                HumanoidBoneNames::RightLeg,
                HumanoidBoneNames::RightFoot
            };
            
            for (const char* humanoid : humanoidNames) {
                auto variations = HumanoidBoneMapping::getNameVariations(humanoid);
                for (const auto& variation : variations) {
                    // Case-insensitive comparison
                    std::string lowerBone = boneName;
                    std::string lowerVar = variation;
                    for (char& c : lowerBone) c = tolower(c);
                    for (char& c : lowerVar) c = tolower(c);
                    
                    if (lowerBone.find(lowerVar) != std::string::npos ||
                        lowerVar.find(lowerBone) != std::string::npos) {
                        mapping_.humanoidToTargetIndex[humanoid] = i;
                        break;
                    }
                }
            }
        }
        
        targetSkeleton_ = &targetSkeleton;
    }
    
    // Apply a pose to the target skeleton
    void applyPose(const AnimationPose& pose, float weight = 1.0f) {
        if (targetSkeleton_) {
            pose.apply(*targetSkeleton_, mapping_, weight);
        }
    }
    
    // Apply a clip at a specific time
    void applyClip(const RetargetableClip& clip, float time, float weight = 1.0f) {
        AnimationPose pose = clip.sample(time);
        applyPose(pose, weight);
    }
    
    // Get the mapping for manual adjustment
    HumanoidBoneMapping& getMapping() { return mapping_; }
    const HumanoidBoneMapping& getMapping() const { return mapping_; }
    
private:
    HumanoidBoneMapping mapping_;
    Skeleton* targetSkeleton_ = nullptr;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline PoseLibrary& getPoseLibrary() {
    return PoseLibrary::getInstance();
}

inline AnimationLibrary& getAnimationLibrary() {
    return AnimationLibrary::getInstance();
}

} // namespace luma
