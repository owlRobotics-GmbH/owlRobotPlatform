from __future__ import annotations

import asyncio
import itertools
import json
import os
import pwd
import re
import struct
import subprocess
from datetime import datetime, timezone
from pathlib import Path as FsPath
from typing import Annotated, Any, Dict, Optional, Union

from fastapi import FastAPI, File, Form, HTTPException, Path, UploadFile, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from pydantic_settings import BaseSettings

from .can_protocol import CanCommand, CanValue, OwldriveCanBus, OwldriveFrame, socketcan_interfaces
from .config_schema import FIELD_BY_PATH, PROFILE_SIZE, SUPPORTED_DATABASE_VERSIONS, decode_config, decode_database_version, encode_field, schema_json
from .firmware_images import download_url, firmware_payload, list_github_images, list_local_images
from .presets import (
    find_motion_preset,
    find_motor_preset,
    find_pcb_preset,
    find_preset,
    list_motion_presets,
    list_motor_presets,
    list_pcb_presets,
    public_presets,
)


SERVICE_ROOT = FsPath(__file__).resolve().parents[1]
TOOLS_ROOT = SERVICE_ROOT.parents[0]
STATIC = SERVICE_ROOT / "static"
DATA_DIR = SERVICE_ROOT / "data"
STOPPED_CAN_SERVICES = DATA_DIR / "stopped-can-services.json"
RC_SWITCH_CONFLICTING_SERVICE = "owl-wido-robot.service"


def ensure_supported_database_version(raw_config: bytes) -> int:
    database_version = decode_database_version(raw_config)
    if database_version not in SUPPORTED_DATABASE_VERSIONS:
        supported = ", ".join(str(version) for version in sorted(SUPPORTED_DATABASE_VERSIONS))
        raise HTTPException(
            status_code=409,
            detail=f"Unsupported database version {database_version}. Settings are disabled; supported database versions: {supported}.",
        )
    return database_version


class Settings(BaseSettings):
    can_channel: str = "can0"
    can_bitrate: int = 1_000_000
    host_node_id: int = 62
    can_msg_id: int = 300
    rc_switch_msg_id: int = 600
    rc_switch_node_id: int = 59
    owl_controller_msg_id: int = 500
    owl_controller_node_id: int = 60
    service_user: str = ""

    class Config:
        env_prefix = "OWLDRIVE_"


class SetValueRequest(BaseModel):
    value: CanValue
    data: Union[float, int]
    wait_ack: bool = False


class SaveConfigRequest(BaseModel):
    reboot: bool = False


class ConfigPatchRequest(BaseModel):
    values: Dict[str, Any]
    save: bool = False
    reboot: bool = False


class RcCalibrationRequest(BaseModel):
    action: str


class RcOutputRequest(BaseModel):
    channel: int
    pulse_us: int


RC_SWITCH_CONFIG_SIZE = 168
RC_SWITCH_CONFIG_FIELDS = {
    "pwmMinimumUs": (4, "H", 500, 2500),
    "pwmNeutralUs": (6, "H", 500, 2500),
    "pwmMaximumUs": (8, "H", 500, 2500),
    "modeRcMaximumUs": (10, "H", 500, 2500),
    "modeCanMinimumUs": (12, "H", 500, 2500),
    "maximumVelocity": (16, "f", 0.01, 1000.0),
    "maximumTorque": (20, "f", 0.0, 1.0),
    **{f"node{node}.pidP": (24 + 4 * (node - 1), "f", 0.0, 1000.0) for node in range(1, 4)},
    **{f"node{node}.pidI": (36 + 4 * (node - 1), "f", 0.0, 1000.0) for node in range(1, 4)},
    **{f"node{node}.pidD": (48 + 4 * (node - 1), "f", 0.0, 1000.0) for node in range(1, 4)},
    **{f"node{node}.pidRamp": (60 + 4 * (node - 1), "f", 0.0, 100000.0) for node in range(1, 4)},
    **{f"node{node}.outputInverted": (72 + (node - 1), "B", 0, 1) for node in range(1, 4)},
    **{f"input{channel}.minimumUs": (76 + 2 * (channel - 1), "H", 800, 2200) for channel in range(1, 9)},
    **{f"input{channel}.neutralUs": (92 + 2 * (channel - 1), "H", 800, 2200) for channel in range(1, 9)},
    **{f"input{channel}.maximumUs": (108 + 2 * (channel - 1), "H", 800, 2200) for channel in range(1, 9)},
    **{f"input{channel}.deadzoneUs": (124 + 2 * (channel - 1), "H", 0, 250) for channel in range(1, 9)},
    **{f"input{channel}.inverted": (140 + (channel - 1), "B", 0, 1) for channel in range(1, 9)},
    "inputTimeoutUs": (148, "I", 10000, 5000000),
    "inputFailsafeMode": (152, "B", 0, 1),
    **{f"output{channel}.mode": (153 + (channel - 4), "B", 0, 2) for channel in range(4, 9)},
    **{f"output{channel}.inverted": (158 + (channel - 4), "B", 0, 1) for channel in range(4, 9)},
}


class ApplyPresetRequest(BaseModel):
    preset_id: str
    save: bool = False
    reboot: bool = False
    keep_node_id: bool = True


class ApplyMotorPresetRequest(BaseModel):
    preset_id: str
    save: bool = False
    reboot: bool = False


class ApplyMotionPresetRequest(BaseModel):
    preset_id: str
    save: bool = False
    reboot: bool = False


class ApplyPcbPresetRequest(BaseModel):
    preset_id: str
    save: bool = False
    reboot: bool = False


class FlashImageRequest(BaseModel):
    image_id: str


class MultiFlashImageRequest(BaseModel):
    image_id: str
    node_ids: list[int]


class ServiceActionRequest(BaseModel):
    action: str
    scope: str = "user"
    owner: Optional[str] = None


class FlashJob(BaseModel):
    id: int
    node_id: int
    filename: str
    done: int = 0
    total: int
    state: str = "queued"
    error: Optional[str] = None
    crc: Optional[int] = None
    cancel_requested: bool = False


