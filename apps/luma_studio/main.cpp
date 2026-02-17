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

// Mode Handler Architecture (Phase 5)
#include "engine/editor/mode_handler.h"
#include "engine/editor/edit_mode_handler.h"
#include "engine/editor/scene_mode_handler.h"
#include "engine/editor/input_system.h"

// Material Node Editor (Phase 6)
#include "engine/editor/material_edit/material_node_editor.h"
#include "engine/editor/material_edit/material_preview.h"

// Character Mode (Phase 7 - Character Creation & Face Sculpting)
#include "engine/editor/character_mode_handler.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declaration of helper function (defined after Application)
luma::Ray getMouseRay(float mouseX, float mouseY);

// Forward declaration of ImGui state (used in InputSystem init)
static bool g_imguiInitialized = false;

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
    
    // Mode Handler Architecture (Phase 5)
    luma::editor::ModeHandlerManager modeHandlers;     // 模式处理器管理器
    luma::editor::EditModeHandler* editModeHandler = nullptr;   // Edit 模式处理器指针
    luma::editor::SceneModeHandler* sceneModeHandler = nullptr; // Scene 模式处理器指针
    luma::editor::CharacterModeHandler* characterModeHandler = nullptr; // Character 模式处理器指针
    luma::editor::InputSystem inputSystem;             // 统一输入系统
    bool inputSystemInitialized = false;               // InputSystem 初始化标志
    
    // Material Node Editor (Phase 6)
    luma::MaterialNodeEditorState materialNodeEditorState;
    luma::MaterialPreviewState materialPreviewState;
    
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
    
    // (Deferred edit mode actions are managed by EditModeHandler::processPendingActions)
    
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
        
        // Initialize Mode Handler Architecture (Phase 5)
        initModeHandlers();
    }
    
    // Initialize Mode Handlers (Phase 5)
    void initModeHandlers() {
        // Setup shared context
        luma::editor::ModeHandlerContext ctx;
        ctx.renderer = &renderer;
        ctx.scene = &scene;
        ctx.viewport = &viewport;
        ctx.gizmo = &gizmo;
        ctx.drawManager = &drawManager;
        ctx.editPipeline = &editPipeline;
        ctx.nativeWindowHandle = hwnd;
        ctx.windowWidth = width;
        ctx.windowHeight = height;
        modeHandlers.setContext(ctx);
        
        // Create and register Scene Mode Handler
        auto sceneHandler = std::make_unique<luma::editor::SceneModeHandler>();
        sceneHandler->init(&modeHandlers.getContext());
        
        // Setup ray callback for Scene mode
        sceneHandler->setRayCallback([](float sx, float sy) -> luma::Ray {
            return ::getMouseRay(sx, sy);
        });
        
        // Setup screen scale callback for gizmo
        sceneHandler->setScreenScaleCallback([this](const luma::Vec3& pos) -> float {
            float cameraEye[3], cameraTgt[3];
            float sceneRadius = getSceneRadius();
            float sceneCenter[3];
            getSceneCenter(sceneCenter);
            viewport.camera.getEyeAndTarget(sceneCenter, sceneRadius, cameraEye, cameraTgt);
            luma::Vec3 cameraPos(cameraEye[0], cameraEye[1], cameraEye[2]);
            return luma::TransformGizmo::calculateScreenScale(
                pos, cameraPos, 100.0f, (float)height, 3.14159f / 4.0f);
        });
        
        sceneModeHandler = sceneHandler.get();
        modeHandlers.registerHandler(std::move(sceneHandler));
        
        // Create and register Edit Mode Handler
        auto editHandler = std::make_unique<luma::editor::EditModeHandler>();
        editHandler->init(&modeHandlers.getContext());
        
        // Setup projection callback for Edit mode
        editHandler->setProjectionCallback([this](float wx, float wy, float wz, float& sx, float& sy) -> bool {
            float viewMatrix[16], projMatrix[16];
            renderer.getViewMatrix(viewMatrix);
            renderer.getProjectionMatrix(projMatrix);
            
            float viewPos[4];
            viewPos[0] = viewMatrix[0]*wx + viewMatrix[4]*wy + viewMatrix[8]*wz + viewMatrix[12];
            viewPos[1] = viewMatrix[1]*wx + viewMatrix[5]*wy + viewMatrix[9]*wz + viewMatrix[13];
            viewPos[2] = viewMatrix[2]*wx + viewMatrix[6]*wy + viewMatrix[10]*wz + viewMatrix[14];
            viewPos[3] = viewMatrix[3]*wx + viewMatrix[7]*wy + viewMatrix[11]*wz + viewMatrix[15];
            
            float clipPos[4];
            clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
            clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
            clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
            clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
            
            if (clipPos[3] <= 0.0f) return false;
            sx = (clipPos[0] / clipPos[3] + 1.0f) * 0.5f * width;
            sy = (1.0f - clipPos[1] / clipPos[3]) * 0.5f * height;
            return true;
        });
        
        // Setup ray callback for Edit mode
        editHandler->setRayCallback([](float sx, float sy) -> luma::Ray {
            return ::getMouseRay(sx, sy);
        });
        
        // Setup mesh changed callback for GPU mesh rebuild
        editHandler->setMeshChangedCallback([this]() {
            editMeshDirty = true;
        });

        editModeHandler = editHandler.get();
        modeHandlers.registerHandler(std::move(editHandler));
        
        // Create and register Character Mode Handler
        auto charHandler = std::make_unique<luma::editor::CharacterModeHandler>();
        charHandler->init(&modeHandlers.getContext());
        
        // Setup projection callback for Character mode
        charHandler->setProjectionCallback([this](float wx, float wy, float wz, float& sx, float& sy) -> bool {
            float viewMatrix[16], projMatrix[16];
            renderer.getViewMatrix(viewMatrix);
            renderer.getProjectionMatrix(projMatrix);
            
            float viewPos[4];
            viewPos[0] = viewMatrix[0]*wx + viewMatrix[4]*wy + viewMatrix[8]*wz + viewMatrix[12];
            viewPos[1] = viewMatrix[1]*wx + viewMatrix[5]*wy + viewMatrix[9]*wz + viewMatrix[13];
            viewPos[2] = viewMatrix[2]*wx + viewMatrix[6]*wy + viewMatrix[10]*wz + viewMatrix[14];
            viewPos[3] = viewMatrix[3]*wx + viewMatrix[7]*wy + viewMatrix[11]*wz + viewMatrix[15];
            
            float clipPos[4];
            clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
            clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
            clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
            clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
            
            if (clipPos[3] <= 0.0f) return false;
            sx = (clipPos[0] / clipPos[3] + 1.0f) * 0.5f * width;
            sy = (1.0f - clipPos[1] / clipPos[3]) * 0.5f * height;
            return true;
        });
        
        charHandler->setRayCallback([](float sx, float sy) -> luma::Ray {
            return ::getMouseRay(sx, sy);
        });
        
        charHandler->setCharacterChangedCallback([this]() {
            editorState.consoleLogs.push_back("[INFO] Character updated");
        });
        
        characterModeHandler = charHandler.get();
        modeHandlers.registerHandler(std::move(charHandler));
        
        // Start in Scene mode (or Welcome if no scene)
        modeHandlers.switchMode(luma::editor::EditorMode::Scene);
        
        // Initialize Input System
        initInputSystem();
        
        std::cout << "[luma] Mode handlers initialized" << std::endl;
    }
    
    // Initialize Input System (Phase 6 - centralized input handling)
    void initInputSystem() {
        luma::editor::InputSystemContext ctx;
        ctx.modeHandlers = &modeHandlers;
        ctx.scene = &scene;
        ctx.gizmo = &gizmo;
        ctx.viewport = &viewport;
        ctx.hwnd = hwnd;
        ctx.windowWidth = &width;
        ctx.windowHeight = &height;
        ctx.needResize = &needResize;
        ctx.shouldQuit = &shouldQuit;
        ctx.mouseDownX = &mouseDownX;
        ctx.mouseDownY = &mouseDownY;
        ctx.mouseWasDown = &mouseWasDown;
        ctx.imguiInitialized = &g_imguiInitialized;
        ctx.welcomeScreenVisible = &welcomeScreen.isVisible;
        
        // Setup callbacks
        ctx.getMouseRay = [](float x, float y) { return ::getMouseRay(x, y); };
        ctx.getSceneRadius = [this]() { return getSceneRadius(); };
        ctx.openContextMenu = [this](float x, float y) { addObjectMenu.openAt(x, y); };
        ctx.onModeSwitch = [this](luma::editor::EditorMode mode) {
            modeManager.switchMode(mode);
        };
        
        inputSystem.init(ctx);
        inputSystemInitialized = true;
    }
    
    // Note: getMouseRay is a global function, use it via ::getMouseRay()
    
    // Sync EditMesh to all modules
    void syncEditMeshToModules() {
        if (!currentEditMesh) return;
        selectionSystem.setMesh(currentEditMesh.get());
        if (meshEditModule) meshEditModule->setEditMesh(currentEditMesh.get());
        if (uvEditModule) uvEditModule->setEditMesh(currentEditMesh.get());
        if (materialEditModule) materialEditModule->setEditMesh(currentEditMesh.get());
        if (editModeHandler) editModeHandler->setEditMesh(currentEditMesh.get());
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
            // Normal scene rendering (includes character entities)
            scene.traverseRenderables([this](luma::Entity* entity) {
                renderer.renderModel(entity->model, entity->worldMatrix.m);
            });
        }
        
        // Character mode: update and render character mesh in real-time
        if (modeManager.currentMode == luma::editor::EditorMode::Character && characterModeHandler) {
            // The character mesh is already part of the scene as an entity,
            // so it gets rendered above. We just need to ensure the mode handler updates.
        }
        
        // === Mode-specific rendering (Gizmos, overlays, etc.) ===
        // Delegate to the current mode handler
        luma::RenderContext renderCtx;
        renderCtx.renderer = &renderer;
        modeHandlers.render(renderCtx);
        
        // Finish 3D scene (applies post-processing)
        renderer.finishSceneRendering();
    }
};

