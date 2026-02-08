// LUMA Draw Manager
// Orchestrates rendering for different modes (Scene, Edit, Animation, etc.)
// Separates PBR scene rendering from Edit mode specialized rendering

#pragma once

#include "engine/renderer/unified_renderer.h"
#include "engine/scene/entity.h"
#include <vector>
#include <functional>

namespace luma {

// Forward declarations
class SceneGraph;
class EditMesh;

// Render context passed to render functions
struct RenderContext {
    UnifiedRenderer* renderer = nullptr;
    float viewMatrix[16];
    float projMatrix[16];
    float cameraPos[3];
    float lightDir[3];
    int viewportWidth = 1280;
    int viewportHeight = 720;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

// Render view mode for Edit mode
enum class RenderViewMode {
    Material,    // Full PBR/textured
    Solid,       // Solid gray shading
    Wireframe    // Wireframe only
};

// Draw Manager - Orchestrates rendering pipelines
class DrawManager {
public:
    DrawManager() = default;
    ~DrawManager() = default;
    
    // Initialize
    bool init(UnifiedRenderer* renderer) {
        this->renderer = renderer;
        return renderer != nullptr;
    }
    
    void shutdown() {
        renderer = nullptr;
    }
    
    // Access renderer
    UnifiedRenderer* getRenderer() { return renderer; }
    
    // High-level render commands
    void beginFrame() {
        if (renderer) renderer->beginFrame();
    }
    
    void endFrame() {
        if (renderer) renderer->endFrame();
    }
    
    // Scene mode rendering (PBR)
    void renderSceneMode(const RenderContext& ctx, 
                        const std::function<void(Entity*)>& entityCallback) {
        if (!renderer) return;
        // Delegate to renderer's PBR pipeline
        // entityCallback is called for each entity to render
    }
    
    // Edit mode rendering
    void renderEditMode(const RenderContext& ctx,
                       RenderViewMode viewMode,
                       Entity* editingEntity,
                       EditMesh* editMesh) {
        if (!renderer || !editingEntity) return;
        
        // View mode determines how to render
        switch (viewMode) {
            case RenderViewMode::Material:
                // Use standard PBR rendering
                renderer->renderModel(editingEntity->model, editingEntity->worldMatrix.m);
                break;
                
            case RenderViewMode::Solid: {
                // Render with solid gray color
                float grayColor[4] = {0.6f, 0.6f, 0.65f, 1.0f};
                renderer->renderModelSolid(editingEntity->model, editingEntity->worldMatrix.m, grayColor);
                break;
            }
            
            case RenderViewMode::Wireframe: {
                // Render wireframe only (GPU-side, fast)
                float wireColor[4] = {0.4f, 0.5f, 0.6f, 1.0f};
                for (size_t meshIdx = 0; meshIdx < editingEntity->model.meshes.size(); ++meshIdx) {
                    const auto& gpuMesh = editingEntity->model.meshes[meshIdx];
                    if (gpuMesh.hasOriginalEdges) {
                        renderer->renderOriginalEdges(editingEntity->model, static_cast<int>(meshIdx),
                                                     editingEntity->worldMatrix.m, wireColor, false);
                    } else {
                        renderer->renderMeshWireframeOverlay(editingEntity->model, editingEntity->worldMatrix.m,
                                                           static_cast<int>(meshIdx), wireColor);
                    }
                }
                break;
            }
        }
    }
    
    // Render grid
    void renderGrid(const RHICameraParams& camParams, float sceneRadius) {
        if (renderer) {
            renderer->renderGrid(camParams, sceneRadius);
        }
    }
    
    // Render gizmo
    void renderGizmoLines(const float* lineData, uint32_t lineCount) {
        if (renderer) {
            renderer->renderGizmoLines(lineData, lineCount);
        }
    }
    
    // Render selection overlay (vertices, edges, faces)
    void renderSelectionOverlay(const RenderContext& ctx,
                               const EditMesh* editMesh,
                               const float* worldMatrix) {
        if (!renderer || !editMesh) return;
        // Selection overlay rendering is handled separately
        // This is a placeholder for future integration
    }
    
private:
    UnifiedRenderer* renderer = nullptr;
};

// Edit Mode Pipeline - Specialized rendering for mesh editing
class EditModePipeline {
public:
    EditModePipeline() = default;
    ~EditModePipeline() = default;
    
    bool init(UnifiedRenderer* renderer) {
        this->renderer = renderer;
        return renderer != nullptr;
    }
    
    void shutdown() {
        renderer = nullptr;
    }
    
    // X-Ray mode: when true, wireframe/selection shows through surfaces (Blender Alt+Z)
    // When false, occluded elements are hidden (default solid view behavior)
    void setXRayMode(bool enabled) { xRayMode = enabled; }
    bool getXRayMode() const { return xRayMode; }
    
