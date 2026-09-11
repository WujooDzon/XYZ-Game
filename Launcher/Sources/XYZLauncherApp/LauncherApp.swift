import SwiftUI
import XYZLauncherCore

@main
struct LauncherApp: App {
    @State private var manager: BuildManager

    init() {
        _manager = State(
            initialValue: BuildManager(paths: ProjectPaths.discover())
        )
    }

    var body: some Scene {
        WindowGroup("XYZ DEV") {
            ContentView(manager: manager)
                .frame(minWidth: 720, minHeight: 760)
        }
        .defaultSize(width: 800, height: 860)
    }
}
