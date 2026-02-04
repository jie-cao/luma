// LUMA MeshConverter - EditMesh ↔ RenderMesh 转换器
// 关键：保存/固化后转换为 RenderMesh 以获得最佳渲染性能

#pragma once

#include "edit_mesh.h"
#include "render_mesh.h"
#include <map>
#include <unordered_map>
#include <iostream>

namespace luma {

// ============================================================================
// MeshConverter - 网格数据转换器
// ============================================================================
class MeshConverter {
public:
    
    // =========================================================================
    // EditMesh → RenderMesh（保存/固化时调用）
    // 三角化所有面，优化顶点布局，生成 GPU 友好的数据
    // =========================================================================
    
    static RenderMesh toRenderMesh(const EditMesh& editMesh, bool optimize = true) {
        RenderMeshBuilder builder;
        
        // 按材质分组面
        std::map<uint32_t, std::vector<size_t>> facesByMaterial;
        for (size_t i = 0; i < editMesh.faces.size(); i++) {
            facesByMaterial[editMesh.faces[i].materialIndex].push_back(i);
        }
        
        // 顶点去重映射（位置+UV+法线 → 索引）
        struct VertexKey {
            float pos[3];
            float uv[2];
            float normal[3];
            
            bool operator==(const VertexKey& other) const {
                const float eps = 1e-6f;
                for (int i = 0; i < 3; i++) {
                    if (std::abs(pos[i] - other.pos[i]) > eps) return false;
                    if (std::abs(normal[i] - other.normal[i]) > eps) return false;
                }
                for (int i = 0; i < 2; i++) {
                    if (std::abs(uv[i] - other.uv[i]) > eps) return false;
                }
                return true;
            }
        };
        
        struct VertexKeyHash {
            size_t operator()(const VertexKey& k) const {
                size_t h = 0;
                auto hashFloat = [](float f) -> size_t {
                    return std::hash<int>{}(static_cast<int>(f * 10000));
                };
                for (int i = 0; i < 3; i++) h ^= hashFloat(k.pos[i]) << (i * 5);
                for (int i = 0; i < 2; i++) h ^= hashFloat(k.uv[i]) << ((i + 3) * 5);
                for (int i = 0; i < 3; i++) h ^= hashFloat(k.normal[i]) << ((i + 5) * 5);
                return h;
            }
        };
        
        std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;
        std::vector<RenderVertex> vertices;
        std::vector<uint32_t> indices;
        
        auto getOrAddVertex = [&](const EditMesh& mesh, const Loop& loop) -> uint32_t {
            const EditVertex& ev = mesh.vertices[loop.vertexIndex];
            
            VertexKey key;
            memcpy(key.pos, ev.position, sizeof(key.pos));
            memcpy(key.uv, loop.uv, sizeof(key.uv));
            memcpy(key.normal, loop.normal, sizeof(key.normal));
            
            auto it = vertexMap.find(key);
            if (it != vertexMap.end()) {
                return it->second;
            }
            
            RenderVertex rv;
            memcpy(rv.position, ev.position, sizeof(rv.position));
            memcpy(rv.normal, loop.normal, sizeof(rv.normal));
            memcpy(rv.tangent, loop.tangent, sizeof(rv.tangent));
            memcpy(rv.uv, loop.uv, sizeof(rv.uv));
            memcpy(rv.color, loop.color, 3 * sizeof(float));
            
            uint32_t idx = static_cast<uint32_t>(vertices.size());
            vertices.push_back(rv);
            vertexMap[key] = idx;
            return idx;
        };
        
        // 为每个材质组创建子网格
        for (const auto& [matIndex, faceIndices] : facesByMaterial) {
            builder.beginSubMesh(matIndex);
            
            for (size_t fi : faceIndices) {
                const EditFace& face = editMesh.faces[fi];
                
                if (face.loops.size() < 3) continue;
                
                // 三角化：扇形三角化（适用于凸多边形）
                // 对于非凸多边形，应该使用耳切法
                uint32_t i0 = getOrAddVertex(editMesh, face.loops[0]);
                
                for (size_t j = 1; j < face.loops.size() - 1; j++) {
                    uint32_t i1 = getOrAddVertex(editMesh, face.loops[j]);
                    uint32_t i2 = getOrAddVertex(editMesh, face.loops[j + 1]);
                    builder.addTriangle(i0, i1, i2);
                }
            }
            
            builder.endSubMesh();
        }
        
        // 手动设置顶点和索引（因为 builder 内部有自己的存储）
        RenderMesh result;
        result.vertices = std::move(vertices);
        
        // 重新遍历生成索引
        for (const auto& [matIndex, faceIndices] : facesByMaterial) {
            RenderSubMesh sub;
            sub.materialIndex = matIndex;
            sub.indexOffset = static_cast<uint32_t>(result.indices.size());
            sub.indexCount = 0;
            
            for (size_t fi : faceIndices) {
                const EditFace& face = editMesh.faces[fi];
                
                if (face.loops.size() < 3) continue;
                
                uint32_t i0 = 0;
                const EditVertex& ev0 = editMesh.vertices[face.loops[0].vertexIndex];
                VertexKey key0;
                memcpy(key0.pos, ev0.position, sizeof(key0.pos));
                memcpy(key0.uv, face.loops[0].uv, sizeof(key0.uv));
                memcpy(key0.normal, face.loops[0].normal, sizeof(key0.normal));
                i0 = vertexMap[key0];
                
                for (size_t j = 1; j < face.loops.size() - 1; j++) {
                    const EditVertex& ev1 = editMesh.vertices[face.loops[j].vertexIndex];
                    const EditVertex& ev2 = editMesh.vertices[face.loops[j + 1].vertexIndex];
                    
                    VertexKey key1, key2;
                    memcpy(key1.pos, ev1.position, sizeof(key1.pos));
                    memcpy(key1.uv, face.loops[j].uv, sizeof(key1.uv));
                    memcpy(key1.normal, face.loops[j].normal, sizeof(key1.normal));
                    
                    memcpy(key2.pos, ev2.position, sizeof(key2.pos));
                    memcpy(key2.uv, face.loops[j + 1].uv, sizeof(key2.uv));
                    memcpy(key2.normal, face.loops[j + 1].normal, sizeof(key2.normal));
                    
                    result.indices.push_back(i0);
                    result.indices.push_back(vertexMap[key1]);
                    result.indices.push_back(vertexMap[key2]);
                    sub.indexCount += 3;
                }
            }
            
            if (sub.indexCount > 0) {
                result.subMeshes.push_back(sub);
            }
        }
        
        // 计算包围盒
        result.calculateBounds();
        
        // 优化
        if (optimize) {
            result.mergeVertices();
            result.optimizeVertexCache();
        }
        
        std::cout << "[MeshConverter] EditMesh -> RenderMesh: "
                  << editMesh.vertices.size() << " verts, "
                  << editMesh.faces.size() << " faces -> "
                  << result.vertices.size() << " verts, "
                  << result.triangleCount() << " tris" << std::endl;
        
        return result;
    }
    
