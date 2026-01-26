// LUMA Editor UI System
// Complete ImGui-based editor interface
#pragma once

#include "imgui.h"
#include "engine/viewport/viewport.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/post_process.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/material/material.h"
#include "engine/editor/gizmo.h"
#include "engine/editor/command.h"
#include "engine/animation/animation.h"
#include "engine/export/screenshot.h"
#include "engine/lighting/light.h"
#include "engine/rendering/lod.h"
#include "engine/rendering/instancing.h"
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

namespace luma {
namespace ui {

// ===== Editor State =====
struct EditorState {
    // Window visibility
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAssetBrowser = true;
    bool showAnimationTimeline = false;
    bool showPostProcess = true;
    bool showRenderSettings = true;
    bool showLighting = true;
    bool showConsole = false;
    bool showHelp = false;
    bool showStats = true;
    bool showShaderStatus = true;
    bool showScreenshotDialog = false;
    
    // Gizmo
    GizmoMode gizmoMode = GizmoMode::Translate;
    bool gizmoLocalSpace = false;
    bool snapEnabled = false;
    float snapTranslate = 1.0f;
    float snapRotate = 15.0f;
    float snapScale = 0.1f;
    
    // Asset browser
    std::string currentAssetPath = ".";
    std::string selectedAsset;
    
    // Animation
    bool animationPlaying = false;
    float animationTime = 0.0f;
    float animationSpeed = 1.0f;
    std::string currentClip;
    
    // Console
    std::vector<std::string> consoleLogs;
    
    // History panel
    bool showHistory = false;
    
    // Screenshot settings
    ScreenshotSettings screenshotSettings;
    std::string lastScreenshotPath;
    bool screenshotPending = false;
    
    // Performance optimization stats
    struct CullStats {
        size_t totalObjects = 0;
        size_t visibleObjects = 0;
        size_t culledObjects = 0;
    } cullStats;
    
    bool showOptimizationStats = false;
    
    // Callbacks
    std::function<void(const std::string&)> onModelLoad;
    std::function<void(const std::string&)> onSceneSave;
    std::function<void(const std::string&)> onSceneLoad;
};

// ===== Icons (using Unicode symbols) =====
namespace Icons {
    constexpr const char* Play = "\xE2\x96\xB6";      // ▶
    constexpr const char* Pause = "\xE2\x8F\xB8";    // ⏸
    constexpr const char* Stop = "\xE2\x96\xA0";     // ■
    constexpr const char* StepForward = "\xE2\x8F\xAD";  // ⏭
    constexpr const char* StepBack = "\xE2\x8F\xAE";     // ⏮
    constexpr const char* Folder = "\xF0\x9F\x93\x81";   // 📁
    constexpr const char* File = "\xF0\x9F\x93\x84";     // 📄
    constexpr const char* Model = "\xF0\x9F\x8E\xB2";    // 🎲
    constexpr const char* Image = "\xF0\x9F\x96\xBC";    // 🖼
    constexpr const char* Refresh = "\xE2\x86\xBB";      // ↻
    constexpr const char* Settings = "\xE2\x9A\x99";     // ⚙
    constexpr const char* Eye = "\xF0\x9F\x91\x81";      // 👁
    constexpr const char* EyeOff = "\xE2\x80\x95";       // ―
}

// ===== Main Menu Bar =====
inline void drawMainMenuBar(EditorState& state, Viewport& viewport, bool& shouldQuit) {
    if (ImGui::BeginMainMenuBar()) {
        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                // TODO: New scene
            }
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                // TODO: Open scene dialog
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                if (state.onSceneSave) state.onSceneSave("");
            }
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
                // TODO: Save as dialog
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Model...")) {
                // TODO: Import dialog
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Screenshot", "F12")) {
                state.screenshotPending = true;
            }
            if (ImGui::MenuItem("Screenshot Settings...")) {
                state.showScreenshotDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                shouldQuit = true;
            }
            ImGui::EndMenu();
        }
        
        // Edit Menu
        if (ImGui::BeginMenu("Edit")) {
            auto& history = getCommandHistory();
            
            std::string undoLabel = history.canUndo() 
                ? "Undo " + history.getUndoDescription() 
                : "Undo";
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, history.canUndo())) {
                history.undo();
            }
            
            std::string redoLabel = history.canRedo() 
                ? "Redo " + history.getRedoDescription() 
                : "Redo";
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Shift+Z", false, history.canRedo())) {
                history.redo();
            }
            
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Delete")) {
                // TODO: Delete selected
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                // TODO: Duplicate selected
            }
            ImGui::Separator();
            ImGui::MenuItem("History Panel", nullptr, &state.showHistory);
            ImGui::EndMenu();
        }
        
        // View Menu
        if (ImGui::BeginMenu("View")) {
            // Panels
            ImGui::MenuItem("Scene Hierarchy", nullptr, &state.showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
            ImGui::MenuItem("Asset Browser", nullptr, &state.showAssetBrowser);
            ImGui::MenuItem("Animation Timeline", nullptr, &state.showAnimationTimeline);
            ImGui::Separator();
            ImGui::MenuItem("Post Processing", nullptr, &state.showPostProcess);
            ImGui::MenuItem("Render Settings", nullptr, &state.showRenderSettings);
            ImGui::MenuItem("Lighting", nullptr, &state.showLighting);
            ImGui::Separator();
            ImGui::MenuItem("Console", nullptr, &state.showConsole);
            ImGui::MenuItem("Statistics", nullptr, &state.showStats);
            ImGui::MenuItem("Shader Status", nullptr, &state.showShaderStatus);
            ImGui::Separator();
            
            // Camera views
            if (ImGui::BeginMenu("Camera View")) {
                if (ImGui::MenuItem("Front", "Numpad 1")) viewport.viewFront();
                if (ImGui::MenuItem("Back", "Ctrl+Numpad 1")) viewport.viewBack();
                if (ImGui::MenuItem("Left", "Numpad 3")) viewport.viewLeft();
                if (ImGui::MenuItem("Right", "Ctrl+Numpad 3")) viewport.viewRight();
                if (ImGui::MenuItem("Top", "Numpad 7")) viewport.viewTop();
                if (ImGui::MenuItem("Bottom", "Ctrl+Numpad 7")) viewport.viewBottom();
                ImGui::Separator();
                if (ImGui::MenuItem("Perspective", "Numpad 0")) viewport.viewPerspective();
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Camera", "F")) viewport.camera.reset();
                ImGui::EndMenu();
            }
            
            // Camera bookmarks
            if (ImGui::BeginMenu("Camera Bookmarks")) {
                if (ImGui::MenuItem("Save Current View...")) {
                    // Open save dialog - for now just save with auto-name
                    static int bookmarkNum = 1;
                    viewport.savePreset("Bookmark " + std::to_string(bookmarkNum++));
                }
                ImGui::Separator();
                auto& presets = viewport.getSavedPresets();
                if (presets.empty()) {
                    ImGui::TextDisabled("No saved bookmarks");
                } else {
                    for (auto& [name, preset] : presets) {
                        if (ImGui::MenuItem(name.c_str())) {
                            viewport.loadPreset(name);
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Clear All Bookmarks")) {
                        // Can't modify while iterating, so just leave a note
                        // viewport.clearAllPresets(); // TODO
                    }
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            ImGui::MenuItem("Show Grid", "G", &viewport.settings.showGrid);
            ImGui::MenuItem("Wireframe", nullptr, &viewport.settings.wireframe);
            ImGui::MenuItem("Orthographic", nullptr, &viewport.settings.orthographic);
            ImGui::Separator();
            ImGui::MenuItem("Statistics", nullptr, &state.showStats);
            ImGui::MenuItem("Optimization Stats", nullptr, &state.showOptimizationStats);
            ImGui::Separator();
            ImGui::MenuItem("Help", "F1", &state.showHelp);
            ImGui::EndMenu();
        }
        
        // Window Menu
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Reset Layout")) {
                // TODO: Reset window positions
            }
            ImGui::EndMenu();
        }
        
        // Right-aligned items
        float rightOffset = ImGui::GetWindowWidth() - 200;
        ImGui::SameLine(rightOffset);
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "FPS: %.0f", ImGui::GetIO().Framerate);
        
        ImGui::EndMainMenuBar();
    }
}

