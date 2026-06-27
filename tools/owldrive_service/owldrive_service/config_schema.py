from __future__ import annotations

import struct
from dataclasses import dataclass
from typing import Any, Optional


PROFILE_SIZE = 418


@dataclass(frozen=True)
class ConfigField:
    path: str
    label: str
    group: str
    offset: int
    type: str
    size: int
    minimum: Optional[float] = None
    maximum: Optional[float] = None
    options: Optional[list[str]] = None
    reboot: bool = False
    precision: Optional[int] = None
    visible: bool = True


MOTION_CONTROL = ["torque", "velocity", "angle", "velocity_openloop", "angle_openloop"]
TORQUE_CONTROL = ["voltage", "dc_current", "foc_current"]
FOC_MODULATION = ["SinePWM", "SpaceVectorPWM", "Trapezoid_120", "Trapezoid_150", "Off"]
FOC_SENSOR = ["None", "Hall(3ch)", "AS5600_I2C", "AS5048_I2C", "Enc(1ch)", "Enc(2ch)", "Flux(3ph)"]
FOC_MOTOR = ["BLDC-3-phase", "DC", "BLDC-1-phase", "3xDC"]
FOC_DRIVER = ["BLDC 3PWM", "BLDC 6PWM"]
MISC_SENSOR = ["None", "IMU_MPU6050", "DIST_DYPA22", "DIST_VL53L0X", "DIST_VL53L1X"]
SERIAL_PROTOCOL = ["off", "human readable", "telemetry"]


