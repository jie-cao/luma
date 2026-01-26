// Animation System Test Utilities
// Creates procedural animated skeletons for testing
#pragma once

#include "engine/animation/animation.h"
#include "engine/renderer/mesh.h"
#include <cmath>
#include <iostream>

namespace luma {
namespace test {

// Create a simple 3-bone arm skeleton for testing
// Bone 0: Shoulder (root)
// Bone 1: Elbow (child of shoulder)
// Bone 2: Wrist (child of elbow)
inline std::unique_ptr<Skeleton> createTestArmSkeleton() {
    auto skeleton = std::make_unique<Skeleton>();
    
    // Add bones
    skeleton->addBone("Shoulder", -1);  // Root bone, no parent
    skeleton->addBone("Elbow", 0);      // Parent is Shoulder
    skeleton->addBone("Wrist", 1);      // Parent is Elbow
    
    // Set inverse bind matrices (identity for simplicity)
    Mat4 identity = Mat4::identity();
    skeleton->setInverseBindMatrix(0, identity);
    skeleton->setInverseBindMatrix(1, identity);
    skeleton->setInverseBindMatrix(2, identity);
    
    return skeleton;
}

// Create a waving animation for the test arm
inline std::unique_ptr<AnimationClip> createWaveAnimation() {
    auto clip = std::make_unique<AnimationClip>();
    clip->name = "Wave";
    clip->duration = 2.0f;  // 2 second loop
    
    // Channel for Shoulder (bone 0) - stays still
    AnimationChannel shoulderChannel;
    shoulderChannel.targetBoneIndex = 0;
    shoulderChannel.targetBone = "Shoulder";
    shoulderChannel.positionKeys.push_back({0.0f, Vec3(0, 0, 0)});
    shoulderChannel.rotationKeys.push_back({0.0f, Quat()});
    shoulderChannel.scaleKeys.push_back({0.0f, Vec3(1, 1, 1)});
    clip->channels.push_back(std::move(shoulderChannel));
    
    // Channel for Elbow (bone 1) - rotates back and forth
    AnimationChannel elbowChannel;
    elbowChannel.targetBoneIndex = 1;
    elbowChannel.targetBone = "Elbow";
    elbowChannel.positionKeys.push_back({0.0f, Vec3(1, 0, 0)});  // Offset from shoulder
    // Rotation keyframes for wave motion
    elbowChannel.rotationKeys.push_back({0.0f, Quat::fromEuler(0, 0, 0)});
    elbowChannel.rotationKeys.push_back({0.5f, Quat::fromEuler(0, 0, 0.5f)});  // Bend up
    elbowChannel.rotationKeys.push_back({1.0f, Quat::fromEuler(0, 0, 0)});
    elbowChannel.rotationKeys.push_back({1.5f, Quat::fromEuler(0, 0, -0.5f)}); // Bend down
    elbowChannel.rotationKeys.push_back({2.0f, Quat::fromEuler(0, 0, 0)});
    elbowChannel.scaleKeys.push_back({0.0f, Vec3(1, 1, 1)});
    clip->channels.push_back(std::move(elbowChannel));
    
    // Channel for Wrist (bone 2) - also waves
    AnimationChannel wristChannel;
    wristChannel.targetBoneIndex = 2;
    wristChannel.targetBone = "Wrist";
    wristChannel.positionKeys.push_back({0.0f, Vec3(1, 0, 0)});  // Offset from elbow
    // Faster wave for wrist
    wristChannel.rotationKeys.push_back({0.0f, Quat::fromEuler(0, 0, 0)});
    wristChannel.rotationKeys.push_back({0.25f, Quat::fromEuler(0, 0, 0.3f)});
    wristChannel.rotationKeys.push_back({0.5f, Quat::fromEuler(0, 0, 0)});
    wristChannel.rotationKeys.push_back({0.75f, Quat::fromEuler(0, 0, -0.3f)});
    wristChannel.rotationKeys.push_back({1.0f, Quat::fromEuler(0, 0, 0)});
    wristChannel.rotationKeys.push_back({1.25f, Quat::fromEuler(0, 0, 0.3f)});
    wristChannel.rotationKeys.push_back({1.5f, Quat::fromEuler(0, 0, 0)});
    wristChannel.rotationKeys.push_back({1.75f, Quat::fromEuler(0, 0, -0.3f)});
    wristChannel.rotationKeys.push_back({2.0f, Quat::fromEuler(0, 0, 0)});
    wristChannel.scaleKeys.push_back({0.0f, Vec3(1, 1, 1)});
    clip->channels.push_back(std::move(wristChannel));
    
    return clip;
}

// Create a simple skinned mesh (3 connected cubes representing arm segments)
inline Mesh createTestArmMesh() {
    Mesh mesh;
    
    // Segment dimensions
    float segmentLength = 1.0f;
    float segmentWidth = 0.2f;
    
    auto addCubeVertices = [&](float xOffset, int boneIdx) {
        uint32_t baseIdx = static_cast<uint32_t>(mesh.vertices.size());
        float x0 = xOffset;
        float x1 = xOffset + segmentLength;
        float hw = segmentWidth / 2.0f;
        
        // 8 corners of the cube
        float positions[8][3] = {
            {x0, -hw, -hw}, {x1, -hw, -hw}, {x1,  hw, -hw}, {x0,  hw, -hw},  // back
            {x0, -hw,  hw}, {x1, -hw,  hw}, {x1,  hw,  hw}, {x0,  hw,  hw}   // front
        };
        
        // 6 faces, 4 vertices each (24 vertices per cube)
        int faceIndices[6][4] = {
            {0, 1, 2, 3},  // back
            {4, 7, 6, 5},  // front
            {0, 4, 5, 1},  // bottom
            {2, 6, 7, 3},  // top
            {0, 3, 7, 4},  // left
            {1, 5, 6, 2}   // right
        };
        
        float normals[6][3] = {
            {0, 0, -1}, {0, 0, 1}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}
        };
        
        for (int face = 0; face < 6; face++) {
            for (int v = 0; v < 4; v++) {
                Vertex vert;
                int pi = faceIndices[face][v];
                vert.position[0] = positions[pi][0];
                vert.position[1] = positions[pi][1];
                vert.position[2] = positions[pi][2];
                vert.normal[0] = normals[face][0];
                vert.normal[1] = normals[face][1];
                vert.normal[2] = normals[face][2];
                vert.tangent[0] = 1; vert.tangent[1] = 0; vert.tangent[2] = 0; vert.tangent[3] = 1;
                vert.uv[0] = (v == 0 || v == 3) ? 0.0f : 1.0f;
                vert.uv[1] = (v == 0 || v == 1) ? 0.0f : 1.0f;
                vert.color[0] = vert.color[1] = vert.color[2] = 1.0f;
                mesh.vertices.push_back(vert);
                
                // Skinned vertex data
                SkinnedVertex sv;
                sv.boneIndices[0] = boneIdx;
                sv.boneIndices[1] = sv.boneIndices[2] = sv.boneIndices[3] = 0;
                sv.boneWeights[0] = 1.0f;
                sv.boneWeights[1] = sv.boneWeights[2] = sv.boneWeights[3] = 0.0f;
                mesh.skinnedVertices.push_back(sv);
            }
            
            // Add indices for 2 triangles per face
            uint32_t faceBase = baseIdx + face * 4;
            mesh.indices.push_back(faceBase + 0);
            mesh.indices.push_back(faceBase + 1);
            mesh.indices.push_back(faceBase + 2);
            mesh.indices.push_back(faceBase + 0);
            mesh.indices.push_back(faceBase + 2);
            mesh.indices.push_back(faceBase + 3);
        }
    };
    
