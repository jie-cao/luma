// Landmark Mapper - Load and use landmark-to-vertex mappings for face fitting
// Part of LUMA Photo-to-Avatar System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/serialization/json.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace luma {

// ============================================================================
// Landmark Definition
// ============================================================================

struct LandmarkMapping {
    int id = -1;                           // 0-67 for iBUG 68
    std::string name;                      // e.g., "nose_tip"
    std::string region;                    // e.g., "nose", "eye", "mouth"
    std::vector<int> vertexIndices;        // Vertex indices in the mesh
    std::vector<float> weights;            // Weights for each vertex (sum to 1)
    bool missing = false;                  // True if not marked
    
    // Get weighted average position from mesh vertices
    Vec3 getPosition(const std::vector<Vec3>& vertices) const {
        if (vertexIndices.empty() || missing) {
            return Vec3(0, 0, 0);
        }
        
        Vec3 pos(0, 0, 0);
        for (size_t i = 0; i < vertexIndices.size(); i++) {
            int vi = vertexIndices[i];
            float w = (i < weights.size()) ? weights[i] : (1.0f / vertexIndices.size());
            if (vi >= 0 && vi < (int)vertices.size()) {
                pos.x += vertices[vi].x * w;
                pos.y += vertices[vi].y * w;
                pos.z += vertices[vi].z * w;
            }
        }
        return pos;
    }
};

// ============================================================================
// Landmark Map Loader
// ============================================================================

class LandmarkMapper {
public:
    std::string meshFile;
    std::string standard = "ibug68";
    int landmarkCount = 68;
    int markedCount = 0;
    std::vector<LandmarkMapping> landmarks;
    
    // Index by region for quick access
    std::vector<int> jawIndices;      // 0-16
    std::vector<int> leftBrowIndices; // 17-21
    std::vector<int> rightBrowIndices;// 22-26
    std::vector<int> noseIndices;     // 27-35
    std::vector<int> leftEyeIndices;  // 36-41
    std::vector<int> rightEyeIndices; // 42-47
    std::vector<int> mouthIndices;    // 48-67
    
    bool loaded = false;
    
    // Load from JSON file
    bool load(const std::string& jsonPath) {
        printf("[LandmarkMapper] Loading from: %s\n", jsonPath.c_str());
        
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            printf("[LandmarkMapper] Failed to open file\n");
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();
        
        // Parse JSON
        JsonValue root;
        if (!JsonParser::parse(jsonStr, root)) {
            printf("[LandmarkMapper] Failed to parse JSON\n");
            return false;
        }
        
        // Read metadata
        if (root.hasKey("meshFile")) {
            meshFile = root["meshFile"].asString();
        }
        if (root.hasKey("landmarkStandard")) {
            standard = root["landmarkStandard"].asString();
        }
        if (root.hasKey("landmarkCount")) {
            landmarkCount = root["landmarkCount"].asInt();
        }
        if (root.hasKey("markedCount")) {
            markedCount = root["markedCount"].asInt();
        }
        
        // Read landmarks array
        if (!root.hasKey("landmarks") || !root["landmarks"].isArray()) {
            printf("[LandmarkMapper] Missing 'landmarks' array\n");
            return false;
        }
        
        const auto& landmarksArray = root["landmarks"].asArray();
        landmarks.resize(landmarkCount);
        
        for (size_t i = 0; i < landmarksArray.size(); i++) {
            const auto& lm = landmarksArray[i];
            
            int id = lm.hasKey("id") ? lm["id"].asInt() : (int)i;
            if (id < 0 || id >= landmarkCount) continue;
            
            LandmarkMapping& mapping = landmarks[id];
            mapping.id = id;
            mapping.name = lm.hasKey("name") ? lm["name"].asString() : "";
            mapping.region = lm.hasKey("region") ? lm["region"].asString() : "";
            mapping.missing = lm.hasKey("missing") ? lm["missing"].asBool() : false;
            
            // Read vertex indices
            if (lm.hasKey("vertices") && lm["vertices"].isArray()) {
                const auto& verts = lm["vertices"].asArray();
                for (const auto& v : verts) {
                    mapping.vertexIndices.push_back(v.asInt());
                }
            }
            
            // Read weights
            if (lm.hasKey("weights") && lm["weights"].isArray()) {
                const auto& ws = lm["weights"].asArray();
                for (const auto& w : ws) {
                    mapping.weights.push_back(w.asFloat());
                }
            }
            
            // Default weights if not provided
            if (mapping.weights.empty() && !mapping.vertexIndices.empty()) {
                float w = 1.0f / mapping.vertexIndices.size();
                mapping.weights.resize(mapping.vertexIndices.size(), w);
            }
            
            // Mark as missing if no vertices
            if (mapping.vertexIndices.empty()) {
                mapping.missing = true;
            }
        }
        
        // Build region indices
        buildRegionIndices();
        
        loaded = true;
        printf("[LandmarkMapper] Loaded %d landmarks (%d marked)\n", 
               landmarkCount, markedCount);
        
        return true;
    }
    
