// LUMA EditMesh - 编辑优化的网格数据结构
// 支持四边面、N-gon，用于 3D 建模编辑
// 保存/固化后转换为 RenderMesh 以获得最佳渲染性能

#pragma once

#include <vector>
#include <set>
#include <map>
#include <deque>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <memory>

namespace luma {

// ============================================================================
// 前向声明
// ============================================================================
struct RenderMesh;

// ============================================================================
// 基础数学工具
// ============================================================================
namespace math {
    inline void normalize3(float* v) {
        float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
        if (len > 1e-6f) { v[0] /= len; v[1] /= len; v[2] /= len; }
    }
    
    inline void cross3(float* out, const float* a, const float* b) {
        out[0] = a[1]*b[2] - a[2]*b[1];
        out[1] = a[2]*b[0] - a[0]*b[2];
        out[2] = a[0]*b[1] - a[1]*b[0];
    }
    
    inline void lerp2(float* out, const float* a, const float* b, float t) {
        out[0] = a[0] + (b[0] - a[0]) * t;
        out[1] = a[1] + (b[1] - a[1]) * t;
    }
    
    inline void lerp3(float* out, const float* a, const float* b, float t) {
        out[0] = a[0] + (b[0] - a[0]) * t;
        out[1] = a[1] + (b[1] - a[1]) * t;
        out[2] = a[2] + (b[2] - a[2]) * t;
    }
}

// ============================================================================
// EditVertex - 编辑顶点（只存储位置）
// ============================================================================
struct EditVertex {
    float position[3] = {0, 0, 0};
    uint32_t id = 0;              // 唯一标识符，用于编辑追踪
    
    EditVertex() = default;
    EditVertex(float x, float y, float z, uint32_t _id = 0) 
        : position{x, y, z}, id(_id) {}
};

// ============================================================================
// Loop - 面的一个角落（存储 UV、法线等 per-face-vertex 数据）
// 这是支持 UV 接缝的关键：同一顶点在不同面上可以有不同的 UV
// ============================================================================
struct Loop {
    uint32_t vertexIndex = 0;     // 指向 EditVertex
    float uv[2] = {0, 0};         // UV 坐标
    float normal[3] = {0, 1, 0};  // 法线
    float tangent[4] = {1, 0, 0, 1}; // 切线 (xyz) + handedness (w)
    float color[4] = {1, 1, 1, 1};   // 顶点色
    
    Loop() = default;
    Loop(uint32_t vIdx) : vertexIndex(vIdx) {}
};

// ============================================================================
// EditFace - 编辑面（支持三角形、四边形、N-gon）
// ============================================================================
struct EditFace {
    std::vector<Loop> loops;      // 面的所有角落
    uint32_t materialIndex = 0;   // 材质索引
    uint32_t id = 0;              // 唯一标识符
    
    // 辅助函数
    int vertexCount() const { return static_cast<int>(loops.size()); }
    bool isTriangle() const { return loops.size() == 3; }
    bool isQuad() const { return loops.size() == 4; }
    bool isNgon() const { return loops.size() > 4; }
    
    // 计算面法线
    void calculateNormal(const std::vector<EditVertex>& vertices) {
        if (loops.size() < 3) return;
        
        const float* p0 = vertices[loops[0].vertexIndex].position;
        const float* p1 = vertices[loops[1].vertexIndex].position;
        const float* p2 = vertices[loops[2].vertexIndex].position;
        
        float e1[3] = { p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2] };
        float e2[3] = { p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2] };
        
        float n[3];
        math::cross3(n, e1, e2);
        math::normalize3(n);
        
        // 设置所有 loop 的法线（平滑着色需要后续处理）
        for (auto& loop : loops) {
            loop.normal[0] = n[0];
            loop.normal[1] = n[1];
            loop.normal[2] = n[2];
        }
    }
};

// ============================================================================
// EditEdge - 编辑边
// ============================================================================
struct EditEdge {
    uint32_t v0 = 0, v1 = 0;      // 两个端点的顶点索引
    bool sharp = false;           // 硬边（影响法线计算）
    bool seam = false;            // UV 接缝
    bool originalEdge = true;     // 原始边（非三角化产生）
    uint32_t id = 0;              // 唯一标识符
    
    EditEdge() = default;
    EditEdge(uint32_t a, uint32_t b, bool orig = true) 
        : v0(std::min(a,b)), v1(std::max(a,b)), originalEdge(orig) {}
    
    bool operator==(const EditEdge& other) const {
        return v0 == other.v0 && v1 == other.v1;
    }
    
    bool hasVertex(uint32_t v) const {
        return v0 == v || v1 == v;
    }
    
    uint32_t otherVertex(uint32_t v) const {
        return v == v0 ? v1 : v0;
    }
};

// ============================================================================
// UV 投影方法
// ============================================================================
enum class UVProjection {
    Planar,         // 平面投影
    Cylindrical,    // 圆柱投影
    Spherical,      // 球形投影
    Box,            // 立方体投影
    FromView,       // 从视角投影
};

// ============================================================================
// UV 展开方法
// ============================================================================
enum class UVUnwrap {
    LSCM,           // 最小二乘共形映射
    ABF,            // 角度基础展平
    SmartProject,   // 智能投影
};

// ============================================================================
// UV 问题类型
// ============================================================================
struct UVProblem {
    enum Type {
        Stretching,     // UV 拉伸
        Overlapping,    // UV 重叠
        Flipped,        // UV 翻转
        Missing,        // 缺失 UV
        OutOfBounds,    // UV 超出 0-1 范围
    };
    
    Type type;
    std::vector<uint32_t> affectedFaces;
    float severity = 0.0f;  // 严重程度 0-1
};

// ============================================================================
// EditMesh 快照（用于 Undo/Redo）
// ============================================================================
struct EditMeshSnapshot {
    std::vector<EditVertex> vertices;
    std::vector<EditFace> faces;
    std::vector<EditEdge> edges;
    std::set<uint32_t> selectedVertices;
    std::set<uint32_t> selectedEdges;
    std::set<uint32_t> selectedFaces;
};

// ============================================================================
// EditMesh - 编辑网格主类
// ============================================================================
class EditMesh {
public:
    // === 数据 ===
    std::vector<EditVertex> vertices;
    std::vector<EditFace> faces;
    std::vector<EditEdge> edges;
    
    // === 选择状态 ===
    std::set<uint32_t> selectedVertices;
    std::set<uint32_t> selectedEdges;
    std::set<uint32_t> selectedFaces;
    
    // === 构造 ===
    EditMesh() = default;
    
    // === ID 生成 ===
    uint32_t nextVertexId() { return nextVertexId_++; }
    uint32_t nextFaceId() { return nextFaceId_++; }
    uint32_t nextEdgeId() { return nextEdgeId_++; }
    
    // === 清空 ===
    void clear() {
        vertices.clear();
        faces.clear();
        edges.clear();
        selectedVertices.clear();
        selectedEdges.clear();
        selectedFaces.clear();
        undoStack_.clear();
        redoStack_.clear();
        nextVertexId_ = 0;
        nextFaceId_ = 0;
        nextEdgeId_ = 0;
        isDirty_ = false;
    }
    
    // =========================================================================
    // 基础操作
    // =========================================================================
    
    // 添加顶点
    uint32_t addVertex(float x, float y, float z) {
        uint32_t id = nextVertexId();
        vertices.emplace_back(x, y, z, id);
        return static_cast<uint32_t>(vertices.size() - 1);
    }
    
    // 添加面（从顶点索引创建）
    uint32_t addFace(const std::vector<uint32_t>& vertexIndices, uint32_t matIndex = 0) {
        EditFace face;
        face.id = nextFaceId();
        face.materialIndex = matIndex;
        
        for (uint32_t vi : vertexIndices) {
            Loop loop(vi);
            face.loops.push_back(loop);
        }
        
        // 计算面法线
        face.calculateNormal(vertices);
        
        faces.push_back(std::move(face));
        
        // 添加边
        for (size_t i = 0; i < vertexIndices.size(); i++) {
            uint32_t v0 = vertexIndices[i];
            uint32_t v1 = vertexIndices[(i + 1) % vertexIndices.size()];
            addEdgeIfNotExists(v0, v1, true);
        }
        
        markDirty();
        return static_cast<uint32_t>(faces.size() - 1);
    }
    
    // 添加边（如果不存在）
    void addEdgeIfNotExists(uint32_t v0, uint32_t v1, bool isOriginal) {
        EditEdge newEdge(v0, v1, isOriginal);
        for (auto& e : edges) {
            if (e.v0 == newEdge.v0 && e.v1 == newEdge.v1) {
                // 如果已存在，更新 originalEdge 标记
                e.originalEdge = e.originalEdge || isOriginal;
                return;
            }
        }
        newEdge.id = nextEdgeId();
        edges.push_back(newEdge);
    }
    
    // =========================================================================
    // 从渲染网格创建
    // =========================================================================
    
    // 从带有原始面拓扑的网格创建（保留四边面）
    template<typename MeshType>
    void fromOriginalFaces(const MeshType& srcMesh) {
        clear();
        
        // 复制顶点
        for (size_t i = 0; i < srcMesh.vertices.size(); i++) {
            const auto& sv = srcMesh.vertices[i];
            uint32_t vi = addVertex(sv.position[0], sv.position[1], sv.position[2]);
            (void)vi;
        }
        
        // 从原始面创建（保留四边面/N-gon）
        for (const auto& origFace : srcMesh.originalFaces) {
            if (origFace.vertexIndices.size() < 3) continue;
            
            std::vector<uint32_t> faceVerts(origFace.vertexIndices.begin(), 
                                            origFace.vertexIndices.end());
            uint32_t fi = addFace(faceVerts);
            
            // 复制 UV 和法线
            auto& face = faces[fi];
            for (size_t li = 0; li < face.loops.size(); li++) {
                uint32_t vi = face.loops[li].vertexIndex;
                if (vi < srcMesh.vertices.size()) {
                    const auto& sv = srcMesh.vertices[vi];
                    face.loops[li].uv[0] = sv.uv[0];
                    face.loops[li].uv[1] = sv.uv[1];
                    face.loops[li].normal[0] = sv.normal[0];
                    face.loops[li].normal[1] = sv.normal[1];
                    face.loops[li].normal[2] = sv.normal[2];
                }
            }
        }
        
        rebuildEdges();
    }
    
