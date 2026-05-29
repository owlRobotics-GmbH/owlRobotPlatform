#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

arduino \
  --pref "sketchbook.path=${SCRIPT_DIR}" \
  --verify "${SCRIPT_DIR}/owlController_RP2040_00.ino"
