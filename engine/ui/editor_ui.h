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
#include "engine/animation/animation_layer.h"
#include "engine/animation/blend_tree.h"
#include "engine/animation/state_machine.h"
#include "engine/animation/ik_system.h"
#include "engine/export/screenshot.h"
#include "engine/lighting/light.h"
#include "engine/rendering/lod.h"
#include "engine/rendering/instancing.h"
#include "engine/rendering/ssao.h"
#include "engine/rendering/ssr.h"
#include "engine/rendering/volumetrics.h"
#include "engine/rendering/advanced_shadows.h"
#include "engine/rendering/ibl.h"
#include "engine/editor/demo_mode.h"
#include "engine/particles/particle.h"
#include "engine/particles/particle_presets.h"
#include "engine/physics/physics_world.h"
#include "engine/physics/collision.h"
#include "engine/physics/constraints.h"
#include "engine/physics/physics_debug.h"
#include "engine/physics/raycast.h"
#include "engine/terrain/terrain.h"
#include "engine/terrain/terrain_generator.h"
#include "engine/terrain/foliage.h"
#include "engine/audio/audio.h"
#include "engine/renderer/gi/gi_system.h"
#include "engine/video/video_export.h"
#include "engine/network/network.h"
#include "engine/script/script_engine.h"
#include "engine/ai/navmesh.h"
#include "engine/ai/nav_agent.h"
#include "engine/ai/behavior_tree.h"
#include "engine/game_ui/ui_system.h"
#include "engine/scene/scene_manager.h"
#include "engine/scene/prefab.h"
#include "engine/data/data_system.h"
#include "engine/build/build_system.h"
#include "engine/asset/asset_browser.h"
#include "engine/script/visual_script.h"
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

namespace luma {
namespace ui {

// ===== Responsive Layout System =====
struct EditorLayout {
    // Layout constants
    static constexpr float kMenuBarHeight = 19.0f;
    static constexpr float kToolbarHeight = 36.0f;
    static constexpr float kStatusBarHeight = 24.0f;
    static constexpr float kLeftPanelWidth = 280.0f;
    static constexpr float kRightPanelWidth = 320.0f;
    static constexpr float kBottomPanelHeight = 200.0f;
    
    // Calculate layout regions based on current window size
    static float getTopOffset() { return kMenuBarHeight + kToolbarHeight; }
    
    static ImVec2 getLeftPanelPos() { 
        return ImVec2(0, getTopOffset()); 
    }
    static ImVec2 getLeftPanelSize(float windowHeight, bool hasBottomPanel) {
        float height = windowHeight - getTopOffset() - kStatusBarHeight;
        if (hasBottomPanel) height -= kBottomPanelHeight;
        return ImVec2(kLeftPanelWidth, height);
    }
    
    static ImVec2 getRightPanelPos(float windowWidth) {
        return ImVec2(windowWidth - kRightPanelWidth, getTopOffset());
    }
    static ImVec2 getRightPanelSize(float windowHeight, bool hasBottomPanel) {
        float height = windowHeight - getTopOffset() - kStatusBarHeight;
        if (hasBottomPanel) height -= kBottomPanelHeight;
        return ImVec2(kRightPanelWidth, height);
    }
    
    static ImVec2 getBottomPanelPos(float windowHeight) {
        return ImVec2(0, windowHeight - kBottomPanelHeight - kStatusBarHeight);
    }
    static ImVec2 getBottomPanelSize(float windowWidth) {
        return ImVec2(windowWidth, kBottomPanelHeight);
    }
    
    static ImVec2 getViewportPos() {
        return ImVec2(kLeftPanelWidth, getTopOffset());
    }
    static ImVec2 getViewportSize(float windowWidth, float windowHeight, bool hasBottomPanel) {
        float width = windowWidth - kLeftPanelWidth - kRightPanelWidth;
        float height = windowHeight - getTopOffset() - kStatusBarHeight;
        if (hasBottomPanel) height -= kBottomPanelHeight;
        return ImVec2(width, height);
    }
};

// ===== Editor State =====
struct EditorState {
    // Window visibility - Core panels (always docked)
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAssetBrowser = true;
    
    // Window visibility - Secondary panels (open via View menu)
    bool showAnimationTimeline = false;
    bool showPostProcess = false;      // Open via View > Post Processing
    bool showRenderSettings = false;   // Open via View > Render Settings
    bool showLighting = false;         // Open via View > Lighting
    bool showConsole = false;          // Open via View > Console
    bool showHelp = false;
    bool showStats = true;             // Small overlay, always useful
    bool showShaderStatus = false;     // Only show when shader errors
    bool showScreenshotDialog = false;
    
    // Window visibility - Advanced (NEW)
    bool showAdvancedPostProcess = false;
    bool showAdvancedShadows = false;
    bool showEnvironment = false;
    bool showStateMachineEditor = false;
    bool showBlendTreeEditor = false;
    bool showIKSettings = false;
    bool showAnimationLayers = false;
    bool showLODSettings = false;
    bool showDemoMenu = false;
    bool showParticleEditor = false;
    bool showPhysicsEditor = false;
    bool showTerrainEditor = false;
    bool showAudioEditor = false;
    bool showGIEditor = false;
    bool showVideoExport = false;
    bool showNetworkPanel = false;
    bool showScriptEditor = false;
    bool showAIEditor = false;
    bool showGameUIEditor = false;
    bool showSceneManager = false;
    bool showDataManager = false;
    bool showBuildSettings = false;
    bool showVisualScript = false;
    bool showCharacterCreator = false;
    
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
    
    // Animation - Basic
    bool animationPlaying = false;
    float animationTime = 0.0f;
    float animationSpeed = 1.0f;
    std::string currentClip;
    
    // Animation - State Machine Editor
    int selectedStateIndex = -1;
    int selectedTransitionIndex = -1;
    std::string newStateName;
    std::string newParameterName;
    int newParameterType = 0;
    
    // Animation - Blend Tree Editor
    int selectedBlendTreeMotion = -1;
    float blendTreeParam1 = 0.0f;
    float blendTreeParam2 = 0.0f;
    
    // Animation - IK
    int selectedIKChain = -1;
    
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
    
    // Environment / IBL
    std::string currentHDRPath;
    float iblIntensity = 1.0f;
    float iblRotation = 0.0f;
    
    // Callbacks
    std::function<void(const std::string&)> onModelLoad;
    std::function<void(const std::string&)> onSceneSave;
    std::function<void(const std::string&)> onSceneLoad;
    std::function<void(const std::string&)> onHDRLoad;
    std::function<void(const std::string&)> onDemoGenerate;
    
    // Asset Browser callbacks
    std::function<void(const std::string& path, BrowserAssetType type)> onAssetDoubleClick;
    std::function<void(const std::string& path, BrowserAssetType type)> onAssetDragDropToScene;
    std::function<void(const std::string& path)> onAssetPreview;
    
    // Prefab callbacks
    std::function<void(Entity*)> onSaveAsPrefab;
    std::function<void(const std::string&)> onInstantiatePrefab;
};

// ===== Advanced Settings Structures =====
struct AdvancedPostProcessState {
    // SSAO
    SSAOSettings ssao;
    bool ssaoEnabled = false;
    
    // SSR
    SSRSettings ssr;
    bool ssrEnabled = false;
    
    // Volumetrics
    VolumetricFogSettings fog;
    bool fogEnabled = false;
    
    GodRaySettings godRays;
    bool godRaysEnabled = false;
};

struct AdvancedShadowState {
    // CSM
    CSMSettings csm;
    bool csmEnabled = true;
    
    // PCSS
    bool pcssEnabled = false;
    int pcssBlockerSamples = 16;
    int pcssPCFSamples = 32;
    float pcssLightSize = 0.02f;
};

enum class LODQualityPreset { Low, Medium, High, Ultra };

struct LODState {
    LODQualityPreset qualityPreset = LODQualityPreset::Medium;
    float lodBias = 1.0f;
    float maxDistance = 1000.0f;
    bool showLODDebug = false;
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

// ===== Callback wrapper for scene operations =====
struct SceneCallbacks {
    std::function<void()> onNewScene;
    std::function<void()> onDeleteSelected;
    std::function<void()> onDuplicateSelected;
};

inline SceneCallbacks& getSceneCallbacks() {
    static SceneCallbacks callbacks;
    return callbacks;
}

// ===== Main Menu Bar =====
inline void drawMainMenuBar(EditorState& state, Viewport& viewport, bool& shouldQuit) {
    if (ImGui::BeginMainMenuBar()) {
        // File Menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                auto& cb = getSceneCallbacks();
                if (cb.onNewScene) cb.onNewScene();
            }
            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                if (state.onSceneLoad) state.onSceneLoad("");
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                if (state.onSceneSave) state.onSceneSave("");
            }
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
                if (state.onSceneSave) state.onSceneSave("");  // Will show dialog if path empty
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Model...")) {
                if (state.onModelLoad) state.onModelLoad("");
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
                auto& cb = getSceneCallbacks();
                if (cb.onDeleteSelected) cb.onDeleteSelected();
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                auto& cb = getSceneCallbacks();
                if (cb.onDuplicateSelected) cb.onDuplicateSelected();
            }
            ImGui::Separator();
            ImGui::MenuItem("History Panel", nullptr, &state.showHistory);
            ImGui::EndMenu();
        }
        
        // View Menu
        if (ImGui::BeginMenu("View")) {
            // Core Panels (docked)
            if (ImGui::BeginMenu("Panels")) {
                ImGui::MenuItem("Hierarchy", "H", &state.showHierarchy);
                ImGui::MenuItem("Inspector", "I", &state.showInspector);
                ImGui::MenuItem("Asset Browser", "A", &state.showAssetBrowser);
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            // Rendering panels
            if (ImGui::BeginMenu("Rendering")) {
                ImGui::MenuItem("Post Processing", nullptr, &state.showPostProcess);
                ImGui::MenuItem("Render Settings", nullptr, &state.showRenderSettings);
                ImGui::MenuItem("Lighting", nullptr, &state.showLighting);
                ImGui::EndMenu();
            }
            
            // Animation
            ImGui::MenuItem("Animation Timeline", nullptr, &state.showAnimationTimeline);
            
            ImGui::Separator();
            
            // Debug/Development
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Console", "`", &state.showConsole);
                ImGui::MenuItem("Statistics", nullptr, &state.showStats);
                ImGui::MenuItem("Shader Status", nullptr, &state.showShaderStatus);
                ImGui::EndMenu();
            }
            
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
            ImGui::Text("Panels");
            ImGui::Separator();
            ImGui::MenuItem("Hierarchy", nullptr, &state.showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
            ImGui::MenuItem("Asset Browser", nullptr, &state.showAssetBrowser);
            ImGui::MenuItem("Console", nullptr, &state.showConsole);
            ImGui::MenuItem("Statistics", nullptr, &state.showStats);
            
            ImGui::Separator();
            ImGui::Text("Rendering");
            ImGui::Separator();
            ImGui::MenuItem("Post-Processing", nullptr, &state.showPostProcess);
            ImGui::MenuItem("Advanced Post-Process", nullptr, &state.showAdvancedPostProcess);
            ImGui::MenuItem("Advanced Shadows", nullptr, &state.showAdvancedShadows);
            ImGui::MenuItem("Environment / IBL", nullptr, &state.showEnvironment);
            ImGui::MenuItem("Lighting", nullptr, &state.showLighting);
            ImGui::MenuItem("LOD Settings", nullptr, &state.showLODSettings);
            ImGui::MenuItem("Particle Editor", nullptr, &state.showParticleEditor);
            ImGui::MenuItem("Physics Editor", nullptr, &state.showPhysicsEditor);
            ImGui::MenuItem("Terrain Editor", nullptr, &state.showTerrainEditor);
            ImGui::MenuItem("Audio Editor", nullptr, &state.showAudioEditor);
            ImGui::MenuItem("GI Editor", nullptr, &state.showGIEditor);
            ImGui::MenuItem("Video Export", nullptr, &state.showVideoExport);
            ImGui::MenuItem("Network", nullptr, &state.showNetworkPanel);
            ImGui::MenuItem("AI Editor", nullptr, &state.showAIEditor);
            ImGui::MenuItem("Game UI Editor", nullptr, &state.showGameUIEditor);
            ImGui::MenuItem("Scene Manager", nullptr, &state.showSceneManager);
            ImGui::MenuItem("Data Manager", nullptr, &state.showDataManager);
            ImGui::Separator();
            ImGui::MenuItem("Build Settings", nullptr, &state.showBuildSettings);
            
            ImGui::Separator();
            ImGui::Text("Tools");
            ImGui::Separator();
            ImGui::MenuItem("Character Creator", nullptr, &state.showCharacterCreator);
            
            ImGui::Separator();
            ImGui::Text("Scripting");
            ImGui::Separator();
            ImGui::MenuItem("Visual Script", nullptr, &state.showVisualScript);
            ImGui::MenuItem("Script Editor", nullptr, &state.showScriptEditor);
            
            ImGui::Separator();
            ImGui::Text("Animation");
            ImGui::Separator();
            ImGui::MenuItem("Timeline", nullptr, &state.showAnimationTimeline);
            ImGui::MenuItem("State Machine Editor", nullptr, &state.showStateMachineEditor);
            ImGui::MenuItem("Blend Tree Editor", nullptr, &state.showBlendTreeEditor);
            ImGui::MenuItem("Animation Layers", nullptr, &state.showAnimationLayers);
            ImGui::MenuItem("IK Settings", nullptr, &state.showIKSettings);
            
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                state.showHierarchy = true;
                state.showInspector = true;
                state.showPostProcess = true;
                state.showStats = true;
            }
            ImGui::EndMenu();
        }
        
        // Help Menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Demo Scenes...")) {
                state.showDemoMenu = true;
            }
            ImGui::Separator();
            ImGui::MenuItem("Keyboard Shortcuts", "F1", &state.showHelp);
            ImGui::Separator();
            if (ImGui::MenuItem("About LUMA Studio")) {
                // Show about dialog
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
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, EditorLayout::kMenuBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorLayout::kToolbarHeight), ImGuiCond_Always);
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
        
        // Right-aligned panel toggles
        float rightOffset = io.DisplaySize.x - 320;
        ImGui::SameLine(rightOffset);
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Rendering panels quick access
        if (state.showPostProcess) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
        if (ImGui::Button("PP", ImVec2(30, 26))) state.showPostProcess = !state.showPostProcess;
        if (state.showPostProcess) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Post Processing");
        
        ImGui::SameLine();
        if (state.showRenderSettings) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
        if (ImGui::Button("RS", ImVec2(30, 26))) state.showRenderSettings = !state.showRenderSettings;
        if (state.showRenderSettings) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render Settings");
        
        ImGui::SameLine();
        if (state.showLighting) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
        if (ImGui::Button("LT", ImVec2(30, 26))) state.showLighting = !state.showLighting;
        if (state.showLighting) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lighting");
        
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Console toggle
        if (state.showConsole) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("C", ImVec2(26, 26))) state.showConsole = !state.showConsole;
        if (state.showConsole) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Console (`)");
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
}

// ===== Scene Hierarchy Panel =====
inline void drawHierarchyPanel(SceneGraph& scene, EditorState& state) {
    if (!state.showHierarchy) return;
    
    // Responsive layout - dock to left side
    ImGuiIO& io = ImGui::GetIO();
    bool hasBottomPanel = state.showAssetBrowser || state.showAnimationTimeline || state.showConsole;
    ImGui::SetNextWindowPos(EditorLayout::getLeftPanelPos(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(EditorLayout::getLeftPanelSize(io.DisplaySize.y, hasBottomPanel), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Hierarchy", &state.showHierarchy, flags)) {
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
                // Accept assets from asset browser
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_ITEM")) {
                    std::string assetPath((const char*)payload->Data);
                    if (state.onAssetDragDropToScene) {
                        // Determine asset type from extension
                        BrowserAssetType type = BrowserAssetType::Unknown;
                        size_t dotPos = assetPath.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            std::string ext = assetPath.substr(dotPos);
                            // Check extension
                            if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
                                type = BrowserAssetType::Model;
                            } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".hdr") {
                                type = BrowserAssetType::Texture;
                            } else if (ext == ".luma") {
                                type = BrowserAssetType::Scene;
                            }
                        }
                        state.onAssetDragDropToScene(assetPath, type);
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
                    scene.duplicateEntity(entity);
                }
                ImGui::Separator();
                
                // Prefab options
                bool isPrefab = getPrefabManager().isPrefabInstance(entity->id);
                if (ImGui::MenuItem("Save as Prefab...")) {
                    if (state.onSaveAsPrefab) {
                        state.onSaveAsPrefab(entity);
                    }
                }
                if (isPrefab) {
                    if (ImGui::MenuItem("Apply Prefab")) {
                        getPrefabManager().applyPrefab(entity->id, scene);
                    }
                    if (ImGui::MenuItem("Unpack Prefab")) {
                        getPrefabManager().unpackInstance(entity->id);
                    }
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
        
        // Drop zone at bottom of hierarchy
        ImGui::Separator();
        ImGui::InvisibleButton("##DropZone", ImVec2(-1, 30));
        if (ImGui::BeginDragDropTarget()) {
            // Accept entity for reparenting to root
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                Entity* dragged = *(Entity**)payload->Data;
                scene.setParent(dragged, nullptr);
            }
            // Accept assets from browser
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_ITEM")) {
                std::string assetPath((const char*)payload->Data);
                if (state.onAssetDragDropToScene) {
                    // Determine type from extension
                    BrowserAssetType type = BrowserAssetType::Unknown;
                    size_t dotPos = assetPath.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        std::string ext = assetPath.substr(dotPos);
                        if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") {
                            type = BrowserAssetType::Model;
                        } else if (ext == ".luma") {
                            type = BrowserAssetType::Scene;
                        }
                    }
                    state.onAssetDragDropToScene(assetPath, type);
                }
            }
            ImGui::EndDragDropTarget();
        }
        // Visual feedback
        if (ImGui::IsItemHovered()) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                IM_COL32(100, 100, 150, 50)
            );
        }
        ImGui::TextDisabled("Drop assets or entities here");
    }
    ImGui::End();
}

// ===== Inspector Panel =====
inline void drawInspectorPanel(SceneGraph& scene, EditorState& state) {
    if (!state.showInspector) return;
    
    // Responsive layout - dock to right side
    ImGuiIO& io = ImGui::GetIO();
    bool hasBottomPanel = state.showAssetBrowser || state.showAnimationTimeline || state.showConsole;
    ImGui::SetNextWindowPos(EditorLayout::getRightPanelPos(io.DisplaySize.x), ImGuiCond_Always);
    ImGui::SetNextWindowSize(EditorLayout::getRightPanelSize(io.DisplaySize.y, hasBottomPanel), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Inspector", &state.showInspector, flags)) {
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
    
    // Floating panel - positioned to the left of the main Inspector
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - EditorLayout::kRightPanelWidth - 330, 100), ImGuiCond_FirstUseEver);
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
    
    // Floating panel - positioned below left panel area
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(EditorLayout::kLeftPanelWidth + 10, 100), ImGuiCond_FirstUseEver);
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
    
    // Floating panel - positioned in viewport area
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(EditorLayout::kLeftPanelWidth + 10, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 280), ImGuiCond_FirstUseEver);
    
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
    
    // Responsive layout - dock to bottom (shares space with Asset Browser)
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(EditorLayout::getBottomPanelPos(io.DisplaySize.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(EditorLayout::getBottomPanelSize(io.DisplaySize.x), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Animation", &state.showAnimationTimeline, flags)) {
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
    
    // Responsive layout - dock to bottom
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(EditorLayout::getBottomPanelPos(io.DisplaySize.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(EditorLayout::getBottomPanelSize(io.DisplaySize.x), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Assets", &state.showAssetBrowser, flags)) {
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
    
    // Responsive layout - dock to bottom (can share with Asset Browser)
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(EditorLayout::getBottomPanelPos(io.DisplaySize.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(EditorLayout::getBottomPanelSize(io.DisplaySize.x), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Console", &state.showConsole, flags)) {
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
    
    // Small overlay in viewport area (top-right of viewport, not overlapping Inspector)
    ImGuiIO& io = ImGui::GetIO();
    float x = EditorLayout::kLeftPanelWidth + 10;
    float y = EditorLayout::getTopOffset() + 10;
    
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(150, 0), ImGuiCond_Always);  // Auto height
    ImGui::SetNextWindowBgAlpha(0.6f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##Stats", nullptr, flags)) {
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "FPS: %.1f", io.Framerate);
        ImGui::Text("Frame: %.2f ms", 1000.0f / io.Framerate);
        ImGui::Separator();
        ImGui::Text("Objects: %zu", state.cullStats.totalObjects);
        ImGui::Text("Visible: %zu", state.cullStats.visibleObjects);
        if (state.cullStats.totalObjects > 0) {
            float cullRatio = (float)state.cullStats.culledObjects / state.cullStats.totalObjects * 100.0f;
            ImGui::Text("Culled: %.0f%%", cullRatio);
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
    
    // Responsive layout - dock to bottom
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(EditorLayout::getBottomPanelPos(io.DisplaySize.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(EditorLayout::getBottomPanelSize(io.DisplaySize.x), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Assets", &state.showAssetBrowser, flags)) {
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

// ===== Advanced Post-Processing Panel =====
inline void drawAdvancedPostProcessPanel(AdvancedPostProcessState& state, EditorState& editorState) {
    if (!editorState.showAdvancedPostProcess) return;
    
    ImGui::SetNextWindowSize(ImVec2(320, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Advanced Post-Processing", &editorState.showAdvancedPostProcess)) {
        
        // SSAO Section
        if (ImGui::CollapsingHeader("SSAO (Ambient Occlusion)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable SSAO", &state.ssaoEnabled);
            if (state.ssaoEnabled) {
                ImGui::Indent();
                
                // Presets
                const char* presets[] = { "Low", "Medium", "High", "Ultra", "Custom" };
                static int currentPreset = 1;
                if (ImGui::Combo("Preset", &currentPreset, presets, 5)) {
                    switch (currentPreset) {
                        case 0: state.ssao = SSAOPresets::low(); break;
                        case 1: state.ssao = SSAOPresets::medium(); break;
                        case 2: state.ssao = SSAOPresets::high(); break;
                        case 3: state.ssao = SSAOPresets::ultra(); break;
                    }
                }
                
                ImGui::SliderInt("Samples", &state.ssao.sampleCount, 8, 64);
                ImGui::SliderFloat("Radius", &state.ssao.radius, 0.1f, 2.0f, "%.2f");
                ImGui::SliderFloat("Bias", &state.ssao.bias, 0.001f, 0.1f, "%.3f");
                ImGui::SliderFloat("Intensity", &state.ssao.intensity, 0.5f, 3.0f, "%.2f");
                ImGui::SliderFloat("Power", &state.ssao.power, 1.0f, 4.0f, "%.1f");
                ImGui::Checkbox("Half Resolution", &state.ssao.halfResolution);
                ImGui::Checkbox("Enable Blur", &state.ssao.enableBlur);
                if (state.ssao.enableBlur) {
                    ImGui::SliderInt("Blur Passes", &state.ssao.blurPasses, 1, 4);
                }
                
                ImGui::Unindent();
            }
        }
        
        ImGui::Separator();
        
        // SSR Section
        if (ImGui::CollapsingHeader("SSR (Screen Space Reflections)")) {
            ImGui::Checkbox("Enable SSR", &state.ssrEnabled);
            if (state.ssrEnabled) {
                ImGui::Indent();
                
                // Presets
                const char* presets[] = { "Low", "Medium", "High", "Custom" };
                static int currentPreset = 1;
                if (ImGui::Combo("Preset##SSR", &currentPreset, presets, 4)) {
                    switch (currentPreset) {
                        case 0: state.ssr = SSRPresets::low(); break;
                        case 1: state.ssr = SSRPresets::medium(); break;
                        case 2: state.ssr = SSRPresets::high(); break;
                    }
                }
                
                ImGui::SliderInt("Max Steps", &state.ssr.maxSteps, 16, 256);
                ImGui::SliderInt("Binary Steps", &state.ssr.binarySearchSteps, 0, 16);
                ImGui::SliderFloat("Max Distance", &state.ssr.maxDistance, 10.0f, 500.0f);
                ImGui::SliderFloat("Thickness", &state.ssr.thickness, 0.1f, 2.0f);
                ImGui::SliderFloat("Roughness Threshold", &state.ssr.roughnessThreshold, 0.0f, 1.0f);
                ImGui::SliderFloat("Fade Start", &state.ssr.fadeStart, 0.0f, 1.0f);
                ImGui::Checkbox("Half Resolution##SSR", &state.ssr.halfResolution);
                
                ImGui::Unindent();
            }
        }
        
        ImGui::Separator();
        
        // Volumetric Fog Section
        if (ImGui::CollapsingHeader("Volumetric Fog")) {
            ImGui::Checkbox("Enable Fog", &state.fogEnabled);
            if (state.fogEnabled) {
                ImGui::Indent();
                
                // Presets
                const char* presets[] = { "Light Fog", "Dense Fog", "Ground Fog", "Custom" };
                static int currentPreset = 0;
                if (ImGui::Combo("Preset##Fog", &currentPreset, presets, 4)) {
                    switch (currentPreset) {
                        case 0: state.fog = VolumetricPresets::lightFog(); break;
                        case 1: state.fog = VolumetricPresets::denseFog(); break;
                        case 2: state.fog = VolumetricPresets::groundFog(); break;
                    }
                }
                
                ImGui::SliderFloat("Density", &state.fog.density, 0.001f, 0.5f, "%.3f");
                ImGui::ColorEdit3("Albedo", &state.fog.albedo.x);
                ImGui::SliderFloat("Scattering", &state.fog.scattering, 0.0f, 1.0f);
                ImGui::SliderFloat("Absorption", &state.fog.absorption, 0.0f, 1.0f);
                ImGui::SliderFloat("Height Falloff", &state.fog.heightFalloff, 0.0f, 0.5f);
                ImGui::SliderFloat("Height Offset", &state.fog.heightOffset, -100.0f, 100.0f);
                ImGui::SliderInt("Steps", &state.fog.steps, 16, 128);
                
                ImGui::Unindent();
            }
        }
        
        ImGui::Separator();
        
        // God Rays Section
        if (ImGui::CollapsingHeader("God Rays")) {
            ImGui::Checkbox("Enable God Rays", &state.godRaysEnabled);
            if (state.godRaysEnabled) {
                ImGui::Indent();
                
                ImGui::DragFloat3("Light Position", &state.godRays.lightPosition.x, 1.0f);
                ImGui::ColorEdit3("Light Color", &state.godRays.lightColor.x);
                ImGui::SliderInt("Samples##GodRay", &state.godRays.samples, 32, 200);
                ImGui::SliderFloat("Density##GodRay", &state.godRays.density, 0.5f, 2.0f);
                ImGui::SliderFloat("Weight", &state.godRays.weight, 0.001f, 0.05f, "%.3f");
                ImGui::SliderFloat("Decay", &state.godRays.decay, 0.9f, 1.0f, "%.3f");
                ImGui::SliderFloat("Exposure##GodRay", &state.godRays.exposure, 0.1f, 2.0f);
                
                ImGui::Unindent();
            }
        }
    }
    ImGui::End();
}

// ===== Advanced Shadows Panel =====
inline void drawAdvancedShadowsPanel(AdvancedShadowState& state, EditorState& editorState) {
    if (!editorState.showAdvancedShadows) return;
    
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Advanced Shadows", &editorState.showAdvancedShadows)) {
        
        // CSM Section
        if (ImGui::CollapsingHeader("Cascaded Shadow Maps", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable CSM", &state.csmEnabled);
            if (state.csmEnabled) {
                ImGui::Indent();
                
                ImGui::SliderInt("Cascade Count", &state.csm.numCascades, 1, 4);
                ImGui::SliderFloat("Max Shadow Distance", &state.csm.maxShadowDistance, 50.0f, 500.0f);
                ImGui::SliderInt("Shadow Map Size", &state.csm.shadowMapSize, 512, 4096);
                
                ImGui::Separator();
                ImGui::Text("Cascade Splits:");
                for (int i = 0; i < state.csm.numCascades; i++) {
                    ImGui::PushID(i);
                    char label[32];
                    snprintf(label, sizeof(label), "Split %d", i);
                    ImGui::SliderFloat(label, &state.csm.cascadeSplits[i], 0.0f, 1.0f);
                    ImGui::PopID();
                }
                
                ImGui::Separator();
                ImGui::Checkbox("Stabilize Cascades", &state.csm.stabilizeCascades);
                ImGui::SliderFloat("Blend Width", &state.csm.cascadeBlendWidth, 0.0f, 0.5f);
                ImGui::SliderFloat("Constant Bias", &state.csm.constantBias, 0.0f, 0.01f, "%.4f");
                ImGui::SliderFloat("Slope Bias", &state.csm.slopeBias, 0.0f, 5.0f);
                
                ImGui::Unindent();
            }
        }
        
        ImGui::Separator();
        
        // PCSS Section
        if (ImGui::CollapsingHeader("PCSS (Soft Shadows)")) {
            ImGui::Checkbox("Enable PCSS", &state.pcssEnabled);
            if (state.pcssEnabled) {
                ImGui::Indent();
                
                ImGui::SliderInt("Blocker Samples", &state.pcssBlockerSamples, 8, 64);
                ImGui::SliderInt("PCF Samples", &state.pcssPCFSamples, 16, 128);
                ImGui::SliderFloat("Light Size", &state.pcssLightSize, 0.001f, 0.1f, "%.3f");
                ImGui::Text("(Larger = softer shadows)");
                
                ImGui::Unindent();
            }
        }
        
        ImGui::Separator();
        
        // Debug
        if (ImGui::CollapsingHeader("Debug")) {
            static bool showCascades = false;
            ImGui::Checkbox("Visualize Cascades", &showCascades);
            if (showCascades) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Cascade 0 (Near)");
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Cascade 1");
                ImGui::TextColored(ImVec4(0, 0, 1, 1), "Cascade 2");
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Cascade 3 (Far)");
            }
        }
    }
    ImGui::End();
}

// ===== Environment / IBL Panel =====
inline void drawEnvironmentPanel(EditorState& state) {
    if (!state.showEnvironment) return;
    
    ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Environment", &state.showEnvironment)) {
        
        // HDR Environment Map
        if (ImGui::CollapsingHeader("HDR Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Current: %s", state.currentHDRPath.empty() ? "(None)" : 
                        std::filesystem::path(state.currentHDRPath).filename().string().c_str());
            
            if (ImGui::Button("Load HDR...")) {
                // In real implementation, open file dialog
                if (state.onHDRLoad) {
                    state.onHDRLoad("");  // Empty string triggers file dialog
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                state.currentHDRPath = "";
            }
            
            ImGui::Separator();
            
            ImGui::SliderFloat("Intensity", &state.iblIntensity, 0.0f, 5.0f);
            ImGui::SliderFloat("Rotation", &state.iblRotation, 0.0f, 360.0f, "%.0f deg");
        }
        
        ImGui::Separator();
        
        // Built-in Environments
        if (ImGui::CollapsingHeader("Quick Presets")) {
            if (ImGui::Button("Studio", ImVec2(80, 0))) {
                // Apply studio lighting preset
            }
            ImGui::SameLine();
            if (ImGui::Button("Outdoor", ImVec2(80, 0))) {
                // Apply outdoor preset
            }
            ImGui::SameLine();
            if (ImGui::Button("Night", ImVec2(80, 0))) {
                // Apply night preset
            }
        }
    }
    ImGui::End();
}

// ===== Animation State Machine Editor =====
inline void drawStateMachineEditor(AnimationStateMachine* sm, EditorState& state) {
    if (!state.showStateMachineEditor) return;
    if (!sm) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Animation State Machine", &state.showStateMachineEditor)) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No state machine selected");
            ImGui::Text("Select an animated entity with a state machine.");
        }
        ImGui::End();
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Animation State Machine", &state.showStateMachineEditor)) {
        
        // Parameters Section
        if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto params = sm->getParameterNames();
            
            for (const auto& name : params) {
                ImGui::PushID(name.c_str());
                auto type = sm->getParameterType(name);
                
                switch (type) {
                    case ParameterType::Float: {
                        float val = sm->getFloat(name);
                        if (ImGui::SliderFloat(name.c_str(), &val, 0.0f, 1.0f)) {
                            sm->setFloat(name, val);
                        }
                        break;
                    }
                    case ParameterType::Int: {
                        int val = sm->getInt(name);
                        if (ImGui::SliderInt(name.c_str(), &val, 0, 10)) {
                            sm->setInt(name, val);
                        }
                        break;
                    }
                    case ParameterType::Bool: {
                        bool val = sm->getBool(name);
                        if (ImGui::Checkbox(name.c_str(), &val)) {
                            sm->setBool(name, val);
                        }
                        break;
                    }
                    case ParameterType::Trigger: {
                        ImGui::Text("%s", name.c_str());
                        ImGui::SameLine();
                        if (ImGui::Button("Fire")) {
                            sm->setTrigger(name);
                        }
                        break;
                    }
                }
                ImGui::PopID();
            }
            
            ImGui::Separator();
            
            // Add new parameter
            ImGui::InputText("##NewParam", &state.newParameterName[0], 64);
            ImGui::SameLine();
            const char* types[] = { "Float", "Int", "Bool", "Trigger" };
            ImGui::SetNextItemWidth(80);
            ImGui::Combo("##ParamType", &state.newParameterType, types, 4);
            ImGui::SameLine();
            if (ImGui::Button("Add Parameter")) {
                if (!state.newParameterName.empty()) {
                    sm->addParameter(state.newParameterName, static_cast<ParameterType>(state.newParameterType));
                    state.newParameterName.clear();
                }
            }
        }
        
        ImGui::Separator();
        
        // States Section
        if (ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Current State: %s", sm->getCurrentStateName().c_str());
            
            ImGui::BeginChild("StateList", ImVec2(0, 150), true);
            auto stateNames = sm->getStateNames();
            for (size_t i = 0; i < stateNames.size(); i++) {
                bool isSelected = (state.selectedStateIndex == static_cast<int>(i));
                bool isCurrent = (stateNames[i] == sm->getCurrentStateName());
                
                if (isCurrent) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
                }
                
                if (ImGui::Selectable(stateNames[i].c_str(), isSelected)) {
                    state.selectedStateIndex = static_cast<int>(i);
                }
                
                if (isCurrent) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
            
            // Add new state
            ImGui::InputText("##NewState", &state.newStateName[0], 64);
            ImGui::SameLine();
            if (ImGui::Button("Add State")) {
                if (!state.newStateName.empty()) {
                    sm->createState(state.newStateName);
                    state.newStateName.clear();
                }
            }
        }
        
        ImGui::Separator();
        
        // State Details
        if (state.selectedStateIndex >= 0) {
            auto stateNames = sm->getStateNames();
            if (state.selectedStateIndex < static_cast<int>(stateNames.size())) {
                std::string stateName = stateNames[state.selectedStateIndex];
                ImGui::Text("Selected State: %s", stateName.c_str());
                
                if (ImGui::Button("Set as Default")) {
                    sm->setDefaultState(stateName);
                }
                ImGui::SameLine();
                if (ImGui::Button("Force Transition")) {
                    sm->forceState(stateName);
                }
            }
        }
    }
    ImGui::End();
}

// ===== Blend Tree Editor =====
inline void drawBlendTreeEditor(BlendTree1D* tree1D, BlendTree2D* tree2D, EditorState& state) {
    if (!state.showBlendTreeEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Blend Tree Editor", &state.showBlendTreeEditor)) {
        
        if (tree1D) {
            ImGui::Text("1D Blend Tree: %s", tree1D->parameterName.c_str());
            
            // Parameter slider
            float param = tree1D->getParameter(tree1D->parameterName);
            if (ImGui::SliderFloat("Parameter", &param, 0.0f, 1.0f)) {
                tree1D->setParameter(tree1D->parameterName, param);
            }
            
            ImGui::Separator();
            
            // Motion list
            ImGui::Text("Motions:");
            ImGui::BeginChild("Motions1D", ImVec2(0, 150), true);
            for (size_t i = 0; i < tree1D->motions.size(); i++) {
                auto& motion = tree1D->motions[i];
                ImGui::PushID(static_cast<int>(i));
                
                bool selected = (state.selectedBlendTreeMotion == static_cast<int>(i));
                if (ImGui::Selectable(("Motion " + std::to_string(i)).c_str(), selected)) {
                    state.selectedBlendTreeMotion = static_cast<int>(i);
                }
                ImGui::SameLine(150);
                ImGui::Text("Threshold: %.2f", motion.threshold);
                
                ImGui::PopID();
            }
            ImGui::EndChild();
            
            // Visualization
            ImGui::Text("Blend Visualization:");
            ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 30);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                                    IM_COL32(40, 40, 40, 255));
            
            // Draw motion markers
            for (const auto& motion : tree1D->motions) {
                float x = pos.x + motion.threshold * size.x;
                drawList->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y), 
                                  IM_COL32(100, 150, 255, 255), 2.0f);
            }
            
            // Draw current position
            float currentX = pos.x + param * size.x;
            drawList->AddTriangleFilled(
                ImVec2(currentX - 5, pos.y + size.y),
                ImVec2(currentX + 5, pos.y + size.y),
                ImVec2(currentX, pos.y + size.y - 10),
                IM_COL32(255, 200, 50, 255));
            
            ImGui::Dummy(size);
        }
        else if (tree2D) {
            ImGui::Text("2D Blend Tree");
            ImGui::Text("X: %s, Y: %s", tree2D->parameterX.c_str(), tree2D->parameterY.c_str());
            
            // Parameter sliders
            if (ImGui::SliderFloat("X Parameter", &state.blendTreeParam1, -1.0f, 1.0f)) {
                tree2D->setParameter(tree2D->parameterX, state.blendTreeParam1);
            }
            if (ImGui::SliderFloat("Y Parameter", &state.blendTreeParam2, -1.0f, 1.0f)) {
                tree2D->setParameter(tree2D->parameterY, state.blendTreeParam2);
            }
            
            ImGui::Separator();
            
            // 2D Visualization
            ImGui::Text("Blend Space:");
            ImVec2 size = ImVec2(200, 200);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            // Background
            drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                                    IM_COL32(30, 30, 30, 255));
            drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                              IM_COL32(60, 60, 60, 255));
            
            // Grid lines
            drawList->AddLine(ImVec2(pos.x + size.x/2, pos.y), 
                              ImVec2(pos.x + size.x/2, pos.y + size.y), 
                              IM_COL32(60, 60, 60, 255));
            drawList->AddLine(ImVec2(pos.x, pos.y + size.y/2), 
                              ImVec2(pos.x + size.x, pos.y + size.y/2), 
                              IM_COL32(60, 60, 60, 255));
            
            // Motion points
            for (const auto& motion : tree2D->motions) {
                float x = pos.x + (motion.positionX + 1.0f) * 0.5f * size.x;
                float y = pos.y + (1.0f - (motion.positionY + 1.0f) * 0.5f) * size.y;
                drawList->AddCircleFilled(ImVec2(x, y), 6.0f, IM_COL32(100, 150, 255, 255));
            }
            
            // Current position
            float cx = pos.x + (state.blendTreeParam1 + 1.0f) * 0.5f * size.x;
            float cy = pos.y + (1.0f - (state.blendTreeParam2 + 1.0f) * 0.5f) * size.y;
            drawList->AddCircleFilled(ImVec2(cx, cy), 8.0f, IM_COL32(255, 200, 50, 255));
            
            ImGui::Dummy(size);
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No blend tree selected");
        }
    }
    ImGui::End();
}

// ===== IK Settings Panel =====
inline void drawIKSettingsPanel(IKManager* ikManager, Skeleton* skeleton, EditorState& state) {
    if (!state.showIKSettings) return;
    
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("IK Settings", &state.showIKSettings)) {
        
        if (!ikManager) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No IK manager available");
            ImGui::End();
            return;
        }
        
        // Chain list
        if (ImGui::CollapsingHeader("IK Chains", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("IKChains", ImVec2(0, 150), true);
            
            size_t chainCount = ikManager->getChainCount();
            for (size_t i = 0; i < chainCount; i++) {
                ImGui::PushID(static_cast<int>(i));
                
                bool selected = (state.selectedIKChain == static_cast<int>(i));
                std::string label = "Chain " + std::to_string(i);
                
                if (ImGui::Selectable(label.c_str(), selected)) {
                    state.selectedIKChain = static_cast<int>(i);
                }
                
                ImGui::PopID();
            }
            
            ImGui::EndChild();
            
            // Add IK chain
            if (skeleton && ImGui::Button("Add Two-Bone IK")) {
                // Would show bone selection dialog
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Look-At")) {
                // Would show bone selection dialog
            }
        }
        
        ImGui::Separator();
        
        // Selected chain properties
        if (state.selectedIKChain >= 0 && state.selectedIKChain < static_cast<int>(ikManager->getChainCount())) {
            ImGui::Text("Chain %d Properties:", state.selectedIKChain);
            
            // Target position
            static Vec3 targetPos = {0, 0, 0};
            ImGui::DragFloat3("Target", &targetPos.x, 0.1f);
            
            // Weight
            static float weight = 1.0f;
            ImGui::SliderFloat("Weight", &weight, 0.0f, 1.0f);
            
            // Pole target (for two-bone)
            static Vec3 poleTarget = {0, 0, 1};
            ImGui::DragFloat3("Pole Target", &poleTarget.x, 0.1f);
            
            ImGui::Separator();
            
            // Apply button
            if (ImGui::Button("Apply IK")) {
                // Apply IK with current settings
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                // Reset to bind pose
            }
        }
    }
    ImGui::End();
}

// ===== Animation Layers Panel =====
inline void drawAnimationLayersPanel(AnimationLayerManager* layerManager, EditorState& state) {
    if (!state.showAnimationLayers) return;
    
    ImGui::SetNextWindowSize(ImVec2(320, 350), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Animation Layers", &state.showAnimationLayers)) {
        
        if (!layerManager) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No animation layer manager");
            ImGui::End();
            return;
        }
        
        size_t layerCount = layerManager->getLayerCount();
        
        for (size_t i = 0; i < layerCount; i++) {
            auto* layer = layerManager->getLayer(static_cast<int>(i));
            if (!layer) continue;
            
            ImGui::PushID(static_cast<int>(i));
            
            bool open = ImGui::CollapsingHeader(layer->name.c_str(), 
                                                 ImGuiTreeNodeFlags_DefaultOpen);
            
            if (open) {
                ImGui::Indent();
                
                // Weight
                ImGui::SliderFloat("Weight", &layer->weight, 0.0f, 1.0f);
                
                // Blend mode
                const char* blendModes[] = { "Override", "Additive", "Multiply" };
                int blendMode = static_cast<int>(layer->blendMode);
                if (ImGui::Combo("Blend Mode", &blendMode, blendModes, 3)) {
                    layer->blendMode = static_cast<AnimationBlendMode>(blendMode);
                }
                
                // Mask info
                if (layer->mask.isEmpty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Mask: Full Body");
                } else {
                    ImGui::Text("Mask: %zu bones", layer->mask.getBoneCount());
                }
                
                // Clip info
                ImGui::Text("Clip: %s", layer->currentClip ? layer->currentClip->name.c_str() : "None");
                
                ImGui::Unindent();
            }
            
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        // Add layer
        if (ImGui::Button("Add Layer")) {
            layerManager->createLayer("NewLayer");
        }
    }
    ImGui::End();
}

// ===== LOD Settings Panel =====
inline void drawLODSettingsPanel(LODState& lodState, EditorState& editorState) {
    if (!editorState.showLODSettings) return;
    
    ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("LOD Settings", &editorState.showLODSettings)) {
        
        // Quality preset
        const char* presets[] = { "Low", "Medium", "High", "Ultra" };
        int preset = static_cast<int>(lodState.qualityPreset);
        if (ImGui::Combo("Quality Preset", &preset, presets, 4)) {
            lodState.qualityPreset = static_cast<LODQualityPreset>(preset);
        }
        
        ImGui::Separator();
        
        // Manual settings
        ImGui::Text("Manual Settings:");
        ImGui::SliderFloat("LOD Bias", &lodState.lodBias, 0.5f, 2.0f);
        ImGui::Text("(Lower = more detail, Higher = less)");
        
        ImGui::SliderFloat("Max Distance", &lodState.maxDistance, 100.0f, 2000.0f);
        
        ImGui::Separator();
        
        // Debug
        ImGui::Checkbox("Show LOD Debug Colors", &lodState.showLODDebug);
        if (lodState.showLODDebug) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOD 0 - Green");
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "LOD 1 - Yellow");
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "LOD 2 - Orange");
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "LOD 3 - Red");
        }
        
        ImGui::Separator();
        
        // Statistics
        if (ImGui::CollapsingHeader("Statistics")) {
            auto& manager = getLODManager();
            ImGui::Text("Global LOD Bias: %.2f", manager.getGlobalLODBias());
            ImGui::Text("Max LOD Level: %d", manager.getMaxLODLevel());
            // Could add more stats here
        }
    }
    ImGui::End();
}

