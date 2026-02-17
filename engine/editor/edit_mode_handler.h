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

// Deferred action for edit mode (processed at start of frame to avoid GPU sync issues)
enum class EditModeAction {
    None,
    SaveAndExit,    // Keep current state, exit edit mode
    CancelAndExit   // Restore baseline, exit edit mode
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
    
    // Undo/Redo for mesh editing operations
    bool undoMeshEdit();
    bool redoMeshEdit();
    
    // ===== History Baseline System (Maya-like Freeze History) =====
    
    // saveBaseline(): Snapshot current GPU + EditMesh state as the committed baseline.
    //   Called when entering edit mode and after commitChanges().
    void saveBaseline();
    
    // commitChanges(): "Freeze history" — saves current state as new baseline,
    //   clears undo/redo stacks. Future undo/cancel will only go back to this point.
    void commitChanges();
    
    // Whether there are uncommitted changes (edits since last commit/baseline)
    bool hasUncommittedChanges() const;
    
    // ===== Deferred Actions =====
    // These set flags processed at frame start by processPendingActions().
    // This avoids GPU sync issues (destroying buffers while draw calls are in-flight).
    
    // Request save & exit: keep current mesh state, exit to Scene mode
    void requestSaveAndExit() { m_pendingAction = EditModeAction::SaveAndExit; }
    
    // Request cancel: restore baseline GPU/mesh state, exit to Scene mode
    void requestCancel() { m_pendingAction = EditModeAction::CancelAndExit; }
    
    // Process deferred actions. Call at START of frame, before any draw commands.
    // Returns the action that was processed. Caller handles mode switching if != None.
    EditModeAction processPendingActions();
    
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
        
        // 4) Update toolbar selection counts for UI hints
        if (m_editMesh) {
            toolbar.selectedFaceCount = static_cast<int>(m_editMesh->selectedFaces.size());
            toolbar.selectedEdgeCount = static_cast<int>(m_editMesh->selectedEdges.size());
            toolbar.selectedVertexCount = static_cast<int>(m_editMesh->selectedVertices.size());
        }
        
        // 5a) Track tool changes and reset interactive state
        if (toolbar.currentTool != m_currentEditTool) {
            resetToolState();
            m_currentEditTool = toolbar.currentTool;
        }
        
        // 5b) Handle geometry tool "Apply" actions (from sidebar buttons)
        if (toolbar.applyRequested) {
            toolbar.applyRequested = false;
            using ET = EditModeToolbar::EditTool;
            switch (toolbar.currentTool) {
                case ET::LoopCut:
                    performLoopCut(toolbar.loopCutFactor);
                    break;
                case ET::Cut:
                    performCut();
                    break;
                case ET::MergeFaces:
                    performMergeFaces();
                    break;
                case ET::MergeByDistance:
                    performMergeByDistance(toolbar.mergeDistance);
                    break;
                default: break;
            }
        }
        
