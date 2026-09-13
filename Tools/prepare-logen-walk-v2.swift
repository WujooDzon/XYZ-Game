import CoreGraphics
import Darwin
import Foundation
import ImageIO
import UniformTypeIdentifiers

private let outputWidth = 384
private let outputHeight = 512
private let outputBaseline = 500

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("prepare-logen-walk-v2: \(message)\n".utf8))
    exit(2)
}

guard CommandLine.arguments.count == 4 else {
    fail("usage: prepare-logen-walk-v2.swift first-half.png second-half.png output-directory")
}

func loadImage(_ path: String) -> CGImage {
    let url = URL(fileURLWithPath: path)
    guard let source = CGImageSourceCreateWithURL(url as CFURL, nil),
          let image = CGImageSourceCreateImageAtIndex(source, 0, nil) else {
        fail("could not read \(path)")
    }
    return image
}

func renderCell(_ image: CGImage, column: Int, rows: Int) -> [UInt8] {
    let columns = 4
    guard image.width % columns == 0, image.height % rows == 0 else {
        fail("source dimensions do not divide into the expected grid")
    }
    let cellWidth = image.width / columns
    let cellHeight = image.height / rows
    let sourceY = rows == 2 ? cellHeight : 0
    guard let cell = image.cropping(to: CGRect(
        x: column * cellWidth,
        y: sourceY,
        width: cellWidth,
        height: cellHeight)) else {
        fail("could not crop source cell \(column)")
    }

    var pixels = [UInt8](repeating: 0, count: outputWidth * outputHeight * 4)
    let rendered = pixels.withUnsafeMutableBytes { buffer -> Bool in
        guard let address = buffer.baseAddress,
              let context = CGContext(
                  data: address,
                  width: outputWidth,
                  height: outputHeight,
                  bitsPerComponent: 8,
                  bytesPerRow: outputWidth * 4,
                  space: CGColorSpaceCreateDeviceRGB(),
                  bitmapInfo: CGBitmapInfo.byteOrder32Big.rawValue
                      | CGImageAlphaInfo.premultipliedLast.rawValue) else {
            return false
        }
        context.setBlendMode(.copy)
        context.interpolationQuality = .none
        context.draw(cell, in: CGRect(x: 0, y: 0, width: outputWidth, height: outputHeight))
        return true
    }
    guard rendered else { fail("could not render source cell \(column)") }
    return pixels
}

func removeConnectedBackground(_ pixels: inout [UInt8]) {
    func isBackground(_ index: Int) -> Bool {
        let offset = index * 4
        if pixels[offset + 3] == 0 { return true }
        let red = Int(pixels[offset])
        let green = Int(pixels[offset + 1])
        let blue = Int(pixels[offset + 2])
        return max(red, green, blue) - min(red, green, blue) <= 10
            && min(red, green, blue) >= 178
    }

    var removed = [Bool](repeating: false, count: outputWidth * outputHeight)
    var queue: [Int] = []
    func enqueue(_ x: Int, _ y: Int) {
        guard x >= 0 && x < outputWidth && y >= 0 && y < outputHeight else { return }
        let index = y * outputWidth + x
        guard !removed[index] && isBackground(index) else { return }
        removed[index] = true
        queue.append(index)
    }

    for x in 0..<outputWidth {
        enqueue(x, 0)
        enqueue(x, outputHeight - 1)
    }
    for y in 0..<outputHeight {
        enqueue(0, y)
        enqueue(outputWidth - 1, y)
    }

    var cursor = 0
    while cursor < queue.count {
        let index = queue[cursor]
        cursor += 1
        let x = index % outputWidth
        let y = index / outputWidth
        enqueue(x - 1, y)
        enqueue(x + 1, y)
        enqueue(x, y - 1)
        enqueue(x, y + 1)
    }

    for index in removed.indices where removed[index] {
        let offset = index * 4
        pixels[offset] = 0
        pixels[offset + 1] = 0
        pixels[offset + 2] = 0
        pixels[offset + 3] = 0
    }
}

func alignToBaseline(_ pixels: [UInt8]) -> [UInt8] {
    var visibleBottom = -1
    for y in 0..<outputHeight {
        for x in 0..<outputWidth where pixels[(y * outputWidth + x) * 4 + 3] >= 128 {
            visibleBottom = y
        }
    }
    guard visibleBottom >= 0 else { fail("frame has no visible character pixels") }

    let shift = outputBaseline - visibleBottom
    var aligned = [UInt8](repeating: 0, count: pixels.count)
    for y in 0..<outputHeight {
        let targetY = y + shift
        guard targetY >= 0 && targetY < outputHeight else { continue }
        let sourceOffset = y * outputWidth * 4
        let targetOffset = targetY * outputWidth * 4
        aligned.replaceSubrange(
            targetOffset..<(targetOffset + outputWidth * 4),
            with: pixels[sourceOffset..<(sourceOffset + outputWidth * 4)])
    }
    return aligned
}

func writePNG(_ pixels: [UInt8], to url: URL) {
    guard let provider = CGDataProvider(data: Data(pixels) as CFData),
          let image = CGImage(
              width: outputWidth,
              height: outputHeight,
              bitsPerComponent: 8,
              bitsPerPixel: 32,
              bytesPerRow: outputWidth * 4,
              space: CGColorSpaceCreateDeviceRGB(),
              bitmapInfo: CGBitmapInfo.byteOrder32Big.union(
                  CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue)),
              provider: provider,
              decode: nil,
              shouldInterpolate: false,
              intent: .defaultIntent),
          let destination = CGImageDestinationCreateWithURL(
              url as CFURL,
              UTType.png.identifier as CFString,
              1,
              nil) else {
        fail("could not create PNG destination for \(url.path)")
    }
    CGImageDestinationAddImage(destination, image, nil)
    guard CGImageDestinationFinalize(destination) else {
        fail("could not write \(url.path)")
    }
}

let firstHalf = loadImage(CommandLine.arguments[1])
let secondHalf = loadImage(CommandLine.arguments[2])
let outputDirectory = URL(fileURLWithPath: CommandLine.arguments[3], isDirectory: true)
try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)

for frameIndex in 0..<8 {
    var pixels = frameIndex < 4
        ? renderCell(firstHalf, column: frameIndex, rows: 2)
        : renderCell(secondHalf, column: frameIndex - 4, rows: 1)
    removeConnectedBackground(&pixels)
    pixels = alignToBaseline(pixels)
    let name = String(format: "Logen_walk_right_%02d.png", frameIndex + 1)
    writePNG(pixels, to: outputDirectory.appendingPathComponent(name))
    print("wrote \(name) \(outputWidth)x\(outputHeight)")
}
