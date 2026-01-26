// Blend Tree System
// Parameter-driven animation blending (like Unity's Blend Trees)
#pragma once

#include "animation_clip.h"
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>

namespace luma {

// ===== Blend Tree Types =====
enum class BlendTreeType {
    Simple1D,       // Blend based on single parameter (e.g., speed)
    Simple2D,       // Blend based on two parameters (e.g., speed + direction)
    FreeformDirectional2D,  // Cartesian 2D blending
    FreeformCartesian2D     // Polar 2D blending
};

// ===== Blend Motion =====
// A single animation entry in a blend tree
struct BlendMotion {
    AnimationClip* clip = nullptr;
    float threshold = 0.0f;      // 1D threshold
    float positionX = 0.0f;      // 2D X position
    float positionY = 0.0f;      // 2D Y position
    float speed = 1.0f;          // Playback speed multiplier
    bool mirror = false;         // Mirror animation
    float cycleOffset = 0.0f;    // Time offset (0-1)
    
    // Runtime state
    mutable float weight = 0.0f;
    mutable float time = 0.0f;
};

// ===== Blend Tree Node =====
class BlendTreeNode {
public:
    virtual ~BlendTreeNode() = default;
    
    // Evaluate the node and fill output pose
    virtual void evaluate(float deltaTime,
                          Vec3* positions, Quat* rotations, Vec3* scales,
                          int boneCount) = 0;
    
    // Set parameter value
    virtual void setParameter(const std::string& name, float value) = 0;
    
    // Get current duration
    virtual float getDuration() const = 0;
    
    // Get normalized time (0-1)
    virtual float getNormalizedTime() const = 0;
};

// ===== 1D Blend Tree =====
// Blends animations based on a single parameter
class BlendTree1D : public BlendTreeNode {
public:
    std::string parameterName = "Speed";
    std::vector<BlendMotion> motions;
    
    float parameter = 0.0f;
    float time = 0.0f;
    bool syncMotions = true;  // Sync animation times
    
    void setParameter(const std::string& name, float value) override {
        if (name == parameterName) {
            parameter = value;
        }
    }
    
    void addMotion(AnimationClip* clip, float threshold, float speed = 1.0f) {
        BlendMotion motion;
        motion.clip = clip;
        motion.threshold = threshold;
        motion.speed = speed;
        motions.push_back(motion);
        
        // Sort by threshold
        std::sort(motions.begin(), motions.end(),
            [](const BlendMotion& a, const BlendMotion& b) {
                return a.threshold < b.threshold;
            });
    }
    
    void evaluate(float deltaTime,
                  Vec3* positions, Quat* rotations, Vec3* scales,
                  int boneCount) override {
        if (motions.empty()) return;
        
        // Calculate weights based on parameter
        calculateWeights();
        
        // Update time
        time += deltaTime;
        
        // Initialize output
        for (int i = 0; i < boneCount; i++) {
            positions[i] = Vec3(0, 0, 0);
            rotations[i] = Quat();
            scales[i] = Vec3(0, 0, 0);
        }
        
        float totalWeight = 0.0f;
        
        // Blend motions
        for (auto& motion : motions) {
            if (motion.weight <= 0.0f || !motion.clip) continue;
            
            totalWeight += motion.weight;
            
            // Get motion time
            float motionTime;
            if (syncMotions) {
                // Sync time across all motions
                float normalizedTime = motion.clip->duration > 0 ? 
                    std::fmod(time * motion.speed, motion.clip->duration) / motion.clip->duration : 0.0f;
                motionTime = normalizedTime * motion.clip->duration;
            } else {
                motion.time += deltaTime * motion.speed;
                if (motion.clip->looping && motion.time > motion.clip->duration) {
                    motion.time = std::fmod(motion.time, motion.clip->duration);
                }
                motionTime = motion.time;
            }
            
            // Sample motion
            std::vector<Vec3> motionPos(boneCount);
            std::vector<Quat> motionRot(boneCount);
            std::vector<Vec3> motionScl(boneCount);
            motion.clip->sample(motionTime, motionPos.data(), motionRot.data(), motionScl.data(), boneCount);
            
            // Accumulate
            for (int i = 0; i < boneCount; i++) {
                positions[i] = positions[i] + motionPos[i] * motion.weight;
                scales[i] = scales[i] + motionScl[i] * motion.weight;
            }
        }
        
        // Normalize
        if (totalWeight > 0.0f) {
            float invWeight = 1.0f / totalWeight;
            for (int i = 0; i < boneCount; i++) {
                positions[i] = positions[i] * invWeight;
                scales[i] = scales[i] * invWeight;
            }
        }
        
        // For rotations, use sequential slerp (more accurate but slower)
        for (int i = 0; i < boneCount; i++) {
            Quat result;
            float accWeight = 0.0f;
            
            for (const auto& motion : motions) {
                if (motion.weight <= 0.0f || !motion.clip) continue;
                
                float motionTime = syncMotions ? 
                    std::fmod(time * motion.speed, motion.clip->duration) : motion.time;
                
                std::vector<Quat> motionRot(boneCount);
                Vec3 dummy[1];
                motion.clip->sample(motionTime, dummy, motionRot.data(), dummy, boneCount);
                
                float newWeight = accWeight + motion.weight;
                if (newWeight > 0.0f) {
                    float t = motion.weight / newWeight;
                    result = anim::slerp(result, motionRot[i], t);
                }
                accWeight = newWeight;
            }
            
            rotations[i] = result;
        }
    }
    
