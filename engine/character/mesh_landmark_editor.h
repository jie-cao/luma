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
    // 下巴轮廓 (0-16)
    {0, "jaw_right_ear", "jaw", "右耳下方"},
    {1, "jaw_right_1", "jaw", "右下颌1"},
    {2, "jaw_right_2", "jaw", "右下颌2"},
    {3, "jaw_right_3", "jaw", "右下颌3"},
    {4, "jaw_right_4", "jaw", "右下颌4"},
    {5, "jaw_right_chin", "jaw", "右下巴"},
    {6, "jaw_chin_right", "chin", "下巴右侧"},
    {7, "jaw_chin_center", "chin", "下巴中心偏右"},
    {8, "jaw_chin_bottom", "chin", "下巴最低点"},
    {9, "jaw_chin_left", "chin", "下巴中心偏左"},
    {10, "jaw_left_chin", "jaw", "左下巴"},
    {11, "jaw_left_4", "jaw", "左下颌4"},
    {12, "jaw_left_3", "jaw", "左下颌3"},
    {13, "jaw_left_2", "jaw", "左下颌2"},
    {14, "jaw_left_1", "jaw", "左下颌1"},
    {15, "jaw_left_ear_low", "jaw", "左耳下方低"},
    {16, "jaw_left_ear", "jaw", "左耳下方"},
    
    // 左眉毛 (17-21)
    {17, "left_brow_inner", "left_eyebrow", "左眉内侧"},
    {18, "left_brow_1", "left_eyebrow", "左眉1"},
    {19, "left_brow_center", "left_eyebrow", "左眉中心"},
    {20, "left_brow_2", "left_eyebrow", "左眉2"},
    {21, "left_brow_outer", "left_eyebrow", "左眉外侧"},
    
    // 右眉毛 (22-26)
    {22, "right_brow_inner", "right_eyebrow", "右眉内侧"},
    {23, "right_brow_1", "right_eyebrow", "右眉1"},
    {24, "right_brow_center", "right_eyebrow", "右眉中心"},
    {25, "right_brow_2", "right_eyebrow", "右眉2"},
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
        // 左眉毛 (17-21) - 注意：iBUG 的"左"是图像中的左，即人脸的右边 (X > 0)
        for (int i = 17; i <= 21; i++) {
            float t = (i - 17) / 4.0f;  // 0 到 1
            float targetX = center.x + eyeOffsetX * 0.5f + t * eyeOffsetX * 0.8f;
            float targetY = browY;
            float targetZ = meshMax_.z * 0.5f;
            
            int idx = findClosestVertex(targetX, targetY, targetZ, 1.0f, 1.0f, 0.3f);
            if (idx >= 0) {
                mappings_[i].vertexIndex = idx;
                mappings_[i].isSet = true;
            }
        }
        
        // 右眉毛 (22-26) - 人脸的左边 (X < 0)
        for (int i = 22; i <= 26; i++) {
            float t = (i - 22) / 4.0f;  // 0 到 1
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
    
    int getMarkedCount() const {
        int count = 0;
        for (const auto& m : mappings_) if (m.isSet) count++;
        return count;
    }

private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<LandmarkMapping> mappings_;
    
    Vec3 meshMin_, meshMax_;
    
    bool active_ = false;
    int currentLandmark_ = 0;
};

// 渲染 Landmark 编辑器 UI
inline void renderLandmarkEditorUI(MeshLandmarkEditor& editor, const std::string& savePath) {
    if (!editor.isActive()) return;
    
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    bool open = editor.isActive();
    if (!ImGui::Begin("Landmark 标记器", &open)) {
        ImGui::End();
        editor.setActive(open);
        return;
    }
    editor.setActive(open);
    
    // 进度
    int marked = editor.getMarkedCount();
    ImGui::Text("进度: %d / 68", marked);
    ImGui::ProgressBar(marked / 68.0f);
    ImGui::Separator();
    
    // 当前 landmark
    int current = editor.getCurrentLandmark();
    const auto& def = LANDMARK_DEFS[current];
    const auto& mapping = editor.getMappings()[current];
    
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "当前: #%d %s", current, def.description);
    ImGui::Text("区域: %s", def.region);
    
    if (mapping.isSet) {
        Vec3 pos;
        if (editor.getLandmarkPosition(current, pos)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "顶点: %d (%.3f, %.3f, %.3f)", 
                              mapping.vertexIndex, pos.x, pos.y, pos.z);
        }
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "未标记 - 点击网格选择顶点");
    }
    
    ImGui::Separator();
    
    // 导航
    if (ImGui::Button("<< 上一个") && current > 0) {
        editor.prevLandmark();
    }
    ImGui::SameLine();
    if (ImGui::Button("下一个 >>") && current < 67) {
        editor.nextLandmark();
    }
    
    // 快速跳转
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("跳转到", &current)) {
        if (current >= 0 && current < 68) {
            editor.setCurrentLandmark(current);
        }
    }
    
    // 清除当前
    if (ImGui::Button("清除当前标记")) {
        editor.clearLandmark(current);
    }
    
    ImGui::Separator();
    
    // 区域列表
    if (ImGui::CollapsingHeader("按区域浏览", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* regions[] = {"jaw", "chin", "left_eyebrow", "right_eyebrow", 
                                  "nose", "left_eye", "right_eye", "mouth", "mouth_inner"};
        const char* regionNames[] = {"下颌轮廓", "下巴", "左眉毛", "右眉毛",
                                      "鼻子", "左眼", "右眼", "嘴唇外", "嘴唇内"};
        
        for (int r = 0; r < 9; r++) {
            Vec3 color = getRegionColor(regions[r]);
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.x * 0.5f, color.y * 0.5f, color.z * 0.5f, 0.5f));
            
            if (ImGui::TreeNode(regionNames[r])) {
                for (int i = 0; i < 68; i++) {
                    if (strcmp(LANDMARK_DEFS[i].region, regions[r]) == 0) {
                        const auto& m = editor.getMappings()[i];
                        
                        char label[128];
                        if (m.isSet) {
                            snprintf(label, sizeof(label), "[%d] %s (v%d)", i, LANDMARK_DEFS[i].description, m.vertexIndex);
                        } else {
                            snprintf(label, sizeof(label), "[%d] %s (未标记)", i, LANDMARK_DEFS[i].description);
                        }
                        
                        bool selected = (i == editor.getCurrentLandmark());
                        if (ImGui::Selectable(label, selected)) {
                            editor.setCurrentLandmark(i);
                        }
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopStyleColor();
        }
    }
    
    ImGui::Separator();
    
    // 自动识别
    if (ImGui::Button("重新自动识别", ImVec2(-1, 0))) {
        editor.autoDetectLandmarks();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("基于几何分析重新自动识别所有 landmark 位置");
    }
    
    ImGui::Spacing();
    
    // 保存/加载
    if (ImGui::Button("保存映射", ImVec2(160, 0))) {
        editor.saveMapping(savePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("重新加载", ImVec2(-1, 0))) {
        editor.loadMapping(savePath);
    }
    
    ImGui::Separator();
    ImGui::TextWrapped("提示:\n- 在 3D 视图中点击网格顶点来标记当前 landmark\n- 方向键或 A/D 切换 landmark\n- Delete/X 清除当前标记");
    
    ImGui::End();
}

} // namespace luma
