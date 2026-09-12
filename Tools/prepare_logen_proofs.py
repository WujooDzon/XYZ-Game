#!/usr/bin/env python3
"""Prepare two review-only Logen proof frames on one game-resolution canvas."""

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

from PIL import Image, ImageDraw


CANVAS = (960, 540)
ROOT = (480, 500)
VISIBLE_HEIGHT = 188


def remove_baked_checkerboard(image: Image.Image) -> Image.Image:
    """Flood away connected bright near-grey pixels without touching enclosed mask whites."""
    source = image.convert("RGBA")
    width, height = source.size
    pixels = source.load()
    background = bytearray(width * height)
    queue: deque[tuple[int, int]] = deque()

    def candidate(x: int, y: int) -> bool:
        red, green, blue, _ = pixels[x, y]
        return max(red, green, blue) - min(red, green, blue) <= 18 and min(red, green, blue) >= 96

    def enqueue(x: int, y: int) -> None:
        offset = y * width + x
        if not background[offset] and candidate(x, y):
            background[offset] = 1
            queue.append((x, y))

    for x in range(width):
        enqueue(x, 0)
        enqueue(x, height - 1)
    for y in range(height):
        enqueue(0, y)
        enqueue(width - 1, y)
    while queue:
        x, y = queue.popleft()
        if x > 0:
            enqueue(x - 1, y)
        if x + 1 < width:
            enqueue(x + 1, y)
        if y > 0:
            enqueue(x, y - 1)
        if y + 1 < height:
            enqueue(x, y + 1)

    output = Image.new("RGBA", source.size, (0, 0, 0, 0))
    output_pixels = output.load()
    for y in range(height):
        for x in range(width):
            if not background[y * width + x]:
                red, green, blue, _ = pixels[x, y]
                output_pixels[x, y] = (red, green, blue, 255)
    return output


def harden_alpha(image: Image.Image, threshold: int = 128) -> Image.Image:
    source = image.convert("RGBA")
    output = Image.new("RGBA", source.size, (0, 0, 0, 0))
    source_pixels = source.load()
    output_pixels = output.load()
    for y in range(source.height):
        for x in range(source.width):
            red, green, blue, alpha = source_pixels[x, y]
            if alpha >= threshold:
                output_pixels[x, y] = (red, green, blue, 255)
    return output


def place_with_shared_transform(
    image: Image.Image,
    scale: float,
    source_anchor: tuple[float, float],
) -> Image.Image:
    resized = image.resize(
        (round(image.width * scale), round(image.height * scale)),
        Image.Resampling.NEAREST,
    )
    canvas = Image.new("RGBA", CANVAS, (0, 0, 0, 0))
    destination = (
        round(ROOT[0] - source_anchor[0] * scale),
        round(ROOT[1] - source_anchor[1] * scale),
    )
    canvas.alpha_composite(resized, destination)
    return harden_alpha(canvas)


def create_comparison(
    runtime_contact: Image.Image,
    runtime_passing: Image.Image,
    proof_contact: Image.Image,
    proof_passing: Image.Image,
) -> Image.Image:
    sheet = Image.new("RGBA", (1920, 1080), (35, 31, 40, 255))
    draw = ImageDraw.Draw(sheet)
    panels = (
        (runtime_contact, (0, 0), "CONTACT A - RUNTIME RAW"),
        (proof_contact, (960, 0), "CONTACT A - PROOF"),
        (runtime_passing, (0, 540), "PASSING A - RUNTIME RAW"),
        (proof_passing, (960, 540), "PASSING A - PROOF"),
    )
    for image, origin, label in panels:
        panel = Image.new("RGBA", CANVAS, (13, 11, 18, 255))
        panel.alpha_composite(image)
        sheet.alpha_composite(panel, origin)
        draw.text((origin[0] + 16, origin[1] + 16), label, fill=(242, 226, 190, 255))
        draw.line(
            (origin[0], origin[1] + ROOT[1], origin[0] + CANVAS[0], origin[1] + ROOT[1]),
            fill=(128, 101, 92, 255),
        )
    return sheet


def prepare(
    contact_source: Path,
    passing_source: Path,
    runtime_contact: Path,
    runtime_passing: Path,
    output_directory: Path,
) -> dict[str, object]:
    contact = remove_baked_checkerboard(Image.open(contact_source))
    passing = harden_alpha(Image.open(passing_source))
    contact_bounds = contact.getchannel("A").getbbox()
    passing_bounds = passing.getchannel("A").getbbox()
    if contact_bounds is None or passing_bounds is None:
        raise ValueError("proof source contains no visible character")

    scale = VISIBLE_HEIGHT / float(contact_bounds[3] - contact_bounds[1])
    source_anchor = (contact_source_image_width(contact_source) / 2.0, float(contact_bounds[3] - 1))
    proof_contact = place_with_shared_transform(contact, scale, source_anchor)
    proof_passing = place_with_shared_transform(passing, scale, source_anchor)

    output_directory.mkdir(parents=True, exist_ok=True)
    contact_output = output_directory / "Logen_contact_a_proof.png"
    passing_output = output_directory / "Logen_passing_a_proof.png"
    proof_contact.save(contact_output)
    proof_passing.save(passing_output)
    comparison = create_comparison(
        Image.open(runtime_contact).convert("RGBA"),
        Image.open(runtime_passing).convert("RGBA"),
        proof_contact,
        proof_passing,
    )
    comparison.save(output_directory / "Logen_proof_comparison.png")
    return {
        "scale": scale,
        "source_anchor": source_anchor,
        "contact_source_bounds": contact_bounds,
        "passing_source_bounds": passing_bounds,
        "contact_output": str(contact_output),
        "passing_output": str(passing_output),
    }


def contact_source_image_width(path: Path) -> int:
    with Image.open(path) as image:
        return image.width


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--contact-source", type=Path, required=True)
    parser.add_argument("--passing-source", type=Path, required=True)
    parser.add_argument("--runtime-contact", type=Path, required=True)
    parser.add_argument("--runtime-passing", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    arguments = parser.parse_args()
    result = prepare(
        arguments.contact_source,
        arguments.passing_source,
        arguments.runtime_contact,
        arguments.runtime_passing,
        arguments.out,
    )
    print(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
