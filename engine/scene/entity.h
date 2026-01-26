// Entity - Scene graph node with transform and hierarchy
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/animation/animation.h"
#include "engine/material/material.h"
#include "engine/lighting/light.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>

namespace luma {

// ===== Transform Component =====
struct Transform {
    Vec3 position{0, 0, 0};
    Quat rotation{};
    Vec3 scale{1, 1, 1};
    
    Mat4 toMatrix() const {
        return Mat4::translation(position) * Mat4::fromQuat(rotation) * Mat4::scale(scale);
    }
    
    // Get Euler angles in degrees for UI
    Vec3 getEulerDegrees() const {
        Vec3 rad = rotation.toEuler();
        return {rad.x * 57.2958f, rad.y * 57.2958f, rad.z * 57.2958f};
    }
    
    void setEulerDegrees(const Vec3& deg) {
        rotation = Quat::fromEuler(deg.x * 0.0174533f, deg.y * 0.0174533f, deg.z * 0.0174533f);
    }
};

// ===== Entity =====
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;

class Entity {
public:
    EntityID id = INVALID_ENTITY;
    std::string name = "Entity";
    bool enabled = true;
    
    // Transform
    Transform localTransform;
    Mat4 worldMatrix;  // Cached world transform
    
    // Hierarchy
    Entity* parent = nullptr;
    std::vector<Entity*> children;
    
    // Rendering (optional - entity may not have a model)
    bool hasModel = false;
    RHILoadedModel model;
    
    // Material (optional - uses default material if nullptr)
    std::shared_ptr<Material> material;
    
    // Light component (optional)
    bool hasLight = false;
    Light light;
    
    // Animation (optional - entity may not have a skeleton)
    std::unique_ptr<Skeleton> skeleton;
    std::unique_ptr<Animator> animator;
    std::unordered_map<std::string, std::unique_ptr<AnimationClip>> animationClips;
    
    bool hasSkeleton() const { return skeleton && skeleton->getBoneCount() > 0; }
    bool hasAnimations() const { return !animationClips.empty(); }
    
    // Initialize animator with skeleton and clips
    void setupAnimator() {
        if (!skeleton) return;
        if (!animator) {
            animator = std::make_unique<Animator>();
        }
        animator->setSkeleton(skeleton.get());
        
        // Add clips to animator
        for (auto& [name, clip] : animationClips) {
            auto clipCopy = std::make_unique<AnimationClip>(*clip);
            animator->addClip(name, std::move(clipCopy));
        }
    }
    
    // Get skinning matrices for rendering
    void getSkinningMatrices(Mat4* outMatrices) const {
        if (animator) {
            animator->getSkinningMatrices(outMatrices);
        } else if (skeleton) {
            skeleton->computeSkinningMatrices(outMatrices);
        } else {
            // Return identity matrices
            for (uint32_t i = 0; i < MAX_BONES; i++) {
                outMatrices[i] = Mat4::identity();
            }
        }
    }
    
    // Update world matrix from local transform and parent
    void updateWorldMatrix() {
        if (parent) {
            worldMatrix = parent->worldMatrix * localTransform.toMatrix();
        } else {
            worldMatrix = localTransform.toMatrix();
        }
        // Update children
        for (auto* child : children) {
            child->updateWorldMatrix();
        }
    }
    
    // Get world position
    Vec3 getWorldPosition() const {
        return {worldMatrix.m[12], worldMatrix.m[13], worldMatrix.m[14]};
    }
    
    // Add child entity
    void addChild(Entity* child) {
        if (child->parent) {
            child->parent->removeChild(child);
        }
        child->parent = this;
        children.push_back(child);
        child->updateWorldMatrix();
    }
    
    // Remove child entity
    void removeChild(Entity* child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            child->parent = nullptr;
        }
    }
};

}  // namespace luma
