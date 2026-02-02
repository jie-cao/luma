// Facial Rig System - Standard facial animation and expressions
// Supports: ARKit (52 BlendShapes), VRM, Unity, Viseme (Lip-sync)
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <cmath>

namespace luma {

// ============================================================================
// Facial Animation Standards
// ============================================================================

enum class FacialStandard {
    ARKit,          // Apple ARKit 52 BlendShapes
    VRM,            // VRM 1.0 expressions
    Viseme,         // Lip-sync phonemes (15 visemes)
    FaceRobot,      // Autodesk FaceRobot
    FACS,           // Facial Action Coding System
    Custom
};

// ============================================================================
// ARKit BlendShape Names (52 shapes) - Industry Standard
// ============================================================================

namespace ARKitBlendShapes {
    // Eye Left
    constexpr const char* eyeBlinkLeft = "eyeBlinkLeft";
    constexpr const char* eyeLookDownLeft = "eyeLookDownLeft";
    constexpr const char* eyeLookInLeft = "eyeLookInLeft";
    constexpr const char* eyeLookOutLeft = "eyeLookOutLeft";
    constexpr const char* eyeLookUpLeft = "eyeLookUpLeft";
    constexpr const char* eyeSquintLeft = "eyeSquintLeft";
    constexpr const char* eyeWideLeft = "eyeWideLeft";
    
    // Eye Right
    constexpr const char* eyeBlinkRight = "eyeBlinkRight";
    constexpr const char* eyeLookDownRight = "eyeLookDownRight";
    constexpr const char* eyeLookInRight = "eyeLookInRight";
    constexpr const char* eyeLookOutRight = "eyeLookOutRight";
    constexpr const char* eyeLookUpRight = "eyeLookUpRight";
    constexpr const char* eyeSquintRight = "eyeSquintRight";
    constexpr const char* eyeWideRight = "eyeWideRight";
    
    // Jaw
    constexpr const char* jawForward = "jawForward";
    constexpr const char* jawLeft = "jawLeft";
    constexpr const char* jawRight = "jawRight";
    constexpr const char* jawOpen = "jawOpen";
    
    // Mouth
    constexpr const char* mouthClose = "mouthClose";
    constexpr const char* mouthFunnel = "mouthFunnel";
    constexpr const char* mouthPucker = "mouthPucker";
    constexpr const char* mouthLeft = "mouthLeft";
    constexpr const char* mouthRight = "mouthRight";
    constexpr const char* mouthSmileLeft = "mouthSmileLeft";
    constexpr const char* mouthSmileRight = "mouthSmileRight";
    constexpr const char* mouthFrownLeft = "mouthFrownLeft";
    constexpr const char* mouthFrownRight = "mouthFrownRight";
    constexpr const char* mouthDimpleLeft = "mouthDimpleLeft";
    constexpr const char* mouthDimpleRight = "mouthDimpleRight";
    constexpr const char* mouthStretchLeft = "mouthStretchLeft";
    constexpr const char* mouthStretchRight = "mouthStretchRight";
    constexpr const char* mouthRollLower = "mouthRollLower";
    constexpr const char* mouthRollUpper = "mouthRollUpper";
    constexpr const char* mouthShrugLower = "mouthShrugLower";
    constexpr const char* mouthShrugUpper = "mouthShrugUpper";
    constexpr const char* mouthPressLeft = "mouthPressLeft";
    constexpr const char* mouthPressRight = "mouthPressRight";
    constexpr const char* mouthLowerDownLeft = "mouthLowerDownLeft";
    constexpr const char* mouthLowerDownRight = "mouthLowerDownRight";
    constexpr const char* mouthUpperUpLeft = "mouthUpperUpLeft";
    constexpr const char* mouthUpperUpRight = "mouthUpperUpRight";
    
    // Brow
    constexpr const char* browDownLeft = "browDownLeft";
    constexpr const char* browDownRight = "browDownRight";
    constexpr const char* browInnerUp = "browInnerUp";
    constexpr const char* browOuterUpLeft = "browOuterUpLeft";
    constexpr const char* browOuterUpRight = "browOuterUpRight";
    
    // Cheek
    constexpr const char* cheekPuff = "cheekPuff";
    constexpr const char* cheekSquintLeft = "cheekSquintLeft";
    constexpr const char* cheekSquintRight = "cheekSquintRight";
    
