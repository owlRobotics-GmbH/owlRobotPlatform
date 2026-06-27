from __future__ import annotations

import json
import ast
import operator
import re
from pathlib import Path
from typing import Any


PROFILE_PRESETS: list[dict[str, Any]] = [
    {
        "id": "default",
        "name": "Default",
        "description": "Default profile values, keeps current motor/PCB block",
        "values": {
            "name": "Default",
            "torqueCtl": 0,
            "motionCtl": 0,
            "motion.pAngleP": 20,
            "motion.velocityLimit": 20,
            "motion.pidVelocityP": 0.01,
            "motion.pidVelocityI": 0.1,
            "motion.pidVelocityD": 0,
            "motion.pidVelocityRamp": 100,
            "motion.lpfVelocityTf": 0.3,
            "canSpeed": 1000,
            "canMsgId": 300,
            "canBroadcastMask": 0,
            "canFollowNodeId": 0,
            "canFollowValue": 1,
            "fanDuty": 0.5,
            "align": False,
            "invDir": False,
            "pinEndSwitchIn": 0,
            "endSwitchActiveLow": True,
            "endSwitchAllowPosDirTarget": True,
            "endSwitchAllowNegDirTarget": True,
            "serialOutProt": 1,
            "enableIn": False,
            "pwmIn": False,
            "pwmTestOut": True,
            "pwmTestOutFreq": 30000,
            "voltLimit": 15,
            "currLimit": 2,
            "focModulation": 0,
            "usePhR": False,
            "usePhL": True,
            "useKV": False,
            "comTimeout": 0,
            "uartBaudRate": 115200,
        },
    },
    {
        "id": "hero",
        "name": "Hero",
        "description": "Hero profile values, keeps current motor/PCB block",
        "values": {
            "name": "Hero",
            "torqueCtl": 0,
            "motionCtl": 1,
            "motion.pAngleP": 20,
            "motion.velocityLimit": 200,
            "motion.pidVelocityP": 0.1,
            "motion.pidVelocityI": 0.05,
            "motion.pidVelocityD": 0,
            "motion.pidVelocityRamp": 100,
            "motion.lpfVelocityTf": 0.3,
            "canSpeed": 1000,
            "canMsgId": 300,
            "canBroadcastMask": 0,
            "canFollowNodeId": 0,
            "canFollowValue": 1,
            "fanDuty": 0.5,
            "align": False,
            "invDir": False,
            "pinEndSwitchIn": 0,
            "endSwitchActiveLow": True,
            "endSwitchAllowPosDirTarget": True,
            "endSwitchAllowNegDirTarget": True,
            "serialOutProt": 1,
            "enableIn": False,
            "pwmIn": False,
            "pwmTestOut": False,
            "pwmTestOutFreq": 30000,
            "voltLimit": 18,
            "currLimit": 2,
            "focModulation": 0,
            "usePhR": False,
            "usePhL": True,
            "useKV": False,
            "comTimeout": 0,
            "uartBaudRate": 115200,
        },
    },
    {
        "id": "art",
        "name": "Art",
        "description": "Art profile values, keeps current motor/PCB block",
        "values": {
            "name": "Art",
            "torqueCtl": 0,
            "motionCtl": 2,
            "motion.pAngleP": 20,
            "motion.velocityLimit": 900,
            "motion.pidVelocityP": 0.01,
            "motion.pidVelocityI": 1.0,
            "motion.pidVelocityD": 0,
            "motion.pidVelocityRamp": 10000,
            "motion.lpfVelocityTf": 0.01,
            "canSpeed": 250,
            "canMsgId": 300,
            "canBroadcastMask": 0,
            "canFollowNodeId": 0,
            "canFollowValue": 1,
            "fanDuty": 0.5,
            "align": False,
            "invDir": False,
            "pinEndSwitchIn": 12,
            "endSwitchActiveLow": True,
            "endSwitchAllowPosDirTarget": True,
            "endSwitchAllowNegDirTarget": False,
            "serialOutProt": 1,
            "enableIn": False,
            "pwmIn": False,
            "pwmTestOut": False,
            "pwmTestOutFreq": 30000,
            "voltLimit": 15,
            "currLimit": 2,
            "focModulation": 0,
            "usePhR": False,
            "usePhL": True,
            "useKV": False,
            "comTimeout": 0,
            "uartBaudRate": 115200,
        },
    },
]


