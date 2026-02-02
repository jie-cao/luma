// LUMA Viewer iOS - Main View Controller Implementation
// Metal-based 3D model viewer with touch gesture controls

#import "LumaViewController.h"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>

#include "engine/renderer/unified_renderer.h"
#include "engine/renderer/post_process.h"
#include "engine/scene/scene_graph.h"
#include "engine/asset/model_loader.h"
#include "engine/serialization/scene_serializer.h"
#include "engine/rendering/culling.h"
#include "engine/rendering/lod.h"

#include <memory>
#include <chrono>
#include <cmath>

// Touch gesture mode
typedef NS_ENUM(NSInteger, TouchMode) {
    TouchModeNone = 0,
    TouchModeOrbit,      // Single finger drag - orbit rotation
    TouchModePan,        // Two finger drag - pan
    TouchModeZoom        // Pinch - zoom
};

#pragma mark - LumaMetalView

@interface LumaMetalView : MTKView
@property (nonatomic, weak) LumaViewController *viewController;
@end

@implementation LumaMetalView
@end

#pragma mark - LumaViewController

@interface LumaViewController () <MTKViewDelegate> {
    // Core systems
    std::unique_ptr<luma::UnifiedRenderer> _renderer;
    std::unique_ptr<luma::SceneGraph> _scene;
    luma::RHICameraParams _camera;
    
    // Performance systems
    std::unique_ptr<luma::FrustumCuller> _frustumCuller;
    std::unique_ptr<luma::LODManager> _lodManager;
    
    // Camera state
    float _cameraYaw;
    float _cameraPitch;
    float _cameraDistance;
    float _cameraTargetX;
    float _cameraTargetY;
    float _cameraTargetZ;
    
    // Touch state
    TouchMode _touchMode;
    CGPoint _lastTouchLocation;
    CGFloat _lastPinchScale;
    CGPoint _lastPanTranslation;
    
    // Timing
    std::chrono::high_resolution_clock::time_point _lastTime;
    float _totalTime;
    float _deltaTime;
    
    // Settings
    BOOL _showGrid;
    BOOL _autoRotate;
    float _autoRotateSpeed;
    BOOL _wireframeMode;
    
    // Model info
    NSString *_modelName;
    NSUInteger _vertexCount;
    NSUInteger _meshCount;
}

@property (nonatomic, strong) LumaMetalView *metalView;
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;

// Gesture recognizers
@property (nonatomic, strong) UIPanGestureRecognizer *panGesture;
@property (nonatomic, strong) UIPinchGestureRecognizer *pinchGesture;
@property (nonatomic, strong) UIRotationGestureRecognizer *rotationGesture;
@property (nonatomic, strong) UITapGestureRecognizer *doubleTapGesture;

// UI elements
@property (nonatomic, strong) UILabel *infoLabel;
@property (nonatomic, strong) UIToolbar *toolbar;

@end

@implementation LumaViewController

#pragma mark - Lifecycle

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.view.backgroundColor = [UIColor blackColor];
    
    // Initialize Metal
    [self setupMetal];
    
    // Initialize camera
    [self resetCamera];
    
    // Initialize renderer and scene
    [self setupRenderer];
    
    // Setup gesture recognizers
    [self setupGestures];
    
    // Setup UI
    [self setupUI];
    
    // Initialize timing
    _lastTime = std::chrono::high_resolution_clock::now();
    _totalTime = 0.0f;
    
    // Default settings
    _showGrid = YES;
    _autoRotate = NO;
    _autoRotateSpeed = 0.5f;
    _wireframeMode = NO;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    // Load demo scene if no model loaded
    if (!_scene || _scene->getRootEntities().empty()) {
        [self createDemoScene];
    }
}

- (void)dealloc {
    _renderer.reset();
    _scene.reset();
    _frustumCuller.reset();
    _lodManager.reset();
}

#pragma mark - Metal Setup

