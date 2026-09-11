# XYZ Game — Stage 01C: Logen Rig Calibration and Walk Quality Design

**Date:** 2026-09-11
**Status:** Approved for implementation

## Goal

Turn the Stage 01B hierarchical Logen cutout into a calibrated, stable runtime rig. The neutral pose is the authority for scale and grounding; the eight walk poses are rebuilt around planted feet and are blended into and out of locomotion without changing the gameplay root.

Stage 01C remains a focused character-rendering pass. It does not add game systems, NPCs, dialogue, interaction triggers, quests, audio, inventory, economy, or a launcher redesign.

## Asset audit decisions

The source rig PNGs are clean alpha-isolated components from `Logen_Rig_v2_manifest.json`; their pixels remain untouched. The canonical master is used only as an optional F4 calibration overlay. The legacy full-frame idle/walk PNGs remain in the repository for reference, but the active scene does not load them.

The active neutral composition uses the back cloak, both cloak front strips, both thighs, both shins, both boots, torso, the combined waist/belt/front accessory, the empty right sleeve, the complete left arm, and the head. The standalone `Logen_rig_red_cloth_front.png` is intentionally excluded because the combined waist/belt/front component already contains the belt, potion bottles, and red hanging cloth. No right forearm or hand asset is loaded.

## Rig-space and grounding model

All node positions, pivots, animation offsets, and the fixed `root_to_ground` anchor are expressed in one unscaled source-rig coordinate space. `target_height` is authoritative. `global_scale` is an optional multiplier applied after automatic normalization; it defaults to `1.0` and is not used to hide a virtual-pixel offset.

On load, `Rig2D` evaluates the neutral rig at unit scale, records its neutral bounds, and calculates:

```text
effective_scale = target_height / neutral_rig_height * global_scale
```

The gameplay root is the player X and the fixed floor Y. The internal pelvis/root origin is derived from `root_to_ground` in rig space, never from the animated bounds:

```text
root_world = round(gameplay_root) - root_to_ground * effective_scale
```

The gameplay root X is integer-snapped. Render-only visual correction is kept separate from `PlayerController`; it is an additive X offset applied after gameplay anchoring.

Final node translations are quantized to a small 1/16 logical-pixel grid, rotations remain data-driven and small, and all textures use SDL nearest-neighbor sampling.

## Runtime architecture

`Rig2D` owns the texture-backed hierarchy, unit-space neutral calibration, target-height normalization, fixed grounding anchor, render-only root correction, debug transforms, and calibration save. `RigAnimation` parses labeled key poses and samples interpolated `RigPose` values. `RigAnimator` owns idle and walk phase clocks, preserves walk locomotion phase across animation switches, and blends pose transitions.

`FootPlantController` is a small render-side correction component. It observes boot contact anchors and walk phase, selects a support foot, keeps the selected contact point stable across the current pose segment, and smoothly transfers ownership at a phase boundary. It never changes player X, target X, velocity, or arrival logic.

`GuffmanBasementScene` remains the composition boundary. It handles click-to-move and keyboard input, selects animations, advances walk by actual displacement, calls the foot-plant correction, renders the optional master overlay, and exposes the F3/F4/F5 diagnostics. `PlayerController` is not refactored for rig concerns.

## Debug calibration controls

F3 toggles the rig diagnostics/calibration overlay. When enabled:

- `TAB` / `SHIFT+TAB` selects the next/previous rig node;
- arrow keys move the selected node in rig units, with `SHIFT` changing the step from 1 to 10;
- `Q`/`E` rotate the selected node by 1 degree, or 5 degrees with `SHIFT`;
- optional `J`/`L` and `I`/`K` adjust pivot X/Y by 0.01;
- `S` writes the current neutral calibration to `Logen_rig_definition.json`, after creating `Logen_rig_definition.backup.json`;
- F4 toggles the 30–40% alpha `Logen_Master_Right_v1.png` overlay;
- F5 pauses/unpauses the rig animation; while paused, comma/period step the walk pose backward/forward.

The overlay reports the selected node, parent, local position, pivot, base rotation, z-order, world position/rotation, target and actual neutral height, player X/velocity/target, animation state, walk phase/key pose label, support foot, visual root correction, and master overlay state.

## Animation and stop behavior

Walk data contains eight labeled poses: contact A, down A, passing A, up A, contact B, down B, passing B, and up B. The poses move the legs through an actual alternating stride while the upper body remains restrained. Foot contact metadata lives in the rig definition, not in `PlayerController`.

`RigAnimator::setAnimation` uses transition durations of 0.12 seconds for idle→walk and 0.18 seconds for walk→idle. A walk transition restores the saved locomotion phase instead of resetting to phase zero. Idle has its own time phase. Stopping first settles the current walk pose into a stable contact/neutral pose and then blends to idle within 0.25 seconds; the gameplay coordinate is unchanged.

## Verification

Automated tests cover target-height normalization, the fixed ground anchor, parent/child transforms, calibration mutation/save backup, animation interpolation, phase preservation and blending, input flags, click-to-move/manual control, legacy frame exclusion, and render-side foot correction independence. Acceptance runs the complete Debug and Release CTest suites, the game self-test, the launcher build workflow, and an interactive visual pass with F3/F4/F5 and click-to-move across the basement.