        // 6) Edit tool → gizmo mode
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
            // Geometry tools — these are "action" tools (immediate operation, no gizmo)
            case ET::LoopCut:
            case ET::Cut:
            case ET::MergeFaces:
            case ET::MergeByDistance:
                setShowGizmo(false);
                setExtrudeMode(false);
                break;
            case ET::Select:
            default:
                setShowGizmo(false);
                setExtrudeMode(false);
                break;
        }
    }
    
    // =========================================================================
    // Geometry Operations — triggered from toolbar buttons
    // =========================================================================
    
    // Loop Cut: insert edge loop through a specific edge (or selected edge as fallback)
    bool performLoopCut(float factor = 0.5f, uint32_t edgeIdx = UINT32_MAX) {
        if (!m_editMesh) { printf("[LoopCut] No editMesh!\n"); return false; }
        
        // Use provided edge, or fall back to selected edge
        if (edgeIdx == UINT32_MAX) {
            if (m_editMesh->selectedEdges.empty()) { printf("[LoopCut] No edge!\n"); return false; }
            edgeIdx = *m_editMesh->selectedEdges.begin();
        }
        
        printf("[LoopCut] Applying: edge=%u, factor=%.3f, totalEdges=%zu\n",
               edgeIdx, factor, m_editMesh->edges.size());
        
        std::vector<uint32_t> newVerts = m_editMesh->insertEdgeLoop(edgeIdx, factor);
        
        if (newVerts.empty()) { printf("[LoopCut] insertEdgeLoop returned 0 verts!\n"); return false; }
        
        printf("[LoopCut] Success: %zu new vertices created\n", newVerts.size());
        syncEditMeshToGPU();
        rebuildGPUEdgeIndexBuffer();
        m_dirty = true;
        return true;
    }
    
    // Apply the knife tool cut (using collected cut points)
    bool applyKnifeCut() {
        if (!m_editMesh || m_knife.points.size() < 2) return false;
        
        // For now, use the first two cut points
        auto& p0 = m_knife.points[0];
        auto& p1 = m_knife.points[1];
        
        // Find which face contains both edges
        // Build edge-to-face adjacency
        for (uint32_t fi = 0; fi < static_cast<uint32_t>(m_editMesh->faces.size()); fi++) {
            const auto& face = m_editMesh->faces[fi];
            bool hasEdge0 = false, hasEdge1 = false;
            int loopIdx0 = -1, loopIdx1 = -1;
            
            for (int i = 0; i < static_cast<int>(face.loops.size()); i++) {
                int next = (i + 1) % face.loops.size();
                uint32_t a = face.loops[i].vertexIndex;
                uint32_t b = face.loops[next].vertexIndex;
                
                auto key = std::make_pair(std::min(a,b), std::max(a,b));
                auto key0 = std::make_pair(
                    std::min(m_editMesh->edges[p0.edgeIndex].v0, m_editMesh->edges[p0.edgeIndex].v1),
                    std::max(m_editMesh->edges[p0.edgeIndex].v0, m_editMesh->edges[p0.edgeIndex].v1));
                auto key1 = std::make_pair(
                    std::min(m_editMesh->edges[p1.edgeIndex].v0, m_editMesh->edges[p1.edgeIndex].v1),
                    std::max(m_editMesh->edges[p1.edgeIndex].v0, m_editMesh->edges[p1.edgeIndex].v1));
                
                if (key == key0) { hasEdge0 = true; loopIdx0 = i; }
                if (key == key1) { hasEdge1 = true; loopIdx1 = i; }
            }
            
            if (hasEdge0 && hasEdge1 && loopIdx0 != loopIdx1) {
                // Found the face — cut it
                if (m_editMesh->cutFaceOnEdges(fi, loopIdx0, p0.t, loopIdx1, p1.t)) {
                    syncEditMeshToGPU();
                    rebuildGPUEdgeIndexBuffer();
                    m_dirty = true;
                    
                    // Reset knife state for next cut
                    m_knife.points.clear();
                    m_knife.active = false;
                    return true;
                }
            }
        }
        
        // Cut points aren't on the same face — reset
        m_knife.points.clear();
        m_knife.active = false;
        return false;
    }
    
    // Cut: split a selected face by connecting two selected vertices
    bool performCut() {
        if (!m_editMesh) return false;
        
        // Need exactly one face selected and at least two vertices selected
        if (m_editMesh->selectedFaces.size() != 1) return false;
        if (m_editMesh->selectedVertices.size() < 2) return false;
        
        uint32_t faceIdx = *m_editMesh->selectedFaces.begin();
        
        // Use the first two selected vertices
        auto it = m_editMesh->selectedVertices.begin();
        uint32_t v0 = *it++;
        uint32_t v1 = *it;
        
        if (!m_editMesh->splitFace(faceIdx, v0, v1)) return false;
        
        syncEditMeshToGPU();
        rebuildGPUEdgeIndexBuffer();
        m_dirty = true;
        return true;
    }
    
    // Merge Faces: combine selected adjacent faces into one polygon
    bool performMergeFaces() {
        if (!m_editMesh) return false;
        if (m_editMesh->selectedFaces.size() < 2) return false;
        
        if (!m_editMesh->mergeSelectedFaces()) return false;
        
        syncEditMeshToGPU();
        rebuildGPUEdgeIndexBuffer();
        m_dirty = true;
        return true;
    }
    
    // Merge by Distance: merge nearby vertices
    bool performMergeByDistance(float threshold) {
        if (!m_editMesh) return false;
        
        m_editMesh->mergeByDistance(threshold);
        
        syncEditMeshToGPU();
        rebuildGPUEdgeIndexBuffer();
        m_dirty = true;
        return true;
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
    
    // Baseline state (for Cancel / Commit)
    MeshGPUBackup m_baselineGPU;              // GPU vertex/index data at commit point
    EditMeshSnapshot m_baselineEditMesh;       // EditMesh state at commit point
    bool m_hasBaseline = false;               // Whether a baseline has been saved
    
    // Deferred action (set by UI, processed at frame start)
    EditModeAction m_pendingAction = EditModeAction::None;
    
    // Internal: restore baseline GPU + EditMesh state
    void cancelChanges();
    
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
    
    // Current tool (for interactive tools like LoopCut, Knife)
    EditModeToolbar::EditTool m_currentEditTool = EditModeToolbar::EditTool::Select;
    
    // ===== Interactive Tool State =====
    
    // Loop Cut tool (Blender Ctrl+R style)
    struct LoopCutState {
        enum class Phase { Hover, Slide };  // Hover=picking edge, Slide=adjusting factor
        Phase phase = Phase::Hover;
        uint32_t hoverEdge = UINT32_MAX;    // Edge under mouse cursor
        uint32_t confirmedEdge = UINT32_MAX;// Edge confirmed for cutting
        float factor = 0.5f;                // Current slide position (0-1)
        float slideStartY = 0.0f;           // Mouse Y when slide started
        std::vector<EditMesh::LoopCutSegment> previewSegments; // Current preview
    } m_loopCut;
    
    // Knife/Cut tool (Blender K style)
    struct KnifeCutState {
        bool active = false;                // Whether knife mode is active
        struct CutPoint {
            uint32_t edgeIndex;             // Which edge this point is on
            float t;                        // Parametric position along edge (0-1)
            float worldPos[3];              // 3D position of cut point
        };
        std::vector<CutPoint> points;       // Placed cut points
        float hoverPos[3] = {0,0,0};       // Current hover position (for preview line)
        uint32_t hoverEdge = UINT32_MAX;    // Edge under cursor
        float hoverT = 0.5f;               // T value on hovered edge
        bool hasHover = false;              // Whether we have a valid hover point
    } m_knife;
    
    // Sync EditMesh vertex positions back to GPU vertex buffer (real-time deform)
    // Public so undo/redo in main.cpp can also trigger it
    public:
    void syncEditMeshToGPU();
    void rebuildGPUEdgeIndexBuffer();
    // Force complete GPU rebuild from EditMesh loop data (for undo/redo where vertex
    // ordering may change — old GPU data cannot be preserved)
    void forceFullGPURebuild();
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
    void renderLoopCutPreview();   // Draw loop cut preview lines
    void renderKnifePreview();     // Draw knife tool cut path
    bool handleLoopCutInput(const InputEvent& event);  // Loop cut tool interaction
    bool handleKnifeInput(const InputEvent& event);     // Knife tool interaction
    void resetToolState();         // Reset interactive tool state when switching tools
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
    
    // Render interactive tool previews
    renderLoopCutPreview();
    renderKnifePreview();
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
                // Fill polygon without visible triangulation lines
                drawList->AddConvexPolyFilled(screenPts.data(), static_cast<int>(screenPts.size()), faceColor);
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

inline void EditModeHandler::renderLoopCutPreview() {
    if (m_currentEditTool != EditModeToolbar::EditTool::LoopCut) return;
    if (!m_active || !m_editMesh || !m_ctx || m_loopCut.previewSegments.empty()) return;
    
    auto* selectedEntity = m_ctx->scene ? m_ctx->scene->getSelectedEntity() : nullptr;
    if (!selectedEntity) return;
    const float* wm = selectedEntity->worldMatrix.m;
    
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    // Compute view direction for depth culling (camera → scene center)
    float viewDir[3];
    if (m_ctx->cameraPos && m_ctx->sceneCenter) {
        viewDir[0] = m_ctx->sceneCenter[0] - m_ctx->cameraPos[0];
        viewDir[1] = m_ctx->sceneCenter[1] - m_ctx->cameraPos[1];
        viewDir[2] = m_ctx->sceneCenter[2] - m_ctx->cameraPos[2];
        float len = sqrtf(viewDir[0]*viewDir[0] + viewDir[1]*viewDir[1] + viewDir[2]*viewDir[2]);
        if (len > 0.0001f) { viewDir[0]/=len; viewDir[1]/=len; viewDir[2]/=len; }
    } else {
        viewDir[0] = 0; viewDir[1] = 0; viewDir[2] = 1;
    }
    
    // Colors - Blender style
    ImU32 lineColor = (m_loopCut.phase == LoopCutState::Phase::Slide) 
        ? IM_COL32(255, 200, 0, 255)   // Bright yellow during slide
        : IM_COL32(255, 220, 50, 200); // Softer yellow during hover
    float lineWidth = (m_loopCut.phase == LoopCutState::Phase::Slide) ? 3.0f : 2.0f;
    
    for (const auto& seg : m_loopCut.previewSegments) {
        // Depth cull: skip segments on back-facing faces
        if (seg.faceIdx < m_editMesh->faces.size()) {
            const auto& face = m_editMesh->faces[seg.faceIdx];
            if (face.loops.size() >= 3) {
                // Compute face normal in world space
                const float* v0p = m_editMesh->vertices[face.loops[0].vertexIndex].position;
                const float* v1p = m_editMesh->vertices[face.loops[1].vertexIndex].position;
                const float* v2p = m_editMesh->vertices[face.loops[2].vertexIndex].position;
                
                // Edges in local space
                float e1[3] = { v1p[0]-v0p[0], v1p[1]-v0p[1], v1p[2]-v0p[2] };
                float e2[3] = { v2p[0]-v0p[0], v2p[1]-v0p[1], v2p[2]-v0p[2] };
                // Cross product (local normal)
                float ln[3] = {
                    e1[1]*e2[2] - e1[2]*e2[1],
                    e1[2]*e2[0] - e1[0]*e2[2],
                    e1[0]*e2[1] - e1[1]*e2[0]
                };
                // Transform normal to world space (rotation part of world matrix)
                float wn[3] = {
                    wm[0]*ln[0] + wm[4]*ln[1] + wm[8]*ln[2],
                    wm[1]*ln[0] + wm[5]*ln[1] + wm[9]*ln[2],
                    wm[2]*ln[0] + wm[6]*ln[1] + wm[10]*ln[2]
                };
                float dot = wn[0]*viewDir[0] + wn[1]*viewDir[1] + wn[2]*viewDir[2];
                if (dot < 0.0f) continue; // Back-facing — skip
            }
        }
        
        // Transform to world space
        float w0[3], w1[3];
        w0[0] = wm[0]*seg.p0[0] + wm[4]*seg.p0[1] + wm[8]*seg.p0[2] + wm[12];
        w0[1] = wm[1]*seg.p0[0] + wm[5]*seg.p0[1] + wm[9]*seg.p0[2] + wm[13];
        w0[2] = wm[2]*seg.p0[0] + wm[6]*seg.p0[1] + wm[10]*seg.p0[2] + wm[14];
        w1[0] = wm[0]*seg.p1[0] + wm[4]*seg.p1[1] + wm[8]*seg.p1[2] + wm[12];
        w1[1] = wm[1]*seg.p1[0] + wm[5]*seg.p1[1] + wm[9]*seg.p1[2] + wm[13];
        w1[2] = wm[2]*seg.p1[0] + wm[6]*seg.p1[1] + wm[10]*seg.p1[2] + wm[14];
        
        float sx0, sy0, sx1, sy1;
        if (projectToScreen(w0[0], w0[1], w0[2], sx0, sy0) &&
            projectToScreen(w1[0], w1[1], w1[2], sx1, sy1)) {
            drawList->AddLine(ImVec2(sx0, sy0), ImVec2(sx1, sy1), lineColor, lineWidth);
        }
    }
    
    // During hover, highlight the edge under cursor
    if (m_loopCut.phase == LoopCutState::Phase::Hover && m_loopCut.hoverEdge != UINT32_MAX) {
        if (m_loopCut.hoverEdge < m_editMesh->edges.size()) {
            const auto& edge = m_editMesh->edges[m_loopCut.hoverEdge];
            if (edge.v0 < m_editMesh->vertices.size() && edge.v1 < m_editMesh->vertices.size()) {
                float w0[3], w1[3];
                const float* p0 = m_editMesh->vertices[edge.v0].position;
                const float* p1 = m_editMesh->vertices[edge.v1].position;
                w0[0] = wm[0]*p0[0] + wm[4]*p0[1] + wm[8]*p0[2] + wm[12];
                w0[1] = wm[1]*p0[0] + wm[5]*p0[1] + wm[9]*p0[2] + wm[13];
                w0[2] = wm[2]*p0[0] + wm[6]*p0[1] + wm[10]*p0[2] + wm[14];
                w1[0] = wm[0]*p1[0] + wm[4]*p1[1] + wm[8]*p1[2] + wm[12];
                w1[1] = wm[1]*p1[0] + wm[5]*p1[1] + wm[9]*p1[2] + wm[13];
                w1[2] = wm[2]*p1[0] + wm[6]*p1[1] + wm[10]*p1[2] + wm[14];
                
                float sx0, sy0, sx1, sy1;
                if (projectToScreen(w0[0], w0[1], w0[2], sx0, sy0) &&
                    projectToScreen(w1[0], w1[1], w1[2], sx1, sy1)) {
                    drawList->AddLine(ImVec2(sx0, sy0), ImVec2(sx1, sy1), 
                        IM_COL32(0, 220, 255, 255), 3.0f);
                }
            }
        }
    }
    
    // Status text
    if (m_loopCut.phase == LoopCutState::Phase::Hover) {
        drawList->AddText(ImVec2(10, ImGui::GetIO().DisplaySize.y - 30),
            IM_COL32(255, 220, 50, 255), "Loop Cut: Hover over edge, LMB to confirm | ESC to cancel");
    } else {
        char buf[128];
        snprintf(buf, sizeof(buf), "Loop Cut: Slide to adjust (%.0f%%) | LMB to apply | ESC to cancel",
            m_loopCut.factor * 100.0f);
        drawList->AddText(ImVec2(10, ImGui::GetIO().DisplaySize.y - 30),
            IM_COL32(255, 200, 0, 255), buf);
    }
}

inline void EditModeHandler::renderKnifePreview() {
    if (m_currentEditTool != EditModeToolbar::EditTool::Cut) return;
    if (!m_active || !m_editMesh || !m_ctx) return;
    
    auto* selectedEntity = m_ctx->scene ? m_ctx->scene->getSelectedEntity() : nullptr;
    if (!selectedEntity) return;
    const float* wm = selectedEntity->worldMatrix.m;
    
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    auto worldTransform = [wm](const float* p, float* out) {
        out[0] = wm[0]*p[0] + wm[4]*p[1] + wm[8]*p[2] + wm[12];
        out[1] = wm[1]*p[0] + wm[5]*p[1] + wm[9]*p[2] + wm[13];
        out[2] = wm[2]*p[0] + wm[6]*p[1] + wm[10]*p[2] + wm[14];
    };
    
    // Draw placed cut points
    ImU32 pointColor = IM_COL32(255, 50, 50, 255);
    ImU32 lineColor = IM_COL32(255, 80, 80, 220);
    ImU32 hoverPointColor = IM_COL32(255, 150, 50, 255);
    
    std::vector<ImVec2> screenPoints;
    for (const auto& pt : m_knife.points) {
        float w[3];
        worldTransform(pt.worldPos, w);
        float sx, sy;
        if (projectToScreen(w[0], w[1], w[2], sx, sy)) {
            screenPoints.push_back(ImVec2(sx, sy));
            // Red diamond for placed cut points
            drawList->AddCircleFilled(ImVec2(sx, sy), 5.0f, pointColor, 4);
            drawList->AddCircle(ImVec2(sx, sy), 5.0f, IM_COL32(255, 255, 255, 200), 4, 1.5f);
        }
    }
    
    // Draw lines between placed points
    for (size_t i = 1; i < screenPoints.size(); i++) {
        drawList->AddLine(screenPoints[i-1], screenPoints[i], lineColor, 2.5f);
    }
    
    // Draw hover point and preview line from last placed point
    if (m_knife.hasHover) {
        float w[3];
        worldTransform(m_knife.hoverPos, w);
        float sx, sy;
        if (projectToScreen(w[0], w[1], w[2], sx, sy)) {
            // Orange circle for hover position
            drawList->AddCircleFilled(ImVec2(sx, sy), 4.0f, hoverPointColor, 8);
            drawList->AddCircle(ImVec2(sx, sy), 6.0f, IM_COL32(255, 200, 100, 200), 8, 1.5f);
            
            // Dashed preview line from last point to hover
            if (!screenPoints.empty()) {
                ImU32 dashColor = IM_COL32(255, 120, 50, 180);
                ImVec2 from = screenPoints.back();
                ImVec2 to(sx, sy);
                // Simple dashed line
                float dx = to.x - from.x, dy = to.y - from.y;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 0) {
                    float dashLen = 8.0f, gapLen = 4.0f;
                    float nx = dx/len, ny = dy/len;
                    float d = 0;
                    while (d < len) {
                        float d1 = std::min(d + dashLen, len);
                        drawList->AddLine(
                            ImVec2(from.x + nx*d, from.y + ny*d),
                            ImVec2(from.x + nx*d1, from.y + ny*d1),
                            dashColor, 2.0f);
                        d = d1 + gapLen;
                    }
                }
            }
        }
    }
    
    // Highlight hovered edge in magenta
    if (m_knife.hoverEdge != UINT32_MAX && m_knife.hoverEdge < m_editMesh->edges.size()) {
        const auto& edge = m_editMesh->edges[m_knife.hoverEdge];
        if (edge.v0 < m_editMesh->vertices.size() && edge.v1 < m_editMesh->vertices.size()) {
            float w0[3], w1[3];
            worldTransform(m_editMesh->vertices[edge.v0].position, w0);
            worldTransform(m_editMesh->vertices[edge.v1].position, w1);
            float sx0, sy0, sx1, sy1;
            if (projectToScreen(w0[0], w0[1], w0[2], sx0, sy0) &&
                projectToScreen(w1[0], w1[1], w1[2], sx1, sy1)) {
                drawList->AddLine(ImVec2(sx0, sy0), ImVec2(sx1, sy1),
                    IM_COL32(255, 50, 200, 255), 2.5f);
            }
        }
    }
    
    // Status text (scissors icon ✂)
    char buf[128];
    snprintf(buf, sizeof(buf), "Knife: %d/%d points placed | LMB on edge to place | ESC to cancel",
        (int)m_knife.points.size(), 2);
    drawList->AddText(ImVec2(10, ImGui::GetIO().DisplaySize.y - 30),
        IM_COL32(255, 80, 80, 255), buf);
}