// ===== Demo Menu =====
inline void drawDemoMenu(SceneGraph& scene, EditorState& state) {
    if (!state.showDemoMenu) return;
    
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Demo Scenes", &state.showDemoMenu)) {
        ImGui::TextWrapped("Select a demo to generate. This will replace the current scene.");
        ImGui::Separator();
        
        auto& demoMode = DemoMode::get();
        auto demos = demoMode.getAvailableDemos();
        
        std::string currentCategory;
        
        for (const auto& demo : demos) {
            // Category header
            if (demo.category != currentCategory) {
                if (!currentCategory.empty()) {
                    ImGui::Separator();
                }
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", demo.category.c_str());
                currentCategory = demo.category;
            }
            
            // Demo button
            ImGui::PushID(demo.id.c_str());
            if (ImGui::Button(demo.name.c_str(), ImVec2(150, 0))) {
                demoMode.generateDemo(demo.id, scene);
                state.consoleLogs.push_back("[INFO] Generated demo: " + demo.name);
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "%s", demo.description.c_str());
            ImGui::PopID();
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1), 
                          "Demos are also available in Help > Demos menu");
    }
    ImGui::End();
}

// ===== Particle Editor State =====
struct ParticleEditorState {
    ParticleSystem* selectedSystem = nullptr;
    int selectedEmitterIndex = -1;
    int selectedPresetIndex = -1;
    
    // Emission shape editor
    int shapeType = 0;
    
    // Color gradient editor
    std::vector<std::pair<float, Vec4>> colorKeys;
    
    // Preview controls
    bool previewPlaying = true;
    float previewSpeed = 1.0f;
};

// ===== Particle Editor Panel =====
inline void drawParticleEditorPanel(ParticleEditorState& particleState, EditorState& state) {
    if (!state.showParticleEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Particle Editor", &state.showParticleEditor)) {
        ImGui::End();
        return;
    }
    
    auto& manager = getParticleManager();
    
    // === System List ===
    if (ImGui::CollapsingHeader("Particle Systems", ImGuiTreeNodeFlags_DefaultOpen)) {
        // System list
        const auto& systems = manager.getSystems();
        for (size_t i = 0; i < systems.size(); i++) {
            bool selected = (particleState.selectedSystem == systems[i].get());
            if (ImGui::Selectable(systems[i]->getName().c_str(), selected)) {
                particleState.selectedSystem = systems[i].get();
                particleState.selectedEmitterIndex = 0;
            }
        }
        
        // Create new system
        ImGui::Separator();
        if (ImGui::Button("+ New System")) {
            particleState.selectedSystem = manager.createSystem("New Particle System");
            particleState.selectedSystem->addEmitter();
            particleState.selectedEmitterIndex = 0;
        }
        ImGui::SameLine();
        
        // Create from preset
        static int presetIdx = 0;
        auto presets = ParticlePresets::getAllPresetNames();
        if (ImGui::Button("+ From Preset")) {
            if (presetIdx >= 0 && presetIdx < (int)presets.size()) {
                particleState.selectedSystem = manager.createSystem(presets[presetIdx].second);
                auto& emitter = particleState.selectedSystem->addEmitter();
                emitter.setSettings(ParticlePresets::getPreset(presets[presetIdx].first));
                particleState.selectedEmitterIndex = 0;
            }
        }
        ImGui::SameLine();
        
        std::vector<const char*> presetNames;
        for (const auto& p : presets) presetNames.push_back(p.second.c_str());
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("##PresetSelect", &presetIdx, presetNames.data(), (int)presetNames.size());
    }
    
    // No system selected
    if (!particleState.selectedSystem) {
        ImGui::Text("Select or create a particle system");
        ImGui::End();
        return;
    }
    
    ParticleSystem* sys = particleState.selectedSystem;
    
    // === Preview Controls ===
    ImGui::Separator();
    if (ImGui::Button(particleState.previewPlaying ? "Pause" : "Play")) {
        particleState.previewPlaying = !particleState.previewPlaying;
    }
    ImGui::SameLine();
    if (ImGui::Button("Restart")) {
        sys->play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        sys->stop(true);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Speed", &particleState.previewSpeed, 0.1f, 3.0f);
    
    // Stats
    ImGui::Text("Particles: %zu", sys->getTotalParticleCount());
    
    // === Emitter Tabs ===
    ImGui::Separator();
    if (ImGui::BeginTabBar("EmitterTabs")) {
        for (size_t i = 0; i < sys->getEmitterCount(); i++) {
            char tabName[32];
            snprintf(tabName, sizeof(tabName), "Emitter %zu", i + 1);
            
            if (ImGui::BeginTabItem(tabName)) {
                particleState.selectedEmitterIndex = (int)i;
                ImGui::EndTabItem();
            }
        }
        
        // Add emitter button
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing)) {
            sys->addEmitter();
        }
        
        ImGui::EndTabBar();
    }
    
    // === Emitter Settings ===
    if (particleState.selectedEmitterIndex >= 0 && 
        particleState.selectedEmitterIndex < (int)sys->getEmitterCount()) {
        
        ParticleEmitter* emitter = sys->getEmitter(particleState.selectedEmitterIndex);
        ParticleEmitterSettings& settings = emitter->getSettings();
        
        // --- Emission ---
        if (ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Rate", &settings.emissionRate, 0.0f, 1000.0f);
            ImGui::SliderInt("Max Particles", &settings.maxParticles, 1, 10000);
            ImGui::Checkbox("Looping", &settings.looping);
            if (!settings.looping) {
                ImGui::SliderFloat("Duration", &settings.duration, 0.1f, 30.0f);
            }
            ImGui::SliderFloat("Start Delay", &settings.startDelay, 0.0f, 5.0f);
            
            // Bursts
            ImGui::Separator();
            ImGui::Text("Bursts");
            for (size_t bi = 0; bi < settings.bursts.size(); bi++) {
                auto& burst = settings.bursts[bi];
                ImGui::PushID((int)bi);
                
                ImGui::SliderFloat("Time", &burst.time, 0.0f, settings.duration);
                ImGui::SliderInt("Min Count", &burst.minCount, 1, 500);
                ImGui::SliderInt("Max Count", &burst.maxCount, burst.minCount, 500);
                ImGui::SliderInt("Cycles", &burst.cycles, -1, 10);
                if (burst.cycles != 1) {
                    ImGui::SliderFloat("Interval", &burst.interval, 0.1f, 5.0f);
                }
                
                if (ImGui::Button("Remove")) {
                    settings.bursts.erase(settings.bursts.begin() + bi);
                    bi--;
                }
                ImGui::PopID();
                ImGui::Separator();
            }
            if (ImGui::Button("+ Add Burst")) {
                ParticleBurst newBurst;
                newBurst.minCount = 10;
                newBurst.maxCount = 20;
                settings.bursts.push_back(newBurst);
            }
        }
        
        // --- Shape ---
        if (ImGui::CollapsingHeader("Shape")) {
            const char* shapes[] = { "Point", "Sphere", "Hemisphere", "Cone", "Box", "Circle", "Edge" };
            int shapeIdx = static_cast<int>(settings.shape.shape);
            if (ImGui::Combo("Shape", &shapeIdx, shapes, 7)) {
                settings.shape.shape = static_cast<EmissionShape>(shapeIdx);
            }
            
            switch (settings.shape.shape) {
                case EmissionShape::Sphere:
                case EmissionShape::Hemisphere:
                    ImGui::SliderFloat("Radius", &settings.shape.radius, 0.01f, 10.0f);
                    ImGui::SliderFloat("Thickness", &settings.shape.radiusThickness, 0.0f, 1.0f);
                    break;
                    
                case EmissionShape::Cone:
                    ImGui::SliderFloat("Angle", &settings.shape.coneAngle, 0.0f, 90.0f);
                    ImGui::SliderFloat("Radius", &settings.shape.coneRadius, 0.01f, 5.0f);
                    ImGui::SliderFloat("Length", &settings.shape.coneLength, 0.1f, 10.0f);
                    break;
                    
                case EmissionShape::Box:
                    ImGui::DragFloat3("Size", &settings.shape.boxSize.x, 0.1f, 0.01f, 100.0f);
                    break;
                    
                case EmissionShape::Circle:
                    ImGui::SliderFloat("Radius", &settings.shape.radius, 0.01f, 10.0f);
                    ImGui::SliderFloat("Arc", &settings.shape.arcAngle, 0.0f, 360.0f);
                    break;
                    
                case EmissionShape::Edge:
                    ImGui::SliderFloat("Length", &settings.shape.radius, 0.1f, 10.0f);
                    break;
                    
                default:
                    break;
            }
            
            ImGui::Checkbox("Randomize Direction", &settings.shape.randomizeDirection);
            ImGui::SliderFloat("Direction Spread", &settings.shape.directionalSpread, 0.0f, 1.0f);
        }
        
        // --- Lifetime ---
        if (ImGui::CollapsingHeader("Lifetime")) {
            ImGui::SliderFloat("Life Min", &settings.startLife.min, 0.1f, 20.0f);
            ImGui::SliderFloat("Life Max", &settings.startLife.max, settings.startLife.min, 20.0f);
        }
        
        // --- Velocity ---
        if (ImGui::CollapsingHeader("Velocity")) {
            ImGui::SliderFloat("Speed Min", &settings.startSpeed.min, 0.0f, 50.0f);
            ImGui::SliderFloat("Speed Max", &settings.startSpeed.max, settings.startSpeed.min, 50.0f);
            
            ImGui::Separator();
            ImGui::Text("Physics");
            ImGui::SliderFloat("Gravity", &settings.gravityMultiplier, -2.0f, 2.0f);
            ImGui::SliderFloat("Drag", &settings.drag, 0.0f, 5.0f);
        }
        
        // --- Size ---
        if (ImGui::CollapsingHeader("Size")) {
            ImGui::SliderFloat("Start Size Min", &settings.startSize.min, 0.01f, 5.0f);
            ImGui::SliderFloat("Start Size Max", &settings.startSize.max, settings.startSize.min, 5.0f);
            ImGui::SliderFloat("End Size Min", &settings.endSize.min, 0.0f, 5.0f);
            ImGui::SliderFloat("End Size Max", &settings.endSize.max, settings.endSize.min, 5.0f);
        }
        
        // --- Color ---
        if (ImGui::CollapsingHeader("Color")) {
            ImGui::ColorEdit4("Start Color", &settings.startColor.x);
            ImGui::ColorEdit4("End Color", &settings.endColor.x);
            ImGui::Checkbox("Use Gradient", &settings.useColorGradient);
        }
        
        // --- Rotation ---
        if (ImGui::CollapsingHeader("Rotation")) {
            ImGui::SliderFloat("Start Rotation Min", &settings.startRotation.min, 0.0f, 360.0f);
            ImGui::SliderFloat("Start Rotation Max", &settings.startRotation.max, settings.startRotation.min, 360.0f);
            ImGui::SliderFloat("Angular Velocity Min", &settings.angularVelocity.min, -360.0f, 360.0f);
            ImGui::SliderFloat("Angular Velocity Max", &settings.angularVelocity.max, settings.angularVelocity.min, 360.0f);
        }
        
        // --- Rendering ---
        if (ImGui::CollapsingHeader("Rendering")) {
            ImGui::Checkbox("Billboard", &settings.billboard);
            ImGui::Checkbox("Stretch with Velocity", &settings.stretchWithVelocity);
            if (settings.stretchWithVelocity) {
                ImGui::SliderFloat("Stretch Amount", &settings.velocityStretch, 0.0f, 2.0f);
            }
            
            const char* sortModes[] = { "None", "By Distance", "By Age" };
            ImGui::Combo("Sort Mode", &settings.sortMode, sortModes, 3);
            
            // Texture sheet
            ImGui::Separator();
            ImGui::Text("Texture Sheet");
            ImGui::SliderInt("Rows", &settings.textureRows, 1, 16);
            ImGui::SliderInt("Columns", &settings.textureCols, 1, 16);
            ImGui::Checkbox("Animate", &settings.animateTexture);
            if (settings.animateTexture) {
                ImGui::SliderFloat("Anim Speed", &settings.textureAnimSpeed, 1.0f, 60.0f);
            }
        }
        
        // --- World Space ---
        ImGui::Separator();
        ImGui::Checkbox("World Space", &settings.worldSpace);
        
        // Apply preset to this emitter
        ImGui::Separator();
        if (ImGui::Button("Apply Preset...")) {
            ImGui::OpenPopup("ApplyPresetPopup");
        }
        
        if (ImGui::BeginPopup("ApplyPresetPopup")) {
            auto presets = ParticlePresets::getAllPresetNames();
            for (const auto& preset : presets) {
                if (ImGui::MenuItem(preset.second.c_str())) {
                    emitter->setSettings(ParticlePresets::getPreset(preset.first));
                }
            }
            ImGui::EndPopup();
        }
        
        // Delete emitter
        ImGui::SameLine();
        if (sys->getEmitterCount() > 1) {
            if (ImGui::Button("Delete Emitter")) {
                sys->removeEmitter(particleState.selectedEmitterIndex);
                particleState.selectedEmitterIndex = std::max(0, particleState.selectedEmitterIndex - 1);
            }
        }
    }
    
    // Delete system
    ImGui::Separator();
    if (ImGui::Button("Delete System")) {
        manager.destroySystem(sys);
        particleState.selectedSystem = nullptr;
        particleState.selectedEmitterIndex = -1;
    }
    
    ImGui::End();
}

// ===== Physics Editor State =====
struct PhysicsEditorState {
    RigidBody* selectedBody = nullptr;
    Constraint* selectedConstraint = nullptr;
    
    // Creation mode
    int createBodyType = 1;  // 0=Static, 1=Dynamic, 2=Kinematic
    int createColliderType = 1;  // 0=Sphere, 1=Box, 2=Capsule
    
    // Debug visualization
    bool showColliders = true;
    bool showAABBs = false;
    bool showContacts = true;
    bool showConstraints = true;
    bool showVelocities = false;
    float velocityScale = 0.2f;
    
    // Simulation control
    bool simulationPaused = false;
    float timeScale = 1.0f;
    
    // Raycast testing
    bool raycastTestMode = false;
    Vec3 raycastOrigin = {0, 5, 0};
    Vec3 raycastDirection = {0, -1, 0};
    float raycastDistance = 100.0f;
    RaycastHit lastRaycastHit;
    
    // Debug lines storage (for rendering)
    std::vector<float> debugLineData;
    size_t debugLineCount = 0;
};