FIELDS: list[ConfigField] = [
    ConfigField("name", "Profile name", "profile", 0, "string", 51),
    ConfigField("motor.name", "Motor preset name", "motor", 51, "string", 51),
    ConfigField("motor.typ", "Motor type", "motor", 102, "u8", 1, options=FOC_MOTOR, reboot=True),
    ConfigField("motor.u", "Supply voltage", "motor", 103, "float", 4, 0, 100),
    ConfigField("motor.sen", "Sensor type", "motor", 107, "u8", 1, options=FOC_SENSOR, reboot=True),
    ConfigField("motor.kv", "KV rating", "motor", 111, "float", 4, 1, 9000),
    ConfigField("motor.rpm", "Nominal RPM", "motor", 115, "u16", 2, 1, 20000),
    ConfigField("motor.pp", "Pole pairs", "motor", 117, "u8", 1, 0, 100),
    ConfigField("motor.alignV", "Sensor align voltage", "motor", 119, "float", 4, 0, 100),
    ConfigField("motor.senDirCW", "Sensor direction CW", "motor", 123, "bool", 1),
    ConfigField("motor.zeroOfs", "Zero electric offset", "motor", 127, "float", 4, -6.283, 6.283),
    ConfigField("motor.phR", "Phase resistance", "motor", 131, "float", 4, 0, 100),
    ConfigField("motor.phL", "Phase inductance", "motor", 135, "float", 4, 0, 1),
    ConfigField("motor.pwmFreq", "PWM frequency", "motor", 139, "i32", 4, 5000, 66000, reboot=True),
    ConfigField("motor.deadZone", "Deadzone / deadtime", "motor", 143, "float", 4, 0, 1, reboot=True),
    ConfigField("motor.ppr", "Encoder PPR", "motor", 147, "float", 4, 0, 10000),
    ConfigField("motor.pullUp", "Internal pullup", "motor", 151, "bool", 1),

    ConfigField("pcb.name", "PCB preset name", "pcb", 155, "string", 51, visible=False),
    ConfigField("pcb.focDriver", "FOC driver", "pcb", 206, "u8", 1, options=FOC_DRIVER, reboot=True),
    ConfigField("pcb.pinMotorAH", "Pin motor AH", "pcb", 207, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinMotorBH", "Pin motor BH", "pcb", 208, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinMotorCH", "Pin motor CH", "pcb", 209, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinMotorAL", "Pin motor AL", "pcb", 210, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinMotorBL", "Pin motor BL", "pcb", 211, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinMotorCL", "Pin motor CL", "pcb", 212, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinMotorEn", "Pin motor enable", "pcb", 213, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinHallA", "Pin Hall A", "pcb", 214, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinHallB", "Pin Hall B", "pcb", 215, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinHallC", "Pin Hall C", "pcb", 216, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSensA", "Pin sense A", "pcb", 217, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSensB", "Pin sense B", "pcb", 218, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSensC", "Pin sense C", "pcb", 219, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSupply", "Pin supply", "pcb", 220, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSDA", "Pin SDA", "pcb", 221, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSCL", "Pin SCL", "pcb", 222, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinUartTx", "Pin UART TX", "pcb", 223, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinUartRx", "Pin UART RX", "pcb", 224, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSpiTx", "Pin SPI TX", "pcb", 225, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSpiRx", "Pin SPI RX", "pcb", 226, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSpiSCK", "Pin SPI SCK", "pcb", 227, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinSpiCS", "Pin SPI CS", "pcb", 228, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinEnableIn", "Pin enable input", "pcb", 229, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinPwmIn", "Pin PWM input", "pcb", 230, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinOdoOut", "Pin odometry output", "pcb", 231, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinFan", "Pin fan", "pcb", 232, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinDirIn", "Pin direction input", "pcb", 233, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinBrkIn", "Pin brake input", "pcb", 234, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinFaultOut", "Pin fault output", "pcb", 235, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinPwmTestOut", "Pin PWM test output", "pcb", 236, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.pinLED", "Pin LED", "pcb", 237, "u8", 1, 0, 255, reboot=True, visible=False),
    ConfigField("pcb.shuntR", "Shunt R", "pcb", 243, "float", 4, 0, 5, reboot=True),
    ConfigField("pcb.opAmpGain", "OpAmp gain", "pcb", 247, "float", 4, -100, 100, reboot=True),
    ConfigField("pcb.opAmpOfs", "OpAmp offset", "pcb", 251, "float", 4, -5, 5, reboot=True),
    ConfigField("pcb.supplyGain", "Supply gain", "pcb", 255, "float", 4, -100, 100, reboot=True),
    ConfigField("pcb.overCurrent", "Overcurrent limit", "pcb", 259, "float", 4, 0, 1000),
    ConfigField("pcb.overVoltage", "Overvoltage protection", "pcb", 263, "bool", 1),
    ConfigField("pcb.underVoltage", "Undervoltage limit", "pcb", 267, "float", 4, 0, 80),
    ConfigField("pcb.emfProtect", "EMF protection", "pcb", 271, "bool", 1),
    ConfigField("pcb.tempSensor", "Temperature sensor", "pcb", 238, "bool", 1, reboot=True),
    ConfigField("pcb.misc", "Misc sensor", "pcb", 239, "u8", 1, options=MISC_SENSOR, reboot=True),

    ConfigField("torqueCtl", "Torque control", "motion", 275, "u8", 1, options=TORQUE_CONTROL),
    ConfigField("motionCtl", "Motion control", "motion", 276, "u8", 1, options=MOTION_CONTROL),
    ConfigField("motion.pAngleP", "Angle P", "motion", 329, "float", 4, 0, 1000),
    ConfigField("motion.velocityLimit", "Velocity limit", "motion", 333, "float", 4, 0, 4000),
    ConfigField("motion.pidVelocityP", "Velocity PID P", "motion", 337, "float", 4, 0, 1000),
    ConfigField("motion.pidVelocityI", "Velocity PID I", "motion", 341, "float", 4, 0, 1000),
    ConfigField("motion.pidVelocityD", "Velocity PID D", "motion", 345, "float", 4, 0, 1000),
    ConfigField("motion.pidVelocityRamp", "Velocity PID ramp", "motion", 349, "float", 4, 0, 100000),
    ConfigField("motion.lpfVelocityTf", "Velocity LPF Tf", "motion", 353, "float", 4, 0, 100),
    ConfigField("voltLimit", "Voltage limit", "motion", 393, "float", 4, 0, 100),
    ConfigField("currLimit", "Current limit", "motion", 397, "float", 4, 0, 50),
    ConfigField("focModulation", "FOC modulation", "motion", 401, "u8", 1, options=FOC_MODULATION),
    ConfigField("usePhR", "Use phase resistance", "motion", 402, "bool", 1),
    ConfigField("usePhL", "Use phase inductance", "motion", 403, "bool", 1),
    ConfigField("useKV", "Use KV", "motion", 404, "bool", 1),

    ConfigField("canSpeed", "CAN speed", "can", 357, "i32", 4, 5, 1000, reboot=True),
    ConfigField("canMsgId", "CAN message ID", "can", 361, "i32", 4, 0, 2047, reboot=True),
    ConfigField("canNodeId", "CAN node ID", "can", 365, "u8", 1, 1, 62, reboot=True),
    ConfigField("canBroadcastMask", "Broadcast mask", "can", 366, "i32", 4, 0, 255),
    ConfigField("canFollowNodeId", "Follow node ID", "can", 370, "u8", 1, 0, 62),
    ConfigField("canFollowValue", "Follow value", "can", 371, "i32", 4, 0, 5),

    ConfigField("fanDuty", "Fan duty", "profile", 375, "float", 4, 0, 1),
    ConfigField("align", "Auto-align at start", "profile", 379, "bool", 1, reboot=True),
    ConfigField("invDir", "Invert motor direction", "profile", 380, "bool", 1),
    ConfigField("pinEndSwitchIn", "End-switch GPIO", "profile", 381, "u8", 1, 0, 30, reboot=True),
    ConfigField("endSwitchActiveLow", "End-switch active-low", "profile", 382, "bool", 1),
    ConfigField("endSwitchAllowPosDirTarget", "Allow positive target at end-switch", "profile", 383, "bool", 1),
    ConfigField("endSwitchAllowNegDirTarget", "Allow negative target at end-switch", "profile", 384, "bool", 1),
    ConfigField("serialOutProt", "Serial output protocol", "profile", 385, "u8", 1, options=SERIAL_PROTOCOL),
    ConfigField("enableIn", "Enable input", "profile", 386, "bool", 1),
    ConfigField("pwmIn", "PWM input", "profile", 387, "bool", 1),
    ConfigField("pwmTestOut", "PWM test output", "profile", 388, "bool", 1),
    ConfigField("pwmTestOutFreq", "PWM test frequency", "profile", 389, "i32", 4, 1, 100000),
    ConfigField("comTimeout", "Communication timeout", "profile", 405, "u8", 1, 0, 600),
    ConfigField("uartBaudRate", "UART baud rate", "profile", 406, "i32", 4, 110, 921600, reboot=True),
    ConfigField("databaseVer", "Database version", "profile", 414, "i32", 4, visible=False),
]


