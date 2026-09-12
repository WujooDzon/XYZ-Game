# Logen Audit Repair Design

**Audit baseline:** `f1cec5d914a53d5118e5d4afefcef61b19e2e481`

## Goal

Remove only evidence-backed Logen Rig V3 defects, produce reproducible diagnostics, and deliver two full-frame visual proofs without replacing the runtime animation system or proceeding to Stage 02.

## Constraints

- Keep Rig V3, click-to-move, keyboard movement, launcher Build & Run, and the 960x540 logical game.
- Keep the missing right arm missing.
- Do not add gameplay systems, an editor, ECS, IK, or a new renderer framework.
- Preserve original Rig V3 PNGs unless a pixel-level audit proves a source defect. Diagnostic derivatives live under `Build/LogenAudit/`.
- Technical fixes are isolated on `audit/logen-f1cec5d`; no automatic merge to `main`.

## Evidence flow

1. Record repository/build/runtime provenance and export untouched baseline renders.
2. Scan every active PNG for alpha bounds, alpha/RGB statistics, hashes, bright opaque and translucent candidates, connected regions, and threshold/background diagnostics.
3. Reproduce each suspected defect independently in tests or controlled renderer captures.
4. Fix only confirmed causes: visible-content calibration, mirrored contact/bounds transforms, and any demonstrated image-pipeline defect.
5. Compare direct logical presentation with an explicit 960x540 target using identical assets and poses. Keep the existing presentation path unless the captures demonstrate a regression worth changing.
6. Export two transparent full-frame proofs (`contact_a`, `passing_a`) plus raw/runtime/comparison evidence. They remain review material, not a replacement walk cycle.

## Geometry conventions

- A texture keeps separate canvas bounds and alpha-visible bounds at threshold 128.
- Rig normalization uses the union of transformed visible bounds in the neutral pose; debug/render canvas bounds remain available separately.
- Mirroring is around the same explicit root X for node pivots, rendered textures, contacts, and bounds.
- A mirrored contact transforms both normalized pivot and normalized contact (`p' = 1-p`, `c' = 1-c`) exactly once.
- Master reference scale is `188 / visible_bbox_height`; its visible sole line, not the file edge, is aligned to the gameplay baseline.

## Test boundaries

- `Texture`: visible alpha bounds, padded content, and empty-alpha failure.
- `Rig2D`: mirrored contact symmetry under rotations/parents/scales/roots and bounds matching rendered geometry.
- Audit tool: source immutability, canvas preservation, opaque-white detection, alpha thresholds, and straight/premultiplied control-color round trip.
- Logen assets: support contacts measured against the actual baseline and swing-foot clearance, without asserting a prescribed angle table.
- Exporter: deterministic 960x540 evidence and transparent proof dimensions/anchors.

## Deliverables

Generated evidence is written to a unique directory below `Build/LogenAudit/` and zipped for review. Tracked changes are limited to focused engine/game fixes, tests, audit/export tooling, documentation, and only explicitly selected proof assets if their fidelity is demonstrated.
