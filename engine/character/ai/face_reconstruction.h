// Face Reconstruction - Photo to 3D face using AI
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/ai/ai_inference.h"
#include "engine/character/character_face.h"
#include "engine/character/blend_shape.h"
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <memory>
#include <functional>

namespace luma {

// ============================================================================
// Face Landmark Detection (MediaPipe Face Mesh compatible)
// ============================================================================

// MediaPipe Face Mesh outputs 468 3D landmarks
constexpr int FACE_MESH_LANDMARK_COUNT = 468;

// Key landmark indices
namespace FaceLandmarkIndex {
    // Face contour
    constexpr int CHIN = 152;
    constexpr int LEFT_CHEEK = 234;
    constexpr int RIGHT_CHEEK = 454;
    
    // Eyes
    constexpr int LEFT_EYE_INNER = 133;
    constexpr int LEFT_EYE_OUTER = 33;
    constexpr int LEFT_EYE_TOP = 159;
    constexpr int LEFT_EYE_BOTTOM = 145;
    constexpr int RIGHT_EYE_INNER = 362;
    constexpr int RIGHT_EYE_OUTER = 263;
    constexpr int RIGHT_EYE_TOP = 386;
    constexpr int RIGHT_EYE_BOTTOM = 374;
    
    // Eyebrows
    constexpr int LEFT_BROW_INNER = 107;
    constexpr int LEFT_BROW_OUTER = 46;
    constexpr int RIGHT_BROW_INNER = 336;
    constexpr int RIGHT_BROW_OUTER = 276;
    
    // Nose
    constexpr int NOSE_TIP = 4;
    constexpr int NOSE_BRIDGE = 6;
    constexpr int LEFT_NOSTRIL = 102;
    constexpr int RIGHT_NOSTRIL = 331;
    
    // Mouth
    constexpr int MOUTH_LEFT = 61;
    constexpr int MOUTH_RIGHT = 291;
    constexpr int UPPER_LIP_TOP = 0;
    constexpr int LOWER_LIP_BOTTOM = 17;
    constexpr int UPPER_LIP_CENTER = 13;
    constexpr int LOWER_LIP_CENTER = 14;
    
    // Forehead (approximate)
    constexpr int FOREHEAD_CENTER = 10;
}

struct FaceLandmarks {
    std::array<Vec3, FACE_MESH_LANDMARK_COUNT> points;
    float confidence = 0.0f;
    
    // Bounding box
    Vec2 bboxMin;
    Vec2 bboxMax;
    
    // Helper accessors
    Vec3 getChin() const { return points[FaceLandmarkIndex::CHIN]; }
    Vec3 getNoseTip() const { return points[FaceLandmarkIndex::NOSE_TIP]; }
    Vec3 getLeftEyeCenter() const {
        return (points[FaceLandmarkIndex::LEFT_EYE_INNER] + 
                points[FaceLandmarkIndex::LEFT_EYE_OUTER]) * 0.5f;
    }
    Vec3 getRightEyeCenter() const {
        return (points[FaceLandmarkIndex::RIGHT_EYE_INNER] + 
                points[FaceLandmarkIndex::RIGHT_EYE_OUTER]) * 0.5f;
    }
    Vec3 getMouthCenter() const {
        return (points[FaceLandmarkIndex::MOUTH_LEFT] + 
                points[FaceLandmarkIndex::MOUTH_RIGHT]) * 0.5f;
    }
    
    // Calculate derived metrics
    float getEyeDistance() const {
        return (getLeftEyeCenter() - getRightEyeCenter()).length();
    }
    
    float getFaceWidth() const {
        return (points[FaceLandmarkIndex::LEFT_CHEEK] - 
                points[FaceLandmarkIndex::RIGHT_CHEEK]).length();
    }
    
    float getFaceHeight() const {
        return (points[FaceLandmarkIndex::FOREHEAD_CENTER] - getChin()).length();
    }
};

// ============================================================================
// Face Detector
// ============================================================================

struct FaceDetection {
    Vec2 bboxMin;           // Top-left corner (normalized 0-1)
    Vec2 bboxMax;           // Bottom-right corner
    float confidence = 0.0f;
    float roll = 0.0f;      // Head rotation (radians)
    float yaw = 0.0f;
    float pitch = 0.0f;
    
    // Key points (6 points for basic alignment)
    Vec2 leftEye;
    Vec2 rightEye;
    Vec2 nose;
    Vec2 mouth;
    Vec2 leftEar;
    Vec2 rightEar;
    