// ===== Toolbar =====
inline void drawToolbar(EditorState& state, TransformGizmo& gizmo) {
    ImGui::SetNextWindowPos(ImVec2(0, 19), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 36), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings;
    
    if (ImGui::Begin("##Toolbar", nullptr, flags)) {
        // Transform tools
        bool isTranslate = state.gizmoMode == GizmoMode::Translate;
        bool isRotate = state.gizmoMode == GizmoMode::Rotate;
        bool isScale = state.gizmoMode == GizmoMode::Scale;
        
        if (isTranslate) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Move (W)", ImVec2(70, 26))) {
            state.gizmoMode = GizmoMode::Translate;
            gizmo.setMode(GizmoMode::Translate);
        }
        if (isTranslate) ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (isRotate) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Rotate (E)", ImVec2(70, 26))) {
            state.gizmoMode = GizmoMode::Rotate;
            gizmo.setMode(GizmoMode::Rotate);
        }
        if (isRotate) ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (isScale) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Scale (R)", ImVec2(70, 26))) {
            state.gizmoMode = GizmoMode::Scale;
            gizmo.setMode(GizmoMode::Scale);
        }
        if (isScale) ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Local/World space toggle
        if (ImGui::Button(state.gizmoLocalSpace ? "Local" : "World", ImVec2(60, 26))) {
            state.gizmoLocalSpace = !state.gizmoLocalSpace;
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Snap toggle
        ImGui::Checkbox("Snap", &state.snapEnabled);
        if (state.snapEnabled) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            if (state.gizmoMode == GizmoMode::Translate) {
                ImGui::DragFloat("##SnapVal", &state.snapTranslate, 0.1f, 0.1f, 10.0f, "%.1f");
            } else if (state.gizmoMode == GizmoMode::Rotate) {
                ImGui::DragFloat("##SnapVal", &state.snapRotate, 1.0f, 1.0f, 90.0f, "%.0f");
            } else {
                ImGui::DragFloat("##SnapVal", &state.snapScale, 0.01f, 0.01f, 1.0f, "%.2f");
            }
        }
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Play controls (for animation preview)
        if (ImGui::Button(state.animationPlaying ? "||" : ">", ImVec2(26, 26))) {
            state.animationPlaying = !state.animationPlaying;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play/Pause Animation");
        
        ImGui::SameLine();
        if (ImGui::Button("[]", ImVec2(26, 26))) {
            state.animationPlaying = false;
            state.animationTime = 0.0f;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop Animation");
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
}

// ===== Scene Hierarchy Panel =====
inline void drawHierarchyPanel(SceneGraph& scene, EditorState& state) {
    if (!state.showHierarchy) return;
    
    ImGui::SetNextWindowPos(ImVec2(0, 55), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Hierarchy", &state.showHierarchy)) {
        // Search bar
        static char searchBuf[128] = "";
        ImGui::SetNextItemWidth(-60);
        ImGui::InputTextWithHint("##Search", "Search...", searchBuf, sizeof(searchBuf));
        ImGui::SameLine();
        if (ImGui::Button("+", ImVec2(24, 0))) {
            ImGui::OpenPopup("AddEntityPopup");
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Entity");
        ImGui::SameLine();
        if (ImGui::Button("x", ImVec2(24, 0))) {
            searchBuf[0] = '\0';
        }
        
        // Add entity popup
        if (ImGui::BeginPopup("AddEntityPopup")) {
            if (ImGui::MenuItem("Empty Entity")) {
                scene.createEntity("New Entity");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cube")) {
                // TODO: Create primitive
            }
            if (ImGui::MenuItem("Sphere")) {}
            if (ImGui::MenuItem("Plane")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Point Light")) {}
            if (ImGui::MenuItem("Directional Light")) {}
            if (ImGui::MenuItem("Spot Light")) {}
            ImGui::EndPopup();
        }
        
        ImGui::Separator();
        
        // Entity tree
        std::string searchStr(searchBuf);
        std::function<void(Entity*)> drawNode = [&](Entity* entity) {
            if (!entity) return;
            
            // Filter by search
            bool matchesSearch = searchStr.empty() || 
                entity->name.find(searchStr) != std::string::npos;
            
            // Check if any child matches
            bool childMatches = false;
            for (Entity* child : entity->children) {
                if (child->name.find(searchStr) != std::string::npos) {
                    childMatches = true;
                    break;
                }
            }
            
            if (!matchesSearch && !childMatches && !searchStr.empty()) {
                return;
            }
            
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                       ImGuiTreeNodeFlags_SpanAvailWidth;
            if (entity->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
            if (scene.getSelectedEntity() == entity) flags |= ImGuiTreeNodeFlags_Selected;
            
            // Dim disabled entities
            if (!entity->enabled) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            }
            
            // Icon based on entity type
            const char* icon = entity->hasModel ? "\xE2\x97\x86" : "\xE2\x97\x8B";  // ◆ or ○
            
            std::string label = std::string(icon) + " " + entity->name;
            bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity->id, flags, "%s", label.c_str());
            
            if (!entity->enabled) ImGui::PopStyleColor();
            
            // Selection
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                scene.setSelectedEntity(entity);
            }
            
            // Drag & drop for reparenting
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(Entity*));
                ImGui::Text("Move: %s", entity->name.c_str());
                ImGui::EndDragDropSource();
            }
            
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                    Entity* dragged = *(Entity**)payload->Data;
                    if (dragged != entity) {
                        scene.setParent(dragged, entity);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Add Child")) {
                    Entity* child = scene.createEntity("New Child");
                    scene.setParent(child, entity);
                }
                if (ImGui::MenuItem("Duplicate")) {
                    // TODO: Duplicate entity
                }
                ImGui::Separator();
                if (ImGui::MenuItem(entity->enabled ? "Disable" : "Enable")) {
                    entity->enabled = !entity->enabled;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del")) {
                    scene.destroyEntity(entity);
                    ImGui::EndPopup();
                    if (nodeOpen) ImGui::TreePop();
                    return;
                }
                ImGui::EndPopup();
            }
            
            if (nodeOpen) {
                for (Entity* child : entity->children) {
                    drawNode(child);
                }
                ImGui::TreePop();
            }
        };
        
        for (Entity* root : scene.getRootEntities()) {
            drawNode(root);
        }
        
        // Simple separator for drop-to-root functionality
        ImGui::Separator();
        if (ImGui::Selectable("(Drop here to make root)", false)) {
            // Selection behavior placeholder
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                Entity* dragged = *(Entity**)payload->Data;
                scene.setParent(dragged, nullptr);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::End();
}

