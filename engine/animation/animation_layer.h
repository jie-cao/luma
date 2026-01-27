// Animation Layer System
// Supports multiple animation layers with masking and blending modes
#pragma once

#include "skeleton.h"
#include "animation_clip.h"
#include <vector>
#include <string>
#include <memory>
#include <bitset>
#include <unordered_set>

namespace luma {

// ===== Blend Mode =====
enum class AnimationBlendMode {
    Override,    // Replace lower layers
    Additive,    // Add to lower layers
    Multiply     // Multiply with lower layers (for scaling effects)
};

// ===== Bone Mask =====
// Defines which bones are affected by a layer
class BoneMask {
public:
    BoneMask() = default;
    
    // Create mask from bone names
    void addBone(const std::string& boneName) {
        includedBones_.insert(boneName);
    }
    
    void removeBone(const std::string& boneName) {
        includedBones_.erase(boneName);
    }
    
    void clear() {
        includedBones_.clear();
        resolvedMask_.reset();
    }
    
    // Include bone and all children
    void addBoneRecursive(const Skeleton& skeleton, const std::string& rootBone) {
        int boneIndex = skeleton.findBoneByName(rootBone);
        if (boneIndex < 0) return;
        
        addBoneRecursiveInternal(skeleton, boneIndex);
    }
    
    // Resolve bone names to indices for fast lookup
    void resolve(const Skeleton& skeleton) {
        resolvedMask_.reset();
        
        for (const auto& boneName : includedBones_) {
            int index = skeleton.findBoneByName(boneName);
            if (index >= 0 && index < MAX_BONES) {
                resolvedMask_.set(index);
            }
        }
        
        // If no bones specified, include all
        if (includedBones_.empty()) {
            resolvedMask_.set();  // All bits set
        }
    }
    
    // Check if bone is included
    bool includes(int boneIndex) const {
        if (boneIndex < 0 || boneIndex >= MAX_BONES) return false;
        return resolvedMask_.test(boneIndex);
    }
    
    // Get weight for bone (with optional falloff)
    float getWeight(int boneIndex, float layerWeight = 1.0f) const {
        return includes(boneIndex) ? layerWeight : 0.0f;
    }
    
    // For UI: check if mask is empty
    bool isEmpty() const { return includedBones_.empty(); }
    size_t getBoneCount() const { return includedBones_.size(); }
    const std::unordered_set<std::string>& getIncludedBones() const { return includedBones_; }
    
    // Preset masks
    static BoneMask fullBody() {
        BoneMask mask;
        // Empty includedBones_ means all bones
        return mask;
    }
    
    static BoneMask upperBody(const Skeleton& skeleton) {
        BoneMask mask;
        // Common upper body bone names
        mask.addBoneRecursive(skeleton, "spine");
        mask.addBoneRecursive(skeleton, "Spine");
        mask.addBoneRecursive(skeleton, "spine_01");
        mask.addBoneRecursive(skeleton, "Spine1");
        return mask;
    }
    
    static BoneMask lowerBody(const Skeleton& skeleton) {
        BoneMask mask;
        // Common lower body bone names
        mask.addBoneRecursive(skeleton, "pelvis");
        mask.addBoneRecursive(skeleton, "Hips");
        mask.addBoneRecursive(skeleton, "hip");
        mask.addBoneRecursive(skeleton, "thigh_l");
        mask.addBoneRecursive(skeleton, "thigh_r");
        mask.addBoneRecursive(skeleton, "LeftUpLeg");
        mask.addBoneRecursive(skeleton, "RightUpLeg");
        return mask;
    }
    
    static BoneMask arms(const Skeleton& skeleton) {
        BoneMask mask;
        mask.addBoneRecursive(skeleton, "shoulder_l");
        mask.addBoneRecursive(skeleton, "shoulder_r");
        mask.addBoneRecursive(skeleton, "LeftShoulder");
        mask.addBoneRecursive(skeleton, "RightShoulder");
        mask.addBoneRecursive(skeleton, "clavicle_l");
        mask.addBoneRecursive(skeleton, "clavicle_r");
        return mask;
    }
    
private:
    void addBoneRecursiveInternal(const Skeleton& skeleton, int boneIndex) {
        const Bone* bone = skeleton.getBone(boneIndex);
        if (!bone) return;
        
        includedBones_.insert(bone->name);
        
        // Find children by checking all bones for this parent
        for (uint32_t i = 0; i < skeleton.getBoneCount(); i++) {
            const Bone* child = skeleton.getBone(i);
            if (child && child->parentIndex == boneIndex) {
                addBoneRecursiveInternal(skeleton, i);
            }
        }
    }
    
