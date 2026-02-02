// LUMA Character UV Mapping System
// Provides optimized UV layouts for character textures
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <cmath>

namespace luma {

// ============================================================================
// Body Region for UV Layout
// ============================================================================

enum class BodyRegion {
    Head,       // 0.0 - 0.2 V
    Neck,       // 0.2 - 0.25 V
    Torso,      // 0.25 - 0.5 V
    Pelvis,     // 0.5 - 0.55 V
    UpperLeg,   // 0.55 - 0.75 V
    LowerLeg,   // 0.75 - 0.95 V
    Foot        // 0.95 - 1.0 V
};

// ============================================================================
// UV Region Definition (for texture atlas)
// ============================================================================

struct UVRegion {
    float u0, v0;  // Bottom-left
    float u1, v1;  // Top-right
    
    UVRegion() : u0(0), v0(0), u1(1), v1(1) {}
    UVRegion(float _u0, float _v0, float _u1, float _v1) 
        : u0(_u0), v0(_v0), u1(_u1), v1(_v1) {}
    
    // Remap UV from 0-1 space to this region
    Vec2 remap(float u, float v) const {
        return Vec2(u0 + u * (u1 - u0), v0 + v * (v1 - v0));
    }
};

// ============================================================================
// UV Layout Presets
// ============================================================================

struct UVLayoutPreset {
    std::string name;
    
    // Regions for different body parts
    UVRegion headFront;
    UVRegion headBack;
    UVRegion torsoFront;
    UVRegion torsoBack;
    UVRegion leftArm;
    UVRegion rightArm;
    UVRegion leftLeg;
    UVRegion rightLeg;
    
    // Standard layout - simple cylindrical projection
    static UVLayoutPreset cylindrical() {
        UVLayoutPreset p;
        p.name = "Cylindrical";
        // Full UV space, simple wrap
        p.headFront = UVRegion(0, 0.8f, 1, 1.0f);
        p.headBack = UVRegion(0, 0.8f, 1, 1.0f);
        p.torsoFront = UVRegion(0, 0.4f, 0.5f, 0.8f);
        p.torsoBack = UVRegion(0.5f, 0.4f, 1.0f, 0.8f);
        p.leftArm = UVRegion(0, 0.2f, 0.25f, 0.4f);
        p.rightArm = UVRegion(0.25f, 0.2f, 0.5f, 0.4f);
        p.leftLeg = UVRegion(0, 0, 0.25f, 0.2f);
        p.rightLeg = UVRegion(0.25f, 0, 0.5f, 0.2f);
        return p;
    }
    
    // Atlas layout - optimized for separate body part textures
    static UVLayoutPreset atlas() {
        UVLayoutPreset p;
        p.name = "Atlas";
        // Organized grid layout
        p.headFront = UVRegion(0, 0.75f, 0.25f, 1.0f);
        p.headBack = UVRegion(0.25f, 0.75f, 0.5f, 1.0f);
        p.torsoFront = UVRegion(0, 0.25f, 0.5f, 0.75f);
        p.torsoBack = UVRegion(0.5f, 0.25f, 1.0f, 0.75f);
        p.leftArm = UVRegion(0.5f, 0.75f, 0.75f, 1.0f);
        p.rightArm = UVRegion(0.75f, 0.75f, 1.0f, 1.0f);
        p.leftLeg = UVRegion(0, 0, 0.25f, 0.25f);
        p.rightLeg = UVRegion(0.25f, 0, 0.5f, 0.25f);
        return p;
    }
};

// ============================================================================
// UV Mapping Utility
// ============================================================================

class UVMapper {
public:
    // Apply cylindrical UV mapping to a mesh (default for procedural bodies)
    static void applyCylindricalUV(std::vector<Vertex>& vertices, 
                                    float heightMin = 0.0f, 
                                    float heightMax = 1.8f) {
        for (auto& v : vertices) {
            // U: angle around Y axis (0-1)
            float u = std::atan2(v.position[2], v.position[0]);
            u = (u + 3.14159f) / (2.0f * 3.14159f);  // Normalize to 0-1
            
            // V: height (normalized)
            float vCoord = (v.position[1] - heightMin) / (heightMax - heightMin);
            vCoord = std::clamp(vCoord, 0.0f, 1.0f);
            
            v.uv[0] = u;
            v.uv[1] = vCoord;
        }
    }
    
