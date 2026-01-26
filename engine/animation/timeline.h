// Timeline Editor System
// Multi-track timeline with keyframe editing and curve support
#pragma once

#include "animation_clip.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>

namespace luma {

// ===== Curve Types =====
enum class CurveInterpolation {
    Constant,   // Step interpolation
    Linear,     // Linear interpolation
    Bezier,     // Cubic Bezier curves
    Hermite,    // Hermite spline
    Auto        // Auto-computed tangents
};

// ===== Bezier Tangent =====
struct BezierTangent {
    float time = 0.0f;    // Time offset from keyframe
    float value = 0.0f;   // Value offset from keyframe
    bool broken = false;  // If true, in/out tangents are independent
    
    BezierTangent() = default;
    BezierTangent(float t, float v) : time(t), value(v) {}
};

// ===== Curve Keyframe =====
template<typename T>
struct CurveKeyframe {
    float time = 0.0f;
    T value{};
    CurveInterpolation interpolation = CurveInterpolation::Bezier;
    
    // Tangent handles (for Bezier)
    BezierTangent inTangent;
    BezierTangent outTangent;
    
    // Weight for weighted tangents
    float inWeight = 1.0f;
    float outWeight = 1.0f;
    
    CurveKeyframe() = default;
    CurveKeyframe(float t, const T& v) : time(t), value(v) {}
};

// ===== Animation Curve =====
// Editable curve with various interpolation modes
template<typename T>
class AnimationCurve {
public:
    std::string name;
    std::vector<CurveKeyframe<T>> keyframes;
    
    // Default value when no keyframes
    T defaultValue{};
    
    // Add keyframe
    int addKeyframe(float time, const T& value) {
        CurveKeyframe<T> key(time, value);
        
        // Find insertion point
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), key,
            [](const CurveKeyframe<T>& a, const CurveKeyframe<T>& b) {
                return a.time < b.time;
            });
        
        // Check if keyframe exists at this time
        if (it != keyframes.end() && std::abs(it->time - time) < 0.0001f) {
            it->value = value;
            return static_cast<int>(it - keyframes.begin());
        }
        
        // Insert new keyframe
        it = keyframes.insert(it, key);
        
        // Auto-compute tangents if needed
        int index = static_cast<int>(it - keyframes.begin());
        autoComputeTangent(index);
        
