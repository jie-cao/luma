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
            library.initializeDefaults();
            
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
        mesh.metallic = 0.0f;
        mesh.roughness = 0.5f;
        return mesh;
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
