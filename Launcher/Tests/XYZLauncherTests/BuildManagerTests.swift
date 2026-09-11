import Foundation
import XCTest
@testable import XYZLauncherCore

@MainActor
final class BuildManagerTests: XCTestCase {
    func testConfigureUsesNinjaAndSelectedConfiguration() async throws {
        let fixture = try makeFixture()
        let runner = RecordingRunner(responses: [0])
        let manager = makeManager(root: fixture, runner: runner)

        await manager.configure()

        XCTAssertEqual(runner.calls.count, 1)
        XCTAssertEqual(
            runner.calls[0].arguments,
            [
                "-S", fixture.path,
                "-B", fixture.appendingPathComponent("Build/Debug").path,
                "-G", "Ninja",
                "-DCMAKE_BUILD_TYPE=Debug"
            ]
        )
        XCTAssertEqual(manager.state, .succeeded(.configure))
    }

    func testBuildAutoConfiguresWhenSelectedTreeIsNotConfigured() async throws {
        let fixture = try makeFixture()
        let runner = RecordingRunner(responses: [0, 0])
        let manager = makeManager(root: fixture, runner: runner)

        await manager.build()

        XCTAssertEqual(runner.calls.count, 2)
        XCTAssertTrue(runner.calls[0].arguments.contains("-G"))
        XCTAssertEqual(runner.calls[1].arguments.first, "--build")
        XCTAssertEqual(manager.state, .succeeded(.build))
        XCTAssertEqual(manager.settings.lastBuildSucceeded, true)
    }

    func testBuildAndRunStopsBeforeLaunchWhenBuildFails() async throws {
        let fixture = try makeFixture()
        try createConfiguredTree(at: fixture)
        let runner = RecordingRunner(responses: [7])
        let manager = makeManager(root: fixture, runner: runner)

        await manager.buildAndRun()

        XCTAssertEqual(runner.calls.count, 1)
        XCTAssertEqual(manager.state, .failed(.buildAndRun, exitCode: 7))
        XCTAssertEqual(manager.settings.lastBuildSucceeded, false)
    }

    func testBuildAndRunLaunchesOnlyAfterSuccessfulBuild() async throws {
        let fixture = try makeFixture()
        try createConfiguredTree(at: fixture)
        let executable = fixture.appendingPathComponent("Build/Debug/Game/XYZGame")
        try FileManager.default.createDirectory(
            at: executable.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )
        try Data("#!/bin/sh\n".utf8).write(to: executable)
        try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: executable.path)
        let runner = RecordingRunner(responses: [0, 0])
        let manager = makeManager(root: fixture, runner: runner)

        await manager.buildAndRun()

        XCTAssertEqual(runner.calls.count, 2)
        XCTAssertEqual(runner.calls[0].arguments.first, "--build")
        XCTAssertEqual(runner.calls[1].executable, executable.path)
        XCTAssertEqual(manager.state, .succeeded(.buildAndRun))
    }

    func testBuildAndRunPassesLaunchArgumentsToGame() async throws {
        let fixture = try makeFixture()
        try createConfiguredTree(at: fixture)
        let executable = fixture.appendingPathComponent("Build/Debug/Game/XYZGame")
        try FileManager.default.createDirectory(
            at: executable.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )
        try Data("#!/bin/sh\n".utf8).write(to: executable)
        try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: executable.path)
        let runner = RecordingRunner(responses: [0, 0])
        let manager = makeManager(root: fixture, runner: runner)

        await manager.buildAndRun(arguments: ["--self-test"])

        XCTAssertEqual(runner.calls.last?.arguments, ["--self-test"])
        XCTAssertEqual(manager.state, .succeeded(.buildAndRun))
    }

    func testCleanRemovesOnlySelectedConfigurationTree() async throws {
        let fixture = try makeFixture()
        try createConfiguredTree(at: fixture)
        let releaseMarker = fixture.appendingPathComponent("Build/Release/keep.txt")
        try FileManager.default.createDirectory(
            at: releaseMarker.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )
        try Data("keep".utf8).write(to: releaseMarker)
        let manager = makeManager(root: fixture, runner: RecordingRunner(responses: []))

        await manager.clean()

        XCTAssertFalse(FileManager.default.fileExists(atPath: fixture.appendingPathComponent("Build/Debug").path))
        XCTAssertTrue(FileManager.default.fileExists(atPath: releaseMarker.path))
        XCTAssertEqual(manager.state, .succeeded(.clean))
    }

    private func makeManager(root: URL, runner: RecordingRunner) -> BuildManager {
        BuildManager(
            paths: ProjectPaths(rootURL: root),
            processRunner: runner,
            toolchainDetector: FakeToolchainDetector(),
            settings: LauncherSettings(defaults: UserDefaults(suiteName: "XYZManagerTests.\(UUID().uuidString)" )!)
        )
    }

    private func makeFixture() throws -> URL {
        let root = FileManager.default.temporaryDirectory
            .appendingPathComponent("xyz-manager-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
        try Data("cmake_minimum_required(VERSION 3.24)\n".utf8)
            .write(to: root.appendingPathComponent("CMakeLists.txt"))
        addTeardownBlock { try? FileManager.default.removeItem(at: root) }
        return root
    }

    private func createConfiguredTree(at root: URL) throws {
        let directory = root.appendingPathComponent("Build/Debug")
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        try Data("# fake cache\n".utf8).write(to: directory.appendingPathComponent("CMakeCache.txt"))
    }
}

private final class RecordingRunner: ProcessRunning {
    struct Call {
        let executable: String
        let arguments: [String]
        let workingDirectory: URL
    }

    private(set) var calls: [Call] = []
    private var responses: [Int32]

    init(responses: [Int32]) {
        self.responses = responses
    }

    func run(
        executable: String,
        arguments: [String],
        workingDirectory: URL,
        onOutput: @escaping (ProcessStream, String) -> Void
    ) async throws -> ProcessResult {
        calls.append(Call(executable: executable, arguments: arguments, workingDirectory: workingDirectory))
        onOutput(.stdout, "simulated process output")
        return ProcessResult(exitCode: responses.isEmpty ? 0 : responses.removeFirst(), duration: 0)
    }
}

private struct FakeToolchainDetector: ToolchainDetecting {
    func detect() -> [ToolchainStatus] {
        ["clang", "cmake", "ninja"].map {
            ToolchainStatus(
                command: $0,
                executableURL: URL(fileURLWithPath: "/fake/toolchain/\($0)"),
                version: "fake 1.0"
            )
        }
    }
}
