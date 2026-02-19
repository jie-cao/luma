// Mesh Landmark Editor
// 在 3D 视图中直接标记拓扑头上的 landmark 顶点
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

namespace luma {

// iBUG 68 landmark 信息
struct LandmarkInfo {
    int id;
    const char* name;
    const char* region;
    const char* description;  // 中文描述
};

// 68 个 landmark 的定义
inline const LandmarkInfo LANDMARK_DEFS[68] = {
    // 下巴轮廓 (0-16) - 从右耳沿下巴到左耳
    {0, "jaw_right_ear", "jaw", "右耳旁"},
    {1, "jaw_right_1", "jaw", "右脸颊上"},
    {2, "jaw_right_2", "jaw", "右脸颊中"},
    {3, "jaw_right_3", "jaw", "右脸颊下"},
    {4, "jaw_right_4", "jaw", "右下颌角"},
    {5, "jaw_right_chin", "jaw", "右下颌"},
    {6, "jaw_chin_right", "chin", "下巴右侧"},
    {7, "jaw_chin_center_r", "chin", "下巴偏右"},
    {8, "jaw_chin_bottom", "chin", "下巴尖"},
    {9, "jaw_chin_center_l", "chin", "下巴偏左"},
    {10, "jaw_left_chin", "jaw", "左下颌"},
    {11, "jaw_left_4", "jaw", "左下颌角"},
    {12, "jaw_left_3", "jaw", "左脸颊下"},
    {13, "jaw_left_2", "jaw", "左脸颊中"},
    {14, "jaw_left_1", "jaw", "左脸颊上"},
    {15, "jaw_left_ear_low", "jaw", "左耳下"},
    {16, "jaw_left_ear", "jaw", "左耳旁"},
    
    // 左眉毛 (17-21) - 从外到内
    {17, "left_brow_outer", "left_eyebrow", "左眉外侧"},
    {18, "left_brow_2", "left_eyebrow", "左眉外2"},
    {19, "left_brow_center", "left_eyebrow", "左眉中心"},
    {20, "left_brow_1", "left_eyebrow", "左眉内2"},
    {21, "left_brow_inner", "left_eyebrow", "左眉内侧"},
    
    // 右眉毛 (22-26) - 从内到外
    {22, "right_brow_inner", "right_eyebrow", "右眉内侧"},
    {23, "right_brow_1", "right_eyebrow", "右眉内2"},
    {24, "right_brow_center", "right_eyebrow", "右眉中心"},
    {25, "right_brow_2", "right_eyebrow", "右眉外2"},
    {26, "right_brow_outer", "right_eyebrow", "右眉外侧"},
    
    // 鼻梁 (27-30)
    {27, "nose_bridge_top", "nose", "鼻梁顶部(眉心)"},
    {28, "nose_bridge_1", "nose", "鼻梁上部"},
    {29, "nose_bridge_2", "nose", "鼻梁中部"},
    {30, "nose_bridge_bottom", "nose", "鼻梁底部"},
    
    // 鼻子底部 (31-35)
    {31, "nose_left_wing", "nose", "左鼻翼"},
    {32, "nose_left_nostril", "nose", "左鼻孔"},
    {33, "nose_tip", "nose", "鼻尖"},
    {34, "nose_right_nostril", "nose", "右鼻孔"},
    {35, "nose_right_wing", "nose", "右鼻翼"},
    
    // 左眼 (36-41)
    {36, "left_eye_outer", "left_eye", "左眼外角"},
    {37, "left_eye_top_outer", "left_eye", "左眼上外"},
    {38, "left_eye_top_inner", "left_eye", "左眼上内"},
    {39, "left_eye_inner", "left_eye", "左眼内角"},
    {40, "left_eye_bottom_inner", "left_eye", "左眼下内"},
    {41, "left_eye_bottom_outer", "left_eye", "左眼下外"},
    
    // 右眼 (42-47)
    {42, "right_eye_inner", "right_eye", "右眼内角"},
    {43, "right_eye_top_inner", "right_eye", "右眼上内"},
    {44, "right_eye_top_outer", "right_eye", "右眼上外"},
    {45, "right_eye_outer", "right_eye", "右眼外角"},
    {46, "right_eye_bottom_outer", "right_eye", "右眼下外"},
    {47, "right_eye_bottom_inner", "right_eye", "右眼下内"},
    
    // 外嘴唇 (48-59)
    {48, "mouth_left", "mouth", "左嘴角"},
    {49, "mouth_top_left_1", "mouth", "上唇左1"},
    {50, "mouth_top_left_2", "mouth", "上唇左2"},
    {51, "mouth_top_center", "mouth", "上唇中心"},
    {52, "mouth_top_right_2", "mouth", "上唇右2"},
    {53, "mouth_top_right_1", "mouth", "上唇右1"},
    {54, "mouth_right", "mouth", "右嘴角"},
    {55, "mouth_bottom_right_1", "mouth", "下唇右1"},
    {56, "mouth_bottom_right_2", "mouth", "下唇右2"},
    {57, "mouth_bottom_center", "mouth", "下唇中心"},
    {58, "mouth_bottom_left_2", "mouth", "下唇左2"},
    {59, "mouth_bottom_left_1", "mouth", "下唇左1"},
    
    // 内嘴唇 (60-67)
    {60, "inner_mouth_left", "mouth_inner", "内唇左"},
    {61, "inner_mouth_top_left", "mouth_inner", "内上唇左"},
    {62, "inner_mouth_top_center", "mouth_inner", "内上唇中"},
    {63, "inner_mouth_top_right", "mouth_inner", "内上唇右"},
    {64, "inner_mouth_right", "mouth_inner", "内唇右"},
    {65, "inner_mouth_bottom_right", "mouth_inner", "内下唇右"},
    {66, "inner_mouth_bottom_center", "mouth_inner", "内下唇中"},
    {67, "inner_mouth_bottom_left", "mouth_inner", "内下唇左"},
};

// 区域颜色
inline Vec3 getRegionColor(const char* region) {
    if (strcmp(region, "jaw") == 0) return Vec3(1.0f, 0.5f, 0.2f);
    if (strcmp(region, "chin") == 0) return Vec3(1.0f, 0.6f, 0.3f);
    if (strcmp(region, "left_eyebrow") == 0) return Vec3(0.2f, 0.6f, 1.0f);
    if (strcmp(region, "right_eyebrow") == 0) return Vec3(0.3f, 0.7f, 1.0f);
    if (strcmp(region, "nose") == 0) return Vec3(0.2f, 1.0f, 0.4f);
    if (strcmp(region, "left_eye") == 0) return Vec3(1.0f, 0.2f, 1.0f);
    if (strcmp(region, "right_eye") == 0) return Vec3(0.9f, 0.3f, 0.9f);
    if (strcmp(region, "mouth") == 0) return Vec3(1.0f, 0.2f, 0.2f);
    if (strcmp(region, "mouth_inner") == 0) return Vec3(0.9f, 0.4f, 0.4f);
    return Vec3(0.5f, 0.5f, 0.5f);
}

// Landmark 编辑器状态
class MeshLandmarkEditor {
public:
    struct LandmarkMapping {
        int vertexIndex = -1;
        bool isSet = false;
    };
    