    float getDuration() const override {
        // Return duration of dominant motion
        float maxWeight = 0.0f;
        float duration = 0.0f;
        for (const auto& motion : motions) {
            if (motion.weight > maxWeight && motion.clip) {
                maxWeight = motion.weight;
                duration = motion.clip->duration;
            }
        }
        return duration;
    }
    
    float getNormalizedTime() const override {
        float duration = getDuration();
        return duration > 0 ? std::fmod(time, duration) / duration : 0.0f;
    }
    
private:
    void calculateWeights() {
        if (motions.empty()) return;
        
        // Reset weights
        for (auto& motion : motions) {
            motion.weight = 0.0f;
        }
        
        // Find the two motions to blend between
        int lowerIdx = 0;
        int upperIdx = 0;
        
        for (size_t i = 0; i < motions.size(); i++) {
            if (motions[i].threshold <= parameter) {
                lowerIdx = static_cast<int>(i);
            }
            if (motions[i].threshold >= parameter && upperIdx == 0) {
                upperIdx = static_cast<int>(i);
            }
        }
        
        // Clamp to range
        if (parameter <= motions.front().threshold) {
            motions.front().weight = 1.0f;
            return;
        }
        if (parameter >= motions.back().threshold) {
            motions.back().weight = 1.0f;
            return;
        }
        
        // Blend between adjacent motions
        if (lowerIdx == upperIdx) {
            motions[lowerIdx].weight = 1.0f;
        } else {
            float range = motions[upperIdx].threshold - motions[lowerIdx].threshold;
            if (range > 0.0001f) {
                float t = (parameter - motions[lowerIdx].threshold) / range;
                motions[lowerIdx].weight = 1.0f - t;
                motions[upperIdx].weight = t;
            } else {
                motions[lowerIdx].weight = 0.5f;
                motions[upperIdx].weight = 0.5f;
            }
        }
    }
};

// ===== 2D Blend Tree =====
// Blends animations based on two parameters (e.g., velocity X and Y)
class BlendTree2D : public BlendTreeNode {
public:
    std::string parameterX = "VelocityX";
    std::string parameterY = "VelocityY";
    std::vector<BlendMotion> motions;
    BlendTreeType type = BlendTreeType::FreeformCartesian2D;
    
    float paramX = 0.0f;
    float paramY = 0.0f;
    float time = 0.0f;
    bool syncMotions = true;
    
    void setParameter(const std::string& name, float value) override {
        if (name == parameterX) paramX = value;
        if (name == parameterY) paramY = value;
    }
    
    void addMotion(AnimationClip* clip, float posX, float posY, float speed = 1.0f) {
        BlendMotion motion;
        motion.clip = clip;
        motion.positionX = posX;
        motion.positionY = posY;
        motion.speed = speed;
        motions.push_back(motion);
    }
    
    void evaluate(float deltaTime,
                  Vec3* positions, Quat* rotations, Vec3* scales,
                  int boneCount) override {
        if (motions.empty()) return;
        
        // Calculate weights
        calculateWeights2D();
        
        // Update time
        time += deltaTime;
        
        // Initialize output
        for (int i = 0; i < boneCount; i++) {
            positions[i] = Vec3(0, 0, 0);
            rotations[i] = Quat();
            scales[i] = Vec3(0, 0, 0);
        }
        
        float totalWeight = 0.0f;
        
        // Blend motions (same as 1D)
        for (auto& motion : motions) {
            if (motion.weight <= 0.0f || !motion.clip) continue;
            
            totalWeight += motion.weight;
            
            float motionTime = syncMotions ? 
                std::fmod(time * motion.speed, motion.clip->duration) : 
                (motion.time += deltaTime * motion.speed, 
                 motion.clip->looping ? std::fmod(motion.time, motion.clip->duration) : motion.time);
            
            std::vector<Vec3> motionPos(boneCount);
            std::vector<Quat> motionRot(boneCount);
            std::vector<Vec3> motionScl(boneCount);
            motion.clip->sample(motionTime, motionPos.data(), motionRot.data(), motionScl.data(), boneCount);
            
            for (int i = 0; i < boneCount; i++) {
                positions[i] = positions[i] + motionPos[i] * motion.weight;
                scales[i] = scales[i] + motionScl[i] * motion.weight;
            }
        }
        
        if (totalWeight > 0.0f) {
            float invWeight = 1.0f / totalWeight;
            for (int i = 0; i < boneCount; i++) {
                positions[i] = positions[i] * invWeight;
                scales[i] = scales[i] * invWeight;
            }
        }
        
        // Rotation blending (simplified)
        for (int i = 0; i < boneCount; i++) {
            Quat result;
            float accWeight = 0.0f;
            
            for (const auto& motion : motions) {
                if (motion.weight <= 0.0f || !motion.clip) continue;
                
                float motionTime = std::fmod(time * motion.speed, motion.clip->duration);
                std::vector<Quat> motionRot(boneCount);
                Vec3 dummy[1];
                motion.clip->sample(motionTime, dummy, motionRot.data(), dummy, boneCount);
                
                float newWeight = accWeight + motion.weight;
                if (newWeight > 0.0f) {
                    result = anim::slerp(result, motionRot[i], motion.weight / newWeight);
                }
                accWeight = newWeight;
            }
            
            rotations[i] = result;
        }
    }
    
