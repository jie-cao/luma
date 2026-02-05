// LUMA Edit Mode Renderer
// 编辑模式专用渲染器 - 参考 Blender/Maya 架构
// 完全接管编辑模式下的渲染，不依赖 main.cpp 的渲染逻辑
#pragma once

#include "editor_mode.h"
#include "mode_ui.h"
#include "../mesh/edit_mesh.h"
#include "../renderer/unified_renderer.h"
#include "../scene/entity.h"
#include <vector>
#include <set>
#include <cstring>

// Forward declaration
namespace luma {
    struct RHICameraParams;
}

namespace luma {
namespace editor {

// ============================================================================
// EditModeRenderer - 编辑模式渲染器
// ============================================================================
// 
// 职责：
// 1. 完全接管编辑模式下的3D渲染
// 2. 根据 ViewMode 渲染不同效果（Material/Solid/Wireframe）
// 3. 渲染选中高亮（顶点/边/面）
// 4. 正确初始化渲染状态，确保所有模式都能正常显示
//
// 设计理念（参考 Blender）：
// - 编辑模式是独立的渲染模式，不是在场景渲染上叠加
// - 先渲染背景（网格/天空），再渲染编辑中的物体，最后渲染覆盖层
// - 每个 Pass 都有明确的职责和正确的渲染状态
//
class EditModeRenderer {
public:
    // =========================================================================
    // 配置
    // =========================================================================
    
    // 颜色配置
    struct Colors {
        float wireframe[4] = {0.4f, 0.5f, 0.6f, 1.0f};      // 线框颜色（淡蓝灰）
        float solidGray[4] = {0.6f, 0.6f, 0.65f, 1.0f};     // 实体模式灰色
        float selectedVertex[4] = {1.0f, 0.5f, 0.0f, 1.0f}; // 选中顶点（橙色）
        float selectedEdge[4] = {1.0f, 0.6f, 0.0f, 1.0f};   // 选中边（橙色）
        float selectedFace[4] = {1.0f, 0.5f, 0.0f, 0.3f};   // 选中面（半透明橙）
        float vertexPoint[4] = {0.2f, 0.2f, 0.2f, 1.0f};    // 普通顶点（深灰）
        float background[4] = {0.15f, 0.16f, 0.18f, 1.0f};  // 背景色
    } colors;
    
    // 显示选项
    struct DisplayOptions {
        bool showGrid = true;           // 显示网格
        bool showWireframeOverlay = true; // 材质/实体模式下叠加线框
        bool showVerticesInVertexMode = true; // 点模式显示所有顶点
        float vertexPointSize = 3.0f;   // 顶点大小
        float selectedVertexSize = 5.0f; // 选中顶点大小
        float wireframeWidth = 1.0f;    // 线框宽度
    } display;
    
    // =========================================================================
    // 主渲染入口
    // =========================================================================
    
    // 渲染编辑模式（主入口）
    // 这个函数完全接管编辑模式的渲染，main.cpp 只需要调用这一个函数
    void render(
        UnifiedRenderer& renderer,
        Entity* entity,              // 正在编辑的实体
        ViewMode viewMode,           // 视图模式
        EditMesh* editMesh,          // 编辑网格数据
        int selectedMeshIndex,       // 选中的 mesh 索引
        EditModeToolbar::SelectMode selectMode,  // 选择模式（点/线/面）
        const RHICameraParams& camera,  // 相机参数
        float sceneRadius            // 场景半径（用于网格）
    ) {
        if (!entity || !entity->hasModel) return;
        
        // =====================================================================
        // PASS 0: 背景渲染（网格）
        // =====================================================================
        if (display.showGrid) {
            renderer.renderGrid(camera, sceneRadius);
        }
        
        // =====================================================================
        // PASS 1: 主体渲染（根据 ViewMode）
        // =====================================================================
        switch (viewMode) {
            case ViewMode::Material:
                renderMaterialMode(renderer, entity);
                break;
                
            case ViewMode::Solid:
                renderSolidMode(renderer, entity);
                break;
                
            case ViewMode::Wireframe:
                renderWireframeMode(renderer, entity);
                break;
        }
        
        // =====================================================================
        // PASS 2: 线框覆盖层（Material/Solid 模式下的线框叠加）
        // =====================================================================
        if (viewMode != ViewMode::Wireframe && display.showWireframeOverlay) {
            renderWireframeOverlay(renderer, entity, selectedMeshIndex);
        }
        
        // =====================================================================
        // PASS 3: 选中高亮渲染
        // =====================================================================
        if (editMesh && selectedMeshIndex >= 0) {
            renderSelectionHighlight(renderer, entity, editMesh, selectedMeshIndex, selectMode);
        }
        
        // =====================================================================
        // PASS 4: 顶点显示（点模式）
        // =====================================================================
        if (selectMode == EditModeToolbar::SelectMode::Vertex && 
            display.showVerticesInVertexMode && editMesh) {
            renderVertexPoints(renderer, entity, editMesh, selectedMeshIndex);
        }
    }
    
private:
    // =========================================================================
    // PASS 1: 材质模式渲染
    // =========================================================================
    void renderMaterialMode(UnifiedRenderer& renderer, Entity* entity) {
        // 正常 PBR 渲染
        renderer.renderModel(entity->model, entity->worldMatrix.m);
    }
    
