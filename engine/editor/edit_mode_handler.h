// LUMA Edit Mode Handler
// Handles all input, rendering, and UI for Edit mode
// Integrates MeshEditModule, UVEditModule, MaterialEditModule

#pragma once

#include "engine/editor/mode_handler.h"
#include "engine/editor/mesh_edit/mesh_edit_module.h"
#include "engine/editor/mesh_edit/mesh_edit_gizmo.h"
#include "engine/editor/uv_edit/uv_edit_module.h"
#include "engine/editor/material_edit/material_edit_module.h"
#include "engine/editor/selection_system.h"
#include "engine/editor/mesh_picking.h"
#include "engine/editor/mode_ui.h"
#include "engine/mesh/edit_mesh.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/renderer/unified_renderer.h"
#include <memory>

namespace luma {
namespace editor {

// Sub-modes within Edit mode
enum class EditSubMode {
    Mesh,       // Mesh editing (vertices, edges, faces)
    UV,         // UV editing
    Material    // Material editing
};

// Edit Mode Handler
class EditModeHandler : public EditorModeHandler {
public:
    EditModeHandler();
    ~EditModeHandler() override;
    
    // Initialize with context
    bool init(ModeHandlerContext* ctx);
    
    // ===== EditorModeHandler interface =====
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render(const RenderContext& ctx) override;
    void renderUI() override;
    bool handleInput(const InputEvent& event) override;
    
    // ===== Edit Mode specific =====
    
    // Sub-mode management
    void setSubMode(EditSubMode subMode);
    EditSubMode getSubMode() const { return m_subMode; }
    
    // EditMesh management
    void setEditMesh(EditMesh* mesh);
    EditMesh* getEditMesh() const { return m_editMesh; }
    
    // Selection mode (vertex, edge, face)
    void setSelectionMode(SelectionMode mode);
    SelectionMode getSelectionMode() const { return m_selectionMode; }
    
    // View mode (Material, Solid, Wireframe)
    void setViewMode(ViewMode mode) { m_viewMode = mode; }
    ViewMode getViewMode() const { return m_viewMode; }
    
    // Access sub-modules
    MeshEditModule* getMeshEditModule() { return m_meshEdit.get(); }
    UVEditModule* getUVEditModule() { return m_uvEdit.get(); }
    MaterialEditModule* getMaterialEditModule() { return m_materialEdit.get(); }
    
    // UI state
    int getSelectedMeshIndex() const { return m_selectedMeshIndex; }
    void setSelectedMeshIndex(int idx) { m_selectedMeshIndex = idx; }
    
    // X-Ray mode (see-through wireframe/selection)
    bool isXRayMode() const { return m_xRayMode; }
    void setXRayMode(bool enabled) { 
        m_xRayMode = enabled;
        // Sync to edit pipeline
        if (m_ctx && m_ctx->editPipeline) {
            m_ctx->editPipeline->setXRayMode(enabled);
        }
    }
    
    // Toolbar state
    bool showOriginalEdges() const { return m_showOriginalEdges; }
    void setShowOriginalEdges(bool v) { m_showOriginalEdges = v; }
    bool showAllEdges() const { return m_showAllEdges; }
    void setShowAllEdges(bool v) { m_showAllEdges = v; }
    bool showVertices() const { return m_showVertices; }
    
    // Selection box state (for UI overlay sync)
    bool isSelecting() const { return m_isSelecting; }
    void getSelectionBounds(float& x1, float& y1, float& x2, float& y2) const {
        x1 = m_selectStartX; y1 = m_selectStartY;
        x2 = m_selectEndX; y2 = m_selectEndY;
    }
    SelectionTool getSelectTool() const { return m_selectTool; }
    void setSelectTool(SelectionTool tool) { m_selectTool = tool; }
    
    // Mesh Edit Gizmo (for transforming selected vertices/edges/faces)
    MeshEditGizmo& getMeshGizmo() { return m_meshGizmo; }
    GizmoMode getGizmoMode() const { return m_gizmoMode; }
    void setGizmoMode(GizmoMode mode) { 
        m_gizmoMode = mode; 
        // Clamp to valid mode for current selection type
        m_meshGizmo.setMode(getEffectiveGizmoMode());
    }
    bool isGizmoDragging() const { return m_meshGizmo.isDragging(); }
    
    // Whether gizmo should be shown (Move/Rotate/Scale/Extrude tools)
    bool shouldShowGizmo() const { return m_showGizmo; }
    void setShowGizmo(bool show) { m_showGizmo = show; }
    