// ===== Physics Editor Panel =====
inline void drawPhysicsEditorPanel(PhysicsEditorState& physicsState, EditorState& state) {
    if (!state.showPhysicsEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 550), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Physics Editor", &state.showPhysicsEditor)) {
        ImGui::End();
        return;
    }
    
    auto& world = getPhysicsWorld();
    auto& constraints = getConstraintManager();
    
    // === Simulation Control ===
    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button(physicsState.simulationPaused ? "Resume" : "Pause")) {
            physicsState.simulationPaused = !physicsState.simulationPaused;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            world.step(1.0f / 60.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            world.clear();
            constraints.clear();
            physicsState.selectedBody = nullptr;
            physicsState.selectedConstraint = nullptr;
        }
        
        ImGui::SliderFloat("Time Scale", &physicsState.timeScale, 0.0f, 2.0f);
        
        // Stats
        ImGui::Text("Bodies: %zu", world.getBodyCount());
        ImGui::Text("Contacts: %zu", world.getCollisions().size());
        ImGui::Text("Constraints: %zu", constraints.getConstraints().size());
    }
    
    // === World Settings ===
    if (ImGui::CollapsingHeader("World Settings")) {
        auto& settings = world.getSettings();
        
        ImGui::DragFloat3("Gravity", &settings.gravity.x, 0.1f);
        ImGui::SliderInt("Velocity Iterations", &settings.velocityIterations, 1, 20);
        ImGui::SliderInt("Position Iterations", &settings.positionIterations, 1, 10);
        ImGui::SliderFloat("Fixed Timestep", &settings.fixedTimeStep, 0.001f, 0.033f, "%.4f");
        ImGui::Checkbox("Enable Sleeping", &settings.enableSleeping);
        if (settings.enableSleeping) {
            ImGui::SliderFloat("Sleep Threshold", &settings.sleepThreshold, 0.001f, 0.1f);
            ImGui::SliderFloat("Sleep Time", &settings.sleepTime, 0.1f, 2.0f);
        }
        ImGui::SliderFloat("Default Friction", &settings.defaultFriction, 0.0f, 1.0f);
        ImGui::SliderFloat("Default Restitution", &settings.defaultRestitution, 0.0f, 1.0f);
    }
    
    // === Debug Visualization ===
    if (ImGui::CollapsingHeader("Debug Visualization")) {
        auto& debugRenderer = getPhysicsDebugRenderer();
        
        if (ImGui::Checkbox("Show Colliders", &physicsState.showColliders)) {
            debugRenderer.setDrawColliders(physicsState.showColliders);
        }
        if (ImGui::Checkbox("Show AABBs", &physicsState.showAABBs)) {
            debugRenderer.setDrawAABBs(physicsState.showAABBs);
        }
        if (ImGui::Checkbox("Show Contacts", &physicsState.showContacts)) {
            debugRenderer.setDrawContacts(physicsState.showContacts);
        }
        if (ImGui::Checkbox("Show Constraints", &physicsState.showConstraints)) {
            debugRenderer.setDrawConstraints(physicsState.showConstraints);
        }
        if (ImGui::Checkbox("Show Velocities", &physicsState.showVelocities)) {
            debugRenderer.setDrawVelocities(physicsState.showVelocities);
        }
        if (physicsState.showVelocities) {
            if (ImGui::SliderFloat("Velocity Scale", &physicsState.velocityScale, 0.05f, 1.0f)) {
                debugRenderer.setVelocityScale(physicsState.velocityScale);
            }
        }
        
        ImGui::Text("Debug Lines: %zu", physicsState.debugLineCount);
    }
    
    // === Raycast Testing ===
    if (ImGui::CollapsingHeader("Raycast Testing")) {
        ImGui::Checkbox("Enable Raycast Test", &physicsState.raycastTestMode);
        
        if (physicsState.raycastTestMode) {
            ImGui::DragFloat3("Origin", &physicsState.raycastOrigin.x, 0.1f);
            ImGui::DragFloat3("Direction", &physicsState.raycastDirection.x, 0.01f);
            ImGui::DragFloat("Max Distance", &physicsState.raycastDistance, 1.0f, 0.1f, 1000.0f);
            
            if (ImGui::Button("Cast Ray")) {
                RaycastOptions options;
                options.maxDistance = physicsState.raycastDistance;
                physicsState.lastRaycastHit = PhysicsRaycaster::raycast(
                    world, 
                    Ray(physicsState.raycastOrigin, physicsState.raycastDirection.normalized()),
                    options
                );
            }
            
            ImGui::Separator();
            if (physicsState.lastRaycastHit.hit) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "HIT!");
                ImGui::Text("Distance: %.3f", physicsState.lastRaycastHit.distance);
                ImGui::Text("Point: (%.2f, %.2f, %.2f)", 
                    physicsState.lastRaycastHit.point.x,
                    physicsState.lastRaycastHit.point.y,
                    physicsState.lastRaycastHit.point.z);
                ImGui::Text("Normal: (%.2f, %.2f, %.2f)",
                    physicsState.lastRaycastHit.normal.x,
                    physicsState.lastRaycastHit.normal.y,
                    physicsState.lastRaycastHit.normal.z);
                if (physicsState.lastRaycastHit.body) {
                    ImGui::Text("Body ID: %u", physicsState.lastRaycastHit.body->getId());
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No hit");
            }
        }
    }
    
    // === Create Body ===
    if (ImGui::CollapsingHeader("Create Body")) {
        const char* bodyTypes[] = { "Static", "Dynamic", "Kinematic" };
        ImGui::Combo("Body Type", &physicsState.createBodyType, bodyTypes, 3);
        
        const char* colliderTypes[] = { "Sphere", "Box", "Capsule", "Plane" };
        ImGui::Combo("Collider Type", &physicsState.createColliderType, colliderTypes, 4);
        
        if (ImGui::Button("Create Body")) {
            RigidBodyType type = static_cast<RigidBodyType>(physicsState.createBodyType);
            RigidBody* body = world.createBody(type);
            
            auto collider = std::make_shared<Collider>(static_cast<ColliderType>(physicsState.createColliderType));
            
            switch (physicsState.createColliderType) {
                case 0: // Sphere
                    collider->asSphere().radius = 0.5f;
                    break;
                case 1: // Box
                    collider->asBox().halfExtents = Vec3(0.5f, 0.5f, 0.5f);
                    break;
                case 2: // Capsule
                    collider->asCapsule().radius = 0.25f;
                    collider->asCapsule().height = 1.0f;
                    break;
                case 3: // Plane
                    collider->asPlane().normal = Vec3(0, 1, 0);
                    collider->asPlane().distance = 0.0f;
                    body->setType(RigidBodyType::Static);
                    break;
            }
            
            body->setCollider(collider);
            body->setPosition(Vec3(0, 5, 0));
            physicsState.selectedBody = body;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Create Ground")) {
            RigidBody* ground = world.createBody(RigidBodyType::Static);
            auto collider = std::make_shared<Collider>(ColliderType::Box);
            collider->asBox().halfExtents = Vec3(10.0f, 0.5f, 10.0f);
            ground->setCollider(collider);
            ground->setPosition(Vec3(0, -0.5f, 0));
        }
    }
    
    // === Body List ===
    if (ImGui::CollapsingHeader("Bodies", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& bodies = world.getBodies();
        
        for (size_t i = 0; i < bodies.size(); i++) {
            RigidBody* body = bodies[i].get();
            
            char label[64];
            const char* typeStr = "?";
            switch (body->getType()) {
                case RigidBodyType::Static: typeStr = "S"; break;
                case RigidBodyType::Dynamic: typeStr = "D"; break;
                case RigidBodyType::Kinematic: typeStr = "K"; break;
            }
            snprintf(label, sizeof(label), "[%s] Body %u%s", typeStr, body->getId(),
                     body->isSleeping() ? " (zzz)" : "");
            
            bool selected = (physicsState.selectedBody == body);
            if (ImGui::Selectable(label, selected)) {
                physicsState.selectedBody = body;
            }
        }
    }
    
    // === Selected Body Inspector ===
    if (physicsState.selectedBody) {
        RigidBody* body = physicsState.selectedBody;
        
        ImGui::Separator();
        ImGui::Text("Selected Body: %u", body->getId());
        
        // Type
        const char* types[] = { "Static", "Dynamic", "Kinematic" };
        int currentType = static_cast<int>(body->getType());
        if (ImGui::Combo("Type", &currentType, types, 3)) {
            body->setType(static_cast<RigidBodyType>(currentType));
        }
        
        // Transform
        Vec3 pos = body->getPosition();
        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            body->setPosition(pos);
            body->wakeUp();
        }
        
        // Mass
        if (body->getType() == RigidBodyType::Dynamic) {
            float mass = body->getMass();
            if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.01f, 1000.0f)) {
                body->setMass(mass);
            }
        }
        
        // Velocity
        if (body->getType() != RigidBodyType::Static) {
            Vec3 linVel = body->getLinearVelocity();
            if (ImGui::DragFloat3("Linear Velocity", &linVel.x, 0.1f)) {
                body->setLinearVelocity(linVel);
            }
            
            Vec3 angVel = body->getAngularVelocity();
            if (ImGui::DragFloat3("Angular Velocity", &angVel.x, 0.1f)) {
                body->setAngularVelocity(angVel);
            }
        }
        
        // Material
        float restitution = body->getRestitution();
        if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f)) {
            body->setRestitution(restitution);
        }
        
        float friction = body->getFriction();
        if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f)) {
            body->setFriction(friction);
        }
        
        // Damping
        float linDamp = body->getLinearDamping();
        if (ImGui::SliderFloat("Linear Damping", &linDamp, 0.0f, 1.0f)) {
            body->setLinearDamping(linDamp);
        }
        
        float angDamp = body->getAngularDamping();
        if (ImGui::SliderFloat("Angular Damping", &angDamp, 0.0f, 1.0f)) {
            body->setAngularDamping(angDamp);
        }
        
        // Collider
        if (body->getCollider()) {
            Collider* col = body->getCollider();
            ImGui::Separator();
            
            const char* shapeTypes[] = { "Sphere", "Box", "Capsule", "Plane", "Mesh", "Compound" };
            ImGui::Text("Collider: %s", shapeTypes[static_cast<int>(col->getType())]);
            
            switch (col->getType()) {
                case ColliderType::Sphere:
                    ImGui::DragFloat("Radius", &col->asSphere().radius, 0.01f, 0.01f, 100.0f);
                    break;
                case ColliderType::Box:
                    ImGui::DragFloat3("Half Extents", &col->asBox().halfExtents.x, 0.01f, 0.01f, 100.0f);
                    break;
                case ColliderType::Capsule:
                    ImGui::DragFloat("Radius##cap", &col->asCapsule().radius, 0.01f, 0.01f, 10.0f);
                    ImGui::DragFloat("Height", &col->asCapsule().height, 0.01f, 0.01f, 10.0f);
                    break;
                default:
                    break;
            }
            
            bool isTrigger = col->isTrigger();
            if (ImGui::Checkbox("Is Trigger", &isTrigger)) {
                col->setTrigger(isTrigger);
            }
        }
        
        // Actions
        ImGui::Separator();
        if (ImGui::Button("Apply Impulse Up")) {
            body->addImpulse(Vec3(0, 10, 0));
        }
        ImGui::SameLine();
        if (ImGui::Button("Wake Up")) {
            body->wakeUp();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            world.destroyBody(body);
            physicsState.selectedBody = nullptr;
        }
    }
    
    // === Constraints ===
    if (ImGui::CollapsingHeader("Constraints")) {
        const auto& constraintList = constraints.getConstraints();
        
        for (size_t i = 0; i < constraintList.size(); i++) {
            Constraint* c = constraintList[i].get();
            
            const char* typeStr = "?";
            switch (c->getType()) {
                case ConstraintType::Distance: typeStr = "Distance"; break;
                case ConstraintType::BallSocket: typeStr = "BallSocket"; break;
                case ConstraintType::Hinge: typeStr = "Hinge"; break;
                case ConstraintType::Slider: typeStr = "Slider"; break;
                case ConstraintType::Fixed: typeStr = "Fixed"; break;
                case ConstraintType::Spring: typeStr = "Spring"; break;
                case ConstraintType::Cone: typeStr = "Cone"; break;
            }
            
            char label[64];
            snprintf(label, sizeof(label), "%s (%u-%u)%s", typeStr, 
                     c->getBodyA()->getId(), c->getBodyB()->getId(),
                     c->isBroken() ? " [BROKEN]" : "");
            
            bool selected = (physicsState.selectedConstraint == c);
            if (ImGui::Selectable(label, selected)) {
                physicsState.selectedConstraint = c;
            }
        }
        
        // Create constraint button
        if (physicsState.selectedBody && world.getBodyCount() > 1) {
            if (ImGui::Button("Create Distance Constraint")) {
                // Find another body
                for (const auto& b : world.getBodies()) {
                    if (b.get() != physicsState.selectedBody) {
                        constraints.createConstraint<DistanceConstraint>(
                            physicsState.selectedBody, b.get(),
                            Vec3(0, 0, 0), Vec3(0, 0, 0)
                        );
                        break;
                    }
                }
            }
            
            if (ImGui::Button("Create Spring Constraint")) {
                for (const auto& b : world.getBodies()) {
                    if (b.get() != physicsState.selectedBody) {
                        constraints.createConstraint<SpringConstraint>(
                            physicsState.selectedBody, b.get(),
                            Vec3(0, 0, 0), Vec3(0, 0, 0),
                            -1.0f, 100.0f, 10.0f
                        );
                        break;
                    }
                }
            }
        }
    }
    
    ImGui::End();
}

// ===== Terrain Editor State =====
struct TerrainEditorState {
    // Generation settings
    FractalNoiseSettings noiseSettings;
    ErosionSettings erosionSettings;
    int selectedPreset = 1;  // Hills
    int selectedErosionPreset = 1;  // Medium
    bool applyErosion = true;
    uint32_t seed = 12345;
    
    // Terrain settings
    int heightmapResolution = 257;
    float terrainSize = 256.0f;
    float heightScale = 50.0f;
    
    // Brush settings
    int brushMode = 0;  // 0=raise, 1=lower, 2=smooth, 3=flatten, 4=paint
    float brushRadius = 5.0f;
    float brushStrength = 0.5f;
    int paintLayer = 0;
    
    // Foliage
    int selectedFoliageLayer = -1;
    bool showFoliageSettings = false;
    
    // State
    bool terrainInitialized = false;
    bool needsRebuild = false;
};

// ===== Terrain Editor Panel =====
inline void drawTerrainEditorPanel(TerrainEditorState& terrainState, EditorState& state) {
    if (!state.showTerrainEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Terrain Editor", &state.showTerrainEditor)) {
        ImGui::End();
        return;
    }
    
    auto& terrain = getTerrain();
    auto& generator = getTerrainGenerator();
    auto& foliage = getFoliageSystem();
    
    // === Terrain Generation ===
    if (ImGui::CollapsingHeader("Generation", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Presets
        const char* presets[] = { "Flat", "Hills", "Mountains", "Islands", "Canyon" };
        if (ImGui::Combo("Preset", &terrainState.selectedPreset, presets, 5)) {
            switch (terrainState.selectedPreset) {
                case 0: terrainState.noiseSettings = TerrainGenerator::presetFlat(); break;
                case 1: terrainState.noiseSettings = TerrainGenerator::presetHills(); break;
                case 2: terrainState.noiseSettings = TerrainGenerator::presetMountains(); break;
                case 3: terrainState.noiseSettings = TerrainGenerator::presetIslands(); break;
                case 4: terrainState.noiseSettings = TerrainGenerator::presetCanyon(); break;
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Noise Settings");
        ImGui::SliderInt("Octaves", &terrainState.noiseSettings.octaves, 1, 10);
        ImGui::SliderFloat("Frequency", &terrainState.noiseSettings.frequency, 0.001f, 0.02f, "%.4f");
        ImGui::SliderFloat("Amplitude", &terrainState.noiseSettings.amplitude, 0.1f, 2.0f);
        ImGui::SliderFloat("Lacunarity", &terrainState.noiseSettings.lacunarity, 1.5f, 3.0f);
        ImGui::SliderFloat("Persistence", &terrainState.noiseSettings.persistence, 0.2f, 0.8f);
        ImGui::SliderFloat("Exponent", &terrainState.noiseSettings.exponent, 0.5f, 3.0f);
        ImGui::Checkbox("Ridged Noise", &terrainState.noiseSettings.ridged);
        if (terrainState.noiseSettings.ridged) {
            ImGui::SliderFloat("Ridge Offset", &terrainState.noiseSettings.ridgeOffset, 0.5f, 1.5f);
        }
        
        ImGui::Separator();
        ImGui::Checkbox("Apply Erosion", &terrainState.applyErosion);
        if (terrainState.applyErosion) {
            const char* erosionPresets[] = { "Light", "Medium", "Heavy" };
            if (ImGui::Combo("Erosion Preset", &terrainState.selectedErosionPreset, erosionPresets, 3)) {
                switch (terrainState.selectedErosionPreset) {
                    case 0: terrainState.erosionSettings = TerrainGenerator::erosionLight(); break;
                    case 1: terrainState.erosionSettings = TerrainGenerator::erosionMedium(); break;
                    case 2: terrainState.erosionSettings = TerrainGenerator::erosionHeavy(); break;
                }
            }
            ImGui::SliderInt("Iterations", &terrainState.erosionSettings.iterations, 1000, 200000);
        }
        
        ImGui::Separator();
        ImGui::DragInt("Seed", (int*)&terrainState.seed, 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Random")) {
            terrainState.seed = (uint32_t)time(nullptr);
        }
        
        if (ImGui::Button("Generate Terrain", ImVec2(-1, 30))) {
            // Initialize terrain if needed
            if (!terrainState.terrainInitialized) {
                TerrainSettings settings;
                settings.heightmapResolution = terrainState.heightmapResolution;
                settings.terrainSize = terrainState.terrainSize;
                settings.heightScale = terrainState.heightScale;
                terrain.initialize(settings);
                foliage.initialize(settings.terrainSize, 16);
                terrainState.terrainInitialized = true;
            }
            
            generator.setSeed(terrainState.seed);
            generator.generate(terrain, terrainState.noiseSettings, 
                              terrainState.erosionSettings, terrainState.applyErosion);
            terrainState.needsRebuild = true;
        }
    }
    
    // === Terrain Settings ===
    if (ImGui::CollapsingHeader("Terrain Settings")) {
        bool changed = false;
        
        const char* resolutions[] = { "129", "257", "513", "1025" };
        int resIdx = 0;
        if (terrainState.heightmapResolution == 129) resIdx = 0;
        else if (terrainState.heightmapResolution == 257) resIdx = 1;
        else if (terrainState.heightmapResolution == 513) resIdx = 2;
        else if (terrainState.heightmapResolution == 1025) resIdx = 3;
        
        if (ImGui::Combo("Resolution", &resIdx, resolutions, 4)) {
            const int resValues[] = { 129, 257, 513, 1025 };
            terrainState.heightmapResolution = resValues[resIdx];
            changed = true;
        }
        
        changed |= ImGui::DragFloat("Size", &terrainState.terrainSize, 1.0f, 64.0f, 1024.0f);
        changed |= ImGui::DragFloat("Height Scale", &terrainState.heightScale, 1.0f, 10.0f, 200.0f);
        
        if (changed) {
            terrainState.terrainInitialized = false;  // Need to re-initialize
        }
        
        // Stats
        if (terrainState.terrainInitialized) {
            ImGui::Separator();
            ImGui::Text("Chunks: %zu", terrain.getChunkCount());
            float minH, maxH;
            terrain.getHeightmap().getMinMax(minH, maxH);
            ImGui::Text("Height Range: %.2f - %.2f", minH * terrainState.heightScale, maxH * terrainState.heightScale);
        }
    }
    
    // === Material Layers ===
    if (ImGui::CollapsingHeader("Material Layers")) {
        if (terrainState.terrainInitialized) {
            auto& settings = terrain.getSettings();
            
            for (size_t i = 0; i < settings.layers.size(); i++) {
                auto& layer = settings.layers[i];
                ImGui::PushID((int)i);
                
                if (ImGui::TreeNode(layer.name.c_str())) {
                    ImGui::ColorEdit3("Tint", &layer.tint.x);
                    ImGui::SliderFloat("Metallic", &layer.metallic, 0.0f, 1.0f);
                    ImGui::SliderFloat("Roughness", &layer.roughness, 0.0f, 1.0f);
                    ImGui::SliderFloat("Tile Scale", &layer.tileScale, 1.0f, 50.0f);
                    
                    ImGui::Separator();
                    ImGui::Text("Height Blend");
                    ImGui::SliderFloat("Min Height", &layer.minHeight, 0.0f, 1.0f);
                    ImGui::SliderFloat("Max Height", &layer.maxHeight, 0.0f, 1.0f);
                    ImGui::SliderFloat("Blend", &layer.blendSharpness, 0.1f, 5.0f);
                    
                    ImGui::Separator();
                    ImGui::Text("Slope Blend");
                    ImGui::SliderFloat("Min Slope", &layer.minSlope, 0.0f, 1.0f);
                    ImGui::SliderFloat("Max Slope", &layer.maxSlope, 0.0f, 1.0f);
                    ImGui::SliderFloat("Slope Blend", &layer.slopeBlendSharpness, 0.1f, 5.0f);
                    
                    ImGui::TreePop();
                }
                
                ImGui::PopID();
            }
            
            if (ImGui::Button("Regenerate Splatmap")) {
                terrain.autoGenerateSplatmap();
                terrainState.needsRebuild = true;
            }
        } else {
            ImGui::TextDisabled("Generate terrain first");
        }
    }
    
    // === Foliage ===
    if (ImGui::CollapsingHeader("Foliage")) {
        if (terrainState.terrainInitialized) {
            // Layer list
            for (size_t i = 0; i < foliage.getLayerCount(); i++) {
                auto& layer = foliage.getLayers()[i];
                bool selected = (terrainState.selectedFoliageLayer == (int)i);
                
                char label[64];
                snprintf(label, sizeof(label), "%s (%zu instances)", 
                        layer->getSettings().name.c_str(), layer->getTotalInstances());
                
                if (ImGui::Selectable(label, selected)) {
                    terrainState.selectedFoliageLayer = (int)i;
                }
            }
            
            ImGui::Separator();
            
            // Add foliage buttons
            if (ImGui::Button("+ Grass")) {
                foliage.addLayer(FoliageSystem::presetGrass());
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Tall Grass")) {
                foliage.addLayer(FoliageSystem::presetTallGrass());
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Flowers")) {
                foliage.addLayer(FoliageSystem::presetFlowers());
            }
            
            if (ImGui::Button("+ Trees")) {
                foliage.addLayer(FoliageSystem::presetTrees());
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Rocks")) {
                foliage.addLayer(FoliageSystem::presetRocks());
            }
            
            // Selected layer settings
            if (terrainState.selectedFoliageLayer >= 0 && 
                terrainState.selectedFoliageLayer < (int)foliage.getLayerCount()) {
                
                auto& layer = foliage.getLayers()[terrainState.selectedFoliageLayer];
                auto& settings = layer->getSettings();
                
                ImGui::Separator();
                ImGui::Text("Layer: %s", settings.name.c_str());
                
                ImGui::SliderFloat("Density", &settings.density, 0.1f, 50.0f);
                ImGui::SliderFloat("Min Scale", &settings.minScale, 0.1f, 2.0f);
                ImGui::SliderFloat("Max Scale", &settings.maxScale, 0.1f, 3.0f);
                ImGui::ColorEdit3("Base Color", &settings.baseColor.x);
                ImGui::SliderFloat("Wind Strength", &settings.windStrength, 0.0f, 2.0f);
                ImGui::SliderFloat("Cull Distance", &settings.cullDistance, 50.0f, 500.0f);
                
                ImGui::Separator();
                ImGui::Text("Placement");
                ImGui::SliderFloat("Min Height##fol", &settings.minHeight, 0.0f, 1.0f);
                ImGui::SliderFloat("Max Height##fol", &settings.maxHeight, 0.0f, 1.0f);
                ImGui::SliderFloat("Max Slope##fol", &settings.maxSlope, 0.0f, 1.0f);
                
                if (ImGui::Button("Remove Layer")) {
                    foliage.removeLayer(terrainState.selectedFoliageLayer);
                    terrainState.selectedFoliageLayer = -1;
                }
            }
            
            ImGui::Separator();
            if (ImGui::Button("Generate All Foliage", ImVec2(-1, 25))) {
                foliage.generateAll(terrain, terrainState.seed);
            }
            
            ImGui::Text("Total: %zu  Visible: %zu", 
                       foliage.getTotalInstances(), foliage.getVisibleInstances());
        } else {
            ImGui::TextDisabled("Generate terrain first");
        }
    }
    
    // === Brush Tools (placeholder) ===
    if (ImGui::CollapsingHeader("Brush Tools")) {
        const char* brushModes[] = { "Raise", "Lower", "Smooth", "Flatten", "Paint Layer" };
        ImGui::Combo("Mode", &terrainState.brushMode, brushModes, 5);
        ImGui::SliderFloat("Radius", &terrainState.brushRadius, 1.0f, 50.0f);
        ImGui::SliderFloat("Strength", &terrainState.brushStrength, 0.01f, 1.0f);
        
        if (terrainState.brushMode == 4) {
            ImGui::SliderInt("Paint Layer", &terrainState.paintLayer, 0, 3);
        }
        
        ImGui::TextDisabled("Click and drag on terrain to sculpt");
    }
    
    ImGui::End();
}

// ===== Audio Editor State =====
struct AudioEditorState {
    // Selected items
    int selectedSourceIndex = -1;
    int selectedClipIndex = -1;
    int selectedMixerGroup = 0;
    
    // Test tone
    bool testToneEnabled = false;
    float testToneFrequency = 440.0f;
    float testToneDuration = 1.0f;
    
    // Source creation
    float newSourcePosition[3] = {0, 0, 0};
    float newSourceVolume = 1.0f;
    bool newSourceLoop = false;
    
    // Visualization
    bool showSourceGizmos = true;
    bool showListenerGizmo = true;
    
    // Recording (placeholder)
    bool isRecording = false;
};

// ===== Audio Editor Panel =====
inline void drawAudioEditorPanel(AudioEditorState& audioState, EditorState& state) {
    if (!state.showAudioEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 550), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Audio Editor", &state.showAudioEditor)) {
        ImGui::End();
        return;
    }
    
    auto& audioSystem = getAudioSystem();
    
    // Initialize if needed
    if (!audioSystem.isInitialized()) {
        audioSystem.initialize();
    }
    
    // === Master Controls ===
    if (ImGui::CollapsingHeader("Master", ImGuiTreeNodeFlags_DefaultOpen)) {
        float masterVol = audioSystem.getMasterVolume();
        if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f)) {
            audioSystem.setMasterVolume(masterVol);
        }
        
        bool muted = audioSystem.isMuted();
        if (ImGui::Checkbox("Mute", &muted)) {
            audioSystem.setMuted(muted);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Stop All")) {
            audioSystem.stopAll();
        }
        
        ImGui::Text("Playing: %zu sources", audioSystem.getPlayingCount());
    }
    
    // === Listener ===
    if (ImGui::CollapsingHeader("Listener")) {
        auto& listener = audioSystem.getListener();
        
        Vec3 pos = listener.getPosition();
        float posArr[3] = {pos.x, pos.y, pos.z};
        if (ImGui::DragFloat3("Position", posArr, 0.1f)) {
            listener.setPosition({posArr[0], posArr[1], posArr[2]});
        }
        
        Vec3 fwd = listener.getForward();
        float fwdArr[3] = {fwd.x, fwd.y, fwd.z};
        if (ImGui::DragFloat3("Forward", fwdArr, 0.1f)) {
            listener.setForward({fwdArr[0], fwdArr[1], fwdArr[2]});
        }
        
        float listenerVol = listener.getVolume();
        if (ImGui::SliderFloat("Volume##listener", &listenerVol, 0.0f, 1.0f)) {
            listener.setVolume(listenerVol);
        }
        
        ImGui::Checkbox("Show Listener Gizmo", &audioState.showListenerGizmo);
    }
    
    // === Mixer ===
    if (ImGui::CollapsingHeader("Mixer")) {
        auto& mixer = audioSystem.getMixer();
        auto& groups = mixer.getGroups();
        
        for (size_t i = 0; i < groups.size(); i++) {
            auto& group = groups[i];
            
            ImGui::PushID((int)i);
            
            // Indent based on parent
            float indent = 0.0f;
            if (group.parentIndex >= 0) indent = 20.0f;
            ImGui::Indent(indent);
            
            bool selected = (audioState.selectedMixerGroup == (int)i);
            if (ImGui::Selectable(group.name.c_str(), selected, 0, ImVec2(100, 0))) {
                audioState.selectedMixerGroup = (int)i;
            }
            
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::SliderFloat("##vol", &group.volume, 0.0f, 1.0f);
            
            ImGui::SameLine();
            ImGui::Checkbox("M##mute", &group.mute);
            
            ImGui::SameLine();
            ImGui::Checkbox("S##solo", &group.solo);
            
            ImGui::Unindent(indent);
            ImGui::PopID();
        }
        
        // Selected group details
        if (audioState.selectedMixerGroup >= 0 && audioState.selectedMixerGroup < (int)groups.size()) {
            ImGui::Separator();
            auto& group = groups[audioState.selectedMixerGroup];
            ImGui::Text("Group: %s", group.name.c_str());
            ImGui::Text("Effective Volume: %.2f", mixer.getEffectiveVolume(audioState.selectedMixerGroup));
            
            ImGui::Checkbox("Low Pass Filter", &group.lowPassEnabled);
            if (group.lowPassEnabled) {
                ImGui::SliderFloat("Cutoff", &group.lowPassCutoff, 100.0f, 22000.0f, "%.0f Hz");
            }
            
            ImGui::Checkbox("Reverb", &group.reverbEnabled);
            if (group.reverbEnabled) {
                ImGui::SliderFloat("Reverb Mix", &group.reverbMix, 0.0f, 1.0f);
            }
        }
    }
    
    // === Audio Sources ===
    if (ImGui::CollapsingHeader("Audio Sources", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& sources = audioSystem.getSources();
        
        // Source list
        ImGui::BeginChild("SourceList", ImVec2(0, 120), true);
        for (size_t i = 0; i < sources.size(); i++) {
            const auto& source = sources[i];
            const AudioClip* clip = source->getClip();
            
            char label[64];
            snprintf(label, sizeof(label), "[%u] %s %s",
                    source->getId(),
                    clip ? clip->getName().c_str() : "(no clip)",
                    source->isPlaying() ? "(playing)" : "");
            
            bool selected = (audioState.selectedSourceIndex == (int)i);
            if (ImGui::Selectable(label, selected)) {
                audioState.selectedSourceIndex = (int)i;
            }
        }
        ImGui::EndChild();
        
        // Create source
        if (ImGui::Button("+ Create Source")) {
            AudioSource* source = audioSystem.createSource();
            audioState.selectedSourceIndex = (int)audioSystem.getSources().size() - 1;
        }
        
        // Selected source details
        if (audioState.selectedSourceIndex >= 0 && audioState.selectedSourceIndex < (int)sources.size()) {
            auto& source = sources[audioState.selectedSourceIndex];
            auto& settings = source->getSettings();
            
            ImGui::Separator();
            ImGui::Text("Source ID: %u", source->getId());
            
            // Clip selection (simplified)
            ImGui::Text("Clip: %s", source->getClip() ? source->getClip()->getName().c_str() : "None");
            
            // Playback controls
            if (ImGui::Button("Play")) source->play();
            ImGui::SameLine();
            if (ImGui::Button("Pause")) source->pause();
            ImGui::SameLine();
            if (ImGui::Button("Stop")) source->stop();
            
            // State
            const char* stateStr = source->isPlaying() ? "Playing" :
                                  (source->getState() == AudioState::Paused ? "Paused" : "Stopped");
            ImGui::Text("State: %s  Time: %.2fs", stateStr, source->getTime());
            
            // Settings
            ImGui::SliderFloat("Volume##src", &settings.volume, 0.0f, 1.0f);
            ImGui::SliderFloat("Pitch", &settings.pitch, 0.1f, 3.0f);
            ImGui::Checkbox("Loop##src", &settings.loop);
            
            // 3D Settings
            ImGui::Separator();
            ImGui::Text("3D Settings");
            ImGui::Checkbox("Spatialize", &settings.spatialize);
            
            if (settings.spatialize) {
                Vec3 pos = source->getPosition();
                float posArr[3] = {pos.x, pos.y, pos.z};
                if (ImGui::DragFloat3("Position##src", posArr, 0.1f)) {
                    source->setPosition({posArr[0], posArr[1], posArr[2]});
                }
                
                ImGui::SliderFloat("Min Distance", &settings.minDistance, 0.1f, 50.0f);
                ImGui::SliderFloat("Max Distance", &settings.maxDistance, 10.0f, 1000.0f);
                
                const char* rolloffModes[] = { "Linear", "Logarithmic", "Custom" };
                int rolloffIdx = (int)settings.rolloff;
                if (ImGui::Combo("Rolloff", &rolloffIdx, rolloffModes, 3)) {
                    settings.rolloff = (AudioRolloff)rolloffIdx;
                }
                
                ImGui::SliderFloat("Doppler Level", &settings.dopplerLevel, 0.0f, 5.0f);
            }
            
            // Debug info
            ImGui::Text("Computed Volume: %.3f", source->computedVolume);
            ImGui::Text("Pan L/R: %.2f / %.2f", source->computedPanL, source->computedPanR);
            
            if (ImGui::Button("Delete Source")) {
                audioSystem.destroySource(source.get());
                audioState.selectedSourceIndex = -1;
            }
        }
    }
    
    // === Test Tones ===
    if (ImGui::CollapsingHeader("Test Sounds")) {
        ImGui::SliderFloat("Frequency", &audioState.testToneFrequency, 100.0f, 2000.0f, "%.0f Hz");
        ImGui::SliderFloat("Duration", &audioState.testToneDuration, 0.1f, 5.0f, "%.1f s");
        
        if (ImGui::Button("Generate Sine")) {
            auto clip = audioSystem.createClip("TestSine");
            clip->generateSineWave(audioState.testToneFrequency, audioState.testToneDuration);
            
            AudioSource* source = audioSystem.createSource();
            source->setClip(clip);
            source->play();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Generate Noise")) {
            auto clip = audioSystem.createClip("TestNoise");
            clip->generateWhiteNoise(audioState.testToneDuration);
            
            AudioSource* source = audioSystem.createSource();
            source->setClip(clip);
            source->play();
        }
        
        // Position for 3D test
        ImGui::DragFloat3("Test Position", audioState.newSourcePosition, 0.5f);
        
        if (ImGui::Button("Play at Position")) {
            auto clip = audioSystem.getClip("TestSine");
            if (!clip) {
                clip = audioSystem.createClip("TestSine");
                clip->generateSineWave(440.0f, 1.0f);
            }
            
            Vec3 pos = {
                audioState.newSourcePosition[0],
                audioState.newSourcePosition[1],
                audioState.newSourcePosition[2]
            };
            audioSystem.playOneShot(clip, pos);
        }
    }
    
    // === Visualization ===
    if (ImGui::CollapsingHeader("Visualization")) {
        ImGui::Checkbox("Show Source Gizmos", &audioState.showSourceGizmos);
        ImGui::Checkbox("Show Listener Gizmo##vis", &audioState.showListenerGizmo);
    }
    
    ImGui::End();
}

// ===== GI Editor State =====
struct GIEditorState {
    // Light probe grid settings
    float gridMin[3] = {-50, 0, -50};
    float gridMax[3] = {50, 20, 50};
    int gridResolution[3] = {5, 3, 5};
    bool gridInitialized = false;
    
    // Baking
    bool isBaking = false;
    int bakeProgress = 0;
    int bakeTotal = 0;
    
    // Selected items
    int selectedLightProbeGroup = -1;
    int selectedReflectionProbe = -1;
    
    // Visualization
    bool showLightProbes = true;
    bool showReflectionProbes = true;
    bool showProbeInfluence = false;
    float visualizationScale = 0.5f;
    
    // Preview
    Vec3 previewPosition = {0, 1, 0};
    Vec3 previewNormal = {0, 1, 0};
};

// ===== GI Editor Panel =====
inline void drawGIEditorPanel(GIEditorState& giState, EditorState& state) {
    if (!state.showGIEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("GI Editor", &state.showGIEditor)) {
        ImGui::End();
        return;
    }
    
    auto& giSystem = getGISystem();
    auto& settings = giSystem.getSettings();
    
    // === GI Settings ===
    if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Light Probes", &settings.lightProbesEnabled);
        if (settings.lightProbesEnabled) {
            ImGui::SliderFloat("Probe Intensity", &settings.lightProbeIntensity, 0.0f, 2.0f);
        }
        
        ImGui::Checkbox("Reflection Probes", &settings.reflectionProbesEnabled);
        if (settings.reflectionProbesEnabled) {
            ImGui::SliderFloat("Reflection Intensity", &settings.reflectionProbeIntensity, 0.0f, 2.0f);
        }
        
        ImGui::Separator();
        ImGui::Text("Ambient");
        ImGui::ColorEdit3("Sky Color", &settings.ambientSkyColor.x);
        ImGui::ColorEdit3("Ground Color", &settings.ambientGroundColor.x);
        ImGui::SliderFloat("Ambient Intensity", &settings.ambientIntensity, 0.0f, 1.0f);
    }
    
    // === Light Probe Grid ===
    if (ImGui::CollapsingHeader("Light Probe Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Grid Min", giState.gridMin, 1.0f);
        ImGui::DragFloat3("Grid Max", giState.gridMax, 1.0f);
        ImGui::DragInt3("Resolution", giState.gridResolution, 0.1f, 1, 20);
        
        if (ImGui::Button("Initialize Grid", ImVec2(-1, 25))) {
            Vec3 min = {giState.gridMin[0], giState.gridMin[1], giState.gridMin[2]};
            Vec3 max = {giState.gridMax[0], giState.gridMax[1], giState.gridMax[2]};
            giSystem.initializeLightProbeGrid(min, max, 
                giState.gridResolution[0], giState.gridResolution[1], giState.gridResolution[2]);
            giState.gridInitialized = true;
        }
        
        if (giSystem.hasLightProbeGrid()) {
            auto& grid = giSystem.getLightProbeGrid();
            ImGui::Text("Probes: %zu", grid.getProbeCount());
            ImGui::Text("Cell Size: %.2f x %.2f x %.2f", 
                       grid.getCellSize().x, grid.getCellSize().y, grid.getCellSize().z);
        }
    }
    
    // === Light Probe Groups ===
    if (ImGui::CollapsingHeader("Light Probe Groups")) {
        const auto& groups = giSystem.getLightProbeGroups();
        
        for (size_t i = 0; i < groups.size(); i++) {
            const auto& group = groups[i];
            bool selected = (giState.selectedLightProbeGroup == (int)i);
            
            char label[64];
            snprintf(label, sizeof(label), "%s (%zu probes)", 
                    group->getName().c_str(), group->getProbeCount());
            
            if (ImGui::Selectable(label, selected)) {
                giState.selectedLightProbeGroup = (int)i;
            }
        }
        
        if (ImGui::Button("+ Add Group")) {
            giSystem.addLightProbeGroup("LightProbeGroup");
        }
        
        // Selected group details
        if (giState.selectedLightProbeGroup >= 0 && 
            giState.selectedLightProbeGroup < (int)groups.size()) {
            
            auto& group = groups[giState.selectedLightProbeGroup];
            ImGui::Separator();
            ImGui::Text("Group: %s", group->getName().c_str());
            
            // Add probe at position
            static float addPos[3] = {0, 0, 0};
            ImGui::DragFloat3("Position##addprobe", addPos, 0.1f);
            if (ImGui::Button("Add Probe")) {
                group->addProbe({addPos[0], addPos[1], addPos[2]});
            }
        }
    }
    
    // === Reflection Probes ===
    if (ImGui::CollapsingHeader("Reflection Probes")) {
        auto& probeManager = getReflectionProbeManager();
        auto& probes = probeManager.getProbes();
        
        for (size_t i = 0; i < probes.size(); i++) {
            const auto& probe = probes[i];
            bool selected = (giState.selectedReflectionProbe == (int)i);
            
            char label[64];
            snprintf(label, sizeof(label), "%s [%d]%s", 
                    probe->getName().c_str(), probe->getPriority(),
                    probe->isEnabled() ? "" : " (disabled)");
            
            if (ImGui::Selectable(label, selected)) {
                giState.selectedReflectionProbe = (int)i;
            }
        }
        
        if (ImGui::Button("+ Add Reflection Probe")) {
            probeManager.createProbe("ReflectionProbe");
        }
        
        // Selected probe details
        if (giState.selectedReflectionProbe >= 0 && 
            giState.selectedReflectionProbe < (int)probes.size()) {
            
            auto& probe = probes[giState.selectedReflectionProbe];
            auto& probeSettings = probe->getSettings();
            
            ImGui::Separator();
            
            // Name input (simplified)
            ImGui::Text("Name: %s", probe->getName().c_str());
            
            bool enabled = probe->isEnabled();
            if (ImGui::Checkbox("Enabled##rp", &enabled)) {
                probe->setEnabled(enabled);
            }
            
            // Position
            Vec3 pos = probe->getPosition();
            float posArr[3] = {pos.x, pos.y, pos.z};
            if (ImGui::DragFloat3("Position##rp", posArr, 0.1f)) {
                probe->setPosition({posArr[0], posArr[1], posArr[2]});
            }
            
            // Shape
            const char* shapes[] = { "Box", "Sphere" };
            int shapeIdx = (int)probe->getShape();
            if (ImGui::Combo("Shape", &shapeIdx, shapes, 2)) {
                probe->setShape((ReflectionProbeShape)shapeIdx);
            }
            
            if (probe->getShape() == ReflectionProbeShape::Box) {
                Vec3 size = probe->getBoxSize();
                float sizeArr[3] = {size.x, size.y, size.z};
                if (ImGui::DragFloat3("Box Size", sizeArr, 0.1f, 0.1f, 100.0f)) {
                    probe->setBoxSize({sizeArr[0], sizeArr[1], sizeArr[2]});
                }
                
                ImGui::Checkbox("Box Projection", &probeSettings.boxProjection);
            } else {
                float radius = probe->getSphereRadius();
                if (ImGui::DragFloat("Sphere Radius", &radius, 0.1f, 0.1f, 1000.0f)) {
                    probe->setSphereRadius(radius);
                }
            }
            
            float influence = probe->getInfluenceRadius();
            if (ImGui::DragFloat("Influence Radius", &influence, 0.1f, 0.1f, 1000.0f)) {
                probe->setInfluenceRadius(influence);
            }
            
            int priority = probe->getPriority();
            if (ImGui::DragInt("Priority", &priority)) {
                probe->setPriority(priority);
            }
            
            float intensity = probe->getIntensity();
            if (ImGui::SliderFloat("Intensity##rp", &intensity, 0.0f, 2.0f)) {
                probe->setIntensity(intensity);
            }
            
            // Resolution
            const char* resolutions[] = { "64", "128", "256", "512", "1024" };
            int resIdx = 0;
            if (probeSettings.resolution == 64) resIdx = 0;
            else if (probeSettings.resolution == 128) resIdx = 1;
            else if (probeSettings.resolution == 256) resIdx = 2;
            else if (probeSettings.resolution == 512) resIdx = 3;
            else if (probeSettings.resolution == 1024) resIdx = 4;
            
            if (ImGui::Combo("Resolution##rp", &resIdx, resolutions, 5)) {
                const int resValues[] = {64, 128, 256, 512, 1024};
                probeSettings.resolution = resValues[resIdx];
            }
            
            ImGui::Checkbox("Realtime", &probeSettings.realtime);
            
            if (ImGui::Button("Bake Probe")) {
                probe->setDirty(true);
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Delete Probe")) {
                probeManager.removeProbe(probe.get());
                giState.selectedReflectionProbe = -1;
            }
        }
    }
    
    // === Baking ===
    if (ImGui::CollapsingHeader("Baking", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Bake Settings");
        ImGui::SliderInt("Bounces", &settings.bounces, 0, 4);
        ImGui::SliderInt("Samples", &settings.lightProbeSamples, 16, 256);
        ImGui::SliderInt("Rays/Sample", &settings.raysPerSample, 8, 128);
        
        ImGui::Separator();
        
        if (giState.isBaking) {
            ImGui::Text("Baking... %d / %d", giState.bakeProgress, giState.bakeTotal);
            float progress = giState.bakeTotal > 0 ? (float)giState.bakeProgress / giState.bakeTotal : 0;
            ImGui::ProgressBar(progress);
        } else {
            if (ImGui::Button("Bake All Light Probes", ImVec2(-1, 30))) {
                // Start baking (simplified - would be async in real implementation)
                std::vector<GISystem::LightInfo> lights;
                
                // Add a default directional light for testing
                GISystem::LightInfo sunLight;
                sunLight.type = GISystem::LightInfo::Type::Directional;
                sunLight.direction = Vec3(0.5f, -0.7f, 0.3f).normalized();
                sunLight.color = {1.0f, 0.95f, 0.8f};
                sunLight.intensity = 1.0f;
                lights.push_back(sunLight);
                
                giSystem.bakeAllLightProbes(lights, [&giState](int current, int total) {
                    giState.bakeProgress = current;
                    giState.bakeTotal = total;
                });
                
                giSystem.bakeAllLightProbeGroups(lights);
            }
            
            if (ImGui::Button("Clear Baked Data")) {
                giSystem.clearBakedData();
            }
        }
    }
    
    // === Preview ===
    if (ImGui::CollapsingHeader("Preview")) {
        ImGui::DragFloat3("Position##prev", &giState.previewPosition.x, 0.1f);
        ImGui::DragFloat3("Normal##prev", &giState.previewNormal.x, 0.01f, -1.0f, 1.0f);
        giState.previewNormal = giState.previewNormal.normalized();
        
        Vec3 irradiance = giSystem.sampleIndirectDiffuse(giState.previewPosition, giState.previewNormal);
        ImGui::Text("Irradiance: (%.3f, %.3f, %.3f)", irradiance.x, irradiance.y, irradiance.z);
        
        // Color preview
        ImVec4 color(irradiance.x, irradiance.y, irradiance.z, 1.0f);
        ImGui::ColorButton("##irr", color, 0, ImVec2(50, 50));
    }
    
    // === Visualization ===
    if (ImGui::CollapsingHeader("Visualization")) {
        ImGui::Checkbox("Show Light Probes", &giState.showLightProbes);
        ImGui::Checkbox("Show Reflection Probes", &giState.showReflectionProbes);
        ImGui::Checkbox("Show Influence Volumes", &giState.showProbeInfluence);
        ImGui::SliderFloat("Gizmo Scale", &giState.visualizationScale, 0.1f, 2.0f);
    }
    
    ImGui::End();
}

// ===== Video Export State =====
struct VideoExportState {
    VideoExportSettings settings;
    
    // UI state
    int formatIndex = 0;
    int qualityIndex = 2;  // High
    int resolutionPreset = 0;  // Custom
    
    // Recording state
    bool showAdvanced = false;
    double recordStartTime = 0.0;
    double lastFrameTime = 0.0;
    float avgFrameTime = 0.0f;
    
    VideoExportState() {
        settings.outputPath = "output.mp4";
        settings.width = 1920;
        settings.height = 1080;
        settings.frameRate = 30;
        settings.startTime = 0.0f;
        settings.endTime = 10.0f;
    }
};

// ===== Video Export Panel =====
inline void drawVideoExportPanel(VideoExportState& exportState, EditorState& state) {
    if (!state.showVideoExport) return;
    
    ImGui::SetNextWindowSize(ImVec2(380, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Video Export", &state.showVideoExport)) {
        ImGui::End();
        return;
    }
    
    auto& recorder = getRecordingManager();
    auto& settings = exportState.settings;
    RecordingState recState = recorder.getState();
    
    bool isRecording = (recState == RecordingState::Recording || recState == RecordingState::Paused);
    
    // === Output Settings ===
    if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Format
        const char* formats[] = {
            "MP4 (H.264)", "MP4 (H.265)", "WebM (VP9)", "AVI (MJPEG)",
            "GIF", "PNG Sequence", "JPG Sequence", "TGA Sequence"
        };
        
        if (ImGui::Combo("Format", &exportState.formatIndex, formats, 8)) {
            const VideoFormat formatValues[] = {
                VideoFormat::MP4_H264, VideoFormat::MP4_H265,
                VideoFormat::WebM_VP9, VideoFormat::AVI_MJPEG,
                VideoFormat::GIF,
                VideoFormat::ImageSequence_PNG, VideoFormat::ImageSequence_JPG,
                VideoFormat::ImageSequence_TGA
            };
            settings.format = formatValues[exportState.formatIndex];
        }
        
        // Quality
        const char* qualities[] = { "Low", "Medium", "High", "Lossless" };
        if (ImGui::Combo("Quality", &exportState.qualityIndex, qualities, 4)) {
            settings.quality = (VideoQuality)exportState.qualityIndex;
        }
        
        // Output path
        char pathBuf[256];
        strncpy(pathBuf, settings.outputPath.c_str(), sizeof(pathBuf) - 1);
        if (ImGui::InputText("Output File", pathBuf, sizeof(pathBuf))) {
            settings.outputPath = pathBuf;
        }
    }
    
    // === Resolution ===
    if (ImGui::CollapsingHeader("Resolution", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* presets[] = {
            "Custom", "720p (1280x720)", "1080p (1920x1080)",
            "1440p (2560x1440)", "4K (3840x2160)"
        };
        
        if (ImGui::Combo("Preset", &exportState.resolutionPreset, presets, 5)) {
            switch (exportState.resolutionPreset) {
                case 1: settings.width = 1280; settings.height = 720; break;
                case 2: settings.width = 1920; settings.height = 1080; break;
                case 3: settings.width = 2560; settings.height = 1440; break;
                case 4: settings.width = 3840; settings.height = 2160; break;
            }
        }
        
        if (exportState.resolutionPreset == 0) {
            ImGui::DragInt("Width", &settings.width, 1, 64, 7680);
            ImGui::DragInt("Height", &settings.height, 1, 64, 4320);
        } else {
            ImGui::Text("Resolution: %d x %d", settings.width, settings.height);
        }
        
        ImGui::Checkbox("Match Viewport", &settings.matchViewport);
    }
    
    // === Timeline ===
    if (ImGui::CollapsingHeader("Timeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragInt("Frame Rate", &settings.frameRate, 1, 1, 120);
        ImGui::DragFloat("Start Time", &settings.startTime, 0.1f, 0.0f, 3600.0f, "%.2f s");
        ImGui::DragFloat("End Time", &settings.endTime, 0.1f, 0.0f, 3600.0f, "%.2f s");
        
        float duration = settings.endTime - settings.startTime;
        int totalFrames = settings.getTotalFrames();
        ImGui::Text("Duration: %.2f s  |  Frames: %d", duration, totalFrames);
    }
    
    // === Advanced ===
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::DragInt("Bitrate (bps)", &settings.bitrate, 100000, 500000, 50000000);
        ImGui::DragInt("Keyframe Interval", &settings.keyframeInterval, 1, 1, 300);
        ImGui::Checkbox("Capture Every Frame", &settings.captureEveryFrame);
        ImGui::Checkbox("Multi-threaded", &settings.multiThreaded);
        if (settings.multiThreaded) {
            ImGui::DragInt("Encoder Threads", &settings.encoderThreads, 1, 1, 16);
        }
        
        // Estimated file size
        size_t estSize = recorder.getEstimatedFileSize();
        if (estSize > 1024 * 1024 * 1024) {
            ImGui::Text("Est. Size: %.2f GB", estSize / (1024.0 * 1024.0 * 1024.0));
        } else if (estSize > 1024 * 1024) {
            ImGui::Text("Est. Size: %.2f MB", estSize / (1024.0 * 1024.0));
        } else {
            ImGui::Text("Est. Size: %.2f KB", estSize / 1024.0);
        }
    }
    
    ImGui::Separator();
    
    // === Recording Controls ===
    if (!isRecording) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Start Recording", ImVec2(-1, 35))) {
            exportState.recordStartTime = 0.0;
            recorder.startRecording(settings);
        }
        ImGui::PopStyleColor();
    } else {
        // Progress
        float progress = recorder.getProgress();
        ImGui::ProgressBar(progress, ImVec2(-1, 20));
        
        ImGui::Text("Frame: %d / %d", (int)recorder.getFrameCount(), recorder.getTotalFrames());
        
        // ETA
        if (exportState.avgFrameTime > 0.0f) {
            double eta = recorder.getEstimatedTimeRemaining(exportState.avgFrameTime);
            if (eta > 60.0) {
                ImGui::Text("ETA: %.1f min", eta / 60.0);
            } else {
                ImGui::Text("ETA: %.1f s", eta);
            }
        }
        
        // Controls
        if (recState == RecordingState::Recording) {
            if (ImGui::Button("Pause", ImVec2(100, 30))) {
                recorder.pauseRecording();
            }
        } else if (recState == RecordingState::Paused) {
            if (ImGui::Button("Resume", ImVec2(100, 30))) {
                recorder.resumeRecording();
            }
        }
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Stop", ImVec2(100, 30))) {
            recorder.stopRecording();
        }
        ImGui::PopStyleColor();
    }
    
    // State indicator
    ImGui::Separator();
    const char* stateStr = "Idle";
    ImVec4 stateColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    
    switch (recState) {
        case RecordingState::Preparing:
            stateStr = "Preparing...";
            stateColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
            break;
        case RecordingState::Recording:
            stateStr = "Recording";
            stateColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            break;
        case RecordingState::Paused:
            stateStr = "Paused";
            stateColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
            break;
        case RecordingState::Finalizing:
            stateStr = "Finalizing...";
            stateColor = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
            break;
        case RecordingState::Complete:
            stateStr = "Complete";
            stateColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case RecordingState::Error:
            stateStr = "Error";
            stateColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;
        default:
            break;
    }
    
    ImGui::TextColored(stateColor, "Status: %s", stateStr);
    
    if (recState == RecordingState::Error) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", recorder.getError().c_str());
    }
    
    ImGui::End();
}

