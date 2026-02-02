// LUMA Studio - Metal View Implementation with ImGui
// Using UnifiedRenderer (RHI-based), SceneGraph, and Editor UI

#import "LumaView.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/post_process.h"
#include "engine/renderer/mesh.h"
#include "engine/scene/scene_graph.h"
#include "engine/editor/gizmo.h"
#include "engine/ui/editor_ui.h"
#include "engine/serialization/scene_serializer.h"
#include "engine/asset/model_loader.h"
#include "engine/asset/asset_manager.h"
#include "engine/editor/command.h"
#include "engine/editor/commands/transform_commands.h"
#include "engine/editor/commands/scene_commands.h"
#include "engine/rendering/culling.h"
#include "engine/rendering/lod.h"
#include "engine/rendering/instancing.h"
#include "engine/rendering/render_optimizer.h"
#include "engine/particles/particle.h"
#include "engine/particles/particle_presets.h"
#include "engine/physics/physics_world.h"
#include "engine/physics/collision.h"
#include "engine/physics/constraints.h"
#include "engine/physics/physics_debug.h"
#include "engine/physics/raycast.h"
#include "engine/terrain/terrain.h"
#include "engine/terrain/terrain_generator.h"
#include "engine/terrain/foliage.h"
#include "engine/audio/audio.h"
#include "engine/renderer/gi/gi_system.h"
#include "engine/video/video_export.h"
#include "engine/network/network.h"
#include "engine/script/script_engine.h"
#include "engine/ai/navmesh.h"
#include "engine/ai/nav_agent.h"
#include "engine/ai/behavior_tree.h"
#include "engine/game_ui/ui_system.h"
#include "engine/scene/scene_manager.h"
#include "engine/data/data_system.h"
#include "engine/build/build_system.h"

// Character creation system
#include "engine/character/character.h"
#include "engine/character/base_human_loader.h"
#include "engine/character/character_renderer.h"
#include "engine/character/character_exporter.h"
#include "engine/character/clothing_system.h"
#include "engine/character/animation_retargeting.h"
#include "engine/character/stylized_rendering.h"
#include "engine/character/texture_system.h"
#include "engine/character/hair_system.h"
#include "engine/character/ai/face_reconstruction.h"
#include "engine/character/ai/ai_model_manager.h"

#include <memory>
#include <chrono>
#include <cmath>

// ImGui
#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_osx.h"

// Camera mode enum
enum class CameraMode { None, Orbit, Pan, Zoom };

@interface LumaView () {
    // Core systems
    std::unique_ptr<luma::UnifiedRenderer> _renderer;
    std::unique_ptr<luma::SceneGraph> _scene;
    std::unique_ptr<luma::TransformGizmo> _gizmo;
    luma::RHICameraParams _camera;
    
    // Editor state
    luma::ui::EditorState _editorState;
    luma::PostProcessSettings _postProcess;
    luma::ui::RenderSettings _renderSettings;
    luma::ui::LightSettings _lighting;
    luma::ui::AnimationState _animation;
    
    // Advanced rendering state (NEW)
    luma::ui::AdvancedPostProcessState _advancedPostProcess;
    luma::ui::AdvancedShadowState _advancedShadows;
    luma::ui::LODState _lodState;
    
    // Animation systems (for advanced UI)
    std::unique_ptr<luma::AnimationStateMachine> _stateMachine;
    std::unique_ptr<luma::AnimationLayerManager> _layerManager;
    std::unique_ptr<luma::IKManager> _ikManager;
    luma::BlendTree1D* _activeBlendTree1D;
    luma::BlendTree2D* _activeBlendTree2D;
    luma::Skeleton* _activeSkeleton;
    
    // Particle system
    luma::ui::ParticleEditorState _particleState;
    
    // Physics system
    luma::ui::PhysicsEditorState _physicsState;
    
    // Terrain system
    luma::ui::TerrainEditorState _terrainState;
    
    // Audio system
    luma::ui::AudioEditorState _audioState;
    
    // GI system
    luma::ui::GIEditorState _giState;
    
    // Video export
    luma::ui::VideoExportState _videoExportState;
    
    // Network
    luma::ui::NetworkPanelState _networkState;
    
    // Scripting
    luma::ui::ScriptEditorState _scriptState;
    
    // AI
    luma::ui::AIEditorState _aiState;
    
    // Game UI
    luma::ui::GameUIEditorState _gameUIState;
    
    // Scene/Data/Build
    luma::ui::SceneManagerState _sceneState;
    luma::ui::DataManagerState _dataState;
    luma::ui::BuildSettingsState _buildState;
    
    // Asset Browser & Visual Script
    luma::ui::AssetBrowserState _assetBrowserState;
    luma::ui::VisualScriptState _visualScriptState;
    
    // Character Creator
    luma::ui::CharacterCreatorState _characterCreatorState;
    std::unique_ptr<luma::Character> _character;
    std::unique_ptr<luma::CharacterRenderer> _characterRenderer;
    luma::RHIGPUMesh _characterMesh;
    bool _characterNeedsUpdate;
    
    // Clothing System
    std::unique_ptr<luma::ClothingManager> _clothingManager;
    std::vector<luma::RHIGPUMesh> _clothingMeshes;
    bool _clothingNeedsUpdate;
    
    // Animation System
    std::unique_ptr<luma::CharacterAnimationRetargeter> _animRetargeter;
    std::string _currentAnimation;
    float _animationTime;
    bool _isAnimationPlaying;
    
    // Texture System
    luma::CharacterTextureSet _characterTextures;
    luma::RHIGPUMesh _skinDiffuseTexture;
    luma::RHIGPUMesh _skinNormalTexture;
    bool _texturesNeedUpdate;
    
    // Hair System
    std::unique_ptr<luma::HairManager> _hairManager;
    luma::RHIGPUMesh _hairMesh;
    bool _hairNeedsUpdate;
    
    // Camera state
    CameraMode _cameraMode;
    float _lastMouseX;
    float _lastMouseY;
    
    // Settings
    bool _showGrid;
    bool _autoRotate;
    float _autoRotateSpeed;
    
    // Metal objects for ImGui
    id<MTLCommandQueue> _commandQueue;
    
    CVDisplayLinkRef _displayLink;
    std::chrono::high_resolution_clock::time_point _lastTime;
    float _totalTime;
    
    BOOL _imguiInitialized;
    
    // Scene path
    std::string _currentScenePath;
}

@end

// Display link callback
static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* context) {
    @autoreleasepool {
        LumaView* view = (__bridge LumaView*)context;
        dispatch_async(dispatch_get_main_queue(), ^{
            [view setNeedsDisplay:YES];
        });
    }
    return kCVReturnSuccess;
}

@implementation LumaView

- (CALayer*)makeBackingLayer {
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = MTLCreateSystemDefaultDevice();
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = NO;
    layer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
    return layer;
}

- (CAMetalLayer*)metalLayer {
    return (CAMetalLayer*)self.layer;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setWantsLayer:YES];
        [self.layer setBackgroundColor:[[NSColor blackColor] CGColor]];
        
        _imguiInitialized = NO;
        _lastTime = std::chrono::high_resolution_clock::now();
        _totalTime = 0.0f;
        
        // Initialize camera
        _camera = luma::RHICameraParams();
        _camera.yaw = 0.78f;
        _camera.pitch = 0.5f;
        _camera.distance = 1.0f;
        _cameraMode = CameraMode::None;
        
        // Settings
        _showGrid = true;
        _autoRotate = false;
        _autoRotateSpeed = 0.5f;
        
        // Initialize renderer after layer is ready
        dispatch_async(dispatch_get_main_queue(), ^{
            [self initializeRenderer];
            
            CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
            CVDisplayLinkSetOutputCallback(_displayLink, displayLinkCallback, (__bridge void*)self);
            CVDisplayLinkStart(_displayLink);
        });
        
        // Setup tracking area for mouse events
        NSTrackingAreaOptions options = NSTrackingMouseMoved | 
                                        NSTrackingActiveInKeyWindow |
                                        NSTrackingInVisibleRect;
        NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                                    options:options
                                                                      owner:self
                                                                   userInfo:nil];
        [self addTrackingArea:trackingArea];
    }
    return self;
}

- (void)dealloc {
    if (_displayLink) {
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
    }
    
    if (_imguiInitialized) {
        ImGui_ImplMetal_Shutdown();
        ImGui_ImplOSX_Shutdown();
        ImGui::DestroyContext();
    }
    
    _gizmo.reset();
    _scene.reset();
    _renderer.reset();
}

- (void)initializeRenderer {
    NSRect bounds = self.bounds;
    CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    uint32_t width = (uint32_t)(bounds.size.width * scale);
    uint32_t height = (uint32_t)(bounds.size.height * scale);
    
    if (width == 0 || height == 0) {
        width = 1280;
        height = 720;
    }
    
    _renderer = std::make_unique<luma::UnifiedRenderer>();
    
    if (!_renderer->initialize((__bridge void*)self, width, height)) {
        NSLog(@"[luma] Failed to initialize renderer");
        return;
    }
    
    // Enable shader hot-reload
    _renderer->setShaderHotReload(true);
    
    // Create command queue for ImGui
    _commandQueue = [self.metalLayer.device newCommandQueue];
    
    // Initialize ImGui
    [self initializeImGui];
    
    // Initialize scene graph and gizmo
    _scene = std::make_unique<luma::SceneGraph>();
    _gizmo = std::make_unique<luma::TransformGizmo>();
    
    // Setup editor callbacks
    [self setupEditorCallbacks];
    
    // Setup character creator callbacks
    [self setupCharacterCreatorCallbacks];
    
    // Log startup
    _editorState.consoleLogs.push_back("[INFO] LUMA Studio started");
    _editorState.consoleLogs.push_back("[INFO] Press F1 for keyboard shortcuts");
    
    // Create default cube as first entity
    luma::Mesh cube = luma::create_cube();
    luma::RHILoadedModel cubeModel;
    cubeModel.meshes.push_back(_renderer->uploadMesh(cube));
    cubeModel.center[0] = cubeModel.center[1] = cubeModel.center[2] = 0.0f;
    cubeModel.radius = 1.0f;
    cubeModel.name = "Default Cube";
    cubeModel.debugName = "primitives/cube";
    cubeModel.totalVerts = cube.vertices.size();
    cubeModel.totalTris = cube.indices.size() / 3;
    
    luma::Entity* cubeEntity = _scene->createEntityWithModel("Cube", cubeModel);
    _scene->setSelectedEntity(cubeEntity);
    
    NSLog(@"[luma] LUMA Studio ready");
}

- (void)initializeImGui {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Apply editor theme
    luma::ui::applyEditorTheme();
    
    ImGui_ImplOSX_Init(self);
    ImGui_ImplMetal_Init(self.metalLayer.device);
    
    _imguiInitialized = YES;
    NSLog(@"[luma] ImGui initialized");
}

