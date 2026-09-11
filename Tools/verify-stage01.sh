#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ACCEPTANCE_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xyz-stage01.XXXXXX")"
trap 'rm -rf "$ACCEPTANCE_DIR"' EXIT

printf '%s\n' "== Stage 01B rig assets =="
CHARACTER_DIR="$ROOT_DIR/Assets/Characters/Logen"
test -f "$CHARACTER_DIR/Logen_Master_Right_v1.png"
test -f "$CHARACTER_DIR/Rig/Logen_rig_definition.json"
test -f "$CHARACTER_DIR/Rig/Logen_walk.json"
test -f "$CHARACTER_DIR/Rig/Logen_idle.json"
for RIG_PART in \
    Logen_rig_cloak_back_full.png \
    Logen_rig_cloak_front_left.png \
    Logen_rig_cloak_front_right.png \
    Logen_rig_head_mask_hood.png \
    Logen_rig_left_boot.png \
    Logen_rig_left_forearm_hand.png \
    Logen_rig_left_shin.png \
    Logen_rig_left_thigh.png \
    Logen_rig_left_upper_arm.png \
    Logen_rig_red_cloth_front.png \
    Logen_rig_right_boot.png \
    Logen_rig_right_empty_sleeve.png \
    Logen_rig_right_shin.png \
    Logen_rig_right_thigh.png \
    Logen_rig_torso_upper.png \
    Logen_rig_waist_belt_front.png; do
    test -f "$CHARACTER_DIR/$RIG_PART"
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
    grep -Fq "XYZ Game Stage 01B self-test passed." <<<"$SELF_TEST_OUTPUT"
    printf '%s\n' "$SELF_TEST_OUTPUT"
done

printf '%s\n' "Stage 01B acceptance passed."
