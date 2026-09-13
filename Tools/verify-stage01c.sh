#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ACCEPTANCE_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xyz-stage01c.XXXXXX")"
trap 'rm -rf "$ACCEPTANCE_DIR"' EXIT

printf '%s\n' "== Stage 01C rig audit and source checks =="
test -f "$ROOT_DIR/docs/logen-rig-audit.md"
CHARACTER_DIR="$ROOT_DIR/Assets/Characters/Logen"
RIG_DIR="$CHARACTER_DIR/RigV3"
test -f "$CHARACTER_DIR/Logen_Master_Right_v1.png"
test -f "$RIG_DIR/Logen_rig_v3_manifest.json"
test -f "$RIG_DIR/Logen_rig_v3_definition.json"
test -f "$RIG_DIR/Logen_walk_v3.json"
test -f "$RIG_DIR/Logen_idle_v3.json"
test -f "$RIG_DIR/README.md"
for FRAME in 01 02 03 04 05 06 07 08; do
    test -f "$CHARACTER_DIR/WalkV2/Logen_walk_right_${FRAME}.png"
done
rg -q 'WalkV2/Logen_walk_right_01\.png' "$ROOT_DIR/Game/src"
! rg -q 'right_(hand|forearm)' "$RIG_DIR/Logen_rig_v3_definition.json"
for RIG_PART in \
    Logen_rig_v3_body_shell.png \
    Logen_rig_v3_left_arm.png \
    Logen_rig_v3_cloak_tail.png \
    Logen_rig_v3_cloak_front.png \
    Logen_rig_v3_far_thigh.png \
    Logen_rig_v3_far_shin.png \
    Logen_rig_v3_far_boot.png \
    Logen_rig_v3_near_thigh.png \
    Logen_rig_v3_near_shin.png \
    Logen_rig_v3_near_boot.png; do
    test -f "$RIG_DIR/$RIG_PART"
done

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
    grep -Fq "XYZ Game Stage 01D self-test passed." <<<"$SELF_TEST_OUTPUT"
    printf '%s\n' "$SELF_TEST_OUTPUT"
done

printf '%s\n' "Stage 01C acceptance passed."
