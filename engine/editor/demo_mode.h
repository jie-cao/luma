// Demo Mode - Built-in demonstration scenes and tutorials
// Access via Help menu in LUMA Studio
#pragma once

#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/material/material.h"
#include "engine/lighting/light.h"
#include "engine/foundation/math_types.h"
#include <functional>
#include <string>
#include <vector>
#include <cmath>
#include <random>

namespace luma {

// ============================================
// Demo Scene Generator
// ============================================

struct DemoInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string category;
};

class DemoMode {
public:
    static DemoMode& get() {
        static DemoMode instance;
        return instance;
    }
    
    // Get all available demos
    std::vector<DemoInfo> getAvailableDemos() const {
        return {
            {"basic", "Basic Scene", "Simple objects with basic lighting", "Getting Started"},
            {"materials", "Material Showcase", "PBR material grid demonstration", "Materials"},
            {"lighting", "Multi-Light Demo", "Various light types and colors", "Lighting"},
            {"hierarchy", "Scene Hierarchy", "Parent-child relationships demo", "Scene"},
            {"animation_ready", "Animation Ready Scene", "Character placeholder for animation", "Animation"},
            {"post_process", "Post-Processing Demo", "Bloom and tone mapping showcase", "Effects"},
            {"stress_test", "Performance Stress Test", "Large scene for optimization testing", "Performance"},
            {"material_presets", "Material Presets", "All built-in material presets", "Materials"},
            {"emissive", "Emissive Materials", "Glowing and neon effects", "Materials"},
            {"three_point", "Three-Point Lighting", "Classic cinematography setup", "Lighting"}
        };
    }
    
    // Generate a demo scene
    bool generateDemo(const std::string& demoId, SceneGraph& scene) {
        // Clear existing scene
        scene.clear();
        
        if (demoId == "basic") return generateBasicScene(scene);
        if (demoId == "materials") return generateMaterialShowcase(scene);
        if (demoId == "lighting") return generateLightingDemo(scene);
        if (demoId == "hierarchy") return generateHierarchyDemo(scene);
        if (demoId == "animation_ready") return generateAnimationReadyScene(scene);
        if (demoId == "post_process") return generatePostProcessDemo(scene);
        if (demoId == "stress_test") return generateStressTest(scene);
        if (demoId == "material_presets") return generateMaterialPresets(scene);
        if (demoId == "emissive") return generateEmissiveDemo(scene);
        if (demoId == "three_point") return generateThreePointLighting(scene);
        
        return false;
    }
    
private:
    DemoMode() = default;
    
    // ============================================
    // Demo Generators
    // ============================================
    
    bool generateBasicScene(SceneGraph& scene) {
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.position = Vec3(0, 0, 0);
        ground->localTransform.scale = Vec3(20, 0.1f, 20);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.3f, 0.3f, 0.3f);
        ground->material->roughness = 0.9f;
        
        // Red Cube
        Entity* cube = scene.createEntity("RedCube");
        cube->localTransform.position = Vec3(-3, 1, 0);
        cube->material = std::make_shared<Material>(Material::createPlastic(Vec3(0.8f, 0.2f, 0.1f)));
        
        // Gold Sphere
        Entity* sphere = scene.createEntity("GoldSphere");
        sphere->localTransform.position = Vec3(0, 1, 0);
        sphere->material = std::make_shared<Material>(Material::createGold());
        
        // Blue Cylinder
        Entity* cylinder = scene.createEntity("BlueCylinder");
        cylinder->localTransform.position = Vec3(3, 1.5f, 0);
        cylinder->localTransform.scale = Vec3(1, 3, 1);
        cylinder->material = std::make_shared<Material>(Material::createPlastic(Vec3(0.1f, 0.3f, 0.8f)));
        
        // Add lights
        addDefaultLighting(scene);
        