    float getWidth() const { return bboxMax.x - bboxMin.x; }
    float getHeight() const { return bboxMax.y - bboxMin.y; }
    Vec2 getCenter() const { return (bboxMin + bboxMax) * 0.5f; }
};

class FaceDetector {
public:
    FaceDetector() = default;
    
    // Initialize with model
    bool initialize(const std::string& modelPath) {
        modelPath_ = modelPath;
        if (!session_.loadModel(modelPath)) {
            return false;
        }
        initialized_ = true;
        return true;
    }
    
    // Detect faces in image
    // Returns list of detected faces
    std::vector<FaceDetection> detect(const uint8_t* imageData, int width, int height, int channels) {
        std::vector<FaceDetection> results;
        
        if (!initialized_) {
            // Fallback: return dummy detection for center of image
            FaceDetection det;
            det.bboxMin = Vec2(0.2f, 0.1f);
            det.bboxMax = Vec2(0.8f, 0.9f);
            det.confidence = 0.99f;
            det.leftEye = Vec2(0.35f, 0.35f);
            det.rightEye = Vec2(0.65f, 0.35f);
            det.nose = Vec2(0.5f, 0.55f);
            det.mouth = Vec2(0.5f, 0.75f);
            results.push_back(det);
            return results;
        }
        
        // Prepare input tensor
        Tensor input = ImagePreprocess::prepareImageTensor(
            imageData, width, height, channels,
            INPUT_SIZE, INPUT_SIZE, true, true);
        
        // Run inference
        Tensor output = session_.runSingle(input);
        
        // Parse output (would depend on specific model format)
        // For now, return dummy result
        FaceDetection det;
        det.bboxMin = Vec2(0.2f, 0.1f);
        det.bboxMax = Vec2(0.8f, 0.9f);
        det.confidence = 0.95f;
        results.push_back(det);
        
        return results;
    }
    
    // Detect single face (largest/most confident)
    bool detectSingle(const uint8_t* imageData, int width, int height, int channels,
                      FaceDetection& outDetection) {
        auto detections = detect(imageData, width, height, channels);
        if (detections.empty()) return false;
        
        // Find largest face
        float maxArea = 0;
        for (const auto& det : detections) {
            float area = det.getWidth() * det.getHeight();
            if (area > maxArea) {
                maxArea = area;
                outDetection = det;
            }
        }
        return true;
    }
    
private:
    static constexpr int INPUT_SIZE = 320;
    
    bool initialized_ = false;
    std::string modelPath_;
    InferenceSession session_;
};

// ============================================================================
// Face Mesh Estimator (MediaPipe Face Mesh compatible)
// ============================================================================

class FaceMeshEstimator {
public:
    FaceMeshEstimator() = default;
    
    // Initialize with model
    bool initialize(const std::string& modelPath) {
        modelPath_ = modelPath;
        if (!session_.loadModel(modelPath)) {
            return false;
        }
        initialized_ = true;
        return true;
    }
    
    // Estimate face mesh from cropped face image
    bool estimate(const uint8_t* faceImageData, int width, int height, int channels,
                  FaceLandmarks& outLandmarks) {
        
        if (!initialized_) {
            // Generate synthetic landmarks for testing
            generateSyntheticLandmarks(outLandmarks);
            return true;
        }
        
        // Prepare input tensor
        Tensor input = ImagePreprocess::prepareImageTensor(
            faceImageData, width, height, channels,
            INPUT_SIZE, INPUT_SIZE, true, true);
        
        // Run inference
        Tensor output = session_.runSingle(input);
        
        // Parse output (468 x 3 coordinates)
        const float* data = output.dataAs<float>();
        for (int i = 0; i < FACE_MESH_LANDMARK_COUNT; i++) {
            outLandmarks.points[i] = Vec3(
                data[i * 3 + 0],
                data[i * 3 + 1],
                data[i * 3 + 2]
            );
        }
        outLandmarks.confidence = 0.95f;
        
        return true;
    }
    