def public_presets() -> list[dict[str, Any]]:
    return [
        {
            "id": preset["id"],
            "name": preset["name"],
            "description": preset["description"],
            "values": preset["values"],
        }
        for preset in PROFILE_PRESETS
    ]


def find_preset(preset_id: str) -> dict[str, Any] | None:
    for preset in PROFILE_PRESETS:
        if preset["id"] == preset_id:
            return preset
    return None


MOTOR_ENUMS = {
    "mot_bldc": 0,
    "mot_dc": 1,
    "mot_bldc1": 2,
    "mot_3dc": 3,
    "sensor_none": 0,
    "sensor_hall": 1,
    "sensor_as5600": 2,
    "sensor_as5048": 3,
    "sensor_enc1": 4,
    "sensor_enc2": 5,
    "sensor_flux3": 6,
    "true": True,
    "false": False,
    "bldc_3pwm": 0,
    "bldc_6pwm": 1,
    "misc_none": 0,
    "misc_mpu6050": 1,
    "misc_dyp_a22": 2,
    "misc_vl53l0x": 3,
    "misc_vl53l1x": 4,
    "A0": 26,
    "A1": 27,
    "A2": 28,
    "A3": 29,
}

MOTOR_FIELD_MAP = {
    "name": "motor.name",
    "typ": "motor.typ",
    "u": "motor.u",
    "sen": "motor.sen",
    "kv": "motor.kv",
    "rpm": "motor.rpm",
    "pp": "motor.pp",
    "alignV": "motor.alignV",
    "senDirCW": "motor.senDirCW",
    "zeroOfs": "motor.zeroOfs",
    "phR": "motor.phR",
    "phL": "motor.phL",
    "pwmFreq": "motor.pwmFreq",
    "deadZone": "motor.deadZone",
    "ppr": "motor.ppr",
    "pullUp": "motor.pullUp",
}

PCB_FIELD_MAP = {
    "name": "pcb.name",
    "focDriver": "pcb.focDriver",
    "pinMotorAH": "pcb.pinMotorAH",
    "pinMotorBH": "pcb.pinMotorBH",
    "pinMotorCH": "pcb.pinMotorCH",
    "pinMotorAL": "pcb.pinMotorAL",
    "pinMotorBL": "pcb.pinMotorBL",
    "pinMotorCL": "pcb.pinMotorCL",
    "pinMotorEn": "pcb.pinMotorEn",
    "pinHallA": "pcb.pinHallA",
    "pinHallB": "pcb.pinHallB",
    "pinHallC": "pcb.pinHallC",
    "pinSensA": "pcb.pinSensA",
    "pinSensB": "pcb.pinSensB",
    "pinSensC": "pcb.pinSensC",
    "pinSupply": "pcb.pinSupply",
    "pinSDA": "pcb.pinSDA",
    "pinSCL": "pcb.pinSCL",
    "pinUartTx": "pcb.pinUartTx",
    "pinUartRx": "pcb.pinUartRx",
    "pinSpiTx": "pcb.pinSpiTx",
    "pinSpiRx": "pcb.pinSpiRx",
    "pinSpiSCK": "pcb.pinSpiSCK",
    "pinSpiCS": "pcb.pinSpiCS",
    "pinEnableIn": "pcb.pinEnableIn",
    "pinPwmIn": "pcb.pinPwmIn",
    "pinOdoOut": "pcb.pinOdoOut",
    "pinFan": "pcb.pinFan",
    "pinDirIn": "pcb.pinDirIn",
    "pinBrkIn": "pcb.pinBrkIn",
    "pinFaultOut": "pcb.pinFaultOut",
    "pinPwmTestOut": "pcb.pinPwmTestOut",
    "pinLED": "pcb.pinLED",
    "tempSensor": "pcb.tempSensor",
    "misc": "pcb.misc",
    "shuntR": "pcb.shuntR",
    "opAmpGain": "pcb.opAmpGain",
    "opAmpOfs": "pcb.opAmpOfs",
    "supplyGain": "pcb.supplyGain",
    "overCurrent": "pcb.overCurrent",
    "overVoltage": "pcb.overVoltage",
    "underVoltage": "pcb.underVoltage",
    "emfProtect": "pcb.emfProtect",
}

