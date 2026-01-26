// Animation Tools
// Retargeting, compression, root motion, utilities
#pragma once

#include "skeleton.h"
#include "animation_clip.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace luma {

// ===== Animation Retargeting =====
// Maps animations from one skeleton to another

struct BoneMapping {
    std::string sourceBone;
    std::string targetBone;
    
    // Optional transform adjustments
    Vec3 rotationOffset{0, 0, 0};  // Euler angles
    Vec3 scaleMultiplier{1, 1, 1};
    bool mirrorX = false;
    bool mirrorY = false;
    bool mirrorZ = false;
};

class AnimationRetargeter {
public:
    std::vector<BoneMapping> boneMappings;
    
    // Auto-generate mappings between similar skeletons
    void autoGenerateMappings(const Skeleton& source, const Skeleton& target) {
        boneMappings.clear();
        
        // Common bone name variations
        static const std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>> namePatterns = {
            // Spine
            {{"spine", "Spine", "spine_01"}, {"spine", "Spine", "spine_01"}},
            {{"spine1", "Spine1", "spine_02"}, {"spine1", "Spine1", "spine_02"}},
            {{"spine2", "Spine2", "spine_03"}, {"spine2", "Spine2", "spine_03"}},
            
            // Head/Neck
            {{"head", "Head", "HEAD"}, {"head", "Head", "HEAD"}},
            {{"neck", "Neck", "neck_01"}, {"neck", "Neck", "neck_01"}},
            
            // Left Arm
            {{"shoulder_l", "LeftShoulder", "clavicle_l"}, {"shoulder_l", "LeftShoulder", "clavicle_l"}},
            {{"upperarm_l", "LeftArm", "arm_l"}, {"upperarm_l", "LeftArm", "arm_l"}},
            {{"lowerarm_l", "LeftForeArm", "forearm_l"}, {"lowerarm_l", "LeftForeArm", "forearm_l"}},
            {{"hand_l", "LeftHand", "hand_l"}, {"hand_l", "LeftHand", "hand_l"}},
            
            // Right Arm
            {{"shoulder_r", "RightShoulder", "clavicle_r"}, {"shoulder_r", "RightShoulder", "clavicle_r"}},
            {{"upperarm_r", "RightArm", "arm_r"}, {"upperarm_r", "RightArm", "arm_r"}},
            {{"lowerarm_r", "RightForeArm", "forearm_r"}, {"lowerarm_r", "RightForeArm", "forearm_r"}},
            {{"hand_r", "RightHand", "hand_r"}, {"hand_r", "RightHand", "hand_r"}},
            
            // Left Leg
            {{"thigh_l", "LeftUpLeg", "upperleg_l"}, {"thigh_l", "LeftUpLeg", "upperleg_l"}},
            {{"calf_l", "LeftLeg", "lowerleg_l"}, {"calf_l", "LeftLeg", "lowerleg_l"}},
            {{"foot_l", "LeftFoot", "foot_l"}, {"foot_l", "LeftFoot", "foot_l"}},
            {{"toe_l", "LeftToeBase", "ball_l"}, {"toe_l", "LeftToeBase", "ball_l"}},
            
            // Right Leg
            {{"thigh_r", "RightUpLeg", "upperleg_r"}, {"thigh_r", "RightUpLeg", "upperleg_r"}},
            {{"calf_r", "RightLeg", "lowerleg_r"}, {"calf_r", "RightLeg", "lowerleg_r"}},
            {{"foot_r", "RightFoot", "foot_r"}, {"foot_r", "RightFoot", "foot_r"}},
            {{"toe_r", "RightToeBase", "ball_r"}, {"toe_r", "RightToeBase", "ball_r"}},
            
            // Pelvis
            {{"pelvis", "Hips", "hip"}, {"pelvis", "Hips", "hip"}},
        };
        
        // Try to match bones
        for (const auto& [sourceNames, targetNames] : namePatterns) {
            int sourceIdx = -1;
            int targetIdx = -1;
            std::string sourceName, targetName;
            
            // Find source bone
            for (const auto& name : sourceNames) {
                sourceIdx = source.findBoneByName(name);
                if (sourceIdx >= 0) {
                    sourceName = name;
                    break;
                }
            }
            
            // Find target bone
            for (const auto& name : targetNames) {
                targetIdx = target.findBoneByName(name);
                if (targetIdx >= 0) {
                    targetName = name;
                    break;
                }
            }
            
            if (sourceIdx >= 0 && targetIdx >= 0) {
                BoneMapping mapping;
                mapping.sourceBone = sourceName;
                mapping.targetBone = targetName;
                boneMappings.push_back(mapping);
            }
        }
    }
    