// ===== Inspector Panel =====
inline void drawInspectorPanel(SceneGraph& scene, EditorState& state) {
    if (!state.showInspector) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, 55), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Inspector", &state.showInspector)) {
        Entity* selected = scene.getSelectedEntity();
        
        if (!selected) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No entity selected");
            ImGui::End();
            return;
        }
        
        // Entity header
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
        
        // Enabled checkbox
        ImGui::Checkbox("##Enabled", &selected->enabled);
        ImGui::SameLine();
        
        // Name
        char nameBuf[128];
        strncpy(nameBuf, selected->name.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
            selected->name = nameBuf;
        }
        
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // Transform component
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            
            // Position
            float pos[3] = {selected->localTransform.position.x, 
                           selected->localTransform.position.y, 
                           selected->localTransform.position.z};
            ImGui::Text("Position");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat3("##Position", pos, 0.1f)) {
                selected->localTransform.position = {pos[0], pos[1], pos[2]};
                selected->updateWorldMatrix();
            }
            
            // Rotation
            Vec3 eulerDeg = selected->localTransform.getEulerDegrees();
            float rot[3] = {eulerDeg.x, eulerDeg.y, eulerDeg.z};
            ImGui::Text("Rotation");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat3("##Rotation", rot, 1.0f)) {
                selected->localTransform.setEulerDegrees({rot[0], rot[1], rot[2]});
                selected->updateWorldMatrix();
            }
            
            // Scale
            float scl[3] = {selected->localTransform.scale.x, 
                           selected->localTransform.scale.y, 
                           selected->localTransform.scale.z};
            ImGui::Text("Scale");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat3("##Scale", scl, 0.01f, 0.001f, 100.0f)) {
                selected->localTransform.scale = {scl[0], scl[1], scl[2]};
                selected->updateWorldMatrix();
            }
            
            if (ImGui::Button("Reset", ImVec2(-1, 0))) {
                selected->localTransform = Transform();
                selected->updateWorldMatrix();
            }
            
            ImGui::Unindent(10);
        }
        
        // Model component
        if (selected->hasModel) {
            if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10);
                ImGui::Text("Name: %s", selected->model.name.c_str());
                ImGui::Text("Meshes: %zu", selected->model.meshes.size());
                ImGui::Text("Vertices: %zu", selected->model.totalVerts);
                ImGui::Text("Triangles: %zu", selected->model.totalTris);
                ImGui::Text("Textures: %d", selected->model.textureCount);
                ImGui::Unindent(10);
            }
        }
        
        // Material component
        if (selected->hasModel) {
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10);
                
                // Ensure entity has a material
                if (!selected->material) {
                    selected->material = std::make_shared<Material>();
                }
                Material& mat = *selected->material;
                
                // Material name
                char matNameBuf[128];
                strncpy(matNameBuf, mat.name.c_str(), sizeof(matNameBuf) - 1);
                matNameBuf[sizeof(matNameBuf) - 1] = '\0';
                ImGui::Text("Name");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##MatName", matNameBuf, sizeof(matNameBuf))) {
                    mat.name = matNameBuf;
                }
                
                ImGui::Spacing();
                
                // Base Color with color picker
                ImGui::Text("Base Color");
                float baseColor[4] = {mat.baseColor.x, mat.baseColor.y, mat.baseColor.z, mat.alpha};
                ImGui::SetNextItemWidth(-1);
                if (ImGui::ColorEdit4("##BaseColor", baseColor, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
                    mat.baseColor = {baseColor[0], baseColor[1], baseColor[2]};
                    mat.alpha = baseColor[3];
                }
                
                ImGui::Spacing();
                
                // Metallic
                ImGui::Text("Metallic");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##Metallic", &mat.metallic, 0.0f, 1.0f, "%.2f");
                
                // Roughness
                ImGui::Text("Roughness");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##Roughness", &mat.roughness, 0.0f, 1.0f, "%.2f");
                
                // Ambient Occlusion
                ImGui::Text("AO Strength");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##AO", &mat.ao, 0.0f, 1.0f, "%.2f");
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Emissive
                if (ImGui::TreeNode("Emissive")) {
                    float emissive[3] = {mat.emissiveColor.x, mat.emissiveColor.y, mat.emissiveColor.z};
                    ImGui::Text("Color");
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::ColorEdit3("##EmissiveColor", emissive, ImGuiColorEditFlags_NoInputs)) {
                        mat.emissiveColor = {emissive[0], emissive[1], emissive[2]};
                    }
                    ImGui::Text("Intensity");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##EmissiveIntensity", &mat.emissiveIntensity, 0.0f, 20.0f, "%.1f");
                    ImGui::TreePop();
                }
                
                // Advanced properties
                if (ImGui::TreeNode("Advanced")) {
                    ImGui::Text("Normal Strength");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##NormalStrength", &mat.normalStrength, 0.0f, 2.0f, "%.2f");
                    
                    ImGui::Text("IOR");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##IOR", &mat.ior, 1.0f, 3.0f, "%.2f");
                    
                    ImGui::Checkbox("Two Sided", &mat.twoSided);
                    ImGui::Checkbox("Alpha Blend", &mat.alphaBlend);
                    
                    if (mat.alphaBlend || mat.alpha < 1.0f) {
                        ImGui::Checkbox("Alpha Cutoff", &mat.alphaCutoff);
                        if (mat.alphaCutoff) {
                            ImGui::SetNextItemWidth(-1);
                            ImGui::SliderFloat("##AlphaCutoff", &mat.alphaCutoffValue, 0.0f, 1.0f, "%.2f");
                        }
                    }
                    ImGui::TreePop();
                }
                
                // Texture slots
                if (ImGui::TreeNode("Textures")) {
                    for (int i = 0; i < (int)TEXTURE_SLOT_COUNT; i++) {
                        TextureSlot slot = static_cast<TextureSlot>(i);
                        const char* slotName = Material::getSlotName(slot);
                        
                        ImGui::PushID(i);
                        
                        // Show texture status
                        bool hasTexture = mat.hasTexture(slot);
                        ImGui::Text("%s:", slotName);
                        ImGui::SameLine(120);
                        
                        if (hasTexture) {
                            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "[Loaded]");
                        } else {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[None]");
                        }
                        
                        // Texture path display
                        if (!mat.texturePaths[i].empty()) {
                            ImGui::TextWrapped("  %s", mat.texturePaths[i].c_str());
                        }
                        
                        // Drop target for texture drag & drop
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                                const char* path = static_cast<const char*>(payload->Data);
                                mat.texturePaths[i] = path;
                                // TODO: Actually load texture
                            }
                            ImGui::EndDragDropTarget();
                        }
                        
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Material presets dropdown
                if (ImGui::Button("Apply Preset", ImVec2(-1, 0))) {
                    ImGui::OpenPopup("MaterialPresets");
                }
                
                if (ImGui::BeginPopup("MaterialPresets")) {
                    if (ImGui::MenuItem("Default")) {
                        *selected->material = Material::createDefault();
                        selected->material->name = "Default";
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Gold")) {
                        *selected->material = Material::createGold();
                    }
                    if (ImGui::MenuItem("Silver")) {
                        *selected->material = Material::createSilver();
                    }
                    if (ImGui::MenuItem("Copper")) {
                        *selected->material = Material::createCopper();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Plastic (Red)")) {
                        *selected->material = Material::createPlastic();
                    }
                    if (ImGui::MenuItem("Rubber")) {
                        *selected->material = Material::createRubber();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Glass")) {
                        *selected->material = Material::createGlass();
                    }
                    if (ImGui::MenuItem("Emissive")) {
                        *selected->material = Material::createEmissive();
                    }
                    ImGui::EndPopup();
                }
                
                ImGui::Unindent(10);
            }
        }
        
        // Light component
        if (selected->hasLight) {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10);
                Light& light = selected->light;
                
                // Light type (read-only display)
                ImGui::Text("Type: %s", Light::getTypeName(light.type));
                
                // Enabled
                ImGui::Checkbox("Enabled##Light", &light.enabled);
                
                // Color
                float col[3] = {light.color.x, light.color.y, light.color.z};
                if (ImGui::ColorEdit3("Color##LightCol", col)) {
                    light.color = {col[0], col[1], col[2]};
                }
                
                // Intensity
                ImGui::SliderFloat("Intensity##LightInt", &light.intensity, 0.0f, 10.0f);
                
                // Type-specific properties
                if (light.type == LightType::Directional) {
                    float dir[3] = {light.direction.x, light.direction.y, light.direction.z};
                    if (ImGui::DragFloat3("Direction##LightDir", dir, 0.01f, -1.0f, 1.0f)) {
                        float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
                        if (len > 0.001f) {
                            light.direction = {dir[0]/len, dir[1]/len, dir[2]/len};
                        }
                    }
                }
                
                if (light.type == LightType::Point || light.type == LightType::Spot) {
                    // Position comes from entity transform
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Position from Transform)");
                    ImGui::SliderFloat("Range##LightRange", &light.range, 0.1f, 100.0f);
                }
                
                if (light.type == LightType::Spot) {
                    float dir[3] = {light.direction.x, light.direction.y, light.direction.z};
                    if (ImGui::DragFloat3("Direction##LightDir", dir, 0.01f, -1.0f, 1.0f)) {
                        float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
                        if (len > 0.001f) {
                            light.direction = {dir[0]/len, dir[1]/len, dir[2]/len};
                        }
                    }
                    ImGui::SliderFloat("Inner Angle##LightInner", &light.innerConeAngle, 1.0f, 89.0f);
                    ImGui::SliderFloat("Outer Angle##LightOuter", &light.outerConeAngle, 1.0f, 90.0f);
                    if (light.innerConeAngle > light.outerConeAngle) {
                        light.innerConeAngle = light.outerConeAngle;
                    }
                }
                
                // Shadow settings
                if (ImGui::TreeNode("Shadows##LightShadows")) {
                    ImGui::Checkbox("Cast Shadows##LightCast", &light.castShadows);
                    if (light.castShadows) {
                        ImGui::SliderFloat("Bias##LightBias", &light.shadowBias, 0.0f, 0.05f, "%.4f");
                        ImGui::SliderFloat("Softness##LightSoft", &light.shadowSoftness, 0.0f, 5.0f);
                    }
                    ImGui::TreePop();
                }
                
                // Remove light component button
                ImGui::Spacing();
                if (ImGui::Button("Remove Light Component", ImVec2(-1, 0))) {
                    selected->hasLight = false;
                }
                
                ImGui::Unindent(10);
            }
        }
        
        // Add component button
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Add Component", ImVec2(-1, 28))) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        
        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (!selected->hasLight) {
                if (ImGui::BeginMenu("Light")) {
                    if (ImGui::MenuItem("Point Light")) {
                        selected->hasLight = true;
                        selected->light = Light::createPoint();
                        selected->light.position = selected->localTransform.position;
                    }
                    if (ImGui::MenuItem("Spot Light")) {
                        selected->hasLight = true;
                        selected->light = Light::createSpot();
                        selected->light.position = selected->localTransform.position;
                    }
                    if (ImGui::MenuItem("Directional Light")) {
                        selected->hasLight = true;
                        selected->light = Light::createDirectional();
                    }
                    ImGui::EndMenu();
                }
            }
            if (ImGui::MenuItem("Animator")) {
                // TODO: Add animator component
            }
            if (ImGui::MenuItem("Audio Source")) {}
            if (ImGui::MenuItem("Collider")) {}
            if (ImGui::MenuItem("Script")) {}
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

