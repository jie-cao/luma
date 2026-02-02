// Animation Preview System - Preview animations on characters
// Supports: Built-in animations, Mixamo import, blend preview
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include "engine/animation/animation.h"
#include "engine/character/standard_rig.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace luma {

// ============================================================================
// Animation Clip (simplified for preview)
// ============================================================================

struct BoneKeyframe {
    float time = 0.0f;
    Vec3 position;
    Quat rotation;
    Vec3 scale{1, 1, 1};
};

struct BoneTrack {
    std::string boneName;
    std::vector<BoneKeyframe> keyframes;
    
    // Interpolate at time
    BoneKeyframe sample(float time) const {
        if (keyframes.empty()) {
            return {};
        }
        
        if (time <= keyframes.front().time) {
            return keyframes.front();
        }
        
        if (time >= keyframes.back().time) {
            return keyframes.back();
        }
        
        // Find keyframes to interpolate
        for (size_t i = 0; i < keyframes.size() - 1; i++) {
            if (time >= keyframes[i].time && time < keyframes[i + 1].time) {
                float t = (time - keyframes[i].time) / 
                          (keyframes[i + 1].time - keyframes[i].time);
                
                BoneKeyframe result;
                result.time = time;
                result.position = keyframes[i].position.lerp(keyframes[i + 1].position, t);
                result.rotation = keyframes[i].rotation.slerp(keyframes[i + 1].rotation, t);
                result.scale = keyframes[i].scale.lerp(keyframes[i + 1].scale, t);
                return result;
            }
        }
        
        return keyframes.back();
    }
};

struct AnimationClipData {
    std::string name;
    float duration = 0.0f;
    float framesPerSecond = 30.0f;
    bool loop = true;
    
    std::vector<BoneTrack> tracks;
    
    // Find track by bone name
    const BoneTrack* getTrack(const std::string& boneName) const {
        for (const auto& track : tracks) {
            if (track.boneName == boneName) {
                return &track;
            }
        }
        return nullptr;
    }
};

// ============================================================================
// Built-in Animation Generator
// ============================================================================

class ProceduralAnimationGenerator {
public:
    // Generate idle animation
    static AnimationClipData generateIdle(float duration = 2.0f) {
        AnimationClipData clip;
        clip.name = "Idle";
        clip.duration = duration;
        clip.loop = true;
        
        // Subtle breathing motion on spine
        {
            BoneTrack track;
            track.boneName = StandardBones::Spine;
            
            int numKeys = static_cast<int>(duration * 30);
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float breathe = std::sin(t * 3.14159f * 2.0f / duration) * 0.01f;
                
                BoneKeyframe kf;
                kf.time = t;
                kf.position = Vec3(0, breathe, 0);
                kf.rotation = Quat();
                kf.scale = Vec3(1, 1, 1);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        // Slight head motion
        {
            BoneTrack track;
            track.boneName = StandardBones::Head;
            
            int numKeys = static_cast<int>(duration * 30);
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float nod = std::sin(t * 3.14159f / duration) * 0.02f;
                
                BoneKeyframe kf;
                kf.time = t;
                kf.position = Vec3(0, 0, 0);
                kf.rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), nod);
                kf.scale = Vec3(1, 1, 1);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        return clip;
    }
    
    // Generate T-Pose (reference pose)
    static AnimationClipData generateTPose() {
        AnimationClipData clip;
        clip.name = "T-Pose";
        clip.duration = 0.1f;
        clip.loop = false;
        
        // Arms out to sides
        auto addBoneTrack = [&](const std::string& name, const Vec3& pos, const Quat& rot) {
            BoneTrack track;
            track.boneName = name;
            BoneKeyframe kf;
            kf.time = 0.0f;
            kf.position = pos;
            kf.rotation = rot;
            track.keyframes.push_back(kf);
            clip.tracks.push_back(track);
        };
        
        // Left arm straight out
        addBoneTrack(StandardBones::LeftUpperArm, Vec3(0, 0, 0), 
                    Quat::fromAxisAngle(Vec3(0, 0, 1), 1.5708f));  // 90 degrees
        addBoneTrack(StandardBones::LeftLowerArm, Vec3(0, 0, 0), Quat());
        
        // Right arm straight out
        addBoneTrack(StandardBones::RightUpperArm, Vec3(0, 0, 0),
                    Quat::fromAxisAngle(Vec3(0, 0, 1), -1.5708f));
        addBoneTrack(StandardBones::RightLowerArm, Vec3(0, 0, 0), Quat());
        
        return clip;
    }
    
