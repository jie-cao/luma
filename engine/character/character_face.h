// Character Face System - Parametric face customization and photo-based generation
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <array>

namespace luma {

// ============================================================================
// Face Region Enums
// ============================================================================

enum class FaceRegion {
    Overall,        // Overall face shape
    Forehead,
    Eyes,
    Eyebrows,
    Nose,
    Mouth,
    Chin,
    Jaw,
    Cheeks,
    Ears
};

// ============================================================================
// Face Shape Parameters
// ============================================================================

struct FaceShapeParams {
    // Overall face
    float faceWidth = 0.5f;           // Narrow to wide
    float faceLength = 0.5f;          // Short to long
    float faceRoundness = 0.5f;       // Angular to round
    
    // Forehead
    float foreheadHeight = 0.5f;
    float foreheadWidth = 0.5f;
    float foreheadSlope = 0.5f;       // Flat to sloped
    
    // Eyes
    float eyeSize = 0.5f;
    float eyeWidth = 0.5f;            // Narrow to wide
    float eyeHeight = 0.5f;           // Height position on face
    float eyeSpacing = 0.5f;          // Close to far apart
    float eyeAngle = 0.5f;            // Downward to upward tilt
    float eyeDepth = 0.5f;            // Shallow to deep set
    float upperEyelid = 0.5f;         // Eyelid fold
    float lowerEyelid = 0.5f;
    float eyeCornerInner = 0.5f;      // Inner corner shape
    float eyeCornerOuter = 0.5f;      // Outer corner shape
    
    // Eyebrows
    float browHeight = 0.5f;          // Low to high
    float browThickness = 0.5f;
    float browLength = 0.5f;
    float browAngle = 0.5f;           // Downward to arched
    float browSpacing = 0.5f;         // Distance from eyes
    float browCurve = 0.5f;           // Straight to curved
    
    // Nose
    float noseLength = 0.5f;
    float noseWidth = 0.5f;           // Narrow to wide
    float noseHeight = 0.5f;          // Flat to prominent
    float noseBridge = 0.5f;          // Bridge width/height
    float noseBridgeCurve = 0.5f;     // Straight to curved
    float noseTip = 0.5f;             // Tip shape (pointed to round)
    float noseTipAngle = 0.5f;        // Upturned to downturned
    float nostrilWidth = 0.5f;
    float nostrilFlare = 0.5f;
    
    // Mouth
    float mouthWidth = 0.5f;
    float mouthHeight = 0.5f;         // Vertical position
    float upperLipThickness = 0.5f;
    float lowerLipThickness = 0.5f;
    float lipProtrusion = 0.5f;       // How much lips stick out
    float mouthCorners = 0.5f;        // Downturned to upturned
    float philtrum = 0.5f;            // Philtrum depth
    float lipCurve = 0.5f;            // Cupid's bow definition
    
    // Chin and Jaw
    float chinLength = 0.5f;
    float chinWidth = 0.5f;
    float chinProtrusion = 0.5f;      // Receding to protruding
    float chinShape = 0.5f;           // Pointed to square
    float chinCleft = 0.0f;           // Cleft chin depth
    float jawWidth = 0.5f;
    float jawAngle = 0.5f;            // Jaw angle definition
    float jawLine = 0.5f;             // Soft to defined
    
    // Cheeks
    float cheekboneHeight = 0.5f;
    float cheekboneWidth = 0.5f;
    float cheekboneProminence = 0.5f;
    float cheekFullness = 0.5f;       // Hollow to full
    float cheekFat = 0.5f;
    
    // Ears
    float earSize = 0.5f;
    float earAngle = 0.5f;            // Close to protruding
    float earLobe = 0.5f;             // Attached to free
    float earPointiness = 0.5f;       // Round to pointed (for fantasy)
    
