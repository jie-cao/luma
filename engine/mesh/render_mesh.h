// LUMA RenderMesh - GPU 优化的网格数据结构
// 用于场景模式的高效渲染
// 从 EditMesh 转换而来，三角化 + 交错顶点布局

#pragma once

#include <vector>
#include <set>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace luma {

// ============================================================================
// RenderVertex - GPU 顶点（交错布局，缓存友好）
// ============================================================================
struct RenderVertex {
    float position[3];    // 12 bytes - 位置
    float normal[3];      // 12 bytes - 法线
    float tangent[4];     // 16 bytes - 切线 (xyz) + handedness (w)
    float uv[2];          // 8 bytes  - UV 坐标
    float color[3];       // 12 bytes - 顶点色 (可选)
    // Total: 60 bytes
    
    RenderVertex() {
        memset(this, 0, sizeof(RenderVertex));
        normal[1] = 1.0f;      // 默认法线朝上
        tangent[0] = 1.0f;     // 默认切线
        tangent[3] = 1.0f;     // handedness
        color[0] = color[1] = color[2] = 1.0f;  // 默认白色
    }
};

// ============================================================================
// RenderSubMesh - 子网格（对应一个材质）
// ============================================================================
struct RenderSubMesh {
    uint32_t indexOffset = 0;     // 在索引缓冲中的起始位置
    uint32_t indexCount = 0;      // 索引数量
    uint32_t materialIndex = 0;   // 材质索引
    
    // 包围盒（用于剔除）
    float boundsMin[3] = {0, 0, 0};
    float boundsMax[3] = {0, 0, 0};
};

// ============================================================================
// RenderMesh - GPU 优化的渲染网格
// ============================================================================
class RenderMesh {
public:
    // === 数据 ===
    std::vector<RenderVertex> vertices;
    std::vector<uint32_t> indices;          // 三角形索引
    std::vector<RenderSubMesh> subMeshes;   // 子网格（按材质分组）
    
    // === 包围盒 ===
    float boundsMin[3] = {0, 0, 0};
    float boundsMax[3] = {0, 0, 0};
    float boundsCenter[3] = {0, 0, 0};
    float boundsRadius = 0.0f;
    
    // === GPU 资源句柄（由渲染器管理）===
    uint64_t gpuVertexBuffer = 0;
    uint64_t gpuIndexBuffer = 0;
    bool gpuDataValid = false;
    
    // =========================================================================
    // 构造
    // =========================================================================
    
    RenderMesh() = default;
    
    // 清空
    void clear() {
        vertices.clear();
        indices.clear();
        subMeshes.clear();
        gpuDataValid = false;
    }
    
    // =========================================================================
    // 统计信息
    // =========================================================================
    
    uint32_t vertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t indexCount() const { return static_cast<uint32_t>(indices.size()); }
    uint32_t triangleCount() const { return static_cast<uint32_t>(indices.size() / 3); }
    uint32_t subMeshCount() const { return static_cast<uint32_t>(subMeshes.size()); }
    
    // 内存占用（字节）
    size_t memoryUsage() const {
        return vertices.size() * sizeof(RenderVertex) + 
               indices.size() * sizeof(uint32_t);
    }
    
    // =========================================================================
    // 包围盒计算
    // =========================================================================
    
    void calculateBounds() {
        if (vertices.empty()) {
            boundsMin[0] = boundsMin[1] = boundsMin[2] = 0;
            boundsMax[0] = boundsMax[1] = boundsMax[2] = 0;
            boundsCenter[0] = boundsCenter[1] = boundsCenter[2] = 0;
            boundsRadius = 0;
            return;
        }
        
        boundsMin[0] = boundsMin[1] = boundsMin[2] = 1e10f;
        boundsMax[0] = boundsMax[1] = boundsMax[2] = -1e10f;
        
        for (const auto& v : vertices) {
            for (int i = 0; i < 3; i++) {
                boundsMin[i] = std::min(boundsMin[i], v.position[i]);
                boundsMax[i] = std::max(boundsMax[i], v.position[i]);
            }
        }
        
        for (int i = 0; i < 3; i++) {
            boundsCenter[i] = (boundsMin[i] + boundsMax[i]) * 0.5f;
        }
        
        // 计算包围球半径
        boundsRadius = 0;
        for (const auto& v : vertices) {
            float dx = v.position[0] - boundsCenter[0];
            float dy = v.position[1] - boundsCenter[1];
            float dz = v.position[2] - boundsCenter[2];
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            boundsRadius = std::max(boundsRadius, dist);
        }
        
        // 更新子网格包围盒
        for (auto& sub : subMeshes) {
            calculateSubMeshBounds(sub);
        }
    }
    
    void calculateSubMeshBounds(RenderSubMesh& sub) {
        sub.boundsMin[0] = sub.boundsMin[1] = sub.boundsMin[2] = 1e10f;
        sub.boundsMax[0] = sub.boundsMax[1] = sub.boundsMax[2] = -1e10f;
        
        for (uint32_t i = sub.indexOffset; i < sub.indexOffset + sub.indexCount; i++) {
            uint32_t vi = indices[i];
            const auto& v = vertices[vi];
            for (int j = 0; j < 3; j++) {
                sub.boundsMin[j] = std::min(sub.boundsMin[j], v.position[j]);
                sub.boundsMax[j] = std::max(sub.boundsMax[j], v.position[j]);
            }
        }
    }
    
