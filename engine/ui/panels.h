// UI Panels - ImGui panel definitions for LUMA Creator
#pragma once

#include "imgui.h"
#include "engine/viewport/viewport.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/scene/scene_graph.h"

namespace luma {
namespace ui {

// Draw the main menu bar
// Returns: path to open if user selected "Open Model", empty otherwise
inline std::string drawMenuBar(Viewport& viewport, bool& shouldQuit, bool& showHelp) {
    std::string openPath;
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Model...", "Ctrl+O")) {
                openPath = "__OPEN_DIALOG__";  // Signal to open file dialog
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                shouldQuit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Camera", "F")) {
                viewport.camera.reset();
            }
            ImGui::MenuItem("Auto Rotate", nullptr, &viewport.settings.autoRotate);
            ImGui::Separator();
            ImGui::MenuItem("Show Grid", "G", &viewport.settings.showGrid);
            ImGui::MenuItem("Show Help", "F1", &showHelp);
            ImGui::EndMenu();
        }
        
        // Right-aligned FPS
        float fpsWidth = ImGui::CalcTextSize("FPS: 999.9").x + 20;
        ImGui::SameLine(ImGui::GetWindowWidth() - fpsWidth);
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
        
        ImGui::EndMainMenuBar();
    }
    
    return openPath;
}

// Draw model info panel
// Returns: true if user clicked "Open Model" button
// asyncProgress: 0.0-1.0 texture loading progress (1.0 = complete)
inline bool drawModelPanel(const RHILoadedModel& model, float asyncProgress = 1.0f) {
    bool openClicked = false;
    
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Model", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::Button("Open Model...", ImVec2(-1, 28))) {
            openClicked = true;
        }
        
        ImGui::Separator();
        
        if (!model.meshes.empty()) {
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "%s", model.name.c_str());
            ImGui::Spacing();
            ImGui::Text("Meshes:    %zu", model.meshes.size());
            ImGui::Text("Vertices:  %zu", model.totalVerts);
            ImGui::Text("Triangles: %zu", model.totalTris);
            ImGui::Text("Textures:  %d", model.textureCount);
            
            // Show texture loading progress
            if (asyncProgress < 1.0f) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Loading textures...");
                ImGui::ProgressBar(asyncProgress, ImVec2(-1, 0));
            }
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No model loaded");
        }
    }
    ImGui::End();
    
    return openClicked;
}

// Helper to draw entity tree recursively
inline void drawEntityNode(Entity* entity, SceneGraph& scene) {
    if (!entity) return;
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (entity->children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (scene.getSelectedEntity() == entity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!entity->enabled) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }
    
    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity->id, flags, "%s", entity->name.c_str());
    
    if (!entity->enabled) {
        ImGui::PopStyleColor();
    }
    
    // Click to select
    if (ImGui::IsItemClicked()) {
        scene.setSelectedEntity(entity);
    }
    
    // Right-click context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
            scene.destroyEntity(entity);
            ImGui::EndPopup();
            if (nodeOpen) ImGui::TreePop();
            return;
        }
        if (ImGui::MenuItem(entity->enabled ? "Disable" : "Enable")) {
            entity->enabled = !entity->enabled;
        }
        ImGui::EndPopup();
    }
    
    if (nodeOpen) {
        for (Entity* child : entity->children) {
            drawEntityNode(child, scene);
        }
        ImGui::TreePop();
    }
}

// Draw scene hierarchy panel
// Returns true if "Add Object" was clicked
inline bool drawSceneHierarchyPanel(SceneGraph& scene) {
    bool addClicked = false;
    
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::Button("Add Object...", ImVec2(-1, 24))) {
            addClicked = true;
        }
        
        ImGui::Separator();
        
        if (scene.getEntityCount() == 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Empty scene");
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Load a model to add objects");
        } else {
            // Draw root entities
            for (Entity* root : scene.getRootEntities()) {
                drawEntityNode(root, scene);
            }
        }
    }
    ImGui::End();
    
    return addClicked;
}

// Draw inspector panel for selected entity
inline void drawInspectorPanel(SceneGraph& scene) {
    Entity* selected = scene.getSelectedEntity();
    
    ImGui::SetNextWindowPos(ImVec2(10, 240), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 280), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if (!selected) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No object selected");
        } else {
            // Name
            char nameBuf[128];
            strncpy(nameBuf, selected->name.c_str(), sizeof(nameBuf) - 1);
            nameBuf[sizeof(nameBuf) - 1] = '\0';
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                selected->name = nameBuf;
            }
            
            ImGui::Checkbox("Enabled", &selected->enabled);
            
            ImGui::Separator();
            ImGui::Text("Transform");
            
            // Position
            float pos[3] = {selected->localTransform.position.x, 
                           selected->localTransform.position.y, 
                           selected->localTransform.position.z};
            if (ImGui::DragFloat3("Position", pos, 0.1f)) {
                selected->localTransform.position = {pos[0], pos[1], pos[2]};
                selected->updateWorldMatrix();
            }
            
            // Rotation (Euler degrees)
            Vec3 eulerDeg = selected->localTransform.getEulerDegrees();
            float rot[3] = {eulerDeg.x, eulerDeg.y, eulerDeg.z};
            if (ImGui::DragFloat3("Rotation", rot, 1.0f)) {
                selected->localTransform.setEulerDegrees({rot[0], rot[1], rot[2]});
                selected->updateWorldMatrix();
            }
            
            // Scale
            float scl[3] = {selected->localTransform.scale.x, 
                           selected->localTransform.scale.y, 
                           selected->localTransform.scale.z};
            if (ImGui::DragFloat3("Scale", scl, 0.01f, 0.001f, 100.0f)) {
                selected->localTransform.scale = {scl[0], scl[1], scl[2]};
                selected->updateWorldMatrix();
            }
            
            if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
                selected->localTransform = Transform();
                selected->updateWorldMatrix();
            }
            
            // Model info
            if (selected->hasModel) {
                ImGui::Separator();
                ImGui::Text("Model");
                ImGui::Text("Meshes: %zu", selected->model.meshes.size());
                ImGui::Text("Vertices: %zu", selected->model.totalVerts);
                ImGui::Text("Triangles: %zu", selected->model.totalTris);
            }
        }
    }
    ImGui::End();
}

