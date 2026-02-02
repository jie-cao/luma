// Example 07: Character Creator Demo
// Demonstrates the character creation system with BlendShapes
// Part of LUMA Engine Examples

#pragma once

#include "engine/character/character.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character_body.h"
#include "engine/character/character_face.h"
#include "engine/character/base_human_loader.h"
#include "engine/character/character_renderer.h"
#include "engine/character/ai/face_reconstruction.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/mesh.h"
#include <memory>
#include <string>

// ImGui forward declaration
struct ImGuiContext;

namespace luma {
namespace examples {

// ============================================================================
// Character Creator Demo
// ============================================================================

class CharacterCreatorDemo {
public:
    CharacterCreatorDemo() = default;
    ~CharacterCreatorDemo() = default;
    
    // === Setup ===
    
    bool initialize(UnifiedRenderer* renderer) {
        renderer_ = renderer;
        
        // Initialize model library
        auto& library = BaseHumanModelLibrary::getInstance();
        library.initializeDefaults();
        
        // Create character
        character_ = CharacterFactory::createBlank("Demo Character");
        
        // Load procedural model
        const BaseHumanModel* model = library.getModel("procedural_human");
        if (model) {
            character_->setBaseMesh(model->vertices, model->indices);
            
            // Copy BlendShape data
            auto& charBlendShapes = character_->getBlendShapeMesh();
            for (size_t i = 0; i < model->blendShapes.getTargetCount(); i++) {
                const BlendShapeTarget* target = model->blendShapes.getTarget(static_cast<int>(i));
                if (target) {
                    charBlendShapes.addTarget(*target);
                }
            }
            for (size_t i = 0; i < model->blendShapes.getChannelCount(); i++) {
                const BlendShapeChannel* channel = model->blendShapes.getChannel(static_cast<int>(i));
                if (channel) {
                    charBlendShapes.addChannel(*channel);
                }
            }
            
            currentModel_ = model;
        }
        
        // Setup character renderer
        charRenderer_.initialize(renderer);
        charRenderer_.setupCharacter(character_.get());
        
        // Initialize preset libraries
        bodyPresetLibrary_.initializeDefaults();
        facePresetLibrary_.initializeDefaults();
        
        // Initialize photo pipeline (without models for now)
        PhotoToFacePipeline::Config pipelineConfig;
        pipelineConfig.extractTexture = true;
        pipelineConfig.use3DMM = true;
        photoPipeline_.initialize(pipelineConfig);
        
        initialized_ = true;
        return true;
    }
    
    // === Update ===
    
    void update(float deltaTime) {
        if (!initialized_) return;
        
        // Update character animation
        character_->update(deltaTime);
        
        // Update BlendShape mesh
        charRenderer_.updateBlendShapes();
        
        // Auto-rotate if enabled
        if (autoRotate_) {
            rotationY_ += deltaTime * 0.5f;
        }
    }
    
    // === Render ===
    
    void render() {
        if (!initialized_ || !renderer_) return;
        
        // Get deformed mesh
        if (charRenderer_.needsGPUUpdate()) {
            currentMesh_ = charRenderer_.getCurrentMesh();
            
            // Apply skin color
            auto& bodyParams = character_->getBody().getParams();
            currentMesh_.baseColor[0] = bodyParams.skinColor.x;
            currentMesh_.baseColor[1] = bodyParams.skinColor.y;
            currentMesh_.baseColor[2] = bodyParams.skinColor.z;
            
            // Upload to GPU
            gpuMesh_ = renderer_->uploadMesh(currentMesh_);
            charRenderer_.markGPUUpdated();
        }
        
        // Render character
        if (gpuMesh_.indexCount > 0) {
            // Create model struct
            RHILoadedModel model;
            model.meshes.push_back(gpuMesh_);
            model.radius = 1.0f;
            model.center[0] = 0.0f;
            model.center[1] = 0.9f;  // Center at body mid-height
            model.center[2] = 0.0f;
            
            // Render with rotation
            float worldMatrix[16];
            createRotationMatrix(worldMatrix, rotationY_);
            
            renderer_->renderModel(model, worldMatrix);
        }
    }
    
    // === UI ===
    
    void renderUI() {
        if (!initialized_) return;
        
        renderMainPanel();
        renderPreviewPanel();
    }
    
    // === Camera Control ===
    
    void setAutoRotate(bool enable) { autoRotate_ = enable; }
    bool isAutoRotating() const { return autoRotate_; }
    
    void setRotation(float y) { rotationY_ = y; }
    float getRotation() const { return rotationY_; }
    
    // === Photo Import ===
    
