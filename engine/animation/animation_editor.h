// Animation Editor - Keyframe-based animation editing
// Full-featured animation editor with curves and timeline
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace luma {

// ============================================================================
// Interpolation Types
// ============================================================================

enum class InterpolationType {
    Constant,       // Step/hold - no interpolation
    Linear,         // Linear interpolation
    Bezier,         // Cubic Bezier
    Hermite,        // Hermite spline
    CatmullRom,     // Catmull-Rom spline
    EaseIn,         // Slow start
    EaseOut,        // Slow end
    EaseInOut,      // Slow start and end
    Bounce,         // Bounce effect
    Elastic,        // Elastic/spring effect
    Back            // Overshoot
};

inline std::string interpolationTypeToString(InterpolationType type) {
    switch (type) {
        case InterpolationType::Constant: return "Constant";
        case InterpolationType::Linear: return "Linear";
        case InterpolationType::Bezier: return "Bezier";
        case InterpolationType::Hermite: return "Hermite";
        case InterpolationType::CatmullRom: return "CatmullRom";
        case InterpolationType::EaseIn: return "EaseIn";
        case InterpolationType::EaseOut: return "EaseOut";
        case InterpolationType::EaseInOut: return "EaseInOut";
        case InterpolationType::Bounce: return "Bounce";
        case InterpolationType::Elastic: return "Elastic";
        case InterpolationType::Back: return "Back";
        default: return "Unknown";
    }
}

// ============================================================================
// Easing Functions
// ============================================================================

class EasingFunctions {
public:
    static float apply(InterpolationType type, float t) {
        switch (type) {
            case InterpolationType::Constant: return 0.0f;
            case InterpolationType::Linear: return t;
            case InterpolationType::EaseIn: return easeInQuad(t);
            case InterpolationType::EaseOut: return easeOutQuad(t);
            case InterpolationType::EaseInOut: return easeInOutQuad(t);
            case InterpolationType::Bounce: return easeOutBounce(t);
            case InterpolationType::Elastic: return easeOutElastic(t);
            case InterpolationType::Back: return easeOutBack(t);
            default: return t;
        }
    }
    
private:
    static float easeInQuad(float t) { return t * t; }
    static float easeOutQuad(float t) { return t * (2.0f - t); }
    static float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    
    static float easeOutBounce(float t) {
        if (t < 1.0f / 2.75f) {
            return 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }
    
    static float easeOutElastic(float t) {
        if (t == 0 || t == 1) return t;
        float p = 0.3f;
        float s = p / 4.0f;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - s) * 6.283185f / p) + 1.0f;
    }
    
    static float easeOutBack(float t) {
        float c = 1.70158f;
        return 1.0f + (c + 1.0f) * std::pow(t - 1.0f, 3.0f) + c * std::pow(t - 1.0f, 2.0f);
    }
};

// ============================================================================
// Bezier Handle
// ============================================================================

struct BezierHandle {
    Vec2 position{0, 0};    // Position relative to keyframe
    bool broken = false;    // True if handles are independent
};

// ============================================================================
// Keyframe - Single animation keyframe
// ============================================================================

template<typename T>
struct Keyframe {
    float time = 0;
    T value{};
    
    InterpolationType interpolation = InterpolationType::Linear;
    
    // Bezier handles (in tangent, out tangent)
    BezierHandle inHandle;
    BezierHandle outHandle;
    
    // For tangent-based curves
    T inTangent{};
    T outTangent{};
    
    bool selected = false;
    bool locked = false;
};

// ============================================================================
// Animation Track - Channel of animated values
// ============================================================================

enum class TrackType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Transform
};

template<typename T>
class AnimationTrack {
public:
    std::string name;
    std::string targetPath;     // e.g., "Hips/rotation" or "BlendShapes/smile"
    TrackType type;
    bool muted = false;
    bool locked = false;
    
    std::vector<Keyframe<T>> keyframes;
    