- (void)setupEditorCallbacks {
    // Model load callback
    _editorState.onModelLoad = [self](const std::string& path) {
        [self loadModelAtPath:[NSString stringWithUTF8String:path.c_str()]];
    };
    
    // Scene save callback  
    _editorState.onSceneSave = [self](const std::string& path) {
        [self saveSceneToPath:path.empty() ? nil : [NSString stringWithUTF8String:path.c_str()]];
    };
    
    // Scene load callback
    _editorState.onSceneLoad = [self](const std::string& path) {
        [self loadSceneFromPath:path.empty() ? nil : [NSString stringWithUTF8String:path.c_str()]];
    };
    
    // HDR environment load callback
    _editorState.onHDRLoad = [self](const std::string& path) {
        [self loadHDREnvironment:path.empty() ? nil : [NSString stringWithUTF8String:path.c_str()]];
    };
    
    // Demo generate callback
    _editorState.onDemoGenerate = [self](const std::string& demoId) {
        auto& demoMode = luma::DemoMode::get();
        if (demoMode.generateDemo(demoId, *_scene)) {
            _editorState.consoleLogs.push_back("[INFO] Generated demo: " + demoId);
        }
    };
    
    // Asset browser: double-click callback
    _editorState.onAssetDoubleClick = [self](const std::string& path, luma::BrowserAssetType type) {
        switch (type) {
            case luma::BrowserAssetType::Model:
                // Load model into scene
                [self loadModelAtPath:[NSString stringWithUTF8String:path.c_str()]];
                _editorState.consoleLogs.push_back("[INFO] Loaded model: " + path);
                break;
                
            case luma::BrowserAssetType::Scene:
                // Load scene
                [self loadSceneFromPath:[NSString stringWithUTF8String:path.c_str()]];
                break;
                
            case luma::BrowserAssetType::Texture:
                // Preview texture (TODO: texture preview window)
                _editorState.consoleLogs.push_back("[INFO] Texture: " + path);
                break;
                
            case luma::BrowserAssetType::Script:
                // Open script in editor (TODO: script editor)
                _editorState.consoleLogs.push_back("[INFO] Script: " + path);
                break;
                
            default:
                _editorState.consoleLogs.push_back("[INFO] Selected: " + path);
                break;
        }
    };
    
    // Asset browser: drag & drop to scene callback
    _editorState.onAssetDragDropToScene = [self](const std::string& path, luma::BrowserAssetType type) {
        switch (type) {
            case luma::BrowserAssetType::Model:
                // Load model and create entity at origin
                [self loadModelAtPath:[NSString stringWithUTF8String:path.c_str()]];
                _editorState.consoleLogs.push_back("[INFO] Dropped model: " + path);
                break;
                
            case luma::BrowserAssetType::Scene:
                // Could add scene as prefab (TODO)
                _editorState.consoleLogs.push_back("[INFO] Scene drop not yet supported");
                break;
                
            case luma::BrowserAssetType::Texture:
                // Apply texture to selected entity's material (as albedo by default)
                if (auto* selected = _scene->getSelectedEntity()) {
                    if (selected->material) {
                        // Use the albedo slot (index 0)
                        selected->material->texturePaths[static_cast<size_t>(luma::TextureSlot::Albedo)] = path;
                        _editorState.consoleLogs.push_back("[INFO] Applied texture to " + selected->name);
                    }
                }
                break;
                
            default:
                break;
        }
    };
    
    // Setup AssetManager with model loader
    auto& assetMgr = luma::getAssetManager();
    assetMgr.setModelLoader([](const std::string& path) -> std::shared_ptr<void> {
        auto model = luma::load_model(path);
        if (model) {
            return std::make_shared<luma::Model>(std::move(*model));
        }
        return nullptr;
    });
    
    // Configure cache settings
    assetMgr.setMaxCacheSize(512 * 1024 * 1024);  // 512 MB
    assetMgr.setUnusedTimeout(std::chrono::seconds(300));  // 5 minutes
    
    // Initialize animation systems
    _ikManager = std::make_unique<luma::IKManager>();
    _activeBlendTree1D = nullptr;
    _activeBlendTree2D = nullptr;
    _activeSkeleton = nullptr;
    
    // Initialize advanced settings with sensible defaults
    _advancedPostProcess.ssao = luma::SSAOPresets::medium();
    _advancedPostProcess.ssr = luma::SSRPresets::medium();
    _advancedPostProcess.fog = luma::VolumetricPresets::lightFog();
    _advancedShadows.csm.numCascades = 3;
    _advancedShadows.csm.maxShadowDistance = 100.0f;
    _lodState.qualityPreset = luma::ui::LODQualityPreset::Medium;
    
    // Setup scene operation callbacks
    auto& sceneCallbacks = luma::ui::getSceneCallbacks();
    sceneCallbacks.onNewScene = [self]() {
        _scene->clear();
        _editorState.consoleLogs.push_back("[INFO] New scene created");
    };
    sceneCallbacks.onDeleteSelected = [self]() {
        if (auto* selected = _scene->getSelectedEntity()) {
            auto cmd = std::make_unique<luma::DeleteEntityCommand>(_scene.get(), selected);
            _scene->clearSelection();
            luma::getCommandHistory().execute(std::move(cmd));
        }
    };
    sceneCallbacks.onDuplicateSelected = [self]() {
        if (auto* selected = _scene->getSelectedEntity()) {
            auto cmd = std::make_unique<luma::DuplicateEntityCommand>(_scene.get(), selected);
            luma::getCommandHistory().execute(std::move(cmd));
        }
    };
}

- (void)checkAIModelsReady {
    NSFileManager* fm = [NSFileManager defaultManager];
    NSString* modelsDir = @"models/ai";
    
    // Check for required models
    BOOL faceDetectorExists = [fm fileExistsAtPath:[modelsDir stringByAppendingPathComponent:@"face_detector.onnx"]];
    BOOL faceMeshExists = [fm fileExistsAtPath:[modelsDir stringByAppendingPathComponent:@"face_mesh.onnx"]];
    
    _characterCreatorState.aiModelsReady = faceDetectorExists && faceMeshExists;
    
    if (_characterCreatorState.aiModelsReady) {
        _editorState.consoleLogs.push_back("[INFO] All required AI models ready");
    }
}

- (void)initializeCharacter {
    // Initialize model library
    auto& library = luma::BaseHumanModelLibrary::getInstance();
    library.initializeDefaults();
    
    // Create character
    _character = luma::CharacterFactory::createBlank("Character");
    
    // Load procedural model
    const luma::BaseHumanModel* model = library.getModel("procedural_human");
    if (model) {
        _character->setBaseMesh(model->vertices, model->indices);
        
        // Copy BlendShape data
        auto& charBlendShapes = _character->getBlendShapeMesh();
        for (size_t i = 0; i < model->blendShapes.getTargetCount(); i++) {
            const luma::BlendShapeTarget* target = model->blendShapes.getTarget(static_cast<int>(i));
            if (target) {
                charBlendShapes.addTarget(*target);
            }
        }
        for (size_t i = 0; i < model->blendShapes.getChannelCount(); i++) {
            const luma::BlendShapeChannel* channel = model->blendShapes.getChannel(static_cast<int>(i));
            if (channel) {
                charBlendShapes.addChannel(*channel);
            }
        }
        
        // Initialize character renderer
        _characterRenderer = std::make_unique<luma::CharacterRenderer>();
        _characterRenderer->initialize(_renderer.get());
        _characterRenderer->setupCharacter(_character.get());
        
        // Update stats
        _characterCreatorState.vertexCount = static_cast<uint32_t>(_character->getBaseVertices().size());
        _characterCreatorState.triangleCount = static_cast<uint32_t>(_character->getIndices().size() / 3);
        _characterCreatorState.blendShapeCount = static_cast<uint32_t>(charBlendShapes.getChannelCount());
        _characterCreatorState.boneCount = _character->getSkeleton().getBoneCount();
        
        // Update BlendShape list for UI
        _characterCreatorState.blendShapeWeights.clear();
        for (size_t i = 0; i < charBlendShapes.getChannelCount(); i++) {
            const luma::BlendShapeChannel* ch = charBlendShapes.getChannel(static_cast<int>(i));
            if (ch) {
                _characterCreatorState.blendShapeWeights.push_back({ch->name, ch->weight});
            }
        }
        
        _editorState.consoleLogs.push_back("[INFO] Character initialized with " + 
            std::to_string(_characterCreatorState.vertexCount) + " vertices");
        
        // Initialize clothing system
        luma::ClothingLibrary::getInstance().initializeDefaults();
        _clothingManager = std::make_unique<luma::ClothingManager>();
        _clothingNeedsUpdate = true;
        
        _editorState.consoleLogs.push_back("[INFO] Clothing library initialized");
        
        // Initialize animation system
        luma::getPoseLibrary().initializeDefaults();
        luma::getAnimationLibrary().initializeDefaults();
        _animRetargeter = std::make_unique<luma::CharacterAnimationRetargeter>();
        _animRetargeter->setup(_character->getSkeleton());
        _isAnimationPlaying = false;
        _animationTime = 0.0f;
        
        _editorState.consoleLogs.push_back("[INFO] Animation library initialized");
        
        // Initialize texture system and generate initial textures
        luma::SkinTextureParams skinParams = luma::SkinTextureParams::caucasian();
        luma::EyeTextureParams eyeParams = luma::EyeTextureParams::brown();
        luma::LipTextureParams lipParams = luma::LipTextureParams::natural();
        _characterTextures = luma::getTextureManager().generateTextureSet(skinParams, eyeParams, lipParams, 1024);
        _texturesNeedUpdate = true;
        _editorState.consoleLogs.push_back("[INFO] Initial textures generated (1024x1024)");
        
        // Initialize hair system
        luma::getHairLibrary().initializeDefaults();
        _hairManager = std::make_unique<luma::HairManager>();
        _hairManager->setStyle("bald");  // Start with no hair
        _hairNeedsUpdate = false;
        
        // Populate available hair styles in UI
        _characterCreatorState.availableHairStyles = luma::getHairLibrary().getAllStyleIds();
        _editorState.consoleLogs.push_back("[INFO] Hair system initialized with " + 
            std::to_string(_characterCreatorState.availableHairStyles.size()) + " styles");
    } else {
        _editorState.consoleLogs.push_back("[ERROR] Failed to load procedural human model");
    }
}

