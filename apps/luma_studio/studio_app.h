// LUMA Studio Application
// Main application class extracted from main.cpp
// Manages renderer, scene, editor modes, and modules

#pragma once

#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/draw_manager.h"
#include "engine/renderer/shader_manager.h"
#include "engine/scene/scene.h"
#include "engine/editor/editor_mode.h"
#include "engine/editor/edit_module.h"
#include "engine/editor/mesh_edit/mesh_edit_module.h"
#include "engine/editor/uv_edit/uv_edit_module.h"
#include "engine/editor/material_edit/material_edit_module.h"
#include "engine/mesh/edit_mesh.h"

#include <memory>
#include <functional>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace luma {

// Application configuration
struct StudioConfig {
    int windowWidth = 1600;
    int windowHeight = 900;
    std::string windowTitle = "LUMA Studio";
    bool vsyncEnabled = true;
    bool shadowsEnabled = true;
    bool postProcessEnabled = true;
};

// Studio Application
class StudioApp {
public:
    StudioApp();
    ~StudioApp();
    
    // Lifecycle
#if defined(_WIN32)
    bool init(HWND hwnd, int width, int height);
#endif
    void shutdown();
    
    // Frame update and render
    void update(float deltaTime);
    void render();
    void resize(int width, int height);
    
    // Mode management
    void setEditorMode(editor::EditorMode mode);
    editor::EditorMode getEditorMode() const { return currentMode; }
    
    void setViewMode(editor::ViewMode mode);
    editor::ViewMode getViewMode() const { return currentViewMode; }
    
    // Scene management
    Scene& getScene() { return scene; }
    const Scene& getScene() const { return scene; }
    
    void newScene();
    bool loadScene(const std::string& path);
    bool saveScene(const std::string& path);
    bool loadModel(const std::string& path);
    
    // Edit mesh
    EditMesh* getEditMesh() { return editMesh.get(); }
    void enterEditMode(Entity* entity);
    void exitEditMode(bool apply);
    bool isInEditMode() const { return currentMode == editor::EditorMode::Edit && editMesh != nullptr; }
    
    // Edit modules
    editor::MeshEditModule* getMeshEditModule() { return meshEditModule.get(); }
    editor::UVEditModule* getUVEditModule() { return uvEditModule.get(); }
    editor::MaterialEditModule* getMaterialEditModule() { return materialEditModule.get(); }
    
    // Renderer access
    UnifiedRenderer* getRenderer() { return renderer.get(); }
    DrawManager* getDrawManager() { return drawManager.get(); }
    
    // Camera
    void setCameraPosition(float x, float y, float z);
    void setCameraTarget(float x, float y, float z);
    void orbitCamera(float deltaYaw, float deltaPitch);
    void panCamera(float deltaX, float deltaY);
    void zoomCamera(float delta);
    void resetCamera();
    
    // Selection
    Entity* getSelectedEntity() { return scene.getSelectedEntity(); }
    void selectEntity(Entity* entity);
    void clearSelection();
    
    // Callbacks
    using RenderCallback = std::function<void()>;
    void setPreRenderCallback(RenderCallback cb) { preRenderCallback = cb; }
    void setPostRenderCallback(RenderCallback cb) { postRenderCallback = cb; }
    
    using ModeChangedCallback = std::function<void(editor::EditorMode)>;
    void setModeChangedCallback(ModeChangedCallback cb) { modeChangedCallback = cb; }
    
    // State queries
    bool isReady() const { return ready; }
    int getViewportWidth() const { return viewportWidth; }
    int getViewportHeight() const { return viewportHeight; }
    
    // Stats
    float getFrameTime() const { return frameTime; }
    int getTriangleCount() const { return triangleCount; }
    int getDrawCallCount() const { return drawCallCount; }
    
private:
    // Core systems
    std::unique_ptr<UnifiedRenderer> renderer;
    std::unique_ptr<DrawManager> drawManager;
    Scene scene;
    
    // Edit mode
    std::unique_ptr<EditMesh> editMesh;
    Entity* editingEntity = nullptr;
    
    // Edit modules
    std::unique_ptr<editor::MeshEditModule> meshEditModule;
    std::unique_ptr<editor::UVEditModule> uvEditModule;
    std::unique_ptr<editor::MaterialEditModule> materialEditModule;
    
