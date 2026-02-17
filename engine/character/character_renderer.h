// Character Renderer - BlendShape mesh rendering integration
// Part of LUMA Character Creation System
#pragma once

#include "engine/foundation/math_types.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character.h"
#include "engine/character/base_human_loader.h"
#include "engine/character/uv_mapping.h"
#include "engine/renderer/mesh.h"
#include <vector>
#include <memory>

namespace luma {

// Forward declaration
class UnifiedRenderer;

// ============================================================================
// Character GPU Mesh Data
// ============================================================================

struct CharacterGPUData {
    // Base mesh buffers (positions, normals - for BlendShape modification)
    std::vector<float> basePositions;   // 3 floats per vertex
    std::vector<float> baseNormals;     // 3 floats per vertex
    
    // Current deformed mesh (after BlendShape)
    std::vector<Vertex> deformedVertices;
    
    // Indices
    std::vector<uint32_t> indices;
    
    // Vertex count
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    
    // GPU state
    bool gpuDataValid = false;
    bool needsUpdate = false;
    
    // Renderer mesh handle
    uint32_t meshHandle = 0;
};

// ============================================================================
// Character Renderer - Manages character rendering with BlendShapes
// ============================================================================

class CharacterRenderer {
public:
    CharacterRenderer() = default;
    ~CharacterRenderer() = default;
    
    // === Initialization ===
    
    // Initialize with renderer reference
    void initialize(UnifiedRenderer* renderer) {
        renderer_ = renderer;
    }
    
    // === Character Setup ===
    
    // Setup GPU data from character
    void setupCharacter(Character* character) {
        character_ = character;
        
        // Get base mesh data
        const auto& baseVerts = character->getBaseVertices();
        if (baseVerts.empty()) {
            // Try to get from model library
            auto& library = BaseHumanModelLibrary::getInstance();
            // Note: initializeDefaults should have been called by CharacterModeHandler with modelDir
            // This is a fallback in case it wasn't
            if (library.getModelIds().empty()) {
                library.initializeDefaults();  // No model dir - will use procedural fallback
            }
            
            const BaseHumanModel* model = library.getModel("procedural_human");
            if (model) {
                character->setBaseMesh(model->vertices, model->indices);
                
                // Copy BlendShape data
                auto& charBlendShapes = character->getBlendShapeMesh();
                const auto& modelBlendShapes = model->blendShapes;
                
                // Copy targets
                for (size_t i = 0; i < modelBlendShapes.getTargetCount(); i++) {
                    const BlendShapeTarget* target = modelBlendShapes.getTarget(static_cast<int>(i));
                    if (target) {
                        charBlendShapes.addTarget(*target);
                    }
                }
                
                // Copy channels
                for (size_t i = 0; i < modelBlendShapes.getChannelCount(); i++) {
                    const BlendShapeChannel* channel = modelBlendShapes.getChannel(static_cast<int>(i));
                    if (channel) {
                        charBlendShapes.addChannel(*channel);
                    }
                }
            }
        }
        
        setupGPUData();
    }
    
    // Setup GPU data from base model
    void setupFromModel(const BaseHumanModel& model) {
        gpuData_.vertexCount = static_cast<uint32_t>(model.vertices.size());
        gpuData_.indexCount = static_cast<uint32_t>(model.indices.size());
        
        // Extract positions and normals
        gpuData_.basePositions.resize(gpuData_.vertexCount * 3);
        gpuData_.baseNormals.resize(gpuData_.vertexCount * 3);
        gpuData_.deformedVertices = model.vertices;
        gpuData_.indices = model.indices;
        
        for (uint32_t i = 0; i < gpuData_.vertexCount; i++) {
            gpuData_.basePositions[i * 3 + 0] = model.vertices[i].position[0];
            gpuData_.basePositions[i * 3 + 1] = model.vertices[i].position[1];
            gpuData_.basePositions[i * 3 + 2] = model.vertices[i].position[2];
            
            gpuData_.baseNormals[i * 3 + 0] = model.vertices[i].normal[0];
            gpuData_.baseNormals[i * 3 + 1] = model.vertices[i].normal[1];
            gpuData_.baseNormals[i * 3 + 2] = model.vertices[i].normal[2];
        }
        
        gpuData_.needsUpdate = true;
    }
    