    // Get all parameter names and values for iteration
    std::vector<std::pair<std::string, float*>> getAllParams() {
        return {
            {"face_width", &faceWidth},
            {"face_length", &faceLength},
            {"face_roundness", &faceRoundness},
            {"forehead_height", &foreheadHeight},
            {"forehead_width", &foreheadWidth},
            {"forehead_slope", &foreheadSlope},
            {"eye_size", &eyeSize},
            {"eye_width", &eyeWidth},
            {"eye_height", &eyeHeight},
            {"eye_spacing", &eyeSpacing},
            {"eye_angle", &eyeAngle},
            {"eye_depth", &eyeDepth},
            {"upper_eyelid", &upperEyelid},
            {"lower_eyelid", &lowerEyelid},
            {"eye_corner_inner", &eyeCornerInner},
            {"eye_corner_outer", &eyeCornerOuter},
            {"brow_height", &browHeight},
            {"brow_thickness", &browThickness},
            {"brow_length", &browLength},
            {"brow_angle", &browAngle},
            {"brow_spacing", &browSpacing},
            {"brow_curve", &browCurve},
            {"nose_length", &noseLength},
            {"nose_width", &noseWidth},
            {"nose_height", &noseHeight},
            {"nose_bridge", &noseBridge},
            {"nose_bridge_curve", &noseBridgeCurve},
            {"nose_tip", &noseTip},
            {"nose_tip_angle", &noseTipAngle},
            {"nostril_width", &nostrilWidth},
            {"nostril_flare", &nostrilFlare},
            {"mouth_width", &mouthWidth},
            {"mouth_height", &mouthHeight},
            {"upper_lip_thickness", &upperLipThickness},
            {"lower_lip_thickness", &lowerLipThickness},
            {"lip_protrusion", &lipProtrusion},
            {"mouth_corners", &mouthCorners},
            {"philtrum", &philtrum},
            {"lip_curve", &lipCurve},
            {"chin_length", &chinLength},
            {"chin_width", &chinWidth},
            {"chin_protrusion", &chinProtrusion},
            {"chin_shape", &chinShape},
            {"chin_cleft", &chinCleft},
            {"jaw_width", &jawWidth},
            {"jaw_angle", &jawAngle},
            {"jaw_line", &jawLine},
            {"cheekbone_height", &cheekboneHeight},
            {"cheekbone_width", &cheekboneWidth},
            {"cheekbone_prominence", &cheekboneProminence},
            {"cheek_fullness", &cheekFullness},
            {"cheek_fat", &cheekFat},
            {"ear_size", &earSize},
            {"ear_angle", &earAngle},
            {"ear_lobe", &earLobe},
            {"ear_pointiness", &earPointiness}
        };
    }
    
    // Reset all to defaults
    void reset() {
        for (auto& [name, ptr] : getAllParams()) {
            *ptr = 0.5f;
        }
        chinCleft = 0.0f;
        earPointiness = 0.5f;
    }
    
    // Interpolate between two face shapes
    static FaceShapeParams lerp(const FaceShapeParams& a, 
                                 const FaceShapeParams& b, 
                                 float t) {
        FaceShapeParams result;
        auto paramsA = const_cast<FaceShapeParams&>(a).getAllParams();
        auto paramsB = const_cast<FaceShapeParams&>(b).getAllParams();
        auto paramsR = result.getAllParams();
        
        for (size_t i = 0; i < paramsR.size(); i++) {
            *paramsR[i].second = *paramsA[i].second + 
                (*paramsB[i].second - *paramsA[i].second) * t;
        }
        return result;
    }
};

// ============================================================================
// ARKit Compatible Expression Parameters (52 blend shapes)
// ============================================================================

struct FaceExpressionParams {
    // Eyes
    float eyeBlinkLeft = 0.0f;
    float eyeBlinkRight = 0.0f;
    float eyeLookDownLeft = 0.0f;
    float eyeLookDownRight = 0.0f;
    float eyeLookInLeft = 0.0f;
    float eyeLookInRight = 0.0f;
    float eyeLookOutLeft = 0.0f;
    float eyeLookOutRight = 0.0f;
    float eyeLookUpLeft = 0.0f;
    float eyeLookUpRight = 0.0f;
    float eyeSquintLeft = 0.0f;
    float eyeSquintRight = 0.0f;
    float eyeWideLeft = 0.0f;
    float eyeWideRight = 0.0f;
    