// ===== Post-Processing Panel =====
inline void drawPostProcessPanel(PostProcessSettings& settings, EditorState& state) {
    if (!state.showPostProcess) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Post Processing", &state.showPostProcess)) {
        // Bloom
        if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##Bloom", &settings.bloom.enabled);
            if (settings.bloom.enabled) {
                ImGui::SliderFloat("Threshold", &settings.bloom.threshold, 0.0f, 5.0f);
                ImGui::SliderFloat("Intensity##Bloom", &settings.bloom.intensity, 0.0f, 3.0f);
                ImGui::SliderFloat("Radius", &settings.bloom.radius, 1.0f, 10.0f);
                ImGui::SliderInt("Iterations", &settings.bloom.iterations, 1, 10);
            }
            ImGui::Unindent(10);
        }
        
        // Tone Mapping
        if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##TM", &settings.toneMapping.enabled);
            if (settings.toneMapping.enabled) {
                const char* modes[] = {"None", "Reinhard", "ACES", "Filmic", "Uncharted 2"};
                int mode = (int)settings.toneMapping.mode;
                if (ImGui::Combo("Mode", &mode, modes, 5)) {
                    settings.toneMapping.mode = (ToneMappingSettings::Mode)mode;
                }
                ImGui::SliderFloat("Exposure", &settings.toneMapping.exposure, 0.1f, 5.0f);
                ImGui::SliderFloat("Gamma", &settings.toneMapping.gamma, 1.0f, 3.0f);
            }
            ImGui::Unindent(10);
        }
        
        // Color Grading
        if (ImGui::CollapsingHeader("Color Grading")) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##CG", &settings.colorGrading.enabled);
            if (settings.colorGrading.enabled) {
                ImGui::SliderFloat("Saturation", &settings.toneMapping.saturation, 0.0f, 2.0f);
                ImGui::SliderFloat("Contrast", &settings.toneMapping.contrast, 0.5f, 2.0f);
                ImGui::SliderFloat("Temperature", &settings.colorGrading.temperature, -1.0f, 1.0f);
                ImGui::SliderFloat("Tint", &settings.colorGrading.tint, -1.0f, 1.0f);
                
                if (ImGui::TreeNode("Lift / Gamma / Gain")) {
                    ImGui::ColorEdit3("Lift", settings.colorGrading.lift);
                    ImGui::DragFloat3("Gamma", settings.colorGrading.gamma_adj, 0.01f, 0.5f, 2.0f);
                    ImGui::DragFloat3("Gain", settings.colorGrading.gain, 0.01f, 0.0f, 2.0f);
                    ImGui::TreePop();
                }
            }
            ImGui::Unindent(10);
        }
        
        // Vignette
        if (ImGui::CollapsingHeader("Vignette")) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##Vignette", &settings.vignette.enabled);
            if (settings.vignette.enabled) {
                ImGui::SliderFloat("Intensity##Vig", &settings.vignette.intensity, 0.0f, 1.0f);
                ImGui::SliderFloat("Smoothness", &settings.vignette.smoothness, 0.0f, 1.0f);
                ImGui::SliderFloat("Roundness", &settings.vignette.roundness, 0.0f, 1.0f);
            }
            ImGui::Unindent(10);
        }
        
        // Chromatic Aberration
        if (ImGui::CollapsingHeader("Chromatic Aberration")) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##CA", &settings.chromaticAberration.enabled);
            if (settings.chromaticAberration.enabled) {
                ImGui::SliderFloat("Intensity##CA", &settings.chromaticAberration.intensity, 0.0f, 0.1f);
            }
            ImGui::Unindent(10);
        }
        
        // Film Grain
        if (ImGui::CollapsingHeader("Film Grain")) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##Grain", &settings.filmGrain.enabled);
            if (settings.filmGrain.enabled) {
                ImGui::SliderFloat("Intensity##Grain", &settings.filmGrain.intensity, 0.0f, 0.5f);
                ImGui::SliderFloat("Response", &settings.filmGrain.response, 0.0f, 1.0f);
            }
            ImGui::Unindent(10);
        }
        
        // FXAA
        if (ImGui::CollapsingHeader("Anti-Aliasing")) {
            ImGui::Indent(10);
            ImGui::Checkbox("FXAA", &settings.fxaa.enabled);
            if (settings.fxaa.enabled) {
                ImGui::SliderFloat("Subpixel", &settings.fxaa.subpixelBlending, 0.0f, 1.0f);
            }
            ImGui::Unindent(10);
        }
        
        ImGui::Spacing();
        if (ImGui::Button("Reset All", ImVec2(-1, 0))) {
            settings = PostProcessSettings();
        }
    }
    ImGui::End();
}

// ===== Render Settings Panel =====
struct RenderSettings {
    // Shadows
    bool shadowsEnabled = true;
    int shadowMapSize = 2048;
    float shadowBias = 0.005f;
    float shadowNormalBias = 0.02f;
    int pcfSamples = 3;
    
    // IBL
    bool iblEnabled = true;
    float iblIntensity = 1.0f;
    float iblRotation = 0.0f;
    
    // Debug
    bool showWireframe = false;
    bool showNormals = false;
    bool showBoundingBoxes = false;
};

inline void drawRenderSettingsPanel(RenderSettings& settings, EditorState& state) {
    if (!state.showRenderSettings) return;
    
    ImGui::SetNextWindowPos(ImVec2(10, 360), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Render Settings", &state.showRenderSettings)) {
        // Shadows
        if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##Shadow", &settings.shadowsEnabled);
            if (settings.shadowsEnabled) {
                const char* sizes[] = {"512", "1024", "2048", "4096"};
                int sizeIdx = 0;
                if (settings.shadowMapSize == 512) sizeIdx = 0;
                else if (settings.shadowMapSize == 1024) sizeIdx = 1;
                else if (settings.shadowMapSize == 2048) sizeIdx = 2;
                else sizeIdx = 3;
                
                if (ImGui::Combo("Resolution", &sizeIdx, sizes, 4)) {
                    const int values[] = {512, 1024, 2048, 4096};
                    settings.shadowMapSize = values[sizeIdx];
                }
                
                ImGui::SliderFloat("Bias", &settings.shadowBias, 0.0f, 0.01f, "%.4f");
                ImGui::SliderFloat("Normal Bias", &settings.shadowNormalBias, 0.0f, 0.1f);
                ImGui::SliderInt("PCF Samples", &settings.pcfSamples, 1, 5);
            }
            ImGui::Unindent(10);
        }
        
        // IBL
        if (ImGui::CollapsingHeader("Environment Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            ImGui::Checkbox("Enabled##IBL", &settings.iblEnabled);
            if (settings.iblEnabled) {
                ImGui::SliderFloat("Intensity##IBL", &settings.iblIntensity, 0.0f, 2.0f);
                ImGui::SliderFloat("Rotation##IBL", &settings.iblRotation, 0.0f, 360.0f, "%.0f deg");
                
                if (ImGui::Button("Load HDR...", ImVec2(-1, 0))) {
                    // TODO: HDR file dialog
                }
            }
            ImGui::Unindent(10);
        }
        
        // Debug visualization
        if (ImGui::CollapsingHeader("Debug")) {
            ImGui::Indent(10);
            ImGui::Checkbox("Wireframe", &settings.showWireframe);
            ImGui::Checkbox("Show Normals", &settings.showNormals);
            ImGui::Checkbox("Bounding Boxes", &settings.showBoundingBoxes);
            ImGui::Unindent(10);
        }
    }
    ImGui::End();
}