static Application g_app;

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
// Extremely minimal - only handles window events, all input goes to InputSystem
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // ImGui gets first chance
    if (g_imguiInitialized && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    
    // Window-level events
    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_app.width = LOWORD(lParam);
            g_app.height = HIWORD(lParam);
            g_app.needResize = true;
        }
        return 0;
    case WM_DESTROY:
        g_app.shouldQuit = true;
        PostQuitMessage(0);
        return 0;
    }
    
    // InputSystem handles input only after initialization
    // Before init, fall back to DefWindowProc (important for WM_CREATE etc.)
    if (g_app.inputSystemInitialized) {
        return g_app.inputSystem.handleWindowMessage(msg, wParam, lParam);
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
        g_app.modeHandlers.switchMode(luma::editor::EditorMode::Scene);
        g_app.projectName = "未命名场景";
        g_app.currentScenePath.clear();
        g_app.editorState.consoleLogs.push_back("[INFO] 新建场景");
    };
    
    g_app.welcomeScreen.onOpenProject = []() {
        std::string loadPath = OpenFileDialog("LUMA Scene (*.luma)\0*.luma\0All Files (*.*)\0*.*\0");
        if (!loadPath.empty()) {
            g_app.editorState.onSceneLoad(loadPath);
            g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
            g_app.modeHandlers.switchMode(luma::editor::EditorMode::Scene);
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
        g_app.modeHandlers.switchMode(luma::editor::EditorMode::Scene);
    };
    
    g_app.welcomeScreen.onLoadPreset = [](const std::string& preset) {
        g_app.scene.clear();
        g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
        g_app.modeHandlers.switchMode(luma::editor::EditorMode::Scene);
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
        // Switch to Character mode with creation UI
        if (g_app.characterModeHandler) {
            g_app.modeManager.switchMode(luma::editor::EditorMode::Character);
            g_app.modeHandlers.switchMode(luma::editor::EditorMode::Character);
            g_app.emptySceneGuide.isVisible = false;
            g_app.editorState.consoleLogs.push_back("[INFO] 进入角色创建模式");
        }
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
    
    g_app.addObjectMenu.menu.onCreateCharacter = []() {
        // Create character and switch to Character mode
        if (g_app.characterModeHandler) {
            g_app.characterModeHandler->createFromTemplate("Human", luma::CharacterStyle::Realistic);
            g_app.modeManager.switchMode(luma::editor::EditorMode::Character);
            g_app.modeHandlers.switchMode(luma::editor::EditorMode::Character);
            g_app.emptySceneGuide.isVisible = false;
            g_app.editorState.consoleLogs.push_back("[INFO] 角色已创建，进入角色编辑模式");
        }
    };
    
    g_app.addObjectMenu.menu.onCreateLight = [](const std::string& type) {
        // TODO: Implement light creation
        g_app.editorState.consoleLogs.push_back("[INFO] 创建光源: " + type);
    };
    
    // Mode change callback
    g_app.modeManager.onModeChanged = [](luma::editor::EditorMode mode) {
        std::string modeName = luma::editor::EditorModeManager::getModeName(mode);
        g_app.editorState.consoleLogs.push_back("[INFO] 切换模式: " + std::string(modeName));
        
        // Handle Character mode entry
        if (mode == luma::editor::EditorMode::Character) {
            // Character mode entered - handler's onEnter() handles initialization
            g_app.editorState.consoleLogs.push_back("[INFO] Entered character editing mode");
        }
        
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
                    
                    // Sync to all modules including EditModeHandler
                    g_app.syncEditMeshToModules();
                    
                    // Save baseline (for Cancel to restore original state)
                    if (g_app.editModeHandler) {
                        g_app.editModeHandler->setSelectedMeshIndex(meshIdx);
                        g_app.editModeHandler->saveBaseline();
                    }
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
                    g_app.modeHandlers.switchMode(mode);
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
                
                if (!enabled && ImGui::IsItemHovered() && tooltipKey) {
                    ImGui::SetTooltip("%s", loc(tooltipKey));
                }
                
                ImGui::SameLine(0, 4);
            };
            
            drawModeBtn(EditorMode::Scene, "Scene", true);
            // Character mode: always available (can create characters from scratch)
            bool charAvail = availability.character || (g_app.characterModeHandler && g_app.characterModeHandler->getCharacter());
            drawModeBtn(EditorMode::Character, "Character", true);
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
                if (g_app.editModeHandler) g_app.editModeHandler->requestSaveAndExit();
            }
            ImGui::Separator();
            
            // ========== Selection Mode & Edit Tools ==========
            if (ImGui::CollapsingHeader(loc("Edit Tools"), ImGuiTreeNodeFlags_DefaultOpen)) {
                g_app.editToolbar.draw(300.0f);
            }
            g_app.editToolbar.handleShortcuts();
            
            // ========== Undo/Redo ==========
            g_app.undoRedoBar.getUndoCount = [&]() { 
                return g_app.currentEditMesh ? g_app.currentEditMesh->getUndoCount() : 0; 
            };
            g_app.undoRedoBar.getRedoCount = [&]() { 
                return g_app.currentEditMesh ? g_app.currentEditMesh->getRedoCount() : 0; 
            };
            g_app.undoRedoBar.onUndo = [&]() {
                if (g_app.editModeHandler) {
                    g_app.editModeHandler->undoMeshEdit();
                }
            };
            g_app.undoRedoBar.onRedo = [&]() {
                if (g_app.editModeHandler) {
                    g_app.editModeHandler->redoMeshEdit();
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
                    
                    // Save baseline for the newly selected mesh
                    if (g_app.editModeHandler) {
                        g_app.editModeHandler->setSelectedMeshIndex(meshIdx);
                        g_app.editModeHandler->saveBaseline();
                    }
                };
                
                g_app.meshListPanel.draw(selectedEntity, 300.0f);
            }
            
            ImGui::Spacing();
            
            // ========== Material editor ==========
            if (ImGui::CollapsingHeader(loc("Material Properties"))) {
                g_app.materialEditor.draw(selectedEntity, g_app.meshListPanel.selectedMeshIndex, 300.0f);
                
                // Handle "Open Node Editor" button from material editor
                if (g_app.materialEditor.showNodeEditor) {
                    g_app.editorState.showMaterialNodeEditor = true;
                    g_app.materialEditor.showNodeEditor = false;
                    
                    // Import current mesh material into node editor
                    int meshIdx = g_app.meshListPanel.selectedMeshIndex;
                    if (selectedEntity && selectedEntity->hasModel && 
                        meshIdx >= 0 && meshIdx < static_cast<int>(selectedEntity->model.meshes.size())) {
                        auto& mesh = selectedEntity->model.meshes[meshIdx];
                        if (!g_app.materialNodeEditorState.graph) {
                            g_app.materialNodeEditorState.init();
                        }
                        luma::importMeshMaterialToGraph(
                            *g_app.materialNodeEditorState.graph,
                            mesh.baseColor, mesh.metallic, mesh.roughness,
                            mesh.hasDiffuseTexture, mesh.diffuseTexturePath,
                            mesh.hasNormalTexture, mesh.normalTexturePath,
                            mesh.hasSpecularTexture, mesh.specularTexturePath
                        );
                        g_app.materialNodeEditorState.reset();
                    }
                }
            }
            
            // ========== Save/Commit/Cancel bar at bottom ==========
            g_app.saveBar.onSave = [&]() {
                if (g_app.editModeHandler) g_app.editModeHandler->requestSaveAndExit();
            };
            g_app.saveBar.onCancel = [&]() {
                if (g_app.editModeHandler) g_app.editModeHandler->requestCancel();
            };
            g_app.saveBar.onCommit = [&]() {
                if (g_app.editModeHandler) g_app.editModeHandler->commitChanges();
            };
            {
                bool hasChanges = g_app.editMeshDirty;
                bool hasUncommitted = g_app.editModeHandler ? g_app.editModeHandler->hasUncommittedChanges() : false;
                g_app.saveBar.draw(hasChanges, hasUncommitted, 300.0f);
            }
        }
        ImGui::End();
        
        // ========== UV Editor Window (floating) ==========
        g_app.uvEditor.draw();
    } else if (g_app.modeManager.currentMode == luma::editor::EditorMode::Character) {
        // Character mode: Show character editing panel
        float rightPanelX = (float)g_app.width - 320.0f;
        float topOffset = luma::ui::EditorLayout::getTopOffset();
        
        ImGui::SetNextWindowPos(ImVec2(rightPanelX, topOffset));
        ImGui::SetNextWindowSize(ImVec2(320.0f, (float)g_app.height - topOffset - 24.0f));
        
        using luma::ui::loc;
        
        if (ImGui::Begin(loc("Character Mode - Inspector"), nullptr, 
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            if (g_app.characterModeHandler) {
                g_app.characterModeHandler->renderUI();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Exit button
            if (ImGui::Button(loc("Back to Scene"), ImVec2(-1, 28))) {
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
                g_app.modeHandlers.switchMode(luma::editor::EditorMode::Scene);
            }
        }
        ImGui::End();
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
    
    // ========== Material Node Editor ==========
    luma::drawMaterialNodeEditor(g_app.materialNodeEditorState, g_app.editorState.showMaterialNodeEditor);
    luma::drawMaterialPreviewPanel(g_app.materialPreviewState, g_app.materialNodeEditorState, g_app.editorState.showMaterialPreview);
    
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
        g_app.viewportHeader.xRayMode = &g_app.editToolbar.xRayMode;
        g_app.viewportHeader.onUndo = [&]() {
            if (g_app.editModeHandler) {
                g_app.editModeHandler->undoMeshEdit();
            }
        };
        g_app.viewportHeader.onRedo = [&]() {
            if (g_app.editModeHandler) {
                g_app.editModeHandler->redoMeshEdit();
            }
        };
        
        g_app.viewportHeader.draw(viewportX, viewportY, viewportW);
    }
    
    // ========== Add Object Context Menu ==========
    g_app.addObjectMenu.draw();
    
    // ========== Selection Box Overlay (框选/圆选) ==========
    g_app.selectionBox.draw();
    
    // ========== Mode-specific UI (selection highlights, etc.) ==========
    // Delegated to EditModeHandler::renderUI() which handles face/edge/vertex highlighting
    if (g_app.editModeHandler) {
        g_app.editModeHandler->renderUI();
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
        // Sync toolbar → handler (all logic lives in syncFromToolbar)
        if (g_app.editModeHandler && g_app.modeManager.currentMode == luma::editor::EditorMode::Edit) {
            g_app.editModeHandler->syncFromToolbar(
                g_app.editToolbar,
                g_app.viewToolbar.currentViewMode,
                g_app.meshListPanel.selectedMeshIndex
            );
        }
        
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
        
        // ===== Deferred edit mode actions =====
        // EditModeHandler owns the pending action state. We just call processPendingActions()
        // at the start of each frame (before draw commands) and handle mode switching if needed.
        if (g_app.editModeHandler && g_app.modeManager.currentMode == luma::editor::EditorMode::Edit) {
            auto action = g_app.editModeHandler->processPendingActions();
            if (action != luma::editor::EditModeAction::None) {
                // GPU is safe now (no in-flight draw calls referencing old buffers)
                g_app.modeManager.switchMode(luma::editor::EditorMode::Scene);
                g_app.modeHandlers.switchMode(luma::editor::EditorMode::Scene);
                g_app.currentEditMesh.reset();
                g_app.editMeshDirty = false;
            }
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
        
        // Update mode handler context and state
        {
            auto& ctx = g_app.modeHandlers.getContext();
            ctx.windowWidth = g_app.width;
            ctx.windowHeight = g_app.height;
            g_app.renderer.getViewMatrix(ctx.viewMatrix);
            g_app.renderer.getProjectionMatrix(ctx.projMatrix);
            g_app.getSceneCenter(ctx.sceneCenter);
            ctx.sceneRadius = g_app.getSceneRadius();
            
            // Update camera position and FOV for gizmo rendering
            float cameraEye[3], cameraTgt[3];
            g_app.viewport.camera.getEyeAndTarget(ctx.sceneCenter, ctx.sceneRadius, cameraEye, cameraTgt);
            ctx.cameraPos[0] = cameraEye[0];
            ctx.cameraPos[1] = cameraEye[1];
            ctx.cameraPos[2] = cameraEye[2];
            ctx.fovY = 3.14159f / 4.0f;  // 45 degrees
            
            // Update mode handlers
            g_app.modeHandlers.update(dt);
            
            // Sync selection box state to overlay for UI rendering
            // Note: MeshEditModule handles selection, not EditModeHandler directly
            if (g_app.editModeHandler && g_app.modeManager.currentMode == luma::editor::EditorMode::Edit) {
                if (auto* meshEdit = g_app.editModeHandler->getMeshEditModule()) {
                    g_app.selectionBox.isSelecting = meshEdit->isSelecting();
                    if (meshEdit->isSelecting()) {
                        float x1, y1, x2, y2;
                        meshEdit->getSelectionBounds(x1, y1, x2, y2);
                        g_app.selectionBox.startPos = ImVec2(x1, y1);
                        g_app.selectionBox.currentPos = ImVec2(x2, y2);
                        g_app.selectionBox.selectTool = g_app.editToolbar.selectTool;
                    }
                }
            }
        }
        
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
