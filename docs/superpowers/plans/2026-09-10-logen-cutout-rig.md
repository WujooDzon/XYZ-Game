# Logen Cutout Rig Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the active frame-based Logen renderer with a data-driven hierarchical SDL3 cutout rig that has a coherent neutral pose, an eight-pose distance-driven walk, subtle idle motion, and runtime rig diagnostics.

**Architecture:** Add a small dependency-free JSON reader and a reusable engine rig layer. `Rig2D` owns texture-backed nodes and evaluates parent transforms, while `RigAnimation`/`RigAnimator` sample JSON keyframes. `GuffmanBasementScene` remains the composition boundary: it owns the rig, player controller, animation state, click-to-move, debug toggles, and reload handling.

**Tech Stack:** C++20, CMake, SDL3, SDL3_image, SDL renderer logical presentation at 960×540, nearest-neighbor textures, AppleClang/Ninja on macOS Apple Silicon.

**Spec:** `docs/superpowers/specs/2026-09-10-logen-cutout-rig-design.md`

## Global Constraints

- Do not alter, regenerate, move, or delete any supplied PNG asset.
- Keep the existing launcher appearance and CMake/launcher workflow intact.
- Keep the exact Stage 01B PNGs in `Assets/Characters/Logen/`; place new JSON tuning files in `Assets/Characters/Logen/Rig/` and reference images with `../` paths.
- The active scene must not load or reference `Logen_walk_right_01.png` through `Logen_walk_right_08.png`.
- The rig contains `Logen_rig_right_empty_sleeve.png` only for the right arm side; never attach a right forearm or hand.
- The composed character target is approximately 188 virtual pixels high, with global scale separate from per-node scale.
- Walk phase advances from absolute player displacement divided by JSON `stride_distance`, not from elapsed time.
- Keyboard input cancels click targets; click-to-move remains primary.
- No ECS, third-party skeletal animation library, dialogue, NPC, interaction, quest, economy, inventory, combat, save, or audio system is part of this plan.

---

### Task 1: Add the JSON value reader and tests

**Files:**
- Create: `Engine/include/XYZ/Engine/Json.h`
- Create: `Engine/src/Json.cpp`
- Create: `Engine/Tests/JsonTests.cpp`
- Modify: `Engine/CMakeLists.txt`

**Interfaces:**
- `JsonValue::parseFile(const std::filesystem::path&, std::string&) -> std::optional<JsonValue>`
- `JsonValue::parse(std::string_view, std::string_view sourceName, std::string&) -> std::optional<JsonValue>`
- `JsonValue::isNull/isBoolean/isNumber/isString/isArray/isObject() const noexcept`
- `JsonValue::boolean/number/string/array/object() const`
- `JsonValue::find(std::string_view key) const -> const JsonValue*`

- [x] **Step 1: Write failing parser tests**

Add an active check-based executable with these assertions:

```cpp
const auto document = xyz::engine::JsonValue::parse(
    R"({"name":"walk","loop":true,"phase":0.5,"nodes":[null,{"id":"left_thigh"}]})",
    "memory",
    error);
check(document.has_value(), "valid JSON parses");
check(document->find("name")->string() == "walk", "strings are readable");
check(document->find("loop")->boolean(), "booleans are readable");
check(document->find("phase")->number() == 0.5, "numbers are readable");
check(document->find("nodes")->array().size() == 2, "arrays are readable");
check(document->find("nodes")->array()[0].isNull(), "null is readable");

const auto malformed = xyz::engine::JsonValue::parse("{\"missing\":", "memory", error);
check(!malformed.has_value(), "malformed JSON fails");
check(error.find("memory") != std::string::npos, "parse error names its source");
```

- [x] **Step 2: Run the test to verify it fails**

Run:

```bash
cmake --fresh -S . -B Build/Stage01BJsonRed -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build Build/Stage01BJsonRed --target XYZJsonTests
```

Expected: compilation fails because `XYZ/Engine/Json.h` and the parser implementation do not exist yet.

- [x] **Step 3: Implement the minimal strict parser**

