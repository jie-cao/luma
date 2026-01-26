# Tutorial 05: Lighting

Create atmospheric scenes with LUMA's lighting system.

## Light Types

### Directional Light

Simulates distant light sources (sun, moon).

```
Properties:
- Direction (where light points)
- Color
- Intensity
- Casts parallel shadows

Best for: Outdoor scenes, main light source
```

### Point Light

Emits light in all directions from a point.

```
Properties:
- Position
- Color
- Intensity
- Attenuation (falloff)

Best for: Lamps, candles, explosions
```

### Spot Light

Cone-shaped light with direction.

```
Properties:
- Position
- Direction
- Inner/Outer cone angles
- Color, Intensity
- Attenuation

Best for: Flashlights, stage lights, car headlights
```

## Adding Lights

### Via Inspector

1. Select entity
2. Click **Add Component** → **Light**
3. Choose light type
4. Adjust properties

### Via Code

```cpp
Entity* light = scene.createEntity("MyLight");
light->hasLight = true;
light->light = Light::createPoint();
light->light.color = Vec3(1.0f, 0.9f, 0.8f);
light->light.intensity = 100.0f;
light->localTransform.position = Vec3(0, 5, 0);
```

## Light Properties

### Color

RGB color of the light:
- Warm (orange): (1.0, 0.8, 0.6)
- Cool (blue): (0.8, 0.9, 1.0)
- Neutral: (1.0, 1.0, 1.0)

### Intensity

Brightness level:
- Directional: 1-10 (arbitrary units)
- Point/Spot: 10-1000 (roughly lumens-like)

### Attenuation (Point/Spot)

How light fades with distance:

```
Attenuation = 1 / (constant + linear*d + quadratic*d²)

Typical values:
- Constant: 1.0
- Linear: 0.09
- Quadratic: 0.032

Distance ~7m before 90% fade
```

## Color Temperature

Convert Kelvin to RGB:

| Kelvin | Color | Use |
|--------|-------|-----|
| 1850K | Orange-red | Candle |
| 2700K | Warm white | Incandescent |
| 3500K | Neutral | Office light |
| 5500K | Daylight | Noon sun |
| 6500K | Cool | Overcast |
| 10000K | Blue | Clear sky |

## Three-Point Lighting

Classic cinematography setup:

### Key Light (Main)

- Strongest light
- 45° to side, 45° above
- Creates main shadows
- Usually directional

### Fill Light

- Softer, less intense
- Opposite side from key
- Fills in shadows
- About 50% of key intensity

### Rim/Back Light

- Behind subject
- Creates edge highlight
- Separates subject from background
- Often point or spot light

```
       Fill            Key
         \              /
          \    Subject /
           \    ●     /
            \   |   /
             \  |  /
              \ | /
               \|/
        Camera ◎
```

## Shadow Settings

### Directional Light Shadows

- Shadow Map Resolution: 1024-4096
- Cascaded Shadow Maps for distance
- Bias to prevent shadow acne

### Point/Spot Shadows

- Shadow cubemap or 2D map
- Adjust near/far planes
- Higher resolution = sharper shadows

## Demo Scene

Load `assets/scenes/demo_lighting.luma` to see:
- Directional sun light
- Colored point lights (RGB)
- Spotlight from above
- Multiple light interaction

## Lighting Scenarios

### Outdoor Day

```
Sun (Directional):
  - Direction: (-0.3, -1, -0.5)
  - Color: (1.0, 0.95, 0.9)
  - Intensity: 3-5

Sky Fill (Directional):
  - Direction: (0, -1, 0)
  - Color: (0.5, 0.7, 1.0)
  - Intensity: 0.5
```

### Indoor

```
Ceiling Light (Point):
  - Position: Room center, near ceiling
  - Color: (1.0, 0.95, 0.9)
  - Intensity: 100-200

Window Light (Directional):
  - Through window direction
  - Blue-ish color
  - Cast shadows
```

### Night/Horror

```
Single Point Light:
  - Low intensity (20-50)
  - Warm color
  - Strong shadows

Rim Lights:
  - Cool blue/green
  - Behind characters
```

## Performance Tips

1. **Limit light count** - Each light adds cost
2. **Use shadow distance** - Don't shadow far objects
3. **Bake static lights** - If objects don't move
4. **Lower shadow resolution** - For less important lights

## IBL (Image-Based Lighting)

For realistic ambient lighting:

1. Load HDR environment map
2. System generates:
   - Irradiance map (diffuse)
   - Prefiltered map (specular)
   - BRDF LUT

This provides ambient light from all directions based on the environment.

## Next: [Tutorial 06: Optimization](06_optimization.md)