    MeshLandmarkEditor() {
        mappings_.resize(68);
    }
    
    // 设置要编辑的网格
    void setMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        vertices_ = vertices;
        indices_ = indices;
        
        // 计算包围盒
        if (!vertices.empty()) {
            meshMin_ = Vec3(1e30f, 1e30f, 1e30f);
            meshMax_ = Vec3(-1e30f, -1e30f, -1e30f);
            for (const auto& v : vertices) {
                meshMin_.x = std::min(meshMin_.x, v.position[0]);
                meshMin_.y = std::min(meshMin_.y, v.position[1]);
                meshMin_.z = std::min(meshMin_.z, v.position[2]);
                meshMax_.x = std::max(meshMax_.x, v.position[0]);
                meshMax_.y = std::max(meshMax_.y, v.position[1]);
                meshMax_.z = std::max(meshMax_.z, v.position[2]);
            }
        }
    }
    
    // 加载现有映射
    bool loadMapping(const std::string& jsonPath) {
        std::ifstream file(jsonPath);
        if (!file.is_open()) return false;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        
        // 简单解析 JSON
        for (int i = 0; i < 68; i++) {
            char pattern[64];
            snprintf(pattern, sizeof(pattern), "\"id\": %d", i);
            
            size_t pos = content.find(pattern);
            if (pos == std::string::npos) continue;
            
            size_t verticesPos = content.find("\"vertices\":", pos);
            if (verticesPos == std::string::npos || verticesPos > pos + 300) continue;
            
            size_t bracketStart = content.find("[", verticesPos);
            size_t bracketEnd = content.find("]", bracketStart);
            if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                std::string numStr = content.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                // 去除空格
                numStr.erase(std::remove_if(numStr.begin(), numStr.end(), ::isspace), numStr.end());
                if (!numStr.empty()) {
                    size_t commaPos = numStr.find(',');
                    if (commaPos != std::string::npos) {
                        numStr = numStr.substr(0, commaPos);
                    }
                    try {
                        int vertIdx = std::stoi(numStr);
                        mappings_[i].vertexIndex = vertIdx;
                        mappings_[i].isSet = true;
                    } catch (...) {}
                }
            }
        }
        
