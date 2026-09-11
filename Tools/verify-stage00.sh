#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ACCEPTANCE_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xyz-stage00.XXXXXX")"
trap 'rm -rf "$ACCEPTANCE_DIR"' EXIT

printf '%s\n' "== Swift launcher tests =="
swift test --package-path "$ROOT_DIR/Launcher"
swift build --package-path "$ROOT_DIR/Launcher" -c debug
swift build --package-path "$ROOT_DIR/Launcher" -c release
LAUNCHER_APP="$("$ROOT_DIR/Tools/package-launcher-app.sh")"
test -x "$LAUNCHER_APP/Contents/MacOS/XYZLauncher"

printf '%s\n' "== Debug configure/build/run =="
cmake -S "$ROOT_DIR" -B "$ACCEPTANCE_DIR/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$ACCEPTANCE_DIR/debug" --parallel

DEBUG_OUTPUT="$(XYZ_PROJECT_ROOT="$ROOT_DIR" "$ACCEPTANCE_DIR/debug/Game/XYZGame" --self-test)"
grep -Fq "XYZ Game Stage 01B self-test passed." <<<"$DEBUG_OUTPUT"

printf '%s\n' "== Release configure/build/run =="
cmake -S "$ROOT_DIR" -B "$ACCEPTANCE_DIR/release" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$ACCEPTANCE_DIR/release" --parallel

RELEASE_OUTPUT="$(XYZ_PROJECT_ROOT="$ROOT_DIR" "$ACCEPTANCE_DIR/release/Game/XYZGame" --self-test)"
grep -Fq "XYZ Game Stage 01B self-test passed." <<<"$RELEASE_OUTPUT"

printf '%s\n' "== Non-zero build exit is observable =="
set +e
FAILED_BUILD_OUTPUT="$(cmake --build "$ACCEPTANCE_DIR/debug" --target xyz_missing 2>&1)"
FAILED_BUILD_CODE=$?
set -e
test "$FAILED_BUILD_CODE" -ne 0
printf '%s\n' "$FAILED_BUILD_OUTPUT"

printf '%s\n' "== Clean generated Release output =="
cmake --build "$ACCEPTANCE_DIR/release" --target clean
test ! -x "$ACCEPTANCE_DIR/release/Game/XYZGame"

printf '%s\n' "Stage 00 acceptance passed."
