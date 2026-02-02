// Camera Animation System - Keyframe-based camera paths
// Cinematic camera movement with smooth transitions
#pragma once

#include "engine/foundation/math_types.h"
#include <string>
#include <vector>
#include <cmath>
#include <functional>

namespace luma {

// ============================================================================
// Camera Keyframe
// ============================================================================

struct CameraKeyframe {
    float time = 0;
    
    // Position and orientation
    Vec3 position{0, 1.5f, 5};
    Vec3 target{0, 1, 0};       // Look-at target
    Vec3 up{0, 1, 0};           // Up vector
    
    // Camera properties
    float fov = 45.0f;          // Field of view in degrees
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    // Depth of field
    float focusDistance = 5.0f;
    float aperture = 2.8f;
    bool dofEnabled = false;
    
    // Interpolation
    enum class EaseType { Linear, EaseIn, EaseOut, EaseInOut, Bezier };
    EaseType easeIn = EaseType::EaseInOut;
    EaseType easeOut = EaseType::EaseInOut;
    
    // Bezier control points (relative to position)
    Vec3 inTangent{0, 0, 0};
    Vec3 outTangent{0, 0, 0};
};

// ============================================================================
// Camera Path - Collection of keyframes
// ============================================================================

struct CameraPath {
    std::string name;
    std::string nameCN;
    float duration = 0;
    bool loop = false;
    
    std::vector<CameraKeyframe> keyframes;
    
    // Add keyframe
    void addKeyframe(const CameraKeyframe& kf) {
        // Insert sorted by time
        auto it = keyframes.begin();
        while (it != keyframes.end() && it->time < kf.time) {
            ++it;
        }
        keyframes.insert(it, kf);
        
        if (kf.time > duration) {
            duration = kf.time;
        }
    }
    
    // Remove keyframe at index
    void removeKeyframe(int index) {
        if (index >= 0 && index < static_cast<int>(keyframes.size())) {
            keyframes.erase(keyframes.begin() + index);
            updateDuration();
        }
    }
    
    // Update keyframe time
    void setKeyframeTime(int index, float time) {
        if (index >= 0 && index < static_cast<int>(keyframes.size())) {
            CameraKeyframe kf = keyframes[index];
            kf.time = time;
            removeKeyframe(index);
            addKeyframe(kf);
        }
    }
    
    // Get keyframe at index
    CameraKeyframe* getKeyframe(int index) {
        if (index >= 0 && index < static_cast<int>(keyframes.size())) {
            return &keyframes[index];
        }
        return nullptr;
    }
    
    // Find keyframe index near time
    int findKeyframeNear(float time, float tolerance = 0.1f) const {
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (std::abs(keyframes[i].time - time) < tolerance) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
private:
    void updateDuration() {
        duration = 0;
        for (const auto& kf : keyframes) {
            if (kf.time > duration) {
                duration = kf.time;
            }
        }
    }
};

// ============================================================================
// Camera Interpolator
// ============================================================================

class CameraInterpolator {
public:
    // Interpolate between keyframes at time t
    static CameraKeyframe interpolate(const CameraPath& path, float time) {
        if (path.keyframes.empty()) {
            return CameraKeyframe{};
        }
        
        if (path.keyframes.size() == 1) {
            return path.keyframes[0];
        }
        
        // Handle looping
        if (path.loop && path.duration > 0) {
            time = std::fmod(time, path.duration);
        }
        
        // Clamp to range
        if (time <= path.keyframes.front().time) {
            return path.keyframes.front();
        }
        if (time >= path.keyframes.back().time) {
            return path.keyframes.back();
        }
        
        // Find surrounding keyframes
        int idx = 0;
        for (size_t i = 0; i < path.keyframes.size() - 1; i++) {
            if (path.keyframes[i + 1].time > time) {
                idx = static_cast<int>(i);
                break;
            }
        }
        
        const CameraKeyframe& kf0 = path.keyframes[idx];
        const CameraKeyframe& kf1 = path.keyframes[idx + 1];
        
        float segmentDuration = kf1.time - kf0.time;
        float t = (time - kf0.time) / segmentDuration;
        
        // Apply easing
        t = applyEasing(t, kf0.easeOut, kf1.easeIn);
        
        // Interpolate all properties
        CameraKeyframe result;
        result.time = time;
        
        // Position - use Bezier if tangents are set
        if (kf0.outTangent.lengthSquared() > 0.001f || kf1.inTangent.lengthSquared() > 0.001f) {
            result.position = bezierInterpolate(
                kf0.position,
                kf0.position + kf0.outTangent,
                kf1.position + kf1.inTangent,
                kf1.position,
                t
            );
        } else {
            result.position = lerp(kf0.position, kf1.position, t);
        }
        
        // Target - smooth interpolation
        result.target = lerp(kf0.target, kf1.target, t);
        
        // Up vector - slerp for orientation
        result.up = slerp(kf0.up, kf1.up, t);
        
        // Scalar properties
        result.fov = lerp(kf0.fov, kf1.fov, t);
        result.nearPlane = lerp(kf0.nearPlane, kf1.nearPlane, t);
        result.farPlane = lerp(kf0.farPlane, kf1.farPlane, t);
        result.focusDistance = lerp(kf0.focusDistance, kf1.focusDistance, t);
        result.aperture = lerp(kf0.aperture, kf1.aperture, t);
        result.dofEnabled = (t < 0.5f) ? kf0.dofEnabled : kf1.dofEnabled;
        
        return result;
    }
    
private:
    static float applyEasing(float t, CameraKeyframe::EaseType outEase, 
                             CameraKeyframe::EaseType inEase) {
        // Combine out ease of first keyframe with in ease of second
        // For simplicity, use average of both
        float t1 = applyEaseSingle(t, outEase);
        float t2 = applyEaseSingle(t, inEase);
        return (t1 + t2) * 0.5f;
    }
    