MOTION_FIELD_MAP = {
    "name": "motion.name",
    "pAngleP": "motion.pAngleP",
    "velocityLimit": "motion.velocityLimit",
    "pidVelocityP": "motion.pidVelocityP",
    "pidVelocityI": "motion.pidVelocityI",
    "pidVelocityD": "motion.pidVelocityD",
    "pidVelocityRamp": "motion.pidVelocityRamp",
    "lpfVelocityTf": "motion.lpfVelocityTf",
}


def list_motor_presets(service_root: Path) -> list[dict[str, Any]]:
    json_presets = load_presets_json(service_root, "motor-presets.json")
    if json_presets:
        return json_presets
    db_cpp = service_root / "db.cpp"
    if not db_cpp.exists():
        return []
    return parse_motor_presets_from_db(db_cpp)


def parse_motor_presets_from_db(db_cpp: Path) -> list[dict[str, Any]]:
    source = db_cpp.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"motor_t\s+motorDatabase\[\]\s*=\s*\{(.*?)\n\};", source, re.S)
    if not match:
        return []
    presets = []
    for idx, body in enumerate(struct_bodies(match.group(1))):
        values = parse_motor_body(body)
        name = str(values.get("motor.name") or f"Motor {idx}").strip()
        presets.append(
            {
                "id": f"motor:{idx}",
                "index": idx,
                "name": name,
                "description": motor_description(values),
                "values": values,
            }
        )
    return presets


def find_motor_preset(service_root: Path, preset_id: str) -> dict[str, Any] | None:
    for preset in list_motor_presets(service_root):
        if preset["id"] == preset_id:
            return preset
    return None


def list_pcb_presets(service_root: Path) -> list[dict[str, Any]]:
    json_presets = load_presets_json(service_root, "pcb-presets.json")
    if json_presets:
        return json_presets
    db_cpp = service_root / "db.cpp"
    if not db_cpp.exists():
        return []
    return parse_pcb_presets_from_db(db_cpp)


def find_pcb_preset(service_root: Path, preset_id: str) -> dict[str, Any] | None:
    for preset in list_pcb_presets(service_root):
        if preset["id"] == preset_id:
            return preset
    return None


def list_motion_presets(service_root: Path) -> list[dict[str, Any]]:
    json_presets = load_presets_json(service_root, "motion-presets.json")
    if json_presets:
        return json_presets
    db_cpp = service_root / "db.cpp"
    if not db_cpp.exists():
        return []
    return parse_motion_presets_from_db(db_cpp)


def find_motion_preset(service_root: Path, preset_id: str) -> dict[str, Any] | None:
    for preset in list_motion_presets(service_root):
        if preset["id"] == preset_id:
            return preset
    return None


def load_presets_json(service_root: Path, filename: str) -> list[dict[str, Any]]:
    candidates = [
        service_root / "data" / filename,
        service_root / "service" / "data" / filename,
        service_root / "owldrive_service" / "data" / filename,
        service_root.parent / "data" / filename,
    ]
    path = next((candidate for candidate in candidates if candidate.exists()), None)
    if path is None:
        return []
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return []
    presets = payload.get("presets", payload if isinstance(payload, list) else [])
    return presets if isinstance(presets, list) else []


def parse_motor_body(body: str) -> dict[str, Any]:
    return parse_struct_body(body, MOTOR_FIELD_MAP)


def parse_pcb_presets_from_db(db_cpp: Path) -> list[dict[str, Any]]:
    source = db_cpp.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"pcb_t\s+pcbDatabase\[\]\s*=\s*\{(.*?)\n\};", source, re.S)
    if not match:
        return []
    presets = []
    for idx, body in enumerate(struct_bodies(match.group(1))):
        values = parse_struct_body(body, PCB_FIELD_MAP)
        name = str(values.get("pcb.name") or f"PCB {idx}").strip()
        presets.append(
            {
                "id": f"pcb:{idx}",
                "index": idx,
                "name": name,
                "description": pcb_description(values),
                "values": values,
            }
        )
    return presets


