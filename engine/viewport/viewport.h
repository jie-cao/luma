// Viewport - 3D viewport with camera control and grid
#pragma once

#include "orbit_camera.h"
#include "engine/renderer/unified_renderer.h"
#include <string>
#include <unordered_map>

namespace luma {

// Viewport settings
struct ViewportSettings {
    bool showGrid = true;
    bool autoRotate = false;
    float autoRotateSpeed = 0.5f;
    
    // View modes
    bool wireframe = false;
    bool orthographic = false;
};

// Camera bookmark/preset
struct CameraPreset {
    std::string name;
    float yaw;
    float pitch;
    float distance;
    float targetX, targetY, targetZ;
    
    // Standard presets
    static CameraPreset Front() {
        return {"Front", 0.0f, 0.0f, 2.5f, 0, 0, 0};
    }
    static CameraPreset Back() {
        return {"Back", 3.14159f, 0.0f, 2.5f, 0, 0, 0};
    }
    static CameraPreset Left() {
        return {"Left", 1.5708f, 0.0f, 2.5f, 0, 0, 0};
    }
    static CameraPreset Right() {
        return {"Right", -1.5708f, 0.0f, 2.5f, 0, 0, 0};
    }
    static CameraPreset Top() {
        return {"Top", 0.0f, 1.5f, 2.5f, 0, 0, 0};
    }
    static CameraPreset Bottom() {
        return {"Bottom", 0.0f, -1.5f, 2.5f, 0, 0, 0};
    }
    static CameraPreset Perspective() {
        return {"Perspective", 0.785f, 0.5f, 2.5f, 0, 0, 0};
    }
};

// 3D Viewport controller
class Viewport {
public:
    OrbitCamera camera;
    ViewportSettings settings;
    CameraMode cameraMode = CameraMode::None;
    
    // Process mouse input (call from WndProc)
    void onMouseDown(int button, float x, float y, bool altPressed) {
        lastMouseX_ = x;
        lastMouseY_ = y;
        
        if (altPressed) {
            if (button == 0) cameraMode = CameraMode::Orbit;      // Left
            else if (button == 1) cameraMode = CameraMode::Zoom;  // Right
            else if (button == 2) cameraMode = CameraMode::Pan;   // Middle
        }
    }
    
    void onMouseUp(int button) {
        if ((button == 0 && cameraMode == CameraMode::Orbit) ||
            (button == 1 && cameraMode == CameraMode::Zoom) ||
            (button == 2 && cameraMode == CameraMode::Pan)) {
            cameraMode = CameraMode::None;
        }
    }
    
    void onMouseMove(float x, float y, float modelRadius) {
        if (cameraMode == CameraMode::None) return;
        
        float dx = x - lastMouseX_;
        float dy = y - lastMouseY_;
        lastMouseX_ = x;
        lastMouseY_ = y;
        
        switch (cameraMode) {
        case CameraMode::Orbit:
            camera.orbit(dx, dy);
            break;
        case CameraMode::Pan:
            camera.pan(dx, dy, modelRadius);
            break;
        case CameraMode::Zoom:
            camera.zoom(-dy * 0.1f, modelRadius);
            break;
        default:
            break;
        }
    }
    
    void onMouseWheel(float delta, float modelRadius) {
        camera.zoom(delta, modelRadius);
    }
    
    void onKeyDown(int key) {
        if (key == 'F') camera.reset();
        if (key == 'G') settings.showGrid = !settings.showGrid;
    }
    
    // Update (for auto-rotation)
    void update(float deltaTime) {
        if (settings.autoRotate) {
            camera.yaw += deltaTime * settings.autoRotateSpeed;
        }
    }
    
    // Build camera params for renderer (RHI version)
    RHICameraParams getCameraParams() const {
        RHICameraParams params;
        params.yaw = camera.yaw;
        params.pitch = camera.pitch;
        params.distance = camera.distance;
        params.targetOffsetX = camera.targetX;
        params.targetOffsetY = camera.targetY;
        params.targetOffsetZ = camera.targetZ;
        return params;
    }
    
    // Render viewport content (UnifiedRenderer)
    void render(UnifiedRenderer& renderer, const RHILoadedModel& model) {
        auto camParams = getCameraParams();
        
        if (settings.showGrid) {
            renderer.renderGrid(camParams, model.radius);
        }
        
        renderer.render(model, camParams);
    }
    
    // === Camera Presets ===
    
    void applyCameraPreset(const CameraPreset& preset) {
        camera.yaw = preset.yaw;
        camera.pitch = preset.pitch;
        camera.distance = preset.distance;
        camera.targetX = preset.targetX;
        camera.targetY = preset.targetY;
        camera.targetZ = preset.targetZ;
    }
    
    CameraPreset getCurrentPreset(const std::string& name = "Custom") const {
        CameraPreset preset;
        preset.name = name;
        preset.yaw = camera.yaw;
        preset.pitch = camera.pitch;
        preset.distance = camera.distance;
        preset.targetX = camera.targetX;
        preset.targetY = camera.targetY;
        preset.targetZ = camera.targetZ;
        return preset;
    }
    
    void savePreset(const std::string& name) {
        savedPresets_[name] = getCurrentPreset(name);
    }
    
    void loadPreset(const std::string& name) {
        auto it = savedPresets_.find(name);
        if (it != savedPresets_.end()) {
            applyCameraPreset(it->second);
        }
    }
    
    bool hasPreset(const std::string& name) const {
        return savedPresets_.find(name) != savedPresets_.end();
    }
    
    const std::unordered_map<std::string, CameraPreset>& getSavedPresets() const {
        return savedPresets_;
    }
    
    void deletePreset(const std::string& name) {
        savedPresets_.erase(name);
    }
    
    // Quick views
    void viewFront() { applyCameraPreset(CameraPreset::Front()); }
    void viewBack() { applyCameraPreset(CameraPreset::Back()); }
    void viewLeft() { applyCameraPreset(CameraPreset::Left()); }
    void viewRight() { applyCameraPreset(CameraPreset::Right()); }
    void viewTop() { applyCameraPreset(CameraPreset::Top()); }
    void viewBottom() { applyCameraPreset(CameraPreset::Bottom()); }
    void viewPerspective() { applyCameraPreset(CameraPreset::Perspective()); }
    
private:
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
    std::unordered_map<std::string, CameraPreset> savedPresets_;
};

}  // namespace luma
