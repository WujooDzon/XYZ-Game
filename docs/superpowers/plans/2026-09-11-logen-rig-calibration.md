# Logen Rig Calibration and Walk Quality Pass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Calibrate the Stage 01B hierarchical Logen rig into one coherent rig-space model with authoritative 188 px neutral sizing, fixed grounding, a usable F3/F4/F5 calibration workflow, planted-foot walk quality, and stable animation transitions.

**Architecture:** Keep `GuffmanBasementScene` as the game composition boundary. Extend the existing engine `Rig2D`, `RigAnimation`, `RigAnimator`, `Renderer2D`, and `Input` contracts; add a pure render-side `FootPlantController`; update only the Logen rig data and focused tests. Player movement remains owned by `PlayerController`, and the launcher/build workflow is preserved.

**Tech Stack:** C++20, CMake, SDL3, SDL3_image, SDL logical presentation at 960×540, nearest-neighbor textures, AppleClang/Ninja on Apple Silicon.

**Spec:** `docs/superpowers/specs/2026-09-11-logen-rig-calibration-design.md`

## Global Constraints

- Do not alter, regenerate, move, or delete supplied PNG assets.
- Do not add gameplay systems or begin Stage 02.
- Keep click-to-move primary and A/D/arrow movement as secondary/debug controls.
- Keep the active scene free of legacy full-frame walk/idle image references.
- Keep the right sleeve empty: no right forearm or hand node may be introduced.
- Use one rig-space coordinate model; do not reintroduce `root_offset` virtual pixels.
- Do not change the basement artwork, camera, launcher UI, or launcher process architecture.
- Use test-first steps: each production change is preceded by a focused failing assertion or test extension.
- Finish with fresh Debug, Release, CTest, self-test, launcher, and visual checks before claiming completion.

---

### Task 1: Record the rig audit and add the Stage 01C acceptance target

**Files:**
- Create: `docs/logen-rig-audit.md`
- Create: `Tools/verify-stage01c.sh`
- Modify: `Game/src/GameApp.cpp`

- [ ] Document every supplied rig PNG, its role, the combined accessory duplication, the intentional exclusions, the empty right sleeve, and the legacy-frame exclusion.
- [ ] Add a Stage 01C self-test assertion for the 15 active art components, `root_to_ground`, target height 188, and absence of right hand/forearm nodes.
- [ ] Add an executable verification script that configures/builds/tests Debug and Release, runs the game self-test, and checks that active source files do not reference legacy walk frame names.
- [ ] Run the script before implementation changes to record the expected red baseline where the new target is not yet present.

### Task 2: Add failing rig-space and calibration tests

**Files:**
- Modify: `Engine/Tests/Rig2DTests.cpp`
- Modify: `Game/Tests/GuffmanBasementSceneTests.cpp`
- Create: `Game/Tests/FootPlantControllerTests.cpp`
- Modify: `Game/CMakeLists.txt`

- [ ] Extend the rig fixture to use `root_to_ground`, assert neutral target normalization, assert scale is consistent across nodes, and assert a fixed ground anchor remains stable when a pose changes.
- [ ] Add calibration mutation checks for node position, rotation, and pivot plus a save/backup check using temporary JSON files.
- [ ] Add scene checks for F4/F5, node selection, pose stepping, labeled key poses, and the full diagnostics state.
- [ ] Add foot-plant tests proving a render correction can move the visual root while `PlayerController` X and target remain unchanged.
- [ ] Build the focused new tests and observe the expected compile/test failures before changing production implementations.

### Task 3: Replace mixed-unit Rig2D grounding with target-height normalization

**Files:**
- Modify: `Engine/include/XYZ/Engine/Rig2D.h`
- Modify: `Engine/src/Rig2D.cpp`
- Modify: `Engine/Tests/Rig2DTests.cpp`

**Interfaces:**

- `void setRootPosition(SDL_FPoint gameplayRoot) noexcept`
- `void setVisualRootCorrectionX(float correction) noexcept`
- `[[nodiscard]] float neutralHeight() const noexcept`
- `[[nodiscard]] SDL_FPoint rootToGround() const noexcept`
- `[[nodiscard]] std::optional<std::size_t> nodeIndex(std::string_view id) const noexcept`
- `bool adjustNodePosition(std::size_t, SDL_FPoint delta) noexcept`
- `bool adjustNodeRotation(std::size_t, float deltaDegrees) noexcept`
- `bool adjustNodePivot(std::size_t, SDL_FPoint delta) noexcept`
- `bool saveCalibration(std::string& error) const`
- `[[nodiscard]] SDL_FPoint footContactPosition(std::string_view id, const RigPose&) const noexcept`

- [ ] Parse `root_to_ground`; accept `global_scale` only as a post-normalization multiplier and remove all `root_offset` evaluation.
- [ ] Evaluate a unit-scale neutral bounds after all textures load, calculate `effectiveScale = targetHeight / neutralHeight * multiplier`, and keep the loaded rig unchanged on any failure.
- [ ] Anchor the internal root from the integer-snapped gameplay ground point and apply the independent visual correction only to rendering/evaluation.
- [ ] Quantize evaluated node translations to 1/16 logical pixel and keep mirrored pivots/foot contact transforms coherent.
- [ ] Implement node lookup, bounded calibration edits, foot contact metadata, and direct JSON save with a same-directory `.backup.json` created before overwrite.
- [ ] Run `XYZRig2DTests` and the prior engine suite; fix only failures caused by this task.

### Task 4: Add alpha modulation for the master reference overlay