    static float applyEaseSingle(float t, CameraKeyframe::EaseType ease) {
        switch (ease) {
            case CameraKeyframe::EaseType::Linear:
                return t;
            case CameraKeyframe::EaseType::EaseIn:
                return t * t;
            case CameraKeyframe::EaseType::EaseOut:
                return t * (2.0f - t);
            case CameraKeyframe::EaseType::EaseInOut:
                return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
            case CameraKeyframe::EaseType::Bezier:
                // Smooth step
                return t * t * (3.0f - 2.0f * t);
            default:
                return t;
        }
    }
    
    static float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a * (1.0f - t) + b * t;
    }
    
    static Vec3 slerp(const Vec3& a, const Vec3& b, float t) {
        // Spherical interpolation for direction vectors
        Vec3 na = a.normalized();
        Vec3 nb = b.normalized();
        
        float dot = na.dot(nb);
        dot = std::clamp(dot, -1.0f, 1.0f);
        
        float theta = std::acos(dot) * t;
        Vec3 relative = (nb - na * dot).normalized();
        
        return na * std::cos(theta) + relative * std::sin(theta);
    }
    
    static Vec3 bezierInterpolate(const Vec3& p0, const Vec3& p1, 
                                   const Vec3& p2, const Vec3& p3, float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        
        Vec3 p = p0 * uuu;
        p = p + p1 * (3 * uu * t);
        p = p + p2 * (3 * u * tt);
        p = p + p3 * ttt;
        
        return p;
    }
};

// ============================================================================
// Camera Presets - Common camera movements
// ============================================================================

class CameraPresets {
public:
    // Orbit around target
    static CameraPath createOrbit(const Vec3& center, float radius, float height,
                                   float duration = 5.0f, bool clockwise = true) {
        CameraPath path;
        path.name = "Orbit";
        path.nameCN = "环绕";
        path.duration = duration;
        path.loop = true;
        
        int numKeyframes = 8;
        for (int i = 0; i <= numKeyframes; i++) {
            float t = static_cast<float>(i) / numKeyframes;
            float angle = t * 3.14159f * 2.0f * (clockwise ? -1.0f : 1.0f);
            
            CameraKeyframe kf;
            kf.time = t * duration;
            kf.position = Vec3(
                center.x + radius * std::cos(angle),
                center.y + height,
                center.z + radius * std::sin(angle)
            );
            kf.target = center;
            
            path.addKeyframe(kf);
        }
        
        return path;
    }
    
    // Dolly (move forward/backward)
    static CameraPath createDolly(const Vec3& start, const Vec3& end, const Vec3& target,
                                   float duration = 3.0f) {
        CameraPath path;
        path.name = "Dolly";
        path.nameCN = "推拉";
        path.duration = duration;
        
        CameraKeyframe kf0;
        kf0.time = 0;
        kf0.position = start;
        kf0.target = target;
        kf0.easeOut = CameraKeyframe::EaseType::EaseInOut;
        path.addKeyframe(kf0);
        
        CameraKeyframe kf1;
        kf1.time = duration;
        kf1.position = end;
        kf1.target = target;
        kf1.easeIn = CameraKeyframe::EaseType::EaseInOut;
        path.addKeyframe(kf1);
        
        return path;
    }
    
