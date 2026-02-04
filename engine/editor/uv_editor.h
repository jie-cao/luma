// LUMA UV Editor
// 2D UV 编辑器窗口
// 支持 UV 投影、展开、修复

#pragma once

#include "../mesh/edit_mesh.h"
#include "../ui/localization.h"
#include "imgui.h"
#include <vector>
#include <set>
#include <functional>
#include <cmath>

namespace luma {
namespace editor {

// ============================================================================
// UV 编辑器
// ============================================================================
class UVEditor {
public:
    // 状态
    bool isOpen = false;
    EditMesh* mesh = nullptr;
    
    // 视图参数
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    
    // 选择
    std::set<uint32_t> selectedLoops;  // Loop 索引（面索引 * 1000 + loop索引）
    
    // 显示选项
    bool showTexture = true;
    bool showGrid = true;
    bool showStretch = false;
    bool showOverlap = false;
    bool showBounds = true;
    
    // 纹理 ID（由外部设置）
    ImTextureID textureId = nullptr;
    
    // 回调
    std::function<void()> onUVChanged;
    std::function<void()> onClose;
    
    // =========================================================================
    // 绘制主窗口
    // =========================================================================
    
    void draw() {
        if (!isOpen || !mesh) return;
        
        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin(loc("UV Editor"), &isOpen, ImGuiWindowFlags_MenuBar)) {
            drawMenuBar();
            drawToolbar();
            drawUVCanvas();
            drawStatusBar();
        }
        ImGui::End();
        
        if (!isOpen && onClose) {
            onClose();
        }
    }
    
    // =========================================================================
    // 打开/关闭
    // =========================================================================
    
    void open(EditMesh* editMesh) {
        mesh = editMesh;
        isOpen = true;
        selectedLoops.clear();
        resetView();
    }
    
    void close() {
        isOpen = false;
        mesh = nullptr;
        selectedLoops.clear();
    }
    
    void resetView() {
        zoom = 1.0f;
        panX = 0.0f;
        panY = 0.0f;
    }
    
private:
    // =========================================================================
    // 菜单栏
    // =========================================================================
    
