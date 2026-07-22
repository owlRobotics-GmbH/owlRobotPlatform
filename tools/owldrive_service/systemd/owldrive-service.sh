#!/bin/sh
set -eu

RUN_USER="${SERVICE_USER:-${SUDO_USER:-$(id -un)}}"

user_home() {
  if command -v getent >/dev/null 2>&1; then
    getent passwd "$1" | cut -d: -f6
  else
    eval "printf '%s\n' ~$1"
  fi
}

HOME_DIR="${HOME_DIR:-$(user_home "$RUN_USER")}"
if [ -z "$HOME_DIR" ]; then
  echo "Could not determine home directory for user: $RUN_USER" >&2
  exit 1
fi

SERVICE_DIR="${SERVICE_DIR:-$HOME_DIR/owlRobotPlatform/tools/owldrive_service}"
UNIT_NAME="${UNIT_NAME:-owldrive-web.service}"
UNIT_TARGET="/etc/systemd/system/$UNIT_NAME"

usage() {
  cat <<EOF
Usage: $0 <command>

Commands:
  install    Install environment and unit, enable and start service
  enable     Enable service start at boot and start it now
  disable    Stop service and disable start at boot
  start      Start service
  stop       Stop service
  restart    Restart service
  status     Show service status
  logs       Follow service journal
  uninstall  Stop, disable and remove unit file

Environment overrides:
  SERVICE_DIR=$SERVICE_DIR
  UNIT_NAME=$UNIT_NAME
  SERVICE_USER=$RUN_USER
EOF
}

need_root() {
  if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0 $*" >&2
    exit 1
  fi
}

install_unit() {
  tmp="$(mktemp)"
  cat > "$tmp" <<EOF
[Unit]
Description=owlDrive CAN web interface
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
WorkingDirectory=$SERVICE_DIR
Environment=OWLDRIVE_CAN_CHANNEL=can0
Environment=OWLDRIVE_CAN_BITRATE=1000000
Environment=OWLDRIVE_HOST_NODE_ID=62
Environment=OWLDRIVE_CAN_MSG_ID=300
ExecStart=$SERVICE_DIR/.venv/bin/uvicorn owldrive_service.app:app --host 0.0.0.0 --port 8080
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF
  install -m 0644 "$tmp" "$UNIT_TARGET"
  rm -f "$tmp"
  systemctl daemon-reload
}

service_user() {
  echo "$RUN_USER"
}

run_as_service_user() {
  user="$(service_user)"
  if [ "$user" != "root" ] && command -v runuser >/dev/null 2>&1; then
    runuser -u "$user" -- "$@"
  else
    "$@"
  fi
}

ensure_os_packages() {
  if command -v apt-get >/dev/null 2>&1; then
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y python3 python3-venv python3-dev build-essential
  fi
}

ensure_environment() {
  if [ ! -d "$SERVICE_DIR" ]; then
    echo "Service directory not found: $SERVICE_DIR" >&2
    exit 1
  fi
  if [ ! -f "$SERVICE_DIR/requirements.txt" ]; then
    echo "requirements.txt not found in $SERVICE_DIR" >&2
    exit 1
  fi
  ensure_os_packages
  if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found. Install python3 and python3-venv first." >&2
    exit 1
  fi

  mkdir -p "$SERVICE_DIR/data"
  chown "$(service_user):" "$SERVICE_DIR/data" 2>/dev/null || true

  if [ ! -x "$SERVICE_DIR/.venv/bin/python" ]; then
    echo "Creating Python virtual environment..."
    run_as_service_user python3 -m venv "$SERVICE_DIR/.venv"
  fi

  echo "Installing Python requirements..."
  run_as_service_user "$SERVICE_DIR/.venv/bin/python" -m pip install --upgrade pip
  run_as_service_user "$SERVICE_DIR/.venv/bin/python" -m pip install -r "$SERVICE_DIR/requirements.txt"

  if command -v visudo >/dev/null 2>&1; then
    tmp="$(mktemp)"
    cat > "$tmp" <<EOF
$RUN_USER ALL=(root) NOPASSWD: /usr/bin/systemctl start *.service
$RUN_USER ALL=(root) NOPASSWD: /usr/bin/systemctl stop *.service
$RUN_USER ALL=(root) NOPASSWD: /usr/bin/systemctl restart *.service
$RUN_USER ALL=(root) NOPASSWD: /usr/bin/lsof -nP
EOF
    visudo -cf "$tmp" >/dev/null
    install -m 0440 "$tmp" "/etc/sudoers.d/owldrive-web"
    rm -f "$tmp"
  fi
}

disable_old_user_unit() {
  if command -v runuser >/dev/null 2>&1 && id "$RUN_USER" >/dev/null 2>&1; then
    uid="$(id -u "$RUN_USER")"
    runuser -u "$RUN_USER" -- env XDG_RUNTIME_DIR="/run/user/$uid" systemctl --user disable --now "$UNIT_NAME" >/dev/null 2>&1 || true
  fi
}

cmd="${1:-}"
case "$cmd" in
  install)
    need_root "$cmd"
    ensure_environment
    install_unit
    disable_old_user_unit
    systemctl enable --now "$UNIT_NAME"
    systemctl --no-pager --full status "$UNIT_NAME"
    ;;
  enable)
    need_root "$cmd"
    ensure_environment
    if [ ! -f "$UNIT_TARGET" ]; then
      install_unit
    else
      systemctl daemon-reload
    fi
    disable_old_user_unit
    systemctl enable --now "$UNIT_NAME"
    systemctl --no-pager --full status "$UNIT_NAME"
    ;;
  disable)
    need_root "$cmd"
    systemctl disable --now "$UNIT_NAME"
    systemctl --no-pager --full status "$UNIT_NAME" || true
    ;;
  start|stop|restart)
    need_root "$cmd"
    systemctl "$cmd" "$UNIT_NAME"
    systemctl --no-pager --full status "$UNIT_NAME" || true
    ;;
  status)
    systemctl --no-pager --full status "$UNIT_NAME"
    ;;
  logs)
    journalctl -u "$UNIT_NAME" -f
    ;;
  uninstall)
    need_root "$cmd"
    systemctl disable --now "$UNIT_NAME" || true
    rm -f "$UNIT_TARGET"
    systemctl daemon-reload
    ;;
  -h|--help|help|"")
    usage
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    usage >&2
    exit 1
    ;;
esac