    // Get 3D landmark positions from mesh vertices
    std::vector<Vec3> getLandmarkPositions(const std::vector<Vec3>& meshVertices) const {
        std::vector<Vec3> positions(landmarkCount);
        
        for (int i = 0; i < landmarkCount; i++) {
            if (i < (int)landmarks.size()) {
                positions[i] = landmarks[i].getPosition(meshVertices);
            }
        }
        
        return positions;
    }
    
    // Get landmarks for a specific region
    std::vector<Vec3> getRegionLandmarks(const std::string& region, 
                                          const std::vector<Vec3>& meshVertices) const {
        std::vector<Vec3> positions;
        
        for (const auto& lm : landmarks) {
            if (lm.region == region && !lm.missing) {
                positions.push_back(lm.getPosition(meshVertices));
            }
        }
        
        return positions;
    }
    
    // Calculate face measurements from landmarks
    struct FaceMeasurements {
        float faceWidth = 0;      // Distance between jaw points 0 and 16
        float faceHeight = 0;     // Distance from chin to forehead
        float jawWidth = 0;       // Distance between jaw points 4 and 12
        float eyeDistance = 0;    // Distance between eye centers
        float noseLength = 0;     // Distance from nose bridge top to tip
        float noseWidth = 0;      // Distance between nose wings
        float mouthWidth = 0;     // Distance between mouth corners
        float eyeWidth = 0;       // Average eye width
        float browHeight = 0;     // Average brow height above eyes
    };
    
    FaceMeasurements calculateMeasurements(const std::vector<Vec3>& meshVertices) const {
        FaceMeasurements m;
        auto pos = getLandmarkPositions(meshVertices);
        
        if (pos.size() < 68) return m;
        
        // Face width (jaw 0 to 16)
        m.faceWidth = (pos[0] - pos[16]).length();
        
        // Face height (chin 8 to nose bridge top 27)
        m.faceHeight = (pos[8] - pos[27]).length();
        
        // Jaw width (jaw 4 to 12)
        m.jawWidth = (pos[4] - pos[12]).length();
        
        // Eye distance (left eye center to right eye center)
        Vec3 leftEyeCenter = (pos[36] + pos[39]) * 0.5f;
        Vec3 rightEyeCenter = (pos[42] + pos[45]) * 0.5f;
        m.eyeDistance = (leftEyeCenter - rightEyeCenter).length();
        
        // Nose length (27 to 33)
        m.noseLength = (pos[27] - pos[33]).length();
        
        // Nose width (31 to 35)
        m.noseWidth = (pos[31] - pos[35]).length();
        
        // Mouth width (48 to 54)
        m.mouthWidth = (pos[48] - pos[54]).length();
        
        // Eye width (average of both eyes)
        float leftEyeWidth = (pos[36] - pos[39]).length();
        float rightEyeWidth = (pos[42] - pos[45]).length();
        m.eyeWidth = (leftEyeWidth + rightEyeWidth) * 0.5f;
        
        // Brow height (average distance from eye top to brow)
        Vec3 leftBrowCenter = pos[19];
        Vec3 rightBrowCenter = pos[24];
        Vec3 leftEyeTop = (pos[37] + pos[38]) * 0.5f;
        Vec3 rightEyeTop = (pos[43] + pos[44]) * 0.5f;
        m.browHeight = ((leftBrowCenter - leftEyeTop).length() + 
                        (rightBrowCenter - rightEyeTop).length()) * 0.5f;
        
        return m;
    }
    
private:
    void buildRegionIndices() {
        jawIndices.clear();
        leftBrowIndices.clear();
        rightBrowIndices.clear();
        noseIndices.clear();
        leftEyeIndices.clear();
        rightEyeIndices.clear();
        mouthIndices.clear();
        
        for (int i = 0; i <= 16; i++) jawIndices.push_back(i);
        for (int i = 17; i <= 21; i++) leftBrowIndices.push_back(i);
        for (int i = 22; i <= 26; i++) rightBrowIndices.push_back(i);
        for (int i = 27; i <= 35; i++) noseIndices.push_back(i);
        for (int i = 36; i <= 41; i++) leftEyeIndices.push_back(i);
        for (int i = 42; i <= 47; i++) rightEyeIndices.push_back(i);
        for (int i = 48; i <= 67; i++) mouthIndices.push_back(i);
    }
};

// ============================================================================
// 2D Landmark to 3D Parameter Fitter
// ============================================================================

class LandmarkFitter {
public:
    const LandmarkMapper* mapper = nullptr;
    
    // Reference measurements from the base mesh (neutral pose)
    LandmarkMapper::FaceMeasurements referenceMeasurements;
    
    // Initialize with landmark mapper and base mesh
    void initialize(const LandmarkMapper* lmMapper, 
                    const std::vector<Vec3>& baseMeshVertices) {
        mapper = lmMapper;
        if (mapper && mapper->loaded) {
            referenceMeasurements = mapper->calculateMeasurements(baseMeshVertices);
        }
    }
    