    // Generate A-Pose (arms slightly down)
    static AnimationClipData generateAPose() {
        AnimationClipData clip;
        clip.name = "A-Pose";
        clip.duration = 0.1f;
        clip.loop = false;
        
        auto addBoneTrack = [&](const std::string& name, const Quat& rot) {
            BoneTrack track;
            track.boneName = name;
            BoneKeyframe kf;
            kf.time = 0.0f;
            kf.rotation = rot;
            track.keyframes.push_back(kf);
            clip.tracks.push_back(track);
        };
        
        // Arms at ~45 degrees
        addBoneTrack(StandardBones::LeftUpperArm, 
                    Quat::fromAxisAngle(Vec3(0, 0, 1), 0.785f));  // 45 degrees
        addBoneTrack(StandardBones::RightUpperArm,
                    Quat::fromAxisAngle(Vec3(0, 0, 1), -0.785f));
        
        return clip;
    }
    
    // Generate simple wave animation
    static AnimationClipData generateWave(float duration = 2.0f) {
        AnimationClipData clip;
        clip.name = "Wave";
        clip.duration = duration;
        clip.loop = true;
        
        // Right arm raises and waves
        {
            BoneTrack track;
            track.boneName = StandardBones::RightUpperArm;
            
            int numKeys = static_cast<int>(duration * 30);
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float raise = std::min(t / 0.5f, 1.0f);  // Raise in first 0.5s
                float angle = -1.5708f * raise;  // Raise to side
                
                BoneKeyframe kf;
                kf.time = t;
                kf.rotation = Quat::fromAxisAngle(Vec3(0, 0, 1), angle);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        // Forearm wave
        {
            BoneTrack track;
            track.boneName = StandardBones::RightLowerArm;
            
            int numKeys = static_cast<int>(duration * 30);
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float wave = 0.0f;
                if (t > 0.5f) {
                    wave = std::sin((t - 0.5f) * 10.0f) * 0.5f;  // Wave motion
                }
                
                BoneKeyframe kf;
                kf.time = t;
                kf.rotation = Quat::fromAxisAngle(Vec3(0, 1, 0), wave);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        return clip;
    }
    
    // Generate simple walk cycle
    static AnimationClipData generateWalk(float duration = 1.0f) {
        AnimationClipData clip;
        clip.name = "Walk";
        clip.duration = duration;
        clip.loop = true;
        
        float pi = 3.14159f;
        int numKeys = static_cast<int>(duration * 30);
        
        // Hips bob
        {
            BoneTrack track;
            track.boneName = StandardBones::Hips;
            
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float phase = t / duration * pi * 2.0f;
                float bob = std::sin(phase * 2.0f) * 0.02f;
                float sway = std::sin(phase) * 0.02f;
                
                BoneKeyframe kf;
                kf.time = t;
                kf.position = Vec3(sway, bob, 0);
                kf.rotation = Quat::fromAxisAngle(Vec3(0, 1, 0), std::sin(phase) * 0.05f);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        // Left leg
        {
            BoneTrack track;
            track.boneName = StandardBones::LeftUpperLeg;
            
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float phase = t / duration * pi * 2.0f;
                float swing = std::sin(phase) * 0.5f;  // Forward/back
                
                BoneKeyframe kf;
                kf.time = t;
                kf.rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), swing);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        // Right leg (opposite phase)
        {
            BoneTrack track;
            track.boneName = StandardBones::RightUpperLeg;
            
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float phase = t / duration * pi * 2.0f + pi;  // Offset by 180 degrees
                float swing = std::sin(phase) * 0.5f;
                
                BoneKeyframe kf;
                kf.time = t;
                kf.rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), swing);
                track.keyframes.push_back(kf);
            }
            clip.tracks.push_back(track);
        }
        
