# First Playable Scene Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Stage 00 placeholder with a tested SDL3 Guffman basement scene where Logen moves by click-to-move or keyboard and switches crisp idle/walk animations.

**Architecture:** Keep the Swift launcher’s process/build services intact and replace only the C++ placeholder with a small static `XYZEngine` library plus a `XYZGame` composition layer. Pure animation and movement state stay independent of SDL for fast unit tests; SDL-specific application, input, texture, renderer, and scene code live behind focused interfaces.

**Tech Stack:** C++20, CMake 3.24+, Ninja, SDL3 3.4.14, SDL3_image, CTest, Swift/SwiftUI launcher from Stage 00, Homebrew on Apple Silicon macOS.

**Spec:** `docs/superpowers/specs/2026-09-09-first-playable-scene-design.md`

## Global Constraints

- Target virtual resolution is exactly `960×540`.
- Use nearest-neighbor sampling and preserve a 16:9 presentation.
- Logen visible sprite height is exactly `160` logical pixels.
- Idle animation is 4 frames at `6 FPS`; walk animation is 8 frames at `10 FPS`.
- Click-to-move is primary; A/D and left/right arrows remain secondary manual controls.
- Do not implement gameplay systems outside this scene: no dialogue, NPC, interaction, quest, economy, inventory, combat, save, audio, or camera system.
- Do not alter the provided PNG pixels.
- Keep the built executable at `Build/<Configuration>/Game/XYZGame` for launcher compatibility.

---

### Task 1: Add SDL dependencies and test targets without production implementation

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `Engine/CMakeLists.txt`
- Modify: `Game/CMakeLists.txt`
- Create: `Engine/Tests/AnimationTests.cpp`
- Create: `Game/Tests/PlayerControllerTests.cpp`

**Interfaces:**
- Consumes: existing Stage 00 `XYZEngine` interface target and `XYZGame` executable target.
- Produces: CMake targets `XYZEngine`, `XYZGame`, `XYZAnimationTests`, and `XYZPlayerControllerTests`; CTest entries with the same test executable names.

- [x] **Step 1: Add dependency discovery and CTest registration**

Add to the root `CMakeLists.txt` before subdirectories:

```cmake
include(CTest)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
enable_testing()
```

Keep C++20 settings and the existing `add_subdirectory(Engine)` / `add_subdirectory(Game)` calls.

- [x] **Step 2: Write the failing animation test**

Create `Engine/Tests/AnimationTests.cpp` with a small assertion helper and these exact behaviors:

```cpp
#include <cassert>
#include <cmath>
#include <optional>
#include <iostream>
#include "XYZ/Engine/Animation.h"

int main() {
    xyz::engine::Animation animation({"idle_01", "idle_02", "idle_03", "idle_04"}, 6.0F);
    assert(animation.currentFrame() == 0);
    animation.update(1.0F / 6.0F);
    assert(animation.currentFrame() == 1);
    animation.update(3.0F / 6.0F);
    assert(animation.currentFrame() == 0);
    animation.setFramesPerSecond(10.0F);
    animation.reset();
    animation.update(0.1F);
    assert(animation.currentFrame() == 1);
    std::cout << "Animation tests passed.\n";
}
```

The test must fail at compile time because `Animation.h` does not yet exist.

- [x] **Step 3: Write the failing controller test**

Create `Game/Tests/PlayerControllerTests.cpp`:

```cpp
#include <cassert>
#include <cmath>
#include <iostream>
#include "XYZ/Game/PlayerController.h"

int main() {
    xyz::game::PlayerController player({80.0F, 880.0F}, 335.0F, 455.0F, 220.0F);
    player.setMoveTarget(700.0F);
    player.update(1.0F, {});
    assert(player.isMoving());
    assert(player.x() == 555.0F);
    assert(player.facing() == xyz::game::Facing::Right);
    player.update(1.0F, {});
    assert(!player.isMoving());
    assert(std::fabs(player.x() - 700.0F) < 0.001F);
    player.setMoveTarget(-100.0F);
    player.update(1.0F, {});
    assert(player.facing() == xyz::game::Facing::Left);
    player.update(10.0F, {});
    assert(player.x() == 80.0F);
    xyz::engine::InputState right{};
    right.moveRight = true;
    player.setMoveTarget(400.0F);
    player.update(0.1F, right);
    assert(player.targetX() == std::nullopt);
    assert(player.facing() == xyz::game::Facing::Right);
    std::cout << "PlayerController tests passed.\n";
}
```

- [x] **Step 4: Wire the tests and verify RED**