settings = Settings()
app = FastAPI(title="owlDrive Service", version="0.1.0")
app.mount("/static", StaticFiles(directory=STATIC), name="static")

bus: OwldriveCanBus | None = None
rc_switch_bus: OwldriveCanBus | None = None
owl_controller_bus: OwldriveCanBus | None = None
job_counter = itertools.count(1)
jobs: Dict[int, FlashJob] = {}
exclusive_job_lock = asyncio.Lock()


def _validate_flash_nodes(node_ids: list[int]) -> list[int]:
    nodes = sorted(set(int(node_id) for node_id in node_ids))
    if not nodes:
        raise HTTPException(status_code=400, detail="no devices checked")
    invalid = [node_id for node_id in nodes if node_id < 1 or node_id > 62]
    if invalid:
        raise HTTPException(status_code=400, detail=f"invalid node IDs: {invalid}")
    return nodes


def _create_flash_job(node_id: int, filename: str, total: int) -> FlashJob:
    job_id = next(job_counter)
    job = FlashJob(id=job_id, node_id=node_id, filename=filename, total=total)
    jobs[job_id] = job
    return job


def _start_flash_job(job: FlashJob, upload_coro_factory) -> None:
    async def runner():
        async with exclusive_job_lock:
            if job.cancel_requested:
                job.state = "cancelled"
                job.error = "cancelled by user"
                return
            job.state = "running"

            def progress(done: int, total: int):
                if job.cancel_requested:
                    raise asyncio.CancelledError()
                job.done = max(job.done, done)
                job.total = total

            try:
                job.crc = await upload_coro_factory(progress)
                if job.cancel_requested:
                    job.state = "cancelled"
                    job.error = "cancelled by user"
                else:
                    job.done = job.total
                    job.state = "done"
            except asyncio.CancelledError:
                job.state = "cancelled"
                job.error = "cancelled by user"
            except Exception as exc:
                if job.cancel_requested:
                    job.state = "cancelled"
                    job.error = "cancelled by user"
                else:
                    job.error = str(exc)
                    job.state = "failed"

    asyncio.create_task(runner())


async def _upload_rc_switch(firmware: bytes, progress) -> int:
    """Flash ID 600 without continuous, higher-priority Wido ID 300 traffic."""
    status = await asyncio.to_thread(
        _run_systemctl, "system", ["is-active", RC_SWITCH_CONFLICTING_SERVICE], 5
    )
    was_active = status.returncode == 0
    if was_active:
        stopped = await asyncio.to_thread(
            _run_systemctl, "system", ["stop", RC_SWITCH_CONFLICTING_SERVICE], 15
        )
        if stopped.returncode != 0:
            raise RuntimeError(stopped.stderr.strip() or "could not stop Wido CAN service")
        await asyncio.sleep(0.5)
    try:
        return await get_rc_switch_bus().upload_firmware(
            settings.rc_switch_node_id, firmware, progress
        )
    finally:
        if was_active:
            started = await asyncio.to_thread(
                _run_systemctl, "system", ["start", RC_SWITCH_CONFLICTING_SERVICE], 15
            )
            if started.returncode != 0:
                raise RuntimeError(started.stderr.strip() or "could not restart Wido CAN service")


async def _upload_owl_controller(firmware: bytes, progress) -> int:
    """Flash the controller while suppressing competing CAN traffic."""
    status = await asyncio.to_thread(
        _run_systemctl, "system", ["is-active", RC_SWITCH_CONFLICTING_SERVICE], 5
    )
    was_active = status.returncode == 0
    if was_active:
        stopped = await asyncio.to_thread(
            _run_systemctl, "system", ["stop", RC_SWITCH_CONFLICTING_SERVICE], 15
        )
        if stopped.returncode != 0:
            raise RuntimeError(stopped.stderr.strip() or "could not stop Wido CAN service")
        await asyncio.sleep(0.5)
    try:
        return await get_owl_controller_bus().upload_firmware(
            settings.owl_controller_node_id, firmware, progress
        )
    finally:
        if was_active:
            started = await asyncio.to_thread(
                _run_systemctl, "system", ["start", RC_SWITCH_CONFLICTING_SERVICE], 15
            )
            if started.returncode != 0:
                raise RuntimeError(started.stderr.strip() or "could not restart Wido CAN service")


@app.on_event("startup")
async def startup():
    global bus, rc_switch_bus, owl_controller_bus
    if os.getenv("OWLDRIVE_DISABLE_CAN") == "1":
        return
    bus = OwldriveCanBus(
        channel=settings.can_channel,
        bitrate=settings.can_bitrate,
        host_node=settings.host_node_id,
        msg_id=settings.can_msg_id,
    )
    rc_switch_bus = OwldriveCanBus(
        channel=settings.can_channel,
        bitrate=settings.can_bitrate,
        host_node=settings.host_node_id,
        msg_id=settings.rc_switch_msg_id,
    )
    owl_controller_bus = OwldriveCanBus(
        channel=settings.can_channel,
        bitrate=settings.can_bitrate,
        host_node=settings.host_node_id,
        msg_id=settings.owl_controller_msg_id,
    )


@app.on_event("shutdown")
async def shutdown():
    if bus:
        bus.close()
    if rc_switch_bus:
        rc_switch_bus.close()
    if owl_controller_bus:
        owl_controller_bus.close()


def get_bus() -> OwldriveCanBus:
    if bus is None:
        raise HTTPException(status_code=503, detail="CAN bus is not available")
    return bus


def get_rc_switch_bus() -> OwldriveCanBus:
    if rc_switch_bus is None:
        raise HTTPException(status_code=503, detail="RC-Switch CAN bus is not available")
    return rc_switch_bus


def get_owl_controller_bus() -> OwldriveCanBus:
    if owl_controller_bus is None:
        raise HTTPException(status_code=503, detail="owlController CAN bus is not available")
    return owl_controller_bus


@app.get("/api/owl-controller")
async def owl_controller_status():
    started = asyncio.get_running_loop().time()
    version = await get_owl_controller_bus().request(
        settings.owl_controller_node_id, CanValue.firmware_ver, timeout=0.2
    )
    return {
        "online": version is not None,
        "node_id": settings.owl_controller_node_id,
        "message_id": settings.owl_controller_msg_id,
        "firmware_version": version,
        "answer_ms": round((asyncio.get_running_loop().time() - started) * 1000, 2),
    }


