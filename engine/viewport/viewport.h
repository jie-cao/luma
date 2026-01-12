// Viewport - 3D viewport with camera control and grid
#pragma once

#include "orbit_camera.h"
#include "engine/renderer/pbr_renderer.h"

namespace luma {

// Viewport settings
struct ViewportSettings {
    bool showGrid = true;
    bool autoRotate = false;
    float autoRotateSpeed = 0.5f;
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
    
    // Build camera params for renderer
    PBRRenderer::CameraParams getCameraParams() const {
        PBRRenderer::CameraParams params;
        params.yaw = camera.yaw;
        params.pitch = camera.pitch;
        params.distance = camera.distance;
        params.targetOffsetX = camera.targetX;
        params.targetOffsetY = camera.targetY;
        params.targetOffsetZ = camera.targetZ;
        return params;
    }
    
    // Render viewport content
    void render(PBRRenderer& renderer, const LoadedModel& model) {
        auto camParams = getCameraParams();
        
        if (settings.showGrid) {
            renderer.renderGrid(camParams, model.radius);
        }
        
        renderer.render(model, camParams);
    }
    
private:
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
};

}  // namespace luma
