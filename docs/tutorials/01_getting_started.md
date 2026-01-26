# Getting Started with LUMA Studio

Welcome to LUMA Studio! This tutorial will help you get up and running quickly.

## Building LUMA Studio

### Prerequisites

- CMake 3.25+
- Ninja (recommended) or Make
- C++17 compatible compiler
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studio 2019+

### macOS Build

```bash
# Clone the repository
git clone https://github.com/your-repo/luma.git
cd luma

# Configure
cmake -B build -G Ninja

# Build
cmake --build build --target luma_studio_macos

# Run
./build/luma_studio_macos.app/Contents/MacOS/luma_studio_macos
```

### Windows Build

```bash
cmake -B build -G Ninja
cmake --build build --target luma_studio
./build/luma_studio.exe
```

## User Interface Overview

When you launch LUMA Studio, you'll see:

```
┌─────────────────────────────────────────────────────────────┐
│  Menu Bar (File, Edit, View, Help)                          │
├────────────┬────────────────────────────┬───────────────────┤
│            │                            │                   │
│ Hierarchy  │       3D Viewport          │    Inspector      │
│   Panel    │                            │      Panel        │
│            │                            │                   │
│            │                            │                   │
├────────────┴────────────────────────────┴───────────────────┤
│  Asset Browser / Timeline / Console                          │
└─────────────────────────────────────────────────────────────┘
```

### Panels

- **Hierarchy** - Scene tree showing all entities
- **Viewport** - 3D view for visualization and interaction
- **Inspector** - Properties of selected entity
- **Asset Browser** - File navigation and model loading
- **Timeline** - Animation playback controls

## Camera Controls

| Action | Input |
|--------|-------|
| Orbit | Alt + Left Mouse |
| Pan | Alt + Middle Mouse |
| Zoom | Alt + Right Mouse / Scroll |
| Focus on Selection | F |
| Reset Camera | Home |

## Your First Scene

### 1. Create an Entity

- **Menu**: Edit → Create Entity
- **Shortcut**: Ctrl/Cmd + Shift + N

### 2. Transform the Entity

Select the entity in the Hierarchy, then:

- **W** - Translate mode
- **E** - Rotate mode
- **R** - Scale mode

Drag the gizmo handles in the viewport.

### 3. Add a Material

In the Inspector panel:

1. Expand the **Material** section
2. Set **Base Color** (click the color picker)
3. Adjust **Metallic** slider (0 = plastic, 1 = metal)
4. Adjust **Roughness** slider (0 = mirror, 1 = matte)

### 4. Add a Light

1. Create a new entity
2. In Inspector, click **Add Component** → **Light**
3. Choose light type: Directional, Point, or Spot
4. Adjust color and intensity

### 5. Save Your Scene

- **Menu**: File → Save Scene
- **Shortcut**: Ctrl/Cmd + S

## Loading Models

### Supported Formats

- FBX (recommended for animated models)
- glTF / GLB
- OBJ
- DAE (Collada)
- 3DS
- STL
- PLY

### How to Load

1. Open **Asset Browser** panel
2. Navigate to your model file
3. **Double-click** or drag into viewport

## Running Demo Scenes

Pre-built demo scenes are available in `assets/scenes/`:

- `demo_basic.luma` - Simple scene setup
- `demo_materials.luma` - PBR material showcase
- `demo_lighting.luma` - Multi-light configuration

Load via **File → Load Scene**.

## Next Steps

- [Tutorial 02: Scene Setup](02_scene_setup.md)
- [Tutorial 03: Materials](03_materials.md)
- [Tutorial 04: Animation](04_animation.md)
- [Tutorial 05: Lighting](05_lighting.md)
- [Tutorial 06: Optimization](06_optimization.md)

## Getting Help

- Check the [API Reference](../API_REFERENCE.md)
- See [Architecture Overview](../ARCHITECTURE.md)
- Run tests: `./build/luma_tests --help`
