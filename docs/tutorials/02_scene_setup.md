# Tutorial 02: Scene Setup

Learn how to build and organize scenes in LUMA Studio.

## Scene Hierarchy

### Creating Entities

```
Method 1: Menu → Edit → Create Entity
Method 2: Right-click in Hierarchy → Create Entity
Method 3: Shortcut Ctrl/Cmd + Shift + N
```

### Naming Conventions

Use descriptive, hierarchical names:

```
✓ Good:
  Character_Player
  Environment_Tree_01
  Light_Main_Sun

✗ Avoid:
  Entity1
  new_entity
  asdf
```

### Parent-Child Relationships

To parent an entity:

1. **Drag & Drop** in Hierarchy panel
2. Or select child, then in Inspector set Parent

Benefits of parenting:
- Child transforms are relative to parent
- Moving parent moves all children
- Useful for grouping (e.g., car + wheels)

```
Example Hierarchy:
├── Environment
│   ├── Ground
│   ├── Trees
│   │   ├── Tree_01
│   │   ├── Tree_02
│   │   └── Tree_03
│   └── Buildings
├── Characters
│   └── Player
└── Lights
    ├── Sun
    └── FillLight
```

## Transform System

### Position

World coordinates (X, Y, Z):
- **X** = Right
- **Y** = Up
- **Z** = Forward (into screen)

### Rotation

Euler angles in degrees:
- **Pitch** (X) = Tilt forward/back
- **Yaw** (Y) = Turn left/right
- **Roll** (Z) = Tilt sideways

### Scale

Multiplier on each axis:
- Default is (1, 1, 1)
- Negative values flip the object

### Transform Shortcuts

| Key | Mode |
|-----|------|
| W | Translate |
| E | Rotate |
| R | Scale |
| Q | Select (no gizmo) |

### Snap Settings

Hold **Ctrl/Cmd** while dragging for snapping:
- Position: Grid snap
- Rotation: 15° increments
- Scale: 0.1 increments

## Selection

### Select Methods

- **Click** in Viewport or Hierarchy
- **Ctrl/Cmd + Click** for multi-select
- **Shift + Click** for range select

### Selection Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl/Cmd + A | Select All |
| Escape | Deselect |
| Delete | Delete Selected |
| Ctrl/Cmd + D | Duplicate |
| Ctrl/Cmd + C/V | Copy/Paste |

## Common Scene Setups

### Basic Scene Template

```
Scene
├── Environment
│   └── Ground (scale: 20, 0.1, 20)
├── Props
│   └── [Your objects here]
├── Lights
│   ├── Sun (Directional)
│   └── Fill (Directional, softer)
└── Camera Targets
    └── FocusPoint
```

### Character Scene

```
Scene
├── Character
│   ├── Model (with skeleton)
│   └── Props
│       └── Weapon
├── Environment
└── Lights
```

## Saving & Loading

### Save Scene

1. File → Save Scene (Ctrl/Cmd + S)
2. Choose location
3. Enter filename (`.luma` extension)

### Load Scene

1. File → Load Scene (Ctrl/Cmd + O)
2. Browse to `.luma` file
3. Click Open

### Scene File Format

LUMA uses JSON format:

```json
{
  "version": "1.0",
  "name": "MyScene",
  "entities": [...],
  "lights": [...],
  "camera": {...},
  "postProcess": {...}
}
```

## Tips

1. **Group related objects** under empty parent entities
2. **Use consistent naming** for easier searching
3. **Save frequently** (Ctrl/Cmd + S)
4. **Use Focus (F)** to quickly navigate to objects
5. **Reset transform** by typing 0 in position fields

## Next: [Tutorial 03: Materials](03_materials.md)