    std::unordered_set<std::string> includedBones_;
    std::bitset<MAX_BONES> resolvedMask_;
};

// ===== Animation Layer =====
class AnimationLayer {
public:
    std::string name = "Layer";
    int index = 0;
    
    // Layer settings
    float weight = 1.0f;
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    BoneMask mask;
    bool enabled = true;
    
    // Current animation state
    AnimationClip* currentClip = nullptr;
    float time = 0.0f;
    float speed = 1.0f;
    bool playing = false;
    bool loop = true;
    
    // Blending state
    AnimationClip* previousClip = nullptr;
    float previousTime = 0.0f;
    float blendProgress = 1.0f;  // 0 = previous, 1 = current
    float blendDuration = 0.2f;
    
    // IK targets (for IK layers)
    struct IKTarget {
        Vec3 position;
        float weight = 0.0f;
        int targetBoneIndex = -1;
    };
    std::vector<IKTarget> ikTargets;
    
    // Play animation on this layer
    void play(AnimationClip* clip, float crossfade = 0.2f) {
        if (currentClip && crossfade > 0.0f) {
            previousClip = currentClip;
            previousTime = time;
            blendProgress = 0.0f;
            blendDuration = crossfade;
        } else {
            previousClip = nullptr;
            blendProgress = 1.0f;
        }
        
        currentClip = clip;
        time = 0.0f;
        playing = true;
        if (clip) {
            loop = clip->looping;
        }
    }
    
    void stop() {
        playing = false;
        currentClip = nullptr;
        previousClip = nullptr;
    }
    
    // Update layer
    void update(float deltaTime) {
        if (!enabled || !playing) return;
        
        // Update blend transition
        if (blendProgress < 1.0f && blendDuration > 0.0f) {
            blendProgress += deltaTime / blendDuration;
            blendProgress = std::min(1.0f, blendProgress);
            
            // Update previous animation time too
            if (previousClip) {
                previousTime += deltaTime * speed;
                if (previousTime >= previousClip->duration) {
                    if (previousClip->looping) {
                        previousTime = std::fmod(previousTime, previousClip->duration);
                    } else {
                        previousTime = previousClip->duration;
                    }
                }
            }
        }
        
        // Update current animation
        if (currentClip) {
            time += deltaTime * speed;
            
            if (time >= currentClip->duration) {
                if (loop) {
                    time = std::fmod(time, currentClip->duration);
                } else {
                    time = currentClip->duration;
                    playing = false;
                }
            }
        }
    }
    