Use a `std::variant` value containing null, bool, finite double, string, `std::vector<JsonValue>`, and `std::map<std::string, JsonValue>`. Parse whitespace, objects, arrays, strings with JSON escapes, the three literals, and JSON numbers. Reject trailing input, duplicate object keys, invalid escapes, malformed numbers, and non-finite numeric values. Include the source name and byte offset in every error.

- [x] **Step 4: Run parser tests and CTest**

Run:

```bash
cmake --build Build/Stage01BJsonRed --target XYZJsonTests
ctest --test-dir Build/Stage01BJsonRed -R XYZJsonTests --output-on-failure
```

Expected: `XYZJsonTests` passes.

---

### Task 2: Add pivot-aware rendering and debug primitives

**Files:**
- Modify: `Engine/include/XYZ/Engine/Renderer2D.h`
- Modify: `Engine/src/Renderer2D.cpp`
- Modify: `Engine/Tests/InputTests.cpp` only if the new renderer test setup needs shared SDL initialization

**Interfaces:**
- Preserve `drawTexture(const Texture&, const SDL_FRect&, bool flipHorizontal = false) const`.
- Add `drawTexture(const Texture&, const SDL_FRect&, float rotationDegrees, const SDL_FPoint& center, bool flipHorizontal = false) const`.
- Add `drawDebugLine(SDL_FPoint start, SDL_FPoint end, SDL_Color color) const`.
- Add `drawDebugRect(const SDL_FRect& rectangle, SDL_Color color) const`.

- [x] **Step 1: Add a focused renderer behavior test**

Create `Engine/Tests/Renderer2DTests.cpp` with SDL video initialization, a hidden 960×540 window, and these checks:

```cpp
xyz::engine::Renderer2D renderer(window);
check(renderer.initialized(), "renderer initializes");
check(renderer.drawDebugLine({10.0F, 10.0F}, {20.0F, 20.0F}, {255, 0, 0, 255}), "debug line draws");
check(renderer.drawDebugRect({10.0F, 10.0F, 20.0F, 20.0F}, {0, 255, 0, 255}), "debug rect draws");
```

 Load `Assets/Locations/GuffmanBasement/GuffmanBasement_BG_v1.png` into a `Texture` and exercise the rotated overload with that real texture; the test must only assert the call succeeds, not renderer pixels.

- [x] **Step 2: Run the focused test to verify the API is missing**

Add the test target to CMake, configure `Build/Stage01BRendererRed`, and run the target. Expected: compile failure for the new methods.

- [x] **Step 3: Implement the overloads**

Route the existing two-argument draw through `SDL_RenderTextureRotated` with zero rotation and the texture center. Route the new overload directly to `SDL_RenderTextureRotated`. For lines and rectangles, set the supplied color, call `SDL_RenderLine`/`SDL_RenderRect`, and return `false` only when the renderer is unavailable or SDL reports failure.

- [x] **Step 4: Run renderer tests**

Run:

```bash
cmake --build Build/Stage01BRendererRed --target XYZRendererTests
ctest --test-dir Build/Stage01BRendererRed -R XYZRendererTests --output-on-failure
```

Expected: pass with no SDL errors.

---

### Task 3: Implement `Rig2D` hierarchy loading and transform evaluation

**Files:**
- Create: `Engine/include/XYZ/Engine/Rig2D.h`
- Create: `Engine/src/Rig2D.cpp`
- Create: `Engine/Tests/Rig2DTests.cpp`
- Modify: `Engine/CMakeLists.txt`