    // Add keyframe at time
    void addKeyframe(float time, const T& value, 
                     InterpolationType interp = InterpolationType::Linear) {
        Keyframe<T> kf;
        kf.time = time;
        kf.value = value;
        kf.interpolation = interp;
        
        // Insert sorted by time
        auto it = keyframes.begin();
        while (it != keyframes.end() && it->time < time) {
            ++it;
        }
        
        // Replace if same time exists
        if (it != keyframes.end() && std::abs(it->time - time) < 0.0001f) {
            *it = kf;
        } else {
            keyframes.insert(it, kf);
        }
        
        computeTangents();
    }
    
    // Remove keyframe at index
    void removeKeyframe(int index) {
        if (index >= 0 && index < static_cast<int>(keyframes.size())) {
            keyframes.erase(keyframes.begin() + index);
            computeTangents();
        }
    }
    
    // Remove keyframes in time range
    void removeKeyframesInRange(float startTime, float endTime) {
        keyframes.erase(
            std::remove_if(keyframes.begin(), keyframes.end(),
                [startTime, endTime](const Keyframe<T>& kf) {
                    return kf.time >= startTime && kf.time <= endTime;
                }),
            keyframes.end());
        computeTangents();
    }
    
    // Find keyframe index at time
    int findKeyframeAt(float time, float tolerance = 0.01f) const {
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (std::abs(keyframes[i].time - time) < tolerance) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    // Evaluate track at time
    T evaluate(float time) const {
        if (keyframes.empty()) return T{};
        if (keyframes.size() == 1) return keyframes[0].value;
        
        // Find surrounding keyframes
        if (time <= keyframes.front().time) return keyframes.front().value;
        if (time >= keyframes.back().time) return keyframes.back().value;
        
        int i = 0;
        while (i < static_cast<int>(keyframes.size()) - 1 && 
               keyframes[i + 1].time < time) {
            i++;
        }
        
        const Keyframe<T>& k0 = keyframes[i];
        const Keyframe<T>& k1 = keyframes[i + 1];
        
        float t = (time - k0.time) / (k1.time - k0.time);
        
        return interpolate(k0, k1, t);
    }
    
    // Get duration
    float getDuration() const {
        if (keyframes.empty()) return 0;
        return keyframes.back().time;
    }
    
    // Compute auto tangents
    void computeTangents() {
        for (size_t i = 0; i < keyframes.size(); i++) {
            auto& kf = keyframes[i];
            
            if (i == 0) {
                if (keyframes.size() > 1) {
                    kf.outTangent = computeTangent(kf.value, keyframes[i + 1].value);
                }
                kf.inTangent = kf.outTangent;
            } else if (i == keyframes.size() - 1) {
                kf.inTangent = computeTangent(keyframes[i - 1].value, kf.value);
                kf.outTangent = kf.inTangent;
            } else {
                // Catmull-Rom style tangents
                kf.inTangent = computeTangent(keyframes[i - 1].value, keyframes[i + 1].value);
                kf.outTangent = kf.inTangent;
            }
        }
    }
    
private:
    T interpolate(const Keyframe<T>& k0, const Keyframe<T>& k1, float t) const;
    T computeTangent(const T& v0, const T& v1) const;
};

// Specializations for different types
template<>
inline float AnimationTrack<float>::interpolate(const Keyframe<float>& k0,
                                                 const Keyframe<float>& k1,
                                                 float t) const {
    t = EasingFunctions::apply(k0.interpolation, t);
    
    switch (k0.interpolation) {
        case InterpolationType::Constant:
            return k0.value;
        case InterpolationType::Hermite:
        case InterpolationType::CatmullRom: {
            float t2 = t * t;
            float t3 = t2 * t;
            float h00 = 2*t3 - 3*t2 + 1;
            float h10 = t3 - 2*t2 + t;
            float h01 = -2*t3 + 3*t2;
            float h11 = t3 - t2;
            float dt = k1.time - k0.time;
            return h00 * k0.value + h10 * dt * k0.outTangent +
                   h01 * k1.value + h11 * dt * k1.inTangent;
        }
        default:
            return k0.value * (1.0f - t) + k1.value * t;
    }
}

template<>
inline float AnimationTrack<float>::computeTangent(const float& v0, const float& v1) const {
    return (v1 - v0) * 0.5f;
}

template<>
inline Vec3 AnimationTrack<Vec3>::interpolate(const Keyframe<Vec3>& k0,
                                               const Keyframe<Vec3>& k1,
                                               float t) const {
    t = EasingFunctions::apply(k0.interpolation, t);
    
    if (k0.interpolation == InterpolationType::Constant) {
        return k0.value;
    }
    
    return k0.value * (1.0f - t) + k1.value * t;
}

template<>
inline Vec3 AnimationTrack<Vec3>::computeTangent(const Vec3& v0, const Vec3& v1) const {
    return (v1 - v0) * 0.5f;
}

template<>
inline Quat AnimationTrack<Quat>::interpolate(const Keyframe<Quat>& k0,
                                               const Keyframe<Quat>& k1,
                                               float t) const {
    t = EasingFunctions::apply(k0.interpolation, t);
    
    if (k0.interpolation == InterpolationType::Constant) {
        return k0.value;
    }
    
    // Spherical linear interpolation for quaternions
    return k0.value.slerp(k1.value, t);
}

template<>
inline Quat AnimationTrack<Quat>::computeTangent(const Quat& v0, const Quat& v1) const {
    return Quat::identity();  // Not used for quaternions
}

// ============================================================================
// Animation Clip - Complete animation
// ============================================================================

struct AnimationClip {
    std::string name;
    std::string nameCN;
    float duration = 0;
    float frameRate = 30.0f;
    bool loop = false;
    
