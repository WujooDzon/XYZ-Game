import Foundation

public enum BuildConfiguration: String, CaseIterable, Codable, Identifiable, Sendable {
    case debug = "Debug"
    case release = "Release"

    public var id: String { rawValue }

    public var cmakeValue: String { rawValue }
}
