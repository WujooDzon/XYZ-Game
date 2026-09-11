import Foundation

public enum BuildOperation: String, Equatable, Sendable {
    case configure = "Configure"
    case build = "Build"
    case buildAndRun = "Build & Run"
    case clean = "Clean"
}

public enum BuildState: Equatable, Sendable {
    case idle
    case running(BuildOperation)
    case succeeded(BuildOperation)
    case failed(BuildOperation, exitCode: Int32?)
}
