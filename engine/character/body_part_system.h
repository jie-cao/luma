// LUMA Body Part System
// Modular body parts for assembling different character types
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace luma {

// ============================================================================
// Body Part Types
// ============================================================================

enum class BodyPartType {
    // Core
    Head,
    Torso,
    
    // Limbs
    LeftArm,
    RightArm,
    LeftLeg,
    RightLeg,
    LeftHand,
    RightHand,
    LeftFoot,
    RightFoot,
    
    // Head features
    LeftEye,
    RightEye,
    Nose,
    Mouth,
    LeftEar,
    RightEar,
    
    // Extras
    Tail,
    LeftWing,
    RightWing,
    Antenna,
    
    // Accessories attachment points
    Hat,
    Bow,
    Collar,
    
    Custom
};

inline const char* getBodyPartName(BodyPartType type) {
    switch (type) {
        case BodyPartType::Head: return "Head";
        case BodyPartType::Torso: return "Torso";
        case BodyPartType::LeftArm: return "Left Arm";
        case BodyPartType::RightArm: return "Right Arm";
        case BodyPartType::LeftLeg: return "Left Leg";
        case BodyPartType::RightLeg: return "Right Leg";
        case BodyPartType::LeftHand: return "Left Hand";
        case BodyPartType::RightHand: return "Right Hand";
        case BodyPartType::LeftFoot: return "Left Foot";
        case BodyPartType::RightFoot: return "Right Foot";
        case BodyPartType::LeftEye: return "Left Eye";
        case BodyPartType::RightEye: return "Right Eye";
        case BodyPartType::Nose: return "Nose";
        case BodyPartType::Mouth: return "Mouth";
        case BodyPartType::LeftEar: return "Left Ear";
        case BodyPartType::RightEar: return "Right Ear";
        case BodyPartType::Tail: return "Tail";
        case BodyPartType::LeftWing: return "Left Wing";
        case BodyPartType::RightWing: return "Right Wing";
        case BodyPartType::Antenna: return "Antenna";
        case BodyPartType::Hat: return "Hat";
        case BodyPartType::Bow: return "Bow";
        case BodyPartType::Collar: return "Collar";
        case BodyPartType::Custom: return "Custom";
    }
    return "Unknown";
}

// ============================================================================
// Body Part Shape
// ============================================================================

enum class PartShape {
    Sphere,         // Basic sphere
    Ellipsoid,      // Stretched sphere
    Capsule,        // Pill shape
    Cylinder,       // Cylinder
    Cone,           // Cone
    Box,            // Box/cube
    Custom          // Custom mesh
};

// ============================================================================
// Body Part Definition
// ============================================================================

struct BodyPartDef {
    std::string id;
    std::string name;
    BodyPartType type = BodyPartType::Custom;
    
    // Shape
    PartShape shape = PartShape::Sphere;
    Vec3 size{1, 1, 1};          // Size in each axis
    Vec3 offset{0, 0, 0};        // Position offset from attachment point
    Quat rotation;               // Local rotation
    
    // Mesh generation params
    int segments = 16;           // Subdivision level
    
    // Color
    Vec3 color{1, 1, 1};
    bool useParentColor = true;  // Inherit color from parent
    
    // Attachment
    std::string parentPartId;    // Which part this attaches to
    Vec3 attachPoint{0, 0, 0};   // Local attachment position on parent
    
    // Bones
    std::string boneName;        // Associated bone name
    bool createBone = true;      // Auto-create bone for this part
    
    // Mirroring
    bool isMirrored = false;     // Is this part mirrored from another?
    std::string mirrorSourceId;  // Source part ID for mirror
    
    // Custom mesh (if shape == Custom)
    std::vector<Vertex> customVertices;
    std::vector<uint32_t> customIndices;
};

// ============================================================================
// Procedural Part Generator
// ============================================================================

class ProceduralPartGenerator {
public:
    // Generate mesh for a body part
    static void generatePartMesh(const BodyPartDef& def,
                                 std::vector<Vertex>& outVertices,
                                 std::vector<uint32_t>& outIndices) {
        switch (def.shape) {
            case PartShape::Sphere:
                generateSphere(def, outVertices, outIndices);
                break;
            case PartShape::Ellipsoid:
                generateEllipsoid(def, outVertices, outIndices);
                break;
            case PartShape::Capsule:
                generateCapsule(def, outVertices, outIndices);
                break;
            case PartShape::Cylinder:
                generateCylinder(def, outVertices, outIndices);
                break;
            case PartShape::Cone:
                generateCone(def, outVertices, outIndices);
                break;
            case PartShape::Box:
                generateBox(def, outVertices, outIndices);
                break;
            case PartShape::Custom:
                outVertices = def.customVertices;
                outIndices = def.customIndices;
                break;
        }
        
        // Apply transform
        applyTransform(outVertices, def.offset, def.rotation, def.size);
        
        // Apply color
        Vec3 color = def.color;
        for (auto& v : outVertices) {
            v.color[0] = color.x;
            v.color[1] = color.y;
            v.color[2] = color.z;
        }
    }
    
private:
    static void generateSphere(const BodyPartDef& def,
                              std::vector<Vertex>& verts,
                              std::vector<uint32_t>& indices) {
        int segments = def.segments;
        int rings = segments / 2;
        
        // Vertices
        for (int lat = 0; lat <= rings; lat++) {
            float theta = lat * 3.14159f / rings;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * 3.14159f / segments;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);
                
                Vertex v;
                v.position[0] = sinTheta * cosPhi * 0.5f;
                v.position[1] = cosTheta * 0.5f;
                v.position[2] = sinTheta * sinPhi * 0.5f;
                
                v.normal[0] = sinTheta * cosPhi;
                v.normal[1] = cosTheta;
                v.normal[2] = sinTheta * sinPhi;
                
                v.uv[0] = (float)lon / segments;
                v.uv[1] = (float)lat / rings;
                
                v.tangent[0] = -sinPhi;
                v.tangent[1] = 0;
                v.tangent[2] = cosPhi;
                v.tangent[3] = 1;
                
                verts.push_back(v);
            }
        }
        
        // Indices
        int vertsPerRow = segments + 1;
        for (int lat = 0; lat < rings; lat++) {
            for (int lon = 0; lon < segments; lon++) {
                int current = lat * vertsPerRow + lon;
                int next = current + vertsPerRow;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    }
    
    static void generateEllipsoid(const BodyPartDef& def,
                                  std::vector<Vertex>& verts,
                                  std::vector<uint32_t>& indices) {
        // Same as sphere, scaling handled by applyTransform
        generateSphere(def, verts, indices);
    }
    
    static void generateCapsule(const BodyPartDef& def,
                               std::vector<Vertex>& verts,
                               std::vector<uint32_t>& indices) {
        int segments = def.segments;
        int rings = segments / 4;
        float radius = 0.5f;
        float height = 1.0f;
        float halfHeight = height * 0.5f - radius;
        
        // Top hemisphere
        for (int lat = 0; lat <= rings; lat++) {
            float theta = lat * 3.14159f / (rings * 2);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * 3.14159f / segments;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);
                
                Vertex v;
                v.position[0] = sinTheta * cosPhi * radius;
                v.position[1] = cosTheta * radius + halfHeight;
                v.position[2] = sinTheta * sinPhi * radius;
                
                v.normal[0] = sinTheta * cosPhi;
                v.normal[1] = cosTheta;
                v.normal[2] = sinTheta * sinPhi;
                
                v.uv[0] = (float)lon / segments;
                v.uv[1] = (float)lat / (rings * 2 + 2) * 0.5f;
                
                verts.push_back(v);
            }
        }
        
        // Cylinder middle
        for (int row = 0; row <= 1; row++) {
            float y = halfHeight - row * 2 * halfHeight;
            
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * 3.14159f / segments;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);
                
                Vertex v;
                v.position[0] = cosPhi * radius;
                v.position[1] = y;
                v.position[2] = sinPhi * radius;
                
                v.normal[0] = cosPhi;
                v.normal[1] = 0;
                v.normal[2] = sinPhi;
                
                v.uv[0] = (float)lon / segments;
                v.uv[1] = 0.25f + row * 0.5f;
                
                verts.push_back(v);
            }
        }
        
        // Bottom hemisphere
        for (int lat = rings; lat >= 0; lat--) {
            float theta = lat * 3.14159f / (rings * 2) + 3.14159f * 0.5f;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * 3.14159f / segments;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);
                
                Vertex v;
                v.position[0] = sinTheta * cosPhi * radius;
                v.position[1] = cosTheta * radius - halfHeight;
                v.position[2] = sinTheta * sinPhi * radius;
                
                v.normal[0] = sinTheta * cosPhi;
                v.normal[1] = cosTheta;
                v.normal[2] = sinTheta * sinPhi;
                
                v.uv[0] = (float)lon / segments;
                v.uv[1] = 0.75f + (float)(rings - lat) / (rings * 2) * 0.25f;
                
                verts.push_back(v);
            }
        }
        
        // Generate indices
        int totalRings = rings + 2 + rings;
        int vertsPerRow = segments + 1;
        for (int ring = 0; ring < totalRings; ring++) {
            for (int lon = 0; lon < segments; lon++) {
                int current = ring * vertsPerRow + lon;
                int next = current + vertsPerRow;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    }
    
    static void generateCylinder(const BodyPartDef& def,
                                std::vector<Vertex>& verts,
                                std::vector<uint32_t>& indices) {
        int segments = def.segments;
        float radius = 0.5f;
        float halfHeight = 0.5f;
        
        // Side vertices
        for (int row = 0; row <= 1; row++) {
            float y = halfHeight - row * 2 * halfHeight;
            
            for (int seg = 0; seg <= segments; seg++) {
                float angle = seg * 2.0f * 3.14159f / segments;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                
                Vertex v;
                v.position[0] = cosA * radius;
                v.position[1] = y;
                v.position[2] = sinA * radius;
                
                v.normal[0] = cosA;
                v.normal[1] = 0;
                v.normal[2] = sinA;
                
                v.uv[0] = (float)seg / segments;
                v.uv[1] = (float)row;
                
                verts.push_back(v);
            }
        }
        
        // Top cap
        int topCenterIdx = verts.size();
        Vertex topCenter;
        topCenter.position[0] = 0;
        topCenter.position[1] = halfHeight;
        topCenter.position[2] = 0;
        topCenter.normal[0] = 0;
        topCenter.normal[1] = 1;
        topCenter.normal[2] = 0;
        topCenter.uv[0] = 0.5f;
        topCenter.uv[1] = 0.5f;
        verts.push_back(topCenter);
        
        for (int seg = 0; seg <= segments; seg++) {
            float angle = seg * 2.0f * 3.14159f / segments;
            Vertex v;
            v.position[0] = std::cos(angle) * radius;
            v.position[1] = halfHeight;
            v.position[2] = std::sin(angle) * radius;
            v.normal[0] = 0;
            v.normal[1] = 1;
            v.normal[2] = 0;
            v.uv[0] = std::cos(angle) * 0.5f + 0.5f;
            v.uv[1] = std::sin(angle) * 0.5f + 0.5f;
            verts.push_back(v);
        }
        
        // Bottom cap
        int bottomCenterIdx = verts.size();
        Vertex bottomCenter;
        bottomCenter.position[0] = 0;
        bottomCenter.position[1] = -halfHeight;
        bottomCenter.position[2] = 0;
        bottomCenter.normal[0] = 0;
        bottomCenter.normal[1] = -1;
        bottomCenter.normal[2] = 0;
        bottomCenter.uv[0] = 0.5f;
        bottomCenter.uv[1] = 0.5f;
        verts.push_back(bottomCenter);
        
        for (int seg = 0; seg <= segments; seg++) {
            float angle = seg * 2.0f * 3.14159f / segments;
            Vertex v;
            v.position[0] = std::cos(angle) * radius;
            v.position[1] = -halfHeight;
            v.position[2] = std::sin(angle) * radius;
            v.normal[0] = 0;
            v.normal[1] = -1;
            v.normal[2] = 0;
            v.uv[0] = std::cos(angle) * 0.5f + 0.5f;
            v.uv[1] = std::sin(angle) * 0.5f + 0.5f;
            verts.push_back(v);
        }
        
        // Side indices
        int vertsPerRow = segments + 1;
        for (int seg = 0; seg < segments; seg++) {
            int i0 = seg;
            int i1 = seg + 1;
            int i2 = seg + vertsPerRow;
            int i3 = seg + 1 + vertsPerRow;
            
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
        
        // Top cap indices
        for (int seg = 0; seg < segments; seg++) {
            indices.push_back(topCenterIdx);
            indices.push_back(topCenterIdx + 1 + seg);
            indices.push_back(topCenterIdx + 1 + seg + 1);
        }
        
        // Bottom cap indices
        for (int seg = 0; seg < segments; seg++) {
            indices.push_back(bottomCenterIdx);
            indices.push_back(bottomCenterIdx + 1 + seg + 1);
            indices.push_back(bottomCenterIdx + 1 + seg);
        }
    }
    
    static void generateCone(const BodyPartDef& def,
                            std::vector<Vertex>& verts,
                            std::vector<uint32_t>& indices) {
        int segments = def.segments;
        float radius = 0.5f;
        float height = 1.0f;
        
        // Tip
        int tipIdx = verts.size();
        Vertex tip;
        tip.position[0] = 0;
        tip.position[1] = height * 0.5f;
        tip.position[2] = 0;
        tip.normal[0] = 0;
        tip.normal[1] = 1;
        tip.normal[2] = 0;
        tip.uv[0] = 0.5f;
        tip.uv[1] = 0;
        verts.push_back(tip);
        
        // Base ring
        for (int seg = 0; seg <= segments; seg++) {
            float angle = seg * 2.0f * 3.14159f / segments;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            
            Vertex v;
            v.position[0] = cosA * radius;
            v.position[1] = -height * 0.5f;
            v.position[2] = sinA * radius;
            
            // Normal points outward and up
            float ny = radius / height;
            float nlen = std::sqrt(1.0f + ny * ny);
            v.normal[0] = cosA / nlen;
            v.normal[1] = ny / nlen;
            v.normal[2] = sinA / nlen;
            
            v.uv[0] = (float)seg / segments;
            v.uv[1] = 1;
            
            verts.push_back(v);
        }
        
        // Base center
        int baseCenterIdx = verts.size();
        Vertex baseCenter;
        baseCenter.position[0] = 0;
        baseCenter.position[1] = -height * 0.5f;
        baseCenter.position[2] = 0;
        baseCenter.normal[0] = 0;
        baseCenter.normal[1] = -1;
        baseCenter.normal[2] = 0;
        baseCenter.uv[0] = 0.5f;
        baseCenter.uv[1] = 0.5f;
        verts.push_back(baseCenter);
        
        // Base ring (bottom facing)
        for (int seg = 0; seg <= segments; seg++) {
            float angle = seg * 2.0f * 3.14159f / segments;
            Vertex v;
            v.position[0] = std::cos(angle) * radius;
            v.position[1] = -height * 0.5f;
            v.position[2] = std::sin(angle) * radius;
            v.normal[0] = 0;
            v.normal[1] = -1;
            v.normal[2] = 0;
            v.uv[0] = std::cos(angle) * 0.5f + 0.5f;
            v.uv[1] = std::sin(angle) * 0.5f + 0.5f;
            verts.push_back(v);
        }
        
        // Side indices
        for (int seg = 0; seg < segments; seg++) {
            indices.push_back(tipIdx);
            indices.push_back(tipIdx + 1 + seg + 1);
            indices.push_back(tipIdx + 1 + seg);
        }
        
        // Base indices
        for (int seg = 0; seg < segments; seg++) {
            indices.push_back(baseCenterIdx);
            indices.push_back(baseCenterIdx + 1 + seg);
            indices.push_back(baseCenterIdx + 1 + seg + 1);
        }
    }
    
    static void generateBox(const BodyPartDef& def,
                           std::vector<Vertex>& verts,
                           std::vector<uint32_t>& indices) {
        float h = 0.5f;
        
        // Face normals and positions
        struct Face {
            Vec3 normal;
            Vec3 tangent;
            Vec3 corners[4];
        };
        
        Face faces[6] = {
            // Front (+Z)
            {{0,0,1}, {1,0,0}, {{-h,-h,h}, {h,-h,h}, {h,h,h}, {-h,h,h}}},
            // Back (-Z)
            {{0,0,-1}, {-1,0,0}, {{h,-h,-h}, {-h,-h,-h}, {-h,h,-h}, {h,h,-h}}},
            // Right (+X)
            {{1,0,0}, {0,0,1}, {{h,-h,h}, {h,-h,-h}, {h,h,-h}, {h,h,h}}},
            // Left (-X)
            {{-1,0,0}, {0,0,-1}, {{-h,-h,-h}, {-h,-h,h}, {-h,h,h}, {-h,h,-h}}},
            // Top (+Y)
            {{0,1,0}, {1,0,0}, {{-h,h,h}, {h,h,h}, {h,h,-h}, {-h,h,-h}}},
            // Bottom (-Y)
            {{0,-1,0}, {1,0,0}, {{-h,-h,-h}, {h,-h,-h}, {h,-h,h}, {-h,-h,h}}}
        };
        
        Vec2 uvs[4] = {{0,0}, {1,0}, {1,1}, {0,1}};
        
        for (int f = 0; f < 6; f++) {
            uint32_t baseIdx = verts.size();
            
            for (int c = 0; c < 4; c++) {
                Vertex v;
                v.position[0] = faces[f].corners[c].x;
                v.position[1] = faces[f].corners[c].y;
                v.position[2] = faces[f].corners[c].z;
                v.normal[0] = faces[f].normal.x;
                v.normal[1] = faces[f].normal.y;
                v.normal[2] = faces[f].normal.z;
                v.uv[0] = uvs[c].x;
                v.uv[1] = uvs[c].y;
                v.tangent[0] = faces[f].tangent.x;
                v.tangent[1] = faces[f].tangent.y;
                v.tangent[2] = faces[f].tangent.z;
                v.tangent[3] = 1;
                verts.push_back(v);
            }
            
            // Two triangles per face
            indices.push_back(baseIdx);
            indices.push_back(baseIdx + 1);
            indices.push_back(baseIdx + 2);
            
            indices.push_back(baseIdx);
            indices.push_back(baseIdx + 2);
            indices.push_back(baseIdx + 3);
        }
    }
    
    static void applyTransform(std::vector<Vertex>& verts,
                              const Vec3& offset,
                              const Quat& rotation,
                              const Vec3& scale) {
        Mat4 rotMat = Mat4::fromQuat(rotation);
        
        for (auto& v : verts) {
            // Scale
            v.position[0] *= scale.x;
            v.position[1] *= scale.y;
            v.position[2] *= scale.z;
            
            // Rotate
            Vec3 pos(v.position[0], v.position[1], v.position[2]);
            Vec3 nor(v.normal[0], v.normal[1], v.normal[2]);
            
            pos = rotMat.transformDirection(pos);
            nor = rotMat.transformDirection(nor);
            
            v.position[0] = pos.x + offset.x;
            v.position[1] = pos.y + offset.y;
            v.position[2] = pos.z + offset.z;
            
            v.normal[0] = nor.x;
            v.normal[1] = nor.y;
            v.normal[2] = nor.z;
        }
    }
};

// ============================================================================
// Body Part Instance
// ============================================================================

struct BodyPartInstance {
    std::string id;
    BodyPartDef definition;
    
    // Generated mesh data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vertexStartIndex = 0;  // Index in combined mesh
    
    // Current transform (can be animated)
    Vec3 position;
    Quat rotation;
    Vec3 scale{1, 1, 1};
    
    // Associated bone index
    int boneIndex = -1;
    
    bool isGenerated = false;
};

// ============================================================================
// Body Part Assembly
// ============================================================================

class BodyPartAssembly {
public:
    BodyPartAssembly() = default;
    
    // Add a part definition
    void addPart(const BodyPartDef& def) {
        BodyPartInstance instance;
        instance.id = def.id;
        instance.definition = def;
        instance.position = def.offset;
        instance.rotation = def.rotation;
        parts_[def.id] = instance;
        partOrder_.push_back(def.id);
    }
    
    // Get a part
    BodyPartInstance* getPart(const std::string& id) {
        auto it = parts_.find(id);
        return it != parts_.end() ? &it->second : nullptr;
    }
    
    const BodyPartInstance* getPart(const std::string& id) const {
        auto it = parts_.find(id);
        return it != parts_.end() ? &it->second : nullptr;
    }
    
    // Generate all meshes
    void generateAll() {
        for (auto& id : partOrder_) {
            auto& part = parts_[id];
            if (!part.isGenerated) {
                ProceduralPartGenerator::generatePartMesh(
                    part.definition,
                    part.vertices,
                    part.indices
                );
                part.isGenerated = true;
            }
        }
    }
    
    // Combine all parts into a single mesh
    Mesh combineMesh() const {
        Mesh combined;
        
        for (const auto& id : partOrder_) {
            const auto& part = parts_.at(id);
            if (!part.isGenerated) continue;
            
            uint32_t baseIdx = combined.vertices.size();
            
            // Add vertices
            for (const auto& v : part.vertices) {
                combined.vertices.push_back(v);
            }
            
            // Add indices with offset
            for (uint32_t idx : part.indices) {
                combined.indices.push_back(baseIdx + idx);
            }
        }
        
        return combined;
    }
    
    // Create skeleton from parts
    Skeleton createSkeleton() const {
        Skeleton skeleton;
        
        // Add root bone
        skeleton.addBone("root", -1);
        
        // Add bones for each part that requests one
        for (const auto& id : partOrder_) {
            const auto& part = parts_.at(id);
            if (!part.definition.createBone) continue;
            
            std::string boneName = part.definition.boneName;
            if (boneName.empty()) {
                boneName = part.id + "_bone";
            }
            
            // Find parent bone
            int parentIdx = 0;  // Default to root
            if (!part.definition.parentPartId.empty()) {
                const auto* parent = getPart(part.definition.parentPartId);
                if (parent && parent->boneIndex >= 0) {
                    parentIdx = parent->boneIndex;
                }
            }
            
            int boneIdx = skeleton.addBone(boneName, parentIdx);
            skeleton.setBoneLocalTransform(boneIdx, 
                                          part.position,
                                          part.rotation,
                                          part.scale);
        }
        
        return skeleton;
    }
    
    // Get part count
    size_t getPartCount() const { return parts_.size(); }
    
    // Get all part IDs
    const std::vector<std::string>& getPartOrder() const { return partOrder_; }
    
    // Clear all parts
    void clear() {
        parts_.clear();
        partOrder_.clear();
    }
    
private:
    std::unordered_map<std::string, BodyPartInstance> parts_;
    std::vector<std::string> partOrder_;  // Maintains insertion order
};

}  // namespace luma
