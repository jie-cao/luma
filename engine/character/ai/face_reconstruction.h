// Face Reconstruction Pipeline - Photo to 3D face using ONNX models
// Supports InsightFace (detection + landmarks) and 3DDFA (3DMM regression)
// Falls back to geometric heuristics when models are not available.
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/ai/onnx_inference.h"
#include "engine/character/character_face.h"
#include "engine/character/blend_shape.h"
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace luma {

// ============================================================================
// Constants and Landmark Indices
// ============================================================================

// 68-point landmark indices (dlib / InsightFace compatible)
namespace Landmark68 {
    // Jaw contour: 0-16
    constexpr int JAW_START = 0;
    constexpr int JAW_END = 16;
    constexpr int CHIN = 8;

    // Right eyebrow: 17-21
    constexpr int RIGHT_BROW_START = 17;
    constexpr int RIGHT_BROW_END = 21;

    // Left eyebrow: 22-26
    constexpr int LEFT_BROW_START = 22;
    constexpr int LEFT_BROW_END = 26;

    // Nose ridge: 27-30
    constexpr int NOSE_BRIDGE_TOP = 27;
    constexpr int NOSE_TIP = 30;

    // Nose bottom: 31-35
    constexpr int NOSE_LEFT = 31;
    constexpr int NOSE_RIGHT = 35;

    // Right eye: 36-41
    constexpr int RIGHT_EYE_LEFT = 36;
    constexpr int RIGHT_EYE_RIGHT = 39;

    // Left eye: 42-47
    constexpr int LEFT_EYE_LEFT = 42;
    constexpr int LEFT_EYE_RIGHT = 45;

    // Outer mouth: 48-59
    constexpr int MOUTH_LEFT = 48;
    constexpr int MOUTH_RIGHT = 54;
    constexpr int MOUTH_TOP = 51;
    constexpr int MOUTH_BOTTOM = 57;

    // Inner mouth: 60-67
}

// ============================================================================
// Data Structures
// ============================================================================

struct FaceDetection {
    float x1, y1, x2, y2;  // Bounding box in pixels
    float confidence = 0.0f;
    // 5-point landmarks from detection (eyes, nose, mouth corners)
    Vec2 keypoints[5];

    float width() const { return x2 - x1; }
    float height() const { return y2 - y1; }
    Vec2 center() const { return Vec2((x1+x2)*0.5f, (y1+y2)*0.5f); }
};

struct FaceLandmarks {
    std::vector<Vec2> points;  // 2D landmarks in image pixels
    std::vector<Vec3> points3D; // 3D landmarks (if available)
    float confidence = 0.0f;
    int numPoints = 0;

    // Compute geometric measurements (relative to face bounding box)
    float getInterocularDistance() const {
        if (numPoints < 68) return 0.0f;
        Vec2 leftEye = (points[Landmark68::LEFT_EYE_LEFT] + points[Landmark68::LEFT_EYE_RIGHT]) * 0.5f;
        Vec2 rightEye = (points[Landmark68::RIGHT_EYE_LEFT] + points[Landmark68::RIGHT_EYE_RIGHT]) * 0.5f;
        return (leftEye - rightEye).length();
    }

    float getFaceWidth() const {
        if (numPoints < 68) return 0.0f;
        return (points[Landmark68::JAW_START] - points[Landmark68::JAW_END]).length();
    }

    float getFaceHeight() const {
        if (numPoints < 68) return 0.0f;
        Vec2 browCenter = (points[Landmark68::RIGHT_BROW_START] + points[Landmark68::LEFT_BROW_END]) * 0.5f;
        return (browCenter - points[Landmark68::CHIN]).length();
    }

    float getNoseWidth() const {
        if (numPoints < 68) return 0.0f;
        return (points[Landmark68::NOSE_LEFT] - points[Landmark68::NOSE_RIGHT]).length();
    }

    float getMouthWidth() const {
        if (numPoints < 68) return 0.0f;
        return (points[Landmark68::MOUTH_LEFT] - points[Landmark68::MOUTH_RIGHT]).length();
    }

    float getNoseLength() const {
        if (numPoints < 68) return 0.0f;
        return (points[Landmark68::NOSE_BRIDGE_TOP] - points[Landmark68::NOSE_TIP]).length();
    }
};

struct Face3DMMResult {
    std::vector<float> identityCoeffs;   // Shape identity coefficients
    std::vector<float> expressionCoeffs; // Expression coefficients
    float pose[12] = {};                  // 3x4 rotation+translation
    float confidence = 0.0f;
};

// ============================================================================
// Face Detector (InsightFace RetinaFace via ONNX)
// ============================================================================

class FaceDetector {
public:
    bool initialize(const std::string& modelPath) {
        modelPath_ = modelPath;
        printf("[FaceDetector] Checking model file: %s\n", modelPath.c_str());
        bool fileExists = OnnxModelManager::modelFileExists(modelPath);
        printf("[FaceDetector] File exists: %d\n", fileExists);
        if (fileExists) {
            hasModel_ = onnxSession_.loadModel(modelPath);
            printf("[FaceDetector] Model load result: %d\n", hasModel_);
            if (hasModel_) {
                printf("[FaceDetector] Successfully loaded ONNX model: %s\n", modelPath.c_str());
            } else {
                printf("[FaceDetector] FAILED to load model: %s\n", modelPath.c_str());
            }
        } else {
            printf("[FaceDetector] Model file not found, will use fallback\n");
        }
        initialized_ = true;
        return hasModel_;
    }

    std::vector<FaceDetection> detect(const uint8_t* imageData,
                                       int width, int height, int channels) {
        std::vector<FaceDetection> results;

        if (hasModel_) {
            results = detectWithOnnx(imageData, width, height, channels);
        }

        if (results.empty()) {
            results = detectFallback(imageData, width, height, channels);
        }

        return results;
    }

    bool detectSingle(const uint8_t* imageData, int width, int height, int channels,
                      FaceDetection& outDet) {
        auto dets = detect(imageData, width, height, channels);
        if (dets.empty()) return false;

        // Return largest face
        float maxArea = 0;
        for (const auto& d : dets) {
            float area = d.width() * d.height();
            if (area > maxArea) { maxArea = area; outDet = d; }
        }
        return true;
    }

private:
    std::vector<FaceDetection> detectWithOnnx(const uint8_t* imageData,
                                               int w, int h, int ch) {
        std::vector<FaceDetection> results;

        // Prepare input: resize to 640x640, normalize with InsightFace mean/std, NCHW format
        int targetSize = 640;
        std::vector<float> inputData(3 * targetSize * targetSize);

        float scaleX = (float)w / targetSize;
        float scaleY = (float)h / targetSize;

        // InsightFace/SCRFD normalization: (pixel - 127.5) / 128.0, BGR order
        for (int y = 0; y < targetSize; y++) {
            for (int x = 0; x < targetSize; x++) {
                int srcX = std::min((int)(x * scaleX), w - 1);
                int srcY = std::min((int)(y * scaleY), h - 1);
                int srcIdx = (srcY * w + srcX) * ch;

                for (int c = 0; c < 3 && c < ch; c++) {
                    // RGB to BGR conversion
                    int outC = 2 - c;
                    inputData[outC * targetSize * targetSize + y * targetSize + x] =
                        (imageData[srcIdx + c] - 127.5f) / 128.0f;
                }
            }
        }

        std::vector<int64_t> inputShape = {1, 3, targetSize, targetSize};
        std::vector<std::vector<float>> outputs;

        if (!onnxSession_.runMultiOutput(inputData, inputShape, outputs)) {
            printf("[FaceDetector] ONNX inference failed\n");
            return results;
        }

        printf("[FaceDetector] Got %zu outputs from model\n", outputs.size());

        // SCRFD det_10g has 9 outputs: 3 scales × (scores, bboxes, keypoints)
        // Output order depends on model, let's detect it from output shapes
        if (outputs.size() >= 9) {
            const int strides[] = {8, 16, 32};
            const int fmc = 3;
            const int numAnchors = 2;

            // Debug: print output sizes
            printf("[FaceDetector] Output sizes: ");
            for (size_t i = 0; i < outputs.size(); i++) {
                printf("%zu ", outputs[i].size());
            }
            printf("\n");

            // SCRFD output format: for each stride level
            // - scores: [N, num_anchors, H, W] or flattened [H*W*num_anchors]
            // - bbox: [N, num_anchors*4, H, W] or flattened [H*W*num_anchors*4]
            // - kps: [N, num_anchors*10, H, W] or flattened [H*W*num_anchors*10]

            for (int idx = 0; idx < fmc; idx++) {
                int stride = strides[idx];
                int fmH = targetSize / stride;
                int fmW = targetSize / stride;
                int fmSize = fmH * fmW;

                // Outputs are typically: [scores0, scores1, scores2, bbox0, bbox1, bbox2, kps0, kps1, kps2]
                const auto& scores = outputs[idx];
                const auto& bboxes = outputs[idx + fmc];
                const auto& kps = outputs[idx + fmc * 2];

                printf("[FaceDetector] Scale %d: stride=%d, fm=%dx%d, scores=%zu, bbox=%zu, kps=%zu\n",
                       idx, stride, fmH, fmW, scores.size(), bboxes.size(), kps.size());

                // Expected sizes
                int expectedScores = fmSize * numAnchors;
                int expectedBbox = fmSize * numAnchors * 4;
                int expectedKps = fmSize * numAnchors * 10;

                // Check if scores need softmax (2 classes: bg, face)
                bool needSoftmax = (scores.size() == (size_t)(fmSize * numAnchors * 2));

                for (int ay = 0; ay < fmH; ay++) {
                    for (int ax = 0; ax < fmW; ax++) {
                        for (int an = 0; an < numAnchors; an++) {
                            int posIdx = ay * fmW + ax;
                            int anchorIdx = posIdx * numAnchors + an;

                            float score;
                            if (needSoftmax) {
                                // 2-class output: [bg_score, face_score]
                                int scoreIdx = anchorIdx * 2;
                                if (scoreIdx + 1 >= (int)scores.size()) continue;
                                float bg = scores[scoreIdx];
                                float fg = scores[scoreIdx + 1];
                                // Softmax
                                float maxVal = std::max(bg, fg);
                                float expBg = std::exp(bg - maxVal);
                                float expFg = std::exp(fg - maxVal);
                                score = expFg / (expBg + expFg);
                            } else {
                                // Single score output
                                if (anchorIdx >= (int)scores.size()) continue;
                                score = scores[anchorIdx];
                                // Apply sigmoid if raw logit
                                if (score < 0.0f) {
                                    score = 1.0f / (1.0f + std::exp(-score));
                                }
                            }

                            if (score < 0.5f) continue;

                            // Anchor center
                            float anchorX = (ax + 0.5f) * stride;
                            float anchorY = (ay + 0.5f) * stride;

                            // Decode bbox using distance2bbox
                            // SCRFD outputs distances: [left, top, right, bottom] from anchor
                            int bboxIdx = anchorIdx * 4;
                            if (bboxIdx + 3 >= (int)bboxes.size()) continue;

                            float left = bboxes[bboxIdx + 0] * stride;
                            float top = bboxes[bboxIdx + 1] * stride;
                            float right = bboxes[bboxIdx + 2] * stride;
                            float bottom = bboxes[bboxIdx + 3] * stride;

                            FaceDetection det;
                            det.x1 = (anchorX - left) * scaleX;
                            det.y1 = (anchorY - top) * scaleY;
                            det.x2 = (anchorX + right) * scaleX;
                            det.y2 = (anchorY + bottom) * scaleY;
                            det.confidence = score;

                            // Decode keypoints using distance2kps
                            int kpsIdx = anchorIdx * 10;
                            if (kpsIdx + 9 < (int)kps.size()) {
                                for (int k = 0; k < 5; k++) {
                                    float kpX = anchorX + kps[kpsIdx + k * 2] * stride;
                                    float kpY = anchorY + kps[kpsIdx + k * 2 + 1] * stride;
                                    det.keypoints[k].x = kpX * scaleX;
                                    det.keypoints[k].y = kpY * scaleY;
                                }
                            }

                            // Validity check
                            if (det.width() > 10 && det.height() > 10 &&
                                det.x1 >= -10 && det.y1 >= -10 &&
                                det.x2 <= w + 10 && det.y2 <= h + 10) {
                                results.push_back(det);
                            }
                        }
                    }
                }
            }

            printf("[FaceDetector] Found %zu candidates before NMS\n", results.size());

            // NMS
            std::sort(results.begin(), results.end(),
                [](const FaceDetection& a, const FaceDetection& b) {
                    return a.confidence > b.confidence;
                });

            std::vector<FaceDetection> nmsResults;
            std::vector<bool> suppressed(results.size(), false);

            for (size_t i = 0; i < results.size(); i++) {
                if (suppressed[i]) continue;
                nmsResults.push_back(results[i]);

                for (size_t j = i + 1; j < results.size(); j++) {
                    if (suppressed[j]) continue;
                    float ix1 = std::max(results[i].x1, results[j].x1);
                    float iy1 = std::max(results[i].y1, results[j].y1);
                    float ix2 = std::min(results[i].x2, results[j].x2);
                    float iy2 = std::min(results[i].y2, results[j].y2);
                    float iw = std::max(0.0f, ix2 - ix1);
                    float ih = std::max(0.0f, iy2 - iy1);
                    float inter = iw * ih;
                    float areaI = results[i].width() * results[i].height();
                    float areaJ = results[j].width() * results[j].height();
                    float iou = inter / (areaI + areaJ - inter + 1e-6f);
                    if (iou > 0.4f) suppressed[j] = true;
                }
            }

            results = std::move(nmsResults);
            printf("[FaceDetector] ONNX detected %zu faces after NMS\n", results.size());

            if (!results.empty()) {
                printf("[FaceDetector] Best face: conf=%.2f, bbox=(%.0f,%.0f)-(%.0f,%.0f)\n",
                       results[0].confidence, results[0].x1, results[0].y1,
                       results[0].x2, results[0].y2);
            }
        }

        return results;
    }

    // Fallback: skin color based face detection
    std::vector<FaceDetection> detectFallback(const uint8_t* imageData,
                                               int w, int h, int ch) {
        std::vector<FaceDetection> results;
        if (!imageData || ch < 3) return results;

        // Simple skin-color histogram approach
        int gridSize = 16;
        int cellW = w / gridSize;
        int cellH = h / gridSize;
        std::vector<float> skinMap(gridSize * gridSize, 0);

        for (int gy = 0; gy < gridSize; gy++) {
            for (int gx = 0; gx < gridSize; gx++) {
                int skinCount = 0;
                int total = 0;

                for (int y = gy * cellH; y < std::min((gy+1) * cellH, h); y += 2) {
                    for (int x = gx * cellW; x < std::min((gx+1) * cellW, w); x += 2) {
                        int idx = (y * w + x) * ch;
                        int r = imageData[idx], g = imageData[idx+1], b = imageData[idx+2];

                        // Skin color heuristic (RGB)
                        bool isSkin = (r > 95 && g > 40 && b > 20 &&
                                      r > g && r > b &&
                                      std::abs(r - g) > 15 &&
                                      r - b > 15);
                        if (isSkin) skinCount++;
                        total++;
                    }
                }
                skinMap[gy * gridSize + gx] = (total > 0) ? (float)skinCount / total : 0.0f;
            }
        }

        // Find the region with highest skin density
        float maxDensity = 0;
        int bestGx = gridSize/2, bestGy = gridSize/3;

        for (int gy = 1; gy < gridSize - 2; gy++) {
            for (int gx = 2; gx < gridSize - 2; gx++) {
                float density = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        density += skinMap[(gy+dy) * gridSize + (gx+dx)];
                    }
                }
                if (density > maxDensity) {
                    maxDensity = density;
                    bestGx = gx; bestGy = gy;
                }
            }
        }

        FaceDetection det;
        float cx = (bestGx + 0.5f) * cellW;
        float cy = (bestGy + 0.5f) * cellH;
        float faceSize = std::min(w, h) * 0.35f;

        det.x1 = cx - faceSize * 0.5f;
        det.y1 = cy - faceSize * 0.6f;
        det.x2 = cx + faceSize * 0.5f;
        det.y2 = cy + faceSize * 0.4f;
        det.confidence = std::clamp(maxDensity / 5.0f, 0.3f, 0.9f);

        // Estimate keypoints from bounding box
        float fw = det.width(), fh = det.height();
        det.keypoints[0] = Vec2(det.x1 + fw*0.35f, det.y1 + fh*0.35f); // Right eye
        det.keypoints[1] = Vec2(det.x1 + fw*0.65f, det.y1 + fh*0.35f); // Left eye
        det.keypoints[2] = Vec2(det.x1 + fw*0.50f, det.y1 + fh*0.55f); // Nose
        det.keypoints[3] = Vec2(det.x1 + fw*0.37f, det.y1 + fh*0.75f); // Right mouth
        det.keypoints[4] = Vec2(det.x1 + fw*0.63f, det.y1 + fh*0.75f); // Left mouth

        results.push_back(det);
        printf("[FaceDetector] Fallback detection: conf=%.2f at (%.0f,%.0f)-(%.0f,%.0f)\n",
               det.confidence, det.x1, det.y1, det.x2, det.y2);
        return results;
    }

    bool initialized_ = false;
    bool hasModel_ = false;
    std::string modelPath_;
    OnnxSession onnxSession_;
};