async def _controller_i2c_devices() -> list[dict[str, int | str]]:
    raw_count = await get_owl_controller_bus().request_raw(
        settings.owl_controller_node_id, CanValue.controller_i2c_count, timeout=0.3
    )
    if raw_count is None:
        raise HTTPException(status_code=504, detail="owlController I2C inventory timeout")
    names = {
        0x20: "PCF8574 I/O", 0x21: "PCF8574 I/O", 0x3C: "OLED display",
        0x3D: "OLED display", 0x48: "PCF8591 ADC/DAC", 0x68: "IMU / ADC",
        0x69: "IMU / ADC", 0x70: "TCA9548A I2C multiplexer",
    }
    devices = []
    for index in range(min(raw_count[0], 48)):
        raw = await get_owl_controller_bus().request_raw(
            settings.owl_controller_node_id, CanValue.controller_i2c_device,
            bytes([index, 0, 0, 0]), timeout=0.15
        )
        if raw is None or raw[3] != 1:
            continue
        channel, address = raw[1], raw[2]
        devices.append({
            "index": index, "channel": channel,
            "bus": "direct" if channel == 255 else f"TCA channel {channel}",
            "address": address, "address_hex": f"0x{address:02X}",
            "name": names.get(address, "Unknown I2C device"),
        })
    return devices


@app.get("/api/owl-controller/i2c")
async def owl_controller_i2c():
    devices = await _controller_i2c_devices()
    return {"node_id": settings.owl_controller_node_id, "devices": devices,
            "count": len(devices)}


@app.post("/api/owl-controller/i2c/scan")
async def scan_owl_controller_i2c():
    ok = await get_owl_controller_bus().set_raw(
        settings.owl_controller_node_id, CanValue.controller_i2c_scan,
        b"\x01\x00\x00\x00", wait_ack=True, timeout=0.5
    )
    if not ok:
        raise HTTPException(status_code=504, detail="owlController I2C scan acknowledgement timeout")
    await asyncio.sleep(1.2)
    devices = await _controller_i2c_devices()
    return {"node_id": settings.owl_controller_node_id, "devices": devices,
            "count": len(devices)}


@app.get("/api/rc-switch")
async def rc_switch_status():
    started = asyncio.get_running_loop().time()
    version = await get_rc_switch_bus().request(
        settings.rc_switch_node_id, CanValue.firmware_ver, timeout=0.2
    )
    return {
        "online": version is not None,
        "node_id": settings.rc_switch_node_id,
        "message_id": settings.rc_switch_msg_id,
        "firmware_version": version,
        "answer_ms": round((asyncio.get_running_loop().time() - started) * 1000, 2),
    }


def _decode_rc_switch_config(raw: bytes) -> dict[str, int | float]:
    values: dict[str, int | float] = {}
    for name, (offset, fmt, _minimum, _maximum) in RC_SWITCH_CONFIG_FIELDS.items():
        values[name] = struct.unpack_from("<" + fmt, raw, offset)[0]
    return values


@app.get("/api/rc-switch/config")
async def read_rc_switch_config():
    try:
        raw = await get_rc_switch_bus().read_config(
            settings.rc_switch_node_id, RC_SWITCH_CONFIG_SIZE, timeout=0.1
        )
    except TimeoutError as exc:
        raise HTTPException(status_code=504, detail=str(exc)) from exc
    return {"node_id": settings.rc_switch_node_id, "values": _decode_rc_switch_config(raw)}


@app.patch("/api/rc-switch/config")
async def patch_rc_switch_config(req: ConfigPatchRequest):
    async with exclusive_job_lock:
        try:
            current = bytearray(await get_rc_switch_bus().read_config(
                settings.rc_switch_node_id, RC_SWITCH_CONFIG_SIZE, timeout=0.1
            ))
            changes: dict[int, int] = {}
            for name, value in req.values.items():
                field = RC_SWITCH_CONFIG_FIELDS.get(name)
                if field is None:
                    raise HTTPException(status_code=400, detail=f"unknown RC-Switch config field: {name}")
                offset, fmt, minimum, maximum = field
                numeric = float(value)
                if numeric < minimum or numeric > maximum:
                    raise HTTPException(status_code=400, detail=f"{name} must be between {minimum} and {maximum}")
                encoded = struct.pack("<" + fmt, int(numeric) if fmt in {"H", "B", "I"} else numeric)
                for index, byte in enumerate(encoded):
                    if current[offset + index] != byte:
                        changes[offset + index] = byte
            if changes:
                await get_rc_switch_bus().write_config_bytes(settings.rc_switch_node_id, changes)
            if req.save or req.reboot:
                await get_rc_switch_bus().save_config(settings.rc_switch_node_id, reboot=req.reboot)
            return {"ok": True, "bytes_changed": len(changes), "values": req.values}
        except TimeoutError as exc:
            raise HTTPException(status_code=504, detail=str(exc)) from exc


@app.post("/api/rc-switch/calibration")
async def rc_switch_calibration(req: RcCalibrationRequest):
    action_value = {"start": 1, "finish": 2}.get(req.action)
    if action_value is None:
        raise HTTPException(status_code=400, detail="action must be start or finish")
    ok = await get_rc_switch_bus().set_value(
        settings.rc_switch_node_id, CanValue.rc_calibration,
        action_value, wait_ack=True, timeout=1.0
    )
    if not ok:
        raise HTTPException(status_code=504, detail="RC calibration acknowledgement timeout")
    return {"ok": True, "action": req.action}