    // Estimate from full image with detection
    bool estimateFromFullImage(const uint8_t* imageData, int width, int height, int channels,
                                const FaceDetection& detection, FaceLandmarks& outLandmarks) {
        // Crop face region with padding
        float padding = 0.2f;
        int x0 = static_cast<int>((detection.bboxMin.x - padding) * width);
        int y0 = static_cast<int>((detection.bboxMin.y - padding) * height);
        int x1 = static_cast<int>((detection.bboxMax.x + padding) * width);
        int y1 = static_cast<int>((detection.bboxMax.y + padding) * height);
        
        x0 = std::max(0, x0);
        y0 = std::max(0, y0);
        x1 = std::min(width, x1);
        y1 = std::min(height, y1);
        
        int cropW = x1 - x0;
        int cropH = y1 - y0;
        
        // Extract crop
        std::vector<uint8_t> cropData(cropW * cropH * channels);
        for (int y = 0; y < cropH; y++) {
            for (int x = 0; x < cropW; x++) {
                for (int c = 0; c < channels; c++) {
                    cropData[(y * cropW + x) * channels + c] = 
                        imageData[((y + y0) * width + (x + x0)) * channels + c];
                }
            }
        }
        
        // Run estimation on crop
        bool result = estimate(cropData.data(), cropW, cropH, channels, outLandmarks);
        
        // Transform landmarks back to full image coordinates
        if (result) {
            float scaleX = static_cast<float>(cropW) / INPUT_SIZE;
            float scaleY = static_cast<float>(cropH) / INPUT_SIZE;
            
            for (auto& point : outLandmarks.points) {
                point.x = (point.x * scaleX + x0) / width;
                point.y = (point.y * scaleY + y0) / height;
                // Z is relative depth, keep as is
            }
            
            outLandmarks.bboxMin = detection.bboxMin;
            outLandmarks.bboxMax = detection.bboxMax;
        }
        
        return result;
    }
    
private:
    static constexpr int INPUT_SIZE = 192;
    
    bool initialized_ = false;
    std::string modelPath_;
    InferenceSession session_;
    
    // Generate synthetic landmarks for testing
    void generateSyntheticLandmarks(FaceLandmarks& landmarks) {
        // Create a face-like distribution of points
        for (int i = 0; i < FACE_MESH_LANDMARK_COUNT; i++) {
            // Distribute points in a face-like ellipse
            float t = static_cast<float>(i) / FACE_MESH_LANDMARK_COUNT;
            float angle = t * 6.28318f;
            
            // Different radii for different regions
            float r = 0.3f + 0.1f * std::sin(angle * 3);
            
            landmarks.points[i] = Vec3(
                0.5f + r * std::cos(angle),
                0.5f + r * std::sin(angle) * 0.8f,  // Slightly squashed vertically
                0.1f * std::sin(angle * 2)           // Slight depth variation
            );
        }
        
        // Set key landmarks to reasonable positions
        landmarks.points[FaceLandmarkIndex::CHIN] = Vec3(0.5f, 0.85f, 0.0f);
        landmarks.points[FaceLandmarkIndex::FOREHEAD_CENTER] = Vec3(0.5f, 0.15f, 0.0f);
        landmarks.points[FaceLandmarkIndex::LEFT_EYE_INNER] = Vec3(0.4f, 0.35f, 0.02f);
        landmarks.points[FaceLandmarkIndex::LEFT_EYE_OUTER] = Vec3(0.3f, 0.35f, 0.01f);
        landmarks.points[FaceLandmarkIndex::RIGHT_EYE_INNER] = Vec3(0.6f, 0.35f, 0.02f);
        landmarks.points[FaceLandmarkIndex::RIGHT_EYE_OUTER] = Vec3(0.7f, 0.35f, 0.01f);
        landmarks.points[FaceLandmarkIndex::NOSE_TIP] = Vec3(0.5f, 0.55f, 0.08f);
        landmarks.points[FaceLandmarkIndex::NOSE_BRIDGE] = Vec3(0.5f, 0.4f, 0.05f);
        landmarks.points[FaceLandmarkIndex::MOUTH_LEFT] = Vec3(0.35f, 0.7f, 0.02f);
        landmarks.points[FaceLandmarkIndex::MOUTH_RIGHT] = Vec3(0.65f, 0.7f, 0.02f);
        landmarks.points[FaceLandmarkIndex::LEFT_CHEEK] = Vec3(0.25f, 0.5f, 0.0f);
        landmarks.points[FaceLandmarkIndex::RIGHT_CHEEK] = Vec3(0.75f, 0.5f, 0.0f);
        
        landmarks.confidence = 0.9f;
        landmarks.bboxMin = Vec2(0.2f, 0.1f);
        landmarks.bboxMax = Vec2(0.8f, 0.9f);
    }
};

// ============================================================================
// 3D Morphable Model (3DMM) Parameters
// ============================================================================

// FLAME model compatible parameters
struct FLAME3DMMParams {
    // Shape parameters (identity) - typically 100-300 dimensions
    std::vector<float> shape;
    
    // Expression parameters - typically 50-100 dimensions
    std::vector<float> expression;
    
    // Pose parameters
    Vec3 globalRotation;     // Head pose (pitch, yaw, roll)
    Vec3 globalTranslation;
    Vec3 jawPose;            // Jaw rotation
    std::array<Vec3, 3> eyePose;  // Left eye, right eye, neck
    