FIELD_BY_PATH = {field.path: field for field in FIELDS}

FIELD_PRECISION = {
    "motor.u": 2,
    "motor.kv": 2,
    "motor.alignV": 2,
    "motor.zeroOfs": 4,
    "motor.phR": 4,
    "motor.phL": 5,
    "motor.deadZone": 4,
    "motor.ppr": 2,
    "pcb.shuntR": 4,
    "pcb.opAmpGain": 4,
    "pcb.opAmpOfs": 4,
    "pcb.supplyGain": 4,
    "pcb.overCurrent": 4,
    "pcb.underVoltage": 2,
    "motion.pAngleP": 4,
    "motion.velocityLimit": 4,
    "motion.pidVelocityP": 4,
    "motion.pidVelocityI": 4,
    "motion.pidVelocityD": 4,
    "motion.pidVelocityRamp": 4,
    "motion.lpfVelocityTf": 4,
    "voltLimit": 4,
    "currLimit": 4,
    "fanDuty": 4,
}


def decode_field(data: bytes, field: ConfigField) -> Any:
    raw = data[field.offset : field.offset + field.size]
    if field.type == "string":
        return raw.split(b"\x00", 1)[0].decode("utf-8", "replace").rstrip()
    if field.type == "bool":
        return raw[0] != 0
    if field.type == "u8":
        return raw[0]
    if field.type == "u16":
        return struct.unpack("<H", raw)[0]
    if field.type == "i32":
        return struct.unpack("<i", raw)[0]
    if field.type == "float":
        return struct.unpack("<f", raw)[0]
    raise ValueError(f"unsupported field type {field.type}")


def encode_field(value: Any, field: ConfigField) -> bytes:
    if field.type == "string":
        text = str(value).encode("utf-8")[: field.size - 1]
        return text + b"\x00" * (field.size - len(text))
    if field.type == "bool":
        return bytes([1 if bool(value) else 0])
    if field.type == "u8":
        return bytes([int(value) & 0xFF])
    if field.type == "u16":
        return struct.pack("<H", int(value))
    if field.type == "i32":
        return struct.pack("<i", int(value))
    if field.type == "float":
        return struct.pack("<f", float(value))
    raise ValueError(f"unsupported field type {field.type}")


def decode_config(data: bytes) -> dict[str, Any]:
    return {field.path: decode_field(data, field) for field in FIELDS}


def schema_json() -> list[dict[str, Any]]:
    return [
        {
            "path": field.path,
            "label": field.label,
            "group": field.group,
            "type": field.type,
            "min": field.minimum,
            "max": field.maximum,
            "options": field.options,
            "reboot": field.reboot,
            "precision": FIELD_PRECISION.get(field.path, field.precision),
            "visible": field.visible,
        }
        for field in FIELDS
    ]
