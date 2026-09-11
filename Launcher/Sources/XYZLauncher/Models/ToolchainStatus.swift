import Foundation

public struct ToolchainStatus: Identifiable, Sendable {
    public let command: String
    public let executableURL: URL?
    public let version: String?

    public init(command: String, executableURL: URL?, version: String?) {
        self.command = command
        self.executableURL = executableURL
        self.version = version
    }

    public var id: String { command }
    public var isAvailable: Bool { executableURL != nil }
}

public enum LogStream: String, Sendable {
    case stdout
    case stderr

    public var label: String {
        switch self {
        case .stdout:
            return "OUT"
        case .stderr:
            return "ERR"
        }
    }
}
