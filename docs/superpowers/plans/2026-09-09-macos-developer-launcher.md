# macOS Developer Launcher Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and verify a native SwiftUI macOS developer launcher around a minimal CMake/Ninja C++ placeholder target, without implementing game or engine systems.

**Architecture:** A Swift Package core target owns testable launcher services while a separate SwiftUI executable target provides the macOS shell. `BuildManager` orchestrates typed build operations through `ProcessRunner`, `ToolchainDetector`, `ProjectPaths`, `LauncherSettings`, and `BuildLog`. The root CMake project exposes an empty interface engine target and a placeholder game executable, with separate `Build/Debug` and `Build/Release` trees.

**Tech Stack:** Swift 6 language mode 5, SwiftUI, Foundation, AppKit, XCTest, C++20, CMake, Ninja, Clang, Apple Silicon macOS.

**Spec:** `docs/superpowers/specs/2026-09-09-macos-developer-launcher-design.md`

## Global Constraints

- Do not implement gameplay, rendering, scenes, dialogue, animation, quests, economy, audio, or engine systems.
- The launcher is a native macOS SwiftUI executable in `Launcher/`, built with Swift Package Manager and packaged as `Build/XYZ DEV.app` for launching.
- CMake uses Ninja and an isolated build directory for each `Debug`/`Release` configuration.
- The UI contains only currently implemented actions; do not render fake future-feature buttons.
- Process output must be streamed as stdout/stderr and non-zero exit codes must stop dependent operations.
- Clean may remove only generated files inside the selected project-local `Build/<Configuration>` directory.

---

### Task 1: Repository scaffolding and placeholder CMake target

**Files:**
- Create: `CMakeLists.txt`
- Create: `Game/CMakeLists.txt`
- Create: `Game/src/main.cpp`
- Create: `Engine/CMakeLists.txt`
- Create: `Assets/.gitkeep`
- Create: `Build/.gitkeep`
- Create: `Tools/.gitkeep`
- Create: `Docs/README.md`
- Create: `.gitignore`

**Interfaces:**
- Produces CMake target `XYZGame` at `<build-dir>/Game/XYZGame`.
- Produces an `XYZEngine` interface target with no engine implementation.

- [ ] **Step 1: Write the failing acceptance probe**

Create a temporary command expectation in `Tools/verify-stage00.sh` that configures a clean temporary Debug tree, builds `XYZGame`, runs it, and asserts the output contains `XYZ Game placeholder — Stage 00`. Keep the script executable and make it fail clearly while the root CMake files do not exist.

- [ ] **Step 2: Run the probe to verify it fails**

Run:

```bash
./Tools/verify-stage00.sh
```

Expected: FAIL during CMake configure because the root project has not been created.

- [ ] **Step 3: Add the minimal CMake project and placeholder executable**

Use `cmake_minimum_required(VERSION 3.24)`, `project(XYZ LANGUAGES CXX)`, C++20, and `add_subdirectory(Engine)` / `add_subdirectory(Game)`. Define `XYZEngine` as `add_library(XYZEngine INTERFACE)`. Define `XYZGame` from one `main.cpp`, link `XYZEngine`, print the exact placeholder line, and return zero. Do not add a game loop or gameplay code.

- [ ] **Step 4: Run the acceptance probe to verify it passes**

Run:

```bash
./Tools/verify-stage00.sh
```

Expected: CMake configure succeeds with Ninja, the executable builds, and its stdout contains the placeholder line.

- [ ] **Step 5: Commit when Git is available**

```bash
git add CMakeLists.txt Game Engine Assets Build Tools Docs .gitignore
git commit -m "chore: scaffold XYZ CMake project"
```

If the workspace is not a Git checkout, retain the files and record that commit creation was unavailable.

### Task 2: Swift package, domain models, settings, and log state

