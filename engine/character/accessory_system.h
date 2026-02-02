// Accessory System - Glasses, hats, earrings, necklaces, etc.
// Attach accessories to character bone attachment points
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include "engine/animation/skeleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace luma {

// ============================================================================
// Accessory Types
// ============================================================================

enum class AccessoryType {
    // Head
    Hat,            // 帽子
    Glasses,        // 眼镜
    Sunglasses,     // 墨镜
    Mask,           // 面具
    Headband,       // 发带
    Hairpin,        // 发簪/发卡
    Crown,          // 皇冠/头冠
    Helmet,         // 头盔
    
    // Ears
    Earring,        // 耳环
    Earphone,       // 耳机
    
    // Face
    Beard,          // 胡子
    Makeup,         // 妆容 (贴花)
    FacePaint,      // 脸部彩绘
    
    // Neck
    Necklace,       // 项链
    Scarf,          // 围巾
    Tie,            // 领带
    Bowtie,         // 蝴蝶结
    Choker,         // 颈链
    
    // Arms
    Watch,          // 手表
    Bracelet,       // 手镯
    Gloves,         // 手套
    
    // Hands
    Ring,           // 戒指
    
    // Back
    Wings,          // 翅膀
    Cape,           // 披风
    Backpack,       // 背包
    Weapon,         // 武器（背负）
    
    // Waist
    Belt,           // 腰带
    Pouch,          // 腰包
    
    // Other
    Prop,           // 手持道具
    Custom          // 自定义
};

inline std::string accessoryTypeToString(AccessoryType type) {
    switch (type) {
        case AccessoryType::Hat: return "Hat";
        case AccessoryType::Glasses: return "Glasses";
        case AccessoryType::Sunglasses: return "Sunglasses";
        case AccessoryType::Mask: return "Mask";
        case AccessoryType::Headband: return "Headband";
        case AccessoryType::Hairpin: return "Hairpin";
        case AccessoryType::Crown: return "Crown";
        case AccessoryType::Helmet: return "Helmet";
        case AccessoryType::Earring: return "Earring";
        case AccessoryType::Earphone: return "Earphone";
        case AccessoryType::Beard: return "Beard";
        case AccessoryType::Makeup: return "Makeup";
        case AccessoryType::FacePaint: return "FacePaint";
        case AccessoryType::Necklace: return "Necklace";
        case AccessoryType::Scarf: return "Scarf";
        case AccessoryType::Tie: return "Tie";
        case AccessoryType::Bowtie: return "Bowtie";
        case AccessoryType::Choker: return "Choker";
        case AccessoryType::Watch: return "Watch";
        case AccessoryType::Bracelet: return "Bracelet";
        case AccessoryType::Gloves: return "Gloves";
        case AccessoryType::Ring: return "Ring";
        case AccessoryType::Wings: return "Wings";
        case AccessoryType::Cape: return "Cape";
        case AccessoryType::Backpack: return "Backpack";
        case AccessoryType::Weapon: return "Weapon";
        case AccessoryType::Belt: return "Belt";
        case AccessoryType::Pouch: return "Pouch";
        case AccessoryType::Prop: return "Prop";
        case AccessoryType::Custom: return "Custom";
        default: return "Unknown";
    }
}