    // Add manual mapping
    void addMapping(const std::string& source, const std::string& target) {
        BoneMapping mapping;
        mapping.sourceBone = source;
        mapping.targetBone = target;
        boneMappings.push_back(mapping);
    }
    
    // Retarget animation clip
    std::unique_ptr<AnimationClip> retarget(
        const AnimationClip& sourceClip,
        const Skeleton& sourceSkeleton,
        const Skeleton& targetSkeleton) const 
    {
        auto result = std::make_unique<AnimationClip>();
        result->name = sourceClip.name + "_retargeted";
        result->duration = sourceClip.duration;
        result->looping = sourceClip.looping;
        
        // For each mapping, transfer animation channels
        for (const auto& mapping : boneMappings) {
            // Find source channel
            const AnimationChannel* sourceChannel = nullptr;
            for (const auto& channel : sourceClip.channels) {
                if (channel.targetBone == mapping.sourceBone) {
                    sourceChannel = &channel;
                    break;
                }
            }
            
            if (!sourceChannel) continue;
            
            // Create target channel
            AnimationChannel newChannel;
            newChannel.targetBone = mapping.targetBone;
            
            // Copy and adjust keyframes
            for (const auto& key : sourceChannel->positionKeys) {
                VectorKeyframe newKey = key;
                newKey.value = Vec3(
                    newKey.value.x * mapping.scaleMultiplier.x * (mapping.mirrorX ? -1 : 1),
                    newKey.value.y * mapping.scaleMultiplier.y * (mapping.mirrorY ? -1 : 1),
                    newKey.value.z * mapping.scaleMultiplier.z * (mapping.mirrorZ ? -1 : 1)
                );
                newChannel.positionKeys.push_back(newKey);
            }
            
            for (const auto& key : sourceChannel->rotationKeys) {
                QuatKeyframe newKey = key;
                // Apply rotation offset
                if (mapping.rotationOffset.x != 0 || mapping.rotationOffset.y != 0 || mapping.rotationOffset.z != 0) {
                    Quat offset = Quat::fromEuler(mapping.rotationOffset.x, mapping.rotationOffset.y, mapping.rotationOffset.z);
                    newKey.value = offset * newKey.value;
                }
                // Apply mirroring (flip rotation axis)
                if (mapping.mirrorX) newKey.value.x = -newKey.value.x;
                if (mapping.mirrorY) newKey.value.y = -newKey.value.y;
                if (mapping.mirrorZ) newKey.value.z = -newKey.value.z;
                newChannel.rotationKeys.push_back(newKey);
            }
            
            for (const auto& key : sourceChannel->scaleKeys) {
                newChannel.scaleKeys.push_back(key);
            }
            
            result->channels.push_back(std::move(newChannel));
        }
        
        // Resolve bone indices for target skeleton
        result->resolveBoneIndices(targetSkeleton);
        
        return result;
    }
};

// ===== Animation Compression =====
// Reduces animation data size while preserving quality

struct CompressionSettings {
    float positionTolerance = 0.001f;    // Position error tolerance
    float rotationTolerance = 0.001f;    // Rotation error tolerance (radians)
    float scaleTolerance = 0.001f;       // Scale error tolerance
    bool removeStaticChannels = true;    // Remove channels with no change
    bool optimizeLinearKeys = true;      // Remove redundant linear keys
    int maxSampleRate = 30;              // Max keyframes per second
};

class AnimationCompressor {
public:
    CompressionSettings settings;
    
