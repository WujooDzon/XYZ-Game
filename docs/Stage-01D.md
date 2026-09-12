# XYZ — Stage 01D Logen Walk Biomechanics

Stage 01D is a corrective pass on the existing Rig V3 cutout. It keeps the current hierarchy, target-height normalization, nearest-neighbor renderer, distance-driven locomotion, click-to-move, keyboard movement, and F3/F4/F5 diagnostics.

The walk data now contains exactly eight labeled poses: contact, down, passing, and up for both sides of the stride. The passing poses put one Near/Far leg under the pelvis while the opposite knee bends and its foot leaves the floor. Hip origins are calibrated to 3–5 final logical pixels, and body-weight bob is authored as a logical root offset (0, +2, +1, -1) rather than a scaled rig-space translation.

Foot planting is render-only. The controller uses Near/Far terminology, explicit Far/transfer/Near/transfer phase windows, captures the newly planted foot's current world position, and transfers smoothly with a maximum 12 logical pixel correction. F6 toggles this correction for diagnosis and the debug overlay reports both foot positions, the planted anchor, desired correction, and applied correction.

## Review tooling

- F5 pauses and steps walk poses with comma/period. The F3 overlay displays labels such as 3/8 PASSING A and the Near/Far foot Y positions relative to the gameplay baseline.
- F7 exports the actual 188 px rig at 960x540 to Build/RigReview/: idle, all eight walk poses, and Logen_walk_contact_sheet.png. The exporter uses a neutral background and baseline without joint clutter.

Run the complete Stage 01D acceptance check from the repository root:

```bash
./Tools/verify-stage01d.sh
```

The check builds the Swift launcher and C++ game in Debug and Release, runs all CTest targets including the hidden-window scene/exporter checks, and runs XYZGame --self-test.

No Stage 02 gameplay systems are included. There is no dialogue, NPC interaction, quest, economy, inventory, combat, save/load, audio, renderer rewrite, ECS, or new Rig V4.