inline std::string accessoryTypeToDisplayName(AccessoryType type) {
    switch (type) {
        case AccessoryType::Hat: return "帽子 Hat";
        case AccessoryType::Glasses: return "眼镜 Glasses";
        case AccessoryType::Sunglasses: return "墨镜 Sunglasses";
        case AccessoryType::Mask: return "面具 Mask";
        case AccessoryType::Headband: return "发带 Headband";
        case AccessoryType::Hairpin: return "发簪 Hairpin";
        case AccessoryType::Crown: return "头冠 Crown";
        case AccessoryType::Helmet: return "头盔 Helmet";
        case AccessoryType::Earring: return "耳环 Earring";
        case AccessoryType::Earphone: return "耳机 Earphone";
        case AccessoryType::Beard: return "胡子 Beard";
        case AccessoryType::Makeup: return "妆容 Makeup";
        case AccessoryType::FacePaint: return "彩绘 FacePaint";
        case AccessoryType::Necklace: return "项链 Necklace";
        case AccessoryType::Scarf: return "围巾 Scarf";
        case AccessoryType::Tie: return "领带 Tie";
        case AccessoryType::Bowtie: return "蝴蝶结 Bowtie";
        case AccessoryType::Choker: return "颈链 Choker";
        case AccessoryType::Watch: return "手表 Watch";
        case AccessoryType::Bracelet: return "手镯 Bracelet";
        case AccessoryType::Gloves: return "手套 Gloves";
        case AccessoryType::Ring: return "戒指 Ring";
        case AccessoryType::Wings: return "翅膀 Wings";
        case AccessoryType::Cape: return "披风 Cape";
        case AccessoryType::Backpack: return "背包 Backpack";
        case AccessoryType::Weapon: return "武器 Weapon";
        case AccessoryType::Belt: return "腰带 Belt";
        case AccessoryType::Pouch: return "腰包 Pouch";
        case AccessoryType::Prop: return "道具 Prop";
        case AccessoryType::Custom: return "自定义 Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Attachment Points (Standard bone attachment locations)
// ============================================================================

enum class AttachmentPoint {
    Head,
    HeadTop,
    HeadFront,
    LeftEar,
    RightEar,
    Nose,
    Neck,
    Chest,
    Spine,
    LeftShoulder,
    RightShoulder,
    LeftUpperArm,
    RightUpperArm,
    LeftLowerArm,
    RightLowerArm,
    LeftHand,
    RightHand,
    LeftFingerIndex,
    LeftFingerMiddle,
    LeftFingerRing,
    RightFingerIndex,
    RightFingerMiddle,
    RightFingerRing,
    Hips,
    LeftUpperLeg,
    RightUpperLeg,
    LeftFoot,
    RightFoot,
    Back,
    Custom
};

inline std::string attachmentPointToString(AttachmentPoint point) {
    switch (point) {
        case AttachmentPoint::Head: return "Head";
        case AttachmentPoint::HeadTop: return "HeadTop";
        case AttachmentPoint::HeadFront: return "HeadFront";
        case AttachmentPoint::LeftEar: return "LeftEar";
        case AttachmentPoint::RightEar: return "RightEar";
        case AttachmentPoint::Nose: return "Nose";
        case AttachmentPoint::Neck: return "Neck";
        case AttachmentPoint::Chest: return "Chest";
        case AttachmentPoint::Spine: return "Spine";
        case AttachmentPoint::LeftShoulder: return "LeftShoulder";
        case AttachmentPoint::RightShoulder: return "RightShoulder";
        case AttachmentPoint::LeftUpperArm: return "LeftUpperArm";
        case AttachmentPoint::RightUpperArm: return "RightUpperArm";
        case AttachmentPoint::LeftLowerArm: return "LeftLowerArm";
        case AttachmentPoint::RightLowerArm: return "RightLowerArm";
        case AttachmentPoint::LeftHand: return "LeftHand";
        case AttachmentPoint::RightHand: return "RightHand";
        case AttachmentPoint::LeftFingerIndex: return "LeftFingerIndex";
        case AttachmentPoint::LeftFingerMiddle: return "LeftFingerMiddle";
        case AttachmentPoint::LeftFingerRing: return "LeftFingerRing";
        case AttachmentPoint::RightFingerIndex: return "RightFingerIndex";
        case AttachmentPoint::RightFingerMiddle: return "RightFingerMiddle";
        case AttachmentPoint::RightFingerRing: return "RightFingerRing";
        case AttachmentPoint::Hips: return "Hips";
        case AttachmentPoint::LeftUpperLeg: return "LeftUpperLeg";
        case AttachmentPoint::RightUpperLeg: return "RightUpperLeg";
        case AttachmentPoint::LeftFoot: return "LeftFoot";
        case AttachmentPoint::RightFoot: return "RightFoot";
        case AttachmentPoint::Back: return "Back";
        case AttachmentPoint::Custom: return "Custom";
        default: return "Unknown";
    }
}

// Get default attachment point for accessory type
inline AttachmentPoint getDefaultAttachmentPoint(AccessoryType type) {
    switch (type) {
        case AccessoryType::Hat:
        case AccessoryType::Headband:
        case AccessoryType::Crown:
        case AccessoryType::Helmet:
            return AttachmentPoint::HeadTop;
            
        case AccessoryType::Glasses:
        case AccessoryType::Sunglasses:
        case AccessoryType::Mask:
            return AttachmentPoint::HeadFront;
            
        case AccessoryType::Hairpin:
            return AttachmentPoint::Head;
            
        case AccessoryType::Earring:
        case AccessoryType::Earphone:
            return AttachmentPoint::LeftEar;  // Will need pair for right
            
        case AccessoryType::Beard:
        case AccessoryType::Makeup:
        case AccessoryType::FacePaint:
            return AttachmentPoint::HeadFront;
            
        case AccessoryType::Necklace:
        case AccessoryType::Tie:
        case AccessoryType::Bowtie:
        case AccessoryType::Choker:
        case AccessoryType::Scarf:
            return AttachmentPoint::Neck;
            
        case AccessoryType::Watch:
        case AccessoryType::Bracelet:
            return AttachmentPoint::LeftLowerArm;
            
        case AccessoryType::Gloves:
            return AttachmentPoint::LeftHand;
            
        case AccessoryType::Ring:
            return AttachmentPoint::LeftFingerRing;
            
        case AccessoryType::Wings:
        case AccessoryType::Cape:
        case AccessoryType::Backpack:
        case AccessoryType::Weapon:
            return AttachmentPoint::Back;
            
        case AccessoryType::Belt:
        case AccessoryType::Pouch:
            return AttachmentPoint::Hips;
            
        case AccessoryType::Prop:
            return AttachmentPoint::RightHand;
            
        default:
            return AttachmentPoint::Custom;
    }
}

// ============================================================================
// Accessory Asset
// ============================================================================

struct AccessoryAsset {
    std::string id;
    std::string name;
    std::string nameCN;
    std::string description;
    AccessoryType type = AccessoryType::Custom;
    
    // Mesh
    std::string meshPath;
    Mesh mesh;
    bool meshLoaded = false;
    
    // Textures
    std::string diffuseTexturePath;
    std::string normalTexturePath;
    
    // Attachment
    AttachmentPoint defaultAttachment = AttachmentPoint::Custom;
    Vec3 positionOffset{0, 0, 0};
    Vec3 rotationOffset{0, 0, 0};  // Euler angles in degrees
    Vec3 scaleOffset{1, 1, 1};
    
    // Paired accessories (e.g., earrings need left and right)
    bool isPaired = false;
    AttachmentPoint pairedAttachment = AttachmentPoint::Custom;
    
    // Material
    Vec3 baseColor{1, 1, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    bool allowColorCustomization = true;
    
    // Tags for filtering
    std::vector<std::string> tags;
    std::vector<std::string> compatibleStyles;  // "realistic", "anime", "fantasy", etc.
    
    // Thumbnail
    std::string thumbnailPath;
};

// ============================================================================
// Equipped Accessory Instance
// ============================================================================

struct EquippedAccessory {
    std::string assetId;
    const AccessoryAsset* asset = nullptr;
    
    // Instance transforms (can be adjusted per character)
    Vec3 positionOffset{0, 0, 0};
    Vec3 rotationOffset{0, 0, 0};
    float scale = 1.0f;
    
    // Custom color
    Vec3 color{1, 1, 1};
    bool useCustomColor = false;
    
    // Attachment
    AttachmentPoint attachment = AttachmentPoint::Custom;
    std::string customBoneName;  // If attachment is Custom
    
    // Visibility
    bool visible = true;
    
    // Computed world transform (updated each frame)
    Mat4 worldTransform;
};

// ============================================================================
// Built-in Accessory Generator
// ============================================================================

class ProceduralAccessoryGenerator {
public:
    // Generate simple glasses mesh
    static Mesh generateGlasses(float width = 0.14f, float height = 0.04f) {
        Mesh mesh;
        mesh.name = "Glasses";
        
        // Frame around eyes
        float frameThickness = 0.003f;
        float lensWidth = 0.045f;
        float lensHeight = 0.035f;
        float bridgeWidth = 0.015f;
        float templeLength = 0.1f;
        
        // Left lens frame
        addRoundedRect(mesh, Vec3(-width/4, 0, 0), lensWidth, lensHeight, frameThickness);
        
        // Right lens frame
        addRoundedRect(mesh, Vec3(width/4, 0, 0), lensWidth, lensHeight, frameThickness);
        
        // Bridge
        addBox(mesh, Vec3(0, 0, 0), bridgeWidth, frameThickness, frameThickness);
        
        // Left temple
        addBox(mesh, Vec3(-width/2 - templeLength/2, 0, -0.02f), templeLength, frameThickness, frameThickness);
        
        // Right temple
        addBox(mesh, Vec3(width/2 + templeLength/2, 0, -0.02f), templeLength, frameThickness, frameThickness);
        
        mesh.baseColor[0] = 0.1f;
        mesh.baseColor[1] = 0.1f;
        mesh.baseColor[2] = 0.1f;
        mesh.metallic = 0.8f;
        mesh.roughness = 0.2f;
        
        return mesh;
    }
    
    // Generate simple hat mesh
    static Mesh generateHat(float radius = 0.12f, float height = 0.1f) {
        Mesh mesh;
        mesh.name = "Hat";
        
        int segments = 24;
        float brimRadius = radius * 1.3f;
        float brimHeight = 0.01f;
        
        // Crown (cylinder)
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * 6.28318f;
            float angle2 = (float)(i + 1) / segments * 6.28318f;
            
            float x1 = std::cos(angle1) * radius;
            float z1 = std::sin(angle1) * radius;
            float x2 = std::cos(angle2) * radius;
            float z2 = std::sin(angle2) * radius;
            
            // Side
            addQuad(mesh,
                Vec3(x1, 0, z1), Vec3(x2, 0, z2),
                Vec3(x2, height, z2), Vec3(x1, height, z1));
        }
        
        // Top cap
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * 6.28318f;
            float angle2 = (float)(i + 1) / segments * 6.28318f;
            
            Vertex v0, v1, v2;
            v0.position[0] = 0; v0.position[1] = height; v0.position[2] = 0;
            v1.position[0] = std::cos(angle1) * radius; v1.position[1] = height; v1.position[2] = std::sin(angle1) * radius;
            v2.position[0] = std::cos(angle2) * radius; v2.position[1] = height; v2.position[2] = std::sin(angle2) * radius;
            v0.normal[1] = v1.normal[1] = v2.normal[1] = 1;
            
            uint32_t base = mesh.vertices.size();
            mesh.vertices.push_back(v0);
            mesh.vertices.push_back(v1);
            mesh.vertices.push_back(v2);
            mesh.indices.push_back(base);
            mesh.indices.push_back(base + 1);
            mesh.indices.push_back(base + 2);
        }
        
        // Brim (ring)
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * 6.28318f;
            float angle2 = (float)(i + 1) / segments * 6.28318f;
            
            float x1i = std::cos(angle1) * radius;
            float z1i = std::sin(angle1) * radius;
            float x1o = std::cos(angle1) * brimRadius;
            float z1o = std::sin(angle1) * brimRadius;
            float x2i = std::cos(angle2) * radius;
            float z2i = std::sin(angle2) * radius;
            float x2o = std::cos(angle2) * brimRadius;
            float z2o = std::sin(angle2) * brimRadius;
            
            // Top of brim
            addQuad(mesh,
                Vec3(x1i, brimHeight, z1i), Vec3(x2i, brimHeight, z2i),
                Vec3(x2o, brimHeight, z2o), Vec3(x1o, brimHeight, z1o));
            
            // Bottom of brim
            addQuad(mesh,
                Vec3(x1o, 0, z1o), Vec3(x2o, 0, z2o),
                Vec3(x2i, 0, z2i), Vec3(x1i, 0, z1i));
        }
        
        mesh.baseColor[0] = 0.2f;
        mesh.baseColor[1] = 0.15f;
        mesh.baseColor[2] = 0.1f;
        
        return mesh;
    }
    
    // Generate earring
    static Mesh generateEarring(float size = 0.015f) {
        Mesh mesh;
        mesh.name = "Earring";
        
        // Simple hoop or stud
        int segments = 12;
        float radius = size;
        
        // Ring
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * 6.28318f;
            float angle2 = (float)(i + 1) / segments * 6.28318f;
            
            float x1 = std::cos(angle1) * radius;
            float y1 = std::sin(angle1) * radius;
            float x2 = std::cos(angle2) * radius;
            float y2 = std::sin(angle2) * radius;
            
            float tubeRadius = size * 0.15f;
            
            // Create tube segment
            for (int j = 0; j < 6; j++) {
                float ta1 = (float)j / 6 * 6.28318f;
                float ta2 = (float)(j + 1) / 6 * 6.28318f;
                
                Vec3 center1(x1, y1, 0);
                Vec3 center2(x2, y2, 0);
                
                // This is simplified - real implementation would create proper tube
            }
        }
        
        // Add a simple sphere for now
        addSphere(mesh, Vec3(0, -size, 0), size * 0.5f, 8, 6);
        
        mesh.baseColor[0] = 0.9f;
        mesh.baseColor[1] = 0.8f;
        mesh.baseColor[2] = 0.4f;
        mesh.metallic = 0.9f;
        mesh.roughness = 0.1f;
        
        return mesh;
    }
    
    // Generate necklace
    static Mesh generateNecklace(float radius = 0.08f) {
        Mesh mesh;
        mesh.name = "Necklace";
        
        int segments = 32;
        float chainRadius = 0.002f;
        
        // Chain as a series of small spheres/cylinders
        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 3.14159f + 3.14159f/2;  // Semi-circle
            float x = std::cos(angle) * radius;
            float y = std::sin(angle) * radius * 0.3f;  // Flatten
            
            addSphere(mesh, Vec3(x, y, 0), chainRadius, 4, 3);
        }
        
        // Pendant
        addSphere(mesh, Vec3(0, -radius * 0.3f - 0.02f, 0), 0.01f, 8, 6);
        
        mesh.baseColor[0] = 0.95f;
        mesh.baseColor[1] = 0.85f;
        mesh.baseColor[2] = 0.5f;
        mesh.metallic = 0.95f;
        mesh.roughness = 0.1f;
        
        return mesh;
    }
    