    // Truck (move left/right)
    static CameraPath createTruck(const Vec3& start, const Vec3& end,
                                   float duration = 3.0f) {
        CameraPath path;
        path.name = "Truck";
        path.nameCN = "横移";
        path.duration = duration;
        
        // Calculate forward direction
        Vec3 forward = (end - start).normalized();
        Vec3 right = forward.cross(Vec3(0, 1, 0)).normalized();
        
        CameraKeyframe kf0;
        kf0.time = 0;
        kf0.position = start;
        kf0.target = start + forward * 5.0f;
        kf0.easeOut = CameraKeyframe::EaseType::EaseInOut;
        path.addKeyframe(kf0);
        
        CameraKeyframe kf1;
        kf1.time = duration;
        kf1.position = end;
        kf1.target = end + forward * 5.0f;
        kf1.easeIn = CameraKeyframe::EaseType::EaseInOut;
        path.addKeyframe(kf1);
        
        return path;
    }
    
    // Crane (move up/down)
    static CameraPath createCrane(const Vec3& startPos, float startHeight, float endHeight,
                                   const Vec3& target, float duration = 4.0f) {
        CameraPath path;
        path.name = "Crane";
        path.nameCN = "升降";
        path.duration = duration;
        
        CameraKeyframe kf0;
        kf0.time = 0;
        kf0.position = Vec3(startPos.x, startHeight, startPos.z);
        kf0.target = target;
        kf0.easeOut = CameraKeyframe::EaseType::EaseInOut;
        path.addKeyframe(kf0);
        
        CameraKeyframe kf1;
        kf1.time = duration;
        kf1.position = Vec3(startPos.x, endHeight, startPos.z);
        kf1.target = target;
        kf1.easeIn = CameraKeyframe::EaseType::EaseInOut;
        path.addKeyframe(kf1);
        
        return path;
    }
    
    // Zoom (change FOV)
    static CameraPath createZoom(const Vec3& position, const Vec3& target,
                                  float startFov, float endFov, float duration = 2.0f) {
        CameraPath path;
        path.name = "Zoom";
        path.nameCN = "变焦";
        path.duration = duration;
        
        CameraKeyframe kf0;
        kf0.time = 0;
        kf0.position = position;
        kf0.target = target;
        kf0.fov = startFov;
        path.addKeyframe(kf0);
        
        CameraKeyframe kf1;
        kf1.time = duration;
        kf1.position = position;
        kf1.target = target;
        kf1.fov = endFov;
        path.addKeyframe(kf1);
        
        return path;
    }
    
    // Dolly Zoom (Vertigo effect)
    static CameraPath createDollyZoom(const Vec3& target, float startDist, float endDist,
                                       float duration = 3.0f) {
        CameraPath path;
        path.name = "Dolly Zoom";
        path.nameCN = "希区柯克变焦";
        path.duration = duration;
        
        // Calculate FOV to keep subject same size
        float targetSize = 1.0f;  // Assume 1 unit subject
        float startFov = 2.0f * std::atan(targetSize / (2.0f * startDist)) * 180.0f / 3.14159f;
        float endFov = 2.0f * std::atan(targetSize / (2.0f * endDist)) * 180.0f / 3.14159f;
        
        CameraKeyframe kf0;
        kf0.time = 0;
        kf0.position = target + Vec3(0, 0, startDist);
        kf0.target = target;
        kf0.fov = startFov;
        path.addKeyframe(kf0);
        
        CameraKeyframe kf1;
        kf1.time = duration;
        kf1.position = target + Vec3(0, 0, endDist);
        kf1.target = target;
        kf1.fov = endFov;
        path.addKeyframe(kf1);
        
        return path;
    }
    
    // Arc shot
    static CameraPath createArc(const Vec3& center, float radius, float height,
                                 float startAngle, float endAngle, float duration = 4.0f) {
        CameraPath path;
        path.name = "Arc";
        path.nameCN = "弧形移动";
        path.duration = duration;
        
        int numKeyframes = 5;
        for (int i = 0; i <= numKeyframes; i++) {
            float t = static_cast<float>(i) / numKeyframes;
            float angle = startAngle + (endAngle - startAngle) * t;
            
            CameraKeyframe kf;
            kf.time = t * duration;
            kf.position = Vec3(
                center.x + radius * std::cos(angle),
                center.y + height,
                center.z + radius * std::sin(angle)
            );
            kf.target = center;
            kf.easeIn = CameraKeyframe::EaseType::EaseInOut;
            kf.easeOut = CameraKeyframe::EaseType::EaseInOut;
            
            path.addKeyframe(kf);
        }
        
        return path;
    }
    
