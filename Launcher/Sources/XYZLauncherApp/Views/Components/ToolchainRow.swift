import SwiftUI
import XYZLauncherCore

struct ToolchainRow: View {
    let status: ToolchainStatus

    var body: some View {
        HStack(spacing: 10) {
            Image(systemName: status.isAvailable ? "checkmark.circle.fill" : "xmark.circle.fill")
                .foregroundStyle(status.isAvailable ? .green : .red)
            Text(status.command)
                .font(.system(.body, design: .monospaced))
            Spacer()
            if let version = status.version {
                Text(version)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
                    .truncationMode(.tail)
            } else if !status.isAvailable {
                Text("Not found")
                    .font(.caption)
                    .foregroundStyle(.red)
            }
        }
        .padding(.vertical, 7)
        .accessibilityElement(children: .combine)
        .accessibilityLabel("\(status.command), \(status.isAvailable ? "available" : "not found")")
    }
}