    // Jaw
    float jawForward = 0.0f;
    float jawLeft = 0.0f;
    float jawRight = 0.0f;
    float jawOpen = 0.0f;
    
    // Mouth
    float mouthClose = 0.0f;
    float mouthFunnel = 0.0f;
    float mouthPucker = 0.0f;
    float mouthLeft = 0.0f;
    float mouthRight = 0.0f;
    float mouthSmileLeft = 0.0f;
    float mouthSmileRight = 0.0f;
    float mouthFrownLeft = 0.0f;
    float mouthFrownRight = 0.0f;
    float mouthDimpleLeft = 0.0f;
    float mouthDimpleRight = 0.0f;
    float mouthStretchLeft = 0.0f;
    float mouthStretchRight = 0.0f;
    float mouthRollLower = 0.0f;
    float mouthRollUpper = 0.0f;
    float mouthShrugLower = 0.0f;
    float mouthShrugUpper = 0.0f;
    float mouthPressLeft = 0.0f;
    float mouthPressRight = 0.0f;
    float mouthLowerDownLeft = 0.0f;
    float mouthLowerDownRight = 0.0f;
    float mouthUpperUpLeft = 0.0f;
    float mouthUpperUpRight = 0.0f;
    
    // Brow
    float browDownLeft = 0.0f;
    float browDownRight = 0.0f;
    float browInnerUp = 0.0f;
    float browOuterUpLeft = 0.0f;
    float browOuterUpRight = 0.0f;
    
    // Cheek
    float cheekPuff = 0.0f;
    float cheekSquintLeft = 0.0f;
    float cheekSquintRight = 0.0f;
    
    // Nose
    float noseSneerLeft = 0.0f;
    float noseSneerRight = 0.0f;
    
    // Tongue
    float tongueOut = 0.0f;
    
    // Get all ARKit blend shape names
    static std::vector<std::string> getARKitBlendShapeNames() {
        return {
            "eyeBlinkLeft", "eyeBlinkRight",
            "eyeLookDownLeft", "eyeLookDownRight",
            "eyeLookInLeft", "eyeLookInRight",
            "eyeLookOutLeft", "eyeLookOutRight",
            "eyeLookUpLeft", "eyeLookUpRight",
            "eyeSquintLeft", "eyeSquintRight",
            "eyeWideLeft", "eyeWideRight",
            "jawForward", "jawLeft", "jawRight", "jawOpen",
            "mouthClose", "mouthFunnel", "mouthPucker",
            "mouthLeft", "mouthRight",
            "mouthSmileLeft", "mouthSmileRight",
            "mouthFrownLeft", "mouthFrownRight",
            "mouthDimpleLeft", "mouthDimpleRight",
            "mouthStretchLeft", "mouthStretchRight",
            "mouthRollLower", "mouthRollUpper",
            "mouthShrugLower", "mouthShrugUpper",
            "mouthPressLeft", "mouthPressRight",
            "mouthLowerDownLeft", "mouthLowerDownRight",
            "mouthUpperUpLeft", "mouthUpperUpRight",
            "browDownLeft", "browDownRight", "browInnerUp",
            "browOuterUpLeft", "browOuterUpRight",
            "cheekPuff", "cheekSquintLeft", "cheekSquintRight",
            "noseSneerLeft", "noseSneerRight",
            "tongueOut"
        };
    }
    
