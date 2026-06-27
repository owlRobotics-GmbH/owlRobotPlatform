#!/bin/sh
set -eu

SERVICE_DIR="${SERVICE_DIR:-/home/orangepi/owlRobotPlatform/tools/owldrive_service}"
UNIT_NAME="${UNIT_NAME:-owldrive-web.service}"
UNIT_SOURCE="${UNIT_SOURCE:-$SERVICE_DIR/systemd/owldrive.service}"
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
  SERVICE_USER=<owner of SERVICE_DIR>
EOF
}

need_root() {
  if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0 $*" >&2
    exit 1
  fi
}

install_unit() {
  if [ ! -f "$UNIT_SOURCE" ]; then
    echo "Unit source not found: $UNIT_SOURCE" >&2
    exit 1
  fi
  install -m 0644 "$UNIT_SOURCE" "$UNIT_TARGET"
  systemctl daemon-reload
}

service_user() {
  if [ -n "${SERVICE_USER:-}" ]; then
    echo "$SERVICE_USER"
  elif [ -d "$SERVICE_DIR" ]; then
    stat -c "%U" "$SERVICE_DIR"
  else
    echo "root"
  fi
}

run_as_service_user() {
  user="$(service_user)"
  if [ "$user" != "root" ] && command -v runuser >/dev/null 2>&1; then
    runuser -u "$user" -- "$@"
  else
    "$@"
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
  if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found. Install python3 and python3-venv first." >&2
    exit 1
  fi

  mkdir -p "$SERVICE_DIR/data"
  chown "$(service_user)":"$(service_user)" "$SERVICE_DIR/data" 2>/dev/null || true

  if [ ! -x "$SERVICE_DIR/.venv/bin/python" ]; then
    echo "Creating Python virtual environment..."
    run_as_service_user python3 -m venv "$SERVICE_DIR/.venv"
  fi

  echo "Installing Python requirements..."
  run_as_service_user "$SERVICE_DIR/.venv/bin/python" -m pip install --upgrade pip
  run_as_service_user "$SERVICE_DIR/.venv/bin/python" -m pip install -r "$SERVICE_DIR/requirements.txt"

  if [ -f "$SERVICE_DIR/systemd/owldrive-web.sudoers" ] && command -v visudo >/dev/null 2>&1; then
    install -m 0440 "$SERVICE_DIR/systemd/owldrive-web.sudoers" "/etc/sudoers.d/owldrive-web"
    visudo -cf "/etc/sudoers.d/owldrive-web" >/dev/null
  fi
}

disable_old_user_unit() {
  if command -v runuser >/dev/null 2>&1 && id orangepi >/dev/null 2>&1; then
    runuser -u orangepi -- env XDG_RUNTIME_DIR=/run/user/1000 systemctl --user disable --now "$UNIT_NAME" >/dev/null 2>&1 || true
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