    // Compress animation clip
    std::unique_ptr<AnimationClip> compress(const AnimationClip& source) const {
        auto result = std::make_unique<AnimationClip>();
        result->name = source.name;
        result->duration = source.duration;
        result->looping = source.looping;
        
        for (const auto& channel : source.channels) {
            AnimationChannel newChannel;
            newChannel.targetBone = channel.targetBone;
            newChannel.targetBoneIndex = channel.targetBoneIndex;
            
            // Compress position keys
            if (!channel.positionKeys.empty()) {
                newChannel.positionKeys = compressVectorKeys(channel.positionKeys, settings.positionTolerance);
            }
            
            // Compress rotation keys
            if (!channel.rotationKeys.empty()) {
                newChannel.rotationKeys = compressQuatKeys(channel.rotationKeys, settings.rotationTolerance);
            }
            
            // Compress scale keys
            if (!channel.scaleKeys.empty()) {
                newChannel.scaleKeys = compressVectorKeys(channel.scaleKeys, settings.scaleTolerance);
            }
            
            // Check if channel has meaningful data
            bool hasData = !newChannel.positionKeys.empty() ||
                          !newChannel.rotationKeys.empty() ||
                          !newChannel.scaleKeys.empty();
            
            if (!settings.removeStaticChannels || hasData) {
                result->channels.push_back(std::move(newChannel));
            }
        }
        
        return result;
    }
    
    // Get compression statistics
    struct CompressionStats {
        size_t originalKeyframes = 0;
        size_t compressedKeyframes = 0;
        float compressionRatio = 1.0f;
    };
    
    CompressionStats getStats(const AnimationClip& original, const AnimationClip& compressed) const {
        CompressionStats stats;
        
        for (const auto& ch : original.channels) {
            stats.originalKeyframes += ch.positionKeys.size();
            stats.originalKeyframes += ch.rotationKeys.size();
            stats.originalKeyframes += ch.scaleKeys.size();
        }
        
        for (const auto& ch : compressed.channels) {
            stats.compressedKeyframes += ch.positionKeys.size();
            stats.compressedKeyframes += ch.rotationKeys.size();
            stats.compressedKeyframes += ch.scaleKeys.size();
        }
        
        if (stats.originalKeyframes > 0) {
            stats.compressionRatio = static_cast<float>(stats.compressedKeyframes) / 
                                     static_cast<float>(stats.originalKeyframes);
        }
        
        return stats;
    }
    
private:
    // Compress vector keyframes
    std::vector<VectorKeyframe> compressVectorKeys(
        const std::vector<VectorKeyframe>& keys, float tolerance) const 
    {
        if (keys.size() <= 2) return keys;
        
        std::vector<VectorKeyframe> result;
        result.push_back(keys.front());
        
        for (size_t i = 1; i < keys.size() - 1; i++) {
            const auto& prev = result.back();
            const auto& curr = keys[i];
            const auto& next = keys[i + 1];
            
            // Check if key is necessary (linear interpolation test)
            if (!settings.optimizeLinearKeys || !isKeyRedundant(prev, curr, next, tolerance)) {
                result.push_back(curr);
            }
        }
        
        result.push_back(keys.back());
        return result;
    }
    
    // Compress quaternion keyframes
    std::vector<QuatKeyframe> compressQuatKeys(
        const std::vector<QuatKeyframe>& keys, float tolerance) const 
    {
        if (keys.size() <= 2) return keys;
        
        std::vector<QuatKeyframe> result;
        result.push_back(keys.front());
        
        for (size_t i = 1; i < keys.size() - 1; i++) {
            const auto& prev = result.back();
            const auto& curr = keys[i];
            const auto& next = keys[i + 1];
            
            if (!settings.optimizeLinearKeys || !isQuatKeyRedundant(prev, curr, next, tolerance)) {
                result.push_back(curr);
            }
        }
        
        result.push_back(keys.back());
        return result;
    }
    
    bool isKeyRedundant(const VectorKeyframe& prev, const VectorKeyframe& curr,
                        const VectorKeyframe& next, float tolerance) const {
        float t = (curr.time - prev.time) / (next.time - prev.time);
        Vec3 interpolated = anim::lerp(prev.value, next.value, t);
        
        float error = (curr.value - interpolated).length();
        return error < tolerance;
    }
    
