// Manual Landmark Annotation UI
// Allows users to click on a photo to mark facial landmarks
#pragma once

#include "engine/foundation/math_types.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <functional>

namespace luma {

// Simplified landmark set for manual annotation (key points only)
// Users mark these 15 key points, and we interpolate the rest
struct KeyLandmarks {
    // 15 key points that define face shape
    Vec2 chinCenter;        // 下巴中心
    Vec2 jawLeft;           // 左下颌角
    Vec2 jawRight;          // 右下颌角
    
    Vec2 leftEyeInner;      // 左眼内角
    Vec2 leftEyeOuter;      // 左眼外角
    Vec2 rightEyeInner;     // 右眼内角
    Vec2 rightEyeOuter;     // 右眼外角
    
    Vec2 leftBrowInner;     // 左眉内侧
    Vec2 leftBrowOuter;     // 左眉外侧
    Vec2 rightBrowInner;    // 右眉内侧
    Vec2 rightBrowOuter;    // 右眉外侧
    
    Vec2 noseBridge;        // 鼻梁 (两眼之间)
    Vec2 noseTip;           // 鼻尖
    
    Vec2 mouthLeft;         // 左嘴角
    Vec2 mouthRight;        // 右嘴角
    
    bool isValid() const {
        // Check if all points are set (non-zero)
        return chinCenter.x != 0 || chinCenter.y != 0;
    }
};

// Landmark annotation state
struct LandmarkAnnotationState {
    bool active = false;
    int currentPointIndex = 0;  // Which point we're marking
    KeyLandmarks landmarks;
    
    // Photo display
    unsigned int textureId = 0;
    int photoWidth = 0;
    int photoHeight = 0;
    std::vector<uint8_t> photoData;
    
    // UI state
    float zoomLevel = 1.0f;
    Vec2 panOffset{0, 0};
    
    void reset() {
        currentPointIndex = 0;
        landmarks = KeyLandmarks();
    }
};

// Point names for UI
inline const char* getKeyLandmarkName(int index) {
    static const char* names[] = {
        "下巴中心 (Chin Center)",
        "左下颌角 (Left Jaw)",
        "右下颌角 (Right Jaw)",
        "左眼内角 (Left Eye Inner)",
        "左眼外角 (Left Eye Outer)",
        "右眼内角 (Right Eye Inner)",
        "右眼外角 (Right Eye Outer)",
        "左眉内侧 (Left Brow Inner)",
        "左眉外侧 (Left Brow Outer)",
        "右眉内侧 (Right Brow Inner)",
        "右眉外侧 (Right Brow Outer)",
        "鼻梁 (Nose Bridge)",
        "鼻尖 (Nose Tip)",
        "左嘴角 (Mouth Left)",
        "右嘴角 (Mouth Right)",
    };
    if (index >= 0 && index < 15) return names[index];
    return "Unknown";
}

inline Vec2* getKeyLandmarkPtr(KeyLandmarks& lm, int index) {
    Vec2* ptrs[] = {
        &lm.chinCenter, &lm.jawLeft, &lm.jawRight,
        &lm.leftEyeInner, &lm.leftEyeOuter, &lm.rightEyeInner, &lm.rightEyeOuter,
        &lm.leftBrowInner, &lm.leftBrowOuter, &lm.rightBrowInner, &lm.rightBrowOuter,
        &lm.noseBridge, &lm.noseTip,
        &lm.mouthLeft, &lm.mouthRight,
    };
    if (index >= 0 && index < 15) return ptrs[index];
    return nullptr;
}

// Render the annotation UI
inline void renderLandmarkAnnotationUI(LandmarkAnnotationState& state, 
                                        std::function<void(const KeyLandmarks&)> onComplete) {
    if (!state.active) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("标记面部特征点", &state.active)) {
        ImGui::End();
        return;
    }
    
