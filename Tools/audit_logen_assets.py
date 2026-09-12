#!/usr/bin/env python3
"""Non-destructive pixel audit for Logen PNG assets."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import deque
from pathlib import Path
from typing import Any, Iterable, Optional

from PIL import Image


BRIGHT_MIN = 180
THRESHOLDS = (1, 64, 128, 192)
BACKGROUNDS = {
    "dark": (13, 11, 18, 255),
    "neutral": (96, 96, 96, 255),
    "contrast": (18, 76, 92, 255),
}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _bbox(alpha: Image.Image, threshold: int) -> Optional[list[int]]:
    bounds = alpha.point(lambda value: 255 if value >= threshold else 0).getbbox()
    return list(bounds) if bounds is not None else None


def _components(points: set[tuple[int, int]]) -> list[dict[str, Any]]:
    remaining = set(points)
    result: list[dict[str, Any]] = []
    while remaining:
        start = remaining.pop()
        queue = deque([start])
        component = [start]
        while queue:
            x, y = queue.popleft()
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    neighbor = (x + dx, y + dy)
                    if neighbor in remaining:
                        remaining.remove(neighbor)
                        queue.append(neighbor)
                        component.append(neighbor)
        xs = [point[0] for point in component]
        ys = [point[1] for point in component]
        result.append(
            {
                "pixels": len(component),
                "bbox": [min(xs), min(ys), max(xs) + 1, max(ys) + 1],
            }
        )
    return sorted(result, key=lambda item: (-item["pixels"], item["bbox"]))


def scan_image(path: Path) -> dict[str, Any]:
    image = Image.open(path).convert("RGBA")
    alpha = image.getchannel("A")
    histogram = alpha.histogram()
    partial_pixels: list[tuple[int, int, int]] = []
    bright_opaque: set[tuple[int, int]] = set()
    bright_partial: set[tuple[int, int]] = set()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, value = image.getpixel((x, y))
            if 0 < value < 255:
                partial_pixels.append((red, green, blue))
            if red > BRIGHT_MIN and green > BRIGHT_MIN and blue > BRIGHT_MIN:
                if value == 255:
                    bright_opaque.add((x, y))
                elif value > 0:
                    bright_partial.add((x, y))

    partial_range = None
    if partial_pixels:
        partial_range = {
            "min": [min(pixel[channel] for pixel in partial_pixels) for channel in range(3)],
            "max": [max(pixel[channel] for pixel in partial_pixels) for channel in range(3)],
        }

    return {
        "path": str(path.resolve()),
        "sha256": sha256_file(path),
        "dimensions": [image.width, image.height],
        "bbox_alpha_gt_0": _bbox(alpha, 1),
        "bbox_alpha_ge_128": _bbox(alpha, 128),
        "alpha_counts": {
            "zero": histogram[0],
            "partial": sum(histogram[1:255]),
            "opaque": histogram[255],
        },
        "partial_rgb_range": partial_range,
        "bright_pixels": {
            "opaque": len(bright_opaque),
            "partial": len(bright_partial),
        },
        "bright_components": {
            "opaque": _components(bright_opaque),
            "partial": _components(bright_partial),
        },
    }


def _threshold_copy(image: Image.Image, threshold: int) -> Image.Image:
    output = Image.new("RGBA", image.size, (0, 0, 0, 0))
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, alpha = image.getpixel((x, y))
            if alpha >= threshold:
                output.putpixel((x, y), (red, green, blue, 255))
    return output


def _bright_candidate_mask(image: Image.Image) -> Image.Image:
    output = Image.new("RGBA", image.size, (0, 0, 0, 0))
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, alpha = image.getpixel((x, y))
            if alpha > 0 and red > BRIGHT_MIN and green > BRIGHT_MIN and blue > BRIGHT_MIN:
                output.putpixel((x, y), (255, 32, 32, 255))
    return output


def write_diagnostics(path: Path, output_directory: Path) -> list[Path]:
    image = Image.open(path).convert("RGBA")
    output_directory.mkdir(parents=True, exist_ok=True)
    generated: list[Path] = []
    for threshold in THRESHOLDS:
        output = output_directory / f"alpha_threshold_{threshold:03d}.png"
        _threshold_copy(image, threshold).save(output)
        generated.append(output)
    for name, color in BACKGROUNDS.items():
        background = Image.new("RGBA", image.size, color)
        background.alpha_composite(image)
        output = output_directory / f"background_{name}.png"
        background.save(output)
        generated.append(output)
    mask_output = output_directory / "bright_candidate_mask.png"
    _bright_candidate_mask(image).save(mask_output)
    generated.append(mask_output)
    return generated


def audit_assets(paths: Iterable[Path], output_directory: Path) -> dict[str, Any]:
    paths = [path.resolve() for path in paths]
    before = {path: sha256_file(path) for path in paths}
    reports = []
    for path in paths:
        diagnostics = output_directory / path.stem
        report = scan_image(path)
        report["diagnostics"] = [
            str(generated.resolve()) for generated in write_diagnostics(path, diagnostics)
        ]
        reports.append(report)
    after = {path: sha256_file(path) for path in paths}
    if before != after:
        changed = [str(path) for path in paths if before[path] != after[path]]
        raise RuntimeError("audit modified source images: " + ", ".join(changed))
    return {"images": reports, "sources_unchanged": True}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--include", type=Path, action="append", default=[])
    arguments = parser.parse_args()

    paths = sorted(arguments.assets.glob("*.png")) + arguments.include
    if not paths:
        parser.error(f"no PNG files found in {arguments.assets}")
    arguments.out.mkdir(parents=True, exist_ok=True)
    report = audit_assets(paths, arguments.out)
    report_path = arguments.out / "asset_report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(report_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
