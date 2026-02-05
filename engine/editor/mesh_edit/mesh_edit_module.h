// LUMA Mesh Edit Module
// Modular mesh editing functionality (vertex/edge/face editing, transforms, modeling ops)

#pragma once

#include "engine/editor/edit_module.h"
#include "engine/mesh/edit_mesh.h"
#include "engine/renderer/unified_renderer.h"
#include <memory>
#include <functional>

namespace luma {
namespace editor {

// Tool types for mesh editing
enum class MeshTool {
    Select,     // Selection tool
    Move,       // Move/translate
    Rotate,     // Rotate
    Scale,      // Scale
    Extrude,    // Extrude faces
    Inset,      // Inset faces
    Bevel,      // Bevel edges/vertices
    LoopCut,    // Loop cut
    Knife,      // Knife tool
};

// Selection tool types
enum class SelectionTool {
    Click,      // Single click selection
    Box,        // Box/rectangle selection
    Circle,     // Circle selection
    Lasso,      // Lasso/freeform selection
};

// Mesh Edit Module
class MeshEditModule : public EditModule {
public:
    MeshEditModule();
    ~MeshEditModule() override;
    
    // Initialize with renderer and mesh
    bool init(UnifiedRenderer* renderer);
    void setEditMesh(EditMesh* mesh);
    EditMesh* getEditMesh() const { return editMesh; }
    
    // EditModule interface
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render(DrawManager& drawManager, const RenderContext& ctx) override;
    void renderUI() override;
    bool handleInput(const InputEvent& event) override;
    void clearSelection() override;
    void selectAll() override;
    
    // Selection mode
    void setSelectionMode(SelectionMode mode);
    SelectionMode getSelectionMode() const { return selectionMode; }
    
    // Tools
    void setCurrentTool(MeshTool tool);
    MeshTool getCurrentTool() const { return currentTool; }
    
    void setSelectionTool(SelectionTool tool);
    SelectionTool getSelectionTool() const { return selectionTool; }
    
    // Modeling operations
    void extrude(float distance = 0.0f);
    void subdivide();
    void deleteSelected();
    void duplicateSelected();
    void mergeVertices(float threshold = 0.001f);
    void flipNormals();
    void recalculateNormals();
    
    // Transform operations
    void beginTransform(MeshTool tool);
    void updateTransform(float dx, float dy, float dz);
    void endTransform(bool apply);
    bool isTransforming() const { return transforming; }
    
    // Snapping
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    void setSnapGrid(float grid) { snapGrid = grid; }
    bool isSnapEnabled() const { return snapEnabled; }
    float getSnapGrid() const { return snapGrid; }
    
    // Callbacks
    using MeshChangedCallback = std::function<void()>;
    void setMeshChangedCallback(MeshChangedCallback cb) { meshChangedCallback = cb; }
    
private:
    UnifiedRenderer* renderer = nullptr;
    EditMesh* editMesh = nullptr;
    
    // Current state
    SelectionMode selectionMode = SelectionMode::Vertex;
    MeshTool currentTool = MeshTool::Select;
    SelectionTool selectionTool = SelectionTool::Click;
    
    // Transform state
    bool transforming = false;
    MeshTool transformTool = MeshTool::Move;
    float transformPivot[3] = {0, 0, 0};
    float transformAccum[3] = {0, 0, 0};
    
    // Snapping
    bool snapEnabled = false;
    float snapGrid = 0.1f;
    
    // Selection state for box/circle/lasso
    bool selecting = false;
    float selectionStartX = 0, selectionStartY = 0;
    float selectionEndX = 0, selectionEndY = 0;
    std::vector<Vec2> lassoPoints;
    
    // Callbacks
    MeshChangedCallback meshChangedCallback;
    
    // Internal helpers
    void notifyMeshChanged();
    void calculateSelectionPivot();
    float snapValue(float value);
    
    // Selection helpers
    void performClickSelection(float x, float y, bool additive);
    void performBoxSelection(float x1, float y1, float x2, float y2, bool additive);
    void performCircleSelection(float cx, float cy, float radius, bool additive);
    void performLassoSelection(const std::vector<Vec2>& points, bool additive);
    