        return true;
    }
    
    bool generateMaterialShowcase(SceneGraph& scene) {
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(30, 0.1f, 30);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.15f, 0.15f, 0.15f);
        ground->material->roughness = 0.95f;
        
        // Create 5x5 grid of spheres
        const int gridSize = 5;
        const float spacing = 2.5f;
        
        for (int row = 0; row < gridSize; row++) {
            for (int col = 0; col < gridSize; col++) {
                float metallic = static_cast<float>(col) / (gridSize - 1);
                float roughness = static_cast<float>(row) / (gridSize - 1);
                
                std::string name = "Sphere_M" + std::to_string(col) + "_R" + std::to_string(row);
                Entity* sphere = scene.createEntity(name);
                
                float x = (col - gridSize / 2.0f) * spacing;
                float z = (row - gridSize / 2.0f) * spacing;
                sphere->localTransform.position = Vec3(x, 1, z);
                sphere->localTransform.scale = Vec3(0.8f, 0.8f, 0.8f);
                
                sphere->material = std::make_shared<Material>();
                sphere->material->baseColor = Vec3(0.8f, 0.1f, 0.1f);
                sphere->material->metallic = metallic;
                sphere->material->roughness = roughness;
            }
        }
        
        // Labels (empty entities for reference)
        Entity* xLabel = scene.createEntity("X_Axis_Metallic");
        Entity* zLabel = scene.createEntity("Z_Axis_Roughness");
        
        addDefaultLighting(scene);
        return true;
    }
    
    bool generateLightingDemo(SceneGraph& scene) {
        // Ground and walls
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(30, 0.1f, 30);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.4f, 0.4f, 0.4f);
        ground->material->roughness = 0.8f;
        
        Entity* wall = scene.createEntity("BackWall");
        wall->localTransform.position = Vec3(0, 5, -10);
        wall->localTransform.scale = Vec3(20, 10, 0.2f);
        wall->material = std::make_shared<Material>();
        wall->material->baseColor = Vec3(0.6f, 0.6f, 0.6f);
        wall->material->roughness = 0.9f;
        
        // Center sphere (white, shows lighting well)
        Entity* center = scene.createEntity("CenterSphere");
        center->localTransform.position = Vec3(0, 2, 0);
        center->localTransform.scale = Vec3(2, 2, 2);
        center->material = std::make_shared<Material>();
        center->material->baseColor = Vec3(0.9f, 0.9f, 0.9f);
        center->material->roughness = 0.3f;
        
        // Left - Gold
        Entity* left = scene.createEntity("GoldSphere");
        left->localTransform.position = Vec3(-5, 1.5f, 0);
        left->material = std::make_shared<Material>(Material::createGold());
        
        // Right - Silver
        Entity* right = scene.createEntity("SilverSphere");
        right->localTransform.position = Vec3(5, 1.5f, 0);
        right->material = std::make_shared<Material>(Material::createSilver());
        
        // Light bulb visuals (emissive spheres)
        createLightBulb(scene, "RedLight", Vec3(-6, 3, 3), Vec3(1, 0.2f, 0.1f));
        createLightBulb(scene, "GreenLight", Vec3(0, 3, 5), Vec3(0.2f, 1, 0.3f));
        createLightBulb(scene, "BlueLight", Vec3(6, 3, 3), Vec3(0.1f, 0.3f, 1));
        
        // Add directional sun
        Entity* sun = scene.createEntity("Sun");
        sun->hasLight = true;
        sun->light = Light::createDirectional();
        sun->light.color = Vec3(1, 0.95f, 0.9f);
        sun->light.intensity = 2.0f;
        sun->light.direction = Vec3(-0.3f, -1, -0.5f).normalized();
        
        // Add colored point lights
        Entity* redLight = scene.createEntity("RedPointLight");
        redLight->localTransform.position = Vec3(-6, 3, 3);
        redLight->hasLight = true;
        redLight->light = Light::createPoint();
        redLight->light.color = Vec3(1, 0.2f, 0.1f);
        redLight->light.intensity = 100.0f;
        
        Entity* greenLight = scene.createEntity("GreenPointLight");
        greenLight->localTransform.position = Vec3(0, 3, 5);
        greenLight->hasLight = true;
        greenLight->light = Light::createPoint();
        greenLight->light.color = Vec3(0.2f, 1, 0.3f);
        greenLight->light.intensity = 100.0f;
        
        Entity* blueLight = scene.createEntity("BluePointLight");
        blueLight->localTransform.position = Vec3(6, 3, 3);
        blueLight->hasLight = true;
        blueLight->light = Light::createPoint();
        blueLight->light.color = Vec3(0.1f, 0.3f, 1);
        blueLight->light.intensity = 100.0f;
        
        // Add spotlight
        Entity* spot = scene.createEntity("Spotlight");
        spot->localTransform.position = Vec3(0, 8, 0);
        spot->hasLight = true;
        spot->light = Light::createSpot();
        spot->light.color = Vec3(1, 1, 0.9f);
        spot->light.intensity = 300.0f;
        spot->light.direction = Vec3(0, -1, 0);
        spot->light.innerConeAngle = 0.2f;
        spot->light.outerConeAngle = 0.4f;
        
        return true;
    }
    
    bool generateHierarchyDemo(SceneGraph& scene) {
        // Root parent
        Entity* solarSystem = scene.createEntity("SolarSystem");
        solarSystem->localTransform.position = Vec3(0, 3, 0);
        
        // Sun (center)
        Entity* sun = scene.createEntity("Sun");
        sun->localTransform.scale = Vec3(2, 2, 2);
        sun->material = std::make_shared<Material>();
        sun->material->emissiveColor = Vec3(1, 0.8f, 0.3f);
        sun->material->emissiveIntensity = 5.0f;
        scene.setParent(sun, solarSystem);
        
        // Earth orbit
        Entity* earthOrbit = scene.createEntity("EarthOrbit");
        earthOrbit->localTransform.rotation = Quat::fromEuler(0, 0.5f, 0);
        scene.setParent(earthOrbit, solarSystem);
        
        Entity* earth = scene.createEntity("Earth");
        earth->localTransform.position = Vec3(5, 0, 0);
        earth->localTransform.scale = Vec3(0.8f, 0.8f, 0.8f);
        earth->material = std::make_shared<Material>(Material::createPlastic(Vec3(0.2f, 0.4f, 0.8f)));
        scene.setParent(earth, earthOrbit);
        
        // Moon
        Entity* moonOrbit = scene.createEntity("MoonOrbit");
        scene.setParent(moonOrbit, earth);
        
        Entity* moon = scene.createEntity("Moon");
        moon->localTransform.position = Vec3(1.5f, 0, 0);
        moon->localTransform.scale = Vec3(0.3f, 0.3f, 0.3f);
        moon->material = std::make_shared<Material>();
        moon->material->baseColor = Vec3(0.7f, 0.7f, 0.7f);
        moon->material->roughness = 0.8f;
        scene.setParent(moon, moonOrbit);
        
        // Mars orbit
        Entity* marsOrbit = scene.createEntity("MarsOrbit");
        marsOrbit->localTransform.rotation = Quat::fromEuler(0, 1.2f, 0);
        scene.setParent(marsOrbit, solarSystem);
        
        Entity* mars = scene.createEntity("Mars");
        mars->localTransform.position = Vec3(8, 0, 0);
        mars->localTransform.scale = Vec3(0.6f, 0.6f, 0.6f);
        mars->material = std::make_shared<Material>(Material::createPlastic(Vec3(0.8f, 0.3f, 0.1f)));
        scene.setParent(mars, marsOrbit);
        
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(30, 0.1f, 30);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.2f, 0.2f, 0.2f);
        
        addDefaultLighting(scene);
        return true;
    }
    
    bool generateAnimationReadyScene(SceneGraph& scene) {
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(20, 0.1f, 20);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.35f, 0.35f, 0.35f);
        
        // Character placeholder (replace with loaded model)
        Entity* character = scene.createEntity("Character");
        character->localTransform.position = Vec3(0, 0, 0);
        
        // Body parts as hierarchy
        Entity* hips = scene.createEntity("Hips");
        hips->localTransform.position = Vec3(0, 1, 0);
        scene.setParent(hips, character);
        
        Entity* spine = scene.createEntity("Spine");
        spine->localTransform.position = Vec3(0, 0.5f, 0);
        scene.setParent(spine, hips);
        
        Entity* head = scene.createEntity("Head");
        head->localTransform.position = Vec3(0, 0.8f, 0);
        scene.setParent(head, spine);
        
        Entity* armL = scene.createEntity("Arm_L");
        armL->localTransform.position = Vec3(-0.4f, 0.3f, 0);
        scene.setParent(armL, spine);
        
        Entity* armR = scene.createEntity("Arm_R");
        armR->localTransform.position = Vec3(0.4f, 0.3f, 0);
        scene.setParent(armR, spine);
        
        Entity* legL = scene.createEntity("Leg_L");
        legL->localTransform.position = Vec3(-0.2f, -0.5f, 0);
        scene.setParent(legL, hips);
        
        Entity* legR = scene.createEntity("Leg_R");
        legR->localTransform.position = Vec3(0.2f, -0.5f, 0);
        scene.setParent(legR, hips);
        
        addDefaultLighting(scene);
        return true;
    }
    
    bool generatePostProcessDemo(SceneGraph& scene) {
        // Dark ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(30, 0.1f, 30);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.1f, 0.1f, 0.1f);
        
        // Bright emissive objects for bloom
        for (int i = 0; i < 5; i++) {
            float angle = i * (2.0f * 3.14159f / 5.0f);
            float x = std::cos(angle) * 5.0f;
            float z = std::sin(angle) * 5.0f;
            
            Entity* bright = scene.createEntity("Bright_" + std::to_string(i));
            bright->localTransform.position = Vec3(x, 2, z);
            bright->material = std::make_shared<Material>();
            
            // Different bright colors
            Vec3 colors[] = {
                Vec3(1, 0.2f, 0.1f),
                Vec3(0.2f, 1, 0.2f),
                Vec3(0.1f, 0.4f, 1),
                Vec3(1, 1, 0.2f),
                Vec3(1, 0.2f, 1)
            };
            bright->material->emissiveColor = colors[i];
            bright->material->emissiveIntensity = 8.0f;
        }
        
        // Center reflective sphere
        Entity* chrome = scene.createEntity("ChromeSphere");
        chrome->localTransform.position = Vec3(0, 2, 0);
        chrome->localTransform.scale = Vec3(2, 2, 2);
        chrome->material = std::make_shared<Material>(Material::createSilver());
        chrome->material->roughness = 0.0f;
        
        // Minimal lighting (let emissives shine)
        Entity* light = scene.createEntity("AmbientLight");
        light->hasLight = true;
        light->light = Light::createDirectional();
        light->light.intensity = 0.5f;
        light->light.direction = Vec3(-0.5f, -1, -0.5f).normalized();
        
        return true;
    }
    
    bool generateStressTest(SceneGraph& scene) {
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(100, 0.1f, 100);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.3f, 0.3f, 0.3f);
        
        // Create grid of objects
        const int gridSize = 30;  // 30x30 = 900 objects
        const float spacing = 3.0f;
        
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> rotDist(0, 6.28f);
        std::uniform_real_distribution<float> scaleDist(0.5f, 1.5f);
        
        int count = 0;
        for (int x = 0; x < gridSize; x++) {
            for (int z = 0; z < gridSize; z++) {
                Entity* e = scene.createEntity("Object_" + std::to_string(count++));
                
                float px = (x - gridSize / 2.0f) * spacing;
                float pz = (z - gridSize / 2.0f) * spacing;
                e->localTransform.position = Vec3(px, 0.5f, pz);
                e->localTransform.rotation = Quat::fromEuler(0, rotDist(rng), 0);
                
                float s = scaleDist(rng);
                e->localTransform.scale = Vec3(s, s, s);
                
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
        
        addDefaultLighting(scene);
        return true;
    }
    
    bool generateMaterialPresets(SceneGraph& scene) {
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(20, 0.1f, 20);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.2f, 0.2f, 0.2f);
        
        // Create all material presets
        struct PresetInfo {
            const char* name;
            Material material;
        };
        
        std::vector<PresetInfo> presets = {
            {"Gold", Material::createGold()},
            {"Silver", Material::createSilver()},
            {"Copper", Material::createCopper()},
            {"RedPlastic", Material::createPlastic(Vec3(0.8f, 0.1f, 0.1f))},
            {"GreenPlastic", Material::createPlastic(Vec3(0.1f, 0.8f, 0.1f))},
            {"BluePlastic", Material::createPlastic(Vec3(0.1f, 0.1f, 0.8f))},
            {"BlackRubber", Material::createRubber(Vec3(0.1f, 0.1f, 0.1f))},
            {"Glass", Material::createGlass()}
        };
        
        int cols = 4;
        for (size_t i = 0; i < presets.size(); i++) {
            int row = static_cast<int>(i) / cols;
            int col = static_cast<int>(i) % cols;
            
            float x = (col - cols / 2.0f + 0.5f) * 3.0f;
            float z = (row - 1.0f) * 3.0f;
            
            Entity* sphere = scene.createEntity(presets[i].name);
            sphere->localTransform.position = Vec3(x, 1, z);
            sphere->material = std::make_shared<Material>(presets[i].material);
        }
        
        addDefaultLighting(scene);
        return true;
    }
    
    bool generateEmissiveDemo(SceneGraph& scene) {
        // Dark ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(30, 0.1f, 30);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.05f, 0.05f, 0.05f);
        
        // Neon signs
        struct NeonSign {
            const char* name;
            Vec3 position;
            Vec3 color;
        };
        
        std::vector<NeonSign> signs = {
            {"Neon_Red", Vec3(-5, 2, -5), Vec3(1, 0.1f, 0.1f)},
            {"Neon_Green", Vec3(0, 2, -5), Vec3(0.1f, 1, 0.2f)},
            {"Neon_Blue", Vec3(5, 2, -5), Vec3(0.1f, 0.3f, 1)},
            {"Neon_Yellow", Vec3(-2.5f, 3.5f, -5), Vec3(1, 1, 0.1f)},
            {"Neon_Cyan", Vec3(2.5f, 3.5f, -5), Vec3(0.1f, 1, 1)},
            {"Neon_Magenta", Vec3(0, 5, -5), Vec3(1, 0.1f, 1)}
        };
        
        for (const auto& sign : signs) {
            Entity* e = scene.createEntity(sign.name);
            e->localTransform.position = sign.position;
            e->localTransform.scale = Vec3(2, 0.5f, 0.2f);
            e->material = std::make_shared<Material>();
            e->material->baseColor = Vec3(0.1f, 0.1f, 0.1f);
            e->material->emissiveColor = sign.color;
            e->material->emissiveIntensity = 10.0f;
        }
        
        // Lava pool
        Entity* lava = scene.createEntity("LavaPool");
        lava->localTransform.position = Vec3(0, 0.1f, 3);
        lava->localTransform.scale = Vec3(5, 0.1f, 3);
        lava->material = std::make_shared<Material>();
        lava->material->baseColor = Vec3(0.2f, 0.05f, 0.0f);
        lava->material->emissiveColor = Vec3(1, 0.3f, 0.0f);
        lava->material->emissiveIntensity = 3.0f;
        lava->material->roughness = 0.9f;
        
        // Very dim lighting
        Entity* light = scene.createEntity("DimLight");
        light->hasLight = true;
        light->light = Light::createDirectional();
        light->light.intensity = 0.2f;
        
        return true;
    }
    
    bool generateThreePointLighting(SceneGraph& scene) {
        // Ground
        Entity* ground = scene.createEntity("Ground");
        ground->localTransform.scale = Vec3(20, 0.1f, 20);
        ground->material = std::make_shared<Material>();
        ground->material->baseColor = Vec3(0.3f, 0.3f, 0.3f);
        
        // Subject
        Entity* subject = scene.createEntity("Subject");
        subject->localTransform.position = Vec3(0, 1.5f, 0);
        subject->localTransform.scale = Vec3(1.5f, 3, 1.5f);
        subject->material = std::make_shared<Material>();
        subject->material->baseColor = Vec3(0.8f, 0.6f, 0.5f);
        subject->material->roughness = 0.5f;
        
        // Key Light (main, warm, strong)
        Entity* keyLight = scene.createEntity("KeyLight");
        keyLight->hasLight = true;
        keyLight->light = Light::createDirectional();
        keyLight->light.name = "Key";
        keyLight->light.color = Vec3(1.0f, 0.95f, 0.9f);
        keyLight->light.intensity = 4.0f;
        keyLight->light.direction = Vec3(-0.5f, -0.7f, -0.5f).normalized();
        
        // Fill Light (softer, cooler, weaker)
        Entity* fillLight = scene.createEntity("FillLight");
        fillLight->hasLight = true;
        fillLight->light = Light::createDirectional();
        fillLight->light.name = "Fill";
        fillLight->light.color = Vec3(0.8f, 0.9f, 1.0f);
        fillLight->light.intensity = 1.5f;
        fillLight->light.direction = Vec3(0.5f, -0.3f, -0.5f).normalized();
        
        // Rim/Back Light (edge highlight)
        Entity* rimLight = scene.createEntity("RimLight");
        rimLight->localTransform.position = Vec3(0, 4, 5);
        rimLight->hasLight = true;
        rimLight->light = Light::createPoint();
        rimLight->light.name = "Rim";
        rimLight->light.color = Vec3(1, 1, 1);
        rimLight->light.intensity = 150.0f;
        
        return true;
    }
    
    // ============================================
    // Helper Functions
    // ============================================
    
    void addDefaultLighting(SceneGraph& scene) {
        // Main sun light
        Entity* sun = scene.createEntity("Sun");
        sun->hasLight = true;
        sun->light = Light::createDirectional();
        sun->light.color = Vec3(1, 0.95f, 0.9f);
        sun->light.intensity = 3.0f;
        sun->light.direction = Vec3(-0.5f, -1, -0.3f).normalized();
        
        // Fill light
        Entity* fill = scene.createEntity("FillLight");
        fill->hasLight = true;
        fill->light = Light::createDirectional();
        fill->light.color = Vec3(0.7f, 0.8f, 1.0f);
        fill->light.intensity = 1.0f;
        fill->light.direction = Vec3(0.5f, -0.3f, 0.5f).normalized();
    }
    
    void createLightBulb(SceneGraph& scene, const std::string& name, const Vec3& position, const Vec3& color) {
        Entity* bulb = scene.createEntity(name + "_Bulb");
        bulb->localTransform.position = position;
        bulb->localTransform.scale = Vec3(0.3f, 0.3f, 0.3f);
        bulb->material = std::make_shared<Material>();
        bulb->material->baseColor = Vec3(0.1f, 0.1f, 0.1f);
        bulb->material->emissiveColor = color;
        bulb->material->emissiveIntensity = 10.0f;
    }
};

