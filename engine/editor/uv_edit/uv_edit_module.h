// LUMA UV Edit Module
// Modular UV editing functionality

#pragma once

#include "engine/editor/edit_module.h"
#include "engine/editor/uv_editor.h"
#include "engine/mesh/edit_mesh.h"
#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/draw_manager.h"
#include <memory>

namespace luma {
namespace editor {

// UV Projection Method
enum class UVProjectionMethod {
    Planar,
    Box,
    Cylindrical,
    Spherical,
    FromView
};

// UV Edit Module
class UVEditModule : public EditModule {
public:
    UVEditModule();
    ~UVEditModule() override = default;
    
    // Initialize
    bool init(UnifiedRenderer* renderer);
    
    // Set mesh to edit
    void setEditMesh(EditMesh* mesh);
    
    // EditModule interface
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render(DrawManager& drawManager, const RenderContext& ctx) override;
    void renderUI() override;
    bool handleInput(const InputEvent& event) override;
    void clearSelection() override;
    void selectAll() override;
    
    // UV Projection
    void projectPlanar(const float* normal = nullptr);
    void projectBox();
    void projectCylindrical();
    void projectSpherical();
    void projectFromView(const float* viewMatrix);
    
    // UV Unwrapping (advanced)
    void unwrapLSCM();
    void unwrapABF();
    void unwrapSmartProject();
    
    // UV Transform
    void translateUV(float du, float dv);
    void rotateUV(float angle);
    void scaleUV(float su, float sv);
    void flipUVHorizontal();
    void flipUVVertical();
    
    // UV Island operations
    void packIslands();
    void averageIslandScale();
    
    // UV Analysis
    void checkStretching();
    void checkOverlapping();
    void highlightProblems(bool enable);
    
    // UI state
    bool isUVEditorVisible() const { return uvEditorVisible; }
    void showUVEditor(bool show) { uvEditorVisible = show; }
    
private:
    UnifiedRenderer* renderer = nullptr;
    EditMesh* editMesh = nullptr;
    std::unique_ptr<UVEditor> uvEditor;
    bool uvEditorVisible = true;
    
    void markDirty() { dirty = true; }
};

// ============================================================================
// Implementation
// ============================================================================

inline UVEditModule::UVEditModule() : EditModule("UV Edit") {
}

inline bool UVEditModule::init(UnifiedRenderer* r) {
    renderer = r;
    uvEditor = std::make_unique<UVEditor>();
    return renderer != nullptr;
}

inline void UVEditModule::setEditMesh(EditMesh* mesh) {
    editMesh = mesh;
    // UVEditor doesn't have setEditMesh yet - placeholder
}

inline void UVEditModule::onEnter() {
    active = true;
    uvEditorVisible = true;
}

inline void UVEditModule::onExit() {
    active = false;
}

inline void UVEditModule::update(float deltaTime) {
    (void)deltaTime;
}

inline void UVEditModule::render(DrawManager& drawManager, const RenderContext& ctx) {
    (void)drawManager;
    (void)ctx;
    // UV editor is rendered separately via renderUI
}

inline void UVEditModule::renderUI() {
    if (uvEditorVisible && uvEditor && editMesh) {
        uvEditor->draw();  // Use existing draw() method
    }
}

inline bool UVEditModule::handleInput(const InputEvent& event) {
    if (!editMesh) return false;
    
    // Handle UV editor specific shortcuts
    if (event.type == InputEvent::Type::KeyDown) {
        // Project shortcuts
        if (event.key == 'P' && event.isCtrl()) {
            projectPlanar();
            return true;
        }
        if (event.key == 'B' && event.isCtrl()) {
            projectBox();
            return true;
        }
    }
    
    return false;
}

inline void UVEditModule::clearSelection() {
    // UV selection would be separate from mesh selection
}

inline void UVEditModule::selectAll() {
    // Select all UV vertices
}

inline void UVEditModule::projectPlanar(const float* normal) {
    if (!editMesh) return;
    float defaultNormal[3] = {0, 0, 1};
    editMesh->projectUVPlanar(normal ? normal : defaultNormal);
    markDirty();
}

inline void UVEditModule::projectBox() {
    if (!editMesh) return;
    editMesh->projectUVBox();
    markDirty();
}

inline void UVEditModule::projectCylindrical() {
    // TODO: Implement cylindrical UV projection
    // editMesh->projectUVCylindrical();
    markDirty();
}

inline void UVEditModule::projectSpherical() {
    // TODO: Implement spherical UV projection
    // editMesh->projectUVSpherical();
    markDirty();
}

inline void UVEditModule::projectFromView(const float* viewMatrix) {
    // TODO: Implement view projection
    (void)viewMatrix;
}

inline void UVEditModule::unwrapLSCM() {
    // TODO: Implement LSCM unwrapping
}

inline void UVEditModule::unwrapABF() {
    // TODO: Implement ABF unwrapping
}

inline void UVEditModule::unwrapSmartProject() {
    // TODO: Implement smart projection
}

inline void UVEditModule::translateUV(float du, float dv) {
    if (!editMesh) return;
    // Manual UV translation
    for (auto& face : editMesh->faces) {
        for (auto& loop : face.loops) {
            loop.uv[0] += du;
            loop.uv[1] += dv;
        }
    }
    markDirty();
}

inline void UVEditModule::rotateUV(float angle) {
    if (!editMesh) return;
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    for (auto& face : editMesh->faces) {
        for (auto& loop : face.loops) {
            float u = loop.uv[0] - 0.5f;
            float v = loop.uv[1] - 0.5f;
            loop.uv[0] = u * cosA - v * sinA + 0.5f;
            loop.uv[1] = u * sinA + v * cosA + 0.5f;
        }
    }
    markDirty();
}

inline void UVEditModule::scaleUV(float su, float sv) {
    if (!editMesh) return;
    for (auto& face : editMesh->faces) {
        for (auto& loop : face.loops) {
            loop.uv[0] = (loop.uv[0] - 0.5f) * su + 0.5f;
            loop.uv[1] = (loop.uv[1] - 0.5f) * sv + 0.5f;
        }
    }
    markDirty();
}

inline void UVEditModule::flipUVHorizontal() {
    if (!editMesh) return;
    for (auto& face : editMesh->faces) {
        for (auto& loop : face.loops) {
            loop.uv[0] = 1.0f - loop.uv[0];
        }
    }
    markDirty();
}

inline void UVEditModule::flipUVVertical() {
    if (!editMesh) return;
    for (auto& face : editMesh->faces) {
        for (auto& loop : face.loops) {
            loop.uv[1] = 1.0f - loop.uv[1];
        }
    }
    markDirty();
}

inline void UVEditModule::packIslands() {
    // TODO: Implement island packing
}

inline void UVEditModule::averageIslandScale() {
    // TODO: Implement average island scale
}

inline void UVEditModule::checkStretching() {
    if (!editMesh) return;
    auto problems = editMesh->analyzeUV();
    // Report stretching problems
    (void)problems;
}

inline void UVEditModule::checkOverlapping() {
    if (!editMesh) return;
    auto problems = editMesh->analyzeUV();
    // Report overlapping problems
    (void)problems;
}

inline void UVEditModule::highlightProblems(bool enable) {
    // TODO: Visual highlight of UV problems
    (void)enable;
}

} // namespace editor
} // namespace luma
