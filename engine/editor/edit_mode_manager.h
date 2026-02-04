// LUMA Edit Mode Manager
// 管理编辑模式下的 EditMesh 数据和保存时的转换
// 核心职责：场景模式 ↔ 编辑模式 的数据转换

#pragma once

#include "../mesh/edit_mesh.h"
#include "../mesh/render_mesh.h"
#include "../mesh/mesh_converter.h"
#include <memory>
#include <functional>
#include <iostream>

namespace luma {
namespace editor {

// ============================================================================
// 编辑模式状态
// ============================================================================
enum class EditModeState {
    Inactive,       // 未在编辑模式
    Active,         // 正在编辑
    PendingSave,    // 等待保存确认（有 UV 问题等）
};

// ============================================================================
// 编辑模式选择类型
// ============================================================================
enum class SelectionMode {
    Vertex,     // 顶点选择
    Edge,       // 边选择
    Face,       // 面选择
};

// ============================================================================
// 编辑模式工具
// ============================================================================
enum class EditTool {
    Select,         // 选择工具
    Move,           // 移动
    Rotate,         // 旋转
    Scale,          // 缩放
    Extrude,        // 挤出
    Inset,          // 内插
    Bevel,          // 倒角
    LoopCut,        // 环切
    Knife,          // 切割
};

// ============================================================================
// 保存选项
// ============================================================================
struct SaveOptions {
    bool optimize = true;           // 优化顶点缓存
    bool mergeVertices = true;      // 合并重复顶点
    bool recalculateNormals = false; // 重新计算法线
    bool autoRepairUV = true;       // 自动修复 UV 问题
};

// ============================================================================
// EditModeManager - 编辑模式管理器
// ============================================================================
class EditModeManager {
public:
    // =========================================================================
    // 回调函数
    // =========================================================================
    
    // 进入/退出编辑模式
    std::function<void(void* entity)> onEnterEditMode;
    std::function<void(void* entity, bool saved)> onExitEditMode;
    
    // 网格数据更新（需要重新上传 GPU）
    std::function<void(void* entity, const RenderMesh& newMesh)> onMeshUpdated;
    
    // 显示确认对话框
    std::function<bool(const std::string& title, const std::string& message)> showConfirmDialog;
    
    // 显示 UV 修复对话框
    std::function<bool(const std::vector<UVProblem>& problems)> showUVRepairDialog;
    
    // =========================================================================
    // 状态
    // =========================================================================
    
    EditModeState state() const { return state_; }
    SelectionMode selectionMode() const { return selectionMode_; }
    EditTool currentTool() const { return currentTool_; }
    
    bool isActive() const { return state_ != EditModeState::Inactive; }
    bool hasUnsavedChanges() const { return editMesh_ && editMesh_->hasModifications(); }
    
    void* editingEntity() const { return editingEntity_; }
    EditMesh* editMesh() { return editMesh_.get(); }
    const EditMesh* editMesh() const { return editMesh_.get(); }
    
    // =========================================================================
    // 进入编辑模式
    // =========================================================================
    
    bool enterEditMode(void* entity, const RenderMesh& currentMesh) {
        if (state_ != EditModeState::Inactive) {
            // 已经在编辑模式，先退出
            if (!exitEditMode(true)) {
                return false;
            }
        }
        
        editingEntity_ = entity;
        
        // 保存原始网格（用于取消）
        originalMesh_ = currentMesh;
        
        // 转换为 EditMesh
        editMesh_ = std::make_unique<EditMesh>(
            MeshConverter::toEditMesh(currentMesh)
        );
        
        state_ = EditModeState::Active;
        selectionMode_ = SelectionMode::Face;
        currentTool_ = EditTool::Select;
        
        std::cout << "[EditModeManager] Entered edit mode for entity " << entity << std::endl;
        std::cout << "  Vertices: " << editMesh_->vertices.size() << std::endl;
        std::cout << "  Faces: " << editMesh_->faces.size() << std::endl;
        std::cout << "  Edges: " << editMesh_->edges.size() << std::endl;
        std::cout << "  Quads: " << editMesh_->quadCount() << std::endl;
        std::cout << "  Triangles: " << editMesh_->triangleCount() << std::endl;
        
        if (onEnterEditMode) {
            onEnterEditMode(entity);
        }
        
        return true;
    }
    