- (void)setupMetal {
    // Get Metal device
    self.device = MTLCreateSystemDefaultDevice();
    if (!self.device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    // Create command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Create Metal view
    self.metalView = [[LumaMetalView alloc] initWithFrame:self.view.bounds device:self.device];
    self.metalView.viewController = self;
    self.metalView.delegate = self;
    self.metalView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    self.metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    self.metalView.sampleCount = 1;
    self.metalView.preferredFramesPerSecond = 60;
    self.metalView.clearColor = MTLClearColorMake(0.1, 0.1, 0.15, 1.0);
    
    [self.view addSubview:self.metalView];
}

- (void)setupRenderer {
    if (!self.device) return;
    
    // Create renderer
    _renderer = std::make_unique<luma::UnifiedRenderer>();
    
    CGSize size = self.metalView.bounds.size;
    float scale = self.metalView.contentScaleFactor;
    int width = (int)(size.width * scale);
    int height = (int)(size.height * scale);
    
    _renderer->initialize((__bridge void*)self.device, width, height);
    
    // Create scene
    _scene = std::make_unique<luma::SceneGraph>();
    
    // Create performance systems
    _frustumCuller = std::make_unique<luma::FrustumCuller>();
    _lodManager = std::make_unique<luma::LODManager>();
    _lodManager->setQuality(luma::LODQuality::High);
}

#pragma mark - Gesture Setup

- (void)setupGestures {
    // Single finger pan for orbit rotation
    self.panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanGesture:)];
    self.panGesture.minimumNumberOfTouches = 1;
    self.panGesture.maximumNumberOfTouches = 1;
    [self.metalView addGestureRecognizer:self.panGesture];
    
    // Pinch for zoom
    self.pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(handlePinchGesture:)];
    [self.metalView addGestureRecognizer:self.pinchGesture];
    
    // Two finger pan for camera pan
    UIPanGestureRecognizer *twoFingerPan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleTwoFingerPanGesture:)];
    twoFingerPan.minimumNumberOfTouches = 2;
    twoFingerPan.maximumNumberOfTouches = 2;
    [self.metalView addGestureRecognizer:twoFingerPan];
    
    // Double tap to reset camera
    self.doubleTapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleDoubleTapGesture:)];
    self.doubleTapGesture.numberOfTapsRequired = 2;
    [self.metalView addGestureRecognizer:self.doubleTapGesture];
    
    // Allow simultaneous gesture recognition
    self.panGesture.delegate = (id<UIGestureRecognizerDelegate>)self;
    self.pinchGesture.delegate = (id<UIGestureRecognizerDelegate>)self;
}

#pragma mark - UI Setup