    // === Update ===
    
    // Update deformed mesh from BlendShape weights (CPU fallback)
    void updateBlendShapes() {
        if (!character_) return;
        
        const auto& blendShapes = character_->getBlendShapeMesh();
        const auto& baseVerts = character_->getBaseVertices();
        
        if (baseVerts.empty()) return;
        
        // Apply BlendShapes
        blendShapes.applyToMesh(baseVerts, gpuData_.deformedVertices);
        
        // Recalculate tangents for normal mapping after deformation
        if (!gpuData_.indices.empty()) {
            UVMapper::calculateTangents(gpuData_.deformedVertices, gpuData_.indices);
        }
        
        gpuData_.needsUpdate = true;
    }
    
    // Update from external BlendShapeMesh
    void updateBlendShapes(const BlendShapeMesh& blendShapes, const std::vector<Vertex>& baseVerts) {
        if (baseVerts.empty()) return;
        
        blendShapes.applyToMesh(baseVerts, gpuData_.deformedVertices);
        gpuData_.needsUpdate = true;
    }
    
    // === Rendering ===
    
    // Get deformed mesh for rendering
    const std::vector<Vertex>& getDeformedVertices() const {
        return gpuData_.deformedVertices;
    }
    
    const std::vector<uint32_t>& getIndices() const {
        return gpuData_.indices;
    }
    
    // Get base mesh (undeformed) for landmark deformation
    std::vector<Vertex> getBaseMesh() const {
        if (character_) {
            return character_->getBaseVertices();
        }
        // Reconstruct from stored base positions/normals
        std::vector<Vertex> base(gpuData_.vertexCount);
        for (uint32_t i = 0; i < gpuData_.vertexCount; i++) {
            base[i].position[0] = gpuData_.basePositions[i * 3 + 0];
            base[i].position[1] = gpuData_.basePositions[i * 3 + 1];
            base[i].position[2] = gpuData_.basePositions[i * 3 + 2];
            base[i].normal[0] = gpuData_.baseNormals[i * 3 + 0];
            base[i].normal[1] = gpuData_.baseNormals[i * 3 + 1];
            base[i].normal[2] = gpuData_.baseNormals[i * 3 + 2];
            if (i < gpuData_.deformedVertices.size()) {
                base[i].uv[0] = gpuData_.deformedVertices[i].uv[0];
                base[i].uv[1] = gpuData_.deformedVertices[i].uv[1];
                base[i].color[0] = gpuData_.deformedVertices[i].color[0];
                base[i].color[1] = gpuData_.deformedVertices[i].color[1];
                base[i].color[2] = gpuData_.deformedVertices[i].color[2];
            }
        }
        return base;
    }
    
    // Set deformed vertices directly (for landmark-based deformation)
    void setDeformedVertices(const std::vector<Vertex>& vertices) {
        if (vertices.size() != gpuData_.vertexCount && gpuData_.vertexCount > 0) {
            printf("[CharacterRenderer] Warning: vertex count mismatch (%zu vs %u)\n", 
                   vertices.size(), gpuData_.vertexCount);
        }
        gpuData_.deformedVertices = vertices;
        gpuData_.needsUpdate = true;
    }
    
    // Check if needs GPU upload
    bool needsGPUUpdate() const {
        return gpuData_.needsUpdate;
    }
    
    // Mark as updated (after GPU upload)
    void markGPUUpdated() {
        gpuData_.needsUpdate = false;
        gpuData_.gpuDataValid = true;
    }
    
    // Get current mesh as Mesh struct for renderer
    Mesh getCurrentMesh() const {
        Mesh mesh;
        mesh.vertices = gpuData_.deformedVertices;
        mesh.indices = gpuData_.indices;
        mesh.baseColor[0] = 0.85f;
        mesh.baseColor[1] = 0.65f;
        mesh.baseColor[2] = 0.5f;
        mesh.metallic = 0.02f;   // Slight metallic for skin sheen
        mesh.roughness = 0.35f;  // Lower roughness = sharper specular highlights
        return mesh;
    }
    
