// LUMA Input System
// Centralized input handling - removes all WM_* case statements from main.cpp
// 
// This module handles:
// - Global shortcuts (Undo/Redo, Tab, Escape, F1, F5)
// - Gizmo interaction
// - Mode handler delegation
// - Viewport/camera controls as fallback

#pragma once

#include "engine/editor/edit_module.h"
#include "engine/editor/mode_handler.h"
#include "engine/editor/edit_mode_handler.h"
#include "engine/editor/gizmo.h"
#include "engine/editor/command.h"
#include "engine/editor/commands/scene_commands.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/picking.h"
#include "engine/viewport/viewport.h"
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#endif

namespace luma {
namespace editor {

// Input system context - all dependencies needed for input processing
struct InputSystemContext {
    // Core systems
    ModeHandlerManager* modeHandlers = nullptr;
    SceneGraph* scene = nullptr;
    luma::TransformGizmo* gizmo = nullptr;
    luma::Viewport* viewport = nullptr;
    
    // Window state
    HWND hwnd = nullptr;
    int* windowWidth = nullptr;
    int* windowHeight = nullptr;
    bool* needResize = nullptr;
    bool* shouldQuit = nullptr;
    
    // Mouse tracking
    float* mouseDownX = nullptr;
    float* mouseDownY = nullptr;
    bool* mouseWasDown = nullptr;
    
    // UI state
    bool* imguiInitialized = nullptr;
    bool* welcomeScreenVisible = nullptr;
    
    // Callbacks - use luma::Ray for gizmo compatibility
    std::function<luma::Ray(float, float)> getMouseRay;
    std::function<float()> getSceneRadius;
    std::function<void(float, float)> openContextMenu;
    
    // Mode switching callback (to sync with EditorModeManager)
    std::function<void(EditorMode)> onModeSwitch;
};

// Centralized input system
class InputSystem {
public:
    InputSystem() = default;
    
    void init(const InputSystemContext& ctx) { m_ctx = ctx; }
    
    // Main entry point - handles all Windows messages related to input
    // Returns the appropriate LRESULT
    LRESULT handleWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (!m_ctx.modeHandlers) return DefWindowProcW(m_ctx.hwnd, msg, wParam, lParam);
        
        // Check ImGui state
        bool imguiWantsMouse = *m_ctx.imguiInitialized && ImGui::GetIO().WantCaptureMouse;
        bool imguiWantsKeyboard = *m_ctx.imguiInitialized && ImGui::GetIO().WantCaptureKeyboard;
        
        // Create InputEvent for mode delegation
        InputEvent event = createInputEvent(msg, wParam, lParam);
        
        // ==================== KEYBOARD ====================
        if (msg == WM_KEYDOWN && !imguiWantsKeyboard) {
            return handleKeyDown(wParam, event);
        }
        
        // ==================== MOUSE ====================
        if (imguiWantsMouse) {
            return DefWindowProcW(m_ctx.hwnd, msg, wParam, lParam);
        }
        
        float mouseX = (float)GET_X_LPARAM(lParam);
        float mouseY = (float)GET_Y_LPARAM(lParam);
        bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
        
        switch (msg) {
        case WM_LBUTTONDOWN: return handleMouseDown(0, mouseX, mouseY, altPressed, event);
        case WM_RBUTTONDOWN: return handleMouseDown(1, mouseX, mouseY, altPressed, event);
        case WM_MBUTTONDOWN: return handleMouseDown(2, mouseX, mouseY, altPressed, event);
        case WM_LBUTTONUP:   return handleMouseUp(0, event);
        case WM_RBUTTONUP:   return handleMouseUp(1, event);
        case WM_MBUTTONUP:   return handleMouseUp(2, event);
        case WM_MOUSEMOVE:   return handleMouseMove(mouseX, mouseY, event);
        case WM_MOUSEWHEEL:  return handleMouseWheel(wParam, event);
        }
        
        return DefWindowProcW(m_ctx.hwnd, msg, wParam, lParam);
    }
    
private:
    InputSystemContext m_ctx;
    
    // ==================== Input Event Creation ====================
    InputEvent createInputEvent(UINT msg, WPARAM wParam, LPARAM lParam) {
        InputEvent event;
        event.modifiers = 0;
        if (GetKeyState(VK_CONTROL) & 0x8000) event.modifiers |= LUMA_MOD_CTRL;
        if (GetKeyState(VK_SHIFT) & 0x8000)   event.modifiers |= LUMA_MOD_SHIFT;
        if (GetKeyState(VK_MENU) & 0x8000)    event.modifiers |= LUMA_MOD_ALT;
        
        event.mouseX = (float)GET_X_LPARAM(lParam);
        event.mouseY = (float)GET_Y_LPARAM(lParam);
        
        switch (msg) {
        case WM_KEYDOWN: case WM_KEYUP:
            event.type = (msg == WM_KEYDOWN) ? InputEvent::Type::KeyDown : InputEvent::Type::KeyUp;
            event.key = (int)wParam;
            break;
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
            event.type = InputEvent::Type::MouseDown;
            event.key = (msg == WM_LBUTTONDOWN) ? 0 : (msg == WM_RBUTTONDOWN) ? 1 : 2;
            break;
        case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
            event.type = InputEvent::Type::MouseUp;
            event.key = (msg == WM_LBUTTONUP) ? 0 : (msg == WM_RBUTTONUP) ? 1 : 2;
            break;
        case WM_MOUSEMOVE:
            event.type = InputEvent::Type::MouseMove;
            break;
        case WM_MOUSEWHEEL:
            event.type = InputEvent::Type::MouseWheel;
            event.wheelDelta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            break;
        default:
            event.type = InputEvent::Type::None;
        }
        return event;
    }
    