// ============================================================================
// Face Landmark Detector (InsightFace 2d106det or 68-point)
// ============================================================================

class FaceLandmarkDetector {
public:
    bool initialize(const std::string& modelPath) {
        modelPath_ = modelPath;
        if (OnnxModelManager::modelFileExists(modelPath)) {
            hasModel_ = onnxSession_.loadModel(modelPath);
            if (hasModel_) {
                printf("[FaceLandmarkDetector] Loaded ONNX model: %s\n", modelPath.c_str());
            }
        }
        initialized_ = true;
        return true;
    }

    bool detect(const uint8_t* imageData, int width, int height, int channels,
                const FaceDetection& face, FaceLandmarks& outLandmarks) {
        if (hasModel_) {
            return detectWithOnnx(imageData, width, height, channels, face, outLandmarks);
        }
        return detectFallback(imageData, width, height, channels, face, outLandmarks);
    }

private:
    bool detectWithOnnx(const uint8_t* imageData, int w, int h, int ch,
                        const FaceDetection& face, FaceLandmarks& out) {
        // 2d106det expects 192x192 input with InsightFace normalization
        int cropSize = 192;

        // Compute square crop centered on face
        float cx = (face.x1 + face.x2) * 0.5f;
        float cy = (face.y1 + face.y2) * 0.5f;
        float faceSize = std::max(face.width(), face.height());
        float scale = faceSize * 1.5f; // Add margin

        float fx1 = cx - scale * 0.5f;
        float fy1 = cy - scale * 0.5f;
        float fx2 = cx + scale * 0.5f;
        float fy2 = cy + scale * 0.5f;

        // Clamp to image bounds
        fx1 = std::max(0.0f, fx1);
        fy1 = std::max(0.0f, fy1);
        fx2 = std::min((float)w, fx2);
        fy2 = std::min((float)h, fy2);

        float cropW = fx2 - fx1;
        float cropH = fy2 - fy1;

        // Resize cropped face to model input size, NCHW format with InsightFace normalization
        std::vector<float> inputData(3 * cropSize * cropSize);
        float scaleX = cropW / cropSize;
        float scaleY = cropH / cropSize;

        for (int y = 0; y < cropSize; y++) {
            for (int x = 0; x < cropSize; x++) {
                int srcX = std::min((int)(fx1 + x * scaleX), w - 1);
                int srcY = std::min((int)(fy1 + y * scaleY), h - 1);
                int srcIdx = (srcY * w + srcX) * ch;

                for (int c = 0; c < 3 && c < ch; c++) {
                    // InsightFace normalization: (pixel - 127.5) / 128.0, BGR order
                    int outC = 2 - c; // RGB to BGR
                    inputData[outC * cropSize * cropSize + y * cropSize + x] =
                        (imageData[srcIdx + c] - 127.5f) / 128.0f;
                }
            }
        }

        std::vector<int64_t> inputShape = {1, 3, cropSize, cropSize};
        std::vector<float> outputData;

        if (!onnxSession_.run(inputData, inputShape, outputData)) {
            printf("[FaceLandmarkDetector] ONNX inference failed\n");
            return false;
        }

        printf("[FaceLandmarkDetector] ONNX output size: %zu\n", outputData.size());

        // 2d106det outputs 106 landmarks as 212 values (x,y pairs)
        // Output coordinates are normalized to [-1, 1] range (center-origin)
        int numPts = (int)outputData.size() / 2;
        if (numPts < 68) {
            printf("[FaceLandmarkDetector] Too few points: %d\n", numPts);
            return false;
        }

        out.points.resize(numPts);
        out.numPoints = numPts;

        // Debug: print first few output values
        printf("[FaceLandmarkDetector] Raw output[0..5]: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n",
               outputData[0], outputData[1], outputData[2], outputData[3], outputData[4], outputData[5]);

        // Compute crop center in original image
        float cropCenterX = (fx1 + fx2) * 0.5f;
        float cropCenterY = (fy1 + fy2) * 0.5f;

        for (int i = 0; i < numPts; i++) {
            // Output is in [-1, 1] range, centered at crop center
            float nx = outputData[i * 2 + 0];
            float ny = outputData[i * 2 + 1];

            // Map from [-1, 1] to original image coordinates
            // nx=-1 -> left edge of crop, nx=1 -> right edge
            float lx = cropCenterX + nx * (cropW * 0.5f);
            float ly = cropCenterY + ny * (cropH * 0.5f);
            out.points[i] = Vec2(lx, ly);
        }

        // Convert 106-point to 68-point if needed (for compatibility)
        if (numPts == 106) {
            convert106to68(out);
        }

        out.confidence = 0.9f;
        printf("[FaceLandmarkDetector] ONNX success: %d points\n", out.numPoints);
        return true;
    }

