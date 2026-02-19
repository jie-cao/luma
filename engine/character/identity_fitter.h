// Identity Fitter - FLAME/DECA-style photo-to-face fitting
// Based on: "Learning a model of facial shape and expression from 4D scans" (SIGGRAPH Asia 2017)
// and "DECA: Detailed Expression Capture and Animation" (SIGGRAPH 2021)
//
// Core approach: Minimize landmark reprojection error with regularization
// Loss = Σ || project(V_base + Σ w_i * BS_i) - landmarks_2D ||² + λ * Σ w_i²
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character_face.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace luma {

class IdentityFitter {
public:
    struct FitResult {
        std::unordered_map<std::string, float> weights;
        float finalError = 0.0f;
        int iterations = 0;
        bool converged = false;
    };
    
    struct FitParams {
        int maxIterations = 300;
        float convergenceThreshold = 1e-7f;
        float learningRate = 0.1f;
        float regularization = 0.01f;
        bool verbose = true;
    };

    IdentityFitter() = default;
    
    void initialize(const std::vector<Vertex>& baseMesh,
                    const std::unordered_map<int, int>& landmarkVertexMap) {
        baseMesh_ = baseMesh;
        landmarkVertexMap_ = landmarkVertexMap;
        
        baseLandmarks3D_.clear();
        baseLandmarks3D_.resize(68, Vec3(0, 0, 0));
        for (const auto& [lmIdx, vtxIdx] : landmarkVertexMap_) {
            if (lmIdx >= 0 && lmIdx < 68 && vtxIdx >= 0 && vtxIdx < (int)baseMesh_.size()) {
                const auto& v = baseMesh_[vtxIdx];
                baseLandmarks3D_[lmIdx] = Vec3(v.position[0], v.position[1], v.position[2]);
            }
        }
        
        printf("[IdentityFitter] Initialized with %zu vertices, %zu landmark mappings\n",
               baseMesh_.size(), landmarkVertexMap_.size());
    }
    
    void setBlendShapes(const BlendShapeMesh& blendShapes,
                        const std::vector<std::string>& identityShapeNames) {
        printf("[IdentityFitter] setBlendShapes called with %zu names\n", identityShapeNames.size());
        
        blendShapeNames_.clear();
        blendShapeLandmarkDeltas_.clear();
        
        int foundCount = 0;
        for (const auto& name : identityShapeNames) {
            int idx = blendShapes.findTargetIndex(name);
            if (idx < 0) continue;
            
            const BlendShapeTarget* target = blendShapes.getTarget(idx);
            if (!target) continue;
            
            std::vector<Vec3> landmarkDeltas(68, Vec3(0, 0, 0));
            int landmarkHits = 0;
            
            for (const auto& delta : target->deltas) {
                for (const auto& [lmIdx, vtxIdx] : landmarkVertexMap_) {
                    if ((int)delta.vertexIndex == vtxIdx && lmIdx >= 0 && lmIdx < 68) {
                        landmarkDeltas[lmIdx] = delta.positionDelta;
                        landmarkHits++;
                        break;
                    }
                }
            }
            
            if (landmarkHits > 0) {
                blendShapeNames_.push_back(name);
                blendShapeLandmarkDeltas_[name] = landmarkDeltas;
                foundCount++;
                
                float maxMag = 0.0f;
                for (const auto& d : landmarkDeltas) {
                    float mag = std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
                    maxMag = std::max(maxMag, mag);
                }
                printf("[IdentityFitter]   %s: %d landmark hits, max delta=%.4f\n",
                       name.c_str(), landmarkHits, maxMag);
            }
        }
        printf("[IdentityFitter] Loaded %d identity BlendShapes with landmark data\n", foundCount);
    }
    
    // ========================================================================
    // FLAME/DECA-style fitting using numerical gradient descent
    // ========================================================================
    FitResult fit(const std::vector<Vec2>& landmarks2D, float aspectRatio,
                  const FitParams& params = FitParams()) {
        FitResult result;
        
        if (landmarks2D.size() < 68 || blendShapeLandmarkDeltas_.empty()) {
            printf("[IdentityFitter] Invalid input: %zu landmarks, %zu shapes\n",
                   landmarks2D.size(), blendShapeLandmarkDeltas_.size());
            return result;
        }
        
        const int numWeights = (int)blendShapeNames_.size();
        std::vector<float> weights(numWeights, 0.0f);
        
        // Landmark weights (following DECA's weighted_landmark_loss)
        std::vector<float> landmarkWeights = getLandmarkWeights();
        
        // ====================================================================
        // Step 1: Estimate initial camera/projection parameters (fixed during optimization)
        // ====================================================================
        float scale, tx, ty;
        estimateWeakPerspective(landmarks2D, weights, scale, tx, ty);
        
        if (params.verbose) {
            printf("[IdentityFitter] Initial projection: scale=%.4f, tx=%.4f, ty=%.4f\n",
                   scale, tx, ty);
            
            // Debug: show sample landmark comparisons
            std::vector<Vec3> baseLm3D = computeDeformedLandmarks3D(weights);
            printf("[IdentityFitter] Sample landmark comparison (2D target vs projected 3D):\n");
            int sampleLandmarks[] = {8, 30, 36, 45, 48, 54};  // chin, nose, eyes, mouth
            for (int lmIdx : sampleLandmarks) {
                if (landmarkVertexMap_.find(lmIdx) != landmarkVertexMap_.end()) {
                    Vec2 proj = projectLandmark(baseLm3D[lmIdx], scale, tx, ty);
                    printf("  LM %d: target=(%.3f,%.3f), proj=(%.3f,%.3f), diff=(%.3f,%.3f)\n",
                           lmIdx, landmarks2D[lmIdx].x, landmarks2D[lmIdx].y,
                           proj.x, proj.y,
                           landmarks2D[lmIdx].x - proj.x, landmarks2D[lmIdx].y - proj.y);
                }
            }
        }
        
        // ====================================================================
        // Step 2: Optimize BlendShape weights using numerical gradient descent
        // ====================================================================
        float prevError = computeLandmarkLoss(weights, landmarks2D, scale, tx, ty, landmarkWeights);
        float bestError = prevError;
        std::vector<float> bestWeights = weights;
        
        // Adaptive learning rate
        float lr = params.learningRate;
        int noImprovementCount = 0;
        
        for (int iter = 0; iter < params.maxIterations; iter++) {
            // Compute numerical gradient for each weight
            std::vector<float> gradient(numWeights, 0.0f);
            const float eps = 0.001f;
            
            for (int wi = 0; wi < numWeights; wi++) {
                // Forward difference
                weights[wi] += eps;
                float errorPlus = computeLandmarkLoss(weights, landmarks2D, scale, tx, ty, landmarkWeights);
                weights[wi] -= 2.0f * eps;
                float errorMinus = computeLandmarkLoss(weights, landmarks2D, scale, tx, ty, landmarkWeights);
                weights[wi] += eps;  // Restore
                
                gradient[wi] = (errorPlus - errorMinus) / (2.0f * eps);
                
                // Add L2 regularization gradient
                gradient[wi] += 2.0f * params.regularization * weights[wi];
            }
            
            // Update weights with gradient descent
            for (int wi = 0; wi < numWeights; wi++) {
                weights[wi] -= lr * gradient[wi];
                weights[wi] = std::clamp(weights[wi], -1.0f, 1.0f);
            }
            
            // Compute new error
            float newError = computeLandmarkLoss(weights, landmarks2D, scale, tx, ty, landmarkWeights);
            
            // Track best result
            if (newError < bestError) {
                bestError = newError;
                bestWeights = weights;
                noImprovementCount = 0;
            } else {
                noImprovementCount++;
                // Reduce learning rate if no improvement
                if (noImprovementCount > 10) {
                    lr *= 0.5f;
                    noImprovementCount = 0;
                    if (params.verbose && lr < params.learningRate * 0.1f) {
                        printf("[IdentityFitter] Iter %d: reducing lr to %.6f\n", iter, lr);
                    }
                }
            }
            
            if (params.verbose && iter % 50 == 0) {
                printf("[IdentityFitter] Iter %d: loss=%.6f (best=%.6f), lr=%.4f\n", 
                       iter, newError, bestError, lr);
            }
            
            // Check convergence
            if (std::abs(prevError - newError) < params.convergenceThreshold && iter > 20) {
                result.converged = true;
                result.iterations = iter + 1;
                break;
            }
            
            // Early stop if learning rate too small
            if (lr < 1e-6f) {
                result.converged = true;
                result.iterations = iter + 1;
                break;
            }
            
            prevError = newError;
            result.iterations = iter + 1;
        }
        
        // Use best weights found
        weights = bestWeights;
        result.finalError = bestError;
        
        // Store results
        for (int i = 0; i < numWeights; i++) {
            result.weights[blendShapeNames_[i]] = weights[i];
        }
        
        if (params.verbose) {
            printf("[IdentityFitter] Finished: %d iterations, loss=%.6f, converged=%d\n",
                   result.iterations, result.finalError, result.converged);
            printf("[IdentityFitter] Top weights:\n");
            std::vector<std::pair<std::string, float>> sortedWeights;
            for (const auto& [name, w] : result.weights) {
                sortedWeights.push_back({name, w});
            }
            std::sort(sortedWeights.begin(), sortedWeights.end(),
                      [](const auto& a, const auto& b) { return std::abs(a.second) > std::abs(b.second); });
            for (int i = 0; i < std::min(10, (int)sortedWeights.size()); i++) {
                printf("  %s: %.3f\n", sortedWeights[i].first.c_str(), sortedWeights[i].second);
            }
        }
        
        return result;
    }
    
    // Convert fit result to FaceShapeParams
    void applyToParams(const FitResult& result, FaceShapeParams& params) {
        auto getWeight = [&](const std::string& name) -> float {
            auto it = result.weights.find(name);
            if (it == result.weights.end()) return 0.5f;
            return std::clamp(0.5f + it->second * 0.5f, 0.0f, 1.0f);
        };
        
        params.faceWidth = getWeight("faceWidth");
        params.faceLength = getWeight("faceLength");
        params.faceRoundness = getWeight("faceRoundness");
        params.jawWidth = getWeight("jawWidth");
        params.noseWidth = getWeight("noseWidth");
        params.noseLength = getWeight("noseLength");
        params.mouthWidth = getWeight("mouthWidth");
        params.eyeSize = getWeight("eyeSize");
        params.eyeHeight = getWeight("eyeHeight");
        params.eyeSpacing = getWeight("eyeSpacing");
        params.browHeight = getWeight("browHeight");
        params.chinLength = getWeight("chinLength");
        params.foreheadHeight = getWeight("foreheadHeight");
        
        printf("[IdentityFitter] Applied to params: faceWidth=%.2f, jawWidth=%.2f, noseWidth=%.2f\n",
               params.faceWidth, params.jawWidth, params.noseWidth);
    }

private:
    // ========================================================================
    // Landmark weights following DECA's weighted_landmark_loss
    // ========================================================================
    std::vector<float> getLandmarkWeights() {
        std::vector<float> weights(68, 1.0f);
        
        // Jaw contour (0-16): lower weight - affected by pose and hair
        for (int i = 0; i <= 16; i++) {
            weights[i] = 0.5f;
        }
        weights[0] = 0.3f;
        weights[16] = 0.3f;
        
        // Eyebrows (17-26): medium weight
        for (int i = 17; i <= 26; i++) {
            weights[i] = 0.8f;
        }
        
        // Nose bridge (27-30): high weight - stable
        for (int i = 27; i <= 30; i++) {
            weights[i] = 1.2f;
        }
        
        // Nose bottom (31-35): high weight
        for (int i = 31; i <= 35; i++) {
            weights[i] = 1.0f;
        }
        
        // Eyes (36-47): highest weight - most reliable
        for (int i = 36; i <= 47; i++) {
            weights[i] = 1.5f;
        }
        
        // Mouth outer (48-59): medium weight
        for (int i = 48; i <= 59; i++) {
            weights[i] = 0.8f;
        }
        
        // Mouth inner (60-67): lower weight - expression dependent
        for (int i = 60; i <= 67; i++) {
            weights[i] = 0.5f;
        }
        
        return weights;
    }
    
    // ========================================================================
    // Weak perspective projection
    // ========================================================================
    Vec2 projectLandmark(const Vec3& p3d, float scale, float tx, float ty) {
        return Vec2(scale * p3d.x + tx, scale * (-p3d.y) + ty);
    }
    
    // ========================================================================
    // Estimate weak perspective parameters using Procrustes alignment
    // ========================================================================
    void estimateWeakPerspective(const std::vector<Vec2>& target2D,
                                  const std::vector<float>& weights,
                                  float& scale, float& tx, float& ty) {
        std::vector<Vec3> landmarks3D = computeDeformedLandmarks3D(weights);
        
        Vec2 centroid2D(0, 0);
        Vec2 centroid3D_proj(0, 0);
        int count = 0;
        
        for (int i = 0; i < 68; i++) {
            if (landmarkVertexMap_.find(i) == landmarkVertexMap_.end()) continue;
            centroid2D = centroid2D + target2D[i];
            centroid3D_proj = centroid3D_proj + Vec2(landmarks3D[i].x, -landmarks3D[i].y);
            count++;
        }
        
        if (count == 0) {
            scale = 1.0f; tx = 0.5f; ty = 0.5f;
            return;
        }
        
        centroid2D = centroid2D * (1.0f / count);
        centroid3D_proj = centroid3D_proj * (1.0f / count);
        
        float rms2D = 0, rms3D = 0;
        for (int i = 0; i < 68; i++) {
            if (landmarkVertexMap_.find(i) == landmarkVertexMap_.end()) continue;
            Vec2 d2D = target2D[i] - centroid2D;
            Vec2 d3D = Vec2(landmarks3D[i].x, -landmarks3D[i].y) - centroid3D_proj;
            rms2D += d2D.x * d2D.x + d2D.y * d2D.y;
            rms3D += d3D.x * d3D.x + d3D.y * d3D.y;
        }
        rms2D = std::sqrt(rms2D / count);
        rms3D = std::sqrt(rms3D / count);
        
        scale = (rms3D > 1e-6f) ? (rms2D / rms3D) : 1.0f;
        tx = centroid2D.x - scale * centroid3D_proj.x;
        ty = centroid2D.y - scale * centroid3D_proj.y;
    }
    
    // ========================================================================
    // Compute deformed 3D landmarks
    // ========================================================================
    std::vector<Vec3> computeDeformedLandmarks3D(const std::vector<float>& weights) {
        std::vector<Vec3> landmarks3D = baseLandmarks3D_;
        
        for (size_t wi = 0; wi < weights.size() && wi < blendShapeNames_.size(); wi++) {
            const auto& name = blendShapeNames_[wi];
            auto it = blendShapeLandmarkDeltas_.find(name);
            if (it == blendShapeLandmarkDeltas_.end()) continue;
            
            const auto& deltas = it->second;
            for (int lmIdx = 0; lmIdx < 68; lmIdx++) {
                landmarks3D[lmIdx] = landmarks3D[lmIdx] + deltas[lmIdx] * weights[wi];
            }
        }
        
        return landmarks3D;
    }
    
    // ========================================================================
    // Compute weighted landmark loss
    // ========================================================================
    float computeLandmarkLoss(const std::vector<float>& weights,
                              const std::vector<Vec2>& target2D,
                              float scale, float tx, float ty,
                              const std::vector<float>& landmarkWeights) {
        std::vector<Vec3> landmarks3D = computeDeformedLandmarks3D(weights);
        
        float totalLoss = 0.0f;
        float totalWeight = 0.0f;
        
        for (int lmIdx = 0; lmIdx < 68; lmIdx++) {
            if (landmarkVertexMap_.find(lmIdx) == landmarkVertexMap_.end()) continue;
            
            Vec2 proj = projectLandmark(landmarks3D[lmIdx], scale, tx, ty);
            float dx = proj.x - target2D[lmIdx].x;
            float dy = proj.y - target2D[lmIdx].y;
            
            float w = landmarkWeights[lmIdx];
            totalLoss += w * (dx * dx + dy * dy);
            totalWeight += w;
        }
        
        return totalWeight > 0 ? totalLoss / totalWeight : 0.0f;
    }
    
    std::vector<Vertex> baseMesh_;
    std::unordered_map<int, int> landmarkVertexMap_;
    std::vector<Vec3> baseLandmarks3D_;
    std::vector<std::string> blendShapeNames_;
    std::unordered_map<std::string, std::vector<Vec3>> blendShapeLandmarkDeltas_;
};

} // namespace luma
