// Mesh structure for rendering with PBR textures
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include "engine/foundation/math_types.h"

namespace luma {

struct Vertex {
    float position[3];
    float normal[3];
    float tangent[4];  // xyz = tangent, w = handedness
    float uv[2];       // Texture coordinates
    float color[3];    // Fallback color (used if no texture)
};

// Skinned vertex with bone weights (for skeletal animation)
struct SkinnedVertex {
    float position[3];
    float normal[3];
    float tangent[4];  // xyz = tangent, w = handedness
    float uv[2];       // Texture coordinates
    float color[3];    // Fallback color (used if no texture)
    uint32_t boneIndices[4] = {0, 0, 0, 0};  // Up to 4 bone influences
    float boneWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

// Texture data loaded from file
struct TextureData {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 4;  // RGBA
    std::string path;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Skinned vertices (used if hasSkeleton is true)
    std::vector<SkinnedVertex> skinnedVertices;
    bool hasSkeleton = false;
    
    // PBR Textures (optional)
    TextureData diffuseTexture;      // Base color / Albedo
    TextureData normalTexture;       // Normal map
    TextureData specularTexture;     // Specular / Metallic-Roughness
    bool hasDiffuseTexture = false;
    bool hasNormalTexture = false;
    bool hasSpecularTexture = false;
    