    // Convert InsightFace 106-point landmarks to standard 68-point format
    // Using the correct mapping from InsightFace documentation
    void convert106to68(FaceLandmarks& lm) {
        if (lm.numPoints != 106) return;

        // Correct mapping from 106 to 68 points (InsightFace format)
        static const int mapping[68] = {
            // Jaw contour (0-16): 17 points
            1, 10, 12, 14, 16, 3, 5, 7, 0, 23, 21, 19, 32, 30, 28, 26, 17,
            // Left eyebrow (17-21): 5 points
            43, 48, 49, 51, 50,
            // Right eyebrow (22-26): 5 points
            102, 103, 104, 105, 101,
            // Nose (27-35): 9 points
            72, 73, 74, 86, 78, 79, 80, 85, 84,
            // Left eye (36-41): 6 points
            35, 41, 42, 39, 37, 36,
            // Right eye (42-47): 6 points
            89, 95, 96, 93, 91, 90,
            // Mouth outer (48-59): 12 points
            52, 64, 63, 71, 67, 68, 61, 58, 59, 53, 56, 55,
            // Mouth inner (60-67): 8 points
            65, 66, 62, 70, 69, 57, 60, 54
        };

        std::vector<Vec2> pts68(68);
        for (int i = 0; i < 68; i++) {
            int srcIdx = mapping[i];
            if (srcIdx < (int)lm.points.size()) {
                pts68[i] = lm.points[srcIdx];
            }
        }

        lm.points = pts68;
        lm.numPoints = 68;
    }

