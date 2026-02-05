// LUMA Input Handler
// Centralized input processing for editor (keyboard, mouse, shortcuts)

#pragma once

#include "engine/editor/edit_module.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

namespace luma {
namespace editor {

// Key codes (matching Windows VK codes for portability)
namespace Key {
    constexpr int Escape = 0x1B;
    constexpr int Enter = 0x0D;
    constexpr int Tab = 0x09;
    constexpr int Backspace = 0x08;
    constexpr int Delete = 0x2E;
    constexpr int Insert = 0x2D;
    constexpr int Home = 0x24;
    constexpr int End = 0x23;
    constexpr int PageUp = 0x21;
    constexpr int PageDown = 0x22;
    constexpr int Left = 0x25;
    constexpr int Up = 0x26;
    constexpr int Right = 0x27;
    constexpr int Down = 0x28;
    constexpr int Space = 0x20;
    constexpr int F1 = 0x70;
    constexpr int F2 = 0x71;
    constexpr int F3 = 0x72;
    constexpr int F4 = 0x73;
    constexpr int F5 = 0x74;
    constexpr int F6 = 0x75;
    constexpr int F7 = 0x76;
    constexpr int F8 = 0x77;
    constexpr int F9 = 0x78;
    constexpr int F10 = 0x79;
    constexpr int F11 = 0x7A;
    constexpr int F12 = 0x7B;
}

// Mouse button codes
namespace Mouse {
    constexpr int Left = 0;
    constexpr int Right = 1;
    constexpr int Middle = 2;
}

// Shortcut binding
struct Shortcut {
    int key;
    int modifiers;  // MOD_CTRL | MOD_SHIFT | MOD_ALT
    std::string action;
    std::function<void()> callback;
    
    bool matches(int k, int mods) const {
        return key == k && modifiers == mods;
    }
};

// Camera control state
struct CameraControlState {
    bool orbiting = false;
    bool panning = false;
    bool dollying = false;
    float lastMouseX = 0;
    float lastMouseY = 0;
};

// Input Handler
class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;
    
    // Process input events
    bool processKeyDown(int key, int modifiers);
    bool processKeyUp(int key, int modifiers);
    bool processMouseDown(int button, float x, float y, int modifiers);
    bool processMouseUp(int button, float x, float y, int modifiers);
    bool processMouseMove(float x, float y, int modifiers);
    bool processMouseWheel(float delta, float x, float y, int modifiers);
    bool processChar(char c);
    
    // Shortcut management
    void registerShortcut(const std::string& action, int key, int modifiers, std::function<void()> callback);
    void unregisterShortcut(const std::string& action);
    void clearShortcuts();
    
    // Default shortcuts
    void registerDefaultShortcuts();
    
    // Active module (receives priority for input)
    void setActiveModule(EditModule* module) { activeModule = module; }
    EditModule* getActiveModule() const { return activeModule; }
    
    // Camera controls
    void enableCameraControl(bool enable) { cameraControlEnabled = enable; }
    bool isCameraControlEnabled() const { return cameraControlEnabled; }
    CameraControlState& getCameraState() { return cameraState; }
    
    // Callbacks for camera actions
    using CameraOrbitCallback = std::function<void(float deltaYaw, float deltaPitch)>;
    using CameraPanCallback = std::function<void(float deltaX, float deltaY)>;
    using CameraZoomCallback = std::function<void(float delta)>;
    
    void setCameraOrbitCallback(CameraOrbitCallback cb) { cameraOrbitCallback = cb; }
    void setCameraPanCallback(CameraPanCallback cb) { cameraPanCallback = cb; }
    void setCameraZoomCallback(CameraZoomCallback cb) { cameraZoomCallback = cb; }
    
    // Mouse state
    float getMouseX() const { return mouseX; }
    float getMouseY() const { return mouseY; }
    bool isMouseButtonDown(int button) const { return mouseButtons[button]; }
    
