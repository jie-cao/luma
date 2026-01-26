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
#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/post_process.h"
#include "engine/viewport/viewport.h"
#include "engine/ui/editor_ui.h"
#include "engine/asset/model_loader.h"
#include "engine/asset/asset_manager.h"
#include "engine/scene/scene_graph.h"
#include "engine/editor/gizmo.h"
#include "engine/editor/command.h"
#include "engine/editor/commands/transform_commands.h"
#include "engine/editor/commands/scene_commands.h"
#include "engine/serialization/scene_serializer.h"
#include "engine/serialization/json.h"

using Microsoft::WRL::ComPtr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ===== Application State =====
struct Application {
    // Core systems
    luma::UnifiedRenderer renderer;
    luma::Viewport viewport;
    luma::SceneGraph scene;
    luma::TransformGizmo gizmo;
    
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
};

static Application g_app;
static bool g_imguiInitialized = false;

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
        }
        g_app.viewport.onKeyDown((int)wParam);
        return 0;
    }
        
    case WM_LBUTTONDOWN:
        if (!imguiWantsMouse) {
            g_app.viewport.onMouseDown(0, (float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), altPressed);
            if (altPressed) SetCapture(hwnd);
        }
        return 0;
        
    case WM_RBUTTONDOWN:
        if (!imguiWantsMouse) {
            g_app.viewport.onMouseDown(1, (float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), altPressed);
            if (altPressed) SetCapture(hwnd);
        }
        return 0;
        
    case WM_MBUTTONDOWN:
        if (!imguiWantsMouse) {
            g_app.viewport.onMouseDown(2, (float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), altPressed);
            if (altPressed) SetCapture(hwnd);
        }
        return 0;
        
    case WM_LBUTTONUP:
        g_app.viewport.onMouseUp(0);
        if (g_app.viewport.cameraMode == luma::CameraMode::None) ReleaseCapture();
        return 0;
        
    case WM_RBUTTONUP:
        g_app.viewport.onMouseUp(1);
        if (g_app.viewport.cameraMode == luma::CameraMode::None) ReleaseCapture();
        return 0;
        
    case WM_MBUTTONUP:
        g_app.viewport.onMouseUp(2);
        if (g_app.viewport.cameraMode == luma::CameraMode::None) ReleaseCapture();
        return 0;
        
    case WM_MOUSEMOVE:
        if (g_app.viewport.cameraMode != luma::CameraMode::None) {
            g_app.viewport.onMouseMove((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), g_app.getSceneRadius());
        }
        return 0;
        
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
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Apply new editor theme
    luma::ui::applyEditorTheme();
    
    auto* device = static_cast<ID3D12Device*>(g_app.renderer.getNativeDevice());
    
    // Create a dedicated SRV heap for ImGui
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_imguiSrvHeap));
    
    ImGui_ImplWin32_Init(g_app.hwnd);
    ImGui_ImplDX12_Init(device, 2, DXGI_FORMAT_R8G8B8A8_UNORM, g_imguiSrvHeap.Get(),
        g_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplDX12_CreateDeviceObjects();
    
    g_imguiInitialized = true;
    std::cout << "[luma] ImGui initialized" << std::endl;
    return true;
}