    // Extrude mode
    bool isExtrudeMode() const { return m_extrudeMode; }
    void setExtrudeMode(bool enabled) { 
        m_extrudeMode = enabled; 
        m_meshGizmo.setExtrudeMode(enabled);
        if (enabled) {
            updateExtrudeNormal();
        }
    }
    
    // Get effective gizmo mode (clamped by selection type)
    // Vertex: Translate only
    // Edge: Translate + Rotate
    // Face: Translate + Rotate + Scale
    GizmoMode getEffectiveGizmoMode() const {
        switch (m_selectionMode) {
            case SelectionMode::Vertex:
                return GizmoMode::Translate;  // Vertices: move only
            case SelectionMode::Edge:
                if (m_gizmoMode == GizmoMode::Scale) return GizmoMode::Translate;
                return m_gizmoMode;  // Edges: move + rotate
            case SelectionMode::Face:
            default:
                return m_gizmoMode;  // Faces: all modes
        }
    }
    
    // Dirty state (has unsaved changes)
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }
    
    // Callbacks
    using MeshChangedCallback = std::function<void()>;
    using SaveCallback = std::function<void()>;
    using CancelCallback = std::function<void()>;
    
    void setMeshChangedCallback(MeshChangedCallback cb) { m_meshChangedCallback = cb; }
    void setSaveCallback(SaveCallback cb) { m_saveCallback = cb; }
    void setCancelCallback(CancelCallback cb) { m_cancelCallback = cb; }
    
    // Projection helpers (set by main app)
    using ProjectionCallback = std::function<bool(float wx, float wy, float wz, float& sx, float& sy)>;
    // Note: Uses luma::Ray (from scene/picking.h), not luma::editor::Ray
    using RayCallback = std::function<luma::Ray(float screenX, float screenY)>;
    
    void setProjectionCallback(ProjectionCallback cb) { m_projectionCallback = cb; }
    void setRayCallback(RayCallback cb) { m_rayCallback = cb; }
    
    // Update the extrude normal from the current selection
    void updateExtrudeNormal() {
        if (!m_editMesh) return;
        
        float localNormal[3] = {0, 1, 0};
        
        if (m_selectionMode == SelectionMode::Face) {
            m_editMesh->getSelectedFacesNormal(localNormal);
        } else if (m_selectionMode == SelectionMode::Edge) {
            // For edges, use the average normal of adjacent faces
            // Simplified: use up direction
            localNormal[0] = 0; localNormal[1] = 1; localNormal[2] = 0;
        } else {
            // For vertices, use up direction
            localNormal[0] = 0; localNormal[1] = 1; localNormal[2] = 0;
        }
        
        // Transform normal to world space (rotation only)
        auto* selectedEntity = m_ctx && m_ctx->scene ? m_ctx->scene->getSelectedEntity() : nullptr;
        if (selectedEntity) {
            const float* wm = selectedEntity->worldMatrix.m;
            Vec3 worldNormal(
                wm[0]*localNormal[0] + wm[4]*localNormal[1] + wm[8]*localNormal[2],
                wm[1]*localNormal[0] + wm[5]*localNormal[1] + wm[9]*localNormal[2],
                wm[2]*localNormal[0] + wm[6]*localNormal[1] + wm[10]*localNormal[2]
            );
            worldNormal = worldNormal.normalized();
            m_meshGizmo.setExtrudeNormal(worldNormal);
        } else {
            m_meshGizmo.setExtrudeNormal(Vec3(localNormal[0], localNormal[1], localNormal[2]));
        }
    }
    
