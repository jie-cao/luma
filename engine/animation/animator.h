// Animator - Animation playback and blending
#pragma once

#include "skeleton.h"
#include "animation_clip.h"
#include <memory>
#include <functional>

namespace luma {

// ===== Animation State =====
// Represents a playing animation
struct AnimationState {
    AnimationClip* clip = nullptr;
    float time = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;
    bool playing = false;
    bool loop = true;
    
    // Blend in/out
    float blendTime = 0.0f;
    float blendDuration = 0.2f;
    bool blendingIn = false;
    bool blendingOut = false;
    
    void reset() {
        time = 0.0f;
        playing = false;
        blendTime = 0.0f;
        blendingIn = false;
        blendingOut = false;
    }
};

// ===== Animator =====
class Animator {
public:
    Animator();
    ~Animator();
    
    // === Setup ===
    
    // Set the skeleton this animator controls
    void setSkeleton(Skeleton* skeleton) { skeleton_ = skeleton; }
    Skeleton* getSkeleton() const { return skeleton_; }
    
    // Add an animation clip (animator takes ownership)
    void addClip(const std::string& name, std::unique_ptr<AnimationClip> clip);
    
    // Get clip by name
    AnimationClip* getClip(const std::string& name);
    const AnimationClip* getClip(const std::string& name) const;
    
    // Get all clip names
    std::vector<std::string> getClipNames() const;
    
    // === Playback Control ===
    
    // Play an animation (crossfades from current)
    void play(const std::string& clipName, float crossfadeDuration = 0.2f);
    
    // Stop all animations
    void stop();
    
    // Pause/resume
    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }
    
    // Set playback speed (1.0 = normal)
    void setSpeed(float speed) { globalSpeed_ = speed; }
    float getSpeed() const { return globalSpeed_; }
    
    // Set looping for current animation
    void setLooping(bool loop);
    
    // Set current animation time (for scrubbing)
    void setTime(float time);
    
    // === Blending ===
    
    // Set blend weight for a layer (0-1)
    void setLayerWeight(int layer, float weight);
    
    // Blend two animations additively
    void blendAdditive(const std::string& clipName, float weight);
    
    // === Update ===
    
    // Update animation (call every frame)
    // deltaTime: time since last update in seconds
    void update(float deltaTime);
    
    // === Output ===
    
    // Get current bone matrices for skinning
    // Output array must have space for MAX_BONES matrices
    void getSkinningMatrices(Mat4* outMatrices) const;
    
    // Get current animation time
    float getCurrentTime() const;
    
    // Get current clip name
    std::string getCurrentClipName() const;
    
    // Is any animation playing?
    bool isPlaying() const;
    
    // === Callbacks ===
    
    // Called when animation finishes (non-looping only)
    std::function<void(const std::string& clipName)> onAnimationFinished;
    
    // Called on animation events/markers
    std::function<void(const std::string& clipName, const std::string& eventName)> onAnimationEvent;
    
private:
    void applyAnimation(const AnimationClip* clip, float time, float weight);
    void blendPose(const Vec3* positions, const Quat* rotations, const Vec3* scales, 
                   int boneCount, float weight);
    
    Skeleton* skeleton_ = nullptr;
    
    // Clip library
    std::unordered_map<std::string, std::unique_ptr<AnimationClip>> clips_;
    
    // Current playing states (for blending)
    std::vector<AnimationState> activeStates_;
    
    // Accumulated pose for blending
    std::vector<Vec3> blendedPositions_;
    std::vector<Quat> blendedRotations_;
    std::vector<Vec3> blendedScales_;
    std::vector<float> blendedWeights_;  // Per-bone accumulated weight
    
    // Playback state
    bool paused_ = false;
    float globalSpeed_ = 1.0f;
    
    // Cached skinning matrices
    mutable std::vector<Mat4> skinningMatrices_;
    mutable bool matricesDirty_ = true;
};

// ===== Implementation =====

inline Animator::Animator() = default;
inline Animator::~Animator() = default;

inline void Animator::addClip(const std::string& name, std::unique_ptr<AnimationClip> clip) {
    if (skeleton_) {
        clip->resolveBoneIndices(*skeleton_);
    }
    clip->name = name;
    clips_[name] = std::move(clip);
}

