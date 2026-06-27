# owlDrive Linux Service

This service provides a web interface for owlDrive CAN networks:

- SocketCAN discovery for `can0`, `can1`, ...
- Device scan through `can_val_firmware_ver`
- Live telemetry over WebSocket with a plot
- Settings actions for operation state, motion mode, motor enable, and save/reboot
- Exclusive config and flash jobs so motion commands are not sent during config/flash work
- Firmware upload through the existing byte-wise CAN protocol

## Development

```bash
cd owldrive_service
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
uvicorn owldrive_service.app:app --reload --host 0.0.0.0 --port 8080
```

Open `http://localhost:8080`.

The app can be imported without CAN hardware. Running it against real devices requires `python-can` and a SocketCAN interface.

## SocketCAN Setup

Example for 1 Mbit/s:

```bash
sudo ip link set can0 down || true
sudo ip link set can0 type can bitrate 1000000
sudo ip link set can0 up
```

## Systemd

On the OrangePi, from `/home/orangepi/owlRobotPlatform/tools/owldrive_service`:

```bash
sudo systemd/owldrive-service.sh install
```

The installer:

- creates `data/` if it is missing
- creates `.venv` if it is missing
- installs or updates Python requirements
- installs the system unit as `owldrive-web.service`
- disables an older user-level unit if present
- enables and starts the service

Useful service commands:

```bash
sudo systemd/owldrive-service.sh enable
sudo systemd/owldrive-service.sh disable
sudo systemd/owldrive-service.sh restart
systemd/owldrive-service.sh status
systemd/owldrive-service.sh logs
```

Configuration is done through environment variables:

- `OWLDRIVE_CAN_CHANNEL`, default `can0`
- `OWLDRIVE_CAN_BITRATE`, default `1000000`
- `OWLDRIVE_HOST_NODE_ID`, default `62`
- `OWLDRIVE_CAN_MSG_ID`, default `300`

## Firmware Compatibility

The web service can request `can_val_operation_state = 35`. Older firmware does not answer that value; live plot, config, and flashing can still work, but older firmware relies on its existing FOC pause/stop behavior instead of an explicit operation-state lock.

Operation states used by newer firmware:

- `standby`: motors/FIFO stopped, FOC paused
- `drive`: motion commands allowed
- `config`: config/save active, motion commands blocked
- `flash`: firmware upload active, motion commands blocked
- `fault`: motors/FIFO stopped, motion commands blocked