**Interfaces:**
- `struct RigPoseTransform { SDL_FPoint positionOffset; float rotationDegrees; SDL_FPoint scaleMultiplier; }`
- `struct RigPose { std::map<std::string, RigPoseTransform> nodes; }`
- `struct RigNode { std::string id; int parentIndex; std::vector<int> children; Texture texture; SDL_FPoint localPosition; SDL_FPoint pivot; float baseRotationDegrees; SDL_FPoint localScale; int zOrder; }`
- `struct RigWorldNode { std::string id; int parentIndex; SDL_FPoint position; float rotationDegrees; SDL_FPoint scale; SDL_FPoint pivot; SDL_FRect bounds; int zOrder; }`
- `class Rig2D`:
  - `bool loadDefinition(Renderer2D&, const std::filesystem::path&, std::string& error)`
  - `void setRootPosition(SDL_FPoint baselineAnchor) noexcept`
  - `void setMirrored(bool) noexcept`
  - `bool render(Renderer2D&, const RigPose&) const`
  - `bool debugRender(Renderer2D&, const RigPose&) const`
  - `std::vector<RigWorldNode> worldNodes(const RigPose&) const`
  - `SDL_FRect bounds(const RigPose&) const`
  - `const std::vector<RigNode>& nodes() const noexcept`
  - `float globalScale() const noexcept`, `float targetHeight() const noexcept`, `const std::filesystem::path& definitionPath() const noexcept`

- [x] **Step 1: Write failing hierarchy tests**

Add a temporary valid definition file from the test using `std::ofstream`, containing a textureless `root`, a `parent` node, and a textured `child` node. Assert:

```cpp
check(rig.loadDefinition(renderer, definitionPath, error), error.c_str());
check(rig.nodes().size() == 3, "all nodes load");
check(rig.nodes()[1].children.size() == 1, "children are linked");
const auto world = rig.worldNodes({});
check(world[2].position.x == 15.0F, "child position follows parent translation");
```

Add malformed definitions for duplicate ids, missing parents, cycles, pivot values outside 0..1, missing node ids, and a missing image file; each must fail without replacing the last valid loaded rig. Add a parent rotation pose and assert the child world position changes as expected. Add an assertion that the loaded asset node set contains `right_empty_sleeve` and no node id containing `right_forearm` or `right_hand`.

- [x] **Step 2: Run the tests to verify the hierarchy API is missing**

Configure and build `Build/Stage01BRigRed` with the new target. Expected: compilation fails because `Rig2D` and its types do not exist.

- [x] **Step 3: Implement data-driven rig loading**

Parse `global_scale`, `target_height`, `root_offset`, and `nodes`. Resolve each non-empty `image` path relative to the definition file, load it through the existing `Texture`, validate ids/parents/cycles/pivots/numbers, and build child indices. Commit a new definition only after every node and texture succeeds. Use the definition's root offset when mapping `setRootPosition` to the root world position.

World evaluation must apply local pose offsets, parent scale/rotation, and parent translation recursively. Sort render records by z-order. Compute transformed bounds from the four texture corners. For horizontal mirroring, reflect every world position around the root X, negate world rotation, and let the renderer use the mirrored normalized pivot `(1 - pivot.x, pivot.y)` so joints remain anchored.

- [x] **Step 4: Implement rig rendering and diagnostics**

For each textured world node, calculate scaled destination dimensions and a destination rectangle whose anchor is the node pivot. Call the pivot-aware renderer overload with the node rotation and mirror-adjusted pivot. `debugRender` draws parent-child lines, root/pivot points, transformed bounds, and node ids using the renderer debug primitives.

- [x] **Step 5: Run rig tests**

Run:

```bash
cmake --build Build/Stage01BRigRed --target XYZRig2DTests
ctest --test-dir Build/Stage01BRigRed -R XYZRig2DTests --output-on-failure
```

Expected: all hierarchy, validation, mirroring, and missing-right-arm assertions pass.

---

### Task 4: Implement data-driven rig animations and animator

**Files:**
- Create: `Engine/include/XYZ/Engine/RigAnimation.h`
- Create: `Engine/src/RigAnimation.cpp`
- Create: `Engine/Tests/RigAnimationTests.cpp`
- Modify: `Engine/CMakeLists.txt`

**Interfaces:**
- `struct RigAnimationKeyframe { float phase; RigPose pose; }`
- `class RigAnimation`:
  - `bool load(const std::filesystem::path&, std::string& error)`
  - `RigPose sample(float normalizedPhase) const`
  - `std::size_t keyframeCount() const noexcept`
  - `float strideDistance() const noexcept`
  - `float durationSeconds() const noexcept`
  - `std::string_view name() const noexcept`
  - `bool loop() const noexcept`