        int count = 0;
        for (const auto& m : mappings_) if (m.isSet) count++;
        printf("[LandmarkEditor] Loaded %d mappings from %s\n", count, jsonPath.c_str());
        return true;
    }
    
    // 保存映射
    bool saveMapping(const std::string& jsonPath) {
        std::ofstream file(jsonPath);
        if (!file.is_open()) return false;
        
        file << "{\n";
        file << "  \"version\": \"1.0\",\n";
        file << "  \"meshFile\": \"head_base.bin\",\n";
        file << "  \"landmarkStandard\": \"ibug68\",\n";
        file << "  \"landmarkCount\": 68,\n";
        
        int markedCount = 0;
        for (const auto& m : mappings_) if (m.isSet) markedCount++;
        file << "  \"markedCount\": " << markedCount << ",\n";
        file << "  \"auto_estimated\": false,\n";
        file << "  \"note\": \"由 LUMA Studio 手动标记\",\n";
        file << "  \"landmarks\": [\n";
        
        for (int i = 0; i < 68; i++) {
            const auto& def = LANDMARK_DEFS[i];
            const auto& m = mappings_[i];
            
            file << "    {\n";
            file << "      \"id\": " << i << ",\n";
            file << "      \"name\": \"" << def.name << "\",\n";
            file << "      \"region\": \"" << def.region << "\",\n";
            
            if (m.isSet && m.vertexIndex >= 0) {
                file << "      \"vertices\": [" << m.vertexIndex << "],\n";
                file << "      \"weights\": [1.0],\n";
            } else {
                file << "      \"vertices\": [],\n";
                file << "      \"weights\": [],\n";
            }
            
            file << "      \"auto_estimated\": false\n";
            file << "    }" << (i < 67 ? "," : "") << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        printf("[LandmarkEditor] Saved %d mappings to %s\n", markedCount, jsonPath.c_str());
        return true;
    }
    
    // 自动识别 landmark 位置（基于几何分析）
    void autoDetectLandmarks() {
        if (vertices_.empty()) {
            printf("[LandmarkEditor] ERROR: No mesh loaded\n");
            return;
        }
        
        printf("[LandmarkEditor] Auto-detecting landmarks on mesh with %zu vertices...\n", vertices_.size());
        
        // 计算网格统计信息
        Vec3 center(0, 0, 0);
        for (const auto& v : vertices_) {
            center.x += v.position[0];
            center.y += v.position[1];
            center.z += v.position[2];
        }
        center = center * (1.0f / vertices_.size());
        
        float meshHeight = meshMax_.y - meshMin_.y;
        float meshWidth = meshMax_.x - meshMin_.x;
        float meshDepth = meshMax_.z - meshMin_.z;
        
        printf("[LandmarkEditor] Mesh bounds: (%.3f, %.3f, %.3f) to (%.3f, %.3f, %.3f)\n",
               meshMin_.x, meshMin_.y, meshMin_.z, meshMax_.x, meshMax_.y, meshMax_.z);
        printf("[LandmarkEditor] Mesh size: W=%.3f H=%.3f D=%.3f, Center=(%.3f, %.3f, %.3f)\n",
               meshWidth, meshHeight, meshDepth, center.x, center.y, center.z);
        
        // 辅助函数：找到最接近目标位置的顶点
        auto findClosestVertex = [this](float targetX, float targetY, float targetZ, 
                                        float xWeight = 1.0f, float yWeight = 1.0f, float zWeight = 1.0f) -> int {
            int bestIdx = -1;
            float bestDist = 1e30f;
            for (size_t i = 0; i < vertices_.size(); i++) {
                float dx = (vertices_[i].position[0] - targetX) * xWeight;
                float dy = (vertices_[i].position[1] - targetY) * yWeight;
                float dz = (vertices_[i].position[2] - targetZ) * zWeight;
                float dist = dx*dx + dy*dy + dz*dz;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = (int)i;
                }
            }
            return bestIdx;
        };
        
        // 辅助函数：找到在指定区域内 Z 值最大（最前面）的顶点
        auto findFrontmostInRegion = [this](float xMin, float xMax, float yMin, float yMax) -> int {
            int bestIdx = -1;
            float bestZ = -1e30f;
            for (size_t i = 0; i < vertices_.size(); i++) {
                float x = vertices_[i].position[0];
                float y = vertices_[i].position[1];
                float z = vertices_[i].position[2];
                if (x >= xMin && x <= xMax && y >= yMin && y <= yMax && z > bestZ) {
                    bestZ = z;
                    bestIdx = (int)i;
                }
            }
            return bestIdx;
        };
        
        // 辅助函数：找到在指定区域内 Y 值最小（最低）的顶点
        auto findLowestInRegion = [this](float xMin, float xMax, float zMin, float zMax) -> int {
            int bestIdx = -1;
            float bestY = 1e30f;
            for (size_t i = 0; i < vertices_.size(); i++) {
                float x = vertices_[i].position[0];
                float y = vertices_[i].position[1];
                float z = vertices_[i].position[2];
                if (x >= xMin && x <= xMax && z >= zMin && z <= zMax && y < bestY) {
                    bestY = y;
                    bestIdx = (int)i;
                }
            }
            return bestIdx;
        };
        
        // 估算关键高度（假设 Y 轴向上）
        float chinY = meshMin_.y + meshHeight * 0.0f;      // 下巴底部
        float mouthY = meshMin_.y + meshHeight * 0.25f;    // 嘴巴
        float noseY = meshMin_.y + meshHeight * 0.45f;     // 鼻子
        float eyeY = meshMin_.y + meshHeight * 0.55f;      // 眼睛
        float browY = meshMin_.y + meshHeight * 0.62f;     // 眉毛
        float foreheadY = meshMin_.y + meshHeight * 0.75f; // 额头
        
        // 估算关键宽度
        float faceHalfWidth = meshWidth * 0.35f;
        float eyeOffsetX = meshWidth * 0.15f;  // 眼睛距中心的距离
        float mouthHalfWidth = meshWidth * 0.12f;
        float noseHalfWidth = meshWidth * 0.06f;
        
        // ========== 下巴轮廓 (0-16) ==========
        // 从右耳到左耳的轮廓
        for (int i = 0; i <= 16; i++) {
            float t = i / 16.0f;  // 0 到 1
            float angle = (t - 0.5f) * 3.14159f;  // -π/2 到 π/2
            
            float targetX = center.x + sinf(angle) * faceHalfWidth;
            float targetY;
            if (i >= 6 && i <= 10) {
                // 下巴底部区域
                targetY = chinY;
            } else {
                // 侧面区域，Y 值逐渐升高
                float distFromCenter = fabsf(t - 0.5f) * 2.0f;  // 0 到 1
                targetY = chinY + distFromCenter * meshHeight * 0.3f;
            }
            float targetZ = meshMax_.z * 0.3f;  // 前面一点
            
            int idx = findClosestVertex(targetX, targetY, targetZ, 1.0f, 2.0f, 0.5f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        // ========== 眉毛 (17-26) ==========
        // 左眉毛 (17-21) - 从外侧到内侧（17=太阳穴端，21=鼻子端）
        // iBUG 的"左"是图像中的左，即观察者的左边
        for (int i = 17; i <= 21; i++) {
            float t = (i - 17) / 4.0f;  // 0 到 1
            // t=0 时在外侧（远离中心），t=1 时在内侧（靠近中心）
            float targetX = center.x + eyeOffsetX * 1.3f - t * eyeOffsetX * 0.8f;
            float targetY = browY;
            float targetZ = meshMax_.z * 0.5f;
            
            int idx = findClosestVertex(targetX, targetY, targetZ, 1.0f, 1.0f, 0.3f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        // 右眉毛 (22-26) - 从内侧到外侧（22=鼻子端，26=太阳穴端）
        for (int i = 22; i <= 26; i++) {
            float t = (i - 22) / 4.0f;  // 0 到 1
            // t=0 时在内侧（靠近中心），t=1 时在外侧（远离中心）
            float targetX = center.x - eyeOffsetX * 0.5f - t * eyeOffsetX * 0.8f;
            float targetY = browY;
            float targetZ = meshMax_.z * 0.5f;
            
            int idx = findClosestVertex(targetX, targetY, targetZ, 1.0f, 1.0f, 0.3f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        // ========== 鼻梁 (27-30) ==========
        for (int i = 27; i <= 30; i++) {
            float t = (i - 27) / 3.0f;  // 0 到 1
            float targetY = browY - t * (browY - noseY);
            
            int idx = findFrontmostInRegion(center.x - noseHalfWidth, center.x + noseHalfWidth,
                                            targetY - meshHeight * 0.03f, targetY + meshHeight * 0.03f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        // ========== 鼻子底部 (31-35) ==========
        float noseBottomY = noseY - meshHeight * 0.08f;
        // 31: 左鼻翼
        mappings_[31].vertexIndex = findClosestVertex(center.x + noseHalfWidth * 1.5f, noseBottomY, meshMax_.z * 0.4f);
        mappings_[31].isSet = true;
        // 32: 左鼻孔
        mappings_[32].vertexIndex = findClosestVertex(center.x + noseHalfWidth * 0.7f, noseBottomY, meshMax_.z * 0.5f);
        mappings_[32].isSet = true;
        // 33: 鼻尖
        mappings_[33].vertexIndex = findFrontmostInRegion(center.x - noseHalfWidth, center.x + noseHalfWidth,
                                                          noseBottomY - meshHeight * 0.05f, noseY);
        mappings_[33].isSet = true;
        // 34: 右鼻孔
        mappings_[34].vertexIndex = findClosestVertex(center.x - noseHalfWidth * 0.7f, noseBottomY, meshMax_.z * 0.5f);
        mappings_[34].isSet = true;
        // 35: 右鼻翼
        mappings_[35].vertexIndex = findClosestVertex(center.x - noseHalfWidth * 1.5f, noseBottomY, meshMax_.z * 0.4f);
        mappings_[35].isSet = true;
        
        // ========== 左眼 (36-41) ==========
        float eyeHalfWidth = meshWidth * 0.05f;
        float leftEyeCenterX = center.x + eyeOffsetX;
        // 36: 左眼外角
        mappings_[36].vertexIndex = findClosestVertex(leftEyeCenterX + eyeHalfWidth, eyeY, meshMax_.z * 0.4f);
        mappings_[36].isSet = true;
        // 37: 左眼上外
        mappings_[37].vertexIndex = findClosestVertex(leftEyeCenterX + eyeHalfWidth * 0.5f, eyeY + meshHeight * 0.02f, meshMax_.z * 0.45f);
        mappings_[37].isSet = true;
        // 38: 左眼上内
        mappings_[38].vertexIndex = findClosestVertex(leftEyeCenterX - eyeHalfWidth * 0.5f, eyeY + meshHeight * 0.02f, meshMax_.z * 0.45f);
        mappings_[38].isSet = true;
        // 39: 左眼内角
        mappings_[39].vertexIndex = findClosestVertex(leftEyeCenterX - eyeHalfWidth, eyeY, meshMax_.z * 0.4f);
        mappings_[39].isSet = true;
        // 40: 左眼下内
        mappings_[40].vertexIndex = findClosestVertex(leftEyeCenterX - eyeHalfWidth * 0.5f, eyeY - meshHeight * 0.015f, meshMax_.z * 0.45f);
        mappings_[40].isSet = true;
        // 41: 左眼下外
        mappings_[41].vertexIndex = findClosestVertex(leftEyeCenterX + eyeHalfWidth * 0.5f, eyeY - meshHeight * 0.015f, meshMax_.z * 0.45f);
        mappings_[41].isSet = true;
        
        // ========== 右眼 (42-47) ==========
        float rightEyeCenterX = center.x - eyeOffsetX;
        // 42: 右眼内角
        mappings_[42].vertexIndex = findClosestVertex(rightEyeCenterX + eyeHalfWidth, eyeY, meshMax_.z * 0.4f);
        mappings_[42].isSet = true;
        // 43: 右眼上内
        mappings_[43].vertexIndex = findClosestVertex(rightEyeCenterX + eyeHalfWidth * 0.5f, eyeY + meshHeight * 0.02f, meshMax_.z * 0.45f);
        mappings_[43].isSet = true;
        // 44: 右眼上外
        mappings_[44].vertexIndex = findClosestVertex(rightEyeCenterX - eyeHalfWidth * 0.5f, eyeY + meshHeight * 0.02f, meshMax_.z * 0.45f);
        mappings_[44].isSet = true;
        // 45: 右眼外角
        mappings_[45].vertexIndex = findClosestVertex(rightEyeCenterX - eyeHalfWidth, eyeY, meshMax_.z * 0.4f);
        mappings_[45].isSet = true;
        // 46: 右眼下外
        mappings_[46].vertexIndex = findClosestVertex(rightEyeCenterX - eyeHalfWidth * 0.5f, eyeY - meshHeight * 0.015f, meshMax_.z * 0.45f);
        mappings_[46].isSet = true;
        // 47: 右眼下内
        mappings_[47].vertexIndex = findClosestVertex(rightEyeCenterX + eyeHalfWidth * 0.5f, eyeY - meshHeight * 0.015f, meshMax_.z * 0.45f);
        mappings_[47].isSet = true;
        
        // ========== 外嘴唇 (48-59) ==========
        float lipY = mouthY;
        // 48: 左嘴角
        mappings_[48].vertexIndex = findClosestVertex(center.x + mouthHalfWidth, lipY, meshMax_.z * 0.4f);
        mappings_[48].isSet = true;
        // 49-53: 上唇
        for (int i = 49; i <= 53; i++) {
            float t = (i - 49) / 4.0f;
            float targetX = center.x + mouthHalfWidth * (1.0f - t * 2.0f);
            float targetY = lipY + meshHeight * 0.015f;
            int idx = findClosestVertex(targetX, targetY, meshMax_.z * 0.45f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        // 54: 右嘴角
        mappings_[54].vertexIndex = findClosestVertex(center.x - mouthHalfWidth, lipY, meshMax_.z * 0.4f);
        mappings_[54].isSet = true;
        // 55-59: 下唇
        for (int i = 55; i <= 59; i++) {
            float t = (i - 55) / 4.0f;
            float targetX = center.x - mouthHalfWidth * (1.0f - t * 2.0f);
            float targetY = lipY - meshHeight * 0.02f;
            int idx = findClosestVertex(targetX, targetY, meshMax_.z * 0.45f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        // ========== 内嘴唇 (60-67) ==========
        float innerLipScale = 0.6f;
        // 60: 内唇左
        mappings_[60].vertexIndex = findClosestVertex(center.x + mouthHalfWidth * innerLipScale, lipY, meshMax_.z * 0.42f);
        mappings_[60].isSet = true;
        // 61-63: 内上唇
        for (int i = 61; i <= 63; i++) {
            float t = (i - 61) / 2.0f;
            float targetX = center.x + mouthHalfWidth * innerLipScale * (1.0f - t * 2.0f);
            float targetY = lipY + meshHeight * 0.008f;
            int idx = findClosestVertex(targetX, targetY, meshMax_.z * 0.43f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        // 64: 内唇右
        mappings_[64].vertexIndex = findClosestVertex(center.x - mouthHalfWidth * innerLipScale, lipY, meshMax_.z * 0.42f);
        mappings_[64].isSet = true;
        // 65-67: 内下唇
        for (int i = 65; i <= 67; i++) {
            float t = (i - 65) / 2.0f;
            float targetX = center.x - mouthHalfWidth * innerLipScale * (1.0f - t * 2.0f);
            float targetY = lipY - meshHeight * 0.01f;
            int idx = findClosestVertex(targetX, targetY, meshMax_.z * 0.43f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        int count = 0;
        for (const auto& m : mappings_) if (m.isSet) count++;
        printf("[LandmarkEditor] Auto-detected %d/68 landmarks\n", count);
    }
    
    // 设置当前 landmark 的顶点
    void setLandmarkVertex(int landmarkId, int vertexIndex) {
        if (landmarkId >= 0 && landmarkId < 68) {
            mappings_[landmarkId].vertexIndex = vertexIndex;
            mappings_[landmarkId].isSet = true;
        }
    }
    
    // 清除 landmark
    void clearLandmark(int landmarkId) {
        if (landmarkId >= 0 && landmarkId < 68) {
            mappings_[landmarkId].vertexIndex = -1;
            mappings_[landmarkId].isSet = false;
        }
    }
    
    // 获取 landmark 顶点位置
    bool getLandmarkPosition(int landmarkId, Vec3& outPos) const {
        if (landmarkId < 0 || landmarkId >= 68) return false;
        const auto& m = mappings_[landmarkId];
        if (!m.isSet || m.vertexIndex < 0 || m.vertexIndex >= (int)vertices_.size()) return false;
        
        outPos.x = vertices_[m.vertexIndex].position[0];
        outPos.y = vertices_[m.vertexIndex].position[1];
        outPos.z = vertices_[m.vertexIndex].position[2];
        return true;
    }
    
    // 射线拾取顶点
    int pickVertex(const Vec3& rayOrigin, const Vec3& rayDir, float maxDist = 1000.0f) const {
        int bestIdx = -1;
        float bestDist = maxDist;
        
        for (size_t i = 0; i < vertices_.size(); i++) {
            Vec3 vPos(vertices_[i].position[0], vertices_[i].position[1], vertices_[i].position[2]);
            
            // 计算点到射线的距离
            Vec3 toVert = vPos - rayOrigin;
            float t = toVert.dot(rayDir);
            if (t < 0) continue;  // 在射线后面
            
            Vec3 closestOnRay = rayOrigin + rayDir * t;
            float dist = (vPos - closestOnRay).length();
            
            // 使用屏幕空间距离阈值
            float threshold = 0.005f;  // 约 5mm
            if (dist < threshold && dist < bestDist) {
                bestDist = dist;
                bestIdx = (int)i;
            }
        }
        
        return bestIdx;
    }
    
    // 获取所有 landmark 位置用于渲染
    std::vector<std::pair<Vec3, Vec3>> getLandmarkMarkers() const {
        std::vector<std::pair<Vec3, Vec3>> markers;  // position, color
        
        for (int i = 0; i < 68; i++) {
            Vec3 pos;
            if (getLandmarkPosition(i, pos)) {
                Vec3 color = getRegionColor(LANDMARK_DEFS[i].region);
                markers.push_back({pos, color});
            }
        }
        
        return markers;
    }
    
    // UI 状态
    bool isActive() const { return active_; }
    void setActive(bool active) { active_ = active; }
    int getCurrentLandmark() const { return currentLandmark_; }
    void setCurrentLandmark(int id) { currentLandmark_ = id; }
    void nextLandmark() { if (currentLandmark_ < 67) currentLandmark_++; }
    void prevLandmark() { if (currentLandmark_ > 0) currentLandmark_--; }
    
    const std::vector<LandmarkMapping>& getMappings() const { return mappings_; }
    const std::vector<Vertex>& getVertices() const { return vertices_; }
    const std::vector<uint32_t>& getIndices() const { return indices_; }
    
    int getMarkedCount() const {
        int count = 0;
        for (const auto& m : mappings_) if (m.isSet) count++;
        return count;
    }
    
    // ========== 显示选项 ==========
    struct DisplayOptions {
        bool showMesh = false;          // 显示 PBR 网格（默认关闭，用点云）
        bool meshTransparent = true;    // 网格半透明
        float meshOpacity = 0.5f;       // 网格不透明度
        bool showVertexDots = true;     // 显示所有顶点点（默认开启）
        bool showLandmarks = true;      // 显示 landmark 标记
        float landmarkSize = 0.004f;    // landmark 标记大小
        bool highlightRegion = true;    // 高亮当前区域
    };
    
    DisplayOptions& getDisplayOptions() { return displayOptions_; }
    const DisplayOptions& getDisplayOptions() const { return displayOptions_; }
    
    // ========== Hover 功能 ==========
    void updateHover(const Vec3& rayOrigin, const Vec3& rayDir, const Vec3& cameraPos) {
        hoveredVertex_ = -1;
        hoveredLandmark_ = -1;
        float bestDist = 1e30f;
        
        // 背面剔除辅助函数
        auto isFacingCamera = [&](const Vertex& v) -> bool {
            Vec3 toCamera(
                cameraPos.x - v.position[0],
                cameraPos.y - v.position[1],
                cameraPos.z - v.position[2]
            );
            float dot = v.normal[0] * toCamera.x + v.normal[1] * toCamera.y + v.normal[2] * toCamera.z;
            return dot > 0;
        };
        
        // 优先检测普通顶点（用于设置 landmark）
        // 这样即使鼠标靠近已标记的 landmark，也能选择附近的顶点
        for (size_t i = 0; i < vertices_.size(); i++) {
            // 背面剔除
            if (!isFacingCamera(vertices_[i])) continue;
            
            Vec3 pos(vertices_[i].position[0], vertices_[i].position[1], vertices_[i].position[2]);
            Vec3 toPos = pos - rayOrigin;
            float t = toPos.dot(rayDir);
            if (t < 0) continue;
            
            Vec3 closestOnRay = rayOrigin + rayDir * t;
            float dist = (pos - closestOnRay).length();
            
            float threshold = 0.003f;
            if (dist < threshold && dist < bestDist) {
                bestDist = dist;
                hoveredVertex_ = (int)i;
            }
        }
        
        // 如果 hover 到了顶点，检查这个顶点是否是某个已标记的 landmark
        if (hoveredVertex_ >= 0) {
            for (int i = 0; i < 68; i++) {
                if (mappings_[i].isSet && mappings_[i].vertexIndex == hoveredVertex_) {
                    hoveredLandmark_ = i;
                    break;
                }
            }
        }
        // 如果没有 hover 到顶点，再检查是否 hover 在已标记的 landmark 附近（用于切换当前 landmark）
        else {
            bestDist = 1e30f;
            for (int i = 0; i < 68; i++) {
                if (!mappings_[i].isSet) continue;
                int vIdx = mappings_[i].vertexIndex;
                if (vIdx < 0 || vIdx >= (int)vertices_.size()) continue;
                
                // Landmark 也做背面剔除
                if (!isFacingCamera(vertices_[vIdx])) continue;
                
                Vec3 pos(vertices_[vIdx].position[0], vertices_[vIdx].position[1], vertices_[vIdx].position[2]);
                Vec3 toPos = pos - rayOrigin;
                float t = toPos.dot(rayDir);
                if (t < 0) continue;
                
                Vec3 closestOnRay = rayOrigin + rayDir * t;
                float dist = (pos - closestOnRay).length();
                
                // Landmark 有更大的拾取范围（用于切换）
                float threshold = displayOptions_.landmarkSize * 3.0f;
                if (dist < threshold && dist < bestDist) {
                    bestDist = dist;
                    hoveredLandmark_ = i;
                    hoveredVertex_ = vIdx;
                }
            }
        }
    }
    
    // 兼容旧接口
    void updateHover(const Vec3& rayOrigin, const Vec3& rayDir) {
        updateHover(rayOrigin, rayDir, rayOrigin);
    }
    
    int getHoveredVertex() const { return hoveredVertex_; }
    int getHoveredLandmark() const { return hoveredLandmark_; }
    
    // 获取 hover 的顶点位置
    bool getHoveredPosition(Vec3& outPos) const {
        if (hoveredVertex_ < 0 || hoveredVertex_ >= (int)vertices_.size()) return false;
        outPos.x = vertices_[hoveredVertex_].position[0];
        outPos.y = vertices_[hoveredVertex_].position[1];
        outPos.z = vertices_[hoveredVertex_].position[2];
        return true;
    }
    
    // ========== 快捷操作 ==========
    // 跳转到下一个未标记的 landmark
    void nextUnmarked() {
        for (int i = currentLandmark_ + 1; i < 68; i++) {
            if (!mappings_[i].isSet) {
                currentLandmark_ = i;
                return;
            }
        }
        // 从头开始找
        for (int i = 0; i < currentLandmark_; i++) {
            if (!mappings_[i].isSet) {
                currentLandmark_ = i;
                return;
            }
        }
    }
    
    // 跳转到指定区域的第一个 landmark
    void jumpToRegion(const char* region) {
        for (int i = 0; i < 68; i++) {
            if (strcmp(LANDMARK_DEFS[i].region, region) == 0) {
                currentLandmark_ = i;
                return;
            }
        }
    }
    
    // 获取当前 landmark 所在区域
    const char* getCurrentRegion() const {
        return LANDMARK_DEFS[currentLandmark_].region;
    }
    
    // 清除所有标记
    void clearAll() {
        for (auto& m : mappings_) {
            m.vertexIndex = -1;
            m.isSet = false;
        }
    }
    
    // ========== 镜像功能 ==========
    // 左右对称的 landmark 对应关系
    // 注意：对应的是语义上相同位置的点（如左眉内侧↔右眉内侧）
    static constexpr int MIRROR_PAIRS[][2] = {
        // 下巴轮廓 (0-16 是对称的，8 是中心)
        {0, 16}, {1, 15}, {2, 14}, {3, 13}, {4, 12}, {5, 11}, {6, 10}, {7, 9},
        // 眉毛：左眉外侧(17)↔右眉外侧(26), 左眉内侧(21)↔右眉内侧(22)
        {17, 26}, {18, 25}, {19, 24}, {20, 23}, {21, 22},
        // 鼻子 (31-35)：左鼻翼(31)↔右鼻翼(35), 左鼻孔(32)↔右鼻孔(34)
        {31, 35}, {32, 34},
        // 眼睛：左眼外角(36)↔右眼外角(45), 左眼内角(39)↔右眼内角(42)
        // 左眼上外(37)↔右眼上外(44), 左眼上内(38)↔右眼上内(43)
        // 左眼下内(40)↔右眼下内(47), 左眼下外(41)↔右眼下外(46)
        {36, 45}, {37, 44}, {38, 43}, {39, 42}, {40, 47}, {41, 46},
        // 外嘴唇：左嘴角(48)↔右嘴角(54)
        // 上唇左1(49)↔上唇右1(53), 上唇左2(50)↔上唇右2(52)
        // 下唇左1(59)↔下唇右1(55), 下唇左2(58)↔下唇右2(56)
        {48, 54}, {49, 53}, {50, 52}, {59, 55}, {58, 56},
        // 内嘴唇：内唇左(60)↔内唇右(64)
        // 内上唇左(61)↔内上唇右(63), 内下唇左(67)↔内下唇右(65)
        {60, 64}, {61, 63}, {67, 65}
    };
    
    // 从左边镜像到右边（X > 0 的点镜像到 X < 0）
    void mirrorLeftToRight() {
        if (vertices_.empty()) return;
        
        // 计算中心 X
        float centerX = (meshMin_.x + meshMax_.x) * 0.5f;
        
        for (const auto& pair : MIRROR_PAIRS) {
            int leftIdx = pair[0];
            int rightIdx = pair[1];
            
            // 检查左边是否已标记
            if (!mappings_[leftIdx].isSet) continue;
            
            int srcVertIdx = mappings_[leftIdx].vertexIndex;
            if (srcVertIdx < 0 || srcVertIdx >= (int)vertices_.size()) continue;
            
            // 获取源顶点位置
            float srcX = vertices_[srcVertIdx].position[0];
            float srcY = vertices_[srcVertIdx].position[1];
            float srcZ = vertices_[srcVertIdx].position[2];
            
            // 计算镜像位置
            float mirrorX = 2.0f * centerX - srcX;
            
            // 找到最接近镜像位置的顶点
            int bestIdx = -1;
            float bestDist = 1e30f;
            for (size_t i = 0; i < vertices_.size(); i++) {
                float dx = vertices_[i].position[0] - mirrorX;
                float dy = vertices_[i].position[1] - srcY;
                float dz = vertices_[i].position[2] - srcZ;
                float dist = dx*dx + dy*dy + dz*dz;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = (int)i;
                }
            }
            
            if (bestIdx >= 0) {
                mappings_[rightIdx].vertexIndex = bestIdx;
                mappings_[rightIdx].isSet = true;
            }
        }
        
        printf("[LandmarkEditor] Mirrored left to right\n");
    }
    
    // 从右边镜像到左边（X < 0 的点镜像到 X > 0）
    void mirrorRightToLeft() {
        if (vertices_.empty()) return;
        
        float centerX = (meshMin_.x + meshMax_.x) * 0.5f;
        
        for (const auto& pair : MIRROR_PAIRS) {
            int leftIdx = pair[0];
            int rightIdx = pair[1];
            
            // 检查右边是否已标记
            if (!mappings_[rightIdx].isSet) continue;
            
            int srcVertIdx = mappings_[rightIdx].vertexIndex;
            if (srcVertIdx < 0 || srcVertIdx >= (int)vertices_.size()) continue;
            
            float srcX = vertices_[srcVertIdx].position[0];
            float srcY = vertices_[srcVertIdx].position[1];
            float srcZ = vertices_[srcVertIdx].position[2];
            
            float mirrorX = 2.0f * centerX - srcX;
            
            int bestIdx = -1;
            float bestDist = 1e30f;
            for (size_t i = 0; i < vertices_.size(); i++) {
                float dx = vertices_[i].position[0] - mirrorX;
                float dy = vertices_[i].position[1] - srcY;
                float dz = vertices_[i].position[2] - srcZ;
                float dist = dx*dx + dy*dy + dz*dz;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = (int)i;
                }
            }
            
            if (bestIdx >= 0) {
                mappings_[leftIdx].vertexIndex = bestIdx;
                mappings_[leftIdx].isSet = true;
            }
        }
        
        printf("[LandmarkEditor] Mirrored right to left\n");
    }
    
    // 交换左右（用于修正左右相反的问题）
    void swapLeftRight() {
        for (const auto& pair : MIRROR_PAIRS) {
            std::swap(mappings_[pair[0]], mappings_[pair[1]]);
        }
        printf("[LandmarkEditor] Swapped left and right landmarks\n");
    }
    
    // 获取区域内已标记数量
    int getRegionMarkedCount(const char* region) const {
        int count = 0;
        for (int i = 0; i < 68; i++) {
            if (strcmp(LANDMARK_DEFS[i].region, region) == 0 && mappings_[i].isSet) {
                count++;
            }
        }
        return count;
    }
    
    int getRegionTotalCount(const char* region) const {
        int count = 0;
        for (int i = 0; i < 68; i++) {
            if (strcmp(LANDMARK_DEFS[i].region, region) == 0) count++;
        }
        return count;
    }
    
    // 获取包围盒
    Vec3 getMeshMin() const { return meshMin_; }
    Vec3 getMeshMax() const { return meshMax_; }
    Vec3 getMeshCenter() const { return (meshMin_ + meshMax_) * 0.5f; }

private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<LandmarkMapping> mappings_;
    
    Vec3 meshMin_, meshMax_;
    
    bool active_ = false;
    int currentLandmark_ = 0;
    
    // 显示选项
    DisplayOptions displayOptions_;
    
    // Hover 状态
    int hoveredVertex_ = -1;
    int hoveredLandmark_ = -1;
};

// 渲染 Landmark 编辑器 UI
inline void renderLandmarkEditorUI(MeshLandmarkEditor& editor, const std::string& savePath) {
    if (!editor.isActive()) return;
    
    ImGui::SetNextWindowSize(ImVec2(380, 600), ImGuiCond_FirstUseEver);
    bool open = editor.isActive();
    if (!ImGui::Begin("Landmark 标记器", &open)) {
        ImGui::End();
        editor.setActive(open);
        return;
    }
    editor.setActive(open);
    
    auto& opts = editor.getDisplayOptions();
    
    // ========== 显示选项 ==========
    if (ImGui::CollapsingHeader("显示选项", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 显示模式选择
        ImGui::Text("显示模式:");
        ImGui::SameLine();
        if (ImGui::RadioButton("点云", !opts.showMesh)) {
            opts.showMesh = false;
            opts.showVertexDots = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("网格", opts.showMesh)) {
            opts.showMesh = true;
            opts.showVertexDots = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("两者", opts.showMesh && opts.showVertexDots)) {
            opts.showMesh = true;
            opts.showVertexDots = true;
        }
        
        if (opts.showMesh) {
            ImGui::Checkbox("网格半透明", &opts.meshTransparent);
            if (opts.meshTransparent) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::SliderFloat("##opacity", &opts.meshOpacity, 0.1f, 1.0f, "%.1f");
            }
        }
        
        ImGui::Spacing();
        ImGui::Checkbox("显示 Landmark", &opts.showLandmarks);
        if (opts.showLandmarks) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::SliderFloat("大小##lmsize", &opts.landmarkSize, 0.002f, 0.01f, "%.3f");
        }
        
        ImGui::Checkbox("高亮当前区域", &opts.highlightRegion);
    }
    
    ImGui::Separator();
    
    // ========== Hover 信息 ==========
    int hoveredVert = editor.getHoveredVertex();
    int hoveredLm = editor.getHoveredLandmark();
    if (hoveredLm >= 0) {
        Vec3 color = getRegionColor(LANDMARK_DEFS[hoveredLm].region);
        ImGui::TextColored(ImVec4(color.x, color.y, color.z, 1.0f), 
                          "悬停: Landmark #%d %s", hoveredLm, LANDMARK_DEFS[hoveredLm].description);
    } else if (hoveredVert >= 0) {
        Vec3 pos;
        if (editor.getHoveredPosition(pos)) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                              "悬停: 顶点 %d (%.3f, %.3f, %.3f)", hoveredVert, pos.x, pos.y, pos.z);
        }
    } else {
        ImGui::TextDisabled("将鼠标移到顶点上查看信息");
    }
    
    ImGui::Separator();
    
    // ========== 进度总览 ==========
    int marked = editor.getMarkedCount();
    
    // 区域快捷按钮
    ImGui::Text("快速跳转区域:");
    const char* regions[] = {"jaw", "chin", "left_eyebrow", "right_eyebrow", 
                              "nose", "left_eye", "right_eye", "mouth", "mouth_inner"};
    const char* regionShort[] = {"下颌", "下巴", "左眉", "右眉", "鼻", "左眼", "右眼", "外唇", "内唇"};
    
    for (int r = 0; r < 9; r++) {
        if (r > 0 && r != 4 && r != 7) ImGui::SameLine(0, 2);
        if (r == 4 || r == 7) {} // 换行
        
        int regionMarked = editor.getRegionMarkedCount(regions[r]);
        int regionTotal = editor.getRegionTotalCount(regions[r]);
        bool isCurrentRegion = (strcmp(editor.getCurrentRegion(), regions[r]) == 0);
        
        Vec3 color = getRegionColor(regions[r]);
        if (isCurrentRegion) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, 1.0f));
        } else if (regionMarked == regionTotal) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.4f, 0.1f, 1.0f));  // 完成 - 绿色
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x * 0.3f, color.y * 0.3f, color.z * 0.3f, 0.8f));
        }
        
        char btnLabel[32];
        snprintf(btnLabel, sizeof(btnLabel), "%s\n%d/%d", regionShort[r], regionMarked, regionTotal);
        if (ImGui::Button(btnLabel, ImVec2(38, 36))) {
            editor.jumpToRegion(regions[r]);
        }
        ImGui::PopStyleColor();
    }
    
    ImGui::Spacing();
    ImGui::Text("总进度: %d / 68", marked);
    ImGui::ProgressBar(marked / 68.0f);
    
    ImGui::Separator();
    
    // ========== 当前 landmark ==========
    int current = editor.getCurrentLandmark();
    const auto& def = LANDMARK_DEFS[current];
    const auto& mapping = editor.getMappings()[current];
    
    Vec3 regionColor = getRegionColor(def.region);
    ImGui::TextColored(ImVec4(regionColor.x, regionColor.y, regionColor.z, 1.0f), 
                      "当前: #%d %s", current, def.description);
    
    if (mapping.isSet) {
        Vec3 pos;
        if (editor.getLandmarkPosition(current, pos)) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "已标记: 顶点 %d", mapping.vertexIndex);
            ImGui::TextDisabled("位置: (%.4f, %.4f, %.4f)", pos.x, pos.y, pos.z);
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "未标记");
    }
    
    // 导航按钮
    ImGui::Spacing();
    if (ImGui::Button("<<", ImVec2(30, 0)) && current > 0) {
        editor.prevLandmark();
    }
    ImGui::SameLine();
    if (ImGui::Button("上一个", ImVec2(60, 0)) && current > 0) {
        editor.prevLandmark();
    }
    ImGui::SameLine();
    if (ImGui::Button("下一个", ImVec2(60, 0)) && current < 67) {
        editor.nextLandmark();
    }
    ImGui::SameLine();
    if (ImGui::Button(">>", ImVec2(30, 0)) && current < 67) {
        editor.nextLandmark();
    }
    ImGui::SameLine();
    if (ImGui::Button("下一未标记", ImVec2(-1, 0))) {
        editor.nextUnmarked();
    }
    
    // 快速跳转
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("##jumpTo", &current, 0, 0)) {
        if (current >= 0 && current < 68) {
            editor.setCurrentLandmark(current);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("清除", ImVec2(50, 0))) {
        editor.clearLandmark(editor.getCurrentLandmark());
    }
    ImGui::SameLine();
    if (ImGui::Button("清除全部", ImVec2(-1, 0))) {
        editor.clearAll();
    }
    
    ImGui::Separator();
    
    // ========== 区域详情 ==========
    if (ImGui::CollapsingHeader("区域详情")) {
        const char* regionNames[] = {"下颌轮廓", "下巴", "左眉毛", "右眉毛",
                                      "鼻子", "左眼", "右眼", "嘴唇外", "嘴唇内"};
        
        for (int r = 0; r < 9; r++) {
            Vec3 color = getRegionColor(regions[r]);
            int regionMarked = editor.getRegionMarkedCount(regions[r]);
            int regionTotal = editor.getRegionTotalCount(regions[r]);
            
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.x * 0.4f, color.y * 0.4f, color.z * 0.4f, 0.6f));
            
            char headerLabel[64];
            snprintf(headerLabel, sizeof(headerLabel), "%s (%d/%d)###region%d", 
                    regionNames[r], regionMarked, regionTotal, r);
            
            if (ImGui::TreeNode(headerLabel)) {
                for (int i = 0; i < 68; i++) {
                    if (strcmp(LANDMARK_DEFS[i].region, regions[r]) == 0) {
                        const auto& m = editor.getMappings()[i];
                        bool isCurrent = (i == editor.getCurrentLandmark());
                        bool isHovered = (i == hoveredLm);
                        
                        if (isCurrent) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
                        } else if (isHovered) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1, 1, 1));
                        } else if (m.isSet) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1, 0.5f, 1));
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.5f, 0.3f, 1));
                        }
                        
                        char label[128];
                        if (m.isSet) {
                            snprintf(label, sizeof(label), "%s[%d] %s (v%d)", 
                                    isCurrent ? "> " : "  ", i, LANDMARK_DEFS[i].description, m.vertexIndex);
                        } else {
                            snprintf(label, sizeof(label), "%s[%d] %s", 
                                    isCurrent ? "> " : "  ", i, LANDMARK_DEFS[i].description);
                        }
                        
                        if (ImGui::Selectable(label, isCurrent)) {
                            editor.setCurrentLandmark(i);
                        }
                        
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopStyleColor();
        }
    }
    
    ImGui::Separator();
    
    // ========== 镜像操作 ==========
    ImGui::Text("镜像操作:");
    if (ImGui::Button("左→右", ImVec2(80, 24))) {
        editor.mirrorLeftToRight();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("将左边的标记镜像到右边\n（用于只标注了左半边的情况）");
    }
    ImGui::SameLine();
    if (ImGui::Button("右→左", ImVec2(80, 24))) {
        editor.mirrorRightToLeft();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("将右边的标记镜像到左边\n（用于只标注了右半边的情况）");
    }
    ImGui::SameLine();
    if (ImGui::Button("交换左右", ImVec2(-1, 24))) {
        editor.swapLeftRight();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("交换左右对称的 landmark\n（用于修正左右标反的情况）");
    }
    
    ImGui::Spacing();
    
    // ========== 操作按钮 ==========
    if (ImGui::Button("重新自动识别", ImVec2(-1, 28))) {
        editor.autoDetectLandmarks();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("基于几何分析重新自动识别所有 landmark 位置\n（会覆盖当前标记）");
    }
    
    ImGui::Spacing();
    
    if (ImGui::Button("保存", ImVec2(100, 28))) {
        editor.saveMapping(savePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("加载", ImVec2(100, 28))) {
        editor.loadMapping(savePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭", ImVec2(-1, 28))) {
        editor.setActive(false);
    }
    
    ImGui::Separator();
    
    // ========== 快捷键提示 ==========
    ImGui::TextDisabled("快捷键:");
    ImGui::TextDisabled("  A/← 上一个  D/→ 下一个");
    ImGui::TextDisabled("  点击顶点标记  Delete/X 清除");
    ImGui::TextDisabled("  ESC 关闭编辑器");
    
    ImGui::End();
}

} // namespace luma
