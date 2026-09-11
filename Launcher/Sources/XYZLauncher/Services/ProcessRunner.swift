import Foundation

public typealias ProcessStream = LogStream

public struct ProcessResult: Sendable {
    public let exitCode: Int32
    public let duration: TimeInterval

    public init(exitCode: Int32, duration: TimeInterval) {
        self.exitCode = exitCode
        self.duration = duration
    }
}

public enum ProcessRunnerError: LocalizedError {
    case failedToLaunch(executable: String, reason: String)

    public var errorDescription: String? {
        switch self {
        case let .failedToLaunch(executable, reason):
            return "Failed to launch \(executable): \(reason)"
        }
    }
}

public protocol ProcessRunning: AnyObject {
    func run(
        executable: String,
        arguments: [String],
        workingDirectory: URL,
        onOutput: @escaping (ProcessStream, String) -> Void
    ) async throws -> ProcessResult
}

public final class ProcessRunner: ProcessRunning {
    public init() {}

    public func run(
        executable: String,
        arguments: [String],
        workingDirectory: URL,
        onOutput: @escaping (ProcessStream, String) -> Void
    ) async throws -> ProcessResult {
        try await withCheckedThrowingContinuation { continuation in
            let process = Process()
            let standardOutput = Pipe()
            let standardError = Pipe()
            let state = ProcessExecutionState(
                continuation: continuation,
                startDate: Date(),
                onOutput: onOutput
            )

            process.executableURL = URL(fileURLWithPath: executable)
            process.arguments = arguments
            process.currentDirectoryURL = workingDirectory
            process.standardOutput = standardOutput
            process.standardError = standardError

            attachReader(to: standardOutput, stream: .stdout, state: state)
            attachReader(to: standardError, stream: .stderr, state: state)

            process.terminationHandler = { process in
                standardOutput.fileHandleForReading.readabilityHandler = nil
                standardError.fileHandleForReading.readabilityHandler = nil

                state.emit(standardOutput.fileHandleForReading.readDataToEndOfFile(), stream: .stdout)
                state.emit(standardError.fileHandleForReading.readDataToEndOfFile(), stream: .stderr)
                state.finish(exitCode: process.terminationStatus)
            }

            do {
                try process.run()
            } catch {
                standardOutput.fileHandleForReading.readabilityHandler = nil
                standardError.fileHandleForReading.readabilityHandler = nil
                state.fail(ProcessRunnerError.failedToLaunch(
                    executable: executable,
                    reason: error.localizedDescription
                ))
            }
        }
    }

    private func attachReader(
        to pipe: Pipe,
        stream: ProcessStream,
        state: ProcessExecutionState
    ) {
        pipe.fileHandleForReading.readabilityHandler = { handle in
            state.emit(handle.availableData, stream: stream)
        }
    }
}

private final class ProcessExecutionState: @unchecked Sendable {
    private let continuation: CheckedContinuation<ProcessResult, Error>
    private let startDate: Date
    private let onOutput: (ProcessStream, String) -> Void
    private let lock = NSLock()
    private var hasFinished = false

    init(
        continuation: CheckedContinuation<ProcessResult, Error>,
        startDate: Date,
        onOutput: @escaping (ProcessStream, String) -> Void
    ) {
        self.continuation = continuation
        self.startDate = startDate
        self.onOutput = onOutput
    }

    func emit(_ data: Data, stream: ProcessStream) {
        guard !data.isEmpty else { return }
        onOutput(stream, String(decoding: data, as: UTF8.self))
    }

    func finish(exitCode: Int32) {
        complete(.success(ProcessResult(
            exitCode: exitCode,
            duration: Date().timeIntervalSince(startDate)
        )))
    }

    func fail(_ error: Error) {
        complete(.failure(error))
    }

    private func complete(_ result: Result<ProcessResult, Error>) {
        lock.lock()
        guard !hasFinished else {
            lock.unlock()
            return
        }
        hasFinished = true
        lock.unlock()
        continuation.resume(with: result)
    }
}