**Files:**
- Create: `Launcher/Package.swift`
- Create: `Launcher/Resources/Info.plist`
- Create: `Launcher/Tests/XYZLauncherTests/DomainTests.swift`
- Create: `Launcher/Sources/XYZLauncher/Models/BuildConfiguration.swift`
- Create: `Launcher/Sources/XYZLauncher/Models/ProjectPaths.swift`
- Create: `Launcher/Sources/XYZLauncher/Models/BuildState.swift`
- Create: `Launcher/Sources/XYZLauncher/Models/ToolchainStatus.swift`
- Create: `Launcher/Sources/XYZLauncher/Services/LauncherSettings.swift`
- Create: `Launcher/Sources/XYZLauncher/Services/BuildLog.swift`

**Interfaces:**
- `BuildConfiguration` maps `.debug` to `Debug` and `.release` to `Release` and persists as a string.
- `ProjectPaths(rootURL:)` exposes `buildDirectory(for:)`, `gameExecutable(for:)`, `logsDirectory`, `assetsDirectory`, and `isValidProjectRoot`.
- `LauncherSettings` exposes `selectedConfiguration`, `lastBuildDate`, and `lastBuildSucceeded`.
- `BuildLog.append(stream:text:)`, `appendStatus(_:)`, `renderedText`, and `write(to:)` are available to the manager and UI.

- [ ] **Step 1: Write failing tests**

Add tests that assert configuration mapping, project-local Debug/Release paths, invalid-root rejection, fresh settings defaulting to Debug, persistence of Release and last-build metadata, and log rendering that includes stdout/stderr/status entries.

- [ ] **Step 2: Run only the new tests**

Run:

```bash
swift test --package-path Launcher --filter DomainTests
```

Expected: FAIL because the package and model types do not yet exist.

- [ ] **Step 3: Implement the minimal models and package manifest**

Declare a macOS 14 platform, a testable `XYZLauncherCore` target, and one test target. The executable target is added with the SwiftUI shell task. Implement only behavior asserted by the tests; use injected `UserDefaults` in `LauncherSettings` so tests do not touch the user's defaults.

- [ ] **Step 4: Run the tests again**

Run the same command and expect all domain tests to pass.

- [ ] **Step 5: Commit when Git is available**

```bash
git add Launcher/Package.swift Launcher/Sources/XYZLauncher/Models Launcher/Sources/XYZLauncher/Services Launcher/Tests/XYZLauncherTests/DomainTests.swift
git commit -m "feat: add launcher domain state"
```

### Task 3: Live process runner

**Files:**
- Create: `Launcher/Tests/XYZLauncherTests/ProcessRunnerTests.swift`
- Create: `Launcher/Sources/XYZLauncher/Services/ProcessRunner.swift`

**Interfaces:**
- `ProcessRunner.run(executable:arguments:workingDirectory:onOutput:) async throws -> ProcessResult`.
- `ProcessResult` contains `exitCode` and `duration`.
- `ProcessStream` distinguishes `.stdout` and `.stderr`.
- `ProcessRunnerError` describes launch failures.

- [ ] **Step 1: Write failing process tests**

Run `/bin/sh -c "printf out; printf err >&2; exit 7"`, collect callback events, and assert both streams are observed and the result exit code is `7`. Add a second test for a successful command and non-negative duration.

- [ ] **Step 2: Run the tests and confirm the expected failure**

```bash
swift test --package-path Launcher --filter ProcessRunnerTests
```

Expected: compile/test failure because `ProcessRunner` is absent.

- [ ] **Step 3: Implement the runner**

Use `Foundation.Process` and separate `Pipe` instances for stdout/stderr. Attach readability handlers before launch, forward decoded chunks immediately, resolve on termination, remove handlers, and preserve the exit code. Never use shell interpolation for build commands.

- [ ] **Step 4: Run the focused tests**

Run the same command and expect both tests to pass with captured stdout and stderr.

- [ ] **Step 5: Commit when Git is available**