- `class RigAnimator`:
  - `void setAnimation(const RigAnimation*) noexcept`
  - `void reset() noexcept`
  - `void advanceByDistance(float distance) noexcept`
  - `void advanceByTime(float deltaSeconds) noexcept`
  - `const RigPose& pose() const`
  - `float phase() const noexcept`
  - `std::string_view animationName() const noexcept`

- [x] **Step 1: Write failing animation tests**

Create temporary animation JSON with eight phases and distinct `left_thigh`/`right_thigh` rotations. Assert:

```cpp
check(animation.load(path, error), error.c_str());
check(animation.keyframeCount() == 8, "walk has eight keyframes");
const auto halfway = animation.sample(0.0625F);
check(halfway.nodes.at("left_thigh").rotationDegrees == -9.0F, "poses interpolate");

xyz::engine::RigAnimator animator;
animator.setAnimation(&animation);
animator.advanceByDistance(animation.strideDistance());
check(animator.phase() == 0.0F, "one stride loops phase");
animator.advanceByDistance(animation.strideDistance() * 0.5F);
check(animator.phase() == 0.5F, "walk phase uses distance");
```

Add idle JSON with a 2.4-second duration and assert `advanceByTime(1.2F)` samples the midpoint. Assert a malformed animation with unsorted phases or a phase outside `[0,1)` fails.

- [x] **Step 2: Run the test to verify the animation API is missing**

Build `XYZRigAnimationTests` in `Build/Stage01BAnimationRed`. Expected: compile failure for the missing classes.

- [x] **Step 3: Implement animation loading and interpolation**

Load `name`, `loop`, `stride_distance`, `duration_seconds`, and `keyframes`. Validate non-empty keyframes, strictly increasing phases, phase range, positive stride/duration, and finite transforms. Sample the previous/next keyframe with linear interpolation, including the wrapped last-to-first segment. Missing node entries use identity transforms. Keep the sampled pose in `RigAnimator` so rendering does not reparse JSON.

- [x] **Step 4: Implement animator timing**

`advanceByDistance` adds `abs(distance) / stride_distance` to phase. `advanceByTime` adds `deltaSeconds / duration_seconds`. Both wrap only for looping animations and clamp otherwise. Reset returns phase and pose to the first keyframe.

- [x] **Step 5: Run animation tests**

Run:

```bash
cmake --build Build/Stage01BAnimationRed --target XYZRigAnimationTests
ctest --test-dir Build/Stage01BAnimationRed -R XYZRigAnimationTests --output-on-failure
```

Expected: interpolation, wrapping, timing, and validation tests pass.

---

### Task 5: Add the Logen rig definition and animation data

**Files:**
- Create: `Assets/Characters/Logen/Rig/Logen_rig_definition.json`
- Create: `Assets/Characters/Logen/Rig/Logen_walk.json`
- Create: `Assets/Characters/Logen/Rig/Logen_idle.json`

**Interfaces:**
- Definition paths are relative to `Assets/Characters/Logen/Rig/` and use `../Logen_rig_*.png`.
- The root id is `pelvis`; its `root_offset` places the pelvis above the existing player baseline.
- `global_scale` starts at `0.15` and `target_height` is `188`.

- [x] **Step 1: Add the neutral definition data**

Create the full hierarchy with these ids and parents:

```text
pelvis
├── cloak_back
├── right_thigh -> right_shin -> right_boot
├── left_thigh -> left_shin -> left_boot
├── cloak_front_left
├── cloak_front_right
└── torso
    ├── waist_belt
    ├── red_cloth
    ├── right_empty_sleeve
    ├── left_upper_arm -> left_forearm_hand
    └── head_mask_hood
```