- (void)updateAndRenderCharacter:(float)deltaTime {
    if (!_character || !_characterRenderer) {
        // Initialize character on first use
        [self initializeCharacter];
        if (!_character) return;
    }
    
    // Auto rotate character
    if (_characterCreatorState.autoRotate) {
        _characterCreatorState.rotationY += deltaTime * 0.5f;
    }
    
    // Sync UI parameters to character
    if (_characterNeedsUpdate) {
        // Update body parameters
        auto& body = _character->getBody();
        body.setGender(static_cast<luma::Gender>(_characterCreatorState.gender));
        
        auto& m = body.getParams().measurements;
        m.height = _characterCreatorState.height;
        m.weight = _characterCreatorState.weight;
        m.muscularity = _characterCreatorState.muscularity;
        m.bodyFat = _characterCreatorState.bodyFat;
        m.shoulderWidth = _characterCreatorState.shoulderWidth;
        m.chestSize = _characterCreatorState.chestSize;
        m.waistSize = _characterCreatorState.waistSize;
        m.hipWidth = _characterCreatorState.hipWidth;
        m.armLength = _characterCreatorState.armLength;
        m.armThickness = _characterCreatorState.armThickness;
        m.legLength = _characterCreatorState.legLength;
        m.thighThickness = _characterCreatorState.thighThickness;
        m.bustSize = _characterCreatorState.bustSize;
        
        body.getParams().skinColor = luma::Vec3(
            _characterCreatorState.skinColor[0],
            _characterCreatorState.skinColor[1],
            _characterCreatorState.skinColor[2]
        );
        
        // Update face parameters
        auto& face = _character->getFace();
        auto& shape = face.getShapeParams();
        shape.faceWidth = _characterCreatorState.faceWidth;
        shape.faceLength = _characterCreatorState.faceLength;
        shape.faceRoundness = _characterCreatorState.faceRoundness;
        shape.eyeSize = _characterCreatorState.eyeSize;
        shape.eyeSpacing = _characterCreatorState.eyeSpacing;
        shape.eyeHeight = _characterCreatorState.eyeHeight;
        shape.eyeAngle = _characterCreatorState.eyeAngle;
        shape.noseLength = _characterCreatorState.noseLength;
        shape.noseWidth = _characterCreatorState.noseWidth;
        shape.noseHeight = _characterCreatorState.noseHeight;
        shape.noseBridge = _characterCreatorState.noseBridge;
        shape.mouthWidth = _characterCreatorState.mouthWidth;
        shape.upperLipThickness = _characterCreatorState.upperLipThickness;
        shape.lowerLipThickness = _characterCreatorState.lowerLipThickness;
        shape.jawWidth = _characterCreatorState.jawWidth;
        shape.jawLine = _characterCreatorState.jawLine;
        shape.chinLength = _characterCreatorState.chinLength;
        shape.chinWidth = _characterCreatorState.chinWidth;
        
        face.getTextureParams().eyeColor = luma::Vec3(
            _characterCreatorState.eyeColor[0],
            _characterCreatorState.eyeColor[1],
            _characterCreatorState.eyeColor[2]
        );
        
        // Apply BlendShape weights
        body.updateBlendShapeWeights();
        face.applyParameters();
        
        _characterNeedsUpdate = false;
    }
    
    // Update animation if playing
    if (_isAnimationPlaying && _animRetargeter && !_currentAnimation.empty()) {
        const luma::RetargetableClip* clip = luma::getAnimationLibrary().getClip(_currentAnimation);
        if (clip) {
            _animationTime += deltaTime * _characterCreatorState.animationSpeed;
            _characterCreatorState.animationTime = _animationTime;
            
            // Apply animation
            _animRetargeter->applyClip(*clip, _animationTime);
            _characterNeedsUpdate = true;
            
            // Check if non-looping animation finished
            if (!clip->looping && _animationTime >= clip->duration) {
                _isAnimationPlaying = false;
                _characterCreatorState.animationPlaying = false;
            }
        }
    }
    
    // Update character (animation, etc.)
    _character->update(deltaTime);
    
    // Update BlendShape mesh
    _characterRenderer->updateBlendShapes();
    
    // Update GPU mesh if needed
    if (_characterRenderer->needsGPUUpdate() || _texturesNeedUpdate) {
        luma::Mesh mesh = _characterRenderer->getCurrentMesh();
        
        // Apply skin color as fallback
        auto& bodyParams = _character->getBody().getParams();
        mesh.baseColor[0] = bodyParams.skinColor.x;
        mesh.baseColor[1] = bodyParams.skinColor.y;
        mesh.baseColor[2] = bodyParams.skinColor.z;
        mesh.metallic = 0.0f;
        mesh.roughness = 0.6f;
        
        // Apply generated textures if available
        if (_characterTextures.isGenerated) {
            // Skin diffuse texture
            if (!_characterTextures.skinDiffuse.pixels.empty()) {
                mesh.diffuseTexture = _characterTextures.skinDiffuse;
                mesh.hasDiffuseTexture = true;
            }
            
            // Skin normal map
            if (!_characterTextures.skinNormal.pixels.empty()) {
                mesh.normalTexture = _characterTextures.skinNormal;
                mesh.hasNormalTexture = true;
            }
            
            // Use roughness as specular (for now)
            if (!_characterTextures.skinRoughness.pixels.empty()) {
                mesh.specularTexture = _characterTextures.skinRoughness;
                mesh.hasSpecularTexture = true;
            }
            
            // Set material properties from texture params
            mesh.roughness = _characterTextures.skinParams.roughness;
        }
        
        _characterMesh = _renderer->uploadMesh(mesh);
        _characterRenderer->markGPUUpdated();
        _texturesNeedUpdate = false;
    }
    
    // Create world matrix with rotation (shared for character and clothing)
    float angle = _characterCreatorState.rotationY;
    float c = cosf(angle);
    float s = sinf(angle);
    
    float worldMatrix[16] = {
        c,    0.0f, -s,   0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s,    0.0f, c,    0.0f,
        3.0f, 0.0f, 0.0f, 1.0f  // Offset to the right of scene center
    };
    
    // Render character body
    if (_characterMesh.indexCount > 0) {
        luma::RHILoadedModel charModel;
        charModel.meshes.push_back(_characterMesh);
        charModel.radius = 1.0f;
        charModel.center[0] = 0.0f;
        charModel.center[1] = 0.9f;
        charModel.center[2] = 0.0f;
        
        _renderer->renderModel(charModel, worldMatrix);
    }
    
    // Update and render hair
    if (_hairManager && _hairManager->hasHair()) {
        if (_hairNeedsUpdate) {
            const luma::Mesh& hairMesh = _hairManager->getHairMesh();
            _hairMesh = _renderer->uploadMesh(hairMesh);
            _hairNeedsUpdate = false;
        }
        
        if (_hairMesh.indexCount > 0) {
            // Apply hair offset
            luma::Vec3 offset = _hairManager->getOffset();
            float hairWorldMatrix[16] = {
                c,    0.0f, -s,   0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                s,    0.0f, c,    0.0f,
                3.0f + offset.x, offset.y, offset.z, 1.0f
            };
            
            luma::RHILoadedModel hairModel;
            hairModel.meshes.push_back(_hairMesh);
            hairModel.radius = 0.2f;
            hairModel.center[0] = 0.0f;
            hairModel.center[1] = 1.55f;
            hairModel.center[2] = 0.0f;
            
            _renderer->renderModel(hairModel, hairWorldMatrix);
        }
    }
    
    // Update and render clothing
    if (_clothingManager) {
        // Adapt clothing to current body shape if needed
        if (_clothingNeedsUpdate) {
            _clothingManager->adaptToBody(_character->getBody().getParams().measurements);
            
            // Upload clothing meshes to GPU
            _clothingMeshes.clear();
            auto meshes = _clothingManager->getClothingMeshes();
            for (const auto& mesh : meshes) {
                _clothingMeshes.push_back(_renderer->uploadMesh(mesh));
            }
            
            _clothingNeedsUpdate = false;
        }
        
        // Render each clothing item
        for (const auto& clothingMesh : _clothingMeshes) {
            if (clothingMesh.indexCount > 0) {
                luma::RHILoadedModel clothingModel;
                clothingModel.meshes.push_back(clothingMesh);
                clothingModel.radius = 1.0f;
                clothingModel.center[0] = 0.0f;
                clothingModel.center[1] = 0.9f;
                clothingModel.center[2] = 0.0f;
                
                _renderer->renderModel(clothingModel, worldMatrix);
            }
        }
    }
}