    // Add 3 cube segments, each bound to one bone
    addCubeVertices(0.0f, 0);  // Shoulder segment
    addCubeVertices(1.0f, 1);  // Elbow segment
    addCubeVertices(2.0f, 2);  // Wrist segment
    
    mesh.hasSkeleton = true;
    mesh.baseColor[0] = 0.8f;
    mesh.baseColor[1] = 0.6f;
    mesh.baseColor[2] = 0.4f;
    mesh.metallic = 0.0f;
    mesh.roughness = 0.7f;
    
    return mesh;
}

// Run animation system unit tests
inline bool runAnimationTests() {
    std::cout << "\n=== Animation System Tests ===" << std::endl;
    bool allPassed = true;
    
    // Test 1: Skeleton creation
    std::cout << "[Test 1] Skeleton creation... ";
    auto skeleton = createTestArmSkeleton();
    if (skeleton && skeleton->getBoneCount() == 3) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        allPassed = false;
    }
    
    // Test 2: Animation clip creation
    std::cout << "[Test 2] Animation clip creation... ";
    auto clip = createWaveAnimation();
    if (clip && clip->duration == 2.0f && clip->channels.size() == 3) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        allPassed = false;
    }
    
    // Test 3: Animator playback
    std::cout << "[Test 3] Animator playback... ";
    Animator animator;
    animator.setSkeleton(skeleton.get());
    auto clipCopy = std::make_unique<AnimationClip>(*clip);
    animator.addClip("Wave", std::move(clipCopy));
    animator.play("Wave");
    animator.update(0.5f);  // Advance half a second
    
    Mat4 boneMatrices[MAX_BONES];
    animator.getSkinningMatrices(boneMatrices);
    
    // Check that matrices are different from identity (animation applied)
    bool matricesChanged = false;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 16; j++) {
            float expected = (j % 5 == 0) ? 1.0f : 0.0f;  // Identity diagonal
            if (std::abs(boneMatrices[i].m[j] - expected) > 0.001f) {
                matricesChanged = true;
                break;
            }
        }
    }
    if (matricesChanged) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED (matrices not animated)" << std::endl;
        allPassed = false;
    }
    
    // Test 4: Animation time tracking
    std::cout << "[Test 4] Animation time tracking... ";
    float time = animator.getCurrentTime();
    if (std::abs(time - 0.5f) < 0.001f) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED (expected 0.5, got " << time << ")" << std::endl;
        allPassed = false;
    }
    
    // Test 5: Mesh with skinning data
    std::cout << "[Test 5] Skinned mesh creation... ";
    Mesh armMesh = createTestArmMesh();
    if (armMesh.hasSkeleton && 
        armMesh.vertices.size() == armMesh.skinnedVertices.size() &&
        armMesh.skinnedVertices.size() > 0) {
        std::cout << "PASSED (" << armMesh.vertices.size() << " vertices)" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        allPassed = false;
    }
    
    // Test 6: Looping
    std::cout << "[Test 6] Animation looping... ";
    animator.setLooping(true);
    animator.update(2.0f);  // Advance past duration (now at 2.5s total)
    float loopedTime = animator.getCurrentTime();
    if (loopedTime < 1.0f) {  // Should have wrapped
        std::cout << "PASSED (time wrapped to " << loopedTime << ")" << std::endl;
    } else {
        std::cout << "FAILED (time = " << loopedTime << ")" << std::endl;
        allPassed = false;
    }
    
    // Test 7: Stop/Reset
    std::cout << "[Test 7] Animation stop/reset... ";
    animator.stop();
    if (!animator.isPlaying()) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        allPassed = false;
    }
    
    std::cout << "\n=== Animation Tests " << (allPassed ? "ALL PASSED" : "SOME FAILED") << " ===" << std::endl;
    return allPassed;
}

}  // namespace test
}  // namespace luma