inline AnimationClip* Animator::getClip(const std::string& name) {
    auto it = clips_.find(name);
    return (it != clips_.end()) ? it->second.get() : nullptr;
}

inline const AnimationClip* Animator::getClip(const std::string& name) const {
    auto it = clips_.find(name);
    return (it != clips_.end()) ? it->second.get() : nullptr;
}

inline std::vector<std::string> Animator::getClipNames() const {
    std::vector<std::string> names;
    names.reserve(clips_.size());
    for (const auto& [name, clip] : clips_) {
        names.push_back(name);
    }
    return names;
}

inline void Animator::play(const std::string& clipName, float crossfadeDuration) {
    AnimationClip* clip = getClip(clipName);
    if (!clip) return;
    
    // Mark existing animations for blend out
    for (auto& state : activeStates_) {
        if (state.playing && !state.blendingOut) {
            state.blendingOut = true;
            state.blendTime = 0.0f;
            state.blendDuration = crossfadeDuration;
        }
    }
    
    // Add new animation state
    AnimationState newState;
    newState.clip = clip;
    newState.time = 0.0f;
    newState.playing = true;
    newState.loop = clip->looping;
    newState.weight = 0.0f;
    newState.blendingIn = (crossfadeDuration > 0.0f);
    newState.blendTime = 0.0f;
    newState.blendDuration = crossfadeDuration;
    
    if (!newState.blendingIn) {
        newState.weight = 1.0f;
    }
    
    activeStates_.push_back(newState);
    matricesDirty_ = true;
}

inline void Animator::stop() {
    activeStates_.clear();
    matricesDirty_ = true;
}

inline void Animator::update(float deltaTime) {
    if (paused_ || !skeleton_) return;
    
    float dt = deltaTime * globalSpeed_;
    
    // Update all active states
    for (auto it = activeStates_.begin(); it != activeStates_.end(); ) {
        AnimationState& state = *it;
        
        if (!state.playing) {
            ++it;
            continue;
        }
        
        // Update blend
        if (state.blendingIn) {
            state.blendTime += deltaTime;
            state.weight = std::min(1.0f, state.blendTime / state.blendDuration);
            if (state.blendTime >= state.blendDuration) {
                state.blendingIn = false;
                state.weight = 1.0f;
            }
        }
        
        if (state.blendingOut) {
            state.blendTime += deltaTime;
            state.weight = std::max(0.0f, 1.0f - state.blendTime / state.blendDuration);
            if (state.blendTime >= state.blendDuration) {
                // Remove this state
                it = activeStates_.erase(it);
                continue;
            }
        }
        
        // Update time
        state.time += dt * state.speed;
        
        // Check for animation end
        if (state.clip && state.time >= state.clip->duration) {
            if (state.loop) {
                state.time = std::fmod(state.time, state.clip->duration);
            } else {
                state.time = state.clip->duration;
                state.playing = false;
                
                if (onAnimationFinished) {
                    onAnimationFinished(state.clip->name);
                }
            }
        }
        
        ++it;
    }
    
    // Compute blended pose
    int boneCount = skeleton_->getBoneCount();
    blendedPositions_.resize(boneCount);
    blendedRotations_.resize(boneCount);
    blendedScales_.resize(boneCount);
    blendedWeights_.resize(boneCount);
    
    // Reset to identity
    for (int i = 0; i < boneCount; i++) {
        const Bone* bone = skeleton_->getBone(i);
        if (bone) {
            blendedPositions_[i] = bone->localPosition;
            blendedRotations_[i] = bone->localRotation;
            blendedScales_[i] = bone->localScale;
        } else {
            blendedPositions_[i] = Vec3(0, 0, 0);
            blendedRotations_[i] = Quat();
            blendedScales_[i] = Vec3(1, 1, 1);
        }
        blendedWeights_[i] = 0.0f;
    }
    
    // Blend all active animations
    for (const auto& state : activeStates_) {
        if (state.playing && state.clip && state.weight > 0.0f) {
            std::vector<Vec3> positions(boneCount);
            std::vector<Quat> rotations(boneCount);
            std::vector<Vec3> scales(boneCount);
            
            state.clip->sample(state.time, positions.data(), rotations.data(), scales.data(), boneCount);
            blendPose(positions.data(), rotations.data(), scales.data(), boneCount, state.weight);
        }
    }
    
    // Apply blended pose to skeleton
    for (int i = 0; i < boneCount; i++) {
        skeleton_->setBoneLocalTransform(i, blendedPositions_[i], blendedRotations_[i], blendedScales_[i]);
    }
    
    matricesDirty_ = true;
}