    // Focus pull (change focus distance)
    static CameraPath createFocusPull(const Vec3& position, const Vec3& target,
                                       float nearFocus, float farFocus, float duration = 2.0f) {
        CameraPath path;
        path.name = "Focus Pull";
        path.nameCN = "焦点转移";
        path.duration = duration;
        
        CameraKeyframe kf0;
        kf0.time = 0;
        kf0.position = position;
        kf0.target = target;
        kf0.focusDistance = nearFocus;
        kf0.dofEnabled = true;
        kf0.aperture = 1.8f;
        path.addKeyframe(kf0);
        
        CameraKeyframe kf1;
        kf1.time = duration;
        kf1.position = position;
        kf1.target = target;
        kf1.focusDistance = farFocus;
        kf1.dofEnabled = true;
        kf1.aperture = 1.8f;
        path.addKeyframe(kf1);
        
        return path;
    }
    
    // Shake effect
    static CameraPath createShake(const Vec3& basePosition, const Vec3& target,
                                   float intensity, float frequency, float duration = 1.0f) {
        CameraPath path;
        path.name = "Shake";
        path.nameCN = "震动";
        path.duration = duration;
        
        int numKeyframes = static_cast<int>(duration * frequency * 2);
        
        for (int i = 0; i <= numKeyframes; i++) {
            float t = static_cast<float>(i) / numKeyframes;
            
            // Random offset that decreases over time
            float decay = 1.0f - t;
            float offsetX = (std::sin(i * 7.3f) * intensity * decay);
            float offsetY = (std::sin(i * 11.7f) * intensity * decay * 0.5f);
            
            CameraKeyframe kf;
            kf.time = t * duration;
            kf.position = basePosition + Vec3(offsetX, offsetY, 0);
            kf.target = target;
            kf.easeIn = CameraKeyframe::EaseType::Linear;
            kf.easeOut = CameraKeyframe::EaseType::Linear;
            
            path.addKeyframe(kf);
        }
        
        return path;
    }
};

// ============================================================================
// Camera Animation Player
// ============================================================================

class CameraAnimationPlayer {
public:
    void setPath(const CameraPath& path) {
        path_ = path;
        currentTime_ = 0;
    }
    
    void play() { isPlaying_ = true; }
    void pause() { isPlaying_ = false; }
    void stop() { isPlaying_ = false; currentTime_ = 0; }
    void togglePlayPause() { isPlaying_ = !isPlaying_; }
    
    void setTime(float time) {
        currentTime_ = std::max(0.0f, time);
        if (path_.loop && path_.duration > 0) {
            currentTime_ = std::fmod(currentTime_, path_.duration);
        } else {
            currentTime_ = std::min(currentTime_, path_.duration);
        }
    }
    
    void setSpeed(float speed) { playbackSpeed_ = speed; }
    void setLoop(bool loop) { path_.loop = loop; }
    
    void update(float deltaTime) {
        if (!isPlaying_) return;
        
        currentTime_ += deltaTime * playbackSpeed_;
        
        if (currentTime_ > path_.duration) {
            if (path_.loop) {
                currentTime_ = std::fmod(currentTime_, path_.duration);
            } else {
                currentTime_ = path_.duration;
                isPlaying_ = false;
                if (onComplete_) onComplete_();
            }
        }
        
        // Interpolate current frame
        currentFrame_ = CameraInterpolator::interpolate(path_, currentTime_);
        
        // Notify
        if (onFrameUpdate_) {
            onFrameUpdate_(currentFrame_);
        }
    }
    
    bool isPlaying() const { return isPlaying_; }
    float getCurrentTime() const { return currentTime_; }
    float getDuration() const { return path_.duration; }
    const CameraKeyframe& getCurrentFrame() const { return currentFrame_; }
    CameraPath& getPath() { return path_; }
    
    void setOnFrameUpdate(std::function<void(const CameraKeyframe&)> callback) {
        onFrameUpdate_ = callback;
    }
    
