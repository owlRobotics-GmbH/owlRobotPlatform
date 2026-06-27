import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from owldrive_service.firmware_images import (  # noqa: E402
    RP2040_FLASH_BASE,
    UF2_MAGIC_END,
    UF2_MAGIC_START0,
    UF2_MAGIC_START1,
    firmware_payload,
    is_owldrive_image_name,
    uf2_to_binary,
)


def uf2_block(addr, payload, block_no=0, num_blocks=1):
    block = bytearray(512)
    struct.pack_into("<IIIIIIII", block, 0, UF2_MAGIC_START0, UF2_MAGIC_START1, 0, addr, len(payload), block_no, num_blocks, 0xE48BFF56)
    block[32 : 32 + len(payload)] = payload
    struct.pack_into("<I", block, 508, UF2_MAGIC_END)
    return bytes(block)


class FirmwareImagesTest(unittest.TestCase):
    def test_uf2_to_binary_concatenates_flash_payloads(self):
        data = uf2_block(RP2040_FLASH_BASE, b"abc", 0, 2) + uf2_block(RP2040_FLASH_BASE + 4, b"de", 1, 2)

        self.assertEqual(uf2_to_binary(data), b"abc\xffde")

    def test_firmware_payload_detects_uf2_extension(self):
        data = uf2_block(RP2040_FLASH_BASE, b"abc")

        self.assertEqual(firmware_payload(data, "image.uf2"), b"abc")

    def test_owldrive_image_filter_rejects_controller_images(self):
        self.assertTrue(is_owldrive_image_name("owldrive_v44.uf2"))
        self.assertTrue(is_owldrive_image_name("owlDrive_v45.uf2"))
        self.assertFalse(is_owldrive_image_name("owlController_RP2040_00.uf2"))
        self.assertFalse(is_owldrive_image_name("readme.txt"))


if __name__ == "__main__":
    unittest.main()