private:
    static void addBox(Mesh& mesh, const Vec3& center, float w, float h, float d) {
        float hw = w / 2, hh = h / 2, hd = d / 2;
        
        Vec3 corners[8] = {
            {center.x - hw, center.y - hh, center.z - hd},
            {center.x + hw, center.y - hh, center.z - hd},
            {center.x + hw, center.y + hh, center.z - hd},
            {center.x - hw, center.y + hh, center.z - hd},
            {center.x - hw, center.y - hh, center.z + hd},
            {center.x + hw, center.y - hh, center.z + hd},
            {center.x + hw, center.y + hh, center.z + hd},
            {center.x - hw, center.y + hh, center.z + hd}
        };
        
        // Front
        addQuad(mesh, corners[0], corners[1], corners[2], corners[3]);
        // Back
        addQuad(mesh, corners[5], corners[4], corners[7], corners[6]);
        // Top
        addQuad(mesh, corners[3], corners[2], corners[6], corners[7]);
        // Bottom
        addQuad(mesh, corners[4], corners[5], corners[1], corners[0]);
        // Left
        addQuad(mesh, corners[4], corners[0], corners[3], corners[7]);
        // Right
        addQuad(mesh, corners[1], corners[5], corners[6], corners[2]);
    }
    
    static void addQuad(Mesh& mesh, const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3) {
        Vec3 normal = ((v1 - v0).cross(v3 - v0)).normalized();
        
        Vertex verts[4];
        Vec3 positions[4] = {v0, v1, v2, v3};
        
        for (int i = 0; i < 4; i++) {
            verts[i].position[0] = positions[i].x;
            verts[i].position[1] = positions[i].y;
            verts[i].position[2] = positions[i].z;
            verts[i].normal[0] = normal.x;
            verts[i].normal[1] = normal.y;
            verts[i].normal[2] = normal.z;
        }
        
        verts[0].uv[0] = 0; verts[0].uv[1] = 0;
        verts[1].uv[0] = 1; verts[1].uv[1] = 0;
        verts[2].uv[0] = 1; verts[2].uv[1] = 1;
        verts[3].uv[0] = 0; verts[3].uv[1] = 1;
        
        uint32_t base = mesh.vertices.size();
        for (int i = 0; i < 4; i++) mesh.vertices.push_back(verts[i]);
        
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }
    
    static void addRoundedRect(Mesh& mesh, const Vec3& center, float w, float h, float thickness) {
        // Simplified: just a box for now
        addBox(mesh, center, w, h, thickness);
    }
    
    static void addSphere(Mesh& mesh, const Vec3& center, float radius, int rings, int segments) {
        for (int i = 0; i < rings; i++) {
            float phi1 = 3.14159f * i / rings;
            float phi2 = 3.14159f * (i + 1) / rings;
            
            for (int j = 0; j < segments; j++) {
                float theta1 = 2 * 3.14159f * j / segments;
                float theta2 = 2 * 3.14159f * (j + 1) / segments;
                
                Vec3 p1(
                    center.x + radius * std::sin(phi1) * std::cos(theta1),
                    center.y + radius * std::cos(phi1),
                    center.z + radius * std::sin(phi1) * std::sin(theta1)
                );
                Vec3 p2(
                    center.x + radius * std::sin(phi1) * std::cos(theta2),
                    center.y + radius * std::cos(phi1),
                    center.z + radius * std::sin(phi1) * std::sin(theta2)
                );
                Vec3 p3(
                    center.x + radius * std::sin(phi2) * std::cos(theta2),
                    center.y + radius * std::cos(phi2),
                    center.z + radius * std::sin(phi2) * std::sin(theta2)
                );
                Vec3 p4(
                    center.x + radius * std::sin(phi2) * std::cos(theta1),
                    center.y + radius * std::cos(phi2),
                    center.z + radius * std::sin(phi2) * std::sin(theta1)
                );
                
                addQuad(mesh, p1, p2, p3, p4);
            }
        }
    }
};

