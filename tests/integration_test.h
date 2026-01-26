// LUMA Studio - Integration Test Suite
// Tests all major features for Phase 10.7 verification
#pragma once

#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/foundation/math_types.h"
#include "engine/animation/animation.h"
#include "engine/serialization/scene_serializer.h"
#include "engine/serialization/json.h"
#include "engine/renderer/post_process.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>

namespace luma {
namespace test {

// ===== Test Result Tracking =====
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

inline std::vector<TestResult> g_testResults;

inline void recordTest(const std::string& name, bool passed, const std::string& msg = "") {
    g_testResults.push_back({name, passed, msg});
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name;
    if (!msg.empty()) std::cout << " - " << msg;
    std::cout << std::endl;
}

inline void printTestSummary() {
    int passed = 0, failed = 0;
    std::cout << "\n========== TEST SUMMARY ==========\n";
    for (const auto& r : g_testResults) {
        if (r.passed) passed++; else failed++;
    }
    std::cout << "Total: " << g_testResults.size() << " | ";
    std::cout << "Passed: " << passed << " | ";
    std::cout << "Failed: " << failed << std::endl;
    
    if (failed > 0) {
        std::cout << "\nFailed tests:\n";
        for (const auto& r : g_testResults) {
            if (!r.passed) {
                std::cout << "  - " << r.name << ": " << r.message << std::endl;
            }
        }
    }
    std::cout << "==================================\n";
}

// ===== 1. Scene Graph Tests =====
inline void testSceneGraph() {
    std::cout << "\n--- Scene Graph Tests ---\n";
    
    SceneGraph scene;
    
    // Test 1: Create entity
    Entity* e1 = scene.createEntity("TestEntity1");
    recordTest("SceneGraph: Create entity", e1 != nullptr && e1->name == "TestEntity1");
    
    // Test 2: Create multiple entities
    Entity* e2 = scene.createEntity("TestEntity2");
    Entity* e3 = scene.createEntity("Child1");
    recordTest("SceneGraph: Multiple entities", scene.getEntityCount() == 3);
    
    // Test 3: Parent-child relationship
    scene.setParent(e3, e1);
    recordTest("SceneGraph: Set parent", e3->parent == e1);
    recordTest("SceneGraph: Children updated", 
               std::find(e1->children.begin(), e1->children.end(), e3) != e1->children.end());
    
    // Test 4: Find by name
    Entity* found = scene.findEntityByName("TestEntity2");
    recordTest("SceneGraph: Find by name", found == e2);
    
    // Test 5: Get root entities
    auto roots = scene.getRootEntities();
    recordTest("SceneGraph: Root entities", roots.size() == 2);  // e1 and e2 are roots
    
    // Test 6: Rename entity
    e1->name = "RenamedEntity";
    recordTest("SceneGraph: Rename", e1->name == "RenamedEntity");
    
    // Test 7: Delete entity
    scene.destroyEntity(e2->id);
    recordTest("SceneGraph: Delete entity", scene.getEntityCount() == 2);
    
    // Test 8: Delete parent (children should become orphans or deleted)
    EntityID e3Id = e3->id;
    scene.destroyEntity(e1->id);
    // After deleting parent, child should also be removed or become root
    recordTest("SceneGraph: Delete parent cleans children", scene.getEntityCount() <= 1);
}

// ===== 2. Transform Tests =====
inline void testTransform() {
    std::cout << "\n--- Transform Tests ---\n";
    
    // Test 1: Default transform
    Transform t;
    recordTest("Transform: Default position", 
               t.position.x == 0 && t.position.y == 0 && t.position.z == 0);
    recordTest("Transform: Default scale",
               t.scale.x == 1 && t.scale.y == 1 && t.scale.z == 1);
    
    // Test 2: Set position
    t.position = Vec3(1, 2, 3);
    Mat4 m = t.toMatrix();
    recordTest("Transform: Position in matrix",
               std::abs(m.m[12] - 1.0f) < 0.001f &&
               std::abs(m.m[13] - 2.0f) < 0.001f &&
               std::abs(m.m[14] - 3.0f) < 0.001f);
    
    // Test 3: Set scale
    t.scale = Vec3(2, 2, 2);
    m = t.toMatrix();
    // Scale affects diagonal elements
    recordTest("Transform: Scale affects matrix", std::abs(m.m[0]) > 1.0f);
    
    // Test 4: Euler angles conversion
    t.setEulerDegrees(Vec3(0, 90, 0));
    Vec3 euler = t.getEulerDegrees();
    recordTest("Transform: Euler conversion", std::abs(euler.y - 90.0f) < 1.0f);
    
    // Test 5: Matrix multiplication
    Mat4 a = Mat4::translation(Vec3(1, 0, 0));
    Mat4 b = Mat4::translation(Vec3(0, 1, 0));
    Mat4 c = a * b;
    recordTest("Transform: Matrix multiply",
               std::abs(c.m[12] - 1.0f) < 0.001f &&
               std::abs(c.m[13] - 1.0f) < 0.001f);
}

// ===== 3. Animation Tests =====
inline void testAnimation() {
    std::cout << "\n--- Animation Tests ---\n";
    
    // Test 1: Create skeleton
    Skeleton skel;
    int rootIdx = skel.addBone("root", -1);
    int childIdx = skel.addBone("child", rootIdx);
    recordTest("Animation: Create skeleton", skel.getBoneCount() == 2);
    
    // Test 2: Find bone by name
    int foundIdx = skel.findBoneByName("child");
    recordTest("Animation: Find bone", foundIdx == childIdx);
    
    // Test 3: Create animation clip
    AnimationClip clip;
    clip.name = "test_anim";
    clip.duration = 1.0f;
    auto& ch = clip.addChannel("root");
    
    VectorKeyframe k0, k1;
    k0.time = 0.0f; k0.value = Vec3(0, 0, 0);
    k1.time = 1.0f; k1.value = Vec3(1, 0, 0);
    ch.positionKeys.push_back(k0);
    ch.positionKeys.push_back(k1);
    
    recordTest("Animation: Create clip", clip.channels.size() == 1);
    
    // Test 4: Sample animation
    Vec3 pos, scale;
    Quat rot;
    ch.sample(0.5f, pos, rot, scale);
    recordTest("Animation: Sample interpolation", std::abs(pos.x - 0.5f) < 0.01f);
    
    // Test 5: Animator playback
    Animator animator;
    animator.setSkeleton(&skel);
    auto clipPtr = std::make_unique<AnimationClip>(clip);
    animator.addClip("test", std::move(clipPtr));
    animator.play("test");
    recordTest("Animation: Animator play", animator.isPlaying());
    
    // Test 6: Animator update
    animator.update(0.5f);
    recordTest("Animation: Animator update", std::abs(animator.getCurrentTime() - 0.5f) < 0.01f);
    
    // Test 7: Animator stop
    animator.stop();
    recordTest("Animation: Animator stop", !animator.isPlaying());
}

// ===== 4. Serialization Tests =====
inline void testSerialization() {
    std::cout << "\n--- Serialization Tests ---\n";
    
    // Test 1: JSON parsing
    std::string jsonStr = R"({"name": "test", "value": 42, "nested": {"x": 1.5}})";
    JsonValue json = parseJson(jsonStr);
    recordTest("JSON: Parse object", json.isObject());
    recordTest("JSON: Get string", json["name"].asString() == "test");
    recordTest("JSON: Get number", std::abs(json["value"].asNumber() - 42.0) < 0.001);
    recordTest("JSON: Get nested", std::abs(json["nested"]["x"].asNumber() - 1.5) < 0.001);
    
    // Test 2: JSON writing
    JsonValue out = JsonValue::object();
    out["test"] = "hello";
    out["num"] = 123.0;
    std::string written = toJson(out);
    recordTest("JSON: Write contains key", written.find("test") != std::string::npos);
    
    // Test 3: Scene serialization
    SceneGraph scene;
    Entity* e = scene.createEntity("SerializeTest");
    e->localTransform.position = Vec3(1, 2, 3);
    
    RHICameraParams camera;
    camera.yaw = 0.5f;
    camera.distance = 5.0f;
    
    PostProcessSettings pp;
    pp.bloom.enabled = true;
    pp.bloom.intensity = 0.8f;
    
    // Serialize to JSON
    JsonValue sceneJson = SceneSerializer::serializeScene(scene, "TestScene", &camera, &pp);
    recordTest("Serialize: Scene to JSON", sceneJson.isObject());
    recordTest("Serialize: Has entities", sceneJson["entities"].isArray());
    
    // Test 4: Camera serialization
    JsonValue camJson = SceneSerializer::serializeCameraParams(camera);
    recordTest("Serialize: Camera yaw", std::abs(camJson["yaw"].asNumber() - 0.5) < 0.001);
    
    // Test 5: Post-process serialization  
    JsonValue ppJson = SceneSerializer::serializePostProcess(pp);
    recordTest("Serialize: PostProcess bloom", ppJson["bloomEnabled"].asBool() == true);
}

// ===== 5. Math Types Tests =====
inline void testMathTypes() {
    std::cout << "\n--- Math Types Tests ---\n";
    
    // Test 1: Vec3 operations
    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);
    Vec3 c = a + b;
    recordTest("Math: Vec3 add", c.x == 5 && c.y == 7 && c.z == 9);
    
