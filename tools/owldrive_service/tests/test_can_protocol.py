import math
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from owldrive_service.can_protocol import (  # noqa: E402
    CanCommand,
    CanValue,
    OwldriveFrame,
    decode_value,
    pack_float,
    pack_large_offset_byte,
    unpack_large_offset,
)


class CanProtocolTest(unittest.TestCase):
    def test_frame_roundtrip_uses_six_bit_node_ids(self):
        frame = OwldriveFrame(62, 7, CanCommand.set, CanValue.target, pack_float(1.25))

        decoded = OwldriveFrame.decode(frame.encode())

        self.assertEqual(decoded.source, 62)
        self.assertEqual(decoded.dest, 7)
        self.assertEqual(decoded.cmd, CanCommand.set)
        self.assertEqual(decoded.value, CanValue.target)
        self.assertTrue(math.isclose(decode_value(decoded.value, decoded.data), 1.25))

    def test_large_offset_wire_format_matches_firmware(self):
        payload = pack_large_offset_byte(0x123456, 0xA5)

        self.assertEqual(payload, bytes([0x12, 0x34, 0x56, 0xA5]))
        self.assertEqual(unpack_large_offset(payload), 0x123456)


if __name__ == "__main__":
    unittest.main()