// ============================================================================
// Accessory Library
// ============================================================================

class AccessoryLibrary {
public:
    static AccessoryLibrary& getInstance() {
        static AccessoryLibrary instance;
        return instance;
    }
    
    void initialize() {
        if (initialized_) return;
        
        // Add built-in accessories
        
        // === Glasses ===
        {
            AccessoryAsset asset;
            asset.id = "glasses_round";
            asset.name = "Round Glasses";
            asset.nameCN = "圆框眼镜";
            asset.type = AccessoryType::Glasses;
            asset.defaultAttachment = AttachmentPoint::HeadFront;
            asset.positionOffset = Vec3(0, 0.02f, 0.08f);
            asset.mesh = ProceduralAccessoryGenerator::generateGlasses(0.12f, 0.04f);
            asset.meshLoaded = true;
            asset.tags = {"glasses", "round", "classic"};
            asset.compatibleStyles = {"realistic", "anime", "cartoon"};
            addAsset(asset);
        }
        
        {
            AccessoryAsset asset;
            asset.id = "sunglasses_aviator";
            asset.name = "Aviator Sunglasses";
            asset.nameCN = "飞行员墨镜";
            asset.type = AccessoryType::Sunglasses;
            asset.defaultAttachment = AttachmentPoint::HeadFront;
            asset.positionOffset = Vec3(0, 0.02f, 0.08f);
            asset.mesh = ProceduralAccessoryGenerator::generateGlasses(0.14f, 0.045f);
            asset.meshLoaded = true;
            asset.baseColor = Vec3(0.05f, 0.05f, 0.05f);
            asset.metallic = 0.9f;
            asset.tags = {"sunglasses", "aviator", "cool"};
            addAsset(asset);
        }
        
        // === Hats ===
        {
            AccessoryAsset asset;
            asset.id = "hat_fedora";
            asset.name = "Fedora Hat";
            asset.nameCN = "礼帽";
            asset.type = AccessoryType::Hat;
            asset.defaultAttachment = AttachmentPoint::HeadTop;
            asset.positionOffset = Vec3(0, 0.05f, 0);
            asset.mesh = ProceduralAccessoryGenerator::generateHat(0.1f, 0.08f);
            asset.meshLoaded = true;
            asset.tags = {"hat", "fedora", "classic"};
            addAsset(asset);
        }
        
        {
            AccessoryAsset asset;
            asset.id = "crown_gold";
            asset.name = "Golden Crown";
            asset.nameCN = "金冠";
            asset.type = AccessoryType::Crown;
            asset.defaultAttachment = AttachmentPoint::HeadTop;
            asset.positionOffset = Vec3(0, 0.05f, 0);
            asset.mesh = ProceduralAccessoryGenerator::generateHat(0.08f, 0.06f);
            asset.mesh.baseColor[0] = 0.95f;
            asset.mesh.baseColor[1] = 0.8f;
            asset.mesh.baseColor[2] = 0.3f;
            asset.mesh.metallic = 0.95f;
            asset.meshLoaded = true;
            asset.tags = {"crown", "royal", "gold"};
            asset.compatibleStyles = {"fantasy", "gufeng"};
            addAsset(asset);
        }
        
        // === Earrings ===
        {
            AccessoryAsset asset;
            asset.id = "earring_gold_hoop";
            asset.name = "Gold Hoop Earring";
            asset.nameCN = "金色耳环";
            asset.type = AccessoryType::Earring;
            asset.defaultAttachment = AttachmentPoint::LeftEar;
            asset.pairedAttachment = AttachmentPoint::RightEar;
            asset.isPaired = true;
            asset.positionOffset = Vec3(0, -0.02f, 0);
            asset.mesh = ProceduralAccessoryGenerator::generateEarring(0.012f);
            asset.meshLoaded = true;
            asset.tags = {"earring", "hoop", "gold"};
            addAsset(asset);
        }
        
        // === Necklaces ===
        {
            AccessoryAsset asset;
            asset.id = "necklace_gold_chain";
            asset.name = "Gold Chain Necklace";
            asset.nameCN = "金项链";
            asset.type = AccessoryType::Necklace;
            asset.defaultAttachment = AttachmentPoint::Neck;
            asset.positionOffset = Vec3(0, -0.05f, 0.02f);
            asset.mesh = ProceduralAccessoryGenerator::generateNecklace(0.08f);
            asset.meshLoaded = true;
            asset.tags = {"necklace", "chain", "gold"};
            addAsset(asset);
        }
        
        initialized_ = true;
    }
    