Use normalized anatomical pivots near neck base, pelvis, shoulders, elbows, hips, knees, ankles, cloak attachment, and belt attachment. Set z-order to 0/10–12/20–22/28–30/35/38/50/60–61/70. Use starting source-pixel positions that yield a neutral assembled height near 188 after global scale: leg chain positions around thigh `[0, 210]`, shin-to-boot `[0, 267]`, head around `[40, -280]`, torso at `[0, 0]`, and root offset `[0, -102]`. The file must contain no right hand or right forearm node.

- [x] **Step 2: Add eight visibly distinct walk poses**

Use phases `0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875`. In contact poses alternate the extended forward/backward legs. In down poses lower the pelvis by about 2 source pixels and bend the loaded knee. In passing poses move the swing thigh forward, bend its shin, and lift the boot with a negative local Y offset. In up poses raise the pelvis by about 2 source pixels and prepare the next contact. Start thigh rotations in the ±15–22° range, shin local rotations in the ±10–30° range, boot rotations in the ±5–12° range, torso lean around 2–4°, and left-arm swing around 6–10°. Keep the empty sleeve sway at only a few degrees and never add a hand to it. Set `stride_distance` to `64`.

- [x] **Step 3: Add subtle idle data**

Use a 2.4-second loop with neutral first/last pose. Limit torso vertical motion to about 1 source pixel, head and arm rotation to a few degrees, and cloak/red-cloth motion to subtle delayed offsets. The first pose must be a sensible standing stance so walk-to-idle resets cleanly.

- [x] **Step 4: Validate the data through the parser/rig tests**

Run the rig and animation tests against the checked-in files. Expected: all 16 supplied rig images load, every parent resolves, the definition has one root, animations contain the required keyframe counts, and the right-arm node set remains empty-sleeved.

---

### Task 6: Smooth player movement and debug input events

**Files:**
- Modify: `Game/include/XYZ/Game/PlayerController.h`
- Modify: `Game/src/PlayerController.cpp`
- Modify: `Game/Tests/PlayerControllerTests.cpp`
- Modify: `Engine/include/XYZ/Engine/Input.h`
- Modify: `Engine/src/Input.cpp`
- Modify: `Engine/Tests/InputTests.cpp`

**Interfaces:**
- Preserve `setMoveTarget`, `clearMoveTarget`, and `update(float, InputState)`.
- Add `float velocity() const noexcept` to `PlayerController`.
- Add `bool rigDebugTogglePressed() const noexcept` and `bool rigReloadPressed() const noexcept` to `Input`.

- [x] **Step 1: Write failing movement and input tests**

Extend the player tests with these behaviors:

```cpp
player.setMoveTarget(700.0F);
player.update(1.0F / 60.0F, {});
check(player.velocity() > 0.0F && player.velocity() < 220.0F, "target movement accelerates");
for (int frame = 0; frame < 30; ++frame) player.update(1.0F / 60.0F, {});
check(player.velocity() > 150.0F, "target movement reaches cruise speed");
for (int frame = 0; frame < 180; ++frame) player.update(1.0F / 60.0F, {});
check(!player.isMoving() && player.velocity() == 0.0F, "target movement brakes to rest");
```

Add input events for F3 and R, assert each one-frame getter is true only until `beginFrame()`, and keep existing A/D/arrow assertions.

- [x] **Step 2: Run focused tests to verify the new API is missing**

Build the existing player and input test targets. Expected: compilation fails for `velocity`, `rigDebugTogglePressed`, and `rigReloadPressed`.

- [x] **Step 3: Implement acceleration, braking, and target ease-out**

Add velocity state with a 220 px/s cap, approximately 125 ms acceleration, and approximately 150 ms deceleration. For a target, reduce desired speed using a braking-speed calculation based on remaining distance; approach desired velocity without overshooting. Clamp X to room bounds, clear the target, and zero velocity on arrival. Manual input cancels the target and uses the same cap. `isMoving` remains true during the short deceleration tail and becomes false below a small velocity epsilon.

- [x] **Step 4: Implement one-frame F3/R commands**

Clear both command flags in `beginFrame`. On non-repeat key-down events, set F3 or R flags. Do not change the existing movement state mapping or Escape behavior.

