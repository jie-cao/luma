// LUMA Mesh Picking
// 射线拾取系统 - 用于编辑模式下选择顶点/边/面

#pragma once

#include "../mesh/edit_mesh.h"
#include <cmath>
#include <limits>
#include <optional>

namespace luma {
namespace editor {

// 选择模式
enum class SelectionMode {
    Vertex,
    Edge,
    Face
};

// ============================================================================
// 射线结构
// ============================================================================
struct Ray {
    float origin[3];
    float direction[3];
    
    Ray() {
        origin[0] = origin[1] = origin[2] = 0;
        direction[0] = direction[1] = 0;
        direction[2] = -1;
    }
    
    Ray(const float* o, const float* d) {
        for (int i = 0; i < 3; i++) {
            origin[i] = o[i];
            direction[i] = d[i];
        }
    }
    
    // 获取射线上的点
    void getPoint(float t, float* out) const {
        for (int i = 0; i < 3; i++) {
            out[i] = origin[i] + direction[i] * t;
        }
    }
};

// ============================================================================
// 拾取结果
// ============================================================================
struct PickResult {
    enum class Type { None, Vertex, Edge, Face };
    
    Type type = Type::None;
    uint32_t index = 0;          // 顶点/边/面索引
    float distance = 0.0f;       // 到射线原点的距离
    float hitPoint[3] = {0, 0, 0}; // 命中点
    
    bool hit() const { return type != Type::None; }
};

// ============================================================================
// 网格拾取器
// ============================================================================
class MeshPicker {
public:
    // 拾取阈值（世界空间单位）
    // 对于典型角色模型（高度约 2 米），使用较大的值
    float vertexRadius = 0.15f;   // 顶点拾取半径
    float edgeThreshold = 0.08f;  // 边拾取阈值
    
    // =========================================================================
    // 从屏幕坐标创建射线
    // =========================================================================
    
    static Ray createRayFromScreen(float screenX, float screenY, 
                                    int viewportWidth, int viewportHeight,
                                    const float* viewMatrix, 
                                    const float* projMatrix) {
        Ray ray;
        
        // 将屏幕坐标转换为 NDC (-1 到 1)
        float ndcX = (2.0f * screenX / viewportWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * screenY / viewportHeight);  // Y 翻转
        
        // 计算逆 VP 矩阵
        float vp[16], invVP[16];
        multiplyMatrix(vp, viewMatrix, projMatrix);
        invertMatrix(invVP, vp);
        
        // 近平面点
        float nearPoint[4] = { ndcX, ndcY, 0.0f, 1.0f };
        float nearWorld[4];
        transformPoint(nearWorld, nearPoint, invVP);
        
        // 远平面点
        float farPoint[4] = { ndcX, ndcY, 1.0f, 1.0f };
        float farWorld[4];
        transformPoint(farWorld, farPoint, invVP);
        
        // 射线原点和方向
        for (int i = 0; i < 3; i++) {
            ray.origin[i] = nearWorld[i];
            ray.direction[i] = farWorld[i] - nearWorld[i];
        }
        
        // 归一化方向
        float len = std::sqrt(ray.direction[0]*ray.direction[0] + 
                              ray.direction[1]*ray.direction[1] + 
                              ray.direction[2]*ray.direction[2]);
        if (len > 1e-6f) {
            for (int i = 0; i < 3; i++) {
                ray.direction[i] /= len;
            }
        }
        
        return ray;
    }
    
    // =========================================================================
    // 拾取顶点
    // =========================================================================
    
    PickResult pickVertex(const Ray& ray, const EditMesh& mesh, 
                          const float* worldMatrix = nullptr) {
        PickResult result;
        result.type = PickResult::Type::None;
        float minDist = std::numeric_limits<float>::max();
        
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            float worldPos[3];
            transformPosition(worldPos, mesh.vertices[i].position, worldMatrix);
            
            // 计算点到射线的距离
            float dist = pointToRayDistance(worldPos, ray);
            
            if (dist < vertexRadius && dist < minDist) {
                // 计算沿射线的距离（用于深度排序）
                float t = rayParameterAtClosestPoint(worldPos, ray);
                if (t > 0) {  // 只拾取前方的点
                    minDist = dist;
                    result.type = PickResult::Type::Vertex;
                    result.index = static_cast<uint32_t>(i);
                    result.distance = t;
                    for (int j = 0; j < 3; j++) result.hitPoint[j] = worldPos[j];
                }
            }
        }
        