    // State
    editor::EditorMode currentMode = editor::EditorMode::Scene;
    editor::ViewMode currentViewMode = editor::ViewMode::Material;
    bool ready = false;
    
    // Viewport
    int viewportWidth = 800;
    int viewportHeight = 600;
    
    // Camera
    float cameraPos[3] = {0, 2, 5};
    float cameraTarget[3] = {0, 0, 0};
    float cameraYaw = 0;
    float cameraPitch = 0;
    float cameraDistance = 5.0f;
    
    // Light
    float lightDirection[3] = {0.5f, -0.7f, 0.5f};
    
    // Stats
    float frameTime = 0;
    int triangleCount = 0;
    int drawCallCount = 0;
    
    // Callbacks
    RenderCallback preRenderCallback;
    RenderCallback postRenderCallback;
    ModeChangedCallback modeChangedCallback;
    
    // Internal methods
    void updateCamera();
    void setupRenderContext(RenderContext& ctx);
    void renderSceneMode();
    void renderEditMode();
};

// ============================================================================
// Implementation
// ============================================================================

inline StudioApp::StudioApp() = default;

inline StudioApp::~StudioApp() {
    shutdown();
}

#if defined(_WIN32)
inline bool StudioApp::init(HWND hwnd, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    
    // Initialize shader manager
    ShaderManager::instance().init();
    
    // Create renderer
    renderer = std::make_unique<UnifiedRenderer>();
    if (!renderer->init(hwnd, width, height)) {
        return false;
    }
    
    // Create draw manager
    drawManager = std::make_unique<DrawManager>();
    if (!drawManager->init(renderer.get())) {
        return false;
    }
    
    // Create edit modules
    meshEditModule = std::make_unique<editor::MeshEditModule>();
    meshEditModule->init(renderer.get());
    
    uvEditModule = std::make_unique<editor::UVEditModule>();
    uvEditModule->init(renderer.get());
    
    materialEditModule = std::make_unique<editor::MaterialEditModule>();
    materialEditModule->init(renderer.get());
    
    ready = true;
    return true;
}
#endif

inline void StudioApp::shutdown() {
    editMesh.reset();
    meshEditModule.reset();
    uvEditModule.reset();
    materialEditModule.reset();
    drawManager.reset();
    renderer.reset();
    ready = false;
}

inline void StudioApp::update(float deltaTime) {
    frameTime = deltaTime;
    
    // Update active edit module
    if (currentMode == editor::EditorMode::Edit) {
        if (meshEditModule && meshEditModule->isActive()) {
            meshEditModule->update(deltaTime);
        }
        if (uvEditModule && uvEditModule->isActive()) {
            uvEditModule->update(deltaTime);
        }
    }
}

inline void StudioApp::render() {
    if (!ready || !renderer) return;
    
    // Pre-render callback
    if (preRenderCallback) preRenderCallback();
    
    // Begin frame
    renderer->beginFrame();
    
    // Setup render context
    RenderContext ctx;
    setupRenderContext(ctx);
    
    // Render based on mode
    if (currentMode == editor::EditorMode::Edit && editMesh) {
        renderEditMode();
    } else {
        renderSceneMode();
    }
    
    // End frame
    renderer->endFrame();
    
    // Post-render callback
    if (postRenderCallback) postRenderCallback();
}

inline void StudioApp::resize(int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    if (renderer) {
        renderer->resize(width, height);
    }
}

inline void StudioApp::setEditorMode(editor::EditorMode mode) {
    if (currentMode == mode) return;
    
    // Exit current mode
    if (currentMode == editor::EditorMode::Edit) {
        if (meshEditModule) meshEditModule->onExit();
        if (uvEditModule) uvEditModule->onExit();
    }
    
    currentMode = mode;
    
    // Enter new mode
    if (currentMode == editor::EditorMode::Edit) {
        if (meshEditModule) meshEditModule->onEnter();
    }
    
    // Notify callback
    if (modeChangedCallback) {
        modeChangedCallback(currentMode);
    }
}

inline void StudioApp::setViewMode(editor::ViewMode mode) {
    currentViewMode = mode;
}

inline void StudioApp::newScene() {
    scene.clear();
    editMesh.reset();
    editingEntity = nullptr;
    setEditorMode(editor::EditorMode::Scene);
}

inline bool StudioApp::loadScene(const std::string& path) {
    // TODO: Implement scene loading
    (void)path;
    return false;
}