    bool isQuatKeyRedundant(const QuatKeyframe& prev, const QuatKeyframe& curr,
                           const QuatKeyframe& next, float tolerance) const {
        float t = (curr.time - prev.time) / (next.time - prev.time);
        Quat interpolated = anim::slerp(prev.value, next.value, t);
        
        // Angular difference
        float dot = std::abs(curr.value.x * interpolated.x + 
                            curr.value.y * interpolated.y +
                            curr.value.z * interpolated.z + 
                            curr.value.w * interpolated.w);
        float angle = 2.0f * std::acos(std::min(1.0f, dot));
        
        return angle < tolerance;
    }
};

// ===== Root Motion =====
// Extracts and applies root motion from animations

class RootMotionExtractor {
public:
    std::string rootBoneName = "root";  // Or "Hips", etc.
    
    // Extract mode
    enum class Mode {
        None,           // No root motion extraction
        XZ,             // Extract horizontal movement only
        XYZ,            // Extract all movement
        Y_Only,         // Extract vertical only (for jumping)
        Rotation_Y      // Extract Y-axis rotation only
    };
    Mode mode = Mode::XZ;
    
    // Extracted motion data
    struct RootMotionData {
        std::vector<float> times;
        std::vector<Vec3> positions;
        std::vector<Quat> rotations;
        
        Vec3 getTotalDisplacement() const {
            if (positions.size() < 2) return Vec3(0, 0, 0);
            return positions.back() - positions.front();
        }
        
        float getTotalRotation() const {
            if (rotations.size() < 2) return 0.0f;
            Quat delta = rotations.back() * Quat(
                -rotations.front().x,
                -rotations.front().y,
                -rotations.front().z,
                rotations.front().w
            );
            return 2.0f * std::acos(std::min(1.0f, std::abs(delta.w)));
        }
        
        // Sample at time
        Vec3 samplePosition(float time) const {
            if (times.empty()) return Vec3(0, 0, 0);
            if (time <= times.front()) return positions.front();
            if (time >= times.back()) return positions.back();
            
            // Find surrounding keys
            for (size_t i = 1; i < times.size(); i++) {
                if (times[i] >= time) {
                    float t = (time - times[i-1]) / (times[i] - times[i-1]);
                    return anim::lerp(positions[i-1], positions[i], t);
                }
            }
            return positions.back();
        }
        
        Quat sampleRotation(float time) const {
            if (times.empty()) return Quat();
            if (time <= times.front()) return rotations.front();
            if (time >= times.back()) return rotations.back();
            
            for (size_t i = 1; i < times.size(); i++) {
                if (times[i] >= time) {
                    float t = (time - times[i-1]) / (times[i] - times[i-1]);
                    return anim::slerp(rotations[i-1], rotations[i], t);
                }
            }
            return rotations.back();
        }
    };
    
    // Extract root motion from animation
    RootMotionData extract(const AnimationClip& clip) const {
        RootMotionData data;
        
        // Find root bone channel
        const AnimationChannel* rootChannel = nullptr;
        for (const auto& channel : clip.channels) {
            if (channel.targetBone == rootBoneName) {
                rootChannel = &channel;
                break;
            }
        }
        
        if (!rootChannel) return data;
        
        // Extract position data
        Vec3 startPos(0, 0, 0);
        if (!rootChannel->positionKeys.empty()) {
            startPos = rootChannel->positionKeys.front().value;
        }
        
        for (const auto& key : rootChannel->positionKeys) {
            data.times.push_back(key.time);
            
            Vec3 pos = key.value;
            switch (mode) {
                case Mode::None:
                    pos = Vec3(0, 0, 0);
                    break;
                case Mode::XZ:
                    pos = Vec3(pos.x - startPos.x, 0, pos.z - startPos.z);
                    break;
                case Mode::XYZ:
                    pos = pos - startPos;
                    break;
                case Mode::Y_Only:
                    pos = Vec3(0, pos.y - startPos.y, 0);
                    break;
                case Mode::Rotation_Y:
                    pos = Vec3(0, 0, 0);
                    break;
            }
            data.positions.push_back(pos);
        }
        
        // Extract rotation data
        Quat startRot;
        if (!rootChannel->rotationKeys.empty()) {
            startRot = rootChannel->rotationKeys.front().value;
        }
        
        for (const auto& key : rootChannel->rotationKeys) {
            Quat rot = key.value;
            if (mode == Mode::Rotation_Y) {
                // Extract Y rotation only
                Vec3 euler = rot.toEuler();
                rot = Quat::fromEuler(0, euler.y, 0);
                
                Vec3 startEuler = startRot.toEuler();
                Quat startY = Quat::fromEuler(0, startEuler.y, 0);
                
                // Delta from start
                rot = rot * Quat(-startY.x, -startY.y, -startY.z, startY.w);
            }
            data.rotations.push_back(rot);
        }
        
        // Ensure data arrays match
        while (data.rotations.size() < data.positions.size()) {
            data.rotations.push_back(Quat());
        }
        while (data.positions.size() < data.rotations.size()) {
            data.positions.push_back(Vec3(0, 0, 0));
        }
        
        return data;
    }
    
