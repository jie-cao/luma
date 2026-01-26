# Tutorial 03: PBR Materials

Master the Physically Based Rendering material system.

## Understanding PBR

LUMA Studio uses the **Metallic-Roughness** workflow, the industry standard for real-time PBR.

### Key Concepts

| Property | Range | Description |
|----------|-------|-------------|
| Base Color | RGB | Surface color (albedo) |
| Metallic | 0-1 | 0 = dielectric, 1 = metal |
| Roughness | 0-1 | 0 = mirror, 1 = matte |
| AO | 0-1 | Ambient occlusion |

## Material Properties

### Base Color

The surface color without lighting effects.

**For Dielectrics (non-metals):**
- Use the actual color of the material
- Avoid pure black or white
- Typical range: 0.02 - 0.9 per channel

**For Metals:**
- This becomes the reflection color
- Gold: (1.0, 0.86, 0.57)
- Silver: (0.97, 0.96, 0.91)
- Copper: (0.98, 0.82, 0.76)

### Metallic

Determines if the surface is metal or non-metal.

```
0.0 = Plastic, wood, fabric, skin, stone
1.0 = Gold, silver, iron, aluminum

Note: In reality, there's no "half metal" - 
use 0 or 1, intermediate values only for 
transitions (rust, dirt on metal).
```

### Roughness

Controls the sharpness of reflections.

```
0.0 = Perfect mirror (polished chrome)
0.2 = Glossy plastic
0.4 = Satin finish
0.6 = Brushed metal
0.8 = Matte paint
1.0 = Chalk, unfinished wood
```

### Emissive

For glowing materials (lights, screens, lava).

- **Emissive Color**: The glow color
- **Emissive Intensity**: Brightness multiplier

## Creating Materials

### In the Inspector

1. Select an entity
2. Find the **Material** section
3. Adjust properties with sliders/color picker

### Using Presets

Click **Apply Preset** dropdown:
- Gold
- Silver
- Copper
- Plastic (customizable color)
- Rubber
- Glass

### Material Code Example

```cpp
Material mat;
mat.baseColor = Vec3(0.8f, 0.2f, 0.1f);  // Red
mat.metallic = 0.0f;                      // Non-metal
mat.roughness = 0.4f;                     // Slightly glossy
mat.ao = 1.0f;                            // No AO
```

## Common Material Recipes

### Polished Gold

```
Base Color: (1.0, 0.86, 0.57)
Metallic: 1.0
Roughness: 0.2
```

### Brushed Steel

```
Base Color: (0.6, 0.6, 0.65)
Metallic: 1.0
Roughness: 0.4
```

### Red Plastic

```
Base Color: (0.8, 0.1, 0.1)
Metallic: 0.0
Roughness: 0.4
```

### Rubber

```
Base Color: (0.1, 0.1, 0.1)
Metallic: 0.0
Roughness: 0.9
```

### Glass

```
Base Color: (1.0, 1.0, 1.0)
Metallic: 0.0
Roughness: 0.0
Alpha: 0.2
Alpha Blend: Enabled
```

### Glowing Neon

```
Base Color: (0.0, 0.0, 0.0)
Emissive Color: (0.0, 0.5, 1.0)
Emissive Intensity: 10.0
```

## Material Showcase

Load `assets/scenes/demo_materials.luma` to see a grid of spheres showing:
- X-axis: Metallic (0 → 1)
- Z-axis: Roughness (0 → 1)

## Texture Slots

(When texture support is enabled)

| Slot | Purpose |
|------|---------|
| Albedo | Base color texture |
| Normal | Surface detail normals |
| Metallic | Metallic map (grayscale) |
| Roughness | Roughness map (grayscale) |
| AO | Ambient occlusion map |
| Emissive | Emission color/intensity |

## Tips

1. **Metallic is binary** - use 0 or 1, not in between
2. **Roughness has the most visual impact** - experiment!
3. **Avoid pure black/white** for base colors
4. **Use reference photos** to match real materials
5. **Load demo_materials.luma** to compare settings

## Reference Values

| Material | Base Color | Metallic | Roughness |
|----------|------------|----------|-----------|
| Gold | (1.0, 0.86, 0.57) | 1.0 | 0.3 |
| Silver | (0.97, 0.96, 0.91) | 1.0 | 0.2 |
| Copper | (0.98, 0.82, 0.76) | 1.0 | 0.3 |
| Iron | (0.56, 0.57, 0.58) | 1.0 | 0.5 |
| Plastic | (any) | 0.0 | 0.4 |
| Rubber | (any) | 0.0 | 0.8 |
| Wood | (0.5, 0.3, 0.15) | 0.0 | 0.7 |
| Concrete | (0.5, 0.5, 0.5) | 0.0 | 0.9 |
| Skin | (0.8, 0.6, 0.5) | 0.0 | 0.5 |

## Next: [Tutorial 04: Animation](04_animation.md)