    // Apply box projection UV mapping (good for head)
    static void applyBoxProjectionUV(std::vector<Vertex>& vertices,
                                      uint32_t startIdx, uint32_t count,
                                      const Vec3& center, float size) {
        for (uint32_t i = startIdx; i < startIdx + count && i < vertices.size(); i++) {
            auto& v = vertices[i];
            
            // Get position relative to center
            Vec3 pos(v.position[0] - center.x, 
                     v.position[1] - center.y, 
                     v.position[2] - center.z);
            
            // Determine dominant axis
            float absX = std::abs(pos.x);
            float absY = std::abs(pos.y);
            float absZ = std::abs(pos.z);
            
            float u, vCoord;
            
            if (absZ >= absX && absZ >= absY) {
                // Front/Back face
                u = (pos.x / size + 1.0f) * 0.5f;
                vCoord = (pos.y / size + 1.0f) * 0.5f;
            } else if (absX >= absY) {
                // Left/Right face
                u = (pos.z / size + 1.0f) * 0.5f;
                vCoord = (pos.y / size + 1.0f) * 0.5f;
            } else {
                // Top/Bottom face
                u = (pos.x / size + 1.0f) * 0.5f;
                vCoord = (pos.z / size + 1.0f) * 0.5f;
            }
            
            v.uv[0] = std::clamp(u, 0.0f, 1.0f);
            v.uv[1] = std::clamp(vCoord, 0.0f, 1.0f);
        }
    }
    
    // Improve UV seams by averaging UVs at seam vertices
    static void fixUVSeams(std::vector<Vertex>& vertices, float threshold = 0.001f) {
        // Find vertices at the same position and average their UVs
        for (size_t i = 0; i < vertices.size(); i++) {
            for (size_t j = i + 1; j < vertices.size(); j++) {
                // Check if positions are the same
                float dx = vertices[i].position[0] - vertices[j].position[0];
                float dy = vertices[i].position[1] - vertices[j].position[1];
                float dz = vertices[i].position[2] - vertices[j].position[2];
                float distSq = dx*dx + dy*dy + dz*dz;
                
                if (distSq < threshold * threshold) {
                    // Check for UV seam (large U difference)
                    float uDiff = std::abs(vertices[i].uv[0] - vertices[j].uv[0]);
                    if (uDiff > 0.5f) {
                        // This is a seam - adjust the smaller U
                        if (vertices[i].uv[0] < 0.5f) {
                            vertices[i].uv[0] += 1.0f;
                        } else {
                            vertices[j].uv[0] += 1.0f;
                        }
                    }
                }
            }
        }
    }
    
    // Scale UVs to fit within a region
    static void remapUVToRegion(std::vector<Vertex>& vertices,
                                 uint32_t startIdx, uint32_t count,
                                 const UVRegion& region) {
        for (uint32_t i = startIdx; i < startIdx + count && i < vertices.size(); i++) {
            Vec2 remapped = region.remap(vertices[i].uv[0], vertices[i].uv[1]);
            vertices[i].uv[0] = remapped.x;
            vertices[i].uv[1] = remapped.y;
        }
    }
    
    // Generate smooth tangents for normal mapping
    static void calculateTangents(std::vector<Vertex>& vertices,
                                   const std::vector<uint32_t>& indices) {
        // Reset tangents
        for (auto& v : vertices) {
            v.tangent[0] = 0;
            v.tangent[1] = 0;
            v.tangent[2] = 0;
            v.tangent[3] = 1.0f;
        }
        
        // Accumulate tangents per triangle
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            
            const Vertex& v0 = vertices[i0];
            const Vertex& v1 = vertices[i1];
            const Vertex& v2 = vertices[i2];
            
            // Edge vectors
            Vec3 edge1(v1.position[0] - v0.position[0],
                       v1.position[1] - v0.position[1],
                       v1.position[2] - v0.position[2]);
            Vec3 edge2(v2.position[0] - v0.position[0],
                       v2.position[1] - v0.position[1],
                       v2.position[2] - v0.position[2]);
            
            // UV deltas
            float du1 = v1.uv[0] - v0.uv[0];
            float dv1 = v1.uv[1] - v0.uv[1];
            float du2 = v2.uv[0] - v0.uv[0];
            float dv2 = v2.uv[1] - v0.uv[1];
            
            float det = du1 * dv2 - du2 * dv1;
            if (std::abs(det) < 0.0001f) det = 1.0f;
            float invDet = 1.0f / det;
            
            Vec3 tangent(
                (edge1.x * dv2 - edge2.x * dv1) * invDet,
                (edge1.y * dv2 - edge2.y * dv1) * invDet,
                (edge1.z * dv2 - edge2.z * dv1) * invDet
            );
            
            // Accumulate
            vertices[i0].tangent[0] += tangent.x;
            vertices[i0].tangent[1] += tangent.y;
            vertices[i0].tangent[2] += tangent.z;
            
            vertices[i1].tangent[0] += tangent.x;
            vertices[i1].tangent[1] += tangent.y;
            vertices[i1].tangent[2] += tangent.z;
            
            vertices[i2].tangent[0] += tangent.x;
            vertices[i2].tangent[1] += tangent.y;
            vertices[i2].tangent[2] += tangent.z;
        }
        
