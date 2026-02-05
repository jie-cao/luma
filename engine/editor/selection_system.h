// LUMA Selection System
// Unified selection logic for vertices, edges, faces, and objects
// Uses existing types from mesh_picking.h

#pragma once

#include "engine/editor/mesh_picking.h"  // Reuse Ray, SelectionMode from here
#include <vector>
#include <set>
#include <functional>
#include <cmath>

namespace luma {
namespace editor {

// Selection method
enum class SelectionMethod {
    Click,      // Single click selection
    Box,        // Rectangle selection
    Circle,     // Circle selection
    Lasso       // Free-form lasso selection
};

// Selection operation
enum class SelectionOp {
    Set,        // Replace selection
    Add,        // Add to selection
    Remove,     // Remove from selection
    Toggle      // Toggle selection state
};

// 2D region for selection
struct SelectionRegion {
    SelectionMethod method = SelectionMethod::Click;
    
    // For click selection
    float clickX = 0, clickY = 0;
    float clickRadius = 5.0f;  // Pixel radius for click tolerance
    
    // For box selection
    float boxMinX = 0, boxMinY = 0;
    float boxMaxX = 0, boxMaxY = 0;
    
    // For circle selection
    float circleX = 0, circleY = 0;
    float circleRadius = 50.0f;
    
    // For lasso selection
    std::vector<std::pair<float, float>> lassoPoints;
    
    // Check if a 2D point is inside this region
    bool contains(float x, float y) const {
        switch (method) {
            case SelectionMethod::Click:
                return pointInCircle(x, y, clickX, clickY, clickRadius);
            case SelectionMethod::Box:
                return pointInBox(x, y, boxMinX, boxMinY, boxMaxX, boxMaxY);
            case SelectionMethod::Circle:
                return pointInCircle(x, y, circleX, circleY, circleRadius);
            case SelectionMethod::Lasso:
                return pointInPolygon(x, y, lassoPoints);
            default:
                return false;
        }
    }
    
private:
    static bool pointInBox(float x, float y, float minX, float minY, float maxX, float maxY) {
        return x >= minX && x <= maxX && y >= minY && y <= maxY;
    }
    
    static bool pointInCircle(float x, float y, float cx, float cy, float radius) {
        float dx = x - cx;
        float dy = y - cy;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
    
    static bool pointInPolygon(float x, float y, const std::vector<std::pair<float, float>>& polygon) {
        if (polygon.size() < 3) return false;
        
        int count = 0;
        size_t n = polygon.size();
        for (size_t i = 0; i < n; i++) {
            size_t j = (i + 1) % n;
            float xi = polygon[i].first, yi = polygon[i].second;
            float xj = polygon[j].first, yj = polygon[j].second;
            
            if ((yi <= y && yj > y) || (yj <= y && yi > y)) {
                float slope = (xj - xi) / (yj - yi);
                float xIntersect = xi + slope * (y - yi);
                if (x < xIntersect) {
                    count++;
                }
            }
        }
        return (count % 2) == 1;
    }
};

// Selection result (similar to PickResult but with different semantics)
struct SelectionHit {
    enum Type { None, Vertex, Edge, Face, Object };
    Type type = None;
    int index = -1;
    float distance = std::numeric_limits<float>::max();
    float hitPoint[3] = {0, 0, 0};
};

// Projection callback (projects 3D point to 2D screen space)
using ProjectionCallback = std::function<bool(float x, float y, float z, float& screenX, float& screenY)>;

// Selection System
class SelectionSystem {
public:
    SelectionSystem() = default;
    ~SelectionSystem() = default;
    
    // Initialize with mesh reference
    void setMesh(EditMesh* mesh) { targetMesh = mesh; }
    EditMesh* getMesh() const { return targetMesh; }
    
    // Set projection callback
    void setProjectionCallback(ProjectionCallback callback) { projectToScreen = callback; }
    
    // Selection mode
    void setMode(SelectionMode mode) { selectionMode = mode; }
    SelectionMode getMode() const { return selectionMode; }
    
    // Perform selection using MeshPicker
    void selectByRay(const Ray& ray, SelectionOp op = SelectionOp::Set) {
        if (!targetMesh) return;
        
        PickResult result = picker.pick(ray, *targetMesh, selectionMode);
        if (!result.hit()) return;
        
        applySelection(result, op);
    }
    
