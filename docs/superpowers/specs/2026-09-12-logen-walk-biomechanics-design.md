# XYZ Game — Logen Walk Biomechanics Design

**Date:** 2026-09-12  
**Status:** Approved by the task request for implementation

## Goal

Correct the existing Logen Rig V3 walk so the rendered character transfers weight between near and far legs instead of reading as a horizontally translated puppet. Keep the existing hierarchy, assets, renderer, SDL3 runtime, click-to-move, keyboard fallback, launcher workflow, target-height normalization, and debug controls.

This is a corrective Stage 01D pass. It does not create Rig V4, regenerate PNGs, reintroduce frame animation, or add gameplay systems.

## Root causes found in the current baseline

- `FootPlantController` exposes `Left`/`Right`, selects support only by `phase < 0.5`, and receives near/far contacts in that misleading order.
- Support transitions capture the old support foot, so the newly planted foot inherits the previous foot's world anchor.
- Walk pose pelvis offsets are authored as rig-space node positions and become sub-pixel after target-height normalization.
- The current definition separates thigh origins at `-58` and `+48` rig units, which produces a front-facing straddle in a side profile.
- Passing poses rotate both thighs in the same general direction, and the body shell/cape rotate too aggressively for the 188 px display scale.
- There is no diagnostic toggle for foot planting and no repeatable V3 pose export mode.

## Runtime design

### Logical animation root offset

`RigPose` gains `rootOffsetLogical`, expressed directly in 960×540 logical pixels. `RigAnimation` optionally reads each keyframe's `root_offset_px` and linearly interpolates it, including the wrapped final-to-first segment and animator transition blending. `Rig2D` evaluates the root as:

```text
gameplayRoot
+ rootOffsetLogical
+ render-only foot-plant correction
- normalized root_to_ground anchor
```

The offset is applied before child hierarchy evaluation and never changes `PlayerController` position, target, velocity, or arrival state.

### Near/far foot semantics

`FootPlantController::SupportFoot` becomes `None`, `Near`, `Far`. `None` represents transfer while walking; the scene labels it `TRANSFER`, and labels it `NONE` while idle. The controller samples support windows:

```text
[0.00, 0.36) Far
[0.36, 0.50) None / transfer
[0.50, 0.86) Near
[0.86, 1.00) None / transfer
```

When entering a support window, the planted world coordinate is captured from the current new support contact plus the current correction. During support the desired correction is the difference between that anchor and the current support contact, clamped to 12 logical pixels. During transfer the desired correction returns to zero over 0.08 seconds. The correction is render-only.

### Rig calibration and walk data

The existing V3 definition remains the only active rig. Both thigh origins are moved near the pelvis center and are tuned against the actual effective scale until final rendered hip separation is 3–5 logical pixels. Knee and ankle child positions/pivots are adjusted only as needed to keep overlaps connected through frozen poses.

The eight walk keyframes retain their labels and phases. Contact A starts with far forward/near rear; Contact B is the opposite. Passing A has far support under the pelvis and a bent airborne near leg; Passing B mirrors that relationship. Body shell rotation is zero unless a measured pixel-stable value under 0.35° is demonstrably better. Cloak motion stays at or below 0.5° and never hides a swing foot. Stride distance is tuned in the 55–80 logical-pixel range against visible planted-foot travel.

### F6 and F7 developer tools

F6 toggles render-only foot-plant correction, defaulting to ON. F3's overlay shows support state, current near/far foot world X, planted anchor, desired correction, and applied correction. F5 keeps pause and pose stepping, but prints the numbered eight-pose label and near/far foot Y relative to the baseline.

F7 invokes a small `RigReviewExporter` dev utility. It renders the existing V3 rig at its real 188 px height on a neutral 960×540 target, with no joint clutter, and writes `Build/RigReview/00_idle.png` through `08_up_b.png` plus `Logen_walk_contact_sheet.png`. The exporter uses the existing SDL renderer and SDL_image PNG writer; it does not change release gameplay behavior or assets.

## Testing strategy

Add focused regression tests before implementation:

- parse and interpolate `root_offset_px`;
- apply logical root offsets before child hierarchy evaluation;
- select Far/Near/transfer windows;
- capture a new support anchor from the new foot, not the old foot;
- validate the eight V3 labels, logical bob values, exact hierarchy, no right arm, hip spacing, and passing-pose invariants;
- map F6/F7 input flags;
- export all nine review images and verify their dimensions/files.

Run the existing launcher and game tests, both Debug and Release CMake builds, direct self-tests, the actual game with F3/F4/F5/F6 and click-to-move, and F7 review output before committing.

## Explicit non-goals

No new rig version, PNG regeneration, renderer rewrite, player-controller rewrite, ECS, third-party animation library, dialogue, NPCs, interaction system, quests, economy, combat, inventory, save system, audio, or Stage 02 work.