    float getDuration() const override {
        float maxWeight = 0.0f;
        float duration = 0.0f;
        for (const auto& motion : motions) {
            if (motion.weight > maxWeight && motion.clip) {
                maxWeight = motion.weight;
                duration = motion.clip->duration;
            }
        }
        return duration;
    }
    
    float getNormalizedTime() const override {
        float duration = getDuration();
        return duration > 0 ? std::fmod(time, duration) / duration : 0.0f;
    }
    
private:
    void calculateWeights2D() {
        if (motions.empty()) return;
        
        // Reset weights
        for (auto& motion : motions) {
            motion.weight = 0.0f;
        }
        
        // Calculate inverse distance weights
        std::vector<float> distances(motions.size());
        float sumInverseDistance = 0.0f;
        
        for (size_t i = 0; i < motions.size(); i++) {
            float dx = paramX - motions[i].positionX;
            float dy = paramY - motions[i].positionY;
            distances[i] = std::sqrt(dx * dx + dy * dy);
            
            // Very close to a motion - use it directly
            if (distances[i] < 0.001f) {
                motions[i].weight = 1.0f;
                return;
            }
            
            // Inverse distance weighting
            float inverseDistance = 1.0f / (distances[i] * distances[i]);
            sumInverseDistance += inverseDistance;
        }
        
        // Normalize weights
        if (sumInverseDistance > 0.0f) {
            for (size_t i = 0; i < motions.size(); i++) {
                float inverseDistance = 1.0f / (distances[i] * distances[i]);
                motions[i].weight = inverseDistance / sumInverseDistance;
            }
        }
    }
};

// ===== Blend Tree Factory =====
namespace BlendTreeFactory {

// Create a locomotion blend tree (idle -> walk -> run)
inline std::unique_ptr<BlendTree1D> createLocomotionTree(
    AnimationClip* idle,
    AnimationClip* walk,
    AnimationClip* run) 
{
    auto tree = std::make_unique<BlendTree1D>();
    tree->parameterName = "Speed";
    
    if (idle) tree->addMotion(idle, 0.0f, 1.0f);
    if (walk) tree->addMotion(walk, 0.5f, 1.0f);
    if (run) tree->addMotion(run, 1.0f, 1.0f);
    
    return tree;
}

// Create a directional movement tree (8-way)
inline std::unique_ptr<BlendTree2D> createDirectionalTree(
    AnimationClip* forward,
    AnimationClip* backward,
    AnimationClip* left,
    AnimationClip* right,
    AnimationClip* forwardLeft = nullptr,
    AnimationClip* forwardRight = nullptr,
    AnimationClip* backwardLeft = nullptr,
    AnimationClip* backwardRight = nullptr)
{
    auto tree = std::make_unique<BlendTree2D>();
    tree->parameterX = "DirectionX";
    tree->parameterY = "DirectionY";
    
    // Cardinal directions
    if (forward) tree->addMotion(forward, 0.0f, 1.0f);
    if (backward) tree->addMotion(backward, 0.0f, -1.0f);
    if (left) tree->addMotion(left, -1.0f, 0.0f);
    if (right) tree->addMotion(right, 1.0f, 0.0f);
    
    // Diagonal directions
    const float diag = 0.707f;  // sqrt(2)/2
    if (forwardLeft) tree->addMotion(forwardLeft, -diag, diag);
    if (forwardRight) tree->addMotion(forwardRight, diag, diag);
    if (backwardLeft) tree->addMotion(backwardLeft, -diag, -diag);
    if (backwardRight) tree->addMotion(backwardRight, diag, -diag);
    
    return tree;
}

}  // namespace BlendTreeFactory

}  // namespace luma