    // Nose
    constexpr const char* noseSneerLeft = "noseSneerLeft";
    constexpr const char* noseSneerRight = "noseSneerRight";
    
    // Tongue
    constexpr const char* tongueOut = "tongueOut";
    
    // Get all ARKit blend shape names (ordered)
    inline std::vector<std::string> getAll() {
        return {
            eyeBlinkLeft, eyeLookDownLeft, eyeLookInLeft, eyeLookOutLeft,
            eyeLookUpLeft, eyeSquintLeft, eyeWideLeft,
            eyeBlinkRight, eyeLookDownRight, eyeLookInRight, eyeLookOutRight,
            eyeLookUpRight, eyeSquintRight, eyeWideRight,
            jawForward, jawLeft, jawRight, jawOpen,
            mouthClose, mouthFunnel, mouthPucker,
            mouthLeft, mouthRight,
            mouthSmileLeft, mouthSmileRight,
            mouthFrownLeft, mouthFrownRight,
            mouthDimpleLeft, mouthDimpleRight,
            mouthStretchLeft, mouthStretchRight,
            mouthRollLower, mouthRollUpper,
            mouthShrugLower, mouthShrugUpper,
            mouthPressLeft, mouthPressRight,
            mouthLowerDownLeft, mouthLowerDownRight,
            mouthUpperUpLeft, mouthUpperUpRight,
            browDownLeft, browDownRight, browInnerUp,
            browOuterUpLeft, browOuterUpRight,
            cheekPuff, cheekSquintLeft, cheekSquintRight,
            noseSneerLeft, noseSneerRight,
            tongueOut
        };
    }
    
    // Get count
    constexpr int getCount() { return 52; }
    
    // Category groupings
    inline std::vector<std::string> getEyeShapes() {
        return {
            eyeBlinkLeft, eyeLookDownLeft, eyeLookInLeft, eyeLookOutLeft,
            eyeLookUpLeft, eyeSquintLeft, eyeWideLeft,
            eyeBlinkRight, eyeLookDownRight, eyeLookInRight, eyeLookOutRight,
            eyeLookUpRight, eyeSquintRight, eyeWideRight
        };
    }
    
    inline std::vector<std::string> getMouthShapes() {
        return {
            jawForward, jawLeft, jawRight, jawOpen,
            mouthClose, mouthFunnel, mouthPucker,
            mouthLeft, mouthRight,
            mouthSmileLeft, mouthSmileRight,
            mouthFrownLeft, mouthFrownRight,
            mouthDimpleLeft, mouthDimpleRight,
            mouthStretchLeft, mouthStretchRight,
            mouthRollLower, mouthRollUpper,
            mouthShrugLower, mouthShrugUpper,
            mouthPressLeft, mouthPressRight,
            mouthLowerDownLeft, mouthLowerDownRight,
            mouthUpperUpLeft, mouthUpperUpRight
        };
    }
    
    inline std::vector<std::string> getBrowShapes() {
        return {
            browDownLeft, browDownRight, browInnerUp,
            browOuterUpLeft, browOuterUpRight
        };
    }
}

// ============================================================================
// VRM Expression Names
// ============================================================================

namespace VRMExpressions {
    // Preset expressions
    constexpr const char* happy = "happy";
    constexpr const char* angry = "angry";
    constexpr const char* sad = "sad";
    constexpr const char* relaxed = "relaxed";
    constexpr const char* surprised = "surprised";
    
    // Eye controls
    constexpr const char* blinkLeft = "blinkLeft";
    constexpr const char* blinkRight = "blinkRight";
    constexpr const char* lookUp = "lookUp";
    constexpr const char* lookDown = "lookDown";
    constexpr const char* lookLeft = "lookLeft";
    constexpr const char* lookRight = "lookRight";
    
    // Mouth
    constexpr const char* aa = "aa";    // A vowel
    constexpr const char* ih = "ih";    // I vowel
    constexpr const char* ou = "ou";    // U vowel
    constexpr const char* ee = "ee";    // E vowel
    constexpr const char* oh = "oh";    // O vowel
    
    // Other
    constexpr const char* neutral = "neutral";
    
