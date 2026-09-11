import AppKit
import Foundation
import Observation

@MainActor
@Observable
public final class BuildManager {
    public let paths: ProjectPaths
    public let settings: LauncherSettings
    public let buildLog = BuildLog()

    private let processRunner: ProcessRunning
    private let toolchainDetector: ToolchainDetecting

    public private(set) var state: BuildState = .idle
    public private(set) var toolchainStatuses: [ToolchainStatus] = []

    public var selectedConfiguration: BuildConfiguration {
        get { settings.selectedConfiguration }
        set { settings.selectedConfiguration = newValue }
    }

    public var lastBuildDate: Date? { settings.lastBuildDate }
    public var lastBuildSucceeded: Bool? { settings.lastBuildSucceeded }
    public var isRunning: Bool {
        if case .running = state { return true }
        return false
    }

    public init(
        paths: ProjectPaths,
        processRunner: ProcessRunning = ProcessRunner(),
        toolchainDetector: ToolchainDetecting = ToolchainDetector(),
        settings: LauncherSettings = LauncherSettings()
    ) {
        self.paths = paths
        self.processRunner = processRunner
        self.toolchainDetector = toolchainDetector
        self.settings = settings
        refreshToolchain()
    }

    public func refreshToolchain() {
        toolchainStatuses = toolchainDetector.detect()
    }

    public func configure() async {
        guard begin(.configure) else { return }
        guard let cmakePath = requiredToolPath(for: "cmake") else {
            finishFailure(.configure, exitCode: nil, message: "CMake is unavailable.")
            return
        }
        guard requiredToolPath(for: "clang") != nil,
              requiredToolPath(for: "ninja") != nil else {
            finishFailure(.configure, exitCode: nil, message: "The complete toolchain is unavailable.")
            return
        }

        if await configureStep(cmakePath: cmakePath) {
            finishSuccess(.configure)
        } else {
            finishFailure(.configure, exitCode: lastCommandExitCode, message: "Configure failed.")
        }
    }

    public func build() async {
        guard begin(.build) else { return }
        guard let cmakePath = requiredToolPath(for: "cmake"),
              requiredToolPath(for: "clang") != nil,
              requiredToolPath(for: "ninja") != nil else {
            finishFailure(.build, exitCode: nil, message: "The complete toolchain is unavailable.")
            return
        }

        if !isConfigured {
            guard await configureStep(cmakePath: cmakePath) else {
                finishFailure(.build, exitCode: lastCommandExitCode, message: "Configure failed; build stopped.")
                return
            }
        }

        guard await buildStep(cmakePath: cmakePath) else {
            finishFailure(.build, exitCode: lastCommandExitCode, message: "Build failed.")
            return
        }
        finishSuccess(.build)
    }

    public func buildAndRun(arguments: [String] = []) async {
        guard begin(.buildAndRun) else { return }
        guard let cmakePath = requiredToolPath(for: "cmake"),
              requiredToolPath(for: "clang") != nil,
              requiredToolPath(for: "ninja") != nil else {
            finishFailure(.buildAndRun, exitCode: nil, message: "The complete toolchain is unavailable.")
            return
        }

        if !isConfigured {
            guard await configureStep(cmakePath: cmakePath) else {
                finishFailure(.buildAndRun, exitCode: lastCommandExitCode, message: "Configure failed; run stopped.")
                return
            }
        }

        guard await buildStep(cmakePath: cmakePath) else {
            finishFailure(.buildAndRun, exitCode: lastCommandExitCode, message: "Build failed; launch skipped.")
            return
        }

        let executable = paths.gameExecutable(for: selectedConfiguration)
        guard FileManager.default.isExecutableFile(atPath: executable.path) else {
            finishFailure(.buildAndRun, exitCode: nil, message: "Built game executable was not found at \(executable.path).")
            return
        }

        guard let result = await runCommand(
            label: "Launching game",
            executable: executable.path,
            arguments: arguments,
            workingDirectory: paths.rootURL
        ) else {
            finishFailure(.buildAndRun, exitCode: nil, message: "Game launch failed.")
            return
        }

        guard result.exitCode == 0 else {
            finishFailure(.buildAndRun, exitCode: result.exitCode, message: "Game exited with a non-zero code.")
            return
        }
        finishSuccess(.buildAndRun)
    }

    public func clean() async {
        guard begin(.clean) else { return }
        let buildDirectory = paths.buildDirectory(for: selectedConfiguration).standardizedFileURL
        let buildRoot = paths.buildRoot.standardizedFileURL
        let allowedPrefix = buildRoot.path.hasSuffix("/") ? buildRoot.path : buildRoot.path + "/"

        guard buildDirectory.path.hasPrefix(allowedPrefix) else {
            finishFailure(.clean, exitCode: nil, message: "Refusing to clean outside the project Build directory.")
            return
        }

        do {
            if FileManager.default.fileExists(atPath: buildDirectory.path) {
                try FileManager.default.removeItem(at: buildDirectory)
            }
            finishSuccess(.clean)
        } catch {
            finishFailure(.clean, exitCode: nil, message: "Clean failed: \(error.localizedDescription)")
        }
    }