inline void EditModeHandler::resetToolState() {
    m_loopCut.phase = LoopCutState::Phase::Hover;
    m_loopCut.hoverEdge = UINT32_MAX;
    m_loopCut.confirmedEdge = UINT32_MAX;
    m_loopCut.previewSegments.clear();
    m_loopCut.factor = 0.5f;
    
    m_knife.active = false;
    m_knife.points.clear();
    m_knife.hoverEdge = UINT32_MAX;
    m_knife.hasHover = false;
}

inline bool EditModeHandler::handleLoopCutInput(const InputEvent& event) {
    if (!m_editMesh || !m_projectionCallback) return false;
    
    auto* selectedEntity = m_ctx->scene ? m_ctx->scene->getSelectedEntity() : nullptr;
    if (!selectedEntity) return false;
    const float* worldMatrix = selectedEntity->worldMatrix.m;
    
    auto projFunc = [this, worldMatrix](float wx, float wy, float wz, float& sx, float& sy) -> bool {
        return projectToScreen(wx, wy, wz, sx, sy);
    };
    
    // Transform projection to include world matrix
    auto projWithWorld = [this, worldMatrix](float lx, float ly, float lz, float& sx, float& sy) -> bool {
        float wx = worldMatrix[0]*lx + worldMatrix[4]*ly + worldMatrix[8]*lz + worldMatrix[12];
        float wy = worldMatrix[1]*lx + worldMatrix[5]*ly + worldMatrix[9]*lz + worldMatrix[13];
        float wz = worldMatrix[2]*lx + worldMatrix[6]*ly + worldMatrix[10]*lz + worldMatrix[14];
        return projectToScreen(wx, wy, wz, sx, sy);
    };
    
    // Compute camera forward direction for back-face culling
    // Use cameraPos toward sceneCenter as a reliable view direction
    float camFwd[3] = {
        m_ctx->sceneCenter[0] - m_ctx->cameraPos[0],
        m_ctx->sceneCenter[1] - m_ctx->cameraPos[1],
        m_ctx->sceneCenter[2] - m_ctx->cameraPos[2]
    };
    float fwdLen = std::sqrt(camFwd[0]*camFwd[0] + camFwd[1]*camFwd[1] + camFwd[2]*camFwd[2]);
    if (fwdLen > 1e-6f) { camFwd[0]/=fwdLen; camFwd[1]/=fwdLen; camFwd[2]/=fwdLen; }
    
    if (m_loopCut.phase == LoopCutState::Phase::Hover) {
        // Phase 1: Hover — find edge under cursor and show preview
        if (event.type == InputEvent::Type::MouseMove) {
            uint32_t edge = m_editMesh->findClosestEdgeToScreenPos(
                event.mouseX, event.mouseY, projWithWorld, worldMatrix, 20.0f, camFwd);
            
            if (edge != m_loopCut.hoverEdge) {
                m_loopCut.hoverEdge = edge;
                if (edge != UINT32_MAX) {
                    m_loopCut.previewSegments = m_editMesh->previewEdgeLoop(edge, 0.5f);
                } else {
                    m_loopCut.previewSegments.clear();
                }
            }
            return true; // Consume mouse move in loop cut mode
        }
        
        if (event.type == InputEvent::Type::MouseDown && event.key == 0 && !event.isAlt()) {
            if (m_loopCut.hoverEdge != UINT32_MAX && !m_loopCut.previewSegments.empty()) {
                // Confirm edge, enter slide phase
                m_loopCut.confirmedEdge = m_loopCut.hoverEdge;
                m_loopCut.phase = LoopCutState::Phase::Slide;
                m_loopCut.factor = 0.5f;
                m_loopCut.slideStartY = event.mouseY;
                return true;
            }
        }
        
        // Escape or RMB cancels
        if ((event.type == InputEvent::Type::KeyDown && event.key == 27) ||
            (event.type == InputEvent::Type::MouseDown && event.key == 1)) {
            resetToolState();
            return true;
        }
    }
    else if (m_loopCut.phase == LoopCutState::Phase::Slide) {
        // Phase 2: Slide — mouse Y adjusts factor (like Blender Ctrl+R)
        if (event.type == InputEvent::Type::MouseMove) {
            float deltaY = event.mouseY - m_loopCut.slideStartY;
            float sensitivity = 0.004f;
            m_loopCut.factor = std::max(0.01f, std::min(0.99f, 0.5f + deltaY * sensitivity));
            
            m_loopCut.previewSegments = m_editMesh->previewEdgeLoop(
                m_loopCut.confirmedEdge, m_loopCut.factor);
            return true;
        }
        
        // LMB applies the cut at current factor
        if (event.type == InputEvent::Type::MouseDown && event.key == 0 && !event.isAlt()) {
            performLoopCut(m_loopCut.factor, m_loopCut.confirmedEdge);
            
            m_loopCut.phase = LoopCutState::Phase::Hover;
            m_loopCut.confirmedEdge = UINT32_MAX;
            m_loopCut.hoverEdge = UINT32_MAX;
            m_loopCut.previewSegments.clear();
            return true;
        }
        
        // Enter applies at factor 0.5 (even cut, like Blender)
        if (event.type == InputEvent::Type::KeyDown && event.key == 13/*Enter*/) {
            performLoopCut(0.5f, m_loopCut.confirmedEdge);
            
            m_loopCut.phase = LoopCutState::Phase::Hover;
            m_loopCut.confirmedEdge = UINT32_MAX;
            m_loopCut.hoverEdge = UINT32_MAX;
            m_loopCut.previewSegments.clear();
            return true;
        }
        
        // RMB or Escape cancels slide (back to hover)
        if ((event.type == InputEvent::Type::KeyDown && event.key == 27) ||
            (event.type == InputEvent::Type::MouseDown && event.key == 1)) {
            m_loopCut.phase = LoopCutState::Phase::Hover;
            m_loopCut.confirmedEdge = UINT32_MAX;
            m_loopCut.previewSegments.clear();
            return true;
        }
    }
    
    return false;
}