    inline std::vector<std::string> getAll() {
        return {
            happy, angry, sad, relaxed, surprised,
            blinkLeft, blinkRight,
            lookUp, lookDown, lookLeft, lookRight,
            aa, ih, ou, ee, oh,
            neutral
        };
    }
}

// ============================================================================
// Viseme Names (Lip-sync)
// ============================================================================

namespace Visemes {
    constexpr const char* sil = "sil";      // Silence
    constexpr const char* PP = "PP";        // p, b, m
    constexpr const char* FF = "FF";        // f, v
    constexpr const char* TH = "TH";        // th
    constexpr const char* DD = "DD";        // t, d
    constexpr const char* kk = "kk";        // k, g
    constexpr const char* CH = "CH";        // ch, j, sh
    constexpr const char* SS = "SS";        // s, z
    constexpr const char* nn = "nn";        // n, l
    constexpr const char* RR = "RR";        // r
    constexpr const char* aa = "aa";        // A
    constexpr const char* E = "E";          // E
    constexpr const char* ih = "ih";        // I
    constexpr const char* oh = "oh";        // O
    constexpr const char* ou = "ou";        // U
    
    inline std::vector<std::string> getAll() {
        return { sil, PP, FF, TH, DD, kk, CH, SS, nn, RR, aa, E, ih, oh, ou };
    }
    
    constexpr int getCount() { return 15; }
}

// ============================================================================
// Facial BlendShape Mapping
// ============================================================================

struct FacialMapping {
    std::string sourceName;     // Name in source standard
    std::string targetName;     // Name in target standard
    float scale = 1.0f;         // Weight multiplier
    float offset = 0.0f;        // Weight offset
};

// ============================================================================
// Facial Mapping Tables
// ============================================================================

class FacialMappingTable {
public:
    static FacialMappingTable& getInstance() {
        static FacialMappingTable instance;
        return instance;
    }
    
    // Get mapping from ARKit to VRM
    std::vector<FacialMapping> getARKitToVRMMappings() const {
        return {
            // Blink
            {ARKitBlendShapes::eyeBlinkLeft, VRMExpressions::blinkLeft, 1.0f, 0.0f},
            {ARKitBlendShapes::eyeBlinkRight, VRMExpressions::blinkRight, 1.0f, 0.0f},
            
            // Look direction
            {ARKitBlendShapes::eyeLookUpLeft, VRMExpressions::lookUp, 0.5f, 0.0f},
            {ARKitBlendShapes::eyeLookUpRight, VRMExpressions::lookUp, 0.5f, 0.0f},
            {ARKitBlendShapes::eyeLookDownLeft, VRMExpressions::lookDown, 0.5f, 0.0f},
            {ARKitBlendShapes::eyeLookDownRight, VRMExpressions::lookDown, 0.5f, 0.0f},
            
            // Mouth vowels (approximate)
            {ARKitBlendShapes::jawOpen, VRMExpressions::aa, 0.8f, 0.0f},
            {ARKitBlendShapes::mouthPucker, VRMExpressions::ou, 0.9f, 0.0f},
            {ARKitBlendShapes::mouthFunnel, VRMExpressions::oh, 0.8f, 0.0f},
        };
    }
    
    // Get mapping from ARKit to Visemes
    std::vector<FacialMapping> getARKitToVisemeMappings() const {
        return {
            // Silence
            {ARKitBlendShapes::mouthClose, Visemes::sil, 1.0f, 0.0f},
            
            // Bilabial (PP: p, b, m)
            {ARKitBlendShapes::mouthPressLeft, Visemes::PP, 0.5f, 0.0f},
            {ARKitBlendShapes::mouthPressRight, Visemes::PP, 0.5f, 0.0f},
            
            // Labiodental (FF: f, v)
            {ARKitBlendShapes::mouthRollLower, Visemes::FF, 0.7f, 0.0f},
            
            // Dental (TH: th)
            {ARKitBlendShapes::tongueOut, Visemes::TH, 0.3f, 0.0f},
            
            // Vowels
            {ARKitBlendShapes::jawOpen, Visemes::aa, 0.8f, 0.0f},
            {ARKitBlendShapes::mouthPucker, Visemes::ou, 0.9f, 0.0f},
            {ARKitBlendShapes::mouthSmileLeft, Visemes::ih, 0.3f, 0.0f},
            {ARKitBlendShapes::mouthSmileRight, Visemes::ih, 0.3f, 0.0f},
        };
    }
    
private:
    FacialMappingTable() = default;
};

// ============================================================================
// Standard Facial Rig Data Structure
// ============================================================================

struct FacialRigData {
    // ARKit compatible weights (52 values)
    std::array<float, 52> arkitWeights = {};
    
