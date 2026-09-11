# XYZ — Stage 00 Developer Launcher

Stage 00 establishes the macOS developer workflow. The launcher remains the central build/configure/run tool; the current C++ target is documented in [Stage 01B](Stage-01B.md).

## Prerequisites

- Apple Silicon Mac with Xcode and the macOS SDK
- Swift 6 / Swift Package Manager
- Clang from Xcode Command Line Tools
- CMake 3.24 or newer
- Ninja
- SDL3 and SDL3_image from Homebrew for the current Stage 01 game target

The launcher searches the inherited `PATH`, `/opt/homebrew/bin`, `/usr/local/bin`, `/usr/bin`, and `/bin` for `clang`, `cmake`, and `ninja`. Set `XYZ_PROJECT_ROOT` when launching from a location outside the repository.

## Build and run the launcher

From the repository root:

```bash
swift build --package-path Launcher
./Tools/run-launcher.sh
```

`run-launcher.sh` builds an ad-hoc signed development bundle at `Build/XYZ DEV.app` and opens it with macOS. The executable can also be run directly with `swift run --package-path Launcher` during development.

The window contains the current Debug/Release selector, toolchain status, Configure/Build/Build & Run/Clean actions, folder shortcuts, last build status/time, and a live stdout/stderr console. Build & Run configures the selected tree when needed, builds it, and launches `Game/XYZGame` only after both earlier steps succeed.

## CMake layout

The root project exposes the `XYZEngine` static library and the `XYZGame` executable:

```text
Build/Debug/Game/XYZGame
Build/Release/Game/XYZGame
Build/Logs/last-build.log
```

The launcher owns process execution and output streaming in services instead of SwiftUI views. Future game launch overrides can be added as typed launch options and new manager commands without adding fake controls to the Stage 00 UI.

## Automated verification

Run the complete acceptance check:

```bash
./Tools/verify-stage00.sh
```

It runs all Swift tests, builds the launcher in Debug and Release, configures/builds the current game target in both configurations, runs the Stage 01B asset self-test, verifies a non-zero build exit, and cleans generated Release output in a temporary build tree.