    // Instructions
    ImGui::TextWrapped("请在照片上点击标记 %d/15 个关键点", state.currentPointIndex + 1);
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "当前: %s", getKeyLandmarkName(state.currentPointIndex));
    ImGui::Separator();
    
    // Progress
    ImGui::ProgressBar(state.currentPointIndex / 15.0f);
    
    // Photo display area
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    availSize.y -= 60;  // Leave room for buttons
    
    if (state.textureId != 0 && state.photoWidth > 0 && state.photoHeight > 0) {
        // Calculate display size maintaining aspect ratio
        float photoAspect = (float)state.photoWidth / state.photoHeight;
        float displayW = availSize.x;
        float displayH = displayW / photoAspect;
        if (displayH > availSize.y) {
            displayH = availSize.y;
            displayW = displayH * photoAspect;
        }
        
        // Center the image
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float offsetX = (availSize.x - displayW) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        
        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        
        // Draw image
        ImGui::Image((ImTextureID)(uintptr_t)state.textureId, ImVec2(displayW, displayH));
        
        // Handle clicks on image
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            
            // Convert to normalized coordinates (0-1)
            float normX = (mousePos.x - imagePos.x) / displayW;
            float normY = (mousePos.y - imagePos.y) / displayH;
            
            // Clamp to valid range
            normX = std::max(0.0f, std::min(1.0f, normX));
            normY = std::max(0.0f, std::min(1.0f, normY));
            
            // Set current landmark
            Vec2* ptr = getKeyLandmarkPtr(state.landmarks, state.currentPointIndex);
            if (ptr) {
                ptr->x = normX;
                ptr->y = normY;
                
                // Move to next point
                state.currentPointIndex++;
                if (state.currentPointIndex >= 15) {
                    // All points marked
                    if (onComplete) {
                        onComplete(state.landmarks);
                    }
                    state.active = false;
                }
            }
        }
        
        // Draw marked points
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (int i = 0; i < 15; i++) {
            Vec2* ptr = getKeyLandmarkPtr(state.landmarks, i);
            if (ptr && (ptr->x != 0 || ptr->y != 0)) {
                float screenX = imagePos.x + ptr->x * displayW;
                float screenY = imagePos.y + ptr->y * displayH;
                
                // Color: green for marked, yellow for current
                ImU32 color = (i < state.currentPointIndex) ? 
                    IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 255);
                
                drawList->AddCircleFilled(ImVec2(screenX, screenY), 5.0f, color);
                drawList->AddCircle(ImVec2(screenX, screenY), 5.0f, IM_COL32(0, 0, 0, 255), 12, 2.0f);
                
                // Draw index number
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", i + 1);
                drawList->AddText(ImVec2(screenX + 8, screenY - 8), IM_COL32(255, 255, 255, 255), buf);
            }
        }
        
        // Draw crosshair at cursor position when hovering
        if (ImGui::IsItemHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            drawList->AddLine(ImVec2(mousePos.x - 10, mousePos.y), ImVec2(mousePos.x + 10, mousePos.y), 
                             IM_COL32(255, 0, 0, 200), 1.0f);
            drawList->AddLine(ImVec2(mousePos.x, mousePos.y - 10), ImVec2(mousePos.x, mousePos.y + 10), 
                             IM_COL32(255, 0, 0, 200), 1.0f);
        }
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "请先加载照片");
    }
    
    ImGui::Separator();
    
    // Buttons
    if (ImGui::Button("撤销上一个点") && state.currentPointIndex > 0) {
        state.currentPointIndex--;
        Vec2* ptr = getKeyLandmarkPtr(state.landmarks, state.currentPointIndex);
        if (ptr) {
            ptr->x = 0;
            ptr->y = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("重新开始")) {
        state.reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("取消")) {
        state.active = false;
    }
    
    ImGui::End();
}