    // Eye gaze (separate from blend shapes for performance)
    Vec3 leftEyeGaze{0, 0, 1};   // Direction vector
    Vec3 rightEyeGaze{0, 0, 1};
    
    // Jaw transform (can use bone or blend shape)
    float jawOpenAmount = 0.0f;
    Quat jawRotation;
    
    // Tongue position
    float tongueOut = 0.0f;
    Vec3 tongueDirection{0, 0, 1};
    
    // Reset to neutral
    void reset() {
        arkitWeights.fill(0.0f);
        leftEyeGaze = rightEyeGaze = Vec3(0, 0, 1);
        jawOpenAmount = 0.0f;
        jawRotation = Quat();
        tongueOut = 0.0f;
        tongueDirection = Vec3(0, 0, 1);
    }
    
    // Set ARKit weight by name
    bool setWeight(const std::string& name, float weight) {
        auto names = ARKitBlendShapes::getAll();
        for (size_t i = 0; i < names.size() && i < 52; i++) {
            if (names[i] == name) {
                arkitWeights[i] = std::clamp(weight, 0.0f, 1.0f);
                return true;
            }
        }
        return false;
    }
    
    // Get ARKit weight by name
    float getWeight(const std::string& name) const {
        auto names = ARKitBlendShapes::getAll();
        for (size_t i = 0; i < names.size() && i < 52; i++) {
            if (names[i] == name) {
                return arkitWeights[i];
            }
        }
        return 0.0f;
    }
    
    // Apply to BlendShapeMesh
    void applyToBlendShapeMesh(BlendShapeMesh& mesh) const {
        auto names = ARKitBlendShapes::getAll();
        for (size_t i = 0; i < names.size() && i < 52; i++) {
            mesh.setWeight(names[i], arkitWeights[i]);
        }
    }
};

// ============================================================================
// Expression Presets
// ============================================================================

struct ExpressionPreset {
    std::string name;
    FacialRigData data;
    float transitionTime = 0.2f;  // Seconds to blend to this expression
    
    ExpressionPreset() = default;
    ExpressionPreset(const std::string& n) : name(n) {}
};

class ExpressionLibrary {
public:
    static ExpressionLibrary& getInstance() {
        static ExpressionLibrary instance;
        return instance;
    }
    
    const ExpressionPreset& getPreset(const std::string& name) const {
        auto it = presets_.find(name);
        return (it != presets_.end()) ? it->second : neutral_;
    }
    
    std::vector<std::string> getPresetNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : presets_) {
            names.push_back(name);
        }
        return names;
    }
    
    void addPreset(const ExpressionPreset& preset) {
        presets_[preset.name] = preset;
    }
    
