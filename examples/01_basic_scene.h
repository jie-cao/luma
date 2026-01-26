// Example 01: Basic Scene Setup
// Demonstrates how to create entities and set up a simple scene
#pragma once

#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/foundation/math_types.h"
#include "engine/material/material.h"
#include "engine/lighting/light.h"

namespace luma {
namespace examples {

// ============================================
// Example 1: Creating a Basic Scene
// ============================================
// This example shows how to:
// - Create a scene graph
// - Add entities with transforms
// - Set up parent-child hierarchies
// - Apply materials

inline void example_BasicScene() {
    // 1. Create a scene graph
    SceneGraph scene;
    
    // 2. Create ground plane
    Entity* ground = scene.createEntity("Ground");
    ground->localTransform.position = Vec3(0, 0, 0);
    ground->localTransform.scale = Vec3(10, 0.1f, 10);
    
    // Apply a gray material
    ground->material = std::make_shared<Material>();
    ground->material->baseColor = Vec3(0.3f, 0.3f, 0.3f);
    ground->material->roughness = 0.8f;
    ground->material->metallic = 0.0f;
    
    // 3. Create a cube
    Entity* cube = scene.createEntity("Cube");
    cube->localTransform.position = Vec3(0, 1, 0);
    cube->localTransform.scale = Vec3(1, 1, 1);
    
    // Apply a red plastic material
    cube->material = std::make_shared<Material>(Material::createPlastic(Vec3(0.8f, 0.2f, 0.2f)));
    
    // 4. Create a sphere
    Entity* sphere = scene.createEntity("Sphere");
    sphere->localTransform.position = Vec3(2, 1, 0);
    
    // Apply a gold material
    sphere->material = std::make_shared<Material>(Material::createGold());
    
    // 5. Create a parent-child hierarchy
    Entity* parent = scene.createEntity("Parent");
    parent->localTransform.position = Vec3(-2, 1, 0);
    
    Entity* child1 = scene.createEntity("Child1");
    child1->localTransform.position = Vec3(0, 1, 0);  // Relative to parent
    
    Entity* child2 = scene.createEntity("Child2");
    child2->localTransform.position = Vec3(1, 0, 0);  // Relative to parent
    
    // Set up hierarchy
    scene.setParent(child1, parent);
    scene.setParent(child2, parent);
    
    // 6. Rotate parent - children will follow
    parent->localTransform.rotation = Quat::fromEuler(0, 0.5f, 0);
    
    // 7. Add a light
    Entity* lightEntity = scene.createEntity("MainLight");
    lightEntity->localTransform.position = Vec3(5, 10, 5);
    lightEntity->hasLight = true;
    lightEntity->light = Light::createPoint();
    lightEntity->light.intensity = 500.0f;
    lightEntity->light.color = Vec3(1, 1, 1);
    
    // 8. Query scene
    std::cout << "Scene created with " << scene.getEntityCount() << " entities\n";
    std::cout << "Root entities: " << scene.getRootEntities().size() << "\n";
    
    // 9. Find entity by name
    Entity* found = scene.findEntityByName("Cube");
    if (found) {
        std::cout << "Found cube at position: " 
                  << found->localTransform.position.x << ", "
                  << found->localTransform.position.y << ", "
                  << found->localTransform.position.z << "\n";
    }
}

// ============================================
// Example 2: Working with Transforms
// ============================================

inline void example_Transforms() {
    // Create transform
    Transform t;
    
    // Set position
    t.position = Vec3(1, 2, 3);
    
    // Set rotation using Euler angles (in degrees)
    t.setEulerDegrees(Vec3(0, 45, 0));  // 45 degrees around Y
    
    // Set scale
    t.scale = Vec3(2, 2, 2);
    
    // Get the 4x4 matrix
    Mat4 worldMatrix = t.toMatrix();
    
    // Combine transforms
    Transform parent;
    parent.position = Vec3(10, 0, 0);
    
    Transform child;
    child.position = Vec3(1, 0, 0);  // Local position
    
    // Child world matrix = parent * child
    Mat4 childWorld = parent.toMatrix() * child.toMatrix();
    
    // Extract final position (column 3)
    Vec3 finalPos(childWorld.m[12], childWorld.m[13], childWorld.m[14]);
    std::cout << "Child world position: " << finalPos.x << ", " << finalPos.y << ", " << finalPos.z << "\n";
}

// ============================================
// Example 3: Selection and Manipulation
// ============================================

inline void example_Selection() {
    SceneGraph scene;
    
    // Create some entities
    Entity* e1 = scene.createEntity("Entity1");
    Entity* e2 = scene.createEntity("Entity2");
    Entity* e3 = scene.createEntity("Entity3");
    
    // Single selection
    scene.selectEntity(e1);
    std::cout << "Selected: " << scene.getSelectedEntities().size() << " entities\n";
    
    // Multi-selection
    scene.addToSelection(e2);
    scene.addToSelection(e3);
    std::cout << "Selected: " << scene.getSelectedEntities().size() << " entities\n";
    
    // Clear selection
    scene.clearSelection();
    std::cout << "Selected: " << scene.getSelectedEntities().size() << " entities\n";
    
    // Copy and paste
    scene.selectEntity(e1);
    scene.copySelection();
    scene.pasteClipboard();  // Creates a duplicate
    
    std::cout << "After paste: " << scene.getEntityCount() << " entities\n";
    
    // Duplicate
    Entity* duplicate = scene.duplicateEntity(e2);
    std::cout << "Duplicated: " << duplicate->name << "\n";
}

}  // namespace examples
}  // namespace luma