inline void Animator::blendPose(const Vec3* positions, const Quat* rotations, const Vec3* scales,
                                 int boneCount, float weight) {
    for (int i = 0; i < boneCount; i++) {
        float totalWeight = blendedWeights_[i] + weight;
        
        if (totalWeight > 0.0001f) {
            float t = weight / totalWeight;
            
            blendedPositions_[i] = anim::lerp(blendedPositions_[i], positions[i], t);
            blendedRotations_[i] = anim::slerp(blendedRotations_[i], rotations[i], t);
            blendedScales_[i] = anim::lerp(blendedScales_[i], scales[i], t);
            blendedWeights_[i] = totalWeight;
        }
    }
}

inline void Animator::getSkinningMatrices(Mat4* outMatrices) const {
    if (!skeleton_) {
        // Return identity matrices
        for (int i = 0; i < MAX_BONES; i++) {
            outMatrices[i] = Mat4::identity();
        }
        return;
    }
    
    skeleton_->computeSkinningMatrices(outMatrices);
}

inline float Animator::getCurrentTime() const {
    for (const auto& state : activeStates_) {
        if (state.playing && !state.blendingOut) {
            return state.time;
        }
    }
    return 0.0f;
}

inline std::string Animator::getCurrentClipName() const {
    for (const auto& state : activeStates_) {
        if (state.playing && !state.blendingOut && state.clip) {
            return state.clip->name;
        }
    }
    return "";
}

inline bool Animator::isPlaying() const {
    for (const auto& state : activeStates_) {
        if (state.playing) return true;
    }
    return false;
}

inline void Animator::setLooping(bool loop) {
    for (auto& state : activeStates_) {
        if (state.playing && !state.blendingOut) {
            state.loop = loop;
        }
    }
}

inline void Animator::setTime(float time) {
    for (auto& state : activeStates_) {
        if (state.playing && !state.blendingOut && state.clip) {
            state.time = std::clamp(time, 0.0f, state.clip->duration);
            matricesDirty_ = true;
        }
    }
    
    // Update skeleton pose immediately
    if (skeleton_ && !activeStates_.empty()) {
        int boneCount = skeleton_->getBoneCount();
        blendedPositions_.resize(boneCount);
        blendedRotations_.resize(boneCount);
        blendedScales_.resize(boneCount);
        blendedWeights_.resize(boneCount);
        
        // Reset to bind pose
        for (int i = 0; i < boneCount; i++) {
            const Bone* bone = skeleton_->getBone(i);
            if (bone) {
                blendedPositions_[i] = bone->localPosition;
                blendedRotations_[i] = bone->localRotation;
                blendedScales_[i] = bone->localScale;
            } else {
                blendedPositions_[i] = Vec3(0, 0, 0);
                blendedRotations_[i] = Quat();
                blendedScales_[i] = Vec3(1, 1, 1);
            }
            blendedWeights_[i] = 0.0f;
        }
        
        // Sample active animations at new time
        for (const auto& state : activeStates_) {
            if (state.playing && state.clip && state.weight > 0.0f) {
                std::vector<Vec3> positions(boneCount);
                std::vector<Quat> rotations(boneCount);
                std::vector<Vec3> scales(boneCount);
                
                state.clip->sample(state.time, positions.data(), rotations.data(), scales.data(), boneCount);
                blendPose(positions.data(), rotations.data(), scales.data(), boneCount, state.weight);
            }
        }
        
        // Apply to skeleton
        for (int i = 0; i < boneCount; i++) {
            skeleton_->setBoneLocalTransform(i, blendedPositions_[i], blendedRotations_[i], blendedScales_[i]);
        }
    }
}

}  // namespace luma