    // Appearance (texture) parameters - typically 50-200 dimensions
    std::vector<float> texture;
    
    // Lighting (spherical harmonics coefficients)
    std::vector<float> lighting;  // 9 x 3 = 27 for SH order 2
    
    FLAME3DMMParams() {
        shape.resize(100, 0.0f);
        expression.resize(50, 0.0f);
        texture.resize(50, 0.0f);
        lighting.resize(27, 0.0f);
        
        // Default lighting (frontal)
        lighting[0] = 1.0f;   // DC component
    }
};

// ============================================================================
// 3DMM Regressor (DECA/EMOCA compatible)
// ============================================================================

class Face3DMMRegressor {
public:
    Face3DMMRegressor() = default;
    
    // Initialize with model
    bool initialize(const std::string& modelPath) {
        modelPath_ = modelPath;
        if (!session_.loadModel(modelPath)) {
            return false;
        }
        initialized_ = true;
        return true;
    }
    
    // Regress 3DMM parameters from face image
    bool regress(const uint8_t* faceImageData, int width, int height, int channels,
                 FLAME3DMMParams& outParams) {
        
        if (!initialized_) {
            // Generate synthetic parameters for testing
            generateSyntheticParams(outParams);
            return true;
        }
        
        // Prepare input tensor
        Tensor input = ImagePreprocess::prepareImageTensor(
            faceImageData, width, height, channels,
            INPUT_SIZE, INPUT_SIZE, true, true);
        
        // Run inference
        Tensor output = session_.runSingle(input);
        
        // Parse output (model-specific format)
        // Typically concatenated: [shape, expression, pose, texture, lighting]
        const float* data = output.dataAs<float>();
        
        // Copy shape parameters (first 100)
        for (int i = 0; i < 100; i++) {
            outParams.shape[i] = data[i];
        }
        
        // Copy expression parameters (next 50)
        for (int i = 0; i < 50; i++) {
            outParams.expression[i] = data[100 + i];
        }
        
        // Copy pose (next 9 = 3 for global, 3 for jaw, 3 for neck)
        outParams.globalRotation = Vec3(data[150], data[151], data[152]);
        outParams.jawPose = Vec3(data[153], data[154], data[155]);
        
        return true;
    }
    
    // Regress from landmarks (simpler, no image needed)
    bool regressFromLandmarks(const FaceLandmarks& landmarks, FLAME3DMMParams& outParams) {
        // Use landmarks to estimate basic face shape
        float faceWidth = landmarks.getFaceWidth();
        float faceHeight = landmarks.getFaceHeight();
        float eyeDistance = landmarks.getEyeDistance();
        
        // Convert to shape parameters (simplified mapping)
        outParams.shape[0] = (faceWidth - 0.5f) * 2.0f;   // Face width
        outParams.shape[1] = (faceHeight - 0.8f) * 2.0f;  // Face height
        outParams.shape[2] = (eyeDistance - 0.3f) * 2.0f; // Eye distance
        
        // Eye size from landmarks
        float leftEyeWidth = (landmarks.points[FaceLandmarkIndex::LEFT_EYE_OUTER] - 
                              landmarks.points[FaceLandmarkIndex::LEFT_EYE_INNER]).length();
        float rightEyeWidth = (landmarks.points[FaceLandmarkIndex::RIGHT_EYE_OUTER] - 
                               landmarks.points[FaceLandmarkIndex::RIGHT_EYE_INNER]).length();
        float avgEyeWidth = (leftEyeWidth + rightEyeWidth) * 0.5f;
        outParams.shape[3] = (avgEyeWidth - 0.1f) * 5.0f;  // Eye size
        
        // Nose from landmarks
        float noseLength = (landmarks.points[FaceLandmarkIndex::NOSE_TIP] - 
                           landmarks.points[FaceLandmarkIndex::NOSE_BRIDGE]).length();
        float nostrilWidth = (landmarks.points[FaceLandmarkIndex::LEFT_NOSTRIL] - 
                             landmarks.points[FaceLandmarkIndex::RIGHT_NOSTRIL]).length();
        outParams.shape[4] = (noseLength - 0.15f) * 3.0f;  // Nose length
        outParams.shape[5] = (nostrilWidth - 0.08f) * 5.0f; // Nose width
        
        // Mouth from landmarks
        float mouthWidth = (landmarks.points[FaceLandmarkIndex::MOUTH_LEFT] - 
                           landmarks.points[FaceLandmarkIndex::MOUTH_RIGHT]).length();
        outParams.shape[6] = (mouthWidth - 0.3f) * 2.0f;  // Mouth width
        
        // Estimate pose from landmark positions
        Vec3 leftEye = landmarks.getLeftEyeCenter();
        Vec3 rightEye = landmarks.getRightEyeCenter();
        Vec3 nose = landmarks.getNoseTip();
        
        // Yaw from eye positions
        float eyeDiffZ = rightEye.z - leftEye.z;
        outParams.globalRotation.y = std::atan2(eyeDiffZ, eyeDistance);
        
        // Pitch from nose position
        Vec3 eyeCenter = (leftEye + rightEye) * 0.5f;
        float noseDrop = nose.y - eyeCenter.y;
        outParams.globalRotation.x = std::atan2(nose.z - eyeCenter.z, noseDrop);
        
        // Roll from eye tilt
        float eyeDiffY = rightEye.y - leftEye.y;
        outParams.globalRotation.z = std::atan2(eyeDiffY, eyeDistance);
        
        return true;
    }
    
private:
    static constexpr int INPUT_SIZE = 224;
    
