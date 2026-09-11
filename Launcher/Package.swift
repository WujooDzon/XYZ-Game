// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "XYZLauncher",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .executable(name: "XYZLauncher", targets: ["XYZLauncher"])
    ],
    targets: [
        .target(
            name: "XYZLauncherCore",
            path: "Sources/XYZLauncher"
        ),
        .executableTarget(
            name: "XYZLauncher",
            dependencies: ["XYZLauncherCore"],
            path: "Sources/XYZLauncherApp"
        ),
        .testTarget(
            name: "XYZLauncherTests",
            dependencies: ["XYZLauncherCore"],
            path: "Tests/XYZLauncherTests"
        )
    ],
    swiftLanguageModes: [.v5]
)
