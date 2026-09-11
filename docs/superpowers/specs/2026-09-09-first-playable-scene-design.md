# XYZ Game — Stage 01: First Playable Scene

## Goal

Replace the Stage 00 placeholder executable with one small, playable SDL3 scene: the game opens in Guffman’s basement, renders the provided background and Logen sprites, and lets the player move horizontally using click-to-move as the primary control scheme.

## Scope

Included:

- SDL3 application/window lifecycle on macOS Apple Silicon.
- SDL3_image PNG loading.
- A fixed-camera Guffman basement scene rendered at a 960×540 logical resolution.
- Nearest-neighbor texture sampling and integer logical presentation where the window size permits it.
- Logen’s four-frame idle animation at 6 FPS.
- Logen’s eight-frame walk animation at 10 FPS.
- Click-to-move on a defined floor band, with horizontal movement toward a clamped X target.
- A/D and left/right arrows as secondary manual movement controls.
- Horizontal flip when Logen faces left.
- Horizontal room bounds and a fixed baseline.
- A small optional debug overlay showing FPS, player X, animation state, and facing.
- A non-window `--self-test` mode for automated asset/configuration checks.

Explicitly excluded:

- Dialogue, NPCs, interaction dispatch, quests, economy, inventory, combat, saves, audio, camera movement, jumping, vertical movement, renderer effects, and engine/gameplay systems beyond this scene.

## Coordinates and presentation

- Logical canvas: `960×540`.
- Window: resizable, with a 16:9 logical presentation and letterboxing when required.
- Background: drawn to the full logical canvas from `Assets/Locations/GuffmanBasement/GuffmanBasement_BG_v1.png`.
- Player baseline: `y = 455` logical pixels.
- Player visible height: `160` logical pixels, preserving each source frame’s aspect ratio.
- Initial player center: `x = 335`, near the bed.
- Player center bounds: `x = 80...880`.
- Clickable floor band: `y = 385...540`; a click in this band maps to its logical X and clamps to the player bounds.
- Click target arrival tolerance: `2` logical pixels.
- Direct movement speed: `220` logical pixels per second.

The baseline and bounds are scene data, not global constants in the renderer. This keeps later scenes free to define their own walkable line.

## Input behavior

The scene converts mouse coordinates from the window into logical coordinates using the renderer’s logical presentation mapping.

- Primary: left mouse button in the floor band sets `targetX`.
- While a target is active, the controller moves directly toward it and clears the target when the arrival tolerance is reached.
- Holding A/D or the left/right arrow keys enters manual movement for that frame and clears `targetX`.
- A manual left input faces left; a manual right input faces right.
- Click-to-move faces toward the target while walking.
- No input produces idle animation.
- Escape or window close exits the application.

The controller exposes target movement as a generic “move to X and report arrival” primitive. Stage 01 does not attach an interaction callback; a future NPC/object system can use the same primitive to stop within an interaction-range X before dispatching an interaction.

## Rendering architecture

The code is separated into these responsibilities:

### Engine

- `Application`: SDL initialization, window/renderer creation, frame loop, and quit handling.
- `Renderer2D`: logical presentation, clear/present, texture drawing, mouse-to-logical coordinate conversion, and optional debug text.
- `Texture`: RAII ownership of SDL textures loaded through SDL3_image, always configured with nearest-neighbor scale mode.
- `Sprite`: destination rectangle, baseline placement, and horizontal flip state.
- `Animation`: frame list, FPS, looped update, and current frame index; pure C++ and unit-testable.
- `Input`: per-frame keyboard/mouse state extracted from SDL events.
- `Scene`: update/render interface receiving `Input` and frame delta time.

### Game

- `GameApp`: application composition and normal/self-test entry points.
- `GuffmanBasementScene`: loads the background and Logen frames, owns the scene coordinates, selects idle/walk animation, and renders the debug overlay.
- `PlayerController`: pure movement state used by the scene, including manual input, click target, facing, bounds, and arrival tolerance.

No ECS or global service locator is introduced.

## Assets and manifest

`Assets/Characters/Logen/Logen_animation_manifest.json` will be added because it is referenced by the Stage 01 contract but is absent from the current workspace. It records the exact idle/walk frame lists, FPS values, visible height, and canonical facing. The scene uses the same paths and values, and `--self-test` verifies that the manifest and every referenced PNG exist.

No sprite pixels are edited or generated. The existing right-facing frames are used directly; left-facing rendering uses SDL horizontal texture flipping.

## Dependency and build policy

- CMake discovers `SDL3` and `SDL3_image` through their Homebrew CMake package files.
- `XYZEngine` becomes a small static library with public headers under `Engine/include/XYZ/Engine`.
- `XYZGame` remains at `Build/<Configuration>/Game/XYZGame`, so the existing Swift launcher needs no path change.
- CTest runs pure logic tests without opening a window.
- The launcher’s existing configure/build/build-and-run process is unchanged; its CMake output will show dependency failures in the existing live console.

## Verification contract

Stage 01 verification must demonstrate:

1. Pure animation tests pass for FPS, looping, and frame transitions.
2. Pure controller tests pass for click targets, arrival, manual override, facing, and bounds.
3. Debug and Release CMake configure/build succeed.
4. `XYZGame --self-test` succeeds in both configurations.
5. The packaged launcher still builds.
6. A real launcher Build & Run reaches the game executable only after a successful build.
7. Manual window inspection confirms the basement, crisp sprites, click-to-move, keyboard fallback, animation switching, left flip, and scene bounds.