inline bool StudioApp::saveScene(const std::string& path) {
    // TODO: Implement scene saving
    (void)path;
    return false;
}

inline bool StudioApp::loadModel(const std::string& path) {
    if (!renderer) return false;
    
    RHILoadedModel model;
    if (!renderer->loadModel(path, model)) {
        return false;
    }
    
    // Add to scene
    auto* entity = scene.createEntity(path);
    entity->model = model;
    entity->hasModel = true;
    
    scene.selectEntity(entity);
    resetCamera();
    
    return true;
}

inline void StudioApp::enterEditMode(Entity* entity) {
    if (!entity || !entity->hasModel) return;
    
    // Create edit mesh from model
    editMesh = std::make_unique<EditMesh>();
    editingEntity = entity;
    
    // Build edit mesh from GPU data
    if (renderer) {
        renderer->buildEditMeshFromGPU(entity->model, *editMesh);
    }
    
    // Setup edit modules
    if (meshEditModule) {
        meshEditModule->setEditMesh(editMesh.get());
    }
    if (uvEditModule) {
        uvEditModule->setEditMesh(editMesh.get());
    }
    if (materialEditModule) {
        materialEditModule->setEditMesh(editMesh.get());
    }
    
    setEditorMode(editor::EditorMode::Edit);
}

inline void StudioApp::exitEditMode(bool apply) {
    if (!editMesh) return;
    
    if (apply && editingEntity && renderer) {
        // Apply changes back to model
        // This would convert EditMesh to RenderMesh and upload to GPU
    }
    
    editMesh.reset();
    editingEntity = nullptr;
    
    // Clear modules
    if (meshEditModule) meshEditModule->setEditMesh(nullptr);
    if (uvEditModule) uvEditModule->setEditMesh(nullptr);
    if (materialEditModule) materialEditModule->setEditMesh(nullptr);
    
    setEditorMode(editor::EditorMode::Scene);
}

inline void StudioApp::setCameraPosition(float x, float y, float z) {
    cameraPos[0] = x;
    cameraPos[1] = y;
    cameraPos[2] = z;
}

inline void StudioApp::setCameraTarget(float x, float y, float z) {
    cameraTarget[0] = x;
    cameraTarget[1] = y;
    cameraTarget[2] = z;
}

inline void StudioApp::orbitCamera(float deltaYaw, float deltaPitch) {
    cameraYaw += deltaYaw;
    cameraPitch += deltaPitch;
    cameraPitch = std::max(-89.0f, std::min(89.0f, cameraPitch));
    updateCamera();
}

inline void StudioApp::panCamera(float deltaX, float deltaY) {
    // TODO: Implement camera panning
    (void)deltaX;
    (void)deltaY;
}

inline void StudioApp::zoomCamera(float delta) {
    cameraDistance = std::max(0.5f, cameraDistance - delta);
    updateCamera();
}

inline void StudioApp::resetCamera() {
    cameraYaw = 0;
    cameraPitch = 20;
    cameraDistance = 5.0f;
    cameraTarget[0] = cameraTarget[1] = cameraTarget[2] = 0;
    updateCamera();
}

inline void StudioApp::updateCamera() {
    float yawRad = cameraYaw * 3.14159f / 180.0f;
    float pitchRad = cameraPitch * 3.14159f / 180.0f;
    
    cameraPos[0] = cameraTarget[0] + cameraDistance * std::cos(pitchRad) * std::sin(yawRad);
    cameraPos[1] = cameraTarget[1] + cameraDistance * std::sin(pitchRad);
    cameraPos[2] = cameraTarget[2] + cameraDistance * std::cos(pitchRad) * std::cos(yawRad);
}

inline void StudioApp::selectEntity(Entity* entity) {
    scene.selectEntity(entity);
}

inline void StudioApp::clearSelection() {
    scene.selectEntity(nullptr);
}

