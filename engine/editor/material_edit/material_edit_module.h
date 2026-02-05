// LUMA Material Edit Module
// Modular material editing functionality (PBR parameters, texture slots)

#pragma once

#include "engine/editor/edit_module.h"
#include "engine/mesh/edit_mesh.h"
#include <memory>
#include <string>
#include <vector>

namespace luma {
namespace editor {

// PBR material parameters
struct MaterialParams {
    float baseColor[3] = {0.8f, 0.8f, 0.8f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emissive[3] = {0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f;
    float alpha = 1.0f;
    
    // Texture paths
    std::string albedoTexture;
    std::string normalTexture;
    std::string metallicTexture;
    std::string roughnessTexture;
    std::string emissiveTexture;
    std::string aoTexture;
};

// Material Edit Module
class MaterialEditModule : public EditModule {
public:
    MaterialEditModule();
    ~MaterialEditModule() override;
    
    // Initialize
    bool init(UnifiedRenderer* renderer);
    void setEditMesh(EditMesh* mesh);
    EditMesh* getEditMesh() const { return editMesh; }
    
    // EditModule interface
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render(DrawManager& drawManager, const RenderContext& ctx) override;
    void renderUI() override;
    bool handleInput(const InputEvent& event) override;
    
    // Material editing
    void setBaseColor(float r, float g, float b);
    void setMetallic(float value);
    void setRoughness(float value);
    void setEmissive(float r, float g, float b, float intensity = 1.0f);
    void setAlpha(float value);
    
    // Texture management
    void setAlbedoTexture(const std::string& path);
    void setNormalTexture(const std::string& path);
    void setMetallicTexture(const std::string& path);
    void setRoughnessTexture(const std::string& path);
    void setEmissiveTexture(const std::string& path);
    void setAOTexture(const std::string& path);
    
    void clearAlbedoTexture();
    void clearNormalTexture();
    void clearAllTextures();
    
    // Material presets
    void applyPresetMetal();
    void applyPresetPlastic();
    void applyPresetCeramic();
    void applyPresetWood();
    void applyPresetFabric();
    
    // Get current material
    const MaterialParams& getMaterial() const { return material; }
    
    // Apply material to mesh
    void applyToMesh();
    void applyToSelectedFaces();
    
private:
    UnifiedRenderer* renderer = nullptr;
    EditMesh* editMesh = nullptr;
    MaterialParams material;
    
    // Callbacks
    using MaterialChangedCallback = std::function<void()>;
    MaterialChangedCallback materialChangedCallback;
    
    void notifyMaterialChanged();
};

// ============================================================================
// Implementation
// ============================================================================

inline MaterialEditModule::MaterialEditModule() 
    : EditModule("MaterialEdit") {
}

inline MaterialEditModule::~MaterialEditModule() = default;

inline bool MaterialEditModule::init(UnifiedRenderer* r) {
    renderer = r;
    return renderer != nullptr;
}

inline void MaterialEditModule::setEditMesh(EditMesh* mesh) {
    editMesh = mesh;
}

inline void MaterialEditModule::onEnter() {
    active = true;
}

inline void MaterialEditModule::onExit() {
    active = false;
}

inline void MaterialEditModule::update(float deltaTime) {
    (void)deltaTime;
}

inline void MaterialEditModule::render(DrawManager& drawManager, const RenderContext& ctx) {
    (void)drawManager;
    (void)ctx;
    // Material preview would be rendered in the viewport
}

inline void MaterialEditModule::renderUI() {
    // ImGui material editor UI would be rendered here
}

inline bool MaterialEditModule::handleInput(const InputEvent& event) {
    (void)event;
    return false;
}

inline void MaterialEditModule::setBaseColor(float r, float g, float b) {
    material.baseColor[0] = r;
    material.baseColor[1] = g;
    material.baseColor[2] = b;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setMetallic(float value) {
    material.metallic = std::max(0.0f, std::min(1.0f, value));
    notifyMaterialChanged();
}

inline void MaterialEditModule::setRoughness(float value) {
    material.roughness = std::max(0.0f, std::min(1.0f, value));
    notifyMaterialChanged();
}

inline void MaterialEditModule::setEmissive(float r, float g, float b, float intensity) {
    material.emissive[0] = r;
    material.emissive[1] = g;
    material.emissive[2] = b;
    material.emissiveIntensity = intensity;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setAlpha(float value) {
    material.alpha = std::max(0.0f, std::min(1.0f, value));
    notifyMaterialChanged();
}

inline void MaterialEditModule::setAlbedoTexture(const std::string& path) {
    material.albedoTexture = path;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setNormalTexture(const std::string& path) {
    material.normalTexture = path;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setMetallicTexture(const std::string& path) {
    material.metallicTexture = path;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setRoughnessTexture(const std::string& path) {
    material.roughnessTexture = path;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setEmissiveTexture(const std::string& path) {
    material.emissiveTexture = path;
    notifyMaterialChanged();
}

inline void MaterialEditModule::setAOTexture(const std::string& path) {
    material.aoTexture = path;
    notifyMaterialChanged();
}

inline void MaterialEditModule::clearAlbedoTexture() {
    material.albedoTexture.clear();
    notifyMaterialChanged();
}

inline void MaterialEditModule::clearNormalTexture() {
    material.normalTexture.clear();
    notifyMaterialChanged();
}

inline void MaterialEditModule::clearAllTextures() {
    material.albedoTexture.clear();
    material.normalTexture.clear();
    material.metallicTexture.clear();
    material.roughnessTexture.clear();
    material.emissiveTexture.clear();
    material.aoTexture.clear();
    notifyMaterialChanged();
}

inline void MaterialEditModule::applyPresetMetal() {
    material.metallic = 1.0f;
    material.roughness = 0.3f;
    material.baseColor[0] = 0.9f;
    material.baseColor[1] = 0.9f;
    material.baseColor[2] = 0.9f;
    notifyMaterialChanged();
}

inline void MaterialEditModule::applyPresetPlastic() {
    material.metallic = 0.0f;
    material.roughness = 0.4f;
    material.baseColor[0] = 0.8f;
    material.baseColor[1] = 0.2f;
    material.baseColor[2] = 0.2f;
    notifyMaterialChanged();
}

inline void MaterialEditModule::applyPresetCeramic() {
    material.metallic = 0.0f;
    material.roughness = 0.1f;
    material.baseColor[0] = 0.95f;
    material.baseColor[1] = 0.95f;
    material.baseColor[2] = 0.9f;
    notifyMaterialChanged();
}

inline void MaterialEditModule::applyPresetWood() {
    material.metallic = 0.0f;
    material.roughness = 0.7f;
    material.baseColor[0] = 0.6f;
    material.baseColor[1] = 0.4f;
    material.baseColor[2] = 0.2f;
    notifyMaterialChanged();
}

inline void MaterialEditModule::applyPresetFabric() {
    material.metallic = 0.0f;
    material.roughness = 0.9f;
    material.baseColor[0] = 0.5f;
    material.baseColor[1] = 0.5f;
    material.baseColor[2] = 0.6f;
    notifyMaterialChanged();
}

inline void MaterialEditModule::applyToMesh() {
    if (!editMesh) return;
    
    // Apply material to all loops
    for (auto& face : editMesh->faces) {
        for (auto& loop : face.loops) {
            loop.color[0] = material.baseColor[0];
            loop.color[1] = material.baseColor[1];
            loop.color[2] = material.baseColor[2];
            loop.color[3] = material.alpha;
        }
    }
    
    markDirty();
}

inline void MaterialEditModule::applyToSelectedFaces() {
    if (!editMesh) return;
    
    for (uint32_t fi : editMesh->selectedFaces) {
        if (fi < editMesh->faces.size()) {
            auto& face = editMesh->faces[fi];
            for (auto& loop : face.loops) {
                loop.color[0] = material.baseColor[0];
                loop.color[1] = material.baseColor[1];
                loop.color[2] = material.baseColor[2];
                loop.color[3] = material.alpha;
            }
        }
    }
    
    markDirty();
}

inline void MaterialEditModule::notifyMaterialChanged() {
    markDirty();
    if (materialChangedCallback) {
        materialChangedCallback();
    }
}

} // namespace editor
} // namespace luma