inline bool EditModeHandler::handleKnifeInput(const InputEvent& event) {
    if (!m_editMesh || !m_projectionCallback) return false;
    
    auto* selectedEntity = m_ctx->scene ? m_ctx->scene->getSelectedEntity() : nullptr;
    if (!selectedEntity) return false;
    const float* worldMatrix = selectedEntity->worldMatrix.m;
    
    auto projWithWorld = [this, worldMatrix](float lx, float ly, float lz, float& sx, float& sy) -> bool {
        float wx = worldMatrix[0]*lx + worldMatrix[4]*ly + worldMatrix[8]*lz + worldMatrix[12];
        float wy = worldMatrix[1]*lx + worldMatrix[5]*ly + worldMatrix[9]*lz + worldMatrix[13];
        float wz = worldMatrix[2]*lx + worldMatrix[6]*ly + worldMatrix[10]*lz + worldMatrix[14];
        return projectToScreen(wx, wy, wz, sx, sy);
    };
    
    // Compute camera forward direction for back-face culling
    float camFwd[3] = {
        m_ctx->sceneCenter[0] - m_ctx->cameraPos[0],
        m_ctx->sceneCenter[1] - m_ctx->cameraPos[1],
        m_ctx->sceneCenter[2] - m_ctx->cameraPos[2]
    };
    float fwdLen = std::sqrt(camFwd[0]*camFwd[0] + camFwd[1]*camFwd[1] + camFwd[2]*camFwd[2]);
    if (fwdLen > 1e-6f) { camFwd[0]/=fwdLen; camFwd[1]/=fwdLen; camFwd[2]/=fwdLen; }
    
    if (event.type == InputEvent::Type::MouseMove) {
        // Update hover position — find closest edge
        float t;
        uint32_t edge = m_editMesh->findClosestEdgeWithT(
            event.mouseX, event.mouseY, projWithWorld, worldMatrix, t, 20.0f, camFwd);
        
        m_knife.hoverEdge = edge;
        m_knife.hoverT = t;
        
        if (edge != UINT32_MAX && edge < m_editMesh->edges.size()) {
            const auto& e = m_editMesh->edges[edge];
            if (e.v0 < m_editMesh->vertices.size() && e.v1 < m_editMesh->vertices.size()) {
                math::lerp3(m_knife.hoverPos,
                    m_editMesh->vertices[e.v0].position,
                    m_editMesh->vertices[e.v1].position, t);
                m_knife.hasHover = true;
            }
        } else {
            m_knife.hasHover = false;
        }
        return true;
    }
    
    if (event.type == InputEvent::Type::MouseDown && event.key == 0 && !event.isAlt()) {
        // Place cut point on the hovered edge
        if (m_knife.hoverEdge != UINT32_MAX && m_knife.hasHover) {
            KnifeCutState::CutPoint pt;
            pt.edgeIndex = m_knife.hoverEdge;
            pt.t = m_knife.hoverT;
            pt.worldPos[0] = m_knife.hoverPos[0];
            pt.worldPos[1] = m_knife.hoverPos[1];
            pt.worldPos[2] = m_knife.hoverPos[2];
            m_knife.points.push_back(pt);
            m_knife.active = true;
            
            // If we have 2 points, auto-apply the cut
            if (m_knife.points.size() >= 2) {
                applyKnifeCut();
            }
            return true;
        }
    }
    
    // Enter confirms (same as having 2 points)
    if (event.type == InputEvent::Type::KeyDown && event.key == 13/*Enter*/) {
        if (m_knife.points.size() >= 2) {
            applyKnifeCut();
        }
        return true;
    }
    
    // Escape or RMB cancels
    if ((event.type == InputEvent::Type::KeyDown && event.key == 27) ||
        (event.type == InputEvent::Type::MouseDown && event.key == 1)) {
        m_knife.points.clear();
        m_knife.active = false;
        return true;
    }
    
    return false;
}