@app.post("/api/rc-switch/output")
async def set_rc_switch_output(req: RcOutputRequest):
    if req.channel < 4 or req.channel > 8:
        raise HTTPException(status_code=400, detail="channel must be between 4 and 8")
    if req.pulse_us < 500 or req.pulse_us > 2500:
        raise HTTPException(status_code=400, detail="pulse_us must be between 500 and 2500")
    payload = bytes([req.channel, req.pulse_us & 0xFF,
                     (req.pulse_us >> 8) & 0xFF, 0])
    async with get_rc_switch_bus()._lock:
        frame = OwldriveFrame(settings.host_node_id, settings.rc_switch_node_id,
                              CanCommand.set, CanValue.rc_output, payload)
        ok = await asyncio.to_thread(get_rc_switch_bus()._set_sync, frame,
                                     settings.rc_switch_node_id,
                                     CanValue.rc_output, True, 0.5)
    if not ok:
        raise HTTPException(status_code=504, detail="RC output acknowledgement timeout")
    return {"ok": True, "channel": req.channel, "pulse_us": req.pulse_us}


@app.get("/")
async def index():
    return FileResponse(STATIC / "index.html")


@app.get("/api/interfaces")
async def interfaces():
    return {"interfaces": socketcan_interfaces(), "active": settings.can_channel}


@app.get("/api/firmware/images")
async def firmware_images():
    images = list_local_images(TOOLS_ROOT)
    try:
        images.extend(list_github_images())
    except Exception as exc:
        return {"images": [public_image(image) for image in images], "warning": str(exc)}
    return {"images": [public_image(image) for image in images]}


def public_image(image):
    return {
        "id": image.id,
        "name": image.name,
        "source": image.source,
        "size": image.size,
        "url": image.url,
    }


def _read_text(path: FsPath) -> str:
    try:
        return path.read_text(errors="replace").strip()
    except OSError:
        return ""


def _process_service(pid: int) -> str:
    cgroup = _read_text(FsPath("/proc") / str(pid) / "cgroup")
    matches = re.findall(r"([^/\s]+\.service)", cgroup)
    return matches[-1] if matches else ""


def _process_cmdline(pid: int) -> str:
    try:
        raw = (FsPath("/proc") / str(pid) / "cmdline").read_bytes()
    except OSError:
        return ""
    return raw.replace(b"\0", b" ").decode(errors="replace").strip()


def _parse_lsof_can_users(output: str) -> list[dict[str, Any]]:
    users: dict[tuple[int, str], dict[str, Any]] = {}
    for line in output.splitlines():
        if "protocol: CAN" not in line:
            continue
        parts = line.split()
        if len(parts) < 8:
            continue
        try:
            pid = int(parts[1])
        except ValueError:
            continue
        user_idx = 4 if len(parts) > 4 and parts[2].isdigit() else 2
        fd_idx = user_idx + 1
        if len(parts) <= fd_idx:
            continue
        fd = parts[fd_idx]
        key = (pid, fd)
        if key in users:
            continue
        protocol = "CAN"
        match = re.search(r"protocol:\s*([A-Z0-9_]+)", line)
        if match:
            protocol = match.group(1)
        users[key] = {
            "pid": pid,
            "command": parts[0],
            "user": parts[user_idx],
            "fd": fd,
            "protocol": protocol,
            "service": _process_service(pid),
            "cmdline": _process_cmdline(pid),
        }
    return sorted(users.values(), key=lambda item: (item["service"] or item["command"], item["pid"], item["fd"]))


def _can_receive_lists() -> list[dict[str, Any]]:
    lists = []
    can_dir = FsPath("/proc/net/can")
    for path in sorted(can_dir.glob("rcvlist_*")):
        entries = []
        for line in _read_text(path).splitlines():
            parts = line.split()
            if len(parts) >= 7 and parts[0].startswith("can"):
                entries.append({
                    "interface": parts[0],
                    "can_id": parts[1],
                    "mask": parts[2],
                    "matches": parts[-2],
                    "ident": parts[-1],
                })
        lists.append({"name": path.name, "entries": entries})
    return lists


def _safe_service_owner(owner: Optional[str]) -> Optional[str]:
    if not owner or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_-]*", owner):
        return None
    try:
        pwd.getpwnam(owner)
    except KeyError:
        return None
    return owner


def _default_service_owner() -> str:
    configured_owner = _safe_service_owner(settings.service_user.strip())
    if configured_owner:
        return configured_owner
    return pwd.getpwuid(SERVICE_ROOT.stat().st_uid).pw_name


def _run_systemctl(scope: str, args: list[str], timeout: float = 3, owner: Optional[str] = None) -> subprocess.CompletedProcess[str]:
    service_owner = _safe_service_owner(owner) if scope == "user" else None
    if scope == "user" and os.geteuid() == 0:
        service_owner = service_owner or _default_service_owner()
        owner_uid = pwd.getpwnam(service_owner).pw_uid
        cmd = ["runuser", "-u", service_owner, "--", "env", f"XDG_RUNTIME_DIR=/run/user/{owner_uid}", "systemctl", "--user", *args]
    elif scope == "user" and service_owner and service_owner != pwd.getpwuid(os.geteuid()).pw_name:
        cmd = ["systemctl", "--user", f"--machine={service_owner}@.host", *args]
    else:
        cmd = ["systemctl", "--user", *args] if scope == "user" else ["systemctl", *args]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, check=False)
    if os.geteuid() != 0 and scope == "system" and proc.returncode != 0 and args and args[0] in {"start", "stop", "restart", "enable", "disable"}:
        sudo_cmd = ["sudo", "-n", "systemctl", *args]
        sudo_proc = subprocess.run(sudo_cmd, capture_output=True, text=True, timeout=timeout, check=False)
        if sudo_proc.returncode == 0 or "a password is required" not in sudo_proc.stderr:
            return sudo_proc
    return proc


def _service_main_pid(scope: str, name: str, owner: Optional[str] = None) -> int:
    proc = _run_systemctl(scope, ["show", name, "--property=MainPID", "--value"], timeout=2, owner=owner)
    try:
        return int(proc.stdout.strip() or "0")
    except ValueError:
        return 0