    // Tracks by bone/target name
    std::unordered_map<std::string, AnimationTrack<Vec3>> positionTracks;
    std::unordered_map<std::string, AnimationTrack<Quat>> rotationTracks;
    std::unordered_map<std::string, AnimationTrack<Vec3>> scaleTracks;
    std::unordered_map<std::string, AnimationTrack<float>> blendShapeTracks;
    
    // Events at specific times
    struct AnimationEvent {
        float time;
        std::string name;
        std::string parameter;
    };
    std::vector<AnimationEvent> events;
    
    // Add position keyframe
    void addPositionKey(const std::string& bone, float time, const Vec3& pos,
                        InterpolationType interp = InterpolationType::Linear) {
        positionTracks[bone].name = bone + ".position";
        positionTracks[bone].targetPath = bone + "/position";
        positionTracks[bone].type = TrackType::Vec3;
        positionTracks[bone].addKeyframe(time, pos, interp);
        updateDuration();
    }
    
    // Add rotation keyframe
    void addRotationKey(const std::string& bone, float time, const Quat& rot,
                        InterpolationType interp = InterpolationType::Linear) {
        rotationTracks[bone].name = bone + ".rotation";
        rotationTracks[bone].targetPath = bone + "/rotation";
        rotationTracks[bone].type = TrackType::Quat;
        rotationTracks[bone].addKeyframe(time, rot, interp);
        updateDuration();
    }
    
    // Add scale keyframe
    void addScaleKey(const std::string& bone, float time, const Vec3& scale,
                     InterpolationType interp = InterpolationType::Linear) {
        scaleTracks[bone].name = bone + ".scale";
        scaleTracks[bone].targetPath = bone + "/scale";
        scaleTracks[bone].type = TrackType::Vec3;
        scaleTracks[bone].addKeyframe(time, scale, interp);
        updateDuration();
    }
    
    // Add blend shape keyframe
    void addBlendShapeKey(const std::string& shape, float time, float weight,
                          InterpolationType interp = InterpolationType::Linear) {
        blendShapeTracks[shape].name = shape;
        blendShapeTracks[shape].targetPath = "BlendShapes/" + shape;
        blendShapeTracks[shape].type = TrackType::Float;
        blendShapeTracks[shape].addKeyframe(time, weight, interp);
        updateDuration();
    }
    