    // Reset all expressions to neutral
    void reset() {
        eyeBlinkLeft = eyeBlinkRight = 0.0f;
        eyeLookDownLeft = eyeLookDownRight = 0.0f;
        eyeLookInLeft = eyeLookInRight = 0.0f;
        eyeLookOutLeft = eyeLookOutRight = 0.0f;
        eyeLookUpLeft = eyeLookUpRight = 0.0f;
        eyeSquintLeft = eyeSquintRight = 0.0f;
        eyeWideLeft = eyeWideRight = 0.0f;
        jawForward = jawLeft = jawRight = jawOpen = 0.0f;
        mouthClose = mouthFunnel = mouthPucker = 0.0f;
        mouthLeft = mouthRight = 0.0f;
        mouthSmileLeft = mouthSmileRight = 0.0f;
        mouthFrownLeft = mouthFrownRight = 0.0f;
        mouthDimpleLeft = mouthDimpleRight = 0.0f;
        mouthStretchLeft = mouthStretchRight = 0.0f;
        mouthRollLower = mouthRollUpper = 0.0f;
        mouthShrugLower = mouthShrugUpper = 0.0f;
        mouthPressLeft = mouthPressRight = 0.0f;
        mouthLowerDownLeft = mouthLowerDownRight = 0.0f;
        mouthUpperUpLeft = mouthUpperUpRight = 0.0f;
        browDownLeft = browDownRight = browInnerUp = 0.0f;
        browOuterUpLeft = browOuterUpRight = 0.0f;
        cheekPuff = cheekSquintLeft = cheekSquintRight = 0.0f;
        noseSneerLeft = noseSneerRight = 0.0f;
        tongueOut = 0.0f;
    }
    
    // Apply a preset expression
    void applySmile(float intensity = 1.0f) {
        mouthSmileLeft = mouthSmileRight = intensity * 0.8f;
        cheekSquintLeft = cheekSquintRight = intensity * 0.3f;
        eyeSquintLeft = eyeSquintRight = intensity * 0.2f;
    }
    
    void applyFrown(float intensity = 1.0f) {
        mouthFrownLeft = mouthFrownRight = intensity * 0.7f;
        browDownLeft = browDownRight = intensity * 0.5f;
    }
    
    void applySurprise(float intensity = 1.0f) {
        eyeWideLeft = eyeWideRight = intensity * 0.8f;
        browInnerUp = intensity * 0.6f;
        browOuterUpLeft = browOuterUpRight = intensity * 0.5f;
        jawOpen = intensity * 0.4f;
    }
    
    void applyAngry(float intensity = 1.0f) {
        browDownLeft = browDownRight = intensity * 0.8f;
        eyeSquintLeft = eyeSquintRight = intensity * 0.4f;
        noseSneerLeft = noseSneerRight = intensity * 0.3f;
        jawForward = intensity * 0.2f;
    }
};

// ============================================================================
// Face Texture Parameters
// ============================================================================

struct FaceTextureParams {
    // Skin
    Vec3 skinTone{0.85f, 0.65f, 0.5f};      // Base skin color
    float skinSaturation = 0.5f;             // Color saturation
    float skinBrightness = 0.5f;             // Overall brightness
    float skinRoughness = 0.5f;              // PBR roughness
    float skinSubsurface = 0.3f;             // Subsurface scattering
    
    // Skin details
    float freckles = 0.0f;                   // Freckle intensity
    float moles = 0.0f;                      // Mole visibility
    float wrinkles = 0.0f;                   // Wrinkle depth
    float pores = 0.3f;                      // Pore visibility
    float blemishes = 0.0f;                  // Skin blemishes
    
    // Facial hair (for applicable characters)
    float stubble = 0.0f;                    // 5 o'clock shadow
    float beard = 0.0f;                      // Full beard
    float mustache = 0.0f;                   // Mustache
    Vec3 facialHairColor{0.2f, 0.15f, 0.1f};
    
    // Makeup (for applicable characters)
    float eyeshadow = 0.0f;
    Vec3 eyeshadowColor{0.3f, 0.2f, 0.4f};
    float eyeliner = 0.0f;
    float blush = 0.0f;
    Vec3 blushColor{0.9f, 0.5f, 0.5f};
    float lipstick = 0.0f;
    Vec3 lipstickColor{0.8f, 0.2f, 0.3f};
    
    // Eyes
    Vec3 eyeColor{0.4f, 0.3f, 0.2f};        // Iris color
    float eyeColorVariation = 0.3f;          // Color variation in iris
    float pupilSize = 0.5f;                  // Pupil dilation
    Vec3 scleraColor{0.95f, 0.95f, 0.92f};  // White of eye
    float scleraRedness = 0.0f;              // Bloodshot effect
    