        return result;
    }
    
    // =========================================================================
    // 拾取边
    // =========================================================================
    
    PickResult pickEdge(const Ray& ray, const EditMesh& mesh,
                        const float* worldMatrix = nullptr) {
        PickResult result;
        result.type = PickResult::Type::None;
        float minDist = std::numeric_limits<float>::max();
        
        for (size_t i = 0; i < mesh.edges.size(); i++) {
            const auto& edge = mesh.edges[i];
            
            float p0[3], p1[3];
            transformPosition(p0, mesh.vertices[edge.v0].position, worldMatrix);
            transformPosition(p1, mesh.vertices[edge.v1].position, worldMatrix);
            
            // 计算射线到线段的最近距离
            float dist, t;
            lineSegmentToRayDistance(p0, p1, ray, dist, t);
            
            if (dist < edgeThreshold && dist < minDist && t > 0) {
                minDist = dist;
                result.type = PickResult::Type::Edge;
                result.index = static_cast<uint32_t>(i);
                result.distance = t;
                // 计算命中点（线段上最近点）
                ray.getPoint(t, result.hitPoint);
            }
        }
        
        return result;
    }
    
    // =========================================================================
    // 拾取面
    // =========================================================================
    
    PickResult pickFace(const Ray& ray, const EditMesh& mesh,
                        const float* worldMatrix = nullptr) {
        PickResult result;
        result.type = PickResult::Type::None;
        float minT = std::numeric_limits<float>::max();
        
        for (size_t fi = 0; fi < mesh.faces.size(); fi++) {
            const auto& face = mesh.faces[fi];
            if (face.loops.size() < 3) continue;
            
            // 三角化面并测试每个三角形
            float p0[3];
            transformPosition(p0, mesh.vertices[face.loops[0].vertexIndex].position, worldMatrix);
            
            for (size_t j = 1; j < face.loops.size() - 1; j++) {
                float p1[3], p2[3];
                transformPosition(p1, mesh.vertices[face.loops[j].vertexIndex].position, worldMatrix);
                transformPosition(p2, mesh.vertices[face.loops[j+1].vertexIndex].position, worldMatrix);
                
                float t;
                if (rayTriangleIntersect(ray, p0, p1, p2, t)) {
                    if (t > 0 && t < minT) {
                        minT = t;
                        result.type = PickResult::Type::Face;
                        result.index = static_cast<uint32_t>(fi);
                        result.distance = t;
                        ray.getPoint(t, result.hitPoint);
                    }
                }
            }
        }
        
        return result;
    }
    
    // =========================================================================
    // 综合拾取（根据选择模式）
    // =========================================================================
    
    PickResult pick(const Ray& ray, const EditMesh& mesh,
                    SelectionMode mode, const float* worldMatrix = nullptr) {
        switch (mode) {
            case SelectionMode::Vertex:
                return pickVertex(ray, mesh, worldMatrix);
            case SelectionMode::Edge:
                return pickEdge(ray, mesh, worldMatrix);
            case SelectionMode::Face:
                return pickFace(ray, mesh, worldMatrix);
            default:
                return PickResult();
        }
    }

private:
    // =========================================================================
    // 辅助数学函数
    // =========================================================================
    