    public func clearOutput() {
        buildLog.clear()
    }

    public func openBuildFolder() {
        openFolder(paths.buildRoot)
    }

    public func openAssetsFolder() {
        openFolder(paths.assetsDirectory)
    }

    public func openLogsFolder() {
        openFolder(paths.logsDirectory)
    }

    private var isConfigured: Bool {
        FileManager.default.fileExists(
            atPath: paths.buildDirectory(for: selectedConfiguration)
                .appendingPathComponent("CMakeCache.txt")
                .path
        )
    }

    private var lastCommandExitCode: Int32? = nil

    private func begin(_ operation: BuildOperation) -> Bool {
        guard !isRunning else { return false }
        lastCommandExitCode = nil
        buildLog.clear()
        buildLog.appendStatus("\(operation.rawValue) started for \(selectedConfiguration.rawValue).")
        state = .running(operation)
        return true
    }

    private func configureStep(cmakePath: String) async -> Bool {
        do {
            try FileManager.default.createDirectory(
                at: paths.buildDirectory(for: selectedConfiguration),
                withIntermediateDirectories: true
            )
        } catch {
            buildLog.appendStatus("Could not create build directory: \(error.localizedDescription)")
            return false
        }

        guard let result = await runCommand(
            label: "Configuring with CMake",
            executable: cmakePath,
            arguments: [
                "-S", paths.rootURL.path,
                "-B", paths.buildDirectory(for: selectedConfiguration).path,
                "-G", "Ninja",
                "-DCMAKE_BUILD_TYPE=\(selectedConfiguration.cmakeValue)"
            ],
            workingDirectory: paths.rootURL
        ) else {
            return false
        }
        lastCommandExitCode = result.exitCode
        return result.exitCode == 0
    }

    private func buildStep(cmakePath: String) async -> Bool {
        guard let result = await runCommand(
            label: "Building \(selectedConfiguration.rawValue)",
            executable: cmakePath,
            arguments: [
                "--build", paths.buildDirectory(for: selectedConfiguration).path,
                "--parallel"
            ],
            workingDirectory: paths.rootURL
        ) else {
            return false
        }
        lastCommandExitCode = result.exitCode
        return result.exitCode == 0
    }

    private func runCommand(
        label: String,
        executable: String,
        arguments: [String],
        workingDirectory: URL
    ) async -> ProcessResult? {
        buildLog.appendStatus("\(label): \(executable) \(arguments.joined(separator: " "))")
        do {
            let result = try await processRunner.run(
                executable: executable,
                arguments: arguments,
                workingDirectory: workingDirectory
            ) { [weak self] stream, text in
                Task { @MainActor [weak self] in
                    self?.buildLog.append(stream: stream, text: text)
                }
            }
            lastCommandExitCode = result.exitCode
            buildLog.appendStatus("\(label) exited with code \(result.exitCode) after \(String(format: "%.2f", result.duration))s.")
            return result
        } catch {
            buildLog.appendStatus("\(label) could not start: \(error.localizedDescription)")
            return nil
        }
    }

    private func requiredToolPath(for command: String) -> String? {
        guard let status = toolchainStatuses.first(where: { $0.command == command }),
              let executableURL = status.executableURL else {
            buildLog.appendStatus("Missing tool: \(command)")
            return nil
        }
        return executableURL.path
    }

    private func finishSuccess(_ operation: BuildOperation) {
        state = .succeeded(operation)
        buildLog.appendStatus("\(operation.rawValue) succeeded.")
        updateLastBuildMetadata(for: operation, succeeded: true)
        persistLog()
    }

    private func finishFailure(_ operation: BuildOperation, exitCode: Int32?, message: String) {
        state = .failed(operation, exitCode: exitCode)
        buildLog.appendStatus(message)
        updateLastBuildMetadata(for: operation, succeeded: false)
        persistLog()
    }

    private func updateLastBuildMetadata(for operation: BuildOperation, succeeded: Bool) {
        guard operation == .build || operation == .buildAndRun else { return }
        settings.lastBuildDate = Date()
        settings.lastBuildSucceeded = succeeded
    }

    private func persistLog() {
        do {
            try buildLog.write(to: paths.logsDirectory.appendingPathComponent("last-build.log"))
        } catch {
            buildLog.appendStatus("Could not write log file: \(error.localizedDescription)")
        }
    }

    private func openFolder(_ url: URL) {
        do {
            try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
            NSWorkspace.shared.open(url)
        } catch {
            buildLog.appendStatus("Could not open folder \(url.path): \(error.localizedDescription)")
        }
    }
}