    // =========================================================================
    // RenderMesh → EditMesh（进入编辑模式时调用）
    // 注意：会丢失原始四边面信息，所有面都是三角形
    // =========================================================================
    
    static EditMesh toEditMesh(const RenderMesh& renderMesh) {
        EditMesh result;
        
        // 复制顶点（去重）
        std::unordered_map<uint64_t, uint32_t> positionToVertex;
        
        auto hashPosition = [](const float* pos) -> uint64_t {
            uint64_t h = 0;
            for (int i = 0; i < 3; i++) {
                h ^= static_cast<uint64_t>(pos[i] * 10000 + 0.5f) << (i * 20);
            }
            return h;
        };
        
        std::vector<uint32_t> renderToEditVertex(renderMesh.vertices.size());
        
        for (size_t i = 0; i < renderMesh.vertices.size(); i++) {
            const RenderVertex& rv = renderMesh.vertices[i];
            uint64_t hash = hashPosition(rv.position);
            
            auto it = positionToVertex.find(hash);
            if (it != positionToVertex.end()) {
                renderToEditVertex[i] = it->second;
            } else {
                uint32_t vi = result.addVertex(rv.position[0], rv.position[1], rv.position[2]);
                positionToVertex[hash] = vi;
                renderToEditVertex[i] = vi;
            }
        }
        
        // 添加三角形面
        for (size_t i = 0; i < renderMesh.indices.size(); i += 3) {
            uint32_t ri0 = renderMesh.indices[i];
            uint32_t ri1 = renderMesh.indices[i + 1];
            uint32_t ri2 = renderMesh.indices[i + 2];
            
            uint32_t vi0 = renderToEditVertex[ri0];
            uint32_t vi1 = renderToEditVertex[ri1];
            uint32_t vi2 = renderToEditVertex[ri2];
            
            // 跳过退化三角形
            if (vi0 == vi1 || vi1 == vi2 || vi2 == vi0) continue;
            
            // 确定材质
            uint32_t matIndex = 0;
            for (const auto& sub : renderMesh.subMeshes) {
                if (i >= sub.indexOffset && i < sub.indexOffset + sub.indexCount) {
                    matIndex = sub.materialIndex;
                    break;
                }
            }
            
            // 添加面
            std::vector<uint32_t> verts = { vi0, vi1, vi2 };
            uint32_t faceIdx = result.addFace(verts, matIndex);
            
            // 复制 UV 和法线
            EditFace& face = result.faces[faceIdx];
            const RenderVertex& rv0 = renderMesh.vertices[ri0];
            const RenderVertex& rv1 = renderMesh.vertices[ri1];
            const RenderVertex& rv2 = renderMesh.vertices[ri2];
            
            memcpy(face.loops[0].uv, rv0.uv, sizeof(rv0.uv));
            memcpy(face.loops[0].normal, rv0.normal, sizeof(rv0.normal));
            memcpy(face.loops[0].tangent, rv0.tangent, sizeof(rv0.tangent));
            
            memcpy(face.loops[1].uv, rv1.uv, sizeof(rv1.uv));
            memcpy(face.loops[1].normal, rv1.normal, sizeof(rv1.normal));
            memcpy(face.loops[1].tangent, rv1.tangent, sizeof(rv1.tangent));
            
            memcpy(face.loops[2].uv, rv2.uv, sizeof(rv2.uv));
            memcpy(face.loops[2].normal, rv2.normal, sizeof(rv2.normal));
            memcpy(face.loops[2].tangent, rv2.tangent, sizeof(rv2.tangent));
        }
        
        // 标记三角化产生的边（都是原始边，因为我们没有四边面信息）
        for (auto& edge : result.edges) {
            edge.originalEdge = true;  // 从 RenderMesh 来的都显示为原始边
        }
        
        std::cout << "[MeshConverter] RenderMesh -> EditMesh: "
                  << renderMesh.vertices.size() << " verts, "
                  << renderMesh.triangleCount() << " tris -> "
                  << result.vertices.size() << " verts, "
                  << result.faces.size() << " faces (all triangles)" << std::endl;
        
        return result;
    }
    
    // =========================================================================
    // 尝试将三角形合并为四边形（可选功能）
    // 用于从 RenderMesh 恢复四边面
    // =========================================================================
    
    static void tryMergeTrianglesToQuads(EditMesh& mesh, float angleThreshold = 0.1f) {
        // TODO: 实现三角形到四边形的合并算法
        // 1. 找到共享边的三角形对
        // 2. 检查合并后是否为有效四边形（共面、凸）
        // 3. 合并并更新边信息
        
        std::cout << "[MeshConverter] tryMergeTrianglesToQuads: Not yet implemented" << std::endl;
    }
};

// ============================================================================
// 便捷转换函数
// ============================================================================

// EditMesh → RenderMesh（保存/固化时）
inline RenderMesh convertToRenderMesh(const EditMesh& editMesh, bool optimize = true) {
    return MeshConverter::toRenderMesh(editMesh, optimize);
}

// RenderMesh → EditMesh（进入编辑模式时）
inline EditMesh convertToEditMesh(const RenderMesh& renderMesh) {
    return MeshConverter::toEditMesh(renderMesh);
}

}  // namespace luma
