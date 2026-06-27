from __future__ import annotations

import asyncio
import itertools
import json
import os
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path as FsPath
from typing import Annotated, Any, Dict, Optional, Union

from fastapi import FastAPI, File, HTTPException, Path, UploadFile, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from pydantic_settings import BaseSettings

from .can_protocol import CanValue, OwldriveCanBus, socketcan_interfaces
from .config_schema import FIELD_BY_PATH, PROFILE_SIZE, decode_config, encode_field, schema_json
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


class Settings(BaseSettings):
    can_channel: str = "can0"
    can_bitrate: int = 1_000_000
    host_node_id: int = 62
    can_msg_id: int = 300

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


class ServiceActionRequest(BaseModel):
    action: str
    scope: str = "user"


class FlashJob(BaseModel):
    id: int
    node_id: int
    filename: str
    done: int = 0
    total: int
    state: str = "queued"
    error: Optional[str] = None
    crc: Optional[int] = None


settings = Settings()
app = FastAPI(title="owlDrive Service", version="0.1.0")
app.mount("/static", StaticFiles(directory=STATIC), name="static")

bus: OwldriveCanBus | None = None
job_counter = itertools.count(1)
jobs: Dict[int, FlashJob] = {}
exclusive_job_lock = asyncio.Lock()


@app.on_event("startup")
async def startup():
    global bus
    if os.getenv("OWLDRIVE_DISABLE_CAN") == "1":
        return
    bus = OwldriveCanBus(
        channel=settings.can_channel,
        bitrate=settings.can_bitrate,
        host_node=settings.host_node_id,
        msg_id=settings.can_msg_id,
    )


@app.on_event("shutdown")
async def shutdown():
    if bus:
        bus.close()


def get_bus() -> OwldriveCanBus:
    if bus is None:
        raise HTTPException(status_code=503, detail="CAN bus is not available")
    return bus


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


def _run_systemctl(scope: str, args: list[str], timeout: float = 3) -> subprocess.CompletedProcess[str]:
    cmd = ["systemctl", "--user", *args] if scope == "user" else ["systemctl", *args]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, check=False)
    if os.geteuid() != 0 and scope == "system" and proc.returncode != 0 and args and args[0] in {"start", "stop", "restart"}:
        sudo_cmd = ["sudo", "-n", "systemctl", *args]
        sudo_proc = subprocess.run(sudo_cmd, capture_output=True, text=True, timeout=timeout, check=False)
        if sudo_proc.returncode == 0 or "a password is required" not in sudo_proc.stderr:
            return sudo_proc
    return proc


def _service_main_pid(scope: str, name: str) -> int:
    proc = _run_systemctl(scope, ["show", name, "--property=MainPID", "--value"], timeout=2)
    try:
        return int(proc.stdout.strip() or "0")
    except ValueError:
        return 0


def _service_units(scope: str) -> list[dict[str, Any]]:
    units = []
    unit_proc = _run_systemctl(scope, ["list-units", "--type=service", "--all", "--no-legend", "--plain"])
    enabled_proc = _run_systemctl(scope, ["list-unit-files", "--type=service", "--no-legend", "--plain"])
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
        main_pid = _service_main_pid(scope, name) if active == "active" else 0
        units.append({
            "name": name,
            "load": load,
            "active": active,
            "sub": sub,
            "enabled": enabled.get(name, "unknown"),
            "description": description,
            "main_pid": main_pid,
            "cmdline": _process_cmdline(main_pid) if main_pid else "",
        })
    return units


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
    can_by_service: dict[tuple[str, str], list[dict[str, Any]]] = {}
    for user in can_socket_users:
        service = user.get("service") or ""
        if service:
            cgroup = _read_text(FsPath("/proc") / str(user.get("pid", "")) / "cgroup")
            scope = "system" if "/system.slice/" in cgroup else "user"
            can_by_service.setdefault((scope, service), []).append(user)
    own_service = _process_service(os.getpid())
    by_key = {}
    for scope in ("user", "system"):
        for unit in _service_units(scope):
            by_key[(scope, unit["name"])] = {**unit, "scope": scope}
    for scope, service in can_by_service:
        if (scope, service) not in by_key:
            by_key[(scope, service)] = {
                "name": service,
                "load": "unknown",
                "active": "unknown",
                "sub": "unknown",
                "enabled": "unknown",
                "description": "",
                "main_pid": 0,
                "cmdline": "",
                "scope": scope,
            }
    services = []
    for unit in sorted(by_key.values(), key=lambda item: (item["scope"], item["name"])):
        name = unit["name"]
        scope = unit["scope"]
        can_users = can_by_service.get((scope, name), [])
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
        proc = _run_systemctl(service["scope"], ["stop", service["name"]], timeout=10)
        item = {"service": service["name"], "scope": service["scope"]}
        if proc.returncode == 0:
            stopped.append(item)
            _remember_stopped_can_service(service)
        else:
            item["error"] = proc.stderr.strip() or proc.stdout.strip() or "systemctl stop failed"
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
        proc = _run_systemctl(scope, ["start", name], timeout=10)
        if proc.returncode == 0:
            started.append(item)
            _forget_stopped_can_service(scope, name)
        else:
            item["error"] = proc.stderr.strip() or proc.stdout.strip() or "systemctl start failed"
            failed.append(item)
    return {"ok": not failed, "started": started, "failed": failed}