Update `Engine/CMakeLists.txt` with the C++20 test executable and registration:

```cmake
add_executable(XYZAnimationTests Tests/AnimationTests.cpp)
target_link_libraries(XYZAnimationTests PRIVATE XYZEngine)
add_test(NAME XYZAnimationTests COMMAND XYZAnimationTests)
```

Update `Game/CMakeLists.txt` with:

```cmake
add_executable(XYZPlayerControllerTests Tests/PlayerControllerTests.cpp)
target_link_libraries(XYZPlayerControllerTests PRIVATE XYZEngine)
add_test(NAME XYZPlayerControllerTests COMMAND XYZPlayerControllerTests)
```

Run:

```bash
cmake -S . -B Build/Stage01Red -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build Build/Stage01Red --target XYZAnimationTests XYZPlayerControllerTests
```

Expected result: compilation fails because the requested production headers do not exist yet. Remove `Build/Stage01Red` after recording the expected RED result.

### Task 2: Implement pure animation, input data, and player movement

**Files:**
- Create: `Engine/include/XYZ/Engine/InputState.h`
- Create: `Engine/include/XYZ/Engine/Animation.h`
- Create: `Engine/src/Animation.cpp`
- Create: `Game/include/XYZ/Game/PlayerController.h`
- Create: `Game/src/PlayerController.cpp`
- Modify: `Engine/CMakeLists.txt`
- Modify: `Game/CMakeLists.txt`

**Interfaces:**
- Consumes: the failing tests from Task 1.
- Produces: `xyz::engine::Animation`, `xyz::engine::InputState`, `xyz::game::Facing`, and `xyz::game::PlayerController` with these public operations:

```cpp
namespace xyz::engine {
class Animation {
public:
    Animation(std::vector<std::string> frames, float framesPerSecond);
    void update(float deltaSeconds);
    void reset();
    void setFramesPerSecond(float framesPerSecond);
    [[nodiscard]] std::size_t currentFrame() const noexcept;
    [[nodiscard]] const std::string& currentFramePath() const;
    [[nodiscard]] float framesPerSecond() const noexcept;
};
}

namespace xyz::game {
enum class Facing { Left, Right };
class PlayerController {
public:
    PlayerController(SceneBounds bounds, float initialX, float baselineY, float speed);
    void setMoveTarget(float targetX);
    void clearMoveTarget();
    void update(float deltaSeconds, InputState input);
    [[nodiscard]] float x() const noexcept;
    [[nodiscard]] float baselineY() const noexcept;
    [[nodiscard]] std::optional<float> targetX() const noexcept;
    [[nodiscard]] bool isMoving() const noexcept;
    [[nodiscard]] Facing facing() const noexcept;
};
}
```

Define `xyz::engine::InputState` in `Engine/include/XYZ/Engine/InputState.h` as `{ bool moveLeft = false; bool moveRight = false; }`; define `xyz::game::SceneBounds` in `Game/include/XYZ/Game/PlayerController.h`.

- [x] **Step 1: Implement the minimal `Animation` state machine**

Store the frame paths, current index, elapsed seconds, and positive FPS. `update()` advances by as many frame durations as elapsed, wraps modulo frame count, and does nothing for an empty frame list. `reset()` returns to frame zero and clears elapsed time.

- [x] **Step 2: Implement the minimal `PlayerController`**

Use a `SceneBounds { float left; float right; }` value. On each update, manual input wins and clears the target. Otherwise move toward the target by `speed * deltaSeconds`, clamp to bounds, and clear the target when the remaining distance is at most `2.0F` or the step reaches it. Update `Facing` from movement direction; `isMoving()` is true only when movement occurred this frame or a target/manual direction is active.

- [x] **Step 3: Link source files and run GREEN**

Make `XYZEngine` a static library containing `src/Animation.cpp`, expose `Engine/include` publicly, add `Game/src/PlayerController.cpp` to `XYZGame`, and add the game include directory to both `XYZGame` and `XYZPlayerControllerTests`.

Run:

```bash
cmake -S . -B Build/Stage01Logic -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build Build/Stage01Logic --target XYZAnimationTests XYZPlayerControllerTests
ctest --test-dir Build/Stage01Logic --output-on-failure
```

Expected result: both tests pass.

### Task 3: Add SDL3 application, input, texture, sprite, renderer, and scene interfaces

