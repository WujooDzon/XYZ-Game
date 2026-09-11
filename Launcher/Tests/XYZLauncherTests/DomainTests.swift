import Foundation
import XCTest
@testable import XYZLauncherCore

final class DomainTests: XCTestCase {
    func testBuildConfigurationMapsToCMakeValuesAndRawValues() {
        XCTAssertEqual(BuildConfiguration.debug.cmakeValue, "Debug")
        XCTAssertEqual(BuildConfiguration.release.cmakeValue, "Release")
        XCTAssertEqual(BuildConfiguration(rawValue: "Release"), .release)
        XCTAssertEqual(BuildConfiguration(rawValue: "unknown"), nil)
    }

    func testProjectPathsStayInsideConfigurationBuildDirectories() throws {
        let root = try makeProjectRoot()
        let paths = ProjectPaths(rootURL: root)

        XCTAssertTrue(paths.isValidProjectRoot)
        XCTAssertEqual(paths.buildDirectory(for: .debug), root.appendingPathComponent("Build/Debug"))
        XCTAssertEqual(paths.buildDirectory(for: .release), root.appendingPathComponent("Build/Release"))
        XCTAssertEqual(
            paths.gameExecutable(for: .debug),
            root.appendingPathComponent("Build/Debug/Game/XYZGame")
        )
        XCTAssertEqual(paths.logsDirectory, root.appendingPathComponent("Build/Logs"))
        XCTAssertEqual(paths.assetsDirectory, root.appendingPathComponent("Assets"))
    }

    func testProjectPathsRejectDirectoryWithoutRootCMakeFile() throws {
        let root = FileManager.default.temporaryDirectory
            .appendingPathComponent("xyz-invalid-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: root) }

        XCTAssertFalse(ProjectPaths(rootURL: root).isValidProjectRoot)
    }

    func testProjectRootDiscoveryWalksUpFromNestedDirectory() throws {
        let root = try makeProjectRoot()
        let nested = root.appendingPathComponent("Build/Debug", isDirectory: true)
        try FileManager.default.createDirectory(at: nested, withIntermediateDirectories: true)

        XCTAssertEqual(ProjectPaths.findProjectRoot(startingAt: nested), root)
    }

    func testLauncherSettingsDefaultsToDebugAndPersistsValues() throws {
        let suiteName = "XYZLauncherTests.\(UUID().uuidString)"
        let defaults = try XCTUnwrap(UserDefaults(suiteName: suiteName))
        defer { defaults.removePersistentDomain(forName: suiteName) }

        let expectedDate = Date(timeIntervalSince1970: 1_735_000_000)
        let settings = LauncherSettings(defaults: defaults)
        XCTAssertEqual(settings.selectedConfiguration, .debug)
        XCTAssertNil(settings.lastBuildDate)
        XCTAssertNil(settings.lastBuildSucceeded)

        settings.selectedConfiguration = .release
        settings.lastBuildDate = expectedDate
        settings.lastBuildSucceeded = true

        let reloaded = LauncherSettings(defaults: defaults)
        XCTAssertEqual(reloaded.selectedConfiguration, .release)
        XCTAssertEqual(reloaded.lastBuildDate, expectedDate)
        XCTAssertEqual(reloaded.lastBuildSucceeded, true)
    }

    func testBuildLogRendersOutputStreamsAndStatus() throws {
        let log = BuildLog()
        log.append(stream: .stdout, text: "compiler warning")
        log.append(stream: .stderr, text: "compiler error")
        log.appendStatus("exit code 1")

        XCTAssertTrue(log.renderedText.contains("compiler warning"))
        XCTAssertTrue(log.renderedText.contains("compiler error"))
        XCTAssertTrue(log.renderedText.contains("exit code 1"))
    }

    private func makeProjectRoot() throws -> URL {
        let root = FileManager.default.temporaryDirectory
            .appendingPathComponent("xyz-project-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(
            at: root.appendingPathComponent("Assets"),
            withIntermediateDirectories: true
        )
        try FileManager.default.createDirectory(
            at: root.appendingPathComponent("Game"),
            withIntermediateDirectories: true
        )
        try Data("cmake_minimum_required(VERSION 3.24)\n".utf8)
            .write(to: root.appendingPathComponent("CMakeLists.txt"))
        addTeardownBlock { try? FileManager.default.removeItem(at: root) }
        return root
    }
}