inline bool EditModeHandler::handleInput(const InputEvent& event) {
    if (!m_active || !m_ctx) return false;
    
    // Handle interactive tool input FIRST (Loop Cut, Knife)
    // These tools override normal selection/gizmo interaction
    if (m_currentEditTool == EditModeToolbar::EditTool::LoopCut) {
        if (handleLoopCutInput(event)) return true;
    }
    if (m_currentEditTool == EditModeToolbar::EditTool::Cut) {
        if (handleKnifeInput(event)) return true;
    }
    
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
            if (event.key == 0 && !event.isAlt()) { // Left mouse, not Alt (Alt = camera orbit)
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
                } else if (m_selectTool == SelectionTool::Click && !event.isAlt()) {
                    // Click selection (not Alt — Alt = camera orbit)
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
    
    // Update GPU vertex buffer positions (automatically does full rebuild if topology changed)
    m_ctx->renderer->updateMeshVerticesFromEditMesh(
        selectedEntity->model, m_selectedMeshIndex, *m_editMesh);
    
    // For the fast path (position-only update), also update skinnedVertices
    // (rebuildMeshBuffersFromEditMesh already handles skinned vertices for topology changes)
    auto& gpuMesh = selectedEntity->model.meshes[m_selectedMeshIndex];
    if (gpuMesh.hasSkinning && gpuMesh.skinnedVertices.size() == m_editMesh->vertices.size()) {
        uint32_t count = static_cast<uint32_t>(m_editMesh->vertices.size());
        for (uint32_t i = 0; i < count; i++) {
            gpuMesh.skinnedVertices[i].position[0] = m_editMesh->vertices[i].position[0];
            gpuMesh.skinnedVertices[i].position[1] = m_editMesh->vertices[i].position[1];
            gpuMesh.skinnedVertices[i].position[2] = m_editMesh->vertices[i].position[2];
        }
    }
}

inline void EditModeHandler::forceFullGPURebuild() {
    if (!m_editMesh || !m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    if (m_selectedMeshIndex < 0) return;
    
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (!selectedEntity || !selectedEntity->hasModel) return;
    if (m_selectedMeshIndex >= static_cast<int>(selectedEntity->model.meshes.size())) return;
    
    // Force a complete rebuild of GPU buffers from EditMesh data.
    // Unlike syncEditMeshToGPU (which tries to preserve old GPU normals/UVs),
    // this always rebuilds from scratch using EditMesh loop data.
    // Required for undo/redo where vertex ordering may have changed.
    m_ctx->renderer->rebuildMeshBuffersFromEditMesh(
        selectedEntity->model, m_selectedMeshIndex, *m_editMesh, true /*forceFromLoops*/);
    
    // Update skinned vertices
    auto& gpuMesh = selectedEntity->model.meshes[m_selectedMeshIndex];
    if (gpuMesh.hasSkinning) {
        uint32_t count = static_cast<uint32_t>(m_editMesh->vertices.size());
        gpuMesh.skinnedVertices.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            gpuMesh.skinnedVertices[i].position[0] = m_editMesh->vertices[i].position[0];
            gpuMesh.skinnedVertices[i].position[1] = m_editMesh->vertices[i].position[1];
            gpuMesh.skinnedVertices[i].position[2] = m_editMesh->vertices[i].position[2];
        }
    }
}

inline bool EditModeHandler::undoMeshEdit() {
    if (!m_editMesh || !m_editMesh->canUndo()) return false;
    
    m_editMesh->undo();
    
    // Undo may change topology AND vertex ordering (removeUnusedVertices remaps indices).
    // We MUST force a full rebuild from EditMesh loop data — we cannot preserve old GPU
    // vertex attributes because they correspond to the post-edit vertex ordering, not the
    // restored pre-edit ordering.
    forceFullGPURebuild();
    
    // Update gizmo state
    m_meshGizmo.setEditMesh(m_editMesh);
    if (m_extrudeMode) updateExtrudeNormal();
    
    m_dirty = true;
    if (m_meshChangedCallback) m_meshChangedCallback();
    return true;
}

inline bool EditModeHandler::redoMeshEdit() {
    if (!m_editMesh || !m_editMesh->canRedo()) return false;
    
    m_editMesh->redo();
    
    // Same as undo — vertex ordering may differ, force full rebuild
    forceFullGPURebuild();
    
    // Update gizmo state
    m_meshGizmo.setEditMesh(m_editMesh);
    if (m_extrudeMode) updateExtrudeNormal();
    
    m_dirty = true;
    if (m_meshChangedCallback) m_meshChangedCallback();
    return true;
}

// ===== Baseline / Commit / Cancel =====

inline void EditModeHandler::saveBaseline() {
    if (!m_editMesh || !m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    if (m_selectedMeshIndex < 0) return;
    
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (!selectedEntity || !selectedEntity->hasModel) return;
    if (m_selectedMeshIndex >= static_cast<int>(selectedEntity->model.meshes.size())) return;
    
    // Save GPU data backup
    m_baselineGPU = m_ctx->renderer->backupMeshGPUData(selectedEntity->model, m_selectedMeshIndex);
    
    // Save EditMesh snapshot
    m_baselineEditMesh = m_editMesh->createSnapshot();
    
    m_hasBaseline = true;
    printf("[EditMode] Baseline saved (mesh %d)\n", m_selectedMeshIndex);
}

inline void EditModeHandler::commitChanges() {
    if (!m_editMesh) return;
    
    // Save current state as new baseline
    saveBaseline();
    
    // Clear undo/redo history — no going back past this point
    m_editMesh->clearUndoHistory();
    
    m_dirty = false;
    printf("[EditMode] Changes committed — history frozen\n");
}

inline void EditModeHandler::cancelChanges() {
    if (!m_editMesh || !m_hasBaseline) return;
    if (!m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    if (m_selectedMeshIndex < 0) return;
    
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (!selectedEntity || !selectedEntity->hasModel) return;
    if (m_selectedMeshIndex >= static_cast<int>(selectedEntity->model.meshes.size())) return;
    
    // Restore GPU buffers from baseline
    m_ctx->renderer->restoreMeshGPUData(selectedEntity->model, m_selectedMeshIndex, m_baselineGPU);
    
    // Restore EditMesh from baseline snapshot
    m_editMesh->restoreFromSnapshot(m_baselineEditMesh);
    
    // Update gizmo and state
    m_meshGizmo.setEditMesh(m_editMesh);
    m_extrudeMode = false;
    m_meshGizmo.setExtrudeMode(false);
    
    m_dirty = false;
    // Note: Do NOT call m_meshChangedCallback here — the caller will handle
    // mode switching and cleanup. Calling it could trigger side effects while
    // we're in a transitional state.
    printf("[EditMode] Changes cancelled — restored to baseline\n");
}

inline bool EditModeHandler::hasUncommittedChanges() const {
    if (!m_editMesh) return false;
    return m_editMesh->canUndo();
}

inline EditModeAction EditModeHandler::processPendingActions() {
    auto action = m_pendingAction;
    m_pendingAction = EditModeAction::None;
    
    if (action == EditModeAction::None) return action;
    
    if (action == EditModeAction::CancelAndExit) {
        // Restore baseline GPU buffers + EditMesh state
        // Safe to do here because we're at the start of the frame,
        // before any draw commands reference the current buffers.
        cancelChanges();
        printf("[EditMode] Cancel processed — baseline restored\n");
    } else if (action == EditModeAction::SaveAndExit) {
        // Keep current GPU state as-is
        printf("[EditMode] Save & exit processed\n");
    }
    
    m_dirty = false;
    return action;
}

} // namespace editor
} // namespace luma