    // Eyebrows
    Vec3 eyebrowColor{0.2f, 0.15f, 0.1f};
    float eyebrowDensity = 0.7f;
    
    // Lips
    Vec3 lipColor{0.75f, 0.45f, 0.45f};     // Natural lip color
    float lipMoisture = 0.5f;                // Glossiness
};

// ============================================================================
// Photo-to-Face Result (from AI pipeline)
// ============================================================================

struct PhotoFaceResult {
    bool success = false;
    std::string errorMessage;
    
    // 3DMM shape parameters (FLAME model compatible)
    std::vector<float> shapeParams;          // ~300 parameters
    std::vector<float> expressionParams;     // ~100 parameters
    
    // Estimated pose
    Vec3 headRotation;                       // Euler angles (pitch, yaw, roll)
    Vec3 headTranslation;
    
    // Estimated lighting (for texture extraction)
    std::vector<float> lightingParams;       // Spherical harmonics coefficients
    
    // Extracted texture (UV mapped)
    std::vector<uint8_t> textureData;
    int textureWidth = 0;
    int textureHeight = 0;
    
    // Confidence scores
    float overallConfidence = 0.0f;
    float poseConfidence = 0.0f;
    float expressionConfidence = 0.0f;
    
    // Detected landmarks (468 points from MediaPipe)
    std::vector<Vec3> landmarks;
};

// ============================================================================
// Character Face - Main face management class
// ============================================================================

class CharacterFace {
public:
    CharacterFace() = default;
    
    // === Shape Parameters ===
    
    void setShapeParams(const FaceShapeParams& params) {
        shapeParams_ = params;
        updateBlendShapeWeights();
    }
    
    const FaceShapeParams& getShapeParams() const { return shapeParams_; }
    FaceShapeParams& getShapeParams() { return shapeParams_; }
    
    // === Expression Parameters ===
    
    void setExpressionParams(const FaceExpressionParams& params) {
        expressionParams_ = params;
        updateExpressionWeights();
    }
    
    const FaceExpressionParams& getExpressionParams() const { return expressionParams_; }
    FaceExpressionParams& getExpressionParams() { return expressionParams_; }
    
    // === Texture Parameters ===
    
    void setTextureParams(const FaceTextureParams& params) {
        textureParams_ = params;
        texturesDirty_ = true;
    }
    
    const FaceTextureParams& getTextureParams() const { return textureParams_; }
    FaceTextureParams& getTextureParams() { return textureParams_; }
    
    // === Quick Setters (common adjustments) ===
    
    void setEyeSize(float size) { 
        shapeParams_.eyeSize = size; 
        updateBlendShapeWeights(); 
    }
    
    void setNoseLength(float length) { 
        shapeParams_.noseLength = length; 
        updateBlendShapeWeights(); 
    }
    
    void setMouthWidth(float width) { 
        shapeParams_.mouthWidth = width; 
        updateBlendShapeWeights(); 
    }
    
    void setJawWidth(float width) { 
        shapeParams_.jawWidth = width; 
        updateBlendShapeWeights(); 
    }
    
    void setSkinTone(const Vec3& color) {
        textureParams_.skinTone = color;
        texturesDirty_ = true;
    }
    
    void setEyeColor(const Vec3& color) {
        textureParams_.eyeColor = color;
        texturesDirty_ = true;
    }
    
    // === BlendShape Integration ===
    
    void setBlendShapeMesh(BlendShapeMesh* mesh) {
        blendShapeMesh_ = mesh;
        updateBlendShapeWeights();
        updateExpressionWeights();
    }
    
    BlendShapeMesh* getBlendShapeMesh() { return blendShapeMesh_; }
    