    // Selection helpers using EditMesh's existing API
    void selectAll() {
        if (!targetMesh) return;
        
        switch (selectionMode) {
            case SelectionMode::Vertex:
                for (size_t i = 0; i < targetMesh->vertices.size(); i++) {
                    targetMesh->selectedVertices.insert(static_cast<uint32_t>(i));
                }
                break;
            case SelectionMode::Edge:
                for (size_t i = 0; i < targetMesh->edges.size(); i++) {
                    targetMesh->selectedEdges.insert(static_cast<uint32_t>(i));
                }
                break;
            case SelectionMode::Face:
                for (size_t i = 0; i < targetMesh->faces.size(); i++) {
                    targetMesh->selectedFaces.insert(static_cast<uint32_t>(i));
                }
                break;
        }
    }
    
    void selectNone() {
        if (!targetMesh) return;
        targetMesh->selectedVertices.clear();
        targetMesh->selectedEdges.clear();
        targetMesh->selectedFaces.clear();
    }
    
    void invertSelection() {
        if (!targetMesh) return;
        
        switch (selectionMode) {
            case SelectionMode::Vertex: {
                std::set<uint32_t> newSel;
                for (size_t i = 0; i < targetMesh->vertices.size(); i++) {
                    if (targetMesh->selectedVertices.find(i) == targetMesh->selectedVertices.end()) {
                        newSel.insert(static_cast<uint32_t>(i));
                    }
                }
                targetMesh->selectedVertices = newSel;
                break;
            }
            case SelectionMode::Edge: {
                std::set<uint32_t> newSel;
                for (size_t i = 0; i < targetMesh->edges.size(); i++) {
                    if (targetMesh->selectedEdges.find(i) == targetMesh->selectedEdges.end()) {
                        newSel.insert(static_cast<uint32_t>(i));
                    }
                }
                targetMesh->selectedEdges = newSel;
                break;
            }
            case SelectionMode::Face: {
                std::set<uint32_t> newSel;
                for (size_t i = 0; i < targetMesh->faces.size(); i++) {
                    if (targetMesh->selectedFaces.find(i) == targetMesh->selectedFaces.end()) {
                        newSel.insert(static_cast<uint32_t>(i));
                    }
                }
                targetMesh->selectedFaces = newSel;
                break;
            }
        }
    }
    
    // Get selection counts
    size_t getSelectedVertexCount() const {
        return targetMesh ? targetMesh->selectedVertices.size() : 0;
    }
    
    size_t getSelectedEdgeCount() const {
        return targetMesh ? targetMesh->selectedEdges.size() : 0;
    }
    
    size_t getSelectedFaceCount() const {
        return targetMesh ? targetMesh->selectedFaces.size() : 0;
    }
    
    // Batch select by screen region
    std::vector<int> selectVerticesInRegion(const SelectionRegion& region) const {
        std::vector<int> result;
        if (!targetMesh || !projectToScreen) return result;
        
        for (size_t i = 0; i < targetMesh->vertices.size(); i++) {
            const auto& v = targetMesh->vertices[i];
            float screenX, screenY;
            if (projectToScreen(v.position[0], v.position[1], v.position[2], screenX, screenY)) {
                if (region.contains(screenX, screenY)) {
                    result.push_back(static_cast<int>(i));
                }
            }
        }
        
        return result;
    }
    
    std::vector<int> selectFacesInRegion(const SelectionRegion& region) const {
        std::vector<int> result;
        if (!targetMesh || !projectToScreen) return result;
        
        for (size_t f = 0; f < targetMesh->faces.size(); f++) {
            const auto& face = targetMesh->faces[f];
            if (face.loops.empty()) continue;
            
            // Project face center to screen
            float centerX = 0, centerY = 0, centerZ = 0;
            for (const auto& loop : face.loops) {
                const auto& v = targetMesh->vertices[loop.vertexIndex];
                centerX += v.position[0];
                centerY += v.position[1];
                centerZ += v.position[2];
            }
            centerX /= face.loops.size();
            centerY /= face.loops.size();
            centerZ /= face.loops.size();
            
            float screenX, screenY;
            if (projectToScreen(centerX, centerY, centerZ, screenX, screenY)) {
                if (region.contains(screenX, screenY)) {
                    result.push_back(static_cast<int>(f));
                }
            }
        }
        
        return result;
    }
    