    // Render based on view mode
    void render(const RenderContext& ctx,
               RenderViewMode viewMode,
               Entity* entity,
               int meshIndex = -1) {
        if (!renderer || !entity || !entity->hasModel) return;
        
        switch (viewMode) {
            case RenderViewMode::Material:
                renderer->renderModel(entity->model, entity->worldMatrix.m);
                break;
                
            case RenderViewMode::Solid: {
                float grayColor[4] = {0.6f, 0.6f, 0.65f, 1.0f};
                renderer->renderModelSolid(entity->model, entity->worldMatrix.m, grayColor);
                break;
            }
            
            case RenderViewMode::Wireframe:
                // When X-Ray is OFF, render depth pre-pass first (fills depth buffer)
                // so edge lines can be depth-tested for hidden line removal
                if (!xRayMode) {
                    renderer->renderModelDepthOnly(entity->model, entity->worldMatrix.m);
                }
                renderWireframe(entity, meshIndex);
                break;
        }
    }
    
    // Render wireframe overlay for specific mesh
    void renderWireframeOverlay(Entity* entity, int meshIndex, const float* color) {
        if (!renderer || !entity || !entity->hasModel) return;
        if (meshIndex < 0 || meshIndex >= static_cast<int>(entity->model.meshes.size())) return;
        
        const auto& gpuMesh = entity->model.meshes[meshIndex];
        // depthTest = !xRayMode: when X-Ray OFF, depth test against pre-pass for hidden line removal
        bool useDepthTest = !xRayMode;
        if (gpuMesh.hasOriginalEdges) {
            renderer->renderOriginalEdges(entity->model, meshIndex,
                                         entity->worldMatrix.m, color, useDepthTest);
        } else {
            renderer->renderMeshWireframeOverlay(entity->model, entity->worldMatrix.m,
                                                meshIndex, color);
        }
    }
    
    // Render selected vertices as points
    void renderSelectedVertices(const EditMesh* editMesh, const float* worldMatrix, const float* color) {
        if (!renderer || !editMesh || editMesh->selectedVertices.empty()) return;
        
        std::vector<float> pointLines;
        float pointSize = 0.015f;
        
        for (uint32_t vi : editMesh->selectedVertices) {
            if (vi >= editMesh->vertices.size()) continue;
            const auto& v = editMesh->vertices[vi];
            
            float wx = worldMatrix[0]*v.position[0] + worldMatrix[4]*v.position[1] + worldMatrix[8]*v.position[2] + worldMatrix[12];
            float wy = worldMatrix[1]*v.position[0] + worldMatrix[5]*v.position[1] + worldMatrix[9]*v.position[2] + worldMatrix[13];
            float wz = worldMatrix[2]*v.position[0] + worldMatrix[6]*v.position[1] + worldMatrix[10]*v.position[2] + worldMatrix[14];
            
            // Cross marker for each selected vertex
            pointLines.insert(pointLines.end(), {wx - pointSize, wy, wz, wx + pointSize, wy, wz, color[0], color[1], color[2], color[3]});
            pointLines.insert(pointLines.end(), {wx, wy - pointSize, wz, wx, wy + pointSize, wz, color[0], color[1], color[2], color[3]});
            pointLines.insert(pointLines.end(), {wx, wy, wz - pointSize, wx, wy, wz + pointSize, color[0], color[1], color[2], color[3]});
        }
        
        if (!pointLines.empty()) {
            if (xRayMode) {
                renderer->renderGizmoLines(pointLines.data(), static_cast<uint32_t>(pointLines.size() / 10));
            } else {
                renderer->renderGizmoLinesWithDepth(pointLines.data(), static_cast<uint32_t>(pointLines.size() / 10));
            }
        }
    }
    
