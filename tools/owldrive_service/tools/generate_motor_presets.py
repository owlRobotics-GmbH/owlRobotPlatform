#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    service_dir = Path(__file__).resolve().parents[1]
    repo_root = service_dir.parents[0]
    sys.path.insert(0, str(service_dir))

    from owldrive_service.presets import parse_motor_presets_from_db, parse_pcb_presets_from_db

    db_cpp = repo_root / "db.cpp"
    output_dir = service_dir / "data"
    output_dir.mkdir(parents=True, exist_ok=True)

    motor_presets = parse_motor_presets_from_db(db_cpp)
    motor_output = output_dir / "motor-presets.json"
    motor_output.write_text(json.dumps({"source": "db.cpp", "count": len(motor_presets), "presets": motor_presets}, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {len(motor_presets)} motor presets to {motor_output}")

    pcb_presets = parse_pcb_presets_from_db(db_cpp)
    pcb_output = output_dir / "pcb-presets.json"
    pcb_output.write_text(json.dumps({"source": "db.cpp", "count": len(pcb_presets), "presets": pcb_presets}, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {len(pcb_presets)} PCB presets to {pcb_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