- (void)setupCharacterCreatorCallbacks {
    // Initialize character creator state
    _characterCreatorState.initialized = false;
    _characterNeedsUpdate = false;
    
    // Initialize callback
    _characterCreatorState.onInitialize = [self]() {
        [self initializeCharacter];
    };
    
    // Randomize callback
    _characterCreatorState.onRandomize = [self]() {
        // Randomize body parameters
        _characterCreatorState.height = (float)rand() / RAND_MAX;
        _characterCreatorState.weight = (float)rand() / RAND_MAX;
        _characterCreatorState.muscularity = (float)rand() / RAND_MAX;
        _characterCreatorState.bodyFat = (float)rand() / RAND_MAX;
        _characterCreatorState.shoulderWidth = (float)rand() / RAND_MAX;
        _characterCreatorState.chestSize = (float)rand() / RAND_MAX;
        _characterCreatorState.waistSize = (float)rand() / RAND_MAX;
        _characterCreatorState.hipWidth = (float)rand() / RAND_MAX;
        
        // Randomize face parameters
        _characterCreatorState.faceWidth = (float)rand() / RAND_MAX;
        _characterCreatorState.faceLength = (float)rand() / RAND_MAX;
        _characterCreatorState.eyeSize = (float)rand() / RAND_MAX;
        _characterCreatorState.noseLength = (float)rand() / RAND_MAX;
        _characterCreatorState.mouthWidth = (float)rand() / RAND_MAX;
        _characterCreatorState.jawWidth = (float)rand() / RAND_MAX;
        
        _characterNeedsUpdate = true;
        _editorState.consoleLogs.push_back("[INFO] Character randomized");
    };
    
    // Preset selection callback
    _characterCreatorState.onPresetSelect = [self](int presetIdx) {
        // Apply preset values based on index
        switch (presetIdx) {
            case 0: // Male Slim
                _characterCreatorState.height = 0.5f;
                _characterCreatorState.weight = 0.3f;
                _characterCreatorState.muscularity = 0.2f;
                _characterCreatorState.bodyFat = 0.15f;
                break;
            case 1: // Male Average
                _characterCreatorState.height = 0.5f;
                _characterCreatorState.weight = 0.5f;
                _characterCreatorState.muscularity = 0.4f;
                _characterCreatorState.bodyFat = 0.3f;
                break;
            case 2: // Male Muscular
                _characterCreatorState.height = 0.55f;
                _characterCreatorState.weight = 0.65f;
                _characterCreatorState.muscularity = 0.85f;
                _characterCreatorState.bodyFat = 0.15f;
                break;
            case 3: // Male Heavy
                _characterCreatorState.height = 0.5f;
                _characterCreatorState.weight = 0.8f;
                _characterCreatorState.muscularity = 0.3f;
                _characterCreatorState.bodyFat = 0.7f;
                break;
            case 4: // Male Elderly
                _characterCreatorState.height = 0.45f;
                _characterCreatorState.weight = 0.45f;
                _characterCreatorState.muscularity = 0.2f;
                _characterCreatorState.bodyFat = 0.35f;
                break;
            case 5: // Female Slim
                _characterCreatorState.height = 0.45f;
                _characterCreatorState.weight = 0.25f;
                _characterCreatorState.muscularity = 0.15f;
                _characterCreatorState.bodyFat = 0.2f;
                break;
            case 6: // Female Average
                _characterCreatorState.height = 0.45f;
                _characterCreatorState.weight = 0.45f;
                _characterCreatorState.muscularity = 0.2f;
                _characterCreatorState.bodyFat = 0.35f;
                break;
            case 7: // Female Curvy
                _characterCreatorState.height = 0.45f;
                _characterCreatorState.weight = 0.55f;
                _characterCreatorState.muscularity = 0.15f;
                _characterCreatorState.bodyFat = 0.45f;
                _characterCreatorState.bustSize = 0.7f;
                _characterCreatorState.hipWidth = 0.65f;
                break;
            case 8: // Female Athletic
                _characterCreatorState.height = 0.48f;
                _characterCreatorState.weight = 0.4f;
                _characterCreatorState.muscularity = 0.5f;
                _characterCreatorState.bodyFat = 0.2f;
                break;
            default:
                break;
        }
        _characterNeedsUpdate = true;
        _editorState.consoleLogs.push_back("[INFO] Applied body preset");
    };
    
    // Parameter changed callback
    _characterCreatorState.onParameterChanged = [self]() {
        _characterNeedsUpdate = true;
    };
    
    // BlendShape changed callback
    _characterCreatorState.onBlendShapeChanged = [self](const std::string& name, float weight) {
        _characterNeedsUpdate = true;
    };
    
    // Photo import callback
    _characterCreatorState.onPhotoImport = [self]() {
        // Check AI models first
        [self checkAIModelsReady];
        
        if (!_characterCreatorState.aiModelsReady) {
            _characterCreatorState.aiModelStatus = "Please import required AI models first";
            _characterCreatorState.showAIModelSetup = true;
            return;
        }
        
        // Open file dialog for image
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.allowedContentTypes = @[
            [UTType typeWithMIMEType:@"image/jpeg"],
            [UTType typeWithMIMEType:@"image/png"]
        ];
        panel.allowsMultipleSelection = NO;
        panel.canChooseDirectories = NO;
        panel.message = @"Select Photo for Face Import";
        
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
            if (result == NSModalResponseOK && panel.URLs.count > 0) {
                NSString* selectedPath = panel.URLs[0].path;
                std::string path = [selectedPath UTF8String];
                
                _editorState.consoleLogs.push_back("[INFO] Processing photo: " + path);
                _characterCreatorState.aiModelStatus = "Processing...";
                
                // Load image
                NSImage* image = [[NSImage alloc] initWithContentsOfFile:selectedPath];
                if (!image) {
                    _editorState.consoleLogs.push_back("[ERROR] Failed to load image");
                    _characterCreatorState.aiModelStatus = "Failed to load image";
                    return;
                }
                
                // Get image data
                NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc] initWithData:[image TIFFRepresentation]];
                int width = (int)[bitmap pixelsWide];
                int height = (int)[bitmap pixelsHigh];
                int channels = (int)[bitmap samplesPerPixel];
                
                _editorState.consoleLogs.push_back("[INFO] Image size: " + std::to_string(width) + "x" + 
                    std::to_string(height) + " (" + std::to_string(channels) + " channels)");
                
                // Process with AI pipeline
                if (_character) {
                    luma::PhotoToFacePipeline::Config config;
                    config.faceDetectorModelPath = "models/ai/face_detector.onnx";
                    config.faceMeshModelPath = "models/ai/face_mesh.onnx";
                    config.face3DMMModelPath = "models/ai/3dmm.onnx";
                    config.extractTexture = true;
                    config.use3DMM = [[NSFileManager defaultManager] 
                        fileExistsAtPath:@"models/ai/3dmm.onnx"];
                    
                    luma::PhotoToFacePipeline pipeline;
                    pipeline.initialize(config);
                    
                    luma::PhotoFaceResult faceResult;
                    const uint8_t* imageData = [bitmap bitmapData];
                    
                    if (pipeline.process(imageData, width, height, channels, faceResult)) {
                        // Apply result to character face
                        pipeline.applyToCharacterFace(faceResult, _character->getFace());
                        
                        // Sync UI parameters from character
                        auto& shape = _character->getFace().getShapeParams();
                        _characterCreatorState.faceWidth = shape.faceWidth;
                        _characterCreatorState.faceLength = shape.faceLength;
                        _characterCreatorState.eyeSize = shape.eyeSize;
                        _characterCreatorState.noseLength = shape.noseLength;
                        _characterCreatorState.mouthWidth = shape.mouthWidth;
                        _characterCreatorState.jawWidth = shape.jawWidth;
                        
                        _characterNeedsUpdate = true;
                        _characterCreatorState.aiModelStatus = "Face imported successfully!";
                        _editorState.consoleLogs.push_back("[INFO] Face imported from photo");
                    } else {
                        _characterCreatorState.aiModelStatus = "Failed: " + faceResult.errorMessage;
                        _editorState.consoleLogs.push_back("[ERROR] " + faceResult.errorMessage);
                    }
                }
            }
        }];
    };
    
    // AI Model Import callback
    _characterCreatorState.onImportAIModel = [self](const std::string& modelId, const std::string& modelName) {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.allowedContentTypes = @[[UTType typeWithFilenameExtension:@"onnx"]];
        panel.allowsMultipleSelection = NO;
        panel.canChooseDirectories = NO;
        panel.message = [NSString stringWithFormat:@"Select %s Model (ONNX format)", modelName.c_str()];
        
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
            if (result == NSModalResponseOK && panel.URLs.count > 0) {
                NSString* selectedPath = panel.URLs[0].path;
                
                // Copy to models directory
                NSFileManager* fm = [NSFileManager defaultManager];
                NSString* modelsDir = @"models/ai";
                NSString* destPath = [modelsDir stringByAppendingPathComponent:
                    [NSString stringWithFormat:@"%s.onnx", modelId.c_str()]];
                
                // Create directory if needed
                [fm createDirectoryAtPath:modelsDir withIntermediateDirectories:YES attributes:nil error:nil];
                
                // Copy file
                NSError* error = nil;
                if ([fm fileExistsAtPath:destPath]) {
                    [fm removeItemAtPath:destPath error:nil];
                }
                
                if ([fm copyItemAtPath:selectedPath toPath:destPath error:&error]) {
                    _editorState.consoleLogs.push_back("[INFO] Imported AI model: " + modelId);
                    _characterCreatorState.aiModelStatus = "Model imported: " + modelId;
                    
                    // Check if all required models are now present
                    [self checkAIModelsReady];
                } else {
                    _editorState.consoleLogs.push_back("[ERROR] Failed to import model: " + 
                        std::string([[error localizedDescription] UTF8String]));
                }
            }
        }];
    };
    
    // Export callback
    _characterCreatorState.onExport = [self](const std::string& name, int format, bool skeleton, bool blendShapes, bool textures) {
        if (!_character) {
            _editorState.consoleLogs.push_back("[ERROR] No character to export");
            return;
        }
        
        NSSavePanel* panel = [NSSavePanel savePanel];
        
        luma::CharacterExportFormat exportFormat;
        NSString* ext;
        
        switch (format) {
            case 0:  // GLB
                exportFormat = luma::CharacterExportFormat::GLTF;
                ext = @"glb";
                break;
            case 1:  // glTF
                exportFormat = luma::CharacterExportFormat::GLTF;
                ext = @"gltf";
                break;
            case 2:  // FBX
                exportFormat = luma::CharacterExportFormat::FBX;
                ext = @"fbx";
                break;
            case 3:  // OBJ
                exportFormat = luma::CharacterExportFormat::OBJ;
                ext = @"obj";
                break;
            case 4:  // VRM
                exportFormat = luma::CharacterExportFormat::VRM;
                ext = @"vrm";
                break;
            default:
                exportFormat = luma::CharacterExportFormat::GLTF;
                ext = @"glb";
                break;
        }
        
        panel.allowedContentTypes = @[[UTType typeWithFilenameExtension:ext]];
        panel.nameFieldStringValue = [NSString stringWithFormat:@"%s.%@", name.c_str(), ext];
        panel.message = @"Export Character";
        
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
            if (result == NSModalResponseOK) {
                NSString* path = panel.URL.path;
                std::string outputPath = [path UTF8String];
                
                // Set character name
                _character->setName(name);
                
                // Export options
                luma::CharacterExportOptions options;
                options.includeSkeleton = _characterCreatorState.exportSkeleton;
                options.includeBlendShapes = _characterCreatorState.exportBlendShapes;
                options.scale = 1.0f;
                
                // Perform export
                _editorState.consoleLogs.push_back("[INFO] Exporting to: " + outputPath);
                
                if (luma::CharacterExporter::exportCharacter(*_character, outputPath, exportFormat, options)) {
                    _editorState.consoleLogs.push_back("[INFO] Export successful!");
                    
                    // Show success notification
                    NSAlert* alert = [[NSAlert alloc] init];
                    alert.messageText = @"Export Successful";
                    alert.informativeText = [NSString stringWithFormat:@"Character exported to:\n%@", path];
                    alert.alertStyle = NSAlertStyleInformational;
                    [alert addButtonWithTitle:@"OK"];
                    [alert addButtonWithTitle:@"Show in Finder"];
                    
                    NSModalResponse response = [alert runModal];
                    if (response == NSAlertSecondButtonReturn) {
                        [[NSWorkspace sharedWorkspace] selectFile:path 
                            inFileViewerRootedAtPath:@""];
                    }
                } else {
                    _editorState.consoleLogs.push_back("[ERROR] Export failed");
                    
                    if (exportFormat == luma::CharacterExportFormat::FBX) {
                        _editorState.consoleLogs.push_back("[INFO] FBX export requires Autodesk FBX SDK. Use glTF or OBJ instead.");
                    }
                }
            }
        }];
    };
    
    // Clothing callbacks
    _characterCreatorState.onEquipClothing = [self](const std::string& assetId) {
        if (_clothingManager) {
            _clothingManager->equip(assetId);
            _clothingNeedsUpdate = true;
            [self updateEquippedClothingUI];
            _editorState.consoleLogs.push_back("[INFO] Equipped: " + assetId);
        }
    };
    
    _characterCreatorState.onUnequipClothing = [self](const std::string& assetId) {
        if (_clothingManager) {
            _clothingManager->unequip(assetId);
            _clothingNeedsUpdate = true;
            [self updateEquippedClothingUI];
            _editorState.consoleLogs.push_back("[INFO] Unequipped: " + assetId);
        }
    };
    
    _characterCreatorState.onClothingColorChange = [self](const std::string& assetId, float r, float g, float b) {
        if (_clothingManager) {
            // Find which slot this item is in
            auto equipped = _clothingManager->getAllEquipped();
            for (const auto& [slot, item] : equipped) {
                if (item->assetId == assetId) {
                    _clothingManager->setColor(slot, luma::Vec3(r, g, b));
                    _clothingNeedsUpdate = true;
                    break;
                }
            }
        }
    };
    
    // Setup animation callbacks
    [self setupAnimationCallbacks];
    
    // Setup style callbacks
    [self setupStyleCallbacks];
    
    // Setup texture callbacks
    [self setupTextureCallbacks];
    
    // Setup hair callbacks
    [self setupHairCallbacks];
}

- (void)updateEquippedClothingUI {
    _characterCreatorState.equippedClothing.clear();
    if (_clothingManager) {
        auto equipped = _clothingManager->getAllEquipped();
        for (const auto& [slot, item] : equipped) {
            std::string slotName;
            switch (slot) {
                case luma::ClothingSlot::Shirt: slotName = "Shirt"; break;
                case luma::ClothingSlot::Pants: slotName = "Pants"; break;
                case luma::ClothingSlot::Skirt: slotName = "Skirt"; break;
                case luma::ClothingSlot::Shoes: slotName = "Shoes"; break;
                default: slotName = "Other"; break;
            }
            _characterCreatorState.equippedClothing.push_back({slotName, item->assetId});
        }
    }
}

