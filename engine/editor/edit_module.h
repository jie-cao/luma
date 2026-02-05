// LUMA Edit Module Base Class
// Abstract base for all editing modules (Mesh, UV, Material, etc.)
// Following Blender/Maya style modular editing architecture

#pragma once

#include "engine/renderer/draw_manager.h"
#include <string>
#include <memory>

namespace luma {
namespace editor {

// Forward declarations
class InputEvent;

// Edit module base class
class EditModule {
public:
    EditModule(const std::string& name) : moduleName(name) {}
    virtual ~EditModule() = default;
    
    // Lifecycle
    virtual void onEnter() = 0;                     // Called when entering this module
    virtual void onExit() = 0;                      // Called when exiting this module
    virtual void update(float deltaTime) = 0;       // Called every frame
    
    // Rendering
    virtual void render(DrawManager& drawManager, const RenderContext& ctx) = 0;
    virtual void renderUI() = 0;                    // ImGui UI
    
    // Input handling
    virtual bool handleInput(const InputEvent& event) = 0;  // Return true if handled
    
    // Selection (optional override)
    virtual void clearSelection() {}
    virtual void selectAll() {}
    
    // State
    bool isActive() const { return active; }
    void setActive(bool a) { active = a; }
    const std::string& getName() const { return moduleName; }
    
    // Dirty state (indicates changes need saving)
    bool isDirty() const { return dirty; }
    void markDirty() { dirty = true; }
    void clearDirty() { dirty = false; }
    
protected:
    std::string moduleName;
    bool active = false;
    bool dirty = false;
};

// Input event structure
struct InputEvent {
    enum class Type {
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        MouseWheel,
        Char
    };
    
    Type type;
    int key = 0;          // Key code or mouse button
    int modifiers = 0;    // Ctrl=1, Shift=2, Alt=4
    float mouseX = 0;
    float mouseY = 0;
    float wheelDelta = 0;
    char character = 0;
    
    bool isCtrl() const { return (modifiers & 1) != 0; }
    bool isShift() const { return (modifiers & 2) != 0; }
    bool isAlt() const { return (modifiers & 4) != 0; }
};

// Modifier key constants (use LUMA_ prefix to avoid Windows macro conflicts)
constexpr int LUMA_MOD_CTRL = 1;
constexpr int LUMA_MOD_SHIFT = 2;
constexpr int LUMA_MOD_ALT = 4;

} // namespace editor
} // namespace luma
