import tempfile
import unittest
from pathlib import Path

from PIL import Image

from Tools.prepare_logen_proofs import harden_alpha, place_with_shared_transform, remove_baked_checkerboard


class PrepareLogenProofsTests(unittest.TestCase):
    def test_checkerboard_is_removed_without_erasing_enclosed_white_detail(self) -> None:
        image = Image.new("RGBA", (9, 9), (220, 220, 220, 255))
        pixels = image.load()
        for y in range(2, 7):
            for x in range(2, 7):
                pixels[x, y] = (30, 20, 15, 255)
        pixels[4, 4] = (245, 245, 245, 255)
        result = remove_baked_checkerboard(image)
        self.assertEqual(result.getpixel((0, 0))[3], 0)
        self.assertEqual(result.getpixel((4, 4)), (245, 245, 245, 255))

    def test_shared_transform_produces_game_canvas_and_binary_alpha(self) -> None:
        image = Image.new("RGBA", (20, 20), (0, 0, 0, 0))
        image.putpixel((10, 10), (100, 70, 40, 130))
        result = place_with_shared_transform(harden_alpha(image), 2.0, (10.0, 10.0))
        self.assertEqual(result.size, (960, 540))
        self.assertEqual(set(result.getchannel("A").getdata()), {0, 255})
        self.assertEqual(result.getpixel((480, 500))[3], 255)


if __name__ == "__main__":
    unittest.main()