- [x] **Step 5: Run all movement/input tests**

Run:

```bash
cmake --build Build/Stage01BAnimationRed --target XYZPlayerControllerTests XYZInputTests
ctest --test-dir Build/Stage01BAnimationRed -R 'XYZPlayerControllerTests|XYZInputTests' --output-on-failure
```

Expected: acceleration, deceleration, bounds, keyboard override, and one-frame debug commands pass.

---

### Task 7: Replace the scene’s active frame renderer with the rig

**Files:**
- Modify: `Game/include/XYZ/Game/GuffmanBasementScene.h`
- Modify: `Game/src/GuffmanBasementScene.cpp`
- Modify: `Game/Tests/GuffmanBasementSceneTests.cpp`
- Modify: `Game/CMakeLists.txt`

**Interfaces:**
- Keep the existing `Scene` methods and player getters.
- Add scene getters needed by tests/debug: `bool rigDebugEnabled() const noexcept`, `float walkPhase() const noexcept`, `float playerVelocity() const noexcept`, `float rigHeight() const noexcept`, `bool usesLegacyWalkFrames() const noexcept`.

- [x] **Step 1: Write failing scene integration assertions**

Extend the hidden SDL scene test to initialize real `Assets/Characters/Logen/Rig` data and assert:

```cpp
check(scene.rigHeight() >= 182.0F && scene.rigHeight() <= 194.0F, "assembled rig is near 188 pixels");
check(!scene.usesLegacyWalkFrames(), "legacy frame walk is inactive");
check(scene.walkPhase() == 0.0F, "walk starts at neutral phase");
```

Send a floor click, advance one frame, record phase, advance with a smaller displacement, and assert the phase increment is smaller than for a larger displacement. Continue to a target and assert idle resumes with phase reset. Exercise A/D override and left-facing state. Send F3 then R and assert debug mode toggles and reload preserves the loaded rig.

- [x] **Step 2: Run the scene test to verify the rig integration is missing**

Build `XYZGuffmanBasementSceneTests` in the current tree. Expected: compile failure for the new scene getters and rig members.

- [x] **Step 3: Replace frame members with rig members**

Remove `idleFrames_`, `walkFrames_`, `Animation idleAnimation_`, and `Animation walkAnimation_` from the scene. Add `Rig2D`, `RigAnimation idleRigAnimation_`, `RigAnimation walkRigAnimation_`, one active `RigAnimator`, and debug/reload state. Initialize the definition and both animation files in `initialize`; propagate exact error messages to the existing application error path.

- [x] **Step 4: Integrate movement and distance-driven animation**

In `update`, preserve the existing logical floor click band and target clamping. Capture X before/after `PlayerController::update`, use `abs(deltaX)` for the active rig animator’s `advanceByDistance`, use idle time only when not walking, reset the active animator on start, and reset it on stop. Set rig root position to the integer-snapped player X and existing baseline. Set mirror from `Facing::Left`. Keep keyboard movement as target cancellation.

- [x] **Step 5: Integrate rendering and debug overlay**

Render background first, then the rig with the sampled pose. Draw the existing FPS/player/animation/facing text plus phase, velocity, target X, and character height only when F3 is active. In debug mode call `rig.debugRender`. On R while debug mode is active, reload the definition and both animation files transactionally; on failure append a concise error via the existing debug output and keep the previous valid data. Include the required comment `TODO: dedicated left-facing Logen rig required before final production.` next to the mirror path.

- [x] **Step 6: Run scene tests**

Run:

```bash
cmake --build Build/Stage01BScene --target XYZGuffmanBasementSceneTests
ctest --test-dir Build/Stage01BScene -R XYZGuffmanBasementSceneTests --output-on-failure
```

Expected: the real rig definition/assets initialize, the scene moves by click and keyboard, walk phase follows displacement, idle resets, and debug/reload commands work.

---

### Task 8: Update self-tests, acceptance checks, and documentation

**Files:**
- Modify: `Game/src/GameApp.cpp`
- Modify: `Tools/verify-stage01.sh`
- Modify: `Tools/verify-stage00.sh`
- Modify: `docs/Stage-01.md`
- Modify: `Docs/README.md`