```bash
git add Launcher/Sources/XYZLauncher/Services/ProcessRunner.swift Launcher/Tests/XYZLauncherTests/ProcessRunnerTests.swift
git commit -m "feat: stream external process output"
```

### Task 4: Toolchain detection

**Files:**
- Create: `Launcher/Tests/XYZLauncherTests/ToolchainDetectorTests.swift`
- Create: `Launcher/Sources/XYZLauncher/Services/ToolchainDetector.swift`

**Interfaces:**
- `ToolchainDetector(searchPaths:)`.
- `detect() -> [ToolchainStatus]` returns entries for `clang`, `cmake`, and `ninja`.
- `ToolchainStatus` contains tool name, command, optional executable URL, optional version, and `isAvailable`.

- [ ] **Step 1: Write failing detector tests**

Create a temporary directory containing an executable fake `cmake` and a non-executable `ninja`, pass that directory plus a known clang directory to the detector, and assert the available flags and executable URLs. Also assert the returned list has all three tools in stable order.

- [ ] **Step 2: Run the focused tests**

```bash
swift test --package-path Launcher --filter ToolchainDetectorTests
```

Expected: FAIL because the detector is absent.

- [ ] **Step 3: Implement detection**

Search the injected paths, inherited `PATH`, `/opt/homebrew/bin`, `/usr/local/bin`, and `/usr/bin` for executable files. Use `ProcessRunner` or a small Foundation process helper to read `--version`; a version read failure must not hide an otherwise executable tool.

- [ ] **Step 4: Run the focused tests**

Expect all detector tests to pass.

- [ ] **Step 5: Commit when Git is available**

```bash
git add Launcher/Sources/XYZLauncher/Services/ToolchainDetector.swift Launcher/Tests/XYZLauncherTests/ToolchainDetectorTests.swift
git commit -m "feat: detect launcher toolchain"
```

### Task 5: BuildManager orchestration

**Files:**
- Create: `Launcher/Tests/XYZLauncherTests/BuildManagerTests.swift`
- Create: `Launcher/Sources/XYZLauncher/Services/BuildManager.swift`

**Interfaces:**
- `BuildManager.configure() async`.
- `BuildManager.build() async`.
- `BuildManager.buildAndRun() async`.
- `BuildManager.clean() async`.
- `BuildManager.refreshToolchain()`.
- An injected `ProcessRunning` protocol allows tests to record commands and return chosen exit codes.

- [ ] **Step 1: Write failing orchestration tests**

Use a fake runner and temporary project root to assert: configure uses `cmake -S/-B/-G Ninja/-DCMAKE_BUILD_TYPE=...`; build follows successful configure; build-and-run does not launch after a non-zero build; a configured tree skips redundant configure; and clean removes only the selected configuration directory.

- [ ] **Step 2: Run focused tests and verify red**

```bash
swift test --package-path Launcher --filter BuildManagerTests
```

Expected: FAIL because `BuildManager` and its protocol do not exist.

- [ ] **Step 3: Implement the manager**

Keep the manager `@MainActor` and observable. Append every process output to `BuildLog`, set running/success/failure state, store the last successful build date, and stop immediately on any non-zero result. Treat `Build/Debug` and `Build/Release` as separate configuration trees. Launch the executable only after a successful build.

- [ ] **Step 4: Run focused tests and then all package tests**

```bash
swift test --package-path Launcher --filter BuildManagerTests
swift test --package-path Launcher
```

Expected: all tests pass.

- [ ] **Step 5: Commit when Git is available**

```bash
git add Launcher/Sources/XYZLauncher/Services/BuildManager.swift Launcher/Tests/XYZLauncherTests/BuildManagerTests.swift
git commit -m "feat: orchestrate configure build and run"
```

### Task 6: SwiftUI launcher UI and app entry point