def _service_units(scope: str, owner: Optional[str] = None) -> list[dict[str, Any]]:
    units = []
    unit_proc = _run_systemctl(scope, ["list-units", "--type=service", "--all", "--no-legend", "--plain"], owner=owner)
    enabled_proc = _run_systemctl(scope, ["list-unit-files", "--type=service", "--no-legend", "--plain"], owner=owner)
    enabled = {}
    for line in enabled_proc.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2:
            enabled[parts[0]] = parts[1]
    for line in unit_proc.stdout.splitlines():
        parts = line.split(None, 4)
        if len(parts) < 4:
            continue
        name, load, active, sub = parts[:4]
        description = parts[4] if len(parts) > 4 else ""
        main_pid = _service_main_pid(scope, name, owner) if active == "active" else 0
        units.append({
            "name": name,
            "load": load,
            "active": active,
            "sub": sub,
            "enabled": enabled.get(name, "unknown"),
            "description": description,
            "main_pid": main_pid,
            "cmdline": _process_cmdline(main_pid) if main_pid else "",
            "owner": owner if scope == "user" else None,
        })
    listed_names = {unit["name"] for unit in units}
    for name, state in enabled.items():
        if name in listed_names:
            continue
        units.append({
            "name": name,
            "load": "loaded",
            "active": "inactive",
            "sub": "dead",
            "enabled": state,
            "description": "",
            "main_pid": 0,
            "cmdline": "",
        })
    return units


def _service_unit(scope: str, name: str, owner: Optional[str] = None) -> dict[str, Any]:
    proc = _run_systemctl(scope, ["show", name, "--property=LoadState,ActiveState,SubState,UnitFileState,Description,MainPID"], timeout=3, owner=owner)
    values = dict(line.split("=", 1) for line in proc.stdout.splitlines() if "=" in line)
    try:
        main_pid = int(values.get("MainPID", "0") or "0")
    except ValueError:
        main_pid = 0
    return {
        "name": name,
        "load": values.get("LoadState", "unknown"),
        "active": values.get("ActiveState", "unknown"),
        "sub": values.get("SubState", "unknown"),
        "enabled": values.get("UnitFileState", "unknown"),
        "description": values.get("Description", ""),
        "main_pid": main_pid,
        "cmdline": _process_cmdline(main_pid) if main_pid else "",
        "owner": owner if scope == "user" else None,
    }


def _can_socket_users() -> tuple[list[dict[str, Any]], str]:
    try:
        proc = subprocess.run(["lsof", "-nP"], capture_output=True, text=True, timeout=2, check=False)
        users = _parse_lsof_can_users(proc.stdout)
        error = proc.stderr.strip() if proc.returncode not in (0, 1) else ""
        if os.geteuid() != 0:
            sudo_proc = subprocess.run(["sudo", "-n", "lsof", "-nP"], capture_output=True, text=True, timeout=2, check=False)
            if sudo_proc.returncode == 0 and sudo_proc.stdout:
                sudo_users = _parse_lsof_can_users(sudo_proc.stdout)
                if len(sudo_users) > len(users):
                    users = sudo_users
                    error = ""
            elif not users and "a password is required" in sudo_proc.stderr:
                error = "Full CAN detection requires running this service as root or passwordless sudo lsof"
    except FileNotFoundError:
        users = []
        error = "lsof is not installed; showing kernel receive lists only"
    except subprocess.TimeoutExpired:
        users = []
        error = "lsof timed out; showing kernel receive lists only"
    return users, error


def _service_name_is_safe(name: str) -> bool:
    return bool(re.fullmatch(r"[A-Za-z0-9_.@:-]+\.service", name))


def _service_key(scope: str, name: str) -> str:
    return f"{scope}:{name}"


def _load_stopped_can_services() -> dict[str, dict[str, Any]]:
    try:
        data = json.loads(STOPPED_CAN_SERVICES.read_text())
    except (OSError, json.JSONDecodeError):
        return {}
    if not isinstance(data, dict):
        return {}
    services = data.get("services", {})
    return services if isinstance(services, dict) else {}


def _save_stopped_can_services(services: dict[str, dict[str, Any]]) -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    payload = {"updated_at": datetime.now(timezone.utc).isoformat(), "services": services}
    tmp = STOPPED_CAN_SERVICES.with_suffix(".tmp")
    tmp.write_text(json.dumps(payload, indent=2, sort_keys=True))
    tmp.replace(STOPPED_CAN_SERVICES)


def _remember_stopped_can_service(service: dict[str, Any]) -> None:
    services = _load_stopped_can_services()
    key = _service_key(service["scope"], service["name"])
    services[key] = {
        "scope": service["scope"],
        "name": service["name"],
        "stopped_at": datetime.now(timezone.utc).isoformat(),
        "description": service.get("description", ""),
        "cmdline": service.get("cmdline", ""),
        "can_socket_count": service.get("can_socket_count", 0),
        "owner": service.get("owner"),
    }
    _save_stopped_can_services(services)


def _forget_stopped_can_service(scope: str, name: str) -> None:
    services = _load_stopped_can_services()
    key = _service_key(scope, name)
    if key in services:
        services.pop(key)
        _save_stopped_can_services(services)


@app.get("/api/system/permissions")
async def system_permissions():
    sudo_lsof = False
    sudo_systemctl = False
    if os.geteuid() != 0:
        sudo_lsof_proc = subprocess.run(["sudo", "-n", "lsof", "-nP"], capture_output=True, text=True, timeout=2, check=False)
        sudo_systemctl_proc = subprocess.run(["sudo", "-n", "systemctl", "status", "ssh.service"], capture_output=True, text=True, timeout=2, check=False)
        sudo_lsof = sudo_lsof_proc.returncode == 0
        sudo_systemctl = sudo_systemctl_proc.returncode in (0, 3) and "a password is required" not in sudo_systemctl_proc.stderr
    return {
        "uid": os.geteuid(),
        "root": os.geteuid() == 0,
        "sudo_lsof": sudo_lsof,
        "sudo_systemctl": sudo_systemctl,
        "full_can_detection": os.geteuid() == 0 or sudo_lsof,
        "system_service_control": os.geteuid() == 0 or sudo_systemctl,
    }