// ============================================
// Demo Menu UI Helper
// ============================================

struct DemoMenuState {
    bool showDemoMenu = false;
    int selectedDemo = -1;
};

// Call this in your ImGui render loop to show demo menu
inline void renderDemoMenu(DemoMenuState& state, SceneGraph& scene) {
    // This would be called from the Help menu
    // Help -> Demos -> [list of demos]
    
    if (!state.showDemoMenu) return;
    
    auto& demoMode = DemoMode::get();
    auto demos = demoMode.getAvailableDemos();
    
    // Group by category
    std::string currentCategory;
    
    // ImGui::Begin("Demo Scenes", &state.showDemoMenu);
    // for (const auto& demo : demos) {
    //     if (demo.category != currentCategory) {
    //         if (!currentCategory.empty()) ImGui::Separator();
    //         ImGui::TextColored({0.7f, 0.7f, 1.0f, 1.0f}, "%s", demo.category.c_str());
    //         currentCategory = demo.category;
    //     }
    //     
    //     if (ImGui::MenuItem(demo.name.c_str())) {
    //         demoMode.generateDemo(demo.id, scene);
    //     }
    //     if (ImGui::IsItemHovered()) {
    //         ImGui::SetTooltip("%s", demo.description.c_str());
    //     }
    // }
    // ImGui::End();
}

}  // namespace luma