    bool initialized_ = false;
    std::string modelPath_;
    InferenceSession session_;
    
    void generateSyntheticParams(FLAME3DMMParams& params) {
        // Generate reasonable random parameters
        for (auto& s : params.shape) {
            s = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f;
        }
        for (auto& e : params.expression) {
            e = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f;
        }
        params.globalRotation = Vec3(0, 0, 0);
    }
};

// ============================================================================
// Face Parameter Mapper - Maps 3DMM to LUMA face parameters
// ============================================================================

class FaceParameterMapper {
public:
    // Map FLAME 3DMM parameters to LUMA FaceShapeParams
    static void mapToFaceShapeParams(const FLAME3DMMParams& flame, FaceShapeParams& outParams) {
        // These mappings would ideally be learned or carefully calibrated
        // For now, using approximate linear mappings
        
        // Normalize 3DMM parameters (typically in range [-3, 3] std devs)
        auto normalize = [](float v) {
            return std::clamp((v / 3.0f + 1.0f) * 0.5f, 0.0f, 1.0f);
        };
        
        // Overall face shape
        outParams.faceWidth = normalize(flame.shape[0]);
        outParams.faceLength = normalize(flame.shape[1]);
        outParams.faceRoundness = normalize(flame.shape[2]);
        
        // Eyes
        outParams.eyeSize = normalize(flame.shape[3]);
        outParams.eyeSpacing = normalize(flame.shape[4]);
        outParams.eyeHeight = normalize(flame.shape[5]);
        outParams.eyeAngle = normalize(flame.shape[6]);
        outParams.eyeDepth = normalize(flame.shape[7]);
        
        // Eyebrows
        outParams.browHeight = normalize(flame.shape[8]);
        outParams.browAngle = normalize(flame.shape[9]);
        
        // Nose
        outParams.noseLength = normalize(flame.shape[10]);
        outParams.noseWidth = normalize(flame.shape[11]);
        outParams.noseHeight = normalize(flame.shape[12]);
        outParams.noseBridge = normalize(flame.shape[13]);
        outParams.noseTip = normalize(flame.shape[14]);
        
        // Mouth
        outParams.mouthWidth = normalize(flame.shape[15]);
        outParams.upperLipThickness = normalize(flame.shape[16]);
        outParams.lowerLipThickness = normalize(flame.shape[17]);
        
        // Chin/Jaw
        outParams.chinLength = normalize(flame.shape[18]);
        outParams.chinWidth = normalize(flame.shape[19]);
        outParams.jawWidth = normalize(flame.shape[20]);
        outParams.jawAngle = normalize(flame.shape[21]);
        
        // Cheeks
        outParams.cheekboneProminence = normalize(flame.shape[22]);
        outParams.cheekFullness = normalize(flame.shape[23]);
    }
    
    // Map FLAME expression to LUMA FaceExpressionParams
    static void mapToExpressionParams(const FLAME3DMMParams& flame, FaceExpressionParams& outParams) {
        // FLAME expression basis to ARKit-like mapping
        // This is a simplified version
        
        auto clamp01 = [](float v) { return std::clamp(v * 0.5f + 0.5f, 0.0f, 1.0f); };
        
        // Jaw
        outParams.jawOpen = clamp01(flame.expression[0]);
        
        // Mouth
        outParams.mouthSmileLeft = clamp01(flame.expression[1]);
        outParams.mouthSmileRight = clamp01(flame.expression[2]);
        outParams.mouthFrownLeft = clamp01(-flame.expression[1]);
        outParams.mouthFrownRight = clamp01(-flame.expression[2]);
        outParams.mouthPucker = clamp01(flame.expression[3]);
        
        // Eyes
        outParams.eyeBlinkLeft = clamp01(flame.expression[4]);
        outParams.eyeBlinkRight = clamp01(flame.expression[5]);
        outParams.eyeWideLeft = clamp01(-flame.expression[4]);
        outParams.eyeWideRight = clamp01(-flame.expression[5]);
        
        // Brows
        outParams.browDownLeft = clamp01(flame.expression[6]);
        outParams.browDownRight = clamp01(flame.expression[7]);
        outParams.browInnerUp = clamp01(-flame.expression[6] - flame.expression[7]);
    }
    