- (void)setupAnimationCallbacks {
    // Apply pose callback
    _characterCreatorState.onApplyPose = [self](const std::string& poseId) {
        if (_animRetargeter && _character) {
            const luma::AnimationPose* pose = luma::getPoseLibrary().getPose(poseId);
            if (pose) {
                _animRetargeter->applyPose(*pose);
                _characterNeedsUpdate = true;
                _editorState.consoleLogs.push_back("[INFO] Applied pose: " + poseId);
            }
        }
    };
    
    // Play animation callback
    _characterCreatorState.onPlayAnimation = [self](const std::string& animId) {
        _currentAnimation = animId;
        _animationTime = 0.0f;
        _isAnimationPlaying = true;
        _characterCreatorState.animationPlaying = true;
        _editorState.consoleLogs.push_back("[INFO] Playing animation: " + animId);
    };
    
    // Stop animation callback
    _characterCreatorState.onStopAnimation = [self]() {
        _isAnimationPlaying = false;
        _characterCreatorState.animationPlaying = false;
        _currentAnimation = "";
        
        // Reset to idle pose
        if (_animRetargeter) {
            const luma::AnimationPose* idlePose = luma::getPoseLibrary().getPose("idle");
            if (idlePose) {
                _animRetargeter->applyPose(*idlePose);
                _characterNeedsUpdate = true;
            }
        }
        _editorState.consoleLogs.push_back("[INFO] Animation stopped");
    };
}

- (void)setupStyleCallbacks {
    // Style change callback
    _characterCreatorState.onStyleChange = [self](int styleIndex) {
        luma::RenderingStyle style = static_cast<luma::RenderingStyle>(styleIndex);
        luma::getStylizedRenderer().setStyle(style);
        
        const char* styleName = luma::getStyleName(style);
        _editorState.consoleLogs.push_back(std::string("[INFO] Rendering style: ") + styleName);
        
        // Note: Actual shader changes would require renderer integration
        // For now, we update the manager state which could be used for CPU-based preview
    };
    
    // Style settings change callback
    _characterCreatorState.onStyleSettingsChange = [self]() {
        auto& settings = luma::getStylizedRenderer().getSettings();
        
        // Update settings from UI state
        settings.outline.enabled = _characterCreatorState.outlineEnabled;
        settings.outline.thickness = _characterCreatorState.outlineThickness;
        settings.outline.color = luma::Vec3(
            _characterCreatorState.outlineColor[0],
            _characterCreatorState.outlineColor[1],
            _characterCreatorState.outlineColor[2]
        );
        settings.celShading.shadingBands = _characterCreatorState.celShadingBands;
        settings.celShading.enableRimLight = _characterCreatorState.rimLightEnabled;
        settings.celShading.rimIntensity = _characterCreatorState.rimLightIntensity;
        settings.colorVibrancy = _characterCreatorState.colorVibrancy;
    };
}

- (void)setupTextureCallbacks {
    // Texture update callback
    _characterCreatorState.onTextureUpdate = [self]() {
        // Build texture parameters from UI state
        luma::SkinTextureParams skinParams;
        skinParams.baseColor = luma::Vec3(
            _characterCreatorState.skinColor[0],
            _characterCreatorState.skinColor[1],
            _characterCreatorState.skinColor[2]
        );
        skinParams.saturation = _characterCreatorState.skinSaturation;
        skinParams.brightness = _characterCreatorState.skinBrightness;
        skinParams.roughness = _characterCreatorState.skinRoughness;
        skinParams.poreIntensity = _characterCreatorState.poreIntensity;
        skinParams.wrinkleIntensity = _characterCreatorState.wrinkleIntensity;
        skinParams.freckleIntensity = _characterCreatorState.freckleIntensity;
        skinParams.freckleColor = luma::Vec3(
            _characterCreatorState.freckleColor[0],
            _characterCreatorState.freckleColor[1],
            _characterCreatorState.freckleColor[2]
        );
        skinParams.sssIntensity = _characterCreatorState.sssIntensity;
        
        luma::EyeTextureParams eyeParams;
        eyeParams.irisColor = luma::Vec3(
            _characterCreatorState.eyeColor[0],
            _characterCreatorState.eyeColor[1],
            _characterCreatorState.eyeColor[2]
        );
        eyeParams.irisSize = _characterCreatorState.irisSize;
        eyeParams.pupilSize = _characterCreatorState.pupilSize;
        eyeParams.irisDetail = _characterCreatorState.irisDetail;
        eyeParams.scleraVeins = _characterCreatorState.scleraVeins;
        eyeParams.wetness = _characterCreatorState.eyeWetness;
        
        luma::LipTextureParams lipParams;
        lipParams.color = luma::Vec3(
            _characterCreatorState.lipColor[0],
            _characterCreatorState.lipColor[1],
            _characterCreatorState.lipColor[2]
        );
        lipParams.glossiness = _characterCreatorState.lipGlossiness;
        lipParams.chappedAmount = _characterCreatorState.lipChapped;
        
        // Determine resolution
        int resolution = 1024;
        switch (_characterCreatorState.textureResolution) {
            case 0: resolution = 512; break;
            case 1: resolution = 1024; break;
            case 2: resolution = 2048; break;
        }
        
        // Generate textures
        _characterTextures = luma::getTextureManager().generateTextureSet(skinParams, eyeParams, lipParams, resolution);
        _texturesNeedUpdate = true;
        
        _editorState.consoleLogs.push_back("[INFO] Generated textures at " + std::to_string(resolution) + "x" + std::to_string(resolution));
    };
    
    // Skin preset change
    _characterCreatorState.onSkinPresetChange = [self](int preset) {
        // Update skin color from preset
        switch (preset) {
            case 0:  // Caucasian
                _characterCreatorState.skinColor[0] = 0.9f;
                _characterCreatorState.skinColor[1] = 0.75f;
                _characterCreatorState.skinColor[2] = 0.65f;
                break;
            case 1:  // Asian
                _characterCreatorState.skinColor[0] = 0.95f;
                _characterCreatorState.skinColor[1] = 0.82f;
                _characterCreatorState.skinColor[2] = 0.7f;
                break;
            case 2:  // African
                _characterCreatorState.skinColor[0] = 0.45f;
                _characterCreatorState.skinColor[1] = 0.3f;
                _characterCreatorState.skinColor[2] = 0.2f;
                _characterCreatorState.sssIntensity = 0.2f;  // Less visible on darker skin
                break;
            case 3:  // Latino
                _characterCreatorState.skinColor[0] = 0.75f;
                _characterCreatorState.skinColor[1] = 0.55f;
                _characterCreatorState.skinColor[2] = 0.4f;
                break;
            case 4:  // Middle Eastern
                _characterCreatorState.skinColor[0] = 0.8f;
                _characterCreatorState.skinColor[1] = 0.6f;
                _characterCreatorState.skinColor[2] = 0.45f;
                break;
        }
        _editorState.consoleLogs.push_back("[INFO] Applied skin preset");
    };
    
    // Eye color preset change
    _characterCreatorState.onEyeColorPresetChange = [self](int preset) {
        switch (preset) {
            case 0:  // Brown
                _characterCreatorState.eyeColor[0] = 0.4f;
                _characterCreatorState.eyeColor[1] = 0.25f;
                _characterCreatorState.eyeColor[2] = 0.15f;
                break;
            case 1:  // Blue
                _characterCreatorState.eyeColor[0] = 0.3f;
                _characterCreatorState.eyeColor[1] = 0.5f;
                _characterCreatorState.eyeColor[2] = 0.8f;
                break;
            case 2:  // Green
                _characterCreatorState.eyeColor[0] = 0.35f;
                _characterCreatorState.eyeColor[1] = 0.55f;
                _characterCreatorState.eyeColor[2] = 0.35f;
                break;
            case 3:  // Hazel
                _characterCreatorState.eyeColor[0] = 0.5f;
                _characterCreatorState.eyeColor[1] = 0.4f;
                _characterCreatorState.eyeColor[2] = 0.25f;
                break;
            case 4:  // Gray
                _characterCreatorState.eyeColor[0] = 0.5f;
                _characterCreatorState.eyeColor[1] = 0.55f;
                _characterCreatorState.eyeColor[2] = 0.6f;
                break;
        }
        _editorState.consoleLogs.push_back("[INFO] Applied eye color preset");
    };
}

- (void)setupHairCallbacks {
    // Hair style change
    _characterCreatorState.onHairStyleChange = [self](const std::string& styleId) {
        if (_hairManager) {
            _hairManager->setStyle(styleId);
            _hairNeedsUpdate = true;
            _editorState.consoleLogs.push_back("[INFO] Hair style: " + styleId);
        }
    };
    
    // Hair color preset change
    _characterCreatorState.onHairColorPresetChange = [self](int preset) {
        if (_hairManager) {
            const char* presets[] = {
                "black", "dark_brown", "brown", "auburn", "red",
                "blonde", "platinum", "gray", "white",
                "blue", "pink", "purple", "green"
            };
            if (preset >= 0 && preset < 13) {
                _hairManager->setMaterialPreset(presets[preset]);
                _hairNeedsUpdate = true;
                _editorState.consoleLogs.push_back("[INFO] Hair color: " + std::string(presets[preset]));
            }
        }
    };
    
    // Custom hair color change
    _characterCreatorState.onHairColorChange = [self](float r, float g, float b) {
        if (_hairManager) {
            _hairManager->setColor(luma::Vec3(r, g, b));
            _hairNeedsUpdate = true;
        }
    };
}

- (void)loadHDREnvironment:(NSString*)path {
    if (path == nil) {
        // Open file dialog
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.allowedContentTypes = @[
            [UTType typeWithFilenameExtension:@"hdr"],
            [UTType typeWithFilenameExtension:@"exr"]
        ];
        panel.allowsMultipleSelection = NO;
        panel.canChooseDirectories = NO;
        panel.message = @"Select HDR Environment Map";
        
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
            if (result == NSModalResponseOK && panel.URLs.count > 0) {
                NSString* selectedPath = panel.URLs[0].path;
                [self doLoadHDR:selectedPath];
            }
        }];
    } else {
        [self doLoadHDR:path];
    }
}

