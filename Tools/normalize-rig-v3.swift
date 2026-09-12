import CoreGraphics
import Darwin
import Foundation
import ImageIO
import UniformTypeIdentifiers

struct RGBAImage {
    var pixels: [UInt8]
    let width: Int
    let height: Int
}

struct RGBColor {
    let r: Int
    let g: Int
    let b: Int
}

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("normalize-rig-v3: \(message)\n".utf8))
    exit(2)
}

guard CommandLine.arguments.count >= 3 else {
    fail("usage: swift Tools/normalize-rig-v3.swift input.png output.png [darken-factor]")
}

let inputURL = URL(fileURLWithPath: CommandLine.arguments[1])
let outputURL = URL(fileURLWithPath: CommandLine.arguments[2])
let darkenFactor = CommandLine.arguments.count >= 4
    ? (Float(CommandLine.arguments[3]) ?? 1.0)
    : 1.0

guard darkenFactor > 0.0, darkenFactor <= 1.0 else {
    fail("darken-factor must be in the range (0, 1]")
}

guard let source = CGImageSourceCreateWithURL(inputURL as CFURL, nil),
      let image = CGImageSourceCreateImageAtIndex(source, 0, nil) else {
    fail("could not read \(inputURL.path)")
}

let width = image.width
let height = image.height
var pixels = [UInt8](repeating: 0, count: width * height * 4)

let rendered = pixels.withUnsafeMutableBytes { rawBuffer -> Bool in
    guard let baseAddress = rawBuffer.baseAddress,
          let context = CGContext(
              data: baseAddress,
              width: width,
              height: height,
              bitsPerComponent: 8,
              bytesPerRow: width * 4,
              space: CGColorSpaceCreateDeviceRGB(),
              bitmapInfo: CGBitmapInfo.byteOrder32Big.rawValue
                  | CGImageAlphaInfo.premultipliedLast.rawValue) else {
        return false
    }

    context.setBlendMode(.copy)
    context.interpolationQuality = .none
    context.draw(image, in: CGRect(x: 0, y: 0, width: width, height: height))
    return true
}

guard rendered else {
    fail("could not decode \(inputURL.path) into an RGBA buffer")
}

// CoreGraphics writes premultiplied RGB into this buffer. The normalizer later
// hardens every retained alpha value to 255, so recover straight RGB first or
// translucent source colors become permanently darkened.
for pixelIndex in 0..<(width * height) {
    let offset = pixelIndex * 4
    let alpha = Int(pixels[offset + 3])
    guard alpha > 0, alpha < 255 else { continue }
    for channel in 0..<3 {
        let premultiplied = Int(pixels[offset + channel])
        pixels[offset + channel] = UInt8(min(255, (premultiplied * 255 + alpha / 2) / alpha))
    }
}

@inline(__always)
func pixelOffset(_ x: Int, _ y: Int) -> Int {
    (y * width + x) * 4
}

@inline(__always)
func colorAt(_ pixelIndex: Int) -> RGBColor {
    RGBColor(
        r: Int(pixels[pixelIndex]),
        g: Int(pixels[pixelIndex + 1]),
        b: Int(pixels[pixelIndex + 2]))
}

@inline(__always)
func isGrayscale(_ color: RGBColor) -> Bool {
    max(color.r, max(color.g, color.b)) - min(color.r, min(color.g, color.b)) <= 24
}

@inline(__always)
func distance(_ left: RGBColor, _ right: RGBColor) -> Int {
    max(abs(left.r - right.r), max(abs(left.g - right.g), abs(left.b - right.b)))
}

var borderHistogram: [UInt32: Int] = [:]
func addBorderColor(_ x: Int, _ y: Int) {
    let offset = pixelOffset(x, y)
    guard pixels[offset + 3] > 0 else { return }
    let color = colorAt(offset)
    guard isGrayscale(color) else { return }
    let luminanceBin = UInt32((color.r + color.g + color.b) / 3 / 4)
    let key = luminanceBin
    borderHistogram[key, default: 0] += 1
}