    // Sample this layer's contribution
    void sample(Vec3* positions, Quat* rotations, Vec3* scales, int boneCount) const {
        if (!enabled || !currentClip) return;
        
        // Sample current animation
        std::vector<Vec3> currentPos(boneCount);
        std::vector<Quat> currentRot(boneCount);
        std::vector<Vec3> currentScl(boneCount);
        currentClip->sample(time, currentPos.data(), currentRot.data(), currentScl.data(), boneCount);
        
        // If blending with previous animation
        if (previousClip && blendProgress < 1.0f) {
            std::vector<Vec3> prevPos(boneCount);
            std::vector<Quat> prevRot(boneCount);
            std::vector<Vec3> prevScl(boneCount);
            previousClip->sample(previousTime, prevPos.data(), prevRot.data(), prevScl.data(), boneCount);
            
            // Blend between previous and current
            for (int i = 0; i < boneCount; i++) {
                currentPos[i] = anim::lerp(prevPos[i], currentPos[i], blendProgress);
                currentRot[i] = anim::slerp(prevRot[i], currentRot[i], blendProgress);
                currentScl[i] = anim::lerp(prevScl[i], currentScl[i], blendProgress);
            }
        }
        
        // Apply mask
        for (int i = 0; i < boneCount; i++) {
            float boneWeight = mask.getWeight(i, weight);
            if (boneWeight <= 0.0f) continue;
            
            switch (blendMode) {
                case AnimationBlendMode::Override:
                    positions[i] = anim::lerp(positions[i], currentPos[i], boneWeight);
                    rotations[i] = anim::slerp(rotations[i], currentRot[i], boneWeight);
                    scales[i] = anim::lerp(scales[i], currentScl[i], boneWeight);
                    break;
                    
                case AnimationBlendMode::Additive:
                    // Additive: add delta from reference pose
                    positions[i] = positions[i] + (currentPos[i] * boneWeight);
                    // For rotation, multiply the quaternion
                    {
                        Quat delta = currentRot[i];
                        // Scale the rotation by weight (approximate)
                        delta = anim::slerp(Quat(), delta, boneWeight);
                        rotations[i] = delta * rotations[i];
                    }
                    {
                        Vec3 blendedScl = anim::lerp(Vec3(1,1,1), currentScl[i], boneWeight);
                        scales[i] = Vec3(scales[i].x * blendedScl.x, 
                                        scales[i].y * blendedScl.y, 
                                        scales[i].z * blendedScl.z);
                    }
                    break;
                    
                case AnimationBlendMode::Multiply:
                    // Multiply mode - useful for scaling animations
                    positions[i] = Vec3(
                        positions[i].x * anim::lerp(1.0f, currentPos[i].x, boneWeight),
                        positions[i].y * anim::lerp(1.0f, currentPos[i].y, boneWeight),
                        positions[i].z * anim::lerp(1.0f, currentPos[i].z, boneWeight)
                    );
                    scales[i] = Vec3(
                        scales[i].x * anim::lerp(1.0f, currentScl[i].x, boneWeight),
                        scales[i].y * anim::lerp(1.0f, currentScl[i].y, boneWeight),
                        scales[i].z * anim::lerp(1.0f, currentScl[i].z, boneWeight)
                    );
                    break;
            }
        }
    }
};

// ===== Layer Manager =====
class AnimationLayerManager {
public:
    AnimationLayerManager() {
        // Create default base layer
        createLayer("Base");
    }
    
    // Create a new layer
    AnimationLayer* createLayer(const std::string& name) {
        auto layer = std::make_unique<AnimationLayer>();
        layer->name = name;
        layer->index = static_cast<int>(layers_.size());
        layers_.push_back(std::move(layer));
        return layers_.back().get();
    }
    
    // Get layer by name
    AnimationLayer* getLayer(const std::string& name) {
        for (auto& layer : layers_) {
            if (layer->name == name) return layer.get();
        }
        return nullptr;
    }
    
    // Get layer by index
    AnimationLayer* getLayer(int index) {
        if (index >= 0 && index < static_cast<int>(layers_.size())) {
            return layers_[index].get();
        }
        return nullptr;
    }
    
    // Get base layer (index 0)
    AnimationLayer* getBaseLayer() {
        return layers_.empty() ? nullptr : layers_[0].get();
    }
    
    // Get number of layers
    size_t getLayerCount() const { return layers_.size(); }
    
    // Remove layer by name
    void removeLayer(const std::string& name) {
        if (name == "Base") return;  // Cannot remove base layer
        
        for (auto it = layers_.begin(); it != layers_.end(); ++it) {
            if ((*it)->name == name) {
                layers_.erase(it);
                // Re-index layers
                for (size_t i = 0; i < layers_.size(); i++) {
                    layers_[i]->index = static_cast<int>(i);
                }
                return;
            }
        }
    }
    
    // Set skeleton (resolves bone masks)
    void setSkeleton(Skeleton* skeleton) {
        skeleton_ = skeleton;
        for (auto& layer : layers_) {
            layer->mask.resolve(*skeleton);
        }
    }
    
    // Update all layers
    void update(float deltaTime) {
        for (auto& layer : layers_) {
            layer->update(deltaTime);
        }
    }
    
    // Evaluate all layers and compute final pose
    void evaluate(Vec3* positions, Quat* rotations, Vec3* scales, int boneCount) {
        // Initialize with bind pose
        if (skeleton_) {
            for (int i = 0; i < boneCount; i++) {
                const Bone* bone = skeleton_->getBone(i);
                if (bone) {
                    positions[i] = bone->localPosition;
                    rotations[i] = bone->localRotation;
                    scales[i] = bone->localScale;
                }
            }
        }
        
        // Apply layers in order (base first, additive last)
        for (auto& layer : layers_) {
            if (layer->enabled && layer->weight > 0.0f) {
                layer->sample(positions, rotations, scales, boneCount);
            }
        }
    }
    
private:
    std::vector<std::unique_ptr<AnimationLayer>> layers_;
    Skeleton* skeleton_ = nullptr;
};

}  // namespace luma