- (void)doLoadHDR:(NSString*)path {
    _editorState.currentHDRPath = [path UTF8String];
    _editorState.consoleLogs.push_back("[INFO] Loaded HDR: " + std::string([path UTF8String]));
    // In a full implementation, would call renderer to load and process HDR
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    
    if (_renderer) {
        CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
        uint32_t width = (uint32_t)(newSize.width * scale);
        uint32_t height = (uint32_t)(newSize.height * scale);
        if (width > 0 && height > 0) {
            _renderer->resize(width, height);
            self.metalLayer.drawableSize = CGSizeMake(width, height);
        }
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    [self render];
}

- (float)getSceneRadius {
    float maxRadius = 1.0f;
    if (_scene) {
        for (auto& [id, entity] : _scene->getAllEntities()) {
            if (entity->hasModel) {
                maxRadius = std::max(maxRadius, entity->model.radius);
            }
        }
    }
    return maxRadius;
}

- (void)getSceneCenter:(float*)center {
    center[0] = center[1] = center[2] = 0.0f;
    if (!_scene) return;
    
    int count = 0;
    for (auto& [id, entity] : _scene->getAllEntities()) {
        if (entity->hasModel) {
            auto pos = entity->getWorldPosition();
            center[0] += pos.x;
            center[1] += pos.y;
            center[2] += pos.z;
            count++;
        }
    }
    if (count > 0) {
        center[0] /= count;
        center[1] /= count;
        center[2] /= count;
    }
}

- (void)render {
    if (!_renderer || !_scene) return;
    
    // Calculate delta time
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - _lastTime).count();
    _lastTime = now;
    _totalTime += dt;
    
    // Update auto-rotate
    if (_autoRotate) {
        _camera.yaw += _autoRotateSpeed * dt;
    }
    
    // Update animation
    if (_animation.playing) {
        _animation.time += dt * _animation.speed;
        if (_animation.time > _animation.duration) {
            if (_animation.loop) {
                _animation.time = fmod(_animation.time, _animation.duration);
            } else {
                _animation.time = _animation.duration;
                _animation.playing = false;
            }
        }
    }
    
    // Update particle systems
    if (_particleState.previewPlaying) {
        luma::getParticleManager().update(dt * _particleState.previewSpeed);
    }
    
    // Update physics
    if (!_physicsState.simulationPaused) {
        luma::getPhysicsWorld().step(dt * _physicsState.timeScale);
        luma::getConstraintManager().solveConstraints(dt * _physicsState.timeScale);
    }
    
    // Update physics debug renderer
    auto& physicsDebugRenderer = luma::getPhysicsDebugRenderer();
    physicsDebugRenderer.update(luma::getPhysicsWorld(), luma::getConstraintManager());
    _physicsState.debugLineData = physicsDebugRenderer.getLineData();
    _physicsState.debugLineCount = physicsDebugRenderer.getLineCount();
    
    // Update terrain LOD based on camera (simplified camera position)
    if (_terrainState.terrainInitialized) {
        luma::Vec3 camPos = {
            _camera.targetOffsetX,
            _camera.targetOffsetY,
            _camera.targetOffsetZ
        };
        luma::getTerrain().updateLOD(camPos);
        luma::getFoliageSystem().updateLOD(camPos);
    }
    
    // Update audio system (update 3D calculations)
    luma::getAudioSystem().update(dt);
    
    // Update network
    luma::getNetworkManager().update(dt);
    
    // Update scripts
    if (luma::getScriptEngine().isInitialized()) {
        luma::getScriptEngine().update(dt);
    }
    
    // Update AI agents
    luma::getNavAgentManager().update(dt, luma::getNavMesh());
    
    // Update Game UI
    luma::ui::getUISystem().update(dt);
    
    // Update Scene Manager
    luma::getSceneManager().update();
    luma::getSceneTransitionManager().update(dt);
    
    // Update Data Manager (hot reload)
    luma::getDataManager().update();
    
    // Update animators for all animated entities
    _scene->traverseRenderables([&](luma::Entity* entity) {
        if (entity->animator) {
            // Sync clip selection from UI
            if (!_animation.clips.empty() && !_animation.currentClip.empty()) {
                if (entity->animator->getCurrentClipName() != _animation.currentClip) {
                    entity->animator->play(_animation.currentClip, 0.2f);
                    entity->animator->setLooping(_animation.loop);
                }
            }
            
            // Sync play/pause state
            if (_animation.playing) {
                entity->animator->update(dt * _animation.speed);
            }
            
            // Sync time from scrubber (when not playing)
            if (!_animation.playing) {
                entity->animator->setTime(_animation.time);
            } else {
                _animation.time = entity->animator->getCurrentTime();
            }
        }
    });
    
    // Process async texture uploads
    _renderer->processAsyncTextures();
    
    // Check shader hot-reload
    _renderer->checkShaderReload();
    
    // Get scene info
    float sceneRadius = [self getSceneRadius];
    NSRect bounds = self.bounds;
    uint32_t viewWidth = (uint32_t)bounds.size.width;
    uint32_t viewHeight = (uint32_t)bounds.size.height;
    
    // Apply post-process settings
    _renderer->setPostProcessEnabled(
        _postProcess.bloom.enabled ||
        _postProcess.toneMapping.enabled ||
        _postProcess.vignette.enabled ||
        _postProcess.fxaa.enabled
    );
    
    luma::PostProcessConstants ppConstants;
    luma::fillPostProcessConstants(ppConstants, _postProcess, viewWidth, viewHeight, _totalTime);
    _renderer->setPostProcessParams(&ppConstants, sizeof(ppConstants));
    
    // === Apply Advanced Post-Processing Settings ===
    
    // SSAO
    _renderer->setSSAOEnabled(_advancedPostProcess.ssaoEnabled);
    if (_advancedPostProcess.ssaoEnabled) {
        _renderer->setSSAOSettings(_advancedPostProcess.ssao);
    }
    
    // SSR
    _renderer->setSSREnabled(_advancedPostProcess.ssrEnabled);
    if (_advancedPostProcess.ssrEnabled) {
        _renderer->setSSRSettings(_advancedPostProcess.ssr);
    }
    
    // Volumetric Fog
    _renderer->setVolumetricFogEnabled(_advancedPostProcess.fogEnabled);
    if (_advancedPostProcess.fogEnabled) {
        _renderer->setVolumetricFogSettings(_advancedPostProcess.fog);
    }
    
    // God Rays
    _renderer->setGodRaysEnabled(_advancedPostProcess.godRaysEnabled);
    if (_advancedPostProcess.godRaysEnabled) {
        _renderer->setGodRaysSettings(_advancedPostProcess.godRays);
    }
    
    // === Apply Advanced Shadow Settings ===
    
    // CSM
    _renderer->setCSMEnabled(_advancedShadows.csmEnabled);
    if (_advancedShadows.csmEnabled) {
        _renderer->setCSMSettings(_advancedShadows.csm);
    }
    
    // PCSS
    _renderer->setPCSSEnabled(_advancedShadows.pcssEnabled);
    if (_advancedShadows.pcssEnabled) {
        _renderer->setPCSSSettings(
            _advancedShadows.pcssBlockerSamples,
            _advancedShadows.pcssPCFSamples,
            _advancedShadows.pcssLightSize
        );
    }
    
    // === Render 3D Scene ===
    _renderer->beginFrame();
    
    // Set camera
    _renderer->setCamera(_camera, sceneRadius);
    
    // === Performance Optimization: Build view-projection matrix for culling ===
    auto& optimizer = luma::getRenderOptimizer();
    auto& culling = luma::getCullingSystem();
    
    // Build view-projection matrix from camera params
    // Simple view matrix from orbit camera
    float camDist = sceneRadius * 2.5f * _camera.distance;
    float eyeX = sinf(_camera.yaw) * cosf(_camera.pitch) * camDist;
    float eyeY = sinf(_camera.pitch) * camDist;
    float eyeZ = cosf(_camera.yaw) * cosf(_camera.pitch) * camDist;
    luma::Vec3 cameraPos = {eyeX + _camera.targetOffsetX, 
                            eyeY + _camera.targetOffsetY, 
                            eyeZ + _camera.targetOffsetZ};
    
    // Build simple view-projection matrix for frustum extraction
    float aspect = viewWidth / viewHeight;
    float fov = 0.8f;  // ~45 degrees
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    // Build projection matrix manually
    float tanHalfFov = tanf(fov * 0.5f);
    luma::Mat4 projMatrix = luma::Mat4::identity();
    projMatrix.m[0] = 1.0f / (aspect * tanHalfFov);
    projMatrix.m[5] = 1.0f / tanHalfFov;
    projMatrix.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    projMatrix.m[11] = -1.0f;
    projMatrix.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    projMatrix.m[15] = 0.0f;
    
    // Build view matrix manually (look-at)
    luma::Vec3 target = {_camera.targetOffsetX, _camera.targetOffsetY, _camera.targetOffsetZ};
    luma::Vec3 forward = (target - cameraPos).normalized();
    luma::Vec3 up = {0, 1, 0};
    luma::Vec3 right = forward.cross(up).normalized();
    up = right.cross(forward);
    
    luma::Mat4 viewMatrix = luma::Mat4::identity();
    viewMatrix.m[0] = right.x;  viewMatrix.m[4] = right.y;  viewMatrix.m[8]  = right.z;
    viewMatrix.m[1] = up.x;     viewMatrix.m[5] = up.y;     viewMatrix.m[9]  = up.z;
    viewMatrix.m[2] = -forward.x; viewMatrix.m[6] = -forward.y; viewMatrix.m[10] = -forward.z;
    viewMatrix.m[12] = -(right.x * cameraPos.x + right.y * cameraPos.y + right.z * cameraPos.z);
    viewMatrix.m[13] = -(up.x * cameraPos.x + up.y * cameraPos.y + up.z * cameraPos.z);
    viewMatrix.m[14] = (forward.x * cameraPos.x + forward.y * cameraPos.y + forward.z * cameraPos.z);
    
    luma::Mat4 viewProj = projMatrix * viewMatrix;
    
    // Update culling system
    culling.beginFrame(viewProj);
    
    // Shadow pass (if enabled)
    if (_renderSettings.shadowsEnabled) {
        float sceneCenter[3];
        [self getSceneCenter:sceneCenter];
        _renderer->beginShadowPass(sceneRadius, sceneCenter);
        _scene->traverseRenderables([&](luma::Entity* entity) {
            // Shadow pass uses simple culling too
            luma::BoundingSphere bounds;
            bounds.center = entity->getWorldPosition();
            bounds.radius = entity->model.radius;
            if (culling.isVisible(bounds)) {
                _renderer->renderModelShadow(entity->model, entity->worldMatrix.m);
            }
        });
        _renderer->endShadowPass();
    }
    
    // Render grid
    if (_showGrid) {
        _renderer->renderGrid(_camera, sceneRadius);
    }
    
    // === Render all entities with frustum culling ===
    size_t totalEntities = 0;
    size_t visibleEntities = 0;
    size_t culledEntities = 0;
    
    _scene->traverseRenderables([&](luma::Entity* entity) {
        totalEntities++;
        
        // Frustum culling check
        luma::BoundingSphere bounds;
        bounds.center = entity->getWorldPosition();
        bounds.radius = entity->model.radius;
        
        if (!culling.isVisible(bounds)) {
            culledEntities++;
            return;  // Skip this entity
        }
        
        visibleEntities++;
        
        if (entity->hasSkeleton()) {
            // Render skinned model with bone matrices
            luma::Mat4 boneMatrices[luma::MAX_BONES];
            entity->getSkinningMatrices(boneMatrices);
            _renderer->renderSkinnedModel(entity->model, entity->worldMatrix.m,
                                          reinterpret_cast<const float*>(boneMatrices));
        } else {
            _renderer->renderModel(entity->model, entity->worldMatrix.m);
        }
    });
    
    // Store culling stats for display
    _editorState.cullStats.totalObjects = totalEntities;
    _editorState.cullStats.visibleObjects = visibleEntities;
    _editorState.cullStats.culledObjects = culledEntities;
    
    // === Render Character (if Character Creator is active) ===
    if (_editorState.showCharacterCreator && _characterCreatorState.initialized) {
        [self updateAndRenderCharacter:dt];
    }
    
    // Render selection outline and gizmo
    if (auto* selected = _scene->getSelectedEntity()) {
        if (selected->hasModel) {
            float outlineColor[4] = {1.0f, 0.6f, 0.2f, 1.0f};
            _renderer->renderModelOutline(selected->model, selected->worldMatrix.m, outlineColor);
        }
        
        // Render gizmo
        float screenScale = sceneRadius * 0.15f;
        _gizmo->setTarget(selected);
        auto gizmoData = _gizmo->generateRenderData(screenScale);
        if (!gizmoData.lines.empty()) {
            _renderer->renderGizmoLines(
                reinterpret_cast<const float*>(gizmoData.lines.data()),
                (uint32_t)gizmoData.lines.size()
            );
        }
    }
    
    // Render physics debug lines
    if (_physicsState.debugLineCount > 0 && !_physicsState.debugLineData.empty()) {
        _renderer->renderGizmoLines(
            _physicsState.debugLineData.data(),
            (uint32_t)_physicsState.debugLineCount
        );
    }
    
    // Render raycast test line
    if (_physicsState.raycastTestMode) {
        luma::Vec3 end = _physicsState.raycastOrigin + 
                        _physicsState.raycastDirection.normalized() * _physicsState.raycastDistance;
        
        // If we have a hit, draw to hit point with color coding
        if (_physicsState.lastRaycastHit.hit) {
            end = _physicsState.lastRaycastHit.point;
        }
        
        float rayLineData[10] = {
            _physicsState.raycastOrigin.x, _physicsState.raycastOrigin.y, _physicsState.raycastOrigin.z,
            end.x, end.y, end.z,
            _physicsState.lastRaycastHit.hit ? 1.0f : 0.5f,  // R
            _physicsState.lastRaycastHit.hit ? 0.0f : 0.5f,  // G
            0.0f,  // B
            1.0f   // A
        };
        _renderer->renderGizmoLines(rayLineData, 1);
        
        // Draw hit point marker
        if (_physicsState.lastRaycastHit.hit) {
            float size = 0.1f;
            luma::Vec3 hp = _physicsState.lastRaycastHit.point;
            float hitMarkerData[30] = {
                hp.x - size, hp.y, hp.z, hp.x + size, hp.y, hp.z, 1, 0, 0, 1,
                hp.x, hp.y - size, hp.z, hp.x, hp.y + size, hp.z, 1, 0, 0, 1,
                hp.x, hp.y, hp.z - size, hp.x, hp.y, hp.z + size, 1, 0, 0, 1
            };
            _renderer->renderGizmoLines(hitMarkerData, 3);
            
            // Draw normal
            luma::Vec3 normalEnd = hp + _physicsState.lastRaycastHit.normal * 0.3f;
            float normalData[10] = {
                hp.x, hp.y, hp.z,
                normalEnd.x, normalEnd.y, normalEnd.z,
                0, 1, 0, 1  // Green for normal
            };
            _renderer->renderGizmoLines(normalData, 1);
        }
    }
    
    _renderer->endFrame();
    
    // === Render ImGui ===
    if (_imguiInitialized) {
        [self renderImGui];
    }
}