private:
    ExpressionLibrary() {
        initializeDefaults();
    }
    
    void initializeDefaults() {
        // Neutral
        neutral_.name = "neutral";
        presets_["neutral"] = neutral_;
        
        // Happy/Smile
        {
            ExpressionPreset p("happy");
            p.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.7f);
            p.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.7f);
            p.data.setWeight(ARKitBlendShapes::cheekSquintLeft, 0.3f);
            p.data.setWeight(ARKitBlendShapes::cheekSquintRight, 0.3f);
            p.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.2f);
            p.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.2f);
            presets_["happy"] = p;
        }
        
        // Sad
        {
            ExpressionPreset p("sad");
            p.data.setWeight(ARKitBlendShapes::mouthFrownLeft, 0.6f);
            p.data.setWeight(ARKitBlendShapes::mouthFrownRight, 0.6f);
            p.data.setWeight(ARKitBlendShapes::browInnerUp, 0.5f);
            p.data.setWeight(ARKitBlendShapes::browDownLeft, 0.3f);
            p.data.setWeight(ARKitBlendShapes::browDownRight, 0.3f);
            presets_["sad"] = p;
        }
        
        // Angry
        {
            ExpressionPreset p("angry");
            p.data.setWeight(ARKitBlendShapes::browDownLeft, 0.8f);
            p.data.setWeight(ARKitBlendShapes::browDownRight, 0.8f);
            p.data.setWeight(ARKitBlendShapes::eyeSquintLeft, 0.4f);
            p.data.setWeight(ARKitBlendShapes::eyeSquintRight, 0.4f);
            p.data.setWeight(ARKitBlendShapes::noseSneerLeft, 0.3f);
            p.data.setWeight(ARKitBlendShapes::noseSneerRight, 0.3f);
            p.data.setWeight(ARKitBlendShapes::jawForward, 0.2f);
            presets_["angry"] = p;
        }
        
        // Surprised
        {
            ExpressionPreset p("surprised");
            p.data.setWeight(ARKitBlendShapes::eyeWideLeft, 0.8f);
            p.data.setWeight(ARKitBlendShapes::eyeWideRight, 0.8f);
            p.data.setWeight(ARKitBlendShapes::browInnerUp, 0.6f);
            p.data.setWeight(ARKitBlendShapes::browOuterUpLeft, 0.5f);
            p.data.setWeight(ARKitBlendShapes::browOuterUpRight, 0.5f);
            p.data.setWeight(ARKitBlendShapes::jawOpen, 0.4f);
            presets_["surprised"] = p;
        }
        
        // Fear
        {
            ExpressionPreset p("fear");
            p.data.setWeight(ARKitBlendShapes::eyeWideLeft, 0.9f);
            p.data.setWeight(ARKitBlendShapes::eyeWideRight, 0.9f);
            p.data.setWeight(ARKitBlendShapes::browInnerUp, 0.8f);
            p.data.setWeight(ARKitBlendShapes::mouthStretchLeft, 0.4f);
            p.data.setWeight(ARKitBlendShapes::mouthStretchRight, 0.4f);
            presets_["fear"] = p;
        }
        
        // Disgust
        {
            ExpressionPreset p("disgust");
            p.data.setWeight(ARKitBlendShapes::noseSneerLeft, 0.6f);
            p.data.setWeight(ARKitBlendShapes::noseSneerRight, 0.6f);
            p.data.setWeight(ARKitBlendShapes::mouthUpperUpLeft, 0.4f);
            p.data.setWeight(ARKitBlendShapes::mouthUpperUpRight, 0.4f);
            p.data.setWeight(ARKitBlendShapes::browDownLeft, 0.3f);
            p.data.setWeight(ARKitBlendShapes::browDownRight, 0.3f);
            presets_["disgust"] = p;
        }
        
        // Blink
        {
            ExpressionPreset p("blink");
            p.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 1.0f);
            p.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 1.0f);
            p.transitionTime = 0.05f;  // Quick blink
            presets_["blink"] = p;
        }
        
        // Wink Left
        {
            ExpressionPreset p("wink_left");
            p.data.setWeight(ARKitBlendShapes::eyeBlinkLeft, 1.0f);
            p.data.setWeight(ARKitBlendShapes::mouthSmileLeft, 0.3f);
            p.transitionTime = 0.1f;
            presets_["wink_left"] = p;
        }
        
        // Wink Right
        {
            ExpressionPreset p("wink_right");
            p.data.setWeight(ARKitBlendShapes::eyeBlinkRight, 1.0f);
            p.data.setWeight(ARKitBlendShapes::mouthSmileRight, 0.3f);
            p.transitionTime = 0.1f;
            presets_["wink_right"] = p;
        }
    }
    
    std::unordered_map<std::string, ExpressionPreset> presets_;
    ExpressionPreset neutral_;
};

// ============================================================================
// Facial Rig Controller - Manages facial animation
// ============================================================================

class FacialRigController {
public:
    FacialRigController() = default;
    
    // === Data Access ===
    
    FacialRigData& getData() { return currentData_; }
    const FacialRigData& getData() const { return currentData_; }
    
    // === Weight Control ===
    
    void setWeight(const std::string& name, float weight) {
        currentData_.setWeight(name, weight);
        dirty_ = true;
    }
    
    float getWeight(const std::string& name) const {
        return currentData_.getWeight(name);
    }
    
    // Set all weights from array (for ARKit input)
    void setARKitWeights(const float* weights, int count) {
        int n = std::min(count, 52);
        for (int i = 0; i < n; i++) {
            currentData_.arkitWeights[i] = weights[i];
        }
        dirty_ = true;
    }
    
    // === Eye Gaze ===
    
    void setEyeGaze(const Vec3& leftGaze, const Vec3& rightGaze) {
        currentData_.leftEyeGaze = leftGaze.normalized();
        currentData_.rightEyeGaze = rightGaze.normalized();
        dirty_ = true;
    }
    
