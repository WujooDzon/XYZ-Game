# XYZ Game — Stage 01B: Logen Cutout Rig Design

**Date:** 2026-09-10  
**Status:** Approved for implementation

## Goal

Replace the active frame-based Logen renderer with a small hierarchical 2D cutout rig. The rig must assemble the supplied parts into a coherent neutral pose, play a real eight-pose walk cycle driven by distance travelled, provide subtle rig idle motion, and remain compatible with the existing SDL3 game and XYZ DEV launcher workflow.

## Scope

Included:

- a reusable, lightweight runtime rig in the existing C++ engine;
- strict JSON loading for rig definitions and animation keyframes;
- the supplied Logen rig PNGs, loaded without modifying their pixels;
- a data-driven neutral pose, eight distinct walk poses, and subtle idle poses;
- pivot-aware rotation, scale, z-order, parent/child transforms, and complete-rig horizontal mirroring;
- movement acceleration/deceleration and walk phase derived from actual displacement;
- F3 rig diagnostics and R data reload while diagnostics are enabled;
- automated unit/integration coverage and Debug/Release acceptance checks.

Explicitly excluded:

- dialogue, NPCs, interaction triggering, quests, economy, inventory, combat, saves, audio, ECS, and third-party skeletal animation libraries;
- changes to the basement artwork, launcher appearance, or existing build workflow;
- deletion or modification of the legacy frame PNGs.

## Asset location decision

The repository already contains the exact Stage 01B PNGs in `Assets/Characters/Logen/`, while the prompt names a `Rig/` subdirectory. The PNGs remain in their existing location and are never rewritten or moved. New data files live in `Assets/Characters/Logen/Rig/` and reference the existing images with paths such as `../Logen_rig_torso_upper.png`. This preserves the supplied assets and keeps all tuning data grouped under `Rig/`.

## Runtime architecture

### JSON

Add a small dependency-free JSON reader to the engine. It supports the types required by the checked-in data: null, boolean, number, string, array, and object. It reports a source path and byte offset for malformed input. Rig loading rejects missing fields, duplicate node ids, unknown parents, cycles, invalid normalized pivots, and non-finite numeric values.

### Rig model

`Rig2D` owns an ordered collection of `RigNode` values. A node contains:

- stable string id;
- parent index and child indices;
- optional owned `Texture`;
- local position in virtual pixels;
- normalized pivot in the inclusive range 0..1;
- base rotation in degrees;
- local scale;
- integer z-order.

The root node is a textureless `pelvis` node. A node's local position identifies its pivot relative to its parent origin. Runtime pose values are additive local position/rotation offsets and multiplicative local scale values. World transforms are evaluated from the root and rendered in ascending z-order, so hierarchy controls articulation while z-order controls overlap.

`Renderer2D` gains a pivot-aware rotated texture call. It keeps the existing simple draw overload for the rest of the engine. Rig rendering supplies the correct post-mirror pivot, destination rectangle, rotation, and nearest-neighbor texture filtering. The root world X is rounded to an integer virtual pixel before evaluating the hierarchy; the root Y is the player baseline plus the definition's root offset.

### Data files

`Logen_rig_definition.json` contains `global_scale`, `target_height`, `root_offset`, and the complete node list. Each image path is resolved relative to the definition file. The definition contains the empty right sleeve node but no right hand or right forearm node.

`Logen_walk.json` contains `name`, `loop`, `stride_distance`, and eight keyframes at normalized phases 0.0 through 0.875. Each keyframe can specify node `rotation`, additive `position`, and multiplicative `scale` values. The eight poses are contact A, down A, passing A, up A, contact B, down B, passing B, and up B. Linear interpolation is used between keyframes, including the wrapped final-to-first segment.

`Logen_idle.json` contains a two-to-three-second looping animation with only very small torso, head, left-arm, empty-sleeve, and cloak motion. Its neutral first pose is the stop pose.

### Animator and movement

`RigAnimator` stores the active animation and normalized phase. Walk updates call `advanceByDistance(abs(playerDeltaX))`, dividing by the configured stride distance. Idle updates call `advanceByTime(deltaSeconds)`. Therefore a slow approach naturally advances more slowly and a stop cannot leave the player frozen in an arbitrary walk pose; the scene resets to the neutral idle pose on the walk-to-idle transition.

`PlayerController` retains the generic X target API for future interaction range use and adds velocity reporting. Click targets use the existing logical floor band and are clamped to room bounds. The controller uses approximately 125 ms acceleration and 150 ms braking/deceleration, with a short target ease-out. Keyboard input cancels an active click target and remains the secondary/debug control.

### Facing and diagnostics

The complete canonical right-facing hierarchy is mirrored around the snapped root for left-facing movement. The mirrored normalized pivot is used when calling SDL so articulated anchors remain stable. The implementation includes the comment `TODO: dedicated left-facing Logen rig required before final production.` because mirroring an anatomically asymmetric character is a prototype solution.

When F3 is enabled, the scene draws the root, parent-child joint lines, pivots, node names, transformed part bounds, animation name, normalized phase, velocity, target X, and composed character height. R reloads the definition and both animation files only while F3 is enabled; a failed reload leaves the last valid rig active and reports the error in the debug output.

## Scene integration

`GuffmanBasementScene` continues to own the background, player controller, and fixed camera. It replaces `idleFrames_`, `walkFrames_`, and the old `Animation` instances with one `Rig2D`, one idle `RigAnimation`, one walk `RigAnimation`, and one active `RigAnimator`. The old generated frame assets stay in the repository but are not loaded or referenced by active gameplay code.

The existing click-to-move and keyboard behavior is preserved. The scene passes the player's actual displacement to the walk animator, anchors the rig to the existing gameplay baseline, and keeps the target 188 virtual-pixel assembled height separate from per-part scales.

## Verification strategy

- JSON tests cover valid values, malformed input, and diagnostics.
- Rig tests cover hierarchy evaluation, parent rotation propagation, pivot validation, z-order, mirrored anchors, and the missing-right-arm node set.
- Animation tests cover eight keyframes, wrapped interpolation, looping, idle timing, and distance-driven phase.
- Player tests cover acceleration, target braking, target arrival, bounds, keyboard override, and reported velocity.
- Scene tests initialize the real supplied assets in a hidden SDL window, confirm the rig definition loads, confirm the composed height is near 188, exercise click-to-move and keyboard movement, and verify idle/walk transitions and left mirroring state.
- Acceptance scripts build and test Debug and Release, run the asset/config self-test, verify the legacy walk frames are not active references, and retain launcher Build & Run checks.