    // Add event
    void addEvent(float time, const std::string& eventName, 
                  const std::string& param = "") {
        events.push_back({time, eventName, param});
        std::sort(events.begin(), events.end(),
            [](const AnimationEvent& a, const AnimationEvent& b) {
                return a.time < b.time;
            });
    }
    
    // Sample animation at time
    void sample(float time, Skeleton* skeleton,
                std::unordered_map<std::string, float>* blendShapeWeights = nullptr) const {
        if (!skeleton) return;
        
        // Handle looping
        if (loop && duration > 0) {
            time = std::fmod(time, duration);
        }
        
        // Apply bone transforms
        for (const auto& [boneName, track] : positionTracks) {
            if (track.muted) continue;
            int boneIdx = skeleton->findBoneByName(boneName);
            if (boneIdx >= 0) {
                Bone* bone = skeleton->getBone(boneIdx);
                bone->localPosition = track.evaluate(time);
            }
        }
        
        for (const auto& [boneName, track] : rotationTracks) {
            if (track.muted) continue;
            int boneIdx = skeleton->findBoneByName(boneName);
            if (boneIdx >= 0) {
                Bone* bone = skeleton->getBone(boneIdx);
                bone->localRotation = track.evaluate(time);
            }
        }
        
        for (const auto& [boneName, track] : scaleTracks) {
            if (track.muted) continue;
            int boneIdx = skeleton->findBoneByName(boneName);
            if (boneIdx >= 0) {
                Bone* bone = skeleton->getBone(boneIdx);
                bone->localScale = track.evaluate(time);
            }
        }
        
        skeleton->updateMatrices();
        
        // Apply blend shapes
        if (blendShapeWeights) {
            for (const auto& [shapeName, track] : blendShapeTracks) {
                if (track.muted) continue;
                (*blendShapeWeights)[shapeName] = track.evaluate(time);
            }
        }
    }
    
    // Get events at time
    std::vector<AnimationEvent> getEventsAt(float time, float tolerance = 0.016f) const {
        std::vector<AnimationEvent> result;
        for (const auto& event : events) {
            if (std::abs(event.time - time) < tolerance) {
                result.push_back(event);
            }
        }
        return result;
    }
    
private:
    void updateDuration() {
        duration = 0;
        for (const auto& [_, track] : positionTracks) {
            duration = std::max(duration, track.getDuration());
        }
        for (const auto& [_, track] : rotationTracks) {
            duration = std::max(duration, track.getDuration());
        }
        for (const auto& [_, track] : scaleTracks) {
            duration = std::max(duration, track.getDuration());
        }
        for (const auto& [_, track] : blendShapeTracks) {
            duration = std::max(duration, track.getDuration());
        }
    }
};

// ============================================================================
// Animation Editor State
// ============================================================================

struct AnimationEditorState {
    // Current clip
    AnimationClip* currentClip = nullptr;
    
    // Playback
    float currentTime = 0;
    float playbackSpeed = 1.0f;
    bool isPlaying = false;
    bool loop = true;
    
    // Selection
    std::string selectedBone;
    std::string selectedTrack;
    std::vector<int> selectedKeyframes;
    
    // View settings
    float timelineZoom = 1.0f;
    float timelineScroll = 0.0f;
    float curveZoomY = 1.0f;
    float curveScrollY = 0.0f;
    
    // Snapping
    bool snapToFrame = true;
    bool autoKey = false;
    
    // Display
    bool showCurveEditor = true;
    bool showDopesheet = true;
    bool showEvents = true;
    
    // Editing mode
    enum class EditMode { Select, Move, Scale, BoxSelect };
    EditMode editMode = EditMode::Select;
    
    // Ghost/onion skinning
    bool showGhosts = false;
    int ghostFramesBefore = 3;
    int ghostFramesAfter = 3;
    float ghostOpacity = 0.3f;
};

// ============================================================================
// Animation Editor
// ============================================================================

class AnimationEditor {
public:
    AnimationEditor() = default;
    
