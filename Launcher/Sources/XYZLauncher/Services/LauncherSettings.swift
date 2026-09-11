import Foundation
import Observation

@Observable
public final class LauncherSettings {
    private enum Key {
        static let selectedConfiguration = "selectedConfiguration"
        static let lastBuildDate = "lastBuildDate"
        static let lastBuildSucceeded = "lastBuildSucceeded"
    }

    @ObservationIgnored
    private let defaults: UserDefaults

    public var selectedConfiguration: BuildConfiguration {
        didSet { defaults.set(selectedConfiguration.rawValue, forKey: Key.selectedConfiguration) }
    }

    public var lastBuildDate: Date? {
        didSet { defaults.set(lastBuildDate, forKey: Key.lastBuildDate) }
    }

    public var lastBuildSucceeded: Bool? {
        didSet {
            if let lastBuildSucceeded {
                defaults.set(lastBuildSucceeded, forKey: Key.lastBuildSucceeded)
            } else {
                defaults.removeObject(forKey: Key.lastBuildSucceeded)
            }
        }
    }

    public init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
        self.selectedConfiguration = BuildConfiguration(
            rawValue: defaults.string(forKey: Key.selectedConfiguration) ?? ""
        ) ?? .debug
        self.lastBuildDate = defaults.object(forKey: Key.lastBuildDate) as? Date
        self.lastBuildSucceeded = defaults.object(forKey: Key.lastBuildSucceeded) as? Bool
    }
}