    // 从 Assimp 数据进入编辑模式（保留原始四边面）
    bool enterEditModeFromAssimp(void* entity, const aiMesh* assimpMesh, 
                                  const RenderMesh& currentRenderMesh) {
        if (state_ != EditModeState::Inactive) {
            if (!exitEditMode(true)) {
                return false;
            }
        }
        
        editingEntity_ = entity;
        originalMesh_ = currentRenderMesh;
        
        // 从 Assimp 数据创建 EditMesh（保留四边面！）
        editMesh_ = std::make_unique<EditMesh>(
            MeshConverter::fromAssimpMesh(assimpMesh)
        );
        
        state_ = EditModeState::Active;
        selectionMode_ = SelectionMode::Face;
        currentTool_ = EditTool::Select;
        
        std::cout << "[EditModeManager] Entered edit mode (from Assimp) for entity " << entity << std::endl;
        std::cout << "  Vertices: " << editMesh_->vertices.size() << std::endl;
        std::cout << "  Faces: " << editMesh_->faces.size() << std::endl;
        std::cout << "  Quads: " << editMesh_->quadCount() << " (preserved!)" << std::endl;
        std::cout << "  N-gons: " << editMesh_->ngonCount() << std::endl;
        
        if (onEnterEditMode) {
            onEnterEditMode(entity);
        }
        
        return true;
    }
    
    // =========================================================================
    // 退出编辑模式
    // =========================================================================
    
    bool exitEditMode(bool saveChanges) {
        if (state_ == EditModeState::Inactive) {
            return true;
        }
        
        if (saveChanges && hasUnsavedChanges()) {
            // 检查 UV 问题
            auto problems = editMesh_->analyzeUV();
            
            if (!problems.empty()) {
                std::cout << "[EditModeManager] Found " << problems.size() << " UV problems" << std::endl;
                
                // 显示修复对话框
                if (showUVRepairDialog) {
                    bool shouldRepair = showUVRepairDialog(problems);
                    if (shouldRepair) {
                        // 自动修复
                        autoRepairUV();
                    }
                }
            }
            
            // 转换回 RenderMesh（GPU 优化格式）
            RenderMesh newMesh = commitChanges();
            
            // 通知上层更新 GPU 数据
            if (onMeshUpdated) {
                onMeshUpdated(editingEntity_, newMesh);
            }
            
            std::cout << "[EditModeManager] Saved changes and converted to RenderMesh" << std::endl;
            std::cout << "  Final vertices: " << newMesh.vertices.size() << std::endl;
            std::cout << "  Final triangles: " << newMesh.triangleCount() << std::endl;
        } else if (!saveChanges) {
            std::cout << "[EditModeManager] Discarded changes" << std::endl;
        }
        
        void* entity = editingEntity_;
        
        // 清理
        editMesh_.reset();
        editingEntity_ = nullptr;
        state_ = EditModeState::Inactive;
        
        if (onExitEditMode) {
            onExitEditMode(entity, saveChanges);
        }
        
        return true;
    }
    
    // =========================================================================
    // 提交修改（转换为 RenderMesh）
    // =========================================================================
    
    RenderMesh commitChanges(const SaveOptions& options = SaveOptions()) {
        if (!editMesh_) {
            return RenderMesh();
        }
        
        // 可选：重新计算法线
        if (options.recalculateNormals) {
            recalculateNormals();
        }
        
        // 可选：自动修复 UV
        if (options.autoRepairUV) {
            autoRepairUV();
        }
        
        // 转换为 RenderMesh（三角化 + 优化）
        RenderMesh result = MeshConverter::toRenderMesh(*editMesh_, options.optimize);
        
        // 可选：合并重复顶点
        if (options.mergeVertices) {
            result.mergeVertices();
        }
        
        // 计算包围盒
        result.calculateBounds();
        
        std::cout << "[EditModeManager] Committed changes:" << std::endl;
        std::cout << "  EditMesh: " << editMesh_->vertices.size() << " verts, " 
                  << editMesh_->faces.size() << " faces" << std::endl;
        std::cout << "  RenderMesh: " << result.vertices.size() << " verts, "
                  << result.triangleCount() << " tris" << std::endl;
        std::cout << "  Memory: " << result.memoryUsage() / 1024 << " KB" << std::endl;
        
        return result;
    }
    
    // =========================================================================
    // 选择模式切换
    // =========================================================================
    
    void setSelectionMode(SelectionMode mode) {
        if (selectionMode_ != mode) {
            // 清除当前选择
            if (editMesh_) {
                editMesh_->selectNone();
            }
            selectionMode_ = mode;
            std::cout << "[EditModeManager] Selection mode: " << getSelectionModeName(mode) << std::endl;
        }
    }
    