    const AccessoryAsset* getAsset(const std::string& id) const {
        auto it = assets_.find(id);
        return (it != assets_.end()) ? &it->second : nullptr;
    }
    
    std::vector<std::string> getAssetIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, _] : assets_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    std::vector<const AccessoryAsset*> getAssetsByType(AccessoryType type) const {
        std::vector<const AccessoryAsset*> result;
        for (const auto& [id, asset] : assets_) {
            if (asset.type == type) {
                result.push_back(&asset);
            }
        }
        return result;
    }
    
    void addAsset(const AccessoryAsset& asset) {
        assets_[asset.id] = asset;
    }
    
private:
    AccessoryLibrary() { initialize(); }
    
    std::unordered_map<std::string, AccessoryAsset> assets_;
    bool initialized_ = false;
};

// ============================================================================
// Accessory Manager - Per character
// ============================================================================

class AccessoryManager {
public:
    AccessoryManager() = default;
    
    // Equip accessory
    bool equip(const std::string& assetId) {
        auto* asset = AccessoryLibrary::getInstance().getAsset(assetId);
        if (!asset) return false;
        
        EquippedAccessory equipped;
        equipped.assetId = assetId;
        equipped.asset = asset;
        equipped.attachment = asset->defaultAttachment;
        equipped.positionOffset = asset->positionOffset;
        equipped.rotationOffset = asset->rotationOffset;
        equipped.scale = 1.0f;
        equipped.color = asset->baseColor;
        
        equipped_.push_back(equipped);
        
        // If paired, add the other side
        if (asset->isPaired) {
            EquippedAccessory paired = equipped;
            paired.attachment = asset->pairedAttachment;
            // Mirror X position
            paired.positionOffset.x = -paired.positionOffset.x;
            equipped_.push_back(paired);
        }
        
        return true;
    }
    