    // Sync all settings from the toolbar/UI — call this once per frame from main
    // This replaces ALL the scattered setXxx calls in main.cpp
    void syncFromToolbar(EditModeToolbar& toolbar, ViewMode viewMode, int meshIndex) {
        // 1) Selection mode
        SelectionMode selMode = SelectionMode::Vertex;
        switch (toolbar.selectMode) {
            case EditModeToolbar::SelectMode::Vertex:
                selMode = SelectionMode::Vertex; break;
            case EditModeToolbar::SelectMode::Edge:
                selMode = SelectionMode::Edge; break;
            case EditModeToolbar::SelectMode::Face:
                selMode = SelectionMode::Face; break;
        }
        if (m_selectionMode != selMode) {
            setSelectionMode(selMode);
        }
        
        // 2) Selection tool
        SelectionTool toolbarTool = SelectionTool::Click;
        switch (toolbar.selectTool) {
            case EditModeToolbar::SelectTool::Click:
                toolbarTool = SelectionTool::Click; break;
            case EditModeToolbar::SelectTool::Box:
                toolbarTool = SelectionTool::Box; break;
            case EditModeToolbar::SelectTool::Circle:
                toolbarTool = SelectionTool::Circle; break;
            case EditModeToolbar::SelectTool::Lasso:
                toolbarTool = SelectionTool::Lasso; break;
        }
        setSelectTool(toolbarTool);
        if (m_meshEdit) {
            m_meshEdit->setSelectionTool(toolbarTool);
        }
        
        // 3) Display settings
        m_showOriginalEdges = toolbar.showOriginalEdges;
        m_showAllEdges = toolbar.showAllEdges;
        setXRayMode(toolbar.xRayMode);
        if (m_meshEdit) {
            m_meshEdit->setXRayMode(toolbar.xRayMode);
        }
        setViewMode(viewMode);
        setSelectedMeshIndex(meshIndex);
        
        // 4) Edit tool → gizmo mode
        using ET = EditModeToolbar::EditTool;
        switch (toolbar.currentTool) {
            case ET::Move:
                setShowGizmo(true);
                setExtrudeMode(false);
                setGizmoMode(GizmoMode::Translate);
                break;
            case ET::Rotate:
                setShowGizmo(true);
                setExtrudeMode(false);
                setGizmoMode(GizmoMode::Rotate);
                break;
            case ET::Scale:
                setShowGizmo(true);
                setExtrudeMode(false);
                setGizmoMode(GizmoMode::Scale);
                break;
            case ET::Extrude:
                setShowGizmo(true);
                setExtrudeMode(true);
                setGizmoMode(GizmoMode::Translate);
                break;
            case ET::Select:
            default:
                setShowGizmo(false);
                setExtrudeMode(false);
                break;
        }
    }
    
    // Perform the extrude operation based on current selection mode
    bool performExtrude() {
        if (!m_editMesh) return false;
        
        std::set<uint32_t> newVerts;
        
        switch (m_selectionMode) {
            case SelectionMode::Face:
                newVerts = m_editMesh->extrudeSelectedFacesInPlace();
                break;
            case SelectionMode::Edge:
                newVerts = m_editMesh->extrudeSelectedEdgesInPlace();
                break;
            case SelectionMode::Vertex:
                newVerts = m_editMesh->extrudeSelectedVerticesInPlace();
                break;
        }
        
        if (newVerts.empty()) return false;
        
        // Sync to GPU and rebuild gizmo state
        syncEditMeshToGPU();
        
        // Extrusion changes topology — rebuild GPU edge index buffer
        rebuildGPUEdgeIndexBuffer();
        
        m_dirty = true;
        
        if (m_meshChangedCallback) {
            m_meshChangedCallback();
        }
        
        return true;
    }
    
private:
    // Context
    ModeHandlerContext* m_ctx = nullptr;
    
    // Sub-modules
    std::unique_ptr<MeshEditModule> m_meshEdit;
    std::unique_ptr<UVEditModule> m_uvEdit;
    std::unique_ptr<MaterialEditModule> m_materialEdit;
    
    // State
    EditMesh* m_editMesh = nullptr;
    EditSubMode m_subMode = EditSubMode::Mesh;
    SelectionMode m_selectionMode = SelectionMode::Vertex;
    ViewMode m_viewMode = ViewMode::Material;
    int m_selectedMeshIndex = -1;
    bool m_dirty = false;
    
    // UI state
    bool m_showOriginalEdges = true;
    bool m_showAllEdges = false;
    bool m_showVertices = true;
    bool m_xRayMode = false;  // Default: X-Ray OFF (hidden line removal)
    
    // Selection box state
    bool m_isSelecting = false;
    float m_selectStartX = 0, m_selectStartY = 0;
    float m_selectEndX = 0, m_selectEndY = 0;
    SelectionTool m_selectTool = SelectionTool::Click;
    
    // Mesh Edit Gizmo (for transforming selected elements)
    MeshEditGizmo m_meshGizmo;
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    bool m_showGizmo = true;
    bool m_extrudeMode = false;
    
    // Sync EditMesh vertex positions back to GPU vertex buffer (real-time deform)
    // Public so undo/redo in main.cpp can also trigger it
    public:
    void syncEditMeshToGPU();
    void rebuildGPUEdgeIndexBuffer();
    private:
    
    // Callbacks
    MeshChangedCallback m_meshChangedCallback;
    SaveCallback m_saveCallback;
    CancelCallback m_cancelCallback;
    ProjectionCallback m_projectionCallback;
    RayCallback m_rayCallback;
    