    static const char* getSelectionModeName(SelectionMode mode) {
        switch (mode) {
            case SelectionMode::Vertex: return "顶点";
            case SelectionMode::Edge: return "边";
            case SelectionMode::Face: return "面";
            default: return "未知";
        }
    }
    
    // =========================================================================
    // 工具切换
    // =========================================================================
    
    void setTool(EditTool tool) {
        currentTool_ = tool;
        std::cout << "[EditModeManager] Tool: " << getToolName(tool) << std::endl;
    }
    
    static const char* getToolName(EditTool tool) {
        switch (tool) {
            case EditTool::Select: return "选择";
            case EditTool::Move: return "移动";
            case EditTool::Rotate: return "旋转";
            case EditTool::Scale: return "缩放";
            case EditTool::Extrude: return "挤出";
            case EditTool::Inset: return "内插";
            case EditTool::Bevel: return "倒角";
            case EditTool::LoopCut: return "环切";
            case EditTool::Knife: return "切割";
            default: return "未知";
        }
    }
    
    // =========================================================================
    // Undo/Redo
    // =========================================================================
    
    bool canUndo() const { return editMesh_ && editMesh_->canUndo(); }
    bool canRedo() const { return editMesh_ && editMesh_->canRedo(); }
    
    void undo() {
        if (editMesh_) {
            editMesh_->undo();
            std::cout << "[EditModeManager] Undo" << std::endl;
        }
    }
    
    void redo() {
        if (editMesh_) {
            editMesh_->redo();
            std::cout << "[EditModeManager] Redo" << std::endl;
        }
    }
    
    // =========================================================================
    // UV 操作
    // =========================================================================
    
    void autoRepairUV() {
        if (!editMesh_) return;
        
        auto problems = editMesh_->analyzeUV();
        
        for (const auto& prob : problems) {
            switch (prob.type) {
                case UVProblem::Stretching:
                    // 对高拉伸的面重新投影
                    for (uint32_t fi : prob.affectedFaces) {
                        editMesh_->selectedFaces.insert(fi);
                    }
                    editMesh_->projectUVBox();
                    editMesh_->selectedFaces.clear();
                    break;
                    
                case UVProblem::Missing:
                    // 为缺失 UV 的面投影
                    for (uint32_t fi : prob.affectedFaces) {
                        editMesh_->selectedFaces.insert(fi);
                    }
                    editMesh_->projectUVBox();
                    editMesh_->selectedFaces.clear();
                    break;
                    
                default:
                    break;
            }
        }
        
        std::cout << "[EditModeManager] Auto-repaired " << problems.size() << " UV problems" << std::endl;
    }
    
    // =========================================================================
    // 法线重算
    // =========================================================================
    
    void recalculateNormals() {
        if (!editMesh_) return;
        
        for (auto& face : editMesh_->faces) {
            face.calculateNormal(editMesh_->vertices);
        }
        
        // TODO: 实现平滑法线计算（基于顶点共享）
        
        std::cout << "[EditModeManager] Recalculated normals" << std::endl;
    }
    
    // =========================================================================
    // 线框数据生成（用于编辑模式显示）
    // =========================================================================
    
    // 获取原始边线框（显示四边面）
    void getOriginalEdgeWireframe(std::vector<float>& outVertices,
                                   std::vector<uint32_t>& outIndices) const {
        if (editMesh_) {
            editMesh_->generateOriginalEdgeWireframe(outVertices, outIndices);
        }
    }
    
    // 获取所有边线框（包括三角化边）
    void getAllEdgeWireframe(std::vector<float>& outVertices,
                             std::vector<uint32_t>& outIndices) const {
        if (editMesh_) {
            editMesh_->generateAllEdgeWireframe(outVertices, outIndices);
        }
    }
    
    // =========================================================================
    // 取消编辑（恢复原始网格）
    // =========================================================================
    
    RenderMesh cancelAndGetOriginal() {
        if (state_ != EditModeState::Inactive) {
            exitEditMode(false);  // 不保存
        }
        return originalMesh_;
    }

private:
    EditModeState state_ = EditModeState::Inactive;
    SelectionMode selectionMode_ = SelectionMode::Face;
    EditTool currentTool_ = EditTool::Select;
    
    void* editingEntity_ = nullptr;
    std::unique_ptr<EditMesh> editMesh_;
    RenderMesh originalMesh_;  // 用于取消时恢复
};

} // namespace editor
} // namespace luma
