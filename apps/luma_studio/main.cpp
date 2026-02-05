// LUMA Studio - Main Editor Application
// Cross-platform 3D scene editor with DX12/Metal backend

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <iostream>
#include <string>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

// Engine modules
#include "engine/foundation/math_types.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/post_process.h"
#include "engine/viewport/viewport.h"
#include "engine/ui/editor_ui.h"
#include "engine/asset/model_loader.h"
#include "engine/asset/asset_manager.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/picking.h"
#include "engine/editor/gizmo.h"
#include "engine/editor/command.h"
#include "engine/editor/commands/transform_commands.h"
#include "engine/editor/commands/scene_commands.h"
#include "engine/editor/editor_mode.h"
#include "engine/editor/mode_ui.h"
#include "engine/editor/uv_editor.h"
#include "engine/editor/mesh_picking.h"
#include "engine/editor/edit_mode_renderer.h"
#include "engine/mesh/mesh.h"
#include "engine/ui/localization.h"
#include "engine/serialization/scene_serializer.h"
#include "engine/serialization/json.h"

using Microsoft::WRL::ComPtr;

// New refactored modules (Phase 1-4) - must be after Windows/DX12 headers
#include "engine/renderer/shader_manager.h"
#include "engine/renderer/draw_manager.h"
#include "engine/editor/edit_module.h"
#include "engine/editor/mesh_edit/mesh_edit_module.h"
#include "engine/editor/uv_edit/uv_edit_module.h"
#include "engine/editor/material_edit/material_edit_module.h"
#include "engine/editor/selection_system.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ===== Application State =====
struct Application {
    // Core systems
    luma::UnifiedRenderer renderer;
    luma::Viewport viewport;
    luma::SceneGraph scene;
    luma::TransformGizmo gizmo;
    
    // Mode system (new workflow)
    luma::editor::EditorModeManager modeManager;
    luma::editor::WelcomeScreen welcomeScreen;
    luma::editor::ModeTabBar modeTabBar;
    luma::editor::EmptySceneGuide emptySceneGuide;
    luma::editor::AddObjectContextMenu addObjectMenu;
    luma::editor::EditModeMeshList meshListPanel;
    luma::editor::EditModeMaterialEditor materialEditor;
    luma::editor::EditModeViewToolbar viewToolbar;
    
    // Phase 4: New edit mode components
    luma::editor::EditModeToolbar editToolbar;           // 选择模式 & 编辑工具
    luma::editor::EditModeUndoRedo undoRedoBar;          // 撤销/重做
    luma::editor::EditModeSaveBar saveBar;               // 保存/取消
    luma::editor::EditModeStats meshStats;               // 网格统计
    luma::editor::UVEditor uvEditor;                     // UV 编辑器
    luma::editor::MeshPicker meshPicker;                 // 射线拾取
    luma::editor::EditModeViewportHeader viewportHeader; // 视口顶部工具栏
    luma::editor::SelectionBoxOverlay selectionBox;      // 框选/圆选覆盖层
    luma::editor::EditModeRenderer editModeRenderer;     // 编辑模式渲染器（重构后）
    
    // Edit mode state
    std::unique_ptr<luma::EditMesh> currentEditMesh;  // 当前编辑的 EditMesh
    bool editMeshDirty = false;                       // 是否有未保存的修改
    
    // New refactored modules (Phase 1-4)
    luma::editor::SelectionSystem selectionSystem;     // 统一选择系统
    std::unique_ptr<luma::editor::MeshEditModule> meshEditModule;      // Mesh 编辑模块
    std::unique_ptr<luma::editor::UVEditModule> uvEditModule;          // UV 编辑模块
    std::unique_ptr<luma::editor::MaterialEditModule> materialEditModule;  // 材质编辑模块
    luma::DrawManager drawManager;                     // 渲染调度器
    luma::EditModePipeline editPipeline;               // 编辑模式渲染管线
    
    std::string projectName = "未命名场景";
    
    // UI State
    luma::ui::EditorState editorState;
    luma::PostProcessSettings postProcess;
    luma::ui::RenderSettings renderSettings;
    luma::ui::LightSettings lighting;
    luma::ui::AnimationState animation;
    
    // Window
    HWND hwnd = nullptr;
    int width = 1280;
    int height = 720;
    
    // State
    bool shouldQuit = false;
    bool needResize = false;
    std::string pendingModelPath;
    std::string currentScenePath;
    float totalTime = 0.0f;
    
    // Mouse click tracking for selection
    float mouseDownX = 0.0f;
    float mouseDownY = 0.0f;
    bool mouseWasDown = false;
    
    // Get scene radius for camera
    float getSceneRadius() const {
        float maxRadius = 1.0f;
        for (auto& [id, entity] : scene.getAllEntities()) {
            if (entity->hasModel) {
                maxRadius = std::max(maxRadius, entity->model.radius);
            }
        }
        return maxRadius;
    }
    
    // Get scene center
    void getSceneCenter(float* center) const {
        center[0] = center[1] = center[2] = 0.0f;
        int count = 0;
        for (auto& [id, entity] : scene.getAllEntities()) {
            if (entity->hasModel) {
                auto pos = entity->getWorldPosition();
                center[0] += pos.x;
                center[1] += pos.y;
                center[2] += pos.z;
                count++;
            }
        }
        if (count > 0) {
            center[0] /= count;
            center[1] /= count;
            center[2] /= count;
        }
    }
    
    // Initialize modular architecture (Phase 1-4 refactoring)
    void initModularArchitecture() {
        luma::ShaderManager::instance().init("engine/renderer/shaders/");
        drawManager.init(&renderer);
        editPipeline.init(&renderer);
        
        meshEditModule = std::make_unique<luma::editor::MeshEditModule>();
        meshEditModule->init(&renderer);
        
        uvEditModule = std::make_unique<luma::editor::UVEditModule>();
        uvEditModule->init(&renderer);
        
        materialEditModule = std::make_unique<luma::editor::MaterialEditModule>();
        materialEditModule->init(&renderer);
        
        // Setup selection system projection callback
        selectionSystem.setProjectionCallback([this](float x, float y, float z, float& screenX, float& screenY) -> bool {
            float viewMatrix[16], projMatrix[16];
            renderer.getViewMatrix(viewMatrix);
            renderer.getProjectionMatrix(projMatrix);
            
            float viewPos[4];
            viewPos[0] = viewMatrix[0]*x + viewMatrix[4]*y + viewMatrix[8]*z + viewMatrix[12];
            viewPos[1] = viewMatrix[1]*x + viewMatrix[5]*y + viewMatrix[9]*z + viewMatrix[13];
            viewPos[2] = viewMatrix[2]*x + viewMatrix[6]*y + viewMatrix[10]*z + viewMatrix[14];
            viewPos[3] = viewMatrix[3]*x + viewMatrix[7]*y + viewMatrix[11]*z + viewMatrix[15];
            
            float clipPos[4];
            clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
            clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
            clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
            clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
            
            if (clipPos[3] <= 0.0f) return false;
            float ndcX = clipPos[0] / clipPos[3];
            float ndcY = clipPos[1] / clipPos[3];
            
            screenX = (ndcX + 1.0f) * 0.5f * width;
            screenY = (1.0f - ndcY) * 0.5f * height;
            return true;
        });
        
        std::cout << "[luma] Modular architecture initialized" << std::endl;
    }
    
    // Sync EditMesh to all modules
    void syncEditMeshToModules() {
        if (!currentEditMesh) return;
        selectionSystem.setMesh(currentEditMesh.get());
        if (meshEditModule) meshEditModule->setEditMesh(currentEditMesh.get());
        if (uvEditModule) uvEditModule->setEditMesh(currentEditMesh.get());
        if (materialEditModule) materialEditModule->setEditMesh(currentEditMesh.get());
    }
    
    // Main scene rendering (replaces inline rendering code in main loop)
    void renderScene() {
        auto camParams = viewport.getCameraParams();
        float sceneRadius = getSceneRadius();
        float sceneCenter[3];
        getSceneCenter(sceneCenter);
        renderer.setCamera(camParams, sceneRadius);
        
        // === Shadow Pass ===
        renderer.beginShadowPass(sceneRadius, sceneCenter);
        scene.traverseRenderables([this](luma::Entity* entity) {
            renderer.renderModelShadow(entity->model, entity->worldMatrix.m);
        });
        renderer.endShadowPass();
        
        // === Grid ===
        if (viewport.settings.showGrid) {
            renderer.renderGrid(camParams, sceneRadius);
        }
        
        // === Main Render Pass ===
        bool isEditMode = (modeManager.currentMode == luma::editor::EditorMode::Edit);
        auto* selectedEntity = scene.getSelectedEntity();
        auto currentViewMode = viewToolbar.currentViewMode;
        
        if (isEditMode) {
            // Convert ViewMode to RenderViewMode
            luma::RenderViewMode renderMode = luma::RenderViewMode::Material;
            switch (currentViewMode) {
                case luma::editor::ViewMode::Wireframe: renderMode = luma::RenderViewMode::Wireframe; break;
                case luma::editor::ViewMode::Solid: renderMode = luma::RenderViewMode::Solid; break;
                default: renderMode = luma::RenderViewMode::Material; break;
            }
            
            // Render all entities with view mode
            scene.traverseRenderables([this, renderMode](luma::Entity* entity) {
                editPipeline.render(luma::RenderContext{}, renderMode, entity, -1);
            });
        } else {
            // Normal scene rendering
            scene.traverseRenderables([this](luma::Entity* entity) {
                renderer.renderModel(entity->model, entity->worldMatrix.m);
            });
        }
        
        // === Edit Mode Overlays ===
        if (isEditMode && selectedEntity && selectedEntity->hasModel) {
            int highlightIdx = meshListPanel.selectedMeshIndex;
            
            // Wireframe overlay for selected mesh
            if (highlightIdx >= 0 && highlightIdx < static_cast<int>(selectedEntity->model.meshes.size())) {
                float wireColor[4] = {0.4f, 0.5f, 0.6f, 1.0f};
                const auto& gpuMesh = selectedEntity->model.meshes[highlightIdx];
                
                if (gpuMesh.hasOriginalEdges && editToolbar.showOriginalEdges && !editToolbar.showAllEdges) {
                    if (gpuMesh.hasSkinning && selectedEntity->hasSkeleton()) {
                        luma::Mat4 boneMatrices[luma::MAX_BONES];
                        selectedEntity->getSkinningMatrices(boneMatrices);
                        renderer.renderOriginalEdgesSkinned(
                            selectedEntity->model, highlightIdx,
                            selectedEntity->worldMatrix.m, wireColor,
                            reinterpret_cast<const float*>(boneMatrices),
                            gpuMesh.skinnedVertices);
                    } else {
                        renderer.renderOriginalEdges(selectedEntity->model, highlightIdx,
                                                    selectedEntity->worldMatrix.m, wireColor);
                    }
                } else {
                    renderer.renderMeshWireframeOverlay(selectedEntity->model, selectedEntity->worldMatrix.m,
                                                       highlightIdx, wireColor);
                }
            }
            
            // Selection highlights (vertices, edges, faces)
            if (currentEditMesh) {
                float selectedColor[4] = {1.0f, 0.6f, 0.0f, 1.0f};
                editPipeline.renderSelectedVertices(currentEditMesh.get(), selectedEntity->worldMatrix.m, selectedColor);
                editPipeline.renderSelectedEdges(currentEditMesh.get(), selectedEntity->worldMatrix.m, selectedColor);
                editPipeline.renderSelectedFaces(currentEditMesh.get(), selectedEntity->worldMatrix.m, selectedColor);
            }
        }
        
        // === Selection Outline & Gizmo ===
        if (auto* selected = scene.getSelectedEntity()) {
            bool skipOutline = isEditMode && (currentViewMode == luma::editor::ViewMode::Wireframe);
            
            if (selected->hasModel && !skipOutline) {
                float outlineColor[4] = {1.0f, 0.6f, 0.2f, 1.0f};
                renderer.renderModelOutline(selected->model, selected->worldMatrix.m, outlineColor);
            }
            
            // Gizmo
            luma::Vec3 gizmoPos = selected->getWorldPosition();
            float cameraEye[3], cameraTgt[3];
            viewport.camera.getEyeAndTarget(sceneCenter, sceneRadius, cameraEye, cameraTgt);
            luma::Vec3 cameraPos(cameraEye[0], cameraEye[1], cameraEye[2]);
            float screenScale = luma::TransformGizmo::calculateScreenScale(
                gizmoPos, cameraPos, 100.0f, (float)height, 3.14159f / 4.0f);
            
            gizmo.setTarget(selected);
            auto gizmoData = gizmo.generateRenderData(screenScale);
            if (!gizmoData.lines.empty()) {
                renderer.renderGizmoLines(
                    reinterpret_cast<const float*>(gizmoData.lines.data()),
                    (uint32_t)gizmoData.lines.size());
            }
        }
        
        // Finish 3D scene (applies post-processing)
        renderer.finishSceneRendering();
    }
};