    // Set target
    void setClip(AnimationClip* clip) {
        state_.currentClip = clip;
        state_.currentTime = 0;
        state_.selectedKeyframes.clear();
    }
    
    void setSkeleton(Skeleton* skeleton) {
        skeleton_ = skeleton;
    }
    
    // Playback controls
    void play() { 
        state_.isPlaying = true; 
    }
    
    void pause() { 
        state_.isPlaying = false; 
    }
    
    void stop() {
        state_.isPlaying = false;
        state_.currentTime = 0;
        updateSkeleton();
    }
    
    void togglePlayPause() {
        state_.isPlaying = !state_.isPlaying;
    }
    
    void setTime(float time) {
        state_.currentTime = std::max(0.0f, time);
        if (state_.currentClip && time > state_.currentClip->duration) {
            if (state_.loop) {
                state_.currentTime = std::fmod(time, state_.currentClip->duration);
            } else {
                state_.currentTime = state_.currentClip->duration;
            }
        }
        updateSkeleton();
    }
    
    void nextFrame() {
        if (!state_.currentClip) return;
        float frameTime = 1.0f / state_.currentClip->frameRate;
        setTime(state_.currentTime + frameTime);
    }
    
    void prevFrame() {
        if (!state_.currentClip) return;
        float frameTime = 1.0f / state_.currentClip->frameRate;
        setTime(state_.currentTime - frameTime);
    }
    
    void goToStart() { setTime(0); }
    void goToEnd() { 
        if (state_.currentClip) setTime(state_.currentClip->duration);
    }
    
    // Update (call every frame)
    void update(float deltaTime) {
        if (state_.isPlaying && state_.currentClip) {
            state_.currentTime += deltaTime * state_.playbackSpeed;
            
            if (state_.currentTime > state_.currentClip->duration) {
                if (state_.loop) {
                    state_.currentTime = std::fmod(state_.currentTime, 
                                                   state_.currentClip->duration);
                } else {
                    state_.currentTime = state_.currentClip->duration;
                    state_.isPlaying = false;
                }
            }
            
            updateSkeleton();
            
            // Fire events
            auto events = state_.currentClip->getEventsAt(state_.currentTime);
            for (const auto& event : events) {
                if (onEvent_) {
                    onEvent_(event.name, event.parameter);
                }
            }
        }
    }
    
    // Keyframe operations
    void addKeyframeAtCurrentTime(const std::string& boneName) {
        if (!state_.currentClip || !skeleton_) return;
        
        int boneIdx = skeleton_->findBoneByName(boneName);
        if (boneIdx < 0) return;
        
        Bone* bone = skeleton_->getBone(boneIdx);
        
        float time = snapTime(state_.currentTime);
        
        state_.currentClip->addPositionKey(boneName, time, bone->localPosition);
        state_.currentClip->addRotationKey(boneName, time, bone->localRotation);
        state_.currentClip->addScaleKey(boneName, time, bone->localScale);
    }
    
    void deleteSelectedKeyframes() {
        if (!state_.currentClip) return;
        
        // Delete from all tracks
        // (simplified - in real implementation, track selection matters)
        state_.selectedKeyframes.clear();
    }
    
    void copyKeyframes() {
        // Store selected keyframes in clipboard
        copiedKeyframes_.clear();
        // Implementation depends on track type
    }
    
    void pasteKeyframes(float timeOffset = 0) {
        // Paste from clipboard at current time + offset
    }
    
    // Selection
    void selectKeyframe(int index, bool additive = false) {
        if (!additive) {
            state_.selectedKeyframes.clear();
        }
        state_.selectedKeyframes.push_back(index);
    }
    
    void selectAllKeyframesInRange(float startTime, float endTime) {
        state_.selectedKeyframes.clear();
        // Select keyframes in time range
    }
    
    void clearSelection() {
        state_.selectedKeyframes.clear();
    }
    
    // Curve editing
    void setKeyframeInterpolation(InterpolationType type) {
        // Apply to selected keyframes
    }
    