    // Fallback: estimate 68 landmarks from face detection keypoints + geometric heuristics
    bool detectFallback(const uint8_t* imageData, int w, int h, int ch,
                        const FaceDetection& face, FaceLandmarks& out) {
        (void)imageData; (void)w; (void)h; (void)ch;

        out.points.resize(68);
        out.numPoints = 68;

        float fx = face.x1, fy = face.y1;
        float fw = face.width(), fh = face.height();

        // Generate synthetic 68-point landmarks based on face bounding box
        // Jaw contour (0-16): along the bottom of the face
        for (int i = 0; i <= 16; i++) {
            float t = (float)i / 16.0f;
            float x = fx + fw * t;
            float y;
            if (t < 0.5f) {
                y = fy + fh * (0.4f + 0.55f * (0.5f - t) * 2.0f * (0.5f - t) * 2.0f + 0.3f * t * 2.0f);
            } else {
                float tt = (t - 0.5f) * 2.0f;
                y = fy + fh * (0.4f + 0.55f * tt * tt + 0.3f * (1.0f - tt));
            }
            if (i == 8) y = fy + fh * 0.95f; // Chin is at bottom
            out.points[i] = Vec2(x, std::min(y, fy + fh * 0.98f));
        }

        // Right eyebrow (17-21)
        for (int i = 0; i < 5; i++) {
            float t = (float)i / 4.0f;
            out.points[17 + i] = Vec2(fx + fw * (0.18f + t * 0.20f),
                                       fy + fh * (0.28f - 0.02f * std::sin(t * 3.14159f)));
        }

        // Left eyebrow (22-26)
        for (int i = 0; i < 5; i++) {
            float t = (float)i / 4.0f;
            out.points[22 + i] = Vec2(fx + fw * (0.62f + t * 0.20f),
                                       fy + fh * (0.28f - 0.02f * std::sin(t * 3.14159f)));
        }

        // Nose bridge (27-30)
        for (int i = 0; i < 4; i++) {
            float t = (float)i / 3.0f;
            out.points[27 + i] = Vec2(fx + fw * 0.5f, fy + fh * (0.32f + t * 0.21f));
        }

        // Nose bottom (31-35)
        for (int i = 0; i < 5; i++) {
            float t = (float)i / 4.0f;
            out.points[31 + i] = Vec2(fx + fw * (0.38f + t * 0.24f), fy + fh * 0.56f);
        }

        // Right eye (36-41)
        float reCx = fx + fw * 0.30f, reCy = fy + fh * 0.36f;
        float reW = fw * 0.11f, reH = fh * 0.04f;
        for (int i = 0; i < 6; i++) {
            float a = (float)i / 6.0f * 2.0f * 3.14159f;
            out.points[36 + i] = Vec2(reCx + reW * std::cos(a), reCy + reH * std::sin(a));
        }

        // Left eye (42-47)
        float leCx = fx + fw * 0.70f, leCy = fy + fh * 0.36f;
        float leW = fw * 0.11f, leH = fh * 0.04f;
        for (int i = 0; i < 6; i++) {
            float a = (float)i / 6.0f * 2.0f * 3.14159f;
            out.points[42 + i] = Vec2(leCx + leW * std::cos(a), leCy + leH * std::sin(a));
        }

        // Outer mouth (48-59)
        float mCx = fx + fw * 0.5f, mCy = fy + fh * 0.73f;
        float mW = fw * 0.16f, mH = fh * 0.05f;
        for (int i = 0; i < 12; i++) {
            float a = (float)i / 12.0f * 2.0f * 3.14159f;
            out.points[48 + i] = Vec2(mCx + mW * std::cos(a), mCy + mH * std::sin(a));
        }

        // Inner mouth (60-67)
        for (int i = 0; i < 8; i++) {
            float a = (float)i / 8.0f * 2.0f * 3.14159f;
            out.points[60 + i] = Vec2(mCx + mW*0.6f * std::cos(a), mCy + mH*0.5f * std::sin(a));
        }

        out.confidence = 0.5f;
        printf("[FaceLandmarkDetector] Generated %d fallback landmarks\n", out.numPoints);
        return true;
    }

    bool initialized_ = false;
    bool hasModel_ = false;
    std::string modelPath_;
    OnnxSession onnxSession_;
};

// ============================================================================
// 3DMM Regressor (3DDFA_V2 via ONNX)
// ============================================================================

class Face3DMMRegressor {
public:
    bool initialize(const std::string& modelPath) {
        modelPath_ = modelPath;
        if (OnnxModelManager::modelFileExists(modelPath)) {
            hasModel_ = onnxSession_.loadModel(modelPath);
            if (hasModel_) {
                printf("[Face3DMMRegressor] Loaded ONNX model: %s\n", modelPath.c_str());
            }
        }
        initialized_ = true;
        return true;
    }

    bool regress(const uint8_t* imageData, int width, int height, int channels,
                 const FaceDetection& face, Face3DMMResult& outResult) {
        if (hasModel_) {
            return regressWithOnnx(imageData, width, height, channels, face, outResult);
        }
        return regressFromDetection(face, outResult);
    }