        return index;
    }
    
    // Remove keyframe by index
    void removeKeyframe(int index) {
        if (index >= 0 && index < static_cast<int>(keyframes.size())) {
            keyframes.erase(keyframes.begin() + index);
        }
    }
    
    // Get keyframe count
    int getKeyframeCount() const { return static_cast<int>(keyframes.size()); }
    
    // Get keyframe by index
    CurveKeyframe<T>* getKeyframe(int index) {
        if (index >= 0 && index < static_cast<int>(keyframes.size())) {
            return &keyframes[index];
        }
        return nullptr;
    }
    
    // Find keyframe at time
    int findKeyframe(float time, float tolerance = 0.001f) const {
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (std::abs(keyframes[i].time - time) < tolerance) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    // Evaluate curve at time
    T evaluate(float time) const {
        if (keyframes.empty()) return defaultValue;
        if (keyframes.size() == 1) return keyframes[0].value;
        
        // Before first keyframe
        if (time <= keyframes.front().time) {
            return keyframes.front().value;
        }
        
        // After last keyframe
        if (time >= keyframes.back().time) {
            return keyframes.back().value;
        }
        
        // Find surrounding keyframes
        size_t nextIdx = 0;
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (keyframes[i].time > time) {
                nextIdx = i;
                break;
            }
        }
        
        size_t prevIdx = nextIdx - 1;
        const auto& prev = keyframes[prevIdx];
        const auto& next = keyframes[nextIdx];
        
        float t = (time - prev.time) / (next.time - prev.time);
        
        // Interpolate based on mode
        switch (prev.interpolation) {
            case CurveInterpolation::Constant:
                return prev.value;
                
            case CurveInterpolation::Linear:
                return interpolateLinear(prev.value, next.value, t);
                
            case CurveInterpolation::Bezier:
            case CurveInterpolation::Hermite:
            case CurveInterpolation::Auto:
                return interpolateBezier(prev, next, t);
                
            default:
                return interpolateLinear(prev.value, next.value, t);
        }
    }
    
    // Auto-compute tangent for keyframe
    void autoComputeTangent(int index) {
        if (index < 0 || index >= static_cast<int>(keyframes.size())) return;
        
        auto& key = keyframes[index];
        if (key.interpolation != CurveInterpolation::Auto &&
            key.interpolation != CurveInterpolation::Bezier) return;
        
        // Catmull-Rom style auto tangents
        if (keyframes.size() < 2) return;
        
        float slope = 0.0f;
        
        if (index == 0) {
            // First keyframe - use forward difference
            slope = getSlope(keyframes[0], keyframes[1]);
        } else if (index == static_cast<int>(keyframes.size()) - 1) {
            // Last keyframe - use backward difference
            slope = getSlope(keyframes[index - 1], keyframes[index]);
        } else {
            // Middle keyframe - average of adjacent slopes
            float slopeBefore = getSlope(keyframes[index - 1], keyframes[index]);
            float slopeAfter = getSlope(keyframes[index], keyframes[index + 1]);
            slope = (slopeBefore + slopeAfter) * 0.5f;
        }
        
        // Set tangent lengths (1/3 of segment length is typical)
        float inLength = (index > 0) ? 
            (key.time - keyframes[index - 1].time) * 0.333f : 0.0f;
        float outLength = (index < static_cast<int>(keyframes.size()) - 1) ? 
            (keyframes[index + 1].time - key.time) * 0.333f : 0.0f;
        
        key.inTangent = BezierTangent(-inLength, -inLength * slope);
        key.outTangent = BezierTangent(outLength, outLength * slope);
    }
    
    // Compute all tangents
    void autoComputeAllTangents() {
        for (int i = 0; i < static_cast<int>(keyframes.size()); i++) {
            autoComputeTangent(i);
        }
    }
    
private:
    // Get slope between two keyframes (for float values)
    float getSlope(const CurveKeyframe<T>& a, const CurveKeyframe<T>& b) const;
    
    // Linear interpolation
    T interpolateLinear(const T& a, const T& b, float t) const;
    
    // Bezier interpolation
    T interpolateBezier(const CurveKeyframe<T>& a, const CurveKeyframe<T>& b, float t) const;
};

// Template specializations for common types
template<>
inline float AnimationCurve<float>::getSlope(const CurveKeyframe<float>& a, 
                                              const CurveKeyframe<float>& b) const {
    float dt = b.time - a.time;
    if (std::abs(dt) < 0.0001f) return 0.0f;
    return (b.value - a.value) / dt;
}

template<>
inline float AnimationCurve<float>::interpolateLinear(const float& a, const float& b, float t) const {
    return a + (b - a) * t;
}

template<>
inline float AnimationCurve<float>::interpolateBezier(const CurveKeyframe<float>& a,
                                                       const CurveKeyframe<float>& b,
                                                       float t) const {
    // Cubic Bezier interpolation
    float p0 = a.value;
    float p1 = a.value + a.outTangent.value;
    float p2 = b.value + b.inTangent.value;
    float p3 = b.value;
    
    float oneMinusT = 1.0f - t;
    float oneMinusT2 = oneMinusT * oneMinusT;
    float oneMinusT3 = oneMinusT2 * oneMinusT;
    float t2 = t * t;
    float t3 = t2 * t;
    
    return oneMinusT3 * p0 + 
           3.0f * oneMinusT2 * t * p1 + 
           3.0f * oneMinusT * t2 * p2 + 
           t3 * p3;
}

