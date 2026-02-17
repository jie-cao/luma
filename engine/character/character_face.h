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
    // Setup mapping from FaceShapeParams to FaceTemplateMesh blend shape channels.
    // First arg = blend shape channel name (camelCase, matches FaceTemplateMesh),
    // second arg = parameter key (underscore, matches FaceShapeParams::getAllParams()).
    void setupDefaultMappings() {
        shapeMappings_.clear();
        
        // Overall face
        addShapeMapping("faceWidth", "face_width");
        addShapeMapping("faceLength", "face_length");
        addShapeMapping("faceRoundness", "face_roundness");
        
        // Forehead
        addShapeMapping("foreheadHeight", "forehead_height");
        addShapeMapping("foreheadWidth", "forehead_width");
        addShapeMapping("foreheadSlope", "forehead_slope");
        
        // Eyes
        addShapeMapping("eyeSize", "eye_size");
        addShapeMapping("eyeWidth", "eye_width");
        addShapeMapping("eyeHeight", "eye_height");
        addShapeMapping("eyeSpacing", "eye_spacing");
        addShapeMapping("eyeAngle", "eye_angle");
        addShapeMapping("eyeDepth", "eye_depth");
        addShapeMapping("upperEyelid", "upper_eyelid");
        addShapeMapping("lowerEyelid", "lower_eyelid");
        addShapeMapping("eyeCornerInner", "eye_corner_inner");
        addShapeMapping("eyeCornerOuter", "eye_corner_outer");
        
        // Eyebrows
        addShapeMapping("browHeight", "brow_height");
        addShapeMapping("browThickness", "brow_thickness");
        addShapeMapping("browLength", "brow_length");
        addShapeMapping("browAngle", "brow_angle");
        addShapeMapping("browSpacing", "brow_spacing");
        addShapeMapping("browCurve", "brow_curve");
        
        // Nose
        addShapeMapping("noseLength", "nose_length");
        addShapeMapping("noseWidth", "nose_width");
        addShapeMapping("noseHeight", "nose_height");
        addShapeMapping("noseBridge", "nose_bridge");
        addShapeMapping("noseBridgeCurve", "nose_bridge_curve");
        addShapeMapping("noseTip", "nose_tip");
        addShapeMapping("noseTipAngle", "nose_tip_angle");
        addShapeMapping("nostrilWidth", "nostril_width");
        addShapeMapping("nostrilFlare", "nostril_flare");
        
        // Mouth
        addShapeMapping("mouthWidth", "mouth_width");
        addShapeMapping("mouthHeight", "mouth_height");
        addShapeMapping("upperLipThickness", "upper_lip_thickness");
        addShapeMapping("lowerLipThickness", "lower_lip_thickness");
        addShapeMapping("lipProtrusion", "lip_protrusion");
        addShapeMapping("mouthCorners", "mouth_corners");
        addShapeMapping("philtrum", "philtrum");
        addShapeMapping("lipCurve", "lip_curve");
        
        // Chin
        addShapeMapping("chinLength", "chin_length");
        addShapeMapping("chinWidth", "chin_width");
        addShapeMapping("chinProtrusion", "chin_protrusion");
        addShapeMapping("chinShape", "chin_shape");
        addShapeMapping("chinCleft", "chin_cleft");
        
        // Jaw
        addShapeMapping("jawWidth", "jaw_width");
        addShapeMapping("jawAngle", "jaw_angle");
        addShapeMapping("jawLine", "jaw_line");
        
        // Cheeks
        addShapeMapping("cheekboneHeight", "cheekbone_height");
        addShapeMapping("cheekboneWidth", "cheekbone_width");
        addShapeMapping("cheekboneProminence", "cheekbone_prominence");
        addShapeMapping("cheekFullness", "cheek_fullness");
        addShapeMapping("cheekFat", "cheek_fat");
        
        // Ears
        addShapeMapping("earSize", "ear_size");
        addShapeMapping("earAngle", "ear_angle");
        addShapeMapping("earLobe", "ear_lobe");
        addShapeMapping("earPointiness", "ear_pointiness");
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
    
    // === Face Region Grouping ===
    
    // Get parameter names for a specific face region
    static std::vector<std::string> getParamsForRegion(FaceRegion region) {
        switch (region) {
            case FaceRegion::Overall:
                return {"face_width", "face_length", "face_roundness"};
            case FaceRegion::Forehead:
                return {"forehead_height", "forehead_width", "forehead_slope"};
            case FaceRegion::Eyes:
                return {"eye_size", "eye_width", "eye_height", "eye_spacing", 
                        "eye_angle", "eye_depth", "upper_eyelid", "lower_eyelid",
                        "eye_corner_inner", "eye_corner_outer"};
            case FaceRegion::Eyebrows:
                return {"brow_height", "brow_thickness", "brow_length", 
                        "brow_angle", "brow_spacing", "brow_curve"};
            case FaceRegion::Nose:
                return {"nose_length", "nose_width", "nose_height", "nose_bridge",
                        "nose_bridge_curve", "nose_tip", "nose_tip_angle",
                        "nostril_width", "nostril_flare"};
            case FaceRegion::Mouth:
                return {"mouth_width", "mouth_height", "upper_lip_thickness",
                        "lower_lip_thickness", "lip_protrusion", "mouth_corners",
                        "philtrum", "lip_curve"};
            case FaceRegion::Chin:
                return {"chin_length", "chin_width", "chin_protrusion",
                        "chin_shape", "chin_cleft"};
            case FaceRegion::Jaw:
                return {"jaw_width", "jaw_angle", "jaw_line"};
            case FaceRegion::Cheeks:
                return {"cheekbone_height", "cheekbone_width", "cheekbone_prominence",
                        "cheek_fullness", "cheek_fat"};
            case FaceRegion::Ears:
                return {"ear_size", "ear_angle", "ear_lobe", "ear_pointiness"};
            default:
                return {};
        }
    }
    
    // Get display name for a face region
    static const char* getRegionName(FaceRegion region) {
        switch (region) {
            case FaceRegion::Overall:  return "Overall";
            case FaceRegion::Forehead: return "Forehead";
            case FaceRegion::Eyes:     return "Eyes";
            case FaceRegion::Eyebrows: return "Eyebrows";
            case FaceRegion::Nose:     return "Nose";
            case FaceRegion::Mouth:    return "Mouth";
            case FaceRegion::Chin:     return "Chin";
            case FaceRegion::Jaw:      return "Jaw";
            case FaceRegion::Cheeks:   return "Cheeks";
            case FaceRegion::Ears:     return "Ears";
            default: return "Unknown";
        }
    }
    
    // === Parameter Constraints and Linked Parameters ===
    
    // Apply constraints after parameter change to maintain plausible face shapes
    void applyConstraints() {
        auto& p = shapeParams_;
        
        // Jaw width should generally correlate with face width
        // (wide face tends to have wider jaw)
        float jawMin = std::max(0.0f, p.faceWidth - 0.3f);
        float jawMax = std::min(1.0f, p.faceWidth + 0.3f);
        p.jawWidth = std::max(jawMin, std::min(jawMax, p.jawWidth));
        
        // Chin width should be less than jaw width
        if (p.chinWidth > p.jawWidth + 0.15f) {
            p.chinWidth = p.jawWidth + 0.15f;
        }
        
        // Lip protrusion should somewhat correlate with lip thickness
        float avgLip = (p.upperLipThickness + p.lowerLipThickness) * 0.5f;
        if (p.lipProtrusion < avgLip - 0.3f) {
            p.lipProtrusion = avgLip - 0.3f;
        }
        
        // Clamp all values to valid range
        auto allParams = p.getAllParams();
        for (auto& [name, ptr] : allParams) {
            *ptr = std::max(0.0f, std::min(1.0f, *ptr));
        }
    }
    
    // === Symmetric Editing ===
    
    // Mirror face parameters across the symmetry axis
    // (Most face params are already symmetric, but expression params can be asymmetric)
    void mirrorExpressions() {
        auto& e = expressionParams_;
        float avgBlink = (e.eyeBlinkLeft + e.eyeBlinkRight) * 0.5f;
        e.eyeBlinkLeft = e.eyeBlinkRight = avgBlink;
        
        float avgSmile = (e.mouthSmileLeft + e.mouthSmileRight) * 0.5f;
        e.mouthSmileLeft = e.mouthSmileRight = avgSmile;
        
        float avgFrown = (e.mouthFrownLeft + e.mouthFrownRight) * 0.5f;
        e.mouthFrownLeft = e.mouthFrownRight = avgFrown;
        
        float avgSquint = (e.eyeSquintLeft + e.eyeSquintRight) * 0.5f;
        e.eyeSquintLeft = e.eyeSquintRight = avgSquint;
        
        float avgWide = (e.eyeWideLeft + e.eyeWideRight) * 0.5f;
        e.eyeWideLeft = e.eyeWideRight = avgWide;
        
        float avgBrow = (e.browDownLeft + e.browDownRight) * 0.5f;
        e.browDownLeft = e.browDownRight = avgBrow;
        
        float avgBrowOuter = (e.browOuterUpLeft + e.browOuterUpRight) * 0.5f;
        e.browOuterUpLeft = e.browOuterUpRight = avgBrowOuter;
        
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
    // BFM/3DDFA outputs 40 identity coefficients + 10 expression coefficients
    // This mapping is based on empirical analysis of BFM principal components
    void mapDMMtoFaceParams(const std::vector<float>& dmmParams) {
        if (dmmParams.size() < 10) return;
        
        // BFM principal components interpretation:
        // PC0: Overall face size/width
        // PC1: Face length/height
        // PC2: Nose prominence
        // PC3: Jaw width
        // PC4: Eye region shape
        // PC5: Cheekbone prominence
        // PC6: Chin shape
        // PC7: Forehead height
        // PC8: Nose width
        // PC9: Lip fullness
        // ... (higher PCs capture finer details)
        
        // Helper to safely get coefficient with default
        auto getCoeff = [&](size_t idx, float scale = 0.15f) -> float {
            if (idx >= dmmParams.size()) return 0.0f;
            // Clamp to reasonable range and scale
            float v = std::max(-3.0f, std::min(3.0f, dmmParams[idx]));
            return v * scale;
        };
        
        // Overall face shape
        shapeParams_.faceWidth = 0.5f + getCoeff(0, 0.12f);
        shapeParams_.faceLength = 0.5f + getCoeff(1, 0.10f);
        shapeParams_.faceRoundness = 0.5f - getCoeff(0, 0.08f) + getCoeff(1, 0.05f);
        
        // Forehead
        shapeParams_.foreheadHeight = 0.5f + getCoeff(7, 0.10f);
        shapeParams_.foreheadWidth = 0.5f + getCoeff(0, 0.08f);
        shapeParams_.foreheadSlope = 0.5f + getCoeff(15, 0.08f);
        
        // Eyes
        shapeParams_.eyeSize = 0.5f + getCoeff(4, 0.10f);
        shapeParams_.eyeSpacing = 0.5f + getCoeff(12, 0.08f);
        shapeParams_.eyeHeight = 0.5f + getCoeff(13, 0.06f);
        shapeParams_.eyeDepth = 0.5f + getCoeff(14, 0.08f);
        shapeParams_.eyeAngle = 0.5f + getCoeff(16, 0.06f);
        
        // Eyebrows
        shapeParams_.browHeight = 0.5f + getCoeff(17, 0.08f);
        shapeParams_.browAngle = 0.5f + getCoeff(18, 0.06f);
        
        // Nose
        shapeParams_.noseLength = 0.5f + getCoeff(2, 0.12f);
        shapeParams_.noseWidth = 0.5f + getCoeff(8, 0.10f);
        shapeParams_.noseHeight = 0.5f + getCoeff(2, 0.08f);
        shapeParams_.noseBridge = 0.5f + getCoeff(19, 0.08f);
        shapeParams_.noseTip = 0.5f + getCoeff(20, 0.08f);
        shapeParams_.nostrilWidth = 0.5f + getCoeff(8, 0.06f);
        
        // Mouth
        shapeParams_.mouthWidth = 0.5f + getCoeff(21, 0.10f);
        shapeParams_.upperLipThickness = 0.5f + getCoeff(9, 0.08f);
        shapeParams_.lowerLipThickness = 0.5f + getCoeff(9, 0.10f);
        shapeParams_.lipProtrusion = 0.5f + getCoeff(22, 0.08f);
        
        // Chin and Jaw
        shapeParams_.chinLength = 0.5f + getCoeff(6, 0.10f);
        shapeParams_.chinWidth = 0.5f + getCoeff(23, 0.08f);
        shapeParams_.chinProtrusion = 0.5f + getCoeff(24, 0.08f);
        shapeParams_.jawWidth = 0.5f + getCoeff(3, 0.12f);
        shapeParams_.jawAngle = 0.5f + getCoeff(25, 0.08f);
        shapeParams_.jawLine = 0.5f + getCoeff(26, 0.08f);
        
        // Cheeks
        shapeParams_.cheekboneProminence = 0.5f + getCoeff(5, 0.10f);
        shapeParams_.cheekboneHeight = 0.5f + getCoeff(27, 0.08f);
        shapeParams_.cheekFullness = 0.5f + getCoeff(28, 0.08f);
        
        // Apply constraints to ensure valid face
        applyConstraints();
    }
    
public:
    // Map from 2D landmark fit results to face params
    void applyLandmarkFitResult(float faceWidth, float faceLength, float jawWidth,
                                 float eyeSpacing, float eyeSize, float noseLength,
                                 float noseWidth, float mouthWidth, float browHeight,
                                 float confidence) {
        if (confidence < 0.1f) return;
        
        // Blend with current params based on confidence
        float blend = std::min(1.0f, confidence);
        
        auto lerp = [blend](float current, float target) {
            return current + (target - current) * blend;
        };
        
        shapeParams_.faceWidth = lerp(shapeParams_.faceWidth, faceWidth);
        shapeParams_.faceLength = lerp(shapeParams_.faceLength, faceLength);
        shapeParams_.jawWidth = lerp(shapeParams_.jawWidth, jawWidth);
        shapeParams_.eyeSpacing = lerp(shapeParams_.eyeSpacing, eyeSpacing);
        shapeParams_.eyeSize = lerp(shapeParams_.eyeSize, eyeSize);
        shapeParams_.noseLength = lerp(shapeParams_.noseLength, noseLength);
        shapeParams_.noseWidth = lerp(shapeParams_.noseWidth, noseWidth);
        shapeParams_.mouthWidth = lerp(shapeParams_.mouthWidth, mouthWidth);
        shapeParams_.browHeight = lerp(shapeParams_.browHeight, browHeight);
        
        applyConstraints();
        updateBlendShapeWeights();
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