    void flattenTangents() {
        // Make selected keyframe tangents horizontal
    }
    
    void breakTangents() {
        // Make tangent handles independent
    }
    
    void unifyTangents() {
        // Make tangent handles mirrored
    }
    
    // Event callback
    void setEventCallback(std::function<void(const std::string&, const std::string&)> callback) {
        onEvent_ = callback;
    }
    
    // Getters
    AnimationEditorState& getState() { return state_; }
    const AnimationEditorState& getState() const { return state_; }
    
    float getCurrentTime() const { return state_.currentTime; }
    bool isPlaying() const { return state_.isPlaying; }
    
    // Get current frame number
    int getCurrentFrame() const {
        if (!state_.currentClip) return 0;
        return static_cast<int>(state_.currentTime * state_.currentClip->frameRate);
    }
    
    // Get total frames
    int getTotalFrames() const {
        if (!state_.currentClip) return 0;
        return static_cast<int>(state_.currentClip->duration * state_.currentClip->frameRate);
    }
    
private:
    void updateSkeleton() {
        if (state_.currentClip && skeleton_) {
            state_.currentClip->sample(state_.currentTime, skeleton_);
        }
    }
    
    float snapTime(float time) const {
        if (!state_.snapToFrame || !state_.currentClip) return time;
        float frameTime = 1.0f / state_.currentClip->frameRate;
        return std::round(time / frameTime) * frameTime;
    }
    
    AnimationEditorState state_;
    Skeleton* skeleton_ = nullptr;
    
    std::function<void(const std::string&, const std::string&)> onEvent_;
    
    // Clipboard
    struct CopiedKeyframe {
        float relativeTime;
        std::string track;
        // Value varies by type
    };
    std::vector<CopiedKeyframe> copiedKeyframes_;
};

// ============================================================================
// Animation Curve Drawer - For visualizing curves
// ============================================================================

class AnimationCurveDrawer {
public:
    struct DrawPoint {
        float x, y;
        bool isKeyframe;
        bool isSelected;
    };
    
    // Generate points for drawing a curve
    template<typename T>
    static std::vector<DrawPoint> generateCurvePoints(
        const AnimationTrack<T>& track,
        float viewStartTime, float viewEndTime,
        float viewStartValue, float viewEndValue,
        int viewWidth, int viewHeight,
        int resolution = 100) {
        
        std::vector<DrawPoint> points;
        
        if (track.keyframes.empty()) return points;
        
        float timeRange = viewEndTime - viewStartTime;
        float valueRange = viewEndValue - viewStartValue;
        
        // Add sample points
        for (int i = 0; i <= resolution; i++) {
            float t = viewStartTime + (static_cast<float>(i) / resolution) * timeRange;
            float value = evaluateAsFloat(track, t);
            
            DrawPoint p;
            p.x = (t - viewStartTime) / timeRange * viewWidth;
            p.y = viewHeight - (value - viewStartValue) / valueRange * viewHeight;
            p.isKeyframe = false;
            p.isSelected = false;
            
            points.push_back(p);
        }
        
        return points;
    }
    