        // Arm swing (opposite to legs)
        {
            BoneTrack trackL, trackR;
            trackL.boneName = StandardBones::LeftUpperArm;
            trackR.boneName = StandardBones::RightUpperArm;
            
            for (int i = 0; i <= numKeys; i++) {
                float t = i / 30.0f;
                float phase = t / duration * pi * 2.0f;
                float swingL = std::sin(phase + pi) * 0.3f;  // Opposite to left leg
                float swingR = std::sin(phase) * 0.3f;       // Opposite to right leg
                
                BoneKeyframe kfL, kfR;
                kfL.time = kfR.time = t;
                kfL.rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), swingL);
                kfR.rotation = Quat::fromAxisAngle(Vec3(1, 0, 0), swingR);
                trackL.keyframes.push_back(kfL);
                trackR.keyframes.push_back(kfR);
            }
            clip.tracks.push_back(trackL);
            clip.tracks.push_back(trackR);
        }
        
        return clip;
    }
    
    // Generate simple run cycle
    static AnimationClipData generateRun(float duration = 0.6f) {
        AnimationClipData clip = generateWalk(duration);
        clip.name = "Run";
        
        // Increase amplitude of all movements
        for (auto& track : clip.tracks) {
            for (auto& kf : track.keyframes) {
                // Increase rotation angles
                // (simplified - in real implementation would decompose and scale)
            }
        }
        
        return clip;
    }
};

// ============================================================================
// Animation Library - Built-in animations
// ============================================================================

class AnimationLibrary {
public:
    static AnimationLibrary& getInstance() {
        static AnimationLibrary instance;
        return instance;
    }
    
    struct AnimationEntry {
        std::string id;
        std::string name;
        std::string category;
        AnimationClipData clip;
        bool isBuiltIn = true;
    };
    
    void initialize() {
        if (initialized_) return;
        
        // Add built-in animations
        addBuiltIn("idle", "Idle", "Basic", ProceduralAnimationGenerator::generateIdle());
        addBuiltIn("tpose", "T-Pose", "Reference", ProceduralAnimationGenerator::generateTPose());
        addBuiltIn("apose", "A-Pose", "Reference", ProceduralAnimationGenerator::generateAPose());
        addBuiltIn("wave", "Wave", "Gesture", ProceduralAnimationGenerator::generateWave());
        addBuiltIn("walk", "Walk", "Locomotion", ProceduralAnimationGenerator::generateWalk());
        addBuiltIn("run", "Run", "Locomotion", ProceduralAnimationGenerator::generateRun());
        
        initialized_ = true;
    }
    