// Vec3 specializations
template<>
inline float AnimationCurve<Vec3>::getSlope(const CurveKeyframe<Vec3>& a,
                                             const CurveKeyframe<Vec3>& b) const {
    // Use magnitude of change
    float dt = b.time - a.time;
    if (std::abs(dt) < 0.0001f) return 0.0f;
    Vec3 delta = b.value - a.value;
    return delta.length() / dt;
}

template<>
inline Vec3 AnimationCurve<Vec3>::interpolateLinear(const Vec3& a, const Vec3& b, float t) const {
    return Vec3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

template<>
inline Vec3 AnimationCurve<Vec3>::interpolateBezier(const CurveKeyframe<Vec3>& a,
                                                     const CurveKeyframe<Vec3>& b,
                                                     float t) const {
    // Simple linear for Vec3 (proper Bezier would need per-component tangents)
    return interpolateLinear(a.value, b.value, t);
}

// ===== Animation Event =====
struct AnimationEvent {
    float time = 0.0f;
    std::string name;
    std::string parameter;  // Optional parameter string
    
    AnimationEvent() = default;
    AnimationEvent(float t, const std::string& n) : time(t), name(n) {}
};

// ===== Timeline Track =====
enum class TrackType {
    Transform,
    Float,
    Event,
    Audio,
    Activation  // Enable/disable objects
};

class TimelineTrack {
public:
    std::string name;
    TrackType type = TrackType::Float;
    bool muted = false;
    bool locked = false;
    bool expanded = true;
    
    // Target path (e.g., "character/arm_l/hand")
    std::string targetPath;
    std::string propertyName;  // e.g., "position", "rotation", "scale"
    
    // Color for visualization
    float color[3] = {0.5f, 0.7f, 1.0f};
    
    // Curves (depending on type)
    AnimationCurve<float> floatCurve;
    AnimationCurve<Vec3> positionCurve;
    AnimationCurve<Vec3> scaleCurve;
    AnimationCurve<Quat> rotationCurve;
    
    // Events
    std::vector<AnimationEvent> events;
    
    // Get duration
    float getDuration() const {
        float duration = 0.0f;
        
        if (!floatCurve.keyframes.empty()) {
            duration = std::max(duration, floatCurve.keyframes.back().time);
        }
        if (!positionCurve.keyframes.empty()) {
            duration = std::max(duration, positionCurve.keyframes.back().time);
        }
        if (!scaleCurve.keyframes.empty()) {
            duration = std::max(duration, scaleCurve.keyframes.back().time);
        }
        if (!rotationCurve.keyframes.empty()) {
            duration = std::max(duration, rotationCurve.keyframes.back().time);
        }
        for (const auto& evt : events) {
            duration = std::max(duration, evt.time);
        }
        
        return duration;
    }
    
    // Add event
    void addEvent(float time, const std::string& name) {
        events.emplace_back(time, name);
        std::sort(events.begin(), events.end(),
            [](const AnimationEvent& a, const AnimationEvent& b) {
                return a.time < b.time;
            });
    }
};

// ===== Timeline =====
class Timeline {
public:
    std::string name = "Timeline";
    std::vector<std::unique_ptr<TimelineTrack>> tracks;
    
    // Playback settings
    float duration = 5.0f;
    float frameRate = 30.0f;
    bool loop = false;
    
    // Current state
    float currentTime = 0.0f;
    bool playing = false;
    float playbackSpeed = 1.0f;
    
    // Selection
    int selectedTrack = -1;
    std::vector<std::pair<int, int>> selectedKeyframes;  // (trackIndex, keyIndex)
    
    // Markers
    struct Marker {
        float time;
        std::string name;
        float color[3];
    };
    std::vector<Marker> markers;
    
    // Create track
    TimelineTrack* createTrack(const std::string& name, TrackType type) {
        auto track = std::make_unique<TimelineTrack>();
        track->name = name;
        track->type = type;
        tracks.push_back(std::move(track));
        return tracks.back().get();
    }
    
    // Get track by name
    TimelineTrack* getTrack(const std::string& name) {
        for (auto& track : tracks) {
            if (track->name == name) return track.get();
        }
        return nullptr;
    }
    
    // Remove track
    void removeTrack(int index) {
        if (index >= 0 && index < static_cast<int>(tracks.size())) {
            tracks.erase(tracks.begin() + index);
        }
    }
    
    // Update timeline
    void update(float deltaTime) {
        if (!playing) return;
        
        currentTime += deltaTime * playbackSpeed;
        
        if (currentTime >= duration) {
            if (loop) {
                currentTime = std::fmod(currentTime, duration);
            } else {
                currentTime = duration;
                playing = false;
            }
        }
        
        if (currentTime < 0.0f) {
            if (loop) {
                currentTime = duration + std::fmod(currentTime, duration);
            } else {
                currentTime = 0.0f;
                playing = false;
            }
        }
    }
    
    // Playback controls
    void play() { playing = true; }
    void pause() { playing = false; }
    void stop() { playing = false; currentTime = 0.0f; }
    
    void setTime(float time) {
        currentTime = std::clamp(time, 0.0f, duration);
    }
    
    // Frame stepping
    void nextFrame() {
        float frameTime = 1.0f / frameRate;
        currentTime = std::min(currentTime + frameTime, duration);
    }
    
    void prevFrame() {
        float frameTime = 1.0f / frameRate;
        currentTime = std::max(currentTime - frameTime, 0.0f);
    }
    
    // Jump to marker
    void gotoMarker(const std::string& name) {
        for (const auto& marker : markers) {
            if (marker.name == name) {
                currentTime = marker.time;
                return;
            }
        }
    }
    
    // Add marker
    void addMarker(float time, const std::string& name) {
        Marker m;
        m.time = time;
        m.name = name;
        m.color[0] = 1.0f; m.color[1] = 0.8f; m.color[2] = 0.0f;
        markers.push_back(m);
    }
    
    // Compute total duration from tracks
    void computeDuration() {
        duration = 0.0f;
        for (const auto& track : tracks) {
            duration = std::max(duration, track->getDuration());
        }
        duration = std::max(duration, 1.0f);  // Minimum 1 second
    }
    
    // === Keyframe operations ===
    
    // Copy selected keyframes
    struct KeyframeCopy {
        int trackIndex;
        float time;
        float floatValue;
        Vec3 vec3Value;
    };
    std::vector<KeyframeCopy> clipboard;
    
    void copySelectedKeyframes() {
        clipboard.clear();
        for (const auto& [trackIdx, keyIdx] : selectedKeyframes) {
            if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks.size())) continue;
            
            auto& track = tracks[trackIdx];
            KeyframeCopy copy;
            copy.trackIndex = trackIdx;
            
            if (!track->floatCurve.keyframes.empty() && 
                keyIdx < static_cast<int>(track->floatCurve.keyframes.size())) {
                copy.time = track->floatCurve.keyframes[keyIdx].time;
                copy.floatValue = track->floatCurve.keyframes[keyIdx].value;
            }
            
            clipboard.push_back(copy);
        }
    }
    
    void pasteKeyframes(float timeOffset) {
        for (const auto& copy : clipboard) {
            if (copy.trackIndex < 0 || copy.trackIndex >= static_cast<int>(tracks.size())) continue;
            
            auto& track = tracks[copy.trackIndex];
            track->floatCurve.addKeyframe(copy.time + timeOffset, copy.floatValue);
        }
    }
    
    void deleteSelectedKeyframes() {
        // Sort by index descending so deletion doesn't invalidate indices
        std::sort(selectedKeyframes.begin(), selectedKeyframes.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
        
        for (const auto& [trackIdx, keyIdx] : selectedKeyframes) {
            if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks.size())) continue;
            tracks[trackIdx]->floatCurve.removeKeyframe(keyIdx);
        }
        
        selectedKeyframes.clear();
    }
    
    // === Snapping ===
    
    float snapToFrame(float time) const {
        float frameTime = 1.0f / frameRate;
        return std::round(time / frameTime) * frameTime;
    }
    
    float snapToMarker(float time, float tolerance = 0.1f) const {
        for (const auto& marker : markers) {
            if (std::abs(marker.time - time) < tolerance) {
                return marker.time;
            }
        }
        return time;
    }
};

