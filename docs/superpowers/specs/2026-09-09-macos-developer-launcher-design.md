# XYZ Stage 00 — macOS Developer Launcher Design

## Scope

Stage 00 creates the developer workflow around a future CMake/C++ game. It does not implement gameplay, rendering, scenes, dialogue, animation, quests, economy, audio, or engine systems. The only C++ runtime artifact is a minimal placeholder executable that proves configure, build, launch, and clean workflows.

## Repository layout

```text
XYZ/
├── Launcher/
│   ├── Package.swift
│   ├── Resources/Info.plist
│   ├── Sources/XYZLauncher/
│   ├── Sources/XYZLauncherApp/
│   └── Tests/XYZLauncherTests/
├── Game/
│   ├── CMakeLists.txt
│   └── src/main.cpp
├── Engine/
│   └── CMakeLists.txt
├── Assets/
├── Tools/
├── Build/
├── Docs/
├── CMakeLists.txt
└── .gitignore
```

The launcher is a Swift Package with a testable `XYZLauncherCore` target and a SwiftUI executable target with a `@main` entry point. It is a native Apple Silicon macOS executable, can be opened by Xcode as a package, can be built with `swift build --package-path Launcher`, and is packaged as an ad-hoc signed `Build/XYZ DEV.app` for normal macOS launching. The package uses only Apple SDK frameworks (`SwiftUI`, `Foundation`, and `AppKit`) and XCTest for tests.

## Components

### LauncherApp

Creates the root dependency graph and the main window. It owns the observable `BuildManager` and `LauncherSettings` instances and passes them to the view hierarchy.

### BuildManager

An observable main-actor service that coordinates configure, build, build-and-run, clean, toolchain refresh, and folder shortcuts. It owns operation state, selected configuration, last build status/time, and the current `BuildLog`. It never constructs UI.

### ProcessRunner

Runs one external process with an explicit executable path, argument list, working directory, and output callback. It forwards stdout and stderr as they arrive, resolves with the process exit code and elapsed time, and reports launch failures as errors. Process logic is isolated here so the manager can be tested with a fake runner.

### ToolchainDetector

Finds `clang`, `cmake`, and `ninja` on the inherited `PATH` plus common Apple Silicon and Intel Homebrew locations. It returns executable paths and optional version strings. The detector does not mutate the system and supports injected search paths for deterministic tests.

### LauncherSettings

Persists the last selected `Debug` or `Release` configuration and the last build timestamp/status in `UserDefaults`. Missing or invalid stored values fall back to `Debug`.

### BuildLog

Stores timestamped stdout, stderr, status, and error entries for the visible console. It exposes a rendered text snapshot for the UI and writes the latest operation log to `Build/Logs/last-build.log` so the Open Logs shortcut has a useful target.

### ProjectPaths and models

`BuildConfiguration`, `BuildOperation`, `BuildState`, `ToolchainStatus`, and `ProjectPaths` are value-oriented models. `ProjectPaths` resolves the project root from `XYZ_PROJECT_ROOT`, the current directory, or an ancestor of the executable, then derives configuration-specific build directories, `Assets`, `Build/Logs`, and `Game/XYZGame` without scattering path strings through the UI.

## Build data flow

```text
UI action
  → BuildManager
    → ToolchainDetector / ProjectPaths
    → ProcessRunner(cmake configure)
    → ProcessRunner(cmake --build)
    → ProcessRunner(placeholder executable)
  → BuildLog + BuildState
  → SwiftUI observation
```

`Build` and `Build & Run` refuse to start when a required tool is missing. `Build & Run` configures only when the selected configuration lacks `CMakeCache.txt`, then builds, and launches the game only when configure and build both return exit code zero. Any non-zero exit code transitions the manager to a failed state and prevents later steps in that operation.

Configure uses the Ninja generator and an isolated directory per configuration:

```text
cmake -S <root> -B <root>/Build/Debug   -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake -S <root> -B <root>/Build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build <build-dir> --parallel
```

Clean removes only the selected generated configuration directory after validating it is inside the project `Build` directory. It does not touch source, assets, or logs. Folder shortcuts create `Build/Logs` when needed and open only project-local directories through `NSWorkspace`.

## UI design

The single native window contains the requested sections: title/status, segmented Debug/Release picker, toolchain rows with checkmarks and versions, build action buttons, development shortcuts, last build status/time, and a monospaced live output console. Buttons are disabled while an operation is running; no future-feature placeholder controls are rendered. The view only binds to observable state and invokes manager methods.

## Error handling

- Tool discovery failures are shown in the toolchain section and the output console.
- Process launch failures and non-zero exit codes are logged with the command, stream, and exit code.
- Build-and-run stops immediately after configure or build failure.
- Log/file-system failures are reported in the console without crashing the app.
- A missing project root produces an actionable state instead of silently running commands elsewhere.

## Future extension boundary

The manager exposes operations and paths through typed models rather than view-specific flags. Future launch overrides can be added as a separate `GameLaunchOptions` value and a new launch command without changing process streaming, build state, settings, or existing UI sections. Stage 00 intentionally does not add those controls or pretend implementations.

## Verification

The package has unit tests for settings persistence, path/configuration mapping, tool detection, process output/exit-code capture, and build sequencing with an injected runner. A repository acceptance script performs launcher build, Debug configure/build/run, Release configure/build, error visibility through a deliberately failing process, and clean verification. The final manual smoke test launches the SwiftUI executable and confirms the visible controls and live console.