    // 从三角化网格创建（只有三角形）
    template<typename MeshType>
    void fromTriangles(const MeshType& srcMesh) {
        clear();
        
        // 复制顶点
        for (size_t i = 0; i < srcMesh.vertices.size(); i++) {
            const auto& sv = srcMesh.vertices[i];
            uint32_t vi = addVertex(sv.position[0], sv.position[1], sv.position[2]);
            (void)vi;
        }
        
        // 创建三角形面
        for (size_t i = 0; i + 2 < srcMesh.indices.size(); i += 3) {
            std::vector<uint32_t> triVerts = {
                srcMesh.indices[i],
                srcMesh.indices[i + 1],
                srcMesh.indices[i + 2]
            };
            uint32_t fi = addFace(triVerts);
            
            // 复制 UV 和法线
            auto& face = faces[fi];
            for (size_t li = 0; li < 3; li++) {
                uint32_t vi = triVerts[li];
                if (vi < srcMesh.vertices.size()) {
                    const auto& sv = srcMesh.vertices[vi];
                    face.loops[li].uv[0] = sv.uv[0];
                    face.loops[li].uv[1] = sv.uv[1];
                    face.loops[li].normal[0] = sv.normal[0];
                    face.loops[li].normal[1] = sv.normal[1];
                    face.loops[li].normal[2] = sv.normal[2];
                }
            }
        }
        
        rebuildEdges();
    }
    
    // =========================================================================
    // 选择操作
    // =========================================================================
    
    void selectAll() {
        for (size_t i = 0; i < vertices.size(); i++) selectedVertices.insert(i);
        for (size_t i = 0; i < edges.size(); i++) selectedEdges.insert(i);
        for (size_t i = 0; i < faces.size(); i++) selectedFaces.insert(i);
    }
    
    void selectNone() {
        selectedVertices.clear();
        selectedEdges.clear();
        selectedFaces.clear();
    }
    
    // Alias for selectNone
    void clearSelection() {
        selectNone();
    }
    
    void selectVertex(uint32_t index, bool add = false) {
        if (!add) selectedVertices.clear();
        selectedVertices.insert(index);
    }
    
    void selectEdge(uint32_t index, bool add = false) {
        if (!add) selectedEdges.clear();
        selectedEdges.insert(index);
    }
    
    void selectFace(uint32_t index, bool add = false) {
        if (!add) selectedFaces.clear();
        selectedFaces.insert(index);
    }
    
    // =========================================================================
    // 变换操作
    // =========================================================================
    
    void translateSelected(float dx, float dy, float dz) {
        pushUndo();
        for (uint32_t vi : selectedVertices) {
            vertices[vi].position[0] += dx;
            vertices[vi].position[1] += dy;
            vertices[vi].position[2] += dz;
        }
        markDirty();
    }
    
    void scaleSelected(float sx, float sy, float sz, const float* pivot) {
        pushUndo();
        for (uint32_t vi : selectedVertices) {
            vertices[vi].position[0] = pivot[0] + (vertices[vi].position[0] - pivot[0]) * sx;
            vertices[vi].position[1] = pivot[1] + (vertices[vi].position[1] - pivot[1]) * sy;
            vertices[vi].position[2] = pivot[2] + (vertices[vi].position[2] - pivot[2]) * sz;
        }
        markDirty();
    }
    
    // =========================================================================
    // 建模操作
    // =========================================================================
    
    // 挤出选中的面（带位移）
    void extrudeSelectedFaces(float dx, float dy, float dz) {
        if (selectedFaces.empty()) return;
        pushUndo();
        extrudeSelectedFacesInternal(dx, dy, dz);
    }
    
    // 挤出选中的面（零位移，用于 gizmo 交互式挤出）
    // 返回挤出后新顶点的索引集合（用于后续移动）
    std::set<uint32_t> extrudeSelectedFacesInPlace() {
        if (selectedFaces.empty()) return {};
        pushUndo();
        return extrudeSelectedFacesInternal(0, 0, 0);
    }
    
    // 计算选中面的平均法线（本地空间）
    void getSelectedFacesNormal(float* outNormal) const {
        outNormal[0] = outNormal[1] = outNormal[2] = 0;
        if (selectedFaces.empty()) return;
        
        for (uint32_t fi : selectedFaces) {
            if (fi >= faces.size()) continue;
            const EditFace& face = faces[fi];
            if (face.loops.size() < 3) continue;
            
            const float* p0 = vertices[face.loops[0].vertexIndex].position;
            const float* p1 = vertices[face.loops[1].vertexIndex].position;
            const float* p2 = vertices[face.loops[2].vertexIndex].position;
            
            float e1[3] = { p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2] };
            float e2[3] = { p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2] };
            
            float n[3];
            math::cross3(n, e1, e2);
            math::normalize3(n);
            