    // Test 2: Vec3 length
    Vec3 v(3, 4, 0);
    recordTest("Math: Vec3 length", std::abs(v.length() - 5.0f) < 0.001f);
    
    // Test 3: Vec3 normalize
    Vec3 n = v.normalized();
    recordTest("Math: Vec3 normalize", std::abs(n.length() - 1.0f) < 0.001f);
    
    // Test 4: Quat identity
    Quat q;
    recordTest("Math: Quat identity", q.w == 1.0f && q.x == 0.0f);
    
    // Test 5: Quat multiplication
    Quat q1 = Quat::fromEuler(0, 0.5f, 0);
    Quat q2 = Quat::fromEuler(0, 0.5f, 0);
    Quat q3 = q1 * q2;
    recordTest("Math: Quat multiply", q3.w != 1.0f);  // Should be rotated
    
    // Test 6: Mat4 identity
    Mat4 m = Mat4::identity();
    recordTest("Math: Mat4 identity", m.m[0] == 1.0f && m.m[5] == 1.0f && m.m[10] == 1.0f);
}

// ===== 6. Post-Process Settings Tests =====
inline void testPostProcess() {
    std::cout << "\n--- Post-Process Tests ---\n";
    
    PostProcessSettings pp;
    
    // Test defaults
    recordTest("PostProcess: Default exposure", std::abs(pp.toneMapping.exposure - 1.0f) < 0.001f);
    recordTest("PostProcess: Default bloom enabled", pp.bloom.enabled == true);  // Default is true
    
    // Test setting values
    pp.bloom.enabled = true;
    pp.bloom.intensity = 0.5f;
    pp.bloom.threshold = 1.2f;
    
    recordTest("PostProcess: Set bloom", pp.bloom.enabled == true);
    recordTest("PostProcess: Set intensity", std::abs(pp.bloom.intensity - 0.5f) < 0.001f);
}

// ===== Run All Tests =====
inline bool runAllIntegrationTests() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║   LUMA Studio Integration Test Suite     ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    
    g_testResults.clear();
    
