import Foundation
import XCTest
@testable import XYZLauncherCore

final class ToolchainDetectorTests: XCTestCase {
    func testDetectorFindsExecutableFilesAndKeepsStableToolOrder() throws {
        let directory = FileManager.default.temporaryDirectory
            .appendingPathComponent("xyz-toolchain-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: directory) }

        try makeExecutable(named: "clang", in: directory, output: "Apple clang fake 1.0")
        try makeExecutable(named: "cmake", in: directory, output: "cmake fake 1.0")
        try Data("#!/bin/sh\nprintf ninja\n".utf8)
            .write(to: directory.appendingPathComponent("ninja"))

        let statuses = ToolchainDetector(searchPaths: [directory]).detect()

        XCTAssertEqual(statuses.map(\.command), ["clang", "cmake", "ninja"])
        XCTAssertTrue(statuses[0].isAvailable)
        XCTAssertTrue(statuses[1].isAvailable)
        XCTAssertFalse(statuses[2].isAvailable)
        XCTAssertEqual(statuses[0].executableURL, directory.appendingPathComponent("clang"))
        XCTAssertTrue(statuses[1].version?.contains("cmake fake") == true)
    }

    private func makeExecutable(named name: String, in directory: URL, output: String) throws {
        let url = directory.appendingPathComponent(name)
        try Data("#!/bin/sh\nprintf '\(output)\\n'\n".utf8).write(to: url)
        try FileManager.default.setAttributes(
            [.posixPermissions: 0o755],
            ofItemAtPath: url.path
        )
    }
}
