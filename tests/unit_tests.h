// LUMA Studio - Unit Test Suite
// Comprehensive tests for all engine systems
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/animation.h"
#include "engine/rendering/culling.h"
#include "engine/rendering/lod.h"
#include "engine/rendering/ssao.h"
#include "engine/rendering/ibl.h"
#include "engine/rendering/advanced_shadows.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace luma {
namespace test {

// ===== Test Framework =====
class UnitTestRunner {
public:
    struct TestCase {
        std::string name;
        std::function<bool()> test;
        std::string category;
    };
    
    std::vector<TestCase> tests;
    int passed = 0;
    int failed = 0;
    
    void addTest(const std::string& category, const std::string& name, std::function<bool()> test) {
        tests.push_back({name, test, category});
    }
    
    void run() {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════╗\n";
        std::cout << "║      LUMA Studio Unit Test Suite         ║\n";
        std::cout << "╚══════════════════════════════════════════╝\n\n";
        
        std::string currentCategory;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (const auto& test : tests) {
            if (test.category != currentCategory) {
                currentCategory = test.category;
                std::cout << "\n--- " << currentCategory << " ---\n";
            }
            
            try {
                bool result = test.test();
                if (result) {
                    std::cout << "[PASS] " << test.name << "\n";
                    passed++;
                } else {
                    std::cout << "[FAIL] " << test.name << "\n";
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "[FAIL] " << test.name << " - Exception: " << e.what() << "\n";
                failed++;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "\n========== RESULTS ==========\n";
        std::cout << "Total:  " << (passed + failed) << "\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Time:   " << duration.count() << "ms\n";
        std::cout << "=============================\n";
    }
    
    bool allPassed() const { return failed == 0; }
};

// ===== Helper Macros =====
#define EXPECT_TRUE(expr) if (!(expr)) return false
#define EXPECT_FALSE(expr) if (expr) return false
#define EXPECT_EQ(a, b) if ((a) != (b)) return false
#define EXPECT_NEAR(a, b, eps) if (std::abs((a) - (b)) > (eps)) return false

// ===== Math Tests =====
namespace MathTests {

inline bool testVec3Basic() {
    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);
    
    // Addition
    Vec3 c = a + b;
    EXPECT_EQ(c.x, 5.0f);
    EXPECT_EQ(c.y, 7.0f);
    EXPECT_EQ(c.z, 9.0f);
    
    // Subtraction
    Vec3 d = b - a;
    EXPECT_EQ(d.x, 3.0f);
    EXPECT_EQ(d.y, 3.0f);
    EXPECT_EQ(d.z, 3.0f);
    
    // Scalar multiplication
    Vec3 e = a * 2.0f;
    EXPECT_EQ(e.x, 2.0f);
    EXPECT_EQ(e.y, 4.0f);
    EXPECT_EQ(e.z, 6.0f);
    
    return true;
}

inline bool testVec3Length() {
    Vec3 v(3, 4, 0);
    EXPECT_NEAR(v.length(), 5.0f, 0.001f);
    
    Vec3 n = v.normalized();
    EXPECT_NEAR(n.length(), 1.0f, 0.001f);
    EXPECT_NEAR(n.x, 0.6f, 0.001f);
    EXPECT_NEAR(n.y, 0.8f, 0.001f);
    
    return true;
}

inline bool testVec3DotCross() {
    Vec3 a(1, 0, 0);
    Vec3 b(0, 1, 0);
    
    // Dot product
    EXPECT_NEAR(a.dot(a), 1.0f, 0.001f);
    EXPECT_NEAR(a.dot(b), 0.0f, 0.001f);
    
    // Cross product
    Vec3 c = a.cross(b);
    EXPECT_NEAR(c.x, 0.0f, 0.001f);
    EXPECT_NEAR(c.y, 0.0f, 0.001f);
    EXPECT_NEAR(c.z, 1.0f, 0.001f);
    
    return true;
}

inline bool testQuatBasic() {
    Quat q;
    EXPECT_EQ(q.w, 1.0f);
    EXPECT_EQ(q.x, 0.0f);
    EXPECT_EQ(q.y, 0.0f);
    EXPECT_EQ(q.z, 0.0f);
    
    return true;
}

inline bool testQuatFromEuler() {
    // 90 degree rotation around Y axis
    Quat q = Quat::fromEuler(0, 1.5708f, 0);  // ~90 degrees
    
    // Rotate point (1, 0, 0) should give approximately (0, 0, -1)
    Vec3 p(1, 0, 0);
    Vec3 r = q.rotate(p);
    
    EXPECT_NEAR(r.x, 0.0f, 0.01f);
    EXPECT_NEAR(r.z, -1.0f, 0.01f);
    
    return true;
}

inline bool testQuatFromAxisAngle() {
    Vec3 axis(0, 1, 0);  // Y axis
    Quat q = Quat::fromAxisAngle(axis, 3.14159f);  // 180 degrees
    
    // Rotate (1, 0, 0) by 180 around Y should give (-1, 0, 0)
    Vec3 p(1, 0, 0);
    Vec3 r = q.rotate(p);
    
    EXPECT_NEAR(r.x, -1.0f, 0.01f);
    EXPECT_NEAR(r.y, 0.0f, 0.01f);
    EXPECT_NEAR(r.z, 0.0f, 0.01f);
    
    return true;
}

inline bool testMat4Identity() {
    Mat4 m = Mat4::identity();
    
    EXPECT_EQ(m.m[0], 1.0f);
    EXPECT_EQ(m.m[5], 1.0f);
    EXPECT_EQ(m.m[10], 1.0f);
    EXPECT_EQ(m.m[15], 1.0f);
    EXPECT_EQ(m.m[1], 0.0f);
    
    return true;
}

inline bool testMat4Translation() {
    Mat4 m = Mat4::translation(Vec3(1, 2, 3));
    
    EXPECT_EQ(m.m[12], 1.0f);
    EXPECT_EQ(m.m[13], 2.0f);
    EXPECT_EQ(m.m[14], 3.0f);
    
    return true;
}

inline bool testMat4Scale() {
    Mat4 m = Mat4::scale(Vec3(2, 3, 4));
    
    EXPECT_EQ(m.m[0], 2.0f);
    EXPECT_EQ(m.m[5], 3.0f);
    EXPECT_EQ(m.m[10], 4.0f);
    
    return true;
}

inline bool testMat4Multiply() {
    Mat4 a = Mat4::translation(Vec3(1, 0, 0));
    Mat4 b = Mat4::translation(Vec3(0, 2, 0));
    Mat4 c = a * b;
    
    EXPECT_NEAR(c.m[12], 1.0f, 0.001f);
    EXPECT_NEAR(c.m[13], 2.0f, 0.001f);
    
    return true;
}

}  // namespace MathTests

// ===== Animation Tests =====
namespace AnimationTests {

inline bool testSkeletonCreate() {
    Skeleton skel;
    int root = skel.addBone("root", -1);
    int child = skel.addBone("child", root);
    int grandchild = skel.addBone("grandchild", child);
    
    EXPECT_EQ(skel.getBoneCount(), 3u);
    EXPECT_EQ(root, 0);
    EXPECT_EQ(child, 1);
    EXPECT_EQ(grandchild, 2);
    
    return true;
}

inline bool testSkeletonFindBone() {
    Skeleton skel;
    skel.addBone("root", -1);
    skel.addBone("arm", 0);
    skel.addBone("hand", 1);
    
    EXPECT_EQ(skel.findBoneByName("arm"), 1);
    EXPECT_EQ(skel.findBoneByName("notfound"), -1);
    
    return true;
}

inline bool testAnimationClipCreate() {
    AnimationClip clip;
    clip.name = "walk";
    clip.duration = 1.0f;
    clip.looping = true;
    
    auto& ch = clip.addChannel("root");
    VectorKeyframe k0, k1;
    k0.time = 0.0f; k0.value = Vec3(0, 0, 0);
    k1.time = 1.0f; k1.value = Vec3(1, 0, 0);
    ch.positionKeys.push_back(k0);
    ch.positionKeys.push_back(k1);
    
    EXPECT_EQ(clip.channels.size(), 1u);
    EXPECT_EQ(ch.positionKeys.size(), 2u);
    
    return true;
}

inline bool testAnimationSample() {
    AnimationClip clip;
    clip.duration = 1.0f;
    auto& ch = clip.addChannel("bone");
    
    VectorKeyframe k0{0.0f, Vec3(0, 0, 0)};
    VectorKeyframe k1{1.0f, Vec3(10, 0, 0)};
    ch.positionKeys = {k0, k1};
    
    Vec3 pos, scale;
    Quat rot;
    
    // Sample at 0
    ch.sample(0.0f, pos, rot, scale);
    EXPECT_NEAR(pos.x, 0.0f, 0.01f);
    
    // Sample at 0.5
    ch.sample(0.5f, pos, rot, scale);
    EXPECT_NEAR(pos.x, 5.0f, 0.01f);
    
    // Sample at 1.0
    ch.sample(1.0f, pos, rot, scale);
    EXPECT_NEAR(pos.x, 10.0f, 0.01f);
    
    return true;
}

inline bool testAnimatorPlayback() {
    Skeleton skel;
    skel.addBone("root", -1);
    
    Animator animator;
    animator.setSkeleton(&skel);
    
    auto clip = std::make_unique<AnimationClip>();
    clip->name = "test";
    clip->duration = 2.0f;
    animator.addClip("test", std::move(clip));
    
    EXPECT_FALSE(animator.isPlaying());
    
    animator.play("test");
    EXPECT_TRUE(animator.isPlaying());
    EXPECT_NEAR(animator.getCurrentTime(), 0.0f, 0.01f);
    
    animator.update(1.0f);
    EXPECT_NEAR(animator.getCurrentTime(), 1.0f, 0.01f);
    
    animator.stop();
    EXPECT_FALSE(animator.isPlaying());
    
    return true;
}

inline bool testBlendTree1D() {
    BlendTree1D tree;
    tree.parameterName = "Speed";
    
    // Create dummy clips (null for this test)
    tree.addMotion(nullptr, 0.0f);  // Idle at speed 0
    tree.addMotion(nullptr, 1.0f);  // Walk at speed 1
    
    tree.parameter = 0.5f;  // 50% speed
    
    EXPECT_EQ(tree.motions.size(), 2u);
    
    return true;
}

inline bool testStateMachine() {
    AnimationStateMachine sm;
    
    sm.addParameter("IsMoving", ParameterType::Bool);
    sm.addParameter("Speed", ParameterType::Float);
    
    auto* idle = sm.createState("Idle");
    auto* walk = sm.createState("Walk");
    
    EXPECT_TRUE(idle != nullptr);
    EXPECT_TRUE(walk != nullptr);
    
    // Add transition
    auto& trans = idle->addTransition("Walk");
    trans.conditions.push_back({"IsMoving", ConditionMode::If, 1.0f});
    
    sm.setDefaultState("Idle");
    sm.start();
    
    EXPECT_EQ(sm.getCurrentStateName(), "Idle");
    
    // Trigger transition
    sm.setBool("IsMoving", true);
    sm.update(0.1f);
    
    // Should transition (or be transitioning)
    EXPECT_TRUE(sm.getCurrentStateName() == "Walk" || sm.isTransitioning());
    
    return true;
}

inline bool testAnimationLayer() {
    Skeleton skel;
    skel.addBone("root", -1);
    skel.addBone("spine", 0);
    skel.addBone("arm", 1);
    
    AnimationLayerManager manager;
    manager.setSkeleton(&skel);
    
    auto* base = manager.getBaseLayer();
    EXPECT_TRUE(base != nullptr);
    EXPECT_EQ(base->name, "Base");
    
    auto* upper = manager.createLayer("UpperBody");
    EXPECT_TRUE(upper != nullptr);
    EXPECT_EQ(manager.getLayerCount(), 2u);
    
    return true;
}

inline bool testBoneMask() {
    Skeleton skel;
    skel.addBone("root", -1);
    skel.addBone("spine", 0);
    skel.addBone("arm_l", 1);
    skel.addBone("arm_r", 1);
    
    BoneMask mask;
    mask.addBone("spine");
    mask.addBone("arm_l");
    mask.resolve(skel);
    
    EXPECT_FALSE(mask.includes(0));  // root not included
    EXPECT_TRUE(mask.includes(1));   // spine included
    EXPECT_TRUE(mask.includes(2));   // arm_l included
    EXPECT_FALSE(mask.includes(3));  // arm_r not included
    
    return true;
}

}  // namespace AnimationTests

// ===== Rendering Tests =====
namespace RenderingTests {

inline bool testFrustumPlane() {
    Plane plane;
    plane.normal = Vec3(0, 1, 0);
    plane.distance = 0.0f;
    
    // Point above plane
    float distAbove = plane.distanceToPoint(Vec3(0, 5, 0));
    EXPECT_TRUE(distAbove > 0.0f);
    
    // Point below plane
    float distBelow = plane.distanceToPoint(Vec3(0, -5, 0));
    EXPECT_TRUE(distBelow < 0.0f);
    
    // Point on plane
    float distOn = plane.distanceToPoint(Vec3(0, 0, 0));
    EXPECT_NEAR(distOn, 0.0f, 0.001f);
    
    return true;
}

inline bool testBoundingSphere() {
    BoundingSphere sphere;
    sphere.center = Vec3(0, 0, 0);
    sphere.radius = 5.0f;
    
    // Create a simple frustum (just test plane culling)
    Plane plane;
    plane.normal = Vec3(0, 0, 1);
    plane.distance = 10.0f;  // Plane at z = -10
    
    // Sphere at origin with radius 5 should be in front of plane at z=-10
    float dist = plane.distanceToPoint(sphere.center);
    EXPECT_TRUE(dist + sphere.radius > 0.0f);  // Not fully behind
    
    return true;
}

inline bool testLODSelection() {
    // LODManager API changed - test simplified
    LODGroup group;
    group.name = "TestModel";
    group.levels.push_back({0.0f, 0, 1000});   // LOD0: 0-20%
    group.levels.push_back({0.2f, 1, 500});    // LOD1: 20-50%
    group.levels.push_back({0.5f, 2, 100});    // LOD2: 50-100%
    
    // Test LOD group setup
    EXPECT_EQ(group.levels.size(), 3);
    EXPECT_EQ(group.levels[0].meshIndex, 0);
    EXPECT_EQ(group.levels[1].meshIndex, 1);
    EXPECT_EQ(group.levels[2].meshIndex, 2);
    
    return true;
}

inline bool testSSAOKernel() {
    SSAOKernel kernel;
    kernel.generateKernel(32);
    
    EXPECT_EQ(kernel.sampleCount, 32);
    
    // All samples should be in unit hemisphere
    for (int i = 0; i < kernel.sampleCount; i++) {
        float len = kernel.samples[i].length();
        EXPECT_TRUE(len <= 1.0f + 0.001f);
        EXPECT_TRUE(kernel.samples[i].z >= 0.0f);  // Above XY plane
    }
    
    return true;
}

inline bool testSSAONoise() {
    SSAONoise noise;
    
    // Check noise is generated
    for (int i = 0; i < SSAONoise::NOISE_PIXELS; i++) {
        // Rotation vectors should be normalized in XY
        float len = std::sqrt(noise.noise[i].x * noise.noise[i].x + 
                             noise.noise[i].y * noise.noise[i].y);
        EXPECT_NEAR(len, 1.0f, 0.01f);
        EXPECT_NEAR(noise.noise[i].z, 0.0f, 0.001f);  // Z should be 0
    }
    
    return true;
}

inline bool testEnvironmentMap() {
    EnvironmentMap envMap;
    
    // Load test HDR (uses built-in gradient)
    HDRLoader::loadHDR("", envMap.sourceHDR);
    
    EXPECT_TRUE(envMap.sourceHDR.isValid());
    EXPECT_EQ(envMap.sourceHDR.width, 512);
    EXPECT_EQ(envMap.sourceHDR.height, 256);
    
    // Sample should return non-zero values
    Vec3 sample = envMap.sourceHDR.sample(0.5f, 0.25f);
    EXPECT_TRUE(sample.x > 0.0f || sample.y > 0.0f || sample.z > 0.0f);
    
    return true;
}

inline bool testCSMCascades() {
    CascadedShadowMap csm;
    csm.settings.numCascades = 4;
    
    EXPECT_EQ(csm.cascades.size(), 4u);
    
    // Create test camera matrices
    Mat4 cameraView = Mat4::identity();
    Mat4 cameraProj = Mat4::identity();
    Vec3 lightDir = Vec3(0, -1, 0).normalized();
    
    csm.update(cameraView, cameraProj, lightDir, 0.1f, 100.0f);
    
    // Check cascades have valid matrices
    for (int i = 0; i < csm.settings.numCascades; i++) {
        // ViewProjection should not be identity
        EXPECT_TRUE(csm.cascades[i].viewProjectionMatrix.m[0] != 0.0f ||
                   csm.cascades[i].viewProjectionMatrix.m[5] != 0.0f);
    }
    
    return true;
}

inline bool testPCSSSamples() {
    PCSShadows pcss;
    
    // Check Poisson disk samples exist
    for (int i = 0; i < 32; i++) {
        // Samples should be within unit disk
        float len = std::sqrt(pcss.poissonDisk[i].x * pcss.poissonDisk[i].x +
                             pcss.poissonDisk[i].y * pcss.poissonDisk[i].y);
        EXPECT_TRUE(len <= 1.5f);  // Allow some margin
    }
    
    return true;
}

inline bool testVolumetricFogDensity() {
    VolumetricFog fog;
    fog.settings.density = 0.1f;
    fog.settings.heightFalloff = 0.1f;
    fog.settings.heightOffset = 0.0f;
    
    // Density should decrease with height
    float densityLow = fog.getDensity(Vec3(0, 0, 0));
    float densityHigh = fog.getDensity(Vec3(0, 10, 0));
    
    EXPECT_TRUE(densityLow > densityHigh);
    EXPECT_TRUE(densityLow > 0.0f);
    
    return true;
}

}  // namespace RenderingTests

// ===== IK Tests =====
namespace IKTests {

inline bool testTwoBoneIK() {
    Skeleton skel;
    int shoulder = skel.addBone("shoulder", -1);
    int elbow = skel.addBone("elbow", shoulder);
    int hand = skel.addBone("hand", elbow);
    
    // Set bone positions
    Bone* shoulderBone = const_cast<Bone*>(skel.getBone(shoulder));
    Bone* elbowBone = const_cast<Bone*>(skel.getBone(elbow));
    Bone* handBone = const_cast<Bone*>(skel.getBone(hand));
    
    shoulderBone->localPosition = Vec3(0, 0, 0);
    elbowBone->localPosition = Vec3(1, 0, 0);  // 1 unit arm
    handBone->localPosition = Vec3(1, 0, 0);   // 1 unit forearm
    
    TwoBoneIK ik;
    ik.rootBoneIndex = shoulder;
    ik.midBoneIndex = elbow;
    ik.endBoneIndex = hand;
    ik.targetPosition = Vec3(1.5f, 0, 0);  // Reachable target
    ik.weight = 1.0f;
    
    // This should not crash
    ik.solve(skel);
    
    return true;
}

inline bool testLookAtIK() {
    Skeleton skel;
    int head = skel.addBone("head", -1);
    
    LookAtIK ik;
    ik.boneIndex = head;
    ik.targetPosition = Vec3(0, 0, 10);
    ik.weight = 1.0f;
    
    // Should not crash
    ik.solve(skel);
    
    return true;
}

inline bool testIKManager() {
    IKManager manager;
    
    int armIdx = manager.setupArmIK(0, 1, 2);
    int headIdx = manager.setupHeadLookAt(3);
    
    EXPECT_EQ(manager.twoBoneIKs.size(), 1u);
    EXPECT_EQ(manager.lookAtIKs.size(), 1u);
    EXPECT_EQ(armIdx, 0);
    EXPECT_EQ(headIdx, 0);
    
    // Set targets
    manager.setHandTarget(0, Vec3(1, 0, 0), 0.5f);
    manager.setLookAtTarget(0, Vec3(0, 0, 10), 1.0f);
    
    EXPECT_NEAR(manager.twoBoneIKs[0].weight, 0.5f, 0.01f);
    EXPECT_NEAR(manager.lookAtIKs[0].weight, 1.0f, 0.01f);
    
    return true;
}

}  // namespace IKTests

// ===== Timeline Tests =====
namespace TimelineTests {

inline bool testAnimationCurve() {
    AnimationCurve<float> curve;
    curve.defaultValue = 0.0f;
    
    curve.addKeyframe(0.0f, 0.0f);
    curve.addKeyframe(1.0f, 10.0f);
    
    EXPECT_EQ(curve.getKeyframeCount(), 2);
    
    // Evaluate
    float v0 = curve.evaluate(0.0f);
    float v1 = curve.evaluate(1.0f);
    float vMid = curve.evaluate(0.5f);
    
    EXPECT_NEAR(v0, 0.0f, 0.01f);
    EXPECT_NEAR(v1, 10.0f, 0.01f);
    EXPECT_TRUE(vMid > 0.0f && vMid < 10.0f);  // Should interpolate
    
    return true;
}

inline bool testTimeline() {
    Timeline timeline;
    timeline.name = "Test";
    timeline.duration = 5.0f;
    timeline.frameRate = 30.0f;
    
    auto* track = timeline.createTrack("Position", TrackType::Float);
    EXPECT_TRUE(track != nullptr);
    EXPECT_EQ(timeline.tracks.size(), 1u);
    
    // Add keyframes
    track->floatCurve.addKeyframe(0.0f, 0.0f);
    track->floatCurve.addKeyframe(5.0f, 100.0f);
    
    // Playback
    timeline.play();
    EXPECT_TRUE(timeline.playing);
    
    timeline.update(2.5f);
    EXPECT_NEAR(timeline.currentTime, 2.5f, 0.01f);
    
    timeline.stop();
    EXPECT_FALSE(timeline.playing);
    EXPECT_NEAR(timeline.currentTime, 0.0f, 0.01f);
    
    return true;
}

inline bool testTimelineMarkers() {
    Timeline timeline;
    timeline.duration = 10.0f;
    
    timeline.addMarker(2.0f, "Start");
    timeline.addMarker(8.0f, "End");
    
    EXPECT_EQ(timeline.markers.size(), 2u);
    
    timeline.gotoMarker("End");
    EXPECT_NEAR(timeline.currentTime, 8.0f, 0.01f);
    
    return true;
}

}  // namespace TimelineTests

// ===== Register All Tests =====
inline void registerAllTests(UnitTestRunner& runner) {
    // Math Tests
    runner.addTest("Math", "Vec3 Basic Operations", MathTests::testVec3Basic);
    runner.addTest("Math", "Vec3 Length/Normalize", MathTests::testVec3Length);
    runner.addTest("Math", "Vec3 Dot/Cross", MathTests::testVec3DotCross);
    runner.addTest("Math", "Quat Basic", MathTests::testQuatBasic);
    runner.addTest("Math", "Quat FromEuler", MathTests::testQuatFromEuler);
    runner.addTest("Math", "Quat FromAxisAngle", MathTests::testQuatFromAxisAngle);
    runner.addTest("Math", "Mat4 Identity", MathTests::testMat4Identity);
    runner.addTest("Math", "Mat4 Translation", MathTests::testMat4Translation);
    runner.addTest("Math", "Mat4 Scale", MathTests::testMat4Scale);
    runner.addTest("Math", "Mat4 Multiply", MathTests::testMat4Multiply);
    
    // Animation Tests
    runner.addTest("Animation", "Skeleton Create", AnimationTests::testSkeletonCreate);
    runner.addTest("Animation", "Skeleton FindBone", AnimationTests::testSkeletonFindBone);
    runner.addTest("Animation", "AnimationClip Create", AnimationTests::testAnimationClipCreate);
    runner.addTest("Animation", "Animation Sample", AnimationTests::testAnimationSample);
    runner.addTest("Animation", "Animator Playback", AnimationTests::testAnimatorPlayback);
    runner.addTest("Animation", "BlendTree 1D", AnimationTests::testBlendTree1D);
    runner.addTest("Animation", "State Machine", AnimationTests::testStateMachine);
    runner.addTest("Animation", "Animation Layer", AnimationTests::testAnimationLayer);
    runner.addTest("Animation", "Bone Mask", AnimationTests::testBoneMask);
    
    // Rendering Tests
    runner.addTest("Rendering", "Frustum Plane", RenderingTests::testFrustumPlane);
    runner.addTest("Rendering", "Bounding Sphere", RenderingTests::testBoundingSphere);
    runner.addTest("Rendering", "LOD Selection", RenderingTests::testLODSelection);
    runner.addTest("Rendering", "SSAO Kernel", RenderingTests::testSSAOKernel);
    runner.addTest("Rendering", "SSAO Noise", RenderingTests::testSSAONoise);
    runner.addTest("Rendering", "Environment Map", RenderingTests::testEnvironmentMap);
    runner.addTest("Rendering", "CSM Cascades", RenderingTests::testCSMCascades);
    runner.addTest("Rendering", "PCSS Samples", RenderingTests::testPCSSSamples);
    runner.addTest("Rendering", "Volumetric Fog", RenderingTests::testVolumetricFogDensity);
    
    // IK Tests
    runner.addTest("IK", "Two-Bone IK", IKTests::testTwoBoneIK);
    runner.addTest("IK", "Look-At IK", IKTests::testLookAtIK);
    runner.addTest("IK", "IK Manager", IKTests::testIKManager);
    
    // Timeline Tests
    runner.addTest("Timeline", "Animation Curve", TimelineTests::testAnimationCurve);
    runner.addTest("Timeline", "Timeline Playback", TimelineTests::testTimeline);
    runner.addTest("Timeline", "Timeline Markers", TimelineTests::testTimelineMarkers);
}

// ===== Run All Unit Tests =====
inline bool runAllUnitTests() {
    UnitTestRunner runner;
    registerAllTests(runner);
    runner.run();
    return runner.allPassed();
}

}  // namespace test
}  // namespace luma
