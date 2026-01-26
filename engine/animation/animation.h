// Animation System - Unified header
// Includes all animation-related components
#pragma once

#include "skeleton.h"
#include "animation_clip.h"
#include "animator.h"
#include "animation_layer.h"
#include "blend_tree.h"
#include "state_machine.h"
#include "ik_system.h"
#include "timeline.h"
#include "animation_tools.h"

namespace luma {

// ===== Skinned Mesh Data =====
// Per-vertex skinning data (4 bones per vertex)
struct SkinVertex {
    uint32_t boneIndices[4] = {0, 0, 0, 0};
    float boneWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    void addBoneInfluence(uint32_t boneIndex, float weight) {
        // Find slot with lowest weight
        int minIdx = 0;
        float minWeight = boneWeights[0];
        for (int i = 1; i < 4; i++) {
            if (boneWeights[i] < minWeight) {
                minWeight = boneWeights[i];
                minIdx = i;
            }
        }
        
        // Replace if new weight is higher
        if (weight > minWeight) {
            boneIndices[minIdx] = boneIndex;
            boneWeights[minIdx] = weight;
        }
    }
    
    void normalize() {
        float total = boneWeights[0] + boneWeights[1] + boneWeights[2] + boneWeights[3];
        if (total > 0.0001f) {
            for (int i = 0; i < 4; i++) {
                boneWeights[i] /= total;
            }
        }
    }
};

// ===== Animated Model =====
// Model with skeleton and animations
struct AnimatedModel {
    // Skeleton
    std::unique_ptr<Skeleton> skeleton;
    
    // Animations
    std::unordered_map<std::string, std::unique_ptr<AnimationClip>> animations;
    
    // Per-vertex skin data
    std::vector<SkinVertex> skinData;
    
    bool hasSkeleton() const { return skeleton && skeleton->getBoneCount() > 0; }
    bool hasAnimations() const { return !animations.empty(); }
};

// ===== Animation Utilities =====
namespace AnimUtils {

// Create a simple test animation (rotate around Y axis)
inline std::unique_ptr<AnimationClip> createTestRotationAnimation(float duration = 2.0f) {
    auto clip = std::make_unique<AnimationClip>();
    clip->name = "test_rotation";
    clip->duration = duration;
    clip->looping = true;
    
    // Note: This would animate a bone named "root" if present
    auto& channel = clip->addChannel("root");
    
    // Add rotation keyframes (360 degrees over duration)
    for (int i = 0; i <= 4; i++) {
        float t = (float)i / 4.0f * duration;
        float angle = (float)i / 4.0f * 6.28318f;  // Full rotation
        
        QuatKeyframe key;
        key.time = t;
        key.value = Quat::fromEuler(0, angle, 0);
        channel.rotationKeys.push_back(key);
    }
    
    return clip;
}

// Create an idle/breathing animation
inline std::unique_ptr<AnimationClip> createBreathingAnimation(float duration = 2.0f) {
    auto clip = std::make_unique<AnimationClip>();
    clip->name = "breathing";
    clip->duration = duration;
    clip->looping = true;
    
    auto& channel = clip->addChannel("spine");
    
    // Subtle scale animation to simulate breathing
    for (int i = 0; i <= 4; i++) {
        float t = (float)i / 4.0f * duration;
        float scale = 1.0f + 0.02f * std::sin((float)i / 4.0f * 6.28318f);
        
        VectorKeyframe key;
        key.time = t;
        key.value = Vec3(1.0f, scale, 1.0f);
        channel.scaleKeys.push_back(key);
    }
    
    return clip;
}

}  // namespace AnimUtils

}  // namespace luma
