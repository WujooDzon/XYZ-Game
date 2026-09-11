# Logen Rig V3

This is the active production cutout rig for Stage 01C. All ten PNGs are right-facing, strict side-profile components derived from `../Logen_Master_Right_v1.png` and are rendered with nearest-neighbor sampling.

The hierarchy is loaded by `Logen_rig_v3_definition.json` and is intentionally small:

- `body_shell` owns the stable upper shell and the empty right sleeve.
- `left_arm` is the only intact arm asset.
- `cloak_tail` and `cloak_front` sit behind the body and legs.
- `far_*` and `near_*` form two articulated leg chains; far parts use a restrained 0.78 value factor while keeping the near geometry.
- `near_boot` and `far_boot` define the ground-contact anchors used by foot planting.

`Logen_walk_v3.json` contains the eight contact/down/passing/up poses. `Logen_idle_v3.json` contains the two-pose breathing loop. The target normalized height is 188 logical pixels.

The PNG cleanup pipeline is kept in `Tools/normalize-rig-v3.swift`, `Tools/crop-rig-v3.swift`, `Tools/trim-rig-v3.swift`, and `Tools/darken-rig-v3.swift`. It preserves hard pixel edges, writes RGBA PNGs, removes generated background/ghost components, and provides deterministic far-side derivation.

No right forearm or right hand is part of this rig. No gameplay, dialogue, NPC, quest, economy, save, or audio systems are included in this stage.
