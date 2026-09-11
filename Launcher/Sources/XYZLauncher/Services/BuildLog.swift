import Foundation
import Observation

@Observable
public final class BuildLog {
    public struct Entry: Identifiable, Sendable {
        public let id = UUID()
        public let date: Date
        public let stream: LogStream?
        public let text: String
    }

    public private(set) var entries: [Entry] = []

    public var renderedText: String {
        entries.map { entry in
            let timestamp = Self.timestampFormatter.string(from: entry.date)
            if let stream = entry.stream {
                return "[\(timestamp)] [\(stream.label)] \(entry.text)"
            }
            return "[\(timestamp)] [STATUS] \(entry.text)"
        }
        .joined(separator: "\n")
    }

    public init() {}

    public func append(stream: LogStream, text: String) {
        guard !text.isEmpty else { return }
        entries.append(Entry(date: Date(), stream: stream, text: text.trimmingCharacters(in: .newlines)))
    }

    public func appendStatus(_ text: String) {
        guard !text.isEmpty else { return }
        entries.append(Entry(date: Date(), stream: nil, text: text))
    }

    public func clear() {
        entries.removeAll(keepingCapacity: true)
    }

    public func write(to url: URL) throws {
        try FileManager.default.createDirectory(
            at: url.deletingLastPathComponent(),
            withIntermediateDirectories: true
        )
        try renderedText.write(to: url, atomically: true, encoding: .utf8)
    }

    private static let timestampFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss"
        return formatter
    }()
}
