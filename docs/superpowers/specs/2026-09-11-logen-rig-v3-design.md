# XYZ Game — Logen Rig V3 Design

**Date:** 2026-09-11
**Status:** Approved for implementation

## Goal

Replace the inconsistent Stage 01C Logen cutout composition with a new production 2D side-profile rig derived from the approved `Logen_Master_Right_v1.png`. The runtime must assemble exactly ten transparent PNG components into one coherent right-facing character while preserving the existing idle, distance-driven walk, click-to-move, calibration, and foot-plant workflows.

This is an asset and character-rendering pass only. It does not add dialogue, NPC behavior, interactions, quests, economy, inventory, combat, save/load, audio, or a new engine subsystem.

## Non-negotiable visual rules

- The master reference is the only visual authority; the existing V2 pieces are historical input only.
- Every V3 part is a strict 90-degree side profile facing right, with the same pixel density, palette, lighting, proportions, and transparent-background treatment.
- The character retains the damaged dark-brown hood, white porcelain mask, one icy-blue eye, black scarf, damaged cloak, faded-gold trim, dark trousers, worn boots, belts, potions, red hanging cloth, hunched posture, and missing right arm.
- The right sleeve is empty and belongs to the stable body shell. No right hand, right forearm, or hidden right-arm node is permitted.
- Source PNGs are never altered in place. V2 remains available for provenance and rollback.

## V3 deliverables

Create `Assets/Characters/Logen/RigV3/` containing exactly these ten PNG files:

1. `Logen_rig_v3_body_shell.png`
2. `Logen_rig_v3_left_arm.png`
3. `Logen_rig_v3_cloak_tail.png`
4. `Logen_rig_v3_cloak_front.png`
5. `Logen_rig_v3_far_thigh.png`
6. `Logen_rig_v3_far_shin.png`
7. `Logen_rig_v3_far_boot.png`
8. `Logen_rig_v3_near_thigh.png`
9. `Logen_rig_v3_near_shin.png`
10. `Logen_rig_v3_near_boot.png`

Each PNG must have a genuinely transparent background, no floor/shadow/text/border, generous transparent padding, and matching source pixel density. The arm, cloak, and leg pieces must include hidden overlap at their intended joints: shoulder, hip, knee, ankle, and cloak-to-body seams.

Add JSON data beside the PNGs:

- `Logen_rig_v3_manifest.json` records the exact ten PNG names, roles, source reference, transparency requirement, and validation metadata.
- `Logen_rig_v3_definition.json` describes a pelvis-rooted hierarchy with `body_shell`, `left_arm`, `cloak_tail`, `cloak_front`, `far_thigh -> far_shin -> far_boot`, and `near_thigh -> near_shin -> near_boot`.
- `Logen_walk_v3.json` contains eight labeled contact/down/passing/up poses with stable body shell, subtle cloak motion, and alternating far/near legs.
- `Logen_idle_v3.json` contains a small two-pose breathing loop with a stable grounded silhouette.

## Runtime integration

The generic `Rig2D` and `RigAnimation` APIs remain the integration boundary. `GuffmanBasementScene` loads V3 definition and animation files from `RigV3`, sets the existing 188 logical-pixel target height, and keeps the current gameplay root and `FootPlantController` independent from art calibration.

The V3 node order and names are data-driven. The scene must not contain legacy full-frame PNG paths, V2 part paths, or a right-arm fallback. The existing F3/F4/F5 tools continue to operate against the selected V3 node and V3 poses:

- F3 toggles diagnostics/calibration.
- TAB/SHIFT+TAB selects a node.
- Arrow keys move the selected node; Q/E rotate it; J/L/I/K adjust its pivot.
- S writes the definition with a same-directory backup.
- F4 toggles the canonical master overlay.
- F5 pauses the animator; comma/period step V3 walk poses.

## Validation strategy

Test-first changes cover the asset manifest contract, PNG alpha and dimensions, exact ten-part count, forbidden right-arm names, V3 hierarchy, target-height normalization, stable `root_to_ground`, labeled walk/idle data, and scene loading of V3. The acceptance script builds the launcher and C++ target in Debug and Release, runs CTest and `XYZGame --self-test`, verifies source-reference exclusion, and checks that the V3 asset directory contains no extra PNGs.

The final manual pass checks neutral assembly against the master overlay, right-facing silhouette, empty right sleeve, preserved left arm, matching far/near leg proportions, visible joint overlaps during all stepped walk poses, grounded boots, crisp nearest-neighbor pixels, click-to-move in both directions, and A/D fallback.