    // Regress from landmarks (more geometric, less ML)
    bool regressFromLandmarks(const FaceLandmarks& landmarks,
                               const FaceDetection& face,
                               Face3DMMResult& outResult) {
        if (landmarks.numPoints < 68) return false;

        float iod = landmarks.getInterocularDistance();
        if (iod < 1.0f) return false;

        // Compute geometric ratios normalized by interocular distance
        float faceW = landmarks.getFaceWidth() / iod;
        float faceH = landmarks.getFaceHeight() / iod;
        float noseW = landmarks.getNoseWidth() / iod;
        float noseL = landmarks.getNoseLength() / iod;
        float mouthW = landmarks.getMouthWidth() / iod;

        // Eye dimensions
        float rightEyeW = (landmarks.points[Landmark68::RIGHT_EYE_RIGHT] -
                           landmarks.points[Landmark68::RIGHT_EYE_LEFT]).length() / iod;
        float leftEyeW = (landmarks.points[Landmark68::LEFT_EYE_RIGHT] -
                          landmarks.points[Landmark68::LEFT_EYE_LEFT]).length() / iod;
        float avgEyeW = (rightEyeW + leftEyeW) * 0.5f;

        // Jaw width (at jaw angle points, indices 4 and 12)
        float jawW = (landmarks.points[4] - landmarks.points[12]).length() / iod;

        // Chin length
        float chinL = (landmarks.points[Landmark68::CHIN] -
                       landmarks.points[Landmark68::MOUTH_BOTTOM]).length() / iod;

        // Mouth height
        float mouthH = (landmarks.points[Landmark68::MOUTH_TOP] -
                        landmarks.points[Landmark68::MOUTH_BOTTOM]).length() / iod;

        // Brow height
        float browH = ((landmarks.points[19] + landmarks.points[24]) * 0.5f -
                       (landmarks.points[Landmark68::RIGHT_EYE_LEFT] +
                        landmarks.points[Landmark68::LEFT_EYE_RIGHT]) * 0.5f).length() / iod;

        // Convert ratios to 3DMM-like coefficients centered around 0
        // Average human proportions serve as baseline
        outResult.identityCoeffs.resize(40, 0.0f);

        outResult.identityCoeffs[0]  = (faceW - 3.0f) * 1.5f;        // Face width
        outResult.identityCoeffs[1]  = (faceH - 3.5f) * 1.2f;        // Face height
        outResult.identityCoeffs[2]  = (avgEyeW - 0.6f) * 3.0f;      // Eye size
        outResult.identityCoeffs[3]  = 0.0f;                           // Eye spacing (already normalized)
        outResult.identityCoeffs[4]  = (noseW - 0.65f) * 3.0f;        // Nose width
        outResult.identityCoeffs[5]  = (noseL - 0.75f) * 2.5f;        // Nose length
        outResult.identityCoeffs[6]  = (mouthW - 1.0f) * 2.0f;        // Mouth width
        outResult.identityCoeffs[7]  = (mouthH - 0.25f) * 4.0f;       // Lip thickness
        outResult.identityCoeffs[8]  = (jawW - 2.3f) * 1.5f;          // Jaw width
        outResult.identityCoeffs[9]  = (chinL - 0.5f) * 3.0f;         // Chin length
        outResult.identityCoeffs[10] = (browH - 0.35f) * 4.0f;        // Brow height

        // Estimate face shape from jaw contour curvature
        float jawCurvature = estimateJawCurvature(landmarks);
        outResult.identityCoeffs[11] = jawCurvature * 2.0f;            // Face roundness

        // Head pose estimation from eye/nose triangle
        Vec2 eyeCenter = (landmarks.points[Landmark68::RIGHT_EYE_LEFT] +
                          landmarks.points[Landmark68::LEFT_EYE_RIGHT]) * 0.5f;
        Vec2 nose = landmarks.points[Landmark68::NOSE_TIP];
        float yawEstimate = (nose.x - eyeCenter.x) / iod;

        outResult.pose[0] = 1; outResult.pose[5] = 1; outResult.pose[10] = 1;
        outResult.pose[4] = yawEstimate * 0.5f; // Rough yaw

        outResult.expressionCoeffs.resize(29, 0.0f);
        outResult.confidence = landmarks.confidence * 0.8f;

        return true;
    }

private:
    bool regressWithOnnx(const uint8_t* imageData, int w, int h, int ch,
                          const FaceDetection& face, Face3DMMResult& out) {
        int inputSize = 120;

        // Compute square crop centered on face
        float cx = (face.x1 + face.x2) * 0.5f;
        float cy = (face.y1 + face.y2) * 0.5f;
        float faceSize = std::max(face.width(), face.height());
        float scale = faceSize * 1.2f; // Slight margin

        float fx1 = cx - scale * 0.5f;
        float fy1 = cy - scale * 0.5f;
        float fx2 = cx + scale * 0.5f;
        float fy2 = cy + scale * 0.5f;

        // Clamp to image bounds
        fx1 = std::max(0.0f, fx1);
        fy1 = std::max(0.0f, fy1);
        fx2 = std::min((float)w, fx2);
        fy2 = std::min((float)h, fy2);

        float cropW = fx2 - fx1, cropH = fy2 - fy1;
        float scaleX = cropW / inputSize;
        float scaleY = cropH / inputSize;

        std::vector<float> inputData(3 * inputSize * inputSize);
        for (int y = 0; y < inputSize; y++) {
            for (int x = 0; x < inputSize; x++) {
                int srcX = std::min((int)(fx1 + x * scaleX), w - 1);
                int srcY = std::min((int)(fy1 + y * scaleY), h - 1);
                int srcIdx = (srcY * w + srcX) * ch;

                for (int c = 0; c < 3 && c < ch; c++) {
                    // 3DDFA_V2 uses ImageNet normalization (RGB order)
                    float val = imageData[srcIdx + c] / 255.0f;
                    float mean[] = {0.485f, 0.456f, 0.406f};
                    float std[] = {0.229f, 0.224f, 0.225f};
                    inputData[c * inputSize * inputSize + y * inputSize + x] =
                        (val - mean[c]) / std[c];
                }
            }
        }

        std::vector<int64_t> inputShape = {1, 3, inputSize, inputSize};
        std::vector<float> outputData;

        if (!onnxSession_.run(inputData, inputShape, outputData)) {
            printf("[Face3DMMRegressor] ONNX inference failed\n");
            return false;
        }

        printf("[Face3DMMRegressor] ONNX output size: %zu\n", outputData.size());
        printf("[Face3DMMRegressor] Raw output[0..11]: %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f\n",
               outputData[0], outputData[1], outputData[2], outputData[3],
               outputData[4], outputData[5], outputData[6], outputData[7],
               outputData[8], outputData[9], outputData[10], outputData[11]);

        // 3DDFA_V2 output: 62 parameters total
        // Standard format: [0:12] = pose (3x4 matrix), [12:52] = shape (40), [52:62] = expression (10)
        if (outputData.size() >= 62) {
            // First 12 values are pose (3x4 rotation/translation matrix)
            std::memcpy(out.pose, outputData.data(), 12 * sizeof(float));

            // Next 40 values are shape/identity coefficients
            out.identityCoeffs.assign(outputData.begin() + 12, outputData.begin() + 52);

            // Last 10 values are expression coefficients
            out.expressionCoeffs.assign(outputData.begin() + 52, outputData.end());

            printf("[Face3DMMRegressor] Parsed: 12 pose + 40 identity + %zu expression\n",
                   out.expressionCoeffs.size());
            printf("[Face3DMMRegressor] Identity[0..4]: %.3f, %.3f, %.3f, %.3f, %.3f\n",
                   out.identityCoeffs[0], out.identityCoeffs[1], out.identityCoeffs[2],
                   out.identityCoeffs[3], out.identityCoeffs[4]);
        } else if (outputData.size() > 0) {
            // Fallback: treat all as identity coefficients
            out.identityCoeffs.assign(outputData.begin(), outputData.end());
            printf("[Face3DMMRegressor] Fallback: %zu coefficients as identity\n",
                   out.identityCoeffs.size());
        }

        out.confidence = 0.85f;
        return true;
    }