for x in 0..<width {
    addBorderColor(x, 0)
    if height > 1 { addBorderColor(x, height - 1) }
}
if height > 2 {
    for y in 1..<(height - 1) {
        addBorderColor(0, y)
        if width > 1 { addBorderColor(width - 1, y) }
    }
}

let backgroundColors: [RGBColor] = borderHistogram
    .sorted { $0.value > $1.value }
    .prefix(8)
    .map { key, _ in
        let value = Int(key) * 4 + 2
        return RGBColor(
            r: value,
            g: value,
            b: value)
    }

func isBackgroundPixel(_ x: Int, _ y: Int) -> Bool {
    let offset = pixelOffset(x, y)
    if pixels[offset + 3] == 0 { return true }
    let color = colorAt(offset)
    guard isGrayscale(color) else { return false }
    return backgroundColors.contains { distance(color, $0) <= 24 }
}

var visited = [Bool](repeating: false, count: width * height)
var stack: [Int] = []

func enqueueIfBackground(_ x: Int, _ y: Int) {
    guard x >= 0, x < width, y >= 0, y < height else { return }
    let index = y * width + x
    guard !visited[index], isBackgroundPixel(x, y) else { return }
    visited[index] = true
    stack.append(index)
}

for x in 0..<width {
    enqueueIfBackground(x, 0)
    if height > 1 { enqueueIfBackground(x, height - 1) }
}
if height > 2 {
    for y in 1..<(height - 1) {
        enqueueIfBackground(0, y)
        if width > 1 { enqueueIfBackground(width - 1, y) }
    }
}

while let index = stack.popLast() {
    let x = index % width
    let y = index / width
    enqueueIfBackground(x - 1, y)
    enqueueIfBackground(x + 1, y)
    enqueueIfBackground(x, y - 1)
    enqueueIfBackground(x, y + 1)
}

let alphaThreshold: UInt8 = 32
for y in 0..<height {
    for x in 0..<width {
        let pixelIndex = y * width + x
        let offset = pixelIndex * 4
        if visited[pixelIndex] {
            pixels[offset] = 0
            pixels[offset + 1] = 0
            pixels[offset + 2] = 0
            pixels[offset + 3] = 0
            continue
        }

        if pixels[offset + 3] < alphaThreshold {
            pixels[offset] = 0
            pixels[offset + 1] = 0
            pixels[offset + 2] = 0
            pixels[offset + 3] = 0
            continue
        }
        pixels[offset + 3] = 255
        if darkenFactor < 0.999 {
            pixels[offset] = UInt8(max(0, min(255, Int(Float(pixels[offset]) * darkenFactor))))
            pixels[offset + 1] = UInt8(max(0, min(255, Int(Float(pixels[offset + 1]) * darkenFactor))))
            pixels[offset + 2] = UInt8(max(0, min(255, Int(Float(pixels[offset + 2]) * darkenFactor))))
        }
    }
}

// The generated reference sheet can contain faint checkerboard seams or ghost
// fragments which are not connected to the actual cutout. Keep the largest
// 8-connected foreground component so the runtime receives one clean part.
var foregroundVisited = [Bool](repeating: false, count: width * height)
var largestForeground: [Int] = []
let foregroundDirections = [
    (-1, -1), (0, -1), (1, -1),
    (-1, 0),           (1, 0),
    (-1, 1),  (0, 1),  (1, 1)
]

