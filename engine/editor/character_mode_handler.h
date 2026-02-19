// LUMA Character Mode Handler
// Handles character creation, face sculpting, body customization, clothing, and expressions
// Integrates Character system with editor mode architecture

#pragma once

#include "engine/editor/mode_handler.h"
#include "engine/editor/mode_ui.h"
#include "engine/editor/input_system.h"
#include "engine/character/character.h"
#include "engine/character/character_renderer.h"
#include "engine/character/character_face.h"
#include "engine/character/character_body.h"
#include "engine/character/character_templates.h"
#include "engine/character/character_template.h"
#include "engine/character/blend_shape.h"
#include "engine/character/base_human_loader.h"
#include "engine/character/hair_system.h"
#include "engine/character/clothing_system.h"
#include "engine/character/expression_presets.h"
#include "engine/character/ai/face_reconstruction.h"
#include "engine/character/landmark_deformer.h"
#include "engine/character/identity_fitter.h"
#include "engine/character/mesh_landmark_editor.h"
#include "engine/editor/character_mode/photo_import.h"
#include "engine/scene/scene_graph.h"
#include "engine/scene/entity.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/ui/localization.h"
#include "imgui.h"
#include <memory>
#include <string>
#include <functional>
#include <cstdlib>  // for _fullpath on Windows

namespace luma {
namespace editor {

// ============================================================================
// Character Edit Sub-Modes
// ============================================================================

enum class CharacterSubMode {
    Overview,       // Template selection, creation workflow
    Body,           // Body proportions and measurements
    Face,           // Face sculpting with sliders
    Hair,           // Hair style and color
    Clothing,       // Clothing selection and customization
    Expression,     // Expression presets and blend shapes
    Export          // Export options
};

// ============================================================================
// Face Edit Region (for grouped UI and viewport interaction)
// ============================================================================

enum class FaceEditRegion {
    All,
    Forehead,
    Eyes,
    Eyebrows,
    Nose,
    Mouth,
    Chin,
    Jaw,
    Cheeks,
    Ears
};

// ============================================================================
// Character Creation Method
// ============================================================================

enum class CharacterCreationMethod {
    None,
    Template,       // From a built-in template
    Photo,          // From a photo (AI pipeline)
    Preset,         // From a preset
    Random,         // Random generation
    Blank           // Blank character
};

// ============================================================================
// Photo Import State
// ============================================================================

struct PhotoImportState {
    bool showDialog = false;
    bool processing = false;
    bool completed = false;
    bool failed = false;

    // Multi-photo slots (MetaHuman-style: front + left + right)
    PhotoSlot photos[3];
    int activeSlot = 0; // Currently selected slot in UI

    // Legacy single-photo path (for backward compatibility)
    std::string photoPath;

    PhotoFaceResult result;
    PhotoImportResult multiPhotoResult;
    std::string errorMessage;
    float progress = 0.0f;

    void reset() {
        showDialog = false;
        processing = completed = failed = false;
        progress = 0.0f;
        errorMessage.clear();
        for (auto& p : photos) p.clear();
        photos[0].viewType = PhotoSlot::Front;
        photos[1].viewType = PhotoSlot::LeftProfile;
        photos[2].viewType = PhotoSlot::RightProfile;
        activeSlot = 0;
    }
};

// ============================================================================
// Character Edit State
// ============================================================================

struct CharacterEditState {
    // Active character
    std::unique_ptr<Character> character;
    CharacterRenderer renderer;
    
    // Creation state
    CharacterCreationMethod creationMethod = CharacterCreationMethod::None;
    bool characterCreated = false;
    
    // Editing state
    CharacterSubMode subMode = CharacterSubMode::Overview;
    FaceEditRegion faceRegion = FaceEditRegion::All;
    
    // Photo import
    PhotoImportState photoImport;
    
    // AI pipeline
    PhotoToFacePipeline facePipeline;
    bool pipelineInitialized = false;
    bool hasAIModels = false;
    std::string modelDirectory = "models";  // Path to AI models and BFM data
    
    // Landmark visualization (for debugging - shows 68 points on mesh)
    LandmarkDeformer landmarkDeformer;  // Used only for getCurrentLandmarkPositions()
    bool landmarkDeformerInitialized = false;
    bool showLandmarks = false;
    
    // Identity Fitter (MetaHuman-style optimization)
    IdentityFitter identityFitter;
    bool identityFitterInitialized = false;
    
    // Base topology comparison
    bool showBaseTopo = false;  // When true, show base topology (all params = 0)
    FaceShapeParams savedParams;  // Saved params when comparing
    
    // Landmark editor for marking mesh vertices
    MeshLandmarkEditor landmarkEditor;
    RHILoadedModel landmarkEditorModel;  // GPU model for PBR rendering
    bool landmarkEditorModelLoaded = false;
    
    // Template selection
    std::string selectedTemplate = "human";
    CharacterStyle selectedStyle = CharacterStyle::Realistic;
    
    // Expression editing
    std::string currentExpression = "neutral";
    float expressionIntensity = 1.0f;
    
    // Hair editing
    int selectedHairStyle = 0;
    int selectedHairColor = 0;
    
    // Clothing editing
    std::string selectedClothingCategory = "top";
    
    // Comparison
    bool showComparison = false;
    FaceShapeParams comparisonParams;
    
    // Dirty flags
    bool meshNeedsUpdate = false;
    bool gpuNeedsUpdate = false;
    
    // GPU mesh handle (for rendering)
    bool hasGPUMesh = false;
    
    // Preset library
    FacePresetLibrary facePresets;
    
    // Undo stack for face parameters
    std::vector<FaceShapeParams> faceUndoStack;
    std::vector<FaceShapeParams> faceRedoStack;
    static constexpr int kMaxUndo = 50;
    
    void pushFaceUndo() {
        if (character) {
            faceUndoStack.push_back(character->getFace().getShapeParams());
            if (faceUndoStack.size() > kMaxUndo) {
                faceUndoStack.erase(faceUndoStack.begin());
            }
            faceRedoStack.clear();
        }
    }
    
    bool undoFace() {
        if (faceUndoStack.empty() || !character) return false;
        faceRedoStack.push_back(character->getFace().getShapeParams());
        character->getFace().setShapeParams(faceUndoStack.back());
        faceUndoStack.pop_back();
        meshNeedsUpdate = true;
        return true;
    }
    
    bool redoFace() {
        if (faceRedoStack.empty() || !character) return false;
        faceUndoStack.push_back(character->getFace().getShapeParams());
        character->getFace().setShapeParams(faceRedoStack.back());
        faceRedoStack.pop_back();
        meshNeedsUpdate = true;
        return true;
    }
};

// ============================================================================
// Character Mode Handler
// ============================================================================

class CharacterModeHandler : public EditorModeHandler {
public:
    CharacterModeHandler();
    ~CharacterModeHandler() override;
    
    // Initialize with context
    bool init(ModeHandlerContext* ctx);
    
    // ===== EditorModeHandler interface =====
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render(const RenderContext& ctx) override;
    void renderUI() override;
    bool handleInput(const InputEvent& event) override;
    
    // ===== Character Mode specific =====
    
    // Sub-mode management
    void setSubMode(CharacterSubMode subMode);
    CharacterSubMode getSubMode() const { return m_state.subMode; }
    
    // Character access
    Character* getCharacter() { return m_state.character.get(); }
    const Character* getCharacter() const { return m_state.character.get(); }
    CharacterEditState& getState() { return m_state; }
    
    // Character creation
    void createFromTemplate(const std::string& templateName, CharacterStyle style);
    void createFromPhoto(const std::string& photoPath);
    void createFromPreset(const std::string& presetName);
    void createRandom(unsigned int seed = 0);
    void createBlank();
    
    // Photo import
    void openPhotoImportDialog();
    void processPhotoImport();
    
    // Apply character to scene entity
    void applyToSceneEntity();
    
    // Callbacks
    using CharacterChangedCallback = std::function<void()>;
    void setCharacterChangedCallback(CharacterChangedCallback cb) { m_characterChangedCallback = cb; }
    
    // Projection helpers (set by main app)
    using ProjectionCallback = std::function<bool(float wx, float wy, float wz, float& sx, float& sy)>;
    using RayCallback = std::function<luma::Ray(float screenX, float screenY)>;
    
    void setProjectionCallback(ProjectionCallback cb) { m_projectionCallback = cb; }
    void setRayCallback(RayCallback cb) { m_rayCallback = cb; }
    
private:
    ModeHandlerContext* m_ctx = nullptr;
    CharacterEditState m_state;
    
    // Callbacks
    CharacterChangedCallback m_characterChangedCallback;
    ProjectionCallback m_projectionCallback;
    RayCallback m_rayCallback;
    
    // Internal helpers
    void initializePipeline();
    void initializeIdentityFitter();
    void updateCharacterMesh();
    void uploadCharacterToGPU();
    
    // UI rendering
    void renderOverviewUI();
    void renderBodyUI();
    void renderFaceUI();
    void renderHairUI();
    void renderClothingUI();
    void renderExpressionUI();
    void renderExportUI();
    void renderCharacterToolbar();
    void renderFaceRegionSliders(FaceEditRegion region);
    
    // Landmark editor overlay (screen-space points)
    void renderLandmarkEditorOverlay();
    bool projectToScreen(float wx, float wy, float wz, float& sx, float& sy);
    
    // Landmark visualization (68 points on deformed mesh)
    void renderLandmarkVisualization();
    