def parse_motion_presets_from_db(db_cpp: Path) -> list[dict[str, Any]]:
    source = db_cpp.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"motion_t\s+motionDatabase\[\]\s*=\s*\{(.*?)\n\};", source, re.S)
    if not match:
        return []
    presets = []
    for idx, body in enumerate(struct_bodies(match.group(1))):
        values = parse_struct_body(body, MOTION_FIELD_MAP)
        name = str(values.get("motion.name") or f"Motion {idx}").strip()
        presets.append(
            {
                "id": f"motion:{idx}",
                "index": idx,
                "name": name,
                "description": motion_description(values),
                "values": {key: value for key, value in values.items() if key != "motion.name"},
            }
        )
    return presets


def struct_bodies(source: str) -> list[str]:
    return re.findall(r"\{(.*?)\}\s*,?", source, re.S)


def parse_struct_body(body: str, field_map: dict[str, str]) -> dict[str, Any]:
    values: dict[str, Any] = {}
    name_match = re.search(r'\.name\s*=\s*"([^"]*)"', body)
    if name_match and "name" in field_map:
        values[field_map["name"]] = name_match.group(1).strip()
    for source_key, target_key in field_map.items():
        if source_key == "name":
            continue
        match = re.search(rf"\.{source_key}\s*=\s*([^,}}\n]+)", body)
        if match:
            values[target_key] = parse_value(match.group(1).strip())
    return values


def parse_value(raw: str) -> Any:
    raw = raw.split("//", 1)[0].strip()
    if raw in MOTOR_ENUMS:
        return MOTOR_ENUMS[raw]
    try:
        if "/" in raw or "*" in raw or "+" in raw or "-" in raw[1:]:
            return safe_eval_number(raw)
        if any(ch in raw for ch in ".eE"):
            return float(raw)
        return int(raw, 0)
    except ValueError:
        return raw


def safe_eval_number(expr: str) -> int | float:
    operators = {
        ast.Add: operator.add,
        ast.Sub: operator.sub,
        ast.Mult: operator.mul,
        ast.Div: operator.truediv,
        ast.USub: operator.neg,
        ast.UAdd: operator.pos,
    }

    def eval_node(node):
        if isinstance(node, ast.Expression):
            return eval_node(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return node.value
        if isinstance(node, ast.UnaryOp) and type(node.op) in operators:
            return operators[type(node.op)](eval_node(node.operand))
        if isinstance(node, ast.BinOp) and type(node.op) in operators:
            return operators[type(node.op)](eval_node(node.left), eval_node(node.right))
        raise ValueError("unsupported expression")

    return eval_node(ast.parse(expr, mode="eval"))


def motor_description(values: dict[str, Any]) -> str:
    parts = []
    if "motor.u" in values:
        parts.append(f"{values['motor.u']:g}V")
    if "motor.kv" in values:
        parts.append(f"{values['motor.kv']:g}KV")
    if "motor.rpm" in values:
        parts.append(f"{values['motor.rpm']}rpm")
    if "motor.phR" in values:
        parts.append(f"R={values['motor.phR']:g}")
    if "motor.phL" in values:
        parts.append(f"L={values['motor.phL']:g}")
    return ", ".join(parts)


def pcb_description(values: dict[str, Any]) -> str:
    parts = []
    if "pcb.focDriver" in values:
        parts.append("6PWM" if values["pcb.focDriver"] == 1 else "3PWM")
    if "pcb.shuntR" in values:
        parts.append(f"shunt={values['pcb.shuntR']:g}")
    if "pcb.opAmpGain" in values:
        parts.append(f"gain={values['pcb.opAmpGain']:g}")
    if "pcb.supplyGain" in values:
        parts.append(f"supplyGain={values['pcb.supplyGain']:g}")
    if "pcb.tempSensor" in values:
        parts.append(f"temp={int(bool(values['pcb.tempSensor']))}")
    return ", ".join(parts)


def motion_description(values: dict[str, Any]) -> str:
    parts = []
    if "motion.velocityLimit" in values:
        parts.append(f"vlim {values['motion.velocityLimit']:g}")
    if "motion.pidVelocityP" in values:
        parts.append(f"P {values['motion.pidVelocityP']:g}")
    if "motion.pidVelocityI" in values:
        parts.append(f"I {values['motion.pidVelocityI']:g}")
    if "motion.lpfVelocityTf" in values:
        parts.append(f"LPF {values['motion.lpfVelocityTf']:g}")
    return ", ".join(parts)
