#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ACCEPTANCE_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xyz-stage01.XXXXXX")"
trap 'rm -rf "$ACCEPTANCE_DIR"' EXIT

printf '%s\n' "== Stage 01C calibrated rig assets =="
CHARACTER_DIR="$ROOT_DIR/Assets/Characters/Logen"
RIG_DIR="$CHARACTER_DIR/RigV3"
test -f "$CHARACTER_DIR/Logen_Master_Right_v1.png"
test -f "$RIG_DIR/Logen_rig_v3_manifest.json"
test -f "$RIG_DIR/Logen_rig_v3_definition.json"
test -f "$RIG_DIR/Logen_walk_v3.json"
test -f "$RIG_DIR/Logen_idle_v3.json"
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
! rg -q 'Logen_walk_right_0[1-8]\.png' "$ROOT_DIR/Game/src" "$ROOT_DIR/Game/include"

printf '%s\n' "== SDL dependencies =="
SDL3_VERSION="$(brew list --versions sdl3)"
SDL3_IMAGE_VERSION="$(brew list --versions sdl3_image)"
test -n "$SDL3_VERSION"
test -n "$SDL3_IMAGE_VERSION"
printf '%s\n' "$SDL3_VERSION"
printf '%s\n' "$SDL3_IMAGE_VERSION"

printf '%s\n' "== Swift launcher tests and builds =="
swift test --package-path "$ROOT_DIR/Launcher"
swift build --package-path "$ROOT_DIR/Launcher" -c debug
swift build --package-path "$ROOT_DIR/Launcher" -c release
LAUNCHER_APP="$("$ROOT_DIR/Tools/package-launcher-app.sh")"
test -x "$LAUNCHER_APP/Contents/MacOS/XYZLauncher"

for CONFIGURATION in Debug Release; do
    CONFIGURATION_DIR="$(printf '%s' "$CONFIGURATION" | tr '[:upper:]' '[:lower:]')"
    BUILD_DIR="$ACCEPTANCE_DIR/$CONFIGURATION_DIR"

    printf '%s\n' "== $CONFIGURATION configure/build =="
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$CONFIGURATION" \
        -DBUILD_TESTING=ON
    cmake --build "$BUILD_DIR" --parallel

    printf '%s\n' "== $CONFIGURATION tests =="
    ctest --test-dir "$BUILD_DIR" --output-on-failure

    printf '%s\n' "== $CONFIGURATION asset self-test =="
    SELF_TEST_OUTPUT="$(XYZ_PROJECT_ROOT="$ROOT_DIR" "$BUILD_DIR/Game/XYZGame" --self-test)"
    grep -Fq "XYZ Game Stage 01C self-test passed." <<<"$SELF_TEST_OUTPUT"
    printf '%s\n' "$SELF_TEST_OUTPUT"
done

printf '%s\n' "Stage 01C acceptance passed."
