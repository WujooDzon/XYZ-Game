import Foundation
import XCTest
@testable import XYZLauncherCore

final class ProcessRunnerTests: XCTestCase {
    func testRunnerStreamsBothOutputChannelsAndPreservesExitCode() async throws {
        let collector = OutputCollector()
        let result = try await ProcessRunner().run(
            executable: "/bin/sh",
            arguments: ["-c", "printf out; printf err >&2; exit 7"],
            workingDirectory: FileManager.default.temporaryDirectory
        ) { stream, text in
            collector.append(stream: stream, text: text)
        }

        XCTAssertEqual(result.exitCode, 7)
        XCTAssertTrue(collector.events.contains(where: { $0.stream == .stdout && $0.text.contains("out") }))
        XCTAssertTrue(collector.events.contains(where: { $0.stream == .stderr && $0.text.contains("err") }))
    }

    func testRunnerReportsSuccessfulCompletionAndDuration() async throws {
        let result = try await ProcessRunner().run(
            executable: "/bin/sh",
            arguments: ["-c", "printf success"],
            workingDirectory: FileManager.default.temporaryDirectory,
            onOutput: { _, _ in }
        )

        XCTAssertEqual(result.exitCode, 0)
        XCTAssertGreaterThanOrEqual(result.duration, 0)
    }
}

private final class OutputCollector {
    struct Event {
        let stream: LogStream
        let text: String
    }

    private let lock = NSLock()
    private var storedEvents: [Event] = []

    var events: [Event] {
        lock.lock()
        defer { lock.unlock() }
        return storedEvents
    }

    func append(stream: LogStream, text: String) {
        lock.lock()
        storedEvents.append(Event(stream: stream, text: text))
        lock.unlock()
    }
}
