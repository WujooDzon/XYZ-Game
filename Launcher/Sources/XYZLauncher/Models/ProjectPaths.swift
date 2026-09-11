import Foundation

public struct ProjectPaths: Sendable {
    public let rootURL: URL

    public init(rootURL: URL) {
        self.rootURL = rootURL.standardizedFileURL
    }

    public static func findProjectRoot(startingAt url: URL) -> URL? {
        var candidate = url.standardizedFileURL
        if !candidate.hasDirectoryPath {
            candidate.deleteLastPathComponent()
        }

        while true {
            if ProjectPaths(rootURL: candidate).isValidProjectRoot {
                return candidate
            }

            let parent = candidate.deletingLastPathComponent()
            guard parent.path != candidate.path else { return nil }
            candidate = parent
        }
    }

    public static func discover() -> ProjectPaths {
        var candidates: [URL] = []
        if let configuredRoot = ProcessInfo.processInfo.environment["XYZ_PROJECT_ROOT"] {
            candidates.append(URL(fileURLWithPath: configuredRoot, isDirectory: true))
        }
        candidates.append(URL(fileURLWithPath: FileManager.default.currentDirectoryPath, isDirectory: true))
        if let executableDirectory = Bundle.main.executableURL?.deletingLastPathComponent() {
            candidates.append(executableDirectory)
        }

        for candidate in candidates {
            if let root = findProjectRoot(startingAt: candidate) {
                return ProjectPaths(rootURL: root)
            }
        }

        return ProjectPaths(rootURL: URL(fileURLWithPath: FileManager.default.currentDirectoryPath))
    }

    public var isValidProjectRoot: Bool {
        FileManager.default.fileExists(atPath: rootURL.appendingPathComponent("CMakeLists.txt").path)
    }

    public var buildRoot: URL {
        rootURL.appendingPathComponent("Build")
    }

    public var assetsDirectory: URL {
        rootURL.appendingPathComponent("Assets")
    }

    public var logsDirectory: URL {
        buildRoot.appendingPathComponent("Logs")
    }

    public func buildDirectory(for configuration: BuildConfiguration) -> URL {
        buildRoot.appendingPathComponent(configuration.rawValue)
    }

    public func gameExecutable(for configuration: BuildConfiguration) -> URL {
        buildDirectory(for: configuration)
            .appendingPathComponent("Game", isDirectory: true)
            .appendingPathComponent("XYZGame", isDirectory: false)
    }
}