    // Generate keyframe markers
    template<typename T>
    static std::vector<DrawPoint> generateKeyframeMarkers(
        const AnimationTrack<T>& track,
        float viewStartTime, float viewEndTime,
        float viewStartValue, float viewEndValue,
        int viewWidth, int viewHeight) {
        
        std::vector<DrawPoint> markers;
        
        float timeRange = viewEndTime - viewStartTime;
        float valueRange = viewEndValue - viewStartValue;
        
        for (const auto& kf : track.keyframes) {
            if (kf.time < viewStartTime || kf.time > viewEndTime) continue;
            
            float value = evaluateAsFloat(track, kf.time);
            
            DrawPoint p;
            p.x = (kf.time - viewStartTime) / timeRange * viewWidth;
            p.y = viewHeight - (value - viewStartValue) / valueRange * viewHeight;
            p.isKeyframe = true;
            p.isSelected = kf.selected;
            
            markers.push_back(p);
        }
        
        return markers;
    }
    
private:
    // Helper to evaluate any track as float (for visualization)
    template<typename T>
    static float evaluateAsFloat(const AnimationTrack<T>& track, float time);
};

template<>
inline float AnimationCurveDrawer::evaluateAsFloat(const AnimationTrack<float>& track, float time) {
    return track.evaluate(time);
}

template<>
inline float AnimationCurveDrawer::evaluateAsFloat(const AnimationTrack<Vec3>& track, float time) {
    Vec3 v = track.evaluate(time);
    return v.length();  // Return magnitude
}

template<>
inline float AnimationCurveDrawer::evaluateAsFloat(const AnimationTrack<Quat>& track, float time) {
    Quat q = track.evaluate(time);
    Vec3 euler = q.toEuler();
    return euler.y;  // Return Y rotation
}

// ============================================================================
// Animation Blender - Blend multiple animations
// ============================================================================

class AnimationBlender {
public:
    struct AnimationLayer {
        AnimationClip* clip = nullptr;
        float time = 0;
        float weight = 1.0f;
        bool additive = false;
    };
    
    void addLayer(AnimationClip* clip, float weight = 1.0f, bool additive = false) {
        layers_.push_back({clip, 0, weight, additive});
    }
    
    void removeLayer(int index) {
        if (index >= 0 && index < static_cast<int>(layers_.size())) {
            layers_.erase(layers_.begin() + index);
        }
    }
    
    void setLayerWeight(int index, float weight) {
        if (index >= 0 && index < static_cast<int>(layers_.size())) {
            layers_[index].weight = weight;
        }
    }
    
    void setLayerTime(int index, float time) {
        if (index >= 0 && index < static_cast<int>(layers_.size())) {
            layers_[index].time = time;
        }
    }
    
    // Sample blended animation
    void sample(Skeleton* skeleton) {
        if (!skeleton || layers_.empty()) return;
        
        // Store base pose
        std::vector<Vec3> basePositions(skeleton->getBoneCount());
        std::vector<Quat> baseRotations(skeleton->getBoneCount());
        std::vector<Vec3> baseScales(skeleton->getBoneCount());
        
        for (int i = 0; i < skeleton->getBoneCount(); i++) {
            Bone* bone = skeleton->getBone(i);
            basePositions[i] = bone->localPosition;
            baseRotations[i] = bone->localRotation;
            baseScales[i] = bone->localScale;
        }
        
        // Apply layers
        float totalWeight = 0;
        
        for (const auto& layer : layers_) {
            if (!layer.clip || layer.weight <= 0) continue;
            
            layer.clip->sample(layer.time, skeleton);
            
            // Blend with previous
            if (!layer.additive) {
                float blendWeight = layer.weight / (totalWeight + layer.weight);
                
                for (int i = 0; i < skeleton->getBoneCount(); i++) {
                    Bone* bone = skeleton->getBone(i);
                    
                    bone->localPosition = basePositions[i] * (1 - blendWeight) + 
                                          bone->localPosition * blendWeight;
                    bone->localRotation = baseRotations[i].slerp(bone->localRotation, blendWeight);
                    bone->localScale = baseScales[i] * (1 - blendWeight) + 
                                       bone->localScale * blendWeight;
                    
                    basePositions[i] = bone->localPosition;
                    baseRotations[i] = bone->localRotation;
                    baseScales[i] = bone->localScale;
                }
                
                totalWeight += layer.weight;
            } else {
                // Additive layer
                for (int i = 0; i < skeleton->getBoneCount(); i++) {
                    Bone* bone = skeleton->getBone(i);
                    
                    // Add position offset
                    bone->localPosition = basePositions[i] + 
                        (bone->localPosition - basePositions[i]) * layer.weight;
                    
                    // Multiply rotation
                    Quat addRot = baseRotations[i].inverse() * bone->localRotation;
                    bone->localRotation = baseRotations[i] * 
                        Quat::identity().slerp(addRot, layer.weight);
                }
            }
        }
        
        skeleton->updateMatrices();
    }
    
private:
    std::vector<AnimationLayer> layers_;
};

// ============================================================================
// Animation Retargeter - Transfer animation between skeletons
// ============================================================================

class AnimationRetargeter {
public:
    struct BoneMapping {
        std::string sourceBone;
        std::string targetBone;
        Vec3 rotationOffset{0, 0, 0};
        float scaleMultiplier = 1.0f;
    };
    