    // Render selected edges
    void renderSelectedEdges(const EditMesh* editMesh, const float* worldMatrix, const float* color) {
        if (!renderer || !editMesh || editMesh->selectedEdges.empty()) return;
        
        std::vector<float> edgeLines;
        
        for (uint32_t ei : editMesh->selectedEdges) {
            if (ei >= editMesh->edges.size()) continue;
            const auto& edge = editMesh->edges[ei];
            
            if (edge.v0 >= editMesh->vertices.size() || edge.v1 >= editMesh->vertices.size()) continue;
            
            const auto& v0 = editMesh->vertices[edge.v0];
            const auto& v1 = editMesh->vertices[edge.v1];
            
            float w0x = worldMatrix[0]*v0.position[0] + worldMatrix[4]*v0.position[1] + worldMatrix[8]*v0.position[2] + worldMatrix[12];
            float w0y = worldMatrix[1]*v0.position[0] + worldMatrix[5]*v0.position[1] + worldMatrix[9]*v0.position[2] + worldMatrix[13];
            float w0z = worldMatrix[2]*v0.position[0] + worldMatrix[6]*v0.position[1] + worldMatrix[10]*v0.position[2] + worldMatrix[14];
            float w1x = worldMatrix[0]*v1.position[0] + worldMatrix[4]*v1.position[1] + worldMatrix[8]*v1.position[2] + worldMatrix[12];
            float w1y = worldMatrix[1]*v1.position[0] + worldMatrix[5]*v1.position[1] + worldMatrix[9]*v1.position[2] + worldMatrix[13];
            float w1z = worldMatrix[2]*v1.position[0] + worldMatrix[6]*v1.position[1] + worldMatrix[10]*v1.position[2] + worldMatrix[14];
            
            edgeLines.insert(edgeLines.end(), {w0x, w0y, w0z, w1x, w1y, w1z, color[0], color[1], color[2], color[3]});
        }
        
        if (!edgeLines.empty()) {
            if (xRayMode) {
                renderer->renderGizmoLines(edgeLines.data(), static_cast<uint32_t>(edgeLines.size() / 10));
            } else {
                renderer->renderGizmoLinesWithDepth(edgeLines.data(), static_cast<uint32_t>(edgeLines.size() / 10));
            }
        }
    }
    
    // Render selected faces (outline)
    void renderSelectedFaces(const EditMesh* editMesh, const float* worldMatrix, const float* color) {
        if (!renderer || !editMesh || editMesh->selectedFaces.empty()) return;
        
        std::vector<float> faceEdgeLines;
        
        for (uint32_t fi : editMesh->selectedFaces) {
            if (fi >= editMesh->faces.size()) continue;
            const auto& face = editMesh->faces[fi];
            
            for (size_t i = 0; i < face.loops.size(); ++i) {
                size_t nextI = (i + 1) % face.loops.size();
                uint32_t vi0 = face.loops[i].vertexIndex;
                uint32_t vi1 = face.loops[nextI].vertexIndex;
                
                if (vi0 >= editMesh->vertices.size() || vi1 >= editMesh->vertices.size()) continue;
                
                const auto& v0 = editMesh->vertices[vi0];
                const auto& v1 = editMesh->vertices[vi1];
                
                float w0x = worldMatrix[0]*v0.position[0] + worldMatrix[4]*v0.position[1] + worldMatrix[8]*v0.position[2] + worldMatrix[12];
                float w0y = worldMatrix[1]*v0.position[0] + worldMatrix[5]*v0.position[1] + worldMatrix[9]*v0.position[2] + worldMatrix[13];
                float w0z = worldMatrix[2]*v0.position[0] + worldMatrix[6]*v0.position[1] + worldMatrix[10]*v0.position[2] + worldMatrix[14];
                float w1x = worldMatrix[0]*v1.position[0] + worldMatrix[4]*v1.position[1] + worldMatrix[8]*v1.position[2] + worldMatrix[12];
                float w1y = worldMatrix[1]*v1.position[0] + worldMatrix[5]*v1.position[1] + worldMatrix[9]*v1.position[2] + worldMatrix[13];
                float w1z = worldMatrix[2]*v1.position[0] + worldMatrix[6]*v1.position[1] + worldMatrix[10]*v1.position[2] + worldMatrix[14];
                
                faceEdgeLines.insert(faceEdgeLines.end(), {w0x, w0y, w0z, w1x, w1y, w1z, color[0], color[1], color[2], color[3]});
            }
        }
        
        if (!faceEdgeLines.empty()) {
            if (xRayMode) {
                renderer->renderGizmoLines(faceEdgeLines.data(), static_cast<uint32_t>(faceEdgeLines.size() / 10));
            } else {
                renderer->renderGizmoLinesWithDepth(faceEdgeLines.data(), static_cast<uint32_t>(faceEdgeLines.size() / 10));
            }
        }
    }
    
private:
    UnifiedRenderer* renderer = nullptr;
    bool xRayMode = false;  // Default: X-Ray OFF (hidden line removal)
    
    void renderWireframe(Entity* entity, int meshIndex) {
        float wireColor[4] = {0.4f, 0.5f, 0.6f, 1.0f};
        
        if (meshIndex >= 0 && meshIndex < static_cast<int>(entity->model.meshes.size())) {
            // Render specific mesh
            renderWireframeOverlay(entity, meshIndex, wireColor);
        } else {
            // Render all meshes
            for (size_t i = 0; i < entity->model.meshes.size(); ++i) {
                renderWireframeOverlay(entity, static_cast<int>(i), wireColor);
            }
        }
    }
};

} // namespace luma