    // ==================== Keyboard Handling ====================
    LRESULT handleKeyDown(WPARAM wParam, const InputEvent& event) {
        bool ctrl = event.isCtrl();
        bool shift = event.isShift();
        
        // Global shortcuts — Undo/Redo
        // In Edit mode, route to EditMesh's internal undo system
        // In other modes, use scene-level command history
        if (ctrl && wParam == 'Z') {
            if (m_ctx.modeHandlers->getCurrentMode() == EditorMode::Edit) {
                auto* editHandler = dynamic_cast<EditModeHandler*>(
                    m_ctx.modeHandlers->getHandler(EditorMode::Edit));
                if (editHandler) {
                    if (shift) editHandler->redoMeshEdit();
                    else editHandler->undoMeshEdit();
                    return 0;
                }
            }
            shift ? getCommandHistory().redo() : getCommandHistory().undo();
            return 0;
        }
        if (ctrl && wParam == 'Y') {
            if (m_ctx.modeHandlers->getCurrentMode() == EditorMode::Edit) {
                auto* editHandler = dynamic_cast<EditModeHandler*>(
                    m_ctx.modeHandlers->getHandler(EditorMode::Edit));
                if (editHandler) {
                    editHandler->redoMeshEdit();
                    return 0;
                }
            }
            getCommandHistory().redo();
            return 0;
        }
        if (ctrl && wParam == 'D') {
            if (auto* sel = m_ctx.scene->getSelectedEntity()) {
                getCommandHistory().execute(std::make_unique<DuplicateEntityCommand>(m_ctx.scene, sel));
            }
            return 0;
        }
        if (wParam == VK_F1) {
            // Toggle help - handled via callback or state
            return 0;
        }
        if (wParam == VK_TAB) {
            auto currentMode = m_ctx.modeHandlers->getCurrentMode();
            EditorMode newMode = currentMode;
            if (currentMode == EditorMode::Edit) {
                newMode = EditorMode::Scene;
            } else if (m_ctx.scene->getSelectedEntity() && m_ctx.scene->getSelectedEntity()->hasModel) {
                newMode = EditorMode::Edit;
            }
            if (newMode != currentMode) {
                m_ctx.modeHandlers->switchMode(newMode);
                if (m_ctx.onModeSwitch) m_ctx.onModeSwitch(newMode);
            }
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            if (m_ctx.welcomeScreenVisible && *m_ctx.welcomeScreenVisible) {
                *m_ctx.welcomeScreenVisible = false;
            }
            if (m_ctx.modeHandlers->getCurrentMode() != EditorMode::Scene) {
                m_ctx.modeHandlers->switchMode(EditorMode::Scene);
                if (m_ctx.onModeSwitch) m_ctx.onModeSwitch(EditorMode::Scene);
            }
            return 0;
        }
        if (wParam == VK_F5) {
            m_ctx.modeHandlers->switchMode(EditorMode::Play);
            if (m_ctx.onModeSwitch) m_ctx.onModeSwitch(EditorMode::Play);
            return 0;
        }
        
        // Delegate to mode handler
        if (m_ctx.modeHandlers->handleInput(event)) return 0;
        
        // Fallback to viewport
        if (m_ctx.viewport) m_ctx.viewport->onKeyDown((int)wParam);
        return 0;
    }
    
