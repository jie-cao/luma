// LUMA Mesh Edit Module
// Modular mesh editing functionality (vertex/edge/face editing, transforms, modeling ops)

#pragma once

#include "engine/editor/edit_module.h"
#include "engine/editor/mesh_picking.h"
#include "engine/mesh/edit_mesh.h"
#include "engine/renderer/unified_renderer.h"
#include <memory>
#include <functional>
#include <cmath>

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

// Selection context - matrices and dimensions needed for screen-space selection
struct SelectionContext {
    float viewMatrix[16] = {};
    float projMatrix[16] = {};
    float worldMatrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  // Identity default
    int windowWidth = 1280;
    int windowHeight = 720;
    bool hasWorldMatrix = false;
    
    // Project a 3D point to screen coordinates + depth
    // Returns false if behind camera. depth is the clip-space Z value (0=near, 1=far)
    bool projectToScreenWithDepth(const float* pos, float& screenX, float& screenY, float& depth) const {
        float worldPos[4] = {pos[0], pos[1], pos[2], 1.0f};
        if (hasWorldMatrix) {
            float wp[4];
            wp[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
            wp[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
            wp[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
            worldPos[0] = wp[0]; worldPos[1] = wp[1]; worldPos[2] = wp[2]; worldPos[3] = 1.0f;
        }
        
        float viewPos[4];
        viewPos[0] = viewMatrix[0]*worldPos[0] + viewMatrix[4]*worldPos[1] + viewMatrix[8]*worldPos[2] + viewMatrix[12];
        viewPos[1] = viewMatrix[1]*worldPos[0] + viewMatrix[5]*worldPos[1] + viewMatrix[9]*worldPos[2] + viewMatrix[13];
        viewPos[2] = viewMatrix[2]*worldPos[0] + viewMatrix[6]*worldPos[1] + viewMatrix[10]*worldPos[2] + viewMatrix[14];
        viewPos[3] = viewMatrix[3]*worldPos[0] + viewMatrix[7]*worldPos[1] + viewMatrix[11]*worldPos[2] + viewMatrix[15];
        
        float clipPos[4];
        clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
        clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
        clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
        clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
        
        if (clipPos[3] <= 0.0f) return false;
        
        float ndcX = clipPos[0] / clipPos[3];
        float ndcY = clipPos[1] / clipPos[3];
        depth = clipPos[2] / clipPos[3];  // NDC depth (0 to 1 for DirectX)
        screenX = (ndcX + 1.0f) * 0.5f * windowWidth;
        screenY = (1.0f - ndcY) * 0.5f * windowHeight;
        return true;
    }
    
    // Project a 3D point to screen coordinates
    // Returns false if behind camera
    bool projectToScreen(const float* pos, float& screenX, float& screenY) const {
        // Apply world matrix
        float worldPos[4] = {pos[0], pos[1], pos[2], 1.0f};
        if (hasWorldMatrix) {
            float wp[4];
            wp[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
            wp[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
            wp[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
            worldPos[0] = wp[0]; worldPos[1] = wp[1]; worldPos[2] = wp[2]; worldPos[3] = 1.0f;
        }
        
        // Apply view matrix
        float viewPos[4];
        viewPos[0] = viewMatrix[0]*worldPos[0] + viewMatrix[4]*worldPos[1] + viewMatrix[8]*worldPos[2] + viewMatrix[12];
        viewPos[1] = viewMatrix[1]*worldPos[0] + viewMatrix[5]*worldPos[1] + viewMatrix[9]*worldPos[2] + viewMatrix[13];
        viewPos[2] = viewMatrix[2]*worldPos[0] + viewMatrix[6]*worldPos[1] + viewMatrix[10]*worldPos[2] + viewMatrix[14];
        viewPos[3] = viewMatrix[3]*worldPos[0] + viewMatrix[7]*worldPos[1] + viewMatrix[11]*worldPos[2] + viewMatrix[15];
        
        // Apply projection matrix
        float clipPos[4];
        clipPos[0] = projMatrix[0]*viewPos[0] + projMatrix[4]*viewPos[1] + projMatrix[8]*viewPos[2] + projMatrix[12]*viewPos[3];
        clipPos[1] = projMatrix[1]*viewPos[0] + projMatrix[5]*viewPos[1] + projMatrix[9]*viewPos[2] + projMatrix[13]*viewPos[3];
        clipPos[2] = projMatrix[2]*viewPos[0] + projMatrix[6]*viewPos[1] + projMatrix[10]*viewPos[2] + projMatrix[14]*viewPos[3];
        clipPos[3] = projMatrix[3]*viewPos[0] + projMatrix[7]*viewPos[1] + projMatrix[11]*viewPos[2] + projMatrix[15]*viewPos[3];
        
        // Behind camera check
        if (clipPos[3] <= 0.0f) return false;
        
        // NDC to screen
        float ndcX = clipPos[0] / clipPos[3];
        float ndcY = clipPos[1] / clipPos[3];
        screenX = (ndcX + 1.0f) * 0.5f * windowWidth;
        screenY = (1.0f - ndcY) * 0.5f * windowHeight;
        return true;
    }
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
    
    // Selection context for screen-space operations
    void setSelectionContext(const SelectionContext& ctx) { selectionCtx = ctx; }
    const SelectionContext& getSelectionContext() const { return selectionCtx; }
    
    // X-Ray mode: when false, only select visible (front-facing) elements
    void setXRayMode(bool enabled) { xRayMode = enabled; }
    bool getXRayMode() const { return xRayMode; }
    
    // Selection state getters (for UI overlay sync)
    bool isSelecting() const { return selecting; }
    void getSelectionBounds(float& x1, float& y1, float& x2, float& y2) const {
        x1 = selectionStartX; y1 = selectionStartY;
        x2 = selectionEndX; y2 = selectionEndY;
    }
    
    // Selection operations (public for EditModeHandler delegation)
    void performBoxSelection(float x1, float y1, float x2, float y2, bool additive);
    void performCircleSelection(float cx, float cy, float radius, bool additive);
    
private:
    UnifiedRenderer* renderer = nullptr;
    EditMesh* editMesh = nullptr;
    
    // Current state
    SelectionMode selectionMode = SelectionMode::Vertex;
    MeshTool currentTool = MeshTool::Select;
    SelectionTool selectionTool = SelectionTool::Click;
    
    // X-Ray mode
    bool xRayMode = true;
    
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
    
    // Selection context (matrices, dimensions)
    SelectionContext selectionCtx;
    
    // Callbacks
    MeshChangedCallback meshChangedCallback;
    
    // Internal helpers
    void notifyMeshChanged();
    void calculateSelectionPivot();
    float snapValue(float value);
    
    // Selection helpers
    void performClickSelection(float x, float y, bool additive);
    // performBoxSelection and performCircleSelection are public (declared above)
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
            if (event.key == 0 && !event.isAlt()) {  // Left mouse, not Alt (Alt = camera orbit)
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
    if (!editMesh) return;
    
    // Create ray from screen position
    Ray pickRay = MeshPicker::createRayFromScreen(
        x, y, selectionCtx.windowWidth, selectionCtx.windowHeight,
        selectionCtx.viewMatrix, selectionCtx.projMatrix);
    
    // Get world matrix pointer if available
    const float* worldMatrix = selectionCtx.hasWorldMatrix ? selectionCtx.worldMatrix : nullptr;
    
    // Pick based on current selection mode
    MeshPicker picker;
    PickResult result = picker.pick(pickRay, *editMesh, selectionMode, worldMatrix);
    
    if (result.hit()) {
        switch (result.type) {
            case PickResult::Type::Vertex:
                editMesh->selectVertex(result.index, additive);
                break;
            case PickResult::Type::Edge:
                editMesh->selectEdge(result.index, additive);
                break;
            case PickResult::Type::Face:
                editMesh->selectFace(result.index, additive);
                break;
            default:
                break;
        }
    } else if (!additive) {
        // Clicked on nothing, clear selection
        editMesh->selectNone();
    }
}

inline void MeshEditModule::performBoxSelection(float x1, float y1, float x2, float y2, bool additive) {
    if (!editMesh) return;
    
    // Normalize selection rectangle
    float minX = std::min(x1, x2);
    float maxX = std::max(x1, x2);
    float minY = std::min(y1, y2);
    float maxY = std::max(y1, y2);
    
    // Clear selection if not additive
    if (!additive) {
        editMesh->selectedVertices.clear();
        editMesh->selectedEdges.clear();
        editMesh->selectedFaces.clear();
    }
    
    auto isInBox = [minX, maxX, minY, maxY](float sx, float sy) -> bool {
        return sx >= minX && sx <= maxX && sy >= minY && sy <= maxY;
    };
    
    // Build per-vertex face-normal visibility data (only needed in non-xray mode)
    // A vertex is visible if ANY adjacent face is front-facing
    const float* wm = selectionCtx.hasWorldMatrix ? selectionCtx.worldMatrix : nullptr;
    
    // Camera position in world space (extracted from inverse view matrix)
    // For visibility check: compute view direction for each element
    // View direction in world space: the camera looks along -Z in view space
    // Camera position = -transpose(R) * T where R and T come from the view matrix
    float camPos[3] = {0, 0, 0};
    {
        const float* vm = selectionCtx.viewMatrix;
        // For a view matrix V = [R|t], camera pos = -R^T * t
        camPos[0] = -(vm[0]*vm[12] + vm[1]*vm[13] + vm[2]*vm[14]);
        camPos[1] = -(vm[4]*vm[12] + vm[5]*vm[13] + vm[6]*vm[14]);
        camPos[2] = -(vm[8]*vm[12] + vm[9]*vm[13] + vm[10]*vm[14]);
    }
    
    // Precompute per-vertex visibility (front-facing check)
    std::vector<bool> vertexVisible;
    if (!xRayMode) {
        vertexVisible.resize(editMesh->vertices.size(), false);
        
        for (const auto& face : editMesh->faces) {
            if (face.loops.size() < 3) continue;
            
            // Get face vertices in world space
            auto getWorldPos = [&](uint32_t vi, float* out) {
                const float* p = editMesh->vertices[vi].position;
                if (wm) {
                    out[0] = wm[0]*p[0] + wm[4]*p[1] + wm[8]*p[2]  + wm[12];
                    out[1] = wm[1]*p[0] + wm[5]*p[1] + wm[9]*p[2]  + wm[13];
                    out[2] = wm[2]*p[0] + wm[6]*p[1] + wm[10]*p[2] + wm[14];
                } else {
                    out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
                }
            };
            
            float wp0[3], wp1[3], wp2[3];
            getWorldPos(face.loops[0].vertexIndex, wp0);
            getWorldPos(face.loops[1].vertexIndex, wp1);
            getWorldPos(face.loops[2].vertexIndex, wp2);
            
            // Face normal (world space)
            float e1[3] = {wp1[0]-wp0[0], wp1[1]-wp0[1], wp1[2]-wp0[2]};
            float e2[3] = {wp2[0]-wp0[0], wp2[1]-wp0[1], wp2[2]-wp0[2]};
            float faceNormal[3];
            luma::math::cross3(faceNormal, e1, e2);
            luma::math::normalize3(faceNormal);
            
            // View direction from face center to camera
            float fc[3] = {(wp0[0]+wp1[0]+wp2[0])/3, (wp0[1]+wp1[1]+wp2[1])/3, (wp0[2]+wp1[2]+wp2[2])/3};
            float viewDir[3] = {camPos[0]-fc[0], camPos[1]-fc[1], camPos[2]-fc[2]};
            
            // Front-facing check: dot(normal, viewDir) > 0
            float dot = faceNormal[0]*viewDir[0] + faceNormal[1]*viewDir[1] + faceNormal[2]*viewDir[2];
            if (dot > 0.0f) {
                // This face is front-facing - mark all its vertices as visible
                for (const auto& loop : face.loops) {
                    if (loop.vertexIndex < vertexVisible.size()) {
                        vertexVisible[loop.vertexIndex] = true;
                    }
                }
            }
        }
    }
    
    // Vertex visibility check helper
    auto isVertexVisible = [&](uint32_t vi) -> bool {
        if (xRayMode) return true;
        if (vi >= vertexVisible.size()) return false;
        return vertexVisible[vi];
    };
    
    if (selectionMode == SelectionMode::Vertex) {
        for (size_t vi = 0; vi < editMesh->vertices.size(); ++vi) {
            if (!isVertexVisible(static_cast<uint32_t>(vi))) continue;
            
            const auto& v = editMesh->vertices[vi];
            float screenX, screenY;
            if (selectionCtx.projectToScreen(v.position, screenX, screenY)) {
                if (isInBox(screenX, screenY)) {
                    editMesh->selectedVertices.insert(static_cast<uint32_t>(vi));
                }
            }
        }
    } else if (selectionMode == SelectionMode::Edge) {
        for (size_t ei = 0; ei < editMesh->edges.size(); ++ei) {
            const auto& edge = editMesh->edges[ei];
            if (edge.v0 >= editMesh->vertices.size() || edge.v1 >= editMesh->vertices.size()) continue;
            
            // In non-xray mode, at least one vertex of the edge must be visible
            if (!xRayMode && !isVertexVisible(edge.v0) && !isVertexVisible(edge.v1)) continue;
            
            const auto& v0 = editMesh->vertices[edge.v0];
            const auto& v1 = editMesh->vertices[edge.v1];
            float midPos[3] = {
                (v0.position[0] + v1.position[0]) * 0.5f,
                (v0.position[1] + v1.position[1]) * 0.5f,
                (v0.position[2] + v1.position[2]) * 0.5f
            };
            float screenX, screenY;
            if (selectionCtx.projectToScreen(midPos, screenX, screenY)) {
                if (isInBox(screenX, screenY)) {
                    editMesh->selectedEdges.insert(static_cast<uint32_t>(ei));
                }
            }
        }
    } else if (selectionMode == SelectionMode::Face) {
        for (size_t fi = 0; fi < editMesh->faces.size(); ++fi) {
            const auto& face = editMesh->faces[fi];
            if (face.loops.empty()) continue;
            
            // In non-xray mode, check if face is front-facing
            if (!xRayMode && face.loops.size() >= 3) {
                auto getWorldPos = [&](uint32_t vi, float* out) {
                    const float* p = editMesh->vertices[vi].position;
                    if (wm) {
                        out[0] = wm[0]*p[0] + wm[4]*p[1] + wm[8]*p[2]  + wm[12];
                        out[1] = wm[1]*p[0] + wm[5]*p[1] + wm[9]*p[2]  + wm[13];
                        out[2] = wm[2]*p[0] + wm[6]*p[1] + wm[10]*p[2] + wm[14];
                    } else {
                        out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
                    }
                };
                
                float wp0[3], wp1[3], wp2[3];
                getWorldPos(face.loops[0].vertexIndex, wp0);
                getWorldPos(face.loops[1].vertexIndex, wp1);
                getWorldPos(face.loops[2].vertexIndex, wp2);
                
                float e1[3] = {wp1[0]-wp0[0], wp1[1]-wp0[1], wp1[2]-wp0[2]};
                float e2[3] = {wp2[0]-wp0[0], wp2[1]-wp0[1], wp2[2]-wp0[2]};
                float faceNormal[3];
                luma::math::cross3(faceNormal, e1, e2);
                luma::math::normalize3(faceNormal);
                
                float fc[3] = {(wp0[0]+wp1[0]+wp2[0])/3, (wp0[1]+wp1[1]+wp2[1])/3, (wp0[2]+wp1[2]+wp2[2])/3};
                float viewDir[3] = {camPos[0]-fc[0], camPos[1]-fc[1], camPos[2]-fc[2]};
                
                float dot = faceNormal[0]*viewDir[0] + faceNormal[1]*viewDir[1] + faceNormal[2]*viewDir[2];
                if (dot <= 0.0f) continue;  // Back-facing, skip
            }
            
            float centerPos[3] = {0, 0, 0};
            int validVerts = 0;
            for (const auto& loop : face.loops) {
                if (loop.vertexIndex < editMesh->vertices.size()) {
                    const auto& v = editMesh->vertices[loop.vertexIndex];
                    centerPos[0] += v.position[0];
                    centerPos[1] += v.position[1];
                    centerPos[2] += v.position[2];
                    validVerts++;
                }
            }
            if (validVerts > 0) {
                centerPos[0] /= validVerts;
                centerPos[1] /= validVerts;
                centerPos[2] /= validVerts;
                float screenX, screenY;
                if (selectionCtx.projectToScreen(centerPos, screenX, screenY)) {
                    if (isInBox(screenX, screenY)) {
                        editMesh->selectedFaces.insert(static_cast<uint32_t>(fi));
                    }
                }
            }
        }
    }
}

inline void MeshEditModule::performCircleSelection(float cx, float cy, float radius, bool additive) {
    if (!editMesh) return;
    
    // Clear selection if not additive
    if (!additive) {
        editMesh->selectedVertices.clear();
        editMesh->selectedEdges.clear();
        editMesh->selectedFaces.clear();
    }
    
    float radiusSq = radius * radius;
    auto isInCircle = [cx, cy, radiusSq](float sx, float sy) -> bool {
        float dx = sx - cx;
        float dy = sy - cy;
        return (dx * dx + dy * dy) <= radiusSq;
    };
    
    if (selectionMode == SelectionMode::Vertex) {
        for (size_t vi = 0; vi < editMesh->vertices.size(); ++vi) {
            const auto& v = editMesh->vertices[vi];
            float screenX, screenY;
            if (selectionCtx.projectToScreen(v.position, screenX, screenY)) {
                if (isInCircle(screenX, screenY)) {
                    editMesh->selectedVertices.insert(static_cast<uint32_t>(vi));
                }
            }
        }
    } else if (selectionMode == SelectionMode::Edge) {
        for (size_t ei = 0; ei < editMesh->edges.size(); ++ei) {
            const auto& edge = editMesh->edges[ei];
            if (edge.v0 >= editMesh->vertices.size() || edge.v1 >= editMesh->vertices.size()) continue;
            
            const auto& v0 = editMesh->vertices[edge.v0];
            const auto& v1 = editMesh->vertices[edge.v1];
            float midPos[3] = {
                (v0.position[0] + v1.position[0]) * 0.5f,
                (v0.position[1] + v1.position[1]) * 0.5f,
                (v0.position[2] + v1.position[2]) * 0.5f
            };
            float screenX, screenY;
            if (selectionCtx.projectToScreen(midPos, screenX, screenY)) {
                if (isInCircle(screenX, screenY)) {
                    editMesh->selectedEdges.insert(static_cast<uint32_t>(ei));
                }
            }
        }
    } else if (selectionMode == SelectionMode::Face) {
        for (size_t fi = 0; fi < editMesh->faces.size(); ++fi) {
            const auto& face = editMesh->faces[fi];
            if (face.loops.empty()) continue;
            
            float centerPos[3] = {0, 0, 0};
            int validVerts = 0;
            for (const auto& loop : face.loops) {
                if (loop.vertexIndex < editMesh->vertices.size()) {
                    const auto& v = editMesh->vertices[loop.vertexIndex];
                    centerPos[0] += v.position[0];
                    centerPos[1] += v.position[1];
                    centerPos[2] += v.position[2];
                    validVerts++;
                }
            }
            if (validVerts > 0) {
                centerPos[0] /= validVerts;
                centerPos[1] /= validVerts;
                centerPos[2] /= validVerts;
                float screenX, screenY;
                if (selectionCtx.projectToScreen(centerPos, screenX, screenY)) {
                    if (isInCircle(screenX, screenY)) {
                        editMesh->selectedFaces.insert(static_cast<uint32_t>(fi));
                    }
                }
            }
        }
    }
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