    // Selection visualization settings
    struct SelectionVisual {
        bool showVertices = true;
        bool showEdges = true;
        bool showFaces = true;
        float vertexSize = 6.0f;
        float edgeWidth = 2.0f;
        float selectedColor[4] = {1.0f, 0.5f, 0.0f, 1.0f};  // Orange
        float hoveredColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};   // Yellow
    };
    
    SelectionVisual& getVisualSettings() { return visualSettings; }
    
    // Hover (for highlighting before click)
    void updateHover(const Ray& ray) {
        if (!targetMesh) {
            hoveredElement = SelectionHit();
            return;
        }
        
        PickResult result = picker.pick(ray, *targetMesh, selectionMode);
        if (result.hit()) {
            hoveredElement.type = static_cast<SelectionHit::Type>(static_cast<int>(result.type));
            hoveredElement.index = result.index;
            hoveredElement.distance = result.distance;
            hoveredElement.hitPoint[0] = result.hitPoint[0];
            hoveredElement.hitPoint[1] = result.hitPoint[1];
            hoveredElement.hitPoint[2] = result.hitPoint[2];
        } else {
            hoveredElement = SelectionHit();
        }
    }
    
    SelectionHit getHoveredElement() const { return hoveredElement; }
    void clearHover() { hoveredElement = SelectionHit(); }
    
    // Access to picker for advanced use
    MeshPicker& getPicker() { return picker; }
    
private:
    EditMesh* targetMesh = nullptr;
    SelectionMode selectionMode = SelectionMode::Vertex;
    ProjectionCallback projectToScreen;
    SelectionVisual visualSettings;
    SelectionHit hoveredElement;
    MeshPicker picker;
    
    void applySelection(const PickResult& result, SelectionOp op) {
        if (!targetMesh) return;
        
        uint32_t idx = result.index;
        
        switch (result.type) {
            case PickResult::Type::Vertex: {
                bool selected = targetMesh->selectedVertices.count(idx) > 0;
                switch (op) {
                    case SelectionOp::Set:
                        targetMesh->selectedVertices.clear();
                        targetMesh->selectedVertices.insert(idx);
                        break;
                    case SelectionOp::Add:
                        targetMesh->selectedVertices.insert(idx);
                        break;
                    case SelectionOp::Remove:
                        targetMesh->selectedVertices.erase(idx);
                        break;
                    case SelectionOp::Toggle:
                        if (selected) targetMesh->selectedVertices.erase(idx);
                        else targetMesh->selectedVertices.insert(idx);
                        break;
                }
                break;
            }
            case PickResult::Type::Edge: {
                bool selected = targetMesh->selectedEdges.count(idx) > 0;
                switch (op) {
                    case SelectionOp::Set:
                        targetMesh->selectedEdges.clear();
                        targetMesh->selectedEdges.insert(idx);
                        break;
                    case SelectionOp::Add:
                        targetMesh->selectedEdges.insert(idx);
                        break;
                    case SelectionOp::Remove:
                        targetMesh->selectedEdges.erase(idx);
                        break;
                    case SelectionOp::Toggle:
                        if (selected) targetMesh->selectedEdges.erase(idx);
                        else targetMesh->selectedEdges.insert(idx);
                        break;
                }
                break;
            }
            case PickResult::Type::Face: {
                bool selected = targetMesh->selectedFaces.count(idx) > 0;
                switch (op) {
                    case SelectionOp::Set:
                        targetMesh->selectedFaces.clear();
                        targetMesh->selectedFaces.insert(idx);
                        break;
                    case SelectionOp::Add:
                        targetMesh->selectedFaces.insert(idx);
                        break;
                    case SelectionOp::Remove:
                        targetMesh->selectedFaces.erase(idx);
                        break;
                    case SelectionOp::Toggle:
                        if (selected) targetMesh->selectedFaces.erase(idx);
                        else targetMesh->selectedFaces.insert(idx);
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
};

} // namespace editor
} // namespace luma
