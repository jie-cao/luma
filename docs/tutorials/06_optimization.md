# Tutorial 06: Performance Optimization

Achieve smooth framerates with LUMA's optimization features.

## Performance Panel

Open **View → Performance Stats** to see:
- FPS / Frame time
- Draw calls
- Triangle count
- Culled objects
- Memory usage

## Frustum Culling

Automatically enabled. Objects outside the camera view are not rendered.

### How It Works

1. Each object has a bounding volume (sphere/box)
2. Camera view creates a frustum (pyramid)
3. Objects outside frustum are skipped

### Best Practices

- Ensure bounding volumes are tight
- Split large objects into smaller pieces
- Group small objects under parents

## Level of Detail (LOD)

Reduce detail for distant objects.

### Setting Up LOD

1. Create mesh variants (high, medium, low)
2. Assign to LOD group
3. Set distance thresholds

```cpp
LODGroup tree;
tree.levels.push_back({0.0f, highMesh, 10000});    // Close
tree.levels.push_back({0.1f, mediumMesh, 2500});   // Medium
tree.levels.push_back({0.05f, lowMesh, 500});      // Far
tree.levels.push_back({0.02f, billboard, 2});      // Very far
```

### LOD Guidelines

| Distance | Triangle Budget |
|----------|-----------------|
| Close | Full detail |
| Medium | 25-50% |
| Far | 5-10% |
| Very far | Billboard (2 tris) |

## GPU Instancing

Render many copies of the same mesh in one draw call.

### When to Use

- Grass, trees, rocks
- Particles
- Crowds
- Any repeated geometry

### How It Works

```cpp
// Without instancing: 1000 draw calls
for (each grass blade) {
    draw(grassMesh);
}

// With instancing: 1 draw call
draw_instanced(grassMesh, 1000 instances);
```

### Requirements

- Same mesh
- Same material
- Different transforms

## Render Optimizer

Coordinates all optimization systems.

### Configuration

```cpp
RenderOptimizer& optimizer = getRenderOptimizer();
optimizer.config.enableFrustumCulling = true;
optimizer.config.enableLOD = true;
optimizer.config.enableInstancing = true;
optimizer.config.sortMode = RenderSortMode::FrontToBack;
```

### Sort Modes

- **FrontToBack**: For opaque objects (reduces overdraw)
- **BackToFront**: For transparent objects
- **ByMaterial**: Minimize state changes

## Shadow Optimization

### Cascaded Shadow Maps

Split shadow map by distance:

| Cascade | Distance | Resolution |
|---------|----------|------------|
| 0 | 0-10m | Full |
| 1 | 10-30m | Full |
| 2 | 30-100m | Half |
| 3 | 100-300m | Quarter |

### Shadow Distance

Limit how far shadows render:

```cpp
shadowSettings.maxDistance = 100.0f;  // No shadows beyond 100m
```

## Post-Processing Optimization

### SSAO

```cpp
// Half resolution for 4x performance
ssao.settings.halfResolution = true;

// Reduce sample count
ssao.settings.sampleCount = 16;  // Default 32
```

### Bloom

```cpp
// Fewer blur iterations
bloom.iterations = 4;  // Default 6
```

### SSR

```cpp
// Lower ray march steps
ssr.maxSteps = 32;  // Default 64

// Half resolution
ssr.halfResolution = true;
```

## Memory Optimization

### Texture Guidelines

| Type | Format | Size |
|------|--------|------|
| Albedo | BC7 | 1024-2048 |
| Normal | BC5 | 1024-2048 |
| Roughness/Metallic | BC4 | 512-1024 |

### Mesh Guidelines

- Remove unseen faces
- Merge small meshes
- Use appropriate LODs
- Limit bone count for characters

## Profiling

### Frame Timing

Target frame times:
- 60 FPS = 16.67ms
- 30 FPS = 33.33ms

### Bottleneck Identification

| Symptom | Likely Cause |
|---------|--------------|
| High draw calls | Need instancing/batching |
| High triangle count | Need LOD |
| GPU bound | Reduce shader complexity |
| CPU bound | Reduce update logic |

## Stress Testing

Load `assets/scenes/demo_stress.luma` or run:

```cpp
// Create performance test scene
examples::example_StressTestScene(scene);
```

This creates 2500 objects to test:
- Culling efficiency
- LOD transitions
- Instancing benefits

## Benchmark Results

Run `./build/luma_tests --benchmark`:

```
Typical results:
- Mat4 Multiply: ~15ns
- Quat Slerp: ~25ns
- Entity Creation: ~5μs
- Frustum Check: ~10ns
```

## Optimization Checklist

### Scene Setup
- [ ] Enable frustum culling
- [ ] Set up LOD for complex meshes
- [ ] Use instancing for repeated objects
- [ ] Group static objects

### Shadows
- [ ] Limit shadow distance
- [ ] Use cascaded shadows
- [ ] Reduce far cascade resolution

### Post-Processing
- [ ] Use half-resolution SSAO/SSR
- [ ] Limit bloom iterations
- [ ] Disable unused effects

### Materials
- [ ] Compress textures
- [ ] Reduce texture resolution
- [ ] Combine textures where possible

### Animation
- [ ] LOD for distant characters
- [ ] Reduce bone counts
- [ ] Update less frequently at distance

## Performance Targets

| Platform | Target FPS | Notes |
|----------|------------|-------|
| Desktop | 60 | High settings |
| Laptop | 30-60 | Medium settings |
| Integrated GPU | 30 | Low settings |

## Summary

1. **Cull** what you can't see
2. **LOD** for distance
3. **Instance** repetition
4. **Profile** before optimizing
5. **Balance** quality vs performance
