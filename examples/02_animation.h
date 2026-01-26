// Example 02: Animation System
// Demonstrates skeletal animation, blending, and state machines
#pragma once

#include "engine/animation/animation.h"
#include <iostream>
#include <memory>

namespace luma {
namespace examples {

// ============================================
// Example 1: Basic Skeletal Animation
// ============================================

inline void example_BasicAnimation() {
    // 1. Create a skeleton
    Skeleton skeleton;
    
    // Add bones (returns bone index)
    int root = skeleton.addBone("root", -1);         // No parent
    int spine = skeleton.addBone("spine", root);
    int head = skeleton.addBone("head", spine);
    int armL = skeleton.addBone("arm_l", spine);
    int armR = skeleton.addBone("arm_r", spine);
    
    std::cout << "Created skeleton with " << skeleton.getBoneCount() << " bones\n";
    
    // 2. Set bone rest positions
    Bone* spineBone = const_cast<Bone*>(skeleton.getBone(spine));
    spineBone->localPosition = Vec3(0, 1, 0);
    
    Bone* headBone = const_cast<Bone*>(skeleton.getBone(head));
    headBone->localPosition = Vec3(0, 0.5f, 0);
    
    // 3. Create an animation clip
    AnimationClip walkClip;
    walkClip.name = "walk";
    walkClip.duration = 1.0f;
    walkClip.looping = true;
    
    // Add animation channel for spine
    auto& spineChannel = walkClip.addChannel("spine");
    
    // Add rotation keyframes (bobbing motion)
    QuatKeyframe k0, k1, k2;
    k0.time = 0.0f;   k0.value = Quat::fromEuler(0.1f, 0, 0);
    k1.time = 0.5f;   k1.value = Quat::fromEuler(-0.1f, 0, 0);
    k2.time = 1.0f;   k2.value = Quat::fromEuler(0.1f, 0, 0);
    
    spineChannel.rotationKeys = {k0, k1, k2};
    
    // 4. Create animator and play
    Animator animator;
    animator.setSkeleton(&skeleton);
    animator.addClip("walk", std::make_unique<AnimationClip>(walkClip));
    
    animator.play("walk");
    std::cout << "Playing: " << animator.getCurrentClipName() << "\n";
    
    // 5. Update animation
    for (int frame = 0; frame < 60; frame++) {
        animator.update(1.0f / 60.0f);  // 60 FPS
    }
    
    std::cout << "Current time: " << animator.getCurrentTime() << "s\n";
    
    // 6. Get skinning matrices for rendering
    Mat4 skinningMatrices[MAX_BONES];
    animator.getSkinningMatrices(skinningMatrices);
}

// ============================================
// Example 2: Animation Blending
// ============================================

inline void example_AnimationBlending() {
    Skeleton skeleton;
    skeleton.addBone("root", -1);
    skeleton.addBone("spine", 0);
    
    Animator animator;
    animator.setSkeleton(&skeleton);
    
    // Create idle animation
    auto idle = std::make_unique<AnimationClip>();
    idle->name = "idle";
    idle->duration = 2.0f;
    idle->looping = true;
    animator.addClip("idle", std::move(idle));
    
    // Create walk animation
    auto walk = std::make_unique<AnimationClip>();
    walk->name = "walk";
    walk->duration = 1.0f;
    walk->looping = true;
    animator.addClip("walk", std::move(walk));
    
    // Play idle
    animator.play("idle");
    animator.update(0.5f);
    
    // Crossfade to walk (0.3s blend time)
    animator.play("walk", 0.3f);
    
    // During crossfade, both animations contribute
    for (int i = 0; i < 30; i++) {
        animator.update(1.0f / 60.0f);
    }
    
    std::cout << "Now playing: " << animator.getCurrentClipName() << "\n";
}

// ============================================
// Example 3: Blend Tree (1D)
// ============================================

inline void example_BlendTree1D() {
    // Create a 1D blend tree for locomotion
    BlendTree1D locomotion;
    locomotion.parameterName = "Speed";
    
    // Add motions at different speed thresholds
    // (In real use, these would be actual AnimationClips)
    locomotion.addMotion(nullptr, 0.0f, 1.0f);   // Idle at speed 0
    locomotion.addMotion(nullptr, 0.5f, 1.0f);   // Walk at speed 0.5
    locomotion.addMotion(nullptr, 1.0f, 1.0f);   // Run at speed 1.0
    
    // Set parameter
    locomotion.setParameter("Speed", 0.3f);  // Blend between idle and walk
    
    std::cout << "BlendTree1D with " << locomotion.motions.size() << " motions\n";
    std::cout << "Speed parameter: 0.3 (blending idle and walk)\n";
}

// ============================================
// Example 4: Blend Tree (2D)
// ============================================

inline void example_BlendTree2D() {
    // Create a 2D blend tree for directional movement
    BlendTree2D directional;
    directional.parameterX = "VelocityX";
    directional.parameterY = "VelocityY";
    
    // Add motions at positions (8-way)
    directional.addMotion(nullptr, 0.0f, 1.0f);    // Forward
    directional.addMotion(nullptr, 0.0f, -1.0f);   // Backward
    directional.addMotion(nullptr, -1.0f, 0.0f);   // Left
    directional.addMotion(nullptr, 1.0f, 0.0f);    // Right
    directional.addMotion(nullptr, 0.707f, 0.707f);   // Forward-Right
    directional.addMotion(nullptr, -0.707f, 0.707f);  // Forward-Left
    
    // Set direction
    directional.setParameter("VelocityX", 0.5f);
    directional.setParameter("VelocityY", 0.5f);  // Moving forward-right
    
    std::cout << "BlendTree2D with " << directional.motions.size() << " motions\n";
}

// ============================================
// Example 5: Animation State Machine
// ============================================

inline void example_StateMachine() {
    AnimationStateMachine sm;
    
    // 1. Add parameters
    sm.addParameter("Speed", ParameterType::Float);
    sm.addParameter("IsGrounded", ParameterType::Bool);
    sm.addParameter("Jump", ParameterType::Trigger);
    
    // 2. Create states
    auto* idle = sm.createState("Idle");
    idle->loop = true;
    
    auto* walk = sm.createState("Walk");
    walk->loop = true;
    
    auto* run = sm.createState("Run");
    run->loop = true;
    
    auto* jump = sm.createState("Jump");
    jump->loop = false;
    
    // 3. Add transitions
    // Idle -> Walk (when speed > 0.1)
    auto& idleToWalk = idle->addTransition("Walk");
    idleToWalk.conditions.push_back({"Speed", ConditionMode::Greater, 0.1f});
    idleToWalk.duration = 0.2f;
    
    // Walk -> Run (when speed > 0.6)
    auto& walkToRun = walk->addTransition("Run");
    walkToRun.conditions.push_back({"Speed", ConditionMode::Greater, 0.6f});
    walkToRun.duration = 0.15f;
    
    // Run -> Walk (when speed < 0.5)
    auto& runToWalk = run->addTransition("Walk");
    runToWalk.conditions.push_back({"Speed", ConditionMode::Less, 0.5f});
    runToWalk.duration = 0.15f;
    
    // Walk -> Idle (when speed < 0.1)
    auto& walkToIdle = walk->addTransition("Idle");
    walkToIdle.conditions.push_back({"Speed", ConditionMode::Less, 0.1f});
    walkToIdle.duration = 0.3f;
    
    // Any State -> Jump (on Jump trigger)
    auto& anyToJump = sm.addAnyStateTransition("Jump");
    anyToJump.conditions.push_back({"Jump", ConditionMode::If, 1.0f});
    anyToJump.conditions.push_back({"IsGrounded", ConditionMode::If, 1.0f});
    
    // Jump -> Idle (after animation ends)
    auto& jumpToIdle = jump->addTransition("Idle");
    jumpToIdle.hasExitTime = true;
    jumpToIdle.exitTime = 0.9f;
    
    // 4. Set default and start
    sm.setDefaultState("Idle");
    sm.start();
    
    std::cout << "State Machine started in: " << sm.getCurrentStateName() << "\n";
    
    // 5. Simulate gameplay
    sm.setFloat("Speed", 0.0f);
    sm.setBool("IsGrounded", true);
    sm.update(0.1f);
    std::cout << "State: " << sm.getCurrentStateName() << "\n";
    
    // Start walking
    sm.setFloat("Speed", 0.4f);
    sm.update(0.3f);
    std::cout << "State: " << sm.getCurrentStateName() << "\n";
    
    // Start running
    sm.setFloat("Speed", 0.8f);
    sm.update(0.3f);
    std::cout << "State: " << sm.getCurrentStateName() << "\n";
    
    // Jump!
    sm.setTrigger("Jump");
    sm.update(0.1f);
    std::cout << "State: " << sm.getCurrentStateName() << "\n";
}

// ============================================
// Example 6: Animation Layers
// ============================================

inline void example_AnimationLayers() {
    Skeleton skeleton;
    skeleton.addBone("root", -1);
    skeleton.addBone("spine", 0);
    skeleton.addBone("arm_l", 1);
    skeleton.addBone("arm_r", 1);
    skeleton.addBone("leg_l", 0);
    skeleton.addBone("leg_r", 0);
    
    // Create layer manager
    AnimationLayerManager layerManager;
    layerManager.setSkeleton(&skeleton);
    
    // Base layer (full body locomotion)
    auto* baseLayer = layerManager.getBaseLayer();
    baseLayer->name = "Locomotion";
    baseLayer->weight = 1.0f;
    baseLayer->blendMode = AnimationBlendMode::Override;
    
    // Upper body layer (for shooting/waving while walking)
    auto* upperLayer = layerManager.createLayer("UpperBody");
    upperLayer->weight = 1.0f;
    upperLayer->blendMode = AnimationBlendMode::Override;
    
    // Create mask for upper body
    upperLayer->mask.addBoneRecursive(skeleton, "spine");
    upperLayer->mask.resolve(skeleton);
    
    // Additive layer (for breathing/reactions)
    auto* additiveLayer = layerManager.createLayer("Additive");
    additiveLayer->weight = 0.5f;
    additiveLayer->blendMode = AnimationBlendMode::Additive;
    
    std::cout << "Created " << layerManager.getLayerCount() << " animation layers\n";
    
    // Update all layers
    layerManager.update(1.0f / 60.0f);
    
    // Evaluate final pose
    int boneCount = skeleton.getBoneCount();
    std::vector<Vec3> positions(boneCount);
    std::vector<Quat> rotations(boneCount);
    std::vector<Vec3> scales(boneCount);
    
    layerManager.evaluate(positions.data(), rotations.data(), scales.data(), boneCount);
}

// ============================================
// Example 7: Inverse Kinematics
// ============================================

inline void example_IK() {
    // Create a simple arm skeleton
    Skeleton skeleton;
    int shoulder = skeleton.addBone("shoulder", -1);
    int elbow = skeleton.addBone("elbow", shoulder);
    int hand = skeleton.addBone("hand", elbow);
    
    // Set bone positions (rest pose)
    Bone* shoulderBone = const_cast<Bone*>(skeleton.getBone(shoulder));
    Bone* elbowBone = const_cast<Bone*>(skeleton.getBone(elbow));
    Bone* handBone = const_cast<Bone*>(skeleton.getBone(hand));
    
    shoulderBone->localPosition = Vec3(0, 0, 0);
    elbowBone->localPosition = Vec3(1, 0, 0);   // 1 unit upper arm
    handBone->localPosition = Vec3(1, 0, 0);    // 1 unit forearm
    
    // Create IK manager
    IKManager ikManager;
    
    // Setup arm IK
    int armIK = ikManager.setupArmIK(shoulder, elbow, hand);
    
    // Set target (hand should reach this point)
    Vec3 target(1.5f, 0.5f, 0);  // Within reach
    ikManager.setHandTarget(armIK, target, 1.0f);
    
    // Solve IK
    ikManager.solve(skeleton);
    
    std::cout << "IK solved - hand reaching toward target\n";
    
    // === Look-At IK ===
    Skeleton headSkeleton;
    int headBone = headSkeleton.addBone("head", -1);
    
    int lookAtIK = ikManager.setupHeadLookAt(headBone);
    ikManager.setLookAtTarget(lookAtIK, Vec3(0, 0, 10), 1.0f);  // Look forward
    
    ikManager.solve(headSkeleton);
    std::cout << "Head looking at target\n";
}

}  // namespace examples
}  // namespace luma