    static void multiplyMatrix(float* out, const float* a, const float* b) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out[i * 4 + j] = 0;
                for (int k = 0; k < 4; k++) {
                    out[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
                }
            }
        }
    }
    
    static void invertMatrix(float* out, const float* m) {
        // 简化的 4x4 矩阵求逆（使用伴随矩阵法）
        float inv[16], det;
        int i;
        
        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
                 m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
                 m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
                 m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
                  m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
                 m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
                 m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
                 m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
                  m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
                 m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
                 m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
                  m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
                  m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
                 m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
                 m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
                  m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
                  m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
        
        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        
        if (std::abs(det) < 1e-10f) {
            for (i = 0; i < 16; i++) out[i] = 0;
            return;
        }
        
        det = 1.0f / det;
        for (i = 0; i < 16; i++) {
            out[i] = inv[i] * det;
        }
    }
    
    static void transformPoint(float* out, const float* p, const float* m) {
        out[0] = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12] * p[3];
        out[1] = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13] * p[3];
        out[2] = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14] * p[3];
        out[3] = m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15] * p[3];
        
        // 透视除法
        if (std::abs(out[3]) > 1e-6f) {
            out[0] /= out[3];
            out[1] /= out[3];
            out[2] /= out[3];
        }
    }
    
    static void transformPosition(float* out, const float* p, const float* m) {
        if (!m) {
            out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
            return;
        }
        
        out[0] = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12];
        out[1] = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13];
        out[2] = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14];
    }
    
    static float pointToRayDistance(const float* point, const Ray& ray) {
        // 点到射线的距离
        float v[3] = { point[0] - ray.origin[0], 
                       point[1] - ray.origin[1], 
                       point[2] - ray.origin[2] };
        
        float dot = v[0] * ray.direction[0] + v[1] * ray.direction[1] + v[2] * ray.direction[2];
        
        float proj[3] = { ray.direction[0] * dot, 
                          ray.direction[1] * dot, 
                          ray.direction[2] * dot };
        
        float diff[3] = { v[0] - proj[0], v[1] - proj[1], v[2] - proj[2] };
        
        return std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
    }
    
    static float rayParameterAtClosestPoint(const float* point, const Ray& ray) {
        float v[3] = { point[0] - ray.origin[0], 
                       point[1] - ray.origin[1], 
                       point[2] - ray.origin[2] };
        
        return v[0] * ray.direction[0] + v[1] * ray.direction[1] + v[2] * ray.direction[2];
    }
    
    static void lineSegmentToRayDistance(const float* p0, const float* p1, const Ray& ray,
                                         float& distance, float& rayT) {
        // 计算两条线（射线和线段）之间的最短距离
        float u[3] = { ray.direction[0], ray.direction[1], ray.direction[2] };
        float v[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
        float w[3] = { ray.origin[0] - p0[0], ray.origin[1] - p0[1], ray.origin[2] - p0[2] };
        
        float a = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];  // |u|^2
        float b = u[0] * v[0] + u[1] * v[1] + u[2] * v[2];  // u · v
        float c = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];  // |v|^2
        float d = u[0] * w[0] + u[1] * w[1] + u[2] * w[2];  // u · w
        float e = v[0] * w[0] + v[1] * w[1] + v[2] * w[2];  // v · w
        
        float D = a * c - b * b;
        float sc, tc;
        
        if (D < 1e-6f) {
            // 平行线
            sc = 0.0f;
            tc = (b > c ? d / b : e / c);
        } else {
            sc = (b * e - c * d) / D;
            tc = (a * e - b * d) / D;
        }
        
        // 限制 tc 到 [0, 1]（线段）
        tc = std::clamp(tc, 0.0f, 1.0f);
        
        // 计算最近点
        float cp1[3] = { ray.origin[0] + sc * u[0], 
                         ray.origin[1] + sc * u[1], 
                         ray.origin[2] + sc * u[2] };
        float cp2[3] = { p0[0] + tc * v[0], 
                         p0[1] + tc * v[1], 
                         p0[2] + tc * v[2] };
        
        float diff[3] = { cp1[0] - cp2[0], cp1[1] - cp2[1], cp1[2] - cp2[2] };
        distance = std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
        rayT = sc;
    }
    
    static bool rayTriangleIntersect(const Ray& ray, const float* v0, const float* v1, 
                                      const float* v2, float& t) {
        // Möller–Trumbore intersection algorithm
        const float EPSILON = 1e-6f;
        
        float edge1[3] = { v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2] };
        float edge2[3] = { v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2] };
        
        float h[3];
        cross(h, ray.direction, edge2);
        float a = dot(edge1, h);
        
        if (a > -EPSILON && a < EPSILON) return false;  // 平行
        
        float f = 1.0f / a;
        float s[3] = { ray.origin[0] - v0[0], ray.origin[1] - v0[1], ray.origin[2] - v0[2] };
        float u = f * dot(s, h);
        
        if (u < 0.0f || u > 1.0f) return false;
        
        float q[3];
        cross(q, s, edge1);
        float v = f * dot(ray.direction, q);
        
        if (v < 0.0f || u + v > 1.0f) return false;
        
        t = f * dot(edge2, q);
        return t > EPSILON;
    }
    
    static void cross(float* out, const float* a, const float* b) {
        out[0] = a[1] * b[2] - a[2] * b[1];
        out[1] = a[2] * b[0] - a[0] * b[2];
        out[2] = a[0] * b[1] - a[1] * b[0];
    }
    
    static float dot(const float* a, const float* b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }
};

} // namespace editor
} // namespace luma
