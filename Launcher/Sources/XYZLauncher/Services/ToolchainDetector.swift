import Foundation

public protocol ToolchainDetecting {
    func detect() -> [ToolchainStatus]
}

public final class ToolchainDetector: ToolchainDetecting {
    private static let commands = ["clang", "cmake", "ninja"]
    private let searchPaths: [URL]

    public init(searchPaths: [URL]? = nil) {
        if let searchPaths {
            self.searchPaths = Self.unique(searchPaths)
        } else {
            self.searchPaths = Self.unique(Self.defaultSearchPaths())
        }
    }

    public func detect() -> [ToolchainStatus] {
        Self.commands.map { command in
            let executableURL = findExecutable(named: command)
            return ToolchainStatus(
                command: command,
                executableURL: executableURL,
                version: executableURL.flatMap(version(of:))
            )
        }
    }

    private func findExecutable(named command: String) -> URL? {
        for directory in searchPaths {
            let candidate = directory.appendingPathComponent(command)
            if FileManager.default.isExecutableFile(atPath: candidate.path) {
                return candidate
            }
        }
        return nil
    }

    private func version(of executableURL: URL) -> String? {
        let process = Process()
        let output = Pipe()
        process.executableURL = executableURL
        process.arguments = ["--version"]
        process.standardOutput = output
        process.standardError = output

        do {
            try process.run()
            process.waitUntilExit()
        } catch {
            return nil
        }

        let text = String(decoding: output.fileHandleForReading.readDataToEndOfFile(), as: UTF8.self)
        return text
            .split(whereSeparator: \.isNewline)
            .first
            .map(String.init)
    }

    private static func defaultSearchPaths() -> [URL] {
        let environmentPaths = (ProcessInfo.processInfo.environment["PATH"] ?? "")
            .split(separator: ":")
            .map { URL(fileURLWithPath: String($0), isDirectory: true) }
        let commonPaths = [
            URL(fileURLWithPath: "/opt/homebrew/bin", isDirectory: true),
            URL(fileURLWithPath: "/usr/local/bin", isDirectory: true),
            URL(fileURLWithPath: "/usr/bin", isDirectory: true),
            URL(fileURLWithPath: "/bin", isDirectory: true)
        ]
        return environmentPaths + commonPaths
    }

    private static func unique(_ paths: [URL]) -> [URL] {
        var seen = Set<String>()
        return paths.filter { seen.insert($0.standardizedFileURL.path).inserted }
    }
}
