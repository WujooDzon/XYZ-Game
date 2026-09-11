# XYZ — Stage 01 First Playable Scene

Stage 01 replaces the Stage 00 placeholder with the first playable C++ target. It contains one fixed-camera SDL3 scene: Guffman’s basement with Logen standing near the bed. The active character renderer is extended by [Stage 01B — Logen Cutout Rig](Stage-01B.md).

## Controls

- Primary: click the walkable floor to move Logen horizontally toward that X position.
- Secondary/debug: hold `A` or `Left Arrow` to move left; hold `D` or `Right Arrow` to move right. Keyboard movement cancels the click target.
- `Escape` or the window close button exits.

Clicks are accepted in the logical floor band `y = 385...540`. The player moves on one line at baseline `y = 455`, with X bounds `80...880`. A click outside the floor band does not change the current movement target. Later interaction targets can reuse the controller’s generic target-X and arrival behavior without adding interaction code in this stage.

## Rendering

- Logical canvas: `960×540`.
- Window presentation: 16:9, resizable, integer logical scaling with letterboxing as needed.
- Texture filtering: nearest-neighbor.
- Logen’s Stage 01B rig target height is `188` logical pixels and is assembled from the supplied cutout parts.
- Walk phase is driven by absolute player displacement with a nominal `64 px` stride.
- Left-facing visuals use horizontal mirror for this prototype; the source PNGs are unchanged.

## Build and test

Install the SDL image loader once:

```bash
brew install sdl3 sdl3_image
```

Run the full Stage 01B acceptance check from the repository root:

```bash
./Tools/verify-stage01.sh
```

The test suite covers animation timing, keyboard/mouse event mapping, bounded eased target movement, and a hidden-window scene integration test that loads the real basement and Logen rig PNGs. `XYZGame --self-test` verifies the Stage 01B asset/configuration set in both Debug and Release.

## Launcher workflow

Build and run the launcher:

```bash
./Tools/run-launcher.sh
```

Use `Build & Run` in the launcher. It configures the selected CMake tree if needed, builds the selected Debug/Release target, and launches `Build/<Configuration>/Game/XYZGame` only after a successful build. The launcher’s existing live stdout/stderr output remains the source of truth for build errors.

## Deliberately not implemented

Stage 01 does not include dialogue, Guffman NPC behavior, interactions, quests, economy, inventory, combat, save/load, audio, jumping, vertical movement, camera movement, or other engine/gameplay systems.