    const AnimationEntry* getAnimation(const std::string& id) const {
        auto it = animations_.find(id);
        return (it != animations_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getAnimationIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : animations_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    std::vector<const AnimationEntry*> getAnimationsByCategory(const std::string& category) const {
        std::vector<const AnimationEntry*> result;
        for (const auto& [id, entry] : animations_) {
            if (entry.category == category) {
                result.push_back(&entry);
            }
        }
        return result;
    }
    
    std::vector<std::string> getCategories() const {
        std::vector<std::string> cats;
        for (const auto& [id, entry] : animations_) {
            if (std::find(cats.begin(), cats.end(), entry.category) == cats.end()) {
                cats.push_back(entry.category);
            }
        }
        return cats;
    }
    
    // Add custom animation
    void addAnimation(const std::string& id, const std::string& name,
                     const std::string& category, const AnimationClipData& clip) {
        AnimationEntry entry;
        entry.id = id;
        entry.name = name;
        entry.category = category;
        entry.clip = clip;
        entry.isBuiltIn = false;
        animations_[id] = entry;
    }
    
private:
    AnimationLibrary() { initialize(); }
    
    void addBuiltIn(const std::string& id, const std::string& name,
                   const std::string& category, const AnimationClipData& clip) {
        AnimationEntry entry;
        entry.id = id;
        entry.name = name;
        entry.category = category;
        entry.clip = clip;
        entry.isBuiltIn = true;
        animations_[id] = entry;
    }
    
    std::unordered_map<std::string, AnimationEntry> animations_;
    bool initialized_ = false;
};

// ============================================================================
// Animation Player - Plays animation on skeleton
// ============================================================================

class AnimationPlayer {
public:
    AnimationPlayer() = default;
    
    // Set skeleton to animate
    void setSkeleton(Skeleton* skeleton) {
        skeleton_ = skeleton;
    }
    
    // Play animation
    void play(const AnimationClipData& clip) {
        currentClip_ = &clip;
        currentTime_ = 0.0f;
        isPlaying_ = true;
    }
    
    void play(const std::string& animationId) {
        auto* entry = AnimationLibrary::getInstance().getAnimation(animationId);
        if (entry) {
            play(entry->clip);
        }
    }
    
    void stop() {
        isPlaying_ = false;
        currentTime_ = 0.0f;
    }
    
    void pause() {
        isPlaying_ = false;
    }
    
    void resume() {
        isPlaying_ = true;
    }
    
    // Update animation
    void update(float deltaTime) {
        if (!isPlaying_ || !currentClip_ || !skeleton_) return;
        
        currentTime_ += deltaTime * playbackSpeed_;
        
        // Handle looping
        if (currentTime_ >= currentClip_->duration) {
            if (currentClip_->loop) {
                currentTime_ = std::fmod(currentTime_, currentClip_->duration);
            } else {
                currentTime_ = currentClip_->duration;
                isPlaying_ = false;
            }
        }
        
        // Apply animation to skeleton
        applyToSkeleton();
    }
    
    // Setters
    void setPlaybackSpeed(float speed) { playbackSpeed_ = speed; }
    void setCurrentTime(float time) { currentTime_ = time; }
    void setLoop(bool loop) { if (currentClip_) const_cast<AnimationClipData*>(currentClip_)->loop = loop; }
    
    // Getters
    float getCurrentTime() const { return currentTime_; }
    float getDuration() const { return currentClip_ ? currentClip_->duration : 0.0f; }
    float getProgress() const { return currentClip_ ? currentTime_ / currentClip_->duration : 0.0f; }
    bool isPlaying() const { return isPlaying_; }
    const AnimationClipData* getCurrentClip() const { return currentClip_; }
    
private:
    void applyToSkeleton() {
        if (!skeleton_ || !currentClip_) return;
        
        for (const auto& track : currentClip_->tracks) {
            int boneIdx = skeleton_->findBoneByName(track.boneName);
            
            // Also try with mapping
            if (boneIdx < 0) {
                auto& mapping = BoneMappingTable::getInstance();
                std::string lumaName = mapping.findLumaName(track.boneName, RigStandard::Mixamo);
                if (!lumaName.empty()) {
                    boneIdx = skeleton_->findBoneByName(lumaName);
                }
            }
            
            if (boneIdx < 0) continue;
            
            BoneKeyframe kf = track.sample(currentTime_);
            
            Bone* bone = skeleton_->getBone(boneIdx);
            if (bone) {
                // Blend with base pose
                bone->localPosition = bone->localPosition + kf.position;
                bone->localRotation = bone->localRotation * kf.rotation;
                // bone->localScale = kf.scale;  // Usually don't animate scale
            }
        }
    }
    
    Skeleton* skeleton_ = nullptr;
    const AnimationClipData* currentClip_ = nullptr;
    float currentTime_ = 0.0f;
    float playbackSpeed_ = 1.0f;
    bool isPlaying_ = false;
};

// ============================================================================
// Animation Preview State (for UI)
// ============================================================================

struct AnimationPreviewState {
    // Current animation
    std::string currentAnimationId = "idle";
    bool isPlaying = false;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool loop = true;
    
    // Player
    AnimationPlayer player;
    
    // UI state
    int selectedCategory = 0;
    int selectedAnimation = 0;
    bool showAnimationList = true;
    
    // Available animations
    std::vector<std::string> availableAnimations;
    std::vector<std::string> categories;
    
    void initialize() {
        auto& lib = AnimationLibrary::getInstance();
        lib.initialize();
        
        availableAnimations = lib.getAnimationIds();
        categories = lib.getCategories();
    }
    
    void update(float deltaTime) {
        player.update(deltaTime);
        currentTime = player.getCurrentTime();
        isPlaying = player.isPlaying();
    }
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline AnimationLibrary& getAnimationLibrary() {
    return AnimationLibrary::getInstance();
}

}  // namespace luma
