import hashlib
import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path

from PIL import Image


MODULE_PATH = Path(__file__).parents[1] / "audit_logen_assets.py"
SPEC = importlib.util.spec_from_file_location("audit_logen_assets", MODULE_PATH)
audit_logen_assets = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(audit_logen_assets)


class AuditLogenAssetsTests(unittest.TestCase):
    def test_scan_finds_opaque_and_translucent_bright_pixels(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.png"
            image = Image.new("RGBA", (6, 5), (0, 0, 0, 0))
            image.putpixel((1, 1), (40, 30, 20, 255))
            image.putpixel((2, 2), (255, 255, 255, 255))
            image.putpixel((4, 3), (220, 221, 222, 96))
            image.save(path)

            report = audit_logen_assets.scan_image(path)

            self.assertEqual(report["dimensions"], [6, 5])
            self.assertEqual(report["bbox_alpha_gt_0"], [1, 1, 5, 4])
            self.assertEqual(report["bbox_alpha_ge_128"], [1, 1, 3, 3])
            self.assertEqual(report["alpha_counts"], {"zero": 27, "partial": 1, "opaque": 2})
            self.assertEqual(report["bright_pixels"]["opaque"], 1)
            self.assertEqual(report["bright_pixels"]["partial"], 1)
            self.assertEqual(report["partial_rgb_range"], {"min": [220, 221, 222], "max": [220, 221, 222]})

    def test_diagnostics_preserve_source_bytes_and_canvas(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source.png"
            output = root / "diagnostics"
            image = Image.new("RGBA", (7, 9), (0, 0, 0, 0))
            image.putpixel((3, 4), (255, 255, 255, 255))
            image.putpixel((4, 4), (100, 80, 60, 64))
            image.save(source)
            before = source.read_bytes()

            generated = audit_logen_assets.write_diagnostics(source, output)

            self.assertEqual(before, source.read_bytes())
            self.assertEqual(hashlib.sha256(before).hexdigest(), audit_logen_assets.sha256_file(source))
            expected = {
                "alpha_threshold_001.png",
                "alpha_threshold_064.png",
                "alpha_threshold_128.png",
                "alpha_threshold_192.png",
                "background_dark.png",
                "background_neutral.png",
                "background_contrast.png",
                "bright_candidate_mask.png",
            }
            self.assertEqual({path.name for path in generated}, expected)
            for path in generated:
                with Image.open(path) as diagnostic:
                    self.assertEqual(diagnostic.size, (7, 9))

    def test_empty_alpha_is_reported_without_division(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "empty.png"
            Image.new("RGBA", (3, 2), (99, 88, 77, 0)).save(path)

            report = audit_logen_assets.scan_image(path)

            self.assertIsNone(report["bbox_alpha_gt_0"])
            self.assertIsNone(report["bbox_alpha_ge_128"])
            self.assertIsNone(report["partial_rgb_range"])

    def test_normalizer_preserves_straight_color_when_hardening_alpha(self):
        repo = Path(__file__).parents[2]
        normalizer = repo / "Tools" / "normalize-rig-v3.swift"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "control.png"
            output = root / "normalized.png"
            image = Image.new("RGBA", (7, 7), (0, 0, 0, 0))
            for y in range(2, 5):
                for x in range(2, 5):
                    image.putpixel((x, y), (200, 100, 50, 128))
            image.save(source)

            result = subprocess.run(
                ["swift", str(normalizer), str(source), str(output)],
                cwd=repo,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            normalized = Image.open(output).convert("RGBA")
            visible = [pixel for pixel in normalized.getdata() if pixel[3] > 0]
            self.assertTrue(visible)
            for red, green, blue, alpha in visible:
                self.assertLessEqual(abs(red - 200), 2)
                self.assertLessEqual(abs(green - 100), 2)
                self.assertLessEqual(abs(blue - 50), 2)
                self.assertEqual(alpha, 255)


if __name__ == "__main__":
    unittest.main()
