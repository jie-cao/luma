// LUMA Creator - ImGui Edition
// Application entry point

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
#include "engine/renderer/pbr_renderer.h"
#include "engine/viewport/viewport.h"
#include "engine/ui/panels.h"
#include "engine/asset/model_loader.h"

using Microsoft::WRL::ComPtr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ===== Application State =====
struct Application {
    // Core systems
    luma::PBRRenderer renderer;
    luma::Viewport viewport;
    luma::LoadedModel model;
    
    // Window
    HWND hwnd = nullptr;
    int width = 1280;
    int height = 720;
    
    // State
    bool shouldQuit = false;
    bool needResize = false;
    bool showHelp = false;
    std::string pendingModelPath;
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
        
    case WM_KEYDOWN:
        g_app.viewport.onKeyDown((int)wParam);
        if (wParam == VK_F1) g_app.showHelp = !g_app.showHelp;
        return 0;
        
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
            g_app.viewport.onMouseMove((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), g_app.model.radius);
        }
        return 0;
        
    case WM_MOUSEWHEEL:
        if (!imguiWantsMouse) {
            float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            g_app.viewport.onMouseWheel(delta, g_app.model.radius);
        }
        return 0;
        
    case WM_DESTROY:
        g_app.shouldQuit = true;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ===== File Dialog =====
static std::string OpenFileDialog() {
    char path[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_app.hwnd;
    ofn.lpstrFilter = luma::get_file_filter();
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return path;
    return "";
}

// ===== ImGui Initialization =====
static bool InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    luma::ui::applyDarkTheme();
    
    auto* device = static_cast<ID3D12Device*>(g_app.renderer.getDevice());
    auto* srvHeap = static_cast<ID3D12DescriptorHeap*>(g_app.renderer.getSrvHeap());
    
    ImGui_ImplWin32_Init(g_app.hwnd);
    ImGui_ImplDX12_Init(device, 2, DXGI_FORMAT_R8G8B8A8_UNORM, srvHeap,
        srvHeap->GetCPUDescriptorHandleForHeapStart(),
        srvHeap->GetGPUDescriptorHandleForHeapStart());
    ImGui_ImplDX12_CreateDeviceObjects();
    
    g_imguiInitialized = true;
    std::cout << "[luma] ImGui initialized" << std::endl;
    return true;
}

// ===== Render UI =====
static void RenderUI() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Menu bar
    std::string menuAction = luma::ui::drawMenuBar(g_app.viewport, g_app.shouldQuit, g_app.showHelp);
    if (menuAction == "__OPEN_DIALOG__") {
        std::string path = OpenFileDialog();
        if (!path.empty()) g_app.pendingModelPath = path;
    }
    
    // Panels
    if (luma::ui::drawModelPanel(g_app.model)) {
        std::string path = OpenFileDialog();
        if (!path.empty()) g_app.pendingModelPath = path;
    }
    
    luma::ui::drawCameraPanel(g_app.viewport);
    luma::ui::drawHelpOverlay(g_app.showHelp, g_app.width, g_app.height);
    luma::ui::drawOrientationGizmo(g_app.viewport.camera, g_app.width, g_app.height);
    luma::ui::drawStatusBar(g_app.width, g_app.height);
    
    ImGui::Render();
}

// ===== Main Entry =====
int main() {
    std::cout << "[luma] LUMA Creator starting..." << std::endl;
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LumaCreatorClass";
    RegisterClassExW(&wc);
    
    // Create window
    RECT rc = {0, 0, g_app.width, g_app.height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_app.hwnd = CreateWindowExW(0, L"LumaCreatorClass", L"LUMA Creator",
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
    
    // Initialize ImGui
    if (!InitImGui()) {
        std::cerr << "[luma] Failed to initialize ImGui" << std::endl;
        return 1;
    }
    
    // Create default cube
    luma::Mesh cube = luma::create_cube();
    luma::LoadedModel defaultModel;
    defaultModel.meshes.push_back(g_app.renderer.uploadMesh(cube));
    defaultModel.center[0] = defaultModel.center[1] = defaultModel.center[2] = 0.0f;
    defaultModel.radius = 1.0f;
    defaultModel.name = "Default Cube";
    g_app.model = std::move(defaultModel);
    
    ShowWindow(g_app.hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_app.hwnd);
    
    std::cout << "[luma] Ready - Press F1 for help" << std::endl;
    
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
            g_app.renderer.loadModel(g_app.pendingModelPath, g_app.model);
            g_app.pendingModelPath.clear();
            g_app.viewport.camera.reset();
        }
        
        // Update
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        g_app.viewport.update(dt);
        
        // Render
        g_app.renderer.beginFrame();
        g_app.viewport.render(g_app.renderer, g_app.model);
        
        RenderUI();
        auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(g_app.renderer.getCommandList());
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
    UnregisterClassW(L"LumaCreatorClass", wc.hInstance);
    
    std::cout << "[luma] Shutdown complete" << std::endl;
    return 0;
}