// ===== Network Panel State =====
struct NetworkPanelState {
    // Connection settings
    char serverAddress[64] = "127.0.0.1";
    int serverPort = 7777;
    
    // Selected connection
    int selectedConnection = -1;
    
    // Stats
    bool showStats = true;
};

// ===== Network Panel =====
inline void drawNetworkPanel(NetworkPanelState& netState, EditorState& state) {
    if (!state.showNetworkPanel) return;
    
    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Network", &state.showNetworkPanel)) {
        ImGui::End();
        return;
    }
    
    auto& netMgr = getNetworkManager();
    NetworkRole role = netMgr.getRole();
    
    // === Status ===
    const char* roleStr = "None";
    ImVec4 roleColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    
    switch (role) {
        case NetworkRole::Server:
            roleStr = "Server";
            roleColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
            break;
        case NetworkRole::Client:
            roleStr = "Client";
            roleColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
            break;
        case NetworkRole::Host:
            roleStr = "Host";
            roleColor = ImVec4(0.8f, 0.6f, 0.2f, 1.0f);
            break;
        default:
            break;
    }
    
    ImGui::TextColored(roleColor, "Role: %s", roleStr);
    
    if (!netMgr.isActive()) {
        // === Connection Setup ===
        ImGui::Separator();
        ImGui::Text("Connection Setup");
        
        ImGui::InputText("Address", netState.serverAddress, sizeof(netState.serverAddress));
        ImGui::InputInt("Port", &netState.serverPort);
        
        if (ImGui::Button("Start Server", ImVec2(150, 25))) {
            netMgr.startServer((uint16_t)netState.serverPort);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Start Host", ImVec2(150, 25))) {
            netMgr.startHost((uint16_t)netState.serverPort);
        }
        
        if (ImGui::Button("Connect as Client", ImVec2(-1, 25))) {
            netMgr.startClient(netState.serverAddress, (uint16_t)netState.serverPort);
        }
    } else {
        // === Active Network ===
        ImGui::Separator();
        
        if (ImGui::Button("Disconnect", ImVec2(-1, 25))) {
            netMgr.stop();
        }
        
        // Server info
        if (netMgr.isServer()) {
            auto* server = netMgr.getServer();
            if (server) {
                ImGui::Text("Clients: %zu", server->getClientCount());
                
                // Connection list
                if (ImGui::CollapsingHeader("Connections", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const auto& connections = server->getConnections();
                    
                    for (const auto& [id, conn] : connections) {
                        char label[64];
                        snprintf(label, sizeof(label), "[%u] %s:%u", 
                                id, conn.address.c_str(), conn.port);
                        
                        bool selected = (netState.selectedConnection == (int)id);
                        if (ImGui::Selectable(label, selected)) {
                            netState.selectedConnection = (int)id;
                        }
                    }
                }
                
                // Selected connection details
                if (netState.selectedConnection > 0) {
                    auto* conn = server->getConnection(netState.selectedConnection);
                    if (conn) {
                        ImGui::Separator();
                        ImGui::Text("Connection %u", conn->id);
                        ImGui::Text("Address: %s:%u", conn->address.c_str(), conn->port);
                        ImGui::Text("RTT: %.1f ms", conn->roundTripTime * 1000.0);
                        ImGui::Text("Sent: %llu bytes", (unsigned long long)conn->bytesSent);
                        ImGui::Text("Received: %llu bytes", (unsigned long long)conn->bytesReceived);
                        
                        if (ImGui::Button("Kick")) {
                            server->disconnectClient(conn->id);
                            netState.selectedConnection = -1;
                        }
                    }
                }
            }
        }
        
        // Client info
        if (netMgr.isClient()) {
            auto* client = netMgr.getClient();
            if (client) {
                const char* stateStr = "Unknown";
                switch (client->getConnectionState()) {
                    case ConnectionState::Connecting: stateStr = "Connecting..."; break;
                    case ConnectionState::Connected: stateStr = "Connected"; break;
                    case ConnectionState::Disconnected: stateStr = "Disconnected"; break;
                    case ConnectionState::Disconnecting: stateStr = "Disconnecting..."; break;
                }
                ImGui::Text("State: %s", stateStr);
                
                auto* conn = client->getConnection(SERVER_CONNECTION);
                if (conn) {
                    ImGui::Text("RTT: %.1f ms", conn->roundTripTime * 1000.0);
                    ImGui::Text("Sent: %llu bytes", (unsigned long long)conn->bytesSent);
                    ImGui::Text("Received: %llu bytes", (unsigned long long)conn->bytesReceived);
                }
            }
        }
    }
    
    ImGui::End();
}

// ===== Script Editor State =====
struct ScriptEditorState {
    // Selected class/instance
    int selectedClass = -1;
    int selectedInstance = -1;
    
    // New class
    char newClassName[64] = "MyScript";
    
    // Code editor (simplified)
    char codeBuffer[4096] = "";
    bool codeModified = false;
    
    // Console
    std::vector<std::string> consoleLog;
    char consoleInput[256] = "";
};

