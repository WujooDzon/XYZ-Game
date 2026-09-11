# XYZ — Stage 01B Logen Cutout Rig

Stage 01B replaces the active frame-based Logen renderer with a small data-driven hierarchical 2D cutout rig. The game remains the same fixed-camera Guffman basement prototype; no new gameplay systems are included. The follow-up calibration and walk-quality pass is documented in [Stage 01C — Logen Rig Calibration](Stage-01C.md).

## Rig data

The runtime definition and animation data live in `Assets/Characters/Logen/Rig/`:

- `Logen_rig_definition.json` defines the pelvis-rooted hierarchy, pivots, source-pixel offsets, z-order, and global scale.
- `Logen_walk.json` contains eight looping contact/down/passing/up poses for both sides of the stride.
- `Logen_idle.json` contains the subtle 2.4-second idle loop.

The definition references the supplied PNG parts in the parent `Logen` directory. The right arm uses only `Logen_rig_right_empty_sleeve.png`; no right forearm or hand is added. All textures use nearest-neighbor sampling and the rig root is snapped to integer logical coordinates.

## Controls and diagnostics

- Primary: click the walkable floor (`y = 385...540`) to walk horizontally toward the clicked X.
- Secondary/debug: hold `A`/`Left Arrow` or `D`/`Right Arrow`; keyboard input cancels the click target.
- `F3`: toggle root/pivot/joint lines, node names, transformed bounds, FPS, phase, velocity, target X, and character height.
- `R`: while F3 is active, reload the rig definition and both animation JSON files.
- `Escape` or the window close button exits.

Movement uses approximately 125 ms acceleration and 150 ms deceleration. Walk phase advances from absolute displacement using the Stage 01B nominal `64 px` stride; Stage 01C calibrates the active data to a restrained `72 px` stride and adds planted-foot correction. Idle phase advances from elapsed time. The assembled character target is `188` logical pixels high on the `960×540` virtual canvas. Left-facing motion mirrors the canonical right-facing rig for this prototype.

## Build and test

Run the complete Stage 01B acceptance check from the repository root:

```bash
./Tools/verify-stage01.sh
```

The check builds the launcher and the C++ target in Debug and Release, runs all tests, validates the rig files and supplied parts, checks that legacy walk frames are not referenced by active game code, and verifies the clean build path.

`Build & Run` in the existing `XYZ DEV` launcher continues to configure the selected CMake tree, build the selected configuration, and launch `Game/XYZGame` only after a successful build.

## Deliberately not implemented

Stage 01B does not include dialogue, Guffman NPC behavior, interactions, quests, economy, inventory, combat, save/load, audio, jumping, vertical movement, camera movement, renderer debug systems beyond the rig overlay, or other engine/gameplay systems.
