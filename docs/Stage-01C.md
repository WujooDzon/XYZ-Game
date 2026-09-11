# XYZ — Stage 01C Logen Rig Calibration

Stage 01C is the rig quality pass on top of the Stage 01B hierarchical cutout. The active Logen composition uses one source-rig coordinate space, target-height normalization to 188 logical pixels, a fixed `root_to_ground` anchor, and a render-only foot-plant correction.

The runtime now provides:

- eight labeled contact/down/passing/up walk poses with phase driven by travelled distance;
- idle/walk blending with persistent locomotion phase and a short stop settle;
- F3 node diagnostics and calibration controls, including save-with-backup;
- F4 canonical master-reference overlay at 35% alpha;
- F5 pause plus comma/period walk-pose stepping;
- nearest-neighbor rendering and final translation quantization for pixel stability;
- an explicit audit documenting the combined waist/belt/front asset and the intentionally excluded duplicate red-cloth layer.

Run the Stage 01C acceptance check from the repository root:

```bash
Tools/verify-stage01c.sh
```

The check builds the Swift launcher and the C++ game in Debug and Release, runs all tests, runs `XYZGame --self-test`, and verifies that legacy full-frame animation PNGs are not active source references.

No gameplay systems beyond the existing basement movement prototype are included in this stage.