    // Setup mapping from face params to blend shape channels
    void setupDefaultMappings() {
        shapeMappings_.clear();
        
        // Face shape mappings
        addShapeMapping("face_width", "faceWidth");
        addShapeMapping("face_length", "faceLength");
        addShapeMapping("face_roundness", "faceRoundness");
        
        // Eye mappings
        addShapeMapping("eye_size", "eyeSize");
        addShapeMapping("eye_spacing", "eyeSpacing");
        addShapeMapping("eye_height", "eyeHeight");
        addShapeMapping("eye_angle", "eyeAngle");
        addShapeMapping("eye_depth", "eyeDepth");
        addShapeMapping("upper_eyelid", "upperEyelid");
        addShapeMapping("lower_eyelid", "lowerEyelid");
        
        // Eyebrow mappings
        addShapeMapping("brow_height", "browHeight");
        addShapeMapping("brow_angle", "browAngle");
        addShapeMapping("brow_thickness", "browThickness");
        
        // Nose mappings
        addShapeMapping("nose_length", "noseLength");
        addShapeMapping("nose_width", "noseWidth");
        addShapeMapping("nose_height", "noseHeight");
        addShapeMapping("nose_bridge", "noseBridge");
        addShapeMapping("nose_tip", "noseTip");
        addShapeMapping("nostril_width", "nostrilWidth");
        
        // Mouth mappings
        addShapeMapping("mouth_width", "mouthWidth");
        addShapeMapping("upper_lip", "upperLipThickness");
        addShapeMapping("lower_lip", "lowerLipThickness");
        addShapeMapping("lip_protrusion", "lipProtrusion");
        
        // Chin/Jaw mappings
        addShapeMapping("chin_length", "chinLength");
        addShapeMapping("chin_width", "chinWidth");
        addShapeMapping("chin_protrusion", "chinProtrusion");
        addShapeMapping("jaw_width", "jawWidth");
        addShapeMapping("jaw_line", "jawLine");
        
        // Cheek mappings
        addShapeMapping("cheekbone_prominence", "cheekboneProminence");
        addShapeMapping("cheek_fullness", "cheekFullness");
        
        // Ear mappings
        addShapeMapping("ear_size", "earSize");
        addShapeMapping("ear_angle", "earAngle");
    }
    
    // === Photo-to-Face ===
    
    // Apply results from AI face reconstruction
    void applyPhotoFaceResult(const PhotoFaceResult& result) {
        if (!result.success) return;
        
        // Map 3DMM parameters to our face shape params
        // This requires a trained mapping or heuristic conversion
        mapDMMtoFaceParams(result.shapeParams);
        
        // Store the photo texture
        if (!result.textureData.empty()) {
            photoTexture_ = result.textureData;
            photoTextureWidth_ = result.textureWidth;
            photoTextureHeight_ = result.textureHeight;
            hasPhotoTexture_ = true;
        }
        
        updateBlendShapeWeights();
    }
    
    bool hasPhotoTexture() const { return hasPhotoTexture_; }
    
    const std::vector<uint8_t>& getPhotoTexture() const { return photoTexture_; }
    int getPhotoTextureWidth() const { return photoTextureWidth_; }
    int getPhotoTextureHeight() const { return photoTextureHeight_; }
    
    // === Preset Expressions ===
    
    void setExpression(const std::string& name, float intensity = 1.0f) {
        expressionParams_.reset();
        
        if (name == "neutral") {
            // Already reset
        } else if (name == "smile") {
            expressionParams_.applySmile(intensity);
        } else if (name == "frown") {
            expressionParams_.applyFrown(intensity);
        } else if (name == "surprise") {
            expressionParams_.applySurprise(intensity);
        } else if (name == "angry") {
            expressionParams_.applyAngry(intensity);
        }
        
        updateExpressionWeights();
    }
    
    // === Serialization ===
    
    void serialize(std::unordered_map<std::string, float>& outData) const {
        // Serialize shape params
        auto shapeParamList = const_cast<FaceShapeParams&>(shapeParams_).getAllParams();
        for (const auto& [name, ptr] : shapeParamList) {
            outData["shape_" + name] = *ptr;
        }
        
        // Serialize texture params
        outData["tex_skin_r"] = textureParams_.skinTone.x;
        outData["tex_skin_g"] = textureParams_.skinTone.y;
        outData["tex_skin_b"] = textureParams_.skinTone.z;
        outData["tex_eye_r"] = textureParams_.eyeColor.x;
        outData["tex_eye_g"] = textureParams_.eyeColor.y;
        outData["tex_eye_b"] = textureParams_.eyeColor.z;
        outData["tex_wrinkles"] = textureParams_.wrinkles;
        outData["tex_freckles"] = textureParams_.freckles;
    }
    
