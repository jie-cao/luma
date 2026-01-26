# Tutorial 04: Animation System

Learn to use LUMA's powerful skeletal animation system.

## Animation Overview

LUMA supports:
- Skeletal animation (bone-based)
- Animation blending and crossfade
- Blend trees (1D and 2D)
- Animation state machines
- Inverse Kinematics (IK)
- Animation layers with masks

## Loading Animated Models

### Supported Formats

- **FBX** (recommended) - Full animation support
- **glTF/GLB** - Modern, efficient
- **DAE** - Collada format

### Import Steps

1. Load model via Asset Browser
2. Animations are automatically extracted
3. Check Animation panel for available clips

## Playing Animations

### Basic Playback

```cpp
Animator animator;
animator.setSkeleton(&skeleton);
animator.addClip("walk", walkClip);

// Play animation
animator.play("walk");

// Update each frame
animator.update(deltaTime);
```

### In the Editor

1. Select animated entity
2. Open **Animation** panel
3. Choose clip from dropdown
4. Click **Play** or scrub timeline

### Playback Controls

| Control | Action |
|---------|--------|
| Space | Play/Pause |
| [ | Previous frame |
| ] | Next frame |
| Home | Go to start |
| End | Go to end |

## Animation Blending

### Crossfade

Smooth transition between animations:

```cpp
animator.play("walk");           // Start walking
// ... later ...
animator.play("run", 0.3f);      // Crossfade to run over 0.3 seconds
```

The second parameter is the blend duration.

### Blend Trees

#### 1D Blend Tree (Speed-based)

Blend between animations based on one parameter:

```
Speed: 0.0  →  Idle
Speed: 0.5  →  Walk  
Speed: 1.0  →  Run
```

Intermediate values blend neighboring animations.

#### 2D Blend Tree (Directional)

Blend based on two parameters (X, Y velocity):

```
        Forward
           ↑
    ↖      |      ↗
       \   |   /
Left ←─────┼─────→ Right
       /   |   \
    ↙      |      ↘
           ↓
        Backward
```

## Animation State Machine

Create complex animation logic with states and transitions.

### States

Each state plays an animation:
- Idle
- Walk
- Run
- Jump
- Attack

### Transitions

Conditions that trigger state changes:

```
Idle → Walk:    When Speed > 0.1
Walk → Run:     When Speed > 0.6
Any → Jump:     When Jump trigger is set
Jump → Idle:    When animation ends (exit time)
```

### Parameters

- **Float**: Speed, Direction
- **Bool**: IsGrounded, IsAttacking
- **Trigger**: Jump, Attack (auto-resets)

### State Machine Code

```cpp
AnimationStateMachine sm;

// Add parameters
sm.addParameter("Speed", ParameterType::Float);
sm.addParameter("Jump", ParameterType::Trigger);

// Create states
auto* idle = sm.createState("Idle");
auto* walk = sm.createState("Walk");

// Add transition
auto& trans = idle->addTransition("Walk");
trans.conditions.push_back({"Speed", ConditionMode::Greater, 0.1f});
trans.duration = 0.2f;

// Start
sm.setDefaultState("Idle");
sm.start();

// Update
sm.setFloat("Speed", 0.5f);  // Will transition to Walk
sm.update(deltaTime);
```

## Animation Layers

Play multiple animations simultaneously on different body parts.

### Use Cases

- Upper body: Shooting while walking
- Additive: Breathing animation on top of idle
- Face: Lip sync independent of body

### Layer Setup

```cpp
AnimationLayerManager layers;

// Base layer - full body
auto* base = layers.getBaseLayer();
base->name = "Locomotion";

// Upper body layer - arms only
auto* upper = layers.createLayer("UpperBody");
upper->mask.addBoneRecursive(skeleton, "spine");
upper->blendMode = AnimationBlendMode::Override;
```

### Blend Modes

- **Override**: Replace base animation
- **Additive**: Add to base animation

## Inverse Kinematics

Make bones reach target positions procedurally.

### IK Types

| Type | Use Case |
|------|----------|
| Two-Bone IK | Arms reaching, legs stepping |
| Look-At IK | Head tracking |
| Foot IK | Ground adaptation |
| FABRIK | Chain solving |

### Example: Hand Reaching

```cpp
IKManager ik;
int armIK = ik.setupArmIK(shoulderIndex, elbowIndex, handIndex);

// Set target
ik.setHandTarget(armIK, targetPosition, weight);

// Solve
ik.solve(skeleton);
```

## Timeline Editor

For precise animation control:

1. Open **Timeline** panel
2. Scrub to desired frame
3. Adjust pose manually
4. Set keyframe (K)

### Curve Types

- Linear
- Smooth (Bezier)
- Stepped (no interpolation)

## Best Practices

1. **Use crossfade** for smooth transitions (0.1-0.3s)
2. **Keep clips looping** where appropriate
3. **Use state machines** for complex logic
4. **Apply IK last** (after animation)
5. **Test blend trees** with parameter sliders

## Performance Tips

- Limit visible animated characters
- Use simpler animations for distant characters
- Consider animation LOD (fewer bones at distance)
- Cache skinning matrices

## Next: [Tutorial 05: Lighting](05_lighting.md)
