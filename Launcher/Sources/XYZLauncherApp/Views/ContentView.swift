import Observation
import SwiftUI
import XYZLauncherCore

struct ContentView: View {
    @Bindable private var manager: BuildManager

    init(manager: BuildManager) {
        self.manager = manager
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 18) {
                header
                buildConfigurationSection
                toolchainSection
                buildActionsSection
                developmentSection
                buildStatus
                BuildOutputView(log: manager.buildLog, isRunning: manager.isRunning) {
                    manager.clearOutput()
                }
            }
            .padding(24)
        }
        .toolbar {
            ToolbarItem {
                Button {
                    manager.refreshToolchain()
                } label: {
                    Label("Refresh Toolchain", systemImage: "arrow.clockwise")
                }
                .help("Re-check clang, cmake, and ninja")
            }
        }
    }

    private var header: some View {
        HStack(alignment: .firstTextBaseline) {
            VStack(alignment: .leading, spacing: 4) {
                Text("XYZ DEV")
                    .font(.system(size: 30, weight: .bold, design: .rounded))
                Text(manager.paths.rootURL.path)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
                    .truncationMode(.middle)
            }

            Spacer()
            StatusBadge(state: manager.state)
        }
    }

    private var buildConfigurationSection: some View {
        GroupBox("Build Configuration") {
            Picker("Configuration", selection: $manager.selectedConfiguration) {
                ForEach(BuildConfiguration.allCases) { configuration in
                    Text(configuration.rawValue).tag(configuration)
                }
            }
            .pickerStyle(.segmented)
            .labelsHidden()
        }
    }

    private var toolchainSection: some View {
        GroupBox("Toolchain") {
            VStack(spacing: 0) {
                ForEach(manager.toolchainStatuses) { status in
                    ToolchainRow(status: status)
                    if status.id != manager.toolchainStatuses.last?.id {
                        Divider()
                    }
                }
            }
        }
    }

    private var buildActionsSection: some View {
        GroupBox("Build") {
            HStack(spacing: 10) {
                AsyncActionButton("Configure", systemImage: "slider.horizontal.3") {
                    await manager.configure()
                }
                AsyncActionButton("Build", systemImage: "hammer") {
                    await manager.build()
                }
                AsyncActionButton("Build & Run", systemImage: "play.fill", prominent: true) {
                    await manager.buildAndRun()
                }
                AsyncActionButton("Clean", systemImage: "trash") {
                    await manager.clean()
                }
            }
            .disabled(manager.isRunning)
        }
    }

    private var developmentSection: some View {
        GroupBox("Development") {
            HStack(spacing: 10) {
                Button {
                    manager.openBuildFolder()
                } label: {
                    Label("Open Build Folder", systemImage: "folder")
                }
                Button {
                    manager.openAssetsFolder()
                } label: {
                    Label("Open Assets Folder", systemImage: "photo.on.rectangle")
                }
                Button {
                    manager.openLogsFolder()
                } label: {
                    Label("Open Logs", systemImage: "doc.text.magnifyingglass")
                }
            }
        }
    }

    private var buildStatus: some View {
        HStack(spacing: 8) {
            Image(systemName: manager.lastBuildSucceeded == true ? "checkmark.circle" : "clock")
                .foregroundStyle(manager.lastBuildSucceeded == true ? .green : .secondary)
            if let date = manager.lastBuildDate {
                Text("Last build")
                    .foregroundStyle(.secondary)
                Text(date, format: .dateTime.year().month().day().hour().minute().second())
                if let succeeded = manager.lastBuildSucceeded {
                    Text(succeeded ? "Succeeded" : "Failed")
                        .foregroundStyle(succeeded ? .green : .red)
                }
            } else {
                Text("No build recorded yet")
                    .foregroundStyle(.secondary)
            }
            Spacer()
        }
        .font(.caption)
    }
}

private struct AsyncActionButton: View {
    let title: String
    let systemImage: String
    let prominent: Bool
    let action: () async -> Void

    init(
        _ title: String,
        systemImage: String,
        prominent: Bool = false,
        action: @escaping () async -> Void
    ) {
        self.title = title
        self.systemImage = systemImage
        self.prominent = prominent
        self.action = action
    }

    @ViewBuilder
    var body: some View {
        if prominent {
            Button(action: run) {
                buttonLabel
            }
            .buttonStyle(.borderedProminent)
        } else {
            Button(action: run) {
                buttonLabel
            }
            .buttonStyle(.bordered)
        }
    }

    private func run() {
        Task { @MainActor in
            await action()
        }
    }

    private var buttonLabel: some View {
        Label(title, systemImage: systemImage)
    }
}

private struct StatusBadge: View {
    let state: BuildState

    private var title: String {
        switch state {
        case .idle:
            return "Ready"
        case let .running(operation):
            return "Running · \(operation.rawValue)"
        case let .succeeded(operation):
            return "Succeeded · \(operation.rawValue)"
        case let .failed(operation, exitCode):
            if let exitCode {
                return "Failed · \(operation.rawValue) · \(exitCode)"
            }
            return "Failed · \(operation.rawValue)"
        }
    }

    private var color: Color {
        switch state {
        case .idle:
            return .secondary
        case .running:
            return .blue
        case .succeeded:
            return .green
        case .failed:
            return .red
        }
    }

    var body: some View {
        Text(title)
            .font(.caption.weight(.semibold))
            .foregroundStyle(color)
            .padding(.horizontal, 10)
            .padding(.vertical, 6)
            .background(color.opacity(0.12), in: Capsule())
    }
}