    // Rendering helpers
    void renderSelectionPreview(const RenderContext& ctx);
    void renderToolGizmo(const RenderContext& ctx);
};

// ============================================================================
// Implementation
// ============================================================================

inline MeshEditModule::MeshEditModule() 
    : EditModule("MeshEdit") {
}

inline MeshEditModule::~MeshEditModule() = default;

inline bool MeshEditModule::init(UnifiedRenderer* r) {
    renderer = r;
    return renderer != nullptr;
}

inline void MeshEditModule::setEditMesh(EditMesh* mesh) {
    editMesh = mesh;
}

inline void MeshEditModule::onEnter() {
    active = true;
    transforming = false;
    selecting = false;
}

inline void MeshEditModule::onExit() {
    if (transforming) {
        endTransform(false);  // Cancel any in-progress transform
    }
    active = false;
}

inline void MeshEditModule::update(float deltaTime) {
    // Update any continuous operations
    (void)deltaTime;
}

inline void MeshEditModule::render(DrawManager& drawManager, const RenderContext& ctx) {
    if (!editMesh || !renderer) return;
    
    // Selection preview (box, circle, lasso outline)
    if (selecting) {
        renderSelectionPreview(ctx);
    }
    
    // Tool gizmo
    renderToolGizmo(ctx);
}

inline void MeshEditModule::renderUI() {
    // ImGui UI for mesh editing tools
    // This would be called from the main UI code
}

inline bool MeshEditModule::handleInput(const InputEvent& event) {
    if (!editMesh) return false;
    
    switch (event.type) {
        case InputEvent::Type::KeyDown:
            // Handle keyboard shortcuts
            if (event.key == 'G' && !event.isCtrl()) {
                beginTransform(MeshTool::Move);
                return true;
            }
            if (event.key == 'R' && !event.isCtrl()) {
                beginTransform(MeshTool::Rotate);
                return true;
            }
            if (event.key == 'S' && !event.isCtrl()) {
                beginTransform(MeshTool::Scale);
                return true;
            }
            if (event.key == 'E' && !event.isCtrl()) {
                extrude();
                return true;
            }
            if (event.key == 'X' || event.key == 0x2E /* Delete */) {
                deleteSelected();
                return true;
            }
            if (event.key == 'A' && !event.isCtrl()) {
                selectAll();
                return true;
            }
            if (event.key == 0x1B /* Escape */) {
                if (transforming) {
                    endTransform(false);
                    return true;
                }
                if (selecting) {
                    selecting = false;
                    return true;
                }
                clearSelection();
                return true;
            }
            // Selection mode shortcuts
            if (event.key == '1') {
                setSelectionMode(SelectionMode::Vertex);
                return true;
            }
            if (event.key == '2') {
                setSelectionMode(SelectionMode::Edge);
                return true;
            }
            if (event.key == '3') {
                setSelectionMode(SelectionMode::Face);
                return true;
            }
            break;
            
        case InputEvent::Type::MouseDown:
            if (event.key == 0) {  // Left mouse
                if (currentTool == MeshTool::Select) {
                    if (selectionTool == SelectionTool::Click) {
                        performClickSelection(event.mouseX, event.mouseY, event.isShift());
                    } else {
                        selecting = true;
                        selectionStartX = event.mouseX;
                        selectionStartY = event.mouseY;
                        selectionEndX = event.mouseX;
                        selectionEndY = event.mouseY;
                        if (selectionTool == SelectionTool::Lasso) {
                            lassoPoints.clear();
                            lassoPoints.push_back({event.mouseX, event.mouseY});
                        }
                    }
                    return true;
                }
            }
            break;
            
        case InputEvent::Type::MouseMove:
            if (selecting) {
                selectionEndX = event.mouseX;
                selectionEndY = event.mouseY;
                if (selectionTool == SelectionTool::Lasso) {
                    lassoPoints.push_back({event.mouseX, event.mouseY});
                }
                return true;
            }
            if (transforming) {
                // Update transform based on mouse delta
                // This would calculate the delta and call updateTransform
                return true;
            }
            break;
            
        case InputEvent::Type::MouseUp:
            if (event.key == 0 && selecting) {
                selecting = false;
                bool additive = event.isShift();
                
                switch (selectionTool) {
                    case SelectionTool::Box:
                        performBoxSelection(selectionStartX, selectionStartY,
                                          selectionEndX, selectionEndY, additive);
                        break;
                    case SelectionTool::Circle: {
                        float radius = std::sqrt(
                            (selectionEndX - selectionStartX) * (selectionEndX - selectionStartX) +
                            (selectionEndY - selectionStartY) * (selectionEndY - selectionStartY)
                        );
                        performCircleSelection(selectionStartX, selectionStartY, radius, additive);
                        break;
                    }
                    case SelectionTool::Lasso:
                        performLassoSelection(lassoPoints, additive);
                        lassoPoints.clear();
                        break;
                    default:
                        break;
                }
                return true;
            }
            if (transforming && event.key == 0) {
                endTransform(true);
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

inline void MeshEditModule::clearSelection() {
    if (editMesh) {
        editMesh->clearSelection();
    }
}

inline void MeshEditModule::selectAll() {
    if (editMesh) {
        editMesh->selectAll();
    }
}

inline void MeshEditModule::setSelectionMode(SelectionMode mode) {
    selectionMode = mode;
    clearSelection();
}

inline void MeshEditModule::setCurrentTool(MeshTool tool) {
    if (transforming && tool != currentTool) {
        endTransform(false);
    }
    currentTool = tool;
}

inline void MeshEditModule::setSelectionTool(SelectionTool tool) {
    selectionTool = tool;
}

inline void MeshEditModule::extrude(float distance) {
    if (!editMesh) return;
    
    // Extrude based on selection mode
    if (selectionMode == SelectionMode::Face && !editMesh->selectedFaces.empty()) {
        editMesh->extrudeSelectedFaces(0, distance, 0);
        notifyMeshChanged();
    }
    // TODO: Vertex and edge extrude
}

inline void MeshEditModule::subdivide() {
    if (!editMesh) return;
    
    if (!editMesh->selectedFaces.empty()) {
        editMesh->subdivideSelectedFaces();
        notifyMeshChanged();
    }
}

inline void MeshEditModule::deleteSelected() {
    if (!editMesh) return;
    
    if (!editMesh->selectedFaces.empty()) {
        editMesh->deleteSelectedFaces();
        notifyMeshChanged();
    }
    // TODO: Delete vertices and edges
}

inline void MeshEditModule::duplicateSelected() {
    // TODO: Implement duplication
}

inline void MeshEditModule::mergeVertices(float threshold) {
    if (!editMesh) return;
    editMesh->mergeByDistance(threshold);
    notifyMeshChanged();
}

inline void MeshEditModule::flipNormals() {
    // TODO: Implement flip normals for selected faces
}

inline void MeshEditModule::recalculateNormals() {
    if (!editMesh) return;
    editMesh->recalculateNormals();
    notifyMeshChanged();
}

inline void MeshEditModule::beginTransform(MeshTool tool) {
    if (!editMesh) return;
    
    transforming = true;
    transformTool = tool;
    transformAccum[0] = transformAccum[1] = transformAccum[2] = 0;
    
    calculateSelectionPivot();
    
    // Push undo before transform starts
    editMesh->pushUndo();
}

inline void MeshEditModule::updateTransform(float dx, float dy, float dz) {
    if (!editMesh || !transforming) return;
    
    // Apply snapping if enabled
    if (snapEnabled) {
        dx = snapValue(transformAccum[0] + dx) - transformAccum[0];
        dy = snapValue(transformAccum[1] + dy) - transformAccum[1];
        dz = snapValue(transformAccum[2] + dz) - transformAccum[2];
    }
    
    transformAccum[0] += dx;
    transformAccum[1] += dy;
    transformAccum[2] += dz;
    
    switch (transformTool) {
        case MeshTool::Move:
            editMesh->translateSelected(dx, dy, dz);
            break;
        case MeshTool::Scale:
            editMesh->scaleSelected(1.0f + dx * 0.01f, 1.0f + dy * 0.01f, 1.0f + dz * 0.01f, transformPivot);
            break;
        case MeshTool::Rotate:
            // TODO: Implement rotation
            break;
        default:
            break;
    }
}

inline void MeshEditModule::endTransform(bool apply) {
    if (!editMesh || !transforming) return;
    
    transforming = false;
    
    if (apply) {
        notifyMeshChanged();
    } else {
        // Cancel - undo the transform
        editMesh->undo();
    }
}

inline void MeshEditModule::notifyMeshChanged() {
    markDirty();
    if (meshChangedCallback) {
        meshChangedCallback();
    }
}

inline void MeshEditModule::calculateSelectionPivot() {
    if (!editMesh || editMesh->selectedVertices.empty()) {
        transformPivot[0] = transformPivot[1] = transformPivot[2] = 0;
        return;
    }
    
    float cx = 0, cy = 0, cz = 0;
    for (uint32_t vi : editMesh->selectedVertices) {
        if (vi < editMesh->vertices.size()) {
            cx += editMesh->vertices[vi].position[0];
            cy += editMesh->vertices[vi].position[1];
            cz += editMesh->vertices[vi].position[2];
        }
    }
    
    float n = static_cast<float>(editMesh->selectedVertices.size());
    transformPivot[0] = cx / n;
    transformPivot[1] = cy / n;
    transformPivot[2] = cz / n;
}

inline float MeshEditModule::snapValue(float value) {
    if (snapGrid <= 0) return value;
    return std::round(value / snapGrid) * snapGrid;
}

inline void MeshEditModule::performClickSelection(float x, float y, bool additive) {
    // TODO: Implement ray picking for click selection
    // This would cast a ray and find the nearest vertex/edge/face
    (void)x; (void)y; (void)additive;
}

inline void MeshEditModule::performBoxSelection(float x1, float y1, float x2, float y2, bool additive) {
    // TODO: Implement box selection
    // This would project all vertices to screen and check if they're in the box
    (void)x1; (void)y1; (void)x2; (void)y2; (void)additive;
}

inline void MeshEditModule::performCircleSelection(float cx, float cy, float radius, bool additive) {
    // TODO: Implement circle selection
    (void)cx; (void)cy; (void)radius; (void)additive;
}

inline void MeshEditModule::performLassoSelection(const std::vector<Vec2>& points, bool additive) {
    // TODO: Implement lasso selection
    (void)points; (void)additive;
}

inline void MeshEditModule::renderSelectionPreview(const RenderContext& ctx) {
    // TODO: Render selection rectangle/circle/lasso outline
    (void)ctx;
}

inline void MeshEditModule::renderToolGizmo(const RenderContext& ctx) {
    // TODO: Render transform gizmo
    (void)ctx;
}

} // namespace editor
} // namespace luma