// Draw camera control panel
inline void drawCameraPanel(Viewport& viewport) {
    ImGui::SetNextWindowPos(ImVec2(10, 530), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 180), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Mode indicator
        const char* modeStr = "Idle";
        ImVec4 modeColor(0.5f, 0.5f, 0.5f, 1.0f);
        switch (viewport.cameraMode) {
        case CameraMode::Orbit: modeStr = "Orbiting"; modeColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); break;
        case CameraMode::Pan:   modeStr = "Panning";  modeColor = ImVec4(0.3f, 1.0f, 0.5f, 1.0f); break;
        case CameraMode::Zoom:  modeStr = "Zooming";  modeColor = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); break;
        default: break;
        }
        ImGui::Text("Mode:"); ImGui::SameLine();
        ImGui::TextColored(modeColor, "%s", modeStr);
        
        ImGui::Separator();
        
        ImGui::Text("Yaw:   %.1f deg", viewport.camera.yaw * 57.2958f);
        ImGui::Text("Pitch: %.1f deg", viewport.camera.pitch * 57.2958f);
        ImGui::Text("Dist:  %.2f", viewport.camera.distance);
        
        ImGui::Separator();
        
        ImGui::Checkbox("Auto Rotate", &viewport.settings.autoRotate);
        if (viewport.settings.autoRotate) {
            ImGui::SliderFloat("Speed", &viewport.settings.autoRotateSpeed, 0.1f, 2.0f);
        }
        
        ImGui::Spacing();
        if (ImGui::Button("Reset Camera", ImVec2(-1, 0))) {
            viewport.camera.reset();
        }
    }
    ImGui::End();
}

// Draw help overlay
inline void drawHelpOverlay(bool& showHelp, int windowWidth, int windowHeight) {
    if (!showHelp) return;
    
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), 
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(340, 200));
    
    if (ImGui::Begin("Controls (F1 to close)", &showHelp,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Maya-Style Camera Controls:");
        ImGui::Separator();
        ImGui::BulletText("Alt + Left Mouse:   Orbit (Rotate)");
        ImGui::BulletText("Alt + Middle Mouse: Pan (Move)");
        ImGui::BulletText("Alt + Right Mouse:  Zoom");
        ImGui::BulletText("Mouse Wheel:        Zoom");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Shortcuts:");
        ImGui::Separator();
        ImGui::BulletText("F:  Reset camera");
        ImGui::BulletText("G:  Toggle grid");
        ImGui::BulletText("F1: Toggle this help");
    }
    ImGui::End();
}

// Draw orientation gizmo (viewport corner)
inline void drawOrientationGizmo(const OrbitCamera& camera, int windowWidth, int windowHeight) {
    const float gizmoSize = 60.0f;
    const float margin = 20.0f;
    ImVec2 center(windowWidth - gizmoSize - margin, windowHeight - gizmoSize - margin - 24.0f);
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    float yaw = camera.yaw;
    float pitch = camera.pitch;
    
    // Transform world axes to screen space
    float xAxisX = cosf(yaw);
    float xAxisY = sinf(pitch) * sinf(yaw);
    float yAxisX = 0.0f;
    float yAxisY = -cosf(pitch);
    float zAxisX = -sinf(yaw);
    float zAxisY = sinf(pitch) * cosf(yaw);
    
    float axisLen = gizmoSize * 0.4f;
    
    struct AxisInfo { float x, y, z; ImU32 color; const char* label; };
    AxisInfo axes[3] = {
        { xAxisX, xAxisY, sinf(yaw), IM_COL32(220, 60, 60, 255), "X" },
        { yAxisX, yAxisY, 0.0f, IM_COL32(60, 220, 60, 255), "Y" },
        { zAxisX, zAxisY, cosf(yaw), IM_COL32(60, 100, 220, 255), "Z" }
    };
    
    // Sort by depth
    for (int i = 0; i < 2; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (axes[i].z > axes[j].z) {
                AxisInfo tmp = axes[i]; axes[i] = axes[j]; axes[j] = tmp;
            }
        }
    }
    
    // Background circle
    drawList->AddCircleFilled(center, gizmoSize * 0.5f, IM_COL32(40, 40, 45, 200));
    drawList->AddCircle(center, gizmoSize * 0.5f, IM_COL32(80, 80, 85, 255), 32, 1.5f);
    
    // Axes
    for (int i = 0; i < 3; i++) {
        ImVec2 end(center.x + axes[i].x * axisLen, center.y + axes[i].y * axisLen);
        drawList->AddLine(center, end, axes[i].color, 2.5f);
        drawList->AddText(ImVec2(end.x - 4, end.y - 7), axes[i].color, axes[i].label);
    }
}

// Draw status bar
inline void drawStatusBar(int windowWidth, int windowHeight) {
    float statusBarHeight = 24.0f;
    ImGui::SetNextWindowPos(ImVec2(0, (float)windowHeight - statusBarHeight));
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, statusBarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    
    if (ImGui::Begin("##StatusBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Alt+LMB: Orbit | Alt+MMB: Pan | Alt+RMB/Wheel: Zoom | F: Reset | G: Grid | F1: Help");
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// Apply dark theme
inline void applyDarkTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.42f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
}

}  // namespace ui
}  // namespace luma