    // Internal helpers
    void renderMeshWireframe(Entity* entity, const float* wireColor);
    void renderSelectedOverlay(Entity* entity);
    void renderSelectionHighlights();
    void handleMeshPicking(float mouseX, float mouseY, bool additive);
    void handleBoxSelection(float x1, float y1, float x2, float y2, bool additive);
    bool projectToScreen(float wx, float wy, float wz, float& sx, float& sy);
};

// ============================================================================
// Implementation
// ============================================================================

inline EditModeHandler::EditModeHandler()
    : EditorModeHandler(EditorMode::Edit, "Edit") {
    m_meshEdit = std::make_unique<MeshEditModule>();
    m_uvEdit = std::make_unique<UVEditModule>();
    m_materialEdit = std::make_unique<MaterialEditModule>();
}

inline EditModeHandler::~EditModeHandler() = default;

inline bool EditModeHandler::init(ModeHandlerContext* ctx) {
    m_ctx = ctx;
    if (!ctx || !ctx->renderer) return false;
    
    m_meshEdit->init(ctx->renderer);
    m_uvEdit->init(ctx->renderer);
    m_materialEdit->init(ctx->renderer);
    
    return true;
}

inline void EditModeHandler::onEnter() {
    m_active = true;
    m_subMode = EditSubMode::Mesh;
    m_meshEdit->onEnter();
    m_isSelecting = false;
}

inline void EditModeHandler::onExit() {
    m_meshEdit->onExit();
    m_uvEdit->onExit();
    m_materialEdit->onExit();
    m_active = false;
    m_editMesh = nullptr;
    m_selectedMeshIndex = -1;
}

inline void EditModeHandler::update(float deltaTime) {
    if (!m_active) return;
    
    // Update selection context from ModeHandlerContext
    if (m_ctx) {
        SelectionContext selCtx;
        memcpy(selCtx.viewMatrix, m_ctx->viewMatrix, sizeof(selCtx.viewMatrix));
        memcpy(selCtx.projMatrix, m_ctx->projMatrix, sizeof(selCtx.projMatrix));
        selCtx.windowWidth = m_ctx->windowWidth;
        selCtx.windowHeight = m_ctx->windowHeight;
        
        // Get world matrix from selected entity
        if (m_ctx->scene) {
            if (auto* selectedEntity = m_ctx->scene->getSelectedEntity()) {
                memcpy(selCtx.worldMatrix, selectedEntity->worldMatrix.m, sizeof(selCtx.worldMatrix));
                selCtx.hasWorldMatrix = true;
                
                // Update mesh edit gizmo
                m_meshGizmo.setWorldMatrix(selectedEntity->worldMatrix.m);
            }
        }
        
        // Pass to mesh edit module
        m_meshEdit->setSelectionContext(selCtx);
        m_meshEdit->setXRayMode(m_xRayMode);
        
        // Sync mesh edit gizmo with edit mesh
        m_meshGizmo.setEditMesh(m_editMesh);
        m_meshGizmo.setMode(getEffectiveGizmoMode());
        
        // Update extrude normal when in extrude mode
        if (m_extrudeMode) {
            updateExtrudeNormal();
        }
    }
    
    // Always keep GPU mesh in sync with EditMesh (handles undo/redo, any external changes)
    syncEditMeshToGPU();
    
    switch (m_subMode) {
        case EditSubMode::Mesh:
            m_meshEdit->update(deltaTime);
            break;
        case EditSubMode::UV:
            m_uvEdit->update(deltaTime);
            break;
        case EditSubMode::Material:
            m_materialEdit->update(deltaTime);
            break;
    }
}

inline void EditModeHandler::render(const RenderContext& ctx) {
    if (!m_active || !m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    
    auto* renderer = m_ctx->renderer;
    auto* scene = m_ctx->scene;
    auto* selectedEntity = scene->getSelectedEntity();
    
    // Note: Scene main rendering (models with ViewMode) is done in main.cpp Render3DContent()
    // This method only handles overlays: wireframe overlay, selection highlights, and gizmo
    
    // Render selected mesh wireframe overlay (skip in Wireframe mode to avoid double rendering)
    if (selectedEntity && selectedEntity->hasModel && m_viewMode != ViewMode::Wireframe) {
        renderSelectedOverlay(selectedEntity);
    }
    
    // Render selection highlights (vertices, edges, faces)
    if (m_editMesh && selectedEntity) {
        float selectedColor[4] = {1.0f, 0.6f, 0.0f, 1.0f};
        if (m_ctx->editPipeline) {
            m_ctx->editPipeline->renderSelectedVertices(m_editMesh, selectedEntity->worldMatrix.m, selectedColor);
            m_ctx->editPipeline->renderSelectedEdges(m_editMesh, selectedEntity->worldMatrix.m, selectedColor);
            m_ctx->editPipeline->renderSelectedFaces(m_editMesh, selectedEntity->worldMatrix.m, selectedColor);
        }
        
        // Render mesh edit gizmo (only when a transform tool is active)
        if (m_showGizmo && m_meshGizmo.hasSelection()) {
            // Ensure gizmo reflects the effective mode for current selection type
            m_meshGizmo.setMode(getEffectiveGizmoMode());
            
            Vec3 gizmoPos = m_meshGizmo.getSelectionCenter();
            Vec3 camPos(m_ctx->cameraPos[0], m_ctx->cameraPos[1], m_ctx->cameraPos[2]);
            float screenScale = MeshEditGizmo::calculateScreenScale(
                gizmoPos, camPos, 100.0f, 
                static_cast<float>(m_ctx->windowHeight), m_ctx->fovY);
            
            GizmoRenderData gizmoData = m_meshGizmo.generateRenderData(screenScale);
            if (!gizmoData.lines.empty()) {
                renderer->renderGizmoLines(
                    reinterpret_cast<const float*>(gizmoData.lines.data()),
                    static_cast<uint32_t>(gizmoData.lines.size()));
            }
        }
    }
}

inline void EditModeHandler::renderMeshWireframe(Entity* entity, const float* wireColor) {
    if (!entity || !entity->hasModel || !m_ctx || !m_ctx->renderer) return;
    
    auto* renderer = m_ctx->renderer;
    
    for (size_t meshIdx = 0; meshIdx < entity->model.meshes.size(); ++meshIdx) {
        const auto& gpuMesh = entity->model.meshes[meshIdx];
        if (gpuMesh.hasOriginalEdges) {
            if (gpuMesh.hasSkinning && entity->hasSkeleton()) {
                Mat4 boneMatrices[MAX_BONES];
                entity->getSkinningMatrices(boneMatrices);
                renderer->renderOriginalEdgesSkinned(
                    entity->model, static_cast<int>(meshIdx),
                    entity->worldMatrix.m, wireColor,
                    reinterpret_cast<const float*>(boneMatrices),
                    gpuMesh.skinnedVertices);
            } else {
                renderer->renderOriginalEdges(
                    entity->model, static_cast<int>(meshIdx),
                    entity->worldMatrix.m, wireColor);
            }
        } else {
            renderer->renderMeshWireframeOverlay(
                entity->model, entity->worldMatrix.m,
                static_cast<int>(meshIdx), wireColor);
        }
    }
}

inline void EditModeHandler::renderSelectedOverlay(Entity* entity) {
    if (!entity || !entity->hasModel || !m_ctx || !m_ctx->renderer) return;
    if (!m_editMesh || m_selectedMeshIndex < 0) return;
    
    auto* renderer = m_ctx->renderer;
    float wireColor[4] = {0.4f, 0.5f, 0.6f, 1.0f};
    
    // GPU-side rendering: use model's vertex buffer (already synced with EditMesh
    // via updateMeshVerticesFromEditMesh) and pre-built edge index buffer.
    // Zero CPU per-vertex work — GPU vertex shader does all transformation.
    bool depthTest = !m_xRayMode;
    renderer->renderOriginalEdges(entity->model, m_selectedMeshIndex,
                                  entity->worldMatrix.m, wireColor, depthTest);
}

inline void EditModeHandler::renderUI() {
    // Sub-module UI
    switch (m_subMode) {
        case EditSubMode::Mesh:
            m_meshEdit->renderUI();
            break;
        case EditSubMode::UV:
            m_uvEdit->renderUI();
            break;
        case EditSubMode::Material:
            m_materialEdit->renderUI();
            break;
    }
    
    // Render selection highlights (vertices, edges, faces) as ImGui overlay
    renderSelectionHighlights();
}

inline void EditModeHandler::renderSelectionHighlights() {
    if (!m_active || !m_editMesh || !m_ctx) return;
    
    auto* selectedEntity = m_ctx->scene ? m_ctx->scene->getSelectedEntity() : nullptr;
    if (!selectedEntity) return;
    
    const float* worldMatrix = selectedEntity->worldMatrix.m;
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Helper lambda for projection
    auto projectVertex = [this, worldMatrix](const float* pos, float& sx, float& sy) -> bool {
        float wx = pos[0], wy = pos[1], wz = pos[2];
        if (worldMatrix) {
            float twx = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
            float twy = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
            float twz = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
            wx = twx; wy = twy; wz = twz;
        }
        return projectToScreen(wx, wy, wz, sx, sy);
    };
    
    // ===== Selected Faces =====
    if (m_selectionMode == SelectionMode::Face && !m_editMesh->selectedFaces.empty()) {
        ImU32 faceColor = IM_COL32(255, 140, 0, 80);
        ImU32 edgeColor = IM_COL32(255, 160, 0, 200);
        
        for (uint32_t fi : m_editMesh->selectedFaces) {
            if (fi >= m_editMesh->faces.size()) continue;
            const auto& face = m_editMesh->faces[fi];
            if (face.loops.size() < 3) continue;
            
            std::vector<ImVec2> screenPts;
            bool allVisible = true;
            for (const auto& loop : face.loops) {
                if (loop.vertexIndex >= m_editMesh->vertices.size()) { allVisible = false; break; }
                float sx, sy;
                if (projectVertex(m_editMesh->vertices[loop.vertexIndex].position, sx, sy)) {
                    screenPts.push_back(ImVec2(sx, sy));
                } else { allVisible = false; break; }
            }
            
            if (allVisible && screenPts.size() >= 3) {
                // Fan triangulation for fill
                for (size_t i = 1; i < screenPts.size() - 1; ++i) {
                    drawList->AddTriangleFilled(screenPts[0], screenPts[i], screenPts[i+1], faceColor);
                }
                // Edge outline
                for (size_t i = 0; i < screenPts.size(); ++i) {
                    drawList->AddLine(screenPts[i], screenPts[(i+1) % screenPts.size()], edgeColor, 2.0f);
                }
            }
        }
    }
    
    // ===== Selected Edges =====
    if (m_selectionMode == SelectionMode::Edge && !m_editMesh->selectedEdges.empty()) {
        ImU32 edgeColor = IM_COL32(255, 140, 0, 255);
        
        for (uint32_t ei : m_editMesh->selectedEdges) {
            if (ei >= m_editMesh->edges.size()) continue;
            const auto& edge = m_editMesh->edges[ei];
            if (edge.v0 >= m_editMesh->vertices.size() || edge.v1 >= m_editMesh->vertices.size()) continue;
            
            float sx0, sy0, sx1, sy1;
            if (projectVertex(m_editMesh->vertices[edge.v0].position, sx0, sy0) &&
                projectVertex(m_editMesh->vertices[edge.v1].position, sx1, sy1)) {
                drawList->AddLine(ImVec2(sx0, sy0), ImVec2(sx1, sy1), edgeColor, 4.0f);
            }
        }
    }
    
    // ===== Vertices (all + selected highlight) =====
    if (m_selectionMode == SelectionMode::Vertex && m_showVertices) {
        ImU32 normalColor = IM_COL32(30, 30, 30, 255);
        ImU32 selectedColor = IM_COL32(255, 140, 0, 255);
        ImU32 selectedFill = IM_COL32(255, 180, 50, 220);
        
        // Draw unselected vertices first
        for (size_t vi = 0; vi < m_editMesh->vertices.size(); ++vi) {
            if (m_editMesh->selectedVertices.count(static_cast<uint32_t>(vi)) > 0) continue;
            float sx, sy;
            if (projectVertex(m_editMesh->vertices[vi].position, sx, sy)) {
                drawList->AddCircleFilled(ImVec2(sx, sy), 2.0f, normalColor, 6);
            }
        }
        
        // Draw selected vertices on top
        for (uint32_t vi : m_editMesh->selectedVertices) {
            if (vi >= m_editMesh->vertices.size()) continue;
            float sx, sy;
            if (projectVertex(m_editMesh->vertices[vi].position, sx, sy)) {
                drawList->AddCircleFilled(ImVec2(sx, sy), 4.0f, selectedFill, 8);
                drawList->AddCircle(ImVec2(sx, sy), 4.0f, selectedColor, 8, 1.5f);
            }
        }
    }
}

inline bool EditModeHandler::handleInput(const InputEvent& event) {
    if (!m_active || !m_ctx) return false;
    
    // Handle gizmo interaction first (only when transform tool is active)
    if (m_showGizmo && m_meshGizmo.hasSelection() && m_rayCallback) {
        Vec3 camPos(m_ctx->cameraPos[0], m_ctx->cameraPos[1], m_ctx->cameraPos[2]);
        float screenScale = MeshEditGizmo::calculateScreenScale(
            m_meshGizmo.getSelectionCenter(), camPos,
            100.0f, static_cast<float>(m_ctx->windowHeight), m_ctx->fovY);
        
        if (event.type == InputEvent::Type::MouseDown && event.key == 0 && !event.isAlt()) {
            // Try to begin gizmo drag
            luma::Ray ray = m_rayCallback(event.mouseX, event.mouseY);
            
            if (m_extrudeMode) {
                // Extrude mode: first perform extrude, then begin drag on new geometry
                GizmoAxis axis = m_meshGizmo.testHover(ray, screenScale);
                if (axis != GizmoAxis::None) {
                    // Extrude creates geometry and pushes its own undo
                    performExtrude();
                    // Now begin drag on the newly extruded geometry
                    m_meshGizmo.setEditMesh(m_editMesh);
                    m_meshGizmo.beginDrag(ray, screenScale);
                    return true;
                }
            } else {
                if (m_meshGizmo.beginDrag(ray, screenScale)) {
                    return true;
                }
            }
        }
        else if (event.type == InputEvent::Type::MouseMove && m_meshGizmo.isDragging()) {
            luma::Ray ray = m_rayCallback(event.mouseX, event.mouseY);
            m_meshGizmo.updateDrag(ray);
            // Real-time GPU mesh update (like Maya/Blender - mesh deforms as you drag)
            syncEditMeshToGPU();
            m_dirty = true;
            return true;
        }
        else if (event.type == InputEvent::Type::MouseUp && event.key == 0 && m_meshGizmo.isDragging()) {
            m_meshGizmo.endDrag();
            // Final GPU sync
            syncEditMeshToGPU();
            // Trigger mesh update callback
            if (m_meshChangedCallback) {
                m_meshChangedCallback();
            }
            // Update extrude normal for next extrude
            if (m_extrudeMode) {
                updateExtrudeNormal();
            }
            return true;
        }
        else if (event.type == InputEvent::Type::MouseMove && !m_meshGizmo.isDragging()) {
            // Update hover state
            luma::Ray ray = m_rayCallback(event.mouseX, event.mouseY);
            m_meshGizmo.testHover(ray, screenScale);
        }
    }
    
    // First, try to handle with the active sub-module
    bool handled = false;
    switch (m_subMode) {
        case EditSubMode::Mesh:
            handled = m_meshEdit->handleInput(event);
            break;
        case EditSubMode::UV:
            handled = m_uvEdit->handleInput(event);
            break;
        case EditSubMode::Material:
            handled = m_materialEdit->handleInput(event);
            break;
    }
    
    if (handled) return true;
    
    // Handle Edit mode specific input
    switch (event.type) {
        case InputEvent::Type::KeyDown:
            // Selection mode shortcuts (1=Vertex, 2=Edge, 3=Face)
            if (event.key == '1' && !event.isCtrl()) {
                setSelectionMode(SelectionMode::Vertex);
                return true;
            }
            if (event.key == '2' && !event.isCtrl()) {
                setSelectionMode(SelectionMode::Edge);
                return true;
            }
            if (event.key == '3' && !event.isCtrl()) {
                setSelectionMode(SelectionMode::Face);
                return true;
            }
            // Note: Tool shortcuts (G=Move, R=Rotate, S=Scale, Q=Select, E=Extrude)
            // are handled by the toolbar via ImGui key detection
            break;
            
        case InputEvent::Type::MouseDown:
            if (event.key == 0) { // Left mouse
                // Start selection
                if (m_selectTool != SelectionTool::Click) {
                    m_isSelecting = true;
                    m_selectStartX = event.mouseX;
                    m_selectStartY = event.mouseY;
                    m_selectEndX = event.mouseX;
                    m_selectEndY = event.mouseY;
                    return true;
                }
            }
            break;
            
        case InputEvent::Type::MouseMove:
            if (m_isSelecting) {
                m_selectEndX = event.mouseX;
                m_selectEndY = event.mouseY;
                return true;
            }
            break;
            
        case InputEvent::Type::MouseUp:
            if (event.key == 0) { // Left mouse
                if (m_isSelecting) {
                    // Finish box selection
                    handleBoxSelection(m_selectStartX, m_selectStartY, 
                                      m_selectEndX, m_selectEndY, event.isShift());
                    m_isSelecting = false;
                    return true;
                } else if (m_selectTool == SelectionTool::Click) {
                    // Click selection
                    handleMeshPicking(event.mouseX, event.mouseY, event.isShift());
                    return true;
                }
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

inline void EditModeHandler::setEditMesh(EditMesh* mesh) {
    m_editMesh = mesh;
    m_meshEdit->setEditMesh(mesh);
    m_uvEdit->setEditMesh(mesh);
    m_materialEdit->setEditMesh(mesh);
}

inline void EditModeHandler::setSubMode(EditSubMode subMode) {
    if (m_subMode == subMode) return;
    
    // Exit current sub-mode
    switch (m_subMode) {
        case EditSubMode::Mesh: m_meshEdit->onExit(); break;
        case EditSubMode::UV: m_uvEdit->onExit(); break;
        case EditSubMode::Material: m_materialEdit->onExit(); break;
    }
    
    m_subMode = subMode;
    
    // Enter new sub-mode
    switch (m_subMode) {
        case EditSubMode::Mesh: m_meshEdit->onEnter(); break;
        case EditSubMode::UV: m_uvEdit->onEnter(); break;
        case EditSubMode::Material: m_materialEdit->onEnter(); break;
    }
}

inline void EditModeHandler::setSelectionMode(SelectionMode mode) {
    m_selectionMode = mode;
    m_meshEdit->setSelectionMode(mode);
}

inline bool EditModeHandler::projectToScreen(float wx, float wy, float wz, float& sx, float& sy) {
    if (m_projectionCallback) {
        return m_projectionCallback(wx, wy, wz, sx, sy);
    }
    return false;
}

inline void EditModeHandler::handleMeshPicking(float mouseX, float mouseY, bool additive) {
    if (!m_editMesh || !m_rayCallback || !m_ctx || !m_ctx->scene) return;
    
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (!selectedEntity) return;
    
    // Get luma::Ray from callback and convert to luma::editor::Ray for MeshPicker
    luma::Ray lumaRay = m_rayCallback(mouseX, mouseY);
    Ray ray;
    ray.origin[0] = lumaRay.origin.x;
    ray.origin[1] = lumaRay.origin.y;
    ray.origin[2] = lumaRay.origin.z;
    ray.direction[0] = lumaRay.direction.x;
    ray.direction[1] = lumaRay.direction.y;
    ray.direction[2] = lumaRay.direction.z;
    
    MeshPicker picker;
    auto result = picker.pick(ray, *m_editMesh, m_selectionMode, selectedEntity->worldMatrix.m);
    
    if (result.hit()) {
        if (!additive) {
            m_editMesh->selectNone();
        }
        
        switch (m_selectionMode) {
            case SelectionMode::Vertex:
                m_editMesh->selectedVertices.insert(result.index);
                break;
            case SelectionMode::Edge:
                m_editMesh->selectedEdges.insert(result.index);
                break;
            case SelectionMode::Face:
                m_editMesh->selectedFaces.insert(result.index);
                break;
        }
        
        if (m_meshChangedCallback) {
            m_meshChangedCallback();
        }
    } else if (!additive) {
        m_editMesh->selectNone();
    }
}

inline void EditModeHandler::handleBoxSelection(float x1, float y1, float x2, float y2, bool additive) {
    if (!m_editMesh || !m_ctx || !m_ctx->scene) return;
    
    // Delegate to MeshEditModule which has full X-Ray aware selection
    m_meshEdit->performBoxSelection(x1, y1, x2, y2, additive);
    
    if (m_meshChangedCallback) {
        m_meshChangedCallback();
    }
}

inline void EditModeHandler::rebuildGPUEdgeIndexBuffer() {
    if (!m_editMesh || !m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    if (m_selectedMeshIndex < 0) return;
    
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (!selectedEntity || !selectedEntity->hasModel) return;
    
    // Rebuild GPU edge index buffer from current EditMesh edges
    m_ctx->renderer->rebuildEdgeIndexBuffer(
        selectedEntity->model, m_selectedMeshIndex, *m_editMesh);
}

inline void EditModeHandler::syncEditMeshToGPU() {
    if (!m_editMesh || !m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    if (m_selectedMeshIndex < 0) return;
    
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (!selectedEntity || !selectedEntity->hasModel) return;
    if (m_selectedMeshIndex >= static_cast<int>(selectedEntity->model.meshes.size())) return;
    
    // Update GPU vertex buffer positions
    m_ctx->renderer->updateMeshVerticesFromEditMesh(
        selectedEntity->model, m_selectedMeshIndex, *m_editMesh);
    
    // Also update skinnedVertices (used by skinned mesh wireframe rendering)
    auto& gpuMesh = selectedEntity->model.meshes[m_selectedMeshIndex];
    if (gpuMesh.hasSkinning && !gpuMesh.skinnedVertices.empty()) {
        uint32_t count = std::min(
            static_cast<uint32_t>(gpuMesh.skinnedVertices.size()),
            static_cast<uint32_t>(m_editMesh->vertices.size()));
        for (uint32_t i = 0; i < count; i++) {
            gpuMesh.skinnedVertices[i].position[0] = m_editMesh->vertices[i].position[0];
            gpuMesh.skinnedVertices[i].position[1] = m_editMesh->vertices[i].position[1];
            gpuMesh.skinnedVertices[i].position[2] = m_editMesh->vertices[i].position[2];
        }
    }
}

} // namespace editor
} // namespace luma