    // Map landmarks to FaceShapeParams (direct landmark-based mapping)
    static void mapLandmarksToFaceParams(const FaceLandmarks& landmarks, FaceShapeParams& outParams) {
        // Measure face proportions from landmarks
        float faceWidth = landmarks.getFaceWidth();
        float faceHeight = landmarks.getFaceHeight();
        float eyeDistance = landmarks.getEyeDistance();
        
        // Normalize measurements to 0-1 range based on typical human proportions
        // Face width: typically 0.12-0.18 of image width for frontal face
        outParams.faceWidth = std::clamp((faceWidth - 0.12f) / 0.06f, 0.0f, 1.0f);
        
        // Face height: typically 0.6-0.9 of image height
        outParams.faceLength = std::clamp((faceHeight - 0.6f) / 0.3f, 0.0f, 1.0f);
        
        // Eye distance relative to face width
        float eyeRatio = eyeDistance / faceWidth;
        outParams.eyeSpacing = std::clamp((eyeRatio - 0.3f) / 0.2f, 0.0f, 1.0f);
        
        // Eye size
        float leftEyeW = (landmarks.points[FaceLandmarkIndex::LEFT_EYE_OUTER] - 
                         landmarks.points[FaceLandmarkIndex::LEFT_EYE_INNER]).length();
        float eyeSizeRatio = leftEyeW / faceWidth;
        outParams.eyeSize = std::clamp((eyeSizeRatio - 0.08f) / 0.06f, 0.0f, 1.0f);
        
        // Nose
        float noseLength = (landmarks.points[FaceLandmarkIndex::NOSE_TIP] - 
                           landmarks.points[FaceLandmarkIndex::NOSE_BRIDGE]).length();
        float noseLengthRatio = noseLength / faceHeight;
        outParams.noseLength = std::clamp((noseLengthRatio - 0.2f) / 0.15f, 0.0f, 1.0f);
        
        float noseWidth = (landmarks.points[FaceLandmarkIndex::LEFT_NOSTRIL] - 
                          landmarks.points[FaceLandmarkIndex::RIGHT_NOSTRIL]).length();
        float noseWidthRatio = noseWidth / faceWidth;
        outParams.noseWidth = std::clamp((noseWidthRatio - 0.15f) / 0.1f, 0.0f, 1.0f);
        
        // Mouth
        float mouthWidth = (landmarks.points[FaceLandmarkIndex::MOUTH_LEFT] - 
                           landmarks.points[FaceLandmarkIndex::MOUTH_RIGHT]).length();
        float mouthWidthRatio = mouthWidth / faceWidth;
        outParams.mouthWidth = std::clamp((mouthWidthRatio - 0.3f) / 0.2f, 0.0f, 1.0f);
        
        // Jaw
        float jawWidth = (landmarks.points[FaceLandmarkIndex::LEFT_CHEEK] - 
                         landmarks.points[FaceLandmarkIndex::RIGHT_CHEEK]).length();
        float jawWidthRatio = jawWidth / faceWidth;
        outParams.jawWidth = std::clamp((jawWidthRatio - 0.8f) / 0.2f, 0.0f, 1.0f);
    }
};

// ============================================================================
// Face Texture Extractor
// ============================================================================

class FaceTextureExtractor {
public:
    // Extract face texture from image using landmarks
    static bool extractTexture(const uint8_t* imageData, int width, int height, int channels,
                                const FaceLandmarks& landmarks,
                                std::vector<uint8_t>& outTexture,
                                int textureSize = 512) {
        outTexture.resize(textureSize * textureSize * 4);  // RGBA
        
        // Simple UV projection
        // In production, would use a proper UV unwrapping based on 3D face model
        
        for (int y = 0; y < textureSize; y++) {
            for (int x = 0; x < textureSize; x++) {
                // Map texture coordinates to image coordinates
                float u = static_cast<float>(x) / textureSize;
                float v = static_cast<float>(y) / textureSize;
                
                // Transform UV to image space (simplified - assumes frontal face)
                float imgX = landmarks.bboxMin.x + u * (landmarks.bboxMax.x - landmarks.bboxMin.x);
                float imgY = landmarks.bboxMin.y + v * (landmarks.bboxMax.y - landmarks.bboxMin.y);
                
                int srcX = static_cast<int>(imgX * width);
                int srcY = static_cast<int>(imgY * height);
                
                srcX = std::clamp(srcX, 0, width - 1);
                srcY = std::clamp(srcY, 0, height - 1);
                
                int srcIdx = (srcY * width + srcX) * channels;
                int dstIdx = (y * textureSize + x) * 4;
                
                outTexture[dstIdx + 0] = imageData[srcIdx + 0];  // R
                outTexture[dstIdx + 1] = (channels > 1) ? imageData[srcIdx + 1] : imageData[srcIdx];  // G
                outTexture[dstIdx + 2] = (channels > 2) ? imageData[srcIdx + 2] : imageData[srcIdx];  // B
                outTexture[dstIdx + 3] = 255;  // A
            }
        }
        
        return true;
    }
    