    // ==================== Mouse Handling ====================
    LRESULT handleMouseDown(int button, float mouseX, float mouseY, bool altPressed, const InputEvent& event) {
        if (button == 0) { // Left button
            *m_ctx.mouseDownX = mouseX;
            *m_ctx.mouseDownY = mouseY;
            *m_ctx.mouseWasDown = true;
            
            // Alt+Click = Camera control (highest priority, skip gizmo and mode handlers)
            if (altPressed) {
                *m_ctx.mouseWasDown = false;
                if (m_ctx.viewport) {
                    m_ctx.viewport->onMouseDown(button, mouseX, mouseY, altPressed);
                    SetCapture(m_ctx.hwnd);
                }
                return 0;
            }
            
            // Scene-level gizmo has priority ONLY in Scene mode (not in Edit mode)
            // In Edit mode, the MeshEditGizmo handles its own gizmo via mode handler
            bool isEditMode = m_ctx.modeHandlers->getCurrentMode() == EditorMode::Edit;
            if (!isEditMode && m_ctx.scene->getSelectedEntity() && m_ctx.gizmo && m_ctx.getMouseRay) {
                luma::Ray ray = m_ctx.getMouseRay(mouseX, mouseY);
                luma::Vec3 gizmoPos = m_ctx.scene->getSelectedEntity()->getWorldPosition();
                float screenScale = luma::TransformGizmo::calculateScreenScale(
                    gizmoPos, ray.origin,
                    100.0f, (float)*m_ctx.windowHeight, 3.14159f / 4.0f);
                if (m_ctx.gizmo->beginDrag(ray, screenScale)) {
                    *m_ctx.mouseWasDown = false;
                    SetCapture(m_ctx.hwnd);
                    return 0;
                }
            }
        }
        
        // Alt+Middle/Right = Camera control
        if (altPressed && (button == 1 || button == 2)) {
            if (m_ctx.viewport) {
                m_ctx.viewport->onMouseDown(button, mouseX, mouseY, altPressed);
                SetCapture(m_ctx.hwnd);
            }
            return 0;
        }
        
        // Mode handler (only when not Alt)
        if (m_ctx.modeHandlers->handleInput(event)) {
            SetCapture(m_ctx.hwnd);
            return 0;
        }
        
        // Fallback viewport
        if (m_ctx.viewport) {
            m_ctx.viewport->onMouseDown(button, mouseX, mouseY, altPressed);
        }
        
        // Right click context menu (when not Alt)
        if (button == 1 && !altPressed && m_ctx.openContextMenu) {
            float leftPanel = 280.0f, rightPanel = (float)*m_ctx.windowWidth - 320.0f, topOffset = 87.0f;
            if (mouseX > leftPanel && mouseX < rightPanel && mouseY > topOffset) {
                m_ctx.openContextMenu(mouseX, mouseY);
            }
        }
        
        return 0;
    }
    
    LRESULT handleMouseUp(int button, const InputEvent& event) {
        // Camera mode release has HIGHEST priority.
        // When viewport is orbiting/panning/zooming, the MouseUp MUST reach
        // the viewport to stop the camera movement. This is critical because
        // the user may release Alt before releasing the mouse button, so
        // event.isAlt() can be false even during a camera orbit release.
        if (m_ctx.viewport && m_ctx.viewport->cameraMode != CameraMode::None) {
            m_ctx.viewport->onMouseUp(button);
            if (button == 0) *m_ctx.mouseWasDown = false;
            if (m_ctx.viewport->cameraMode == CameraMode::None) ReleaseCapture();
            return 0;
        }
        
        // Mode handler (MeshEditGizmo endDrag has priority in Edit mode)
        if (m_ctx.modeHandlers->handleInput(event)) {
            if (button == 0) *m_ctx.mouseWasDown = false;
            ReleaseCapture();
            return 0;
        }
        
        if (button == 0) { // Left button
            // End scene-level gizmo drag (only when mode handler didn't consume)
            if (m_ctx.gizmo && m_ctx.gizmo->isDragging()) {
                m_ctx.gizmo->endDrag();
                ReleaseCapture();
                *m_ctx.mouseWasDown = false;
                return 0;
            }
        }
        
        // Fallback
        if (button == 0) *m_ctx.mouseWasDown = false;
        if (m_ctx.viewport) {
            m_ctx.viewport->onMouseUp(button);
            if (m_ctx.viewport->cameraMode == CameraMode::None) ReleaseCapture();
        }
        return 0;
    }
    
    LRESULT handleMouseMove(float mouseX, float mouseY, const InputEvent& event) {
        // Camera mode has highest priority during orbit/pan/zoom
        // (prevents edit mode from intercepting camera movement)
        if (m_ctx.viewport && m_ctx.viewport->cameraMode != CameraMode::None && m_ctx.getSceneRadius) {
            m_ctx.viewport->onMouseMove(mouseX, mouseY, m_ctx.getSceneRadius());
            return 0;
        }
        
        // Mode handler (MeshEditGizmo drag has priority in Edit mode)
        if (m_ctx.modeHandlers->handleInput(event)) return 0;
        
        // Scene-level gizmo drag (only when mode handler didn't consume)
        if (m_ctx.gizmo && m_ctx.gizmo->isDragging() && m_ctx.getMouseRay) {
            luma::Ray ray = m_ctx.getMouseRay(mouseX, mouseY);
            m_ctx.gizmo->updateDrag(ray);
            if (auto* target = m_ctx.scene->getSelectedEntity()) {
                target->updateWorldMatrix();
            }
            return 0;
        }
        
        return 0;
    }
    
    LRESULT handleMouseWheel(WPARAM wParam, const InputEvent& event) {
        // Mode handler
        if (m_ctx.modeHandlers->handleInput(event)) return 0;
        
        // Fallback camera zoom
        if (m_ctx.viewport && m_ctx.getSceneRadius) {
            float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            m_ctx.viewport->onMouseWheel(delta, m_ctx.getSceneRadius());
        }
        return 0;
    }
};

} // namespace editor
} // namespace luma