    void deserialize(const std::unordered_map<std::string, float>& data) {
        auto get = [&](const std::string& key, float def) {
            auto it = data.find(key);
            return (it != data.end()) ? it->second : def;
        };
        
        // Deserialize shape params
        auto shapeParamList = shapeParams_.getAllParams();
        for (auto& [name, ptr] : shapeParamList) {
            *ptr = get("shape_" + name, 0.5f);
        }
        
        // Deserialize texture params
        textureParams_.skinTone.x = get("tex_skin_r", 0.85f);
        textureParams_.skinTone.y = get("tex_skin_g", 0.65f);
        textureParams_.skinTone.z = get("tex_skin_b", 0.5f);
        textureParams_.eyeColor.x = get("tex_eye_r", 0.4f);
        textureParams_.eyeColor.y = get("tex_eye_g", 0.3f);
        textureParams_.eyeColor.z = get("tex_eye_b", 0.2f);
        textureParams_.wrinkles = get("tex_wrinkles", 0.0f);
        textureParams_.freckles = get("tex_freckles", 0.0f);
        
        updateBlendShapeWeights();
    }
    
    // === State ===
    
    bool isTexturesDirty() const { return texturesDirty_; }
    void clearTexturesDirty() { texturesDirty_ = false; }
    
    // Update BlendShape weights based on current parameters
    void applyParameters() {
        updateBlendShapeWeights();
        updateExpressionWeights();
    }
    
private:
    FaceShapeParams shapeParams_;
    FaceExpressionParams expressionParams_;
    FaceTextureParams textureParams_;
    
    BlendShapeMesh* blendShapeMesh_ = nullptr;
    
    // Mapping from face param names to blend shape channel names
    std::vector<std::pair<std::string, std::string>> shapeMappings_;
    
    // Photo texture
    bool hasPhotoTexture_ = false;
    std::vector<uint8_t> photoTexture_;
    int photoTextureWidth_ = 0;
    int photoTextureHeight_ = 0;
    
    bool texturesDirty_ = true;
    
    void addShapeMapping(const std::string& blendShapeName, const std::string& paramName) {
        shapeMappings_.push_back({blendShapeName, paramName});
    }
    
    void updateBlendShapeWeights() {
        if (!blendShapeMesh_) return;
        
        auto paramList = shapeParams_.getAllParams();
        std::unordered_map<std::string, float> paramMap;
        for (const auto& [name, ptr] : paramList) {
            paramMap[name] = *ptr;
        }
        
        for (const auto& [bsName, paramName] : shapeMappings_) {
            auto it = paramMap.find(paramName);
            if (it != paramMap.end()) {
                // Convert 0-1 range to -1 to 1 for blend shapes (centered at 0.5)
                float weight = (it->second - 0.5f) * 2.0f;
                blendShapeMesh_->setWeight(bsName, weight);
            }
        }
    }
    
    void updateExpressionWeights() {
        if (!blendShapeMesh_) return;
        
        // Map ARKit expression names directly to blend shape channels
        auto names = FaceExpressionParams::getARKitBlendShapeNames();
        float* values = reinterpret_cast<float*>(&expressionParams_);
        
        for (size_t i = 0; i < names.size(); i++) {
            blendShapeMesh_->setWeight(names[i], values[i]);
        }
    }
    