static Application g_app;
static bool g_imguiInitialized = false;

// Helper: Create ray from mouse position (using OrbitCamera)
luma::Ray getMouseRay(float mouseX, float mouseY) {
    // Convert pixel to NDC
    float ndcX = (2.0f * mouseX / g_app.width) - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY / g_app.height);  // Y flipped
    
    // Get inverse view-projection matrix from renderer
    float invViewProj[16];
    if (g_app.renderer.getViewProjectionInverse(invViewProj)) {
        // Unproject near and far points
        auto unproject = [&](float z) -> luma::Vec3 {
            float pt[4] = { ndcX, ndcY, z, 1.0f };
            float out[4];
            // Matrix multiply
            for (int i = 0; i < 4; i++) {
                out[i] = invViewProj[i] * pt[0] + invViewProj[4+i] * pt[1] + 
                         invViewProj[8+i] * pt[2] + invViewProj[12+i] * pt[3];
            }
            // Perspective divide
            return luma::Vec3(out[0]/out[3], out[1]/out[3], out[2]/out[3]);
        };
        
        luma::Vec3 nearPt = unproject(0.0f);  // NDC z=0 is near plane
        luma::Vec3 farPt = unproject(1.0f);   // NDC z=1 is far plane
        luma::Vec3 rayDir = (farPt - nearPt).normalized();
        
        return luma::Ray(nearPt, rayDir);
    }
    
    // Fallback: manual calculation
    float sceneCenter[3] = {0, 0, 0};
    g_app.getSceneCenter(sceneCenter);
    float sceneRadius = g_app.getSceneRadius();
    
    float eye[3], target[3];
    g_app.viewport.camera.getEyeAndTarget(sceneCenter, sceneRadius, eye, target);
    
    luma::Vec3 eyePos(eye[0], eye[1], eye[2]);
    luma::Vec3 targetPos(target[0], target[1], target[2]);
    luma::Vec3 forward = (targetPos - eyePos).normalized();
    luma::Vec3 worldUp(0, 1, 0);
    luma::Vec3 right = forward.cross(worldUp).normalized();
    luma::Vec3 up = right.cross(forward).normalized();
    
    float aspect = (float)g_app.width / g_app.height;
    float fovRad = 45.0f * 3.14159f / 180.0f;
    float tanHalfFov = tanf(fovRad * 0.5f);
    
    float viewX = ndcX * tanHalfFov * aspect;
    float viewY = ndcY * tanHalfFov;
    
    luma::Vec3 rayDir = (right * viewX + up * viewY + forward).normalized();
    
    return luma::Ray(eyePos, rayDir);
}