    void setEyeGazeBoth(const Vec3& gaze) {
        setEyeGaze(gaze, gaze);
    }
    
    // Look at a world position (requires head position and orientation)
    void lookAt(const Vec3& targetWorld, const Vec3& headPosition, const Quat& headRotation) {
        Vec3 toTarget = (targetWorld - headPosition).normalized();
        
        // Transform to head-local space
        Quat invHead = headRotation.conjugate();
        Vec3 localDir = invHead.rotate(toTarget);
        
        // Clamp to reasonable eye movement range
        float maxAngle = 0.5f;  // ~30 degrees
        float yaw = std::atan2(localDir.x, localDir.z);
        float pitch = std::asin(std::clamp(localDir.y, -1.0f, 1.0f));
        
        yaw = std::clamp(yaw, -maxAngle, maxAngle);
        pitch = std::clamp(pitch, -maxAngle * 0.7f, maxAngle * 0.7f);
        
        Vec3 clampedDir(
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::cos(yaw) * std::cos(pitch)
        );
        
        currentData_.leftEyeGaze = clampedDir;
        currentData_.rightEyeGaze = clampedDir;
        
        // Also set ARKit look weights
        currentData_.setWeight(ARKitBlendShapes::eyeLookUpLeft, std::max(0.0f, pitch / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookUpRight, std::max(0.0f, pitch / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookDownLeft, std::max(0.0f, -pitch / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookDownRight, std::max(0.0f, -pitch / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookOutLeft, std::max(0.0f, yaw / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookInRight, std::max(0.0f, yaw / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookInLeft, std::max(0.0f, -yaw / maxAngle));
        currentData_.setWeight(ARKitBlendShapes::eyeLookOutRight, std::max(0.0f, -yaw / maxAngle));
        
        dirty_ = true;
    }
    
    // === Expression Presets ===
    
    void setExpression(const std::string& name, float intensity = 1.0f, bool additive = false) {
        const ExpressionPreset& preset = ExpressionLibrary::getInstance().getPreset(name);
        
        if (!additive) {
            currentData_.reset();
        }
        
        for (int i = 0; i < 52; i++) {
            float targetWeight = preset.data.arkitWeights[i] * intensity;
            if (additive) {
                currentData_.arkitWeights[i] = std::clamp(
                    currentData_.arkitWeights[i] + targetWeight, 0.0f, 1.0f);
            } else {
                currentData_.arkitWeights[i] = targetWeight;
            }
        }
        
        targetExpression_ = name;
        dirty_ = true;
    }
    
    // Blend between current and target expression
    void blendExpression(const std::string& name, float blend) {
        const ExpressionPreset& preset = ExpressionLibrary::getInstance().getPreset(name);
        
        for (int i = 0; i < 52; i++) {
            float target = preset.data.arkitWeights[i];
            currentData_.arkitWeights[i] = 
                currentData_.arkitWeights[i] * (1.0f - blend) + target * blend;
        }
        
        dirty_ = true;
    }
    
    // === Automatic Behaviors ===
    
    // Auto blink (call every frame with delta time)
    void updateAutoBlink(float deltaTime) {
        if (!autoBlinkEnabled_) return;
        
        blinkTimer_ -= deltaTime;
        
        if (blinkTimer_ <= 0.0f) {
            // Start blink
            if (!isBlinking_) {
                isBlinking_ = true;
                blinkProgress_ = 0.0f;
            }
        }
        
        if (isBlinking_) {
            blinkProgress_ += deltaTime / blinkDuration_;
            
            float blinkWeight;
            if (blinkProgress_ < 0.5f) {
                // Closing
                blinkWeight = blinkProgress_ * 2.0f;
            } else {
                // Opening
                blinkWeight = (1.0f - blinkProgress_) * 2.0f;
            }
            
            currentData_.setWeight(ARKitBlendShapes::eyeBlinkLeft, blinkWeight);
            currentData_.setWeight(ARKitBlendShapes::eyeBlinkRight, blinkWeight);
            dirty_ = true;
            
            if (blinkProgress_ >= 1.0f) {
                isBlinking_ = false;
                // Random interval for next blink (2-6 seconds)
                blinkTimer_ = 2.0f + (rand() / (float)RAND_MAX) * 4.0f;
            }
        }
    }
    
    void setAutoBlinkEnabled(bool enabled) { autoBlinkEnabled_ = enabled; }
    bool isAutoBlinkEnabled() const { return autoBlinkEnabled_; }
    
    // === Apply to Output ===
    
    void applyToBlendShapeMesh(BlendShapeMesh& mesh) const {
        currentData_.applyToBlendShapeMesh(mesh);
    }
    
    void applyToSkeleton(Skeleton& skeleton) const {
        // Apply jaw
        int jawIdx = skeleton.findBoneByName("jaw");
        if (jawIdx >= 0) {
            float jawOpen = currentData_.getWeight(ARKitBlendShapes::jawOpen);
            // Rotate jaw bone based on jawOpen weight
            Quat jawRot = Quat::fromAxisAngle(Vec3(1, 0, 0), jawOpen * 0.3f);  // ~17 degrees max
            Bone* jaw = skeleton.getBone(jawIdx);
            if (jaw) {
                jaw->localRotation = jawRot;
            }
        }
        
        // Apply eye gaze
        int leftEyeIdx = skeleton.findBoneByName("eye_L");
        int rightEyeIdx = skeleton.findBoneByName("eye_R");
        
        if (leftEyeIdx >= 0) {
            Quat leftEyeRot = gazeToRotation(currentData_.leftEyeGaze);
            Bone* leftEye = skeleton.getBone(leftEyeIdx);
            if (leftEye) {
                leftEye->localRotation = leftEyeRot;
            }
        }
        
        if (rightEyeIdx >= 0) {
            Quat rightEyeRot = gazeToRotation(currentData_.rightEyeGaze);
            Bone* rightEye = skeleton.getBone(rightEyeIdx);
            if (rightEye) {
                rightEye->localRotation = rightEyeRot;
            }
        }
    }
    
    // === State ===
    
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }
    
    void reset() {
        currentData_.reset();
        targetExpression_ = "neutral";
        isBlinking_ = false;
        dirty_ = true;
    }
    
private:
    Quat gazeToRotation(const Vec3& gaze) const {
        // Convert gaze direction to rotation
        float yaw = std::atan2(gaze.x, gaze.z);
        float pitch = std::asin(std::clamp(gaze.y, -1.0f, 1.0f));
        
        return Quat::fromAxisAngle(Vec3(0, 1, 0), yaw) *
               Quat::fromAxisAngle(Vec3(1, 0, 0), -pitch);
    }
    
    FacialRigData currentData_;
    std::string targetExpression_ = "neutral";
    bool dirty_ = true;
    
    // Auto blink state
    bool autoBlinkEnabled_ = true;
    float blinkTimer_ = 3.0f;
    float blinkDuration_ = 0.15f;
    float blinkProgress_ = 0.0f;
    bool isBlinking_ = false;
};

// ============================================================================
// Viseme Controller - For lip-sync
// ============================================================================

class VisemeController {
public:
    VisemeController() = default;
    
    // Set current viseme (from audio analysis)
    void setViseme(const std::string& visemeName, float weight = 1.0f) {
        currentViseme_ = visemeName;
        currentWeight_ = weight;
        dirty_ = true;
    }
    
    // Set viseme by index (0-14)
    void setVisemeByIndex(int index, float weight = 1.0f) {
        auto visemes = Visemes::getAll();
        if (index >= 0 && index < (int)visemes.size()) {
            setViseme(visemes[index], weight);
        }
    }
    
    // Apply viseme to facial rig (converts to ARKit blend shapes)
    void applyToFacialRig(FacialRigController& rig) const {
        if (currentViseme_.empty()) return;
        
        // Reset mouth-related weights first
        auto mouthShapes = ARKitBlendShapes::getMouthShapes();
        for (const auto& shape : mouthShapes) {
            rig.setWeight(shape, 0.0f);
        }
        
        // Map viseme to ARKit weights
        float w = currentWeight_;
        
        if (currentViseme_ == Visemes::sil) {
            rig.setWeight(ARKitBlendShapes::mouthClose, w * 0.3f);
        }
        else if (currentViseme_ == Visemes::PP) {
            rig.setWeight(ARKitBlendShapes::mouthPressLeft, w);
            rig.setWeight(ARKitBlendShapes::mouthPressRight, w);
        }
        else if (currentViseme_ == Visemes::FF) {
            rig.setWeight(ARKitBlendShapes::mouthRollLower, w * 0.6f);
            rig.setWeight(ARKitBlendShapes::mouthUpperUpLeft, w * 0.3f);
            rig.setWeight(ARKitBlendShapes::mouthUpperUpRight, w * 0.3f);
        }
        else if (currentViseme_ == Visemes::TH) {
            rig.setWeight(ARKitBlendShapes::tongueOut, w * 0.3f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.2f);
        }
        else if (currentViseme_ == Visemes::DD || currentViseme_ == Visemes::nn) {
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.2f);
            rig.setWeight(ARKitBlendShapes::mouthClose, w * 0.3f);
        }
        else if (currentViseme_ == Visemes::kk) {
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.3f);
            rig.setWeight(ARKitBlendShapes::mouthShrugUpper, w * 0.4f);
        }
        else if (currentViseme_ == Visemes::CH || currentViseme_ == Visemes::SS) {
            rig.setWeight(ARKitBlendShapes::mouthStretchLeft, w * 0.4f);
            rig.setWeight(ARKitBlendShapes::mouthStretchRight, w * 0.4f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.1f);
        }
        else if (currentViseme_ == Visemes::RR) {
            rig.setWeight(ARKitBlendShapes::mouthPucker, w * 0.5f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.2f);
        }
        else if (currentViseme_ == Visemes::aa) {
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.7f);
            rig.setWeight(ARKitBlendShapes::mouthLowerDownLeft, w * 0.3f);
            rig.setWeight(ARKitBlendShapes::mouthLowerDownRight, w * 0.3f);
        }
        else if (currentViseme_ == Visemes::E) {
            rig.setWeight(ARKitBlendShapes::mouthStretchLeft, w * 0.5f);
            rig.setWeight(ARKitBlendShapes::mouthStretchRight, w * 0.5f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.3f);
        }
        else if (currentViseme_ == Visemes::ih) {
            rig.setWeight(ARKitBlendShapes::mouthSmileLeft, w * 0.3f);
            rig.setWeight(ARKitBlendShapes::mouthSmileRight, w * 0.3f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.2f);
        }
        else if (currentViseme_ == Visemes::oh) {
            rig.setWeight(ARKitBlendShapes::mouthFunnel, w * 0.6f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.4f);
        }
        else if (currentViseme_ == Visemes::ou) {
            rig.setWeight(ARKitBlendShapes::mouthPucker, w * 0.7f);
            rig.setWeight(ARKitBlendShapes::jawOpen, w * 0.3f);
        }
    }
    
    const std::string& getCurrentViseme() const { return currentViseme_; }
    float getCurrentWeight() const { return currentWeight_; }
    
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }
    
private:
    std::string currentViseme_;
    float currentWeight_ = 0.0f;
    bool dirty_ = false;
};

// ============================================================================
// Complete Facial Rig - Combines all facial animation systems
// ============================================================================

class CompleteFacialRig {
public:
    CompleteFacialRig() = default;
    
