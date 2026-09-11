#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
APP_PATH="$("$ROOT_DIR/Tools/package-launcher-app.sh")"
exec open "$APP_PATH"