    void setOnComplete(std::function<void()> callback) {
        onComplete_ = callback;
    }
    
private:
    CameraPath path_;
    CameraKeyframe currentFrame_;
    
    float currentTime_ = 0;
    float playbackSpeed_ = 1.0f;
    bool isPlaying_ = false;
    
    std::function<void(const CameraKeyframe&)> onFrameUpdate_;
    std::function<void()> onComplete_;
};

// ============================================================================
// Camera Animation Manager
// ============================================================================

class CameraAnimationManager {
public:
    static CameraAnimationManager& getInstance() {
        static CameraAnimationManager instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // Register built-in paths
        paths_["orbit"] = CameraPresets::createOrbit(Vec3(0, 1, 0), 5, 2, 8);
        paths_["orbit"].name = "Orbit";
        paths_["orbit"].nameCN = "环绕";
        
        paths_["zoom_in"] = CameraPresets::createZoom(Vec3(0, 1.5f, 5), Vec3(0, 1, 0), 45, 20, 3);
        paths_["zoom_in"].name = "Zoom In";
        paths_["zoom_in"].nameCN = "推进";
        
        paths_["zoom_out"] = CameraPresets::createZoom(Vec3(0, 1.5f, 5), Vec3(0, 1, 0), 45, 70, 3);
        paths_["zoom_out"].name = "Zoom Out";
        paths_["zoom_out"].nameCN = "拉远";
        
        paths_["crane_up"] = CameraPresets::createCrane(Vec3(0, 0, 4), 1, 4, Vec3(0, 1, 0), 4);
        paths_["crane_up"].name = "Crane Up";
        paths_["crane_up"].nameCN = "升起";
        
        paths_["crane_down"] = CameraPresets::createCrane(Vec3(0, 0, 4), 4, 1, Vec3(0, 1, 0), 4);
        paths_["crane_down"].name = "Crane Down";
        paths_["crane_down"].nameCN = "下降";
        
        paths_["dolly_in"] = CameraPresets::createDolly(Vec3(0, 1.5f, 8), Vec3(0, 1.5f, 3), Vec3(0, 1, 0), 4);
        paths_["dolly_in"].name = "Dolly In";
        paths_["dolly_in"].nameCN = "推进";
        
        paths_["arc_left"] = CameraPresets::createArc(Vec3(0, 1, 0), 5, 1.5f, 0, 1.57f, 5);
        paths_["arc_left"].name = "Arc Left";
        paths_["arc_left"].nameCN = "左弧";
        
        paths_["arc_right"] = CameraPresets::createArc(Vec3(0, 1, 0), 5, 1.5f, 0, -1.57f, 5);
        paths_["arc_right"].name = "Arc Right";
        paths_["arc_right"].nameCN = "右弧";
        
        paths_["shake"] = CameraPresets::createShake(Vec3(0, 1.5f, 5), Vec3(0, 1, 0), 0.1f, 10, 1);
        paths_["shake"].name = "Shake";
        paths_["shake"].nameCN = "震动";
        
        paths_["dolly_zoom"] = CameraPresets::createDollyZoom(Vec3(0, 1, 0), 8, 3, 4);
        paths_["dolly_zoom"].name = "Dolly Zoom";
        paths_["dolly_zoom"].nameCN = "眩晕变焦";
        
        initialized_ = true;
    }
    
    // Get player
    CameraAnimationPlayer& getPlayer() { return player_; }
    
    // Path management
    void addPath(const std::string& id, const CameraPath& path) {
        paths_[id] = path;
    }
    
    CameraPath* getPath(const std::string& id) {
        auto it = paths_.find(id);
        return (it != paths_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getPathIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : paths_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    const std::unordered_map<std::string, CameraPath>& getAllPaths() const {
        return paths_;
    }
    
    // Play a preset
    void playPath(const std::string& id) {
        if (CameraPath* path = getPath(id)) {
            player_.setPath(*path);
            player_.play();
        }
    }
    
    // Create new path
    CameraPath createEmptyPath(const std::string& name) {
        CameraPath path;
        path.name = name;
        return path;
    }
    
private:
    CameraAnimationManager() = default;
    
    std::unordered_map<std::string, CameraPath> paths_;
    CameraAnimationPlayer player_;
    bool initialized_ = false;
};

// ============================================================================
// Convenience
// ============================================================================

inline CameraAnimationManager& getCameraAnimation() {
    return CameraAnimationManager::getInstance();
}

}  // namespace luma