    // Main controller
    FacialRigController& getController() { return controller_; }
    const FacialRigController& getController() const { return controller_; }
    
    // Viseme/lip-sync
    VisemeController& getVisemeController() { return visemeController_; }
    const VisemeController& getVisemeController() const { return visemeController_; }
    
    // Update (call every frame)
    void update(float deltaTime) {
        controller_.updateAutoBlink(deltaTime);
        
        // Apply viseme if active
        if (!visemeController_.getCurrentViseme().empty()) {
            visemeController_.applyToFacialRig(controller_);
        }
    }
    
    // Apply to output
    void applyToBlendShapeMesh(BlendShapeMesh& mesh) const {
        controller_.applyToBlendShapeMesh(mesh);
    }
    
    void applyToSkeleton(Skeleton& skeleton) const {
        controller_.applyToSkeleton(skeleton);
    }
    
    // Quick expression setting
    void setExpression(const std::string& name, float intensity = 1.0f) {
        controller_.setExpression(name, intensity);
    }
    
    // Quick eye gaze
    void lookAt(const Vec3& target, const Vec3& headPos, const Quat& headRot) {
        controller_.lookAt(target, headPos, headRot);
    }
    
    // Reset to neutral
    void reset() {
        controller_.reset();
        visemeController_.setViseme("", 0.0f);
    }
    
private:
    FacialRigController controller_;
    VisemeController visemeController_;
};

}  // namespace luma
