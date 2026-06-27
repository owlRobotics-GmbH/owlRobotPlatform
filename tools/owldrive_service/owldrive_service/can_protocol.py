from __future__ import annotations

import asyncio
import enum
import glob
import os
import struct
import time
from dataclasses import dataclass
from typing import Literal

try:
    import can
except ImportError:  # pragma: no cover - allows protocol tests without python-can
    can = None


CAN_MSG_ID = 300
CAN_NODE_ID_MIN = 1
CAN_NODE_ID_MAX = 62
CAN_NODE_ID_BROADCAST = 63
CAN_NODE_ID_HOST = 62


class CanCommand(enum.IntEnum):
    info = 0
    request = 1
    set = 2
    save = 3
    ack = 4


class CanValue(enum.IntEnum):
    target = 0
    voltage = 1
    current = 2
    velocity = 3
    angle = 4
    motion_ctl_mode = 5
    cfg_mem = 6
    motor_enable = 7
    pAngleP = 8
    velocityLimit = 9
    pidVelocityP = 10
    pidVelocityI = 11
    pidVelocityD = 12
    pidVelocityRamp = 13
    lpfVelocityTf = 14
    error = 15
    upload_firmware = 16
    firmware_crc = 17
    firmware_ver = 18
    broadcast_rx_enable = 19
    fifo_target = 20
    endswitch_allow_pos_neg_dtargets = 21
    reboot = 22
    endswitch = 23
    fifo_clock = 24
    control_error = 25
    fifo_target_ack_result_val = 26
    detected_supply_voltage = 27
    angle_add = 28
    pwm_speed = 29
    odo_ticks = 30
    misc_sensor1 = 31
    misc_sensor2 = 32
    total_current = 33
    voltageLimit = 34


FLOAT_VALUES = {
    CanValue.target,
    CanValue.voltage,
    CanValue.current,
    CanValue.velocity,
    CanValue.angle,
    CanValue.control_error,
    CanValue.detected_supply_voltage,
    CanValue.angle_add,
    CanValue.pwm_speed,
    CanValue.misc_sensor1,
    CanValue.misc_sensor2,
    CanValue.total_current,
    CanValue.pAngleP,
    CanValue.velocityLimit,
    CanValue.pidVelocityP,
    CanValue.pidVelocityI,
    CanValue.pidVelocityD,
    CanValue.pidVelocityRamp,
    CanValue.lpfVelocityTf,
    CanValue.voltageLimit,
}


@dataclass(frozen=True)
class OwldriveFrame:
    source: int
    dest: int
    cmd: CanCommand
    value: CanValue
    data: bytes

    @classmethod
    def decode(cls, payload: bytes) -> "OwldriveFrame":
        if len(payload) != 8:
            raise ValueError("owlDrive frames must contain exactly 8 data bytes")
        node_word = payload[0] | (payload[1] << 8)
        return cls(
            source=node_word & 0x3F,
            dest=(node_word >> 6) & 0x3F,
            cmd=CanCommand(payload[2]),
            value=CanValue(payload[3]),
            data=payload[4:8],
        )

    def encode(self) -> bytes:
        node_word = (self.source & 0x3F) | ((self.dest & 0x3F) << 6)
        return bytes(
            [
                node_word & 0xFF,
                (node_word >> 8) & 0xFF,
                int(self.cmd),
                int(self.value),
                *self.data[:4].ljust(4, b"\x00"),
            ]
        )


def pack_float(value: float) -> bytes:
    return struct.pack("<f", float(value))


def unpack_float(data: bytes) -> float:
    return struct.unpack("<f", data)[0]


def pack_int(value: int) -> bytes:
    return struct.pack("<i", int(value))


def unpack_int(data: bytes) -> int:
    return struct.unpack("<i", data)[0]


def pack_byte(value: int) -> bytes:
    return bytes([int(value) & 0xFF, 0, 0, 0])


def pack_offset_byte(offset: int, value: int) -> bytes:
    return struct.pack("<HBx", int(offset) & 0xFFFF, int(value) & 0xFF)


def pack_large_offset_byte(offset: int, value: int) -> bytes:
    return bytes([(offset >> 16) & 0xFF, (offset >> 8) & 0xFF, offset & 0xFF, value & 0xFF])