def _detected_services() -> tuple[list[dict[str, Any]], str]:
    can_socket_users, error = _can_socket_users()
    stopped_can_services = _load_stopped_can_services()
    can_by_service: dict[tuple[str, str, str], list[dict[str, Any]]] = {}
    for user in can_socket_users:
        service = user.get("service") or ""
        if service:
            cgroup = _read_text(FsPath("/proc") / str(user.get("pid", "")) / "cgroup")
            scope = "system" if "/system.slice/" in cgroup else "user"
            owner = user.get("user", "") if scope == "user" else ""
            can_by_service.setdefault((scope, owner, service), []).append(user)
    own_service = _process_service(os.getpid())
    by_key = {}
    for unit in _service_units("system"):
        by_key[("system", "", unit["name"])] = {**unit, "scope": "system"}
    user_services = {(owner, service) for scope, owner, service in can_by_service if scope == "user" and _safe_service_owner(owner)}
    for record in stopped_can_services.values():
        if record.get("scope") != "user" or not _service_name_is_safe(record.get("name", "")):
            continue
        owner = _safe_service_owner(record.get("owner")) or _default_service_owner()
        user_services.add((owner, record["name"]))
    for owner, service in sorted(user_services):
        unit = _service_unit("user", service, owner)
        by_key[("user", owner, service)] = {**unit, "scope": "user", "owner": owner}
    for scope, owner, service in can_by_service:
        if (scope, owner, service) not in by_key:
            by_key[(scope, owner, service)] = {
                "name": service,
                "load": "unknown",
                "active": "unknown",
                "sub": "unknown",
                "enabled": "unknown",
                "description": "",
                "main_pid": 0,
                "cmdline": "",
                "scope": scope,
                "owner": owner or None,
            }
    services = []
    for unit in sorted(by_key.values(), key=lambda item: (item["scope"], item["name"])):
        name = unit["name"]
        scope = unit["scope"]
        owner = unit.get("owner") or ""
        can_users = can_by_service.get((scope, owner, name), [])
        is_self = name == own_service or name == "owldrive-web.service"
        services.append({
            **unit,
            "can_socket_count": len(can_users),
            "can_users": can_users,
            "can_active": bool(can_users),
            "can_start": _service_name_is_safe(name),
            "can_stop": _service_name_is_safe(name) and not is_self,
            "is_self": is_self,
            "stopped_can_record": stopped_can_services.get(_service_key(scope, name)),
        })
    return services, error


@app.get("/api/system/services")
async def services():
    service_list, error = _detected_services()
    return {"services": service_list, "receive_lists": _can_receive_lists(), "error": error}


@app.post("/api/system/services/stop-can")
async def stop_can_services():
    service_list, error = _detected_services()
    stopped = []
    failed = []
    skipped = []
    for service in service_list:
        if not service["can_active"]:
            continue
        if service["is_self"]:
            skipped.append({"service": service["name"], "scope": service["scope"], "reason": "web service self-protection"})
            continue
        if not service["can_stop"]:
            skipped.append({"service": service["name"], "scope": service["scope"], "reason": "stop not allowed"})
            continue
        proc = _run_systemctl(service["scope"], ["disable", "--now", service["name"]], timeout=10, owner=service.get("owner"))
        item = {"service": service["name"], "scope": service["scope"]}
        if proc.returncode == 0:
            stopped.append(item)
            _remember_stopped_can_service(service)
        else:
            item["error"] = proc.stderr.strip() or proc.stdout.strip() or "systemctl disable --now failed"
            failed.append(item)
    return {"ok": not failed, "stopped": stopped, "failed": failed, "skipped": skipped, "detection_error": error}


@app.get("/api/system/services/stopped-can")
async def stopped_can_services():
    return {"services": list(_load_stopped_can_services().values())}


@app.post("/api/system/services/start-stopped-can")
async def start_stopped_can_services():
    services = _load_stopped_can_services()
    started = []
    failed = []
    for service in list(services.values()):
        name = service.get("name", "")
        scope = service.get("scope", "")
        item = {"service": name, "scope": scope}
        if not _service_name_is_safe(name) or scope not in {"user", "system"}:
            item["error"] = "invalid stored service"
            failed.append(item)
            continue
        proc = _run_systemctl(scope, ["enable", "--now", name], timeout=10, owner=service.get("owner"))
        if proc.returncode == 0:
            started.append(item)
            _forget_stopped_can_service(scope, name)
        else:
            item["error"] = proc.stderr.strip() or proc.stdout.strip() or "systemctl enable --now failed"
            failed.append(item)
    return {"ok": not failed, "started": started, "failed": failed}


@app.post("/api/system/services/{service_name:path}")
async def control_service(service_name: str, req: ServiceActionRequest):
    if not _service_name_is_safe(service_name):
        raise HTTPException(status_code=400, detail="invalid service name")
    action = req.action.lower()
    scope = req.scope.lower()
    owner = _safe_service_owner(req.owner) if scope == "user" else None
    if action not in {"start", "stop"}:
        raise HTTPException(status_code=400, detail="action must be start or stop")
    if scope not in {"user", "system"}:
        raise HTTPException(status_code=400, detail="scope must be user or system")
    if action == "stop" and (service_name == _process_service(os.getpid()) or service_name == "owldrive-web.service"):
        raise HTTPException(status_code=400, detail="refusing to stop this web service")
    stopped_can_service = None
    if scope == "user" and not owner:
        service_list, _ = _detected_services()
        for service in service_list:
            if service["scope"] == scope and service["name"] == service_name:
                owner = service.get("owner")
                if action == "stop" and service["can_active"]:
                    stopped_can_service = service
                break
    elif action == "stop":
        service_list, _ = _detected_services()
        stopped_can_service = next((service for service in service_list if service["scope"] == scope
            and service["name"] == service_name and service.get("owner") == owner and service["can_active"]), None)
    if scope == "user" and not owner:
        raise HTTPException(status_code=400, detail="user service owner is required")
    systemctl_args = ["enable", "--now", service_name] if action == "start" else ["disable", "--now", service_name]
    proc = _run_systemctl(scope, systemctl_args, timeout=10, owner=owner)
    if proc.returncode != 0:
        detail = proc.stderr.strip() or proc.stdout.strip() or f"systemctl {action} failed"
        raise HTTPException(status_code=500, detail=detail)
    if action == "start":
        _forget_stopped_can_service(scope, service_name)
    elif stopped_can_service:
        _remember_stopped_can_service(stopped_can_service)
    return {"ok": True, "service": service_name, "action": action}


