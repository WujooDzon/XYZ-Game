#!/usr/bin/env python3
"""Record the exact repository, executable, and assets used by a Logen audit run."""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable, Optional


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def make_run_directory(base: Path, snapshot: str, timestamp: str) -> Path:
    base.mkdir(parents=True, exist_ok=True)
    stem = f"{snapshot}_{timestamp}"
    candidate = base / stem
    suffix = 2
    while candidate.exists():
        candidate = base / f"{stem}_{suffix:02d}"
        suffix += 1
    candidate.mkdir()
    return candidate


def resolve_active_rig_paths(definition_path: Path) -> list[Path]:
    definition_path = definition_path.resolve()
    document = json.loads(definition_path.read_text(encoding="utf-8"))
    paths: list[Path] = []
    for node in document.get("nodes", []):
        image = node.get("image", "")
        if not image:
            continue
        path = (definition_path.parent / image).resolve()
        if not path.is_file():
            raise FileNotFoundError(f"active rig image does not exist: {path}")
        paths.append(path)
    return paths


def _git(root: Path, *arguments: str) -> str:
    result = subprocess.run(
        ["git", *arguments],
        cwd=root,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout.strip()


def _file_record(path: Path) -> dict[str, Any]:
    resolved = path.resolve()
    return {
        "path": str(resolved),
        "size_bytes": resolved.stat().st_size,
        "sha256": sha256_file(resolved),
    }


def _asset_copies(root: Path, active_paths: Iterable[Path]) -> list[dict[str, Any]]:
    active_names = {path.name for path in active_paths}
    copies: list[dict[str, Any]] = []
    build_root = root / "Build"
    if not build_root.is_dir():
        return copies
    for candidate in build_root.rglob("*.png"):
        if "LogenAudit" in candidate.parts or candidate.name not in active_names:
            continue
        copies.append(_file_record(candidate))
    return sorted(copies, key=lambda record: record["path"])


def collect_provenance(
    root: Path,
    executable: Optional[Path] = None,
    runtime_metadata: Optional[Path] = None,
) -> dict[str, Any]:
    root = root.resolve()
    rig_directory = root / "Assets" / "Characters" / "Logen" / "RigV3"
    definition = rig_directory / "Logen_rig_v3_definition.json"
    walk = rig_directory / "Logen_walk_v3.json"
    idle = rig_directory / "Logen_idle_v3.json"
    master = root / "Assets" / "Characters" / "Logen" / "Logen_Master_Right_v1.png"
    active_paths = resolve_active_rig_paths(definition)
    tracked_inputs = [definition, walk, idle, master, *active_paths]
    for path in tracked_inputs:
        if not path.is_file():
            raise FileNotFoundError(f"audit input does not exist: {path}")

    runtime: dict[str, Any] = {}
    if runtime_metadata is not None:
        runtime = json.loads(runtime_metadata.read_text(encoding="utf-8"))

    executable_record = None
    if executable is not None:
        if not executable.is_file():
            raise FileNotFoundError(f"audit executable does not exist: {executable}")
        executable_record = _file_record(executable)

    return {
        "audit_snapshot": "f1cec5d914a53d5118e5d4afefcef61b19e2e481",
        "repository": {
            "project_root": str(root),
            "head": _git(root, "rev-parse", "HEAD"),
            "branch": _git(root, "branch", "--show-current"),
            "status_porcelain": _git(root, "status", "--porcelain=v1"),
        },
        "host": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "python": platform.python_version(),
        },
        "executable": executable_record,
        "runtime": runtime,
        "active": {
            "rig_definition": _file_record(definition),
            "walk_animation": _file_record(walk),
            "idle_animation": _file_record(idle),
            "master_reference": _file_record(master),
            "rig_images": [_file_record(path) for path in active_paths],
        },
        "build_asset_copies": _asset_copies(root, active_paths),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--out-base", type=Path, required=True)
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--runtime-metadata", type=Path)
    parser.add_argument("--timestamp")
    arguments = parser.parse_args()

    root = arguments.root.resolve()
    head = _git(root, "rev-parse", "HEAD")
    timestamp = arguments.timestamp or datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    run_directory = make_run_directory(arguments.out_base, head[:7], timestamp)
    provenance = collect_provenance(root, arguments.executable, arguments.runtime_metadata)
    output = run_directory / "provenance.json"
    output.write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    print(run_directory)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
