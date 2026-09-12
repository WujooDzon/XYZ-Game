# Logen Audit Repair Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Diagnose and repair confirmed Logen Rig V3 geometry/calibration defects and produce two reviewable full-frame proof poses with reproducible audit evidence.

**Architecture:** Add a standalone Pillow audit tool for pixel evidence, extend existing texture/rig geometry with explicit alpha-visible bounds, and extend the existing review exporter for deterministic renderer comparisons and transparent proofs. Keep runtime gameplay, scene ownership, and animation architecture intact.

**Tech Stack:** C++20, CMake, SDL3, SDL3_image, Swift/CoreGraphics normalizer checks, Python 3 + Pillow in a Build-local venv, macOS Apple Silicon.

**Spec:** `docs/superpowers/specs/2026-09-12-logen-audit-repair-design.md`

## Global Constraints

- Work from snapshot `f1cec5d914a53d5118e5d4afefcef61b19e2e481` on branch `audit/logen-f1cec5d`.
- Do not create Rig V4, a new animation framework, editor, ECS, IK, or gameplay systems.
- Do not modify original Rig V3 PNGs without evidence and a pixel-level before/after report.
- Keep click-to-move, A/D/arrows, launcher Build & Run, 960x540 logical output, nearest-neighbor sampling, and the missing right arm.
- Store generated reports and captures under `Build/LogenAudit/`; do not commit generated bulk evidence.
- Execute inline in this session; the user forbids additional agents.

---

### Task 1: Baseline provenance and captures

**Files:**
- Create: `Tools/audit_logen_runtime.py`
- Create: `Tools/Tests/test_audit_logen_runtime.py`
- Create: generated `Build/LogenAudit/<run>/provenance.json`

**Interfaces:**
- Produces: a unique audit directory, resolved source/runtime asset paths, SHA-256 hashes, executable/build paths, SDL renderer metadata, and untouched baseline captures.

- [ ] Write a failing Python test that requires unique output directories, resolved paths, deterministic SHA-256 values, and no source writes.
- [ ] Run the focused Python test and confirm the missing tool is the failure.
- [ ] Implement the minimal provenance/audit-run helpers.
- [ ] Configure a persistent Debug build and capture idle/contact/passing in both facings before fixes.
- [ ] Record executable path, project root, active JSON/PNG paths, asset-copy search results, SDL version/backend/output size, and git state.
- [ ] Run the focused tests green and commit the provenance/tooling slice.

### Task 2: PNG and alpha-pipeline audit

**Files:**
- Create: `Tools/audit_logen_assets.py`
- Create: `Tools/Tests/test_audit_logen_assets.py`
- Modify only if confirmed: `Tools/normalize-rig-v3.swift`
- Create: generated `Build/LogenAudit/<run>/assets/**`

**Interfaces:**
- `audit_image(path, output_dir) -> dict`
- Produces: dimensions, hashes, alpha bounds at `>0` and `>=128`, alpha/RGB counts, bright opaque/translucent candidates, contour masks, threshold copies, and three-background composites without changing source canvas or bytes.

- [ ] Write failing fixtures for transparent padding, an opaque white contaminant, a translucent control color, and an empty image.
- [ ] Run tests and confirm failures are caused by missing scan/derivative behavior.
- [ ] Implement scanning and non-destructive diagnostics with Pillow.
- [ ] Run the scanner over the master and all ten active Rig V3 PNGs.
- [ ] Inspect candidate regions and renderer experiments; classify texture-bound contamination versus background-colored gaps.
- [ ] Add and run a controlled straight/premultiplied alpha round-trip test before changing the Swift normalizer.
- [ ] If and only if the pipeline defect reproduces, fix it minimally and verify red-green; otherwise document it as unconfirmed.
- [ ] Commit the audit-tool slice.

### Task 3: Visible-content bounds and master calibration

**Files:**
- Modify: `Engine/include/XYZ/Engine/Texture.h`
- Modify: `Engine/src/Texture.cpp`
- Modify: `Engine/include/XYZ/Engine/Rig2D.h`
- Modify: `Engine/src/Rig2D.cpp`
- Modify: `Game/src/GuffmanBasementScene.cpp`
- Modify: `Engine/Tests/Renderer2DTests.cpp`
- Modify: `Engine/Tests/Rig2DTests.cpp`
- Modify: `Game/Tests/GuffmanBasementSceneTests.cpp`

**Interfaces:**
- `Texture::visibleBounds() -> std::optional<SDL_Rect>` at alpha threshold 128.
- `RigWorldNode::visibleBounds` and `Rig2D::visibleBounds(pose)` remain distinct from canvas/debug bounds.

- [ ] Add failing tests proving transparent file padding does not change 188px visible normalization and empty alpha returns a clear load error.
- [ ] Run focused tests red.
- [ ] Load an SDL surface once, compute alpha-visible bounds, create the nearest-neighbor texture from that surface, and retain the source-space bounds.
- [ ] Transform visible sub-rect corners through the existing hierarchy and normalize the neutral union to target height.
- [ ] Correct F4 to scale the master by its visible bbox and align the visible sole line with the shared baseline.
- [ ] Export master / baseline rig / corrected rig comparison and run focused tests green.
- [ ] Commit the calibration slice.