**Files:**
- Create: `Launcher/Sources/XYZLauncher/LauncherApp.swift`
- Create: `Launcher/Sources/XYZLauncher/Views/ContentView.swift`
- Create: `Launcher/Sources/XYZLauncher/Views/Components/ToolchainRow.swift`
- Create: `Launcher/Sources/XYZLauncher/Views/Components/BuildOutputView.swift`

**Interfaces:**
- `LauncherApp` creates the settings, paths, detector, process runner, and manager.
- `ContentView` observes the manager and settings, showing only implemented controls.

- [ ] **Step 1: Add a UI compile smoke test expectation**

Run `swift build --package-path Launcher` before adding the app files and record the expected failure that the executable target has no source entry point.

- [ ] **Step 2: Implement the native SwiftUI shell**

Add a macOS 14 `@main` app and a single-window layout with the requested sections, segmented Debug/Release picker, toolchain checks, action buttons, folder shortcuts, last build metadata, and a monospaced scrollable live console. Use `NSWorkspace.shared.open` only through manager methods, not directly from view action bodies.

- [ ] **Step 3: Build the launcher**

```bash
swift build --package-path Launcher -c release
```

Expected: exit code zero with no Swift compiler errors.

- [ ] **Step 4: Commit when Git is available**

```bash
git add Launcher/Sources/XYZLauncher
git commit -m "feat: add SwiftUI developer launcher"
```

### Task 7: Developer scripts and documentation

**Files:**
- Modify: `Tools/verify-stage00.sh`
- Create: `Tools/run-launcher.sh`
- Create: `Tools/package-launcher-app.sh`
- Create: `Docs/Stage-00.md`
- Modify: `Docs/README.md`

**Interfaces:**
- `Tools/verify-stage00.sh` is a repeatable acceptance test for the CMake placeholder and launcher package.
- `Tools/run-launcher.sh` packages and opens the launcher from the project root.

- [ ] **Step 1: Extend the acceptance script**

Add commands that build the launcher, configure/build/run Debug, configure/build/run Release, assert the placeholder output, run a failing command to verify a non-zero exit is visible to the shell, and clean the selected build directory. Use `set -euo pipefail` and project-local temporary paths.

- [ ] **Step 2: Add launcher bundle/run helper and docs**

Package the release executable with `Launcher/Resources/Info.plist` into `Build/XYZ DEV.app`, ad-hoc sign it when `codesign` is available, make `run-launcher.sh` open the bundle, and document prerequisites, commands, UI behavior, generated paths, the placeholder-only scope, and how future engine work can add typed manager commands without adding fake UI controls.

- [ ] **Step 3: Run the acceptance script**

```bash
./Tools/verify-stage00.sh
```

Expected: all CMake and Swift checks pass, Debug and Release placeholder runs print the expected line, and clean removes generated configuration output.

- [ ] **Step 4: Commit when Git is available**

```bash
git add Tools Docs
git commit -m "docs: document Stage 00 developer workflow"
```

### Task 8: Final Definition of Done verification

**Files:**
- Modify only files required by fixes discovered during verification.

- [ ] **Step 1: Run formatting/structure checks**

```bash
find Launcher Game Engine Assets Tools Build Docs -maxdepth 3 -print | sort
```

Confirm the required top-level structure exists and no gameplay/engine subsystem files were added.

- [ ] **Step 2: Run the complete automated checks**

```bash
swift test --package-path Launcher
swift build --package-path Launcher -c release
./Tools/verify-stage00.sh
```

- [ ] **Step 3: Perform the launcher smoke test**

Launch with `swift run --package-path Launcher`, confirm the window opens, confirm clang/cmake/ninja rows are available, select Debug and Release, run Configure/Build/Build & Run, observe live output, then run Clean.

- [ ] **Step 4: Review the requirements checklist**

Confirm each of the ten Definition of Done items from the spec has direct command or UI evidence. Stop after Stage 00; do not add future gameplay or engine functionality.