    bool regressFromDetection(const FaceDetection& face, Face3DMMResult& out) {
        // Minimal fallback when no landmarks or model available
        out.identityCoeffs.resize(40, 0.0f);
        out.expressionCoeffs.resize(29, 0.0f);

        // Use aspect ratio of face bounding box as a very rough shape estimate
        float aspect = face.width() / std::max(face.height(), 1.0f);
        out.identityCoeffs[0] = (aspect - 0.75f) * 3.0f; // Face width
        out.confidence = 0.3f;
        return true;
    }

    float estimateJawCurvature(const FaceLandmarks& lm) {
        // Measure how "round" the jaw contour is
        if (lm.numPoints < 17) return 0.0f;

        Vec2 mid = lm.points[8]; // Chin
        Vec2 left = lm.points[4];
        Vec2 right = lm.points[12];

        Vec2 midLine = (left + right) * 0.5f;
        float deviation = (mid - midLine).length();
        float jawSpan = (left - right).length();

        return (jawSpan > 0.0f) ? deviation / jawSpan : 0.0f;
    }

    bool initialized_ = false;
    bool hasModel_ = false;
    std::string modelPath_;
    OnnxSession onnxSession_;
};

// ============================================================================
// Multi-View Fusion
// ============================================================================

class MultiViewFusion {
public:
    struct ViewResult {
        Face3DMMResult coefficients;
        FaceLandmarks landmarks;
        float confidence = 0.0f;
        enum ViewType { Front, LeftProfile, RightProfile } viewType = Front;
    };

    // Fuse results from multiple views into unified FaceShapeParams
    static void fuse(const std::vector<ViewResult>& views, FaceShapeParams& outParams) {
        if (views.empty()) return;

        // Classify parameters as "width" (best from front) or "depth" (best from side)
        const ViewResult* frontView = nullptr;
        const ViewResult* leftView = nullptr;
        const ViewResult* rightView = nullptr;

        for (const auto& v : views) {
            switch (v.viewType) {
                case ViewResult::Front: frontView = &v; break;
                case ViewResult::LeftProfile: leftView = &v; break;
                case ViewResult::RightProfile: rightView = &v; break;
            }
        }

        // Start with front view (required)
        if (!frontView) frontView = &views[0];

        // Map front view coefficients to face shape params
        mapCoefficientsToParams(frontView->coefficients, frontView->landmarks, outParams);

        // If side views available, override depth-related parameters
        const ViewResult* sideView = leftView ? leftView : rightView;
        if (sideView && sideView->confidence > 0.3f) {
            overrideDepthParams(sideView->coefficients, sideView->landmarks, outParams);
        }
    }

private:
    static void mapCoefficientsToParams(const Face3DMMResult& coeffs,
                                         const FaceLandmarks& landmarks,
                                         FaceShapeParams& p) {
        // 3DDFA coefficients are typically in range [-5, 5] with mean ~0
        // We map them to [0, 1] range for our face params
        auto normalize3DMM = [](float v, float scale = 0.15f) -> float {
            // v is typically in [-5, 5], map to [0, 1] centered at 0.5
            return std::clamp(0.5f + v * scale, 0.0f, 1.0f);
        };

        if (coeffs.identityCoeffs.size() >= 12) {
            const auto& c = coeffs.identityCoeffs;

            printf("[MultiViewFusion] Mapping 3DMM coeffs to face params\n");

            // 3DDFA coefficients interpretation (BFM model):
            // First few coefficients control major shape variations
            p.faceWidth           = normalize3DMM(c[0], 0.12f);
            p.faceLength          = normalize3DMM(c[1], 0.08f);  // c[1] was 3.112, so scale down
            p.eyeSize             = normalize3DMM(c[2], 0.15f);
            p.eyeSpacing          = normalize3DMM(c[3], 0.15f);
            p.noseWidth           = normalize3DMM(c[4], 0.15f);
            p.noseLength          = normalize3DMM(c[5], 0.15f);
            p.mouthWidth          = normalize3DMM(c[6], 0.15f);
            p.upperLipThickness   = normalize3DMM(c[7], 0.12f);
            p.lowerLipThickness   = normalize3DMM(c[7], 0.14f);
            p.jawWidth            = normalize3DMM(c[8], 0.12f);
            p.chinLength          = normalize3DMM(c[9], 0.15f);
            p.browHeight          = normalize3DMM(c[10], 0.15f);
            p.faceRoundness       = normalize3DMM(c[11], 0.15f);

            printf("[MultiViewFusion] After 3DMM: faceWidth=%.2f, faceLength=%.2f, eyeSize=%.2f\n",
                   p.faceWidth, p.faceLength, p.eyeSize);
        }

        // Refine from landmarks if available
        if (landmarks.numPoints >= 68) {
            float iod = landmarks.getInterocularDistance();
            printf("[MultiViewFusion] Landmark IOD=%.1f\n", iod);

            if (iod > 10.0f) {  // Need reasonable IOD (at least 10 pixels)
                float faceW = landmarks.getFaceWidth() / iod;
                float noseW = landmarks.getNoseWidth() / iod;
                float mouthW = landmarks.getMouthWidth() / iod;

                printf("[MultiViewFusion] Landmark ratios: faceW=%.2f, noseW=%.2f, mouthW=%.2f\n",
                       faceW, noseW, mouthW);

                // Only refine if ratios are in reasonable range (typical human proportions)
                // Face width is typically 2.5-3.5x IOD, nose 0.4-0.8x, mouth 0.7-1.2x
                if (faceW > 1.5f && faceW < 5.0f) {
                    float faceWidthNorm = std::clamp((faceW - 2.5f) / 1.5f + 0.5f, 0.0f, 1.0f);
                    p.faceWidth = p.faceWidth * 0.5f + faceWidthNorm * 0.5f;
                }
                if (noseW > 0.2f && noseW < 1.2f) {
                    float noseWidthNorm = std::clamp((noseW - 0.5f) / 0.4f + 0.5f, 0.0f, 1.0f);
                    p.noseWidth = p.noseWidth * 0.5f + noseWidthNorm * 0.5f;
                }
                if (mouthW > 0.4f && mouthW < 1.8f) {
                    float mouthWidthNorm = std::clamp((mouthW - 0.8f) / 0.5f + 0.5f, 0.0f, 1.0f);
                    p.mouthWidth = p.mouthWidth * 0.5f + mouthWidthNorm * 0.5f;
                }

                printf("[MultiViewFusion] After landmark refinement: faceWidth=%.2f, noseWidth=%.2f, mouthWidth=%.2f\n",
                       p.faceWidth, p.noseWidth, p.mouthWidth);
            }
        }
    }

    static void overrideDepthParams(const Face3DMMResult& sideCoeffs,
                                     const FaceLandmarks& sideLandmarks,
                                     FaceShapeParams& p) {
        (void)sideLandmarks;
        auto normalize = [](float v) -> float {
            return std::clamp((v / 3.0f + 1.0f) * 0.5f, 0.0f, 1.0f);
        };

        if (sideCoeffs.identityCoeffs.size() >= 20) {
            const auto& c = sideCoeffs.identityCoeffs;
            // Depth-specific parameters get higher weight from side view
            if (c.size() > 12) p.noseHeight       = normalize(c[12]);
            if (c.size() > 13) p.chinProtrusion    = normalize(c[13]);
            if (c.size() > 14) p.foreheadSlope     = normalize(c[14]);
            if (c.size() > 15) p.lipProtrusion     = normalize(c[15]);
            if (c.size() > 16) p.eyeDepth          = normalize(c[16]);
            if (c.size() > 17) p.noseBridge        = normalize(c[17]);
            if (c.size() > 18) p.jawAngle          = normalize(c[18]);
        }
    }
};