    // Extract average skin color from face region
    static Vec3 extractSkinColor(const uint8_t* imageData, int width, int height, int channels,
                                  const FaceLandmarks& landmarks) {
        // Sample from cheek regions
        auto toVec2 = [](const Vec3& v) { return Vec2(v.x, v.y); };
        
        std::vector<Vec2> samplePoints = {
            (toVec2(landmarks.points[FaceLandmarkIndex::LEFT_CHEEK]) + 
             toVec2(landmarks.getLeftEyeCenter())) * 0.5f,
            (toVec2(landmarks.points[FaceLandmarkIndex::RIGHT_CHEEK]) + 
             toVec2(landmarks.getRightEyeCenter())) * 0.5f,
            toVec2(landmarks.getNoseTip()) + Vec2(0.05f, 0.0f),
            toVec2(landmarks.getNoseTip()) + Vec2(-0.05f, 0.0f)
        };
        
        Vec3 totalColor(0, 0, 0);
        int sampleCount = 0;
        
        for (const auto& pt : samplePoints) {
            int x = static_cast<int>(pt.x * width);
            int y = static_cast<int>(pt.y * height);
            
            if (x >= 0 && x < width && y >= 0 && y < height) {
                int idx = (y * width + x) * channels;
                totalColor.x += imageData[idx + 0] / 255.0f;
                totalColor.y += (channels > 1) ? imageData[idx + 1] / 255.0f : imageData[idx] / 255.0f;
                totalColor.z += (channels > 2) ? imageData[idx + 2] / 255.0f : imageData[idx] / 255.0f;
                sampleCount++;
            }
        }
        
        if (sampleCount > 0) {
            totalColor = totalColor * (1.0f / sampleCount);
        }
        
        return totalColor;
    }
};

// ============================================================================
// Complete Photo-to-Face Pipeline
// ============================================================================

class PhotoToFacePipeline {
public:
    struct Config {
        std::string faceDetectorModelPath;
        std::string faceMeshModelPath;
        std::string face3DMMModelPath;
        int textureSize = 512;
        bool extractTexture = true;
        bool use3DMM = true;
    };
    
    PhotoToFacePipeline() = default;
    
    // Initialize pipeline
    bool initialize(const Config& config) {
        config_ = config;
        
        // Initialize components
        // Face detector
        if (!config.faceDetectorModelPath.empty()) {
            if (!faceDetector_.initialize(config.faceDetectorModelPath)) {
                // Continue without model (will use fallback)
            }
        }
        
        // Face mesh estimator
        if (!config.faceMeshModelPath.empty()) {
            if (!faceMesh_.initialize(config.faceMeshModelPath)) {
                // Continue without model
            }
        }
        
        // 3DMM regressor
        if (!config.face3DMMModelPath.empty() && config.use3DMM) {
            if (!face3DMM_.initialize(config.face3DMMModelPath)) {
                // Continue without model
            }
        }
        
        initialized_ = true;
        return true;
    }
    
