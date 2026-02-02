// Character Creator UI - ImGui-based character creation interface
// Part of LUMA Character Creation System
#pragma once

#include "engine/character/character.h"
#include "engine/character/blend_shape.h"
#include "engine/character/character_body.h"
#include "engine/character/character_face.h"
#include <string>
#include <vector>
#include <functional>

// Forward declare ImGui types to avoid including imgui.h in header
struct ImVec2;
struct ImVec4;

namespace luma {

// ============================================================================
// UI Configuration
// ============================================================================

struct CharacterCreatorUIConfig {
    float panelWidth = 350.0f;              // Width of side panels
    float sliderWidth = 200.0f;             // Width of slider controls
    float thumbnailSize = 64.0f;            // Size of preset thumbnails
    float colorPickerSize = 200.0f;         // Size of color picker
    bool showAdvancedParams = false;        // Show advanced parameters
    bool showDebugInfo = false;             // Show debug info
    bool compactMode = false;               // Use compact layout
};

// ============================================================================
// UI State
// ============================================================================

enum class CreatorTab {
    Body,
    Face,
    Clothing,
    Animation,
    Export
};

enum class FaceSubTab {
    Shape,
    Eyes,
    Nose,
    Mouth,
    Texture,
    Expression
};

enum class BodySubTab {
    Preset,
    Overall,
    Torso,
    Arms,
    Legs,
    Skin
};

// ============================================================================
// Character Creator UI
// ============================================================================

class CharacterCreatorUI {
public:
    CharacterCreatorUI() = default;
    
    // === Setup ===
    
    void setCharacter(Character* character) {
        character_ = character;
    }
    
    void setConfig(const CharacterCreatorUIConfig& config) {
        config_ = config;
    }
    
    // === Callbacks ===
    
    using PhotoCallback = std::function<void(const std::string& path)>;
    using ExportCallback = std::function<void(const std::string& path, CharacterExportFormat format)>;
    using PresetCallback = std::function<void(const std::string& presetName)>;
    
    void setOnPhotoImport(PhotoCallback cb) { onPhotoImport_ = cb; }
    void setOnExport(ExportCallback cb) { onExport_ = cb; }
    void setOnPresetApply(PresetCallback cb) { onPresetApply_ = cb; }
    
    // === Preset Libraries ===
    
    void setBodyPresetLibrary(BodyPresetLibrary* library) {
        bodyPresetLibrary_ = library;
    }
    
    void setFacePresetLibrary(FacePresetLibrary* library) {
        facePresetLibrary_ = library;
    }
    
    // === Render ===
    
    // Main render function - call this in your ImGui render loop
    void render() {
        if (!character_) return;
        
        renderMainPanel();
        
        if (showPreview_) {
            renderPreviewWindow();
        }
    }
    
    // === State ===
    
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    
    CreatorTab getCurrentTab() const { return currentTab_; }
    void setCurrentTab(CreatorTab tab) { currentTab_ = tab; }
    
private:
    Character* character_ = nullptr;
    CharacterCreatorUIConfig config_;
    
    bool visible_ = true;
    bool showPreview_ = true;
    
    CreatorTab currentTab_ = CreatorTab::Body;
    FaceSubTab faceSubTab_ = FaceSubTab::Shape;
    BodySubTab bodySubTab_ = BodySubTab::Preset;
    
    // Callbacks
    PhotoCallback onPhotoImport_;
    ExportCallback onExport_;
    PresetCallback onPresetApply_;
    
    // Preset libraries
    BodyPresetLibrary* bodyPresetLibrary_ = nullptr;
    FacePresetLibrary* facePresetLibrary_ = nullptr;
    
    // Temp state for UI
    std::string photoPath_;
    std::string exportPath_ = "character.glb";
    int exportFormatIndex_ = 0;
    
    // === Render Functions ===
    
    void renderMainPanel();
    void renderTabBar();
    void renderBodyTab();
    void renderFaceTab();
    void renderClothingTab();
    void renderAnimationTab();
    void renderExportTab();
    void renderPreviewWindow();
    
    // Body sub-panels
    void renderBodyPresetPanel();
    void renderBodyOverallPanel();
    void renderBodyTorsoPanel();
    void renderBodyArmsPanel();
    void renderBodyLegsPanel();
    void renderBodySkinPanel();
    
    // Face sub-panels
    void renderFaceShapePanel();
    void renderFaceEyesPanel();
    void renderFaceNosePanel();
    void renderFaceMouthPanel();
    void renderFaceTexturePanel();
    void renderFaceExpressionPanel();
    
