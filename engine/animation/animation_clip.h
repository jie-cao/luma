// AnimationClip - Keyframe animation data
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace luma {

// ===== Keyframe Types =====

template<typename T>
struct Keyframe {
    float time;
    T value;
    
    // For interpolation
    T inTangent;
    T outTangent;
};

using VectorKeyframe = Keyframe<Vec3>;
using QuatKeyframe = Keyframe<Quat>;

// ===== Interpolation =====
enum class InterpolationType {
    Step,       // No interpolation, jump to next value
    Linear,     // Linear interpolation
    Cubic       // Cubic/Hermite interpolation (uses tangents)
};

// ===== Animation Channel =====
// Animates one property of one bone
struct AnimationChannel {
    std::string targetBone;
    int targetBoneIndex = -1;  // Resolved at runtime
    
    // Position keyframes
    std::vector<VectorKeyframe> positionKeys;
    
    // Rotation keyframes
    std::vector<QuatKeyframe> rotationKeys;
    
    // Scale keyframes
    std::vector<VectorKeyframe> scaleKeys;
    
    InterpolationType interpolation = InterpolationType::Linear;
    
    // Sample the channel at a given time
    void sample(float time, Vec3& outPos, Quat& outRot, Vec3& outScale) const;
    
    bool hasPosition() const { return !positionKeys.empty(); }
    bool hasRotation() const { return !rotationKeys.empty(); }
    bool hasScale() const { return !scaleKeys.empty(); }
};

// ===== Animation Clip =====
class AnimationClip {
public:
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 30.0f;
    bool looping = true;
    
    std::vector<AnimationChannel> channels;
    
    // Add a channel for a bone
    AnimationChannel& addChannel(const std::string& boneName);
    
    // Find channel by bone name
    AnimationChannel* findChannel(const std::string& boneName);
    const AnimationChannel* findChannel(const std::string& boneName) const;
    
    // Resolve bone indices from skeleton
    void resolveBoneIndices(const class Skeleton& skeleton);
    
    // Sample all channels at a given time
    // Output: arrays of position, rotation, scale per bone (indexed by bone index)
    void sample(float time, Vec3* outPositions, Quat* outRotations, Vec3* outScales, int boneCount) const;
};

// ===== Interpolation Helpers =====
namespace anim {
    
// Linear interpolation
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return Vec3(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t));
}

// Spherical linear interpolation for quaternions
inline Quat slerp(const Quat& a, const Quat& b, float t) {
    Quat result;
    
    float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    
    // If dot is negative, negate one quaternion to take shorter path
    Quat b2 = b;
    if (dot < 0.0f) {
        b2.x = -b.x; b2.y = -b.y; b2.z = -b.z; b2.w = -b.w;
        dot = -dot;
    }
    
    // If quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        result.x = lerp(a.x, b2.x, t);
        result.y = lerp(a.y, b2.y, t);
        result.z = lerp(a.z, b2.z, t);
        result.w = lerp(a.w, b2.w, t);
    } else {
        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);
        float wa = std::sin((1.0f - t) * theta) / sinTheta;
        float wb = std::sin(t * theta) / sinTheta;
        
        result.x = wa * a.x + wb * b2.x;
        result.y = wa * a.y + wb * b2.y;
        result.z = wa * a.z + wb * b2.z;
        result.w = wa * a.w + wb * b2.w;
    }
    
    // Normalize
    float len = std::sqrt(result.x*result.x + result.y*result.y + 
                          result.z*result.z + result.w*result.w);
    if (len > 0.0001f) {
        result.x /= len; result.y /= len; result.z /= len; result.w /= len;
    }
    
    return result;
}

// Find keyframe index for a given time
template<typename T>
inline int findKeyframeIndex(const std::vector<Keyframe<T>>& keys, float time) {
    if (keys.empty()) return -1;
    if (time <= keys.front().time) return 0;
    if (time >= keys.back().time) return (int)keys.size() - 1;
    
    // Binary search
    int low = 0, high = (int)keys.size() - 1;
    while (low < high - 1) {
        int mid = (low + high) / 2;
        if (keys[mid].time <= time) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return low;
}

}  // namespace anim