    // Map 3DMM parameters to our face shape params
    // This is a simplified mapping - a real implementation would use
    // learned coefficients or a neural network
    void mapDMMtoFaceParams(const std::vector<float>& dmmParams) {
        if (dmmParams.size() < 10) return;
        
        // Simplified mapping (would be more sophisticated in production)
        // 3DMM parameters typically control:
        // - First ~80 params: identity/shape
        // - Next ~64 params: expression
        // - Additional params: jaw pose, eye gaze, etc.
        
        // Very rough mapping
        shapeParams_.faceWidth = 0.5f + dmmParams[0] * 0.1f;
        shapeParams_.faceLength = 0.5f + dmmParams[1] * 0.1f;
        shapeParams_.eyeSize = 0.5f + dmmParams[2] * 0.1f;
        shapeParams_.noseLength = 0.5f + dmmParams[3] * 0.1f;
        shapeParams_.mouthWidth = 0.5f + dmmParams[4] * 0.1f;
        shapeParams_.jawWidth = 0.5f + dmmParams[5] * 0.1f;
        shapeParams_.cheekboneProminence = 0.5f + dmmParams[6] * 0.1f;
        shapeParams_.chinLength = 0.5f + dmmParams[7] * 0.1f;
        // ... more mappings in a real implementation
    }
};

// ============================================================================
// Face Preset Library
// ============================================================================

class FacePresetLibrary {
public:
    struct PresetEntry {
        std::string name;
        std::string category;           // "Male", "Female", "Fantasy", etc.
        FaceShapeParams shapeParams;
        FaceTextureParams textureParams;
        std::string thumbnailPath;
    };
    
    void addPreset(const PresetEntry& entry) {
        presets_.push_back(entry);
        categoryIndex_[entry.category].push_back(presets_.size() - 1);
    }
    
    const std::vector<PresetEntry>& getAllPresets() const { return presets_; }
    
    std::vector<const PresetEntry*> getPresetsByCategory(const std::string& category) const {
        std::vector<const PresetEntry*> result;
        auto it = categoryIndex_.find(category);
        if (it != categoryIndex_.end()) {
            for (size_t idx : it->second) {
                result.push_back(&presets_[idx]);
            }
        }
        return result;
    }
    
    const PresetEntry* findPreset(const std::string& name) const {
        for (const auto& p : presets_) {
            if (p.name == name) return &p;
        }
        return nullptr;
    }
    
    // Initialize with some default face presets
    void initializeDefaults() {
        // Male faces
        {
            PresetEntry p;
            p.name = "Average Male";
            p.category = "Male";
            p.shapeParams.jawWidth = 0.55f;
            p.shapeParams.browHeight = 0.45f;
            p.shapeParams.cheekboneProminence = 0.5f;
            addPreset(p);
        }
        
        {
            PresetEntry p;
            p.name = "Strong Male";
            p.category = "Male";
            p.shapeParams.jawWidth = 0.7f;
            p.shapeParams.jawLine = 0.7f;
            p.shapeParams.browHeight = 0.4f;
            p.shapeParams.cheekboneProminence = 0.6f;
            addPreset(p);
        }
        
        // Female faces
        {
            PresetEntry p;
            p.name = "Average Female";
            p.category = "Female";
            p.shapeParams.jawWidth = 0.4f;
            p.shapeParams.faceRoundness = 0.55f;
            p.shapeParams.eyeSize = 0.55f;
            p.shapeParams.lipProtrusion = 0.55f;
            addPreset(p);
        }
        
        {
            PresetEntry p;
            p.name = "Soft Female";
            p.category = "Female";
            p.shapeParams.jawWidth = 0.35f;
            p.shapeParams.faceRoundness = 0.65f;
            p.shapeParams.eyeSize = 0.6f;
            p.shapeParams.cheekFullness = 0.55f;
            addPreset(p);
        }
        
        // Fantasy
        {
            PresetEntry p;
            p.name = "Elf";
            p.category = "Fantasy";
            p.shapeParams.faceLength = 0.6f;
            p.shapeParams.eyeSize = 0.6f;
            p.shapeParams.eyeAngle = 0.55f;
            p.shapeParams.earPointiness = 0.8f;
            p.shapeParams.jawWidth = 0.4f;
            p.shapeParams.cheekboneProminence = 0.65f;
            addPreset(p);
        }
    }
    
private:
    std::vector<PresetEntry> presets_;
    std::unordered_map<std::string, std::vector<size_t>> categoryIndex_;
};

} // namespace luma