    // Unequip by asset ID
    bool unequip(const std::string& assetId) {
        auto it = std::remove_if(equipped_.begin(), equipped_.end(),
            [&](const EquippedAccessory& e) { return e.assetId == assetId; });
        
        if (it != equipped_.end()) {
            equipped_.erase(it, equipped_.end());
            return true;
        }
        return false;
    }
    
    // Unequip by type
    void unequipByType(AccessoryType type) {
        equipped_.erase(
            std::remove_if(equipped_.begin(), equipped_.end(),
                [&](const EquippedAccessory& e) { 
                    return e.asset && e.asset->type == type; 
                }),
            equipped_.end());
    }
    
    // Clear all
    void clearAll() {
        equipped_.clear();
    }
    
    // Update transforms based on skeleton
    void updateTransforms(const Skeleton* skeleton, const Mat4& characterTransform) {
        if (!skeleton) return;
        
        for (auto& acc : equipped_) {
            Mat4 boneTransform = getBoneTransform(skeleton, acc.attachment, acc.customBoneName);
            
            // Apply local offsets
            Mat4 localTransform = Mat4::identity();
            localTransform = localTransform * Mat4::translation(acc.positionOffset);
            localTransform = localTransform * Mat4::rotationY(acc.rotationOffset.y * 0.0174533f);
            localTransform = localTransform * Mat4::rotationX(acc.rotationOffset.x * 0.0174533f);
            localTransform = localTransform * Mat4::rotationZ(acc.rotationOffset.z * 0.0174533f);
            localTransform = localTransform * Mat4::scale(Vec3(acc.scale, acc.scale, acc.scale));
            
            acc.worldTransform = characterTransform * boneTransform * localTransform;
        }
    }
    