    // PBR material parameters (fallback when no textures)
    float baseColor[3] = {1.0f, 1.0f, 1.0f};  // Albedo/diffuse color
    float metallic = 0.0f;
    float roughness = 0.5f;
    std::string materialName;
};

// Generate a unit cube centered at origin
inline Mesh create_cube() {
    Mesh mesh;
    
    const float p = 0.5f;
    const float n = -0.5f;
    
    // 24 vertices (4 per face) with tangents
    // Format: position[3], normal[3], tangent[4], uv[2], color[3]
    // Note: color is white (1,1,1) - actual color comes from material baseColor
    mesh.vertices = {
        // Front face (z = +0.5), tangent = +X
        {{n, n, p}, {0, 0, 1}, {1, 0, 0, 1}, {0, 1}, {1, 1, 1}},
        {{p, n, p}, {0, 0, 1}, {1, 0, 0, 1}, {1, 1}, {1, 1, 1}},
        {{p, p, p}, {0, 0, 1}, {1, 0, 0, 1}, {1, 0}, {1, 1, 1}},
        {{n, p, p}, {0, 0, 1}, {1, 0, 0, 1}, {0, 0}, {1, 1, 1}},
        // Back face (z = -0.5), tangent = -X
        {{p, n, n}, {0, 0, -1}, {-1, 0, 0, 1}, {0, 1}, {1, 1, 1}},
        {{n, n, n}, {0, 0, -1}, {-1, 0, 0, 1}, {1, 1}, {1, 1, 1}},
        {{n, p, n}, {0, 0, -1}, {-1, 0, 0, 1}, {1, 0}, {1, 1, 1}},
        {{p, p, n}, {0, 0, -1}, {-1, 0, 0, 1}, {0, 0}, {1, 1, 1}},
        // Right face (x = +0.5), tangent = -Z
        {{p, n, p}, {1, 0, 0}, {0, 0, -1, 1}, {0, 1}, {1, 1, 1}},
        {{p, n, n}, {1, 0, 0}, {0, 0, -1, 1}, {1, 1}, {1, 1, 1}},
        {{p, p, n}, {1, 0, 0}, {0, 0, -1, 1}, {1, 0}, {1, 1, 1}},
        {{p, p, p}, {1, 0, 0}, {0, 0, -1, 1}, {0, 0}, {1, 1, 1}},
        // Left face (x = -0.5), tangent = +Z
        {{n, n, n}, {-1, 0, 0}, {0, 0, 1, 1}, {0, 1}, {1, 1, 1}},
        {{n, n, p}, {-1, 0, 0}, {0, 0, 1, 1}, {1, 1}, {1, 1, 1}},
        {{n, p, p}, {-1, 0, 0}, {0, 0, 1, 1}, {1, 0}, {1, 1, 1}},
        {{n, p, n}, {-1, 0, 0}, {0, 0, 1, 1}, {0, 0}, {1, 1, 1}},
        // Top face (y = +0.5), tangent = +X
        {{n, p, p}, {0, 1, 0}, {1, 0, 0, 1}, {0, 1}, {1, 1, 1}},
        {{p, p, p}, {0, 1, 0}, {1, 0, 0, 1}, {1, 1}, {1, 1, 1}},
        {{p, p, n}, {0, 1, 0}, {1, 0, 0, 1}, {1, 0}, {1, 1, 1}},
        {{n, p, n}, {0, 1, 0}, {1, 0, 0, 1}, {0, 0}, {1, 1, 1}},
        // Bottom face (y = -0.5), tangent = +X
        {{n, n, n}, {0, -1, 0}, {1, 0, 0, 1}, {0, 1}, {1, 1, 1}},
        {{p, n, n}, {0, -1, 0}, {1, 0, 0, 1}, {1, 1}, {1, 1, 1}},
        {{p, n, p}, {0, -1, 0}, {1, 0, 0, 1}, {1, 0}, {1, 1, 1}},
        {{n, n, p}, {0, -1, 0}, {1, 0, 0, 1}, {0, 0}, {1, 1, 1}},
    };
    
    mesh.indices = {
        0, 1, 2,  2, 3, 0,   // front
        4, 5, 6,  6, 7, 4,   // back
        8, 9, 10, 10, 11, 8, // right
        12, 13, 14, 14, 15, 12, // left
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20, // bottom
    };
    
    return mesh;
}

// Debug cube with different colors per face (for debugging normals/orientations)
inline Mesh create_debug_cube() {
    Mesh mesh;
    
    const float p = 0.5f;
    const float n = -0.5f;
    
    // Each face has a distinct color for debugging
    mesh.vertices = {
        // Front = Red
        {{n, n, p}, {0, 0, 1}, {1, 0, 0, 1}, {0, 1}, {1, 0, 0}},
        {{p, n, p}, {0, 0, 1}, {1, 0, 0, 1}, {1, 1}, {1, 0, 0}},
        {{p, p, p}, {0, 0, 1}, {1, 0, 0, 1}, {1, 0}, {1, 0, 0}},
        {{n, p, p}, {0, 0, 1}, {1, 0, 0, 1}, {0, 0}, {1, 0, 0}},
        // Back = Green
        {{p, n, n}, {0, 0, -1}, {-1, 0, 0, 1}, {0, 1}, {0, 1, 0}},
        {{n, n, n}, {0, 0, -1}, {-1, 0, 0, 1}, {1, 1}, {0, 1, 0}},
        {{n, p, n}, {0, 0, -1}, {-1, 0, 0, 1}, {1, 0}, {0, 1, 0}},
        {{p, p, n}, {0, 0, -1}, {-1, 0, 0, 1}, {0, 0}, {0, 1, 0}},
        // Right = Blue
        {{p, n, p}, {1, 0, 0}, {0, 0, -1, 1}, {0, 1}, {0, 0, 1}},
        {{p, n, n}, {1, 0, 0}, {0, 0, -1, 1}, {1, 1}, {0, 0, 1}},
        {{p, p, n}, {1, 0, 0}, {0, 0, -1, 1}, {1, 0}, {0, 0, 1}},
        {{p, p, p}, {1, 0, 0}, {0, 0, -1, 1}, {0, 0}, {0, 0, 1}},
        // Left = Yellow
        {{n, n, n}, {-1, 0, 0}, {0, 0, 1, 1}, {0, 1}, {1, 1, 0}},
        {{n, n, p}, {-1, 0, 0}, {0, 0, 1, 1}, {1, 1}, {1, 1, 0}},
        {{n, p, p}, {-1, 0, 0}, {0, 0, 1, 1}, {1, 0}, {1, 1, 0}},
        {{n, p, n}, {-1, 0, 0}, {0, 0, 1, 1}, {0, 0}, {1, 1, 0}},
        // Top = Magenta
        {{n, p, p}, {0, 1, 0}, {1, 0, 0, 1}, {0, 1}, {1, 0, 1}},
        {{p, p, p}, {0, 1, 0}, {1, 0, 0, 1}, {1, 1}, {1, 0, 1}},
        {{p, p, n}, {0, 1, 0}, {1, 0, 0, 1}, {1, 0}, {1, 0, 1}},
        {{n, p, n}, {0, 1, 0}, {1, 0, 0, 1}, {0, 0}, {1, 0, 1}},
        // Bottom = Cyan
        {{n, n, n}, {0, -1, 0}, {1, 0, 0, 1}, {0, 1}, {0, 1, 1}},
        {{p, n, n}, {0, -1, 0}, {1, 0, 0, 1}, {1, 1}, {0, 1, 1}},
        {{p, n, p}, {0, -1, 0}, {1, 0, 0, 1}, {1, 0}, {0, 1, 1}},
        {{n, n, p}, {0, -1, 0}, {1, 0, 0, 1}, {0, 0}, {0, 1, 1}},
    };
    
    mesh.indices = {
        0, 1, 2,  2, 3, 0,   // front
        4, 5, 6,  6, 7, 4,   // back
        8, 9, 10, 10, 11, 8, // right
        12, 13, 14, 14, 15, 12, // left
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20, // bottom
    };
    
    return mesh;
}

// Generate a cylinder mesh (for gizmo axes)
// radius: cylinder radius
// height: cylinder height (along Y axis by default)
// segments: number of segments around the cylinder
inline Mesh create_cylinder(float radius, float height, int segments = 16) {
    Mesh mesh;
    
    const float halfHeight = height * 0.5f;
    
    // Generate vertices for top and bottom circles
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 3.14159f * 2.0f;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        
        // Bottom circle
        float bottomX = x;
        float bottomY = -halfHeight;
        float bottomZ = z;
        float normalX = x / radius;
        float normalY = 0;
        float normalZ = z / radius;
        mesh.vertices.push_back({
            {bottomX, bottomY, bottomZ},
            {normalX, normalY, normalZ},
            {1, 0, 0, 1},  // tangent
            {(float)i / segments, 0},
            {1, 1, 1}  // color
        });
        
        // Top circle
        float topX = x;
        float topY = halfHeight;
        float topZ = z;
        mesh.vertices.push_back({
            {topX, topY, topZ},
            {normalX, normalY, normalZ},
            {1, 0, 0, 1},  // tangent
            {(float)i / segments, 1},
            {1, 1, 1}  // color
        });
    }
    
    // Generate indices for cylinder sides
    for (int i = 0; i < segments; i++) {
        int base = i * 2;
        // First triangle
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        // Second triangle
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 3);
    }
    
    return mesh;
}

}  // namespace luma