**Files:**
- Modify: `Engine/include/XYZ/Engine/Renderer2D.h`
- Modify: `Engine/src/Renderer2D.cpp`
- Modify: `Engine/Tests/Renderer2DTests.cpp`

- [ ] Add a modulation-aware texture draw overload while preserving existing calls.
- [ ] Set/restore SDL texture color and alpha modulation around the draw so the F4 master overlay renders at 35% alpha without changing other sprites.
- [ ] Extend the renderer test to exercise the modulation overload.
- [ ] Run renderer and engine tests.

### Task 5: Add transition-aware, phase-owned RigAnimator

**Files:**
- Modify: `Engine/include/XYZ/Engine/RigAnimation.h`
- Modify: `Engine/src/RigAnimation.cpp`
- Modify: `Engine/Tests/RigAnimationTests.cpp`
- Modify: `Assets/Characters/Logen/Rig/Logen_walk.json`
- Modify: `Assets/Characters/Logen/Rig/Logen_idle.json`

**Interfaces:**

- `void setAnimation(const RigAnimation*, float transitionSeconds = 0.0F, bool preservePhase = true) noexcept`
- `void advanceByDistance(float distance) noexcept`
- `void advanceByTime(float deltaSeconds) noexcept`
- `void advanceKeyframe(int direction) noexcept`
- `void setPaused(bool paused) noexcept`
- `[[nodiscard]] bool paused() const noexcept`
- `[[nodiscard]] bool isBlending() const noexcept`
- `[[nodiscard]] std::size_t keyframeIndex() const noexcept`
- `[[nodiscard]] std::string_view keyframeLabel() const noexcept`

- [ ] Add optional JSON keyframe labels and validate them without changing existing transform syntax.
- [ ] Maintain independent idle and locomotion phases, restore walk phase on idle→walk, and blend sampled poses over the requested transition duration.
- [ ] Make distance progression the only walk clock; make idle progression time-based; pause both without changing phase.
- [ ] Add comma/period pose stepping while paused and expose current label/index for diagnostics.
- [ ] Replace the current exaggerated walk data with eight labeled contact/down/passing/up A/B poses using restrained torso motion, alternating thigh/shin/boot articulation, and neutral first/last continuity.
- [ ] Run animation tests, including phase preservation and transition convergence.

### Task 6: Add input flags and the render-side FootPlantController

**Files:**
- Modify: `Engine/include/XYZ/Engine/Input.h`
- Modify: `Engine/src/Input.cpp`
- Modify: `Engine/Tests/InputTests.cpp`
- Create: `Game/include/XYZ/Game/FootPlantController.h`
- Create: `Game/src/FootPlantController.cpp`
- Create: `Game/Tests/FootPlantControllerTests.cpp`
- Modify: `Game/CMakeLists.txt`

- [ ] Add one-frame flags for F4, F5, TAB navigation, pose stepping, calibration edits, and calibration save; preserve held movement state and click-to-move behavior.
- [ ] Map shift modifier to 10-unit/5-degree calibration increments and keep arrow calibration from accidentally becoming gameplay movement while F3 calibration is active.
- [ ] Implement support-foot ownership from walk phase and boot contact anchors, smooth ownership transfer, visual root correction limits, and idle reset.
- [ ] Keep all correction state independent of `PlayerController` and expose support foot/correction for diagnostics.
- [ ] Run input, foot-plant, player, and scene logic tests.

### Task 7: Integrate F3/F4/F5 calibration and master overlay in the scene

**Files:**
- Modify: `Game/include/XYZ/Game/GuffmanBasementScene.h`
- Modify: `Game/src/GuffmanBasementScene.cpp`
- Modify: `Game/Tests/GuffmanBasementSceneTests.cpp`
- Modify: `Game/CMakeLists.txt`

- [ ] Load `Logen_Master_Right_v1.png` separately and render it at the same target height, fixed baseline, canonical right orientation, and 35% alpha when F4 is enabled.
- [ ] Route F3 controls to selected-node edits, bounded tab navigation, save/backup, reload, and paused walk-pose stepping.
- [ ] Keep click-to-move and A/D/arrow behavior unchanged; switch animations with 0.12/0.18 second blends and stop through stable contact/idle settling within 0.25 seconds.
- [ ] Invoke `FootPlantController` after animation sampling and before rig render; never write its correction into player coordinates.
- [ ] Expand the debug overlay with all required player, animation, calibration, height, foot, and overlay fields.
- [ ] Make `characterHeight()` report the normalized neutral height and keep `usesLegacyWalkFrames()` false.
- [ ] Run the full scene test suite and manually inspect F3/F4/F5 behavior.

### Task 8: Verify the complete Stage 01C workflow and finish once

**Files:**
- Modify: `docs/Stage-01B.md` only if the existing stage notes need a short 01C link
- Modify: `docs/README.md` only if the stage index needs the new audit link

- [ ] Run `cmake --fresh -S . -B Build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON` and build all targets.
- [ ] Run `ctest --test-dir Build/Debug --output-on-failure` and `Tools/verify-stage01c.sh`.
- [ ] Run the same configure/build/CTest/self-test workflow for `Build/Release`.
- [ ] Run the launcher verification and confirm Build & Run still launches the current game executable.
- [ ] Launch the game once for a visual pass: neutral F4 alignment, all F5 poses, click-to-move in both directions, A/D fallback, fixed floor contact, crisp pixels, no duplicated accessory layers, and empty right sleeve.
- [ ] Inspect `git diff`, keep the change scoped to Stage 01C, commit once with a Stage 01C message, verify the final worktree/commit, and stop.