    void addMapping(const std::string& source, const std::string& target,
                    const Vec3& rotOffset = Vec3(0, 0, 0), float scale = 1.0f) {
        mappings_.push_back({source, target, rotOffset, scale});
    }
    
    // Generate automatic mappings by name matching
    void autoMap(const Skeleton& source, const Skeleton& target) {
        mappings_.clear();
        
        for (int i = 0; i < source.getBoneCount(); i++) {
            const std::string& sourceName = source.getBone(i)->name;
            
            // Try to find matching bone in target
            int targetIdx = target.findBoneByName(sourceName);
            if (targetIdx >= 0) {
                addMapping(sourceName, sourceName);
                continue;
            }
            
            // Try common variations
            std::vector<std::string> variations = generateNameVariations(sourceName);
            for (const auto& var : variations) {
                targetIdx = target.findBoneByName(var);
                if (targetIdx >= 0) {
                    addMapping(sourceName, var);
                    break;
                }
            }
        }
    }
    
    // Retarget animation clip
    AnimationClip retarget(const AnimationClip& source,
                           const Skeleton& sourceSkel,
                           const Skeleton& targetSkel) {
        AnimationClip result;
        result.name = source.name + "_retargeted";
        result.duration = source.duration;
        result.frameRate = source.frameRate;
        result.loop = source.loop;
        
        // Retarget rotation tracks
        for (const auto& [boneName, track] : source.rotationTracks) {
            std::string targetBone = findTargetBone(boneName);
            if (targetBone.empty()) continue;
            
            const BoneMapping* mapping = findMapping(boneName);
            
            // Copy keyframes with optional offset
            for (const auto& kf : track.keyframes) {
                Quat rot = kf.value;
                
                if (mapping) {
                    Quat offset = Quat::fromEuler(mapping->rotationOffset.x,
                                                  mapping->rotationOffset.y,
                                                  mapping->rotationOffset.z);
                    rot = offset * rot;
                }
                
                result.addRotationKey(targetBone, kf.time, rot, kf.interpolation);
            }
        }
        
        // Retarget position tracks (with scale adjustment)
        for (const auto& [boneName, track] : source.positionTracks) {
            std::string targetBone = findTargetBone(boneName);
            if (targetBone.empty()) continue;
            
            const BoneMapping* mapping = findMapping(boneName);
            
            for (const auto& kf : track.keyframes) {
                Vec3 pos = kf.value;
                
                if (mapping) {
                    pos = pos * mapping->scaleMultiplier;
                }
                
                result.addPositionKey(targetBone, kf.time, pos, kf.interpolation);
            }
        }
        
        // Copy events
        result.events = source.events;
        
        return result;
    }
    
private:
    std::string findTargetBone(const std::string& sourceBone) const {
        for (const auto& m : mappings_) {
            if (m.sourceBone == sourceBone) {
                return m.targetBone;
            }
        }
        return "";
    }
    
    const BoneMapping* findMapping(const std::string& sourceBone) const {
        for (const auto& m : mappings_) {
            if (m.sourceBone == sourceBone) {
                return &m;
            }
        }
        return nullptr;
    }
    
    std::vector<std::string> generateNameVariations(const std::string& name) {
        std::vector<std::string> variations;
        
        // Common prefixes/suffixes
        variations.push_back("mixamorig:" + name);
        variations.push_back(name + "_bind");
        variations.push_back(name + "_jnt");
        
        // Case variations
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        variations.push_back(lower);
        
        return variations;
    }
    
    std::vector<BoneMapping> mappings_;
};

}  // namespace luma
