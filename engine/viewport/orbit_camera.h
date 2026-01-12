// Orbit Camera - Maya-style camera controller
#pragma once

#include <cmath>
#include <algorithm>

namespace luma {

// Camera control mode
enum class CameraMode { None, Orbit, Pan, Zoom };

// Orbit camera for 3D viewport navigation
class OrbitCamera {
public:
    // Camera state
    float yaw = 0.0f;           // Horizontal rotation (radians)
    float pitch = 0.3f;         // Vertical rotation (radians)
    float distance = 2.5f;      // Distance multiplier
    
    // Target point offset (pivot)
    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = 0.0f;
    
    // Sensitivity
    float orbitSpeed = 0.01f;
    float panSpeed = 0.01f;
    float zoomSpeed = 0.1f;
    
    // Limits
    float minPitch = -1.5f;
    float maxPitch = 1.5f;
    float minDistance = 0.1f;
    float maxDistance = 100.0f;
    
    // Apply orbit rotation
    void orbit(float dx, float dy) {
        yaw -= dx * orbitSpeed;
        pitch += dy * orbitSpeed;
        pitch = std::max(minPitch, std::min(maxPitch, pitch));
    }
    
    // Apply pan movement (based on camera orientation)
    void pan(float dx, float dy, float modelRadius) {
        float scale = distance * modelRadius * panSpeed;
        
        // Right vector based on yaw
        float rightX = cosf(yaw);
        float rightZ = -sinf(yaw);
        
        // Single-axis constraint
        if (std::abs(dx) > std::abs(dy)) {
            targetX -= rightX * dx * scale;
            targetZ -= rightZ * dx * scale;
        } else {
            targetY += dy * scale;
        }
    }
    
    // Apply zoom
    void zoom(float delta, float /*modelRadius*/) {
        distance -= delta * zoomSpeed * distance;
        distance = std::max(minDistance, std::min(maxDistance, distance));
    }
    
    // Reset to default state
    void reset() {
        yaw = 0.0f;
        pitch = 0.3f;
        distance = 2.5f;
        targetX = targetY = targetZ = 0.0f;
    }
    
    // Calculate eye position given model center and radius
    void getEyeAndTarget(const float* modelCenter, float modelRadius,
                         float* outEye, float* outTarget) const {
        // Target = model center + offset
        outTarget[0] = modelCenter[0] + targetX;
        outTarget[1] = modelCenter[1] + targetY;
        outTarget[2] = modelCenter[2] + targetZ;
        
        // Eye orbits around target
        float camDist = modelRadius * 2.5f * distance;
        outEye[0] = outTarget[0] + sinf(yaw) * cosf(pitch) * camDist;
        outEye[1] = outTarget[1] + sinf(pitch) * camDist;
        outEye[2] = outTarget[2] + cosf(yaw) * cosf(pitch) * camDist;
    }
};

}  // namespace luma