### Task 4: Mirrored contact and bounds geometry

**Files:**
- Modify: `Engine/src/Rig2D.cpp`
- Modify: `Engine/Tests/Rig2DTests.cpp`

**Interfaces:**
- Mirrored contacts transform normalized contact and pivot once around the same root axis used by rendering.
- Mirrored `bounds` and `visibleBounds` are recalculated from mirrored position/rotation/pivot.

- [ ] Add a failing regression for `pivot.x=0.18`, `ground_contact.x=0.90` and symmetry `left.x = 2*m - right.x`.
- [ ] Add failing table cases for rotations 0/+25/-25, parent transforms, nonzero roots, scale, Y invariance, double reflection, and rendered bounds.
- [ ] Run the focused test red and record the original numerical error.
- [ ] Implement the minimal one-convention contact transform and bounds recomputation.
- [ ] Run focused and engine tests green; export geometry captures in both directions.
- [ ] Commit the geometry slice.

### Task 5: Ground contact, pace, and rasterization evidence

**Files:**
- Modify: `Game/Tests/LogenRigV3AssetTests.cpp`
- Modify: `Game/src/RigReviewExporter.cpp`
- Modify: `Game/include/XYZ/Game/RigReviewExporter.h`
- Modify: `Game/CMakeLists.txt`
- Create: generated `Build/LogenAudit/<run>/renderer/**`

**Interfaces:**
- Exporter produces identical-pose direct and explicit-target captures at 1x and integer magnification plus a deterministic motion sample strip.
- Asset tests compare transformed support anchors to `PlayerBaselineY` with approximately 1 logical pixel tolerance.

- [ ] Replace angle-table/change-detector assertions with failing world-geometry assertions against the baseline and visible sole contour.
- [ ] Measure actual support X/Y drift, swing clearance, `speed_px_per_s`, `stride_px_per_cycle`, `cycles_per_s`, and `contacts_per_s`.
- [ ] Add a renderer experiment that saves direct logical presentation and explicit 960x540 target output while restoring all SDL state.
- [ ] Export 1x and integer-scale comparisons plus deterministic samples; inspect for texture-bound artifacts, gaps, and jitter.
- [ ] Tune only geometry/pace values justified by measurements; do not mask errors with larger root correction.
- [ ] Run focused tests green and commit the evidence slice.

### Task 6: Two full-frame visual proofs

**Files:**
- Modify: `Game/src/RigReviewExporter.cpp`
- Modify: `Game/Tests/RigReviewExporterTests.cpp`
- Create generated: `Build/LogenAudit/<run>/proof/Logen_contact_a_raw.png`
- Create generated: `Build/LogenAudit/<run>/proof/Logen_contact_a_proof.png`
- Create generated: `Build/LogenAudit/<run>/proof/Logen_passing_a_raw.png`
- Create generated: `Build/LogenAudit/<run>/proof/Logen_passing_a_proof.png`
- Create generated: `Build/LogenAudit/<run>/proof/Logen_proof_comparison.png`

**Interfaces:**
- Both transparent proof PNGs share one 960x540 canvas, root, scale, and baseline; no labels/background/foot-plant correction are embedded.

- [ ] Add failing exporter tests for transparent 960x540 output, common anchor/scale, and exactly two proof poses.
- [ ] Export raw runtime bakes and use them with the approved master as high-fidelity edit references.
- [ ] Correct only legs, cloak continuity, and joint contours while preserving hood/mask/body pixels and true transparency.
- [ ] Inspect alpha, canvas, baseline, and visual identity; reject variants that redesign Logen or invent the missing arm.
- [ ] Save selected proofs and a labeled raw/proof/runtime comparison under the audit run.
- [ ] Run exporter tests green and commit the visual-proof slice as `visual proof / do oceny`.

### Task 7: Full acceptance, report, and publication

**Files:**
- Create: `docs/Logen-Audit-f1cec5d.md`
- Create: `Tools/verify-logen-audit.sh`
- Create generated: `Build/LogenAudit/<run>/test-results.txt`
- Create generated: `Build/LogenAudit/Logen_Audit_f1cec5d_<run>.zip`

**Interfaces:**
- One verification command rebuilds launcher/game Debug and Release, runs Python/C++/Swift checks and game self-test, and records exact results.

- [ ] Add the audit verification script around real tests and output artifacts.
- [ ] Run Python audit tests, focused C++ tests, full Debug/Release CTest, self-test, and launcher tests/build.
- [ ] Exercise launcher Build & Run and manually inspect the running game, click-to-move, keyboard controls, resize, both facings, and F3-F7 tools.
- [ ] Write the report separating confirmed causes, rejected hypotheses, remaining uncertainty, changed files, measured pace/contact values, and user-review items.
- [ ] Zip the complete generated audit directory without overwriting prior runs.
- [ ] Run `git diff --check`, inspect the complete diff, and commit tracked report/verification changes.
- [ ] Push branch `audit/logen-f1cec5d` without force and report branch/SHA; do not merge to `main`.