**Files:**
- Create: `Engine/include/XYZ/Engine/Application.h`
- Create: `Engine/include/XYZ/Engine/Input.h`
- Create: `Engine/include/XYZ/Engine/Renderer2D.h`
- Create: `Engine/include/XYZ/Engine/Texture.h`
- Create: `Engine/include/XYZ/Engine/Sprite.h`
- Create: `Engine/include/XYZ/Engine/Scene.h`
- Create: `Engine/src/Application.cpp`
- Create: `Engine/src/Input.cpp`
- Create: `Engine/src/Renderer2D.cpp`
- Create: `Engine/src/Texture.cpp`
- Create: `Engine/src/Sprite.cpp`
- Modify: `Engine/CMakeLists.txt`

**Interfaces:**
- Consumes: pure animation and controller types.
- Produces: SDL-backed types used by the game scene, with SDL ownership contained in Engine.

- [x] **Step 1: Define `Input` event state**

Expose `Input::beginFrame()`, `Input::handleEvent(const SDL_Event&)`, `Input::state()` returning `xyz::engine::InputState`, `Input::mousePosition()`, `Input::leftMousePressed()`, and `Input::quitRequested()`. Map SDL scancodes A/D/left/right and record mouse coordinates in window pixels.

- [x] **Step 2: Define RAII `Texture` and `Sprite`**

`Texture::load(SDL_Renderer*, const std::filesystem::path&)` uses `IMG_Load`, `SDL_CreateTextureFromSurface`, frees the surface, and sets `SDL_SCALEMODE_NEAREST`. `Sprite` stores an `SDL_FRect` destination and renders with `SDL_FLIP_HORIZONTAL` when requested.

- [x] **Step 3: Define `Renderer2D`**

Create a renderer with `SDL_CreateRenderer`, set `SDL_SetRenderLogicalPresentation(renderer, 960, 540, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)`, and expose `clear`, `drawTexture`, `drawDebugText`, `present`, and `windowToLogical`. Use `SDL_RenderTexture`/`SDL_RenderTextureRotated` and never enable linear filtering.

- [x] **Step 4: Define `Scene` and `Application`**

`Scene` exposes `initialize(Renderer2D&, std::string&)`, `update(float, const Input&, Renderer2D&)`, and `render(Renderer2D&)`. `Application` owns SDL init/quit, window, renderer, input, a scene reference, a monotonic frame timer, and the main loop. Escape/close exits. Keep frame delta clamped to `0.1F` to avoid a huge movement step after a pause.

- [x] **Step 5: Add SDL link/include settings**

Link `XYZEngine` publicly to `SDL3::SDL3` and `SDL3_image::SDL3_image` (using the target names provided by the installed CMake packages); use `SDL3/SDL.h` and `SDL3_image/SDL_image.h` include paths from those targets. Build the logic tests without creating a window.

### Task 4: Implement Guffman basement scene and self-test mode

**Files:**
- Create: `Assets/Characters/Logen/Logen_animation_manifest.json`
- Create: `Game/include/XYZ/Game/GuffmanBasementScene.h`
- Create: `Game/src/GuffmanBasementScene.cpp`
- Create: `Game/include/XYZ/Game/GameApp.h`
- Create: `Game/src/GameApp.cpp`
- Modify: `Game/src/main.cpp`
- Modify: `Game/CMakeLists.txt`

**Interfaces:**
- Consumes: `Application`, `Input`, `Renderer2D`, `Texture`, `Sprite`, `Animation`, and `PlayerController`.
- Produces: normal interactive `XYZGame` execution and `XYZGame --self-test` output `XYZ Game Stage 01 self-test passed.` with exit code 0 when assets exist.

- [x] **Step 1: Add the manifest with exact Stage 01 data**

Write valid JSON containing:

```json
{
  "character": "Logen",
  "canonicalFacing": "right",
  "visibleHeight": 160,
  "idle": { "fps": 6, "frames": ["Logen_idle_right_01.png", "Logen_idle_right_02.png", "Logen_idle_right_03.png", "Logen_idle_right_04.png"] },
  "walk": { "fps": 10, "frames": ["Logen_walk_right_01.png", "Logen_walk_right_02.png", "Logen_walk_right_03.png", "Logen_walk_right_04.png", "Logen_walk_right_05.png", "Logen_walk_right_06.png", "Logen_walk_right_07.png", "Logen_walk_right_08.png"] }
}
```

- [x] **Step 2: Implement scene loading and state transitions**

Load the background and both frame groups from paths relative to the project root. Construct the player at `(335, 455)` with bounds `(80, 880)` and speed `220`. Convert a left-click through `Renderer2D::windowToLogical`; accept only logical Y `385...540`; set the player target X. On each frame, pass manual key state to the controller, update the active animation, and reset/switch animations at the idle/walk boundary so a new state starts from frame zero.