    // Helper: face parameter slider with undo
    bool faceSlider(const char* label, float* value, float min = 0.0f, float max = 1.0f);
    bool faceSliderWithReset(const char* label, float* value, float defaultVal = 0.5f);
};

// ============================================================================
// Implementation
// ============================================================================

inline CharacterModeHandler::CharacterModeHandler()
    : EditorModeHandler(EditorMode::Character, "Character") {
    // Initialize face presets
    m_state.facePresets.initializeDefaults();
}

inline CharacterModeHandler::~CharacterModeHandler() = default;

inline bool CharacterModeHandler::init(ModeHandlerContext* ctx) {
    m_ctx = ctx;
    if (!ctx || !ctx->renderer) return false;
    
    // Initialize character renderer
    m_state.renderer.initialize(ctx->renderer);
    
    return true;
}

inline void CharacterModeHandler::initializePipeline() {
    if (m_state.pipelineInitialized) return;

    // Find models directory: try multiple candidate paths
    // (exe may run from build/, project root, or elsewhere)
    auto checkFile = [](const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (f) { fclose(f); return true; }
        return false;
    };

    // Get absolute path to models directory (important because CWD can change)
    m_state.modelDirectory = "models";
    const char* candidates[] = {
        "models",           // project root (normal case)
        "../models",        // from build/ directory
        "../../models",     // from build/Release/ etc.
    };
    for (const char* dir : candidates) {
        std::string testPath = std::string(dir) + "/det_10g.onnx";
        if (checkFile(testPath)) {
            // Convert to absolute path
            char absPath[1024];
            if (_fullpath(absPath, dir, sizeof(absPath))) {
                m_state.modelDirectory = absPath;
                printf("[CharacterMode] Found models directory: %s (absolute: %s)\n", dir, absPath);
            } else {
                m_state.modelDirectory = dir;
                printf("[CharacterMode] Found models directory: %s\n", dir);
            }
            break;
        }
    }

    PhotoToFacePipeline::Config config;
    config.modelDirectory = m_state.modelDirectory;
    config.faceDetectorModel = "det_10g.onnx";
    config.landmarkModel = "2d106det.onnx";
    config.tddfaModel = "mb1_120x120.onnx";
    config.textureSize = 512;
    config.extractTexture = true;

    m_state.facePipeline.initialize(config);
    m_state.pipelineInitialized = true;

    // Check if AI models are available
    {
        bool hasDetector = checkFile(m_state.modelDirectory + "/det_10g.onnx");
        bool hasLandmark = checkFile(m_state.modelDirectory + "/2d106det.onnx");
        bool hasTddfa = checkFile(m_state.modelDirectory + "/mb1_120x120.onnx");
        bool hasBFM = checkFile(m_state.modelDirectory + "/bfm_mean_face.bin");
        m_state.hasAIModels = hasDetector && hasLandmark && hasTddfa;
        if (!m_state.hasAIModels) {
            printf("[CharacterMode] WARNING: AI models not found.\n");
            printf("[CharacterMode]   Searched: %s/\n", m_state.modelDirectory.c_str());
            printf("[CharacterMode]   Face reconstruction will use basic fallback mode.\n");
            printf("[CharacterMode]   For accurate results, download models per models/README.md\n");
            if (!hasDetector) printf("[CharacterMode]   Missing: det_10g.onnx\n");
            if (!hasLandmark) printf("[CharacterMode]   Missing: 2d106det.onnx\n");
            if (!hasTddfa) printf("[CharacterMode]   Missing: mb1_120x120.onnx\n");
        } else {
            printf("[CharacterMode] All 3 AI models loaded from %s/\n", m_state.modelDirectory.c_str());
        }
        if (hasBFM) {
            printf("[CharacterMode] BFM face model found - will use high-quality face mesh\n");
        }
    }

    // Initialize photo slots
    m_state.photoImport.reset();
}

inline void CharacterModeHandler::onEnter() {
    m_active = true;
    
    // Initialize pipeline on first entry
    initializePipeline();
    
    // If no character exists, go to overview for creation
    if (!m_state.characterCreated) {
        m_state.subMode = CharacterSubMode::Overview;
    }
    
    printf("[CharacterMode] Entered character mode\n");
}

inline void CharacterModeHandler::onExit() {
    m_active = false;
    
    // Apply character changes to scene entity if we have one
    if (m_state.characterCreated) {
        applyToSceneEntity();
    }
    
    printf("[CharacterMode] Exited character mode\n");
}

inline void CharacterModeHandler::update(float deltaTime) {
    if (!m_active) return;
    
    // Update character if it exists
    if (m_state.character) {
        m_state.character->update(deltaTime);
        
        // Update blend shape mesh if parameters changed
        if (m_state.meshNeedsUpdate) {
            updateCharacterMesh();
            m_state.meshNeedsUpdate = false;
            m_state.gpuNeedsUpdate = true;
        }
        
        // Upload to GPU if needed
        if (m_state.gpuNeedsUpdate) {
            uploadCharacterToGPU();
            m_state.gpuNeedsUpdate = false;
        }
    }
    
    // Process async photo import
    if (m_state.photoImport.processing) {
        processPhotoImport();
    }
}

inline void CharacterModeHandler::render(const RenderContext& ctx) {
    if (!m_active || !m_ctx || !m_ctx->renderer) return;
    
    // Render landmark editor (independent of character creation)
    if (m_state.landmarkEditor.isActive()) {
        const auto& opts = m_state.landmarkEditor.getDisplayOptions();
        const auto& vertices = m_state.landmarkEditor.getVertices();
        float landmarkSize = opts.landmarkSize;
        
        std::vector<float> gizmoLines;  // {x0,y0,z0, x1,y1,z1, r,g,b,a} per line
        
        // ========== 渲染 PBR 网格（如果启用）==========
        if (opts.showMesh && m_state.landmarkEditorModelLoaded) {
            // 使用已加载的模型进行 PBR 渲染
            float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            
            if (opts.meshTransparent) {
                // 半透明模式 - 渲染为灰色实体
                float solidColor[4] = {0.5f, 0.5f, 0.5f, opts.meshOpacity};
                m_ctx->renderer->renderModelSolid(m_state.landmarkEditorModel, identity, solidColor);
            } else {
                // 完整 PBR 渲染
                m_ctx->renderer->renderModel(m_state.landmarkEditorModel, identity);
            }
        }
        
        // 顶点点云在 renderLandmarkEditorOverlay() 中用 ImGui 屏幕空间渲染
        
        // Landmark 和顶点都在 renderLandmarkEditorOverlay() 中用 ImGui 屏幕空间渲染
        // 不再使用 3D 空间的十字标记，避免遮挡 hover 检测
        
        // Render all lines at once
        if (!gizmoLines.empty()) {
            uint32_t lineCount = static_cast<uint32_t>(gizmoLines.size() / 10);
            m_ctx->renderer->renderGizmoLines(gizmoLines.data(), lineCount);
        }
        
        return;  // Don't render character stuff when in landmark editor mode
    }
    
    // Character rendering is handled by the main scene render loop
    // since the character is an entity in the scene.
    // Here we just render overlays (face region highlights, wireframe, etc.)
    
    if (!m_state.characterCreated || !m_state.character) return;
    
    // Setup face region highlighting based on current editing region
    if (m_state.subMode == CharacterSubMode::Face) {
        CharacterRenderer::FaceRegionHighlight highlight;
        highlight.enabled = true;
        highlight.region = static_cast<int>(m_state.faceRegion);
        m_state.renderer.setFaceRegionHighlight(highlight);
        
        // Enable wireframe overlay in face sculpting
        CharacterRenderer::WireframeOverlay wireOverlay;
        wireOverlay.enabled = true;
        wireOverlay.color[0] = 0.3f; wireOverlay.color[1] = 0.4f;
        wireOverlay.color[2] = 0.5f; wireOverlay.color[3] = 0.3f;
        m_state.renderer.setWireframeOverlay(wireOverlay);
    } else {
        CharacterRenderer::FaceRegionHighlight noHighlight;
        noHighlight.enabled = false;
        m_state.renderer.setFaceRegionHighlight(noHighlight);
        
        CharacterRenderer::WireframeOverlay noWire;
        noWire.enabled = false;
        m_state.renderer.setWireframeOverlay(noWire);
    }
    
    // Sync eye and skin rendering from character parameters
    if (m_state.character) {
        auto& texParams = m_state.character->getFace().getTextureParams();
        
        CharacterRenderer::SkinRenderParams skin;
        skin.sssEnabled = true;
        skin.sssStrength = texParams.skinSubsurface;
        skin.skinRoughness = texParams.skinRoughness;
        m_state.renderer.setSkinParams(skin);
        
        CharacterRenderer::EyeRenderParams eyes;
        eyes.irisColor = texParams.eyeColor;
        eyes.scleraColor = texParams.scleraColor;
        eyes.pupilSize = texParams.pupilSize;
        m_state.renderer.setEyeParams(eyes);
    }
    
    // Render landmark visualization if enabled
    if (m_state.showLandmarks && m_state.landmarkDeformerInitialized) {
        renderLandmarkVisualization();
    }
}

// Render 68 facial landmarks as colored points on the mesh
inline void CharacterModeHandler::renderLandmarkVisualization() {
    if (!m_ctx || !m_ctx->renderer) return;
    
    // Get current landmark positions from the base mesh
    const auto& currentVertices = m_state.renderer.getBaseMesh();
    if (currentVertices.empty()) return;
    
    auto positions = m_state.landmarkDeformer.getCurrentLandmarkPositions(currentVertices);
    
    // Define colors for different landmark regions
    // Jaw contour (0-16): Red
    // Left eyebrow (17-21): Light green
    // Right eyebrow (22-26): Dark green
    // Nose bridge (27-30): Light blue
    // Nose bottom (31-35): Dark blue
    // Left eye (36-41): Yellow
    // Right eye (42-47): Orange
    // Outer lip (48-59): Purple
    // Inner lip (60-67): Pink
    
    auto getColor = [](int idx) -> std::array<float, 4> {
        if (idx <= 16) return {1.0f, 0.2f, 0.2f, 1.0f};       // Jaw: Red
        if (idx <= 21) return {0.4f, 1.0f, 0.4f, 1.0f};       // Left brow: Light green
        if (idx <= 26) return {0.2f, 0.7f, 0.2f, 1.0f};       // Right brow: Dark green
        if (idx <= 30) return {0.4f, 0.6f, 1.0f, 1.0f};       // Nose bridge: Light blue
        if (idx <= 35) return {0.2f, 0.3f, 0.9f, 1.0f};       // Nose bottom: Dark blue
        if (idx <= 41) return {1.0f, 1.0f, 0.2f, 1.0f};       // Left eye: Yellow
        if (idx <= 47) return {1.0f, 0.6f, 0.2f, 1.0f};       // Right eye: Orange
        if (idx <= 59) return {0.8f, 0.2f, 0.8f, 1.0f};       // Outer lip: Purple
        return {1.0f, 0.5f, 0.7f, 1.0f};                       // Inner lip: Pink
    };
    
    // Render each landmark as a small cross (3D gizmo lines)
    std::vector<float> gizmoLines;
    float crossSize = 0.003f;  // Size of the cross marker
    
    for (int i = 0; i < 68; i++) {
        const Vec3& pos = positions[i];
        if (pos.x == 0 && pos.y == 0 && pos.z == 0) continue;  // Skip invalid
        
        auto color = getColor(i);
        
        // X-axis line
        gizmoLines.insert(gizmoLines.end(), {
            pos.x - crossSize, pos.y, pos.z,
            pos.x + crossSize, pos.y, pos.z,
            color[0], color[1], color[2], color[3]
        });
        
        // Y-axis line
        gizmoLines.insert(gizmoLines.end(), {
            pos.x, pos.y - crossSize, pos.z,
            pos.x, pos.y + crossSize, pos.z,
            color[0], color[1], color[2], color[3]
        });
        
        // Z-axis line (pointing outward from face)
        gizmoLines.insert(gizmoLines.end(), {
            pos.x, pos.y, pos.z,
            pos.x, pos.y, pos.z + crossSize * 2,
            color[0], color[1], color[2], color[3]
        });
    }
    
    if (!gizmoLines.empty()) {
        uint32_t lineCount = static_cast<uint32_t>(gizmoLines.size() / 10);
        m_ctx->renderer->renderGizmoLines(gizmoLines.data(), lineCount);
    }
}

inline void CharacterModeHandler::renderUI() {
    if (!m_active) return;
    
    // Render the character toolbar at the top of the right panel
    renderCharacterToolbar();
    
    // Render sub-mode specific UI
    switch (m_state.subMode) {
        case CharacterSubMode::Overview:    renderOverviewUI(); break;
        case CharacterSubMode::Body:        renderBodyUI(); break;
        case CharacterSubMode::Face:        renderFaceUI(); break;
        case CharacterSubMode::Hair:        renderHairUI(); break;
        case CharacterSubMode::Clothing:    renderClothingUI(); break;
        case CharacterSubMode::Expression:  renderExpressionUI(); break;
        case CharacterSubMode::Export:      renderExportUI(); break;
    }
    
    // Render landmark editor if active
    std::string landmarkPath = m_state.modelDirectory + "/landmark_vertex_map.json";
    renderLandmarkEditorUI(m_state.landmarkEditor, landmarkPath);
    
    // Render landmark editor overlay (screen-space points)
    renderLandmarkEditorOverlay();
}

inline bool CharacterModeHandler::handleInput(const InputEvent& event) {
    if (!m_active || !m_ctx) return false;
    
    // Landmark editor input handling
    if (m_state.landmarkEditor.isActive()) {
        // 计算射线（用于 hover 和点击）
        auto calculateRay = [this](float mouseX, float mouseY, Vec3& outOrigin, Vec3& outDir) -> bool {
            if (!m_ctx || !m_ctx->renderer) return false;
            
            float viewportWidth = (float)m_ctx->renderer->getWidth();
            float viewportHeight = (float)m_ctx->renderer->getHeight();
            
            float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
            float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);
            
            Vec3 cameraPos = m_ctx->renderer->getCameraPosition();
            Vec3 cameraTarget = m_ctx->renderer->getCameraTarget();
            Vec3 cameraUp(0, 1, 0);
            
            Vec3 forward = (cameraTarget - cameraPos).normalized();
            Vec3 right = forward.cross(cameraUp).normalized();
            Vec3 up = right.cross(forward).normalized();
            
            float fov = 45.0f * 3.14159f / 180.0f;
            float aspect = viewportWidth / viewportHeight;
            float tanHalfFov = tanf(fov * 0.5f);
            
            outOrigin = cameraPos;
            outDir = (forward + right * (ndcX * tanHalfFov * aspect) + up * (ndcY * tanHalfFov)).normalized();
            return true;
        };
        
        // 鼠标移动时更新 hover
        if (event.type == InputEvent::Type::MouseMove) {
            if (!ImGui::GetIO().WantCaptureMouse) {
                Vec3 rayOrigin, rayDir;
                if (calculateRay(event.mouseX, event.mouseY, rayOrigin, rayDir)) {
                    Vec3 camPos = m_ctx->renderer->getCameraPosition();
                    m_state.landmarkEditor.updateHover(rayOrigin, rayDir, camPos);
                }
            }
        }
        
        // Keyboard shortcuts for landmark navigation
        if (event.type == InputEvent::Type::KeyDown) {
            if (event.key == VK_LEFT || event.key == 'A') {
                m_state.landmarkEditor.prevLandmark();
                return true;
            }
            if (event.key == VK_RIGHT || event.key == 'D') {
                m_state.landmarkEditor.nextLandmark();
                return true;
            }
            if (event.key == VK_UP || event.key == 'W') {
                // 跳到上一个未标记的
                m_state.landmarkEditor.nextUnmarked();
                return true;
            }
            if (event.key == VK_DELETE || event.key == 'X') {
                m_state.landmarkEditor.clearLandmark(m_state.landmarkEditor.getCurrentLandmark());
                return true;
            }
            if (event.key == VK_ESCAPE) {
                m_state.landmarkEditor.setActive(false);
                return true;
            }
            // 数字键快速跳转区域
            if (event.key >= '1' && event.key <= '9') {
                const char* regions[] = {"jaw", "chin", "left_eyebrow", "right_eyebrow", 
                                          "nose", "left_eye", "right_eye", "mouth", "mouth_inner"};
                int idx = event.key - '1';
                if (idx < 9) {
                    m_state.landmarkEditor.jumpToRegion(regions[idx]);
                    return true;
                }
            }
            // Tab 切换显示模式
            if (event.key == VK_TAB) {
                auto& opts = m_state.landmarkEditor.getDisplayOptions();
                opts.showVertexDots = !opts.showVertexDots;
                return true;
            }
        }
        
        // Mouse click for vertex picking (key == 0 means left button for MouseDown)
        if (event.type == InputEvent::Type::MouseDown && event.key == 0) {
            if (!ImGui::GetIO().WantCaptureMouse) {
                // 首先检查是否点击了已有的 landmark（用于选中它）
                int hoveredLm = m_state.landmarkEditor.getHoveredLandmark();
                if (hoveredLm >= 0) {
                    // 点击已有 landmark，选中它
                    m_state.landmarkEditor.setCurrentLandmark(hoveredLm);
                    printf("[LandmarkEditor] Selected landmark %d\n", hoveredLm);
                    return true;
                }
                
                // 否则，标记当前 landmark 到 hover 的顶点
                int hoveredVert = m_state.landmarkEditor.getHoveredVertex();
                if (hoveredVert >= 0) {
                    int currentLandmark = m_state.landmarkEditor.getCurrentLandmark();
                    m_state.landmarkEditor.setLandmarkVertex(currentLandmark, hoveredVert);
                    
                    // Auto advance to next landmark
                    m_state.landmarkEditor.nextLandmark();
                    
                    printf("[LandmarkEditor] Set landmark %d to vertex %d\n", currentLandmark, hoveredVert);
                    return true;
                }
                
                // 如果没有 hover，用传统的射线拾取
                Vec3 rayOrigin, rayDir;
                if (calculateRay(event.mouseX, event.mouseY, rayOrigin, rayDir)) {
                    int pickedVertex = m_state.landmarkEditor.pickVertex(rayOrigin, rayDir);
                    if (pickedVertex >= 0) {
                        int currentLandmark = m_state.landmarkEditor.getCurrentLandmark();
                        m_state.landmarkEditor.setLandmarkVertex(currentLandmark, pickedVertex);
                        m_state.landmarkEditor.nextLandmark();
                        printf("[LandmarkEditor] Set landmark %d to vertex %d\n", currentLandmark, pickedVertex);
                        return true;
                    }
                }
            }
        }
    }
    