// ===== Lighting Panel =====
// Legacy settings struct for compatibility
struct LightSettings {
    // Directional light (maps to primary light in LightManager)
    float direction[3] = {0.5f, -1.0f, 0.3f};
    float color[3] = {1.0f, 0.98f, 0.95f};
    float intensity = 1.0f;
    
    // Ambient
    float ambientColor[3] = {0.1f, 0.1f, 0.15f};
    float ambientIntensity = 0.3f;
    
    // Sync with LightManager
    void syncFromManager() {
        auto& mgr = getLightManager();
        auto* primary = mgr.getPrimaryDirectional();
        if (primary) {
            direction[0] = primary->direction.x;
            direction[1] = primary->direction.y;
            direction[2] = primary->direction.z;
            color[0] = primary->color.x;
            color[1] = primary->color.y;
            color[2] = primary->color.z;
            intensity = primary->intensity;
        }
        auto& ambient = mgr.getAmbient();
        ambientColor[0] = ambient.color.x;
        ambientColor[1] = ambient.color.y;
        ambientColor[2] = ambient.color.z;
        ambientIntensity = ambient.intensity;
    }
    
    void syncToManager() {
        auto& mgr = getLightManager();
        auto* primary = mgr.getPrimaryDirectional();
        if (primary) {
            primary->direction = {direction[0], direction[1], direction[2]};
            primary->color = {color[0], color[1], color[2]};
            primary->intensity = intensity;
        }
        auto& ambient = mgr.getAmbient();
        ambient.color = {ambientColor[0], ambientColor[1], ambientColor[2]};
        ambient.intensity = ambientIntensity;
    }
};

inline void drawLightingPanel(LightSettings& settings, EditorState& state) {
    if (!state.showLighting) return;
    
    ImGui::SetNextWindowPos(ImVec2(10, 660), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Lighting", &state.showLighting)) {
        auto& mgr = getLightManager();
        
        // Add light buttons
        if (ImGui::Button("+ Directional")) {
            mgr.addLight(LightType::Directional);
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Point")) {
            mgr.addLight(LightType::Point);
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Spot")) {
            mgr.addLight(LightType::Spot);
        }
        
        ImGui::Separator();
        
        // Light count
        ImGui::Text("Lights: %zu / %zu", mgr.getEnabledLightCount(), LightManager::MAX_LIGHTS);
        
        ImGui::Separator();
        
        // Light list
        static int selectedLightId = -1;
        uint32_t lightToRemove = 0;
        
        for (auto& lightPtr : mgr.getLights()) {
            Light& light = *lightPtr;
            ImGui::PushID(light.id);
            
            // Light header with enable checkbox
            bool expanded = ImGui::CollapsingHeader(
                (light.name + " (" + Light::getTypeName(light.type) + ")").c_str(),
                ImGuiTreeNodeFlags_AllowOverlap);
            
            // Enable checkbox on the same line
            ImGui::SameLine(ImGui::GetWindowWidth() - 60);
            ImGui::Checkbox("##Enable", &light.enabled);
            
            // Delete button
            ImGui::SameLine(ImGui::GetWindowWidth() - 30);
            if (ImGui::SmallButton("X")) {
                lightToRemove = light.id;
            }
            
            if (expanded) {
                ImGui::Indent(10);
                
                // Name
                char nameBuf[64];
                strncpy(nameBuf, light.name.c_str(), sizeof(nameBuf) - 1);
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                    light.name = nameBuf;
                }
                
                // Color
                float col[3] = {light.color.x, light.color.y, light.color.z};
                if (ImGui::ColorEdit3("Color", col)) {
                    light.color = {col[0], col[1], col[2]};
                }
                
                // Intensity
                ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
                
                // Type-specific properties
                if (light.type == LightType::Directional) {
                    float dir[3] = {light.direction.x, light.direction.y, light.direction.z};
                    if (ImGui::DragFloat3("Direction", dir, 0.01f, -1.0f, 1.0f)) {
                        // Normalize
                        float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
                        if (len > 0.001f) {
                            light.direction = {dir[0]/len, dir[1]/len, dir[2]/len};
                        }
                    }
                }
                
                if (light.type == LightType::Point || light.type == LightType::Spot) {
                    float pos[3] = {light.position.x, light.position.y, light.position.z};
                    if (ImGui::DragFloat3("Position", pos, 0.1f)) {
                        light.position = {pos[0], pos[1], pos[2]};
                    }
                    ImGui::SliderFloat("Range", &light.range, 0.1f, 100.0f);
                }
                
                if (light.type == LightType::Spot) {
                    float dir[3] = {light.direction.x, light.direction.y, light.direction.z};
                    if (ImGui::DragFloat3("Direction", dir, 0.01f, -1.0f, 1.0f)) {
                        float len = sqrtf(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
                        if (len > 0.001f) {
                            light.direction = {dir[0]/len, dir[1]/len, dir[2]/len};
                        }
                    }
                    ImGui::SliderFloat("Inner Angle", &light.innerConeAngle, 1.0f, 89.0f);
                    ImGui::SliderFloat("Outer Angle", &light.outerConeAngle, 1.0f, 90.0f);
                    if (light.innerConeAngle > light.outerConeAngle) {
                        light.innerConeAngle = light.outerConeAngle;
                    }
                }
                
                // Shadow settings
                if (ImGui::TreeNode("Shadows")) {
                    ImGui::Checkbox("Cast Shadows", &light.castShadows);
                    if (light.castShadows) {
                        ImGui::SliderFloat("Bias", &light.shadowBias, 0.0f, 0.05f, "%.4f");
                        ImGui::SliderFloat("Normal Bias", &light.shadowNormalBias, 0.0f, 0.1f, "%.3f");
                        ImGui::SliderFloat("Softness", &light.shadowSoftness, 0.0f, 5.0f);
                        
                        const char* sizes[] = {"256", "512", "1024", "2048", "4096"};
                        int sizeIdx = 2;
                        if (light.shadowMapSize == 256) sizeIdx = 0;
                        else if (light.shadowMapSize == 512) sizeIdx = 1;
                        else if (light.shadowMapSize == 1024) sizeIdx = 2;
                        else if (light.shadowMapSize == 2048) sizeIdx = 3;
                        else if (light.shadowMapSize == 4096) sizeIdx = 4;
                        
                        if (ImGui::Combo("Shadow Map Size", &sizeIdx, sizes, 5)) {
                            int sizeLookup[] = {256, 512, 1024, 2048, 4096};
                            light.shadowMapSize = sizeLookup[sizeIdx];
                        }
                    }
                    ImGui::TreePop();
                }
                
                ImGui::Unindent(10);
            }
            
            ImGui::PopID();
        }
        
        // Remove light if requested
        if (lightToRemove > 0) {
            mgr.removeLight(lightToRemove);
        }
        
        ImGui::Separator();
        
        // Ambient light
        if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            auto& ambient = mgr.getAmbient();
            float ambCol[3] = {ambient.color.x, ambient.color.y, ambient.color.z};
            if (ImGui::ColorEdit3("Color##Ambient", ambCol)) {
                ambient.color = {ambCol[0], ambCol[1], ambCol[2]};
            }
            ImGui::SliderFloat("Intensity##Ambient", &ambient.intensity, 0.0f, 1.0f);
            
            ImGui::Checkbox("Use IBL", &ambient.useIBL);
            if (ambient.useIBL) {
                ImGui::SliderFloat("IBL Intensity", &ambient.iblIntensity, 0.0f, 5.0f);
            }
            ImGui::Unindent(10);
        }
        
        // Sync legacy settings
        settings.syncFromManager();
    }
    ImGui::End();
}

// ===== Animation Timeline =====
struct AnimationState {
    std::vector<std::string> clips;
    std::string currentClip;
    float time = 0.0f;
    float duration = 1.0f;
    bool playing = false;
    bool loop = true;
    float speed = 1.0f;
};