// ===== Setup Callbacks =====
static void SetupEditorCallbacks() {
    // Model load callback
    g_app.editorState.onModelLoad = [](const std::string& path) {
        g_app.pendingModelPath = path;
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
                g_app.viewport.camera.targetOffset.x = loadedCamera.targetOffsetX;
                g_app.viewport.camera.targetOffset.y = loadedCamera.targetOffsetY;
                g_app.viewport.camera.targetOffset.z = loadedCamera.targetOffsetZ;
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
}

// ===== Render UI =====
static void RenderUI() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Main menu bar
    luma::ui::drawMainMenuBar(g_app.editorState, g_app.viewport, g_app.shouldQuit);
    
    // Toolbar
    luma::ui::drawToolbar(g_app.editorState, g_app.gizmo);
    
    // Left panels
    luma::ui::drawHierarchyPanel(g_app.scene, g_app.editorState);
    
    // Right panels
    luma::ui::drawInspectorPanel(g_app.scene, g_app.editorState);
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
    
    // Log startup
    g_app.editorState.consoleLogs.push_back("[INFO] LUMA Studio started");
    g_app.editorState.consoleLogs.push_back("[INFO] Press F1 for keyboard shortcuts");
    
    // Create default cube as first entity
    {
        luma::Mesh cube = luma::create_cube();
        luma::RHILoadedModel cubeModel;
        cubeModel.meshes.push_back(g_app.renderer.uploadMesh(cube));
        cubeModel.center[0] = cubeModel.center[1] = cubeModel.center[2] = 0.0f;
        cubeModel.radius = 1.0f;
        cubeModel.name = "Default Cube";
        cubeModel.debugName = "primitives/cube";
        
        luma::Entity* cubeEntity = g_app.scene.createEntityWithModel("Cube", cubeModel);
        g_app.scene.setSelectedEntity(cubeEntity);
    }
    
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
            
            // Try to load model with animations first
            auto animModel = luma::load_model_with_animations(g_app.pendingModelPath);
            if (animModel && g_app.renderer.loadModelAsync(g_app.pendingModelPath, newEntity->model)) {
                newEntity->hasModel = true;
                newEntity->model.debugName = g_app.pendingModelPath;
                
                // Transfer skeleton and animations if present
                if (animModel->skeleton) {
                    newEntity->skeleton = std::move(animModel->skeleton);
                    for (auto& [name, clip] : animModel->animations) {
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
        
        // Render
        g_app.renderer.beginFrame();
        
        // Set camera
        auto camParams = g_app.viewport.getCameraParams();
        float sceneRadius = g_app.getSceneRadius();
        g_app.renderer.setCamera(camParams, sceneRadius);
        
        // Shadow pass (if enabled)
        if (g_app.renderSettings.shadowsEnabled) {
            float sceneCenter[3];
            g_app.getSceneCenter(sceneCenter);
            g_app.renderer.beginShadowPass(sceneRadius, sceneCenter);
            g_app.scene.traverseRenderables([&](luma::Entity* entity) {
                g_app.renderer.renderModelShadow(entity->model, entity->worldMatrix.m);
            });
            g_app.renderer.endShadowPass();
        }
        
        // Render grid
        if (g_app.viewport.settings.showGrid) {
            g_app.renderer.renderGrid(camParams, sceneRadius);
        }
        
        // Render all entities
        g_app.scene.traverseRenderables([&](luma::Entity* entity) {
            if (entity->hasSkeleton()) {
                // Render skinned model with bone matrices
                luma::Mat4 boneMatrices[MAX_BONES];
                entity->getSkinningMatrices(boneMatrices);
                g_app.renderer.renderSkinnedModel(entity->model, entity->worldMatrix.m,
                                                   reinterpret_cast<const float*>(boneMatrices));
            } else {
                g_app.renderer.renderModel(entity->model, entity->worldMatrix.m);
            }
        });
        
        // Render selection outline
        if (auto* selected = g_app.scene.getSelectedEntity()) {
            if (selected->hasModel) {
                float outlineColor[4] = {1.0f, 0.6f, 0.2f, 1.0f};
                g_app.renderer.renderModelOutline(selected->model, selected->worldMatrix.m, outlineColor);
            }
            
            // Render gizmo
            float screenScale = sceneRadius * 0.15f;
            g_app.gizmo.setTarget(&selected->localTransform);
            auto gizmoData = g_app.gizmo.generateRenderData(screenScale);
            if (!gizmoData.lines.empty()) {
                g_app.renderer.renderGizmoLines(
                    reinterpret_cast<const float*>(gizmoData.lines.data()),
                    (uint32_t)gizmoData.lines.size()
                );
            }
        }
        
        // Render UI
        RenderUI();
        auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(g_app.renderer.getNativeCommandEncoder());
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