def unpack_large_offset(data: bytes) -> int:
    return (data[0] << 16) | (data[1] << 8) | data[2]


def decode_value(value: CanValue, data: bytes) -> int | float:
    if value in FLOAT_VALUES:
        return unpack_float(data)
    if value in {CanValue.firmware_crc, CanValue.firmware_ver}:
        return unpack_int(data)
    return data[0]


def encode_value(value: CanValue, raw: int | float) -> bytes:
    if value in FLOAT_VALUES:
        return pack_float(float(raw))
    if value in {CanValue.firmware_crc, CanValue.firmware_ver}:
        return pack_int(int(raw))
    return pack_byte(int(raw))


def socketcan_interfaces() -> list[str]:
    interfaces = []
    for path in glob.glob("/sys/class/net/can*"):
        name = os.path.basename(path)
        if name.startswith("can"):
            interfaces.append(name)
    return sorted(interfaces)


class OwldriveCanBus:
    def __init__(self, channel: str, bitrate: int = 1_000_000, host_node: int = CAN_NODE_ID_HOST, msg_id: int = CAN_MSG_ID):
        if can is None:
            raise RuntimeError("python-can is not installed")
        self.channel = channel
        self.bitrate = bitrate
        self.host_node = host_node
        self.msg_id = msg_id
        self._bus = can.Bus(interface="socketcan", channel=channel, bitrate=bitrate)
        self._lock = asyncio.Lock()

    def close(self) -> None:
        self._bus.shutdown()

    def _send(self, frame: OwldriveFrame) -> None:
        self._bus.send(can.Message(arbitration_id=self.msg_id, data=frame.encode(), is_extended_id=False))

    def _drain_pending(self, limit: int = 256) -> None:
        for _ in range(limit):
            if self._bus.recv(timeout=0) is None:
                return

    def _recv_matching(self, predicate, timeout: float):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            msg = self._bus.recv(timeout=max(0.0, end - time.monotonic()))
            if msg is None or msg.arbitration_id != self.msg_id or len(msg.data) != 8:
                continue
            frame = OwldriveFrame.decode(bytes(msg.data))
            if predicate(frame):
                return frame
        return None

    async def request(self, node_id: int, value: CanValue, timeout: float = 0.05) -> int | float | None:
        async with self._lock:
            req = OwldriveFrame(self.host_node, node_id, CanCommand.request, value, b"\x00\x00\x00\x00")
            return await asyncio.to_thread(self._request_sync, req, node_id, value, timeout)

    def _request_sync(self, req: OwldriveFrame, node_id: int, value: CanValue, timeout: float):
        self._drain_pending()
        self._send(req)
        frame = self._recv_matching(
            lambda f: f.cmd == CanCommand.info and f.value == value and f.source == node_id,
            timeout,
        )
        return None if frame is None else decode_value(value, frame.data)

    async def request_cfg_byte(self, node_id: int, offset: int, timeout: float = 0.05) -> int | None:
        async with self._lock:
            req = OwldriveFrame(self.host_node, node_id, CanCommand.request, CanValue.cfg_mem, pack_offset_byte(offset, 0))
            return await asyncio.to_thread(self._request_cfg_byte_sync, req, node_id, timeout)

    def _request_cfg_byte_sync(self, req: OwldriveFrame, node_id: int, timeout: float) -> int | None:
        self._drain_pending()
        self._send(req)
        frame = self._recv_matching(
            lambda f: f.cmd == CanCommand.info and f.value == CanValue.cfg_mem and f.source == node_id,
            timeout,
        )
        return None if frame is None else frame.data[2]

    async def read_config(self, node_id: int, size: int, timeout: float = 0.05) -> bytes:
        data = bytearray()
        for offset in range(size):
            value = await self.request_cfg_byte(node_id, offset, timeout=timeout)
            if value is None:
                raise TimeoutError(f"config read timeout at offset {offset}")
            data.append(value)
        return bytes(data)

    async def set_cfg_byte(self, node_id: int, offset: int, value: int, timeout: float = 0.1) -> bool:
        async with self._lock:
            frame = OwldriveFrame(self.host_node, node_id, CanCommand.set, CanValue.cfg_mem, pack_offset_byte(offset, value))
            return await asyncio.to_thread(self._set_sync, frame, node_id, CanValue.cfg_mem, True, timeout)

    async def write_config_bytes(self, node_id: int, changes: dict[int, int]) -> None:
        for offset, value in sorted(changes.items()):
            ok = await self.set_cfg_byte(node_id, offset, value)
            if not ok:
                raise TimeoutError(f"config write ack timeout at offset {offset}")

    async def set_value(self, node_id: int, value: CanValue, raw: int | float, wait_ack: bool = False, timeout: float = 0.05):
        async with self._lock:
            data = encode_value(value, raw)
            frame = OwldriveFrame(self.host_node, node_id, CanCommand.set, value, data)
            return await asyncio.to_thread(self._set_sync, frame, node_id, value, wait_ack, timeout)

    def _set_sync(self, frame: OwldriveFrame, node_id: int, value: CanValue, wait_ack: bool, timeout: float):
        self._drain_pending()
        self._send(frame)
        if not wait_ack:
            return True
        ack = self._recv_matching(
            lambda f: f.cmd == CanCommand.ack and f.value == value and f.source == node_id,
            timeout,
        )
        return ack is not None

    async def scan(self, first: int = CAN_NODE_ID_MIN, last: int = CAN_NODE_ID_MAX) -> list[dict]:
        found = []
        for node_id in range(first, last + 1):
            for _ in range(2):
                started = time.monotonic()
                version = await self.request(node_id, CanValue.firmware_ver, timeout=0.02)
                if version is not None:
                    found.append({"node_id": node_id, "firmware_version": version, "answer_ms": round((time.monotonic() - started) * 1000, 2)})
                    break
                await asyncio.sleep(0.005)
        return found

    async def telemetry(self, node_id: int) -> dict:
        values = {}
        for key, val in {
            "target": CanValue.target,
            "voltage": CanValue.voltage,
            "current": CanValue.current,
            "velocity": CanValue.velocity,
            "angle": CanValue.angle,
            "control_error": CanValue.control_error,
            "supply_voltage": CanValue.detected_supply_voltage,
            "total_current": CanValue.total_current,
            "error": CanValue.error,
        }.items():
            values[key] = await self.request(node_id, val, timeout=0.02)
        return values

    async def save_config(self, node_id: int, reboot: bool = False) -> bool:
        data = pack_byte(1 if reboot else 0)
        async with self._lock:
            frame = OwldriveFrame(self.host_node, node_id, CanCommand.save, CanValue.cfg_mem, data)
            await asyncio.to_thread(self._send, frame)
        return True

    async def upload_firmware(self, node_id: int, firmware: bytes, progress_cb=None) -> int:
        crc = sum(firmware) & 0xFFFFFFFF
        for offset, byte in enumerate(firmware):
            payload = pack_large_offset_byte(offset, byte)
            frame = OwldriveFrame(self.host_node, node_id, CanCommand.set, CanValue.upload_firmware, payload)
            async with self._lock:
                ok = await asyncio.to_thread(self._set_upload_byte_sync, frame, node_id, offset)
            if not ok:
                raise TimeoutError(f"firmware upload ack timeout at offset {offset}")
            if progress_cb and (offset % 128 == 0 or offset + 1 == len(firmware)):
                progress_cb(offset + 1, len(firmware))
        async with self._lock:
            save = OwldriveFrame(self.host_node, node_id, CanCommand.save, CanValue.upload_firmware, pack_int(crc))
            await asyncio.to_thread(self._send, save)
        return crc

    def _set_upload_byte_sync(self, frame: OwldriveFrame, node_id: int, offset: int) -> bool:
        for _ in range(3):
            self._drain_pending()
            self._send(frame)
            ack_timeout = 10.0 if offset == 0 else 2.0
            ack = self._recv_matching(
                lambda f: f.cmd == CanCommand.ack
                and f.value == CanValue.upload_firmware
                and f.source == node_id
                and unpack_large_offset(f.data) == offset,
                ack_timeout,
            )
            if ack:
                return True
        return False


BusMode = Literal["socketcan"]