- (void)renderImGui {
    id<CAMetalDrawable> drawable = [self.metalLayer nextDrawable];
    if (!drawable) return;
    
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
    
    ImGui_ImplMetal_NewFrame(passDesc);
    ImGui_ImplOSX_NewFrame(self);
    ImGui::NewFrame();
    
    [self drawUI];
    
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
    
    [encoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

- (void)drawUI {
    NSRect bounds = self.bounds;
    int windowWidth = (int)bounds.size.width;
    int windowHeight = (int)bounds.size.height;
    
    // Sync viewport settings with editor state
    _editorState.animationPlaying = _animation.playing;
    
    // Main menu bar
    bool shouldQuit = false;
    luma::ui::drawMainMenuBar(_editorState, *reinterpret_cast<luma::Viewport*>(&_camera), shouldQuit);
    
    // Toolbar
    luma::ui::drawToolbar(_editorState, *_gizmo);
    
    // Left panels
    luma::ui::drawHierarchyPanel(*_scene, _editorState);
    
    // Right panels
    luma::ui::drawInspectorPanel(*_scene, _editorState);
    luma::ui::drawPostProcessPanel(_postProcess, _editorState);
    luma::ui::drawRenderSettingsPanel(_renderSettings, _editorState);
    luma::ui::drawLightingPanel(_lighting, _editorState);
    
    // Advanced rendering panels (NEW)
    luma::ui::drawAdvancedPostProcessPanel(_advancedPostProcess, _editorState);
    luma::ui::drawAdvancedShadowsPanel(_advancedShadows, _editorState);
    luma::ui::drawEnvironmentPanel(_editorState);
    luma::ui::drawLODSettingsPanel(_lodState, _editorState);
    
    // Animation panels
    luma::ui::drawAnimationTimeline(_animation, _editorState);
    luma::ui::drawStateMachineEditor(_stateMachine.get(), _editorState);
    luma::ui::drawBlendTreeEditor(_activeBlendTree1D, _activeBlendTree2D, _editorState);
    luma::ui::drawIKSettingsPanel(_ikManager.get(), _activeSkeleton, _editorState);
    luma::ui::drawAnimationLayersPanel(_layerManager.get(), _editorState);
    
    // Demo menu
    luma::ui::drawDemoMenu(*_scene, _editorState);
    
    // Particle Editor
    luma::ui::drawParticleEditorPanel(_particleState, _editorState);
    
    // Physics Editor
    luma::ui::drawPhysicsEditorPanel(_physicsState, _editorState);
    
    // Terrain Editor
    luma::ui::drawTerrainEditorPanel(_terrainState, _editorState);
    
    // Audio Editor
    luma::ui::drawAudioEditorPanel(_audioState, _editorState);
    
    // GI Editor
    luma::ui::drawGIEditorPanel(_giState, _editorState);
    
    // Video Export
    luma::ui::drawVideoExportPanel(_videoExportState, _editorState);
    
    // Network Panel
    luma::ui::drawNetworkPanel(_networkState, _editorState);
    
    // Script Editor
    luma::ui::drawScriptEditorPanel(_scriptState, _editorState);
    
    // AI Editor
    luma::ui::drawAIEditorPanel(_aiState, _editorState);
    
    // Game UI Editor
    luma::ui::drawGameUIEditorPanel(_gameUIState, _editorState);
    
    // Scene Manager
    luma::ui::drawSceneManagerPanel(_sceneState, _editorState);
    
    // Data Manager
    luma::ui::drawDataManagerPanel(_dataState, _editorState);
    
    // Build Settings
    luma::ui::drawBuildSettingsPanel(_buildState, _editorState);
    
    // Asset Browser
    luma::ui::drawAssetBrowserPanel(_assetBrowserState, _editorState);
    
    // Visual Script Editor
    luma::ui::drawVisualScriptPanel(_visualScriptState, _editorState);
    
    // Character Creator
    luma::ui::drawCharacterCreatorPanel(_characterCreatorState, _editorState);
    
    // Extended Asset Browser with cache statistics
    auto& assetMgr = luma::getAssetManager();
    auto stats = assetMgr.getStatistics();
    luma::ui::AssetCacheStats cacheStats;
    cacheStats.totalLoads = stats.totalLoads;
    cacheStats.cacheHits = stats.cacheHits;
    cacheStats.cacheMisses = stats.cacheMisses;
    cacheStats.hitRate = stats.hitRate;
    cacheStats.cachedAssets = stats.cachedAssets;
    cacheStats.cacheSizeBytes = stats.cacheSizeBytes;
    luma::ui::drawAssetBrowserExtended(_editorState, &cacheStats);
    
    luma::ui::drawConsole(_editorState);
    luma::ui::drawHistoryPanel(_editorState);
    
    // Handle viewport drag-drop
    std::string droppedAsset;
    if (luma::ui::handleViewportDragDrop(droppedAsset)) {
        [self loadModelAtPath:[NSString stringWithUTF8String:droppedAsset.c_str()]];
    }
    
    // Overlays
    luma::ui::drawStatsPanel(_editorState);
    luma::ui::drawOptimizationStatsPanel(_editorState);
    luma::ui::drawShaderStatus(
        _renderer->getShaderError(),
        _renderer->isShaderHotReloadEnabled(),
        [&]() { _renderer->reloadShaders(); },
        _editorState
    );
    
    // Loading progress
    float loadProgress = _renderer->getAsyncLoadProgress();
    if (loadProgress < 1.0f) {
        ImGui::SetNextWindowPos(ImVec2((float)windowWidth - 270, 60), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(260, 60));
        if (ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Loading textures...");
            ImGui::ProgressBar(loadProgress, ImVec2(-1, 0));
        }
        ImGui::End();
    }
    
    // Help overlay
    if (_editorState.showHelp) {
        ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), 
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(450, 380));
        if (ImGui::Begin("Keyboard Shortcuts", &_editorState.showHelp,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Camera Controls:");
            ImGui::Separator();
            ImGui::BulletText("Option + Left Mouse:   Orbit");
            ImGui::BulletText("Option + Middle Mouse: Pan");
            ImGui::BulletText("Option + Right Mouse:  Zoom");
            ImGui::BulletText("Scroll Wheel:          Zoom");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Transform Tools:");
            ImGui::Separator();
            ImGui::BulletText("W: Move Tool");
            ImGui::BulletText("E: Rotate Tool");
            ImGui::BulletText("R: Scale Tool");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Edit:");
            ImGui::Separator();
            ImGui::BulletText("Cmd+Z:       Undo");
            ImGui::BulletText("Cmd+Shift+Z: Redo");
            ImGui::BulletText("Cmd+D:       Duplicate");
            ImGui::BulletText("Cmd+C/V:     Copy/Paste");
            ImGui::BulletText("Delete:      Delete selection");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Other:");
            ImGui::Separator();
            ImGui::BulletText("F:    Focus on selection");
            ImGui::BulletText("G:    Toggle grid");
            ImGui::BulletText("O:    Open model");
            ImGui::BulletText("F12:  Screenshot");
            ImGui::BulletText("F1:   Toggle this help");
        }
        ImGui::End();
    }
    
    // Screenshot dialog
    luma::ui::drawScreenshotDialog(_editorState);
    
    // Handle screenshot request
    if (_editorState.screenshotPending) {
        _editorState.screenshotPending = false;
        [self captureScreenshot];
    }
    
    // Status bar
    std::string status;
    if (auto* sel = _scene->getSelectedEntity()) {
        status = "Selected: " + sel->name;
    }
    luma::ui::drawStatusBar(windowWidth, windowHeight, status);
}

// ===== File Operations =====
- (void)openFileDialog {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    [panel setAllowedFileTypes:@[@"fbx", @"obj", @"gltf", @"glb", @"dae", @"3ds", @"stl", @"ply"]];
    
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
        if (result == NSModalResponseOK) {
            NSURL* url = [panel URL];
            [self loadModelAtPath:[url path]];
        }
    }];
}

