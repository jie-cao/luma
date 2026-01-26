// Example 03: PBR Materials
// Demonstrates material creation and the metallic-roughness workflow
#pragma once

#include "engine/material/material.h"
#include "engine/scene/scene_graph.h"
#include <iostream>

namespace luma {
namespace examples {

// ============================================
// Example 1: Material Basics
// ============================================

inline void example_MaterialBasics() {
    // Create a default material
    Material mat;
    
    // PBR properties
    mat.name = "MyMaterial";
    mat.baseColor = Vec3(0.8f, 0.2f, 0.1f);  // Red
    mat.metallic = 0.0f;                      // Non-metallic (dielectric)
    mat.roughness = 0.5f;                     // Medium roughness
    mat.ao = 1.0f;                            // No ambient occlusion
    
    // Emissive (for glowing materials)
    mat.emissiveColor = Vec3(0, 0, 0);        // No emission
    mat.emissiveIntensity = 0.0f;
    
    std::cout << "Material: " << mat.name << "\n";
    std::cout << "  Base Color: (" << mat.baseColor.x << ", " << mat.baseColor.y << ", " << mat.baseColor.z << ")\n";
    std::cout << "  Metallic: " << mat.metallic << "\n";
    std::cout << "  Roughness: " << mat.roughness << "\n";
}

// ============================================
// Example 2: Material Presets
// ============================================

inline void example_MaterialPresets() {
    // Use built-in presets
    Material gold = Material::createGold();
    Material silver = Material::createSilver();
    Material copper = Material::createCopper();
    Material redPlastic = Material::createPlastic(Vec3(0.8f, 0.1f, 0.1f));
    Material blueRubber = Material::createRubber(Vec3(0.1f, 0.3f, 0.8f));
    Material glass = Material::createGlass();
    
    // Print properties
    auto printMaterial = [](const Material& m) {
        std::cout << m.name << ": metallic=" << m.metallic 
                  << ", roughness=" << m.roughness << "\n";
    };
    
    printMaterial(gold);
    printMaterial(silver);
    printMaterial(copper);
    printMaterial(redPlastic);
    printMaterial(blueRubber);
    printMaterial(glass);
}

// ============================================
// Example 3: Material Library
// ============================================

inline void example_MaterialLibrary() {
    // Get the material library singleton
    MaterialLibrary& library = MaterialLibrary::get();
    
    // Add custom presets
    Material customMetal;
    customMetal.name = "BrushedSteel";
    customMetal.baseColor = Vec3(0.6f, 0.6f, 0.65f);
    customMetal.metallic = 1.0f;
    customMetal.roughness = 0.3f;
    library.addPreset("BrushedSteel", customMetal);
    
    Material customWood;
    customWood.name = "DarkWood";
    customWood.baseColor = Vec3(0.3f, 0.15f, 0.05f);
    customWood.metallic = 0.0f;
    customWood.roughness = 0.7f;
    library.addPreset("DarkWood", customWood);
    
    // Get preset
    Material* steel = library.getPreset("BrushedSteel");
    if (steel) {
        std::cout << "Got preset: " << steel->name << "\n";
    }
    
    // List all presets
    std::cout << "Available presets:\n";
    for (const auto& name : library.getPresetNames()) {
        std::cout << "  - " << name << "\n";
    }
}

// ============================================
// Example 4: Material Showcase Grid
// ============================================
// Creates a grid of spheres showing different material properties

inline void example_MaterialShowcase(SceneGraph& scene) {
    const int gridSize = 7;
    const float spacing = 2.5f;
    
    // Create grid of spheres
    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            // Calculate metallic and roughness
            float metallic = static_cast<float>(col) / (gridSize - 1);
            float roughness = static_cast<float>(row) / (gridSize - 1);
            
            // Create entity
            std::string name = "Sphere_M" + std::to_string(col) + "_R" + std::to_string(row);
            Entity* sphere = scene.createEntity(name);
            
            // Position in grid
            float x = (col - gridSize / 2.0f) * spacing;
            float y = 1.0f;
            float z = (row - gridSize / 2.0f) * spacing;
            sphere->localTransform.position = Vec3(x, y, z);
            
            // Create material
            sphere->material = std::make_shared<Material>();
            sphere->material->baseColor = Vec3(0.8f, 0.1f, 0.1f);  // Red base
            sphere->material->metallic = metallic;
            sphere->material->roughness = roughness;
        }
    }
    
    std::cout << "Created " << gridSize * gridSize << " material showcase spheres\n";
    std::cout << "X axis: Metallic (0 -> 1)\n";
    std::cout << "Z axis: Roughness (0 -> 1)\n";
}

// ============================================
// Example 5: Emissive Materials
// ============================================