    // Estimate face parameters from 2D landmarks
    // This is a simplified version - production would use optimization
    struct FitResult {
        float faceWidth = 0.5f;
        float faceLength = 0.5f;
        float jawWidth = 0.5f;
        float eyeSpacing = 0.5f;
        float eyeSize = 0.5f;
        float noseLength = 0.5f;
        float noseWidth = 0.5f;
        float mouthWidth = 0.5f;
        float browHeight = 0.5f;
        float confidence = 0.0f;
    };
    
    FitResult fitFrom2DLandmarks(const std::vector<Vec2>& landmarks2D,
                                  float imageWidth, float imageHeight) const {
        FitResult result;
        
        if (landmarks2D.size() < 68) {
            return result;
        }
        
        // Calculate 2D measurements (normalized by image size)
        float imgDiag = std::sqrt(imageWidth * imageWidth + imageHeight * imageHeight);
        
        // Face width from jaw
        float faceWidth2D = (landmarks2D[0] - landmarks2D[16]).length() / imgDiag;
        
        // Face height from chin to brow
        float faceHeight2D = (landmarks2D[8] - landmarks2D[27]).length() / imgDiag;
        
        // Jaw width
        float jawWidth2D = (landmarks2D[4] - landmarks2D[12]).length() / imgDiag;
        
        // Eye distance
        Vec2 leftEyeCenter = (landmarks2D[36] + landmarks2D[39]) * 0.5f;
        Vec2 rightEyeCenter = (landmarks2D[42] + landmarks2D[45]) * 0.5f;
        float eyeDist2D = (leftEyeCenter - rightEyeCenter).length() / imgDiag;
        
        // Eye size (average width)
        float leftEyeW = (landmarks2D[36] - landmarks2D[39]).length() / imgDiag;
        float rightEyeW = (landmarks2D[42] - landmarks2D[45]).length() / imgDiag;
        float eyeSize2D = (leftEyeW + rightEyeW) * 0.5f;
        
        // Nose measurements
        float noseLen2D = (landmarks2D[27] - landmarks2D[33]).length() / imgDiag;
        float noseWidth2D = (landmarks2D[31] - landmarks2D[35]).length() / imgDiag;
        
        // Mouth width
        float mouthWidth2D = (landmarks2D[48] - landmarks2D[54]).length() / imgDiag;
        
        // Brow height
        Vec2 leftBrowC = landmarks2D[19];
        Vec2 leftEyeTop = (landmarks2D[37] + landmarks2D[38]) * 0.5f;
        float browH2D = (leftBrowC - leftEyeTop).length() / imgDiag;
        
        // Map to parameters (0-1 range)
        // These are heuristic mappings - would be learned in production
        
        // Face width: typical range 0.15-0.25 of image diagonal
        result.faceWidth = mapToParam(faceWidth2D, 0.15f, 0.25f);
        
        // Face length: typical range 0.20-0.35
        result.faceLength = mapToParam(faceHeight2D, 0.20f, 0.35f);
        
        // Jaw width relative to face width
        float jawRatio = jawWidth2D / (faceWidth2D + 0.001f);
        result.jawWidth = mapToParam(jawRatio, 0.6f, 0.9f);
        
        // Eye spacing relative to face width
        float eyeSpacingRatio = eyeDist2D / (faceWidth2D + 0.001f);
        result.eyeSpacing = mapToParam(eyeSpacingRatio, 0.3f, 0.5f);
        
        // Eye size relative to face width
        float eyeSizeRatio = eyeSize2D / (faceWidth2D + 0.001f);
        result.eyeSize = mapToParam(eyeSizeRatio, 0.08f, 0.15f);
        
        // Nose length relative to face height
        float noseLenRatio = noseLen2D / (faceHeight2D + 0.001f);
        result.noseLength = mapToParam(noseLenRatio, 0.25f, 0.40f);
        
        // Nose width relative to face width
        float noseWidthRatio = noseWidth2D / (faceWidth2D + 0.001f);
        result.noseWidth = mapToParam(noseWidthRatio, 0.15f, 0.30f);
        
        // Mouth width relative to face width
        float mouthRatio = mouthWidth2D / (faceWidth2D + 0.001f);
        result.mouthWidth = mapToParam(mouthRatio, 0.25f, 0.45f);
        
        // Brow height relative to eye size
        float browRatio = browH2D / (eyeSize2D + 0.001f);
        result.browHeight = mapToParam(browRatio, 0.3f, 0.8f);
        
        // Confidence based on face size in image
        result.confidence = std::min(1.0f, faceWidth2D / 0.15f);
        
        return result;
    }
    
private:
    // Map a value from [minVal, maxVal] to [0, 1]
    static float mapToParam(float value, float minVal, float maxVal) {
        float normalized = (value - minVal) / (maxVal - minVal + 0.0001f);
        return std::max(0.0f, std::min(1.0f, normalized));
    }
};

} // namespace luma