// ===== Script Editor Panel =====
inline void drawScriptEditorPanel(ScriptEditorState& scriptState, EditorState& state) {
    if (!state.showScriptEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Script Editor", &state.showScriptEditor)) {
        ImGui::End();
        return;
    }
    
    auto& scriptEngine = getScriptEngine();
    
    // Initialize if needed
    if (!scriptEngine.isInitialized()) {
        if (ImGui::Button("Initialize Script Engine", ImVec2(-1, 30))) {
            scriptEngine.initialize();
        }
        ImGui::End();
        return;
    }
    
    // === Script Classes ===
    if (ImGui::CollapsingHeader("Script Classes", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& classes = scriptEngine.getClasses();
        
        int idx = 0;
        for (const auto& [name, cls] : classes) {
            bool selected = (scriptState.selectedClass == idx);
            
            char label[128];
            snprintf(label, sizeof(label), "%s (%zu props, %zu RPCs)",
                    cls->name.c_str(), cls->properties.size(), cls->rpcs.size());
            
            if (ImGui::Selectable(label, selected)) {
                scriptState.selectedClass = idx;
            }
            idx++;
        }
        
        // Create new class
        ImGui::Separator();
        ImGui::InputText("Class Name", scriptState.newClassName, sizeof(scriptState.newClassName));
        if (ImGui::Button("Create Class")) {
            scriptEngine.registerClass(scriptState.newClassName);
        }
        
        // Selected class details
        if (scriptState.selectedClass >= 0) {
            int i = 0;
            for (auto& [name, cls] : scriptEngine.getClasses()) {
                if (i == scriptState.selectedClass) {
                    ImGui::Separator();
                    ImGui::Text("Class: %s", cls->name.c_str());
                    
                    // Properties
                    if (ImGui::TreeNode("Properties")) {
                        for (auto& prop : cls->properties) {
                            ImGui::BulletText("%s%s", prop.name.c_str(),
                                            prop.networked ? " [networked]" : "");
                        }
                        
                        // Add property
                        static char propName[64] = "";
                        ImGui::InputText("##propname", propName, sizeof(propName));
                        ImGui::SameLine();
                        if (ImGui::Button("Add Property")) {
                            ScriptProperty prop;
                            prop.name = propName;
                            cls->properties.push_back(prop);
                            propName[0] = 0;
                        }
                        
                        ImGui::TreePop();
                    }
                    
                    // RPCs
                    if (ImGui::TreeNode("RPCs")) {
                        for (auto& rpc : cls->rpcs) {
                            const char* auth = "";
                            if (rpc.serverOnly) auth = " [server]";
                            else if (rpc.clientOnly) auth = " [client]";
                            ImGui::BulletText("%s%s", rpc.name.c_str(), auth);
                        }
                        
                        // Add RPC
                        static char rpcName[64] = "";
                        static bool rpcServerOnly = false;
                        ImGui::InputText("##rpcname", rpcName, sizeof(rpcName));
                        ImGui::SameLine();
                        ImGui::Checkbox("Server Only", &rpcServerOnly);
                        ImGui::SameLine();
                        if (ImGui::Button("Add RPC")) {
                            ScriptRPCDef rpc;
                            rpc.name = rpcName;
                            rpc.serverOnly = rpcServerOnly;
                            cls->rpcs.push_back(rpc);
                            rpcName[0] = 0;
                        }
                        
                        ImGui::TreePop();
                    }
                    break;
                }
                i++;
            }
        }
    }
    
    // === Network Integration ===
    if (ImGui::CollapsingHeader("Network Integration")) {
        bool networkEnabled = scriptEngine.isNetworkEnabled();
        if (ImGui::Checkbox("Enable Network Sync", &networkEnabled)) {
            scriptEngine.setNetworkEnabled(networkEnabled);
        }
        
        ImGui::TextDisabled("When enabled:");
        ImGui::BulletText("Networked properties auto-sync");
        ImGui::BulletText("RPC calls go over network");
        ImGui::BulletText("Authority checks enforced");
    }
    
    // === Console ===
    if (ImGui::CollapsingHeader("Console")) {
        // Output
        ImGui::BeginChild("ConsoleOutput", ImVec2(0, 100), true);
        for (const auto& line : scriptState.consoleLog) {
            ImGui::TextUnformatted(line.c_str());
        }
        ImGui::EndChild();
        
        // Input
        ImGui::InputText("##consoleinput", scriptState.consoleInput, sizeof(scriptState.consoleInput));
        ImGui::SameLine();
        if (ImGui::Button("Run") || 
            (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
            if (strlen(scriptState.consoleInput) > 0) {
                scriptState.consoleLog.push_back(std::string("> ") + scriptState.consoleInput);
                
                if (scriptEngine.loadScriptString(scriptState.consoleInput, "console")) {
                    scriptState.consoleLog.push_back("OK");
                } else {
                    scriptState.consoleLog.push_back("Error: " + scriptEngine.getLastError());
                }
                
                scriptState.consoleInput[0] = 0;
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            scriptState.consoleLog.clear();
        }
    }
    
    // === Help ===
    if (ImGui::CollapsingHeader("Lua API Reference")) {
        ImGui::TextDisabled("Built-in Types:");
        ImGui::BulletText("Vec3(x, y, z)");
        ImGui::BulletText("Quat(x, y, z, w)");
        
        ImGui::TextDisabled("Entity Functions:");
        ImGui::BulletText("Entity.getPosition(id)");
        ImGui::BulletText("Entity.setPosition(id, vec3)");
        ImGui::BulletText("Entity.getRotation(id)");
        ImGui::BulletText("Entity.setRotation(id, quat)");
        
        ImGui::TextDisabled("Network Functions:");
        ImGui::BulletText("Network.isServer()");
        ImGui::BulletText("Network.isClient()");
        ImGui::BulletText("Network.hasAuthority(instance)");
        ImGui::BulletText("Network.rpc(instance, name, ...)");
        
        ImGui::TextDisabled("Debug Functions:");
        ImGui::BulletText("print(...)");
        ImGui::BulletText("Debug.drawLine(from, to, color)");
    }
    
    ImGui::End();
}

// ===== AI Editor State =====
struct AIEditorState {
    // NavMesh
    NavMeshBuildSettings navMeshSettings;
    bool navMeshBuilt = false;
    bool showNavMesh = true;
    bool showNavMeshBounds = true;
    
    // Build from terrain
    bool buildFromTerrain = true;
    
    // Agents
    int selectedAgent = -1;
    float agentTestDestination[3] = {0, 0, 0};
    
    // Path testing
    float pathStart[3] = {0, 0, 0};
    float pathEnd[3] = {10, 0, 10};
    bool showTestPath = false;
    NavPath testPath;
    
    // Behavior Tree
    int selectedBTNode = -1;
};

// ===== AI Editor Panel =====
inline void drawAIEditorPanel(AIEditorState& aiState, EditorState& state) {
    if (!state.showAIEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("AI Editor", &state.showAIEditor)) {
        ImGui::End();
        return;
    }
    
    auto& navMesh = getNavMesh();
    auto& agentManager = getNavAgentManager();
    
    // === NavMesh ===
    if (ImGui::CollapsingHeader("NavMesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& settings = aiState.navMeshSettings;
        
        ImGui::Text("Agent Properties");
        ImGui::SliderFloat("Height", &settings.agentHeight, 0.5f, 4.0f);
        ImGui::SliderFloat("Radius", &settings.agentRadius, 0.1f, 2.0f);
        ImGui::SliderFloat("Max Climb", &settings.agentMaxClimb, 0.1f, 1.0f);
        ImGui::SliderFloat("Max Slope", &settings.agentMaxSlope, 15.0f, 60.0f);
        
        ImGui::Separator();
        ImGui::Text("Voxelization");
        ImGui::SliderFloat("Cell Size", &settings.cellSize, 0.1f, 1.0f);
        ImGui::SliderFloat("Cell Height", &settings.cellHeight, 0.05f, 0.5f);
        
        ImGui::Separator();
        
        ImGui::Checkbox("Build from Terrain", &aiState.buildFromTerrain);
        
        if (ImGui::Button("Build NavMesh", ImVec2(-1, 30))) {
            if (aiState.buildFromTerrain) {
                // Would build from terrain heightmap
                // For now, create a simple test grid
                std::vector<Vec3> verts;
                std::vector<int> indices;
                
                // Create flat ground plane
                int gridSize = 20;
                float gridSpacing = 2.0f;
                float halfSize = gridSize * gridSpacing * 0.5f;
                
                for (int z = 0; z <= gridSize; z++) {
                    for (int x = 0; x <= gridSize; x++) {
                        float wx = x * gridSpacing - halfSize;
                        float wz = z * gridSpacing - halfSize;
                        verts.push_back(Vec3(wx, 0, wz));
                    }
                }
                
                for (int z = 0; z < gridSize; z++) {
                    for (int x = 0; x < gridSize; x++) {
                        int i = z * (gridSize + 1) + x;
                        indices.push_back(i);
                        indices.push_back(i + 1);
                        indices.push_back(i + gridSize + 1);
                        
                        indices.push_back(i + 1);
                        indices.push_back(i + gridSize + 2);
                        indices.push_back(i + gridSize + 1);
                    }
                }
                
                navMesh.build(verts, indices, settings);
                aiState.navMeshBuilt = true;
            }
        }
        
        if (navMesh.isValid()) {
            ImGui::Text("Vertices: %zu", navMesh.getVertexCount());
            ImGui::Text("Polygons: %zu", navMesh.getPolyCount());
            ImGui::Text("Edges: %zu", navMesh.getEdges().size());
            
            Vec3 min = navMesh.getMinBounds();
            Vec3 max = navMesh.getMaxBounds();
            ImGui::Text("Bounds: (%.1f,%.1f,%.1f) - (%.1f,%.1f,%.1f)",
                       min.x, min.y, min.z, max.x, max.y, max.z);
        }
        
        ImGui::Checkbox("Show NavMesh", &aiState.showNavMesh);
        ImGui::Checkbox("Show Bounds", &aiState.showNavMeshBounds);
        
        if (ImGui::Button("Clear NavMesh")) {
            navMesh.clear();
            aiState.navMeshBuilt = false;
        }
    }
    
    // === Agents ===
    if (ImGui::CollapsingHeader("Agents")) {
        const auto& agents = agentManager.getAgents();
        
        // Agent list
        for (size_t i = 0; i < agents.size(); i++) {
            const auto& agent = agents[i];
            
            char label[64];
            const char* stateStr = "Idle";
            switch (agent->getState()) {
                case NavAgentState::Moving: stateStr = "Moving"; break;
                case NavAgentState::Arrived: stateStr = "Arrived"; break;
                case NavAgentState::Stuck: stateStr = "Stuck"; break;
                default: break;
            }
            snprintf(label, sizeof(label), "Agent %u [%s]", agent->getId(), stateStr);
            
            bool selected = (aiState.selectedAgent == (int)i);
            if (ImGui::Selectable(label, selected)) {
                aiState.selectedAgent = (int)i;
            }
        }
        
        if (ImGui::Button("+ Create Agent")) {
            auto* agent = agentManager.createAgent();
            agent->setPosition(Vec3(0, 0, 0));
            aiState.selectedAgent = (int)agents.size() - 1;
        }
        
        // Selected agent details
        if (aiState.selectedAgent >= 0 && aiState.selectedAgent < (int)agents.size()) {
            auto& agent = agents[aiState.selectedAgent];
            auto& settings = agent->getSettings();
            
            ImGui::Separator();
            ImGui::Text("Agent %u", agent->getId());
            
            // Position
            Vec3 pos = agent->getPosition();
            float posArr[3] = {pos.x, pos.y, pos.z};
            if (ImGui::DragFloat3("Position##agent", posArr, 0.1f)) {
                agent->setPosition({posArr[0], posArr[1], posArr[2]});
            }
            
            // Rotation
            float rot = agent->getRotation();
            if (ImGui::SliderFloat("Rotation", &rot, 0, 360)) {
                agent->setRotation(rot);
            }
            
            // Settings
            ImGui::SliderFloat("Speed", &settings.speed, 1.0f, 20.0f);
            ImGui::SliderFloat("Acceleration", &settings.acceleration, 1.0f, 50.0f);
            ImGui::SliderFloat("Angular Speed", &settings.angularSpeed, 90.0f, 720.0f);
            ImGui::SliderFloat("Stopping Distance", &settings.stoppingDistance, 0.01f, 1.0f);
            
            // Destination
            ImGui::DragFloat3("Destination", aiState.agentTestDestination, 0.1f);
            if (ImGui::Button("Go To")) {
                agent->setDestination({
                    aiState.agentTestDestination[0],
                    aiState.agentTestDestination[1],
                    aiState.agentTestDestination[2]
                });
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop##agent")) {
                agent->stop();
            }
            
            // Path info
            if (agent->hasPath()) {
                ImGui::Text("Path Points: %zu", agent->getCurrentPath().getPointCount());
                ImGui::Text("Remaining: %.2f m", agent->getRemainingDistance());
            }
            
            if (ImGui::Button("Delete Agent")) {
                agentManager.destroyAgent(agent.get());
                aiState.selectedAgent = -1;
            }
        }
    }
    
    // === Path Testing ===
    if (ImGui::CollapsingHeader("Path Testing")) {
        ImGui::DragFloat3("Start", aiState.pathStart, 0.1f);
        ImGui::DragFloat3("End", aiState.pathEnd, 0.1f);
        
        if (ImGui::Button("Find Path")) {
            if (navMesh.isValid()) {
                NavPathfinder pathfinder(&navMesh);
                Vec3 start = {aiState.pathStart[0], aiState.pathStart[1], aiState.pathStart[2]};
                Vec3 end = {aiState.pathEnd[0], aiState.pathEnd[1], aiState.pathEnd[2]};
                
                if (pathfinder.findPath(start, end, aiState.testPath)) {
                    aiState.showTestPath = true;
                } else {
                    aiState.testPath.clear();
                    aiState.showTestPath = false;
                }
            }
        }
        
        ImGui::Checkbox("Show Path", &aiState.showTestPath);
        
        if (aiState.testPath.valid) {
            ImGui::Text("Path Length: %.2f m", aiState.testPath.totalLength);
            ImGui::Text("Waypoints: %zu", aiState.testPath.getPointCount());
        }
    }
    
    // === Behavior Tree ===
    if (ImGui::CollapsingHeader("Behavior Tree")) {
        ImGui::TextDisabled("Behavior Tree Editor");
        ImGui::BulletText("Sequence - Execute in order");
        ImGui::BulletText("Selector - Try until success");
        ImGui::BulletText("Parallel - Execute simultaneously");
        ImGui::BulletText("Decorators - Modify child results");
        
        ImGui::Separator();
        ImGui::Text("Example BT:");
        ImGui::TextWrapped(
            "BTBuilder()\n"
            "  .selector()\n"
            "    .sequence(\"Attack\")\n"
            "      .condition(inRange(\"target\", 2.0f))\n"
            "      .action(attackTarget)\n"
            "    .end()\n"
            "    .sequence(\"Chase\")\n"
            "      .condition(hasTarget)\n"
            "      .action(moveTo(\"target\"))\n"
            "    .end()\n"
            "    .action(patrol)\n"
            "  .end()\n"
            ".build();"
        );
    }
    
    // === Debug Visualization ===
    if (ImGui::CollapsingHeader("Visualization")) {
        ImGui::Checkbox("Show NavMesh##vis", &aiState.showNavMesh);
        ImGui::Checkbox("Show Agent Paths", &aiState.showTestPath);
        
        static float navMeshColor[4] = {0.2f, 0.6f, 0.3f, 0.5f};
        ImGui::ColorEdit4("NavMesh Color", navMeshColor);
    }
    
    ImGui::End();
}

// ===== Game UI Editor State =====
struct GameUIEditorState {
    // Canvas
    std::string selectedCanvas = "";
    char newCanvasName[64] = "NewCanvas";
    
    // Widget
    uint32_t selectedWidgetId = 0;
    int widgetTypeToCreate = 0;  // 0=Panel, 1=Label, 2=Button, etc.
    
    // Preview
    bool showPreview = true;
    float previewScale = 1.0f;
    
    // Widget creation
    char widgetName[64] = "Widget";
    char labelText[256] = "Label Text";
    char buttonText[256] = "Button";
};

// ===== Game UI Editor Panel =====
inline void drawGameUIEditorPanel(GameUIEditorState& uiState, EditorState& state) {
    if (!state.showGameUIEditor) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Game UI Editor", &state.showGameUIEditor)) {
        ImGui::End();
        return;
    }
    
    auto& uiSystem = luma::ui::getUISystem();
    
    // === Canvas Management ===
    if (ImGui::CollapsingHeader("Canvases", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& canvases = uiSystem.getCanvases();
        
        for (const auto& [name, canvas] : canvases) {
            bool selected = (uiState.selectedCanvas == name);
            char label[128];
            snprintf(label, sizeof(label), "%s [%s]",
                    name.c_str(), canvas->isVisible() ? "Visible" : "Hidden");
            
            if (ImGui::Selectable(label, selected)) {
                uiState.selectedCanvas = name;
            }
        }
        
        ImGui::Separator();
        
        ImGui::InputText("Canvas Name", uiState.newCanvasName, IM_ARRAYSIZE(uiState.newCanvasName));
        if (ImGui::Button("+ Create Canvas")) {
            if (strlen(uiState.newCanvasName) > 0 && !uiSystem.getCanvas(uiState.newCanvasName)) {
                uiSystem.createCanvas(uiState.newCanvasName);
                uiState.selectedCanvas = uiState.newCanvasName;
            }
        }
    }
    
    // === Selected Canvas ===
    luma::ui::UICanvas* selectedCanvas = nullptr;
    if (!uiState.selectedCanvas.empty()) {
        selectedCanvas = uiSystem.getCanvas(uiState.selectedCanvas);
    }
    
    if (selectedCanvas) {
        if (ImGui::CollapsingHeader("Canvas Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool visible = selectedCanvas->isVisible();
            if (ImGui::Checkbox("Visible##canvas", &visible)) {
                selectedCanvas->setVisible(visible);
            }
            
            int order = selectedCanvas->getRenderOrder();
            if (ImGui::InputInt("Render Order", &order)) {
                selectedCanvas->setRenderOrder(order);
            }
            
            ImGui::Text("Screen: %.0f x %.0f", 
                       selectedCanvas->getScreenWidth(),
                       selectedCanvas->getScreenHeight());
            
            if (ImGui::Button("Delete Canvas")) {
                uiSystem.removeCanvas(uiState.selectedCanvas);
                uiState.selectedCanvas = "";
                selectedCanvas = nullptr;
            }
        }
    }
    
    // === Widget Hierarchy ===
    if (selectedCanvas && ImGui::CollapsingHeader("Widget Hierarchy", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::function<void(luma::ui::UIWidget*, int)> drawWidget;
        drawWidget = [&](luma::ui::UIWidget* widget, int depth) {
            if (!widget) return;
            
            ImGui::PushID(widget->getId());
            
            bool selected = (uiState.selectedWidgetId == widget->getId());
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            if (widget->getChildren().empty()) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            if (selected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            
            // Widget type name
            const char* typeNames[] = {"Base", "Panel", "Label", "Image", "Button",
                                       "Checkbox", "Slider", "Progress", "Input",
                                       "Dropdown", "ScrollView", "ListView",
                                       "HLayout", "VLayout", "Grid"};
            int typeIdx = (int)widget->getType();
            const char* typeName = (typeIdx < 15) ? typeNames[typeIdx] : "Widget";
            
            char label[128];
            snprintf(label, sizeof(label), "[%s] %s", typeName, widget->getName().c_str());
            
            bool open = ImGui::TreeNodeEx(label, flags);
            
            if (ImGui::IsItemClicked()) {
                uiState.selectedWidgetId = widget->getId();
            }
            
            if (open) {
                for (const auto& child : widget->getChildren()) {
                    drawWidget(child.get(), depth + 1);
                }
                ImGui::TreePop();
            }
            
            ImGui::PopID();
        };
        
        drawWidget(selectedCanvas->getRoot(), 0);
    }
    
    // === Widget Creation ===
    if (selectedCanvas && ImGui::CollapsingHeader("Create Widget")) {
        ImGui::InputText("Name##widget", uiState.widgetName, IM_ARRAYSIZE(uiState.widgetName));
        
        const char* widgetTypes[] = {"Panel", "Label", "Button", "Checkbox", 
                                     "Slider", "Progress Bar", "Input Field",
                                     "Dropdown", "Scroll View", "List View",
                                     "HBox", "VBox", "Grid"};
        ImGui::Combo("Type", &uiState.widgetTypeToCreate, widgetTypes, IM_ARRAYSIZE(widgetTypes));
        
        // Type-specific settings
        if (uiState.widgetTypeToCreate == 1 || uiState.widgetTypeToCreate == 2) {
            ImGui::InputText("Text", uiState.labelText, IM_ARRAYSIZE(uiState.labelText));
        }
        
        if (ImGui::Button("Create Widget", ImVec2(-1, 30))) {
            std::shared_ptr<luma::ui::UIWidget> newWidget;
            
            switch (uiState.widgetTypeToCreate) {
                case 0: newWidget = luma::ui::UIFactory::createPanel(uiState.widgetName); break;
                case 1: newWidget = luma::ui::UIFactory::createLabel(uiState.labelText, uiState.widgetName); break;
                case 2: newWidget = luma::ui::UIFactory::createButton(uiState.labelText, uiState.widgetName); break;
                case 3: newWidget = luma::ui::UIFactory::createCheckbox("Checkbox", uiState.widgetName); break;
                case 4: newWidget = luma::ui::UIFactory::createSlider(uiState.widgetName); break;
                case 5: newWidget = luma::ui::UIFactory::createProgressBar(uiState.widgetName); break;
                case 6: newWidget = luma::ui::UIFactory::createInputField(uiState.widgetName); break;
                case 7: newWidget = luma::ui::UIFactory::createDropdown(uiState.widgetName); break;
                case 8: newWidget = luma::ui::UIFactory::createScrollView(uiState.widgetName); break;
                case 9: newWidget = luma::ui::UIFactory::createListView(uiState.widgetName); break;
                case 10: newWidget = luma::ui::UIFactory::createHBox(uiState.widgetName); break;
                case 11: newWidget = luma::ui::UIFactory::createVBox(uiState.widgetName); break;
                case 12: newWidget = luma::ui::UIFactory::createGrid(3, uiState.widgetName); break;
            }
            
            if (newWidget) {
                newWidget->setPosition(100, 100);
                newWidget->setSize(200, 40);
                selectedCanvas->addWidget(newWidget);
                uiState.selectedWidgetId = newWidget->getId();
            }
        }
    }
    
    // === Selected Widget Properties ===
    if (selectedCanvas && uiState.selectedWidgetId > 0) {
        // Find widget by ID
        std::function<luma::ui::UIWidget*(luma::ui::UIWidget*)> findWidget;
        findWidget = [&](luma::ui::UIWidget* widget) -> luma::ui::UIWidget* {
            if (!widget) return nullptr;
            if (widget->getId() == uiState.selectedWidgetId) return widget;
            for (const auto& child : widget->getChildren()) {
                if (auto* found = findWidget(child.get())) return found;
            }
            return nullptr;
        };
        
        luma::ui::UIWidget* selected = findWidget(selectedCanvas->getRoot());
        
        if (selected && ImGui::CollapsingHeader("Widget Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Name
            static char nameBuffer[64];
            strncpy(nameBuffer, selected->getName().c_str(), sizeof(nameBuffer) - 1);
            if (ImGui::InputText("Name##prop", nameBuffer, sizeof(nameBuffer))) {
                selected->setName(nameBuffer);
            }
            
            // Visibility
            bool visible = selected->isVisible();
            if (ImGui::Checkbox("Visible##prop", &visible)) {
                selected->setVisible(visible);
            }
            
            ImGui::SameLine();
            bool enabled = selected->isEnabled();
            if (ImGui::Checkbox("Enabled", &enabled)) {
                selected->setEnabled(enabled);
            }
            
            // Transform
            ImGui::Separator();
            ImGui::Text("Transform");
            
            float pos[2] = {selected->getX(), selected->getY()};
            if (ImGui::DragFloat2("Position", pos, 1.0f)) {
                selected->setPosition(pos[0], pos[1]);
            }
            
            float size[2] = {selected->getWidth(), selected->getHeight()};
            if (ImGui::DragFloat2("Size", size, 1.0f, 0.0f, 2000.0f)) {
                selected->setSize(size[0], size[1]);
            }
            
            // Anchor
            const char* anchors[] = {"TopLeft", "TopCenter", "TopRight",
                                     "MiddleLeft", "MiddleCenter", "MiddleRight",
                                     "BottomLeft", "BottomCenter", "BottomRight", "Stretch"};
            int anchorIdx = (int)selected->getAnchor();
            if (ImGui::Combo("Anchor", &anchorIdx, anchors, IM_ARRAYSIZE(anchors))) {
                selected->setAnchor((luma::ui::UIAnchor)anchorIdx);
            }
            
            // Pivot
            float pivot[2] = {selected->getPivot().x, selected->getPivot().y};
            if (ImGui::DragFloat2("Pivot", pivot, 0.01f, 0.0f, 1.0f)) {
                selected->setPivot(pivot[0], pivot[1]);
            }
            
            // Color
            ImGui::Separator();
            float color[4] = {selected->getColor().r, selected->getColor().g,
                             selected->getColor().b, selected->getColor().a};
            if (ImGui::ColorEdit4("Color", color)) {
                selected->setColor({color[0], color[1], color[2], color[3]});
            }
            
            // Type-specific properties
            ImGui::Separator();
            
            if (auto* label = dynamic_cast<luma::ui::UILabel*>(selected)) {
                static char textBuffer[256];
                strncpy(textBuffer, label->getText().c_str(), sizeof(textBuffer) - 1);
                if (ImGui::InputText("Text##label", textBuffer, sizeof(textBuffer))) {
                    label->setText(textBuffer);
                }
                
                float fontSize = label->getFontSize();
                if (ImGui::SliderFloat("Font Size", &fontSize, 8, 72)) {
                    label->setFontSize(fontSize);
                }
            }
            
            if (auto* button = dynamic_cast<luma::ui::UIButton*>(selected)) {
                static char textBuffer[256];
                strncpy(textBuffer, button->getText().c_str(), sizeof(textBuffer) - 1);
                if (ImGui::InputText("Text##button", textBuffer, sizeof(textBuffer))) {
                    button->setText(textBuffer);
                }
                
                float radius = button->getBorderRadius();
                if (ImGui::SliderFloat("Border Radius", &radius, 0, 20)) {
                    button->setBorderRadius(radius);
                }
            }
            
            if (auto* slider = dynamic_cast<luma::ui::UISlider*>(selected)) {
                float value = slider->getValue();
                float min = slider->getMinValue();
                float max = slider->getMaxValue();
                
                if (ImGui::SliderFloat("Value", &value, min, max)) {
                    slider->setValue(value);
                }
                
                if (ImGui::DragFloat("Min", &min, 0.1f)) {
                    slider->setRange(min, max);
                }
                if (ImGui::DragFloat("Max", &max, 0.1f)) {
                    slider->setRange(min, max);
                }
            }
            
            if (auto* progress = dynamic_cast<luma::ui::UIProgressBar*>(selected)) {
                float value = progress->getValue();
                if (ImGui::SliderFloat("Value##progress", &value, 0, 1)) {
                    progress->setValue(value);
                }
                
                bool showText = progress->getShowText();
                if (ImGui::Checkbox("Show Text", &showText)) {
                    progress->setShowText(showText);
                }
            }
            
            if (auto* input = dynamic_cast<luma::ui::UIInputField*>(selected)) {
                static char placeholder[256];
                strncpy(placeholder, input->getPlaceholder().c_str(), sizeof(placeholder) - 1);
                if (ImGui::InputText("Placeholder", placeholder, sizeof(placeholder))) {
                    input->setPlaceholder(placeholder);
                }
                
                int maxLen = input->getMaxLength();
                if (ImGui::InputInt("Max Length", &maxLen)) {
                    input->setMaxLength(maxLen);
                }
            }
            
            // Delete
            ImGui::Separator();
            if (ImGui::Button("Delete Widget")) {
                selected->removeFromParent();
                uiState.selectedWidgetId = 0;
            }
        }
    }
    
    // === Preview Settings ===
    if (ImGui::CollapsingHeader("Preview")) {
        ImGui::Checkbox("Show Preview Window", &uiState.showPreview);
        ImGui::SliderFloat("Preview Scale", &uiState.previewScale, 0.25f, 2.0f);
    }
    
    // === Widget Reference ===
    if (ImGui::CollapsingHeader("Widget Reference")) {
        ImGui::TextDisabled("Available Widgets:");
        ImGui::BulletText("Panel - Container with background");
        ImGui::BulletText("Label - Text display");
        ImGui::BulletText("Image - Texture display");
        ImGui::BulletText("Button - Clickable button");
        ImGui::BulletText("Checkbox - Toggle switch");
        ImGui::BulletText("Slider - Value slider");
        ImGui::BulletText("Progress Bar - Progress display");
        ImGui::BulletText("Input Field - Text input");
        ImGui::BulletText("Dropdown - Selection list");
        
        ImGui::Separator();
        ImGui::TextDisabled("Layout Containers:");
        ImGui::BulletText("HBox - Horizontal layout");
        ImGui::BulletText("VBox - Vertical layout");
        ImGui::BulletText("Grid - Grid layout");
    }
    
    ImGui::End();
}

// ===== Scene Manager State =====
struct SceneManagerState {
    char newSceneName[64] = "NewScene";
    char loadScenePath[256] = "";
    int selectedSceneId = -1;
    bool showTransitionSettings = false;
    int transitionType = 0;
    float transitionDuration = 0.5f;
};

// ===== Scene Manager Panel =====
inline void drawSceneManagerPanel(SceneManagerState& sceneState, EditorState& state) {
    if (!state.showSceneManager) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Scene Manager", &state.showSceneManager)) {
        ImGui::End();
        return;
    }
    
    auto& sceneMgr = luma::getSceneManager();
    
    // === Current Scene ===
    if (ImGui::CollapsingHeader("Current Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* activeScene = sceneMgr.getActiveScene();
        if (activeScene) {
            ImGui::Text("Name: %s", activeScene->getName().c_str());
            ImGui::Text("Path: %s", activeScene->getPath().c_str());
            ImGui::Text("Objects: %zu", activeScene->getData().objects.size());
            
            const char* stateNames[] = {"Unloaded", "Loading", "Loaded", "Active", "Unloading"};
            ImGui::Text("State: %s", stateNames[(int)activeScene->getState()]);
            
            if (activeScene->getState() == luma::SceneState::Loading) {
                ImGui::ProgressBar(activeScene->getLoadProgress());
            }
        } else {
            ImGui::TextDisabled("No active scene");
        }
    }
    
    // === Loaded Scenes ===
    if (ImGui::CollapsingHeader("Loaded Scenes", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& scenes = sceneMgr.getAllScenes();
        
        for (const auto& [id, scene] : scenes) {
            bool selected = (sceneState.selectedSceneId == (int)id);
            bool isActive = (scene.get() == sceneMgr.getActiveScene());
            
            char label[128];
            snprintf(label, sizeof(label), "%s%s [%s]", 
                    isActive ? "* " : "  ",
                    scene->getName().c_str(),
                    scene->getPath().c_str());
            
            if (ImGui::Selectable(label, selected)) {
                sceneState.selectedSceneId = (int)id;
            }
            
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                sceneMgr.setActiveScene(scene.get());
            }
        }
        
        if (scenes.empty()) {
            ImGui::TextDisabled("No scenes loaded");
        }
    }
    
    // === Scene Operations ===
    if (ImGui::CollapsingHeader("Operations", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Scene Path", sceneState.loadScenePath, IM_ARRAYSIZE(sceneState.loadScenePath));
        
        if (ImGui::Button("Load Scene")) {
            if (strlen(sceneState.loadScenePath) > 0) {
                sceneMgr.loadSceneAsync(sceneState.loadScenePath, luma::SceneLoadMode::Single);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Load Additive")) {
            if (strlen(sceneState.loadScenePath) > 0) {
                sceneMgr.loadSceneAsync(sceneState.loadScenePath, luma::SceneLoadMode::Additive);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Preload")) {
            if (strlen(sceneState.loadScenePath) > 0) {
                sceneMgr.preloadScene(sceneState.loadScenePath);
            }
        }
        
        ImGui::Separator();
        
        ImGui::InputText("New Scene Name", sceneState.newSceneName, IM_ARRAYSIZE(sceneState.newSceneName));
        if (ImGui::Button("Create New Scene")) {
            auto* scene = sceneMgr.createScene(sceneState.newSceneName);
            sceneMgr.setActiveScene(scene);
        }
        
        ImGui::Separator();
        
        if (sceneState.selectedSceneId > 0) {
            if (ImGui::Button("Unload Selected")) {
                sceneMgr.unloadScene((uint32_t)sceneState.selectedSceneId);
                sceneState.selectedSceneId = -1;
            }
            ImGui::SameLine();
        }
        
        if (ImGui::Button("Unload All")) {
            sceneMgr.unloadAllScenes();
            sceneState.selectedSceneId = -1;
        }
    }
    
    // === Transition ===
    if (ImGui::CollapsingHeader("Scene Transition")) {
        const char* transitionTypes[] = {"None", "Fade", "Crossfade", "SlideLeft", "SlideRight"};
        ImGui::Combo("Transition Type", &sceneState.transitionType, transitionTypes, IM_ARRAYSIZE(transitionTypes));
        ImGui::SliderFloat("Duration", &sceneState.transitionDuration, 0.1f, 2.0f);
        
        if (ImGui::Button("Transition To Scene")) {
            if (strlen(sceneState.loadScenePath) > 0) {
                luma::getSceneTransitionManager().transitionTo(
                    sceneState.loadScenePath,
                    (luma::SceneTransition::Type)sceneState.transitionType,
                    sceneState.transitionDuration
                );
            }
        }
        
        if (luma::getSceneTransitionManager().isTransitioning()) {
            ImGui::Text("Transitioning...");
            ImGui::ProgressBar(luma::getSceneTransitionManager().getTransition().getProgress());
        }
    }
    
    // === Loading Status ===
    if (sceneMgr.isLoading()) {
        ImGui::Separator();
        ImGui::Text("Loading...");
        ImGui::ProgressBar(sceneMgr.getCurrentLoadProgress());
    }
    
    ImGui::End();
}

// ===== Data Manager State =====
struct DataManagerState {
    char configName[64] = "game";
    char configKey[64] = "";
    char configValue[256] = "";
    int valueType = 0;  // 0=string, 1=int, 2=float, 3=bool
    
    char langCode[8] = "en";
    char localizeKey[64] = "";
    
    int selectedConfig = -1;
};

// ===== Data Manager Panel =====
inline void drawDataManagerPanel(DataManagerState& dataState, EditorState& state) {
    if (!state.showDataManager) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Data Manager", &state.showDataManager)) {
        ImGui::End();
        return;
    }
    
    auto& dataMgr = luma::getDataManager();
    
    // === Config Tables ===
    if (ImGui::CollapsingHeader("Config Tables", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Config Name", dataState.configName, IM_ARRAYSIZE(dataState.configName));
        
        if (ImGui::Button("Load Config")) {
            dataMgr.loadConfig(dataState.configName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            dataMgr.reloadConfig(dataState.configName);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            dataMgr.saveConfig(dataState.configName);
        }
        
        auto* config = dataMgr.getConfig(dataState.configName);
        if (config) {
            ImGui::Separator();
            ImGui::Text("Config: %s", config->getName().c_str());
            
            // Display all values
            const auto& data = config->getAllData();
            for (const auto& [key, value] : data) {
                std::string valueStr;
                if (std::holds_alternative<bool>(value)) {
                    valueStr = std::get<bool>(value) ? "true" : "false";
                } else if (std::holds_alternative<int64_t>(value)) {
                    valueStr = std::to_string(std::get<int64_t>(value));
                } else if (std::holds_alternative<double>(value)) {
                    valueStr = std::to_string(std::get<double>(value));
                } else if (std::holds_alternative<std::string>(value)) {
                    valueStr = std::get<std::string>(value);
                }
                ImGui::Text("%s = %s", key.c_str(), valueStr.c_str());
            }
            
            // Add/edit value
            ImGui::Separator();
            ImGui::InputText("Key", dataState.configKey, IM_ARRAYSIZE(dataState.configKey));
            ImGui::InputText("Value", dataState.configValue, IM_ARRAYSIZE(dataState.configValue));
            
            const char* types[] = {"String", "Int", "Float", "Bool"};
            ImGui::Combo("Type", &dataState.valueType, types, IM_ARRAYSIZE(types));
            
            if (ImGui::Button("Set Value")) {
                switch (dataState.valueType) {
                    case 0: config->setString(dataState.configKey, dataState.configValue); break;
                    case 1: config->setInt(dataState.configKey, std::stoll(dataState.configValue)); break;
                    case 2: config->setFloat(dataState.configKey, std::stod(dataState.configValue)); break;
                    case 3: config->setBool(dataState.configKey, std::string(dataState.configValue) == "true"); break;
                }
            }
        }
    }
    
    // === Localization ===
    if (ImGui::CollapsingHeader("Localization")) {
        auto& loc = dataMgr.getLocalization();
        
        ImGui::Text("Current Language: %s", loc.getLanguage().c_str());
        
        auto langs = loc.getAvailableLanguages();
        if (!langs.empty()) {
            if (ImGui::BeginCombo("Language", loc.getLanguage().c_str())) {
                for (const auto& lang : langs) {
                    if (ImGui::Selectable(lang.c_str(), lang == loc.getLanguage())) {
                        dataMgr.setLanguage(lang);
                    }
                }
                ImGui::EndCombo();
            }
        }
        
        ImGui::Separator();
        
        ImGui::InputText("Lang Code", dataState.langCode, IM_ARRAYSIZE(dataState.langCode));
        if (ImGui::Button("Load Language")) {
            dataMgr.loadLanguage(dataState.langCode);
        }
        
        ImGui::Separator();
        
        ImGui::InputText("Localize Key", dataState.localizeKey, IM_ARRAYSIZE(dataState.localizeKey));
        if (strlen(dataState.localizeKey) > 0) {
            ImGui::Text("Result: %s", dataMgr.localize(dataState.localizeKey).c_str());
        }
    }
    
    // === Hot Reload ===
    if (ImGui::CollapsingHeader("Hot Reload")) {
        bool hotReload = dataMgr.isHotReloadEnabled();
        if (ImGui::Checkbox("Enable Hot Reload", &hotReload)) {
            dataMgr.setHotReloadEnabled(hotReload);
        }
        
        ImGui::Text("Watched Files: %zu", dataMgr.getWatchedFileCount());
    }
    
    ImGui::End();
}

// ===== Build Settings State =====
struct BuildSettingsState {
    bool building = false;
    float buildProgress = 0.0f;
    std::string currentStep;
};

// ===== Build Settings Panel =====
inline void drawBuildSettingsPanel(BuildSettingsState& buildState, EditorState& state) {
    if (!state.showBuildSettings) return;
    
    ImGui::SetNextWindowSize(ImVec2(450, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Build Settings", &state.showBuildSettings)) {
        ImGui::End();
        return;
    }
    
    auto& buildMgr = luma::getBuildManager();
    auto& settings = buildMgr.getSettings();
    
    // === Project Info ===
    if (ImGui::CollapsingHeader("Project", ImGuiTreeNodeFlags_DefaultOpen)) {
        static char projectName[64];
        strncpy(projectName, settings.projectName.c_str(), sizeof(projectName) - 1);
        if (ImGui::InputText("Project Name", projectName, sizeof(projectName))) {
            settings.projectName = projectName;
        }
        
        static char version[32];
        strncpy(version, settings.version.c_str(), sizeof(version) - 1);
        if (ImGui::InputText("Version", version, sizeof(version))) {
            settings.version = version;
        }
        
        static char buildNum[16];
        strncpy(buildNum, settings.buildNumber.c_str(), sizeof(buildNum) - 1);
        if (ImGui::InputText("Build Number", buildNum, sizeof(buildNum))) {
            settings.buildNumber = buildNum;
        }
    }
    
    // === Platform ===
    if (ImGui::CollapsingHeader("Platform", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* platforms[] = {"Windows", "macOS", "iOS", "Android", "Linux", "WebGL"};
        int platformIdx = (int)settings.platform;
        if (ImGui::Combo("Target Platform", &platformIdx, platforms, IM_ARRAYSIZE(platforms))) {
            settings.platform = (luma::BuildPlatform)platformIdx;
        }
        
        const char* configs[] = {"Debug", "Development", "Release"};
        int configIdx = (int)settings.config;
        if (ImGui::Combo("Configuration", &configIdx, configs, IM_ARRAYSIZE(configs))) {
            settings.config = (luma::BuildConfig)configIdx;
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Debug Preset")) buildMgr.useDebugPreset();
        ImGui::SameLine();
        if (ImGui::Button("Dev Preset")) buildMgr.useDevelopmentPreset();
        ImGui::SameLine();
        if (ImGui::Button("Release Preset")) buildMgr.useReleasePreset();
    }
    
    // === Paths ===
    if (ImGui::CollapsingHeader("Paths")) {
        static char outputDir[256];
        strncpy(outputDir, settings.outputDir.c_str(), sizeof(outputDir) - 1);
        if (ImGui::InputText("Output Directory", outputDir, sizeof(outputDir))) {
            settings.outputDir = outputDir;
        }
        
        static char assetsDir[256];
        strncpy(assetsDir, settings.assetsDir.c_str(), sizeof(assetsDir) - 1);
        if (ImGui::InputText("Assets Directory", assetsDir, sizeof(assetsDir))) {
            settings.assetsDir = assetsDir;
        }
    }
    
    // === Options ===
    if (ImGui::CollapsingHeader("Build Options")) {
        ImGui::Checkbox("Compress Assets", &settings.compressAssets);
        ImGui::Checkbox("Strip Debug Info", &settings.stripDebugInfo);
        ImGui::Checkbox("Use Asset Bundles", &settings.useAssetBundles);
        ImGui::Checkbox("Sign Build", &settings.signBuild);
        ImGui::Checkbox("Create Installer", &settings.createInstaller);
    }
    
    // === Platform Specific ===
    if (ImGui::CollapsingHeader("Platform Settings")) {
        static char bundleId[128];
        strncpy(bundleId, settings.bundleIdentifier.c_str(), sizeof(bundleId) - 1);
        if (ImGui::InputText("Bundle Identifier", bundleId, sizeof(bundleId))) {
            settings.bundleIdentifier = bundleId;
        }
        
        if (settings.platform == luma::BuildPlatform::iOS) {
            static char teamId[32];
            strncpy(teamId, settings.teamId.c_str(), sizeof(teamId) - 1);
            if (ImGui::InputText("Team ID", teamId, sizeof(teamId))) {
                settings.teamId = teamId;
            }
        }
        
        if (settings.platform == luma::BuildPlatform::Android) {
            static char keystore[256];
            strncpy(keystore, settings.keystorePath.c_str(), sizeof(keystore) - 1);
            if (ImGui::InputText("Keystore Path", keystore, sizeof(keystore))) {
                settings.keystorePath = keystore;
            }
        }
    }
    
    // === Build ===
    ImGui::Separator();
    
    if (buildState.building) {
        ImGui::Text("Building: %s", buildState.currentStep.c_str());
        ImGui::ProgressBar(buildState.buildProgress);
        
        if (ImGui::Button("Cancel")) {
            buildState.building = false;
        }
    } else {
        if (ImGui::Button("Build", ImVec2(120, 40))) {
            buildState.building = true;
            buildState.buildProgress = 0.0f;
            
            auto result = buildMgr.build([&](const std::string& step, float progress) {
                buildState.currentStep = step;
                buildState.buildProgress = progress;
            });
            
            buildState.building = false;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Build And Run", ImVec2(120, 40))) {
            // Would build and launch
        }
    }
    
    // === Last Build Result ===
    const auto& lastResult = buildMgr.getLastResult();
    if (!lastResult.outputPath.empty()) {
        ImGui::Separator();
        
        if (lastResult.success) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Build Successful");
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "Build Failed");
            ImGui::TextWrapped("Error: %s", lastResult.errorMessage.c_str());
        }
        
        ImGui::Text("Output: %s", lastResult.outputPath.c_str());
        ImGui::Text("Build Time: %.1f seconds", lastResult.buildTimeMs / 1000.0f);
        ImGui::Text("Total Size: %.2f MB", lastResult.totalSize / (1024.0f * 1024.0f));
        
        if (!lastResult.warnings.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Warnings:");
            for (const auto& w : lastResult.warnings) {
                ImGui::BulletText("%s", w.c_str());
            }
        }
    }
    
    ImGui::End();
}

// ===== Status Bar =====
inline void drawStatusBar(int windowWidth, int windowHeight, const std::string& statusText = "") {
    ImGui::SetNextWindowPos(ImVec2(0, (float)windowHeight - EditorLayout::kStatusBarHeight));
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, EditorLayout::kStatusBarHeight));
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

// ===== Asset Browser State =====
struct AssetBrowserState {
    AssetBrowser browser;
    bool initialized = false;
    char searchBuffer[256] = {0};
    int viewMode = 0; // 0 = Grid, 1 = List
    int thumbnailSize = 96;
    int selectedAsset = -1;
    bool showCreateMenu = false;
    char newFolderName[64] = {0};
    char renameBuffer[256] = {0};
    int renamingAsset = -1;
};

// ===== Asset Browser Panel =====
inline void drawAssetBrowserPanel(AssetBrowserState& state, EditorState& editorState) {
    if (!editorState.showAssetBrowser) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Asset Browser", &editorState.showAssetBrowser)) {
        ImGui::End();
        return;
    }
    
    // Initialize if needed
    if (!state.initialized) {
        state.browser.initialize(".");
        state.initialized = true;
    }
    
    // === Toolbar ===
    // Navigation buttons
    ImGui::BeginDisabled(!state.browser.canGoBack());
    if (ImGui::Button("<")) state.browser.navigateBack();
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::BeginDisabled(!state.browser.canGoForward());
    if (ImGui::Button(">")) state.browser.navigateForward();
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::BeginDisabled(!state.browser.canGoUp());
    if (ImGui::Button("^")) state.browser.navigateUp();
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) state.browser.refresh();
    
    ImGui::SameLine();
    ImGui::Separator();
    
    // Breadcrumb path
    ImGui::SameLine();
    auto crumbs = state.browser.getBreadcrumbs();
    for (size_t i = 0; i < crumbs.size(); i++) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled("/");
            ImGui::SameLine();
        }
        if (ImGui::SmallButton(crumbs[i].first.c_str())) {
            state.browser.setCurrentPath(crumbs[i].second);
        }
    }
    
    // Search
    ImGui::SameLine(ImGui::GetWindowWidth() - 250);
    ImGui::SetNextItemWidth(200);
    if (ImGui::InputTextWithHint("##search", "Search...", state.searchBuffer, sizeof(state.searchBuffer))) {
        state.browser.setSearchText(state.searchBuffer);
    }
    
    ImGui::Separator();
    
    // === Main content ===
    float sidebarWidth = 200;
    
    // Sidebar - Folder tree
    ImGui::BeginChild("FolderTree", ImVec2(sidebarWidth, 0), true);
    auto tree = state.browser.getFolderTree();
    
    std::function<void(const AssetBrowser::FolderNode&)> drawFolderNode = [&](const AssetBrowser::FolderNode& node) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (node.children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
        if (state.browser.getCurrentPath() == node.path) flags |= ImGuiTreeNodeFlags_Selected;
        
        bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
        
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            state.browser.setCurrentPath(node.path);
        }
        
        if (open) {
            for (const auto& child : node.children) {
                drawFolderNode(child);
            }
            ImGui::TreePop();
        }
    };
    drawFolderNode(tree);
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Content area
    ImGui::BeginChild("AssetContent", ImVec2(0, 0), true);
    
    // View mode toggle and options
    ImGui::RadioButton("Grid", &state.viewMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("List", &state.viewMode, 1);
    
    if (state.viewMode == 0) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderInt("Size", &state.thumbnailSize, 48, 128);
    }
    
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
    if (ImGui::Button("+ Create")) {
        state.showCreateMenu = true;
        ImGui::OpenPopup("CreateAssetPopup");
    }
    
    // Create popup
    if (ImGui::BeginPopup("CreateAssetPopup")) {
        if (ImGui::MenuItem("New Folder")) {
            strcpy(state.newFolderName, "New Folder");
            ImGui::OpenPopup("NewFolderPopup");
        }
        if (ImGui::MenuItem("New Material")) {}
        if (ImGui::MenuItem("New Script")) {}
        if (ImGui::MenuItem("New Scene")) {}
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    // Asset grid/list
    const auto& assets = state.browser.getAssets();
    
    if (state.viewMode == 0) {
        // Grid view
        float cellSize = (float)state.thumbnailSize + 20;
        int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellSize));
        
        ImGui::Columns(columns, nullptr, false);
        
        for (size_t i = 0; i < assets.size(); i++) {
            const auto& asset = assets[i];
            
            ImGui::PushID((int)i);
            
            // Selection state
            bool selected = state.browser.isSelected(i);
            
            ImGui::BeginGroup();
            
            // Thumbnail/icon area
            ImVec2 iconSize((float)state.thumbnailSize, (float)state.thumbnailSize);
            ImVec4 bgColor = selected ? ImVec4(0.3f, 0.4f, 0.6f, 1.0f) : ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.35f, 0.5f, 1.0f));
            
            if (ImGui::Button("##icon", iconSize)) {
                state.browser.selectAsset(i);
                state.selectedAsset = (int)i;
            }
            
            ImGui::PopStyleColor(2);
            
            // Type icon overlay
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - iconSize.y - 5);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + iconSize.x / 2 - 10);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.8f), "%s", getAssetTypeName(asset.type));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            
            // Drag source for dropping into scene
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                // Store asset path for drag payload
                ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", asset.path.c_str(), asset.path.size() + 1);
                
                // Preview during drag
                ImGui::Text("%s %s", getAssetTypeIcon(asset.type), asset.name.c_str());
                ImGui::EndDragDropSource();
            }
            
            // Double-click to open
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (asset.isDirectory) {
                    state.browser.setCurrentPath(asset.path);
                } else {
                    // Call callback for asset action
                    if (editorState.onAssetDoubleClick) {
                        editorState.onAssetDoubleClick(asset.path, asset.type);
                    }
                }
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Open")) {}
                if (ImGui::MenuItem("Rename")) {
                    state.renamingAsset = (int)i;
                    strncpy(state.renameBuffer, asset.name.c_str(), sizeof(state.renameBuffer) - 1);
                }
                if (ImGui::MenuItem("Delete")) {
                    state.browser.deleteAsset(asset);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Show in Finder")) {
                    // Platform specific
                }
                ImGui::EndPopup();
            }
            
            // Name
            ImGui::TextWrapped("%s", asset.name.c_str());
            
            ImGui::EndGroup();
            
            ImGui::NextColumn();
            ImGui::PopID();
        }
        
        ImGui::Columns(1);
    } else {
        // List view
        if (ImGui::BeginTable("AssetTable", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableHeadersRow();
            
            for (size_t i = 0; i < assets.size(); i++) {
                const auto& asset = assets[i];
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                bool selected = state.browser.isSelected(i);
                if (ImGui::Selectable(asset.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    state.browser.selectAsset(i);
                }
                
                // Drag source for list view
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", asset.path.c_str(), asset.path.size() + 1);
                    ImGui::Text("%s %s", getAssetTypeIcon(asset.type), asset.name.c_str());
                    ImGui::EndDragDropSource();
                }
                
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (asset.isDirectory) {
                        state.browser.setCurrentPath(asset.path);
                    } else {
                        if (editorState.onAssetDoubleClick) {
                            editorState.onAssetDoubleClick(asset.path, asset.type);
                        }
                    }
                }
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", getAssetTypeName(asset.type));
                
                ImGui::TableNextColumn();
                if (!asset.isDirectory) {
                    ImGui::Text("%s", formatFileSize(asset.size).c_str());
                }
                
                ImGui::TableNextColumn();
                // Would format time here
                ImGui::Text("-");
            }
            
            ImGui::EndTable();
        }
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}