    // =========================================================================
    // PASS 1: 实体模式渲染（灰色 Clay）
    // =========================================================================
    void renderSolidMode(UnifiedRenderer& renderer, Entity* entity) {
        renderer.renderModelSolid(entity->model, entity->worldMatrix.m, colors.solidGray);
    }
    
    // =========================================================================
    // PASS 1: 线框模式渲染（纯线框，无实体）
    // =========================================================================
    void renderWireframeMode(UnifiedRenderer& renderer, Entity* entity) {
        // 线框模式：只渲染原始四边面边，不渲染实体
        // 这是编辑模式的核心功能
        
        for (size_t meshIdx = 0; meshIdx < entity->model.meshes.size(); ++meshIdx) {
            const auto& gpuMesh = entity->model.meshes[meshIdx];
            
            if (gpuMesh.hasOriginalEdges) {
                // 有四边面数据，渲染原始边
                if (gpuMesh.hasSkinning && entity->hasSkeleton()) {
                    Mat4 boneMatrices[MAX_BONES];
                    entity->getSkinningMatrices(boneMatrices);
                    renderer.renderOriginalEdgesSkinned(
                        entity->model, static_cast<int>(meshIdx),
                        entity->worldMatrix.m, colors.wireframe,
                        reinterpret_cast<const float*>(boneMatrices),
                        gpuMesh.skinnedVertices);
                } else {
                    renderer.renderOriginalEdges(
                        entity->model, static_cast<int>(meshIdx),
                        entity->worldMatrix.m, colors.wireframe);
                }
            } else {
                // 没有四边面数据，渲染三角化边（fallback）
                renderer.renderMeshWireframeOverlay(
                    entity->model, entity->worldMatrix.m,
                    static_cast<int>(meshIdx), colors.wireframe);
            }
        }
    }
    
    // =========================================================================
    // PASS 2: 线框覆盖层（在实体上叠加线框）
    // =========================================================================
    void renderWireframeOverlay(UnifiedRenderer& renderer, Entity* entity, int selectedMeshIndex) {
        // 只渲染选中的 mesh 的线框覆盖
        if (selectedMeshIndex < 0 || 
            selectedMeshIndex >= static_cast<int>(entity->model.meshes.size())) {
            return;
        }
        
        const auto& gpuMesh = entity->model.meshes[selectedMeshIndex];
        
        if (gpuMesh.hasOriginalEdges) {
            if (gpuMesh.hasSkinning && entity->hasSkeleton()) {
                Mat4 boneMatrices[MAX_BONES];
                entity->getSkinningMatrices(boneMatrices);
                renderer.renderOriginalEdgesSkinned(
                    entity->model, selectedMeshIndex,
                    entity->worldMatrix.m, colors.wireframe,
                    reinterpret_cast<const float*>(boneMatrices),
                    gpuMesh.skinnedVertices);
            } else {
                renderer.renderOriginalEdges(
                    entity->model, selectedMeshIndex,
                    entity->worldMatrix.m, colors.wireframe);
            }
        } else {
            renderer.renderMeshWireframeOverlay(
                entity->model, entity->worldMatrix.m,
                selectedMeshIndex, colors.wireframe);
        }
    }
    