    // Get all equipped accessories
    const std::vector<EquippedAccessory>& getEquipped() const {
        return equipped_;
    }
    
    std::vector<EquippedAccessory>& getEquipped() {
        return equipped_;
    }
    
    // Check if accessory type is equipped
    bool hasAccessoryType(AccessoryType type) const {
        for (const auto& acc : equipped_) {
            if (acc.asset && acc.asset->type == type) {
                return true;
            }
        }
        return false;
    }
    
private:
    Mat4 getBoneTransform(const Skeleton* skeleton, AttachmentPoint point, const std::string& customBone) {
        std::string boneName;
        
        switch (point) {
            case AttachmentPoint::Head: boneName = "Head"; break;
            case AttachmentPoint::HeadTop: boneName = "Head"; break;
            case AttachmentPoint::HeadFront: boneName = "Head"; break;
            case AttachmentPoint::LeftEar: boneName = "Head"; break;
            case AttachmentPoint::RightEar: boneName = "Head"; break;
            case AttachmentPoint::Nose: boneName = "Head"; break;
            case AttachmentPoint::Neck: boneName = "Neck"; break;
            case AttachmentPoint::Chest: boneName = "Spine2"; break;
            case AttachmentPoint::Spine: boneName = "Spine"; break;
            case AttachmentPoint::LeftShoulder: boneName = "LeftShoulder"; break;
            case AttachmentPoint::RightShoulder: boneName = "RightShoulder"; break;
            case AttachmentPoint::LeftUpperArm: boneName = "LeftUpperArm"; break;
            case AttachmentPoint::RightUpperArm: boneName = "RightUpperArm"; break;
            case AttachmentPoint::LeftLowerArm: boneName = "LeftLowerArm"; break;
            case AttachmentPoint::RightLowerArm: boneName = "RightLowerArm"; break;
            case AttachmentPoint::LeftHand: boneName = "LeftHand"; break;
            case AttachmentPoint::RightHand: boneName = "RightHand"; break;
            case AttachmentPoint::Hips: boneName = "Hips"; break;
            case AttachmentPoint::Back: boneName = "Spine1"; break;
            case AttachmentPoint::Custom: boneName = customBone; break;
            default: boneName = "Hips"; break;
        }
        
        int boneIdx = skeleton->findBoneByName(boneName);
        if (boneIdx >= 0) {
            const Bone* bone = skeleton->getBone(boneIdx);
            if (bone) {
                return bone->worldMatrix;
            }
        }
        
        return Mat4::identity();
    }
    
    std::vector<EquippedAccessory> equipped_;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline AccessoryLibrary& getAccessoryLibrary() {
    return AccessoryLibrary::getInstance();
}

}  // namespace luma