- (void)setupUI {
    // Info label at top
    self.infoLabel = [[UILabel alloc] init];
    self.infoLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.infoLabel.textColor = [UIColor whiteColor];
    self.infoLabel.font = [UIFont systemFontOfSize:12];
    self.infoLabel.numberOfLines = 0;
    self.infoLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.5];
    self.infoLabel.textAlignment = NSTextAlignmentLeft;
    [self.view addSubview:self.infoLabel];
    
    // Toolbar at bottom
    UIBarButtonItem *loadButton = [[UIBarButtonItem alloc] initWithTitle:@"Load" style:UIBarButtonItemStylePlain target:self action:@selector(showLoadOptions)];
    UIBarButtonItem *gridButton = [[UIBarButtonItem alloc] initWithTitle:@"Grid" style:UIBarButtonItemStylePlain target:self action:@selector(toggleGrid)];
    UIBarButtonItem *rotateButton = [[UIBarButtonItem alloc] initWithTitle:@"Rotate" style:UIBarButtonItemStylePlain target:self action:@selector(toggleAutoRotate)];
    UIBarButtonItem *resetButton = [[UIBarButtonItem alloc] initWithTitle:@"Reset" style:UIBarButtonItemStylePlain target:self action:@selector(resetCamera)];
    UIBarButtonItem *flexSpace = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];
    
    self.toolbar = [[UIToolbar alloc] init];
    self.toolbar.translatesAutoresizingMaskIntoConstraints = NO;
    self.toolbar.barStyle = UIBarStyleBlack;
    self.toolbar.translucent = YES;
    self.toolbar.items = @[loadButton, flexSpace, gridButton, flexSpace, rotateButton, flexSpace, resetButton];
    [self.view addSubview:self.toolbar];
    
    // Layout constraints
    [NSLayoutConstraint activateConstraints:@[
        [self.infoLabel.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:8],
        [self.infoLabel.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:8],
        [self.infoLabel.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.trailingAnchor constant:-8],
        
        [self.toolbar.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [self.toolbar.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [self.toolbar.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor]
    ]];
    
    [self updateInfoLabel];
}

- (void)updateInfoLabel {
    NSString *info = [NSString stringWithFormat:@"LUMA Viewer\n%@ | %lu meshes | %lu verts | FPS: %.0f",
                      _modelName ?: @"No model",
                      (unsigned long)_meshCount,
                      (unsigned long)_vertexCount,
                      1.0f / (_deltaTime > 0 ? _deltaTime : 0.016f)];
    self.infoLabel.text = info;
}

#pragma mark - Gesture Handlers

- (void)handlePanGesture:(UIPanGestureRecognizer *)gesture {
    CGPoint translation = [gesture translationInView:self.metalView];
    
    if (gesture.state == UIGestureRecognizerStateBegan) {
        _touchMode = TouchModeOrbit;
    } else if (gesture.state == UIGestureRecognizerStateChanged) {
        // Orbit rotation
        float sensitivity = 0.005f;
        _cameraYaw += translation.x * sensitivity;
        _cameraPitch += translation.y * sensitivity;
        
        // Clamp pitch to avoid gimbal lock
        _cameraPitch = fmax(-M_PI_2 + 0.1f, fmin(M_PI_2 - 0.1f, _cameraPitch));
        
        [gesture setTranslation:CGPointZero inView:self.metalView];
    } else if (gesture.state == UIGestureRecognizerStateEnded) {
        _touchMode = TouchModeNone;
    }
}

- (void)handleTwoFingerPanGesture:(UIPanGestureRecognizer *)gesture {
    CGPoint translation = [gesture translationInView:self.metalView];
    
    if (gesture.state == UIGestureRecognizerStateBegan) {
        _touchMode = TouchModePan;
    } else if (gesture.state == UIGestureRecognizerStateChanged) {
        // Pan camera target
        float sensitivity = 0.002f * _cameraDistance;
        
        // Calculate camera right and up vectors
        float cosYaw = cosf(_cameraYaw);
        float sinYaw = sinf(_cameraYaw);
        
        // Move target in screen space
        _cameraTargetX -= translation.x * sensitivity * cosYaw;
        _cameraTargetZ -= translation.x * sensitivity * sinYaw;
        _cameraTargetY += translation.y * sensitivity;
        
        [gesture setTranslation:CGPointZero inView:self.metalView];
    } else if (gesture.state == UIGestureRecognizerStateEnded) {
        _touchMode = TouchModeNone;
    }
}

- (void)handlePinchGesture:(UIPinchGestureRecognizer *)gesture {
    if (gesture.state == UIGestureRecognizerStateBegan) {
        _touchMode = TouchModeZoom;
        _lastPinchScale = gesture.scale;
    } else if (gesture.state == UIGestureRecognizerStateChanged) {
        // Zoom
        float scaleFactor = _lastPinchScale / gesture.scale;
        _cameraDistance *= scaleFactor;
        _cameraDistance = fmax(0.1f, fmin(100.0f, _cameraDistance));
        _lastPinchScale = gesture.scale;
    } else if (gesture.state == UIGestureRecognizerStateEnded) {
        _touchMode = TouchModeNone;
    }
}

- (void)handleDoubleTapGesture:(UITapGestureRecognizer *)gesture {
    [self resetCamera];
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    // Allow pinch and pan to work together
    if ((gestureRecognizer == self.pinchGesture || gestureRecognizer == self.panGesture) &&
        (otherGestureRecognizer == self.pinchGesture || otherGestureRecognizer == self.panGesture)) {
        return YES;
    }
    return NO;
}

#pragma mark - Actions

- (void)showLoadOptions {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Load Model"
                                                                   message:@"Choose an option"
                                                            preferredStyle:UIAlertControllerStyleActionSheet];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Load Demo Scene" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        [self createDemoScene];
    }]];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Browse Files" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        [self showDocumentPicker];
    }]];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    
    // iPad support
    alert.popoverPresentationController.barButtonItem = self.toolbar.items.firstObject;
    
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)showDocumentPicker {
    NSArray *types = @[@"public.geometry-definition-format", @"public.3d-content"];
    UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:types inMode:UIDocumentPickerModeImport];
    picker.delegate = (id<UIDocumentPickerDelegate>)self;
    picker.allowsMultipleSelection = NO;
    [self presentViewController:picker animated:YES completion:nil];
}

