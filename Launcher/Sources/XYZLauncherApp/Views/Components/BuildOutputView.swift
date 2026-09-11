import SwiftUI
import XYZLauncherCore

struct BuildOutputView: View {
    let log: BuildLog
    let isRunning: Bool
    let clearAction: () -> Void

    var body: some View {
        GroupBox {
            VStack(alignment: .leading, spacing: 10) {
                HStack {
                    Label("Build Output", systemImage: isRunning ? "waveform" : "terminal")
                        .font(.headline)
                    Spacer()
                    Button("Clear", action: clearAction)
                        .buttonStyle(.borderless)
                        .disabled(log.entries.isEmpty)
                }

                ScrollView {
                    Text(log.renderedText.isEmpty ? "No output yet." : log.renderedText)
                        .font(.system(size: 12, design: .monospaced))
                        .foregroundStyle(log.renderedText.isEmpty ? .secondary : .primary)
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(12)
                }
                .frame(minHeight: 260, maxHeight: 420)
                .background(Color(nsColor: .textBackgroundColor), in: RoundedRectangle(cornerRadius: 8))
                .overlay {
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color(nsColor: .separatorColor), lineWidth: 1)
                }
            }
        }
    }
}