    // === Character Mode Rendering ===
    
    // Render face region highlight overlay for the editing mode
    struct FaceRegionHighlight {
        bool enabled = false;
        int region = 0;           // FaceRegion enum value
        float highlightColor[4] = {0.3f, 0.7f, 1.0f, 0.3f};  // Semi-transparent blue
        float borderColor[4] = {0.4f, 0.8f, 1.0f, 0.8f};
    };
    
    void setFaceRegionHighlight(const FaceRegionHighlight& highlight) {
        faceHighlight_ = highlight;
    }
    
    // Render wireframe overlay for face sculpting
    struct WireframeOverlay {
        bool enabled = false;
        float color[4] = {0.4f, 0.5f, 0.6f, 0.5f};
        float lineWidth = 1.0f;
    };
    
    void setWireframeOverlay(const WireframeOverlay& overlay) {
        wireOverlay_ = overlay;
    }
    
    // === Skin Rendering Parameters (SSS) ===
    
    struct SkinRenderParams {
        bool sssEnabled = true;
        float sssStrength = 0.5f;
        float sssRadius = 0.01f;        // World-space SSS radius
        Vec3 sssColor{0.9f, 0.3f, 0.15f};  // Subsurface color (red-ish for skin)
        
        float skinRoughness = 0.4f;
        float skinSpecular = 0.3f;
        float skinClearcoat = 0.1f;      // Thin sheen layer (for oily skin)
        
        // Pore detail
        float poreScale = 50.0f;         // UV scale for pore texture
        float poreDepth = 0.02f;         // Normal map strength
        
        // Translucency (for ears, nose, thin skin areas)
        float translucency = 0.3f;
        Vec3 translucencyColor{1.0f, 0.4f, 0.2f};
    };
    
    void setSkinParams(const SkinRenderParams& params) { skinParams_ = params; }
    const SkinRenderParams& getSkinParams() const { return skinParams_; }
    
    // === Eye Rendering Parameters ===
    
    struct EyeRenderParams {
        bool enabled = true;
        
        // Iris
        Vec3 irisColor{0.4f, 0.3f, 0.2f};
        float irisRadius = 0.006f;       // World space
        float irisRoughness = 0.1f;
        float pupilSize = 0.35f;         // 0-1 relative to iris
        
        // Cornea (clear dome over iris)
        float corneaRefraction = 1.376f; // IOR of human cornea
        float corneaRoughness = 0.02f;   // Very smooth
        float corneaBulge = 0.001f;      // Height of cornea dome
        
        // Sclera (white part)
        Vec3 scleraColor{0.95f, 0.95f, 0.92f};
        float scleraRoughness = 0.3f;
        float scleraRedness = 0.0f;      // Blood vessel visibility
        
        // Specular highlight
        float specularIntensity = 1.0f;
        float specularSize = 0.05f;      // Highlight size
        
        // Parallax
        bool parallaxEnabled = true;
        float parallaxDepth = 0.003f;    // Depth of iris behind cornea
        
        // Limbus darkening (ring between iris and sclera)
        float limbusDarkening = 0.3f;
    };
    
    void setEyeParams(const EyeRenderParams& params) { eyeParams_ = params; }
    const EyeRenderParams& getEyeParams() const { return eyeParams_; }
    
    // === Hair Rendering Parameters ===
    
    struct HairRenderParams {
        bool enabled = true;
        Vec3 hairColor{0.15f, 0.10f, 0.08f};
        float hairRoughness = 0.4f;
        float hairAnisotropy = 0.8f;     // Anisotropic highlight direction
        float hairSpecular = 0.5f;
        
        // Kajiya-Kay dual highlight
        Vec3 primaryHighlight{1.0f, 0.9f, 0.8f};
        float primaryShift = -0.1f;      // Primary highlight shift
        Vec3 secondaryHighlight{0.8f, 0.6f, 0.4f};
        float secondaryShift = 0.1f;     // Secondary highlight shift
        
        // Strand properties
        float strandWidth = 0.001f;
        float strandDensity = 1.0f;
        float opacity = 1.0f;
    };
    