- (void)toggleGrid {
    _showGrid = !_showGrid;
}

- (void)toggleAutoRotate {
    _autoRotate = !_autoRotate;
}

- (void)resetCamera {
    _cameraYaw = 0.78f;  // 45 degrees
    _cameraPitch = 0.4f; // ~23 degrees
    _cameraDistance = 3.0f;
    _cameraTargetX = 0.0f;
    _cameraTargetY = 0.0f;
    _cameraTargetZ = 0.0f;
}

#pragma mark - Model Loading

- (void)loadModel:(NSString *)path {
    if (!_renderer || !_scene) return;
    
    // Clear existing scene
    _scene->clear();
    
    // Load model
    std::string pathStr = [path UTF8String];
    luma::RHILoadedModel model;
    
    if (_renderer->loadModel(pathStr, model)) {
        // Create entity
        luma::Entity* entity = _scene->createEntity([path.lastPathComponent UTF8String]);
        entity->modelPath = pathStr;
        entity->model = model;
        
        // Update info
        _modelName = path.lastPathComponent;
        _meshCount = model.meshes.size();
        _vertexCount = 0;
        for (const auto& mesh : model.meshes) {
            _vertexCount += mesh.vertexCount;
        }
        
        // Fit camera to model
        if (model.boundingRadius > 0) {
            _cameraDistance = model.boundingRadius * 2.5f;
            _cameraTargetX = model.centerX;
            _cameraTargetY = model.centerY;
            _cameraTargetZ = model.centerZ;
        }
        
        [self updateInfoLabel];
    }
}

- (void)loadScene:(NSString *)path {
    if (!_renderer || !_scene) return;
    
    std::string pathStr = [path UTF8String];
    
    // Use scene serializer
    luma::SceneSerializer::loadSceneFull(*_scene, pathStr,
        [&](const std::string& modelPath, luma::RHILoadedModel& model) {
            return _renderer->loadModel(modelPath, model);
        },
        _camera, luma::PostProcessSettings());
    
    _modelName = path.lastPathComponent;
    [self updateInfoLabel];
}

- (void)createDemoScene {
    if (!_scene) return;
    
    _scene->clear();
    
    // Create some demo entities
    luma::Entity* cube1 = _scene->createEntity("Cube_1");
    cube1->transform.position = luma::Vec3(-1.5f, 0.0f, 0.0f);
    
    luma::Entity* cube2 = _scene->createEntity("Cube_2");
    cube2->transform.position = luma::Vec3(1.5f, 0.0f, 0.0f);
    
    luma::Entity* cube3 = _scene->createEntity("Cube_3");
    cube3->transform.position = luma::Vec3(0.0f, 1.5f, 0.0f);
    
    _modelName = @"Demo Scene";
    _meshCount = 3;
    _vertexCount = 0;
    
    [self updateInfoLabel];
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    if (_renderer) {
        _renderer->resize((int)size.width, (int)size.height);
    }
}

- (void)drawInMTKView:(MTKView *)view {
    @autoreleasepool {
        [self updateFrame];
        [self renderFrame];
    }
}

#pragma mark - Update & Render