    // Keyboard state
    bool isKeyDown(int key) const { return keyStates.count(key) > 0 && keyStates.at(key); }
    bool isCtrlDown() const { return ctrlDown; }
    bool isShiftDown() const { return shiftDown; }
    bool isAltDown() const { return altDown; }
    
private:
    // Shortcuts
    std::vector<Shortcut> shortcuts;
    
    // Active module
    EditModule* activeModule = nullptr;
    
    // Input state
    float mouseX = 0, mouseY = 0;
    bool mouseButtons[3] = {false, false, false};
    std::unordered_map<int, bool> keyStates;
    bool ctrlDown = false;
    bool shiftDown = false;
    bool altDown = false;
    
    // Camera control
    bool cameraControlEnabled = true;
    CameraControlState cameraState;
    CameraOrbitCallback cameraOrbitCallback;
    CameraPanCallback cameraPanCallback;
    CameraZoomCallback cameraZoomCallback;
    
    // Helper
    bool tryShortcut(int key, int modifiers);
    InputEvent makeInputEvent(InputEvent::Type type, int keyOrButton);
};

// ============================================================================
// Implementation
// ============================================================================

inline InputHandler::InputHandler() {
    registerDefaultShortcuts();
}

inline bool InputHandler::processKeyDown(int key, int modifiers) {
    // Update modifier state
    ctrlDown = (modifiers & LUMA_MOD_CTRL) != 0;
    shiftDown = (modifiers & LUMA_MOD_SHIFT) != 0;
    altDown = (modifiers & LUMA_MOD_ALT) != 0;
    keyStates[key] = true;
    
    // Try shortcut first
    if (tryShortcut(key, modifiers)) {
        return true;
    }
    
    // Forward to active module
    if (activeModule) {
        InputEvent event = makeInputEvent(InputEvent::Type::KeyDown, key);
        event.modifiers = modifiers;
        if (activeModule->handleInput(event)) {
            return true;
        }
    }
    
    return false;
}

inline bool InputHandler::processKeyUp(int key, int modifiers) {
    ctrlDown = (modifiers & LUMA_MOD_CTRL) != 0;
    shiftDown = (modifiers & LUMA_MOD_SHIFT) != 0;
    altDown = (modifiers & LUMA_MOD_ALT) != 0;
    keyStates[key] = false;
    
    if (activeModule) {
        InputEvent event = makeInputEvent(InputEvent::Type::KeyUp, key);
        event.modifiers = modifiers;
        return activeModule->handleInput(event);
    }
    
    return false;
}

inline bool InputHandler::processMouseDown(int button, float x, float y, int modifiers) {
    mouseX = x;
    mouseY = y;
    if (button >= 0 && button < 3) {
        mouseButtons[button] = true;
    }
    
    // Camera control with middle mouse or alt+left
    if (cameraControlEnabled) {
        if (button == Mouse::Middle || (button == Mouse::Left && (modifiers & LUMA_MOD_ALT))) {
            if (modifiers & LUMA_MOD_SHIFT) {
                cameraState.panning = true;
            } else if (modifiers & LUMA_MOD_CTRL) {
                cameraState.dollying = true;
            } else {
                cameraState.orbiting = true;
            }
            cameraState.lastMouseX = x;
            cameraState.lastMouseY = y;
            return true;
        }
    }
    
    // Forward to active module
    if (activeModule) {
        InputEvent event = makeInputEvent(InputEvent::Type::MouseDown, button);
        event.modifiers = modifiers;
        event.mouseX = x;
        event.mouseY = y;
        return activeModule->handleInput(event);
    }
    
    return false;
}

inline bool InputHandler::processMouseUp(int button, float x, float y, int modifiers) {
    mouseX = x;
    mouseY = y;
    if (button >= 0 && button < 3) {
        mouseButtons[button] = false;
    }
    
    // End camera control
    if (button == Mouse::Middle || button == Mouse::Left) {
        cameraState.orbiting = false;
        cameraState.panning = false;
        cameraState.dollying = false;
    }
    
    // Forward to active module
    if (activeModule) {
        InputEvent event = makeInputEvent(InputEvent::Type::MouseUp, button);
        event.modifiers = modifiers;
        event.mouseX = x;
        event.mouseY = y;
        return activeModule->handleInput(event);
    }
    
    return false;
}

inline bool InputHandler::processMouseMove(float x, float y, int modifiers) {
    float deltaX = x - mouseX;
    float deltaY = y - mouseY;
    mouseX = x;
    mouseY = y;
    
    // Camera control
    if (cameraControlEnabled) {
        if (cameraState.orbiting) {
            if (cameraOrbitCallback) {
                cameraOrbitCallback(deltaX * 0.5f, deltaY * 0.5f);
            }
            cameraState.lastMouseX = x;
            cameraState.lastMouseY = y;
            return true;
        }
        if (cameraState.panning) {
            if (cameraPanCallback) {
                cameraPanCallback(deltaX * 0.01f, deltaY * 0.01f);
            }
            cameraState.lastMouseX = x;
            cameraState.lastMouseY = y;
            return true;
        }
        if (cameraState.dollying) {
            if (cameraZoomCallback) {
                cameraZoomCallback(deltaY * 0.05f);
            }
            cameraState.lastMouseX = x;
            cameraState.lastMouseY = y;
            return true;
        }
    }
    
    // Forward to active module
    if (activeModule) {
        InputEvent event = makeInputEvent(InputEvent::Type::MouseMove, 0);
        event.modifiers = modifiers;
        event.mouseX = x;
        event.mouseY = y;
        return activeModule->handleInput(event);
    }
    
    return false;
}

inline bool InputHandler::processMouseWheel(float delta, float x, float y, int modifiers) {
    (void)x;
    (void)y;
    (void)modifiers;
    
    // Camera zoom
    if (cameraControlEnabled && cameraZoomCallback) {
        cameraZoomCallback(delta);
        return true;
    }
    
    return false;
}

inline bool InputHandler::processChar(char c) {
    if (activeModule) {
        InputEvent event;
        event.type = InputEvent::Type::Char;
        event.character = c;
        return activeModule->handleInput(event);
    }
    return false;
}

inline void InputHandler::registerShortcut(const std::string& action, int key, int modifiers, std::function<void()> callback) {
    // Remove existing shortcut for this action
    unregisterShortcut(action);
    
    Shortcut shortcut;
    shortcut.action = action;
    shortcut.key = key;
    shortcut.modifiers = modifiers;
    shortcut.callback = callback;
    shortcuts.push_back(shortcut);
}

inline void InputHandler::unregisterShortcut(const std::string& action) {
    shortcuts.erase(
        std::remove_if(shortcuts.begin(), shortcuts.end(),
            [&action](const Shortcut& s) { return s.action == action; }),
        shortcuts.end()
    );
}

inline void InputHandler::clearShortcuts() {
    shortcuts.clear();
}

inline void InputHandler::registerDefaultShortcuts() {
    // These will be connected to actual callbacks in StudioApp
    // Just registering the key bindings here
}

inline bool InputHandler::tryShortcut(int key, int modifiers) {
    for (const auto& shortcut : shortcuts) {
        if (shortcut.matches(key, modifiers)) {
            if (shortcut.callback) {
                shortcut.callback();
            }
            return true;
        }
    }
    return false;
}

inline InputEvent InputHandler::makeInputEvent(InputEvent::Type type, int keyOrButton) {
    InputEvent event;
    event.type = type;
    event.key = keyOrButton;
    event.mouseX = mouseX;
    event.mouseY = mouseY;
    event.modifiers = (ctrlDown ? LUMA_MOD_CTRL : 0) | 
                      (shiftDown ? LUMA_MOD_SHIFT : 0) | 
                      (altDown ? LUMA_MOD_ALT : 0);
    return event;
}

} // namespace editor
} // namespace luma