inline void example_EmissiveMaterials() {
    // Create a glowing material
    Material emissive;
    emissive.name = "GlowingRed";
    emissive.baseColor = Vec3(0.1f, 0.1f, 0.1f);   // Dark base
    emissive.metallic = 0.0f;
    emissive.roughness = 0.5f;
    
    // Enable emission
    emissive.emissiveColor = Vec3(1.0f, 0.2f, 0.1f);  // Orange-red glow
    emissive.emissiveIntensity = 5.0f;                 // Strong emission
    
    std::cout << "Emissive Material: " << emissive.name << "\n";
    std::cout << "  Emission: (" << emissive.emissiveColor.x * emissive.emissiveIntensity
              << ", " << emissive.emissiveColor.y * emissive.emissiveIntensity
              << ", " << emissive.emissiveColor.z * emissive.emissiveIntensity << ")\n";
    
    // Neon sign effect
    Material neon;
    neon.name = "NeonBlue";
    neon.baseColor = Vec3(0.0f, 0.0f, 0.0f);
    neon.emissiveColor = Vec3(0.0f, 0.5f, 1.0f);
    neon.emissiveIntensity = 10.0f;
    
    // Lava effect
    Material lava;
    lava.name = "Lava";
    lava.baseColor = Vec3(0.1f, 0.05f, 0.0f);
    lava.emissiveColor = Vec3(1.0f, 0.3f, 0.0f);
    lava.emissiveIntensity = 3.0f;
    lava.roughness = 0.9f;
}

// ============================================
// Example 6: Transparent Materials
// ============================================

inline void example_TransparentMaterials() {
    // Glass-like transparent material
    Material glass;
    glass.name = "Glass";
    glass.baseColor = Vec3(1.0f, 1.0f, 1.0f);
    glass.alpha = 0.2f;            // 20% opacity
    glass.metallic = 0.0f;
    glass.roughness = 0.0f;        // Perfectly smooth
    glass.alphaBlend = true;       // Enable alpha blending
    
    // Colored glass
    Material coloredGlass;
    coloredGlass.name = "ColoredGlass";
    coloredGlass.baseColor = Vec3(0.2f, 0.8f, 0.3f);  // Green tint
    coloredGlass.alpha = 0.3f;
    coloredGlass.alphaBlend = true;
    
    // Alpha cutout (for foliage)
    Material foliage;
    foliage.name = "Foliage";
    foliage.baseColor = Vec3(0.2f, 0.6f, 0.1f);
    foliage.alphaCutoff = true;
    foliage.alphaCutoffValue = 0.5f;  // Discard pixels below 50% alpha
    
    // Two-sided material (for thin surfaces)
    Material thinSurface;
    thinSurface.name = "Leaf";
    thinSurface.twoSided = true;  // Render both sides
    
    std::cout << "Glass alpha: " << glass.alpha << "\n";
    std::cout << "Foliage cutoff: " << foliage.alphaCutoffValue << "\n";
}

// ============================================
// Example 7: Material Properties Reference
// ============================================

inline void example_MaterialReference() {
    std::cout << "\n=== PBR Material Properties Reference ===\n\n";
    
    std::cout << "METALLIC (0.0 - 1.0):\n";
    std::cout << "  0.0 = Dielectric (plastic, wood, fabric)\n";
    std::cout << "  1.0 = Metal (gold, silver, iron)\n\n";
    
    std::cout << "ROUGHNESS (0.0 - 1.0):\n";
    std::cout << "  0.0 = Mirror-like (glass, polished metal)\n";
    std::cout << "  0.3 = Satin finish\n";
    std::cout << "  0.5 = Medium rough\n";
    std::cout << "  0.7 = Matte\n";
    std::cout << "  1.0 = Completely rough (chalk, concrete)\n\n";
    
    std::cout << "BASE COLOR:\n";
    std::cout << "  Dielectrics: Albedo color (can be any color)\n";
    std::cout << "  Metals: Reflectance color (usually bright)\n\n";
    
    std::cout << "TYPICAL MATERIAL VALUES:\n";
    std::cout << "  Gold:     metallic=1.0, roughness=0.3, baseColor=(1.0, 0.86, 0.57)\n";
    std::cout << "  Silver:   metallic=1.0, roughness=0.2, baseColor=(0.97, 0.96, 0.91)\n";
    std::cout << "  Plastic:  metallic=0.0, roughness=0.4, baseColor=(any)\n";
    std::cout << "  Rubber:   metallic=0.0, roughness=0.8, baseColor=(any)\n";
    std::cout << "  Glass:    metallic=0.0, roughness=0.0, alpha=0.2\n";
}

}  // namespace examples
}  // namespace luma