inline void drawAnimationTimeline(AnimationState& anim, EditorState& state) {
    if (!state.showAnimationTimeline) return;
    
    float height = 180;
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - height - 24), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, height), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Animation", &state.showAnimationTimeline)) {
        // Clip selector
        if (ImGui::BeginCombo("Clip", anim.currentClip.empty() ? "None" : anim.currentClip.c_str())) {
            for (const auto& clip : anim.clips) {
                bool selected = (clip == anim.currentClip);
                if (ImGui::Selectable(clip.c_str(), selected)) {
                    anim.currentClip = clip;
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        ImGui::Checkbox("Loop", &anim.loop);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderFloat("Speed", &anim.speed, 0.1f, 2.0f);
        
        ImGui::Spacing();
        
        // Transport controls
        float buttonSize = 30;
        float totalWidth = buttonSize * 5 + 20;
        float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        
        ImGui::SetCursorPosX(startX);
        if (ImGui::Button("|<", ImVec2(buttonSize, buttonSize))) {
            anim.time = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("<", ImVec2(buttonSize, buttonSize))) {
            anim.time = std::max(0.0f, anim.time - 1.0f / 30.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button(anim.playing ? "||" : ">", ImVec2(buttonSize, buttonSize))) {
            anim.playing = !anim.playing;
        }
        ImGui::SameLine();
        if (ImGui::Button(">", ImVec2(buttonSize, buttonSize))) {
            anim.time = std::min(anim.duration, anim.time + 1.0f / 30.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button(">|", ImVec2(buttonSize, buttonSize))) {
            anim.time = anim.duration;
        }
        
        ImGui::Spacing();
        
        // Timeline scrubber
        ImGui::Text("Time: %.2f / %.2f", anim.time, anim.duration);
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##Timeline", &anim.time, 0.0f, anim.duration, "");
        
        // Draw timeline visualization
        ImVec2 timelinePos = ImGui::GetCursorScreenPos();
        ImVec2 timelineSize(ImGui::GetContentRegionAvail().x, 40);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Background
        drawList->AddRectFilled(timelinePos, 
            ImVec2(timelinePos.x + timelineSize.x, timelinePos.y + timelineSize.y),
            IM_COL32(30, 30, 35, 255));
        
        // Grid lines (every second)
        for (float t = 0; t <= anim.duration; t += 1.0f) {
            float x = timelinePos.x + (t / anim.duration) * timelineSize.x;
            drawList->AddLine(ImVec2(x, timelinePos.y), ImVec2(x, timelinePos.y + timelineSize.y),
                             IM_COL32(60, 60, 70, 255));
        }
        
        // Playhead
        float playheadX = timelinePos.x + (anim.time / anim.duration) * timelineSize.x;
        drawList->AddLine(ImVec2(playheadX, timelinePos.y), 
                         ImVec2(playheadX, timelinePos.y + timelineSize.y),
                         IM_COL32(255, 80, 80, 255), 2.0f);
        
        // Playhead handle
        drawList->AddTriangleFilled(
            ImVec2(playheadX - 6, timelinePos.y),
            ImVec2(playheadX + 6, timelinePos.y),
            ImVec2(playheadX, timelinePos.y + 8),
            IM_COL32(255, 80, 80, 255));
        
        ImGui::Dummy(timelineSize);
    }
    ImGui::End();
}

// ===== Asset Browser =====
inline void drawAssetBrowser(EditorState& state) {
    if (!state.showAssetBrowser) return;
    
    ImGui::SetNextWindowPos(ImVec2(280, ImGui::GetIO().DisplaySize.y - 250), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Assets", &state.showAssetBrowser)) {
        // Path bar
        ImGui::Text("Path: %s", state.currentAssetPath.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        if (ImGui::Button("Refresh")) {
            // Refresh directory
        }
        
        ImGui::Separator();
        
        // File list
        ImGui::BeginChild("FileList", ImVec2(0, 0), false);
        
        // Parent directory
        if (state.currentAssetPath != ".") {
            if (ImGui::Selectable(".. (Parent)", false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    std::filesystem::path p(state.currentAssetPath);
                    state.currentAssetPath = p.parent_path().string();
                }
            }
        }
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(state.currentAssetPath)) {
                std::string name = entry.path().filename().string();
                bool isDir = entry.is_directory();
                
                // Skip hidden files
                if (name[0] == '.') continue;
                
                // Icon based on type
                std::string icon;
                if (isDir) {
                    icon = "[D] ";
                } else {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
                        icon = "[M] ";
                    } else if (ext == ".png" || ext == ".jpg" || ext == ".hdr") {
                        icon = "[T] ";
                    } else if (ext == ".hlsl" || ext == ".metal") {
                        icon = "[S] ";
                    } else {
                        icon = "[?] ";
                    }
                }
                
                bool selected = (state.selectedAsset == entry.path().string());
                if (ImGui::Selectable((icon + name).c_str(), selected, 
                                      ImGuiSelectableFlags_AllowDoubleClick)) {
                    state.selectedAsset = entry.path().string();
                    
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (isDir) {
                            state.currentAssetPath = entry.path().string();
                        } else if (state.onModelLoad) {
                            state.onModelLoad(entry.path().string());
                        }
                    }
                }
                
                // Drag source for models
                if (!isDir && ImGui::BeginDragDropSource()) {
                    std::string path = entry.path().string();
                    ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.size() + 1);
                    ImGui::Text("Load: %s", name.c_str());
                    ImGui::EndDragDropSource();
                }
            }
        } catch (...) {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Cannot read directory");
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

// ===== Console =====
inline void drawConsole(EditorState& state) {
    if (!state.showConsole) return;
    
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 180), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Console", &state.showConsole)) {
        if (ImGui::Button("Clear")) {
            state.consoleLogs.clear();
        }
        ImGui::SameLine();
        static bool autoScroll = true;
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        
        ImGui::Separator();
        
        ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& log : state.consoleLogs) {
            ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);
            if (log.find("[ERROR]") != std::string::npos) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            else if (log.find("[WARN]") != std::string::npos) color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
            else if (log.find("[INFO]") != std::string::npos) color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
            
            ImGui::TextColored(color, "%s", log.c_str());
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

// ===== History Panel (Undo/Redo) =====
inline void drawHistoryPanel(EditorState& state) {
    if (!state.showHistory) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 220, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("History", &state.showHistory)) {
        auto& history = getCommandHistory();
        
        // Undo/Redo buttons
        ImGui::BeginDisabled(!history.canUndo());
        if (ImGui::Button("Undo")) {
            history.undo();
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        
        ImGui::BeginDisabled(!history.canRedo());
        if (ImGui::Button("Redo")) {
            history.redo();
        }
        ImGui::EndDisabled();
        
        ImGui::SameLine();
        
        if (ImGui::Button("Clear")) {
            history.clear();
        }
        
        ImGui::Separator();
        
        // History stats
        ImGui::Text("Undo: %zu | Redo: %zu", history.undoCount(), history.redoCount());
        
        if (history.isDirty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "(Modified)");
        }
        
        ImGui::Separator();
        
        // History list
        ImGui::BeginChild("HistoryList", ImVec2(0, 0), true);
        
        auto undoHistory = history.getUndoHistory();
        
        // Current state marker
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "> Current State");
        
        // Undo history (most recent first)
        for (size_t i = 0; i < undoHistory.size(); i++) {
            ImGui::PushID((int)i);
            
            // Alternating colors
            ImVec4 color = (i == 0) 
                ? ImVec4(0.9f, 0.9f, 0.9f, 1.0f) 
                : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
            
            ImGui::TextColored(color, "  %s", undoHistory[i].c_str());
            
            // Click to undo to this point
            if (ImGui::IsItemClicked()) {
                // Undo multiple times to reach this state
                for (size_t j = 0; j <= i; j++) {
                    history.undo();
                }
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to undo to this point");
            }
            
            ImGui::PopID();
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

// ===== Screenshot Settings Dialog =====
inline void drawScreenshotDialog(EditorState& state) {
    if (!state.showScreenshotDialog) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 200, 
                                    ImGui::GetIO().DisplaySize.y / 2 - 200), 
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Screenshot Settings", &state.showScreenshotDialog)) {
        auto& settings = state.screenshotSettings;
        
        // Format selection
        ImGui::Text("Format");
        int formatIdx = (settings.format == ScreenshotSettings::Format::PNG) ? 0 : 1;
        if (ImGui::RadioButton("PNG", formatIdx == 0)) {
            settings.format = ScreenshotSettings::Format::PNG;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("JPG", formatIdx == 1)) {
            settings.format = ScreenshotSettings::Format::JPG;
        }
        
        // JPG quality
        if (settings.format == ScreenshotSettings::Format::JPG) {
            ImGui::SliderInt("Quality", &settings.jpgQuality, 1, 100, "%d%%");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Resolution
        ImGui::Text("Resolution");
        
        // Presets
        if (ImGui::Button("Viewport")) {
            settings.width = 0;
            settings.height = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("HD")) {
            settings.width = 1280;
            settings.height = 720;
        }
        ImGui::SameLine();
        if (ImGui::Button("Full HD")) {
            settings.width = 1920;
            settings.height = 1080;
        }
        ImGui::SameLine();
        if (ImGui::Button("4K")) {
            settings.width = 3840;
            settings.height = 2160;
        }
        
        if (ImGui::Button("1K Square")) {
            settings.width = 1024;
            settings.height = 1024;
        }
        ImGui::SameLine();
        if (ImGui::Button("2K Square")) {
            settings.width = 2048;
            settings.height = 2048;
        }
        ImGui::SameLine();
        if (ImGui::Button("4K Square")) {
            settings.width = 4096;
            settings.height = 4096;
        }
        
        // Custom resolution
        ImGui::Spacing();
        int customWidth = (int)settings.width;
        int customHeight = (int)settings.height;
        
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Width", &customWidth, 0, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Height", &customHeight, 0, 0);
        
        if (customWidth > 0) settings.width = (uint32_t)customWidth;
        if (customHeight > 0) settings.height = (uint32_t)customHeight;
        
        if (settings.width == 0 || settings.height == 0) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Using viewport size)");
        }
        
        ImGui::Checkbox("Maintain Aspect Ratio", &settings.maintainAspectRatio);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Options
        ImGui::Text("Options");
        ImGui::Checkbox("Transparent Background", &settings.transparentBackground);
        if (settings.transparentBackground && settings.format == ScreenshotSettings::Format::JPG) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), 
                              "Note: JPG does not support transparency");
        }
        
        ImGui::Checkbox("Include UI", &settings.includeUI);
        
        // Supersampling
        ImGui::Text("Supersampling");
        if (ImGui::RadioButton("Off", settings.supersampling == 1)) settings.supersampling = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton("2x", settings.supersampling == 2)) settings.supersampling = 2;
        ImGui::SameLine();
        if (ImGui::RadioButton("4x", settings.supersampling == 4)) settings.supersampling = 4;
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Output path
        ImGui::Text("Output Path");
        char pathBuf[512];
        strncpy(pathBuf, settings.outputPath.c_str(), sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(-80);
        if (ImGui::InputText("##OutputPath", pathBuf, sizeof(pathBuf))) {
            settings.outputPath = pathBuf;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            // TODO: File dialog
        }
        
        ImGui::Checkbox("Auto-increment filenames", &settings.autoIncrement);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Take screenshot button
        if (ImGui::Button("Take Screenshot (F12)", ImVec2(-1, 35))) {
            state.screenshotPending = true;
        }
        
        // Show last screenshot path
        if (!state.lastScreenshotPath.empty()) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), 
                              "Last: %s", state.lastScreenshotPath.c_str());
        }
    }
    ImGui::End();
}

