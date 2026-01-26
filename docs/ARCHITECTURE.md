# LUMA Studio Architecture

## Overview

LUMA Studio is a cross-platform 3D rendering engine and content creation tool built with modern C++ (C++17/20). It supports DirectX 12 on Windows and Metal on macOS.

## Directory Structure

```
luma/
├── apps/                      # Application targets
│   ├── luma_studio/           # Windows DX12 application
│   └── luma_studio_macos/     # macOS Metal application
├── engine/                    # Core engine modules
│   ├── animation/             # Animation system
│   ├── asset/                 # Asset management
│   ├── editor/                # Editor tools & commands
│   ├── export/                # Screenshot/video export
│   ├── foundation/            # Core types (math, etc.)
│   ├── lighting/              # Light management
│   ├── material/              # PBR material system
│   ├── renderer/              # Post-processing
│   ├── rendering/             # Advanced rendering
│   ├── rhi/                   # Render hardware interface
│   ├── scene/                 # Scene graph & entities
│   ├── serialization/         # JSON/scene serialization
│   └── ui/                    # ImGui editor UI
├── tests/                     # Unit & integration tests
├── shaders/                   # HLSL/Metal shaders
└── docs/                      # Documentation
```

## Core Systems

### 1. Math Foundation (`engine/foundation/`)

Central math types shared across all systems:

- **Vec3** - 3D vector with operations (add, subtract, dot, cross, normalize)
- **Quat** - Quaternion for rotations (fromEuler, fromAxisAngle, slerp)
- **Mat4** - 4x4 matrix for transforms (identity, translation, scale, multiply)

### 2. Scene Graph (`engine/scene/`)

Hierarchical entity-component system:

```cpp
SceneGraph scene;
Entity* e = scene.createEntity("Player");
e->localTransform.position = Vec3(0, 1, 0);
scene.setParent(child, parent);
```

**Key Classes:**
- `SceneGraph` - Root container, manages entity lifecycle
- `Entity` - Scene node with transform, material, light
- `Transform` - Position, rotation, scale

### 3. Animation System (`engine/animation/`)

Full-featured skeletal animation:

```cpp
Animator animator;
animator.setSkeleton(&skeleton);
animator.addClip("walk", walkClip);
animator.play("walk", 0.2f);  // Crossfade
animator.update(deltaTime);
```

**Components:**
- `Skeleton` / `Bone` - Skeletal hierarchy
- `AnimationClip` / `AnimationChannel` - Keyframe data
- `Animator` - Playback and blending
- `AnimationLayerManager` - Multi-layer animation
- `BlendTree1D` / `BlendTree2D` - Parameter-driven blending
- `AnimationStateMachine` - State-based animation control
- `IKManager` - Inverse kinematics (TwoBone, LookAt, Foot, FABRIK)
- `Timeline` - Curve editing and event system

### 4. Material System (`engine/material/`)

Metallic-roughness PBR workflow:

```cpp
Material mat;
mat.baseColor = Vec3(1, 0.5, 0);
mat.metallic = 0.0f;
mat.roughness = 0.4f;
mat.emissiveColor = Vec3(0, 0, 0);
```

**Features:**
- Albedo, metallic, roughness, AO, normal mapping
- Emissive materials
- Material presets (Gold, Silver, Plastic, etc.)

### 5. Lighting (`engine/lighting/`)

Multi-light support:

```cpp
Light light = Light::createPoint();
light.position = Vec3(0, 5, 0);
light.color = Vec3(1, 1, 1);
light.intensity = 100.0f;
LightManager::get().addLight(light);
```

**Light Types:**
- Directional (sun)
- Point (omni)
- Spot (cone)

### 6. Rendering (`engine/rendering/`)

Advanced rendering effects:

**Culling & Optimization:**
- `FrustumCuller` - View frustum culling
- `LODManager` - Level of detail
- `InstancingManager` - GPU instancing
- `RenderOptimizer` - Combined optimization pipeline

**Post-Processing:**
- `SSAOEffect` - Screen-space ambient occlusion
- `SSRTracer` - Screen-space reflections
- `VolumetricFog` - Volumetric lighting
- `AtmosphericScattering` - Sky rendering

**Shadows:**
- `CascadedShadowMap` - CSM with 1-4 cascades
- `PCSShadows` - Percentage-closer soft shadows

**IBL:**
- `EnvironmentMap` - HDR environment processing
- Irradiance convolution
- Pre-filtered specular
- BRDF LUT generation

### 7. Editor (`engine/editor/`)

Undo/redo command system:

```cpp
CommandHistory history;
history.execute(std::make_unique<SetPositionCommand>(entity, newPos));
history.undo();
history.redo();
```

**Commands:**
- Transform commands (position, rotation, scale)
- Scene commands (create, delete, rename, reparent)
- Material commands (property changes)

### 8. Serialization (`engine/serialization/`)

JSON-based scene format:

```cpp
JsonValue sceneJson = SceneSerializer::serializeScene(scene, "MyScene");
SceneSerializer::deserializeScene(sceneJson, scene);
```

## Render Pipeline

### Frame Flow

1. **Begin Frame**
   - Update camera matrices
   - Frustum extraction
   - Culling pass

2. **Shadow Pass**
   - Update CSM cascades
   - Render shadow maps

3. **Main Pass**
   - G-Buffer generation (if deferred)
   - PBR lighting
   - IBL sampling

4. **Post-Processing**
   - SSAO
   - SSR
   - Bloom
   - Tone mapping
   - FXAA

5. **UI Pass**
   - ImGui rendering

### Shader Architecture

```
shaders/
├── pbr.hlsl / pbr.metal         # PBR lighting
├── shadow_map.hlsl              # Shadow generation
├── post_process.hlsl            # Post-effects
└── ssao.hlsl / ssao.metal       # SSAO compute
```

## Platform Abstraction

### RHI Layer (`engine/rhi/`)

Hardware abstraction for DX12/Metal:

```cpp
class RHIRenderer {
    virtual void beginFrame() = 0;
    virtual void submit(RHICommandBuffer*) = 0;
    virtual void present() = 0;
};
```

**Implementations:**
- `UnifiedRendererDX12` - DirectX 12 backend
- `UnifiedRendererMetal` - Metal backend

## Build System

CMake-based build with FetchContent for dependencies:

```bash
# Configure
cmake -B build -G Ninja

# Build macOS
cmake --build build --target luma_studio_macos

# Build Windows
cmake --build build --target luma_studio
```

**Dependencies:**
- Assimp (model loading)
- ImGui (editor UI)
- stb_image (texture loading)

## Testing

```bash
# Run all tests
./build/luma_tests

# Unit tests only
./build/luma_tests --unit

# Integration tests only
./build/luma_tests --integration

# Show manual test checklist
./build/luma_tests --manual
```

## Performance Considerations

1. **Culling** - Frustum culling eliminates non-visible objects
2. **LOD** - Reduced polygon count at distance
3. **Instancing** - Batched rendering for repeated objects
4. **Half-Resolution** - SSAO/SSR can render at half res
5. **Cascade Stabilization** - Prevents shadow swimming

## Extension Points

To add new features:

1. **New Component** - Add to `Entity` struct, update Inspector UI
2. **New Effect** - Create header in `engine/rendering/`, add to post-process chain
3. **New Animation Feature** - Extend `Animator` or create new system
4. **New Light Type** - Add to `LightType` enum, update shader