    bool importPhoto(const uint8_t* imageData, int width, int height, int channels) {
        PhotoFaceResult result;
        if (photoPipeline_.process(imageData, width, height, channels, result)) {
            photoPipeline_.applyToCharacterFace(result, character_->getFace());
            return true;
        }
        return false;
    }
    
private:
    UnifiedRenderer* renderer_ = nullptr;
    std::unique_ptr<Character> character_;
    CharacterRenderer charRenderer_;
    
    const BaseHumanModel* currentModel_ = nullptr;
    Mesh currentMesh_;
    RHIGPUMesh gpuMesh_;
    
    BodyPresetLibrary bodyPresetLibrary_;
    FacePresetLibrary facePresetLibrary_;
    PhotoToFacePipeline photoPipeline_;
    
    bool initialized_ = false;
    bool autoRotate_ = true;
    float rotationY_ = 0.0f;
    
    // UI state
    int currentTab_ = 0;  // 0=Body, 1=Face, 2=BlendShape, 3=Export
    int bodySubTab_ = 0;
    int faceSubTab_ = 0;
    
    void createRotationMatrix(float* m, float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        
        m[0] = c;    m[1] = 0.0f; m[2] = -s;   m[3] = 0.0f;
        m[4] = 0.0f; m[5] = 1.0f; m[6] = 0.0f; m[7] = 0.0f;
        m[8] = s;    m[9] = 0.0f; m[10] = c;   m[11] = 0.0f;
        m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
    }
    
    // UI Rendering (ImGui)
    void renderMainPanel();
    void renderPreviewPanel();
    void renderBodyTab();
    void renderFaceTab();
    void renderBlendShapeTab();
    void renderExportTab();
};

// ============================================================================
// UI Implementation (requires imgui.h)
// ============================================================================

#ifdef LUMA_CHARACTER_CREATOR_DEMO_IMPLEMENTATION

#include "imgui.h"

void CharacterCreatorDemo::renderMainPanel() {
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Character Creator", nullptr, ImGuiWindowFlags_None)) {
        
        // Tab bar
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Body")) {
                currentTab_ = 0;
                renderBodyTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Face")) {
                currentTab_ = 1;
                renderFaceTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("BlendShapes")) {
                currentTab_ = 2;
                renderBlendShapeTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Export")) {
                currentTab_ = 3;
                renderExportTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void CharacterCreatorDemo::renderBodyTab() {
    auto& body = character_->getBody();
    auto& params = body.getParams();
    auto& m = params.measurements;
    
    // Gender selection
    ImGui::Text("Gender");
    int genderIdx = static_cast<int>(params.gender);
    bool genderChanged = false;
    if (ImGui::RadioButton("Male", &genderIdx, 0)) genderChanged = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("Female", &genderIdx, 1)) genderChanged = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("Neutral", &genderIdx, 2)) genderChanged = true;
    
    if (genderChanged) {
        body.setGender(static_cast<Gender>(genderIdx));
    }
    
    ImGui::Separator();
    
    // Presets
    if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* presets[] = {"Slim", "Average", "Muscular", "Heavy", "Elderly"};
        for (int i = 0; i < 5; i++) {
            if (ImGui::Button(presets[i], ImVec2(70, 0))) {
                BodyPreset preset;
                if (params.gender == Gender::Male) {
                    preset = static_cast<BodyPreset>(i);
                } else {
                    preset = static_cast<BodyPreset>(i + 5);
                }
                body.setPreset(preset);
            }
            if (i < 4) ImGui::SameLine();
        }
    }
    
    ImGui::Separator();
    