// ===== Statistics Panel =====
inline void drawStatsPanel(EditorState& state) {
    if (!state.showStats) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 200, 55), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(180, 180), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.7f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("##Stats", &state.showStats, flags)) {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame: %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Separator();
        
        // Culling statistics
        ImGui::Text("Objects: %zu", state.cullStats.totalObjects);
        ImGui::Text("Visible: %zu", state.cullStats.visibleObjects);
        
        // Show culling efficiency
        if (state.cullStats.totalObjects > 0) {
            float cullRatio = (float)state.cullStats.culledObjects / state.cullStats.totalObjects * 100.0f;
            ImVec4 cullColor = cullRatio > 30.0f ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
            ImGui::TextColored(cullColor, "Culled: %zu (%.0f%%)", state.cullStats.culledObjects, cullRatio);
        } else {
            ImGui::Text("Culled: 0");
        }
    }
    ImGui::End();
}

// ===== Optimization Stats Panel =====
inline void drawOptimizationStatsPanel(EditorState& state) {
    if (!state.showOptimizationStats) return;
    
    ImGui::SetNextWindowPos(ImVec2(10, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Performance Optimization", &state.showOptimizationStats)) {
        // Culling section
        if (ImGui::CollapsingHeader("Frustum Culling", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10);
            
            ImGui::Text("Total Objects: %zu", state.cullStats.totalObjects);
            ImGui::Text("Visible: %zu", state.cullStats.visibleObjects);
            ImGui::Text("Culled: %zu", state.cullStats.culledObjects);
            
            if (state.cullStats.totalObjects > 0) {
                float efficiency = (float)state.cullStats.culledObjects / state.cullStats.totalObjects;
                
                // Progress bar showing culling efficiency
                ImGui::Text("Culling Efficiency:");
                ImGui::ProgressBar(efficiency, ImVec2(-1, 0), "");
                ImGui::SameLine(0, 5);
                ImGui::Text("%.1f%%", efficiency * 100.0f);
            }
            
            ImGui::Unindent(10);
        }
        
        // LOD section
        if (ImGui::CollapsingHeader("Level of Detail")) {
            ImGui::Indent(10);
            
            auto& lodMgr = getLODManager();
            auto& lodStats = lodMgr.getStats();
            
            ImGui::Text("Total: %zu objects", lodStats.totalObjects);
            ImGui::Text("Distance Culled: %zu", lodStats.culledByDistance);
            
            // LOD distribution
            ImGui::Text("LOD Distribution:");
            for (int i = 0; i < 4; i++) {
                if (lodStats.lodDistribution[i] > 0) {
                    ImGui::Text("  LOD %d: %zu", i, lodStats.lodDistribution[i]);
                }
            }
            
            // LOD bias slider
            float bias = lodMgr.getGlobalLODBias();
            if (ImGui::SliderFloat("LOD Bias", &bias, -2.0f, 2.0f)) {
                lodMgr.setGlobalLODBias(bias);
            }
            
            ImGui::Unindent(10);
        }
        
        // Instancing section
        if (ImGui::CollapsingHeader("GPU Instancing")) {
            ImGui::Indent(10);
            
            auto& instMgr = getInstancingManager();
            auto& instStats = instMgr.getStats();
            
            ImGui::Text("Total Instances: %zu", instStats.totalInstances);
            ImGui::Text("Visible Instances: %zu", instStats.visibleInstances);
            ImGui::Text("Batches: %zu", instMgr.getBatchCount());
            
            float savings = instMgr.getDrawCallReduction() * 100.0f;
            ImGui::Text("Draw Call Savings: %.1f%%", savings);
            
            ImGui::Unindent(10);
        }
        
        // Summary
        ImGui::Separator();
        
        float fps = ImGui::GetIO().Framerate;
        ImVec4 fpsColor = fps >= 60 ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) :
                          fps >= 30 ? ImVec4(0.8f, 0.8f, 0.4f, 1.0f) :
                                      ImVec4(0.8f, 0.4f, 0.4f, 1.0f);
        ImGui::TextColored(fpsColor, "FPS: %.1f (%.2f ms)", fps, 1000.0f / fps);
    }
    ImGui::End();
}

// ===== Shader Status Panel =====
inline void drawShaderStatus(const std::string& shaderError, bool hotReloadEnabled, 
                             std::function<void()> onReload, EditorState& state) {
    if (!state.showShaderStatus) return;
    
    // Only show if there's an error or hot reload is active
    if (!hotReloadEnabled && shaderError.empty()) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, 
                                   ImGui::GetIO().DisplaySize.y - 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 80), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("##ShaderStatus", nullptr, flags)) {
        if (!shaderError.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Shader Error:");
            ImGui::TextWrapped("%s", shaderError.c_str());
        } else if (hotReloadEnabled) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Shader Hot-Reload: Active");
        }
        
        if (ImGui::Button("Reload Shaders")) {
            if (onReload) onReload();
        }
    }
    ImGui::End();
}