// Convert 15 key landmarks to full 68 landmarks (interpolation)
inline std::vector<Vec2> expandKeyLandmarksTo68(const KeyLandmarks& key) {
    std::vector<Vec2> full(68);
    
    // Jaw contour (0-16): interpolate between jawRight, chinCenter, jawLeft
    for (int i = 0; i <= 8; i++) {
        float t = i / 8.0f;
        full[i].x = key.jawRight.x + (key.chinCenter.x - key.jawRight.x) * t;
        full[i].y = key.jawRight.y + (key.chinCenter.y - key.jawRight.y) * t;
    }
    for (int i = 8; i <= 16; i++) {
        float t = (i - 8) / 8.0f;
        full[i].x = key.chinCenter.x + (key.jawLeft.x - key.chinCenter.x) * t;
        full[i].y = key.chinCenter.y + (key.jawLeft.y - key.chinCenter.y) * t;
    }
    
    // Left eyebrow (17-21)
    for (int i = 0; i < 5; i++) {
        float t = i / 4.0f;
        full[17 + i].x = key.leftBrowInner.x + (key.leftBrowOuter.x - key.leftBrowInner.x) * t;
        full[17 + i].y = key.leftBrowInner.y + (key.leftBrowOuter.y - key.leftBrowInner.y) * t;
    }
    
    // Right eyebrow (22-26)
    for (int i = 0; i < 5; i++) {
        float t = i / 4.0f;
        full[22 + i].x = key.rightBrowInner.x + (key.rightBrowOuter.x - key.rightBrowInner.x) * t;
        full[22 + i].y = key.rightBrowInner.y + (key.rightBrowOuter.y - key.rightBrowInner.y) * t;
    }
    
    // Nose (27-35)
    full[27] = key.noseBridge;
    full[30] = Vec2((key.noseBridge.x + key.noseTip.x) * 0.5f, 
                    (key.noseBridge.y + key.noseTip.y) * 0.5f);
    full[33] = key.noseTip;
    // Interpolate others
    for (int i = 28; i < 30; i++) {
        float t = (i - 27) / 3.0f;
        full[i].x = key.noseBridge.x + (full[30].x - key.noseBridge.x) * t;
        full[i].y = key.noseBridge.y + (full[30].y - key.noseBridge.y) * t;
    }
    // Nose wings
    float noseWidth = std::abs(key.mouthRight.x - key.mouthLeft.x) * 0.4f;
    full[31] = Vec2(key.noseTip.x - noseWidth, key.noseTip.y);
    full[32] = Vec2(key.noseTip.x - noseWidth * 0.5f, key.noseTip.y);
    full[34] = Vec2(key.noseTip.x + noseWidth * 0.5f, key.noseTip.y);
    full[35] = Vec2(key.noseTip.x + noseWidth, key.noseTip.y);
    
    // Left eye (36-41)
    Vec2 leftEyeCenter = Vec2((key.leftEyeInner.x + key.leftEyeOuter.x) * 0.5f,
                               (key.leftEyeInner.y + key.leftEyeOuter.y) * 0.5f);
    float leftEyeW = std::abs(key.leftEyeOuter.x - key.leftEyeInner.x) * 0.5f;
    float leftEyeH = leftEyeW * 0.4f;
    full[36] = key.leftEyeOuter;
    full[37] = Vec2(leftEyeCenter.x + leftEyeW * 0.5f, leftEyeCenter.y - leftEyeH);
    full[38] = Vec2(leftEyeCenter.x - leftEyeW * 0.5f, leftEyeCenter.y - leftEyeH);
    full[39] = key.leftEyeInner;
    full[40] = Vec2(leftEyeCenter.x - leftEyeW * 0.5f, leftEyeCenter.y + leftEyeH);
    full[41] = Vec2(leftEyeCenter.x + leftEyeW * 0.5f, leftEyeCenter.y + leftEyeH);
    
    // Right eye (42-47)
    Vec2 rightEyeCenter = Vec2((key.rightEyeInner.x + key.rightEyeOuter.x) * 0.5f,
                                (key.rightEyeInner.y + key.rightEyeOuter.y) * 0.5f);
    float rightEyeW = std::abs(key.rightEyeOuter.x - key.rightEyeInner.x) * 0.5f;
    float rightEyeH = rightEyeW * 0.4f;
    full[42] = key.rightEyeInner;
    full[43] = Vec2(rightEyeCenter.x - rightEyeW * 0.5f, rightEyeCenter.y - rightEyeH);
    full[44] = Vec2(rightEyeCenter.x + rightEyeW * 0.5f, rightEyeCenter.y - rightEyeH);
    full[45] = key.rightEyeOuter;
    full[46] = Vec2(rightEyeCenter.x + rightEyeW * 0.5f, rightEyeCenter.y + rightEyeH);
    full[47] = Vec2(rightEyeCenter.x - rightEyeW * 0.5f, rightEyeCenter.y + rightEyeH);
    
    // Mouth (48-67)
    Vec2 mouthCenter = Vec2((key.mouthLeft.x + key.mouthRight.x) * 0.5f,
                             (key.mouthLeft.y + key.mouthRight.y) * 0.5f);
    float mouthW = std::abs(key.mouthRight.x - key.mouthLeft.x) * 0.5f;
    float mouthH = mouthW * 0.3f;
    
    // Outer mouth (48-59)
    full[48] = key.mouthLeft;
    full[54] = key.mouthRight;
    full[51] = Vec2(mouthCenter.x, mouthCenter.y - mouthH);  // Top center
    full[57] = Vec2(mouthCenter.x, mouthCenter.y + mouthH);  // Bottom center
    // Interpolate
    for (int i = 49; i <= 50; i++) {
        float t = (i - 48) / 3.0f;
        full[i].x = key.mouthLeft.x + (full[51].x - key.mouthLeft.x) * t;
        full[i].y = key.mouthLeft.y + (full[51].y - key.mouthLeft.y) * t;
    }
    for (int i = 52; i <= 53; i++) {
        float t = (i - 51) / 3.0f;
        full[i].x = full[51].x + (key.mouthRight.x - full[51].x) * t;
        full[i].y = full[51].y + (key.mouthRight.y - full[51].y) * t;
    }
    for (int i = 55; i <= 56; i++) {
        float t = (i - 54) / 3.0f;
        full[i].x = key.mouthRight.x + (full[57].x - key.mouthRight.x) * t;
        full[i].y = key.mouthRight.y + (full[57].y - key.mouthRight.y) * t;
    }
    for (int i = 58; i <= 59; i++) {
        float t = (i - 57) / 3.0f;
        full[i].x = full[57].x + (key.mouthLeft.x - full[57].x) * t;
        full[i].y = full[57].y + (key.mouthLeft.y - full[57].y) * t;
    }
    
    // Inner mouth (60-67) - smaller version
    float innerScale = 0.6f;
    for (int i = 0; i < 8; i++) {
        int outerIdx = 48 + (i * 12 / 8);  // Map to outer mouth
        full[60 + i].x = mouthCenter.x + (full[outerIdx].x - mouthCenter.x) * innerScale;
        full[60 + i].y = mouthCenter.y + (full[outerIdx].y - mouthCenter.y) * innerScale;
    }
    
    return full;
}

} // namespace luma