    // Ctrl+Z / Ctrl+Y for face undo/redo
    if (event.type == InputEvent::Type::KeyDown) {
        if (event.isCtrl() && event.key == 'Z') {
            return m_state.undoFace();
        }
        if (event.isCtrl() && event.key == 'Y') {
            return m_state.redoFace();
        }
    }
    
    return false;
}

// ============================================================================
// Character Creation Methods
// ============================================================================

inline void CharacterModeHandler::createFromTemplate(const std::string& templateName, CharacterStyle style) {
    // Ensure pipeline is initialized (sets modelDirectory)
    initializePipeline();
    
    m_state.character = CharacterFactory::createBlank(templateName);
    m_state.character->setStyle(style);
    
    // Initialize skeleton and blend shapes
    m_state.character->initializeStandardSkeleton();
    m_state.character->connectBlendShapes();
    
    // Generate base mesh from template (use BFM if available)
    auto& library = BaseHumanModelLibrary::getInstance();
    library.initializeDefaults(m_state.modelDirectory);
    
    const BaseHumanModel* model = library.getModel("procedural_human");
    if (model) {
        m_state.character->setBaseMesh(model->vertices, model->indices);
        
        // Copy blend shapes from model to character
        auto& charBlendShapes = m_state.character->getBlendShapeMesh();
        const auto& modelBS = model->blendShapes;
        for (size_t i = 0; i < modelBS.getTargetCount(); i++) {
            const BlendShapeTarget* target = modelBS.getTarget(static_cast<int>(i));
            if (target) charBlendShapes.addTarget(*target);
        }
        for (size_t i = 0; i < modelBS.getChannelCount(); i++) {
            const BlendShapeChannel* channel = modelBS.getChannel(static_cast<int>(i));
            if (channel) charBlendShapes.addChannel(*channel);
        }
    }
    
    // Setup renderer
    m_state.renderer.setupCharacter(m_state.character.get());
    
    m_state.characterCreated = true;
    m_state.creationMethod = CharacterCreationMethod::Template;
    m_state.subMode = CharacterSubMode::Face;
    m_state.meshNeedsUpdate = true;
    
    printf("[CharacterMode] Created character from template: %s\n", templateName.c_str());
}

inline void CharacterModeHandler::createFromPhoto(const std::string& photoPath) {
    // First create a blank character
    createFromTemplate("Photo Character", CharacterStyle::Realistic);

    // Load front photo into slot 0
    m_state.photoImport.reset();
    m_state.photoImport.photoPath = photoPath;
    m_state.photoImport.photos[0].filePath = photoPath;
    m_state.photoImport.photos[0].viewType = PhotoSlot::Front;
    PhotoImportModule::loadPhotoSlot(m_state.photoImport.photos[0]);

    m_state.photoImport.processing = true;
    m_state.photoImport.progress = 0.0f;
    m_state.creationMethod = CharacterCreationMethod::Photo;

    printf("[CharacterMode] Processing photo: %s\n", photoPath.c_str());
}

inline void CharacterModeHandler::createFromPreset(const std::string& presetName) {
    createFromTemplate(presetName, CharacterStyle::Realistic);
    
    // Apply preset face parameters
    const auto* preset = m_state.facePresets.findPreset(presetName);
    if (preset) {
        m_state.character->getFace().setShapeParams(preset->shapeParams);
        m_state.character->getFace().setTextureParams(preset->textureParams);
    }
    
    m_state.creationMethod = CharacterCreationMethod::Preset;
    m_state.meshNeedsUpdate = true;
}

inline void CharacterModeHandler::createRandom(unsigned int seed) {
    createFromTemplate("Random Character", CharacterStyle::Realistic);
    m_state.character->randomize(seed);
    m_state.creationMethod = CharacterCreationMethod::Random;
    m_state.meshNeedsUpdate = true;
}

inline void CharacterModeHandler::createBlank() {
    createFromTemplate("New Character", CharacterStyle::Realistic);
    m_state.creationMethod = CharacterCreationMethod::Blank;
}

// ============================================================================
// Photo Import Processing
// ============================================================================

inline void CharacterModeHandler::openPhotoImportDialog() {
    m_state.photoImport.showDialog = true;
}

inline void CharacterModeHandler::processPhotoImport() {
    if (!m_state.photoImport.processing) return;
    if (!m_state.character) return;

    initializePipeline();

    m_state.photoImport.progress = 0.1f;

    // Check if any photos are loaded
    bool anyLoaded = false;
    for (int i = 0; i < 3; i++) {
        if (m_state.photoImport.photos[i].loaded) { anyLoaded = true; break; }
    }

    // Legacy single-photo path
    if (!anyLoaded && !m_state.photoImport.photoPath.empty()) {
        m_state.photoImport.photos[0].filePath = m_state.photoImport.photoPath;
        m_state.photoImport.photos[0].viewType = PhotoSlot::Front;
        PhotoImportModule::loadPhotoSlot(m_state.photoImport.photos[0]);
        anyLoaded = m_state.photoImport.photos[0].loaded;
    }

    if (!anyLoaded) {
        m_state.photoImport.errorMessage = "No photos loaded";
        m_state.photoImport.failed = true;
        m_state.photoImport.processing = false;
        return;
    }

    m_state.photoImport.progress = 0.3f;

    // Process all loaded photos through the pipeline
    editor::PhotoImportResult importResult = PhotoImportModule::processAllPhotos(
        m_state.photoImport.photos, 3, m_state.facePipeline);

    m_state.photoImport.progress = 0.7f;

    if (importResult.success) {
        // Apply skin color
        m_state.character->getFace().getTextureParams().skinTone = importResult.skinColor;

        // Initialize IdentityFitter if needed
        if (!m_state.identityFitterInitialized) {
            initializeIdentityFitter();
        }
        
        // Use MetaHuman-style optimization: minimize || project(V + Σ w*BS) - landmarks_2D ||²
        if (m_state.identityFitterInitialized && !importResult.landmarks.points.empty()) {
            printf("[CharacterMode] Using MetaHuman-style identity fitting\n");
            
            // Convert landmarks to Vec2 format (normalized to face bbox)
            std::vector<Vec2> landmarks2D;
            const auto& slot = m_state.photoImport.photos[0];
            float faceX = slot.detection.x1;
            float faceY = slot.detection.y1;
            float faceW = slot.detection.width();
            float faceH = slot.detection.height();
            
            if (faceW < 10 || faceH < 10) {
                faceX = 0; faceY = 0;
                faceW = slot.width > 0 ? (float)slot.width : 1.0f;
                faceH = slot.height > 0 ? (float)slot.height : 1.0f;
            }
            
            for (const auto& pt : importResult.landmarks.points) {
                float nx = (pt.x - faceX) / faceW;
                float ny = (pt.y - faceY) / faceH;
                landmarks2D.push_back(Vec2(nx, ny));
            }
            
            // Run optimization with FLAME/DECA-style parameters
            IdentityFitter::FitParams fitParams;
            fitParams.maxIterations = 300;
            fitParams.learningRate = 0.1f;
            fitParams.regularization = 0.01f;
            fitParams.convergenceThreshold = 1e-7f;
            fitParams.verbose = true;
            
            auto fitResult = m_state.identityFitter.fit(landmarks2D, faceW / faceH, fitParams);
            
            // Apply optimized weights to face params
            auto& faceParams = m_state.character->getFace().getShapeParams();
            m_state.identityFitter.applyToParams(fitResult, faceParams);
            
            printf("[CharacterMode] Identity fitting: %d iters, error=%.4f, converged=%d\n",
                   fitResult.iterations, fitResult.finalError, fitResult.converged);
        } else {
            // Fallback to landmark-ratio based mapping
            printf("[CharacterMode] Fallback: using landmark-ratio mapping\n");
            m_state.character->getFace().setShapeParams(importResult.faceParams);
        }
        
        m_state.meshNeedsUpdate = true;
        m_state.photoImport.multiPhotoResult = importResult;

        PhotoFaceResult legacyResult;
        legacyResult.success = true;
        legacyResult.overallConfidence = importResult.confidence;
        legacyResult.textureData = importResult.textureData;
        legacyResult.textureWidth = importResult.textureWidth;
        legacyResult.textureHeight = importResult.textureHeight;
        m_state.photoImport.result = legacyResult;

        printf("[CharacterMode] Multi-photo face generation: conf=%.0f%%, %s\n",
               importResult.confidence * 100.0f, importResult.summary.c_str());
    } else {
        // Fallback: use dominant color extraction for skin tone
        if (!m_state.photoImport.photoPath.empty()) {
            Vec3 color = PhotoImportModule::extractDominantColor(m_state.photoImport.photoPath);
            m_state.character->getFace().getTextureParams().skinTone = color;
        }

        m_state.photoImport.errorMessage = importResult.errorMessage;
        printf("[CharacterMode] Photo import fallback: %s\n", importResult.errorMessage.c_str());
    }

    m_state.photoImport.progress = 0.9f;
    m_state.photoImport.completed = true;
    m_state.photoImport.failed = !importResult.success;
    m_state.photoImport.progress = 1.0f;
    m_state.photoImport.processing = false;
    m_state.meshNeedsUpdate = true;

    updateCharacterMesh();
    uploadCharacterToGPU();

    m_state.subMode = CharacterSubMode::Face;
}

// ============================================================================
// Identity Fitter Initialization
// ============================================================================

inline void CharacterModeHandler::initializeIdentityFitter() {
    if (m_state.identityFitterInitialized) return;
    
    printf("[CharacterMode] Initializing IdentityFitter...\n");
    
    // Get base mesh
    auto baseMesh = m_state.renderer.getBaseMesh();
    if (baseMesh.empty()) {
        printf("[CharacterMode] No base mesh available for IdentityFitter\n");
        return;
    }
    
    // First, initialize LandmarkDeformer if not already done (it handles OBJ->BIN index conversion)
    if (!m_state.landmarkDeformerInitialized) {
        std::string mappingPath = m_state.modelDirectory + "/landmark_vertex_map.json";
        std::string objPath = m_state.modelDirectory + "/Super Average Head.obj";
        if (m_state.landmarkDeformer.loadMapping(mappingPath)) {
            m_state.landmarkDeformer.setBaseMesh(baseMesh, objPath);
            m_state.landmarkDeformerInitialized = true;
        }
    }
    
    if (!m_state.landmarkDeformerInitialized) {
        printf("[CharacterMode] Failed to initialize LandmarkDeformer\n");
        return;
    }
    
    // Get the converted vertex indices from LandmarkDeformer
    const auto& vertexIndices = m_state.landmarkDeformer.getLandmarkVertexIndices();
    std::unordered_map<int, int> landmarkVertexMap;
    
    int validCount = 0;
    for (int i = 0; i < 68; i++) {
        if (vertexIndices[i] >= 0) {
            landmarkVertexMap[i] = vertexIndices[i];
            validCount++;
        }
    }
    
    printf("[CharacterMode] Got %d landmark vertex mappings from LandmarkDeformer\n", validCount);
    
    if (validCount < 68) {
        printf("[CharacterMode] Incomplete landmark mapping: %d/68\n", validCount);
        return;
    }
    
    // Initialize fitter with base mesh and landmark mapping
    m_state.identityFitter.initialize(baseMesh, landmarkVertexMap);
    
    // Get identity BlendShape names from character
    std::vector<std::string> identityShapeNames = {
        "faceWidth", "faceLength", "faceRoundness",
        "jawWidth", "jawAngle", "chinLength", "chinWidth",
        "noseWidth", "noseLength", "noseHeight", "noseBridge",
        "mouthWidth", "upperLipThickness", "lowerLipThickness",
        "eyeSize", "eyeWidth", "eyeHeight", "eyeSpacing", "eyeDepth",
        "browHeight", "browWidth", "browAngle",
        "cheekboneWidth", "cheekboneHeight",
        "foreheadHeight", "foreheadWidth"
    };
    
    // Set BlendShapes for fitting
    auto* blendShapes = m_state.character->getFace().getBlendShapeMesh();
    if (blendShapes) {
        m_state.identityFitter.setBlendShapes(*blendShapes, identityShapeNames);
    }
    
    m_state.identityFitterInitialized = true;
    printf("[CharacterMode] IdentityFitter initialized successfully\n");
}

// ============================================================================
// Mesh Update and GPU Upload
// ============================================================================

inline void CharacterModeHandler::updateCharacterMesh() {
    if (!m_state.character) return;
    
    // Apply parameter constraints to maintain plausible face shapes
    m_state.character->getFace().applyConstraints();
    
    // Update face blend shape weights from current parameters
    m_state.character->getFace().applyParameters();
    m_state.character->getBody().updateBlendShapeWeights();
    
    // Update the renderer's deformed mesh
    m_state.renderer.updateBlendShapes();
}

inline void CharacterModeHandler::uploadCharacterToGPU() {
    if (!m_ctx || !m_ctx->renderer || !m_ctx->scene) return;
    if (!m_state.character) return;
    
    // Get deformed mesh data
    Mesh mesh = m_state.renderer.getCurrentMesh();
    if (mesh.vertices.empty()) return;
    
    // Apply skin color from character face
    auto skinTone = m_state.character->getFace().getTextureParams().skinTone;
    mesh.baseColor[0] = skinTone.x;
    mesh.baseColor[1] = skinTone.y;
    mesh.baseColor[2] = skinTone.z;
    mesh.metallic = 0.02f;   // Slight metallic for skin sheen
    mesh.roughness = 0.35f;  // Lower roughness = sharper highlights
    
    // Find or create the character entity in the scene
    auto* selectedEntity = m_ctx->scene->getSelectedEntity();
    if (selectedEntity && selectedEntity->hasModel) {
        // Update existing entity's mesh
        if (!selectedEntity->model.meshes.empty()) {
            auto gpuMesh = m_ctx->renderer->uploadMesh(mesh);
            selectedEntity->model.meshes[0] = gpuMesh;
        }
    } else {
        // Create a new entity for the character
        RHILoadedModel charModel;
        charModel.meshes.push_back(m_ctx->renderer->uploadMesh(mesh));
        charModel.center[0] = charModel.center[1] = charModel.center[2] = 0.0f;
        charModel.radius = 1.0f;
        charModel.name = m_state.character->getName();
        charModel.debugName = "character/" + m_state.character->getName();
        
        Entity* entity = m_ctx->scene->createEntityWithModel(
            m_state.character->getName(), charModel);
        entity->material = std::make_shared<Material>();
        entity->material->baseColor = {skinTone.x, skinTone.y, skinTone.z};
        entity->material->metallic = 0.02f;   // Slight metallic for skin sheen
        entity->material->roughness = 0.35f;  // Lower roughness = sharper highlights
        
        // Mark as character (create a dummy skeleton for mode detection)
        entity->skeleton = std::make_unique<Skeleton>();
        entity->skeleton->addBone("Root", -1);
        entity->skeleton->addBone("Hips", 0);
        
        m_ctx->scene->setSelectedEntity(entity);
    }
    
    m_state.hasGPUMesh = true;
    m_state.renderer.markGPUUpdated();
    
    if (m_characterChangedCallback) {
        m_characterChangedCallback();
    }
}

inline void CharacterModeHandler::applyToSceneEntity() {
    if (!m_state.character || !m_state.characterCreated) return;
    
    // Final mesh update and GPU upload
    updateCharacterMesh();
    uploadCharacterToGPU();
}

// ============================================================================
// UI Rendering - Toolbar
// ============================================================================

inline void CharacterModeHandler::renderCharacterToolbar() {
    using luma::ui::loc;
    
    ImGui::TextColored(ImVec4(0.26f, 0.53f, 0.96f, 1.0f), "[C] %s", loc("Character"));
    ImGui::Separator();
    
    if (!m_state.characterCreated) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "%s", loc("Create a character to begin"));
        return;
    }
    
    // Sub-mode tabs
    struct TabInfo { CharacterSubMode mode; const char* label; };
    TabInfo tabs[] = {
        { CharacterSubMode::Overview,   "Overview" },
        { CharacterSubMode::Body,       "Body" },
        { CharacterSubMode::Face,       "Face" },
        { CharacterSubMode::Hair,       "Hair" },
        { CharacterSubMode::Clothing,   "Clothing" },
        { CharacterSubMode::Expression, "Expression" },
        { CharacterSubMode::Export,     "Export" },
    };
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    for (int i = 0; i < 7; i++) {
        if (i > 0) ImGui::SameLine(0, 2);
        
        bool selected = (m_state.subMode == tabs[i].mode);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.58f, 1.0f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.24f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.35f, 1.0f));
        }
        
        float tabWidth = (i < 5) ? 42.0f : 50.0f;
        if (ImGui::Button(loc(tabs[i].label), ImVec2(tabWidth, 22))) {
            setSubMode(tabs[i].mode);
        }
        
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleVar();
    
    ImGui::Separator();
    ImGui::Spacing();
}