// ===== Asset Cache Statistics =====
struct AssetCacheStats {
    uint64_t totalLoads = 0;
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    float hitRate = 0.0f;
    size_t cachedAssets = 0;
    size_t cacheSizeBytes = 0;
};

inline void drawAssetCachePanel(const AssetCacheStats& stats, EditorState& state) {
    // Show asset cache statistics (can be toggled via View menu)
    static bool showAssetCache = false;
    
    // Add to View menu if needed
    if (!showAssetCache) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 220, 180), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Asset Cache", &showAssetCache)) {
        ImGui::Text("Total Loads: %llu", (unsigned long long)stats.totalLoads);
        ImGui::Text("Cache Hits: %llu", (unsigned long long)stats.cacheHits);
        ImGui::Text("Cache Misses: %llu", (unsigned long long)stats.cacheMisses);
        ImGui::Separator();
        
        // Hit rate bar
        ImGui::Text("Hit Rate:");
        ImGui::SameLine();
        ImGui::ProgressBar(stats.hitRate, ImVec2(-1, 0), 
            (std::to_string((int)(stats.hitRate * 100)) + "%").c_str());
        
        ImGui::Separator();
        ImGui::Text("Cached: %zu assets", stats.cachedAssets);
        
        // Format cache size
        float sizeMB = stats.cacheSizeBytes / (1024.0f * 1024.0f);
        if (sizeMB >= 1.0f) {
            ImGui::Text("Size: %.1f MB", sizeMB);
        } else {
            ImGui::Text("Size: %.1f KB", stats.cacheSizeBytes / 1024.0f);
        }
    }
    ImGui::End();
}

// ===== Viewport Drag-Drop Target =====
// Call this in the main viewport area to accept dropped assets
inline bool handleViewportDragDrop(std::string& outAssetPath) {
    // Create an invisible drop target over the viewport
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 viewportPos(280, 55);  // Approximate viewport position
    ImVec2 viewportSize(io.DisplaySize.x - 560, io.DisplaySize.y - 280);
    
    // Check if we're in the viewport area
    if (io.MousePos.x >= viewportPos.x && io.MousePos.x <= viewportPos.x + viewportSize.x &&
        io.MousePos.y >= viewportPos.y && io.MousePos.y <= viewportPos.y + viewportSize.y) {
        
        // Accept drop
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                outAssetPath = static_cast<const char*>(payload->Data);
                ImGui::EndDragDropTarget();
                return true;
            }
            ImGui::EndDragDropTarget();
        }
    }
    
    return false;
}

// ===== Extended Asset Browser with Cache Integration =====
inline void drawAssetBrowserExtended(EditorState& state, const AssetCacheStats* cacheStats = nullptr) {
    if (!state.showAssetBrowser) return;
    
    ImGui::SetNextWindowPos(ImVec2(280, ImGui::GetIO().DisplaySize.y - 250), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Assets", &state.showAssetBrowser)) {
        // Tab bar for browser and cache
        if (ImGui::BeginTabBar("AssetTabs")) {
            // File browser tab
            if (ImGui::BeginTabItem("Browser")) {
                // Path bar
                ImGui::Text("Path: %s", state.currentAssetPath.c_str());
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::Button("Refresh")) {
                    // Refresh directory (no-op, will re-read on next frame)
                }
                
                ImGui::Separator();
                
                // File list
                ImGui::BeginChild("FileList", ImVec2(0, 0), false);
                
                // Parent directory
                if (state.currentAssetPath != ".") {
                    if (ImGui::Selectable(".. (Parent)", false, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            std::filesystem::path p(state.currentAssetPath);
                            state.currentAssetPath = p.parent_path().string();
                        }
                    }
                }
                
                try {
                    for (const auto& entry : std::filesystem::directory_iterator(state.currentAssetPath)) {
                        std::string name = entry.path().filename().string();
                        bool isDir = entry.is_directory();
                        
                        // Skip hidden files
                        if (name[0] == '.') continue;
                        
                        // Icon based on type
                        std::string icon;
                        if (isDir) {
                            icon = "[D] ";
                        } else {
                            std::string ext = entry.path().extension().string();
                            if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
                                icon = "[M] ";
                            } else if (ext == ".png" || ext == ".jpg" || ext == ".hdr") {
                                icon = "[T] ";
                            } else if (ext == ".hlsl" || ext == ".metal") {
                                icon = "[S] ";
                            } else if (ext == ".luma") {
                                icon = "[L] ";
                            } else {
                                icon = "[?] ";
                            }
                        }
                        
                        bool selected = (state.selectedAsset == entry.path().string());
                        if (ImGui::Selectable((icon + name).c_str(), selected, 
                                              ImGuiSelectableFlags_AllowDoubleClick)) {
                            state.selectedAsset = entry.path().string();
                            
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                if (isDir) {
                                    state.currentAssetPath = entry.path().string();
                                } else if (state.onModelLoad) {
                                    state.onModelLoad(entry.path().string());
                                }
                            }
                        }
                        
                        // Drag source for assets
                        if (!isDir && ImGui::BeginDragDropSource()) {
                            std::string path = entry.path().string();
                            ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.size() + 1);
                            ImGui::Text("Drop to load: %s", name.c_str());
                            ImGui::EndDragDropSource();
                        }
                        
                        // Tooltip with file info
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("%s", entry.path().string().c_str());
                            if (!isDir) {
                                auto fileSize = std::filesystem::file_size(entry);
                                if (fileSize > 1024 * 1024) {
                                    ImGui::Text("Size: %.1f MB", fileSize / (1024.0f * 1024.0f));
                                } else {
                                    ImGui::Text("Size: %.1f KB", fileSize / 1024.0f);
                                }
                            }
                            ImGui::EndTooltip();
                        }
                    }
                } catch (...) {
                    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Cannot read directory");
                }
                
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            
            // Cache tab
            if (ImGui::BeginTabItem("Cache")) {
                if (cacheStats) {
                    ImGui::Columns(2, "CacheColumns", false);
                    
                    ImGui::Text("Total Loads:");
                    ImGui::NextColumn();
                    ImGui::Text("%llu", (unsigned long long)cacheStats->totalLoads);
                    ImGui::NextColumn();
                    
                    ImGui::Text("Cache Hits:");
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1), "%llu", 
                                       (unsigned long long)cacheStats->cacheHits);
                    ImGui::NextColumn();
                    
                    ImGui::Text("Cache Misses:");
                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1), "%llu", 
                                       (unsigned long long)cacheStats->cacheMisses);
                    ImGui::NextColumn();
                    
                    ImGui::Columns(1);
                    ImGui::Separator();
                    
                    // Hit rate progress bar
                    ImGui::Text("Hit Rate: %.1f%%", cacheStats->hitRate * 100.0f);
                    ImGui::ProgressBar(cacheStats->hitRate, ImVec2(-1, 0));
                    
                    ImGui::Separator();
                    ImGui::Text("Cached Assets: %zu", cacheStats->cachedAssets);
                    
                    float sizeMB = cacheStats->cacheSizeBytes / (1024.0f * 1024.0f);
                    ImGui::Text("Cache Size: %.2f MB", sizeMB);
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Cache stats not available");
                }
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

// ===== Status Bar =====
inline void drawStatusBar(int windowWidth, int windowHeight, const std::string& statusText = "") {
    float statusBarHeight = 24.0f;
    ImGui::SetNextWindowPos(ImVec2(0, (float)windowHeight - statusBarHeight));
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, statusBarHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings;
    
    if (ImGui::Begin("##StatusBar", nullptr, flags)) {
        if (!statusText.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", statusText.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                "W/E/R: Transform | Alt+Mouse: Camera | F: Focus | G: Grid");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// ===== Apply Editor Theme =====
inline void applyEditorTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Rounding
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    
    // Borders
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    
    // Padding
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    
    // Colors - Dark theme with blue accent
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.35f, 0.45f, 1.00f);
    
    colors[ImGuiCol_Button] = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.40f, 0.55f, 1.00f);
    
    colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.24f, 0.32f, 1.00f);
    
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.50f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.60f, 0.80f, 1.00f);
    
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.60f, 0.85f, 1.00f);
    
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.40f, 0.55f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.50f, 0.70f, 1.00f);
    
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.40f, 0.55f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.50f, 0.70f, 0.70f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.55f, 0.80f, 1.00f);
    
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.40f, 0.60f, 1.00f, 0.90f);
}

}  // namespace ui
}  // namespace luma
