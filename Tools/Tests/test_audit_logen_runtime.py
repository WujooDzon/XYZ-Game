import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "audit_logen_runtime.py"
SPEC = importlib.util.spec_from_file_location("audit_logen_runtime", MODULE_PATH)
audit_logen_runtime = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(audit_logen_runtime)


class AuditLogenRuntimeTests(unittest.TestCase):
    def test_run_directory_is_unique_without_overwriting_existing_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            first = audit_logen_runtime.make_run_directory(
                base, "f1cec5d", "20260912T120000Z"
            )
            second = audit_logen_runtime.make_run_directory(
                base, "f1cec5d", "20260912T120000Z"
            )

            self.assertEqual(first.name, "f1cec5d_20260912T120000Z")
            self.assertEqual(second.name, "f1cec5d_20260912T120000Z_02")
            self.assertTrue(first.is_dir())
            self.assertTrue(second.is_dir())

    def test_hashes_and_resolves_only_active_rig_images_without_writing_sources(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rig = root / "Assets" / "Characters" / "Logen" / "RigV3"
            rig.mkdir(parents=True)
            image = rig / "part.png"
            image.write_bytes(b"pixel fixture")
            definition = rig / "definition.json"
            definition.write_text(
                json.dumps(
                    {
                        "nodes": [
                            {"id": "root", "image": ""},
                            {"id": "part", "image": "part.png"},
                        ]
                    }
                ),
                encoding="utf-8",
            )
            before = {path: path.read_bytes() for path in (image, definition)}

            paths = audit_logen_runtime.resolve_active_rig_paths(definition)
            digest = audit_logen_runtime.sha256_file(image)

            self.assertEqual(paths, [image.resolve()])
            self.assertEqual(digest, hashlib.sha256(b"pixel fixture").hexdigest())
            self.assertEqual(before, {path: path.read_bytes() for path in before})

    def test_missing_active_image_is_a_readable_error(self):
        with tempfile.TemporaryDirectory() as temporary:
            definition = Path(temporary) / "definition.json"
            definition.write_text(
                '{"nodes":[{"id":"part","image":"missing.png"}]}',
                encoding="utf-8",
            )

            with self.assertRaisesRegex(FileNotFoundError, "active rig image does not exist"):
                audit_logen_runtime.resolve_active_rig_paths(definition)


if __name__ == "__main__":
    unittest.main()
