// LUMA Mesh System
// 统一的网格数据管理模块
//
// 设计理念：
// - 场景模式：使用 RenderMesh（GPU 优化格式，高效渲染）
// - 编辑模式：使用 EditMesh（支持四边面/N-gon，完整拓扑编辑）
// - 保存/固化：EditMesh → RenderMesh 转换
//
// 使用示例：
//
//   // 进入编辑模式
//   EditMesh editMesh = MeshConverter::toEditMesh(renderMesh);
//   
//   // 编辑操作...
//   editMesh.extrudeSelectedFaces(0, 1, 0);
//   editMesh.subdivideSelectedFaces();
//   
//   // 保存/固化（转换回 GPU 优化格式）
//   RenderMesh newRenderMesh = MeshConverter::toRenderMesh(editMesh);
//   uploadToGPU(newRenderMesh);
//

#pragma once

// 编辑网格（支持四边面、N-gon、完整拓扑编辑）
#include "edit_mesh.h"

// 渲染网格（GPU 优化格式，三角化、交错顶点布局）
#include "render_mesh.h"

// 网格转换器（EditMesh ↔ RenderMesh）
#include "mesh_converter.h"

namespace luma {

// ============================================================================
// 快捷类型别名
// ============================================================================

using EditMeshPtr = std::unique_ptr<EditMesh>;
using RenderMeshPtr = std::unique_ptr<RenderMesh>;

// ============================================================================
// 网格系统版本信息
// ============================================================================

struct MeshSystemInfo {
    static constexpr int VERSION_MAJOR = 1;
    static constexpr int VERSION_MINOR = 0;
    static constexpr const char* VERSION_STRING = "1.0.0";
    
    // 功能支持
    static constexpr bool SUPPORTS_QUADS = true;
    static constexpr bool SUPPORTS_NGONS = true;
    static constexpr bool SUPPORTS_UV_EDITING = true;
    static constexpr bool SUPPORTS_VERTEX_COLORS = true;
    static constexpr int MAX_UV_CHANNELS = 1;  // 当前只支持一个 UV 通道
    static constexpr int MAX_UNDO_LEVELS = 50;
};

// ============================================================================
// 创建基础几何体
// ============================================================================

namespace primitives {

// 创建立方体 EditMesh
inline EditMesh createCube(float size = 1.0f) {
    EditMesh mesh;
    float h = size * 0.5f;
    
    // 8 个顶点
    mesh.addVertex(-h, -h, -h);  // 0
    mesh.addVertex( h, -h, -h);  // 1
    mesh.addVertex( h,  h, -h);  // 2
    mesh.addVertex(-h,  h, -h);  // 3
    mesh.addVertex(-h, -h,  h);  // 4
    mesh.addVertex( h, -h,  h);  // 5
    mesh.addVertex( h,  h,  h);  // 6
    mesh.addVertex(-h,  h,  h);  // 7
    
    // 6 个四边形面
    mesh.addFace({0, 1, 2, 3});  // 前
    mesh.addFace({5, 4, 7, 6});  // 后
    mesh.addFace({4, 0, 3, 7});  // 左
    mesh.addFace({1, 5, 6, 2});  // 右
    mesh.addFace({3, 2, 6, 7});  // 上
    mesh.addFace({4, 5, 1, 0});  // 下
    
    // 设置 UV
    for (auto& face : mesh.faces) {
        if (face.loops.size() == 4) {
            face.loops[0].uv[0] = 0; face.loops[0].uv[1] = 0;
            face.loops[1].uv[0] = 1; face.loops[1].uv[1] = 0;
            face.loops[2].uv[0] = 1; face.loops[2].uv[1] = 1;
            face.loops[3].uv[0] = 0; face.loops[3].uv[1] = 1;
        }
    }
    
    return mesh;
}

// 创建平面 EditMesh
inline EditMesh createPlane(float width = 1.0f, float height = 1.0f) {
    EditMesh mesh;
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    
    mesh.addVertex(-hw, 0, -hh);  // 0
    mesh.addVertex( hw, 0, -hh);  // 1
    mesh.addVertex( hw, 0,  hh);  // 2
    mesh.addVertex(-hw, 0,  hh);  // 3
    
    mesh.addFace({0, 1, 2, 3});
    
    // UV
    mesh.faces[0].loops[0].uv[0] = 0; mesh.faces[0].loops[0].uv[1] = 0;
    mesh.faces[0].loops[1].uv[0] = 1; mesh.faces[0].loops[1].uv[1] = 0;
    mesh.faces[0].loops[2].uv[0] = 1; mesh.faces[0].loops[2].uv[1] = 1;
    mesh.faces[0].loops[3].uv[0] = 0; mesh.faces[0].loops[3].uv[1] = 1;
    
    return mesh;
}

// 创建球体 EditMesh（使用 UV 球方法）
inline EditMesh createSphere(float radius = 0.5f, int segments = 16, int rings = 8) {
    EditMesh mesh;
    
    const float PI = 3.14159265358979323846f;
    
    // 生成顶点
    for (int r = 0; r <= rings; r++) {
        float phi = PI * r / rings;
        float y = radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);
        
        for (int s = 0; s <= segments; s++) {
            float theta = 2.0f * PI * s / segments;
            float x = ringRadius * std::cos(theta);
            float z = ringRadius * std::sin(theta);
            
            mesh.addVertex(x, y, z);
        }
    }
    