- [x] **Step 3: Render the background and Logen at the required scale**

Draw the background to `{0, 0, 960, 540}`. Use the current frame texture’s source aspect ratio with a destination height of `160`, place its bottom at `baselineY`, center it at the controller X, and flip horizontally for `Facing::Left`. Set the scene’s camera behavior to fixed; no camera transform is allowed.

- [x] **Step 4: Add the optional developer overlay**

Use SDL3 debug text to draw four lines at logical coordinates `(12, 12)`, `(12, 28)`, `(12, 44)`, and `(12, 60)`:

```text
FPS: <integer>
Player X: <one decimal>
Animation: idle|walk
Facing: left|right
```

- [x] **Step 5: Add `GameApp` and `--self-test`**

`GameApp::run(arguments)` checks for `--self-test` before opening a window. It verifies the manifest, background, four idle PNGs, and eight walk PNGs beneath the discovered project root, prints the exact success line, and returns 0; a missing path prints a descriptive error and returns 1. Normal execution composes `Application` and `GuffmanBasementScene` and runs until quit.

### Task 5: Integrate launcher verification and documentation

**Files:**
- Modify: `Tools/verify-stage00.sh`
- Create: `Tools/verify-stage01.sh`
- Modify: `docs/Stage-00.md`
- Create: `docs/Stage-01.md`
- Modify: `docs/README.md`

**Interfaces:**
- Consumes: built `XYZGame --self-test`, existing Swift launcher/package scripts, and CTest.
- Produces: repeatable acceptance commands for Stage 01 and a launcher-compatible target path.

- [x] **Step 1: Update Stage 00 smoke expectation**

Replace the old placeholder-output assertions in `Tools/verify-stage00.sh` with `XYZGame --self-test` and the exact Stage 01 success line. Keep Swift tests, launcher debug/release builds, CMake Debug/Release configure/build, non-zero build check, and clean check.

- [x] **Step 2: Add the full Stage 01 acceptance script**

`Tools/verify-stage01.sh` must:

1. assert `sdl3` and `sdl3_image` are discoverable through CMake;
2. configure Debug and Release in temporary directories with Ninja and `BUILD_TESTING=ON`;
3. build `XYZAnimationTests`, `XYZPlayerControllerTests`, and `XYZGame` in both configurations;
4. run CTest in both configurations;
5. run `XYZGame --self-test` in both configurations;
6. build/package the Swift launcher;
7. print `Stage 01 acceptance passed.` only after all commands return 0.

- [x] **Step 3: Document controls and launcher behavior**

`docs/Stage-01.md` must state the window controls, floor click band, fixed 960×540 logical canvas, animation rates, build commands, and the explicit list of excluded systems. `docs/README.md` must link Stage 00 and Stage 01.

### Task 6: Execute dependency install, RED/GREEN cycles, and full verification

**Files:**
- No source files; use the scripts and build trees from earlier tasks.

- [x] **Step 1: Install the approved PNG loader dependency**

Run:

```bash
brew install sdl3_image
```

Verify:

```bash
brew list --versions sdl3 sdl3_image
```

- [x] **Step 2: Run the red test build**

Run the exact `Build/Stage01Red` commands from Task 1 and confirm the missing-header failure. Do not proceed until the failure is caused by the intentionally absent production headers.

- [x] **Step 3: Run the logic green test build**

Run the exact `Build/Stage01Logic` commands from Task 2 and confirm two CTest tests pass.

- [x] **Step 4: Run full acceptance**

Run:

```bash
./Tools/verify-stage01.sh
```

Confirm fresh output shows Debug and Release configure/build, CTest success, self-test success, and launcher packaging success.

- [ ] **Step 5: Inspect the interactive app**

Use the packaged launcher’s Build & Run action. Confirm the game window opens; click left/right on the floor; hold A/D and arrows; observe idle/walk transitions, horizontal flip, baseline, bounds, and crisp nearest-neighbor rendering. Close the game window and confirm the launcher reports the process exit cleanly.

- [x] **Step 6: Run the final checklist**

Execution note: the SDL game window was inspected successfully before the macOS session locked. The final automated scene test covers floor clicks, target arrival, left/right facing, D-key movement, and idle transition; a second live UI pass after the lock could not be performed by the automation surface.

Re-read the Stage 01 spec and record evidence for all twelve requested outcomes. Only then report completion.