// ===== Window Procedure =====
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_imguiInitialized && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    
    bool imguiWantsMouse = g_imguiInitialized && ImGui::GetIO().WantCaptureMouse;
    bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
    
    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_app.width = LOWORD(lParam);
            g_app.height = HIWORD(lParam);
            g_app.needResize = true;
        }
        return 0;
        
    case WM_KEYDOWN: {
        bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        
        // Undo/Redo shortcuts
        if (ctrlPressed && wParam == 'Z') {
            if (shiftPressed) {
                luma::getCommandHistory().redo();
            } else {
                luma::getCommandHistory().undo();
            }
        }
        // Redo with Ctrl+Y as alternative
        else if (ctrlPressed && wParam == 'Y') {
            luma::getCommandHistory().redo();
        }
        // Duplicate with Ctrl+D
        else if (ctrlPressed && wParam == 'D') {
            if (auto* selected = g_app.scene.getSelectedEntity()) {
                auto cmd = std::make_unique<luma::DuplicateEntityCommand>(&g_app.scene, selected);
                luma::getCommandHistory().execute(std::move(cmd));
            }
        }
        // Gizmo tool shortcuts
        else if (wParam == 'W') {
            g_app.editorState.gizmoMode = luma::GizmoMode::Translate;
            g_app.gizmo.setMode(luma::GizmoMode::Translate);
        } else if (wParam == 'E') {
            g_app.editorState.gizmoMode = luma::GizmoMode::Rotate;
            g_app.gizmo.setMode(luma::GizmoMode::Rotate);
        } else if (wParam == 'R') {
            g_app.editorState.gizmoMode = luma::GizmoMode::Scale;
            g_app.gizmo.setMode(luma::GizmoMode::Scale);
        } else if (wParam == VK_DELETE) {
            // Delete selected entity (with undo support)
            if (auto* selected = g_app.scene.getSelectedEntity()) {
                auto cmd = std::make_unique<luma::DeleteEntityCommand>(&g_app.scene, selected);
                g_app.scene.clearSelection();
                luma::getCommandHistory().execute(std::move(cmd));
            }
        } else if (wParam == VK_F1) {
            g_app.editorState.showHelp = !g_app.editorState.showHelp;
        } else if (wParam == VK_TAB) {
            // Toggle Edit mode (like Blender's Tab key)
            if (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit) {
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            } else if (g_app.scene.getSelectedEntity() && g_app.scene.getSelectedEntity()->hasModel) {
                g_app.modeManager.switchMode(luma::editor::EditorMode::Edit);
            }
        } else if (wParam == VK_ESCAPE) {
            // Escape returns to Scene mode or closes welcome screen
            if (g_app.welcomeScreen.isVisible) {
                g_app.welcomeScreen.isVisible = false;
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            } else if (g_app.modeManager.currentMode != luma::editor::EditorMode::Scene) {
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            }
        } else if (wParam == VK_F5) {
            // F5 enters Play mode
            g_app.modeManager.switchMode(luma::editor::EditorMode::Play);
        } else if (wParam == '1') {
            // Number keys for mode shortcuts
            g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
        } else if (wParam == '2') {
            auto* sel = g_app.scene.getSelectedEntity();
            if (sel && sel->skeleton) {
                g_app.modeManager.switchMode(luma::editor::EditorMode::Character);
            }
        } else if (wParam == '3') {
            auto* sel = g_app.scene.getSelectedEntity();
            if (sel && sel->skeleton) {
                g_app.modeManager.switchMode(luma::editor::EditorMode::Animation);
            }
        }
        g_app.viewport.onKeyDown((int)wParam);
        return 0;
    }
        
    case WM_LBUTTONDOWN:
        if (!imguiWantsMouse) {
            float mouseX = (float)GET_X_LPARAM(lParam);
            float mouseY = (float)GET_Y_LPARAM(lParam);
            
            // Record mouse down position for click detection
            g_app.mouseDownX = mouseX;
            g_app.mouseDownY = mouseY;
            g_app.mouseWasDown = true;
            
            // Try gizmo interaction first (if not holding Alt for camera control)
            if (!altPressed && g_app.scene.getSelectedEntity()) {
                luma::Ray ray = getMouseRay(mouseX, mouseY);
                
                // Calculate screen-space gizmo size (100 pixels on screen)
                luma::Vec3 gizmoPos = g_app.scene.getSelectedEntity()->getWorldPosition();
                luma::Vec3 cameraPos = ray.origin;  // Ray origin is camera position
                float screenScale = luma::TransformGizmo::calculateScreenScale(
                    gizmoPos, cameraPos, 100.0f, (float)g_app.height, 3.14159f / 4.0f);
                
                if (g_app.gizmo.beginDrag(ray, screenScale)) {
                    g_app.mouseWasDown = false;  // Gizmo took it
                    SetCapture(hwnd);
                    return 0;  // Gizmo captured the click
                }
            }
            
            // 编辑模式下，框选/圆选/套索 - 开始选择
            bool isEditMode = (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit);
            auto selectTool = g_app.editToolbar.selectTool;
            if (isEditMode && !altPressed && 
                (selectTool == luma::editor::EditModeToolbar::SelectTool::Box ||
                 selectTool == luma::editor::EditModeToolbar::SelectTool::Circle ||
                 selectTool == luma::editor::EditModeToolbar::SelectTool::Lasso)) {
                g_app.selectionBox.beginSelection(mouseX, mouseY, selectTool);
                SetCapture(hwnd);
            }
            
            // Otherwise, handle camera or selection
            g_app.viewport.onMouseDown(0, mouseX, mouseY, altPressed);
            if (altPressed) {
                g_app.mouseWasDown = false;  // Camera control took it
                SetCapture(hwnd);
            }
        }
        return 0;
        
    case WM_RBUTTONDOWN:
        if (!imguiWantsMouse) {
            float mouseX = (float)GET_X_LPARAM(lParam);
            float mouseY = (float)GET_Y_LPARAM(lParam);
            
            if (altPressed) {
                // Alt + Right click = Camera zoom
                g_app.viewport.onMouseDown(1, mouseX, mouseY, altPressed);
                SetCapture(hwnd);
            } else {
                // Right click without Alt = Context menu (in viewport area only)
                // Check if in viewport area
                float leftPanelWidth = 280.0f;
                float rightPanelStart = (float)g_app.width - 320.0f;
                float topOffset = 19.0f + 32.0f + 36.0f;  // Menu + Mode tabs + Toolbar
                
                if (mouseX > leftPanelWidth && mouseX < rightPanelStart && mouseY > topOffset) {
                    g_app.addObjectMenu.openAt(mouseX, mouseY);
                }
            }
        }
        return 0;
        
    case WM_MBUTTONDOWN:
        if (!imguiWantsMouse) {
            g_app.viewport.onMouseDown(2, (float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), altPressed);
            if (altPressed) SetCapture(hwnd);
        }
        return 0;
        
    case WM_LBUTTONUP: {
        // End gizmo drag
        if (g_app.gizmo.isDragging()) {
            g_app.gizmo.endDrag();
            ReleaseCapture();
            g_app.mouseWasDown = false;
            return 0;
        }
        
        // 结束框选/圆选/套索选择
        if (g_app.selectionBox.isSelecting) {
            // 保存选择区域信息
            float selMinX, selMinY, selMaxX, selMaxY;
            g_app.selectionBox.getSelectionRect(selMinX, selMinY, selMaxX, selMaxY);
            auto selectTool = g_app.selectionBox.selectTool;
            float circleRadius = g_app.selectionBox.circleRadius;
            ImVec2 circleCenter = g_app.selectionBox.currentPos;
            
            g_app.selectionBox.endSelection();
            ReleaseCapture();
            
            // 执行实际的框选/圆选逻辑
            bool isEditMode = (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit);
            if (isEditMode && g_app.currentEditMesh) {
                // 获取变换矩阵
                float viewMatrix[16], projMatrix[16];
                g_app.renderer.getViewMatrix(viewMatrix);
                g_app.renderer.getProjectionMatrix(projMatrix);
                
                const float* worldMatrix = nullptr;
                if (auto* sel = g_app.scene.getSelectedEntity()) {
                    worldMatrix = sel->worldMatrix.m;
                }
                
                // 是否按住Shift进行追加选择
                bool additive = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (!additive) {
                    // 清除之前的选择
                    g_app.currentEditMesh->selectedVertices.clear();
                    g_app.currentEditMesh->selectedEdges.clear();
                    g_app.currentEditMesh->selectedFaces.clear();
                }
                
                // 投影顶点到屏幕空间的辅助函数
                auto projectToScreen = [&](const float* pos, float& screenX, float& screenY) -> bool {
                    float worldPos[4] = {pos[0], pos[1], pos[2], 1.0f};
                    
                    // 应用世界矩阵
                    if (worldMatrix) {
                        float wp[4];
                        wp[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
                        wp[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
                        wp[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
                        wp[3] = 1.0f;
                        worldPos[0] = wp[0]; worldPos[1] = wp[1]; worldPos[2] = wp[2]; worldPos[3] = wp[3];
                    }
                    
                    // 应用视图矩阵
                    float viewPos[4];
                    viewPos[0] = viewMatrix[0]*worldPos[0] + viewMatrix[4]*worldPos[1] + viewMatrix[8]*worldPos[2] + viewMatrix[12];
                    viewPos[1] = viewMatrix[1]*worldPos[0] + viewMatrix[5]*worldPos[1] + viewMatrix[9]*worldPos[2] + viewMatrix[13];
                    viewPos[2] = viewMatrix[2]*worldPos[0] + viewMatrix[6]*worldPos[1] + viewMatrix[10]*worldPos[2] + viewMatrix[14];
                    viewPos[3] = viewMatrix[3]*worldPos[0] + viewMatrix[7]*worldPos[1] + viewMatrix[11]*worldPos[2] + viewMatrix[15];
                    
                    // 应用投影矩阵
                    float clipPos[4];
                    clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
                    clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
                    clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
                    clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
                    
                    // 透视除法
                    if (clipPos[3] <= 0.0f) return false;  // 在相机后面
                    float ndcX = clipPos[0] / clipPos[3];
                    float ndcY = clipPos[1] / clipPos[3];
                    
                    // NDC到屏幕坐标
                    screenX = (ndcX + 1.0f) * 0.5f * g_app.width;
                    screenY = (1.0f - ndcY) * 0.5f * g_app.height;  // Y翻转
                    
                    return true;
                };
                
                // 检查点是否在选择区域内
                auto isInSelection = [&](float sx, float sy) -> bool {
                    if (selectTool == luma::editor::EditModeToolbar::SelectTool::Box) {
                        return sx >= selMinX && sx <= selMaxX && sy >= selMinY && sy <= selMaxY;
                    } else if (selectTool == luma::editor::EditModeToolbar::SelectTool::Circle) {
                        float dx = sx - circleCenter.x;
                        float dy = sy - circleCenter.y;
                        return (dx*dx + dy*dy) <= (circleRadius * circleRadius);
                    }
                    // Lasso暂不实现精确判断，使用边界框
                    return sx >= selMinX && sx <= selMaxX && sy >= selMinY && sy <= selMaxY;
                };
                
                auto selectMode = g_app.editToolbar.selectMode;
                int selectedCount = 0;
                
                // 根据选择模式进行选择
                if (selectMode == luma::editor::EditModeToolbar::SelectMode::Vertex) {
                    // 顶点选择
                    for (size_t vi = 0; vi < g_app.currentEditMesh->vertices.size(); ++vi) {
                        const auto& v = g_app.currentEditMesh->vertices[vi];
                        float screenX, screenY;
                        if (projectToScreen(v.position, screenX, screenY)) {
                            if (isInSelection(screenX, screenY)) {
                                g_app.currentEditMesh->selectedVertices.insert(static_cast<uint32_t>(vi));
                                selectedCount++;
                            }
                        }
                    }
                    printf("[BOX SELECT] Selected %d vertices\n", selectedCount);
                } 
                else if (selectMode == luma::editor::EditModeToolbar::SelectMode::Edge) {
                    // 边选择 - 检查边的中点
                    for (size_t ei = 0; ei < g_app.currentEditMesh->edges.size(); ++ei) {
                        const auto& edge = g_app.currentEditMesh->edges[ei];
                        if (edge.v0 >= g_app.currentEditMesh->vertices.size() ||
                            edge.v1 >= g_app.currentEditMesh->vertices.size()) continue;
                        
                        const auto& v0 = g_app.currentEditMesh->vertices[edge.v0];
                        const auto& v1 = g_app.currentEditMesh->vertices[edge.v1];
                        
                        // 计算边的中点
                        float midPos[3] = {
                            (v0.position[0] + v1.position[0]) * 0.5f,
                            (v0.position[1] + v1.position[1]) * 0.5f,
                            (v0.position[2] + v1.position[2]) * 0.5f
                        };
                        
                        float screenX, screenY;
                        if (projectToScreen(midPos, screenX, screenY)) {
                            if (isInSelection(screenX, screenY)) {
                                g_app.currentEditMesh->selectedEdges.insert(static_cast<uint32_t>(ei));
                                selectedCount++;
                            }
                        }
                    }
                    printf("[BOX SELECT] Selected %d edges\n", selectedCount);
                }
                else if (selectMode == luma::editor::EditModeToolbar::SelectMode::Face) {
                    // 面选择 - 检查面的中心点
                    for (size_t fi = 0; fi < g_app.currentEditMesh->faces.size(); ++fi) {
                        const auto& face = g_app.currentEditMesh->faces[fi];
                        if (face.loops.empty()) continue;
                        
                        // 计算面的中心
                        float centerPos[3] = {0, 0, 0};
                        int validVerts = 0;
                        for (const auto& loop : face.loops) {
                            if (loop.vertexIndex < g_app.currentEditMesh->vertices.size()) {
                                const auto& v = g_app.currentEditMesh->vertices[loop.vertexIndex];
                                centerPos[0] += v.position[0];
                                centerPos[1] += v.position[1];
                                centerPos[2] += v.position[2];
                                validVerts++;
                            }
                        }
                        if (validVerts > 0) {
                            centerPos[0] /= validVerts;
                            centerPos[1] /= validVerts;
                            centerPos[2] /= validVerts;
                            
                            float screenX, screenY;
                            if (projectToScreen(centerPos, screenX, screenY)) {
                                if (isInSelection(screenX, screenY)) {
                                    g_app.currentEditMesh->selectedFaces.insert(static_cast<uint32_t>(fi));
                                    selectedCount++;
                                }
                            }
                        }
                    }
                    printf("[BOX SELECT] Selected %d faces\n", selectedCount);
                }
            }
            
            g_app.mouseWasDown = false;
            return 0;
        }
        
        // Check if this was a click (not a drag) for selection
        if (g_app.mouseWasDown && !imguiWantsMouse) {
            float mouseX = (float)GET_X_LPARAM(lParam);
            float mouseY = (float)GET_Y_LPARAM(lParam);
            
            // Consider it a click if mouse moved less than 5 pixels
            float dx = mouseX - g_app.mouseDownX;
            float dy = mouseY - g_app.mouseDownY;
            float distSq = dx * dx + dy * dy;
            
            if (distSq < 25.0f) {
                // Check if in Edit mode with EditMesh
                bool isEditMode = (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit);
                
                if (isEditMode && g_app.currentEditMesh) {
                    // Edit mode: pick vertices/edges/faces (only for Click select tool)
                    auto selectTool = g_app.editToolbar.selectTool;
                    if (selectTool == luma::editor::EditModeToolbar::SelectTool::Click) {
                        float viewMatrix[16], projMatrix[16];
                        g_app.renderer.getViewMatrix(viewMatrix);
                        g_app.renderer.getProjectionMatrix(projMatrix);
                        
                        luma::editor::Ray pickRay = luma::editor::MeshPicker::createRayFromScreen(
                            mouseX, mouseY, g_app.width, g_app.height, viewMatrix, projMatrix);
                        
                        // Get world matrix for selected entity
                        const float* worldMatrix = nullptr;
                        if (auto* sel = g_app.scene.getSelectedEntity()) {
                            worldMatrix = sel->worldMatrix.m;
                        }
                        
                        // Determine selection mode from toolbar
                        luma::editor::SelectionMode selMode = luma::editor::SelectionMode::Face;
                        if (g_app.editToolbar.selectMode == luma::editor::EditModeToolbar::SelectMode::Vertex) {
                            selMode = luma::editor::SelectionMode::Vertex;
                        } else if (g_app.editToolbar.selectMode == luma::editor::EditModeToolbar::SelectMode::Edge) {
                            selMode = luma::editor::SelectionMode::Edge;
                        }
                        
                        luma::editor::PickResult pickResult = g_app.meshPicker.pick(
                            pickRay, *g_app.currentEditMesh, selMode, worldMatrix);
                        
                        if (pickResult.hit()) {
                            bool additive = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                            
                            switch (pickResult.type) {
                                case luma::editor::PickResult::Type::Vertex:
                                    g_app.currentEditMesh->selectVertex(pickResult.index, additive);
                                    printf("[PICK] Selected vertex %u\n", pickResult.index);
                                    break;
                                case luma::editor::PickResult::Type::Edge:
                                    g_app.currentEditMesh->selectEdge(pickResult.index, additive);
                                    printf("[PICK] Selected edge %u\n", pickResult.index);
                                    break;
                                case luma::editor::PickResult::Type::Face:
                                    g_app.currentEditMesh->selectFace(pickResult.index, additive);
                                    printf("[PICK] Selected face %u\n", pickResult.index);
                                    break;
                                default:
                                    break;
                            }
                        } else if (!((GetKeyState(VK_SHIFT) & 0x8000) != 0)) {
                            // Click on empty space without Shift = deselect
                            g_app.currentEditMesh->selectNone();
                        }
                    }
                } else {
                    // Scene mode: pick entities
                    luma::Ray ray = getMouseRay(mouseX, mouseY);
                    luma::PickResult pick = luma::pickEntity(g_app.scene, ray);
                    
                    if (pick.hit()) {
                        g_app.scene.setSelectedEntity(pick.entity);
                    } else {
                        g_app.scene.clearSelection();
                    }
                }
            }
        }
        g_app.mouseWasDown = false;
        
        g_app.viewport.onMouseUp(0);
        if (g_app.viewport.cameraMode == luma::CameraMode::None) ReleaseCapture();
        return 0;
    }
        
    case WM_RBUTTONUP:
        g_app.viewport.onMouseUp(1);
        if (g_app.viewport.cameraMode == luma::CameraMode::None) ReleaseCapture();
        return 0;
        
    case WM_MBUTTONUP:
        g_app.viewport.onMouseUp(2);
        if (g_app.viewport.cameraMode == luma::CameraMode::None) ReleaseCapture();
        return 0;
        
    case WM_MOUSEMOVE: {
        float mouseX = (float)GET_X_LPARAM(lParam);
        float mouseY = (float)GET_Y_LPARAM(lParam);
        
        // Handle gizmo drag
        if (g_app.gizmo.isDragging()) {
            luma::Ray ray = getMouseRay(mouseX, mouseY);
            g_app.gizmo.updateDrag(ray);
            
            // Update the entity's world matrix after transform change
            if (luma::Entity* target = g_app.scene.getSelectedEntity()) {
                target->updateWorldMatrix();
            }
            return 0;
        }
        
        // 更新框选/圆选
        if (g_app.selectionBox.isSelecting) {
            g_app.selectionBox.updateSelection(mouseX, mouseY);
            return 0;
        }
        
        // Handle camera movement
        if (g_app.viewport.cameraMode != luma::CameraMode::None) {
            g_app.viewport.onMouseMove(mouseX, mouseY, g_app.getSceneRadius());
        }
        return 0;
    }
        
    case WM_MOUSEWHEEL:
        if (!imguiWantsMouse) {
            float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            g_app.viewport.onMouseWheel(delta, g_app.getSceneRadius());
        }
        return 0;
        
    case WM_DESTROY:
        g_app.shouldQuit = true;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ===== File Dialogs =====
static std::string OpenFileDialog(const char* filter = nullptr) {
    char path[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_app.hwnd;
    ofn.lpstrFilter = filter ? filter : luma::get_file_filter();
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return path;
    return "";
}

static std::string SaveFileDialog(const char* filter, const char* defaultExt) {
    char path[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_app.hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameA(&ofn)) return path;
    return "";
}

// ===== ImGui Initialization =====
static ComPtr<ID3D12DescriptorHeap> g_imguiSrvHeap;

static bool InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DisplaySize = ImVec2((float)g_app.width, (float)g_app.height);
    
    // Load Chinese font (Microsoft YaHei)
    // Try multiple font paths for compatibility
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",      // 微软雅黑
        "C:\\Windows\\Fonts\\msyhbd.ttc",    // 微软雅黑 Bold
        "C:\\Windows\\Fonts\\simhei.ttf",    // 黑体
        "C:\\Windows\\Fonts\\simsun.ttc",    // 宋体
    };
    
    bool fontLoaded = false;
    for (const char* fontPath : fontPaths) {
        if (GetFileAttributesA(fontPath) != INVALID_FILE_ATTRIBUTES) {
            io.Fonts->AddFontFromFileTTF(fontPath, 16.0f, nullptr, 
                                         io.Fonts->GetGlyphRangesChineseFull());
            fontLoaded = true;
            std::cout << "[luma] Loaded Chinese font: " << fontPath << std::endl;
            break;
        }
    }
    
    if (!fontLoaded) {
        std::cout << "[luma] Warning: No Chinese font found, using default" << std::endl;
        io.Fonts->AddFontDefault();
    }
    
    // Apply new editor theme
    luma::ui::applyEditorTheme();
    
    auto* device = static_cast<ID3D12Device*>(g_app.renderer.getNativeDevice());
    
    // Create dedicated SRV heap for ImGui (separate from renderer's heap)
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HRESULT hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_imguiSrvHeap));
    if (FAILED(hr)) {
        std::cerr << "[luma] Failed to create ImGui SRV heap" << std::endl;
        return false;
    }
    
    if (!ImGui_ImplWin32_Init(g_app.hwnd)) {
        std::cerr << "[luma] Failed to init ImGui Win32" << std::endl;
        return false;
    }
    
    if (!ImGui_ImplDX12_Init(device, 2, DXGI_FORMAT_R8G8B8A8_UNORM, g_imguiSrvHeap.Get(),
        g_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart())) {
        std::cerr << "[luma] Failed to init ImGui DX12" << std::endl;
        return false;
    }
    
    // Build font atlas
    ImGui_ImplDX12_CreateDeviceObjects();
    
    g_imguiInitialized = true;
    std::cout << "[luma] ImGui initialized successfully" << std::endl;
    return true;
}