    testMathTypes();
    testTransform();
    testSceneGraph();
    testAnimation();
    testSerialization();
    testPostProcess();
    
    printTestSummary();
    
    // Return true if all tests passed
    for (const auto& r : g_testResults) {
        if (!r.passed) return false;
    }
    return true;
}

// ===== Manual Test Checklist =====
inline void printManualTestChecklist() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║   Manual Test Checklist (Visual Verification)        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << R"(
[ ] 1. SCENE GRAPH
    - Create new entity (Edit > Create Entity)
    - Rename entity in Hierarchy panel
    - Drag entity to create parent-child relationship
    - Delete entity (Delete key or context menu)

[ ] 2. TRANSFORM & GIZMO
    - Select entity, verify Inspector shows transform
    - Edit position/rotation/scale values directly
    - Press W/E/R to switch gizmo modes
    - Drag gizmo handles to transform object
    - Verify world matrix updates correctly

[ ] 3. CAMERA
    - Alt + Left Mouse: Orbit rotation
    - Alt + Middle Mouse: Pan
    - Alt + Right Mouse / Scroll: Zoom
    - Press F to focus on selected object
    - Press G to toggle grid

[ ] 4. SHADOWS
    - Enable shadows in Lighting panel
    - Adjust shadow bias and softness
    - Verify shadow appears under objects
    - Check PCF soft shadow quality

[ ] 5. IBL / ENVIRONMENT
    - Load HDR environment map
    - Verify reflections on metallic surfaces
    - Adjust environment intensity
    - Check diffuse irradiance

[ ] 6. POST-PROCESSING
    - Enable Bloom in Post-Process panel
    - Adjust bloom threshold and intensity
    - Verify bright areas have bloom glow
    - Test Tone Mapping (exposure adjustment)
    - Check FXAA anti-aliasing

[ ] 7. ANIMATION
    - Load animated model (FBX/glTF with animation)
    - Open Animation Timeline panel
    - Select animation clip from dropdown
    - Press Play/Pause button
    - Drag timeline scrubber
    - Toggle loop mode
    - Adjust playback speed

[ ] 8. ASSET BROWSER
    - Navigate folders in Asset Browser
    - Double-click model to load
    - Drag model file to viewport
    - Check Cache tab for statistics

[ ] 9. SERIALIZATION
    - File > Save Scene
    - Close and reopen app
    - File > Load Scene
    - Verify entities, camera, post-process restored

[ ] 10. SHADER HOT-RELOAD
    - Edit pbr.hlsl or pbr.metal
    - Save file
    - Verify shader updates automatically
    - Check Shader Status panel for errors

)" << std::endl;
}

}  // namespace test
}  // namespace luma