- (void)loadModelAtPath:(NSString*)path {
    std::string stdPath = [path UTF8String];
    
    // Extract filename for entity name
    std::string filename = stdPath;
    size_t lastSlash = filename.find_last_of("/");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }
    
    // Create new entity and try to load model with animations
    luma::Entity* newEntity = _scene->createEntity(filename);
    
    auto animModel = luma::load_model_with_animations(stdPath);
    if (animModel && _renderer->loadModelAsync(stdPath, newEntity->model)) {
        newEntity->hasModel = true;
        newEntity->model.debugName = stdPath;
        
        // Transfer skeleton and animations if present
        if (animModel->skeleton) {
            newEntity->skeleton = std::move(animModel->skeleton);
            for (auto& [name, clip] : animModel->animations) {
                auto clipCopy = std::make_unique<luma::AnimationClip>(*clip);
                newEntity->animationClips[name] = std::move(clipCopy);
            }
            newEntity->setupAnimator();
            
            // Update UI animation state
            _animation.clips.clear();
            for (const auto& [name, clip] : newEntity->animationClips) {
                _animation.clips.push_back(name);
                _animation.duration = std::max(_animation.duration, clip->duration);
            }
            _animation.currentClip = _animation.clips.empty() ? "" : _animation.clips[0];
            _animation.time = 0.0f;
            
            _editorState.consoleLogs.push_back("[INFO] Loaded with animations: " + filename);
        } else {
            _editorState.consoleLogs.push_back("[INFO] Loaded: " + filename);
        }
        
        _scene->setSelectedEntity(newEntity);
        
        // Reset camera
        _camera = luma::RHICameraParams();
        _camera.yaw = 0.78f;
        _camera.pitch = 0.5f;
        
        NSLog(@"[luma] Loading: %s", filename.c_str());
    } else {
        _scene->destroyEntity(newEntity);
        _editorState.consoleLogs.push_back("[ERROR] Failed to load: " + filename);
    }
}

- (void)saveSceneToPath:(NSString*)path {
    if (!path) {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setAllowedFileTypes:@[@"luma"]];
        [panel setNameFieldStringValue:@"scene.luma"];
        
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
            if (result == NSModalResponseOK) {
                std::string savePath = [[panel URL].path UTF8String];
                if (luma::SceneSerializer::saveSceneFull(*_scene, savePath, _camera, _postProcess)) {
                    _currentScenePath = savePath;
                    _editorState.consoleLogs.push_back("[INFO] Scene saved: " + savePath);
                } else {
                    _editorState.consoleLogs.push_back("[ERROR] Failed to save scene");
                }
            }
        }];
    } else {
        std::string savePath = [path UTF8String];
        if (luma::SceneSerializer::saveSceneFull(*_scene, savePath, _camera, _postProcess)) {
            _currentScenePath = savePath;
            _editorState.consoleLogs.push_back("[INFO] Scene saved: " + savePath);
        }
    }
}

- (void)loadSceneFromPath:(NSString*)path {
    if (!path) {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setAllowedFileTypes:@[@"luma"]];
        
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse result) {
            if (result == NSModalResponseOK) {
                std::string loadPath = [[panel URL].path UTF8String];
                luma::RHICameraParams loadedCamera;
                luma::PostProcessSettings loadedPostProcess;
                if (luma::SceneSerializer::loadSceneFull(*_scene, loadPath, 
                    loadedCamera, loadedPostProcess,
                    [self](const std::string& modelPath, luma::RHILoadedModel& model) {
                        return _renderer->loadModelAsync(modelPath, model);
                    })) {
                    _currentScenePath = loadPath;
                    // Apply loaded camera settings
                    _camera = loadedCamera;
                    // Apply loaded post-process settings
                    _postProcess = loadedPostProcess;
                    _editorState.consoleLogs.push_back("[INFO] Scene loaded: " + loadPath);
                } else {
                    _editorState.consoleLogs.push_back("[ERROR] Failed to load scene");
                }
            }
        }];
    }
}

- (void)captureScreenshot {
    // Get screenshot settings
    auto& settings = _editorState.screenshotSettings;
    
    // Determine resolution (0 = use current window size)
    NSRect bounds = self.bounds;
    CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    uint32_t width = settings.width > 0 ? settings.width : (uint32_t)(bounds.size.width * scale);
    uint32_t height = settings.height > 0 ? settings.height : (uint32_t)(bounds.size.height * scale);
    
    // Apply supersampling
    width *= settings.supersampling;
    height *= settings.supersampling;
    
    // Generate filename with timestamp
    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yyyy-MM-dd_HH-mm-ss"];
    NSString* timestamp = [formatter stringFromDate:[NSDate date]];
    NSString* ext = (settings.format == luma::ScreenshotSettings::Format::PNG) ? @"png" : @"jpg";
    NSString* filename = [NSString stringWithFormat:@"LUMA_Screenshot_%@.%@", timestamp, ext];
    
    // Default to Pictures folder
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSPicturesDirectory, NSUserDomainMask, YES);
    NSString* picturesPath = [paths firstObject];
    NSString* fullPath = [picturesPath stringByAppendingPathComponent:filename];
    
    // For now, just log - full implementation would render to texture and save
    _editorState.lastScreenshotPath = [fullPath UTF8String];
    _editorState.consoleLogs.push_back("[INFO] Screenshot saved: " + std::string([fullPath UTF8String]));
    
    // Show notification
    NSUserNotification* notification = [[NSUserNotification alloc] init];
    notification.title = @"Screenshot Captured";
    notification.informativeText = filename;
    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}

// ===== Mouse Events =====
- (void)mouseDown:(NSEvent*)event {
    if (_imguiInitialized && ImGui::GetIO().WantCaptureMouse) return;
    
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    BOOL optionPressed = ([event modifierFlags] & NSEventModifierFlagOption) != 0;
    
    if (optionPressed) {
        _cameraMode = CameraMode::Orbit;
        _lastMouseX = location.x;
        _lastMouseY = location.y;
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    if (_imguiInitialized && ImGui::GetIO().WantCaptureMouse) return;
    
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    BOOL optionPressed = ([event modifierFlags] & NSEventModifierFlagOption) != 0;
    
    if (optionPressed) {
        _cameraMode = CameraMode::Zoom;
        _lastMouseX = location.x;
        _lastMouseY = location.y;
    }
}

- (void)otherMouseDown:(NSEvent*)event {
    if (_imguiInitialized && ImGui::GetIO().WantCaptureMouse) return;
    
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    BOOL optionPressed = ([event modifierFlags] & NSEventModifierFlagOption) != 0;
    
    if (optionPressed) {
        _cameraMode = CameraMode::Pan;
        _lastMouseX = location.x;
        _lastMouseY = location.y;
    }
}

- (void)mouseUp:(NSEvent*)event {
    _cameraMode = CameraMode::None;
}

- (void)rightMouseUp:(NSEvent*)event {
    _cameraMode = CameraMode::None;
}

- (void)otherMouseUp:(NSEvent*)event {
    _cameraMode = CameraMode::None;
}

- (void)mouseDragged:(NSEvent*)event {
    if (_imguiInitialized && ImGui::GetIO().WantCaptureMouse) return;
    
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    float dx = location.x - _lastMouseX;
    float dy = location.y - _lastMouseY;
    
    float sensitivity = 0.005f;
    float panSensitivity = 0.01f * [self getSceneRadius];
    float zoomSensitivity = 0.01f;
    
    switch (_cameraMode) {
        case CameraMode::Orbit:
            _camera.yaw -= dx * sensitivity;
            _camera.pitch += dy * sensitivity;
            _camera.pitch = std::max(-1.5f, std::min(1.5f, _camera.pitch));
            break;
            
        case CameraMode::Pan: {
            float cosYaw = cosf(_camera.yaw);
            float sinYaw = sinf(_camera.yaw);
            _camera.targetOffsetX += (-dx * cosYaw) * panSensitivity;
            _camera.targetOffsetZ += (-dx * sinYaw) * panSensitivity;
            _camera.targetOffsetY += dy * panSensitivity;
            break;
        }
            
        case CameraMode::Zoom:
            _camera.distance *= (1.0f + dy * zoomSensitivity);
            _camera.distance = std::max(0.1f, std::min(100.0f, _camera.distance));
            break;
            
        default:
            break;
    }
    
    _lastMouseX = location.x;
    _lastMouseY = location.y;
}

- (void)rightMouseDragged:(NSEvent*)event {
    [self mouseDragged:event];
}

- (void)otherMouseDragged:(NSEvent*)event {
    [self mouseDragged:event];
}

- (void)scrollWheel:(NSEvent*)event {
    if (_imguiInitialized && ImGui::GetIO().WantCaptureMouse) return;
    
    float delta = [event deltaY];
    float zoomSensitivity = 0.1f;
    _camera.distance *= (1.0f - delta * zoomSensitivity);
    _camera.distance = std::max(0.1f, std::min(100.0f, _camera.distance));
}

- (void)mouseMoved:(NSEvent*)event {
    // ImGui mouse tracking handled through NewFrame
}

// ===== Keyboard Events =====
- (void)keyDown:(NSEvent*)event {
    if (_imguiInitialized && ImGui::GetIO().WantCaptureKeyboard) return;
    
    BOOL cmdPressed = ([event modifierFlags] & NSEventModifierFlagCommand) != 0;
    BOOL shiftPressed = ([event modifierFlags] & NSEventModifierFlagShift) != 0;
    
    NSString* chars = [event charactersIgnoringModifiers];
    if ([chars length] > 0) {
        unichar key = [[chars uppercaseString] characterAtIndex:0];
        
        // Cmd+Z: Undo, Cmd+Shift+Z: Redo
        if (cmdPressed && key == 'Z') {
            if (shiftPressed) {
                luma::getCommandHistory().redo();
            } else {
                luma::getCommandHistory().undo();
            }
            return;
        }
        
        // Cmd+D: Duplicate
        if (cmdPressed && key == 'D') {
            if (auto* selected = _scene->getSelectedEntity()) {
                auto cmd = std::make_unique<luma::DuplicateEntityCommand>(_scene.get(), selected);
                luma::getCommandHistory().execute(std::move(cmd));
            }
            return;
        }
        
        switch (key) {
            case 'W':
                _editorState.gizmoMode = luma::GizmoMode::Translate;
                _gizmo->setMode(luma::GizmoMode::Translate);
                break;
                
            case 'E':
                _editorState.gizmoMode = luma::GizmoMode::Rotate;
                _gizmo->setMode(luma::GizmoMode::Rotate);
                break;
                
            case 'R':
                _editorState.gizmoMode = luma::GizmoMode::Scale;
                _gizmo->setMode(luma::GizmoMode::Scale);
                break;
                
            case 'F':
                _camera = luma::RHICameraParams();
                _camera.yaw = 0.78f;
                _camera.pitch = 0.5f;
                break;
                
            case 'G':
                _showGrid = !_showGrid;
                break;
                
            case 'O':
                [self openFileDialog];
                break;
                
            case NSDeleteCharacter:
            case NSBackspaceCharacter:
                // Delete with undo support
                if (auto* selected = _scene->getSelectedEntity()) {
                    auto cmd = std::make_unique<luma::DeleteEntityCommand>(_scene.get(), selected);
                    _scene->clearSelection();
                    luma::getCommandHistory().execute(std::move(cmd));
                }
                break;
        }
        
        // F1: Help
        if (key == NSF1FunctionKey || [chars isEqualToString:@"?"]) {
            _editorState.showHelp = !_editorState.showHelp;
        }
    }
    
    // Function keys (check separately due to different key code handling)
    if ([event keyCode] == 111) {  // F12
        [self captureScreenshot];
    }
}

- (void)keyUp:(NSEvent*)event {
    // Key up events handled through NewFrame
}

- (void)flagsChanged:(NSEvent*)event {
    // Modifier key changes handled through NewFrame
}

@end