// ===== Setup Callbacks =====
static void SetupEditorCallbacks() {
    // Model load callback
    g_app.editorState.onModelLoad = [](const std::string& path) {
        std::string modelPath = path;
        if (modelPath.empty()) {
            // Open file dialog if no path provided
            modelPath = OpenFileDialog(
                "3D Models\0*.obj;*.fbx;*.gltf;*.glb;*.dae\0"
                "OBJ Files (*.obj)\0*.obj\0"
                "FBX Files (*.fbx)\0*.fbx\0"
                "glTF Files (*.gltf;*.glb)\0*.gltf;*.glb\0"
                "All Files (*.*)\0*.*\0"
            );
        }
        if (!modelPath.empty()) {
            g_app.pendingModelPath = modelPath;
            g_app.editorState.consoleLogs.push_back("[INFO] Loading model: " + modelPath);
        }
    };
    
    // Scene save callback (saves camera and post-process settings)
    g_app.editorState.onSceneSave = [](const std::string& path) {
        std::string savePath = path;
        if (savePath.empty()) {
            if (g_app.currentScenePath.empty()) {
                savePath = SaveFileDialog("LUMA Scene (*.luma)\0*.luma\0", "luma");
            } else {
                savePath = g_app.currentScenePath;
            }
        }
        if (!savePath.empty()) {
            auto camParams = g_app.viewport.getCameraParams();
            if (luma::SceneSerializer::saveSceneFull(g_app.scene, savePath, 
                                                      camParams, g_app.postProcess)) {
                g_app.currentScenePath = savePath;
                g_app.editorState.consoleLogs.push_back("[INFO] Scene saved: " + savePath);
                std::cout << "[luma] Scene saved: " << savePath << std::endl;
            } else {
                g_app.editorState.consoleLogs.push_back("[ERROR] Failed to save scene");
            }
        }
    };
    
    // Scene load callback (loads camera and post-process settings)
    g_app.editorState.onSceneLoad = [](const std::string& path) {
        std::string loadPath = path;
        if (loadPath.empty()) {
            loadPath = OpenFileDialog("LUMA Scene (*.luma)\0*.luma\0All Files (*.*)\0*.*\0");
        }
        if (!loadPath.empty()) {
            luma::RHICameraParams loadedCamera;
            luma::PostProcessSettings loadedPostProcess;
            if (luma::SceneSerializer::loadSceneFull(g_app.scene, loadPath, 
                loadedCamera, loadedPostProcess,
                [](const std::string& modelPath, luma::RHILoadedModel& model) {
                    return g_app.renderer.loadModelAsync(modelPath, model);
                })) {
                g_app.currentScenePath = loadPath;
                // Apply loaded camera settings
                g_app.viewport.camera.yaw = loadedCamera.yaw;
                g_app.viewport.camera.pitch = loadedCamera.pitch;
                g_app.viewport.camera.distance = loadedCamera.distance;
                g_app.viewport.camera.targetX = loadedCamera.targetOffsetX;
                g_app.viewport.camera.targetY = loadedCamera.targetOffsetY;
                g_app.viewport.camera.targetZ = loadedCamera.targetOffsetZ;
                // Apply loaded post-process settings
                g_app.postProcess = loadedPostProcess;
                g_app.editorState.consoleLogs.push_back("[INFO] Scene loaded: " + loadPath);
                std::cout << "[luma] Scene loaded: " << loadPath << std::endl;
            } else {
                g_app.editorState.consoleLogs.push_back("[ERROR] Failed to load scene");
            }
        }
    };
    
    // Setup AssetManager with model loader
    auto& assetMgr = luma::getAssetManager();
    assetMgr.setModelLoader([](const std::string& path) -> std::shared_ptr<void> {
        auto model = luma::load_model(path);
        if (model) {
            return std::make_shared<luma::Model>(std::move(*model));
        }
        return nullptr;
    });
    
    // Configure cache settings
    assetMgr.setMaxCacheSize(512 * 1024 * 1024);  // 512 MB
    assetMgr.setUnusedTimeout(std::chrono::seconds(300));  // 5 minutes
    
    // ========== Mode System Callbacks ==========
    
    // Welcome screen callbacks
    g_app.welcomeScreen.onNewScene = []() {
        g_app.scene.clear();
        g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
        g_app.projectName = "未命名场景";
        g_app.currentScenePath.clear();
        g_app.editorState.consoleLogs.push_back("[INFO] 新建场景");
    };
    
    g_app.welcomeScreen.onOpenProject = []() {
        std::string loadPath = OpenFileDialog("LUMA Scene (*.luma)\0*.luma\0All Files (*.*)\0*.*\0");
        if (!loadPath.empty()) {
            g_app.editorState.onSceneLoad(loadPath);
            g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            g_app.welcomeScreen.isVisible = false;
            // Extract project name from path
            size_t lastSlash = loadPath.find_last_of("/\\");
            size_t lastDot = loadPath.find_last_of(".");
            if (lastSlash != std::string::npos) {
                g_app.projectName = loadPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
            }
        }
    };
    
    g_app.welcomeScreen.onOpenRecent = [](const std::string& path) {
        g_app.editorState.onSceneLoad(path);
        g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
    };
    
    g_app.welcomeScreen.onLoadPreset = [](const std::string& preset) {
        g_app.scene.clear();
        g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
        g_app.projectName = preset;
        
        // Create default setup based on preset
        if (preset == "studio") {
            g_app.projectName = "摄影棚";
            // TODO: Add studio lights, backdrop, etc.
        } else if (preset == "park") {
            g_app.projectName = "户外公园";
            // TODO: Add terrain, trees, sunlight
        } else if (preset == "castle") {
            g_app.projectName = "中世纪城堡";
        } else if (preset == "spaceship") {
            g_app.projectName = "科幻太空船";
        }
        
        g_app.editorState.consoleLogs.push_back("[INFO] 加载预设: " + g_app.projectName);
    };
    
    // Empty scene guide callbacks
    g_app.emptySceneGuide.onCreateCharacter = []() {
        g_app.editorState.showCharacterCreator = true;
    };
    
    g_app.emptySceneGuide.onAddObject = []() {
        // Open add object popup at center of viewport
        float viewportCenterX = 280.0f + (g_app.width - 280.0f - 320.0f) / 2.0f;
        float viewportCenterY = 100.0f + (g_app.height - 100.0f) / 2.0f;
        g_app.addObjectMenu.openAt(viewportCenterX, viewportCenterY);
    };
    
    g_app.emptySceneGuide.onImportModel = []() {
        g_app.editorState.onModelLoad("");
        g_app.emptySceneGuide.isVisible = false;
    };
    
    g_app.emptySceneGuide.onLoadPreset = [](const std::string& preset) {
        g_app.welcomeScreen.onLoadPreset(preset);
    };
    
    // Add object menu callbacks
    g_app.addObjectMenu.menu.onCreatePrimitive = [](const std::string& type) {
        luma::Mesh mesh;
        std::string name;
        
        if (type == "立方体" || type == "Cube") {
            mesh = luma::create_cube();
            name = "Cube";
        } else if (type == "球体" || type == "Sphere") {
            mesh = luma::create_sphere(32, 16);
            name = "Sphere";
        } else if (type == "圆柱体" || type == "Cylinder") {
            mesh = luma::create_cylinder(32, 1.0f, 2.0f);
            name = "Cylinder";
        } else if (type == "平面" || type == "Plane") {
            mesh = luma::create_plane(10.0f, 10.0f);
            name = "Plane";
        } else {
            return;
        }
        
        luma::RHILoadedModel primModel;
        primModel.meshes.push_back(g_app.renderer.uploadMesh(mesh));
        primModel.center[0] = primModel.center[1] = primModel.center[2] = 0.0f;
        primModel.radius = 1.0f;
        primModel.name = name;
        primModel.debugName = "primitives/" + name;
        
        luma::Entity* entity = g_app.scene.createEntityWithModel(name, primModel);
        entity->material = std::make_shared<luma::Material>();
        entity->material->baseColor = {0.8f, 0.8f, 0.8f};
        entity->material->metallic = 0.0f;
        entity->material->roughness = 0.5f;
        
        g_app.scene.setSelectedEntity(entity);
        g_app.emptySceneGuide.isVisible = false;
        
        g_app.editorState.consoleLogs.push_back("[INFO] 创建: " + name);
    };
    
    g_app.addObjectMenu.menu.onImportModel = []() {
        g_app.editorState.onModelLoad("");
        g_app.emptySceneGuide.isVisible = false;
    };
    
    g_app.addObjectMenu.menu.onCreateLight = [](const std::string& type) {
        // TODO: Implement light creation
        g_app.editorState.consoleLogs.push_back("[INFO] 创建光源: " + type);
    };
    
    // Mode change callback
    g_app.modeManager.onModeChanged = [](luma::editor::EditorMode mode) {
        std::string modeName = luma::editor::EditorModeManager::getModeName(mode);
        g_app.editorState.consoleLogs.push_back("[INFO] 切换模式: " + std::string(modeName));
        
        // Reset mesh selection when leaving edit mode
        if (mode != luma::editor::EditorMode::Edit) {
            g_app.meshListPanel.selectedMeshIndex = -1;
            g_app.currentEditMesh.reset();
            g_app.editMeshDirty = false;
            g_app.uvEditor.close();
        } else {
            // Entering Edit mode - create EditMesh from selected entity
            auto* selectedEntity = g_app.scene.getSelectedEntity();
            if (selectedEntity && selectedEntity->hasModel) {
                // Create EditMesh from the selected mesh's GPU data
                int meshIdx = g_app.meshListPanel.selectedMeshIndex;
                if (meshIdx < 0) meshIdx = 0;
                
                if (meshIdx < static_cast<int>(selectedEntity->model.meshes.size())) {
                    const auto& gpuMesh = selectedEntity->model.meshes[meshIdx];
                    g_app.currentEditMesh = std::make_unique<luma::EditMesh>();
                    
                    // Build EditMesh from GPU mesh's original edges and vertex data
                    // We'll read the vertex positions from the renderer
                    if (gpuMesh.hasOriginalEdges && !gpuMesh.originalEdges.empty()) {
                        // Has quad/ngon topology - create from original edges
                        g_app.renderer.buildEditMeshFromGPU(selectedEntity->model, meshIdx, 
                                                            *g_app.currentEditMesh);
                        printf("[EDIT] Created EditMesh from GPU data: %zu verts, %zu edges, %d quads\n",
                               g_app.currentEditMesh->vertices.size(),
                               g_app.currentEditMesh->edges.size(),
                               g_app.currentEditMesh->quadCount());
                    } else {
                        // No original topology - create from triangles
                        g_app.renderer.buildEditMeshFromGPUTriangles(selectedEntity->model, meshIdx,
                                                                      *g_app.currentEditMesh);
                        printf("[EDIT] Created EditMesh from triangles: %zu verts, %zu faces\n",
                               g_app.currentEditMesh->vertices.size(),
                               g_app.currentEditMesh->faces.size());
                    }
                    
                    g_app.editMeshDirty = false;
                }
            }
        }
    };
}