// ============================================================================
// Face Texture Extractor
// ============================================================================

class FaceTextureExtractor {
public:
    // Extract face texture from photo using landmark-based UV projection
    static bool extractTexture(const uint8_t* imageData, int width, int height, int channels,
                                const FaceLandmarks& landmarks,
                                std::vector<uint8_t>& outTexture,
                                int textureSize = 512) {
        if (landmarks.numPoints < 68 || !imageData) return false;

        outTexture.resize(textureSize * textureSize * 4, 128);

        // Compute face bounding box from landmarks
        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        for (int i = 0; i < landmarks.numPoints; i++) {
            minX = std::min(minX, landmarks.points[i].x);
            minY = std::min(minY, landmarks.points[i].y);
            maxX = std::max(maxX, landmarks.points[i].x);
            maxY = std::max(maxY, landmarks.points[i].y);
        }

        float padX = (maxX - minX) * 0.1f;
        float padY = (maxY - minY) * 0.1f;
        minX -= padX; minY -= padY;
        maxX += padX; maxY += padY;

        // Project image onto texture map via bilinear sampling
        for (int ty = 0; ty < textureSize; ty++) {
            for (int tx = 0; tx < textureSize; tx++) {
                float u = (float)tx / textureSize;
                float v = (float)ty / textureSize;

                // Map UV to image coordinates
                float imgX = minX + u * (maxX - minX);
                float imgY = minY + v * (maxY - minY);

                int ix = std::clamp((int)imgX, 0, width - 1);
                int iy = std::clamp((int)imgY, 0, height - 1);

                int srcIdx = (iy * width + ix) * channels;
                int dstIdx = (ty * textureSize + tx) * 4;

                outTexture[dstIdx + 0] = imageData[srcIdx + 0];
                outTexture[dstIdx + 1] = (channels > 1) ? imageData[srcIdx + 1] : imageData[srcIdx];
                outTexture[dstIdx + 2] = (channels > 2) ? imageData[srcIdx + 2] : imageData[srcIdx];
                outTexture[dstIdx + 3] = 255;
            }
        }

        // Color correction: normalize brightness
        normalizeTextureBrightness(outTexture, textureSize);

        return true;
    }

    // Extract average skin color
    static Vec3 extractSkinColor(const uint8_t* imageData, int width, int height, int channels,
                                  const FaceLandmarks& landmarks) {
        if (landmarks.numPoints < 68 || !imageData) return Vec3(0.85f, 0.70f, 0.58f);

        // Sample from cheek regions (reliable skin areas)
        std::vector<Vec2> samplePoints;

        // Left cheek: between left eye and jaw
        Vec2 leftEye = (landmarks.points[42] + landmarks.points[45]) * 0.5f;
        Vec2 leftJaw = landmarks.points[4];
        samplePoints.push_back((leftEye + leftJaw) * 0.5f);

        // Right cheek
        Vec2 rightEye = (landmarks.points[36] + landmarks.points[39]) * 0.5f;
        Vec2 rightJaw = landmarks.points[12];
        samplePoints.push_back((rightEye + rightJaw) * 0.5f);

        // Forehead center
        Vec2 browCenter = (landmarks.points[19] + landmarks.points[24]) * 0.5f;
        samplePoints.push_back(Vec2(browCenter.x, browCenter.y - landmarks.getInterocularDistance() * 0.3f));

        Vec3 totalColor(0, 0, 0);
        int count = 0;
        int sampleRadius = std::max(2, width / 100);

        for (const auto& pt : samplePoints) {
            int cx = (int)pt.x, cy = (int)pt.y;
            for (int dy = -sampleRadius; dy <= sampleRadius; dy++) {
                for (int dx = -sampleRadius; dx <= sampleRadius; dx++) {
                    int x = std::clamp(cx + dx, 0, width - 1);
                    int y = std::clamp(cy + dy, 0, height - 1);
                    int idx = (y * width + x) * channels;

                    float r = imageData[idx] / 255.0f;
                    float g = (channels > 1) ? imageData[idx+1] / 255.0f : r;
                    float b = (channels > 2) ? imageData[idx+2] / 255.0f : r;

                    totalColor.x += r;
                    totalColor.y += g;
                    totalColor.z += b;
                    count++;
                }
            }
        }

        if (count > 0) {
            totalColor = totalColor * (1.0f / count);
        } else {
            totalColor = Vec3(0.85f, 0.70f, 0.58f);
        }

        return totalColor;
    }

private:
    static void normalizeTextureBrightness(std::vector<uint8_t>& texture, int size) {
        // Compute average brightness
        float avgBrightness = 0;
        int count = 0;
        for (int i = 0; i < size * size; i++) {
            int idx = i * 4;
            float lum = texture[idx] * 0.299f + texture[idx+1] * 0.587f + texture[idx+2] * 0.114f;
            avgBrightness += lum;
            count++;
        }
        avgBrightness /= count;

        // Target average brightness (~160 for neutral lighting)
        float targetBrightness = 160.0f;
        float scale = (avgBrightness > 10.0f) ? targetBrightness / avgBrightness : 1.0f;
        scale = std::clamp(scale, 0.5f, 2.0f);

        for (int i = 0; i < size * size; i++) {
            int idx = i * 4;
            texture[idx + 0] = (uint8_t)std::clamp(texture[idx+0] * scale, 0.0f, 255.0f);
            texture[idx + 1] = (uint8_t)std::clamp(texture[idx+1] * scale, 0.0f, 255.0f);
            texture[idx + 2] = (uint8_t)std::clamp(texture[idx+2] * scale, 0.0f, 255.0f);
        }
    }
};

// ============================================================================
// Complete Photo-to-Face Pipeline (orchestrates all components)
// ============================================================================

struct PhotoFacePipelineResult {
    bool success = false;
    std::string errorMessage;

    FaceDetection detection;
    FaceLandmarks landmarks;
    Face3DMMResult coefficients;
    FaceShapeParams faceParams;
    Vec3 skinColor;

    std::vector<uint8_t> textureData;
    int textureWidth = 0;
    int textureHeight = 0;

    float confidence = 0.0f;
};

class PhotoToFacePipeline {
public:
    struct Config {
        std::string modelDirectory = "models";
        std::string faceDetectorModel = "det_10g.onnx";
        std::string landmarkModel = "2d106det.onnx";
        std::string tddfaModel = "mb1_120x120.onnx";
        int textureSize = 512;
        bool extractTexture = true;
    };

    bool initialize(const Config& config) {
        config_ = config;
        std::string dir = config.modelDirectory;
        auto path = [&](const std::string& f) { return dir + "/" + f; };

        printf("[PhotoToFacePipeline] Initializing with models from: %s\n", dir.c_str());
        printf("[PhotoToFacePipeline]   Detector: %s\n", path(config.faceDetectorModel).c_str());
        printf("[PhotoToFacePipeline]   Landmark: %s\n", path(config.landmarkModel).c_str());
        printf("[PhotoToFacePipeline]   3DDFA: %s\n", path(config.tddfaModel).c_str());

        bool d = detector_.initialize(path(config.faceDetectorModel));
        bool l = landmarkDetector_.initialize(path(config.landmarkModel));
        bool r = regressor_.initialize(path(config.tddfaModel));

        printf("[PhotoToFacePipeline] Model load results: detector=%d, landmark=%d, regressor=%d\n", d, l, r);

        initialized_ = true;
        return true;
    }