    // Bake root motion back into animation (in-place modification)
    void bakeIntoAnimation(AnimationClip& clip, const RootMotionData& motion) {
        // Find or create root channel
        AnimationChannel* rootChannel = nullptr;
        for (auto& channel : clip.channels) {
            if (channel.targetBone == rootBoneName) {
                rootChannel = &channel;
                break;
            }
        }
        
        if (!rootChannel) {
            clip.channels.push_back(AnimationChannel());
            rootChannel = &clip.channels.back();
            rootChannel->targetBone = rootBoneName;
        }
        
        // Add motion to existing keys
        for (auto& key : rootChannel->positionKeys) {
            key.value = key.value + motion.samplePosition(key.time);
        }
    }
    
    // Remove root motion from animation
    void removeFromAnimation(AnimationClip& clip) {
        for (auto& channel : clip.channels) {
            if (channel.targetBone == rootBoneName) {
                // Zero out based on mode
                Vec3 startPos(0, 0, 0);
                if (!channel.positionKeys.empty()) {
                    startPos = channel.positionKeys.front().value;
                }
                
                for (auto& key : channel.positionKeys) {
                    switch (mode) {
                        case Mode::XZ:
                            key.value.x = startPos.x;
                            key.value.z = startPos.z;
                            break;
                        case Mode::XYZ:
                            key.value = startPos;
                            break;
                        case Mode::Y_Only:
                            key.value.y = startPos.y;
                            break;
                        default:
                            break;
                    }
                }
                break;
            }
        }
    }
};

// ===== Animation Notifies =====
// Events triggered at specific times in animation

struct AnimationNotify {
    float time = 0.0f;
    std::string name;
    
    enum class Type {
        Event,      // Generic event
        Sound,      // Play sound
        Particle,   // Spawn particle
        FootStep,   // Foot contact
        Custom      // Custom callback
    };
    Type type = Type::Event;
    
    // Optional payload
    std::string payload;  // Sound name, particle name, etc.
    Vec3 offset{0, 0, 0}; // Position offset for spawning
    
    // Callback for custom notifies
    std::function<void()> callback;
};

class AnimationNotifyTrack {
public:
    std::vector<AnimationNotify> notifies;
    
    void addNotify(float time, const std::string& name, AnimationNotify::Type type = AnimationNotify::Type::Event) {
        AnimationNotify notify;
        notify.time = time;
        notify.name = name;
        notify.type = type;
        notifies.push_back(notify);
        
        // Sort by time
        std::sort(notifies.begin(), notifies.end(),
            [](const AnimationNotify& a, const AnimationNotify& b) {
                return a.time < b.time;
            });
    }
    
    void addFootstep(float time, bool isLeftFoot) {
        AnimationNotify notify;
        notify.time = time;
        notify.name = isLeftFoot ? "FootstepLeft" : "FootstepRight";
        notify.type = AnimationNotify::Type::FootStep;
        notifies.push_back(notify);
    }
    
    // Check for notifies in time range
    std::vector<const AnimationNotify*> getNotifiesInRange(float startTime, float endTime) const {
        std::vector<const AnimationNotify*> result;
        
        for (const auto& notify : notifies) {
            if (notify.time >= startTime && notify.time < endTime) {
                result.push_back(&notify);
            }
        }
        
        return result;
    }
};

}  // namespace luma