// ============================================================================
// UI Rendering - Overview (Creation Workflow)
// ============================================================================

inline void CharacterModeHandler::renderOverviewUI() {
    using luma::ui::loc;
    
    // ===== 开发工具：标准拓扑头 Landmark 标记 =====
    if (ImGui::CollapsingHeader("开发工具: Landmark 标记", ImGuiTreeNodeFlags_None)) {
        ImGui::TextWrapped("在标准拓扑头 (Super Average Head.obj) 上标记 68 个 iBUG 特征点，用于照片驱动的人脸重建。");
        ImGui::Spacing();
        
        if (ImGui::Button("打开 Landmark 编辑器 (自动识别)", ImVec2(-1, 32))) {
            // 确保 pipeline 初始化（设置 modelDirectory）
            initializePipeline();
            
            // 加载标准拓扑头 OBJ
            std::string objPath = m_state.modelDirectory + "/Super Average Head.obj";
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            
            // 使用简单 OBJ 解析器加载顶点和面
            std::ifstream objFile(objPath);
            if (objFile.is_open()) {
                std::string line;
                std::vector<Vec3> positions;
                std::vector<Vec3> normals;
                std::vector<std::tuple<int, int, int>> faces;  // vertex indices
                
                while (std::getline(objFile, line)) {
                    if (line.empty() || line[0] == '#') continue;
                    
                    std::istringstream iss(line);
                    std::string prefix;
                    iss >> prefix;
                    
                    if (prefix == "v") {
                        float x, y, z;
                        iss >> x >> y >> z;
                        positions.push_back(Vec3(x, y, z));
                    }
                    else if (prefix == "vn") {
                        float x, y, z;
                        iss >> x >> y >> z;
                        normals.push_back(Vec3(x, y, z));
                    }
                    else if (prefix == "f") {
                        // 解析面（支持 v, v/vt, v/vt/vn, v//vn 格式）
                        std::vector<int> faceVerts;
                        std::string token;
                        while (iss >> token) {
                            int vIdx = 0;
                            size_t slashPos = token.find('/');
                            if (slashPos != std::string::npos) {
                                vIdx = std::stoi(token.substr(0, slashPos)) - 1;
                            } else {
                                vIdx = std::stoi(token) - 1;
                            }
                            faceVerts.push_back(vIdx);
                        }
                        // 三角化（假设是三角形或四边形）
                        if (faceVerts.size() >= 3) {
                            faces.push_back({faceVerts[0], faceVerts[1], faceVerts[2]});
                            if (faceVerts.size() >= 4) {
                                faces.push_back({faceVerts[0], faceVerts[2], faceVerts[3]});
                            }
                        }
                    }
                }
                objFile.close();
                
                // 计算顶点法线（如果没有）
                std::vector<Vec3> computedNormals(positions.size(), Vec3(0, 0, 0));
                for (const auto& [i0, i1, i2] : faces) {
                    if (i0 >= 0 && i1 >= 0 && i2 >= 0 && 
                        i0 < (int)positions.size() && i1 < (int)positions.size() && i2 < (int)positions.size()) {
                        Vec3 v0 = positions[i0];
                        Vec3 v1 = positions[i1];
                        Vec3 v2 = positions[i2];
                        Vec3 edge1 = v1 - v0;
                        Vec3 edge2 = v2 - v0;
                        Vec3 faceNormal = edge1.cross(edge2);
                        computedNormals[i0] = computedNormals[i0] + faceNormal;
                        computedNormals[i1] = computedNormals[i1] + faceNormal;
                        computedNormals[i2] = computedNormals[i2] + faceNormal;
                    }
                }
                for (auto& n : computedNormals) {
                    float len = n.length();
                    if (len > 0.0001f) n = n * (1.0f / len);
                }
                
                // 转换为 Vertex 格式
                vertices.reserve(positions.size());
                for (size_t i = 0; i < positions.size(); i++) {
                    Vertex v{};
                    v.position[0] = positions[i].x;
                    v.position[1] = positions[i].y;
                    v.position[2] = positions[i].z;
                    v.normal[0] = computedNormals[i].x;
                    v.normal[1] = computedNormals[i].y;
                    v.normal[2] = computedNormals[i].z;
                    vertices.push_back(v);
                }
                
                // 转换面为索引
                indices.reserve(faces.size() * 3);
                for (const auto& [i0, i1, i2] : faces) {
                    if (i0 >= 0 && i1 >= 0 && i2 >= 0) {
                        indices.push_back((uint32_t)i0);
                        indices.push_back((uint32_t)i1);
                        indices.push_back((uint32_t)i2);
                    }
                }
                
                if (!vertices.empty()) {
                    m_state.landmarkEditor.setMesh(vertices, indices);
                    
                    // 使用 loadModel 加载完整模型用于 PBR 渲染
                    if (m_ctx && m_ctx->renderer) {
                        m_state.landmarkEditorModelLoaded = m_ctx->renderer->loadModel(
                            objPath, m_state.landmarkEditorModel
                        );
                        if (m_state.landmarkEditorModelLoaded) {
                            printf("[CharacterMode] Loaded model for PBR: %zu meshes, %zu verts\n",
                                   m_state.landmarkEditorModel.meshes.size(),
                                   m_state.landmarkEditorModel.totalVerts);
                        }
                    }
                    
                    // 先尝试加载现有映射
                    std::string landmarkPath = m_state.modelDirectory + "/landmark_vertex_map.json";
                    bool hasExisting = m_state.landmarkEditor.loadMapping(landmarkPath);
                    
                    // 如果没有现有映射或映射不完整，自动识别
                    if (!hasExisting || m_state.landmarkEditor.getMarkedCount() < 68) {
                        printf("[CharacterMode] Running auto-detection...\n");
                        m_state.landmarkEditor.autoDetectLandmarks();
                    }
                    
                    m_state.landmarkEditor.setActive(true);
                    printf("[CharacterMode] Landmark editor opened: %zu vertices, %zu faces, %d landmarks\n", 
                           vertices.size(), faces.size(), m_state.landmarkEditor.getMarkedCount());
                } else {
                    printf("[CharacterMode] ERROR: Failed to load vertices from %s\n", objPath.c_str());
                }
            } else {
                printf("[CharacterMode] ERROR: Cannot open %s\n", objPath.c_str());
            }
        }
        
        if (m_state.landmarkEditor.isActive()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "编辑器已打开 - 进度: %d/68", 
                              m_state.landmarkEditor.getMarkedCount());
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    
    if (!m_state.characterCreated) {
        // ===== Character Creation Options =====
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "%s", loc("Create Character"));
        ImGui::Spacing();
        
        // Template selection
        ImGui::Text("%s:", loc("Style"));
        const char* styleNames[] = { "Realistic", "Stylized", "Anime", "Cartoon", "Chibi" };
        CharacterStyle styles[] = { 
            CharacterStyle::Realistic, CharacterStyle::Stylized, 
            CharacterStyle::Anime, CharacterStyle::Cartoon, CharacterStyle::Chibi 
        };
        
        for (int i = 0; i < 5; i++) {
            bool selected = (m_state.selectedStyle == styles[i]);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
            
            if (ImGui::Button(loc(styleNames[i]), ImVec2(135, 28))) {
                m_state.selectedStyle = styles[i];
            }
            if (selected) ImGui::PopStyleColor();
            
            if (i % 2 == 0) ImGui::SameLine(0, 8);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Creation methods
        ImGui::Text("%s:", loc("Creation Method"));
        ImGui::Spacing();
        
        // From template
        if (ImGui::Button(loc("From Template"), ImVec2(-1, 32))) {
            createFromTemplate("Human", m_state.selectedStyle);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc("Create from a built-in human template"));
        
        ImGui::Spacing();
        
        // From photo (AI)
        if (!m_state.hasAIModels) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.4f, 0.2f, 1.0f));
        }
        if (ImGui::Button(m_state.hasAIModels ? loc("From Photo (AI)") : loc("From Photo (Basic)"), ImVec2(-1, 32))) {
            openPhotoImportDialog();
        }
        if (!m_state.hasAIModels) {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) {
            if (m_state.hasAIModels) {
                ImGui::SetTooltip("%s", loc("Upload a photo to auto-generate face"));
            } else {
                ImGui::SetTooltip("%s", loc("AI models missing - only skin color will be extracted.\nDownload models per models/README.md for full AI reconstruction."));
            }
        }
        
        ImGui::Spacing();
        
        // Random
        if (ImGui::Button(loc("Random Generate"), ImVec2(-1, 32))) {
            createRandom();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc("Generate a random character"));
        
        ImGui::Spacing();
        
        // Blank
        if (ImGui::Button(loc("Blank Character"), ImVec2(-1, 32))) {
            createBlank();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc("Start with default parameters"));
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Presets
        ImGui::Text("%s:", loc("Face Presets"));
        const auto& presets = m_state.facePresets.getAllPresets();
        for (const auto& preset : presets) {
            if (ImGui::Button(preset.name.c_str(), ImVec2(-1, 24))) {
                createFromPreset(preset.name);
            }
        }
        
    } else {
        // ===== Character Overview (after creation) =====
        ImGui::Text("%s: %s", loc("Name"), m_state.character->getName().c_str());
        
        // Rename
        static char nameBuffer[128] = {};
        if (nameBuffer[0] == '\0') {
            strncpy(nameBuffer, m_state.character->getName().c_str(), sizeof(nameBuffer) - 1);
        }
        if (ImGui::InputText(loc("Rename"), nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            m_state.character->setName(nameBuffer);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Quick actions
        if (ImGui::Button(loc("Randomize"), ImVec2(-1, 28))) {
            m_state.pushFaceUndo();
            m_state.character->randomize();
            m_state.meshNeedsUpdate = true;
        }
        
        if (ImGui::Button(loc("Reset to Default"), ImVec2(-1, 28))) {
            m_state.pushFaceUndo();
            m_state.character->getFace().getShapeParams().reset();
            m_state.meshNeedsUpdate = true;
        }
        
        ImGui::Spacing();
        
        // Mesh statistics
        if (ImGui::CollapsingHeader(loc("Statistics"))) {
            ImGui::Text("%s: %u", loc("Vertices"), m_state.renderer.getVertexCount());
            ImGui::Text("%s: %u", loc("Triangles"), m_state.renderer.getIndexCount() / 3);
            
            const auto& body = m_state.character->getBody().getParams();
            ImGui::Text("%s: %s", loc("Gender"), body.gender == Gender::Male ? loc("Male") : loc("Female"));
        }
        
    }
    
    // Photo import dialog (MetaHuman-style multi-photo)
    if (m_state.photoImport.showDialog) {
        ImGui::OpenPopup("PhotoImportPopup");
        m_state.photoImport.showDialog = false;
    }

    if (ImGui::BeginPopupModal("PhotoImportPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", loc("Import Photos for Face Generation"));
        ImGui::Separator();
        ImGui::Spacing();

        if (m_state.photoImport.processing) {
            ImGui::Text("%s...", loc("Processing photos"));
            ImGui::ProgressBar(m_state.photoImport.progress);

            // Show per-photo progress
            const char* slotNames[] = { "Front", "Left Profile", "Right Profile" };
            for (int i = 0; i < 3; i++) {
                auto& slot = m_state.photoImport.photos[i];
                if (slot.loaded) {
                    ImGui::Text("  %s: %.0f%%", slotNames[i], slot.processingProgress * 100.0f);
                }
            }
        } else if (m_state.photoImport.completed) {
            if (m_state.photoImport.failed) {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s",
                    m_state.photoImport.errorMessage.c_str());
            } else {
                float conf = m_state.photoImport.multiPhotoResult.confidence;
                if (conf < 0.6f) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                        "%s", loc("Basic mode: AI models not found"));
                    ImGui::TextWrapped("%s",
                        loc("Only skin color was extracted. For accurate face reconstruction, "
                            "download AI models to the models/ folder. See models/README.md"));
                } else {
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "%s", loc("Face generation complete!"));
                }
                if (!m_state.photoImport.multiPhotoResult.summary.empty()) {
                    ImGui::Text("%s", m_state.photoImport.multiPhotoResult.summary.c_str());
                }
            }
            if (ImGui::Button(loc("OK"), ImVec2(120, 0))) {
                m_state.photoImport.completed = false;
                ImGui::CloseCurrentPopup();
            }
        } else {
            // Multi-photo slot UI
            ImGui::Text("%s", loc("Select photos (front required, side views optional):"));
            ImGui::Spacing();

            const char* slotLabels[] = {
                "Front Photo (Required)",
                "Left Profile (Optional)",
                "Right Profile (Optional)"
            };

            static char photoPaths[3][512] = {};
            void* hwnd = m_ctx ? m_ctx->nativeWindowHandle : nullptr;

            for (int i = 0; i < 3; i++) {
                ImGui::PushID(i);
                auto& slot = m_state.photoImport.photos[i];

                // Slot header with status indicator
                ImVec4 headerColor = slot.loaded ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f)
                                                 : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                ImGui::TextColored(headerColor, "%s %s",
                    slot.loaded ? "[OK]" : "[ ]", slotLabels[i]);

                // File path input + browse button
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
                ImGui::InputText("##path", photoPaths[i], sizeof(photoPaths[i]));
                ImGui::SameLine();
                if (ImGui::Button("...", ImVec2(36, 0))) {
                    std::string selected = PhotoImportModule::openPhotoFileDialog(hwnd);
                    if (!selected.empty()) {
#ifdef _WIN32
                        strncpy_s(photoPaths[i], selected.c_str(), sizeof(photoPaths[i]) - 1);
#else
                        strncpy(photoPaths[i], selected.c_str(), sizeof(photoPaths[i]) - 1);
                        photoPaths[i][sizeof(photoPaths[i]) - 1] = '\0';
#endif
                        slot.filePath = selected;
                        PhotoImportModule::loadPhotoSlot(slot);
                    }
                }
                ImGui::SameLine();
                if (slot.loaded && ImGui::Button("X", ImVec2(24, 0))) {
                    slot.clear();
                    photoPaths[i][0] = '\0';
                }

                ImGui::PopID();

                if (i < 2) ImGui::Spacing();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Generate button (enabled only if front photo is loaded)
            bool canGenerate = m_state.photoImport.photos[0].loaded;
            if (!canGenerate) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(loc("Generate Face"), ImVec2(160, 30))) {
                // Copy paths to slots
                for (int i = 0; i < 3; i++) {
                    if (photoPaths[i][0] != '\0' && !m_state.photoImport.photos[i].loaded) {
                        m_state.photoImport.photos[i].filePath = photoPaths[i];
                        PhotoImportModule::loadPhotoSlot(m_state.photoImport.photos[i]);
                    }
                }

                // Start processing
                if (!m_state.characterCreated) {
                    createFromTemplate("Photo Character", CharacterStyle::Realistic);
                }
                m_state.photoImport.processing = true;
                m_state.photoImport.progress = 0.0f;
                m_state.creationMethod = CharacterCreationMethod::Photo;
                ImGui::CloseCurrentPopup();
            }

            if (!canGenerate) {
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", loc("Front photo required"));
            }

            ImGui::SameLine();
            if (ImGui::Button(loc("Cancel"), ImVec2(120, 30))) {
                m_state.photoImport.reset();
                for (auto& p : photoPaths) p[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::EndPopup();
    }
}