**Interfaces:**
- `XYZGame --self-test` must verify the Stage 01B definition, both animation JSON files, all 16 rig PNG names, the reference image, and the empty-sleeve node data.
- Existing launcher configure/build/run/clean behavior remains unchanged.

- [x] **Step 1: Write failing acceptance assertions**

Before changing self-test code, add checks to the acceptance script for:

```bash
test -f "$ROOT_DIR/Assets/Characters/Logen/Rig/Logen_rig_definition.json"
test -f "$ROOT_DIR/Assets/Characters/Logen/Rig/Logen_walk.json"
test -f "$ROOT_DIR/Assets/Characters/Logen/Rig/Logen_idle.json"
! rg -q 'Logen_walk_right_0[1-8]\.png' Game/src Game/include
```

Run the script and observe failure because the new data and active renderer are not present.

- [x] **Step 2: Update the executable self-test**

Replace the old active-frame asset assertions with exact Stage 01B configuration and part checks. Parse the definition and both animation files with `JsonValue::parseFile`, verify the definition has `pelvis` plus 16 texture-backed nodes, verify walk has eight keyframes and idle has a positive duration, and print `XYZ Game Stage 01B self-test passed.` only after all assets/configuration validate.

- [x] **Step 3: Update acceptance scripts**

Keep Swift launcher tests/build/package, Debug/Release CMake configure/build, CTest, non-zero build observability, clean verification, and game self-test. Add checks that the old frame names do not occur in active `Game/src`/`Game/include` and that Stage 01B JSON files and all rig images exist.

- [x] **Step 4: Update documentation**

Document the rig data location, click/keyboard controls, F3/R diagnostics, 188 px target, eight pose phases, distance-driven phase, and explicit non-goals. Link the new Stage 01B documentation from the existing docs index without changing launcher instructions.

- [x] **Step 5: Run acceptance scripts**

Run the complete Stage 01B acceptance script after all source changes. Expected: launcher tests, Debug/Release C++ builds, all CTests, asset/config self-tests, and clean checks pass.

---

### Task 9: Final visual and release verification

**Files:**
- Modify only the JSON tuning files if visual tuning is required: `Assets/Characters/Logen/Rig/Logen_rig_definition.json`, `Assets/Characters/Logen/Rig/Logen_walk.json`, `Assets/Characters/Logen/Rig/Logen_idle.json`

- [ ] **Step 1: Build and launch from XYZ DEV**

Use the packaged launcher’s Build & Run action in Debug, confirm configure/build completes, and confirm the game window opens on the basement scene.

The packaged arm64 launcher and hidden-window scene render were verified from the shared session. Manual window inspection remains pending while the macOS session is locked.

- [ ] **Step 2: Inspect neutral assembly**

Enable F3. Confirm the assembled character is approximately 188 px tall, has no visible gaps at neck/shoulders/hips/knees/ankles, preserves the missing right arm, and matches the supplied master reference silhouette closely enough for the prototype.

- [ ] **Step 3: Inspect all walk phases**

Click across the floor and observe the eight phases. Confirm one leg extends while the other retracts, both knees bend through passing phases, swing feet lift, hips move only subtly, cloak motion lags, and only the intact left arm swings. Confirm slow movement advances phase more slowly than fast movement and stopping resets to a sensible idle pose.

- [ ] **Step 4: Inspect mirror and diagnostics**

Walk left and right, confirm the full rig flips without anchor gaps, confirm F3 shows root/pivots/names/bounds/phase/velocity/target/height, and confirm R reloads valid JSON without restarting. Confirm invalid reload leaves the last valid rig active.

- [x] **Step 5: Verify Debug/Release artifacts**

Run fresh Debug and Release configure/build/CTest/self-test commands, `plutil -lint` and `codesign --verify --deep --strict` for the packaged launcher, and `file` on launcher/game binaries. Check that both game binaries report Apple Silicon `arm64` and that no old walk frame is referenced by active code.