    // 生成四边形面
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            int i0 = r * (segments + 1) + s;
            int i1 = i0 + 1;
            int i2 = i0 + segments + 2;
            int i3 = i0 + segments + 1;
            
            // 极点附近使用三角形
            if (r == 0) {
                mesh.addFace({(uint32_t)i0, (uint32_t)i1, (uint32_t)i2});
            } else if (r == rings - 1) {
                mesh.addFace({(uint32_t)i0, (uint32_t)i1, (uint32_t)i3});
            } else {
                mesh.addFace({(uint32_t)i0, (uint32_t)i1, (uint32_t)i2, (uint32_t)i3});
            }
        }
    }
    
    // 设置 UV（球形映射）
    for (auto& face : mesh.faces) {
        for (auto& loop : face.loops) {
            const auto& v = mesh.vertices[loop.vertexIndex];
            float theta = std::atan2(v.position[2], v.position[0]);
            float phi = std::acos(v.position[1] / radius);
            loop.uv[0] = (theta + PI) / (2.0f * PI);
            loop.uv[1] = phi / PI;
        }
    }
    
    return mesh;
}

// 创建圆柱体 EditMesh
inline EditMesh createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 16) {
    EditMesh mesh;
    
    const float PI = 3.14159265358979323846f;
    float h = height * 0.5f;
    
    // 顶部中心和底部中心
    uint32_t topCenter = mesh.addVertex(0, h, 0);
    uint32_t bottomCenter = mesh.addVertex(0, -h, 0);
    
    // 侧面顶点
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * PI * i / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        
        mesh.addVertex(x, h, z);   // 顶部环
        mesh.addVertex(x, -h, z);  // 底部环
    }
    
    // 侧面（四边形）
    for (int i = 0; i < segments; i++) {
        uint32_t t0 = 2 + i * 2;
        uint32_t t1 = 2 + (i + 1) * 2;
        uint32_t b0 = t0 + 1;
        uint32_t b1 = t1 + 1;
        
        mesh.addFace({t0, t1, b1, b0});
    }
    
    // 顶部和底部（扇形，使用三角形）
    for (int i = 0; i < segments; i++) {
        uint32_t t0 = 2 + i * 2;
        uint32_t t1 = 2 + (i + 1) * 2;
        uint32_t b0 = t0 + 1;
        uint32_t b1 = t1 + 1;
        
        mesh.addFace({topCenter, t1, t0});     // 顶部
        mesh.addFace({bottomCenter, b0, b1});  // 底部
    }
    
    // 设置 UV
    mesh.projectUVBox();
    
    return mesh;
}

}  // namespace primitives

}  // namespace luma
