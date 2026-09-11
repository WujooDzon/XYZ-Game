#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ACCEPTANCE_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xyz-stage01c.XXXXXX")"
trap 'rm -rf "$ACCEPTANCE_DIR"' EXIT

printf '%s\n' "== Stage 01C rig audit and source checks =="
test -f "$ROOT_DIR/docs/logen-rig-audit.md"
CHARACTER_DIR="$ROOT_DIR/Assets/Characters/Logen"
test -f "$CHARACTER_DIR/Logen_Master_Right_v1.png"
test -f "$CHARACTER_DIR/Rig/Logen_rig_definition.json"
test -f "$CHARACTER_DIR/Rig/Logen_walk.json"
test -f "$CHARACTER_DIR/Rig/Logen_idle.json"
! rg -q 'Logen_walk_right_0[1-8]\.png|Logen_idle_right_0[1-4]\.png' "$ROOT_DIR/Game/src" "$ROOT_DIR/Game/include"
! rg -q 'right_(hand|forearm)' "$CHARACTER_DIR/Rig/Logen_rig_definition.json"

printf '%s\n' "== Swift launcher =="
swift test --package-path "$ROOT_DIR/Launcher"
swift build --package-path "$ROOT_DIR/Launcher" -c debug
swift build --package-path "$ROOT_DIR/Launcher" -c release
LAUNCHER_APP="$("$ROOT_DIR/Tools/package-launcher-app.sh")"
test -x "$LAUNCHER_APP/Contents/MacOS/XYZLauncher"

for CONFIGURATION in Debug Release; do
    CONFIGURATION_DIR="$(printf '%s' "$CONFIGURATION" | tr '[:upper:]' '[:lower:]')"
    BUILD_DIR="$ACCEPTANCE_DIR/$CONFIGURATION_DIR"
    printf '%s\n' "== $CONFIGURATION configure/build/tests =="
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$CONFIGURATION" \
        -DBUILD_TESTING=ON
    cmake --build "$BUILD_DIR" --parallel
    ctest --test-dir "$BUILD_DIR" --output-on-failure
    SELF_TEST_OUTPUT="$(XYZ_PROJECT_ROOT="$ROOT_DIR" "$BUILD_DIR/Game/XYZGame" --self-test)"
    grep -Fq "XYZ Game Stage 01C self-test passed." <<<"$SELF_TEST_OUTPUT"
    printf '%s\n' "$SELF_TEST_OUTPUT"
done

printf '%s\n' "Stage 01C acceptance passed."