            outNormal[0] += n[0];
            outNormal[1] += n[1];
            outNormal[2] += n[2];
        }
        math::normalize3(outNormal);
    }
    
    // 挤出选中的边（零位移，用于 gizmo 交互式挤出）
    std::set<uint32_t> extrudeSelectedEdgesInPlace() {
        if (selectedEdges.empty()) return {};
        pushUndo();
        
        std::set<uint32_t> newVertIndices;
        std::vector<uint32_t> edgesToExtrude(selectedEdges.begin(), selectedEdges.end());
        
        // Map from old vertex to new vertex
        std::map<uint32_t, uint32_t> vertexMap;
        
        for (uint32_t ei : edgesToExtrude) {
            if (ei >= edges.size()) continue;
            const EditEdge& edge = edges[ei];
            
            // Create new vertices for endpoints (if not already created)
            for (uint32_t vi : {edge.v0, edge.v1}) {
                if (vertexMap.find(vi) == vertexMap.end()) {
                    const EditVertex& oldV = vertices[vi];
                    uint32_t newVi = addVertex(oldV.position[0], oldV.position[1], oldV.position[2]);
                    vertexMap[vi] = newVi;
                    newVertIndices.insert(newVi);
                }
            }
            
            // Create a quad face connecting old edge to new edge
            uint32_t nv0 = vertexMap[edge.v0];
            uint32_t nv1 = vertexMap[edge.v1];
            std::vector<uint32_t> faceVerts = { edge.v0, edge.v1, nv1, nv0 };
            addFace(faceVerts);
        }
        
        // Update selection to new vertices
        selectedVertices.clear();
        selectedVertices = newVertIndices;
        
        // Update selected edges to the new edges
        selectedEdges.clear();
        
        rebuildEdges();
        
        // Find the new edges that connect the new vertices
        for (uint32_t ei = 0; ei < edges.size(); ei++) {
            if (newVertIndices.count(edges[ei].v0) && newVertIndices.count(edges[ei].v1)) {
                selectedEdges.insert(ei);
            }
        }
        
        markDirty();
        return newVertIndices;
    }
    
    // 挤出选中的顶点（零位移）
    std::set<uint32_t> extrudeSelectedVerticesInPlace() {
        if (selectedVertices.empty()) return {};
        pushUndo();
        
        std::set<uint32_t> newVertIndices;
        
        for (uint32_t vi : selectedVertices) {
            if (vi >= vertices.size()) continue;
            const EditVertex& oldV = vertices[vi];
            uint32_t newVi = addVertex(oldV.position[0], oldV.position[1], oldV.position[2]);
            newVertIndices.insert(newVi);
            
            // Create an edge connecting old vertex to new vertex
            addEdgeIfNotExists(vi, newVi, true);
        }
        
        // Update selection to new vertices
        selectedVertices = newVertIndices;
        
        rebuildEdges();
        markDirty();
        return newVertIndices;
    }
    
    // 细分选中的面
    void subdivideSelectedFaces() {
        if (selectedFaces.empty()) return;
        pushUndo();
        
        std::vector<uint32_t> facesToSubdivide(selectedFaces.begin(), selectedFaces.end());
        std::sort(facesToSubdivide.rbegin(), facesToSubdivide.rend()); // 从后往前删除
        
        for (uint32_t fi : facesToSubdivide) {
            EditFace& face = faces[fi];
            
            // 计算中心点
            float cx = 0, cy = 0, cz = 0;
            float cu = 0, cv = 0;
            for (const Loop& loop : face.loops) {
                cx += vertices[loop.vertexIndex].position[0];
                cy += vertices[loop.vertexIndex].position[1];
                cz += vertices[loop.vertexIndex].position[2];
                cu += loop.uv[0];
                cv += loop.uv[1];
            }
            float n = static_cast<float>(face.loops.size());
            cx /= n; cy /= n; cz /= n;
            cu /= n; cv /= n;
            
            // 添加中心顶点
            uint32_t centerVi = addVertex(cx, cy, cz);
            
            // 为每条边添加中点
            std::vector<uint32_t> edgeMidpoints;
            std::vector<float> midUVs;
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                const EditVertex& v0 = vertices[face.loops[i].vertexIndex];
                const EditVertex& v1 = vertices[face.loops[next].vertexIndex];
                
                uint32_t midVi = addVertex(
                    (v0.position[0] + v1.position[0]) * 0.5f,
                    (v0.position[1] + v1.position[1]) * 0.5f,
                    (v0.position[2] + v1.position[2]) * 0.5f
                );
                edgeMidpoints.push_back(midVi);
                midUVs.push_back((face.loops[i].uv[0] + face.loops[next].uv[0]) * 0.5f);
                midUVs.push_back((face.loops[i].uv[1] + face.loops[next].uv[1]) * 0.5f);
            }
            
            // 创建新面（每个角落一个四边形）
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t prev = (i + face.loops.size() - 1) % face.loops.size();
                
                std::vector<uint32_t> quadVerts = {
                    face.loops[i].vertexIndex,
                    edgeMidpoints[i],
                    centerVi,
                    edgeMidpoints[prev]
                };
                
                addFace(quadVerts, face.materialIndex);
                
                // 设置 UV（插值计算）
                EditFace& newFace = faces.back();
                newFace.loops[0].uv[0] = face.loops[i].uv[0];
                newFace.loops[0].uv[1] = face.loops[i].uv[1];
                newFace.loops[1].uv[0] = midUVs[i*2];
                newFace.loops[1].uv[1] = midUVs[i*2+1];
                newFace.loops[2].uv[0] = cu;
                newFace.loops[2].uv[1] = cv;
                newFace.loops[3].uv[0] = midUVs[prev*2];
                newFace.loops[3].uv[1] = midUVs[prev*2+1];
            }
            
            // 删除原面
            faces.erase(faces.begin() + fi);
        }
        
        selectedFaces.clear();
        rebuildEdges();
        markDirty();
    }
    
    // 删除选中的面
    void deleteSelectedFaces() {
        if (selectedFaces.empty()) return;
        pushUndo();
        
        std::vector<uint32_t> toDelete(selectedFaces.begin(), selectedFaces.end());
        std::sort(toDelete.rbegin(), toDelete.rend());
        
        for (uint32_t fi : toDelete) {
            faces.erase(faces.begin() + fi);
        }
        
        selectedFaces.clear();
        rebuildEdges();
        removeUnusedVertices();
        markDirty();
    }
    
    // =========================================================================
    // 裁剪/分割面 — Split a face by connecting two non-adjacent vertices
    // Like Maya's Multi-Cut: creates a new edge within a face
    // =========================================================================
    bool splitFace(uint32_t faceIndex, uint32_t vertA, uint32_t vertB) {
        if (faceIndex >= faces.size()) return false;
        const EditFace& face = faces[faceIndex];
        if (face.loops.size() < 4) return false; // Can't split a triangle meaningfully
        
        // Find positions of vertA and vertB in the face's loops
        int posA = -1, posB = -1;
        for (size_t i = 0; i < face.loops.size(); i++) {
            if (face.loops[i].vertexIndex == vertA) posA = static_cast<int>(i);
            if (face.loops[i].vertexIndex == vertB) posB = static_cast<int>(i);
        }
        if (posA < 0 || posB < 0) return false; // Vertices not on this face
        
        // Ensure posA < posB for simpler logic
        if (posA > posB) std::swap(posA, posB);
        
        // Check they're not adjacent (splitting between adjacent verts creates degenerate face)
        int diff = posB - posA;
        int n = static_cast<int>(face.loops.size());
        if (diff <= 1 || diff >= n - 1) return false;
        
        pushUndo();
        
        // Build two new faces from the loops
        // Face 1: loops[posA .. posB] inclusive
        EditFace f1;
        f1.materialIndex = face.materialIndex;
        f1.id = nextFaceId();
        for (int i = posA; i <= posB; i++) {
            f1.loops.push_back(face.loops[i]);
        }
        
        // Face 2: loops[posB .. end, 0 .. posA] inclusive (wrapping)
        EditFace f2;
        f2.materialIndex = face.materialIndex;
        f2.id = nextFaceId();
        for (int i = posB; i < n; i++) {
            f2.loops.push_back(face.loops[i]);
        }
        for (int i = 0; i <= posA; i++) {
            f2.loops.push_back(face.loops[i]);
        }
        
        f1.calculateNormal(vertices);
        f2.calculateNormal(vertices);
        
        // Replace old face with f1, append f2
        faces[faceIndex] = std::move(f1);
        faces.push_back(std::move(f2));
        
        selectedFaces.clear();
        rebuildEdges();
        markDirty();
        return true;
    }
    
    // Cut a face by placing new vertices on two of its edges
    // loopIdx1/loopIdx2: indices within the face's loop array (the edge starts at that loop index)
    // t1/t2: interpolation factor along each edge (0.0 = start vertex, 1.0 = end vertex)
    bool cutFaceOnEdges(uint32_t faceIndex, int loopIdx1, float t1, int loopIdx2, float t2) {
        if (faceIndex >= faces.size()) return false;
        EditFace& face = faces[faceIndex];
        int n = static_cast<int>(face.loops.size());
        if (n < 3) return false;
        if (loopIdx1 < 0 || loopIdx1 >= n || loopIdx2 < 0 || loopIdx2 >= n) return false;
        if (loopIdx1 == loopIdx2) return false;
        
        // Clamp t values
        t1 = std::max(0.01f, std::min(0.99f, t1));
        t2 = std::max(0.01f, std::min(0.99f, t2));
        
        pushUndo();
        
        // Create new vertex on edge 1 (between loop[loopIdx1] and loop[(loopIdx1+1)%n])
        int next1 = (loopIdx1 + 1) % n;
        const Loop& lA = face.loops[loopIdx1];
        const Loop& lB = face.loops[next1];
        float pos1[3], uv1[2];
        math::lerp3(pos1, vertices[lA.vertexIndex].position, vertices[lB.vertexIndex].position, t1);
        math::lerp2(uv1, lA.uv, lB.uv, t1);
        uint32_t newV1 = addVertex(pos1[0], pos1[1], pos1[2]);
        
        // Create new vertex on edge 2
        int next2 = (loopIdx2 + 1) % n;
        const Loop& lC = face.loops[loopIdx2];
        const Loop& lD = face.loops[next2];
        float pos2[3], uv2[2];
        math::lerp3(pos2, vertices[lC.vertexIndex].position, vertices[lD.vertexIndex].position, t2);
        math::lerp2(uv2, lC.uv, lD.uv, t2);
        uint32_t newV2 = addVertex(pos2[0], pos2[1], pos2[2]);
        
        // Insert the new vertices into the face's loops, then split
        // After insertion, the face has n+2 loops
        // We insert newV1 after loopIdx1 and newV2 after loopIdx2
        // Then split between the two new vertices
        
        // Build new loop array with both insertions
        std::vector<Loop> newLoops;
        int insertedPos1 = -1, insertedPos2 = -1;
        
        // Handle insertion order (insert later index first to avoid shifting)
        int firstInsert = std::min(loopIdx1, loopIdx2);
        int secondInsert = std::max(loopIdx1, loopIdx2);
        uint32_t firstV = (firstInsert == loopIdx1) ? newV1 : newV2;
        float* firstUV = (firstInsert == loopIdx1) ? uv1 : uv2;
        uint32_t secondV = (firstInsert == loopIdx1) ? newV2 : newV1;
        float* secondUV = (firstInsert == loopIdx1) ? uv2 : uv1;
        
        for (int i = 0; i < n; i++) {
            newLoops.push_back(face.loops[i]);
            if (i == firstInsert) {
                Loop nl(firstV);
                nl.uv[0] = firstUV[0]; nl.uv[1] = firstUV[1];
                // Interpolate normal
                int ni = (i + 1) % n;
                math::lerp3(nl.normal, face.loops[i].normal, face.loops[ni].normal,
                           (firstInsert == loopIdx1) ? t1 : t2);
                insertedPos1 = static_cast<int>(newLoops.size());
                newLoops.push_back(nl);
            }
            if (i == secondInsert) {
                Loop nl(secondV);
                nl.uv[0] = secondUV[0]; nl.uv[1] = secondUV[1];
                int ni = (i + 1) % n;
                math::lerp3(nl.normal, face.loops[i].normal, face.loops[ni].normal,
                           (secondInsert == loopIdx2) ? t2 : t1);
                insertedPos2 = static_cast<int>(newLoops.size());
                newLoops.push_back(nl);
            }
        }
        
        // Now split the face at the two inserted positions
        // Map insertedPos to actual new vertex positions
        int splitA = insertedPos1, splitB = insertedPos2;
        if (firstInsert != loopIdx1) std::swap(splitA, splitB);
        
        // Ensure splitA < splitB
        if (splitA > splitB) std::swap(splitA, splitB);
        
        int nn = static_cast<int>(newLoops.size());
        EditFace f1, f2;
        f1.materialIndex = face.materialIndex;
        f1.id = nextFaceId();
        f2.materialIndex = face.materialIndex;
        f2.id = nextFaceId();
        
        for (int i = splitA; i <= splitB; i++) {
            f1.loops.push_back(newLoops[i]);
        }
        for (int i = splitB; i < nn; i++) {
            f2.loops.push_back(newLoops[i]);
        }
        for (int i = 0; i <= splitA; i++) {
            f2.loops.push_back(newLoops[i]);
        }
        
        f1.calculateNormal(vertices);
        f2.calculateNormal(vertices);
        
        faces[faceIndex] = std::move(f1);
        faces.push_back(std::move(f2));
        
        selectedFaces.clear();
        rebuildEdges();
        markDirty();
        return true;
    }
    
    // =========================================================================
    // Preview edge loop path (for interactive display, no mesh modification)
    // Returns pairs of (position_on_perp_edge1, position_on_perp_edge2) as 3D points
    // that form the loop cut line segments across each quad face.
    // =========================================================================
    struct LoopCutSegment {
        float p0[3], p1[3]; // Two endpoints of a cut line segment across one face
        uint32_t faceIdx;   // Face this segment belongs to (for depth culling)
    };
    
    std::vector<LoopCutSegment> previewEdgeLoop(uint32_t edgeIndex, float factor = 0.5f) const {
        std::vector<LoopCutSegment> result;
        if (edgeIndex >= edges.size()) return result;
        factor = std::max(0.01f, std::min(0.99f, factor));
        
        const EditEdge& startEdge = edges[edgeIndex];
        uint32_t ev0 = startEdge.v0, ev1 = startEdge.v1;
        auto startKey = std::make_pair(std::min(ev0,ev1), std::max(ev0,ev1));
        
        // Build edge-to-faces adjacency
        std::map<std::pair<uint32_t,uint32_t>, std::vector<uint32_t>> edgeFaceMap;
        for (uint32_t fi = 0; fi < static_cast<uint32_t>(faces.size()); fi++) {
            const EditFace& face = faces[fi];
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                uint32_t a = face.loops[i].vertexIndex;
                uint32_t b = face.loops[next].vertexIndex;
                auto key = std::make_pair(std::min(a,b), std::max(a,b));
                edgeFaceMap[key].push_back(fi);
            }
        }
        
        // Lambda: walk in one direction from a starting edge, collecting cut segments
        // Returns true if it looped back to the start edge
        std::set<uint32_t> visitedFaces;
        
        auto walkDirection = [&](std::pair<uint32_t,uint32_t> firstEdgeKey, 
                                  std::vector<LoopCutSegment>& segs) -> bool {
            auto currentEdgeKey = firstEdgeKey;
            
            for (int iteration = 0; iteration < 10000; iteration++) {
                auto it = edgeFaceMap.find(currentEdgeKey);
                if (it == edgeFaceMap.end()) break;
                
                uint32_t nextFaceIdx = UINT32_MAX;
                for (uint32_t fi : it->second) {
                    if (visitedFaces.count(fi) == 0) { nextFaceIdx = fi; break; }
                }
                if (nextFaceIdx == UINT32_MAX) break;
                
                visitedFaces.insert(nextFaceIdx);
                const EditFace& face = faces[nextFaceIdx];
                int n = static_cast<int>(face.loops.size());
                
                if (n == 4) {
                    // QUAD: standard loop cut segment
                    int edgePos = -1;
                    for (int i = 0; i < n; i++) {
                        uint32_t a = face.loops[i].vertexIndex;
                        uint32_t b = face.loops[(i+1)%n].vertexIndex;
                        auto key = std::make_pair(std::min(a,b), std::max(a,b));
                        if (key == currentEdgeKey) { edgePos = i; break; }
                    }
                    if (edgePos < 0) break;
                    
                    int opposite = (edgePos + 2) % 4;
                    
                    uint32_t ea = face.loops[edgePos].vertexIndex;
                    uint32_t eb = face.loops[(edgePos+1)%4].vertexIndex;
                    uint32_t oa = face.loops[opposite].vertexIndex;
                    uint32_t ob = face.loops[(opposite+1)%4].vertexIndex;
                    
                    LoopCutSegment seg;
                    seg.faceIdx = nextFaceIdx;
                    math::lerp3(seg.p0, vertices[ea].position, vertices[eb].position, factor);
                    math::lerp3(seg.p1, vertices[oa].position, vertices[ob].position, 1.0f - factor);
                    segs.push_back(seg);
                    
                    // Walk to next face via OPPOSITE edge
                    currentEdgeKey = std::make_pair(std::min(oa,ob), std::max(oa,ob));
                } else {
                    // NON-QUAD (triangle, ngon): skip segment but try to continue walk
                    // Find any other edge that leads to an unvisited face
                    bool foundContinuation = false;
                    for (int i = 0; i < n; i++) {
                        uint32_t a = face.loops[i].vertexIndex;
                        uint32_t b = face.loops[(i+1)%n].vertexIndex;
                        auto testKey = std::make_pair(std::min(a,b), std::max(a,b));
                        if (testKey == currentEdgeKey) continue; // Skip entry edge
                        
                        auto it2 = edgeFaceMap.find(testKey);
                        if (it2 == edgeFaceMap.end()) continue;
                        for (uint32_t fi2 : it2->second) {
                            if (visitedFaces.count(fi2) == 0) {
                                currentEdgeKey = testKey;
                                foundContinuation = true;
                                break;
                            }
                        }
                        if (foundContinuation) break;
                    }
                    if (!foundContinuation) break;
                }
                
                if (currentEdgeKey == startKey) return true;
            }
            return false;
        };
        
        // Walk direction 1: from the start edge
        size_t dir1Count = 0;
        bool looped = walkDirection(startKey, result);
        dir1Count = result.size();
        
        if (!looped) {
            // Walk direction 2: from the start edge in the other direction
            std::vector<LoopCutSegment> reverseSegs;
            walkDirection(startKey, reverseSegs);
            
            if (!reverseSegs.empty()) {
                std::reverse(reverseSegs.begin(), reverseSegs.end());
                for (auto& seg : reverseSegs) {
                    std::swap(seg.p0[0], seg.p1[0]);
                    std::swap(seg.p0[1], seg.p1[1]);
                    std::swap(seg.p0[2], seg.p1[2]);
                }
                result.insert(result.begin(), reverseSegs.begin(), reverseSegs.end());
            }
        }
        
        // Debug: print once when edge changes (caller typically caches)
        static uint32_t s_lastDebugEdge = UINT32_MAX;
        if (edgeIndex != s_lastDebugEdge) {
            s_lastDebugEdge = edgeIndex;
            printf("[LoopCut] edge=%u: %s ring, %zu segments (dir1=%zu)\n",
                   edgeIndex, looped ? "CLOSED" : "OPEN", result.size(), dir1Count);
        }
        
        return result;
    }
    
    // Cached edge-to-face adjacency (topology only, rebuilt when mesh changes)
    struct EdgeFaceInfo {
        std::map<std::pair<uint32_t,uint32_t>, std::vector<uint32_t>> edgeFaceMap;
        std::vector<float> faceLocalNormals; // 3 floats per face (LOCAL space normal, no world transform)
        bool valid = false;
    };
    mutable EdgeFaceInfo m_cachedEdgeFaceInfo;
    
    // Invalidate cache (call after any topology change)
    void invalidateEdgeFaceCache() const { m_cachedEdgeFaceInfo.valid = false; }
    
    // Get or rebuild cached edge-face info (topology + local normals)
    const EdgeFaceInfo& getEdgeFaceInfo() const {
        if (m_cachedEdgeFaceInfo.valid) return m_cachedEdgeFaceInfo;
        
        m_cachedEdgeFaceInfo.edgeFaceMap.clear();
        m_cachedEdgeFaceInfo.faceLocalNormals.resize(faces.size() * 3, 0.0f);
        
        for (uint32_t fi = 0; fi < static_cast<uint32_t>(faces.size()); fi++) {
            const auto& face = faces[fi];
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                uint32_t a = face.loops[i].vertexIndex;
                uint32_t b = face.loops[next].vertexIndex;
                auto key = std::make_pair(std::min(a,b), std::max(a,b));
                m_cachedEdgeFaceInfo.edgeFaceMap[key].push_back(fi);
            }
            if (face.loops.size() >= 3) {
                const float* p0 = vertices[face.loops[0].vertexIndex].position;
                const float* p1 = vertices[face.loops[1].vertexIndex].position;
                const float* p2 = vertices[face.loops[2].vertexIndex].position;
                float e1[3] = { p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2] };
                float e2[3] = { p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2] };
                m_cachedEdgeFaceInfo.faceLocalNormals[fi*3+0] = e1[1]*e2[2] - e1[2]*e2[1];
                m_cachedEdgeFaceInfo.faceLocalNormals[fi*3+1] = e1[2]*e2[0] - e1[0]*e2[2];
                m_cachedEdgeFaceInfo.faceLocalNormals[fi*3+2] = e1[0]*e2[1] - e1[1]*e2[0];
            }
        }
        m_cachedEdgeFaceInfo.valid = true;
        return m_cachedEdgeFaceInfo;
    }
    
    // Check if an edge is front-facing using cached adjacency + world matrix
    bool isEdgeFrontFacing(uint32_t edgeIdx, const float* viewDir, 
                           const float* worldMatrix, const EdgeFaceInfo& info) const {
        if (edgeIdx >= edges.size()) return false;
        const auto& edge = edges[edgeIdx];
        auto edgeKey = std::make_pair(std::min(edge.v0, edge.v1), std::max(edge.v0, edge.v1));
        
        auto it = info.edgeFaceMap.find(edgeKey);
        if (it == info.edgeFaceMap.end()) return false;
        
        for (uint32_t fi : it->second) {
            float nx = info.faceLocalNormals[fi*3+0];
            float ny = info.faceLocalNormals[fi*3+1];
            float nz = info.faceLocalNormals[fi*3+2];
            // Transform local normal to world space
            float wn[3];
            if (worldMatrix) {
                wn[0] = worldMatrix[0]*nx + worldMatrix[4]*ny + worldMatrix[8]*nz;
                wn[1] = worldMatrix[1]*nx + worldMatrix[5]*ny + worldMatrix[9]*nz;
                wn[2] = worldMatrix[2]*nx + worldMatrix[6]*ny + worldMatrix[10]*nz;
            } else {
                wn[0] = nx; wn[1] = ny; wn[2] = nz;
            }
            float dot = wn[0]*viewDir[0] + wn[1]*viewDir[1] + wn[2]*viewDir[2];
            if (dot < 0.0f) return true;
        }
        return false;
    }
    
    // Find the closest edge to a screen position (for hover detection in Loop Cut / Knife tool)
    // viewDir: camera forward direction (for back-face culling). Pass nullptr to disable culling.
    uint32_t findClosestEdgeToScreenPos(
        float screenX, float screenY,
        const std::function<bool(float wx, float wy, float wz, float& sx, float& sy)>& project,
        const float* worldMatrix = nullptr,
        float maxScreenDist = 15.0f,
        const float* viewDir = nullptr) const 
    {
        uint32_t bestEdge = UINT32_MAX;
        float bestDist = maxScreenDist;
        
        // Use cached adjacency info for back-face culling (much faster than rebuilding)
        const EdgeFaceInfo* efInfoPtr = nullptr;
        if (viewDir) efInfoPtr = &getEdgeFaceInfo();
        
        auto transformPos = [worldMatrix](const float* pos, float* out) {
            if (worldMatrix) {
                out[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
                out[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
                out[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
            } else {
                out[0] = pos[0]; out[1] = pos[1]; out[2] = pos[2];
            }
        };
        
        for (uint32_t ei = 0; ei < static_cast<uint32_t>(edges.size()); ei++) {
            const auto& edge = edges[ei];
            if (edge.v0 >= vertices.size() || edge.v1 >= vertices.size()) continue;
            
            // Skip back-facing edges when depth culling is enabled
            if (efInfoPtr && !isEdgeFrontFacing(ei, viewDir, worldMatrix, *efInfoPtr)) continue;
            
            float w0[3], w1[3];
            transformPos(vertices[edge.v0].position, w0);
            transformPos(vertices[edge.v1].position, w1);
            
            float sx0, sy0, sx1, sy1;
            if (!project(w0[0], w0[1], w0[2], sx0, sy0)) continue;
            if (!project(w1[0], w1[1], w1[2], sx1, sy1)) continue;
            
            // Point-to-line-segment distance in screen space
            float dx = sx1 - sx0, dy = sy1 - sy0;
            float len2 = dx*dx + dy*dy;
            if (len2 < 1e-6f) continue;
            
            float t = ((screenX - sx0)*dx + (screenY - sy0)*dy) / len2;
            t = std::max(0.0f, std::min(1.0f, t));
            
            float px = sx0 + t * dx;
            float py = sy0 + t * dy;
            float dist = std::sqrt((screenX - px)*(screenX - px) + (screenY - py)*(screenY - py));
            
            if (dist < bestDist) {
                bestDist = dist;
                bestEdge = ei;
            }
        }
        
        return bestEdge;
    }
    
    // Find the closest edge to a screen point and return the parametric t value
    // viewDir: camera forward direction (for back-face culling). Pass nullptr to disable culling.
    uint32_t findClosestEdgeWithT(
        float screenX, float screenY,
        const std::function<bool(float wx, float wy, float wz, float& sx, float& sy)>& project,
        const float* worldMatrix,
        float& outT,
        float maxScreenDist = 15.0f,
        const float* viewDir = nullptr) const
    {
        uint32_t bestEdge = UINT32_MAX;
        float bestDist = maxScreenDist;
        float bestT = 0.5f;
        
        // Use cached adjacency info for back-face culling
        const EdgeFaceInfo* efInfoPtr = nullptr;
        if (viewDir) efInfoPtr = &getEdgeFaceInfo();
        
        auto transformPos = [worldMatrix](const float* pos, float* out) {
            if (worldMatrix) {
                out[0] = worldMatrix[0]*pos[0] + worldMatrix[4]*pos[1] + worldMatrix[8]*pos[2] + worldMatrix[12];
                out[1] = worldMatrix[1]*pos[0] + worldMatrix[5]*pos[1] + worldMatrix[9]*pos[2] + worldMatrix[13];
                out[2] = worldMatrix[2]*pos[0] + worldMatrix[6]*pos[1] + worldMatrix[10]*pos[2] + worldMatrix[14];
            } else {
                out[0] = pos[0]; out[1] = pos[1]; out[2] = pos[2];
            }
        };
        
        for (uint32_t ei = 0; ei < static_cast<uint32_t>(edges.size()); ei++) {
            const auto& edge = edges[ei];
            if (edge.v0 >= vertices.size() || edge.v1 >= vertices.size()) continue;
            
            // Skip back-facing edges when depth culling is enabled
            if (efInfoPtr && !isEdgeFrontFacing(ei, viewDir, worldMatrix, *efInfoPtr)) continue;
            
            float w0[3], w1[3];
            transformPos(vertices[edge.v0].position, w0);
            transformPos(vertices[edge.v1].position, w1);
            
            float sx0, sy0, sx1, sy1;
            if (!project(w0[0], w0[1], w0[2], sx0, sy0)) continue;
            if (!project(w1[0], w1[1], w1[2], sx1, sy1)) continue;
            
            float dx = sx1 - sx0, dy = sy1 - sy0;
            float len2 = dx*dx + dy*dy;
            if (len2 < 1e-6f) continue;
            
            float t = ((screenX - sx0)*dx + (screenY - sy0)*dy) / len2;
            t = std::max(0.0f, std::min(1.0f, t));
            
            float px = sx0 + t * dx;
            float py = sy0 + t * dy;
            float dist = std::sqrt((screenX - px)*(screenX - px) + (screenY - py)*(screenY - py));
            
            if (dist < bestDist) {
                bestDist = dist;
                bestEdge = ei;
                bestT = t;
            }
        }
        
        outT = bestT;
        return bestEdge;
    }
    
    // =========================================================================
    // 插入环形边 — Insert edge loop perpendicular to the given edge
    // Like Blender Ctrl+R / Maya Insert Edge Loop
    // factor: 0.0-1.0 controls position along perpendicular edges (0.5 = midpoint)
    // Returns new vertex indices for interactive sliding
    // =========================================================================
    std::vector<uint32_t> insertEdgeLoop(uint32_t edgeIndex, float factor = 0.5f) {
        if (edgeIndex >= edges.size()) return {};
        factor = std::max(0.01f, std::min(0.99f, factor));
        
        const EditEdge& startEdge = edges[edgeIndex];
        uint32_t ev0 = startEdge.v0, ev1 = startEdge.v1;
        
        // Build edge-to-faces adjacency map
        // Key: (min_v, max_v), Value: list of face indices
        std::map<std::pair<uint32_t,uint32_t>, std::vector<uint32_t>> edgeFaceMap;
        for (uint32_t fi = 0; fi < static_cast<uint32_t>(faces.size()); fi++) {
            const EditFace& face = faces[fi];
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                uint32_t a = face.loops[i].vertexIndex;
                uint32_t b = face.loops[next].vertexIndex;
                auto key = std::make_pair(std::min(a,b), std::max(a,b));
                edgeFaceMap[key].push_back(fi);
            }
        }
        
        // Walk the edge loop: collect faces to cut and which edges to split
        struct LoopFaceInfo {
            uint32_t faceIdx;
            int edgeLoopIdx;    // Position of the "parallel" edge in the face's loops
            int perpLoopIdx1;   // Position of perpendicular edge 1 (shared vertex with parallel edge start)
            int perpLoopIdx2;   // Position of perpendicular edge 2 (shared vertex with parallel edge end)
            int oppositeLoopIdx;// Position of the opposite edge
        };
        
        std::vector<LoopFaceInfo> loopFaces;
        std::set<uint32_t> visitedFaces;
        
        // Start edge key
        auto startKey = std::make_pair(std::min(ev0,ev1), std::max(ev0,ev1));
        
        // Lambda: walk in one direction collecting faces
        auto walkDir = [&](std::pair<uint32_t,uint32_t> firstEdgeKey, 
                           std::vector<LoopFaceInfo>& outFaces) -> bool {
            auto currentEdgeKey = firstEdgeKey;
            for (int iteration = 0; iteration < 10000; iteration++) {
                auto it = edgeFaceMap.find(currentEdgeKey);
                if (it == edgeFaceMap.end()) break;
                
                uint32_t nextFaceIdx = UINT32_MAX;
                for (uint32_t fi : it->second) {
                    if (visitedFaces.count(fi) == 0) { nextFaceIdx = fi; break; }
                }
                if (nextFaceIdx == UINT32_MAX) break;
                
                visitedFaces.insert(nextFaceIdx);
                const EditFace& face = faces[nextFaceIdx];
                int n = static_cast<int>(face.loops.size());
                
                if (n == 4) {
                    int edgePos = -1;
                    for (int i = 0; i < n; i++) {
                        uint32_t a = face.loops[i].vertexIndex;
                        uint32_t b = face.loops[(i+1)%n].vertexIndex;
                        auto key = std::make_pair(std::min(a,b), std::max(a,b));
                        if (key == currentEdgeKey) { edgePos = i; break; }
                    }
                    if (edgePos < 0) break;
                    
                    int perp1 = (edgePos + 1) % 4;
                    int perp2 = (edgePos + 3) % 4;
                    int opposite = (edgePos + 2) % 4;
                    outFaces.push_back({nextFaceIdx, edgePos, perp1, perp2, opposite});
                    
                    // Walk via OPPOSITE edge
                    uint32_t ov0 = face.loops[opposite].vertexIndex;
                    uint32_t ov1 = face.loops[(opposite+1)%4].vertexIndex;
                    currentEdgeKey = std::make_pair(std::min(ov0,ov1), std::max(ov0,ov1));
                } else {
                    // Non-quad: skip but try to continue walk
                    bool foundContinuation = false;
                    for (int i = 0; i < n; i++) {
                        uint32_t a = face.loops[i].vertexIndex;
                        uint32_t b = face.loops[(i+1)%n].vertexIndex;
                        auto testKey = std::make_pair(std::min(a,b), std::max(a,b));
                        if (testKey == currentEdgeKey) continue;
                        auto it2 = edgeFaceMap.find(testKey);
                        if (it2 == edgeFaceMap.end()) continue;
                        for (uint32_t fi2 : it2->second) {
                            if (visitedFaces.count(fi2) == 0) {
                                currentEdgeKey = testKey;
                                foundContinuation = true;
                                break;
                            }
                        }
                        if (foundContinuation) break;
                    }
                    if (!foundContinuation) break;
                }
                
                if (currentEdgeKey == startKey) return true; // Looped back
            }
            return false;
        };
        
        // Walk direction 1
        bool looped = walkDir(startKey, loopFaces);
        
        if (!looped) {
            // Walk direction 2 (visitedFaces already blocks the first direction's faces)
            std::vector<LoopFaceInfo> reverseFaces;
            walkDir(startKey, reverseFaces);
            if (!reverseFaces.empty()) {
                std::reverse(reverseFaces.begin(), reverseFaces.end());
                loopFaces.insert(loopFaces.begin(), reverseFaces.begin(), reverseFaces.end());
            }
        }
        
        if (loopFaces.empty()) return {};
        
        pushUndo();
        
        // Create new vertices on entry/opposite edges (parallel to hovered edge)
        // Key: edge (min_v, max_v), Value: new vertex index
        std::map<std::pair<uint32_t,uint32_t>, uint32_t> edgeNewVerts;
        std::vector<uint32_t> newVertIndices;
        
        auto getOrCreateEdgeVertex = [&](uint32_t va, uint32_t vb, float t) -> uint32_t {
            auto key = std::make_pair(std::min(va,vb), std::max(va,vb));
            auto it = edgeNewVerts.find(key);
            if (it != edgeNewVerts.end()) return it->second;
            
            float pos[3];
            math::lerp3(pos, vertices[va].position, vertices[vb].position, t);
            uint32_t newVi = addVertex(pos[0], pos[1], pos[2]);
            edgeNewVerts[key] = newVi;
            newVertIndices.push_back(newVi);
            return newVi;
        };
        
        // Collect split info: new vertices on entry and opposite edges
        struct FaceSplitInfo {
            uint32_t faceIdx;
            uint32_t newV1, newV2;  // New vertices on entry and opposite edges
        };
        std::vector<FaceSplitInfo> splits;
        
        for (auto& info : loopFaces) {
            const EditFace& face = faces[info.faceIdx];
            int n = static_cast<int>(face.loops.size());
            
            // Entry edge: the edge we entered this face through
            uint32_t ea = face.loops[info.edgeLoopIdx].vertexIndex;
            uint32_t eb = face.loops[(info.edgeLoopIdx+1)%n].vertexIndex;
            
            // Opposite edge: across the face from entry
            uint32_t oa = face.loops[info.oppositeLoopIdx].vertexIndex;
            uint32_t ob = face.loops[(info.oppositeLoopIdx+1)%n].vertexIndex;
            
            // New vertex on entry edge at factor t
            uint32_t nv1 = getOrCreateEdgeVertex(ea, eb, factor);
            // New vertex on opposite edge at (1-factor) due to reversed direction
            uint32_t nv2 = getOrCreateEdgeVertex(oa, ob, 1.0f - factor);
            
            splits.push_back({info.faceIdx, nv1, nv2});
        }
        
        // Now split each face (process in reverse index order to avoid invalidation)
        std::sort(splits.begin(), splits.end(), [](const FaceSplitInfo& a, const FaceSplitInfo& b) {
            return a.faceIdx > b.faceIdx;
        });
        
        for (auto& split : splits) {
            EditFace& face = faces[split.faceIdx];
            int n = static_cast<int>(face.loops.size());
            
            // Insert the two new vertices into the face's loops
            // Find which edges they belong to and insert after the first vertex of each edge
            std::vector<Loop> expandedLoops;
            
            for (int i = 0; i < n; i++) {
                expandedLoops.push_back(face.loops[i]);
                
                int next = (i + 1) % n;
                uint32_t va = face.loops[i].vertexIndex;
                uint32_t vb = face.loops[next].vertexIndex;
                auto key = std::make_pair(std::min(va,vb), std::max(va,vb));
                
                auto it = edgeNewVerts.find(key);
                if (it != edgeNewVerts.end()) {
                    // Insert new vertex after this position
                    Loop nl(it->second);
                    // Interpolate UV and normal
                    float t_interp = factor;
                    // Check direction
                    if (va > vb) t_interp = 1.0f - factor;
                    math::lerp2(nl.uv, face.loops[i].uv, face.loops[next].uv, t_interp);
                    math::lerp3(nl.normal, face.loops[i].normal, face.loops[next].normal, t_interp);
                    expandedLoops.push_back(nl);
                }
            }
            
            // Now find the positions of newV1 and newV2 in expandedLoops
            int posV1 = -1, posV2 = -1;
            for (int i = 0; i < static_cast<int>(expandedLoops.size()); i++) {
                if (expandedLoops[i].vertexIndex == split.newV1) posV1 = i;
                if (expandedLoops[i].vertexIndex == split.newV2) posV2 = i;
            }
            
            if (posV1 < 0 || posV2 < 0) continue;
            if (posV1 > posV2) std::swap(posV1, posV2);
            
            int nn = static_cast<int>(expandedLoops.size());
            
            // Split: face1 = [posV1 .. posV2], face2 = [posV2 .. posV1] wrapping
            EditFace f1, f2;
            f1.materialIndex = face.materialIndex;
            f1.id = nextFaceId();
            f2.materialIndex = face.materialIndex;
            f2.id = nextFaceId();
            
            for (int i = posV1; i <= posV2; i++) {
                f1.loops.push_back(expandedLoops[i]);
            }
            for (int i = posV2; i < nn; i++) {
                f2.loops.push_back(expandedLoops[i]);
            }
            for (int i = 0; i <= posV1; i++) {
                f2.loops.push_back(expandedLoops[i]);
            }
            
            f1.calculateNormal(vertices);
            f2.calculateNormal(vertices);
            
            // Replace original face with f1
            faces[split.faceIdx] = std::move(f1);
            // Append f2
            faces.push_back(std::move(f2));
        }
        
        selectedFaces.clear();
        selectedEdges.clear();
        selectedVertices.clear();
        
        // Select the new edge loop vertices
        for (uint32_t vi : newVertIndices) {
            selectedVertices.insert(vi);
        }
        
        rebuildEdges();
        markDirty();
        return newVertIndices;
    }
    
    // =========================================================================
    // 合并面 — Merge selected faces into one polygon
    // Like Blender's Merge Faces (F key on selected faces)
    // Selected faces must be adjacent (share edges)
    // =========================================================================
    bool mergeSelectedFaces() {
        if (selectedFaces.size() < 2) return false;
        
        std::vector<uint32_t> faceIndices(selectedFaces.begin(), selectedFaces.end());
        
        // Build a set of all edges and classify them as boundary or internal
        // Internal edges: shared by two or more selected faces
        // Boundary edges: belong to only one selected face
        struct EdgeInfo {
            uint32_t v0, v1;
            int count = 0; // How many selected faces share this edge
        };
        
        std::map<std::pair<uint32_t,uint32_t>, int> edgeCount;
        
        for (uint32_t fi : faceIndices) {
            if (fi >= faces.size()) return false;
            const EditFace& face = faces[fi];
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                uint32_t a = face.loops[i].vertexIndex;
                uint32_t b = face.loops[next].vertexIndex;
                auto key = std::make_pair(std::min(a,b), std::max(a,b));
                edgeCount[key]++;
            }
        }
        
        // Collect boundary edges (shared by exactly one selected face)
        std::vector<std::pair<uint32_t,uint32_t>> boundaryEdges;
        for (auto& [key, count] : edgeCount) {
            if (count == 1) {
                boundaryEdges.push_back(key);
            }
        }
        
        if (boundaryEdges.empty()) return false;
        
        // Build the boundary loop (ordered sequence of vertices)
        // Start with any boundary edge and follow the chain
        std::map<uint32_t, std::vector<uint32_t>> adjacency;
        for (auto& [v0, v1] : boundaryEdges) {
            adjacency[v0].push_back(v1);
            adjacency[v1].push_back(v0);
        }
        
        // Walk the boundary
        std::vector<uint32_t> boundaryLoop;
        std::set<uint32_t> visited;
        uint32_t current = boundaryEdges[0].first;
        
        for (size_t iter = 0; iter < boundaryEdges.size() + 1; iter++) {
            if (visited.count(current)) break;
            visited.insert(current);
            boundaryLoop.push_back(current);
            
            auto it = adjacency.find(current);
            if (it == adjacency.end()) break;
            
            bool foundNext = false;
            for (uint32_t next : it->second) {
                if (!visited.count(next)) {
                    current = next;
                    foundNext = true;
                    break;
                }
            }
            if (!foundNext) break;
        }
        
        if (boundaryLoop.size() < 3) return false;
        
        pushUndo();
        
        // Get material from first selected face
        uint32_t matIdx = faces[faceIndices[0]].materialIndex;
        
        // Collect UVs for the boundary vertices from the original faces
        // Use UV from the first face that contains each boundary vertex
        std::map<uint32_t, std::pair<float,float>> vertexUVs;
        for (uint32_t fi : faceIndices) {
            const EditFace& face = faces[fi];
            for (const Loop& loop : face.loops) {
                if (vertexUVs.find(loop.vertexIndex) == vertexUVs.end()) {
                    vertexUVs[loop.vertexIndex] = {loop.uv[0], loop.uv[1]};
                }
            }
        }
        
        // Delete selected faces (from back to front to preserve indices)
        std::sort(faceIndices.rbegin(), faceIndices.rend());
        for (uint32_t fi : faceIndices) {
            faces.erase(faces.begin() + fi);
        }
        
        // Add the merged face
        uint32_t newFi = addFace(boundaryLoop, matIdx);
        
        // Apply UVs to the new face
        EditFace& newFace = faces[newFi];
        for (auto& loop : newFace.loops) {
            auto it = vertexUVs.find(loop.vertexIndex);
            if (it != vertexUVs.end()) {
                loop.uv[0] = it->second.first;
                loop.uv[1] = it->second.second;
            }
        }
        
        selectedFaces.clear();
        selectedFaces.insert(newFi);
        rebuildEdges();
        removeUnusedVertices();
        markDirty();
        return true;
    }
    
    // 按距离合并顶点
    void mergeByDistance(float threshold = 0.001f) {
        if (vertices.empty()) return;
        pushUndo();
        
        // Build mapping from old to new vertex indices
        std::vector<int> vertexMap(vertices.size());
        std::vector<EditVertex> newVertices;
        
        for (size_t i = 0; i < vertices.size(); i++) {
            int merged = -1;
            for (size_t j = 0; j < newVertices.size(); j++) {
                float dx = vertices[i].position[0] - newVertices[j].position[0];
                float dy = vertices[i].position[1] - newVertices[j].position[1];
                float dz = vertices[i].position[2] - newVertices[j].position[2];
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (dist < threshold) {
                    merged = static_cast<int>(j);
                    break;
                }
            }
            
            if (merged >= 0) {
                vertexMap[i] = merged;
            } else {
                vertexMap[i] = static_cast<int>(newVertices.size());
                newVertices.push_back(vertices[i]);
            }
        }
        
        // Update face vertex indices
        for (auto& face : faces) {
            for (auto& loop : face.loops) {
                loop.vertexIndex = static_cast<uint32_t>(vertexMap[loop.vertexIndex]);
            }
        }
        
        // Update edge vertex indices
        for (auto& edge : edges) {
            edge.v0 = static_cast<uint32_t>(vertexMap[edge.v0]);
            edge.v1 = static_cast<uint32_t>(vertexMap[edge.v1]);
        }
        
        vertices = std::move(newVertices);
        
        // Update selection
        std::set<uint32_t> newSelectedVertices;
        for (uint32_t vi : selectedVertices) {
            if (vi < vertexMap.size()) {
                newSelectedVertices.insert(static_cast<uint32_t>(vertexMap[vi]));
            }
        }
        selectedVertices = std::move(newSelectedVertices);
        
        rebuildEdges();
        markDirty();
    }
    
    // 重新计算法线
    void recalculateNormals() {
        // Recalculate face normals
        for (auto& face : faces) {
            face.calculateNormal(vertices);
        }
        
        // For smooth shading, average normals at shared vertices
        std::vector<float> vertexNormals(vertices.size() * 3, 0.0f);
        std::vector<int> vertexFaceCount(vertices.size(), 0);
        
        // Accumulate normals from faces
        for (const auto& face : faces) {
            if (face.loops.empty()) continue;
            const float* faceNormal = face.loops[0].normal;  // All loops have same normal after calculateNormal
            
            for (const auto& loop : face.loops) {
                uint32_t vi = loop.vertexIndex;
                vertexNormals[vi * 3 + 0] += faceNormal[0];
                vertexNormals[vi * 3 + 1] += faceNormal[1];
                vertexNormals[vi * 3 + 2] += faceNormal[2];
                vertexFaceCount[vi]++;
            }
        }
        
        // Normalize accumulated normals
        for (size_t i = 0; i < vertices.size(); i++) {
            if (vertexFaceCount[i] > 0) {
                float* n = &vertexNormals[i * 3];
                math::normalize3(n);
            }
        }
        
        // Apply averaged normals to loops (smooth shading)
        for (auto& face : faces) {
            for (auto& loop : face.loops) {
                uint32_t vi = loop.vertexIndex;
                loop.normal[0] = vertexNormals[vi * 3 + 0];
                loop.normal[1] = vertexNormals[vi * 3 + 1];
                loop.normal[2] = vertexNormals[vi * 3 + 2];
            }
        }
        
        markDirty();
    }
    
    // =========================================================================
    // UV 操作
    // =========================================================================
    
    // 平面投影 UV
    void projectUVPlanar(const float* normal, const float* up = nullptr) {
        if (selectedFaces.empty()) return;
        pushUndo();
        
        // 默认 up 方向
        float defaultUp[3] = {0, 1, 0};
        if (!up) up = defaultUp;
        
        // 计算投影坐标系
        float right[3];
        math::cross3(right, up, normal);
        math::normalize3(right);
        
        float actualUp[3];
        math::cross3(actualUp, normal, right);
        math::normalize3(actualUp);
        
        for (uint32_t fi : selectedFaces) {
            EditFace& face = faces[fi];
            for (Loop& loop : face.loops) {
                const float* pos = vertices[loop.vertexIndex].position;
                loop.uv[0] = pos[0] * right[0] + pos[1] * right[1] + pos[2] * right[2];
                loop.uv[1] = pos[0] * actualUp[0] + pos[1] * actualUp[1] + pos[2] * actualUp[2];
            }
        }
        
        normalizeUVs();
        markDirty();
    }
    
    // 立方体投影 UV
    void projectUVBox() {
        if (selectedFaces.empty()) return;
        pushUndo();
        
        for (uint32_t fi : selectedFaces) {
            EditFace& face = faces[fi];
            
            // 计算面法线的主轴
            float nx = 0, ny = 0, nz = 0;
            for (const Loop& loop : face.loops) {
                nx += std::abs(loop.normal[0]);
                ny += std::abs(loop.normal[1]);
                nz += std::abs(loop.normal[2]);
            }
            
            // 选择主轴进行投影
            for (Loop& loop : face.loops) {
                const float* pos = vertices[loop.vertexIndex].position;
                if (nx >= ny && nx >= nz) {
                    // 投影到 YZ 平面
                    loop.uv[0] = pos[1];
                    loop.uv[1] = pos[2];
                } else if (ny >= nz) {
                    // 投影到 XZ 平面
                    loop.uv[0] = pos[0];
                    loop.uv[1] = pos[2];
                } else {
                    // 投影到 XY 平面
                    loop.uv[0] = pos[0];
                    loop.uv[1] = pos[1];
                }
            }
        }
        
        normalizeUVs();
        markDirty();
    }
    
    // 规范化 UV 到 0-1 范围
    void normalizeUVs() {
        float minU = 1e10f, maxU = -1e10f;
        float minV = 1e10f, maxV = -1e10f;
        
        for (const EditFace& face : faces) {
            for (const Loop& loop : face.loops) {
                minU = std::min(minU, loop.uv[0]);
                maxU = std::max(maxU, loop.uv[0]);
                minV = std::min(minV, loop.uv[1]);
                maxV = std::max(maxV, loop.uv[1]);
            }
        }
        
        float rangeU = maxU - minU;
        float rangeV = maxV - minV;
        if (rangeU < 1e-6f) rangeU = 1.0f;
        if (rangeV < 1e-6f) rangeV = 1.0f;
        
        for (EditFace& face : faces) {
            for (Loop& loop : face.loops) {
                loop.uv[0] = (loop.uv[0] - minU) / rangeU;
                loop.uv[1] = (loop.uv[1] - minV) / rangeV;
            }
        }
    }
    
    // 分析 UV 问题
    std::vector<UVProblem> analyzeUV() const {
        std::vector<UVProblem> problems;
        
        // 检测拉伸
        for (size_t i = 0; i < faces.size(); i++) {
            float stretch = calculateUVStretch(faces[i]);
            if (stretch > 0.5f) {  // 阈值
                UVProblem prob;
                prob.type = UVProblem::Stretching;
                prob.affectedFaces.push_back(i);
                prob.severity = stretch;
                problems.push_back(prob);
            }
        }
        
        // TODO: 检测重叠、翻转等
        
        return problems;
    }
    
    // 计算面的 UV 拉伸度
    float calculateUVStretch(const EditFace& face) const {
        if (face.loops.size() < 3) return 0.0f;
        
        // 计算 3D 空间面积
        const float* p0 = vertices[face.loops[0].vertexIndex].position;
        const float* p1 = vertices[face.loops[1].vertexIndex].position;
        const float* p2 = vertices[face.loops[2].vertexIndex].position;
        
        float e1[3] = { p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2] };
        float e2[3] = { p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2] };
        float cross[3];
        math::cross3(cross, e1, e2);
        float area3D = std::sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]) * 0.5f;
        
        // 计算 UV 空间面积
        float u1 = face.loops[1].uv[0] - face.loops[0].uv[0];
        float v1 = face.loops[1].uv[1] - face.loops[0].uv[1];
        float u2 = face.loops[2].uv[0] - face.loops[0].uv[0];
        float v2 = face.loops[2].uv[1] - face.loops[0].uv[1];
        float areaUV = std::abs(u1 * v2 - u2 * v1) * 0.5f;
        
        if (area3D < 1e-6f || areaUV < 1e-6f) return 0.0f;
        
        // 拉伸度 = |log(面积比)|
        float ratio = areaUV / area3D;
        return std::abs(std::log(ratio + 1e-6f));
    }
    
    // =========================================================================
    // Undo/Redo
    // =========================================================================
    
    void pushUndo() {
        EditMeshSnapshot snapshot;
        snapshot.vertices = vertices;
        snapshot.faces = faces;
        snapshot.edges = edges;
        snapshot.selectedVertices = selectedVertices;
        snapshot.selectedEdges = selectedEdges;
        snapshot.selectedFaces = selectedFaces;
        
        undoStack_.push_back(std::move(snapshot));
        if (undoStack_.size() > maxUndoLevels_) {
            undoStack_.pop_front();
        }
        
        redoStack_.clear();
        invalidateEdgeFaceCache();
        invalidateEdgeFaceCache();
    }
    
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    
    // Clear undo/redo history (called after Commit/Freeze to set new baseline)
    void clearUndoHistory() {
        undoStack_.clear();
        redoStack_.clear();
    }
    
    // Create a full snapshot of current state (for external baseline storage)
    EditMeshSnapshot createSnapshot() const {
        EditMeshSnapshot snapshot;
        snapshot.vertices = vertices;
        snapshot.faces = faces;
        snapshot.edges = edges;
        snapshot.selectedVertices = selectedVertices;
        snapshot.selectedEdges = selectedEdges;
        snapshot.selectedFaces = selectedFaces;
        return snapshot;
    }
    
    // Restore state from an external snapshot (for Cancel to baseline)
    void restoreFromSnapshot(const EditMeshSnapshot& snapshot) {
        vertices = snapshot.vertices;
        faces = snapshot.faces;
        edges = snapshot.edges;
        selectedVertices = snapshot.selectedVertices;
        selectedEdges = snapshot.selectedEdges;
        selectedFaces = snapshot.selectedFaces;
        clearUndoHistory();
        markDirty();
    }
    
    int getUndoCount() const { return static_cast<int>(undoStack_.size()); }
    int getRedoCount() const { return static_cast<int>(redoStack_.size()); }
    
    bool undo() {
        if (undoStack_.empty()) return false;
        
        // 保存当前状态到 redo
        EditMeshSnapshot current;
        current.vertices = vertices;
        current.faces = faces;
        current.edges = edges;
        current.selectedVertices = selectedVertices;
        current.selectedEdges = selectedEdges;
        current.selectedFaces = selectedFaces;
        redoStack_.push_back(std::move(current));
        
        // 恢复上一个状态
        const auto& snapshot = undoStack_.back();
        vertices = snapshot.vertices;
        faces = snapshot.faces;
        edges = snapshot.edges;
        selectedVertices = snapshot.selectedVertices;
        selectedEdges = snapshot.selectedEdges;
        selectedFaces = snapshot.selectedFaces;
        
        undoStack_.pop_back();
        markDirty();
        invalidateEdgeFaceCache();
        return true;
    }
    
    bool redo() {
        if (redoStack_.empty()) return false;
        
        // 保存当前状态到 undo
        EditMeshSnapshot current;
        current.vertices = vertices;
        current.faces = faces;
        current.edges = edges;
        current.selectedVertices = selectedVertices;
        current.selectedEdges = selectedEdges;
        current.selectedFaces = selectedFaces;
        undoStack_.push_back(std::move(current));
        
        // 恢复下一个状态
        const auto& snapshot = redoStack_.back();
        vertices = snapshot.vertices;
        faces = snapshot.faces;
        edges = snapshot.edges;
        selectedVertices = snapshot.selectedVertices;
        selectedEdges = snapshot.selectedEdges;
        selectedFaces = snapshot.selectedFaces;
        
        redoStack_.pop_back();
        markDirty();
        invalidateEdgeFaceCache();
        return true;
    }
    
    // =========================================================================
    // 状态查询
    // =========================================================================
    
    bool isDirty() const { return isDirty_; }
    void clearDirty() { isDirty_ = false; }
    
    bool hasModifications() const { return !undoStack_.empty(); }
    
    bool validate() const {
        // 检查所有 loop 的顶点索引是否有效
        for (const auto& face : faces) {
            for (const auto& loop : face.loops) {
                if (loop.vertexIndex >= vertices.size()) {
                    return false;
                }
            }
        }
        return true;
    }
    
    // =========================================================================
    // 线框生成（用于显示四边面）
    // =========================================================================
    
    // 生成原始边的线框（四边面显示）
    void generateOriginalEdgeWireframe(std::vector<float>& outVertices,
                                       std::vector<uint32_t>& outIndices) const {
        outVertices.clear();
        outIndices.clear();
        
        // 只输出 originalEdge = true 的边
        for (const auto& edge : edges) {
            if (!edge.originalEdge) continue;
            
            uint32_t i0 = static_cast<uint32_t>(outVertices.size() / 3);
            
            const float* p0 = vertices[edge.v0].position;
            const float* p1 = vertices[edge.v1].position;
            
            outVertices.push_back(p0[0]);
            outVertices.push_back(p0[1]);
            outVertices.push_back(p0[2]);
            
            outVertices.push_back(p1[0]);
            outVertices.push_back(p1[1]);
            outVertices.push_back(p1[2]);
            
            outIndices.push_back(i0);
            outIndices.push_back(i0 + 1);
        }
    }
    
    // 生成所有边的线框（包括三角化边）
    void generateAllEdgeWireframe(std::vector<float>& outVertices,
                                  std::vector<uint32_t>& outIndices) const {
        outVertices.clear();
        outIndices.clear();
        
        for (const auto& edge : edges) {
            uint32_t i0 = static_cast<uint32_t>(outVertices.size() / 3);
            
            const float* p0 = vertices[edge.v0].position;
            const float* p1 = vertices[edge.v1].position;
            
            outVertices.push_back(p0[0]);
            outVertices.push_back(p0[1]);
            outVertices.push_back(p0[2]);
            
            outVertices.push_back(p1[0]);
            outVertices.push_back(p1[1]);
            outVertices.push_back(p1[2]);
            
            outIndices.push_back(i0);
            outIndices.push_back(i0 + 1);
        }
    }
    
    // =========================================================================
    // 统计信息
    // =========================================================================
    
    int triangleCount() const {
        int count = 0;
        for (const auto& face : faces) {
            if (face.loops.size() >= 3) {
                count += static_cast<int>(face.loops.size()) - 2;
            }
        }
        return count;
    }
    
    int quadCount() const {
        int count = 0;
        for (const auto& face : faces) {
            if (face.isQuad()) count++;
        }
        return count;
    }
    
    int ngonCount() const {
        int count = 0;
        for (const auto& face : faces) {
            if (face.isNgon()) count++;
        }
        return count;
    }