// ============================================================================
// UI Rendering - Body
// ============================================================================

inline void CharacterModeHandler::renderBodyUI() {
    using luma::ui::loc;
    
    if (!m_state.character) return;
    auto& body = m_state.character->getBody();
    auto& params = body.getParams();
    
    ImGui::Text("%s", loc("Body Customization"));
    ImGui::Spacing();
    
    // Gender
    ImGui::Text("%s:", loc("Gender"));
    ImGui::SameLine();
    bool isMale = (params.gender == Gender::Male);
    if (ImGui::RadioButton(loc("Male"), isMale)) {
        params.gender = Gender::Male;
        body.updateBlendShapeWeights();
        m_state.meshNeedsUpdate = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(loc("Female"), !isMale)) {
        params.gender = Gender::Female;
        body.updateBlendShapeWeights();
        m_state.meshNeedsUpdate = true;
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Body measurements
    auto& measurements = params.measurements;
    bool changed = false;
    
    if (ImGui::CollapsingHeader(loc("Proportions"), ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::SliderFloat(loc("Height"), &measurements.height, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat(loc("Weight"), &measurements.weight, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat(loc("Muscularity"), &measurements.muscularity, 0.0f, 1.0f, "%.2f");
    }
    
    if (ImGui::CollapsingHeader(loc("Upper Body"))) {
        changed |= ImGui::SliderFloat(loc("Shoulder Width"), &measurements.shoulderWidth, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat(loc("Chest"), &measurements.chestSize, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat(loc("Waist"), &measurements.waistSize, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat(loc("Arm Length"), &measurements.armLength, 0.0f, 1.0f, "%.2f");
    }
    
    if (ImGui::CollapsingHeader(loc("Lower Body"))) {
        changed |= ImGui::SliderFloat(loc("Hip Width"), &measurements.hipWidth, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat(loc("Leg Length"), &measurements.legLength, 0.0f, 1.0f, "%.2f");
    }
    
    if (changed) {
        body.updateBlendShapeWeights();
        m_state.meshNeedsUpdate = true;
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Skin color
    if (ImGui::CollapsingHeader(loc("Skin Color"), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& skinTone = m_state.character->getFace().getTextureParams().skinTone;
        float color[3] = { skinTone.x, skinTone.y, skinTone.z };
        if (ImGui::ColorEdit3(loc("Skin Tone"), color)) {
            skinTone = Vec3(color[0], color[1], color[2]);
            m_state.character->getBody().getParams().skinColor = skinTone;
            m_state.gpuNeedsUpdate = true;
        }
    }
    
    ImGui::Spacing();
    
    // Body presets
    if (ImGui::CollapsingHeader(loc("Body Presets"))) {
        struct PresetEntry { const char* name; BodyPreset preset; };
        PresetEntry presets[] = {
            { "Athletic", BodyPreset::MaleMuscular },
            { "Slim",     BodyPreset::MaleSlim },
            { "Average",  BodyPreset::MaleAverage },
            { "Heavy",    BodyPreset::MaleHeavy },
            { "Muscular", BodyPreset::MaleMuscular },
        };
        for (const auto& entry : presets) {
            if (ImGui::Button(loc(entry.name), ImVec2(-1, 24))) {
                body.getParams().measurements.applyPreset(entry.preset);
                body.updateBlendShapeWeights();
                m_state.meshNeedsUpdate = true;
            }
        }
    }
}

// ============================================================================
// UI Rendering - Face
// ============================================================================

inline bool CharacterModeHandler::faceSlider(const char* label, float* value, float min, float max) {
    float old = *value;
    if (ImGui::SliderFloat(label, value, min, max, "%.2f")) {
        if (old != *value) {
            m_state.meshNeedsUpdate = true;
            return true;
        }
    }
    return false;
}

inline bool CharacterModeHandler::faceSliderWithReset(const char* label, float* value, float defaultVal) {
    bool changed = false;
    
    ImGui::PushID(label);
    float width = ImGui::GetContentRegionAvail().x - 30;
    ImGui::SetNextItemWidth(width);
    
    float old = *value;
    if (ImGui::SliderFloat("##slider", value, 0.0f, 1.0f, "%.2f")) {
        if (old != *value) {
            m_state.meshNeedsUpdate = true;
            changed = true;
        }
    }
    
    ImGui::SameLine();
    if (ImGui::SmallButton("R")) {
        if (*value != defaultVal) {
            m_state.pushFaceUndo();
            *value = defaultVal;
            m_state.meshNeedsUpdate = true;
            changed = true;
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset to %.1f", defaultVal);
    
    ImGui::SameLine(0, 0);
    ImGui::TextUnformatted(label);
    
    ImGui::PopID();
    return changed;
}

inline void CharacterModeHandler::renderFaceRegionSliders(FaceEditRegion region) {
    if (!m_state.character) return;
    auto& params = m_state.character->getFace().getShapeParams();
    
    switch (region) {
        case FaceEditRegion::All:
            faceSliderWithReset("Face Width", &params.faceWidth);
            faceSliderWithReset("Face Length", &params.faceLength);
            faceSliderWithReset("Face Roundness", &params.faceRoundness);
            break;
            
        case FaceEditRegion::Forehead:
            faceSliderWithReset("Forehead Height", &params.foreheadHeight);
            faceSliderWithReset("Forehead Width", &params.foreheadWidth);
            faceSliderWithReset("Forehead Slope", &params.foreheadSlope);
            break;
            
        case FaceEditRegion::Eyes:
            faceSliderWithReset("Eye Size", &params.eyeSize);
            faceSliderWithReset("Eye Width", &params.eyeWidth);
            faceSliderWithReset("Eye Height", &params.eyeHeight);
            faceSliderWithReset("Eye Spacing", &params.eyeSpacing);
            faceSliderWithReset("Eye Angle", &params.eyeAngle);
            faceSliderWithReset("Eye Depth", &params.eyeDepth);
            faceSliderWithReset("Upper Eyelid", &params.upperEyelid);
            faceSliderWithReset("Lower Eyelid", &params.lowerEyelid);
            break;
            
        case FaceEditRegion::Eyebrows:
            faceSliderWithReset("Brow Height", &params.browHeight);
            faceSliderWithReset("Brow Thickness", &params.browThickness);
            faceSliderWithReset("Brow Angle", &params.browAngle);
            faceSliderWithReset("Brow Curve", &params.browCurve);
            break;
            
        case FaceEditRegion::Nose:
            faceSliderWithReset("Nose Length", &params.noseLength);
            faceSliderWithReset("Nose Width", &params.noseWidth);
            faceSliderWithReset("Nose Height", &params.noseHeight);
            faceSliderWithReset("Nose Bridge", &params.noseBridge);
            faceSliderWithReset("Nose Tip", &params.noseTip);
            faceSliderWithReset("Nose Tip Angle", &params.noseTipAngle);
            faceSliderWithReset("Nostril Width", &params.nostrilWidth);
            break;
            
        case FaceEditRegion::Mouth:
            faceSliderWithReset("Mouth Width", &params.mouthWidth);
            faceSliderWithReset("Upper Lip", &params.upperLipThickness);
            faceSliderWithReset("Lower Lip", &params.lowerLipThickness);
            faceSliderWithReset("Lip Protrusion", &params.lipProtrusion);
            faceSliderWithReset("Mouth Corners", &params.mouthCorners);
            faceSliderWithReset("Lip Curve", &params.lipCurve);
            break;
            
        case FaceEditRegion::Chin:
            faceSliderWithReset("Chin Length", &params.chinLength);
            faceSliderWithReset("Chin Width", &params.chinWidth);
            faceSliderWithReset("Chin Protrusion", &params.chinProtrusion);
            faceSliderWithReset("Chin Shape", &params.chinShape);
            faceSliderWithReset("Chin Cleft", &params.chinCleft, 0.0f);
            break;
            
        case FaceEditRegion::Jaw:
            faceSliderWithReset("Jaw Width", &params.jawWidth);
            faceSliderWithReset("Jaw Angle", &params.jawAngle);
            faceSliderWithReset("Jaw Line", &params.jawLine);
            break;
            
        case FaceEditRegion::Cheeks:
            faceSliderWithReset("Cheekbone Height", &params.cheekboneHeight);
            faceSliderWithReset("Cheekbone Width", &params.cheekboneWidth);
            faceSliderWithReset("Cheekbone Prominence", &params.cheekboneProminence);
            faceSliderWithReset("Cheek Fullness", &params.cheekFullness);
            break;
            
        case FaceEditRegion::Ears:
            faceSliderWithReset("Ear Size", &params.earSize);
            faceSliderWithReset("Ear Angle", &params.earAngle);
            faceSliderWithReset("Ear Lobe", &params.earLobe);
            faceSliderWithReset("Ear Pointiness", &params.earPointiness);
            break;
    }
}

inline void CharacterModeHandler::renderFaceUI() {
    using luma::ui::loc;
    
    if (!m_state.character) return;
    
    ImGui::Text("%s", loc("Face Sculpting"));
    ImGui::Spacing();
    
    // Undo/Redo buttons
    {
        bool canUndo = !m_state.faceUndoStack.empty();
        bool canRedo = !m_state.faceRedoStack.empty();
        
        if (!canUndo) ImGui::BeginDisabled();
        if (ImGui::SmallButton(loc("Undo"))) m_state.undoFace();
        if (!canUndo) ImGui::EndDisabled();
        
        ImGui::SameLine();
        
        if (!canRedo) ImGui::BeginDisabled();
        if (ImGui::SmallButton(loc("Redo"))) m_state.redoFace();
        if (!canRedo) ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (ImGui::SmallButton(loc("Reset All"))) {
            m_state.pushFaceUndo();
            m_state.character->getFace().getShapeParams().reset();
            m_state.meshNeedsUpdate = true;
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Face region tabs
    struct RegionTab { FaceEditRegion region; const char* label; };
    RegionTab regions[] = {
        { FaceEditRegion::All,       "Overall" },
        { FaceEditRegion::Forehead,  "Forehead" },
        { FaceEditRegion::Eyes,      "Eyes" },
        { FaceEditRegion::Eyebrows,  "Brows" },
        { FaceEditRegion::Nose,      "Nose" },
        { FaceEditRegion::Mouth,     "Mouth" },
        { FaceEditRegion::Chin,      "Chin" },
        { FaceEditRegion::Jaw,       "Jaw" },
        { FaceEditRegion::Cheeks,    "Cheeks" },
        { FaceEditRegion::Ears,      "Ears" },
    };
    
    for (int i = 0; i < 10; i++) {
        if (i > 0) ImGui::SameLine(0, 1);
        
        bool selected = (m_state.faceRegion == regions[i].region);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
        
        if (ImGui::SmallButton(loc(regions[i].label))) {
            m_state.faceRegion = regions[i].region;
        }
        
        if (selected) ImGui::PopStyleColor();
    }
    
    ImGui::Spacing();
    
    // Track mouse release for undo grouping
    static bool wasSliding = false;
    bool isSliding = ImGui::IsMouseDown(0);
    
    if (wasSliding && !isSliding) {
        // Mouse released after sliding - push undo state
        m_state.pushFaceUndo();
    }
    wasSliding = isSliding;
    
    // Render sliders for current region
    renderFaceRegionSliders(m_state.faceRegion);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Texture parameters
    if (ImGui::CollapsingHeader(loc("Appearance"), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& texParams = m_state.character->getFace().getTextureParams();
        
        float skinColor[3] = { texParams.skinTone.x, texParams.skinTone.y, texParams.skinTone.z };
        if (ImGui::ColorEdit3(loc("Skin Tone"), skinColor)) {
            texParams.skinTone = Vec3(skinColor[0], skinColor[1], skinColor[2]);
            m_state.gpuNeedsUpdate = true;
        }
        
        float eyeColor[3] = { texParams.eyeColor.x, texParams.eyeColor.y, texParams.eyeColor.z };
        if (ImGui::ColorEdit3(loc("Eye Color"), eyeColor)) {
            texParams.eyeColor = Vec3(eyeColor[0], eyeColor[1], eyeColor[2]);
        }
        
        float lipColor[3] = { texParams.lipColor.x, texParams.lipColor.y, texParams.lipColor.z };
        if (ImGui::ColorEdit3(loc("Lip Color"), lipColor)) {
            texParams.lipColor = Vec3(lipColor[0], lipColor[1], lipColor[2]);
        }
        
        ImGui::SliderFloat(loc("Wrinkles"), &texParams.wrinkles, 0.0f, 1.0f);
        ImGui::SliderFloat(loc("Freckles"), &texParams.freckles, 0.0f, 1.0f);
        ImGui::SliderFloat(loc("Skin Roughness"), &texParams.skinRoughness, 0.0f, 1.0f);
    }
    
    // Face presets
    if (ImGui::CollapsingHeader(loc("Face Presets"))) {
        const auto& presets = m_state.facePresets.getAllPresets();
        for (const auto& preset : presets) {
            if (ImGui::Button(preset.name.c_str(), ImVec2(-1, 22))) {
                m_state.pushFaceUndo();
                m_state.character->getFace().setShapeParams(preset.shapeParams);
                m_state.meshNeedsUpdate = true;
            }
        }
    }
    
    // Debug visualization options
    if (ImGui::CollapsingHeader(loc("Debug Visualization"), ImGuiTreeNodeFlags_DefaultOpen)) {
        // Base topology comparison toggle
        if (ImGui::Checkbox(loc("Show Base Topology"), &m_state.showBaseTopo)) {
            if (m_state.showBaseTopo) {
                // Save current params and reset to base
                m_state.savedParams = m_state.character->getFace().getShapeParams();
                m_state.character->getFace().getShapeParams().reset();
                m_state.meshNeedsUpdate = true;
            } else {
                // Restore saved params
                m_state.character->getFace().setShapeParams(m_state.savedParams);
                m_state.meshNeedsUpdate = true;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("Toggle between current face and base topology (all parameters = 0)"));
        }
        
        // Landmark visualization toggle
        ImGui::Checkbox(loc("Show Landmarks"), &m_state.showLandmarks);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc("Display 68 facial landmark points on the mesh"));
        }
        
        // Initialize landmark deformer for visualization if needed
        if (m_state.showLandmarks && !m_state.landmarkDeformerInitialized) {
            auto baseMesh = m_state.renderer.getBaseMesh();
            if (!baseMesh.empty()) {
                std::string mappingPath = m_state.modelDirectory + "/landmark_vertex_map.json";
                std::string objPath = m_state.modelDirectory + "/Super Average Head.obj";
                if (m_state.landmarkDeformer.loadMapping(mappingPath)) {
                    m_state.landmarkDeformer.setBaseMesh(baseMesh, objPath);
                    m_state.landmarkDeformerInitialized = true;
                }
            }
        }
    }
}

// ============================================================================
// UI Rendering - Hair
// ============================================================================

inline void CharacterModeHandler::renderHairUI() {
    using luma::ui::loc;
    
    if (!m_state.character) return;
    
    ImGui::Text("%s", loc("Hair Style"));
    ImGui::Spacing();
    
    // Hair style categories
    const char* categories[] = { "Short", "Medium", "Long", "Updo", "Bald" };
    static int selectedCategory = 0;
    
    for (int i = 0; i < 5; i++) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool sel = (selectedCategory == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
        if (ImGui::Button(loc(categories[i]), ImVec2(54, 24))) selectedCategory = i;
        if (sel) ImGui::PopStyleColor();
    }
    
    ImGui::Spacing();
    
    // Hair style list
    auto& hairLib = HairStyleLibrary::getInstance();
    hairLib.initializeDefaults();
    auto styles = hairLib.getStylesByCategory(static_cast<HairCategory>(selectedCategory));
    
    for (int i = 0; i < static_cast<int>(styles.size()); i++) {
        if (ImGui::Selectable(styles[i].c_str(), m_state.selectedHairStyle == i)) {
            m_state.selectedHairStyle = i;
            // TODO: Apply hair style to character
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Hair color
    if (ImGui::CollapsingHeader(loc("Hair Color"), ImGuiTreeNodeFlags_DefaultOpen)) {
        // Preset colors
        struct ColorPreset { const char* name; float r, g, b; };
        ColorPreset colorPresets[] = {
            { "Black",    0.05f, 0.05f, 0.05f },
            { "Brown",    0.35f, 0.20f, 0.10f },
            { "Blonde",   0.85f, 0.75f, 0.50f },
            { "Red",      0.60f, 0.15f, 0.10f },
            { "Gray",     0.60f, 0.60f, 0.60f },
            { "White",    0.90f, 0.90f, 0.90f },
        };
        
        for (int i = 0; i < 6; i++) {
            ImVec4 col(colorPresets[i].r, colorPresets[i].g, colorPresets[i].b, 1.0f);
            ImGui::PushID(i);
            if (ImGui::ColorButton(colorPresets[i].name, col, 0, ImVec2(36, 24))) {
                m_state.selectedHairColor = i;
            }
            ImGui::PopID();
            if (i < 5) ImGui::SameLine(0, 6);
        }
    }
}

// ============================================================================
// UI Rendering - Clothing
// ============================================================================

inline void CharacterModeHandler::renderClothingUI() {
    using luma::ui::loc;
    
    if (!m_state.character) return;
    
    ImGui::Text("%s", loc("Clothing"));
    ImGui::Spacing();
    
    // Clothing categories
    const char* categories[] = { "Top", "Bottom", "Shoes", "Accessory" };
    static int selectedCat = 0;
    
    for (int i = 0; i < 4; i++) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool sel = (selectedCat == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
        if (ImGui::Button(loc(categories[i]), ImVec2(65, 24))) selectedCat = i;
        if (sel) ImGui::PopStyleColor();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Clothing library
    auto& clothingLib = ClothingLibrary::getInstance();
    clothingLib.initializeDefaults();
    
    ClothingCategory category = ClothingCategory::Top;
    switch (selectedCat) {
        case 0: category = ClothingCategory::Top; break;
        case 1: category = ClothingCategory::Bottom; break;
        case 2: category = ClothingCategory::Footwear; break;
        case 3: category = ClothingCategory::Accessory; break;
    }
    
    auto items = clothingLib.getAssetsByCategory(category);
    auto& clothing = m_state.character->getClothing();
    
    for (const auto* item : items) {
        bool equipped = clothing.isEquipped(item->id);
        if (ImGui::Checkbox(item->name.c_str(), &equipped)) {
            if (equipped) {
                clothing.equipItem(item->id);
            } else {
                clothing.unequipItem(item->id);
            }
            m_state.meshNeedsUpdate = true;
        }
    }
    
    if (items.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", loc("No items available"));
    }
}

// ============================================================================
// UI Rendering - Expression
// ============================================================================

inline void CharacterModeHandler::renderExpressionUI() {
    using luma::ui::loc;
    
    if (!m_state.character) return;
    
    ImGui::Text("%s", loc("Expression"));
    ImGui::Spacing();
    
    // Expression intensity slider
    ImGui::SliderFloat(loc("Intensity"), &m_state.expressionIntensity, 0.0f, 1.0f);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Expression presets
    ImGui::Text("%s:", loc("Preset Expressions"));
    ImGui::Spacing();
    
    struct ExprPreset { const char* name; const char* displayName; };
    ExprPreset presets[] = {
        { "neutral",  "Neutral" },
        { "smile",    "Smile" },
        { "frown",    "Frown" },
        { "surprise", "Surprise" },
        { "angry",    "Angry" },
    };
    
    for (int i = 0; i < 5; i++) {
        bool selected = (m_state.currentExpression == presets[i].name);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.53f, 0.96f, 1.0f));
        
        if (ImGui::Button(loc(presets[i].displayName), ImVec2(130, 28))) {
            m_state.currentExpression = presets[i].name;
            m_state.character->getFace().setExpression(presets[i].name, m_state.expressionIntensity);
            m_state.meshNeedsUpdate = true;
        }
        
        if (selected) ImGui::PopStyleColor();
        
        if (i % 2 == 0) ImGui::SameLine(0, 8);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Manual ARKit blend shape controls
    if (ImGui::CollapsingHeader(loc("Advanced Blend Shapes"))) {
        auto& expr = m_state.character->getFace().getExpressionParams();
        bool changed = false;
        
        // Eyes
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "%s", loc("Eyes"));
        changed |= ImGui::SliderFloat("Blink L", &expr.eyeBlinkLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Blink R", &expr.eyeBlinkRight, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Wide L", &expr.eyeWideLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Wide R", &expr.eyeWideRight, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Squint L", &expr.eyeSquintLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Squint R", &expr.eyeSquintRight, 0.0f, 1.0f);
        
        // Mouth
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "%s", loc("Mouth"));
        changed |= ImGui::SliderFloat("Jaw Open", &expr.jawOpen, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Smile L", &expr.mouthSmileLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Smile R", &expr.mouthSmileRight, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Frown L", &expr.mouthFrownLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Frown R", &expr.mouthFrownRight, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Pucker", &expr.mouthPucker, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Funnel", &expr.mouthFunnel, 0.0f, 1.0f);
        
        // Brow
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "%s", loc("Brows"));
        changed |= ImGui::SliderFloat("Brow Down L", &expr.browDownLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Brow Down R", &expr.browDownRight, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Brow Inner Up", &expr.browInnerUp, 0.0f, 1.0f);
        
        // Cheek
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "%s", loc("Cheeks"));
        changed |= ImGui::SliderFloat("Cheek Puff", &expr.cheekPuff, 0.0f, 1.0f);
        
        // Nose
        changed |= ImGui::SliderFloat("Nose Sneer L", &expr.noseSneerLeft, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Nose Sneer R", &expr.noseSneerRight, 0.0f, 1.0f);
        
        if (changed) {
            m_state.character->getFace().setExpressionParams(expr);
            m_state.meshNeedsUpdate = true;
        }
    }
}

// ============================================================================
// UI Rendering - Export
// ============================================================================

inline void CharacterModeHandler::renderExportUI() {
    using luma::ui::loc;
    
    if (!m_state.character) return;
    
    ImGui::Text("%s", loc("Export Character"));
    ImGui::Spacing();
    
    ImGui::Text("%s: %s", loc("Character"), m_state.character->getName().c_str());
    ImGui::Text("%s: %u", loc("Vertices"), m_state.renderer.getVertexCount());
    ImGui::Text("%s: %u", loc("Triangles"), m_state.renderer.getIndexCount() / 3);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("%s:", loc("Export Format"));
    ImGui::Spacing();
    
    struct FormatInfo { const char* name; const char* ext; CharacterExportFormat fmt; };
    FormatInfo formats[] = {
        { "glTF 2.0 (.glb)",    ".glb", CharacterExportFormat::GLTF },
        { "FBX (.fbx)",         ".fbx", CharacterExportFormat::FBX },
        { "OBJ (.obj)",         ".obj", CharacterExportFormat::OBJ },
        { "VRM (.vrm)",         ".vrm", CharacterExportFormat::VRM },
        { "USD (.usd)",         ".usd", CharacterExportFormat::USD },
    };
    
    for (const auto& fmt : formats) {
        if (ImGui::Button(fmt.name, ImVec2(-1, 28))) {
            // Export would use SaveFileDialog - connected in main.cpp
            printf("[CharacterMode] Export requested: %s\n", fmt.name);
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Apply to scene
    if (ImGui::Button(loc("Apply to Scene"), ImVec2(-1, 32))) {
        applyToSceneEntity();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc("Update the character in the scene with current settings"));
    }
}

// ============================================================================
// Sub-mode switching
// ============================================================================

inline void CharacterModeHandler::setSubMode(CharacterSubMode subMode) {
    if (m_state.subMode == subMode) return;
    m_state.subMode = subMode;
}

// ============================================================================
// Landmark Editor Overlay (screen-space rendering)
// ============================================================================

inline bool CharacterModeHandler::projectToScreen(float wx, float wy, float wz, float& sx, float& sy) {
    if (m_projectionCallback) {
        return m_projectionCallback(wx, wy, wz, sx, sy);
    }
    return false;
}

inline void CharacterModeHandler::renderLandmarkEditorOverlay() {
    if (!m_state.landmarkEditor.isActive()) return;
    if (!m_ctx || !m_ctx->renderer) return;
    
    const auto& opts = m_state.landmarkEditor.getDisplayOptions();
    const auto& vertices = m_state.landmarkEditor.getVertices();
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // 获取相机位置用于背面剔除
    Vec3 cameraPos = m_ctx->renderer->getCameraPosition();
    
    // 背面剔除辅助函数：检查顶点法线是否朝向相机
    auto isFacingCamera = [&](const Vertex& v) -> bool {
        // 计算从顶点到相机的方向
        Vec3 toCamera(
            cameraPos.x - v.position[0],
            cameraPos.y - v.position[1],
            cameraPos.z - v.position[2]
        );
        // 点积判断：法线与相机方向同向则可见
        float dot = v.normal[0] * toCamera.x + v.normal[1] * toCamera.y + v.normal[2] * toCamera.z;
        return dot > 0;
    };
    
    // ========== 渲染顶点点云（屏幕空间圆点）==========
    if (!opts.showMesh || opts.showVertexDots) {
        int hoveredVert = m_state.landmarkEditor.getHoveredVertex();
        int hoveredLm = m_state.landmarkEditor.getHoveredLandmark();
        
        ImU32 normalColor = IM_COL32(100, 100, 100, 180);
        ImU32 hoverColor = IM_COL32(0, 255, 255, 255);
        
        // 先画普通顶点（背面剔除）
        for (size_t i = 0; i < vertices.size(); i++) {
            // 跳过 hover 的顶点（后面单独画）
            if ((int)i == hoveredVert && hoveredLm < 0) continue;
            
            const auto& v = vertices[i];
            
            // 背面剔除：跳过背对相机的顶点
            if (!isFacingCamera(v)) continue;
            
            float sx, sy;
            if (projectToScreen(v.position[0], v.position[1], v.position[2], sx, sy)) {
                drawList->AddCircleFilled(ImVec2(sx, sy), 2.0f, normalColor, 6);
            }
        }
        
        // 画 hover 的顶点（如果不是 landmark，且面向相机）
        if (hoveredVert >= 0 && hoveredLm < 0 && hoveredVert < (int)vertices.size()) {
            const auto& v = vertices[hoveredVert];
            if (isFacingCamera(v)) {
                float sx, sy;
                if (projectToScreen(v.position[0], v.position[1], v.position[2], sx, sy)) {
                    drawList->AddCircleFilled(ImVec2(sx, sy), 5.0f, hoverColor, 8);
                    drawList->AddCircle(ImVec2(sx, sy), 5.0f, IM_COL32(255, 255, 255, 200), 8, 1.5f);
                }
            }
        }
    }
    
    // ========== 渲染 Landmark 点（屏幕空间）==========
    if (opts.showLandmarks) {
        int currentId = m_state.landmarkEditor.getCurrentLandmark();
        int hoveredLm = m_state.landmarkEditor.getHoveredLandmark();
        const char* currentRegion = m_state.landmarkEditor.getCurrentRegion();
        
        for (int i = 0; i < 68; i++) {
            Vec3 pos;
            if (!m_state.landmarkEditor.getLandmarkPosition(i, pos)) continue;
            
            float sx, sy;
            if (!projectToScreen(pos.x, pos.y, pos.z, sx, sy)) continue;
            
            Vec3 color = getRegionColor(LANDMARK_DEFS[i].region);
            float radius = 4.0f;
            float alpha = 1.0f;
            
            // 当前 landmark - 黄色，更大
            if (i == currentId) {
                color = Vec3(1.0f, 1.0f, 0.0f);
                radius = 6.0f;
            }
            // Hover 的 landmark - 青色，更大
            else if (i == hoveredLm) {
                color = Vec3(0.0f, 1.0f, 1.0f);
                radius = 5.5f;
            }
            // 高亮当前区域
            else if (opts.highlightRegion && strcmp(LANDMARK_DEFS[i].region, currentRegion) == 0) {
                radius = 4.5f;
            }
            // 其他区域的 landmark 稍暗
            else if (opts.highlightRegion) {
                alpha = 0.5f;
                radius = 3.0f;
            }
            
            ImU32 fillColor = IM_COL32(
                (int)(color.x * 255 * alpha),
                (int)(color.y * 255 * alpha),
                (int)(color.z * 255 * alpha),
                (int)(220 * alpha)
            );
            ImU32 outlineColor = IM_COL32(255, 255, 255, (int)(200 * alpha));
            
            drawList->AddCircleFilled(ImVec2(sx, sy), radius, fillColor, 8);
            drawList->AddCircle(ImVec2(sx, sy), radius, outlineColor, 8, 1.5f);
        }
    }
    
    // ========== 网格模式下单独渲染 hover 顶点 ==========
    if (opts.showMesh && !opts.showVertexDots) {
        int hoveredVert = m_state.landmarkEditor.getHoveredVertex();
        int hoveredLm = m_state.landmarkEditor.getHoveredLandmark();
        
        if (hoveredVert >= 0 && hoveredLm < 0) {
            Vec3 pos;
            if (m_state.landmarkEditor.getHoveredPosition(pos)) {
                float sx, sy;
                if (projectToScreen(pos.x, pos.y, pos.z, sx, sy)) {
                    ImU32 hoverColor = IM_COL32(0, 255, 255, 255);
                    drawList->AddCircleFilled(ImVec2(sx, sy), 5.0f, hoverColor, 8);
                    drawList->AddCircle(ImVec2(sx, sy), 5.0f, IM_COL32(255, 255, 255, 200), 8, 1.5f);
                }
            }
        }
    }
}

} // namespace editor
} // namespace luma