    // Overall parameters
    if (ImGui::CollapsingHeader("Overall", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= ImGui::SliderFloat("Height", &m.height, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Weight", &m.weight, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Muscularity", &m.muscularity, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Body Fat", &m.bodyFat, 0.0f, 1.0f);
        
        if (changed) body.updateBlendShapeWeights();
    }
    
    // Torso
    if (ImGui::CollapsingHeader("Torso")) {
        bool changed = false;
        changed |= ImGui::SliderFloat("Shoulder Width", &m.shoulderWidth, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Chest Size", &m.chestSize, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Waist Size", &m.waistSize, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Hip Width", &m.hipWidth, 0.0f, 1.0f);
        
        if (params.gender == Gender::Female) {
            changed |= ImGui::SliderFloat("Bust Size", &m.bustSize, 0.0f, 1.0f);
        }
        
        if (changed) body.updateBlendShapeWeights();
    }
    
    // Limbs
    if (ImGui::CollapsingHeader("Limbs")) {
        bool changed = false;
        changed |= ImGui::SliderFloat("Arm Length", &m.armLength, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Arm Thickness", &m.armThickness, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Leg Length", &m.legLength, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Thigh Thickness", &m.thighThickness, 0.0f, 1.0f);
        
        if (changed) body.updateBlendShapeWeights();
    }
    
    // Skin color
    if (ImGui::CollapsingHeader("Skin")) {
        float color[3] = {params.skinColor.x, params.skinColor.y, params.skinColor.z};
        if (ImGui::ColorEdit3("Skin Color", color)) {
            params.skinColor = Vec3(color[0], color[1], color[2]);
            character_->matchSkinColors();
        }
        
        // Skin presets
        ImGui::Text("Presets:");
        struct SkinPreset { const char* name; float r, g, b; };
        SkinPreset skinPresets[] = {
            {"Fair", 0.95f, 0.80f, 0.70f},
            {"Light", 0.90f, 0.72f, 0.60f},
            {"Medium", 0.80f, 0.60f, 0.45f},
            {"Olive", 0.70f, 0.55f, 0.40f},
            {"Brown", 0.55f, 0.40f, 0.30f},
            {"Dark", 0.35f, 0.25f, 0.20f}
        };
        
        for (int i = 0; i < 6; i++) {
            ImVec4 c(skinPresets[i].r, skinPresets[i].g, skinPresets[i].b, 1.0f);
            if (ImGui::ColorButton(skinPresets[i].name, c, 0, ImVec2(30, 30))) {
                params.skinColor = Vec3(skinPresets[i].r, skinPresets[i].g, skinPresets[i].b);
                character_->matchSkinColors();
            }
            if (i < 5) ImGui::SameLine();
        }
    }
}

void CharacterCreatorDemo::renderFaceTab() {
    auto& face = character_->getFace();
    auto& shape = face.getShapeParams();
    
    // Photo import button
    if (ImGui::Button("Import from Photo...", ImVec2(-1, 30))) {
        // Would open file dialog
        ImGui::OpenPopup("PhotoImportInfo");
    }
    
    if (ImGui::BeginPopup("PhotoImportInfo")) {
        ImGui::Text("Photo import requires loading an image file.");
        ImGui::Text("Use importPhoto() method with image data.");
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    // Face shape
    if (ImGui::CollapsingHeader("Face Shape", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Face Width", &shape.faceWidth, 0.0f, 1.0f);
        ImGui::SliderFloat("Face Length", &shape.faceLength, 0.0f, 1.0f);
        ImGui::SliderFloat("Face Roundness", &shape.faceRoundness, 0.0f, 1.0f);
    }
    
    // Eyes
    if (ImGui::CollapsingHeader("Eyes")) {
        ImGui::SliderFloat("Eye Size", &shape.eyeSize, 0.0f, 1.0f);
        ImGui::SliderFloat("Eye Spacing", &shape.eyeSpacing, 0.0f, 1.0f);
        ImGui::SliderFloat("Eye Height", &shape.eyeHeight, 0.0f, 1.0f);
        ImGui::SliderFloat("Eye Angle", &shape.eyeAngle, 0.0f, 1.0f);
        
        // Eye color
        auto& tex = face.getTextureParams();
        float eyeColor[3] = {tex.eyeColor.x, tex.eyeColor.y, tex.eyeColor.z};
        if (ImGui::ColorEdit3("Eye Color", eyeColor)) {
            tex.eyeColor = Vec3(eyeColor[0], eyeColor[1], eyeColor[2]);
        }
    }
    
    // Nose
    if (ImGui::CollapsingHeader("Nose")) {
        ImGui::SliderFloat("Nose Length", &shape.noseLength, 0.0f, 1.0f);
        ImGui::SliderFloat("Nose Width", &shape.noseWidth, 0.0f, 1.0f);
        ImGui::SliderFloat("Nose Height", &shape.noseHeight, 0.0f, 1.0f);
        ImGui::SliderFloat("Nose Bridge", &shape.noseBridge, 0.0f, 1.0f);
    }
    
    // Mouth
    if (ImGui::CollapsingHeader("Mouth")) {
        ImGui::SliderFloat("Mouth Width", &shape.mouthWidth, 0.0f, 1.0f);
        ImGui::SliderFloat("Upper Lip", &shape.upperLipThickness, 0.0f, 1.0f);
        ImGui::SliderFloat("Lower Lip", &shape.lowerLipThickness, 0.0f, 1.0f);
    }
    
    // Jaw
    if (ImGui::CollapsingHeader("Jaw & Chin")) {
        ImGui::SliderFloat("Jaw Width", &shape.jawWidth, 0.0f, 1.0f);
        ImGui::SliderFloat("Jaw Line", &shape.jawLine, 0.0f, 1.0f);
        ImGui::SliderFloat("Chin Length", &shape.chinLength, 0.0f, 1.0f);
        ImGui::SliderFloat("Chin Width", &shape.chinWidth, 0.0f, 1.0f);
    }
    
    // Expressions
    if (ImGui::CollapsingHeader("Expressions")) {
        if (ImGui::Button("Neutral")) face.setExpression("neutral");
        ImGui::SameLine();
        if (ImGui::Button("Smile")) face.setExpression("smile");
        ImGui::SameLine();
        if (ImGui::Button("Frown")) face.setExpression("frown");
        
        if (ImGui::Button("Surprise")) face.setExpression("surprise");
        ImGui::SameLine();
        if (ImGui::Button("Angry")) face.setExpression("angry");
    }
}

void CharacterCreatorDemo::renderBlendShapeTab() {
    auto& blendShapes = character_->getBlendShapeMesh();
    
    ImGui::Text("BlendShape Channels: %zu", blendShapes.getChannelCount());
    ImGui::Text("BlendShape Targets: %zu", blendShapes.getTargetCount());
    
    ImGui::Separator();
    
    // Direct BlendShape control
    if (ImGui::CollapsingHeader("Direct Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& channels = blendShapes.getChannels();
        
        for (size_t i = 0; i < channels.size(); i++) {
            const auto& ch = channels[i];
            float weight = ch.weight;
            
            std::string label = ch.name + "##" + std::to_string(i);
            if (ImGui::SliderFloat(label.c_str(), &weight, ch.minWeight, ch.maxWeight)) {
                blendShapes.setWeight(static_cast<int>(i), weight);
            }
        }
    }
    
    // Reset button
    ImGui::Separator();
    if (ImGui::Button("Reset All", ImVec2(-1, 30))) {
        blendShapes.resetAllWeights();
    }
}

void CharacterCreatorDemo::renderExportTab() {
    ImGui::Text("Export Character");
    ImGui::Separator();
    
    // Character name
    static char nameBuf[256] = "MyCharacter";
    ImGui::InputText("Name", nameBuf, sizeof(nameBuf));
    
    // Export format
    static int formatIdx = 0;
    const char* formats[] = {"glTF (.glb)", "FBX", "OBJ"};
    ImGui::Combo("Format", &formatIdx, formats, 3);
    
    ImGui::Separator();
    
    // Export options
    static bool exportSkeleton = true;
    static bool exportBlendShapes = true;
    ImGui::Checkbox("Include Skeleton", &exportSkeleton);
    ImGui::Checkbox("Include BlendShapes", &exportBlendShapes);
    
    ImGui::Separator();
    
    if (ImGui::Button("Export", ImVec2(-1, 40))) {
        // Would trigger export
        ImGui::OpenPopup("ExportInfo");
    }
    
    if (ImGui::BeginPopup("ExportInfo")) {
        ImGui::Text("Export not yet implemented.");
        ImGui::Text("Character data is ready for export.");
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    // Stats
    ImGui::Text("Statistics:");
    ImGui::BulletText("Vertices: %u", charRenderer_.getVertexCount());
    ImGui::BulletText("Triangles: %u", charRenderer_.getIndexCount() / 3);
    ImGui::BulletText("BlendShapes: %zu", character_->getBlendShapeMesh().getTargetCount());
    ImGui::BulletText("Bones: %d", character_->getSkeleton().getBoneCount());
}

void CharacterCreatorDemo::renderPreviewPanel() {
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(440, 20), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Preview Controls", nullptr, ImGuiWindowFlags_None)) {
        
        ImGui::Checkbox("Auto Rotate", &autoRotate_);
        
        if (!autoRotate_) {
            float rotation = rotationY_ * 57.2958f;  // Convert to degrees
            if (ImGui::SliderFloat("Rotation", &rotation, -180.0f, 180.0f)) {
                rotationY_ = rotation / 57.2958f;
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Front", ImVec2(50, 0))) rotationY_ = 0.0f;
        ImGui::SameLine();
        if (ImGui::Button("Back", ImVec2(50, 0))) rotationY_ = 3.14159f;
        ImGui::SameLine();
        if (ImGui::Button("Side", ImVec2(50, 0))) rotationY_ = 1.5708f;
        
        ImGui::Separator();
        
        // Randomize button
        if (ImGui::Button("Randomize", ImVec2(-1, 30))) {
            character_->randomize();
        }
    }
    ImGui::End();
}

#endif // LUMA_CHARACTER_CREATOR_DEMO_IMPLEMENTATION

} // namespace examples
} // namespace luma