@app.post("/api/system/services/{service_name:path}")
async def control_service(service_name: str, req: ServiceActionRequest):
    if not _service_name_is_safe(service_name):
        raise HTTPException(status_code=400, detail="invalid service name")
    action = req.action.lower()
    scope = req.scope.lower()
    if action not in {"start", "stop"}:
        raise HTTPException(status_code=400, detail="action must be start or stop")
    if scope not in {"user", "system"}:
        raise HTTPException(status_code=400, detail="scope must be user or system")
    if action == "stop" and (service_name == _process_service(os.getpid()) or service_name == "owldrive-web.service"):
        raise HTTPException(status_code=400, detail="refusing to stop this web service")
    stopped_can_service = None
    if action == "stop":
        service_list, _ = _detected_services()
        for service in service_list:
            if service["scope"] == scope and service["name"] == service_name and service["can_active"]:
                stopped_can_service = service
                break
    proc = _run_systemctl(scope, [action, service_name], timeout=10)
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
    return {"node_id": node_id, "values": decode_config(raw)}


@app.patch("/api/devices/{node_id}/config")
async def patch_config(node_id: Annotated[int, Path(ge=1, le=62)], req: ConfigPatchRequest):
    async with exclusive_job_lock:
        current = bytearray(await get_bus().read_config(node_id, PROFILE_SIZE))
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
        if changes:
            await get_bus().write_config_bytes(node_id, changes)
        if req.save or req.reboot:
            await get_bus().save_config(node_id, reboot=req.reboot)
        return {"ok": True, "bytes_changed": len(changes), "reboot_required": reboot_required}


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
        await get_bus().save_config(node_id, reboot=req.reboot)
    return {"ok": True}


@app.post("/api/devices/{node_id}/flash")
async def flash(node_id: Annotated[int, Path(ge=1, le=62)], firmware: UploadFile = File(...)):
    uploaded = await firmware.read()
    data = firmware_payload(uploaded, firmware.filename or "")
    if not data:
        raise HTTPException(status_code=400, detail="empty firmware file")
    job_id = next(job_counter)
    job = FlashJob(id=job_id, node_id=node_id, filename=firmware.filename or "firmware.bin", total=len(data))
    jobs[job_id] = job

    async def runner():
        async with exclusive_job_lock:
            job.state = "running"

            def progress(done: int, total: int):
                job.done = done
                job.total = total

            try:
                job.crc = await get_bus().upload_firmware(node_id, data, progress)
                job.done = job.total
                job.state = "done"
            except Exception as exc:
                job.error = str(exc)
                job.state = "failed"

    asyncio.create_task(runner())
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
    job_id = next(job_counter)
    job = FlashJob(id=job_id, node_id=node_id, filename=image.name, total=len(data))
    jobs[job_id] = job

    async def runner():
        async with exclusive_job_lock:
            job.state = "running"

            def progress(done: int, total: int):
                job.done = done
                job.total = total

            try:
                job.crc = await get_bus().upload_firmware(node_id, data, progress)
                job.done = job.total
                job.state = "done"
            except Exception as exc:
                job.error = str(exc)
                job.state = "failed"

    asyncio.create_task(runner())
    return job


@app.get("/api/jobs/{job_id}")
async def get_job(job_id: int):
    job = jobs.get(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="job not found")
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
