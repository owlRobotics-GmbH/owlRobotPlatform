# owlDrive Linux Service

Web interface for managing owlDrive devices on a SocketCAN bus.

## User Setup on OrangePi

Expected location on the target machine:

```bash
~/owlRobotPlatform/tools/owldrive_service
```

Install and start the service:

```bash
cd ~/owlRobotPlatform/tools/owldrive_service
sudo systemd/install-system-service.sh
```

The installer creates the Python environment, installs dependencies, generates the systemd unit for the detected path, enables it at boot, and starts it.

When run through `sudo`, the installer uses `SUDO_USER` to find the real user's home directory. Override the detected values only when needed:

```bash
sudo SERVICE_USER=myuser systemd/install-system-service.sh
sudo SERVICE_DIR=/custom/path/owldrive_service systemd/install-system-service.sh
```

Open the web interface:

```text
http://orangepi5pro.local:8080/
```

Service commands:

```bash
cd ~/owlRobotPlatform/tools/owldrive_service
sudo systemd/owldrive-service.sh status
sudo systemd/owldrive-service.sh restart
sudo systemd/owldrive-service.sh disable
sudo systemd/owldrive-service.sh enable
systemd/owldrive-service.sh logs
```

`disable` stops the service and disables automatic start at boot. `enable` installs missing runtime files if needed, enables automatic start, and starts the service.

Required OS packages:

- `python3`
- `python3-venv`
- `systemd`

SocketCAN must be configured by the platform setup. Example for `can0` at 1 Mbit/s:

```bash
sudo ip link set can0 down || true
sudo ip link set can0 type can bitrate 1000000
sudo ip link set can0 up
```

## What the Service Provides

- SocketCAN discovery for `can0`, `can1`, ...
- owlDrive node scan through `can_val_firmware_ver`
- Live telemetry plot over WebSocket
- Config editor with profile, motor, motion, and PCB presets
- Multi-device firmware flashing from `tools/flash`
- Linux service overview for processes using CAN

## Developer Notes

Run locally:

```bash
cd tools/owldrive_service
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
uvicorn owldrive_service.app:app --reload --host 0.0.0.0 --port 8080
```

Run checks:

```bash
python3 -m unittest discover -s tests
python3 -m py_compile owldrive_service/*.py tools/generate_motor_presets.py
node --check static/app.js
sh -n systemd/owldrive-service.sh systemd/install-system-service.sh
```

Path assumptions:

- service root: `tools/owldrive_service`
- local firmware images: `tools/flash/*.uf2`
- static web files: `tools/owldrive_service/static`
- runtime data: `tools/owldrive_service/data`

Environment variables:

- `OWLDRIVE_CAN_CHANNEL`, default `can0`
- `OWLDRIVE_CAN_BITRATE`, default `1000000`
- `OWLDRIVE_HOST_NODE_ID`, default `62`
- `OWLDRIVE_CAN_MSG_ID`, default `300`

## Firmware Compatibility

The service is intended to work with the existing owlDrive CAN protocol. It does not require an operation-state extension in the firmware.

During config and flash jobs, the web service rejects new motion commands sent through this web API. Firmware flashing still relies on the firmware's existing upload path, which pauses/stops the FOC loop when the upload starts.