    // =========================================================================
    // PASS 3: 选中高亮渲染
    // =========================================================================
    void renderSelectionHighlight(
        UnifiedRenderer& renderer,
        Entity* entity,
        EditMesh* editMesh,
        int meshIndex,
        EditModeToolbar::SelectMode selectMode
    ) {
        if (!editMesh) return;
        
        const float* worldMat = entity->worldMatrix.m;
        
        switch (selectMode) {
            case EditModeToolbar::SelectMode::Vertex:
                renderSelectedVertices(renderer, editMesh, worldMat);
                break;
                
            case EditModeToolbar::SelectMode::Edge:
                renderSelectedEdges(renderer, editMesh, worldMat);
                break;
                
            case EditModeToolbar::SelectMode::Face:
                renderSelectedFaces(renderer, editMesh, worldMat);
                break;
        }
    }
    
    // =========================================================================
    // 渲染选中的顶点（橙色十字）
    // =========================================================================
    void renderSelectedVertices(UnifiedRenderer& renderer, EditMesh* editMesh, const float* worldMat) {
        if (editMesh->selectedVertices.empty()) return;
        
        std::vector<float> pointLines;
        float pointSize = display.selectedVertexSize * 0.01f;
        
        for (uint32_t vi : editMesh->selectedVertices) {
            if (vi >= editMesh->vertices.size()) continue;
            const auto& v = editMesh->vertices[vi];
            
            float wx = worldMat[0]*v.position[0] + worldMat[4]*v.position[1] + worldMat[8]*v.position[2] + worldMat[12];
            float wy = worldMat[1]*v.position[0] + worldMat[5]*v.position[1] + worldMat[9]*v.position[2] + worldMat[13];
            float wz = worldMat[2]*v.position[0] + worldMat[6]*v.position[1] + worldMat[10]*v.position[2] + worldMat[14];
            
            // 三轴十字
            addLine(pointLines, wx - pointSize, wy, wz, wx + pointSize, wy, wz, colors.selectedVertex);
            addLine(pointLines, wx, wy - pointSize, wz, wx, wy + pointSize, wz, colors.selectedVertex);
            addLine(pointLines, wx, wy, wz - pointSize, wx, wy, wz + pointSize, colors.selectedVertex);
        }
        
        if (!pointLines.empty()) {
            renderer.renderGizmoLines(pointLines.data(), static_cast<uint32_t>(pointLines.size() / 10));
        }
    }
    
    // =========================================================================
    // 渲染选中的边（橙色线）
    // =========================================================================
    void renderSelectedEdges(UnifiedRenderer& renderer, EditMesh* editMesh, const float* worldMat) {
        if (editMesh->selectedEdges.empty()) return;
        
        std::vector<float> edgeLines;
        
        for (uint32_t ei : editMesh->selectedEdges) {
            if (ei >= editMesh->edges.size()) continue;
            const auto& edge = editMesh->edges[ei];
            
            if (edge.v0 >= editMesh->vertices.size() || edge.v1 >= editMesh->vertices.size()) continue;
            
            const auto& v0 = editMesh->vertices[edge.v0];
            const auto& v1 = editMesh->vertices[edge.v1];
            
            float w0[3], w1[3];
            transformVertex(v0.position, worldMat, w0);
            transformVertex(v1.position, worldMat, w1);
            
            addLine(edgeLines, w0[0], w0[1], w0[2], w1[0], w1[1], w1[2], colors.selectedEdge);
        }
        
        if (!edgeLines.empty()) {
            renderer.renderGizmoLines(edgeLines.data(), static_cast<uint32_t>(edgeLines.size() / 10));
        }
    }
    