    void setHairParams(const HairRenderParams& params) { hairParams_ = params; }
    const HairRenderParams& getHairParams() const { return hairParams_; }
    
    // === Combined Character Render Data ===
    
    // Get rendering constants for shader upload
    struct CharacterShaderConstants {
        // Skin SSS
        float sssStrength;
        float sssRadius;
        float sssColor[3];
        float skinRoughness;
        float skinSpecular;
        
        // Eye
        float irisColor[3];
        float pupilSize;
        float corneaIOR;
        float scleraColor[3];
        
        // Hair
        float hairColor[3];
        float hairAnisotropy;
        float hairSpecular;
        
        float padding[3]; // Align to 16 bytes
    };
    
    CharacterShaderConstants getShaderConstants() const {
        CharacterShaderConstants c = {};
        
        c.sssStrength = skinParams_.sssStrength;
        c.sssRadius = skinParams_.sssRadius;
        c.sssColor[0] = skinParams_.sssColor.x;
        c.sssColor[1] = skinParams_.sssColor.y;
        c.sssColor[2] = skinParams_.sssColor.z;
        c.skinRoughness = skinParams_.skinRoughness;
        c.skinSpecular = skinParams_.skinSpecular;
        
        c.irisColor[0] = eyeParams_.irisColor.x;
        c.irisColor[1] = eyeParams_.irisColor.y;
        c.irisColor[2] = eyeParams_.irisColor.z;
        c.pupilSize = eyeParams_.pupilSize;
        c.corneaIOR = eyeParams_.corneaRefraction;
        c.scleraColor[0] = eyeParams_.scleraColor.x;
        c.scleraColor[1] = eyeParams_.scleraColor.y;
        c.scleraColor[2] = eyeParams_.scleraColor.z;
        
        c.hairColor[0] = hairParams_.hairColor.x;
        c.hairColor[1] = hairParams_.hairColor.y;
        c.hairColor[2] = hairParams_.hairColor.z;
        c.hairAnisotropy = hairParams_.hairAnisotropy;
        c.hairSpecular = hairParams_.hairSpecular;
        
        return c;
    }
    
    // === State ===
    
    Character* getCharacter() { return character_; }
    const Character* getCharacter() const { return character_; }
    
    uint32_t getVertexCount() const { return gpuData_.vertexCount; }
    uint32_t getIndexCount() const { return gpuData_.indexCount; }
    
private:
    UnifiedRenderer* renderer_ = nullptr;
    Character* character_ = nullptr;
    CharacterGPUData gpuData_;
    
    // Rendering features
    SkinRenderParams skinParams_;
    EyeRenderParams eyeParams_;
    HairRenderParams hairParams_;
    FaceRegionHighlight faceHighlight_;
    WireframeOverlay wireOverlay_;
    
    void setupGPUData() {
        if (!character_) return;
        
        const auto& baseVerts = character_->getBaseVertices();
        const auto& indices = character_->getIndices();
        
        if (baseVerts.empty()) return;
        
        gpuData_.vertexCount = static_cast<uint32_t>(baseVerts.size());
        gpuData_.indexCount = static_cast<uint32_t>(indices.size());
        
        // Extract positions and normals
        gpuData_.basePositions.resize(gpuData_.vertexCount * 3);
        gpuData_.baseNormals.resize(gpuData_.vertexCount * 3);
        gpuData_.deformedVertices = baseVerts;
        gpuData_.indices = indices;
        
        for (uint32_t i = 0; i < gpuData_.vertexCount; i++) {
            gpuData_.basePositions[i * 3 + 0] = baseVerts[i].position[0];
            gpuData_.basePositions[i * 3 + 1] = baseVerts[i].position[1];
            gpuData_.basePositions[i * 3 + 2] = baseVerts[i].position[2];
            
            gpuData_.baseNormals[i * 3 + 0] = baseVerts[i].normal[0];
            gpuData_.baseNormals[i * 3 + 1] = baseVerts[i].normal[1];
            gpuData_.baseNormals[i * 3 + 2] = baseVerts[i].normal[2];
        }
        
        gpuData_.needsUpdate = true;
    }
};

} // namespace luma