- (void)updateFrame {
    // Calculate delta time
    auto now = std::chrono::high_resolution_clock::now();
    _deltaTime = std::chrono::duration<float>(now - _lastTime).count();
    _lastTime = now;
    _totalTime += _deltaTime;
    
    // Auto rotate
    if (_autoRotate) {
        _cameraYaw += _autoRotateSpeed * _deltaTime;
    }
    
    // Update camera
    [self updateCamera];
    
    // Update info label periodically
    static int frameCounter = 0;
    if (++frameCounter % 30 == 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self updateInfoLabel];
        });
    }
}

- (void)updateCamera {
    // Calculate camera position from spherical coordinates
    float cosPitch = cosf(_cameraPitch);
    float sinPitch = sinf(_cameraPitch);
    float cosYaw = cosf(_cameraYaw);
    float sinYaw = sinf(_cameraYaw);
    
    float camX = _cameraTargetX + _cameraDistance * cosPitch * sinYaw;
    float camY = _cameraTargetY + _cameraDistance * sinPitch;
    float camZ = _cameraTargetZ + _cameraDistance * cosPitch * cosYaw;
    
    // Update camera params
    _camera.eyeX = camX;
    _camera.eyeY = camY;
    _camera.eyeZ = camZ;
    _camera.atX = _cameraTargetX;
    _camera.atY = _cameraTargetY;
    _camera.atZ = _cameraTargetZ;
    _camera.upX = 0.0f;
    _camera.upY = 1.0f;
    _camera.upZ = 0.0f;
    
    // Projection
    CGSize size = self.metalView.drawableSize;
    float aspect = size.width / size.height;
    _camera.fovY = 0.785f;  // 45 degrees
    _camera.aspectRatio = aspect;
    _camera.nearZ = 0.01f;
    _camera.farZ = 1000.0f;
}

- (void)renderFrame {
    if (!_renderer || !_scene) return;
    
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    if (!commandBuffer) return;
    
    MTLRenderPassDescriptor *renderPass = self.metalView.currentRenderPassDescriptor;
    if (!renderPass) return;
    
    // Set camera
    _renderer->setCamera(_camera);
    
    // Begin frame
    _renderer->beginFrame((__bridge void*)commandBuffer, (__bridge void*)renderPass);
    
    // Render grid
    if (_showGrid) {
        _renderer->renderGrid(10.0f, 1.0f);
    }
    
    // Render coordinate axes
    _renderer->renderAxes(1.0f);
    
    // Frustum culling
    luma::Frustum frustum;
    float viewProj[16];
    _renderer->getViewProjection(viewProj);
    frustum.extractFromMatrix(viewProj);
    _frustumCuller->setFrustum(frustum);
    
    // Render entities
    luma::Vec3 cameraPos(_camera.eyeX, _camera.eyeY, _camera.eyeZ);
    
    _scene->traverseRenderables([&](luma::Entity* entity) {
        if (!entity->active || !entity->visible) return;
        
        // Frustum culling check
        if (entity->model.meshes.size() > 0) {
            luma::BoundingSphere sphere;
            sphere.center = entity->transform.position;
            sphere.radius = entity->model.boundingRadius * 
                fmax(fmax(entity->transform.scale.x, entity->transform.scale.y), entity->transform.scale.z);
            
            if (!_frustumCuller->isVisible(sphere)) {
                return;
            }
        }
        
        // Get world matrix
        float worldMatrix[16];
        entity->getWorldMatrix().toArray(worldMatrix);
        
        // Render model
        _renderer->renderModel(entity->model, worldMatrix);
    });
    
    // End frame
    _renderer->endFrame();
    
    // Present
    [commandBuffer presentDrawable:self.metalView.currentDrawable];
    [commandBuffer commit];
}

#pragma mark - UIDocumentPickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url = urls.firstObject;
    if (url) {
        [url startAccessingSecurityScopedResource];
        [self loadModel:url.path];
        [url stopAccessingSecurityScopedResource];
    }
}

#pragma mark - Status Bar

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

- (BOOL)prefersStatusBarHidden {
    return NO;
}

@end