// ===== Timeline Manager =====
// Manages multiple timelines
class TimelineManager {
public:
    std::vector<std::unique_ptr<Timeline>> timelines;
    int activeTimelineIndex = 0;
    
    Timeline* createTimeline(const std::string& name) {
        auto timeline = std::make_unique<Timeline>();
        timeline->name = name;
        timelines.push_back(std::move(timeline));
        return timelines.back().get();
    }
    
    Timeline* getActiveTimeline() {
        if (activeTimelineIndex >= 0 && 
            activeTimelineIndex < static_cast<int>(timelines.size())) {
            return timelines[activeTimelineIndex].get();
        }
        return nullptr;
    }
    
    void setActiveTimeline(int index) {
        if (index >= 0 && index < static_cast<int>(timelines.size())) {
            activeTimelineIndex = index;
        }
    }
    
    void update(float deltaTime) {
        if (auto* timeline = getActiveTimeline()) {
            timeline->update(deltaTime);
        }
    }
};

// ===== Curve Editor State (for UI) =====
struct CurveEditorState {
    // View transform
    float viewOffsetX = 0.0f;
    float viewOffsetY = 0.0f;
    float zoomX = 1.0f;
    float zoomY = 1.0f;
    
    // Grid settings
    bool showGrid = true;
    float gridSpacingX = 1.0f;
    float gridSpacingY = 0.1f;
    
