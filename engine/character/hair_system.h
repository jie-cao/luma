// LUMA Character Hair System
// Manages hair styles, colors, and rendering
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/renderer/mesh.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace luma {

// ============================================================================
// Hair Style Categories
// ============================================================================

enum class HairCategory {
    Short,      // Buzz cut, pixie, etc.
    Medium,     // Bob, shoulder length
    Long,       // Below shoulder
    Updo,       // Buns, ponytails
    Bald        // No hair
};

enum class HairGender {
    Male,
    Female,
    Unisex
};

// ============================================================================
// Hair Material Properties
// ============================================================================

struct HairMaterial {
    Vec3 baseColor{0.15f, 0.1f, 0.05f};      // Natural brown
    Vec3 highlightColor{0.3f, 0.2f, 0.1f};   // Highlight tint
    float highlightIntensity = 0.3f;
    
    float roughness = 0.5f;
    float metallic = 0.0f;
    float anisotropy = 0.8f;                  // Hair strand direction
    
    float opacity = 1.0f;                     // For transparent hair cards
    
    // Subsurface scattering for hair
    float scatterIntensity = 0.2f;
    Vec3 scatterColor{0.5f, 0.3f, 0.2f};
    
    // Presets
    static HairMaterial black() {
        HairMaterial m;
        m.baseColor = Vec3(0.02f, 0.02f, 0.02f);
        m.highlightColor = Vec3(0.1f, 0.08f, 0.05f);
        return m;
    }
    
    static HairMaterial brown() {
        HairMaterial m;
        m.baseColor = Vec3(0.15f, 0.1f, 0.05f);
        m.highlightColor = Vec3(0.3f, 0.2f, 0.1f);
        return m;
    }
    
    static HairMaterial darkBrown() {
        HairMaterial m;
        m.baseColor = Vec3(0.08f, 0.05f, 0.03f);
        m.highlightColor = Vec3(0.2f, 0.12f, 0.08f);
        return m;
    }
    
    static HairMaterial blonde() {
        HairMaterial m;
        m.baseColor = Vec3(0.75f, 0.6f, 0.4f);
        m.highlightColor = Vec3(0.9f, 0.8f, 0.6f);
        m.highlightIntensity = 0.5f;
        return m;
    }
    
    static HairMaterial platinum() {
        HairMaterial m;
        m.baseColor = Vec3(0.9f, 0.88f, 0.8f);
        m.highlightColor = Vec3(1.0f, 0.98f, 0.95f);
        m.highlightIntensity = 0.4f;
        return m;
    }
    
    static HairMaterial red() {
        HairMaterial m;
        m.baseColor = Vec3(0.5f, 0.15f, 0.08f);
        m.highlightColor = Vec3(0.7f, 0.25f, 0.1f);
        return m;
    }
    
    static HairMaterial auburn() {
        HairMaterial m;
        m.baseColor = Vec3(0.35f, 0.15f, 0.08f);
        m.highlightColor = Vec3(0.5f, 0.25f, 0.12f);
        return m;
    }
    
    static HairMaterial gray() {
        HairMaterial m;
        m.baseColor = Vec3(0.5f, 0.5f, 0.52f);
        m.highlightColor = Vec3(0.7f, 0.7f, 0.72f);
        m.highlightIntensity = 0.2f;
        return m;
    }
    
    static HairMaterial white() {
        HairMaterial m;
        m.baseColor = Vec3(0.85f, 0.85f, 0.87f);
        m.highlightColor = Vec3(1.0f, 1.0f, 1.0f);
        m.highlightIntensity = 0.3f;
        return m;
    }
    
    // Fantasy colors
    static HairMaterial blue() {
        HairMaterial m;
        m.baseColor = Vec3(0.1f, 0.2f, 0.5f);
        m.highlightColor = Vec3(0.3f, 0.5f, 0.8f);
        m.highlightIntensity = 0.4f;
        return m;
    }
    
    static HairMaterial pink() {
        HairMaterial m;
        m.baseColor = Vec3(0.7f, 0.3f, 0.5f);
        m.highlightColor = Vec3(0.9f, 0.5f, 0.7f);
        m.highlightIntensity = 0.4f;
        return m;
    }
    
    static HairMaterial purple() {
        HairMaterial m;
        m.baseColor = Vec3(0.3f, 0.1f, 0.4f);
        m.highlightColor = Vec3(0.5f, 0.3f, 0.6f);
        m.highlightIntensity = 0.4f;
        return m;
    }
    
    static HairMaterial green() {
        HairMaterial m;
        m.baseColor = Vec3(0.1f, 0.35f, 0.15f);
        m.highlightColor = Vec3(0.2f, 0.5f, 0.25f);
        m.highlightIntensity = 0.4f;
        return m;
    }
};

// ============================================================================
// Hair Style Asset
// ============================================================================

struct HairStyleAsset {
    std::string id;
    std::string name;
    std::string description;
    
    HairCategory category = HairCategory::Medium;
    HairGender gender = HairGender::Unisex;
    
    // Mesh data (can be loaded from file or procedurally generated)
    Mesh mesh;
    bool hasMesh = false;
    
    // File path for external mesh
    std::string meshPath;
    
    // Attachment points (for fitting to head)
    Vec3 attachOffset{0, 0, 0};
    Vec3 attachScale{1, 1, 1};
    float headSizeAdaptation = 0.1f;  // How much to adapt to head size
    
    // Default material
    HairMaterial defaultMaterial;
    
    // Thumbnail for UI
    std::vector<uint8_t> thumbnail;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;
    
    bool isValid() const { return !id.empty() && (hasMesh || !meshPath.empty()); }
};

// ============================================================================
// Procedural Hair Generator
// ============================================================================

class ProceduralHairGenerator {
public:
    // Generate a simple short hair mesh (buzz cut style)
    static Mesh generateBuzzCut(float headRadius = 0.1f, int segments = 16) {
        Mesh mesh;
        
        // Generate a slightly larger sphere cap for hair
        float hairRadius = headRadius * 1.02f;
        
        for (int lat = 0; lat <= segments / 2; lat++) {
            float theta = lat * 3.14159f / segments;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            
            // Only upper hemisphere
            if (cosTheta < 0.3f) continue;
            
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * 3.14159f / segments;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);
                
                Vertex v;
                v.position[0] = hairRadius * sinTheta * cosPhi;
                v.position[1] = hairRadius * cosTheta;
                v.position[2] = hairRadius * sinTheta * sinPhi;
                
                v.normal[0] = sinTheta * cosPhi;
                v.normal[1] = cosTheta;
                v.normal[2] = sinTheta * sinPhi;
                
                v.uv[0] = (float)lon / segments;
                v.uv[1] = (float)lat / (segments / 2);
                
                v.color[0] = 0.1f;
                v.color[1] = 0.08f;
                v.color[2] = 0.05f;
                
                v.tangent[0] = -sinPhi;
                v.tangent[1] = 0;
                v.tangent[2] = cosPhi;
                v.tangent[3] = 1;
                
                mesh.vertices.push_back(v);
            }
        }
        
        // Generate indices
        int vertsPerRow = segments + 1;
        int rows = 0;
        for (int lat = 0; lat <= segments / 2; lat++) {
            float theta = lat * 3.14159f / segments;
            float cosTheta = std::cos(theta);
            if (cosTheta >= 0.3f) rows++;
        }
        
        for (int lat = 0; lat < rows - 1; lat++) {
            for (int lon = 0; lon < segments; lon++) {
                int current = lat * vertsPerRow + lon;
                int next = current + vertsPerRow;
                
                mesh.indices.push_back(current);
                mesh.indices.push_back(next);
                mesh.indices.push_back(current + 1);
                
                mesh.indices.push_back(current + 1);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }
        
        return mesh;
    }
    
    // Generate medium length hair using hair cards
    static Mesh generateMediumHair(float headRadius = 0.1f, int cardCount = 30) {
        Mesh mesh;
        
        // Generate hair cards around the head
        for (int i = 0; i < cardCount; i++) {
            float angle = (float)i / cardCount * 2.0f * 3.14159f;
            float baseAngle = (float)(i % 5) / 5 * 0.5f - 0.25f;  // Variation
            
            // Card dimensions
            float cardWidth = 0.03f + ((i * 13) % 7) * 0.005f;
            float cardLength = 0.15f + ((i * 17) % 11) * 0.02f;
            
            // Card position on head
            float headAngle = 0.3f + baseAngle;  // Angle from top
            float px = headRadius * std::sin(headAngle) * std::cos(angle);
            float py = headRadius * std::cos(headAngle);
            float pz = headRadius * std::sin(headAngle) * std::sin(angle);
            
            // Direction (hanging down with some outward)
            Vec3 dir(-0.1f * std::cos(angle), -1.0f, -0.1f * std::sin(angle));
            dir = dir.normalized();
            
            // Perpendicular (for card width)
            Vec3 perp(-std::sin(angle), 0, std::cos(angle));
            
            // Create quad vertices
            int baseIdx = mesh.vertices.size();
            
            for (int v = 0; v < 4; v++) {
                Vertex vert;
                
                float u = (v & 1) ? 1.0f : 0.0f;
                float t = (v & 2) ? 1.0f : 0.0f;
                
                vert.position[0] = px + perp.x * cardWidth * (u - 0.5f) + dir.x * cardLength * t;
                vert.position[1] = py + perp.y * cardWidth * (u - 0.5f) + dir.y * cardLength * t;
                vert.position[2] = pz + perp.z * cardWidth * (u - 0.5f) + dir.z * cardLength * t;
                
                vert.normal[0] = std::cos(angle);
                vert.normal[1] = 0.1f;
                vert.normal[2] = std::sin(angle);
                
                vert.uv[0] = u;
                vert.uv[1] = t;
                
                vert.color[0] = 0.1f;
                vert.color[1] = 0.08f;
                vert.color[2] = 0.05f;
                
                vert.tangent[0] = perp.x;
                vert.tangent[1] = perp.y;
                vert.tangent[2] = perp.z;
                vert.tangent[3] = 1;
                
                mesh.vertices.push_back(vert);
            }
            
            // Two triangles per card
            mesh.indices.push_back(baseIdx);
            mesh.indices.push_back(baseIdx + 1);
            mesh.indices.push_back(baseIdx + 2);
            
            mesh.indices.push_back(baseIdx + 1);
            mesh.indices.push_back(baseIdx + 3);
            mesh.indices.push_back(baseIdx + 2);
        }
        
        return mesh;
    }
    
    // Generate ponytail
    static Mesh generatePonytail(float length = 0.3f, float radius = 0.04f, int segments = 12) {
        Mesh mesh;
        
        int lengthSegments = 10;
        
        for (int l = 0; l <= lengthSegments; l++) {
            float t = (float)l / lengthSegments;
            float y = -0.1f - t * length;
            float r = radius * (1.0f - t * 0.3f);  // Taper
            
            for (int s = 0; s <= segments; s++) {
                float angle = (float)s / segments * 2.0f * 3.14159f;
                
                Vertex v;
                v.position[0] = r * std::cos(angle);
                v.position[1] = y;
                v.position[2] = r * std::sin(angle) - 0.12f;  // Behind head
                
                v.normal[0] = std::cos(angle);
                v.normal[1] = 0;
                v.normal[2] = std::sin(angle);
                
                v.uv[0] = (float)s / segments;
                v.uv[1] = t;
                
                v.color[0] = 0.1f;
                v.color[1] = 0.08f;
                v.color[2] = 0.05f;
                
                v.tangent[0] = -std::sin(angle);
                v.tangent[1] = 0;
                v.tangent[2] = std::cos(angle);
                v.tangent[3] = 1;
                
                mesh.vertices.push_back(v);
            }
        }
        
        // Generate indices
        int vertsPerRow = segments + 1;
        for (int l = 0; l < lengthSegments; l++) {
            for (int s = 0; s < segments; s++) {
                int current = l * vertsPerRow + s;
                int next = current + vertsPerRow;
                
                mesh.indices.push_back(current);
                mesh.indices.push_back(next);
                mesh.indices.push_back(current + 1);
                
                mesh.indices.push_back(current + 1);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }
        
        return mesh;
    }
};

// ============================================================================
// Hair Style Library
// ============================================================================

class HairStyleLibrary {
public:
    static HairStyleLibrary& getInstance() {
        static HairStyleLibrary instance;
        return instance;
    }
    
    // Initialize with default procedural styles
    void initializeDefaults() {
        if (initialized_) return;
        
        // Bald
        {
            HairStyleAsset style;
            style.id = "bald";
            style.name = "Bald";
            style.description = "No hair";
            style.category = HairCategory::Bald;
            style.gender = HairGender::Unisex;
            style.hasMesh = false;  // No mesh for bald
            addStyle(style);
        }
        
        // Buzz cut
        {
            HairStyleAsset style;
            style.id = "buzz_cut";
            style.name = "Buzz Cut";
            style.description = "Very short, military style";
            style.category = HairCategory::Short;
            style.gender = HairGender::Unisex;
            style.mesh = ProceduralHairGenerator::generateBuzzCut(0.1f, 24);
            style.hasMesh = true;
            style.attachOffset = Vec3(0, 1.55f, 0);  // Head position
            style.defaultMaterial = HairMaterial::black();
            addStyle(style);
        }
        
        // Short casual
        {
            HairStyleAsset style;
            style.id = "short_casual";
            style.name = "Short Casual";
            style.description = "Short layered hair";
            style.category = HairCategory::Short;
            style.gender = HairGender::Unisex;
            style.mesh = ProceduralHairGenerator::generateMediumHair(0.1f, 40);
            style.hasMesh = true;
            style.attachOffset = Vec3(0, 1.55f, 0);
            style.defaultMaterial = HairMaterial::brown();
            addStyle(style);
        }
        
        // Medium length
        {
            HairStyleAsset style;
            style.id = "medium_length";
            style.name = "Medium Length";
            style.description = "Shoulder length hair";
            style.category = HairCategory::Medium;
            style.gender = HairGender::Unisex;
            style.mesh = ProceduralHairGenerator::generateMediumHair(0.1f, 60);
            style.hasMesh = true;
            style.attachOffset = Vec3(0, 1.55f, 0);
            style.defaultMaterial = HairMaterial::brown();
            addStyle(style);
        }
        
        // Ponytail
        {
            HairStyleAsset style;
            style.id = "ponytail";
            style.name = "Ponytail";
            style.description = "Hair tied back in ponytail";
            style.category = HairCategory::Updo;
            style.gender = HairGender::Unisex;
            
            // Combine cap + ponytail
            Mesh cap = ProceduralHairGenerator::generateBuzzCut(0.1f, 20);
            Mesh tail = ProceduralHairGenerator::generatePonytail(0.25f, 0.035f, 10);
            
            // Merge meshes
            style.mesh = cap;
            uint32_t baseIdx = style.mesh.vertices.size();
            for (const auto& v : tail.vertices) {
                style.mesh.vertices.push_back(v);
            }
            for (uint32_t idx : tail.indices) {
                style.mesh.indices.push_back(baseIdx + idx);
            }
            
            style.hasMesh = true;
            style.attachOffset = Vec3(0, 1.55f, 0);
            style.defaultMaterial = HairMaterial::darkBrown();
            addStyle(style);
        }
        
        initialized_ = true;
    }
    
    void addStyle(const HairStyleAsset& style) {
        styles_[style.id] = style;
        
        // Add to category index
        categoryIndex_[style.category].push_back(style.id);
    }
    
    const HairStyleAsset* getStyle(const std::string& id) const {
        auto it = styles_.find(id);
        return it != styles_.end() ? &it->second : nullptr;
    }
    
    std::vector<std::string> getStylesByCategory(HairCategory category) const {
        auto it = categoryIndex_.find(category);
        if (it != categoryIndex_.end()) {
            return it->second;
        }
        return {};
    }
    
    std::vector<std::string> getAllStyleIds() const {
        std::vector<std::string> ids;
        for (const auto& [id, style] : styles_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    size_t getStyleCount() const { return styles_.size(); }
    
private:
    HairStyleLibrary() = default;
    
    std::unordered_map<std::string, HairStyleAsset> styles_;
    std::unordered_map<HairCategory, std::vector<std::string>> categoryIndex_;
    bool initialized_ = false;
};

// ============================================================================
// Hair Instance (equipped on character)
// ============================================================================

struct HairInstance {
    std::string styleId;
    HairMaterial material;
    
    // Transform adjustments
    Vec3 offset{0, 0, 0};
    Vec3 scale{1, 1, 1};
    
    // Generated mesh (with applied color)
    Mesh mesh;
    bool meshGenerated = false;
    
    void applyMaterial(const HairStyleAsset& style) {
        if (!style.hasMesh) return;
        
        mesh = style.mesh;
        
        // Apply material color to vertices
        for (auto& v : mesh.vertices) {
            v.color[0] = material.baseColor.x;
            v.color[1] = material.baseColor.y;
            v.color[2] = material.baseColor.z;
        }
        
        mesh.baseColor[0] = material.baseColor.x;
        mesh.baseColor[1] = material.baseColor.y;
        mesh.baseColor[2] = material.baseColor.z;
        mesh.roughness = material.roughness;
        mesh.metallic = material.metallic;
        
        meshGenerated = true;
    }
};

// ============================================================================
// Hair Manager (per-character)
// ============================================================================

class HairManager {
public:
    HairManager() = default;
    
    // Set hair style
    bool setStyle(const std::string& styleId) {
        const HairStyleAsset* style = HairStyleLibrary::getInstance().getStyle(styleId);
        if (!style) return false;
        
        currentHair_.styleId = styleId;
        currentHair_.material = style->defaultMaterial;
        currentHair_.offset = style->attachOffset;
        currentHair_.applyMaterial(*style);
        
        return true;
    }
    
    // Set hair color
    void setColor(const Vec3& color) {
        currentHair_.material.baseColor = color;
        
        const HairStyleAsset* style = HairStyleLibrary::getInstance().getStyle(currentHair_.styleId);
        if (style) {
            currentHair_.applyMaterial(*style);
        }
    }
    
    // Set hair material preset
    void setMaterialPreset(const std::string& presetName) {
        if (presetName == "black") currentHair_.material = HairMaterial::black();
        else if (presetName == "brown") currentHair_.material = HairMaterial::brown();
        else if (presetName == "dark_brown") currentHair_.material = HairMaterial::darkBrown();
        else if (presetName == "blonde") currentHair_.material = HairMaterial::blonde();
        else if (presetName == "platinum") currentHair_.material = HairMaterial::platinum();
        else if (presetName == "red") currentHair_.material = HairMaterial::red();
        else if (presetName == "auburn") currentHair_.material = HairMaterial::auburn();
        else if (presetName == "gray") currentHair_.material = HairMaterial::gray();
        else if (presetName == "white") currentHair_.material = HairMaterial::white();
        else if (presetName == "blue") currentHair_.material = HairMaterial::blue();
        else if (presetName == "pink") currentHair_.material = HairMaterial::pink();
        else if (presetName == "purple") currentHair_.material = HairMaterial::purple();
        else if (presetName == "green") currentHair_.material = HairMaterial::green();
        
        const HairStyleAsset* style = HairStyleLibrary::getInstance().getStyle(currentHair_.styleId);
        if (style) {
            currentHair_.applyMaterial(*style);
        }
    }
    
    // Get current hair mesh for rendering
    const Mesh& getHairMesh() const { return currentHair_.mesh; }
    bool hasHair() const { 
        return currentHair_.meshGenerated && 
               currentHair_.styleId != "bald" && 
               !currentHair_.styleId.empty(); 
    }
    
    const Vec3& getOffset() const { return currentHair_.offset; }
    const Vec3& getScale() const { return currentHair_.scale; }
    
    const std::string& getCurrentStyleId() const { return currentHair_.styleId; }
    const HairMaterial& getCurrentMaterial() const { return currentHair_.material; }
    
    // Get available color presets
    static std::vector<std::string> getColorPresets() {
        return {
            "black", "dark_brown", "brown", "auburn", "red",
            "blonde", "platinum", "gray", "white",
            "blue", "pink", "purple", "green"
        };
    }
    
private:
    HairInstance currentHair_;
};

// Convenience function
inline HairStyleLibrary& getHairLibrary() {
    return HairStyleLibrary::getInstance();
}

}  // namespace luma