private:
    // ID 计数器
    uint32_t nextVertexId_ = 0;
    uint32_t nextFaceId_ = 0;
    uint32_t nextEdgeId_ = 0;
    
    // Undo/Redo 栈
    std::deque<EditMeshSnapshot> undoStack_;
    std::deque<EditMeshSnapshot> redoStack_;
    size_t maxUndoLevels_ = 50;
    
    // 脏标记
    bool isDirty_ = false;
    
    void markDirty() { isDirty_ = true; }
    
    // 挤出选中面的内部实现（返回新顶点索引集合）
    std::set<uint32_t> extrudeSelectedFacesInternal(float dx, float dy, float dz) {
        std::set<uint32_t> newVertIndices;
        std::vector<uint32_t> facesToExtrude(selectedFaces.begin(), selectedFaces.end());
        
        // Map from old vertex index to new vertex index (shared across faces for connected geometry)
        std::map<uint32_t, uint32_t> vertexMap;
        
        // Collect all unique vertices used by selected faces
        for (uint32_t fi : facesToExtrude) {
            if (fi >= faces.size()) continue;
            const EditFace& face = faces[fi];
            for (const Loop& loop : face.loops) {
                if (vertexMap.find(loop.vertexIndex) == vertexMap.end()) {
                    const EditVertex& oldV = vertices[loop.vertexIndex];
                    uint32_t newVi = addVertex(
                        oldV.position[0] + dx,
                        oldV.position[1] + dy,
                        oldV.position[2] + dz
                    );
                    vertexMap[loop.vertexIndex] = newVi;
                    newVertIndices.insert(newVi);
                }
            }
        }
        
        // For each face, create side faces and update the face to use new vertices
        for (uint32_t fi : facesToExtrude) {
            if (fi >= faces.size()) continue;
            EditFace& face = faces[fi];
            
            // Check if edge is shared with another selected face
            // (shared internal edges should NOT get side faces)
            auto isEdgeInternal = [&](uint32_t va, uint32_t vb) -> bool {
                int sharedCount = 0;
                for (uint32_t otherFi : facesToExtrude) {
                    if (otherFi >= faces.size()) continue;
                    const EditFace& otherFace = faces[otherFi];
                    for (size_t i = 0; i < otherFace.loops.size(); i++) {
                        size_t next = (i + 1) % otherFace.loops.size();
                        uint32_t ev0 = otherFace.loops[i].vertexIndex;
                        uint32_t ev1 = otherFace.loops[next].vertexIndex;
                        if ((ev0 == va && ev1 == vb) || (ev0 == vb && ev1 == va)) {
                            sharedCount++;
                            if (sharedCount >= 2) return true;
                        }
                    }
                }
                return false;
            };
            
            // Create side faces for boundary edges only
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                uint32_t oldV0 = face.loops[i].vertexIndex;
                uint32_t oldV1 = face.loops[next].vertexIndex;
                
                if (!isEdgeInternal(oldV0, oldV1)) {
                    std::vector<uint32_t> sideVerts = {
                        oldV0, oldV1,
                        vertexMap[oldV1], vertexMap[oldV0]
                    };
                    addFace(sideVerts, face.materialIndex);
                    
                    // Generate simple UVs for side faces
                    if (faces.back().loops.size() == 4) {
                        faces.back().loops[0].uv[0] = 0; faces.back().loops[0].uv[1] = 0;
                        faces.back().loops[1].uv[0] = 1; faces.back().loops[1].uv[1] = 0;
                        faces.back().loops[2].uv[0] = 1; faces.back().loops[2].uv[1] = 1;
                        faces.back().loops[3].uv[0] = 0; faces.back().loops[3].uv[1] = 1;
                    }
                }
            }
            
            // Update original face to use new vertices
            for (size_t i = 0; i < face.loops.size(); i++) {
                face.loops[i].vertexIndex = vertexMap[face.loops[i].vertexIndex];
            }
            face.calculateNormal(vertices);
        }
        
        // Update selection to point at new vertices
        selectedVertices.clear();
        selectedVertices = newVertIndices;
        
        rebuildEdges();
        markDirty();
        return newVertIndices;
    }
    
    // 从面重建边数据
    void rebuildEdges() {
        edges.clear();
        
        for (const auto& face : faces) {
            for (size_t i = 0; i < face.loops.size(); i++) {
                size_t next = (i + 1) % face.loops.size();
                uint32_t v0 = face.loops[i].vertexIndex;
                uint32_t v1 = face.loops[next].vertexIndex;
                addEdgeIfNotExists(v0, v1, true);
            }
        }
    }
    
    // 移除未被任何面引用的顶点
    void removeUnusedVertices() {
        std::set<uint32_t> usedVertices;
        for (const auto& face : faces) {
            for (const auto& loop : face.loops) {
                usedVertices.insert(loop.vertexIndex);
            }
        }
        
        // 创建旧索引到新索引的映射
        std::vector<uint32_t> indexMap(vertices.size(), UINT32_MAX);
        std::vector<EditVertex> newVertices;
        
        for (uint32_t oldIdx : usedVertices) {
            indexMap[oldIdx] = static_cast<uint32_t>(newVertices.size());
            newVertices.push_back(vertices[oldIdx]);
        }
        
        // 更新所有引用
        for (auto& face : faces) {
            for (auto& loop : face.loops) {
                loop.vertexIndex = indexMap[loop.vertexIndex];
            }
        }
        
        for (auto& edge : edges) {
            edge.v0 = indexMap[edge.v0];
            edge.v1 = indexMap[edge.v1];
        }
        
        vertices = std::move(newVertices);
        selectedVertices.clear();
    }
};

}  // namespace luma