@app.get("/api/system/can-users")
async def can_users():
    users, error = _can_socket_users()
    return {
        "users": users,
        "receive_lists": _can_receive_lists(),
        "error": error,
    }


@app.get("/api/devices")
async def devices():
    return {"devices": await get_bus().scan()}


@app.get("/api/devices/{node_id}/telemetry")
async def telemetry(node_id: Annotated[int, Path(ge=1, le=62)]):
    return await get_bus().telemetry(node_id)


@app.get("/api/config/schema")
async def config_schema():
    return {"profile_size": PROFILE_SIZE, "fields": schema_json()}


@app.get("/api/config/presets")
async def config_presets():
    return {"presets": public_presets()}


@app.get("/api/config/motor-presets")
async def motor_presets():
    return {"presets": list_motor_presets(SERVICE_ROOT)}


@app.get("/api/config/motion-presets")
async def motion_presets():
    return {"presets": list_motion_presets(SERVICE_ROOT)}


@app.get("/api/config/pcb-presets")
async def pcb_presets():
    return {"presets": list_pcb_presets(SERVICE_ROOT)}


@app.get("/api/devices/{node_id}/config")
async def read_config(node_id: Annotated[int, Path(ge=1, le=62)]):
    raw = await get_bus().read_config(node_id, PROFILE_SIZE)
    database_version = ensure_supported_database_version(raw)
    return {"node_id": node_id, "database_version": database_version, "values": decode_config(raw)}


@app.patch("/api/devices/{node_id}/config")
async def patch_config(node_id: Annotated[int, Path(ge=1, le=62)], req: ConfigPatchRequest):
    async with exclusive_job_lock:
        try:
            current = bytearray(await get_bus().read_config(node_id, PROFILE_SIZE))
            ensure_supported_database_version(current)
            changes: dict[int, int] = {}
            reboot_required = False
            for path, value in req.values.items():
                field = FIELD_BY_PATH.get(path)
                if field is None:
                    raise HTTPException(status_code=400, detail=f"unknown config field: {path}")
                encoded = encode_field(value, field)
                for idx, byte in enumerate(encoded):
                    offset = field.offset + idx
                    if current[offset] != byte:
                        changes[offset] = byte
                reboot_required = reboot_required or field.reboot

            can_node_field = FIELD_BY_PATH["canNodeId"]
            can_node_change = changes.pop(can_node_field.offset, None)
            if changes:
                await get_bus().write_config_bytes(node_id, changes)

            effective_node_id = node_id
            if can_node_change is not None:
                await get_bus().set_cfg_byte(node_id, can_node_field.offset, can_node_change, wait_ack=False)
                effective_node_id = can_node_change

            if req.save or req.reboot:
                await get_bus().save_config(effective_node_id, reboot=req.reboot)
            return {"ok": True, "bytes_changed": len(changes) + (1 if can_node_change is not None else 0), "reboot_required": reboot_required, "node_id": effective_node_id}
        except TimeoutError as exc:
            raise HTTPException(status_code=504, detail=str(exc)) from exc


@app.post("/api/devices/{node_id}/config/apply-preset")
async def apply_preset(node_id: Annotated[int, Path(ge=1, le=62)], req: ApplyPresetRequest):
    preset = find_preset(req.preset_id)
    if preset is None:
        raise HTTPException(status_code=404, detail="preset not found")
    values = dict(preset["values"])
    if req.keep_node_id:
        values.pop("canNodeId", None)
    patch = ConfigPatchRequest(values=values, save=req.save, reboot=req.reboot)
    return await patch_config(node_id, patch)


@app.post("/api/devices/{node_id}/config/apply-motor-preset")
async def apply_motor_preset(node_id: Annotated[int, Path(ge=1, le=62)], req: ApplyMotorPresetRequest):
    preset = find_motor_preset(SERVICE_ROOT, req.preset_id)
    if preset is None:
        raise HTTPException(status_code=404, detail="motor preset not found")
    patch = ConfigPatchRequest(values=preset["values"], save=req.save, reboot=req.reboot)
    result = await patch_config(node_id, patch)
    result["reboot_required"] = True
    return result


@app.post("/api/devices/{node_id}/config/apply-motion-preset")
async def apply_motion_preset(node_id: Annotated[int, Path(ge=1, le=62)], req: ApplyMotionPresetRequest):
    preset = find_motion_preset(SERVICE_ROOT, req.preset_id)
    if preset is None:
        raise HTTPException(status_code=404, detail="motion preset not found")
    patch = ConfigPatchRequest(values=preset["values"], save=req.save, reboot=req.reboot)
    return await patch_config(node_id, patch)


@app.post("/api/devices/{node_id}/config/apply-pcb-preset")
async def apply_pcb_preset(node_id: Annotated[int, Path(ge=1, le=62)], req: ApplyPcbPresetRequest):
    preset = find_pcb_preset(SERVICE_ROOT, req.preset_id)
    if preset is None:
        raise HTTPException(status_code=404, detail="PCB preset not found")
    patch = ConfigPatchRequest(values=preset["values"], save=req.save, reboot=req.reboot)
    result = await patch_config(node_id, patch)
    result["reboot_required"] = True
    return result


@app.post("/api/devices/{node_id}/config/sensor-auto-align")
async def sensor_auto_align(node_id: Annotated[int, Path(ge=1, le=62)]):
    async with exclusive_job_lock:
        raw = await get_bus().read_config(node_id, PROFILE_SIZE)
        ensure_supported_database_version(raw)
        ok = await get_bus().sensor_auto_align(node_id)
        if not ok:
            raise HTTPException(status_code=400, detail="sensor auto-align failed or is not supported by this firmware")
        raw = await get_bus().read_config(node_id, PROFILE_SIZE)
        values = decode_config(raw)
        return {
            "ok": True,
            "values": {
                "motor.zeroOfs": values.get("motor.zeroOfs"),
                "motor.senDirCW": values.get("motor.senDirCW"),
            },
            "reboot_required": False,
        }


