import json
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from owldrive_service.presets import list_motor_presets, list_pcb_presets  # noqa: E402


class PresetsTest(unittest.TestCase):
    def test_motor_presets_load_from_json_without_db_cpp(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data_dir = root / "service" / "data"
            data_dir.mkdir(parents=True)
            (data_dir / "motor-presets.json").write_text(
                json.dumps(
                    {
                        "presets": [
                            {
                                "id": "motor:0",
                                "index": 0,
                                "name": "Test Motor",
                                "description": "test",
                                "values": {"motor.name": "Test Motor"},
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )

            presets = list_motor_presets(root)

        self.assertEqual(len(presets), 1)
        self.assertEqual(presets[0]["name"], "Test Motor")

    def test_pcb_presets_load_from_json_without_db_cpp(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data_dir = root / "service" / "data"
            data_dir.mkdir(parents=True)
            (data_dir / "pcb-presets.json").write_text(
                json.dumps(
                    {
                        "presets": [
                            {
                                "id": "pcb:0",
                                "index": 0,
                                "name": "Test PCB",
                                "description": "test",
                                "values": {"pcb.name": "Test PCB"},
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )

            presets = list_pcb_presets(root)

        self.assertEqual(len(presets), 1)
        self.assertEqual(presets[0]["name"], "Test PCB")


if __name__ == "__main__":
    unittest.main()