    // Selection
    int selectedCurve = 0;
    int selectedKeyframe = -1;
    bool editingTangent = false;
    bool editingInTangent = false;
    
    // Dragging
    bool isDragging = false;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;
    
    // View bounds
    float minTime = 0.0f;
    float maxTime = 5.0f;
    float minValue = -1.0f;
    float maxValue = 1.0f;
    
    // Frame to screen coordinates
    float timeToScreen(float time, float width) const {
        return (time - viewOffsetX) * zoomX * (width / (maxTime - minTime));
    }
    
    float valueToScreen(float value, float height) const {
        return height - (value - viewOffsetY) * zoomY * (height / (maxValue - minValue));
    }
    
    // Screen to frame coordinates
    float screenToTime(float x, float width) const {
        return x / (zoomX * (width / (maxTime - minTime))) + viewOffsetX;
    }
    
    float screenToValue(float y, float height) const {
        return (height - y) / (zoomY * (height / (maxValue - minValue))) + viewOffsetY;
    }
    
    // Fit view to curve data
    void fitToData(const AnimationCurve<float>& curve) {
        if (curve.keyframes.empty()) return;
        
        minTime = curve.keyframes.front().time;
        maxTime = curve.keyframes.back().time;
        
        minValue = curve.keyframes[0].value;
        maxValue = curve.keyframes[0].value;
        
        for (const auto& key : curve.keyframes) {
            minValue = std::min(minValue, key.value);
            maxValue = std::max(maxValue, key.value);
        }
        
        // Add padding
        float timePad = (maxTime - minTime) * 0.1f;
        float valuePad = (maxValue - minValue) * 0.1f;
        
        minTime -= timePad;
        maxTime += timePad;
        minValue -= valuePad;
        maxValue += valuePad;
        
        // Ensure minimum range
        if (maxTime - minTime < 1.0f) {
            maxTime = minTime + 1.0f;
        }
        if (maxValue - minValue < 0.1f) {
            maxValue = minValue + 0.1f;
        }
        
        viewOffsetX = minTime;
        viewOffsetY = minValue;
        zoomX = 1.0f;
        zoomY = 1.0f;
    }
};

}  // namespace luma