// ===== Render UI =====
static void RenderUI() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // ========== Welcome Screen (highest priority) ==========
    if (g_app.welcomeScreen.isVisible) {
        g_app.welcomeScreen.draw((float)g_app.width, (float)g_app.height, 
                                  g_app.modeManager.recentProjects);
        ImGui::Render();
        return;  // Don't render anything else during welcome
    }
    
    // ========== Get current mode state ==========
    luma::Entity* selectedEntity = g_app.scene.getSelectedEntity();
    luma::editor::ObjectType objType = luma::editor::ObjectType::None;
    bool hasSkeleton = false;
    
    if (selectedEntity) {
        if (selectedEntity->hasModel) {
            // Check if it's a character (has skeleton)
            if (selectedEntity->skeleton) {
                objType = luma::editor::ObjectType::Character;
                hasSkeleton = true;
            } else {
                objType = luma::editor::ObjectType::Model;
            }
        }
    }
    
    luma::editor::ModeAvailability availability = 
        g_app.modeManager.getAvailability(objType, hasSkeleton);
    
    // ========== Main menu bar ==========
    luma::ui::drawMainMenuBar(g_app.editorState, g_app.viewport, g_app.shouldQuit);
    
    // ========== Toolbar with integrated Mode Tabs ==========
    if (g_app.modeManager.currentMode != luma::editor::EditorMode::Play) {
        luma::ui::drawToolbar(g_app.editorState, g_app.gizmo);
    }
    
    // ========== Mode Buttons (separate bar below toolbar) ==========
    {
        ImGuiIO& io = ImGui::GetIO();
        float modeBarY = luma::ui::EditorLayout::kMenuBarHeight + luma::ui::EditorLayout::kToolbarHeight;
        float modeBarHeight = luma::ui::EditorLayout::kModeBarHeight;
        
        ImGui::SetNextWindowPos(ImVec2(0, modeBarY));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, modeBarHeight));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.13f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        
        ImGuiWindowFlags modeFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoSavedSettings;
        
        if (ImGui::Begin("##ModeTabs", nullptr, modeFlags)) {
            using luma::editor::EditorMode;
            using luma::ui::loc;
            auto currentMode = g_app.modeManager.currentMode;
            
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", loc("Mode:"));
            ImGui::SameLine(0, 10);
            
            auto drawModeBtn = [&](EditorMode mode, const char* labelKey, bool enabled, const char* tooltipKey = nullptr) {
                bool isSelected = (currentMode == mode);
                
                if (!enabled) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
                } else if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.58f, 1.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.24f, 0.28f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.35f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                }
                
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                if (ImGui::Button(loc(labelKey), ImVec2(55, 24)) && enabled) {
                    g_app.modeManager.switchMode(mode);
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
                
                if (!enabled && ImGui::IsItemHovered() && tooltipKey) {
                    ImGui::SetTooltip("%s", loc(tooltipKey));
                }
                
                ImGui::SameLine(0, 4);
            };
            
            drawModeBtn(EditorMode::Scene, "Scene", true);
            drawModeBtn(EditorMode::Character, "Character", availability.character, "Please select a character object");
            drawModeBtn(EditorMode::Edit, "Edit", availability.edit, "Please select an object");
            drawModeBtn(EditorMode::Animation, "Animation", availability.animation, "Selected object has no skeleton");
            
            ImGui::SameLine(0, 15);
            ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "|");
            ImGui::SameLine(0, 15);
            
            drawModeBtn(EditorMode::Play, "Play", true);
            
            // Language switcher
            ImGui::SameLine(io.DisplaySize.x - 380);
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "%s:", loc("Language"));
            ImGui::SameLine();
            auto& locMgr = luma::ui::Localization::instance();
            bool isChinese = locMgr.getLanguage() == luma::ui::Language::Chinese;
            if (ImGui::SmallButton(isChinese ? "EN" : "中")) {
                locMgr.setLanguage(isChinese ? luma::ui::Language::English : luma::ui::Language::Chinese);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", isChinese ? "Switch to English" : "切换到中文");
            }
            
            // Project name on the right
            ImGui::SameLine(io.DisplaySize.x - 280);
            ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.0f), "%s %s", loc("Project:"), g_app.projectName.c_str());
        }
        ImGui::End();
        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    
    // ========== Left panels ==========
    luma::ui::drawHierarchyPanel(g_app.scene, g_app.editorState);
    
    // ========== Right panels (mode-specific) ==========
    if (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit) {
        // Edit mode: Show mesh list + material editor
        float rightPanelX = (float)g_app.width - 320.0f;
        float topOffset = luma::ui::EditorLayout::getTopOffset();
        
        ImGui::SetNextWindowPos(ImVec2(rightPanelX, topOffset));
        ImGui::SetNextWindowSize(ImVec2(320.0f, (float)g_app.height - topOffset - 24.0f));
        
        using luma::ui::loc;
        
        if (ImGui::Begin(loc("Edit Mode - Inspector"), nullptr, 
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            
            // Mode indicator
            ImGui::TextColored(ImVec4(0.26f, 0.53f, 0.96f, 1.0f), "[E] %s", loc("Edit"));
            ImGui::SameLine(200);
            
            // Open UV Editor button
            if (ImGui::SmallButton("UV")) {
                if (g_app.currentEditMesh) {
                    g_app.uvEditor.open(g_app.currentEditMesh.get());
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", loc("Open UV Editor"));
            }
            
            ImGui::SameLine();
            if (ImGui::SmallButton(loc("Exit"))) {
                // 退出编辑模式前保存
                if (g_app.editMeshDirty) {
                    // TODO: 提示保存
                }
                g_app.currentEditMesh.reset();
                g_app.editMeshDirty = false;
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            }
            ImGui::Separator();
            
            // ========== Selection Mode & Edit Tools ==========
            if (ImGui::CollapsingHeader(loc("Edit Tools"), ImGuiTreeNodeFlags_DefaultOpen)) {
                g_app.editToolbar.draw(300.0f);
            }
            
            // ========== Undo/Redo ==========
            g_app.undoRedoBar.getUndoCount = [&]() { 
                return g_app.currentEditMesh ? g_app.currentEditMesh->getUndoCount() : 0; 
            };
            g_app.undoRedoBar.getRedoCount = [&]() { 
                return g_app.currentEditMesh ? g_app.currentEditMesh->getRedoCount() : 0; 
            };
            g_app.undoRedoBar.onUndo = [&]() {
                if (g_app.currentEditMesh && g_app.currentEditMesh->undo()) {
                    g_app.editMeshDirty = true;
                }
            };
            g_app.undoRedoBar.onRedo = [&]() {
                if (g_app.currentEditMesh && g_app.currentEditMesh->redo()) {
                    g_app.editMeshDirty = true;
                }
            };
            g_app.undoRedoBar.draw(300.0f);
            g_app.undoRedoBar.handleShortcuts();
            
            // ========== View mode toolbar ==========
            if (ImGui::CollapsingHeader(loc("View Mode"), ImGuiTreeNodeFlags_DefaultOpen)) {
                g_app.viewToolbar.draw(300.0f);
            }
            
            // ========== Mesh Statistics ==========
            if (g_app.currentEditMesh) {
                if (ImGui::CollapsingHeader(loc("Mesh Statistics"))) {
                    int vertCount = static_cast<int>(g_app.currentEditMesh->vertices.size());
                    int faceCount = static_cast<int>(g_app.currentEditMesh->faces.size());
                    int triCount = g_app.currentEditMesh->triangleCount();
                    int quadCount = g_app.currentEditMesh->quadCount();
                    int ngonCount = g_app.currentEditMesh->ngonCount();
                    g_app.meshStats.draw(vertCount, faceCount, triCount, quadCount, ngonCount);
                }
            }
            
            // ========== Mesh list with selection hint ==========
            if (ImGui::CollapsingHeader(loc("Mesh List"), ImGuiTreeNodeFlags_DefaultOpen)) {
                // Hint text
                if (g_app.meshListPanel.selectedMeshIndex < 0) {
                    ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "%s", loc("Click mesh in list to highlight"));
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s: Mesh %d", 
                                      loc("Selected Mesh"), g_app.meshListPanel.selectedMeshIndex);
                }
                ImGui::Spacing();
                
                // Set callback to rebuild EditMesh when user selects a different mesh
                g_app.meshListPanel.onMeshSelected = [selectedEntity](int meshIdx) {
                    if (!selectedEntity || !selectedEntity->hasModel) return;
                    if (meshIdx < 0 || meshIdx >= static_cast<int>(selectedEntity->model.meshes.size())) return;
                    
                    const auto& gpuMesh = selectedEntity->model.meshes[meshIdx];
                    g_app.currentEditMesh = std::make_unique<luma::EditMesh>();
                    
                    // Build EditMesh from GPU mesh
                    if (gpuMesh.hasOriginalEdges && !gpuMesh.originalEdges.empty()) {
                        // Has quad/ngon topology - build with original edges
                        g_app.renderer.buildEditMeshFromGPU(selectedEntity->model, meshIdx, *g_app.currentEditMesh);
                        printf("[EDIT] Rebuilt EditMesh from GPU quad data: mesh %d, %zu verts, %zu edges\n",
                               meshIdx, g_app.currentEditMesh->vertices.size(), g_app.currentEditMesh->edges.size());
                    } else {
                        // Triangulated - build from triangles
                        g_app.renderer.buildEditMeshFromGPUTriangles(selectedEntity->model, meshIdx, *g_app.currentEditMesh);
                        printf("[EDIT] Rebuilt EditMesh from GPU triangles: mesh %d, %zu verts, %zu faces\n",
                               meshIdx, g_app.currentEditMesh->vertices.size(), g_app.currentEditMesh->faces.size());
                    }
                    g_app.editMeshDirty = false;
                    g_app.syncEditMeshToModules();
                };
                
                g_app.meshListPanel.draw(selectedEntity, 300.0f);
            }
            
            ImGui::Spacing();
            
            // ========== Material editor ==========
            if (ImGui::CollapsingHeader(loc("Material Properties"))) {
                g_app.materialEditor.draw(selectedEntity, g_app.meshListPanel.selectedMeshIndex, 300.0f);
            }
            
            // ========== Save/Cancel bar at bottom ==========
            g_app.saveBar.onSave = [&]() {
                // 保存修改并退出
                if (g_app.currentEditMesh && selectedEntity && selectedEntity->hasModel) {
                    // TODO: 将 EditMesh 转换回 RenderMesh
                    printf("[EDIT] Saving changes...\n");
                }
                g_app.currentEditMesh.reset();
                g_app.editMeshDirty = false;
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            };
            g_app.saveBar.onCancel = [&]() {
                // 放弃修改并退出
                g_app.currentEditMesh.reset();
                g_app.editMeshDirty = false;
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            };
            g_app.saveBar.draw(g_app.editMeshDirty, 300.0f);
        }
        ImGui::End();
        
        // ========== UV Editor Window (floating) ==========
        g_app.uvEditor.draw();
    } else {
        // Scene mode: Show standard inspector
        luma::ui::drawInspectorPanel(g_app.scene, g_app.editorState);
    }
    
    luma::ui::drawPostProcessPanel(g_app.postProcess, g_app.editorState);
    luma::ui::drawRenderSettingsPanel(g_app.renderSettings, g_app.editorState);
    luma::ui::drawLightingPanel(g_app.lighting, g_app.editorState);
    
    // Bottom panels
    luma::ui::drawAnimationTimeline(g_app.animation, g_app.editorState);
    
    // Extended Asset Browser with cache statistics
    auto& assetMgr = luma::getAssetManager();
    auto stats = assetMgr.getStatistics();
    luma::ui::AssetCacheStats cacheStats;
    cacheStats.totalLoads = stats.totalLoads;
    cacheStats.cacheHits = stats.cacheHits;
    cacheStats.cacheMisses = stats.cacheMisses;
    cacheStats.hitRate = stats.hitRate;
    cacheStats.cachedAssets = stats.cachedAssets;
    cacheStats.cacheSizeBytes = stats.cacheSizeBytes;
    luma::ui::drawAssetBrowserExtended(g_app.editorState, &cacheStats);
    
    luma::ui::drawConsole(g_app.editorState);
    luma::ui::drawHistoryPanel(g_app.editorState);
    
    // Handle viewport drag-drop
    std::string droppedAsset;
    if (luma::ui::handleViewportDragDrop(droppedAsset)) {
        g_app.pendingModelPath = droppedAsset;
        g_app.editorState.consoleLogs.push_back("[INFO] Loading dropped asset: " + droppedAsset);
    }
    
    // ========== Empty Scene Guide (Scene mode only) ==========
    bool isSceneEmpty = g_app.scene.getAllEntities().size() <= 1;  // Just the default cube
    if (isSceneEmpty && g_app.modeManager.currentMode == luma::editor::EditorMode::Scene) {
        float viewportX = luma::ui::EditorLayout::kLeftPanelWidth;
        float viewportY = luma::ui::EditorLayout::getTopOffset();
        float viewportW = (float)g_app.width - luma::ui::EditorLayout::kLeftPanelWidth - luma::ui::EditorLayout::kRightPanelWidth;
        float viewportH = (float)g_app.height - viewportY - luma::ui::EditorLayout::kStatusBarHeight;
        
        g_app.emptySceneGuide.draw(viewportX, viewportY, viewportW, viewportH);
    }
    
    // ========== Edit Mode Viewport Header (Blender-style toolbar) ==========
    if (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit) {
        float viewportX = luma::ui::EditorLayout::kLeftPanelWidth;
        float viewportY = luma::ui::EditorLayout::getTopOffset();
        float viewportW = (float)g_app.width - luma::ui::EditorLayout::kLeftPanelWidth - 320.0f;
        
        // 连接视口工具栏的指针
        g_app.viewportHeader.selectMode = &g_app.editToolbar.selectMode;
        g_app.viewportHeader.selectTool = &g_app.editToolbar.selectTool;
        g_app.viewportHeader.viewMode = &g_app.viewToolbar.currentViewMode;
        g_app.viewportHeader.onUndo = [&]() {
            if (g_app.currentEditMesh && g_app.currentEditMesh->undo()) {
                g_app.editMeshDirty = true;
            }
        };
        g_app.viewportHeader.onRedo = [&]() {
            if (g_app.currentEditMesh && g_app.currentEditMesh->redo()) {
                g_app.editMeshDirty = true;
            }
        };
        
        g_app.viewportHeader.draw(viewportX, viewportY, viewportW);
    }
    
    // ========== Add Object Context Menu ==========
    g_app.addObjectMenu.draw();
    
    // ========== Selection Box Overlay (框选/圆选) ==========
    g_app.selectionBox.draw();
    
    // ========== Selected Faces Highlight (面模式选中高亮) ==========
    bool isEditMode = (g_app.modeManager.currentMode == luma::editor::EditorMode::Edit);
    auto selectMode = g_app.editToolbar.selectMode;
    if (isEditMode && g_app.currentEditMesh && 
        selectMode == luma::editor::EditModeToolbar::SelectMode::Face &&
        !g_app.currentEditMesh->selectedFaces.empty()) {
        
        // 获取变换矩阵
        float viewMatrix[16], projMatrix[16];
        g_app.renderer.getViewMatrix(viewMatrix);
        g_app.renderer.getProjectionMatrix(projMatrix);
        
        const float* worldMatrix = nullptr;
        if (auto* sel = g_app.scene.getSelectedEntity()) {
            worldMatrix = sel->worldMatrix.m;
        }
        
        // 投影顶点到屏幕空间
        auto projectToScreen = [&](const float* pos, float& screenX, float& screenY) -> bool {
            float worldPos[4] = {pos[0], pos[1], pos[2], 1.0f};
            
            if (worldMatrix) {
                float wp[4];
                wp[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
                wp[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
                wp[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
                worldPos[0] = wp[0]; worldPos[1] = wp[1]; worldPos[2] = wp[2]; worldPos[3] = 1.0f;
            }
            
            float viewPos[4];
            viewPos[0] = viewMatrix[0]*worldPos[0] + viewMatrix[4]*worldPos[1] + viewMatrix[8]*worldPos[2] + viewMatrix[12];
            viewPos[1] = viewMatrix[1]*worldPos[0] + viewMatrix[5]*worldPos[1] + viewMatrix[9]*worldPos[2] + viewMatrix[13];
            viewPos[2] = viewMatrix[2]*worldPos[0] + viewMatrix[6]*worldPos[1] + viewMatrix[10]*worldPos[2] + viewMatrix[14];
            viewPos[3] = viewMatrix[3]*worldPos[0] + viewMatrix[7]*worldPos[1] + viewMatrix[11]*worldPos[2] + viewMatrix[15];
            
            float clipPos[4];
            clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
            clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
            clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
            clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
            
            if (clipPos[3] <= 0.0f) return false;
            float ndcX = clipPos[0] / clipPos[3];
            float ndcY = clipPos[1] / clipPos[3];
            
            screenX = (ndcX + 1.0f) * 0.5f * g_app.width;
            screenY = (1.0f - ndcY) * 0.5f * g_app.height;
            return true;
        };
        
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        ImU32 faceColor = IM_COL32(255, 140, 0, 80);  // 橙色半透明填充
        ImU32 edgeColor = IM_COL32(255, 160, 0, 200);  // 橙色边框
        
        for (uint32_t fi : g_app.currentEditMesh->selectedFaces) {
            if (fi >= g_app.currentEditMesh->faces.size()) continue;
            const auto& face = g_app.currentEditMesh->faces[fi];
            if (face.loops.size() < 3) continue;
            
            // 收集面的屏幕坐标
            std::vector<ImVec2> screenPoints;
            bool allVisible = true;
            
            for (const auto& loop : face.loops) {
                if (loop.vertexIndex >= g_app.currentEditMesh->vertices.size()) {
                    allVisible = false;
                    break;
                }
                const auto& v = g_app.currentEditMesh->vertices[loop.vertexIndex];
                float sx, sy;
                if (projectToScreen(v.position, sx, sy)) {
                    screenPoints.push_back(ImVec2(sx, sy));
                } else {
                    allVisible = false;
                    break;
                }
            }
            
            if (allVisible && screenPoints.size() >= 3) {
                // 三角化面并绘制填充（简单扇形三角化）
                for (size_t i = 1; i < screenPoints.size() - 1; ++i) {
                    drawList->AddTriangleFilled(
                        screenPoints[0], 
                        screenPoints[i], 
                        screenPoints[i + 1], 
                        faceColor);
                }
                // 绘制面的边框
                for (size_t i = 0; i < screenPoints.size(); ++i) {
                    size_t nextI = (i + 1) % screenPoints.size();
                    drawList->AddLine(screenPoints[i], screenPoints[nextI], edgeColor, 2.0f);
                }
            }
        }
    }
    
    // ========== Selected Edges Highlight (线模式选中高亮) ==========
    if (isEditMode && g_app.currentEditMesh && 
        selectMode == luma::editor::EditModeToolbar::SelectMode::Edge &&
        !g_app.currentEditMesh->selectedEdges.empty()) {
        
        float viewMatrix[16], projMatrix[16];
        g_app.renderer.getViewMatrix(viewMatrix);
        g_app.renderer.getProjectionMatrix(projMatrix);
        
        const float* worldMatrix = nullptr;
        if (auto* sel = g_app.scene.getSelectedEntity()) {
            worldMatrix = sel->worldMatrix.m;
        }
        
        auto projectToScreen = [&](const float* pos, float& screenX, float& screenY) -> bool {
            float worldPos[4] = {pos[0], pos[1], pos[2], 1.0f};
            
            if (worldMatrix) {
                float wp[4];
                wp[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
                wp[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
                wp[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
                worldPos[0] = wp[0]; worldPos[1] = wp[1]; worldPos[2] = wp[2]; worldPos[3] = 1.0f;
            }
            
            float viewPos[4];
            viewPos[0] = viewMatrix[0]*worldPos[0] + viewMatrix[4]*worldPos[1] + viewMatrix[8]*worldPos[2] + viewMatrix[12];
            viewPos[1] = viewMatrix[1]*worldPos[0] + viewMatrix[5]*worldPos[1] + viewMatrix[9]*worldPos[2] + viewMatrix[13];
            viewPos[2] = viewMatrix[2]*worldPos[0] + viewMatrix[6]*worldPos[1] + viewMatrix[10]*worldPos[2] + viewMatrix[14];
            viewPos[3] = viewMatrix[3]*worldPos[0] + viewMatrix[7]*worldPos[1] + viewMatrix[11]*worldPos[2] + viewMatrix[15];
            
            float clipPos[4];
            clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
            clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
            clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
            clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
            
            if (clipPos[3] <= 0.0f) return false;
            float ndcX = clipPos[0] / clipPos[3];
            float ndcY = clipPos[1] / clipPos[3];
            
            screenX = (ndcX + 1.0f) * 0.5f * g_app.width;
            screenY = (1.0f - ndcY) * 0.5f * g_app.height;
            return true;
        };
        
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        ImU32 edgeColor = IM_COL32(255, 140, 0, 255);  // 橙色
        
        for (uint32_t ei : g_app.currentEditMesh->selectedEdges) {
            if (ei >= g_app.currentEditMesh->edges.size()) continue;
            const auto& edge = g_app.currentEditMesh->edges[ei];
            
            if (edge.v0 >= g_app.currentEditMesh->vertices.size() ||
                edge.v1 >= g_app.currentEditMesh->vertices.size()) continue;
            
            const auto& v0 = g_app.currentEditMesh->vertices[edge.v0];
            const auto& v1 = g_app.currentEditMesh->vertices[edge.v1];
            
            float sx0, sy0, sx1, sy1;
            if (projectToScreen(v0.position, sx0, sy0) && projectToScreen(v1.position, sx1, sy1)) {
                // 画粗线表示选中的边
                drawList->AddLine(ImVec2(sx0, sy0), ImVec2(sx1, sy1), edgeColor, 4.0f);
            }
        }
    }
    
    // ========== 点模式：显示所有顶点，选中的高亮 (ImGui覆盖层) ==========
    if (isEditMode && g_app.currentEditMesh && 
        selectMode == luma::editor::EditModeToolbar::SelectMode::Vertex &&
        g_app.editToolbar.showVertices) {
        
        float viewMatrix[16], projMatrix[16];
        g_app.renderer.getViewMatrix(viewMatrix);
        g_app.renderer.getProjectionMatrix(projMatrix);
        
        const float* worldMatrix = nullptr;
        if (auto* sel = g_app.scene.getSelectedEntity()) {
            worldMatrix = sel->worldMatrix.m;
        }
        
        auto projectToScreen = [&](const float* pos, float& screenX, float& screenY) -> bool {
            float worldPos[4] = {pos[0], pos[1], pos[2], 1.0f};
            
            if (worldMatrix) {
                float wp[4];
                wp[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
                wp[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
                wp[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
                worldPos[0] = wp[0]; worldPos[1] = wp[1]; worldPos[2] = wp[2]; worldPos[3] = 1.0f;
            }
            
            float viewPos[4];
            viewPos[0] = viewMatrix[0]*worldPos[0] + viewMatrix[4]*worldPos[1] + viewMatrix[8]*worldPos[2] + viewMatrix[12];
            viewPos[1] = viewMatrix[1]*worldPos[0] + viewMatrix[5]*worldPos[1] + viewMatrix[9]*worldPos[2] + viewMatrix[13];
            viewPos[2] = viewMatrix[2]*worldPos[0] + viewMatrix[6]*worldPos[1] + viewMatrix[10]*worldPos[2] + viewMatrix[14];
            viewPos[3] = viewMatrix[3]*worldPos[0] + viewMatrix[7]*worldPos[1] + viewMatrix[11]*worldPos[2] + viewMatrix[15];
            
            float clipPos[4];
            clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
            clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
            clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
            clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
            
            if (clipPos[3] <= 0.0f) return false;
            float ndcX = clipPos[0] / clipPos[3];
            float ndcY = clipPos[1] / clipPos[3];
            
            screenX = (ndcX + 1.0f) * 0.5f * g_app.width;
            screenY = (1.0f - ndcY) * 0.5f * g_app.height;
            return true;
        };
        
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        
        // 先画所有未选中的顶点（小黑点）
        ImU32 normalColor = IM_COL32(30, 30, 30, 255);  // 深灰/黑色
        for (size_t vi = 0; vi < g_app.currentEditMesh->vertices.size(); ++vi) {
            bool isSelected = g_app.currentEditMesh->selectedVertices.count(static_cast<uint32_t>(vi)) > 0;
            if (isSelected) continue;  // 选中的后面画
            
            const auto& v = g_app.currentEditMesh->vertices[vi];
            float sx, sy;
            if (projectToScreen(v.position, sx, sy)) {
                drawList->AddCircleFilled(ImVec2(sx, sy), 2.0f, normalColor, 6);
            }
        }
        
        // 再画选中的顶点（橙色，稍大）
        ImU32 selectedColor = IM_COL32(255, 140, 0, 255);  // 橙色
        ImU32 selectedFill = IM_COL32(255, 180, 50, 220);  // 浅橙色填充
        for (uint32_t vi : g_app.currentEditMesh->selectedVertices) {
            if (vi >= g_app.currentEditMesh->vertices.size()) continue;
            const auto& v = g_app.currentEditMesh->vertices[vi];
            
            float sx, sy;
            if (projectToScreen(v.position, sx, sy)) {
                drawList->AddCircleFilled(ImVec2(sx, sy), 4.0f, selectedFill, 8);
                drawList->AddCircle(ImVec2(sx, sy), 4.0f, selectedColor, 8, 1.5f);
            }
        }
    }
    
    // Overlays
    luma::ui::drawStatsPanel(g_app.editorState);
    luma::ui::drawShaderStatus(
        g_app.renderer.getShaderError(),
        g_app.renderer.isShaderHotReloadEnabled(),
        [&]() { g_app.renderer.reloadShaders(); },
        g_app.editorState
    );
    
    // Loading progress
    float loadProgress = g_app.renderer.getAsyncLoadProgress();
    if (loadProgress < 1.0f) {
        ImGui::SetNextWindowPos(ImVec2((float)g_app.width - 270, 60), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(260, 60));
        if (ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Loading textures...");
            ImGui::ProgressBar(loadProgress, ImVec2(-1, 0));
        }
        ImGui::End();
    }
    
    // Help overlay
    if (g_app.editorState.showHelp) {
        ImGui::SetNextWindowPos(ImVec2(g_app.width * 0.5f, g_app.height * 0.5f), 
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 280));
        if (ImGui::Begin("Keyboard Shortcuts", &g_app.editorState.showHelp,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Camera Controls:");
            ImGui::Separator();
            ImGui::BulletText("Alt + Left Mouse:   Orbit");
            ImGui::BulletText("Alt + Middle Mouse: Pan");
            ImGui::BulletText("Alt + Right Mouse:  Zoom");
            ImGui::BulletText("Mouse Wheel:        Zoom");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Transform Tools:");
            ImGui::Separator();
            ImGui::BulletText("W: Move Tool");
            ImGui::BulletText("E: Rotate Tool");
            ImGui::BulletText("R: Scale Tool");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Other:");
            ImGui::Separator();
            ImGui::BulletText("F:   Focus on selection");
            ImGui::BulletText("G:   Toggle grid");
            ImGui::BulletText("Del: Delete selection");
            ImGui::BulletText("F1:  Toggle this help");
        }
        ImGui::End();
    }
    
    // Status bar
    std::string status;
    if (auto* sel = g_app.scene.getSelectedEntity()) {
        status = "Selected: " + sel->name;
    }
    luma::ui::drawStatusBar(g_app.width, g_app.height, status);
    
    ImGui::Render();
}

// ===== Main Entry =====
int main() {
    std::cout << "[luma] LUMA Studio starting..." << std::endl;
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LumaStudioClass";
    RegisterClassExW(&wc);
    
    // Create window
    RECT rc = {0, 0, g_app.width, g_app.height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_app.hwnd = CreateWindowExW(0, L"LumaStudioClass", L"LUMA Studio",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, wc.hInstance, nullptr);
    
    if (!g_app.hwnd) {
        std::cerr << "[luma] Failed to create window" << std::endl;
        return 1;
    }
    
    // Initialize renderer
    if (!g_app.renderer.initialize(g_app.hwnd, g_app.width, g_app.height)) {
        std::cerr << "[luma] Failed to initialize renderer" << std::endl;
        return 1;
    }
    
    // Enable shader hot-reload
    g_app.renderer.setShaderHotReload(true);
    
    // Initialize ImGui
    if (!InitImGui()) {
        std::cerr << "[luma] Failed to initialize ImGui" << std::endl;
        return 1;
    }
    
    // Setup editor callbacks
    SetupEditorCallbacks();
    
    // Initialize modular architecture (Phase 1-4)
    g_app.initModularArchitecture();
    
    // Log startup
    g_app.editorState.consoleLogs.push_back("[INFO] LUMA Studio started");
    g_app.editorState.consoleLogs.push_back("[INFO] Press F1 for keyboard shortcuts");
    g_app.editorState.consoleLogs.push_back("[INFO] Tab键切换编辑模式，右键添加对象");
    
    // Initialize mode system - start with Welcome screen
    g_app.modeManager.currentMode = luma::editor::EditorMode::Welcome;
    g_app.welcomeScreen.isVisible = true;
    
    // Don't create default cube anymore - user will add objects via welcome screen
    // Keep scene empty until user makes a choice
    
    ShowWindow(g_app.hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_app.hwnd);
    
    std::cout << "[luma] Ready" << std::endl;
    
    // Main loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (!g_app.shouldQuit) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_app.shouldQuit = true;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        if (g_app.shouldQuit) break;
        
        // Handle resize
        if (g_app.needResize && g_app.width > 0 && g_app.height > 0) {
            g_app.renderer.resize(g_app.width, g_app.height);
            g_app.needResize = false;
        }
        
        // Handle pending model load
        if (!g_app.pendingModelPath.empty()) {
            std::string filename = g_app.pendingModelPath;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }
            
            luma::Entity* newEntity = g_app.scene.createEntity(filename);
            
            // Load model with source data (skeleton, animations) - single load, no duplication
            luma::Model sourceModel;
            if (g_app.renderer.loadModelAsync(g_app.pendingModelPath, newEntity->model, sourceModel)) {
                newEntity->hasModel = true;
                newEntity->model.debugName = g_app.pendingModelPath;
                
                // Sync material from model's first mesh to entity material
                if (!newEntity->model.meshes.empty()) {
                    const auto& firstMesh = newEntity->model.meshes[0];
                    if (!newEntity->material) {
                        newEntity->material = std::make_shared<luma::Material>();
                    }
                    newEntity->material->baseColor = {firstMesh.baseColor[0], firstMesh.baseColor[1], firstMesh.baseColor[2]};
                    newEntity->material->metallic = firstMesh.metallic;
                    newEntity->material->roughness = firstMesh.roughness;
                }
                
                // Transfer skeleton and animations if present
                if (sourceModel.skeleton) {
                    newEntity->skeleton = std::move(sourceModel.skeleton);
                    for (auto& [name, clip] : sourceModel.animations) {
                        auto clipCopy = std::make_unique<luma::AnimationClip>(*clip);
                        newEntity->animationClips[name] = std::move(clipCopy);
                    }
                    newEntity->setupAnimator();
                    
                    // Update UI animation state
                    g_app.animation.clips.clear();
                    for (const auto& [name, clip] : newEntity->animationClips) {
                        g_app.animation.clips.push_back(name);
                        g_app.animation.duration = std::max(g_app.animation.duration, clip->duration);
                    }
                    g_app.animation.currentClip = g_app.animation.clips.empty() ? "" : g_app.animation.clips[0];
                    g_app.animation.time = 0.0f;
                    
                    g_app.editorState.consoleLogs.push_back("[INFO] Loaded with animations: " + filename);
                } else {
                    g_app.editorState.consoleLogs.push_back("[INFO] Loaded: " + filename);
                }
                
                g_app.scene.setSelectedEntity(newEntity);
            } else {
                g_app.scene.destroyEntity(newEntity);
                g_app.editorState.consoleLogs.push_back("[ERROR] Failed to load: " + filename);
            }
            
            g_app.pendingModelPath.clear();
            g_app.viewport.camera.reset();
        }
        
        // Process async texture uploads
        g_app.renderer.processAsyncTextures();
        
        // Check shader hot-reload
        g_app.renderer.checkShaderReload();
        
        // Update
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        g_app.totalTime += dt;
        g_app.viewport.update(dt);
        
        // Update animation
        if (g_app.animation.playing) {
            g_app.animation.time += dt * g_app.animation.speed;
            if (g_app.animation.time > g_app.animation.duration) {
                if (g_app.animation.loop) {
                    g_app.animation.time = fmod(g_app.animation.time, g_app.animation.duration);
                } else {
                    g_app.animation.time = g_app.animation.duration;
                    g_app.animation.playing = false;
                }
            }
        }
        
        // Update animators for all animated entities
        g_app.scene.traverseRenderables([&](luma::Entity* entity) {
            if (entity->animator) {
                // Sync clip selection from UI
                if (!g_app.animation.clips.empty() && !g_app.animation.currentClip.empty()) {
                    if (entity->animator->getCurrentClipName() != g_app.animation.currentClip) {
                        entity->animator->play(g_app.animation.currentClip, 0.2f);
                        entity->animator->setLooping(g_app.animation.loop);
                    }
                }
                
                // Sync play/pause state
                if (g_app.animation.playing) {
                    entity->animator->update(dt * g_app.animation.speed);
                }
                
                // Sync time from scrubber (when not playing)
                if (!g_app.animation.playing) {
                    entity->animator->setTime(g_app.animation.time);
                } else {
                    g_app.animation.time = entity->animator->getCurrentTime();
                }
            }
        });
        
        // Apply post-process settings
        g_app.renderer.setPostProcessEnabled(
            g_app.postProcess.bloom.enabled ||
            g_app.postProcess.toneMapping.enabled ||
            g_app.postProcess.vignette.enabled ||
            g_app.postProcess.fxaa.enabled
        );
        
        // Fill and send post-process constants (for future use when HDR pipeline is implemented)
        luma::PostProcessConstants ppConstants;
        luma::fillPostProcessConstants(ppConstants, g_app.postProcess, 
                                       g_app.width, g_app.height, g_app.totalTime);
        g_app.renderer.setPostProcessParams(&ppConstants, sizeof(ppConstants));
        
        // Render 3D scene (shadow, main pass, overlays, gizmo)
        g_app.renderer.beginFrame();
        g_app.renderScene();
        
        // Render UI (now renders directly to swapchain, after post-processing)
        RenderUI();
        auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(g_app.renderer.getNativeCommandEncoder());
        
        // Switch to ImGui's dedicated descriptor heap
        ID3D12DescriptorHeap* heaps[] = { g_imguiSrvHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);
        
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
        
        g_app.renderer.endFrame();
    }
    
    // Cleanup
    g_app.renderer.waitForGPU();
    g_imguiInitialized = false;
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    DestroyWindow(g_app.hwnd);
    UnregisterClassW(L"LumaStudioClass", wc.hInstance);
    
    std::cout << "[luma] Shutdown complete" << std::endl;
    return 0;
}
