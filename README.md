# XYZ Game

2D pixel-art narrative game prototype in C++ with a lightweight custom engine and a native macOS SwiftUI developer launcher.

Current milestone: **Stage 01B — Logen cutout-rig animation**.

## Repository layout

- `Launcher/` — macOS SwiftUI developer launcher
- `Engine/` — lightweight C++ engine foundations
- `Game/` — game application and Guffman's Basement scene
- `Assets/` — game art and animation manifests
- `Tools/` — launcher packaging and acceptance scripts
- `docs/` — stage documentation and implementation plans
- `Build/` — local generated build output (ignored by Git)

## Requirements

- macOS on Apple Silicon
- Swift 5.9+
- CMake 3.24+
- Ninja
- Clang
- SDL3 and SDL3_image CMake packages

## Build the game

```sh
cmake -S . -B Build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Build/Debug
ctest --test-dir Build/Debug --output-on-failure
```

Run the placeholder/game executable with:

```sh
XYZ_PROJECT_ROOT="$PWD" ./Build/Debug/Game/XYZGame
```

## Build the launcher

```sh
swift test --package-path Launcher
./Tools/package-launcher-app.sh
open "Build/XYZ DEV.app"
```

The launcher can configure, build, clean, stream output, and run the selected Debug or Release target.

See [`docs/README.md`](docs/README.md) for milestone documentation.