    // Process photo and generate face parameters
    bool process(const uint8_t* imageData, int width, int height, int channels,
                 PhotoFaceResult& outResult) {
        
        outResult.success = false;
        
        // Step 1: Detect face
        FaceDetection detection;
        if (!faceDetector_.detectSingle(imageData, width, height, channels, detection)) {
            outResult.errorMessage = "No face detected";
            return false;
        }
        
        // Step 2: Estimate face mesh (landmarks)
        FaceLandmarks landmarks;
        if (!faceMesh_.estimateFromFullImage(imageData, width, height, channels, 
                                              detection, landmarks)) {
            outResult.errorMessage = "Failed to estimate face mesh";
            return false;
        }
        
        // Store landmarks
        outResult.landmarks.resize(FACE_MESH_LANDMARK_COUNT);
        for (int i = 0; i < FACE_MESH_LANDMARK_COUNT; i++) {
            outResult.landmarks[i] = landmarks.points[i];
        }
        
        // Step 3: Estimate 3DMM parameters
        if (config_.use3DMM) {
            FLAME3DMMParams flameParams;
            
            // Try image-based regression first
            // Crop face region
            int x0 = static_cast<int>(detection.bboxMin.x * width);
            int y0 = static_cast<int>(detection.bboxMin.y * height);
            int x1 = static_cast<int>(detection.bboxMax.x * width);
            int y1 = static_cast<int>(detection.bboxMax.y * height);
            int cropW = x1 - x0;
            int cropH = y1 - y0;
            
            std::vector<uint8_t> faceCrop(cropW * cropH * channels);
            for (int y = 0; y < cropH; y++) {
                for (int x = 0; x < cropW; x++) {
                    for (int c = 0; c < channels; c++) {
                        faceCrop[(y * cropW + x) * channels + c] = 
                            imageData[((y + y0) * width + (x + x0)) * channels + c];
                    }
                }
            }
            
            face3DMM_.regress(faceCrop.data(), cropW, cropH, channels, flameParams);
            
            // Also use landmarks for additional accuracy
            face3DMM_.regressFromLandmarks(landmarks, flameParams);
            
            // Store 3DMM parameters
            outResult.shapeParams = flameParams.shape;
            outResult.expressionParams = flameParams.expression;
            outResult.headRotation = flameParams.globalRotation;
            outResult.headTranslation = flameParams.globalTranslation;
            outResult.lightingParams = flameParams.lighting;
        }
        
        // Step 4: Extract texture
        if (config_.extractTexture) {
            FaceTextureExtractor::extractTexture(
                imageData, width, height, channels,
                landmarks,
                outResult.textureData,
                config_.textureSize);
            
            outResult.textureWidth = config_.textureSize;
            outResult.textureHeight = config_.textureSize;
        }
        
        // Calculate confidence
        outResult.poseConfidence = detection.confidence;
        outResult.expressionConfidence = landmarks.confidence;
        outResult.overallConfidence = (outResult.poseConfidence + outResult.expressionConfidence) * 0.5f;
        
        outResult.success = true;
        return true;
    }
    
    // Convert result to CharacterFace parameters
    bool applyToCharacterFace(const PhotoFaceResult& result, CharacterFace& face) {
        if (!result.success) return false;
        
        // Map 3DMM to face shape params
        if (!result.shapeParams.empty()) {
            FLAME3DMMParams flame;
            flame.shape = result.shapeParams;
            flame.expression = result.expressionParams;
            flame.globalRotation = result.headRotation;
            
            FaceParameterMapper::mapToFaceShapeParams(flame, face.getShapeParams());
            FaceParameterMapper::mapToExpressionParams(flame, face.getExpressionParams());
        }
        
        // Also use direct landmark mapping for missing parameters
        if (!result.landmarks.empty()) {
            FaceLandmarks landmarks;
            for (size_t i = 0; i < result.landmarks.size() && i < FACE_MESH_LANDMARK_COUNT; i++) {
                landmarks.points[i] = result.landmarks[i];
            }
            FaceParameterMapper::mapLandmarksToFaceParams(landmarks, face.getShapeParams());
        }
        
        // Apply texture
        if (!result.textureData.empty()) {
            face.applyPhotoFaceResult(result);
        }
        
        // Extract and apply skin color
        if (!result.textureData.empty()) {
            // Sample skin color from texture
            Vec3 skinColor(0, 0, 0);
            int sampleCount = 0;
            int texSize = result.textureWidth;
            
            // Sample from center-ish region
            for (int y = texSize / 3; y < texSize * 2 / 3; y += 10) {
                for (int x = texSize / 3; x < texSize * 2 / 3; x += 10) {
                    int idx = (y * texSize + x) * 4;
                    skinColor.x += result.textureData[idx + 0] / 255.0f;
                    skinColor.y += result.textureData[idx + 1] / 255.0f;
                    skinColor.z += result.textureData[idx + 2] / 255.0f;
                    sampleCount++;
                }
            }
            
            if (sampleCount > 0) {
                skinColor = skinColor * (1.0f / sampleCount);
                face.getTextureParams().skinTone = skinColor;
            }
        }
        
        return true;
    }
    
private:
    Config config_;
    bool initialized_ = false;
    
    FaceDetector faceDetector_;
    FaceMeshEstimator faceMesh_;
    Face3DMMRegressor face3DMM_;
};

} // namespace luma