    // Helpers
    bool sliderWithReset(const char* label, float* value, float min, float max, float defaultValue = 0.5f);
    void colorPickerWithPresets(const char* label, float* color, const std::vector<std::array<float, 3>>& presets);
    void presetGrid(const std::vector<std::string>& names, const std::string& currentPreset, 
                    std::function<void(const std::string&)> onSelect);
};

// ============================================================================
// Implementation (would normally be in .cpp file)
// ============================================================================

// Note: This implementation requires imgui.h to be included
// In a real project, this would be in a .cpp file

#ifdef LUMA_CHARACTER_CREATOR_UI_IMPLEMENTATION

#include "imgui.h"

void CharacterCreatorUI::renderMainPanel() {
    ImGui::SetNextWindowSize(ImVec2(config_.panelWidth, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Character Creator", &visible_, ImGuiWindowFlags_MenuBar)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Character")) {
                    // Reset character
                }
                if (ImGui::MenuItem("Import Photo...")) {
                    // Open file dialog
                }
                if (ImGui::MenuItem("Export...")) {
                    currentTab_ = CreatorTab::Export;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Show Preview", nullptr, &showPreview_);
                ImGui::MenuItem("Advanced Parameters", nullptr, &config_.showAdvancedParams);
                ImGui::MenuItem("Debug Info", nullptr, &config_.showDebugInfo);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        renderTabBar();
        
        // Tab content
        switch (currentTab_) {
            case CreatorTab::Body:
                renderBodyTab();
                break;
            case CreatorTab::Face:
                renderFaceTab();
                break;
            case CreatorTab::Clothing:
                renderClothingTab();
                break;
            case CreatorTab::Animation:
                renderAnimationTab();
                break;
            case CreatorTab::Export:
                renderExportTab();
                break;
        }
    }
    ImGui::End();
}

void CharacterCreatorUI::renderTabBar() {
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Body")) {
            currentTab_ = CreatorTab::Body;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Face")) {
            currentTab_ = CreatorTab::Face;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Clothing")) {
            currentTab_ = CreatorTab::Clothing;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Animation")) {
            currentTab_ = CreatorTab::Animation;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Export")) {
            currentTab_ = CreatorTab::Export;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void CharacterCreatorUI::renderBodyTab() {
    if (!character_) return;
    
    // Sub-tab buttons
    if (ImGui::Button("Preset")) bodySubTab_ = BodySubTab::Preset;
    ImGui::SameLine();
    if (ImGui::Button("Overall")) bodySubTab_ = BodySubTab::Overall;
    ImGui::SameLine();
    if (ImGui::Button("Torso")) bodySubTab_ = BodySubTab::Torso;
    ImGui::SameLine();
    if (ImGui::Button("Arms")) bodySubTab_ = BodySubTab::Arms;
    ImGui::SameLine();
    if (ImGui::Button("Legs")) bodySubTab_ = BodySubTab::Legs;
    ImGui::SameLine();
    if (ImGui::Button("Skin")) bodySubTab_ = BodySubTab::Skin;
    
    ImGui::Separator();
    
    switch (bodySubTab_) {
        case BodySubTab::Preset: renderBodyPresetPanel(); break;
        case BodySubTab::Overall: renderBodyOverallPanel(); break;
        case BodySubTab::Torso: renderBodyTorsoPanel(); break;
        case BodySubTab::Arms: renderBodyArmsPanel(); break;
        case BodySubTab::Legs: renderBodyLegsPanel(); break;
        case BodySubTab::Skin: renderBodySkinPanel(); break;
    }
}

void CharacterCreatorUI::renderBodyPresetPanel() {
    auto& body = character_->getBody();
    auto& params = body.getParams();
    
    ImGui::Text("Gender");
    int genderIdx = static_cast<int>(params.gender);
    if (ImGui::RadioButton("Male", &genderIdx, 0)) {
        body.setGender(Gender::Male);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Female", &genderIdx, 1)) {
        body.setGender(Gender::Female);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Neutral", &genderIdx, 2)) {
        body.setGender(Gender::Neutral);
    }
    
    ImGui::Separator();
    ImGui::Text("Body Presets");
    
    if (bodyPresetLibrary_) {
        auto categories = bodyPresetLibrary_->getCategories();
        for (const auto& category : categories) {
            if (ImGui::TreeNode(category.c_str())) {
                auto presets = bodyPresetLibrary_->getPresetsByCategory(category);
                for (const auto* preset : presets) {
                    if (ImGui::Selectable(preset->name.c_str())) {
                        body.setParams(preset->params);
                        if (onPresetApply_) onPresetApply_(preset->name);
                    }
                }
                ImGui::TreePop();
            }
        }
    } else {
        // Fallback: show built-in presets
        const char* presetNames[] = {
            "Slim", "Average", "Muscular", "Heavy", "Elderly"
        };
        
        for (int i = 0; i < 5; i++) {
            if (ImGui::Selectable(presetNames[i])) {
                BodyPreset preset;
                if (params.gender == Gender::Male) {
                    preset = static_cast<BodyPreset>(i);  // MaleSlim through MaleElderly
                } else {
                    preset = static_cast<BodyPreset>(i + 5);  // FemaleSlim through FemaleElderly
                }
                body.setPreset(preset);
            }
        }
    }
}

void CharacterCreatorUI::renderBodyOverallPanel() {
    auto& body = character_->getBody();
    auto& m = body.getParams().measurements;
    
    bool changed = false;
    
    ImGui::Text("Overall Body Shape");
    
    changed |= sliderWithReset("Height", &m.height, 0.0f, 1.0f);
    changed |= sliderWithReset("Weight", &m.weight, 0.0f, 1.0f);
    changed |= sliderWithReset("Muscularity", &m.muscularity, 0.0f, 1.0f);
    changed |= sliderWithReset("Body Fat", &m.bodyFat, 0.0f, 1.0f);
    
    if (changed) {
        body.updateBlendShapeWeights();
    }
}

void CharacterCreatorUI::renderBodyTorsoPanel() {
    auto& body = character_->getBody();
    auto& m = body.getParams().measurements;
    
    bool changed = false;
    
    ImGui::Text("Torso");
    
    changed |= sliderWithReset("Shoulder Width", &m.shoulderWidth, 0.0f, 1.0f);
    changed |= sliderWithReset("Chest Size", &m.chestSize, 0.0f, 1.0f);
    changed |= sliderWithReset("Waist Size", &m.waistSize, 0.0f, 1.0f);
    changed |= sliderWithReset("Hip Width", &m.hipWidth, 0.0f, 1.0f);
    changed |= sliderWithReset("Torso Length", &m.torsoLength, 0.0f, 1.0f);
    changed |= sliderWithReset("Neck Thickness", &m.neckThickness, 0.0f, 1.0f);
    
    if (body.getParams().gender == Gender::Female) {
        changed |= sliderWithReset("Bust Size", &m.bustSize, 0.0f, 1.0f);
    }
    
    if (changed) {
        body.updateBlendShapeWeights();
    }
}

void CharacterCreatorUI::renderBodyArmsPanel() {
    auto& body = character_->getBody();
    auto& m = body.getParams().measurements;
    
    bool changed = false;
    
    ImGui::Text("Arms");
    
    changed |= sliderWithReset("Arm Length", &m.armLength, 0.0f, 1.0f);
    changed |= sliderWithReset("Upper Arm", &m.armThickness, 0.0f, 1.0f);
    changed |= sliderWithReset("Forearm", &m.forearmThickness, 0.0f, 1.0f);
    changed |= sliderWithReset("Hand Size", &m.handSize, 0.0f, 1.0f);
    
    if (changed) {
        body.updateBlendShapeWeights();
    }
}

void CharacterCreatorUI::renderBodyLegsPanel() {
    auto& body = character_->getBody();
    auto& m = body.getParams().measurements;
    
    bool changed = false;
    
    ImGui::Text("Legs");
    
    changed |= sliderWithReset("Leg Length", &m.legLength, 0.0f, 1.0f);
    changed |= sliderWithReset("Thigh", &m.thighThickness, 0.0f, 1.0f);
    changed |= sliderWithReset("Calf", &m.calfThickness, 0.0f, 1.0f);
    changed |= sliderWithReset("Foot Size", &m.footSize, 0.0f, 1.0f);
    
    if (changed) {
        body.updateBlendShapeWeights();
    }
}

void CharacterCreatorUI::renderBodySkinPanel() {
    auto& body = character_->getBody();
    auto& params = body.getParams();
    
    ImGui::Text("Skin Tone");
    
    float skinColor[3] = {params.skinColor.x, params.skinColor.y, params.skinColor.z};
    if (ImGui::ColorEdit3("Skin Color", skinColor)) {
        params.skinColor = Vec3(skinColor[0], skinColor[1], skinColor[2]);
        character_->matchSkinColors();  // Sync face and body
    }
    
    // Skin tone presets
    ImGui::Text("Presets:");
    struct SkinPreset { const char* name; float r, g, b; };
    SkinPreset presets[] = {
        {"Fair", 0.95f, 0.80f, 0.70f},
        {"Light", 0.90f, 0.72f, 0.60f},
        {"Medium", 0.80f, 0.60f, 0.45f},
        {"Olive", 0.70f, 0.55f, 0.40f},
        {"Brown", 0.55f, 0.40f, 0.30f},
        {"Dark", 0.35f, 0.25f, 0.20f}
    };
    
    for (const auto& preset : presets) {
        ImVec4 color(preset.r, preset.g, preset.b, 1.0f);
        if (ImGui::ColorButton(preset.name, color, 0, ImVec2(30, 30))) {
            params.skinColor = Vec3(preset.r, preset.g, preset.b);
            character_->matchSkinColors();
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
    
    ImGui::SliderFloat("Roughness", &params.skinRoughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Subsurface", &params.skinSubsurface, 0.0f, 1.0f);
}

void CharacterCreatorUI::renderFaceTab() {
    if (!character_) return;
    
    // Photo import button
    if (ImGui::Button("Import from Photo...")) {
        // Open file dialog
        if (onPhotoImport_) {
            onPhotoImport_(photoPath_);
        }
    }
    
    ImGui::Separator();
    
    // Sub-tab buttons
    if (ImGui::Button("Shape")) faceSubTab_ = FaceSubTab::Shape;
    ImGui::SameLine();
    if (ImGui::Button("Eyes")) faceSubTab_ = FaceSubTab::Eyes;
    ImGui::SameLine();
    if (ImGui::Button("Nose")) faceSubTab_ = FaceSubTab::Nose;
    ImGui::SameLine();
    if (ImGui::Button("Mouth")) faceSubTab_ = FaceSubTab::Mouth;
    ImGui::SameLine();
    if (ImGui::Button("Texture")) faceSubTab_ = FaceSubTab::Texture;
    ImGui::SameLine();
    if (ImGui::Button("Expression")) faceSubTab_ = FaceSubTab::Expression;
    
    ImGui::Separator();
    
    switch (faceSubTab_) {
        case FaceSubTab::Shape: renderFaceShapePanel(); break;
        case FaceSubTab::Eyes: renderFaceEyesPanel(); break;
        case FaceSubTab::Nose: renderFaceNosePanel(); break;
        case FaceSubTab::Mouth: renderFaceMouthPanel(); break;
        case FaceSubTab::Texture: renderFaceTexturePanel(); break;
        case FaceSubTab::Expression: renderFaceExpressionPanel(); break;
    }
}

void CharacterCreatorUI::renderFaceShapePanel() {
    auto& face = character_->getFace();
    auto& shape = face.getShapeParams();
    
    ImGui::Text("Face Shape");
    
    // Face presets
    if (facePresetLibrary_) {
        if (ImGui::TreeNode("Presets")) {
            auto categories = facePresetLibrary_->getCategories();
            for (const auto& category : categories) {
                if (ImGui::TreeNode(category.c_str())) {
                    auto presets = facePresetLibrary_->getPresetsByCategory(category);
                    for (const auto* preset : presets) {
                        if (ImGui::Selectable(preset->name.c_str())) {
                            face.setShapeParams(preset->shapeParams);
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    }
    
    ImGui::Separator();
    
    sliderWithReset("Face Width", &shape.faceWidth, 0.0f, 1.0f);
    sliderWithReset("Face Length", &shape.faceLength, 0.0f, 1.0f);
    sliderWithReset("Face Roundness", &shape.faceRoundness, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Forehead");
    sliderWithReset("Height", &shape.foreheadHeight, 0.0f, 1.0f);
    sliderWithReset("Width", &shape.foreheadWidth, 0.0f, 1.0f);
    sliderWithReset("Slope", &shape.foreheadSlope, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Jaw & Chin");
    sliderWithReset("Jaw Width", &shape.jawWidth, 0.0f, 1.0f);
    sliderWithReset("Jaw Angle", &shape.jawAngle, 0.0f, 1.0f);
    sliderWithReset("Jaw Line", &shape.jawLine, 0.0f, 1.0f);
    sliderWithReset("Chin Length", &shape.chinLength, 0.0f, 1.0f);
    sliderWithReset("Chin Width", &shape.chinWidth, 0.0f, 1.0f);
    sliderWithReset("Chin Shape", &shape.chinShape, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Cheeks");
    sliderWithReset("Cheekbone Height", &shape.cheekboneHeight, 0.0f, 1.0f);
    sliderWithReset("Cheekbone Width", &shape.cheekboneWidth, 0.0f, 1.0f);
    sliderWithReset("Cheekbone Prominence", &shape.cheekboneProminence, 0.0f, 1.0f);
    sliderWithReset("Cheek Fullness", &shape.cheekFullness, 0.0f, 1.0f);
}

void CharacterCreatorUI::renderFaceEyesPanel() {
    auto& face = character_->getFace();
    auto& shape = face.getShapeParams();
    
    ImGui::Text("Eyes");
    
    sliderWithReset("Size", &shape.eyeSize, 0.0f, 1.0f);
    sliderWithReset("Width", &shape.eyeWidth, 0.0f, 1.0f);
    sliderWithReset("Height Position", &shape.eyeHeight, 0.0f, 1.0f);
    sliderWithReset("Spacing", &shape.eyeSpacing, 0.0f, 1.0f);
    sliderWithReset("Angle", &shape.eyeAngle, 0.0f, 1.0f);
    sliderWithReset("Depth", &shape.eyeDepth, 0.0f, 1.0f);
    sliderWithReset("Upper Eyelid", &shape.upperEyelid, 0.0f, 1.0f);
    sliderWithReset("Lower Eyelid", &shape.lowerEyelid, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Eyebrows");
    
    sliderWithReset("Brow Height", &shape.browHeight, 0.0f, 1.0f);
    sliderWithReset("Brow Thickness", &shape.browThickness, 0.0f, 1.0f);
    sliderWithReset("Brow Angle", &shape.browAngle, 0.0f, 1.0f);
    sliderWithReset("Brow Curve", &shape.browCurve, 0.0f, 1.0f);
}

void CharacterCreatorUI::renderFaceNosePanel() {
    auto& face = character_->getFace();
    auto& shape = face.getShapeParams();
    
    ImGui::Text("Nose");
    
    sliderWithReset("Length", &shape.noseLength, 0.0f, 1.0f);
    sliderWithReset("Width", &shape.noseWidth, 0.0f, 1.0f);
    sliderWithReset("Height", &shape.noseHeight, 0.0f, 1.0f);
    sliderWithReset("Bridge", &shape.noseBridge, 0.0f, 1.0f);
    sliderWithReset("Bridge Curve", &shape.noseBridgeCurve, 0.0f, 1.0f);
    sliderWithReset("Tip Shape", &shape.noseTip, 0.0f, 1.0f);
    sliderWithReset("Tip Angle", &shape.noseTipAngle, 0.0f, 1.0f);
    sliderWithReset("Nostril Width", &shape.nostrilWidth, 0.0f, 1.0f);
    sliderWithReset("Nostril Flare", &shape.nostrilFlare, 0.0f, 1.0f);
}

void CharacterCreatorUI::renderFaceMouthPanel() {
    auto& face = character_->getFace();
    auto& shape = face.getShapeParams();
    
    ImGui::Text("Mouth");
    
    sliderWithReset("Width", &shape.mouthWidth, 0.0f, 1.0f);
    sliderWithReset("Height Position", &shape.mouthHeight, 0.0f, 1.0f);
    sliderWithReset("Upper Lip", &shape.upperLipThickness, 0.0f, 1.0f);
    sliderWithReset("Lower Lip", &shape.lowerLipThickness, 0.0f, 1.0f);
    sliderWithReset("Lip Protrusion", &shape.lipProtrusion, 0.0f, 1.0f);
    sliderWithReset("Mouth Corners", &shape.mouthCorners, 0.0f, 1.0f);
    sliderWithReset("Lip Curve", &shape.lipCurve, 0.0f, 1.0f);
    sliderWithReset("Philtrum", &shape.philtrum, 0.0f, 1.0f);
}

void CharacterCreatorUI::renderFaceTexturePanel() {
    auto& face = character_->getFace();
    auto& tex = face.getTextureParams();
    
    ImGui::Text("Skin");
    
    float skinColor[3] = {tex.skinTone.x, tex.skinTone.y, tex.skinTone.z};
    if (ImGui::ColorEdit3("Skin Tone", skinColor)) {
        tex.skinTone = Vec3(skinColor[0], skinColor[1], skinColor[2]);
        character_->matchSkinColors();
    }
    
    ImGui::SliderFloat("Wrinkles", &tex.wrinkles, 0.0f, 1.0f);
    ImGui::SliderFloat("Freckles", &tex.freckles, 0.0f, 1.0f);
    ImGui::SliderFloat("Pores", &tex.pores, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Eyes");
    
    float eyeColor[3] = {tex.eyeColor.x, tex.eyeColor.y, tex.eyeColor.z};
    if (ImGui::ColorEdit3("Eye Color", eyeColor)) {
        tex.eyeColor = Vec3(eyeColor[0], eyeColor[1], eyeColor[2]);
    }
    
    // Eye color presets
    ImGui::Text("Presets:");
    struct EyePreset { const char* name; float r, g, b; };
    EyePreset eyePresets[] = {
        {"Brown", 0.35f, 0.22f, 0.12f},
        {"Hazel", 0.45f, 0.35f, 0.20f},
        {"Green", 0.30f, 0.50f, 0.30f},
        {"Blue", 0.30f, 0.45f, 0.65f},
        {"Gray", 0.45f, 0.50f, 0.55f},
        {"Amber", 0.60f, 0.45f, 0.20f}
    };
    
    for (const auto& preset : eyePresets) {
        ImVec4 color(preset.r, preset.g, preset.b, 1.0f);
        if (ImGui::ColorButton(preset.name, color, 0, ImVec2(25, 25))) {
            tex.eyeColor = Vec3(preset.r, preset.g, preset.b);
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
    
    ImGui::Separator();
    ImGui::Text("Eyebrows");
    
    float browColor[3] = {tex.eyebrowColor.x, tex.eyebrowColor.y, tex.eyebrowColor.z};
    ImGui::ColorEdit3("Eyebrow Color", browColor);
    tex.eyebrowColor = Vec3(browColor[0], browColor[1], browColor[2]);
    
    ImGui::SliderFloat("Eyebrow Density", &tex.eyebrowDensity, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Lips");
    
    float lipColor[3] = {tex.lipColor.x, tex.lipColor.y, tex.lipColor.z};
    ImGui::ColorEdit3("Lip Color", lipColor);
    tex.lipColor = Vec3(lipColor[0], lipColor[1], lipColor[2]);
    
    ImGui::SliderFloat("Lip Moisture", &tex.lipMoisture, 0.0f, 1.0f);
}

void CharacterCreatorUI::renderFaceExpressionPanel() {
    auto& face = character_->getFace();
    
    ImGui::Text("Expression Presets");
    
    if (ImGui::Button("Neutral")) face.setExpression("neutral");
    ImGui::SameLine();
    if (ImGui::Button("Smile")) face.setExpression("smile");
    ImGui::SameLine();
    if (ImGui::Button("Frown")) face.setExpression("frown");
    
    if (ImGui::Button("Surprise")) face.setExpression("surprise");
    ImGui::SameLine();
    if (ImGui::Button("Angry")) face.setExpression("angry");
    
    ImGui::Separator();
    ImGui::Text("Manual Controls");
    
    auto& exp = face.getExpressionParams();
    
    // Simplified expression controls
    if (ImGui::TreeNode("Eyes")) {
        ImGui::SliderFloat("Blink L", &exp.eyeBlinkLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Blink R", &exp.eyeBlinkRight, 0.0f, 1.0f);
        ImGui::SliderFloat("Wide L", &exp.eyeWideLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Wide R", &exp.eyeWideRight, 0.0f, 1.0f);
        ImGui::SliderFloat("Squint L", &exp.eyeSquintLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Squint R", &exp.eyeSquintRight, 0.0f, 1.0f);
        ImGui::TreePop();
    }
    
    if (ImGui::TreeNode("Mouth")) {
        ImGui::SliderFloat("Smile L", &exp.mouthSmileLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Smile R", &exp.mouthSmileRight, 0.0f, 1.0f);
        ImGui::SliderFloat("Frown L", &exp.mouthFrownLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Frown R", &exp.mouthFrownRight, 0.0f, 1.0f);
        ImGui::SliderFloat("Open", &exp.jawOpen, 0.0f, 1.0f);
        ImGui::SliderFloat("Pucker", &exp.mouthPucker, 0.0f, 1.0f);
        ImGui::TreePop();
    }
    
    if (ImGui::TreeNode("Brows")) {
        ImGui::SliderFloat("Down L", &exp.browDownLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Down R", &exp.browDownRight, 0.0f, 1.0f);
        ImGui::SliderFloat("Inner Up", &exp.browInnerUp, 0.0f, 1.0f);
        ImGui::SliderFloat("Outer Up L", &exp.browOuterUpLeft, 0.0f, 1.0f);
        ImGui::SliderFloat("Outer Up R", &exp.browOuterUpRight, 0.0f, 1.0f);
        ImGui::TreePop();
    }
}

void CharacterCreatorUI::renderClothingTab() {
    auto& clothing = character_->getClothing();
    
    ImGui::Text("Clothing");
    
    // Category tabs
    auto categories = clothing.getCategories();
    if (categories.empty()) {
        ImGui::Text("No clothing items available.");
        ImGui::Text("Import clothing packs to get started.");
        return;
    }
    
    static int selectedCategory = 0;
    if (ImGui::BeginCombo("Category", categories.empty() ? "None" : categories[selectedCategory].c_str())) {
        for (int i = 0; i < (int)categories.size(); i++) {
            if (ImGui::Selectable(categories[i].c_str(), selectedCategory == i)) {
                selectedCategory = i;
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::Separator();
    
    // List items in category
    if (selectedCategory < (int)categories.size()) {
        auto items = clothing.getItemsByCategory(categories[selectedCategory]);
        
        for (const auto* item : items) {
            bool equipped = clothing.isEquipped(item->id);
            
            if (ImGui::Checkbox(item->name.c_str(), &equipped)) {
                if (equipped) {
                    clothing.equipItem(item->id);
                } else {
                    clothing.unequipItem(item->id);
                }
            }
            
            // Color adjustment
            if (equipped && item->colorAdjustable) {
                ImGui::SameLine();
                float color[3] = {item->baseColor.x, item->baseColor.y, item->baseColor.z};
                std::string colorId = "##color_" + item->id;
                if (ImGui::ColorEdit3(colorId.c_str(), color, ImGuiColorEditFlags_NoInputs)) {
                    clothing.setItemColor(item->id, Vec3(color[0], color[1], color[2]));
                }
            }
        }
    }
}

void CharacterCreatorUI::renderAnimationTab() {
    auto& animState = character_->getAnimationState();
    
    ImGui::Text("Animation");
    
    ImGui::Separator();
    ImGui::Text("Pose");
    
    if (ImGui::Button("T-Pose")) character_->setPose("t_pose");
    ImGui::SameLine();
    if (ImGui::Button("A-Pose")) character_->setPose("a_pose");
    ImGui::SameLine();
    if (ImGui::Button("Idle")) character_->setPose("idle");
    
    ImGui::Separator();
    ImGui::Text("Animation Playback");
    
    // Animation list would come from animation library
    const char* animations[] = {"None", "Idle", "Walk", "Run", "Wave"};
    static int currentAnim = 0;
    
    if (ImGui::Combo("Animation", &currentAnim, animations, 5)) {
        if (currentAnim == 0) {
            character_->stopAnimation();
        } else {
            character_->playAnimation(animations[currentAnim]);
        }
    }
    
    ImGui::Checkbox("Loop", &animState.animationLooping);
    ImGui::SliderFloat("Speed", &animState.animationSpeed, 0.0f, 2.0f);
    
    if (!animState.currentAnimation.empty()) {
        ImGui::Text("Playing: %s", animState.currentAnimation.c_str());
        ImGui::Text("Time: %.2f", animState.animationTime);
        
        if (ImGui::Button("Stop")) {
            character_->stopAnimation();
            currentAnim = 0;
        }
    }
}

void CharacterCreatorUI::renderExportTab() {
    ImGui::Text("Export Character");
    
    // Name
    static char nameBuf[256];
    strncpy(nameBuf, character_->getName().c_str(), sizeof(nameBuf) - 1);
    if (ImGui::InputText("Character Name", nameBuf, sizeof(nameBuf))) {
        character_->setName(nameBuf);
    }
    
    ImGui::Separator();
    
    // Export format
    const char* formats[] = {"glTF (.glb)", "FBX", "OBJ", "USD", "VRM", "LUMA"};
    ImGui::Combo("Format", &exportFormatIndex_, formats, 6);
    
    // Export path
    static char pathBuf[512] = "character.glb";
    ImGui::InputText("Output Path", pathBuf, sizeof(pathBuf));
    
    ImGui::Separator();
    
    // Export options
    static bool exportTextures = true;
    static bool exportSkeleton = true;
    static bool exportBlendShapes = true;
    static bool exportAnimations = false;
    
    ImGui::Checkbox("Include Textures", &exportTextures);
    ImGui::Checkbox("Include Skeleton", &exportSkeleton);
    ImGui::Checkbox("Include Blend Shapes", &exportBlendShapes);
    ImGui::Checkbox("Include Animations", &exportAnimations);
    
    ImGui::Separator();
    
    if (ImGui::Button("Export", ImVec2(120, 30))) {
        CharacterExportFormat format = static_cast<CharacterExportFormat>(exportFormatIndex_);
        if (onExport_) {
            onExport_(pathBuf, format);
        } else {
            character_->exportTo(pathBuf, format);
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Save Preset", ImVec2(120, 30))) {
        // Save current settings as preset
    }
}

void CharacterCreatorUI::renderPreviewWindow() {
    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Character Preview", &showPreview_)) {
        // This would typically be a 3D viewport
        // For now, just show some stats
        
        ImGui::Text("Character: %s", character_->getName().c_str());
        
        auto& body = character_->getBody();
        auto& face = character_->getFace();
        
        ImGui::Separator();
        ImGui::Text("Body:");
        ImGui::Text("  Gender: %s", 
            body.getParams().gender == Gender::Male ? "Male" : 
            body.getParams().gender == Gender::Female ? "Female" : "Neutral");
        ImGui::Text("  Height: %.0f%%", body.getParams().measurements.height * 100);
        ImGui::Text("  Weight: %.0f%%", body.getParams().measurements.weight * 100);
        
        ImGui::Separator();
        ImGui::Text("Face:");
        ImGui::Text("  Width: %.0f%%", face.getShapeParams().faceWidth * 100);
        ImGui::Text("  Length: %.0f%%", face.getShapeParams().faceLength * 100);
        
        ImGui::Separator();
        ImGui::Text("Clothing: %zu items", character_->getClothing().getEquippedItems().size());
        
        if (config_.showDebugInfo) {
            ImGui::Separator();
            ImGui::Text("Debug:");
            ImGui::Text("  BlendShape targets: %zu", character_->getBlendShapeMesh().getTargetCount());
            ImGui::Text("  BlendShape channels: %zu", character_->getBlendShapeMesh().getChannelCount());
            ImGui::Text("  Skeleton bones: %d", character_->getSkeleton().getBoneCount());
        }
    }
    ImGui::End();
}

bool CharacterCreatorUI::sliderWithReset(const char* label, float* value, float min, float max, float defaultValue) {
    bool changed = false;
    
    ImGui::PushID(label);
    
    // Slider
    ImGui::SetNextItemWidth(config_.sliderWidth);
    changed = ImGui::SliderFloat("##slider", value, min, max);
    
    // Reset button
    ImGui::SameLine();
    if (ImGui::Button("R")) {
        *value = defaultValue;
        changed = true;
    }
    
    // Label
    ImGui::SameLine();
    ImGui::Text("%s", label);
    
    ImGui::PopID();
    
    return changed;
}

void CharacterCreatorUI::colorPickerWithPresets(const char* label, float* color, 
                                                  const std::vector<std::array<float, 3>>& presets) {
    ImGui::ColorEdit3(label, color);
    
    if (!presets.empty()) {
        ImGui::Text("Presets:");
        for (size_t i = 0; i < presets.size(); i++) {
            ImVec4 c(presets[i][0], presets[i][1], presets[i][2], 1.0f);
            std::string id = std::string("##preset") + std::to_string(i);
            if (ImGui::ColorButton(id.c_str(), c, 0, ImVec2(25, 25))) {
                color[0] = presets[i][0];
                color[1] = presets[i][1];
                color[2] = presets[i][2];
            }
            if ((i + 1) % 6 != 0) ImGui::SameLine();
        }
    }
}

void CharacterCreatorUI::presetGrid(const std::vector<std::string>& names, 
                                     const std::string& currentPreset,
                                     std::function<void(const std::string&)> onSelect) {
    int columns = static_cast<int>(config_.panelWidth / (config_.thumbnailSize + 10));
    columns = std::max(columns, 1);
    
    for (size_t i = 0; i < names.size(); i++) {
        if (i % columns != 0) ImGui::SameLine();
        
        bool selected = (names[i] == currentPreset);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        
        if (ImGui::Button(names[i].c_str(), ImVec2(config_.thumbnailSize, config_.thumbnailSize))) {
            if (onSelect) onSelect(names[i]);
        }
        
        if (selected) ImGui::PopStyleColor();
    }
}

#endif // LUMA_CHARACTER_CREATOR_UI_IMPLEMENTATION

} // namespace luma
