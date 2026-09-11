#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
APP_PATH="$ROOT_DIR/Build/XYZ DEV.app"

BIN_DIR="$(swift build --package-path "$ROOT_DIR/Launcher" -c release --show-bin-path)"
BIN_PATH="$BIN_DIR/XYZLauncher"
test -x "$BIN_PATH"

rm -rf "$APP_PATH"
mkdir -p "$APP_PATH/Contents/MacOS" "$APP_PATH/Contents/Resources"
cp "$BIN_PATH" "$APP_PATH/Contents/MacOS/XYZLauncher"
cp "$ROOT_DIR/Launcher/Resources/Info.plist" "$APP_PATH/Contents/Info.plist"

if command -v codesign >/dev/null 2>&1; then
    codesign --force --deep --sign - "$APP_PATH" >/dev/null
fi

printf '%s\n' "$APP_PATH"
