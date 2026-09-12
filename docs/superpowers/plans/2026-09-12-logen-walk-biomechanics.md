# Logen Walk Biomechanics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Correct the existing Rig V3 walk biomechanics, render-only foot planting, logical body-weight bob, and developer review tooling without changing the rig version or gameplay architecture.

**Architecture:** Extend the existing `RigPose`/`RigAnimation` data path with a logical root offset, keep `Rig2D` responsible for root evaluation, and make `FootPlantController` an explicit near/far render-only support state machine. Keep `GuffmanBasementScene` as the composition boundary and isolate F7 PNG capture in a small `RigReviewExporter` that uses the existing SDL renderer and V3 hierarchy.

**Tech Stack:** C++20, CMake, SDL3, SDL3_image, existing V3 transparent PNG assets, nearest-neighbor renderer, macOS Apple Silicon.

**Spec:** `docs/superpowers/specs/2026-09-12-logen-walk-biomechanics-design.md`

## Global Constraints

- Do not create Rig V4 or another rig system.
- Do not regenerate or replace the existing V3 PNG assets.
- Preserve target-height normalization, the pelvis hierarchy, nearest-neighbor rendering, distance-driven locomotion, click-to-move, A/D and arrow controls, F3/F4/F5, and launcher Build & Run.
- Keep `PlayerController` X authoritative; foot planting is render-only.
- Keep the missing right arm missing; do not add right-hand/right-forearm nodes or art.
- Keep the current SDL3/CMake architecture; do not add ECS, third-party animation libraries, or gameplay systems.
- Production code changes require a focused failing test before implementation.

---

### Task 1: Add failing biomechanics contracts

**Files:**
- Modify: `Engine/Tests/RigAnimationTests.cpp`
- Modify: `Engine/Tests/Rig2DTests.cpp`
- Modify: `Engine/Tests/InputTests.cpp`
- Modify: `Game/Tests/FootPlantControllerTests.cpp`
- Modify: `Game/Tests/GuffmanBasementSceneTests.cpp`
- Modify: `Game/Tests/LogenRigV3AssetTests.cpp`
- Modify: `Game/CMakeLists.txt`
- Create: `Game/Tests/RigReviewExporterTests.cpp`

**Interfaces:**
- Consumes: current V3 JSON, current `RigAnimation`, `Rig2D`, `FootPlantController`, and SDL input contracts.
- Produces: executable tests that fail for the missing root offsets, old support semantics, missing F6/F7 flags, incorrect current pose data, and missing exporter.

- [x] **Step 1: Write the failing root-offset assertions.**

  Add a temporary animation fixture with `root_offset_px: [0, 2]` and `root_offset_px: [0, -1]`; assert `RigAnimation::sample()` returns the exact keyframe value and interpolates the midpoint. Add a `Rig2D` fixture assertion that `RigPose.rootOffsetLogical = {4, -2}` moves the root and child by exactly `{4, -2}` after normalization, without changing the local child offset.

- [x] **Step 2: Write the failing near/far controller assertions.**

  Assert phase `0.10` returns `SupportFoot::Far`, phase `0.60` returns `SupportFoot::Near`, and phases `0.40` and `0.90` return `SupportFoot::None`. Use a Far-to-Near transition where the current near contact differs from the old far contact and assert the planted anchor is captured from near. Repeat Near-to-Far with a different far contact.

- [x] **Step 3: Write the failing input/scene assertions.**

  Add F6/F7 one-frame input assertions. Extend the scene integration test to toggle F6, check the new support labels/state accessors, and confirm paused pose labels remain the exact eight-pose sequence.

- [x] **Step 4: Write the failing V3 data invariants.**

  Require the eight exact labels, all eight `root_offset_px` values, body shell rotation bounds, cloak rotation bounds, passing-pose swing/support ordering, final hip separation at the loaded effective scale, and no right-arm identifiers. Keep existing exact-ten-PNG and hierarchy tests.

- [x] **Step 5: Write the failing exporter test.**

  Create a hidden 960×540 SDL window, load the real V3 rig and idle/walk data, run the exporter into a temporary directory, and assert all ten PNGs exist and are 960×540. The test must fail until the exporter and target rendering exist.

- [x] **Step 6: Run the focused tests to verify RED.**

  Run:

  ```bash
  cmake -S . -B Build/Stage01DRed -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
  cmake --build Build/Stage01DRed --target XYZRigAnimationTests XYZRig2DTests XYZInputTests XYZFootPlantControllerTests XYZGuffmanBasementSceneTests XYZLogenRigV3AssetTests XYZRigReviewExporterTests
  ctest --test-dir Build/Stage01DRed --output-on-failure
  ```

  Expected: failures identify missing logical root offsets, `SupportFoot::Near/Far`, F6/F7, corrected JSON invariants, or the missing exporter—not fixture syntax errors.

