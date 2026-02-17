# MetaHuman-Style Avatar Product Technical Solution

## Product Goal

Build an engine-agnostic avatar head system that supports:

- Photo-based identity initialization
- Standard topology full-head output (including back head, ears, neck seam)
- Fine-tuning via face sculpt sliders
- Production-ready facial animation (ARKit/FACS compatible)
- Stable export pipeline for DCC and game runtime

## Core Product Principles

1. **Single fixed topology**
   - One canonical full-head mesh topology for all characters.
   - No per-character topology mutation.
2. **Two-layer parameter architecture**
   - Identity parameters: long-term character appearance.
   - Expression parameters: animation-time facial motion.
3. **Animation standard compatibility**
   - Runtime channels follow ARKit 52 naming for ecosystem compatibility.
4. **Separation of concerns**
   - Photo estimation gives initial identity values.
   - Artist/gameplay can always override through sliders and presets.

## Runtime Architecture

```mermaid
flowchart LR
    photo[PhotoInput] --> detect[FaceDetectAndLandmarks]
    detect --> estimate[IdentityEstimate]
    estimate --> identity[FaceShapeParams]
    identity --> mesh[StandardHeadMeshDeformer]
    mesh --> blend[BlendShapeChannels]
    blend --> arkit[ARKit52CompatibilityLayer]
    arkit --> anim[FacialRigAnimationRuntime]
```

## Implementation in Current Codebase

- `engine/character/face_template_mesh.h`
  - Generates semantic sculpt channels on standard head topology.
  - Adds ARKit compatibility channels by remapping semantic channels.
- `engine/character/character_face.h`
  - Holds identity (`FaceShapeParams`) and expression (`FaceExpressionParams`).
  - Applies shape and expression parameters to `BlendShapeMesh`.
- `engine/character/facial_rig.h`
  - Defines ARKit 52 channel names and expression preset framework.
- `engine/character/ai/face_reconstruction.h`
  - Provides photo-to-parameter estimation pipeline.

## Roadmap

### Phase 1 (Current, functional baseline)

- Standard topology head generation and loading
- Identity sculpt channels
- ARKit-compatible expression channel layer
- Photo initialization + manual sculpt refinement

### Phase 2 (Production stabilization)

- Corrective blendshapes for key channel combinations
  - Example: `jawOpen + mouthClose`, `eyeBlink + eyeLookUp`
- ARKit behavior validation suite
- DCC export consistency checks

### Phase 3 (Self-owned DNA-like system)

- Build identity basis (`mean_head + B_identity`) from aligned same-topology dataset
- Build expression basis and corrective rule set
- Train/optimize robust photo-to-identity estimator against own basis

## Acceptance Criteria

1. Same topology for all generated heads.
2. ARKit channel set can drive visible facial motion end-to-end.
3. Sculpt sliders support bidirectional adjustment around neutral.
4. Photo initialization + manual refinement produces reusable character assets.
5. Exported assets remain consistent between editor and runtime.