inline void StudioApp::setupRenderContext(RenderContext& ctx) {
    ctx.editorMode = currentMode;
    ctx.viewMode = static_cast<RenderViewMode>(currentViewMode);
    ctx.viewportWidth = viewportWidth;
    ctx.viewportHeight = viewportHeight;
    
    // Camera
    ctx.camera.eyeX = cameraPos[0];
    ctx.camera.eyeY = cameraPos[1];
    ctx.camera.eyeZ = cameraPos[2];
    ctx.camera.targetX = cameraTarget[0];
    ctx.camera.targetY = cameraTarget[1];
    ctx.camera.targetZ = cameraTarget[2];
    ctx.camera.fov = 45.0f;
    ctx.camera.aspect = static_cast<float>(viewportWidth) / viewportHeight;
    ctx.camera.nearPlane = 0.1f;
    ctx.camera.farPlane = 1000.0f;
    
    // Light
    ctx.lightDirection[0] = lightDirection[0];
    ctx.lightDirection[1] = lightDirection[1];
    ctx.lightDirection[2] = lightDirection[2];
}

inline void StudioApp::renderSceneMode() {
    if (!renderer) return;
    
    // Setup camera
    RHICameraParams camera;
    camera.eyeX = cameraPos[0];
    camera.eyeY = cameraPos[1];
    camera.eyeZ = cameraPos[2];
    camera.targetX = cameraTarget[0];
    camera.targetY = cameraTarget[1];
    camera.targetZ = cameraTarget[2];
    camera.fov = 45.0f;
    camera.aspect = static_cast<float>(viewportWidth) / viewportHeight;
    camera.nearPlane = 0.1f;
    camera.farPlane = 1000.0f;
    
    renderer->setCamera(camera);
    renderer->setLightDirection(lightDirection[0], lightDirection[1], lightDirection[2]);
    
    // Render all entities
    scene.traverseRenderables([this](Entity* entity) {
        if (!entity->hasModel) return;
        
        switch (currentViewMode) {
            case editor::ViewMode::Material:
                if (entity->hasSkeleton && entity->animator) {
                    renderer->renderSkinnedModel(entity->model, entity->worldMatrix.m,
                        entity->animator->getBoneMatrices(), entity->animator->getBoneCount());
                } else {
                    renderer->renderModel(entity->model, entity->worldMatrix.m);
                }
                break;
            case editor::ViewMode::Solid:
                renderer->renderModelSolid(entity->model, entity->worldMatrix.m);
                break;
            case editor::ViewMode::Wireframe:
                renderer->renderModelWireframe(entity->model, entity->worldMatrix.m);
                break;
        }
    });
    
    // Render selection outline
    if (auto* selected = scene.getSelectedEntity()) {
        if (selected->hasModel) {
            float outlineColor[4] = {1.0f, 0.6f, 0.2f, 1.0f};
            renderer->renderModelOutline(selected->model, selected->worldMatrix.m, outlineColor);
        }
    }
    
    // Finish rendering
    renderer->finishSceneRendering();
}

inline void StudioApp::renderEditMode() {
    if (!renderer || !editMesh || !editingEntity) return;
    
    // Setup camera
    RHICameraParams camera;
    camera.eyeX = cameraPos[0];
    camera.eyeY = cameraPos[1];
    camera.eyeZ = cameraPos[2];
    camera.targetX = cameraTarget[0];
    camera.targetY = cameraTarget[1];
    camera.targetZ = cameraTarget[2];
    camera.fov = 45.0f;
    camera.aspect = static_cast<float>(viewportWidth) / viewportHeight;
    camera.nearPlane = 0.1f;
    camera.farPlane = 1000.0f;
    
    renderer->setCamera(camera);
    renderer->setLightDirection(lightDirection[0], lightDirection[1], lightDirection[2]);
    
    // Render based on view mode
    switch (currentViewMode) {
        case editor::ViewMode::Material:
            renderer->renderModel(editingEntity->model, editingEntity->worldMatrix.m);
            break;
        case editor::ViewMode::Solid:
            renderer->renderModelSolid(editingEntity->model, editingEntity->worldMatrix.m);
            break;
        case editor::ViewMode::Wireframe:
            if (editingEntity->hasSkeleton && editingEntity->animator) {
                renderer->renderOriginalEdgesSkinned(editingEntity->model, editingEntity->worldMatrix.m,
                    editingEntity->animator->getBoneMatrices(), editingEntity->animator->getBoneCount());
            } else {
                renderer->renderOriginalEdges(editingEntity->model, editingEntity->worldMatrix.m);
            }
            break;
    }
    
    // Render selection highlights
    // TODO: Render selected vertices/edges/faces using gizmo lines
    
    // Finish rendering
    renderer->finishSceneRendering();
}

} // namespace luma
