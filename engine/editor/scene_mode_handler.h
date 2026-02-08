// LUMA Scene Mode Handler
// Handles all input, rendering, and UI for Scene mode
// Includes entity selection, gizmo interaction, and PBR rendering

#pragma once

#include "engine/editor/mode_handler.h"
#include "engine/editor/gizmo.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/picking.h"
#include "engine/viewport/viewport.h"
#include "engine/renderer/unified_renderer.h"

namespace luma {
namespace editor {

// Scene Mode Handler
class SceneModeHandler : public EditorModeHandler {
public:
    SceneModeHandler();
    ~SceneModeHandler() override;
    
    // Initialize with context
    bool init(ModeHandlerContext* ctx);
    
    // ===== EditorModeHandler interface =====
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render(const RenderContext& ctx) override;
    void renderUI() override;
    bool handleInput(const InputEvent& event) override;
    
    // ===== Scene Mode specific =====
    
    // Gizmo mode
    void setGizmoMode(GizmoMode mode);
    GizmoMode getGizmoMode() const { return m_gizmoMode; }
    
    // Ray generation callback (set by main app)
    // Note: Uses luma::Ray (from scene/picking.h), not luma::editor::Ray
    using RayCallback = std::function<luma::Ray(float screenX, float screenY)>;
    void setRayCallback(RayCallback cb) { m_rayCallback = cb; }
    
    // Gizmo screen scale callback
    using ScreenScaleCallback = std::function<float(const Vec3& position)>;
    void setScreenScaleCallback(ScreenScaleCallback cb) { m_screenScaleCallback = cb; }
    
private:
    // Context
    ModeHandlerContext* m_ctx = nullptr;
    
    // State
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    bool m_isDraggingGizmo = false;
    
    // Mouse tracking for click detection
    float m_mouseDownX = 0, m_mouseDownY = 0;
    bool m_mouseWasDown = false;
    
    // Camera control state
    bool m_isOrbiting = false;
    bool m_isPanning = false;
    bool m_isDollying = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    
    // Callbacks
    RayCallback m_rayCallback;
    ScreenScaleCallback m_screenScaleCallback;
    
    // Internal helpers
    void handleEntityPicking(float mouseX, float mouseY);
    bool isClickDistance(float x1, float y1, float x2, float y2, float threshold = 5.0f);
};

// ============================================================================
// Implementation
// ============================================================================

inline SceneModeHandler::SceneModeHandler()
    : EditorModeHandler(EditorMode::Scene, "Scene") {
}

inline SceneModeHandler::~SceneModeHandler() = default;

inline bool SceneModeHandler::init(ModeHandlerContext* ctx) {
    m_ctx = ctx;
    return ctx != nullptr;
}

inline void SceneModeHandler::onEnter() {
    m_active = true;
    m_isDraggingGizmo = false;
    m_isOrbiting = false;
    m_isPanning = false;
    m_isDollying = false;
}

inline void SceneModeHandler::onExit() {
    m_active = false;
}

inline void SceneModeHandler::update(float deltaTime) {
    (void)deltaTime;
    // Scene mode doesn't need continuous updates
}

inline void SceneModeHandler::render(const RenderContext& ctx) {
    if (!m_active || !m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    
    auto* renderer = m_ctx->renderer;
    auto* scene = m_ctx->scene;
    
    // Note: Scene main rendering (models) is done in main.cpp Render3DContent()
    // This method only handles overlays: selection outline and transform gizmo
    
    // Render selection outline and gizmo
    if (auto* selected = scene->getSelectedEntity()) {
        // Selection outline
        if (selected->hasModel) {
            float outlineColor[4] = {1.0f, 0.6f, 0.2f, 1.0f};
            renderer->renderModelOutline(selected->model, selected->worldMatrix.m, outlineColor);
        }
        
        // Transform Gizmo
        if (m_ctx->gizmo && m_screenScaleCallback) {
            Vec3 gizmoPos = selected->getWorldPosition();
            float screenScale = m_screenScaleCallback(gizmoPos);
            
            m_ctx->gizmo->setTarget(selected);
            m_ctx->gizmo->setMode(m_gizmoMode);
            auto gizmoData = m_ctx->gizmo->generateRenderData(screenScale);
            if (!gizmoData.lines.empty()) {
                renderer->renderGizmoLines(
                    reinterpret_cast<const float*>(gizmoData.lines.data()),
                    static_cast<uint32_t>(gizmoData.lines.size()));
            }
        }
    }
}

inline void SceneModeHandler::renderUI() {
    // Scene mode UI (hierarchy panel, inspector) is rendered by main RenderUI
    // This is a placeholder for mode-specific UI
}

inline bool SceneModeHandler::handleInput(const InputEvent& event) {
    if (!m_active || !m_ctx) return false;
    
    switch (event.type) {
        case InputEvent::Type::KeyDown:
            // Gizmo tool shortcuts
            if (event.key == 'W' && !event.isCtrl()) {
                setGizmoMode(GizmoMode::Translate);
                return true;
            }
            if (event.key == 'E' && !event.isCtrl()) {
                setGizmoMode(GizmoMode::Rotate);
                return true;
            }
            if (event.key == 'R' && !event.isCtrl()) {
                setGizmoMode(GizmoMode::Scale);
                return true;
            }
            // Note: Delete is handled in WndProc as a global shortcut
            break;
            
        case InputEvent::Type::MouseDown:
            if (event.key == 0) { // Left mouse
                m_mouseDownX = event.mouseX;
                m_mouseDownY = event.mouseY;
                m_mouseWasDown = true;
                
                // Note: Gizmo interaction is handled in WndProc before delegation
                // Note: Camera controls are handled in WndProc fallback
            }
            break;
            
        case InputEvent::Type::MouseUp:
            if (event.key == 0) { // Left mouse
                // Check if this was a click (not a drag)
                if (m_mouseWasDown && isClickDistance(m_mouseDownX, m_mouseDownY, event.mouseX, event.mouseY)) {
                    handleEntityPicking(event.mouseX, event.mouseY);
                    m_mouseWasDown = false;
                    return true;
                }
                m_mouseWasDown = false;
            }
            break;
            
        case InputEvent::Type::MouseMove:
            // Note: Gizmo drag and camera controls are handled in WndProc
            break;
            
        case InputEvent::Type::MouseWheel:
            // Note: Camera zoom is handled in WndProc fallback
            break;
            
        default:
            break;
    }
    
    return false;
}

inline void SceneModeHandler::setGizmoMode(GizmoMode mode) {
    m_gizmoMode = mode;
    if (m_ctx && m_ctx->gizmo) {
        m_ctx->gizmo->setMode(mode);
    }
}

inline void SceneModeHandler::handleEntityPicking(float mouseX, float mouseY) {
    if (!m_ctx || !m_ctx->scene || !m_rayCallback) return;
    
    luma::Ray ray = m_rayCallback(mouseX, mouseY);
    
    // Use the existing pickEntity function from scene/picking.h
    // Note: Use luma::PickResult, not luma::editor::PickResult
    luma::PickResult result = luma::pickEntity(*m_ctx->scene, ray);
    
    if (result.hit()) {
        m_ctx->scene->setSelectedEntity(result.entity);
    } else {
        m_ctx->scene->clearSelection();
    }
}

inline bool SceneModeHandler::isClickDistance(float x1, float y1, float x2, float y2, float threshold) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return (dx * dx + dy * dy) < (threshold * threshold);
}

} // namespace editor
} // namespace luma