    void drawMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu(loc("UV"))) {
                if (ImGui::MenuItem(loc("Select All"), "A")) {
                    selectAll();
                }
                if (ImGui::MenuItem(loc("Select None"), "Alt+A")) {
                    selectNone();
                }
                ImGui::Separator();
                if (ImGui::MenuItem(loc("Pack Islands"), "Ctrl+P")) {
                    packIslands();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu(loc("Projection"))) {
                if (ImGui::MenuItem(loc("Planar (Top)"))) {
                    projectPlanar(0, 1, 0);
                }
                if (ImGui::MenuItem(loc("Planar (Front)"))) {
                    projectPlanar(0, 0, 1);
                }
                if (ImGui::MenuItem(loc("Planar (Side)"))) {
                    projectPlanar(1, 0, 0);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(loc("Box Projection"))) {
                    projectBox();
                }
                if (ImGui::MenuItem(loc("Cylindrical"))) {
                    projectCylindrical();
                }
                if (ImGui::MenuItem(loc("Spherical"))) {
                    projectSpherical();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu(loc("View"))) {
                ImGui::Checkbox(loc("Show Texture"), &showTexture);
                ImGui::Checkbox(loc("Show Grid"), &showGrid);
                ImGui::Checkbox(loc("Show UV Bounds"), &showBounds);
                ImGui::Separator();
                ImGui::Checkbox(loc("Highlight Stretch"), &showStretch);
                ImGui::Checkbox(loc("Highlight Overlap"), &showOverlap);
                ImGui::Separator();
                if (ImGui::MenuItem(loc("Reset View"), "Home")) {
                    resetView();
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
    }
    
    // =========================================================================
    // 工具栏
    // =========================================================================
    
    void drawToolbar() {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        
        // 投影按钮
        if (ImGui::Button(loc("Box"))) { projectBox(); }
        ImGui::SameLine();
        if (ImGui::Button(loc("Planar"))) { projectPlanar(0, 1, 0); }
        ImGui::SameLine();
        if (ImGui::Button(loc("Unwrap"))) { unwrapLSCM(); }
        ImGui::SameLine();
        
        ImGui::Separator();
        ImGui::SameLine();
        
        // 变换按钮
        if (ImGui::Button(loc("Flip H"))) { flipHorizontal(); }
        ImGui::SameLine();
        if (ImGui::Button(loc("Flip V"))) { flipVertical(); }
        ImGui::SameLine();
        if (ImGui::Button(loc("Rotate 90"))) { rotate90(); }
        ImGui::SameLine();
        
        ImGui::Separator();
        ImGui::SameLine();
        
        // 缩放
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("##Zoom", &zoom, 0.1f, 5.0f, "%.1fx");
        ImGui::SameLine();
        if (ImGui::Button(loc("Fit"))) { fitToView(); }
        
        ImGui::PopStyleVar();
        ImGui::Spacing();
    }
    
    // =========================================================================
    // UV 画布
    // =========================================================================
    
    void drawUVCanvas() {
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        canvasSize.y -= 25;  // 留出状态栏空间
        
        if (canvasSize.x < 50 || canvasSize.y < 50) return;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // 背景
        drawList->AddRectFilled(canvasPos, 
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
            IM_COL32(30, 32, 36, 255));
        
        // UV 空间 (0,0) 到 (1,1) 映射到画布
        float uvSize = std::min(canvasSize.x, canvasSize.y) * 0.9f * zoom;
        float offsetX = canvasPos.x + (canvasSize.x - uvSize) * 0.5f + panX;
        float offsetY = canvasPos.y + (canvasSize.y - uvSize) * 0.5f + panY;
        
        auto uvToScreen = [&](float u, float v) -> ImVec2 {
            return ImVec2(offsetX + u * uvSize, offsetY + (1.0f - v) * uvSize);
        };
        
        // 绘制纹理背景
        if (showTexture && textureId) {
            ImVec2 p0 = uvToScreen(0, 1);
            ImVec2 p1 = uvToScreen(1, 0);
            drawList->AddImage(textureId, p0, p1);
        }
        
        // 绘制网格
        if (showGrid) {
            ImU32 gridColor = IM_COL32(60, 62, 66, 255);
            ImU32 gridColorMain = IM_COL32(80, 82, 86, 255);
            
            // 次网格
            for (int i = 0; i <= 10; i++) {
                float t = i / 10.0f;
                ImVec2 h0 = uvToScreen(0, t);
                ImVec2 h1 = uvToScreen(1, t);
                ImVec2 v0 = uvToScreen(t, 0);
                ImVec2 v1 = uvToScreen(t, 1);
                
                ImU32 col = (i == 0 || i == 10 || i == 5) ? gridColorMain : gridColor;
                drawList->AddLine(h0, h1, col);
                drawList->AddLine(v0, v1, col);
            }
        }
        
        // 绘制 UV 边界框
        if (showBounds) {
            ImVec2 p0 = uvToScreen(0, 1);
            ImVec2 p1 = uvToScreen(1, 0);
            drawList->AddRect(p0, p1, IM_COL32(100, 150, 255, 255), 0, 0, 2.0f);
        }
        
        // 绘制 UV 面
        if (mesh) {
            for (size_t fi = 0; fi < mesh->faces.size(); fi++) {
                const auto& face = mesh->faces[fi];
                if (face.loops.size() < 3) continue;
                
                // 检查是否选中
                bool faceSelected = false;
                for (size_t li = 0; li < face.loops.size(); li++) {
                    uint32_t loopId = static_cast<uint32_t>(fi * 1000 + li);
                    if (selectedLoops.count(loopId)) {
                        faceSelected = true;
                        break;
                    }
                }
                
                // 绘制填充（如果显示拉伸）
                if (showStretch) {
                    float stretch = mesh->calculateUVStretch(face);
                    ImU32 stretchColor;
                    if (stretch < 0.1f) {
                        stretchColor = IM_COL32(50, 150, 50, 80);  // 绿色 - 低拉伸
                    } else if (stretch < 0.3f) {
                        stretchColor = IM_COL32(150, 150, 50, 80); // 黄色 - 中拉伸
                    } else {
                        stretchColor = IM_COL32(200, 50, 50, 100); // 红色 - 高拉伸
                    }
                    
                    // 简单三角形填充
                    if (face.loops.size() >= 3) {
                        ImVec2 p0 = uvToScreen(face.loops[0].uv[0], face.loops[0].uv[1]);
                        for (size_t j = 1; j < face.loops.size() - 1; j++) {
                            ImVec2 p1 = uvToScreen(face.loops[j].uv[0], face.loops[j].uv[1]);
                            ImVec2 p2 = uvToScreen(face.loops[j+1].uv[0], face.loops[j+1].uv[1]);
                            drawList->AddTriangleFilled(p0, p1, p2, stretchColor);
                        }
                    }
                }
                
                // 绘制边
                ImU32 edgeColor = faceSelected ? 
                    IM_COL32(255, 180, 50, 255) :   // 选中：橙色
                    IM_COL32(200, 200, 200, 180);   // 未选中：灰色
                
                for (size_t li = 0; li < face.loops.size(); li++) {
                    size_t nextLi = (li + 1) % face.loops.size();
                    ImVec2 p0 = uvToScreen(face.loops[li].uv[0], face.loops[li].uv[1]);
                    ImVec2 p1 = uvToScreen(face.loops[nextLi].uv[0], face.loops[nextLi].uv[1]);
                    drawList->AddLine(p0, p1, edgeColor, 1.5f);
                }
                
                // 绘制顶点
                for (size_t li = 0; li < face.loops.size(); li++) {
                    uint32_t loopId = static_cast<uint32_t>(fi * 1000 + li);
                    bool vertSelected = selectedLoops.count(loopId) > 0;
                    
                    ImVec2 p = uvToScreen(face.loops[li].uv[0], face.loops[li].uv[1]);
                    
                    if (vertSelected) {
                        drawList->AddCircleFilled(p, 5.0f, IM_COL32(255, 150, 50, 255));
                        drawList->AddCircle(p, 5.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
                    } else {
                        drawList->AddCircleFilled(p, 3.0f, IM_COL32(200, 200, 200, 200));
                    }
                }
            }
        }
        
        // 处理输入
        ImGui::SetCursorScreenPos(canvasPos);
        ImGui::InvisibleButton("uv_canvas", canvasSize, 
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        
        bool isHovered = ImGui::IsItemHovered();
        bool isActive = ImGui::IsItemActive();
        
        // 平移（中键或右键拖拽）
        if (isActive && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || 
                         ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            panX += delta.x;
            panY += delta.y;
        }
        
        // 缩放（滚轮）
        if (isHovered) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0) {
                zoom *= (1.0f + wheel * 0.1f);
                zoom = std::clamp(zoom, 0.1f, 10.0f);
            }
        }
        
        // 点击选择
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetIO().MousePos;
            
            // 反向计算 UV 坐标
            float u = (mousePos.x - offsetX) / uvSize;
            float v = 1.0f - (mousePos.y - offsetY) / uvSize;
            
            // 查找最近的 UV 点
            selectNearestLoop(u, v, 0.02f / zoom);
        }
    }
    
    // =========================================================================
    // 状态栏
    // =========================================================================
    
    void drawStatusBar() {
        ImGui::Separator();
        
        if (mesh) {
            int faceCount = static_cast<int>(mesh->faces.size());
            int selectedCount = static_cast<int>(selectedLoops.size());
            
            ImGui::Text("%s: %d | %s: %d | Zoom: %.1fx", 
                       loc("Faces"), faceCount,
                       loc("Selected"), selectedCount,
                       zoom);
        }
    }
    
    // =========================================================================
    // 选择操作
    // =========================================================================
    
    void selectAll() {
        if (!mesh) return;
        selectedLoops.clear();
        for (size_t fi = 0; fi < mesh->faces.size(); fi++) {
            for (size_t li = 0; li < mesh->faces[fi].loops.size(); li++) {
                selectedLoops.insert(static_cast<uint32_t>(fi * 1000 + li));
            }
        }
    }
    
    void selectNone() {
        selectedLoops.clear();
    }
    
    void selectNearestLoop(float u, float v, float threshold) {
        if (!mesh) return;
        
        float minDist = threshold;
        uint32_t nearestLoop = UINT32_MAX;
        
        for (size_t fi = 0; fi < mesh->faces.size(); fi++) {
            for (size_t li = 0; li < mesh->faces[fi].loops.size(); li++) {
                const auto& loop = mesh->faces[fi].loops[li];
                float du = loop.uv[0] - u;
                float dv = loop.uv[1] - v;
                float dist = std::sqrt(du*du + dv*dv);
                
                if (dist < minDist) {
                    minDist = dist;
                    nearestLoop = static_cast<uint32_t>(fi * 1000 + li);
                }
            }
        }
        
        bool additive = ImGui::GetIO().KeyShift;
        if (!additive) {
            selectedLoops.clear();
        }
        
        if (nearestLoop != UINT32_MAX) {
            if (selectedLoops.count(nearestLoop)) {
                selectedLoops.erase(nearestLoop);
            } else {
                selectedLoops.insert(nearestLoop);
            }
        }
    }
    
    // =========================================================================
    // UV 投影操作
    // =========================================================================
    
    void projectPlanar(float nx, float ny, float nz) {
        if (!mesh) return;
        
        mesh->pushUndo();
        
        // 如果有选择，只对选中的面投影
        std::set<uint32_t> facesToProject;
        if (!selectedLoops.empty()) {
            for (uint32_t loopId : selectedLoops) {
                facesToProject.insert(loopId / 1000);
            }
        } else {
            // 没有选择，投影所有面
            for (size_t i = 0; i < mesh->faces.size(); i++) {
                facesToProject.insert(static_cast<uint32_t>(i));
            }
        }
        
        // 保存当前选择
        auto savedSelection = mesh->selectedFaces;
        mesh->selectedFaces.clear();
        for (uint32_t fi : facesToProject) {
            mesh->selectedFaces.insert(fi);
        }
        
        float normal[3] = {nx, ny, nz};
        mesh->projectUVPlanar(normal);
        
        // 恢复选择
        mesh->selectedFaces = savedSelection;
        
        if (onUVChanged) onUVChanged();
    }
    
    void projectBox() {
        if (!mesh) return;
        
        mesh->pushUndo();
        
        auto savedSelection = mesh->selectedFaces;
        
        if (!selectedLoops.empty()) {
            mesh->selectedFaces.clear();
            for (uint32_t loopId : selectedLoops) {
                mesh->selectedFaces.insert(loopId / 1000);
            }
        } else {
            mesh->selectAll();
        }
        
        mesh->projectUVBox();
        
        mesh->selectedFaces = savedSelection;
        
        if (onUVChanged) onUVChanged();
    }
    
    void projectCylindrical() {
        if (!mesh) return;
        
        mesh->pushUndo();
        
        // 简单圆柱投影实现
        for (auto& face : mesh->faces) {
            for (auto& loop : face.loops) {
                const float* pos = mesh->vertices[loop.vertexIndex].position;
                float theta = std::atan2(pos[2], pos[0]);
                loop.uv[0] = (theta + 3.14159265f) / (2.0f * 3.14159265f);
                loop.uv[1] = pos[1];
            }
        }
        
        mesh->normalizeUVs();
        
        if (onUVChanged) onUVChanged();
    }
    
    void projectSpherical() {
        if (!mesh) return;
        
        mesh->pushUndo();
        
        const float PI = 3.14159265f;
        
        for (auto& face : mesh->faces) {
            for (auto& loop : face.loops) {
                const float* pos = mesh->vertices[loop.vertexIndex].position;
                float len = std::sqrt(pos[0]*pos[0] + pos[1]*pos[1] + pos[2]*pos[2]);
                if (len < 1e-6f) len = 1.0f;
                
                float nx = pos[0] / len;
                float ny = pos[1] / len;
                float nz = pos[2] / len;
                
                loop.uv[0] = (std::atan2(nz, nx) + PI) / (2.0f * PI);
                loop.uv[1] = std::acos(std::clamp(ny, -1.0f, 1.0f)) / PI;
            }
        }
        
        if (onUVChanged) onUVChanged();
    }
    
    void unwrapLSCM() {
        // TODO: 实现 LSCM 展开算法
        // 暂时使用 Box 投影代替
        projectBox();
    }
    
    // =========================================================================
    // UV 变换操作
    // =========================================================================
    
    void flipHorizontal() {
        if (!mesh) return;
        mesh->pushUndo();
        
        for (auto& face : mesh->faces) {
            for (auto& loop : face.loops) {
                loop.uv[0] = 1.0f - loop.uv[0];
            }
        }
        
        if (onUVChanged) onUVChanged();
    }
    
    void flipVertical() {
        if (!mesh) return;
        mesh->pushUndo();
        
        for (auto& face : mesh->faces) {
            for (auto& loop : face.loops) {
                loop.uv[1] = 1.0f - loop.uv[1];
            }
        }
        
        if (onUVChanged) onUVChanged();
    }
    
    void rotate90() {
        if (!mesh) return;
        mesh->pushUndo();
        
        for (auto& face : mesh->faces) {
            for (auto& loop : face.loops) {
                float u = loop.uv[0];
                float v = loop.uv[1];
                loop.uv[0] = 1.0f - v;
                loop.uv[1] = u;
            }
        }
        
        if (onUVChanged) onUVChanged();
    }
    
    void packIslands() {
        // TODO: 实现 UV 岛打包算法
        // 暂时只规范化 UV
        if (mesh) {
            mesh->pushUndo();
            mesh->normalizeUVs();
            if (onUVChanged) onUVChanged();
        }
    }
    
    void fitToView() {
        zoom = 1.0f;
        panX = 0.0f;
        panY = 0.0f;
    }
};

} // namespace editor
} // namespace luma