    // Process a single photo
    PhotoFacePipelineResult process(const uint8_t* imageData,
                                     int width, int height, int channels) {
        PhotoFacePipelineResult result;

        if (!imageData || width <= 0 || height <= 0) {
            result.errorMessage = "Invalid image data";
            return result;
        }

        printf("[Pipeline] Processing image: %dx%d, %d channels\n", width, height, channels);

        // Step 1: Detect face
        FaceDetection det;
        if (!detector_.detectSingle(imageData, width, height, channels, det)) {
            result.errorMessage = "No face detected";
            return result;
        }
        result.detection = det;
        printf("[Pipeline] Face detected: conf=%.2f, bbox=(%.0f,%.0f)-(%.0f,%.0f)\n",
               det.confidence, det.x1, det.y1, det.x2, det.y2);

        // Step 2: Detect landmarks
        FaceLandmarks landmarks;
        bool landmarkOk = landmarkDetector_.detect(imageData, width, height, channels, det, landmarks);
        result.landmarks = landmarks;
        printf("[Pipeline] Landmarks: ok=%d, %d points, conf=%.2f\n",
               landmarkOk, landmarks.numPoints, landmarks.confidence);

        // Step 3: 3DMM regression
        Face3DMMResult coeffs;
        bool regressOk = regressor_.regress(imageData, width, height, channels, det, coeffs);
        printf("[Pipeline] 3DMM regression: ok=%d, %zu identity coeffs, conf=%.2f\n",
               regressOk, coeffs.identityCoeffs.size(), coeffs.confidence);

        // Print first few coefficients for debugging
        if (!coeffs.identityCoeffs.empty()) {
            printf("[Pipeline] 3DMM coeffs[0..4]: %.3f, %.3f, %.3f, %.3f, %.3f\n",
                   coeffs.identityCoeffs[0],
                   coeffs.identityCoeffs.size() > 1 ? coeffs.identityCoeffs[1] : 0.0f,
                   coeffs.identityCoeffs.size() > 2 ? coeffs.identityCoeffs[2] : 0.0f,
                   coeffs.identityCoeffs.size() > 3 ? coeffs.identityCoeffs[3] : 0.0f,
                   coeffs.identityCoeffs.size() > 4 ? coeffs.identityCoeffs[4] : 0.0f);
        }

        // Use 3DDFA coefficients directly (don't blend with landmark-based)
        // Landmark-based coefficients have different scale/range and cause issues
        // Instead, landmarks will be used in MultiViewFusion for refinement
        result.coefficients = coeffs;

        // Step 4: Map to FaceShapeParams
        MultiViewFusion::ViewResult view;
        view.coefficients = coeffs;
        view.landmarks = landmarks;
        view.confidence = std::max(det.confidence, landmarks.confidence);
        view.viewType = MultiViewFusion::ViewResult::Front;

        std::vector<MultiViewFusion::ViewResult> views = {view};
        MultiViewFusion::fuse(views, result.faceParams);

        // Debug: print final face params
        printf("[Pipeline] Final FaceParams: faceWidth=%.2f, faceLength=%.2f, eyeSize=%.2f, noseWidth=%.2f, mouthWidth=%.2f\n",
               result.faceParams.faceWidth, result.faceParams.faceLength,
               result.faceParams.eyeSize, result.faceParams.noseWidth, result.faceParams.mouthWidth);

        // Step 5: Extract skin color
        result.skinColor = FaceTextureExtractor::extractSkinColor(
            imageData, width, height, channels, landmarks);

        // Step 6: Extract texture
        if (config_.extractTexture) {
            FaceTextureExtractor::extractTexture(
                imageData, width, height, channels, landmarks,
                result.textureData, config_.textureSize);
            result.textureWidth = config_.textureSize;
            result.textureHeight = config_.textureSize;
        }

        result.confidence = view.confidence;
        result.success = true;
        printf("[Pipeline] Processing complete: conf=%.2f\n", result.confidence);
        return result;
    }

    // Process multiple photos with multi-view fusion
    PhotoFacePipelineResult processMultiView(
        const std::vector<std::pair<const uint8_t*, std::tuple<int,int,int>>>& images,
        const std::vector<MultiViewFusion::ViewResult::ViewType>& viewTypes) {

        PhotoFacePipelineResult result;
        std::vector<MultiViewFusion::ViewResult> allViews;

        for (size_t i = 0; i < images.size(); i++) {
            auto [data, dims] = images[i];
            auto [w, h, ch] = dims;
            auto viewType = (i < viewTypes.size()) ? viewTypes[i]
                                                     : MultiViewFusion::ViewResult::Front;

            // Process each photo individually
            FaceDetection det;
            if (!detector_.detectSingle(data, w, h, ch, det)) continue;

            FaceLandmarks landmarks;
            landmarkDetector_.detect(data, w, h, ch, det, landmarks);

            Face3DMMResult coeffs;
            regressor_.regress(data, w, h, ch, det, coeffs);

            if (landmarks.numPoints >= 68) {
                Face3DMMResult lmCoeffs;
                regressor_.regressFromLandmarks(landmarks, det, lmCoeffs);
                blendCoefficients(coeffs, lmCoeffs, coeffs);
            }

            MultiViewFusion::ViewResult view;
            view.coefficients = coeffs;
            view.landmarks = landmarks;
            view.confidence = std::max(det.confidence, landmarks.confidence);
            view.viewType = viewType;
            allViews.push_back(view);

            // Use front view for detection, texture, skin color
            if (viewType == MultiViewFusion::ViewResult::Front) {
                result.detection = det;
                result.landmarks = landmarks;
                result.coefficients = coeffs;

                result.skinColor = FaceTextureExtractor::extractSkinColor(data, w, h, ch, landmarks);

                if (config_.extractTexture) {
                    FaceTextureExtractor::extractTexture(
                        data, w, h, ch, landmarks,
                        result.textureData, config_.textureSize);
                    result.textureWidth = config_.textureSize;
                    result.textureHeight = config_.textureSize;
                }
            }
        }

        if (allViews.empty()) {
            result.errorMessage = "No faces detected in any view";
            return result;
        }

        // Fuse all views
        MultiViewFusion::fuse(allViews, result.faceParams);
        result.confidence = allViews[0].confidence;
        result.success = true;

        printf("[Pipeline] Multi-view processing: %zu views, conf=%.2f\n",
               allViews.size(), result.confidence);
        return result;
    }

private:
    void blendCoefficients(const Face3DMMResult& a, const Face3DMMResult& b,
                            Face3DMMResult& out) {
        // Weighted blend: ONNX model gets higher weight if available
        float wA = (a.confidence > 0.5f) ? 0.6f : 0.3f;
        float wB = 1.0f - wA;

        size_t maxSize = std::max(a.identityCoeffs.size(), b.identityCoeffs.size());
        out.identityCoeffs.resize(maxSize, 0.0f);

        for (size_t i = 0; i < maxSize; i++) {
            float va = (i < a.identityCoeffs.size()) ? a.identityCoeffs[i] : 0.0f;
            float vb = (i < b.identityCoeffs.size()) ? b.identityCoeffs[i] : 0.0f;
            out.identityCoeffs[i] = va * wA + vb * wB;
        }

        out.confidence = std::max(a.confidence, b.confidence);
    }

    Config config_;
    bool initialized_ = false;
    FaceDetector detector_;
    FaceLandmarkDetector landmarkDetector_;
    Face3DMMRegressor regressor_;
};

} // namespace luma