    // =========================================================================
    // 渲染选中的面（半透明橙色填充 + 边框）
    // =========================================================================
    void renderSelectedFaces(UnifiedRenderer& renderer, EditMesh* editMesh, const float* worldMat) {
        if (editMesh->selectedFaces.empty()) return;
        
        // TODO: 实现面的半透明填充渲染
        // 目前先用边框高亮
        
        std::vector<float> faceLines;
        
        for (uint32_t fi : editMesh->selectedFaces) {
            if (fi >= editMesh->faces.size()) continue;
            const auto& face = editMesh->faces[fi];
            
            // 渲染面的所有边
            for (size_t i = 0; i < face.loops.size(); ++i) {
                uint32_t v0Idx = face.loops[i].vertexIndex;
                uint32_t v1Idx = face.loops[(i + 1) % face.loops.size()].vertexIndex;
                
                if (v0Idx >= editMesh->vertices.size() || v1Idx >= editMesh->vertices.size()) continue;
                
                const auto& v0 = editMesh->vertices[v0Idx];
                const auto& v1 = editMesh->vertices[v1Idx];
                
                float w0[3], w1[3];
                transformVertex(v0.position, worldMat, w0);
                transformVertex(v1.position, worldMat, w1);
                
                addLine(faceLines, w0[0], w0[1], w0[2], w1[0], w1[1], w1[2], colors.selectedEdge);
            }
        }
        
        if (!faceLines.empty()) {
            renderer.renderGizmoLines(faceLines.data(), static_cast<uint32_t>(faceLines.size() / 10));
        }
    }
    
    // =========================================================================
    // PASS 4: 渲染所有顶点（点模式）
    // =========================================================================
    void renderVertexPoints(
        UnifiedRenderer& renderer,
        Entity* entity,
        EditMesh* editMesh,
        int meshIndex
    ) {
        if (!editMesh || editMesh->vertices.empty()) return;
        
        const float* worldMat = entity->worldMatrix.m;
        std::vector<float> pointLines;
        float pointSize = display.vertexPointSize * 0.005f;
        
        for (size_t vi = 0; vi < editMesh->vertices.size(); ++vi) {
            const auto& v = editMesh->vertices[vi];
            
            // 检查是否选中
            bool isSelected = editMesh->selectedVertices.count(static_cast<uint32_t>(vi)) > 0;
            const float* color = isSelected ? colors.selectedVertex : colors.vertexPoint;
            float size = isSelected ? display.selectedVertexSize * 0.01f : pointSize;
            
            float wx = worldMat[0]*v.position[0] + worldMat[4]*v.position[1] + worldMat[8]*v.position[2] + worldMat[12];
            float wy = worldMat[1]*v.position[0] + worldMat[5]*v.position[1] + worldMat[9]*v.position[2] + worldMat[13];
            float wz = worldMat[2]*v.position[0] + worldMat[6]*v.position[1] + worldMat[10]*v.position[2] + worldMat[14];
            
            // 简单十字标记顶点
            addLine(pointLines, wx - size, wy, wz, wx + size, wy, wz, color);
            addLine(pointLines, wx, wy - size, wz, wx, wy + size, wz, color);
        }
        
        if (!pointLines.empty()) {
            renderer.renderGizmoLines(pointLines.data(), static_cast<uint32_t>(pointLines.size() / 10));
        }
    }
    
    // =========================================================================
    // 工具函数
    // =========================================================================
    
    void transformVertex(const float* pos, const float* mat, float* out) const {
        out[0] = mat[0]*pos[0] + mat[4]*pos[1] + mat[8]*pos[2] + mat[12];
        out[1] = mat[1]*pos[0] + mat[5]*pos[1] + mat[9]*pos[2] + mat[13];
        out[2] = mat[2]*pos[0] + mat[6]*pos[1] + mat[10]*pos[2] + mat[14];
    }
    
    void addLine(std::vector<float>& lines, 
                 float x0, float y0, float z0,
                 float x1, float y1, float z1,
                 const float* color) const {
        // 格式: p0.xyz, p1.xyz, color.rgba
        lines.insert(lines.end(), {x0, y0, z0, x1, y1, z1, color[0], color[1], color[2], color[3]});
    }
};

} // namespace editor
} // namespace luma
