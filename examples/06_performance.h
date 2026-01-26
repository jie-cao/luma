// Example 06: Performance Testing & Optimization
// Demonstrates culling, LOD, instancing, and stress testing
#pragma once

#include "engine/scene/scene_graph.h"
#include "engine/rendering/culling.h"
#include "engine/rendering/lod.h"
#include "engine/rendering/instancing.h"
#include "engine/rendering/render_optimizer.h"
#include <iostream>
#include <chrono>
#include <random>

namespace luma {
namespace examples {

// ============================================
// Example 1: Frustum Culling
// ============================================

inline void example_FrustumCulling() {
    CullingSystem& culling = CullingSystem::get();
    
    // Simulate a view-projection matrix
    Mat4 viewProj = Mat4::identity();  // Would be camera.viewProjection in real use
    
    // Begin culling frame
    culling.beginFrame(viewProj);
    
    // Test visibility of objects
    BoundingSphere objectA;
    objectA.center = Vec3(0, 0, 5);   // In front of camera
    objectA.radius = 1.0f;
    
    BoundingSphere objectB;
    objectB.center = Vec3(0, 0, -100);  // Behind camera
    objectB.radius = 1.0f;
    
    bool visibleA = culling.isVisible(objectA);
    bool visibleB = culling.isVisible(objectB);
    
    std::cout << "Frustum Culling Results:\n";
    std::cout << "  Object A (in front): " << (visibleA ? "Visible" : "Culled") << "\n";
    std::cout << "  Object B (behind): " << (visibleB ? "Visible" : "Culled") << "\n";
    
    // Get statistics
    CullStats stats = culling.getStats();
    std::cout << "\nCulling Stats:\n";
    std::cout << "  Total Objects: " << stats.totalObjects << "\n";
    std::cout << "  Visible: " << stats.visibleObjects << "\n";
    std::cout << "  Culled: " << stats.culledObjects << "\n";
}

// ============================================
// Example 2: Level of Detail (LOD)
// ============================================

inline void example_LOD() {
    LODManager lodManager;
    
    // Create LOD group for a tree
    LODGroup tree;
    tree.name = "Tree";
    
    // LOD 0: High detail (close)
    tree.levels.push_back({
        0.0f,      // minScreenSize
        0,         // meshIndex (high detail mesh)
        10000      // triangleCount
    });
    
    // LOD 1: Medium detail
    tree.levels.push_back({
        0.1f,      // Switch at 10% screen size
        1,         // meshIndex (medium detail mesh)
        2500       // triangleCount
    });
    
    // LOD 2: Low detail (far)
    tree.levels.push_back({
        0.05f,     // Switch at 5% screen size
        2,         // meshIndex (low detail mesh)
        500        // triangleCount
    });
    
    // LOD 3: Billboard (very far)
    tree.levels.push_back({
        0.02f,     // Switch at 2% screen size
        3,         // meshIndex (billboard)
        2          // triangleCount (just a quad)
    });
    
    // Simulate distance-based LOD selection
    std::cout << "LOD Selection for Tree:\n";
    
    float screenSizes[] = {0.5f, 0.15f, 0.08f, 0.03f, 0.01f};
    for (float screenSize : screenSizes) {
        int lod = lodManager.selectLOD(tree, screenSize);
        std::cout << "  Screen Size " << screenSize * 100 << "%: LOD " << lod;
        if (lod < static_cast<int>(tree.levels.size())) {
            std::cout << " (" << tree.levels[lod].triangleCount << " tris)";
        }
        std::cout << "\n";
    }
    
    // Calculate triangle savings
    int highDetailTris = 10000;
    int avgTrisWithLOD = (10000 + 2500 + 500 + 2) / 4;
    float savings = (1.0f - static_cast<float>(avgTrisWithLOD) / highDetailTris) * 100;
    std::cout << "\nAverage triangle reduction: " << savings << "%\n";
}

// ============================================
// Example 3: GPU Instancing
// ============================================

inline void example_Instancing() {
    InstancingManager instancing;
    
    // Create instance data for 1000 grass blades
    const int grassCount = 1000;
    std::vector<InstanceData> grassInstances;
    grassInstances.reserve(grassCount);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
    std::uniform_real_distribution<float> rotDist(0.0f, 6.28f);
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
    
    for (int i = 0; i < grassCount; i++) {
        InstanceData inst;
        
        // Random position
        Vec3 pos(posDist(rng), 0, posDist(rng));
        
        // Random Y rotation
        Quat rot = Quat::fromEuler(0, rotDist(rng), 0);
        
        // Random scale variation
        float scale = scaleDist(rng);
        
        // Build world matrix
        inst.worldMatrix = Mat4::translation(pos) * Mat4::fromQuat(rot) * Mat4::scale(Vec3(scale, scale, scale));
        inst.normalMatrix = inst.worldMatrix;  // Simplified
        inst.color = Vec3(0.3f, 0.6f + scaleDist(rng) * 0.2f, 0.2f);  // Green with variation
        inst.materialId = 0;
        inst.lodBlend = 1.0f;
        
        grassInstances.push_back(inst);
    }
    
    std::cout << "Instancing Example:\n";
    std::cout << "  Created " << grassCount << " grass instances\n";
    std::cout << "  Draw calls without instancing: " << grassCount << "\n";
    std::cout << "  Draw calls with instancing: 1\n";
    std::cout << "  Reduction: " << (100.0f - 100.0f / grassCount) << "%\n";
    
    // In real rendering:
    // 1. Upload instance data to GPU buffer
    // 2. Bind instance buffer
    // 3. Draw instanced (single draw call for all grass)
}

// ============================================
// Example 4: Render Optimizer
// ============================================

inline void example_RenderOptimizer() {
    RenderOptimizer& optimizer = getRenderOptimizer();
    
    // Configure optimizer
    optimizer.config.enableFrustumCulling = true;
    optimizer.config.enableOcclusionCulling = false;  // Requires GPU queries
    optimizer.config.enableLOD = true;
    optimizer.config.enableInstancing = true;
    optimizer.config.sortMode = RenderSortMode::FrontToBack;  // For opaque
    
    std::cout << "Render Optimizer Configuration:\n";
    std::cout << "  Frustum Culling: " << (optimizer.config.enableFrustumCulling ? "ON" : "OFF") << "\n";
    std::cout << "  LOD: " << (optimizer.config.enableLOD ? "ON" : "OFF") << "\n";
    std::cout << "  Instancing: " << (optimizer.config.enableInstancing ? "ON" : "OFF") << "\n";
    std::cout << "  Sort Mode: Front-to-Back\n";
    
    // Simulate frame
    Mat4 viewProj = Mat4::identity();
    Vec3 cameraPos(0, 0, 0);
    
    optimizer.beginFrame(viewProj, cameraPos);
    
    // Process entities...
    // optimizer.processEntity(entity, meshId, bounds);
    
    optimizer.endFrame();
    
    // Get frame stats
    auto stats = optimizer.getFrameStats();
    std::cout << "\nFrame Statistics:\n";
    std::cout << "  Total Objects: " << stats.totalObjects << "\n";
    std::cout << "  Visible Objects: " << stats.visibleObjects << "\n";
    std::cout << "  Draw Calls: " << stats.drawCalls << "\n";
    std::cout << "  Triangles: " << stats.trianglesRendered << "\n";
}

// ============================================
// Example 5: Stress Test Scene
// ============================================

inline void example_StressTestScene(SceneGraph& scene) {
    std::cout << "Creating Stress Test Scene...\n";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create a grid of objects
    const int gridSize = 50;  // 50x50 = 2500 objects
    const float spacing = 3.0f;
    
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> rotDist(0, 6.28f);
    std::uniform_real_distribution<float> scaleDist(0.5f, 1.5f);
    
    int objectCount = 0;
    for (int x = 0; x < gridSize; x++) {
        for (int z = 0; z < gridSize; z++) {
            std::string name = "Object_" + std::to_string(objectCount++);
            Entity* e = scene.createEntity(name);
            
            // Position in grid
            float px = (x - gridSize / 2.0f) * spacing;
            float pz = (z - gridSize / 2.0f) * spacing;
            e->localTransform.position = Vec3(px, 0, pz);
            
            // Random rotation
            e->localTransform.rotation = Quat::fromEuler(0, rotDist(rng), 0);
            
            // Random scale
            float s = scaleDist(rng);
            e->localTransform.scale = Vec3(s, s, s);
            
            // Assign material (cycle through presets)
            e->material = std::make_shared<Material>();
            int matType = (x + z) % 5;
            switch (matType) {
                case 0: *e->material = Material::createGold(); break;
                case 1: *e->material = Material::createSilver(); break;
                case 2: *e->material = Material::createPlastic(Vec3(0.8f, 0.2f, 0.2f)); break;
                case 3: *e->material = Material::createPlastic(Vec3(0.2f, 0.8f, 0.2f)); break;
                case 4: *e->material = Material::createPlastic(Vec3(0.2f, 0.2f, 0.8f)); break;
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Stress Test Scene Created:\n";
    std::cout << "  Objects: " << scene.getEntityCount() << "\n";
    std::cout << "  Grid Size: " << gridSize << "x" << gridSize << "\n";
    std::cout << "  Creation Time: " << duration.count() << "ms\n";
    
    // Memory estimate (rough)
    size_t entitySize = sizeof(Entity) + 64;  // Plus overhead
    size_t materialSize = sizeof(Material);
    size_t totalMem = scene.getEntityCount() * (entitySize + materialSize);
    std::cout << "  Estimated Memory: " << totalMem / 1024 << " KB\n";
}

// ============================================
// Example 6: Performance Benchmarking
// ============================================

inline void example_Benchmark() {
    std::cout << "=== Performance Benchmark ===\n\n";
    
    // Matrix multiplication benchmark
    {
        const int iterations = 100000;
        Mat4 a = Mat4::translation(Vec3(1, 2, 3));
        Mat4 b = Mat4::scale(Vec3(2, 2, 2));
        Mat4 result;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            result = a * b;
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        std::cout << "Mat4 Multiply: " << ns / iterations << " ns/op\n";
    }
    
    // Quaternion slerp benchmark
    {
        const int iterations = 100000;
        Quat a = Quat::fromEuler(0, 0, 0);
        Quat b = Quat::fromEuler(1, 1, 1);
        Quat result;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            result = anim::slerp(a, b, 0.5f);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        std::cout << "Quat Slerp: " << ns / iterations << " ns/op\n";
    }
    
    // Vector operations benchmark
    {
        const int iterations = 1000000;
        Vec3 a(1, 2, 3);
        Vec3 b(4, 5, 6);
        Vec3 result;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            result = a.cross(b).normalized();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        std::cout << "Vec3 Cross+Normalize: " << ns / iterations << " ns/op\n";
    }
    
    // Entity creation benchmark
    {
        SceneGraph scene;
        const int count = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < count; i++) {
            scene.createEntity("Entity" + std::to_string(i));
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        std::cout << "Entity Creation: " << us / count << " us/entity (total: " << us / 1000 << "ms)\n";
    }
    
    std::cout << "\nBenchmark complete.\n";
}

// ============================================
// Example 7: Optimization Tips
// ============================================

inline void example_OptimizationTips() {
    std::cout << "=== Performance Optimization Tips ===\n\n";
    
    std::cout << "1. CULLING\n";
    std::cout << "   - Always use frustum culling\n";
    std::cout << "   - Consider occlusion culling for complex scenes\n";
    std::cout << "   - Use bounding spheres for fast rejection\n\n";
    
    std::cout << "2. LEVEL OF DETAIL\n";
    std::cout << "   - Create 3-4 LOD levels per asset\n";
    std::cout << "   - Use billboards for distant objects\n";
    std::cout << "   - Blend between LODs to avoid popping\n\n";
    
    std::cout << "3. BATCHING & INSTANCING\n";
    std::cout << "   - Group similar materials together\n";
    std::cout << "   - Use GPU instancing for repeated objects\n";
    std::cout << "   - Minimize state changes between draws\n\n";
    
    std::cout << "4. SHADOWS\n";
    std::cout << "   - Use cascaded shadow maps\n";
    std::cout << "   - Limit shadow distance\n";
    std::cout << "   - Consider lower resolution for far cascades\n\n";
    
    std::cout << "5. POST-PROCESSING\n";
    std::cout << "   - Render SSAO at half resolution\n";
    std::cout << "   - Limit bloom iterations\n";
    std::cout << "   - Consider temporal reprojection\n\n";
    
    std::cout << "6. MEMORY\n";
    std::cout << "   - Stream textures by distance\n";
    std::cout << "   - Compress textures (BC7/ASTC)\n";
    std::cout << "   - Pool frequently allocated objects\n\n";
    
    std::cout << "7. ANIMATION\n";
    std::cout << "   - Update animations at lower frequency for distant characters\n";
    std::cout << "   - Use animation compression\n";
    std::cout << "   - Limit bone count for LOD levels\n";
}

}  // namespace examples
}  // namespace luma