for y in 0..<height {
    for x in 0..<width {
        let start = y * width + x
        guard !foregroundVisited[start], pixels[start * 4 + 3] >= alphaThreshold else {
            continue
        }

        foregroundVisited[start] = true
        var component = [start]
        var foregroundStack = [start]
        while let index = foregroundStack.popLast() {
            let currentX = index % width
            let currentY = index / width
            for (dx, dy) in foregroundDirections {
                let nextX = currentX + dx
                let nextY = currentY + dy
                guard nextX >= 0, nextX < width, nextY >= 0, nextY < height else {
                    continue
                }
                let nextIndex = nextY * width + nextX
                guard !foregroundVisited[nextIndex],
                      pixels[nextIndex * 4 + 3] >= alphaThreshold else {
                    continue
                }
                foregroundVisited[nextIndex] = true
                component.append(nextIndex)
                foregroundStack.append(nextIndex)
            }
        }

        if component.count > largestForeground.count {
            largestForeground = component
        }
    }
}

guard !largestForeground.isEmpty else {
    fail("no connected foreground component remained after background cleanup")
}

var keepForeground = [Bool](repeating: false, count: width * height)
for index in largestForeground {
    keepForeground[index] = true
}
for index in 0..<(width * height) {
    guard pixels[index * 4 + 3] >= alphaThreshold, !keepForeground[index] else {
        continue
    }
    pixels[index * 4] = 0
    pixels[index * 4 + 1] = 0
    pixels[index * 4 + 2] = 0
    pixels[index * 4 + 3] = 0
}

var minX = width
var minY = height
var maxX = -1
var maxY = -1

for y in 0..<height {
    for x in 0..<width {
        let offset = pixelOffset(x, y)
        guard pixels[offset + 3] >= alphaThreshold else { continue }
        minX = min(minX, x)
        minY = min(minY, y)
        maxX = max(maxX, x)
        maxY = max(maxY, y)
    }
}

guard maxX >= minX, maxY >= minY else {
    fail("no visible pixels remained after background cleanup")
}

let padding = 24
let cropMinX = max(0, minX - padding)
let cropMinY = max(0, minY - padding)
let cropMaxX = min(width - 1, maxX + padding)
let cropMaxY = min(height - 1, maxY + padding)
let outputWidth = cropMaxX - cropMinX + 1
let outputHeight = cropMaxY - cropMinY + 1
var outputPixels = [UInt8](repeating: 0, count: outputWidth * outputHeight * 4)

for y in 0..<outputHeight {
    for x in 0..<outputWidth {
        let sourceOffset = pixelOffset(cropMinX + x, cropMinY + y)
        let outputOffset = (y * outputWidth + x) * 4
        outputPixels[outputOffset] = pixels[sourceOffset]
        outputPixels[outputOffset + 1] = pixels[sourceOffset + 1]
        outputPixels[outputOffset + 2] = pixels[sourceOffset + 2]
        outputPixels[outputOffset + 3] = pixels[sourceOffset + 3]
    }
}

try? FileManager.default.createDirectory(
    at: outputURL.deletingLastPathComponent(),
    withIntermediateDirectories: true)

let encoded = outputPixels.withUnsafeBytes { rawBuffer -> Bool in
    guard let provider = CGDataProvider(data: Data(bytes: rawBuffer.baseAddress!, count: rawBuffer.count) as CFData),
          let outputImage = CGImage(
              width: outputWidth,
              height: outputHeight,
              bitsPerComponent: 8,
              bitsPerPixel: 32,
              bytesPerRow: outputWidth * 4,
              space: CGColorSpaceCreateDeviceRGB(),
              bitmapInfo: CGBitmapInfo(rawValue: CGBitmapInfo.byteOrder32Big.rawValue
                  | CGImageAlphaInfo.premultipliedLast.rawValue),
              provider: provider,
              decode: nil,
              shouldInterpolate: false,
              intent: .defaultIntent),
          let destination = CGImageDestinationCreateWithURL(
              outputURL as CFURL,
              UTType.png.identifier as CFString,
              1,
              nil) else {
        return false
    }

    CGImageDestinationAddImage(destination, outputImage, nil)
    return CGImageDestinationFinalize(destination)
}

guard encoded else {
    fail("could not write \(outputURL.path)")
}

print("normalized \(inputURL.lastPathComponent) -> \(outputURL.path) (\(outputWidth)x\(outputHeight), darken=\(darkenFactor))")
