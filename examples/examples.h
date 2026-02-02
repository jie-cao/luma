// LUMA Examples - Master Include
// Include all example modules
#pragma once

// Basic scene setup and manipulation
#include "01_basic_scene.h"

// Animation system (skeletal, blending, state machine, IK)
#include "02_animation.h"

// PBR Materials
#include "03_materials.h"

// Lighting system
#include "04_lighting.h"

// Post-processing effects
#include "05_post_processing.h"

// Performance optimization
#include "06_performance.h"

// Character creation system
#include "07_character_creator.h"

namespace luma {
namespace examples {

// ============================================
// Run All Examples
// ============================================

inline void runAllExamples() {
    std::cout << "=== LUMA Examples ===\n\n";
    
    // Scene examples
    std::cout << "--- Scene Examples ---\n";
    example_BasicScene();
    example_Transforms();
    example_Selection();
    std::cout << "\n";
    
    // Material examples
    std::cout << "--- Material Examples ---\n";
    example_MaterialBasics();
    example_MaterialPresets();
    example_MaterialLibrary();
    example_EmissiveMaterials();
    example_TransparentMaterials();
    example_MaterialReference();
    std::cout << "\n";
    
    // Lighting examples
    std::cout << "--- Lighting Examples ---\n";
    example_LightTypes();
    example_LightManager();
    example_DynamicLighting();
    example_ColorTemperature();
    std::cout << "\n";
    
    // Animation examples
    std::cout << "--- Animation Examples ---\n";
    example_BasicAnimation();
    example_AnimationBlending();
    example_BlendTree1D();
    example_BlendTree2D();
    example_StateMachine();
    example_AnimationLayers();
    example_IK();
    std::cout << "\n";
    
    // Post-processing examples
    std::cout << "--- Post-Processing Examples ---\n";
    example_BasicPostProcess();
    example_ToneMappingMethods();
    example_BloomSettings();
    example_SSAOSettings();
    example_SSRSettings();
    example_VolumetricEffects();
    example_GodRays();
    example_Atmosphere();
    example_FullPostProcessStack();
    std::cout << "\n";
    
    // Performance examples
    std::cout << "--- Performance Examples ---\n";
    example_FrustumCulling();
    example_LOD();
    example_Instancing();
    example_RenderOptimizer();
    example_Benchmark();
    example_OptimizationTips();
    std::cout << "\n";
    
    std::cout << "=== Examples Complete ===\n";
}

// ============================================
// Example Runner Menu
// ============================================

struct ExampleCategory {
    const char* name;
    std::vector<std::pair<const char*, std::function<void()>>> examples;
};

inline std::vector<ExampleCategory> getExampleCategories() {
    return {
        {"Scene", {
            {"Basic Scene", example_BasicScene},
            {"Transforms", example_Transforms},
            {"Selection", example_Selection}
        }},
        {"Materials", {
            {"Material Basics", example_MaterialBasics},
            {"Material Presets", example_MaterialPresets},
            {"Material Library", example_MaterialLibrary},
            {"Emissive Materials", example_EmissiveMaterials},
            {"Transparent Materials", example_TransparentMaterials},
            {"Material Reference", example_MaterialReference}
        }},
        {"Lighting", {
            {"Light Types", example_LightTypes},
            {"Light Manager", example_LightManager},
            {"Dynamic Lighting", example_DynamicLighting},
            {"Color Temperature", example_ColorTemperature}
        }},
        {"Animation", {
            {"Basic Animation", example_BasicAnimation},
            {"Animation Blending", example_AnimationBlending},
            {"Blend Tree 1D", example_BlendTree1D},
            {"Blend Tree 2D", example_BlendTree2D},
            {"State Machine", example_StateMachine},
            {"Animation Layers", example_AnimationLayers},
            {"Inverse Kinematics", example_IK}
        }},
        {"Post-Processing", {
            {"Basic Post-Process", example_BasicPostProcess},
            {"Tone Mapping", example_ToneMappingMethods},
            {"Bloom Settings", example_BloomSettings},
            {"SSAO Settings", example_SSAOSettings},
            {"SSR Settings", example_SSRSettings},
            {"Volumetric Effects", example_VolumetricEffects},
            {"God Rays", example_GodRays},
            {"Atmosphere", example_Atmosphere},
            {"Full Stack", example_FullPostProcessStack}
        }},
        {"Performance", {
            {"Frustum Culling", example_FrustumCulling},
            {"LOD", example_LOD},
            {"Instancing", example_Instancing},
            {"Render Optimizer", example_RenderOptimizer},
            {"Benchmark", example_Benchmark},
            {"Optimization Tips", example_OptimizationTips}
        }},
        {"Character", {
            // Note: CharacterCreatorDemo is an interactive demo class
            // Use CharacterCreatorDemo in your application instead
        }}
    };
}

}  // namespace examples
}  // namespace luma