    // =========================================================================
    // 优化
    // =========================================================================
    
    // 顶点缓存优化（改善渲染性能）
    void optimizeVertexCache() {
        // TODO: 实现 Tom Forsyth 的顶点缓存优化算法
        // 或使用 meshoptimizer 库
    }
    
    // 合并重复顶点
    void mergeVertices(float positionThreshold = 1e-6f, float uvThreshold = 1e-4f) {
        if (vertices.empty()) return;
        
        std::vector<uint32_t> indexRemap(vertices.size());
        std::vector<RenderVertex> newVertices;
        
        for (size_t i = 0; i < vertices.size(); i++) {
            const auto& v = vertices[i];
            
            // 查找是否有相同顶点
            bool found = false;
            for (size_t j = 0; j < newVertices.size(); j++) {
                const auto& nv = newVertices[j];
                
                // 比较位置
                float dx = v.position[0] - nv.position[0];
                float dy = v.position[1] - nv.position[1];
                float dz = v.position[2] - nv.position[2];
                float posDist = dx*dx + dy*dy + dz*dz;
                
                // 比较 UV
                float du = v.uv[0] - nv.uv[0];
                float dv = v.uv[1] - nv.uv[1];
                float uvDist = du*du + dv*dv;
                
                // 比较法线（点积接近 1）
                float normalDot = v.normal[0]*nv.normal[0] + 
                                  v.normal[1]*nv.normal[1] + 
                                  v.normal[2]*nv.normal[2];
                
                if (posDist < positionThreshold * positionThreshold &&
                    uvDist < uvThreshold * uvThreshold &&
                    normalDot > 0.999f) {
                    indexRemap[i] = static_cast<uint32_t>(j);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                indexRemap[i] = static_cast<uint32_t>(newVertices.size());
                newVertices.push_back(v);
            }
        }
        
        // 更新索引
        for (auto& idx : indices) {
            idx = indexRemap[idx];
        }
        
        vertices = std::move(newVertices);
    }
    
    // =========================================================================
    // 线框生成（用于渲染）
    // =========================================================================
    
    // 生成三角形边的线框索引
    void generateWireframeIndices(std::vector<uint32_t>& outIndices) const {
        outIndices.clear();
        outIndices.reserve(indices.size() * 2);  // 每个三角形 3 条边
        
        // 使用 set 去重边
        std::set<std::pair<uint32_t, uint32_t>> uniqueEdges;
        
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            auto addEdge = [&](uint32_t a, uint32_t b) {
                if (a > b) std::swap(a, b);
                uniqueEdges.insert({a, b});
            };
            
            addEdge(i0, i1);
            addEdge(i1, i2);
            addEdge(i2, i0);
        }
        
        for (const auto& edge : uniqueEdges) {
            outIndices.push_back(edge.first);
            outIndices.push_back(edge.second);
        }
    }
    
    // =========================================================================
    // 验证
    // =========================================================================
    
    bool validate() const {
        // 检查索引范围
        for (uint32_t idx : indices) {
            if (idx >= vertices.size()) {
                return false;
            }
        }
        
        // 检查索引数量是三角形的倍数
        if (indices.size() % 3 != 0) {
            return false;
        }
        
        // 检查子网格
        for (const auto& sub : subMeshes) {
            if (sub.indexOffset + sub.indexCount > indices.size()) {
                return false;
            }
        }
        
        return true;
    }
};

// ============================================================================
// RenderMeshBuilder - 用于高效构建 RenderMesh
// ============================================================================
class RenderMeshBuilder {
public:
    RenderMeshBuilder() = default;
    
    // 开始新的子网格
    void beginSubMesh(uint32_t materialIndex) {
        currentSubMesh_.indexOffset = static_cast<uint32_t>(indices_.size());
        currentSubMesh_.indexCount = 0;
        currentSubMesh_.materialIndex = materialIndex;
    }
    
    // 添加顶点，返回索引
    uint32_t addVertex(const RenderVertex& v) {
        uint32_t idx = static_cast<uint32_t>(vertices_.size());
        vertices_.push_back(v);
        return idx;
    }
    
    // 添加三角形
    void addTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
        indices_.push_back(i0);
        indices_.push_back(i1);
        indices_.push_back(i2);
        currentSubMesh_.indexCount += 3;
    }
    
    // 结束子网格
    void endSubMesh() {
        if (currentSubMesh_.indexCount > 0) {
            subMeshes_.push_back(currentSubMesh_);
        }
    }
    
    // 构建最终的 RenderMesh
    RenderMesh build() {
        RenderMesh mesh;
        mesh.vertices = std::move(vertices_);
        mesh.indices = std::move(indices_);
        mesh.subMeshes = std::move(subMeshes_);
        mesh.calculateBounds();
        return mesh;
    }
    
    // 清空
    void clear() {
        vertices_.clear();
        indices_.clear();
        subMeshes_.clear();
    }
    
private:
    std::vector<RenderVertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<RenderSubMesh> subMeshes_;
    RenderSubMesh currentSubMesh_;
};

}  // namespace luma
