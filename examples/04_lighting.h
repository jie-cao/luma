// Example 04: Lighting System
// Demonstrates different light types and configurations
#pragma once

#include "engine/lighting/light.h"
#include "engine/scene/scene_graph.h"
#include <iostream>
#include <cmath>

namespace luma {
namespace examples {

// ============================================
// Example 1: Light Types
// ============================================

inline void example_LightTypes() {
    // === Directional Light (Sun) ===
    Light sunLight = Light::createDirectional();
    sunLight.name = "Sun";
    sunLight.color = Vec3(1.0f, 0.95f, 0.8f);   // Warm sunlight
    sunLight.intensity = 5.0f;
    sunLight.direction = Vec3(-0.5f, -1.0f, -0.3f).normalized();  // From upper-right
    
    std::cout << "Directional Light (Sun):\n";
    std::cout << "  Direction: (" << sunLight.direction.x << ", " 
              << sunLight.direction.y << ", " << sunLight.direction.z << ")\n";
    std::cout << "  Intensity: " << sunLight.intensity << "\n";
    
    // === Point Light (Bulb) ===
    Light pointLight = Light::createPoint();
    pointLight.name = "Lamp";
    pointLight.color = Vec3(1.0f, 0.9f, 0.7f);  // Warm white
    pointLight.intensity = 100.0f;               // Lumens-like value
    pointLight.position = Vec3(0, 3, 0);
    
    // Attenuation (how light fades with distance)
    pointLight.constantAttenuation = 1.0f;
    pointLight.linearAttenuation = 0.09f;
    pointLight.quadraticAttenuation = 0.032f;
    
    std::cout << "\nPoint Light (Lamp):\n";
    std::cout << "  Position: (" << pointLight.position.x << ", " 
              << pointLight.position.y << ", " << pointLight.position.z << ")\n";
    std::cout << "  Intensity: " << pointLight.intensity << "\n";
    
    // === Spot Light (Flashlight) ===
    Light spotLight = Light::createSpot();
    spotLight.name = "Spotlight";
    spotLight.color = Vec3(1.0f, 1.0f, 1.0f);
    spotLight.intensity = 200.0f;
    spotLight.position = Vec3(0, 5, 0);
    spotLight.direction = Vec3(0, -1, 0);        // Point downward
    
    // Cone angles (in radians)
    spotLight.innerConeAngle = 0.3f;             // ~17 degrees - full intensity
    spotLight.outerConeAngle = 0.5f;             // ~28 degrees - falloff edge
    
    std::cout << "\nSpot Light (Spotlight):\n";
    std::cout << "  Inner Cone: " << (spotLight.innerConeAngle * 180.0f / 3.14159f) << " degrees\n";
    std::cout << "  Outer Cone: " << (spotLight.outerConeAngle * 180.0f / 3.14159f) << " degrees\n";
}

// ============================================
// Example 2: Light Manager
// ============================================

inline void example_LightManager() {
    LightManager& manager = LightManager::get();
    
    // Clear existing lights
    // (In real code, you might want to track IDs to remove specific lights)
    
    // Add main light
    Light mainLight = Light::createDirectional();
    mainLight.name = "MainLight";
    mainLight.intensity = 3.0f;
    int mainId = manager.addLight(mainLight);
    
    // Add fill light
    Light fillLight = Light::createDirectional();
    fillLight.name = "FillLight";
    fillLight.intensity = 1.0f;
    fillLight.direction = Vec3(0.5f, -0.5f, 0.3f).normalized();
    int fillId = manager.addLight(fillLight);
    
    // Add rim light
    Light rimLight = Light::createPoint();
    rimLight.name = "RimLight";
    rimLight.position = Vec3(-5, 3, -5);
    rimLight.intensity = 50.0f;
    int rimId = manager.addLight(rimLight);
    
    std::cout << "Active lights: " << manager.getActiveLightCount() << "\n";
    
    // Get and modify a light
    Light* main = manager.getLight(mainId);
    if (main) {
        main->intensity = 4.0f;  // Increase brightness
    }
    
    // List all lights
    std::cout << "\nAll lights:\n";
    for (const auto& light : manager.getAllLights()) {
        std::cout << "  - " << light.name << " (ID: " << light.id << ")\n";
    }
    
    // Remove a light
    manager.removeLight(fillId);
    std::cout << "\nAfter removing fill light: " << manager.getActiveLightCount() << " lights\n";
}

// ============================================
// Example 3: Three-Point Lighting Setup
// ============================================
// Classic film/photography lighting setup

inline void example_ThreePointLighting(SceneGraph& scene, const Vec3& subjectPosition) {
    LightManager& manager = LightManager::get();
    
    // === Key Light ===
    // Main light source, creates primary shadows
    Light keyLight = Light::createDirectional();
    keyLight.name = "KeyLight";
    keyLight.color = Vec3(1.0f, 0.98f, 0.95f);  // Slightly warm
    keyLight.intensity = 5.0f;
    // Position: 45 degrees to the side, 45 degrees above
    keyLight.direction = Vec3(-0.5f, -0.7f, -0.5f).normalized();
    manager.addLight(keyLight);
    
    // === Fill Light ===
    // Softer light to fill shadows
    Light fillLight = Light::createDirectional();
    fillLight.name = "FillLight";
    fillLight.color = Vec3(0.9f, 0.95f, 1.0f);  // Slightly cool
    fillLight.intensity = 2.0f;  // Less intense than key
    // Opposite side from key light
    fillLight.direction = Vec3(0.5f, -0.3f, -0.5f).normalized();
    manager.addLight(fillLight);
    
    // === Rim/Back Light ===
    // Creates edge highlight, separates subject from background
    Light rimLight = Light::createPoint();
    rimLight.name = "RimLight";
    rimLight.color = Vec3(1.0f, 1.0f, 1.0f);
    rimLight.intensity = 100.0f;
    // Position behind and above subject
    rimLight.position = subjectPosition + Vec3(0, 3, 5);
    manager.addLight(rimLight);
    
    std::cout << "Three-point lighting setup created:\n";
    std::cout << "  - Key Light (main illumination)\n";
    std::cout << "  - Fill Light (shadow softening)\n";
    std::cout << "  - Rim Light (edge highlight)\n";
}

// ============================================
// Example 4: Dynamic Lighting
// ============================================

inline void example_DynamicLighting() {
    LightManager& manager = LightManager::get();
    
    // Create a moving point light
    Light movingLight = Light::createPoint();
    movingLight.name = "MovingLight";
    movingLight.color = Vec3(1.0f, 0.5f, 0.2f);  // Orange
    movingLight.intensity = 150.0f;
    int lightId = manager.addLight(movingLight);
    
    // Simulate light movement (orbit around origin)
    std::cout << "Simulating light orbit:\n";
    for (int frame = 0; frame < 60; frame++) {
        float time = frame / 60.0f;  // 0 to 1 over 60 frames
        float angle = time * 2.0f * 3.14159f;  // Full rotation
        
        Light* light = manager.getLight(lightId);
        if (light) {
            // Orbit at radius 5, height 3
            light->position.x = std::cos(angle) * 5.0f;
            light->position.y = 3.0f;
            light->position.z = std::sin(angle) * 5.0f;
        }
        
        if (frame % 15 == 0) {
            std::cout << "  Frame " << frame << ": position = (" 
                      << light->position.x << ", " << light->position.y << ", " 
                      << light->position.z << ")\n";
        }
    }
}

// ============================================
// Example 5: Color Temperature
// ============================================

inline Vec3 kelvinToRGB(float kelvin) {
    // Approximate color temperature to RGB
    // Based on Tanner Helland's algorithm
    kelvin = std::clamp(kelvin, 1000.0f, 40000.0f) / 100.0f;
    
    float r, g, b;
    
    // Red
    if (kelvin <= 66.0f) {
        r = 255.0f;
    } else {
        r = 329.698727446f * std::pow(kelvin - 60.0f, -0.1332047592f);
    }
    
    // Green
    if (kelvin <= 66.0f) {
        g = 99.4708025861f * std::log(kelvin) - 161.1195681661f;
    } else {
        g = 288.1221695283f * std::pow(kelvin - 60.0f, -0.0755148492f);
    }
    
    // Blue
    if (kelvin >= 66.0f) {
        b = 255.0f;
    } else if (kelvin <= 19.0f) {
        b = 0.0f;
    } else {
        b = 138.5177312231f * std::log(kelvin - 10.0f) - 305.0447927307f;
    }
    
    return Vec3(
        std::clamp(r, 0.0f, 255.0f) / 255.0f,
        std::clamp(g, 0.0f, 255.0f) / 255.0f,
        std::clamp(b, 0.0f, 255.0f) / 255.0f
    );
}

inline void example_ColorTemperature() {
    std::cout << "Color Temperature Examples:\n";
    
    // Candlelight (warm)
    Vec3 candle = kelvinToRGB(1850.0f);
    std::cout << "  Candle (1850K): RGB(" << candle.x << ", " << candle.y << ", " << candle.z << ")\n";
    
    // Incandescent bulb
    Vec3 incandescent = kelvinToRGB(2700.0f);
    std::cout << "  Incandescent (2700K): RGB(" << incandescent.x << ", " << incandescent.y << ", " << incandescent.z << ")\n";
    
    // Daylight
    Vec3 daylight = kelvinToRGB(5500.0f);
    std::cout << "  Daylight (5500K): RGB(" << daylight.x << ", " << daylight.y << ", " << daylight.z << ")\n";
    
    // Overcast sky
    Vec3 overcast = kelvinToRGB(6500.0f);
    std::cout << "  Overcast (6500K): RGB(" << overcast.x << ", " << overcast.y << ", " << overcast.z << ")\n";
    
    // Blue sky
    Vec3 blueSky = kelvinToRGB(10000.0f);
    std::cout << "  Blue Sky (10000K): RGB(" << blueSky.x << ", " << blueSky.y << ", " << blueSky.z << ")\n";
    
    // Create lights with temperature
    Light warmLight = Light::createPoint();
    warmLight.color = kelvinToRGB(2700.0f);  // Warm bulb
    
    Light coolLight = Light::createPoint();
    coolLight.color = kelvinToRGB(6500.0f);  // Cool daylight
}

// ============================================
// Example 6: Light with Entity
// ============================================

inline void example_LightEntity(SceneGraph& scene) {
    // Create an entity with a light component
    Entity* lampEntity = scene.createEntity("TableLamp");
    lampEntity->localTransform.position = Vec3(2, 1, 0);
    
    // Enable light component
    lampEntity->hasLight = true;
    lampEntity->light = Light::createPoint();
    lampEntity->light.name = "TableLampLight";
    lampEntity->light.color = Vec3(1.0f, 0.9f, 0.7f);  // Warm
    lampEntity->light.intensity = 50.0f;
    
    // The light position will follow the entity transform
    lampEntity->light.position = lampEntity->localTransform.position;
    
    // Create a spotlight entity
    Entity* flashlightEntity = scene.createEntity("Flashlight");
    flashlightEntity->localTransform.position = Vec3(0, 2, 5);
    flashlightEntity->localTransform.rotation = Quat::fromEuler(-0.5f, 0, 0);  // Point down
    
    flashlightEntity->hasLight = true;
    flashlightEntity->light = Light::createSpot();
    flashlightEntity->light.name = "FlashlightBeam";
    flashlightEntity->light.intensity = 200.0f;
    flashlightEntity->light.innerConeAngle = 0.2f;
    flashlightEntity->light.outerConeAngle = 0.4f;
    
    std::cout << "Created light entities:\n";
    std::cout << "  - TableLamp (point light)\n";
    std::cout << "  - Flashlight (spot light)\n";
}

}  // namespace examples
}  // namespace luma