// ===== Visual Script State =====
struct VisualScriptState {
    std::unique_ptr<VisualScriptGraph> graph;
    Vec2 scrollOffset = {0, 0};
    float zoom = 1.0f;
    
    // Interaction state
    int selectedNodeId = -1;
    int hoveredNodeId = -1;
    int draggingNodeId = -1;
    Vec2 dragOffset;
    
    // Link creation
    bool creatingLink = false;
    uint32_t linkStartNode = 0;
    uint32_t linkStartPin = 0;
    
    // Context menu
    bool showContextMenu = false;
    Vec2 contextMenuPos;
    char searchBuffer[64] = {0};
    
    // Variables
    char newVarName[64] = {0};
    int newVarType = 0;
    
    VisualScriptState() {
        graph = std::make_unique<VisualScriptGraph>();
        graph->name = "NewScript";
    }
};

// ===== Visual Script Editor Panel =====
inline void drawVisualScriptPanel(VisualScriptState& state, EditorState& editorState) {
    if (!editorState.showVisualScript) return;
    
    ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Visual Script", &editorState.showVisualScript, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    
    auto& graph = *state.graph;
    
    // === Menu Bar ===
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                state.graph = std::make_unique<VisualScriptGraph>();
            }
            if (ImGui::MenuItem("Open...")) {}
            if (ImGui::MenuItem("Save")) {}
            if (ImGui::MenuItem("Save As...")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Export to Lua")) {
                std::string lua = graph.compileToLua();
                // Would save to file
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del")) {
                if (state.selectedNodeId >= 0) {
                    graph.deleteNode(state.selectedNodeId);
                    state.selectedNodeId = -1;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset View")) {
                state.scrollOffset = {0, 0};
                state.zoom = 1.0f;
            }
            if (ImGui::MenuItem("Zoom In")) state.zoom = std::min(2.0f, state.zoom + 0.1f);
            if (ImGui::MenuItem("Zoom Out")) state.zoom = std::max(0.5f, state.zoom - 0.1f);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    // === Left Sidebar - Variables and Properties ===
    ImGui::BeginChild("Sidebar", ImVec2(200, 0), true);
    
    // Variables
    if (ImGui::CollapsingHeader("Variables", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < graph.variables.size(); i++) {
            auto& var = graph.variables[i];
            
            ImGui::PushID((int)i);
            
            const char* typeNames[] = {"Bool", "Int", "Float", "String", "Vec3", "Object"};
            ImGui::Text("%s", var.name.c_str());
            ImGui::SameLine(120);
            ImGui::TextDisabled("%s", typeNames[(int)var.type - 1]);
            
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        ImGui::SetNextItemWidth(100);
        ImGui::InputText("##varname", state.newVarName, sizeof(state.newVarName));
        
        ImGui::SameLine();
        const char* types[] = {"Bool", "Int", "Float", "String", "Vec3"};
        ImGui::SetNextItemWidth(60);
        ImGui::Combo("##vartype", &state.newVarType, types, IM_ARRAYSIZE(types));
        
        ImGui::SameLine();
        if (ImGui::Button("+") && strlen(state.newVarName) > 0) {
            PinType pinTypes[] = {PinType::Bool, PinType::Int, PinType::Float, PinType::String, PinType::Vec3};
            graph.addVariable(state.newVarName, pinTypes[state.newVarType]);
            state.newVarName[0] = '\0';
        }
    }
    
    // Selected node properties
    if (state.selectedNodeId >= 0) {
        if (auto* node = graph.findNode(state.selectedNodeId)) {
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Node: %s", node->displayName.c_str());
                ImGui::Text("ID: %u", node->id);
                
                // Node-specific properties
                for (auto& [key, value] : node->properties) {
                    if (std::holds_alternative<std::string>(value)) {
                        static char buf[256];
                        strncpy(buf, std::get<std::string>(value).c_str(), sizeof(buf) - 1);
                        if (ImGui::InputText(key.c_str(), buf, sizeof(buf))) {
                            node->properties[key] = std::string(buf);
                        }
                    }
                }
                
                // Comment
                static char comment[256];
                strncpy(comment, node->comment.c_str(), sizeof(comment) - 1);
                if (ImGui::InputText("Comment", comment, sizeof(comment))) {
                    node->comment = comment;
                }
                
                ImGui::Checkbox("Breakpoint", &node->breakpoint);
            }
        }
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // === Canvas ===
    ImGui::BeginChild("Canvas", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    // Background grid
    float gridSize = 32.0f * state.zoom;
    ImU32 gridColor = IM_COL32(50, 50, 55, 255);
    ImU32 gridColorBold = IM_COL32(70, 70, 75, 255);
    
    for (float x = fmodf(state.scrollOffset.x, gridSize); x < canvasSize.x; x += gridSize) {
        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y),
            ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
            (int(x - state.scrollOffset.x) % (int)(gridSize * 4) == 0) ? gridColorBold : gridColor
        );
    }
    for (float y = fmodf(state.scrollOffset.y, gridSize); y < canvasSize.y; y += gridSize) {
        drawList->AddLine(
            ImVec2(canvasPos.x, canvasPos.y + y),
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
            (int(y - state.scrollOffset.y) % (int)(gridSize * 4) == 0) ? gridColorBold : gridColor
        );
    }
    
    // Draw links
    for (const auto& link : graph.links) {
        auto* fromNode = graph.findNode(link.fromNode);
        auto* toNode = graph.findNode(link.toNode);
        if (!fromNode || !toNode) continue;
        
        auto* fromPin = fromNode->findPin(link.fromPin);
        auto* toPin = toNode->findPin(link.toPin);
        if (!fromPin || !toPin) continue;
        
        // Calculate positions
        ImVec2 p1(
            canvasPos.x + (fromNode->position.x + fromNode->size.x) * state.zoom + state.scrollOffset.x,
            canvasPos.y + (fromNode->position.y + 30 + 20 * fromPin->id % 5) * state.zoom + state.scrollOffset.y
        );
        ImVec2 p4(
            canvasPos.x + toNode->position.x * state.zoom + state.scrollOffset.x,
            canvasPos.y + (toNode->position.y + 30 + 20 * toPin->id % 5) * state.zoom + state.scrollOffset.y
        );
        
        float dx = std::abs(p4.x - p1.x) * 0.5f;
        ImVec2 p2(p1.x + dx, p1.y);
        ImVec2 p3(p4.x - dx, p4.y);
        
        ImU32 color = getPinColor(fromPin->type);
        drawList->AddBezierCubic(p1, p2, p3, p4, color, 2.0f * state.zoom);
    }
    
    // Draw nodes
    for (const auto& nodePtr : graph.nodes) {
        auto& node = *nodePtr;
        
        ImVec2 nodePos(
            canvasPos.x + node.position.x * state.zoom + state.scrollOffset.x,
            canvasPos.y + node.position.y * state.zoom + state.scrollOffset.y
        );
        ImVec2 nodeSize(node.size.x * state.zoom, node.size.y * state.zoom);
        
        // Node background
        ImU32 bgColor = (state.selectedNodeId == (int)node.id) ? IM_COL32(60, 60, 70, 255) : IM_COL32(40, 40, 45, 255);
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y), bgColor, 4.0f);
        
        // Header
        drawList->AddRectFilled(nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + 24 * state.zoom), node.headerColor, 4.0f, ImDrawFlags_RoundCornersTop);
        
        // Title
        drawList->AddText(ImVec2(nodePos.x + 8, nodePos.y + 4), IM_COL32(255, 255, 255, 255), node.displayName.c_str());
        
        // Border
        ImU32 borderColor = (state.selectedNodeId == (int)node.id) ? IM_COL32(100, 150, 255, 255) : IM_COL32(80, 80, 90, 255);
        drawList->AddRect(nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y), borderColor, 4.0f);
        
        // Breakpoint indicator
        if (node.breakpoint) {
            drawList->AddCircleFilled(ImVec2(nodePos.x + nodeSize.x - 8, nodePos.y + 8), 5, IM_COL32(255, 50, 50, 255));
        }
        
        // Input pins
        float pinY = nodePos.y + 30 * state.zoom;
        for (const auto& pin : node.inputs) {
            ImVec2 pinPos(nodePos.x, pinY);
            drawList->AddCircleFilled(pinPos, 5 * state.zoom, getPinColor(pin.type));
            drawList->AddText(ImVec2(pinPos.x + 10, pinY - 7), IM_COL32(200, 200, 200, 255), pin.name.c_str());
            pinY += 20 * state.zoom;
        }
        
        // Output pins
        pinY = nodePos.y + 30 * state.zoom;
        for (const auto& pin : node.outputs) {
            ImVec2 pinPos(nodePos.x + nodeSize.x, pinY);
            drawList->AddCircleFilled(pinPos, 5 * state.zoom, getPinColor(pin.type));
            
            ImVec2 textSize = ImGui::CalcTextSize(pin.name.c_str());
            drawList->AddText(ImVec2(pinPos.x - textSize.x - 10, pinY - 7), IM_COL32(200, 200, 200, 255), pin.name.c_str());
            pinY += 20 * state.zoom;
        }
        
        // Interaction
        ImGui::SetCursorScreenPos(nodePos);
        ImGui::InvisibleButton(("node_" + std::to_string(node.id)).c_str(), nodeSize);
        
        if (ImGui::IsItemClicked()) {
            state.selectedNodeId = node.id;
        }
        
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            node.position.x += ImGui::GetIO().MouseDelta.x / state.zoom;
            node.position.y += ImGui::GetIO().MouseDelta.y / state.zoom;
        }
    }
    
    // Canvas interaction
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("canvas", canvasSize);
    
    // Pan
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(2)) {
        state.scrollOffset.x += ImGui::GetIO().MouseDelta.x;
        state.scrollOffset.y += ImGui::GetIO().MouseDelta.y;
    }
    
    // Zoom
    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            state.zoom = std::clamp(state.zoom + wheel * 0.1f, 0.25f, 2.0f);
        }
    }
    
    // Right-click context menu
    if (ImGui::IsItemClicked(1)) {
        state.showContextMenu = true;
        state.contextMenuPos = {ImGui::GetMousePos().x - canvasPos.x, ImGui::GetMousePos().y - canvasPos.y};
        ImGui::OpenPopup("NodeContextMenu");
    }
    
    // Context menu
    if (ImGui::BeginPopup("NodeContextMenu")) {
        ImGui::SetNextItemWidth(150);
        ImGui::InputTextWithHint("##search", "Search nodes...", state.searchBuffer, sizeof(state.searchBuffer));
        
        auto& library = NodeLibrary::getInstance();
        
        if (strlen(state.searchBuffer) > 0) {
            // Search results
            auto results = library.searchNodes(state.searchBuffer);
            for (const auto& def : results) {
                if (ImGui::MenuItem(def.displayName.c_str())) {
                    auto* node = graph.createNode(def.name);
                    node->position.x = (state.contextMenuPos.x - state.scrollOffset.x) / state.zoom;
                    node->position.y = (state.contextMenuPos.y - state.scrollOffset.y) / state.zoom;
                    state.searchBuffer[0] = '\0';
                }
            }
        } else {
            // Categories
            NodeCategory categories[] = {
                NodeCategory::Events, NodeCategory::Flow, NodeCategory::Math,
                NodeCategory::Logic, NodeCategory::Variables, NodeCategory::Transform,
                NodeCategory::Physics, NodeCategory::Audio, NodeCategory::Input, NodeCategory::Debug
            };
            
            for (auto cat : categories) {
                if (ImGui::BeginMenu(getCategoryName(cat))) {
                    auto nodes = library.getNodesInCategory(cat);
                    for (const auto& def : nodes) {
                        if (ImGui::MenuItem(def.displayName.c_str())) {
                            auto* node = graph.createNode(def.name);
                            node->position.x = (state.contextMenuPos.x - state.scrollOffset.x) / state.zoom;
                            node->position.y = (state.contextMenuPos.y - state.scrollOffset.y) / state.zoom;
                        }
                    }
                    ImGui::EndMenu();
                }
            }
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}

// ===== Character Creator State =====
struct CharacterCreatorState {
    // Character reference (managed externally)
    void* character = nullptr;  // Character*
    void* blendShapeMesh = nullptr;  // BlendShapeMesh*
    
    // UI state
    int currentTab = 0;  // 0=Presets, 1=Body, 2=Face, 3=BlendShape, 4=Export
    int bodySubTab = 0;
    int faceSubTab = 0;
    
    // Preview
    bool autoRotate = true;
    float rotationY = 0.0f;
    
    // Character name
    char characterName[256] = "MyCharacter";
    
    // Preset system
    int selectedPresetCategory = 0;  // 0=All, 1=Realistic, 2=Anime, 3=Cartoon, etc.
    int selectedPresetIndex = -1;
    std::string selectedPresetId;
    bool showPresetBrowser = false;
    bool presetApplied = false;
    
    // Body parameters (local copy for UI)
    int gender = 0;  // 0=Male, 1=Female, 2=Neutral
    int ageGroup = 3;  // 0=Child, 1=Teen, 2=YoungAdult, 3=Adult, 4=Senior
    float height = 0.5f;
    float weight = 0.5f;
    float muscularity = 0.3f;
    float bodyFat = 0.3f;
    float shoulderWidth = 0.5f;
    float chestSize = 0.5f;
    float waistSize = 0.5f;
    float hipWidth = 0.5f;
    float armLength = 0.5f;
    float armThickness = 0.5f;
    float legLength = 0.5f;
    float thighThickness = 0.5f;
    float bustSize = 0.5f;  // For female
    float skinColor[3] = {0.85f, 0.65f, 0.5f};
    
    // Face parameters
    float faceWidth = 0.5f;
    float faceLength = 0.5f;
    float faceRoundness = 0.5f;
    float eyeSize = 0.5f;
    float eyeSpacing = 0.5f;
    float eyeHeight = 0.5f;
    float eyeAngle = 0.5f;
    float eyeColor[3] = {0.3f, 0.4f, 0.2f};
    float noseLength = 0.5f;
    float noseWidth = 0.5f;
    float noseHeight = 0.5f;
    float noseBridge = 0.5f;
    float mouthWidth = 0.5f;
    float upperLipThickness = 0.5f;
    float lowerLipThickness = 0.5f;
    float jawWidth = 0.5f;
    float jawLine = 0.5f;
    float chinLength = 0.5f;
    float chinWidth = 0.5f;
    
    // BlendShape direct control
    std::vector<std::pair<std::string, float>> blendShapeWeights;
    
    // Export
    int exportFormat = 0;  // 0=GLB, 1=glTF, 2=FBX, 3=OBJ, 4=VRM
    bool exportSkeleton = true;
    bool exportBlendShapes = true;
    bool exportTextures = true;
    bool exportMaterials = true;
    bool embedTextures = true;
    char exportPath[512] = "";
    bool exportInProgress = false;
    float exportProgress = 0.0f;
    std::string exportStatus;
    std::string lastExportPath;
    bool exportSuccess = false;
    
    // Statistics
    uint32_t vertexCount = 0;
    uint32_t triangleCount = 0;
    uint32_t blendShapeCount = 0;
    uint32_t boneCount = 0;
    
    // AI Model status
    bool showAIModelSetup = false;
    bool aiModelsReady = false;
    std::string aiModelStatus;
    
    // Clothing state
    int clothingCategory = 0;  // Current category being viewed
    std::string selectedClothingId;
    float clothingColorEdit[3] = {1.0f, 1.0f, 1.0f};
    std::vector<std::pair<std::string, std::string>> equippedClothing;  // slot, assetId
    
    // Animation/Pose state
    int poseCategory = 0;
    std::string selectedPose;
    std::string currentAnimation;
    float animationTime = 0.0f;
    bool animationPlaying = false;
    float animationSpeed = 1.0f;
    
    // Stylized rendering state
    int renderingStyle = 0;  // 0=Realistic, 1=Anime, 2=Cartoon, 3=Painterly, 4=Sketch
    bool outlineEnabled = false;
    float outlineThickness = 0.003f;
    float outlineColor[3] = {0.1f, 0.1f, 0.15f};
    int celShadingBands = 3;
    bool rimLightEnabled = true;
    float rimLightIntensity = 0.4f;
    float colorVibrancy = 1.0f;
    
    // Texture state
    int skinPreset = 0;  // 0=Caucasian, 1=Asian, 2=African, 3=Latino, 4=MiddleEastern
    float skinSaturation = 1.0f;
    float skinBrightness = 1.0f;
    float skinRoughness = 0.5f;
    float poreIntensity = 0.5f;
    float wrinkleIntensity = 0.0f;
    float freckleIntensity = 0.0f;
    float freckleColor[3] = {0.6f, 0.4f, 0.3f};
    float sssIntensity = 0.3f;
    
    // Eye texture
    int eyeColorPreset = 0;  // 0=Brown, 1=Blue, 2=Green, 3=Hazel, 4=Gray
    float irisSize = 0.5f;
    float pupilSize = 0.3f;
    float irisDetail = 0.7f;
    float scleraVeins = 0.1f;
    float eyeWetness = 0.8f;
    
    // Lip texture
    float lipColor[3] = {0.75f, 0.45f, 0.45f};
    float lipGlossiness = 0.4f;
    float lipChapped = 0.0f;
    
    // Texture generation
    int textureResolution = 1;  // 0=512, 1=1024, 2=2048
    bool textureNeedsUpdate = true;
    
    // Hair state
    int hairStyleIndex = 0;
    int hairColorPreset = 0;  // 0=Black, 1=DarkBrown, 2=Brown, etc.
    float hairColor[3] = {0.15f, 0.1f, 0.05f};
    bool useCustomHairColor = false;
    bool hairNeedsUpdate = true;
    std::vector<std::string> availableHairStyles;
    
    // Callbacks
    std::function<void()> onInitialize;
    std::function<void()> onRandomize;
    std::function<void(int category)> onRandomizeInStyle;  // Random within category
    std::function<void(int)> onPresetSelect;
    std::function<void(const std::string& presetId)> onApplyPreset;
    std::function<void()> onPhotoImport;
    std::function<void(const std::string&)> onPhotoProcess;  // Actual processing
    std::function<void(const std::string& path, int format, bool skeleton, bool blendShapes, bool textures)> onExport;
    std::function<void()> onParameterChanged;
    std::function<void(const std::string&, float)> onBlendShapeChanged;
    std::function<void(const std::string&, const std::string&)> onImportAIModel;
    
    // Clothing callbacks
    std::function<void(const std::string&)> onEquipClothing;
    std::function<void(const std::string&)> onUnequipClothing;
    std::function<void(const std::string&, float, float, float)> onClothingColorChange;
    std::function<std::vector<std::pair<std::string, std::string>>()> getAvailableClothing;  // Returns id, name pairs
    
    // Animation callbacks
    std::function<void(const std::string&)> onApplyPose;
    std::function<void(const std::string&)> onPlayAnimation;
    std::function<void()> onStopAnimation;
    
    // Style callbacks
    std::function<void(int)> onStyleChange;
    std::function<void()> onStyleSettingsChange;
    
    // Texture callbacks
    std::function<void()> onTextureUpdate;
    std::function<void(int)> onSkinPresetChange;
    std::function<void(int)> onEyeColorPresetChange;
    
    // Hair callbacks
    std::function<void(const std::string&)> onHairStyleChange;
    std::function<void(int)> onHairColorPresetChange;
    std::function<void(float, float, float)> onHairColorChange;
    
    // === NEW: Pose Editor State ===
    int poseEditorBoneCategory = 0;  // 0=All, 1=Spine, 2=LeftArm, 3=RightArm, 4=LeftLeg, 5=RightLeg, 6=Head
    std::string selectedBoneName;
    float boneRotationX = 0.0f;
    float boneRotationY = 0.0f;
    float boneRotationZ = 0.0f;
    bool showPoseLibrary = true;
    int selectedPoseCategory = 0;  // 0=Reference, 1=Standing, 2=Action, 3=Sitting, 4=Gesture
    std::string selectedPoseName;
    bool poseAutoMirror = false;
    
    // === NEW: Material Library State ===
    int materialCategory = 0;  // 0=All, 1=Metal, 2=Wood, 3=Stone, etc.
    std::string selectedMaterialId;
    bool showMaterialBrowser = false;
    
    // === NEW: Advanced Hair Rendering ===
    float hairSpecularStrength = 1.0f;
    float hairSpecularShift = 0.1f;
    float hairTransmission = 0.3f;
    float hairScatter = 0.2f;
    float hairCurlFrequency = 2.0f;
    float hairCurlAmplitude = 0.01f;
    float hairFrizz = 0.005f;
    float hairClumping = 0.3f;
    
    // === NEW: Eye Rendering ===
    float eyeIrisDepth = 0.02f;
    float eyeCorneaBulge = 0.03f;
    float eyeCausticStrength = 0.3f;
    float eyeReflection = 0.5f;
    float eyePupilDilation = 0.0f;  // -1 to 1
    
    // === NEW: Skin SSS ===
    float skinSubsurfaceStrength = 0.5f;
    float skinSubsurfaceRadius = 0.01f;
    float skinTranslucency = 0.3f;
    float skinOilAmount = 0.3f;
    float skinPoreDepth = 0.1f;
    float skinBlush = 0.0f;
    float skinBlushColor[3] = {0.9f, 0.4f, 0.4f};
    
    // === NEW: Animation Editor State ===
    bool showAnimationTimeline = false;
    bool showCurveEditor = false;
    int animEditorSelectedTrack = -1;
    float animEditorZoom = 1.0f;
    float animEditorScroll = 0.0f;
    bool animEditorAutoKey = false;
    bool animEditorSnapToFrame = true;
    int animEditorInterpolation = 1;  // 0=Constant, 1=Linear, 2=Bezier, etc.
    bool animEditorShowGhosts = false;
    int animEditorGhostFrames = 3;
    
    // === NEW: Pose Editor Callbacks ===
    std::function<void(const std::string&)> onBoneSelect;
    std::function<void(const std::string&, float, float, float)> onBoneRotate;
    std::function<void()> onPoseReset;
    std::function<void()> onPoseMirror;
    std::function<void(const std::string&)> onPoseLoad;
    std::function<void(const std::string&)> onPoseSave;
    
    // === NEW: Material Callbacks ===
    std::function<void(const std::string&)> onMaterialSelect;
    std::function<std::vector<std::pair<std::string, std::string>>()> getMaterialList;  // id, name
    
    // === NEW: Advanced Rendering Callbacks ===
    std::function<void()> onHairRenderingUpdate;
    std::function<void()> onEyeRenderingUpdate;
    std::function<void()> onSkinRenderingUpdate;
    
    // === NEW: Animation Editor Callbacks ===
    std::function<void(float)> onAnimEditorSeek;
    std::function<void()> onAnimEditorAddKeyframe;
    std::function<void()> onAnimEditorDeleteKeyframe;
    std::function<void(int)> onAnimEditorSetInterpolation;
    
    // Initialized
    bool initialized = false;
};

// ===== Character Creator Panel =====
inline void drawCharacterCreatorPanel(CharacterCreatorState& state, EditorState& editorState) {
    if (!editorState.showCharacterCreator) return;
    
    ImGui::SetNextWindowSize(ImVec2(420, 650), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 80), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Character Creator", &editorState.showCharacterCreator)) {
        
        // Initialize button if not initialized
        if (!state.initialized) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Character Creator not initialized");
            ImGui::Separator();
            ImGui::TextWrapped("Click 'Initialize' to create a new character with procedural model.");
            ImGui::TextWrapped("The character will appear in the 3D viewport to the right of the scene center.");
            ImGui::Spacing();
            if (ImGui::Button("Initialize Character Creator", ImVec2(-1, 40))) {
                if (state.onInitialize) {
                    state.onInitialize();
                }
                state.initialized = true;
            }
            ImGui::End();
            return;
        }
        
        // Character name
        ImGui::InputText("Name", state.characterName, sizeof(state.characterName));
        ImGui::Separator();
        
        // Tab bar
        if (ImGui::BeginTabBar("CharacterTabs")) {
            // === Presets Tab ===
            if (ImGui::BeginTabItem("Presets")) {
                state.currentTab = 0;
                
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Quick Start Presets");
                ImGui::TextDisabled("Select a preset to start quickly");
                ImGui::Separator();
                
                // Category filter
                const char* categories[] = {"All", "Fantasy", "Wuxia", "Gufeng", "Anime", "Cartoon", "Sci-Fi", "Realistic"};
                ImGui::SetNextItemWidth(150);
                ImGui::Combo("Category", &state.selectedPresetCategory, categories, 8);
                
                ImGui::SameLine();
                
                // Randomize button
                if (ImGui::Button("Randomize")) {
                    if (state.selectedPresetCategory == 0) {
                        if (state.onRandomize) state.onRandomize();
                    } else {
                        if (state.onRandomizeInStyle) state.onRandomizeInStyle(state.selectedPresetCategory - 1);
                    }
                }
                
                ImGui::Separator();
                ImGui::Spacing();
                
                // Preset grid (2 columns)
                float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) / 2;
                float buttonHeight = 80;
                
                // Define presets with their categories
                struct PresetInfo {
                    const char* id;
                    const char* name;
                    const char* nameCN;
                    int category;  // 1=Realistic, 2=Anime, 3=Cartoon, 4=Fantasy, 5=SciFi
                    ImVec4 skinColor;
                    ImVec4 hairColor;
                };
                
                // Category: 1=Fantasy, 2=Wuxia, 3=Gufeng, 4=Anime, 5=Cartoon, 6=SciFi, 7=Realistic
                PresetInfo presets[] = {
                    // === 西幻 Fantasy ===
                    {"fantasy_elf", "Elf", "精灵", 1, {0.98f, 0.95f, 0.92f, 1}, {0.95f, 0.92f, 0.85f, 1}},
                    {"fantasy_paladin", "Paladin", "圣骑士", 1, {0.88f, 0.75f, 0.65f, 1}, {0.8f, 0.65f, 0.4f, 1}},
                    {"fantasy_dark_mage", "Dark Mage", "暗黑法师", 1, {0.85f, 0.82f, 0.8f, 1}, {0.08f, 0.05f, 0.12f, 1}},
                    {"fantasy_orc", "Orc Warrior", "兽人战士", 1, {0.4f, 0.55f, 0.35f, 1}, {0.1f, 0.1f, 0.1f, 1}},
                    // === 武侠 Wuxia ===
                    {"wuxia_swordsman", "Swordsman", "剑客", 2, {0.9f, 0.78f, 0.65f, 1}, {0.08f, 0.06f, 0.04f, 1}},
                    {"wuxia_female_knight", "Female Knight", "女侠", 2, {0.95f, 0.85f, 0.75f, 1}, {0.05f, 0.03f, 0.02f, 1}},
                    {"wuxia_monk", "Martial Monk", "武僧", 2, {0.85f, 0.7f, 0.55f, 1}, {0.5f, 0.5f, 0.5f, 1}},
                    // === 古风 Gufeng ===
                    {"gufeng_xianxia_hero", "Xianxia Hero", "仙侠少年", 3, {0.95f, 0.88f, 0.8f, 1}, {0.05f, 0.03f, 0.02f, 1}},
                    {"gufeng_fairy", "Fairy Maiden", "仙子", 3, {0.98f, 0.95f, 0.92f, 1}, {0.1f, 0.08f, 0.05f, 1}},
                    {"gufeng_emperor", "Emperor", "帝王", 3, {0.92f, 0.82f, 0.72f, 1}, {0.05f, 0.03f, 0.02f, 1}},
                    {"gufeng_princess", "Princess", "公主", 3, {0.96f, 0.9f, 0.85f, 1}, {0.05f, 0.03f, 0.02f, 1}},
                    // === 动漫 Anime ===
                    {"anime_girl", "Anime Girl", "动漫少女", 4, {0.98f, 0.92f, 0.88f, 1}, {1.0f, 0.6f, 0.7f, 1}},
                    {"anime_boy", "Anime Boy", "动漫少年", 4, {0.95f, 0.88f, 0.82f, 1}, {0.05f, 0.05f, 0.1f, 1}},
                    {"anime_chibi", "Chibi", "Q版角色", 4, {1.0f, 0.95f, 0.9f, 1}, {0.9f, 0.7f, 0.3f, 1}},
                    // === 卡通 Cartoon ===
                    {"cartoon_western", "Western Cartoon", "西方卡通", 5, {0.95f, 0.85f, 0.7f, 1}, {0.1f, 0.08f, 0.05f, 1}},
                    {"cartoon_pixar", "Pixar Style", "皮克斯风格", 5, {0.92f, 0.78f, 0.65f, 1}, {0.35f, 0.22f, 0.12f, 1}},
                    // === 科幻 Sci-Fi ===
                    {"scifi_cyborg", "Cyborg", "赛博格", 6, {0.75f, 0.72f, 0.7f, 1}, {0.3f, 0.3f, 0.3f, 1}},
                    {"scifi_alien", "Alien", "外星人", 6, {0.6f, 0.7f, 0.8f, 1}, {0.5f, 0.5f, 0.5f, 1}},
                    // === 写实 Realistic ===
                    {"realistic_athlete", "Athlete", "运动员", 7, {0.75f, 0.55f, 0.4f, 1}, {0.05f, 0.05f, 0.05f, 1}},
                    {"realistic_child", "Child", "儿童", 7, {0.92f, 0.78f, 0.68f, 1}, {0.35f, 0.22f, 0.12f, 1}},
                    {"realistic_elderly", "Elderly", "老年人", 7, {0.88f, 0.72f, 0.62f, 1}, {0.7f, 0.7f, 0.7f, 1}},
                    {"realistic_business_man", "Business Man", "商务男士", 7, {0.85f, 0.7f, 0.6f, 1}, {0.15f, 0.1f, 0.05f, 1}},
                    {"realistic_business_woman", "Business Woman", "商务女士", 7, {0.9f, 0.75f, 0.65f, 1}, {0.2f, 0.12f, 0.08f, 1}},
                };
                
                int count = sizeof(presets) / sizeof(presets[0]);
                int col = 0;
                
                for (int i = 0; i < count; i++) {
                    // Filter by category
                    if (state.selectedPresetCategory != 0 && presets[i].category != state.selectedPresetCategory) {
                        continue;
                    }
                    
                    bool isSelected = (state.selectedPresetId == presets[i].id);
                    
                    ImGui::PushID(i);
                    
                    // Styled button with preview colors
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    
                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                    }
                    
                    if (ImGui::Button("##preset", ImVec2(buttonWidth, buttonHeight))) {
                        state.selectedPresetId = presets[i].id;
                        if (state.onApplyPreset) {
                            state.onApplyPreset(presets[i].id);
                        }
                    }
                    
                    if (isSelected) {
                        ImGui::PopStyleColor();
                    }
                    
                    // Draw preview colors on button
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    float circleY = pos.y + 25;
                    
                    // Skin color circle
                    dl->AddCircleFilled(ImVec2(pos.x + 25, circleY), 12, 
                        ImGui::ColorConvertFloat4ToU32(presets[i].skinColor));
                    dl->AddCircle(ImVec2(pos.x + 25, circleY), 12, 
                        IM_COL32(100, 100, 100, 255));
                    
                    // Hair color circle
                    dl->AddCircleFilled(ImVec2(pos.x + 50, circleY), 12, 
                        ImGui::ColorConvertFloat4ToU32(presets[i].hairColor));
                    dl->AddCircle(ImVec2(pos.x + 50, circleY), 12, 
                        IM_COL32(100, 100, 100, 255));
                    
                    // Text
                    dl->AddText(ImVec2(pos.x + 70, pos.y + 15), IM_COL32(255, 255, 255, 255), presets[i].name);
                    dl->AddText(ImVec2(pos.x + 70, pos.y + 35), IM_COL32(180, 180, 180, 255), presets[i].nameCN);
                    
                    // Category badge: 1=Fantasy, 2=Wuxia, 3=Gufeng, 4=Anime, 5=Cartoon, 6=SciFi, 7=Realistic
                    const char* catBadges[] = {"", "F", "W", "G", "A", "C", "S", "R"};
                    ImVec4 catColors[] = {
                        {0, 0, 0, 0},
                        {0.6f, 0.4f, 0.8f, 1},  // Fantasy - purple
                        {0.8f, 0.5f, 0.2f, 1},  // Wuxia - orange
                        {0.9f, 0.3f, 0.4f, 1},  // Gufeng - red
                        {1.0f, 0.5f, 0.7f, 1},  // Anime - pink
                        {0.9f, 0.7f, 0.2f, 1},  // Cartoon - yellow
                        {0.3f, 0.7f, 0.9f, 1},  // SciFi - cyan
                        {0.3f, 0.6f, 0.3f, 1}   // Realistic - green
                    };
                    
                    dl->AddRectFilled(
                        ImVec2(pos.x + buttonWidth - 25, pos.y + 5),
                        ImVec2(pos.x + buttonWidth - 5, pos.y + 22),
                        ImGui::ColorConvertFloat4ToU32(catColors[presets[i].category]),
                        4.0f);
                    dl->AddText(ImVec2(pos.x + buttonWidth - 20, pos.y + 5), 
                        IM_COL32(255, 255, 255, 255), catBadges[presets[i].category]);
                    
                    ImGui::PopID();
                    
                    col++;
                    if (col < 2) {
                        ImGui::SameLine();
                    } else {
                        col = 0;
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // === Body Tab ===
            if (ImGui::BeginTabItem("Body")) {
                state.currentTab = 1;
                
                // Gender selection
                ImGui::Text("Gender");
                bool genderChanged = false;
                if (ImGui::RadioButton("Male", &state.gender, 0)) genderChanged = true;
                ImGui::SameLine();
                if (ImGui::RadioButton("Female", &state.gender, 1)) genderChanged = true;
                ImGui::SameLine();
                if (ImGui::RadioButton("Neutral", &state.gender, 2)) genderChanged = true;
                
                if (genderChanged && state.onParameterChanged) {
                    state.onParameterChanged();
                }
                
                ImGui::Separator();
                
                // Presets
                if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const char* malePresets[] = {"Slim", "Average", "Muscular", "Heavy", "Elderly"};
                    const char* femalePresets[] = {"Slim", "Average", "Curvy", "Athletic", "Elderly"};
                    const char** presets = (state.gender == 1) ? femalePresets : malePresets;
                    
                    for (int i = 0; i < 5; i++) {
                        if (ImGui::Button(presets[i], ImVec2(72, 0))) {
                            int presetIdx = (state.gender == 1) ? (i + 5) : i;
                            if (state.onPresetSelect) state.onPresetSelect(presetIdx);
                        }
                        if (i < 4) ImGui::SameLine();
                    }
                }
                
                ImGui::Separator();
                
                // Overall parameters
                if (ImGui::CollapsingHeader("Overall", ImGuiTreeNodeFlags_DefaultOpen)) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Height", &state.height, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Weight", &state.weight, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Muscularity", &state.muscularity, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Body Fat", &state.bodyFat, 0.0f, 1.0f);
                    
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Torso
                if (ImGui::CollapsingHeader("Torso")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Shoulder Width", &state.shoulderWidth, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Chest Size", &state.chestSize, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Waist Size", &state.waistSize, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Hip Width", &state.hipWidth, 0.0f, 1.0f);
                    
                    if (state.gender == 1) {
                        changed |= ImGui::SliderFloat("Bust Size", &state.bustSize, 0.0f, 1.0f);
                    }
                    
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Limbs
                if (ImGui::CollapsingHeader("Limbs")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Arm Length", &state.armLength, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Arm Thickness", &state.armThickness, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Leg Length", &state.legLength, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Thigh Thickness", &state.thighThickness, 0.0f, 1.0f);
                    
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Skin color
                if (ImGui::CollapsingHeader("Skin")) {
                    if (ImGui::ColorEdit3("Skin Color", state.skinColor)) {
                        if (state.onParameterChanged) state.onParameterChanged();
                    }
                    
                    ImGui::Text("Presets:");
                    struct SkinPreset { const char* name; float r, g, b; };
                    SkinPreset skinPresets[] = {
                        {"Fair", 0.95f, 0.80f, 0.70f},
                        {"Light", 0.90f, 0.72f, 0.60f},
                        {"Medium", 0.80f, 0.60f, 0.45f},
                        {"Olive", 0.70f, 0.55f, 0.40f},
                        {"Brown", 0.55f, 0.40f, 0.30f},
                        {"Dark", 0.35f, 0.25f, 0.20f}
                    };
                    
                    for (int i = 0; i < 6; i++) {
                        ImVec4 c(skinPresets[i].r, skinPresets[i].g, skinPresets[i].b, 1.0f);
                        if (ImGui::ColorButton(skinPresets[i].name, c, 0, ImVec2(30, 30))) {
                            state.skinColor[0] = skinPresets[i].r;
                            state.skinColor[1] = skinPresets[i].g;
                            state.skinColor[2] = skinPresets[i].b;
                            if (state.onParameterChanged) state.onParameterChanged();
                        }
                        ImGui::SameLine();
                        ImGui::Text("%s", skinPresets[i].name);
                        if (i < 5 && (i % 2 == 0)) ImGui::SameLine(200);
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // === Face Tab ===
            if (ImGui::BeginTabItem("Face")) {
                state.currentTab = 1;
                
                // AI Photo Import Section
                if (ImGui::CollapsingHeader("Photo to Face (AI)", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // AI Model status
                    if (state.aiModelsReady) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[OK] AI Models Ready");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[!] AI Models Not Configured");
                        ImGui::TextWrapped("Import ONNX models to enable photo-to-face feature.");
                    }
                    
                    if (ImGui::Button("AI Model Setup...", ImVec2(150, 0))) {
                        state.showAIModelSetup = true;
                    }
                    ImGui::SameLine();
                    
                    // Photo import button
                    if (!state.aiModelsReady) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Import from Photo...", ImVec2(-1, 0))) {
                        if (state.onPhotoImport) state.onPhotoImport();
                    }
                    if (!state.aiModelsReady) {
                        ImGui::EndDisabled();
                    }
                    
                    if (!state.aiModelStatus.empty()) {
                        ImGui::TextWrapped("%s", state.aiModelStatus.c_str());
                    }
                }
                
                ImGui::Separator();
                
                // Face shape
                if (ImGui::CollapsingHeader("Face Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Face Width", &state.faceWidth, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Face Length", &state.faceLength, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Face Roundness", &state.faceRoundness, 0.0f, 1.0f);
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Eyes
                if (ImGui::CollapsingHeader("Eyes")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Eye Size", &state.eyeSize, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Eye Spacing", &state.eyeSpacing, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Eye Height", &state.eyeHeight, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Eye Angle", &state.eyeAngle, 0.0f, 1.0f);
                    
                    if (ImGui::ColorEdit3("Eye Color", state.eyeColor)) changed = true;
                    
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Nose
                if (ImGui::CollapsingHeader("Nose")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Nose Length", &state.noseLength, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Nose Width", &state.noseWidth, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Nose Height", &state.noseHeight, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Nose Bridge", &state.noseBridge, 0.0f, 1.0f);
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Mouth
                if (ImGui::CollapsingHeader("Mouth")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Mouth Width", &state.mouthWidth, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Upper Lip", &state.upperLipThickness, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Lower Lip", &state.lowerLipThickness, 0.0f, 1.0f);
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Jaw
                if (ImGui::CollapsingHeader("Jaw & Chin")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Jaw Width", &state.jawWidth, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Jaw Line", &state.jawLine, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Chin Length", &state.chinLength, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Chin Width", &state.chinWidth, 0.0f, 1.0f);
                    if (changed && state.onParameterChanged) state.onParameterChanged();
                }
                
                // Expressions (quick buttons)
                if (ImGui::CollapsingHeader("Expressions")) {
                    if (ImGui::Button("Neutral", ImVec2(70, 0))) {}
                    ImGui::SameLine();
                    if (ImGui::Button("Smile", ImVec2(70, 0))) {}
                    ImGui::SameLine();
                    if (ImGui::Button("Frown", ImVec2(70, 0))) {}
                    
                    if (ImGui::Button("Surprise", ImVec2(70, 0))) {}
                    ImGui::SameLine();
                    if (ImGui::Button("Angry", ImVec2(70, 0))) {}
                    ImGui::SameLine();
                    if (ImGui::Button("Sad", ImVec2(70, 0))) {}
                }
                
                ImGui::EndTabItem();
            }
            
            // === BlendShape Tab ===
            if (ImGui::BeginTabItem("BlendShapes")) {
                state.currentTab = 2;
                
                ImGui::Text("BlendShape Channels: %zu", state.blendShapeWeights.size());
                ImGui::Separator();
                
                // Direct BlendShape control
                if (ImGui::CollapsingHeader("Direct Control", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Sample BlendShapes for demo
                    if (state.blendShapeWeights.empty()) {
                        state.blendShapeWeights = {
                            {"body_height", 0.0f},
                            {"body_weight", 0.0f},
                            {"body_muscle", 0.0f},
                            {"body_fat", 0.0f},
                            {"face_width", 0.0f},
                            {"face_length", 0.0f},
                            {"eye_size", 0.0f},
                            {"nose_length", 0.0f}
                        };
                    }
                    
                    for (auto& [name, weight] : state.blendShapeWeights) {
                        if (ImGui::SliderFloat(name.c_str(), &weight, -1.0f, 1.0f)) {
                            if (state.onBlendShapeChanged) {
                                state.onBlendShapeChanged(name, weight);
                            }
                        }
                    }
                }
                
                ImGui::Separator();
                if (ImGui::Button("Reset All", ImVec2(-1, 30))) {
                    for (auto& [name, weight] : state.blendShapeWeights) {
                        weight = 0.0f;
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // === Clothing Tab ===
            if (ImGui::BeginTabItem("Clothing")) {
                state.currentTab = 3;
                
                // Category selector
                const char* categories[] = {"Tops", "Bottoms", "Footwear", "Accessories"};
                ImGui::Combo("Category", &state.clothingCategory, categories, 4);
                
                ImGui::Separator();
                
                // Available items grid
                if (ImGui::CollapsingHeader("Available Items", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::BeginChild("ClothingGrid", ImVec2(0, 200), true);
                    
                    // Sample clothing items based on category
                    struct ClothingPreview {
                        const char* id;
                        const char* name;
                        float color[3];
                    };
                    
                    std::vector<ClothingPreview> items;
                    
                    if (state.clothingCategory == 0) {  // Tops
                        items = {
                            {"tshirt_white", "T-Shirt (White)", {0.95f, 0.95f, 0.95f}},
                            {"tshirt_black", "T-Shirt (Black)", {0.1f, 0.1f, 0.1f}},
                            {"tshirt_red", "T-Shirt (Red)", {0.8f, 0.15f, 0.15f}},
                            {"tshirt_blue", "T-Shirt (Blue)", {0.2f, 0.3f, 0.7f}}
                        };
                    } else if (state.clothingCategory == 1) {  // Bottoms
                        items = {
                            {"pants_jeans", "Jeans (Blue)", {0.2f, 0.3f, 0.5f}},
                            {"pants_black", "Pants (Black)", {0.1f, 0.1f, 0.1f}},
                            {"pants_khaki", "Pants (Khaki)", {0.76f, 0.69f, 0.57f}},
                            {"skirt_black", "Skirt (Black)", {0.1f, 0.1f, 0.1f}},
                            {"skirt_red", "Skirt (Red)", {0.7f, 0.15f, 0.15f}}
                        };
                    } else if (state.clothingCategory == 2) {  // Footwear
                        items = {
                            {"shoes_black", "Shoes (Black)", {0.1f, 0.1f, 0.1f}},
                            {"shoes_brown", "Shoes (Brown)", {0.4f, 0.25f, 0.15f}}
                        };
                    }
                    
                    int columns = 3;
                    for (size_t i = 0; i < items.size(); i++) {
                        ImGui::PushID(items[i].id);
                        
                        // Check if equipped
                        bool isEquipped = false;
                        for (const auto& [slot, id] : state.equippedClothing) {
                            if (id == items[i].id) {
                                isEquipped = true;
                                break;
                            }
                        }
                        
                        // Color preview button
                        ImVec4 col(items[i].color[0], items[i].color[1], items[i].color[2], 1.0f);
                        
                        if (isEquipped) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                        }
                        
                        if (ImGui::ColorButton(items[i].name, col, 0, ImVec2(60, 60))) {
                            state.selectedClothingId = items[i].id;
                            state.clothingColorEdit[0] = items[i].color[0];
                            state.clothingColorEdit[1] = items[i].color[1];
                            state.clothingColorEdit[2] = items[i].color[2];
                        }
                        
                        if (isEquipped) {
                            ImGui::PopStyleColor();
                        }
                        
                        // Tooltip
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("%s", items[i].name);
                            if (isEquipped) {
                                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "(Equipped)");
                            }
                            ImGui::EndTooltip();
                        }
                        
                        if ((i + 1) % columns != 0 && i < items.size() - 1) {
                            ImGui::SameLine();
                        }
                        
                        ImGui::PopID();
                    }
                    
                    ImGui::EndChild();
                }
                
                ImGui::Separator();
                
                // Selected item controls
                if (!state.selectedClothingId.empty()) {
                    ImGui::Text("Selected: %s", state.selectedClothingId.c_str());
                    
                    // Color editor
                    if (ImGui::ColorEdit3("Color", state.clothingColorEdit)) {
                        if (state.onClothingColorChange) {
                            state.onClothingColorChange(state.selectedClothingId,
                                state.clothingColorEdit[0],
                                state.clothingColorEdit[1],
                                state.clothingColorEdit[2]);
                        }
                    }
                    
                    // Equip/Unequip buttons
                    bool isEquipped = false;
                    for (const auto& [slot, id] : state.equippedClothing) {
                        if (id == state.selectedClothingId) {
                            isEquipped = true;
                            break;
                        }
                    }
                    
                    if (isEquipped) {
                        if (ImGui::Button("Unequip", ImVec2(-1, 30))) {
                            if (state.onUnequipClothing) {
                                state.onUnequipClothing(state.selectedClothingId);
                            }
                        }
                    } else {
                        if (ImGui::Button("Equip", ImVec2(-1, 30))) {
                            if (state.onEquipClothing) {
                                state.onEquipClothing(state.selectedClothingId);
                            }
                        }
                    }
                }
                
                ImGui::Separator();
                
                // Currently equipped
                if (ImGui::CollapsingHeader("Currently Equipped", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (state.equippedClothing.empty()) {
                        ImGui::TextDisabled("No clothing equipped");
                    } else {
                        for (const auto& [slot, id] : state.equippedClothing) {
                            ImGui::BulletText("%s: %s", slot.c_str(), id.c_str());
                            ImGui::SameLine();
                            ImGui::PushID(id.c_str());
                            if (ImGui::SmallButton("X")) {
                                if (state.onUnequipClothing) {
                                    state.onUnequipClothing(id);
                                }
                            }
                            ImGui::PopID();
                        }
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // === Animation Tab (Enhanced with Pose Editor) ===
            if (ImGui::BeginTabItem("Animation")) {
                state.currentTab = 4;
                
                // Sub-tabs for Animation vs Pose
                static int animSubTab = 0;
                ImGui::RadioButton("Pose Library", &animSubTab, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Bone Editor", &animSubTab, 1);
                ImGui::SameLine();
                ImGui::RadioButton("Playback", &animSubTab, 2);
                ImGui::SameLine();
                ImGui::RadioButton("Timeline", &animSubTab, 3);
                
                ImGui::Separator();
                
                if (animSubTab == 0) {
                    // === Pose Library ===
                    const char* poseCategories[] = {"Reference", "Standing", "Action", "Sitting", "Gesture"};
                    ImGui::Combo("Category", &state.selectedPoseCategory, poseCategories, 5);
                    
                    ImGui::Separator();
                    
                    struct PosePreset {
                        const char* id;
                        const char* name;
                        const char* nameCN;
                        int category;
                    };
                    PosePreset poses[] = {
                        // Reference (0)
                        {"t_pose", "T-Pose", "T字姿势", 0},
                        {"a_pose", "A-Pose", "A字姿势", 0},
                        {"relaxed", "Relaxed", "放松", 0},
                        // Standing (1)
                        {"standing_neutral", "Neutral", "中立站姿", 1},
                        {"standing_heroic", "Heroic", "英雄站姿", 1},
                        {"standing_casual", "Casual", "随意站姿", 1},
                        {"contrapposto", "Contrapposto", "对立式", 1},
                        // Action (2)
                        {"fighting_stance", "Fighting", "战斗姿势", 2},
                        {"running", "Running", "奔跑", 2},
                        {"jumping", "Jumping", "跳跃", 2},
                        {"punching", "Punching", "出拳", 2},
                        {"kicking", "Kicking", "踢腿", 2},
                        // Sitting (3)
                        {"sitting", "Sitting", "坐姿", 3},
                        {"sitting_cross_legged", "Cross-Legged", "盘腿坐", 3},
                        {"kneeling", "Kneeling", "跪姿", 3},
                        // Gesture (4)
                        {"waving", "Waving", "挥手", 4},
                        {"pointing", "Pointing", "指向", 4},
                        {"thinking", "Thinking", "思考", 4},
                        {"arms_raised", "Arms Raised", "举手", 4},
                        {"arms_crossed", "Arms Crossed", "双臂交叉", 4}
                    };
                    
                    ImGui::BeginChild("PoseList", ImVec2(0, 200), true);
                    float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) / 2;
                    int col = 0;
                    
                    for (const auto& pose : poses) {
                        if (pose.category != state.selectedPoseCategory) continue;
                        
                        bool isSelected = (state.selectedPoseName == pose.id);
                        ImGui::PushID(pose.id);
                        
                        if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                        
                        if (ImGui::Button(pose.name, ImVec2(buttonWidth, 35))) {
                            state.selectedPoseName = pose.id;
                            if (state.onPoseLoad) state.onPoseLoad(pose.id);
                        }
                        
                        if (isSelected) ImGui::PopStyleColor();
                        
                        // Show Chinese name as tooltip
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", pose.nameCN);
                        }
                        
                        ImGui::PopID();
                        
                        col++;
                        if (col < 2) ImGui::SameLine();
                        else col = 0;
                    }
                    ImGui::EndChild();
                    
                    // Pose actions
                    ImGui::Separator();
                    if (ImGui::Button("Reset Pose", ImVec2(-1, 0))) {
                        if (state.onPoseReset) state.onPoseReset();
                    }
                    if (ImGui::Button("Mirror Pose", ImVec2(-1, 0))) {
                        if (state.onPoseMirror) state.onPoseMirror();
                    }
                    ImGui::Checkbox("Auto Mirror", &state.poseAutoMirror);
                    
                } else if (animSubTab == 1) {
                    // === Bone Editor ===
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Manual Bone Adjustment");
                    ImGui::TextDisabled("Select bone and adjust rotation");
                    
                    const char* boneCategories[] = {"All", "Spine", "Left Arm", "Right Arm", "Left Leg", "Right Leg", "Head"};
                    ImGui::Combo("Body Part", &state.poseEditorBoneCategory, boneCategories, 7);
                    
                    ImGui::Separator();
                    
                    // Bone list based on category
                    struct BoneInfo { const char* name; const char* display; int category; };
                    BoneInfo bones[] = {
                        // Spine (1)
                        {"Hips", "Hips 髋部", 1},
                        {"Spine", "Spine 脊椎", 1},
                        {"Chest", "Chest 胸部", 1},
                        // Left Arm (2)
                        {"LeftShoulder", "L Shoulder 左肩", 2},
                        {"LeftUpperArm", "L Upper Arm 左上臂", 2},
                        {"LeftLowerArm", "L Lower Arm 左前臂", 2},
                        {"LeftHand", "L Hand 左手", 2},
                        // Right Arm (3)
                        {"RightShoulder", "R Shoulder 右肩", 3},
                        {"RightUpperArm", "R Upper Arm 右上臂", 3},
                        {"RightLowerArm", "R Lower Arm 右前臂", 3},
                        {"RightHand", "R Hand 右手", 3},
                        // Left Leg (4)
                        {"LeftUpperLeg", "L Upper Leg 左大腿", 4},
                        {"LeftLowerLeg", "L Lower Leg 左小腿", 4},
                        {"LeftFoot", "L Foot 左脚", 4},
                        // Right Leg (5)
                        {"RightUpperLeg", "R Upper Leg 右大腿", 5},
                        {"RightLowerLeg", "R Lower Leg 右小腿", 5},
                        {"RightFoot", "R Foot 右脚", 5},
                        // Head (6)
                        {"Neck", "Neck 脖子", 6},
                        {"Head", "Head 头部", 6}
                    };
                    
                    ImGui::BeginChild("BoneList", ImVec2(0, 120), true);
                    for (const auto& bone : bones) {
                        if (state.poseEditorBoneCategory != 0 && bone.category != state.poseEditorBoneCategory) continue;
                        
                        bool isSelected = (state.selectedBoneName == bone.name);
                        if (ImGui::Selectable(bone.display, isSelected)) {
                            state.selectedBoneName = bone.name;
                            state.boneRotationX = 0;
                            state.boneRotationY = 0;
                            state.boneRotationZ = 0;
                            if (state.onBoneSelect) state.onBoneSelect(bone.name);
                        }
                    }
                    ImGui::EndChild();
                    
                    ImGui::Separator();
                    
                    // Rotation controls
                    if (!state.selectedBoneName.empty()) {
                        ImGui::Text("Bone: %s", state.selectedBoneName.c_str());
                        
                        bool changed = false;
                        changed |= ImGui::SliderFloat("Rot X (Pitch)", &state.boneRotationX, -180.0f, 180.0f, "%.1f°");
                        changed |= ImGui::SliderFloat("Rot Y (Yaw)", &state.boneRotationY, -180.0f, 180.0f, "%.1f°");
                        changed |= ImGui::SliderFloat("Rot Z (Roll)", &state.boneRotationZ, -180.0f, 180.0f, "%.1f°");
                        
                        if (changed && state.onBoneRotate) {
                            float rx = state.boneRotationX * 3.14159f / 180.0f;
                            float ry = state.boneRotationY * 3.14159f / 180.0f;
                            float rz = state.boneRotationZ * 3.14159f / 180.0f;
                            state.onBoneRotate(state.selectedBoneName, rx, ry, rz);
                        }
                        
                        if (ImGui::Button("Reset Bone", ImVec2(-1, 0))) {
                            state.boneRotationX = state.boneRotationY = state.boneRotationZ = 0;
                            if (state.onBoneRotate) state.onBoneRotate(state.selectedBoneName, 0, 0, 0);
                        }
                    } else {
                        ImGui::TextDisabled("Select a bone to edit");
                    }
                    
                } else if (animSubTab == 2) {
                    // === Animation Playback ===
                    struct AnimPreset { const char* id; const char* name; const char* nameCN; };
                    AnimPreset animations[] = {
                        {"idle", "Idle", "待机"},
                        {"idle_breathing", "Idle Breathing", "呼吸待机"},
                        {"walk", "Walk", "行走"},
                        {"run", "Run", "跑步"},
                        {"wave", "Wave", "挥手"}
                    };
                    
                    ImGui::Text("Built-in Animations:");
                    ImGui::Separator();
                    
                    for (const auto& anim : animations) {
                        bool isPlaying = (state.currentAnimation == anim.id && state.animationPlaying);
                        
                        ImGui::PushID(anim.id);
                        if (isPlaying) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                            if (ImGui::Button("Stop", ImVec2(60, 0))) {
                                state.animationPlaying = false;
                                state.currentAnimation = "";
                                if (state.onStopAnimation) state.onStopAnimation();
                            }
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
                            if (ImGui::Button("Play", ImVec2(60, 0))) {
                                state.currentAnimation = anim.id;
                                state.animationPlaying = true;
                                state.animationTime = 0.0f;
                                if (state.onPlayAnimation) state.onPlayAnimation(anim.id);
                            }
                            ImGui::PopStyleColor();
                        }
                        ImGui::SameLine();
                        ImGui::Text("%s (%s)", anim.name, anim.nameCN);
                        ImGui::PopID();
                    }
                    
                    if (state.animationPlaying) {
                        ImGui::Separator();
                        ImGui::SliderFloat("Speed", &state.animationSpeed, 0.1f, 2.0f);
                        
                        // Progress bar
                        float progress = fmod(state.animationTime * state.animationSpeed, 1.0f);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2fs", state.animationTime);
                        ImGui::ProgressBar(progress, ImVec2(-1, 0), buf);
                    }
                    
                } else if (animSubTab == 3) {
                    // === Animation Timeline (Editor) ===
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Animation Editor");
                    ImGui::TextDisabled("Keyframe-based animation editing");
                    
                    ImGui::Separator();
                    
                    // Toolbar
                    if (ImGui::Button(state.animationPlaying ? "||" : ">", ImVec2(30, 0))) {
                        state.animationPlaying = !state.animationPlaying;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("|<", ImVec2(30, 0))) {
                        state.animationTime = 0;
                        if (state.onAnimEditorSeek) state.onAnimEditorSeek(0);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("<", ImVec2(30, 0))) {
                        state.animationTime = std::max(0.0f, state.animationTime - 1.0f/30.0f);
                        if (state.onAnimEditorSeek) state.onAnimEditorSeek(state.animationTime);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(">", ImVec2(30, 0))) {
                        state.animationTime += 1.0f/30.0f;
                        if (state.onAnimEditorSeek) state.onAnimEditorSeek(state.animationTime);
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    if (ImGui::DragFloat("##time", &state.animationTime, 0.01f, 0, 10.0f, "%.2fs")) {
                        if (state.onAnimEditorSeek) state.onAnimEditorSeek(state.animationTime);
                    }
                    
                    ImGui::Separator();
                    
                    // Keyframe tools
                    ImGui::Text("Keyframe Tools:");
                    if (ImGui::Button("+ Add Key", ImVec2(80, 0))) {
                        if (state.onAnimEditorAddKeyframe) state.onAnimEditorAddKeyframe();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("- Delete", ImVec2(80, 0))) {
                        if (state.onAnimEditorDeleteKeyframe) state.onAnimEditorDeleteKeyframe();
                    }
                    
                    const char* interpTypes[] = {"Constant", "Linear", "Bezier", "EaseIn", "EaseOut", "EaseInOut"};
                    ImGui::SetNextItemWidth(120);
                    if (ImGui::Combo("Interpolation", &state.animEditorInterpolation, interpTypes, 6)) {
                        if (state.onAnimEditorSetInterpolation) state.onAnimEditorSetInterpolation(state.animEditorInterpolation);
                    }
                    
                    ImGui::Separator();
                    
                    // Settings
                    ImGui::Checkbox("Auto Key", &state.animEditorAutoKey);
                    ImGui::SameLine();
                    ImGui::Checkbox("Snap to Frame", &state.animEditorSnapToFrame);
                    
                    ImGui::Checkbox("Show Ghosts", &state.animEditorShowGhosts);
                    if (state.animEditorShowGhosts) {
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(80);
                        ImGui::SliderInt("Frames", &state.animEditorGhostFrames, 1, 10);
                    }
                    
                    ImGui::Separator();
                    
                    // Simple timeline visualization
                    ImGui::Text("Timeline:");
                    ImVec2 timelineSize(ImGui::GetContentRegionAvail().x, 60);
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(pos, ImVec2(pos.x + timelineSize.x, pos.y + timelineSize.y), 
                                      IM_COL32(40, 40, 45, 255));
                    
                    // Draw time markers
                    float duration = 5.0f;  // 5 second timeline
                    for (int i = 0; i <= 5; i++) {
                        float x = pos.x + (i / 5.0f) * timelineSize.x;
                        dl->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + timelineSize.y), IM_COL32(80, 80, 80, 255));
                        char label[8];
                        snprintf(label, sizeof(label), "%ds", i);
                        dl->AddText(ImVec2(x + 2, pos.y + 2), IM_COL32(150, 150, 150, 255), label);
                    }
                    
                    // Draw playhead
                    float playheadX = pos.x + (state.animationTime / duration) * timelineSize.x;
                    playheadX = std::clamp(playheadX, pos.x, pos.x + timelineSize.x);
                    dl->AddLine(ImVec2(playheadX, pos.y), ImVec2(playheadX, pos.y + timelineSize.y), 
                               IM_COL32(255, 100, 100, 255), 2.0f);
                    
                    // Make timeline clickable
                    ImGui::InvisibleButton("timeline", timelineSize);
                    if (ImGui::IsItemClicked()) {
                        ImVec2 mousePos = ImGui::GetMousePos();
                        float t = (mousePos.x - pos.x) / timelineSize.x * duration;
                        state.animationTime = std::clamp(t, 0.0f, duration);
                        if (state.onAnimEditorSeek) state.onAnimEditorSeek(state.animationTime);
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // === Style Tab ===
            if (ImGui::BeginTabItem("Style")) {
                state.currentTab = 5;
                
                // Style preset
                const char* styles[] = {"Realistic", "Anime", "Cartoon", "Painterly", "Sketch"};
                if (ImGui::Combo("Rendering Style", &state.renderingStyle, styles, 5)) {
                    // Apply preset defaults
                    switch (state.renderingStyle) {
                        case 1:  // Anime
                            state.outlineEnabled = true;
                            state.outlineThickness = 0.004f;
                            state.celShadingBands = 3;
                            state.rimLightEnabled = true;
                            state.rimLightIntensity = 0.5f;
                            state.colorVibrancy = 1.15f;
                            break;
                        case 2:  // Cartoon
                            state.outlineEnabled = true;
                            state.outlineThickness = 0.006f;
                            state.celShadingBands = 2;
                            state.rimLightEnabled = false;
                            state.colorVibrancy = 1.3f;
                            break;
                        case 3:  // Painterly
                            state.outlineEnabled = false;
                            state.celShadingBands = 5;
                            state.rimLightEnabled = true;
                            state.colorVibrancy = 1.2f;
                            break;
                        case 4:  // Sketch
                            state.outlineEnabled = true;
                            state.outlineThickness = 0.002f;
                            state.celShadingBands = 2;
                            state.rimLightEnabled = false;
                            state.colorVibrancy = 0.3f;
                            break;
                        default:  // Realistic
                            state.outlineEnabled = false;
                            state.celShadingBands = 1;
                            state.rimLightEnabled = false;
                            state.colorVibrancy = 1.0f;
                            break;
                    }
                    if (state.onStyleChange) {
                        state.onStyleChange(state.renderingStyle);
                    }
                }
                
                ImGui::Separator();
                
                // Outline settings
                if (ImGui::CollapsingHeader("Outline", ImGuiTreeNodeFlags_DefaultOpen)) {
                    bool changed = false;
                    changed |= ImGui::Checkbox("Enable Outline", &state.outlineEnabled);
                    
                    if (state.outlineEnabled) {
                        changed |= ImGui::SliderFloat("Thickness", &state.outlineThickness, 0.001f, 0.01f, "%.4f");
                        changed |= ImGui::ColorEdit3("Color", state.outlineColor);
                    }
                    
                    if (changed && state.onStyleSettingsChange) {
                        state.onStyleSettingsChange();
                    }
                }
                
                // Cel shading settings
                if (ImGui::CollapsingHeader("Cel Shading", ImGuiTreeNodeFlags_DefaultOpen)) {
                    bool changed = false;
                    changed |= ImGui::SliderInt("Shading Bands", &state.celShadingBands, 1, 5);
                    
                    ImGui::TextDisabled("1 = Smooth, 2-3 = Standard, 4-5 = Detailed");
                    
                    if (changed && state.onStyleSettingsChange) {
                        state.onStyleSettingsChange();
                    }
                }
                
                // Rim light settings
                if (ImGui::CollapsingHeader("Rim Light")) {
                    bool changed = false;
                    changed |= ImGui::Checkbox("Enable Rim Light", &state.rimLightEnabled);
                    
                    if (state.rimLightEnabled) {
                        changed |= ImGui::SliderFloat("Intensity", &state.rimLightIntensity, 0.0f, 1.0f);
                    }
                    
                    if (changed && state.onStyleSettingsChange) {
                        state.onStyleSettingsChange();
                    }
                }
                
                // Color settings
                if (ImGui::CollapsingHeader("Color")) {
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Vibrancy", &state.colorVibrancy, 0.0f, 2.0f);
                    
                    ImGui::TextDisabled("0 = Grayscale, 1 = Normal, 2 = Saturated");
                    
                    if (changed && state.onStyleSettingsChange) {
                        state.onStyleSettingsChange();
                    }
                }
                
                ImGui::Separator();
                
                // Preview comparison
                ImGui::Text("Style: %s", styles[state.renderingStyle]);
                if (state.renderingStyle > 0) {
                    ImGui::TextWrapped("Non-realistic styles require stylized shaders to render correctly. "
                                      "Preview shows approximation.");
                }
                
                ImGui::EndTabItem();
            }
            
            // === Texture Tab ===
            if (ImGui::BeginTabItem("Texture")) {
                state.currentTab = 6;
                
                // Resolution selection
                const char* resolutions[] = {"512x512", "1024x1024", "2048x2048"};
                if (ImGui::Combo("Resolution", &state.textureResolution, resolutions, 3)) {
                    state.textureNeedsUpdate = true;
                }
                
                ImGui::Separator();
                
                // === Skin Texture ===
                if (ImGui::CollapsingHeader("Skin Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Skin preset
                    const char* skinPresets[] = {"Caucasian", "Asian", "African", "Latino", "Middle Eastern", "Custom"};
                    if (ImGui::Combo("Skin Tone Preset", &state.skinPreset, skinPresets, 6)) {
                        // Apply preset skin colors
                        switch (state.skinPreset) {
                            case 0:  // Caucasian
                                state.skinColor[0] = 0.9f; state.skinColor[1] = 0.75f; state.skinColor[2] = 0.65f;
                                break;
                            case 1:  // Asian
                                state.skinColor[0] = 0.95f; state.skinColor[1] = 0.82f; state.skinColor[2] = 0.7f;
                                break;
                            case 2:  // African
                                state.skinColor[0] = 0.45f; state.skinColor[1] = 0.3f; state.skinColor[2] = 0.2f;
                                break;
                            case 3:  // Latino
                                state.skinColor[0] = 0.75f; state.skinColor[1] = 0.55f; state.skinColor[2] = 0.4f;
                                break;
                            case 4:  // Middle Eastern
                                state.skinColor[0] = 0.8f; state.skinColor[1] = 0.6f; state.skinColor[2] = 0.45f;
                                break;
                        }
                        state.textureNeedsUpdate = true;
                        if (state.onSkinPresetChange) {
                            state.onSkinPresetChange(state.skinPreset);
                        }
                    }
                    
                    // Custom skin color (only shown for Custom preset)
                    if (state.skinPreset == 5) {
                        if (ImGui::ColorEdit3("Skin Color", state.skinColor)) {
                            state.textureNeedsUpdate = true;
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    // Skin parameters
                    bool skinChanged = false;
                    skinChanged |= ImGui::SliderFloat("Saturation", &state.skinSaturation, 0.5f, 1.5f);
                    skinChanged |= ImGui::SliderFloat("Brightness", &state.skinBrightness, 0.7f, 1.3f);
                    
                    ImGui::Spacing();
                    ImGui::Text("Surface Detail");
                    skinChanged |= ImGui::SliderFloat("Roughness", &state.skinRoughness, 0.2f, 0.8f);
                    skinChanged |= ImGui::SliderFloat("Pore Intensity", &state.poreIntensity, 0.0f, 1.0f);
                    
                    ImGui::Spacing();
                    ImGui::Text("Age & Variation");
                    skinChanged |= ImGui::SliderFloat("Wrinkle Intensity", &state.wrinkleIntensity, 0.0f, 1.0f);
                    skinChanged |= ImGui::SliderFloat("Freckle Intensity", &state.freckleIntensity, 0.0f, 1.0f);
                    if (state.freckleIntensity > 0.0f) {
                        skinChanged |= ImGui::ColorEdit3("Freckle Color", state.freckleColor);
                    }
                    
                    ImGui::Spacing();
                    ImGui::Text("Subsurface Scattering");
                    skinChanged |= ImGui::SliderFloat("SSS Intensity", &state.sssIntensity, 0.0f, 0.6f);
                    ImGui::TextDisabled("Simulates light passing through skin");
                    
                    // === Advanced SSS (NEW) ===
                    if (ImGui::TreeNode("Advanced SSS Settings")) {
                        skinChanged |= ImGui::SliderFloat("SSS Strength", &state.skinSubsurfaceStrength, 0.0f, 1.0f);
                        ImGui::TextDisabled("Overall subsurface scattering strength");
                        
                        skinChanged |= ImGui::SliderFloat("SSS Radius", &state.skinSubsurfaceRadius, 0.001f, 0.05f, "%.3f");
                        ImGui::TextDisabled("How far light scatters under skin");
                        
                        skinChanged |= ImGui::SliderFloat("Translucency", &state.skinTranslucency, 0.0f, 1.0f);
                        ImGui::TextDisabled("Backlit effect (ears, fingers)");
                        
                        skinChanged |= ImGui::SliderFloat("Oil/Moisture", &state.skinOilAmount, 0.0f, 1.0f);
                        ImGui::TextDisabled("Surface shine layer");
                        
                        skinChanged |= ImGui::SliderFloat("Pore Depth", &state.skinPoreDepth, 0.0f, 0.3f);
                        ImGui::TextDisabled("Micro surface detail");
                        
                        ImGui::Spacing();
                        ImGui::Text("Blush Effect");
                        skinChanged |= ImGui::SliderFloat("Blush Amount", &state.skinBlush, 0.0f, 1.0f);
                        if (state.skinBlush > 0.0f) {
                            skinChanged |= ImGui::ColorEdit3("Blush Color", state.skinBlushColor);
                        }
                        
                        if (skinChanged && state.onSkinRenderingUpdate) {
                            state.onSkinRenderingUpdate();
                        }
                        
                        ImGui::TreePop();
                    }
                    
                    if (skinChanged) {
                        state.textureNeedsUpdate = true;
                    }
                }
                
                // === Eye Texture ===
                if (ImGui::CollapsingHeader("Eye Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Eye color preset
                    const char* eyeColors[] = {"Brown", "Blue", "Green", "Hazel", "Gray", "Custom"};
                    if (ImGui::Combo("Eye Color", &state.eyeColorPreset, eyeColors, 6)) {
                        // Apply preset eye colors
                        switch (state.eyeColorPreset) {
                            case 0:  // Brown
                                state.eyeColor[0] = 0.4f; state.eyeColor[1] = 0.25f; state.eyeColor[2] = 0.15f;
                                break;
                            case 1:  // Blue
                                state.eyeColor[0] = 0.3f; state.eyeColor[1] = 0.5f; state.eyeColor[2] = 0.8f;
                                break;
                            case 2:  // Green
                                state.eyeColor[0] = 0.35f; state.eyeColor[1] = 0.55f; state.eyeColor[2] = 0.35f;
                                break;
                            case 3:  // Hazel
                                state.eyeColor[0] = 0.5f; state.eyeColor[1] = 0.4f; state.eyeColor[2] = 0.25f;
                                break;
                            case 4:  // Gray
                                state.eyeColor[0] = 0.5f; state.eyeColor[1] = 0.55f; state.eyeColor[2] = 0.6f;
                                break;
                        }
                        state.textureNeedsUpdate = true;
                        if (state.onEyeColorPresetChange) {
                            state.onEyeColorPresetChange(state.eyeColorPreset);
                        }
                    }
                    
                    // Custom eye color
                    if (state.eyeColorPreset == 5) {
                        if (ImGui::ColorEdit3("Iris Color", state.eyeColor)) {
                            state.textureNeedsUpdate = true;
                        }
                    }
                    
                    ImGui::Spacing();
                    
                    bool eyeChanged = false;
                    eyeChanged |= ImGui::SliderFloat("Iris Size", &state.irisSize, 0.3f, 0.7f);
                    eyeChanged |= ImGui::SliderFloat("Pupil Size", &state.pupilSize, 0.1f, 0.5f);
                    eyeChanged |= ImGui::SliderFloat("Iris Detail", &state.irisDetail, 0.3f, 1.0f);
                    
                    ImGui::Spacing();
                    eyeChanged |= ImGui::SliderFloat("Sclera Veins", &state.scleraVeins, 0.0f, 0.4f);
                    eyeChanged |= ImGui::SliderFloat("Eye Wetness", &state.eyeWetness, 0.3f, 1.0f);
                    
                    // === Advanced Eye Rendering (NEW) ===
                    if (ImGui::TreeNode("Advanced Eye Settings")) {
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Refraction & Depth");
                        
                        eyeChanged |= ImGui::SliderFloat("Iris Depth", &state.eyeIrisDepth, 0.0f, 0.05f, "%.3f");
                        ImGui::TextDisabled("Parallax depth effect for iris");
                        
                        eyeChanged |= ImGui::SliderFloat("Cornea Bulge", &state.eyeCorneaBulge, 0.0f, 0.06f, "%.3f");
                        ImGui::TextDisabled("Dome over iris for realistic refraction");
                        
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Lighting Effects");
                        
                        eyeChanged |= ImGui::SliderFloat("Caustics", &state.eyeCausticStrength, 0.0f, 1.0f);
                        ImGui::TextDisabled("Light patterns from cornea refraction");
                        
                        eyeChanged |= ImGui::SliderFloat("Reflection", &state.eyeReflection, 0.0f, 1.0f);
                        ImGui::TextDisabled("Environment reflection on cornea");
                        
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Pupil");
                        
                        eyeChanged |= ImGui::SliderFloat("Pupil Dilation", &state.eyePupilDilation, -1.0f, 1.0f);
                        ImGui::TextDisabled("-1 = constricted, +1 = dilated");
                        
                        if (eyeChanged && state.onEyeRenderingUpdate) {
                            state.onEyeRenderingUpdate();
                        }
                        
                        ImGui::TreePop();
                    }
                    
                    if (eyeChanged) {
                        state.textureNeedsUpdate = true;
                    }
                }
                
                // === Lip Texture ===
                if (ImGui::CollapsingHeader("Lip Texture")) {
                    bool lipChanged = false;
                    lipChanged |= ImGui::ColorEdit3("Lip Color", state.lipColor);
                    lipChanged |= ImGui::SliderFloat("Glossiness", &state.lipGlossiness, 0.0f, 1.0f);
                    lipChanged |= ImGui::SliderFloat("Chapped", &state.lipChapped, 0.0f, 1.0f);
                    
                    if (lipChanged) {
                        state.textureNeedsUpdate = true;
                    }
                }
                
                ImGui::Separator();
                
                // Generate/Update button
                if (state.textureNeedsUpdate) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                    if (ImGui::Button("Generate Textures", ImVec2(-1, 35))) {
                        if (state.onTextureUpdate) {
                            state.onTextureUpdate();
                        }
                        state.textureNeedsUpdate = false;
                    }
                    ImGui::PopStyleColor();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Parameters changed - click to update textures");
                } else {
                    if (ImGui::Button("Regenerate Textures", ImVec2(-1, 35))) {
                        if (state.onTextureUpdate) {
                            state.onTextureUpdate();
                        }
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // === Hair Tab ===
            if (ImGui::BeginTabItem("Hair")) {
                state.currentTab = 7;
                
                ImGui::Text("Hair Style");
                ImGui::Separator();
                
                // Hair style selection
                if (state.availableHairStyles.empty()) {
                    ImGui::TextDisabled("Loading hair styles...");
                } else {
                    // Style dropdown
                    const char* currentStyle = state.hairStyleIndex < (int)state.availableHairStyles.size() ?
                        state.availableHairStyles[state.hairStyleIndex].c_str() : "None";
                    
                    if (ImGui::BeginCombo("Style", currentStyle)) {
                        for (int i = 0; i < (int)state.availableHairStyles.size(); i++) {
                            bool isSelected = (state.hairStyleIndex == i);
                            if (ImGui::Selectable(state.availableHairStyles[i].c_str(), isSelected)) {
                                state.hairStyleIndex = i;
                                state.hairNeedsUpdate = true;
                                if (state.onHairStyleChange) {
                                    state.onHairStyleChange(state.availableHairStyles[i]);
                                }
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Hair Color");
                
                // Color preset
                const char* hairColors[] = {
                    "Black", "Dark Brown", "Brown", "Auburn", "Red",
                    "Blonde", "Platinum", "Gray", "White",
                    "Blue", "Pink", "Purple", "Green"
                };
                
                if (ImGui::Combo("Color Preset", &state.hairColorPreset, hairColors, 13)) {
                    state.useCustomHairColor = false;
                    state.hairNeedsUpdate = true;
                    
                    // Set color based on preset
                    switch (state.hairColorPreset) {
                        case 0:  // Black
                            state.hairColor[0] = 0.02f; state.hairColor[1] = 0.02f; state.hairColor[2] = 0.02f;
                            break;
                        case 1:  // Dark Brown
                            state.hairColor[0] = 0.08f; state.hairColor[1] = 0.05f; state.hairColor[2] = 0.03f;
                            break;
                        case 2:  // Brown
                            state.hairColor[0] = 0.15f; state.hairColor[1] = 0.1f; state.hairColor[2] = 0.05f;
                            break;
                        case 3:  // Auburn
                            state.hairColor[0] = 0.35f; state.hairColor[1] = 0.15f; state.hairColor[2] = 0.08f;
                            break;
                        case 4:  // Red
                            state.hairColor[0] = 0.5f; state.hairColor[1] = 0.15f; state.hairColor[2] = 0.08f;
                            break;
                        case 5:  // Blonde
                            state.hairColor[0] = 0.75f; state.hairColor[1] = 0.6f; state.hairColor[2] = 0.4f;
                            break;
                        case 6:  // Platinum
                            state.hairColor[0] = 0.9f; state.hairColor[1] = 0.88f; state.hairColor[2] = 0.8f;
                            break;
                        case 7:  // Gray
                            state.hairColor[0] = 0.5f; state.hairColor[1] = 0.5f; state.hairColor[2] = 0.52f;
                            break;
                        case 8:  // White
                            state.hairColor[0] = 0.85f; state.hairColor[1] = 0.85f; state.hairColor[2] = 0.87f;
                            break;
                        case 9:  // Blue
                            state.hairColor[0] = 0.1f; state.hairColor[1] = 0.2f; state.hairColor[2] = 0.5f;
                            break;
                        case 10: // Pink
                            state.hairColor[0] = 0.7f; state.hairColor[1] = 0.3f; state.hairColor[2] = 0.5f;
                            break;
                        case 11: // Purple
                            state.hairColor[0] = 0.3f; state.hairColor[1] = 0.1f; state.hairColor[2] = 0.4f;
                            break;
                        case 12: // Green
                            state.hairColor[0] = 0.1f; state.hairColor[1] = 0.35f; state.hairColor[2] = 0.15f;
                            break;
                    }
                    
                    if (state.onHairColorPresetChange) {
                        state.onHairColorPresetChange(state.hairColorPreset);
                    }
                }
                
                // Custom color option
                ImGui::Checkbox("Custom Color", &state.useCustomHairColor);
                
                if (state.useCustomHairColor) {
                    if (ImGui::ColorEdit3("Hair Color", state.hairColor)) {
                        state.hairNeedsUpdate = true;
                        if (state.onHairColorChange) {
                            state.onHairColorChange(state.hairColor[0], state.hairColor[1], state.hairColor[2]);
                        }
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // === Advanced Hair Rendering (NEW) ===
                if (ImGui::CollapsingHeader("Advanced Hair Rendering")) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Specular (Marschner Model)");
                    
                    bool hairChanged = false;
                    hairChanged |= ImGui::SliderFloat("Specular Strength", &state.hairSpecularStrength, 0.0f, 2.0f);
                    ImGui::TextDisabled("Primary highlight intensity");
                    
                    hairChanged |= ImGui::SliderFloat("Specular Shift", &state.hairSpecularShift, -0.3f, 0.3f);
                    ImGui::TextDisabled("Highlight position along hair");
                    
                    hairChanged |= ImGui::SliderFloat("Transmission", &state.hairTransmission, 0.0f, 1.0f);
                    ImGui::TextDisabled("Light passing through hair strands");
                    
                    hairChanged |= ImGui::SliderFloat("Scatter", &state.hairScatter, 0.0f, 1.0f);
                    ImGui::TextDisabled("Back-scatter from light");
                    
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Strand Shape");
                    
                    hairChanged |= ImGui::SliderFloat("Curl Frequency", &state.hairCurlFrequency, 0.0f, 10.0f);
                    ImGui::TextDisabled("Number of curls per strand");
                    
                    hairChanged |= ImGui::SliderFloat("Curl Amplitude", &state.hairCurlAmplitude, 0.0f, 0.05f, "%.3f");
                    ImGui::TextDisabled("How tight the curls are");
                    
                    hairChanged |= ImGui::SliderFloat("Frizz", &state.hairFrizz, 0.0f, 0.02f, "%.3f");
                    ImGui::TextDisabled("Random strand variation");
                    
                    hairChanged |= ImGui::SliderFloat("Clumping", &state.hairClumping, 0.0f, 1.0f);
                    ImGui::TextDisabled("How much strands group together");
                    
                    if (hairChanged) {
                        state.hairNeedsUpdate = true;
                        if (state.onHairRenderingUpdate) {
                            state.onHairRenderingUpdate();
                        }
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Hair info
                ImGui::TextDisabled("Tip: Hair styles can be imported from external files.");
                ImGui::TextDisabled("Supported: OBJ, FBX, glTF with proper UV mapping.");
                
                ImGui::EndTabItem();
            }
            
            // === Export Tab ===
            if (ImGui::BeginTabItem("Export")) {
                state.currentTab = 8;
                
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Export Character");
                ImGui::Separator();
                
                // Character name
                ImGui::Text("Name:");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##exportName", state.characterName, 256);
                
                ImGui::Spacing();
                
                // Format selection with descriptions
                ImGui::Text("Format:");
                const char* formats[] = {
                    "GLB (Recommended)", 
                    "glTF (JSON + files)", 
                    "FBX (Maya, 3ds Max)", 
                    "OBJ (Simple mesh)", 
                    "VRM (VTuber)"
                };
                ImGui::SetNextItemWidth(-1);
                ImGui::Combo("##exportFormat", &state.exportFormat, formats, 5);
                
                // Format description
                const char* formatDescs[] = {
                    "Single binary file. Best for Unity, Unreal, Blender, Web.",
                    "JSON format with separate files. Good for debugging.",
                    "Industry standard. Best for Maya, Cinema 4D, 3ds Max.",
                    "Simple mesh only. No skeleton or animation support.",
                    "VTuber avatar format. For VRChat, VSeeFace."
                };
                ImGui::TextDisabled("%s", formatDescs[state.exportFormat]);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Export options based on format
                ImGui::Text("Options:");
                
                bool canExportSkeleton = (state.exportFormat != 3);  // Not OBJ
                bool canExportBlendShapes = (state.exportFormat != 3);
                
                ImGui::BeginDisabled(!canExportSkeleton);
                ImGui::Checkbox("Include Skeleton", &state.exportSkeleton);
                ImGui::EndDisabled();
                
                ImGui::BeginDisabled(!canExportBlendShapes);
                ImGui::Checkbox("Include BlendShapes", &state.exportBlendShapes);
                ImGui::EndDisabled();
                
                ImGui::Checkbox("Include Textures", &state.exportTextures);
                ImGui::Checkbox("Include Materials", &state.exportMaterials);
                
                if (state.exportFormat == 0 || state.exportFormat == 1) {  // GLB or glTF
                    ImGui::Checkbox("Embed Textures in File", &state.embedTextures);
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Export button
                if (state.exportInProgress) {
                    ImGui::BeginDisabled(true);
                    ImGui::Button("Exporting...", ImVec2(-1, 45));
                    ImGui::EndDisabled();
                    
                    ImGui::ProgressBar(state.exportProgress, ImVec2(-1, 0));
                    ImGui::TextDisabled("%s", state.exportStatus.c_str());
                } else {
                    if (ImGui::Button("Export Character...", ImVec2(-1, 45))) {
                        if (state.onExport) {
                            state.onExport(
                                state.characterName, 
                                state.exportFormat,
                                state.exportSkeleton,
                                state.exportBlendShapes,
                                state.exportTextures
                            );
                        }
                    }
                }
                
                // Show last export result
                if (!state.lastExportPath.empty()) {
                    ImGui::Spacing();
                    if (state.exportSuccess) {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Success!");
                        ImGui::TextWrapped("Saved to: %s", state.lastExportPath.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Export failed");
                        ImGui::TextWrapped("%s", state.exportStatus.c_str());
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Stats
                ImGui::Text("Model Statistics:");
                ImGui::Columns(2, "stats", false);
                ImGui::BulletText("Vertices:"); ImGui::NextColumn();
                ImGui::Text("%u", state.vertexCount); ImGui::NextColumn();
                ImGui::BulletText("Triangles:"); ImGui::NextColumn();
                ImGui::Text("%u", state.triangleCount); ImGui::NextColumn();
                ImGui::BulletText("BlendShapes:"); ImGui::NextColumn();
                ImGui::Text("%u", state.blendShapeCount); ImGui::NextColumn();
                ImGui::BulletText("Bones:"); ImGui::NextColumn();
                ImGui::Text("%u", state.boneCount); ImGui::NextColumn();
                ImGui::Columns(1);
                
                // Format compatibility info
                ImGui::Spacing();
                if (ImGui::CollapsingHeader("Format Compatibility")) {
                    ImGui::TextDisabled("GLB/glTF:");
                    ImGui::BulletText("Unity (via glTFast)");
                    ImGui::BulletText("Unreal Engine (native)");
                    ImGui::BulletText("Blender (native)");
                    ImGui::BulletText("Three.js / Web");
                    
                    ImGui::Spacing();
                    ImGui::TextDisabled("VRM:");
                    ImGui::BulletText("VRChat");
                    ImGui::BulletText("VSeeFace");
                    ImGui::BulletText("VMagicMirror");
                    ImGui::BulletText("VTube Studio");
                }
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        
        // Bottom controls
        ImGui::Checkbox("Auto Rotate", &state.autoRotate);
        ImGui::SameLine();
        
        if (!state.autoRotate) {
            ImGui::SetNextItemWidth(150);
            float rotDeg = state.rotationY * 57.2958f;
            if (ImGui::SliderFloat("##rotation", &rotDeg, -180.0f, 180.0f, "%.0f deg")) {
                state.rotationY = rotDeg / 57.2958f;
            }
        }
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        if (ImGui::Button("Randomize", ImVec2(90, 0))) {
            if (state.onRandomize) state.onRandomize();
        }
    }
    ImGui::End();
    
    // === AI Model Setup Window ===
    if (state.showAIModelSetup) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("AI Model Setup", &state.showAIModelSetup)) {
            ImGui::TextWrapped(
                "The Photo-to-Face feature requires AI models in ONNX format. "
                "Please import the following models to enable this feature."
            );
            ImGui::Separator();
            
            // Model list
            struct ModelEntry {
                const char* id;
                const char* name;
                const char* description;
                bool required;
            };
            ModelEntry models[] = {
                {"face_detector", "Face Detector", "Detects faces in photos (MediaPipe compatible)", true},
                {"face_mesh", "Face Mesh", "Extracts 468 3D landmarks (MediaPipe Face Mesh)", true},
                {"3dmm", "3DMM Regressor", "FLAME/DECA model for shape parameters", false},
                {"face_recognition", "Face Recognition", "Identity preservation (ArcFace)", false}
            };
            
            ImGui::Text("Required Models:");
            for (const auto& m : models) {
                if (!m.required) continue;
                
                ImGui::PushID(m.id);
                
                // Status indicator (placeholder)
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[Not Found]");
                ImGui::SameLine();
                ImGui::Text("%s", m.name);
                ImGui::SameLine(300);
                if (ImGui::Button("Import...")) {
                    if (state.onImportAIModel) {
                        state.onImportAIModel(m.id, m.name);
                    }
                }
                ImGui::TextDisabled("  %s", m.description);
                
                ImGui::PopID();
            }
            
            ImGui::Separator();
            ImGui::Text("Optional Models:");
            for (const auto& m : models) {
                if (m.required) continue;
                
                ImGui::PushID(m.id);
                
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[Optional]");
                ImGui::SameLine();
                ImGui::Text("%s", m.name);
                ImGui::SameLine(300);
                if (ImGui::Button("Import...")) {
                    if (state.onImportAIModel) {
                        state.onImportAIModel(m.id, m.name);
                    }
                }
                ImGui::TextDisabled("  %s", m.description);
                
                ImGui::PopID();
            }
            
            ImGui::Separator();
            ImGui::TextWrapped(
                "Recommended models:\n"
                "- MediaPipe Face Detection: https://developers.google.com/mediapipe\n"
                "- MediaPipe Face Mesh: https://developers.google.com/mediapipe\n"
                "- DECA/EMOCA: https://github.com/yfeng95/DECA\n"
                "\n"
                "Models must be converted to ONNX format."
            );
        }
        ImGui::End();
    }
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