        // Normalize and orthogonalize
        for (auto& v : vertices) {
            Vec3 t(v.tangent[0], v.tangent[1], v.tangent[2]);
            Vec3 n(v.normal[0], v.normal[1], v.normal[2]);
            
            // Gram-Schmidt orthogonalize
            float dot = t.x * n.x + t.y * n.y + t.z * n.z;
            t.x -= n.x * dot;
            t.y -= n.y * dot;
            t.z -= n.z * dot;
            
            // Normalize
            float len = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
            if (len > 0.0001f) {
                v.tangent[0] = t.x / len;
                v.tangent[1] = t.y / len;
                v.tangent[2] = t.z / len;
            } else {
                // Fallback tangent
                if (std::abs(n.y) < 0.9f) {
                    v.tangent[0] = 1.0f;
                    v.tangent[1] = 0.0f;
                    v.tangent[2] = 0.0f;
                } else {
                    v.tangent[0] = 0.0f;
                    v.tangent[1] = 0.0f;
                    v.tangent[2] = 1.0f;
                }
            }
            v.tangent[3] = 1.0f;  // Handedness
        }
    }
    
    // Apply optimized UV layout for human body
    static void applyHumanBodyUV(std::vector<Vertex>& vertices,
                                  const std::vector<uint32_t>& indices,
                                  float bodyHeight = 1.8f) {
        // First apply cylindrical mapping
        applyCylindricalUV(vertices, 0.0f, bodyHeight);
        
        // Recalculate tangents for normal mapping
        calculateTangents(vertices, indices);
        
        // Fix seams
        fixUVSeams(vertices);
    }
};

// ============================================================================
// Face UV Mapping (specialized for face region)
// ============================================================================

class FaceUVMapper {
public:
    // Apply spherical UV to face region
    static void applySphericalUV(std::vector<Vertex>& vertices,
                                  uint32_t startIdx, uint32_t count,
                                  const Vec3& faceCenter) {
        for (uint32_t i = startIdx; i < startIdx + count && i < vertices.size(); i++) {
            auto& v = vertices[i];
            
            // Direction from face center
            Vec3 dir(v.position[0] - faceCenter.x,
                     v.position[1] - faceCenter.y,
                     v.position[2] - faceCenter.z);
            
            float len = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
            if (len > 0.001f) {
                dir.x /= len;
                dir.y /= len;
                dir.z /= len;
            }
            
            // Spherical to UV
            float u = 0.5f + std::atan2(dir.x, dir.z) / (2.0f * 3.14159f);
            float vCoord = 0.5f - std::asin(std::clamp(dir.y, -1.0f, 1.0f)) / 3.14159f;
            
            v.uv[0] = std::clamp(u, 0.0f, 1.0f);
            v.uv[1] = std::clamp(vCoord, 0.0f, 1.0f);
        }
    }
    
    // Apply planar projection for face front
    static void applyFrontalUV(std::vector<Vertex>& vertices,
                                uint32_t startIdx, uint32_t count,
                                const Vec3& faceCenter, float faceSize) {
        for (uint32_t i = startIdx; i < startIdx + count && i < vertices.size(); i++) {
            auto& v = vertices[i];
            
            // Project onto XY plane
            float u = (v.position[0] - faceCenter.x) / faceSize + 0.5f;
            float vCoord = (v.position[1] - faceCenter.y) / faceSize + 0.5f;
            
            v.uv[0] = std::clamp(u, 0.0f, 1.0f);
            v.uv[1] = std::clamp(vCoord, 0.0f, 1.0f);
        }
    }
};

}  // namespace luma
