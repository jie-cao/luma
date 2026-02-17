// LUMA Editor Mode Handler
// Base class for mode-level input/render/UI handling
// Each EditorMode (Scene, Edit, Animation, etc.) has its own handler

#pragma once

#include "engine/editor/editor_mode.h"
#include "engine/editor/edit_module.h"
#include "engine/renderer/draw_manager.h"
#include <string>
#include <memory>

namespace luma {

// Forward declarations
class UnifiedRenderer;
class SceneGraph;
class Viewport;
class TransformGizmo;
class EditMesh;

namespace editor {

// Mode handler base class
// Each mode implements this to handle its own input, rendering, and UI
class EditorModeHandler {
public:
    EditorModeHandler(EditorMode mode, const std::string& name)
        : m_mode(mode), m_name(name) {}
    virtual ~EditorModeHandler() = default;
    
    // ===== Lifecycle =====
    // Called when entering this mode
    virtual void onEnter() = 0;
    
    // Called when exiting this mode
    virtual void onExit() = 0;
    
    // Called every frame while active
    virtual void update(float deltaTime) = 0;
    
    // ===== Rendering =====
    // Render 3D content for this mode
    virtual void render(const RenderContext& ctx) = 0;
    
    // Render ImGui UI for this mode
    virtual void renderUI() = 0;
    
    // ===== Input =====
    // Handle input event - return true if consumed
    virtual bool handleInput(const InputEvent& event) = 0;
    
    // ===== Mode Info =====
    EditorMode getMode() const { return m_mode; }
    const std::string& getName() const { return m_name; }
    
    // ===== State =====
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }
    
protected:
    EditorMode m_mode;
    std::string m_name;
    bool m_active = false;
};

// Mode handler context - shared resources passed to handlers
struct ModeHandlerContext {
    UnifiedRenderer* renderer = nullptr;
    SceneGraph* scene = nullptr;
    Viewport* viewport = nullptr;
    TransformGizmo* gizmo = nullptr;
    DrawManager* drawManager = nullptr;
    EditModePipeline* editPipeline = nullptr;
    
    // Native window handle (HWND on Windows, for file dialogs etc.)
    void* nativeWindowHandle = nullptr;
    
    // Window dimensions
    int windowWidth = 1280;
    int windowHeight = 720;
    
    // Camera parameters
    float viewMatrix[16] = {};
    float projMatrix[16] = {};
    float cameraPos[3] = {};
    float fovY = 0.785398f;  // Default ~45 degrees in radians
    float sceneRadius = 1.0f;
    float sceneCenter[3] = {};
    
    // Helper to create render context
    RenderContext toRenderContext() const {
        RenderContext ctx;
        ctx.renderer = renderer;
        memcpy(ctx.viewMatrix, viewMatrix, sizeof(viewMatrix));
        memcpy(ctx.projMatrix, projMatrix, sizeof(projMatrix));
        memcpy(ctx.cameraPos, cameraPos, sizeof(cameraPos));
        ctx.viewportWidth = windowWidth;
        ctx.viewportHeight = windowHeight;
        return ctx;
    }
};

// Mode Manager - manages all mode handlers and dispatches events
class ModeHandlerManager {
public:
    ModeHandlerManager() = default;
    ~ModeHandlerManager() = default;
    
    // Register a mode handler
    void registerHandler(std::unique_ptr<EditorModeHandler> handler) {
        if (handler) {
            m_handlers[handler->getMode()] = std::move(handler);
        }
    }
    
    // Get handler for a specific mode
    EditorModeHandler* getHandler(EditorMode mode) {
        auto it = m_handlers.find(mode);
        return it != m_handlers.end() ? it->second.get() : nullptr;
    }
    
    // Get current active handler
    EditorModeHandler* getCurrentHandler() { return m_currentHandler; }
    
    // Switch to a new mode
    void switchMode(EditorMode newMode) {
        if (m_currentHandler && m_currentHandler->getMode() == newMode) {
            return; // Already in this mode
        }
        
        // Exit current mode
        if (m_currentHandler) {
            m_currentHandler->onExit();
            m_currentHandler->setActive(false);
        }
        
        // Enter new mode
        m_previousMode = m_currentMode;
        m_currentMode = newMode;
        m_currentHandler = getHandler(newMode);
        
        if (m_currentHandler) {
            m_currentHandler->setActive(true);
            m_currentHandler->onEnter();
        }
    }
    
    // Return to previous mode
    void returnToPreviousMode() {
        switchMode(m_previousMode);
    }
    
    // Get current mode
    EditorMode getCurrentMode() const { return m_currentMode; }
    EditorMode getPreviousMode() const { return m_previousMode; }
    
    // ===== Dispatch methods =====
    
    // Update current handler
    void update(float deltaTime) {
        if (m_currentHandler) {
            m_currentHandler->update(deltaTime);
        }
    }
    
    // Render current handler
    void render(const RenderContext& ctx) {
        if (m_currentHandler) {
            m_currentHandler->render(ctx);
        }
    }
    
    // Render UI for current handler
    void renderUI() {
        if (m_currentHandler) {
            m_currentHandler->renderUI();
        }
    }
    
    // Handle input - returns true if consumed
    bool handleInput(const InputEvent& event) {
        if (m_currentHandler) {
            return m_currentHandler->handleInput(event);
        }
        return false;
    }
    
    // Set shared context
    void setContext(const ModeHandlerContext& ctx) {
        m_context = ctx;
    }
    
    ModeHandlerContext& getContext() { return m_context; }
    const ModeHandlerContext& getContext() const { return m_context; }
    
private:
    std::unordered_map<EditorMode, std::unique_ptr<EditorModeHandler>> m_handlers;
    EditorModeHandler* m_currentHandler = nullptr;
    EditorMode m_currentMode = EditorMode::Welcome;
    EditorMode m_previousMode = EditorMode::Welcome;
    ModeHandlerContext m_context;
};

} // namespace editor
} // namespace luma