### Task 2: Implement logical root offsets

**Files:**
- Modify: `Engine/include/XYZ/Engine/Rig2D.h`
- Modify: `Engine/src/Rig2D.cpp`
- Modify: `Engine/src/RigAnimation.cpp`

**Interfaces:**
- Consumes: existing `RigPoseTransform` interpolation and root hierarchy evaluation.
- Produces: `RigPose::rootOffsetLogical`, optional `root_offset_px` loading, interpolation, transition blending, and root evaluation before children.

- [x] **Step 1: Add the smallest data field.**

  Add `SDL_FPoint rootOffsetLogical{0.0F, 0.0F};` to `RigPose`. Keep all existing node transform defaults unchanged.

- [x] **Step 2: Parse optional keyframe offsets.**

  In `RigAnimation::load`, default the offset to zero and, when present, parse `root_offset_px` with the existing finite two-number point validation. Do not make old animation files invalid.

- [x] **Step 3: Interpolate and blend offsets.**

  In `RigAnimation::sample`, linearly interpolate `start->pose.rootOffsetLogical` and `end->pose.rootOffsetLogical` using the same wrapped span as node transforms. In `blendPoses`, interpolate the root field along with the node map.

- [x] **Step 4: Apply the offset at the root.**

  In `Rig2D::evaluateWorldNodes`, add `pose.rootOffsetLogical` to the root position before calling `evaluateNode`. Keep `visualRootCorrectionX_` separate and do not multiply the logical offset by `effectiveScale_`.

- [x] **Step 5: Run the focused green cycle.**

  Run the two engine tests from Task 1, then the complete CTest suite. Expected: root-offset tests pass and existing rig normalization/mirroring tests remain green.

### Task 3: Replace left/right planting with near/far support windows

**Files:**
- Modify: `Game/include/XYZ/Game/FootPlantController.h`
- Modify: `Game/src/FootPlantController.cpp`

**Interfaces:**
- Consumes: near/far boot X contacts, normalized walk phase, current correction, and walking state.
- Produces: `SupportFoot { None, Near, Far }`, debug state fields for near/far contacts, planted anchor, desired correction, and applied correction.

- [x] **Step 1: Rename the public semantics and expand state.**

  Replace `Left`/`Right` with `Near`/`Far`; rename update parameters to `nearFootX` and `farFootX`. Add state fields `nearFootWorldX`, `farFootWorldX`, `plantedWorldX`, and `desiredCorrectionX` while retaining `rootCorrectionX`.

- [x] **Step 2: Implement the phase state machine.**

  Use `<0.36` Far, `<0.50` None, `<0.86` Near, otherwise None. On a transition into Near or Far, set `plantedWorldX_ = currentNewFootX + rootCorrectionX_`. On transfer, clear support ownership and target correction zero. Never derive the new anchor from the old support foot.

- [x] **Step 3: Apply bounded smoothing.**

  Use `kMaximumCorrection = 12.0F` and `kTransferSeconds = 0.08F`. Clamp active desired correction and approach it with the existing helper. Keep reset behavior when not walking.

- [x] **Step 4: Run controller tests to verify GREEN.**

  Run `cmake --build Build/Stage01DRed --target XYZFootPlantControllerTests && ctest --test-dir Build/Stage01DRed -R XYZFootPlantControllerTests --output-on-failure`.

### Task 4: Author biomechanically readable V3 data

**Files:**
- Modify: `Assets/Characters/Logen/RigV3/Logen_rig_v3_definition.json`
- Modify: `Assets/Characters/Logen/RigV3/Logen_walk_v3.json`
- Modify: `Assets/Characters/Logen/RigV3/Logen_idle_v3.json`
- Modify: `Game/Tests/LogenRigV3AssetTests.cpp`

**Interfaces:**
- Consumes: existing ten V3 PNGs and the root-offset-capable runtime.
- Produces: one neutral side-profile assembly with 3–5 logical-pixel hip separation and eight readable contact/down/passing/up poses.

- [x] **Step 1: Narrow the hip origins and calibrate joints.**

  Start far/near thigh positions around `[-12, 0]` and `[12, 0]`, then measure the loaded effective scale and tune until the final world pivot distance is 3–5 logical pixels. Adjust shin/boot child positions or pivots only where frozen-pose inspection shows a gap or excessive lump.