// ===== Implementation =====

inline void AnimationChannel::sample(float time, Vec3& outPos, Quat& outRot, Vec3& outScale) const {
    // Sample position
    if (!positionKeys.empty()) {
        int idx = anim::findKeyframeIndex(positionKeys, time);
        if (idx < 0 || idx >= (int)positionKeys.size() - 1) {
            outPos = positionKeys.empty() ? Vec3(0,0,0) : 
                     (idx < 0 ? positionKeys.front().value : positionKeys.back().value);
        } else {
            const auto& k0 = positionKeys[idx];
            const auto& k1 = positionKeys[idx + 1];
            float t = (time - k0.time) / (k1.time - k0.time);
            outPos = anim::lerp(k0.value, k1.value, t);
        }
    }
    
    // Sample rotation
    if (!rotationKeys.empty()) {
        int idx = anim::findKeyframeIndex(rotationKeys, time);
        if (idx < 0 || idx >= (int)rotationKeys.size() - 1) {
            outRot = rotationKeys.empty() ? Quat() : 
                     (idx < 0 ? rotationKeys.front().value : rotationKeys.back().value);
        } else {
            const auto& k0 = rotationKeys[idx];
            const auto& k1 = rotationKeys[idx + 1];
            float t = (time - k0.time) / (k1.time - k0.time);
            outRot = anim::slerp(k0.value, k1.value, t);
        }
    }
    
    // Sample scale
    if (!scaleKeys.empty()) {
        int idx = anim::findKeyframeIndex(scaleKeys, time);
        if (idx < 0 || idx >= (int)scaleKeys.size() - 1) {
            outScale = scaleKeys.empty() ? Vec3(1,1,1) : 
                       (idx < 0 ? scaleKeys.front().value : scaleKeys.back().value);
        } else {
            const auto& k0 = scaleKeys[idx];
            const auto& k1 = scaleKeys[idx + 1];
            float t = (time - k0.time) / (k1.time - k0.time);
            outScale = anim::lerp(k0.value, k1.value, t);
        }
    }
}

inline AnimationChannel& AnimationClip::addChannel(const std::string& boneName) {
    channels.emplace_back();
    channels.back().targetBone = boneName;
    return channels.back();
}

inline AnimationChannel* AnimationClip::findChannel(const std::string& boneName) {
    for (auto& ch : channels) {
        if (ch.targetBone == boneName) return &ch;
    }
    return nullptr;
}

inline const AnimationChannel* AnimationClip::findChannel(const std::string& boneName) const {
    for (const auto& ch : channels) {
        if (ch.targetBone == boneName) return &ch;
    }
    return nullptr;
}

inline void AnimationClip::resolveBoneIndices(const class Skeleton& skeleton) {
    for (auto& ch : channels) {
        ch.targetBoneIndex = skeleton.findBoneByName(ch.targetBone);
    }
}

inline void AnimationClip::sample(float time, Vec3* outPositions, Quat* outRotations, Vec3* outScales, int boneCount) const {
    // Initialize to identity
    for (int i = 0; i < boneCount; i++) {
        outPositions[i] = Vec3(0, 0, 0);
        outRotations[i] = Quat();
        outScales[i] = Vec3(1, 1, 1);
    }
    
    // Handle looping
    float sampleTime = time;
    if (looping && duration > 0.0f) {
        sampleTime = std::fmod(time, duration);
        if (sampleTime < 0) sampleTime += duration;
    } else {
        sampleTime = std::max(0.0f, std::min(time, duration));
    }
    
    // Sample each channel
    for (const auto& ch : channels) {
        if (ch.targetBoneIndex >= 0 && ch.targetBoneIndex < boneCount) {
            ch.sample(sampleTime, 
                      outPositions[ch.targetBoneIndex],
                      outRotations[ch.targetBoneIndex],
                      outScales[ch.targetBoneIndex]);
        }
    }
}

}  // namespace luma