@app.post("/api/devices/{node_id}/values")
async def set_value(node_id: Annotated[int, Path(ge=1, le=63)], req: SetValueRequest):
    if exclusive_job_lock.locked() and req.value in {
        CanValue.target,
        CanValue.voltage,
        CanValue.current,
        CanValue.velocity,
        CanValue.angle,
        CanValue.angle_add,
        CanValue.pwm_speed,
        CanValue.fifo_target,
        CanValue.fifo_clock,
    }:
        raise HTTPException(status_code=409, detail="exclusive config/flash job is active")
    ok = await get_bus().set_value(node_id, req.value, req.data, req.wait_ack)
    return {"ok": ok}


@app.post("/api/devices/{node_id}/save-config")
async def save_config(node_id: Annotated[int, Path(ge=1, le=63)], req: SaveConfigRequest):
    async with exclusive_job_lock:
        if node_id != 63:
            raw = await get_bus().read_config(node_id, PROFILE_SIZE)
            ensure_supported_database_version(raw)
        await get_bus().save_config(node_id, reboot=req.reboot)
    return {"ok": True}


@app.post("/api/devices/{node_id}/flash")
async def flash(node_id: Annotated[int, Path(ge=1, le=62)], firmware: UploadFile = File(...)):
    uploaded = await firmware.read()
    data = firmware_payload(uploaded, firmware.filename or "")
    if not data:
        raise HTTPException(status_code=400, detail="empty firmware file")
    job = _create_flash_job(node_id, firmware.filename or "firmware.bin", len(data))
    _start_flash_job(job, lambda progress: get_bus().upload_firmware(node_id, data, progress))
    return job


@app.post("/api/rc-switch/flash")
async def flash_rc_switch(firmware: UploadFile = File(...)):
    uploaded = await firmware.read()
    data = firmware_payload(uploaded, firmware.filename or "")
    if not data:
        raise HTTPException(status_code=400, detail="empty firmware file")
    job = _create_flash_job(settings.rc_switch_node_id,
                            f"RC-Switch: {firmware.filename or 'firmware.bin'}", len(data))
    _start_flash_job(job, lambda progress: _upload_rc_switch(data, progress))
    return job


@app.post("/api/owl-controller/flash")
async def flash_owl_controller(firmware: UploadFile = File(...)):
    uploaded = await firmware.read()
    data = firmware_payload(uploaded, firmware.filename or "")
    if not data:
        raise HTTPException(status_code=400, detail="empty firmware file")
    job = _create_flash_job(settings.owl_controller_node_id,
                            f"owlController: {firmware.filename or 'firmware.bin'}", len(data))
    _start_flash_job(job, lambda progress: _upload_owl_controller(data, progress))
    return job


@app.post("/api/devices/flash")
async def flash_multi(node_ids: Annotated[str, Form()], firmware: UploadFile = File(...)):
    try:
        nodes = _validate_flash_nodes(json.loads(node_ids))
    except json.JSONDecodeError as exc:
        raise HTTPException(status_code=400, detail="invalid node_ids") from exc
    uploaded = await firmware.read()
    data = firmware_payload(uploaded, firmware.filename or "")
    if not data:
        raise HTTPException(status_code=400, detail="empty firmware file")
    job = _create_flash_job(63, firmware.filename or "firmware.bin", len(data))
    job.filename = f"{job.filename} ({', '.join(f'Node {node}' for node in nodes)})"
    _start_flash_job(job, lambda progress: get_bus().upload_firmware_broadcast(nodes, data, progress))
    return job


@app.post("/api/devices/{node_id}/flash-image")
async def flash_image(node_id: Annotated[int, Path(ge=1, le=62)], req: FlashImageRequest):
    image = None
    all_images = list_local_images(TOOLS_ROOT)
    try:
        all_images.extend(list_github_images())
    except Exception:
        pass
    for candidate in all_images:
        if candidate.id == req.image_id:
            image = candidate
            break
    if image is None:
        raise HTTPException(status_code=404, detail="firmware image not found")
    raw = image.path.read_bytes() if image.path else download_url(image.url or "")
    data = firmware_payload(raw, image.name)
    job = _create_flash_job(node_id, image.name, len(data))
    _start_flash_job(job, lambda progress: get_bus().upload_firmware(node_id, data, progress))
    return job


@app.post("/api/devices/flash-image")
async def flash_image_multi(req: MultiFlashImageRequest):
    nodes = _validate_flash_nodes(req.node_ids)
    image = None
    all_images = list_local_images(TOOLS_ROOT)
    try:
        all_images.extend(list_github_images())
    except Exception:
        pass
    for candidate in all_images:
        if candidate.id == req.image_id:
            image = candidate
            break
    if image is None:
        raise HTTPException(status_code=404, detail="firmware image not found")
    raw = image.path.read_bytes() if image.path else download_url(image.url or "")
    data = firmware_payload(raw, image.name)
    job = _create_flash_job(63, f"{image.name} ({', '.join(f'Node {node}' for node in nodes)})", len(data))
    _start_flash_job(job, lambda progress: get_bus().upload_firmware_broadcast(nodes, data, progress))
    return job


@app.get("/api/jobs/{job_id}")
async def get_job(job_id: int):
    job = jobs.get(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="job not found")
    return job


@app.post("/api/jobs/{job_id}/cancel")
async def cancel_job(job_id: int):
    job = jobs.get(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="job not found")
    if job.state in {"done", "failed", "cancelled"}:
        return job
    job.cancel_requested = True
    if job.state == "queued":
        job.state = "cancelled"
        job.error = "cancelled by user"
    return job


@app.websocket("/ws/devices/{node_id}/telemetry")
async def telemetry_ws(websocket: WebSocket, node_id: int):
    await websocket.accept()
    try:
        while True:
            await websocket.send_json(await get_bus().telemetry(node_id))
            await asyncio.sleep(0.1)
    except WebSocketDisconnect:
        return


@app.websocket("/ws/devices/scan")
async def scan_ws(websocket: WebSocket):
    await websocket.accept()
    try:
        async for device in get_bus().scan_iter():
            await websocket.send_json({"type": "device", "device": device})
        await websocket.send_json({"type": "done"})
    except WebSocketDisconnect:
        return