- [x] **Step 2: Replace walk keyframes with the requested phase silhouettes.**

  Use the exact eight labels/phases and the prompt’s biomechanical starting signs: Contact A far forward/near rear; Down A accepting far weight; Passing A far support with bent airborne near leg; Up A preparing near contact; Contact B opposite; Down B; Passing B far swing; Up B. Keep body shell at zero, cloak within ±0.5°, and left arm within the requested gentle counter-swing.

- [x] **Step 3: Add logical bob values.**

  Set `root_offset_px` to `[0,0]`, `[0,2]`, `[0,1]`, `[0,-1]` for the first half and the same four values for the second half. Do not use pelvis rig-space Y to fake bob.

- [x] **Step 4: Tune stride from visible foot travel.**

  Test candidate `stride_distance` values between 55 and 80 against equivalent planted contacts in the review output and select the value that matches actual player displacement. Keep player speed/architecture unchanged.

- [x] **Step 5: Run data invariants.**

  Run `ctest --test-dir Build/Stage01DRed -R XYZLogenRigV3AssetTests --output-on-failure` and the scene test. Expected: exact hierarchy, no right arm, bob, hip spacing, and passing invariants pass.

### Task 5: Add F6/F7 diagnostics and repeatable review export

**Files:**
- Modify: `Engine/include/XYZ/Engine/Input.h`
- Modify: `Engine/src/Input.cpp`
- Modify: `Game/include/XYZ/Game/GuffmanBasementScene.h`
- Modify: `Game/src/GuffmanBasementScene.cpp`
- Create: `Game/include/XYZ/Game/RigReviewExporter.h`
- Create: `Game/src/RigReviewExporter.cpp`
- Modify: `Game/CMakeLists.txt`
- Modify: `Game/Tests/RigReviewExporterTests.cpp`

**Interfaces:**
- Consumes: existing input event loop, V3 rig/animations, `Renderer2D::native()`, and `SDL_RenderReadPixels`/`IMG_SavePNG`.
- Produces: one-frame F6/F7 flags, scene foot-plant enable state, expanded overlay, and ten files under `Build/RigReview`.

- [x] **Step 1: Add F6/F7 input flags.**

  Add one-frame accessors/fields, clear them in `beginFrame`, and map non-repeat F6/F7 key-down events without changing F3/F4/F5.

- [x] **Step 2: Toggle planting in the scene.**

  Default `footPlantEnabled_` to true. On F6 toggle it; when disabled reset the controller and apply zero visual root correction. Show `Foot plant: ON/OFF` and `Support: NEAR/FAR/TRANSFER/NONE`.

- [x] **Step 3: Expose the full planting telemetry.**

  Render current near/far foot X, planted world X, desired correction X, and applied correction X. Render paused pose text as `1/8 CONTACT A` etc. and near/far foot Y relative to `PlayerBaselineY`.

- [x] **Step 4: Implement the exporter.**

  Create a target texture at 960×540, render a neutral dark background, baseline, and the actual rig at target height with planting correction disabled. Save idle, eight exact walk keyframe phases, and the 8-wide contact sheet using `SDL_RenderReadPixels` and `IMG_SavePNG`. Restore the previous render target and rig state after export; return errors to the scene overlay.

- [x] **Step 5: Run input, scene, and exporter tests.**

  Run the focused tests from Task 1 and confirm F6/F7 state, telemetry, and all ten 960×540 PNG outputs.

### Task 6: Acceptance, visual pass, and commit

**Files:**
- Create: `Tools/verify-stage01d.sh`
- Create: `docs/Stage-01D.md`
- Modify: `docs/README.md`

- [x] **Step 1: Add the Stage 01D acceptance script.**

  Require the existing Rig V3 directory/assets, source-level no-right-arm checks, Swift launcher tests/builds, fresh CMake Debug/Release configure/build/CTest, direct self-tests, and exporter tests. Keep `Build/RigReview` ignored as generated dev output.

- [x] **Step 2: Run the full acceptance script.**

  Run `bash Tools/verify-stage01d.sh` and read the complete output. It must report zero failed tests in both configurations.

- [x] **Step 3: Run the actual game review.**

  Launch the built game. Use F4 idle overlay for neutral alignment; use F3/F5 to inspect all eight paused poses; confirm Contact A/B opposite separation, Passing A near airborne, Passing B far airborne, stable body shell, connected joints, crisp pixels, both facing directions, click-to-move short/long targets, A/D fallback, and stopping. Press F6 to compare planting ON/OFF, then F7 and inspect every exported image/contact sheet.

- [x] **Step 4: Commit and verify repository state.**

  Run `git diff --check`, verify only Stage 01D changes are present, commit with `Fix Logen Rig V3 walk biomechanics`, and confirm the working tree is clean. Do not begin Stage 02.
