import CoreGraphics
import Darwin
import Foundation
import ImageIO
import UniformTypeIdentifiers

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("trim-rig-v3: \(message)\n".utf8))
    exit(2)
}

guard CommandLine.arguments.count == 4,
      let keepHeight = Int(CommandLine.arguments[3]),
      keepHeight > 0 else {
    fail("usage: swift Tools/trim-rig-v3.swift input.png output.png keep-height")
}

let inputURL = URL(fileURLWithPath: CommandLine.arguments[1])
let outputURL = URL(fileURLWithPath: CommandLine.arguments[2])
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
              bitmapInfo: CGBitmapInfo(rawValue: CGBitmapInfo.byteOrder32Big.rawValue
                  | CGImageAlphaInfo.premultipliedLast.rawValue)) else {
        return false
    }
    context.setBlendMode(.copy)
    context.interpolationQuality = .none
    context.draw(image, in: CGRect(x: 0, y: 0, width: width, height: height))
    return true
}
guard rendered else { fail("could not decode input into RGBA") }

let visibleHeight = min(keepHeight, height)
for y in visibleHeight..<height {
    for x in 0..<width {
        let offset = (y * width + x) * 4
        pixels[offset] = 0
        pixels[offset + 1] = 0
        pixels[offset + 2] = 0
        pixels[offset + 3] = 0
    }
}

let alphaThreshold: UInt8 = 32
var minX = width
var minY = height
var maxX = -1
var maxY = -1
for y in 0..<visibleHeight {
    for x in 0..<width {
        let offset = (y * width + x) * 4
        guard pixels[offset + 3] >= alphaThreshold else { continue }
        minX = min(minX, x)
        minY = min(minY, y)
        maxX = max(maxX, x)
        maxY = max(maxY, y)
    }
}
guard maxX >= minX, maxY >= minY else { fail("no visible pixels remain") }

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
        let sourceOffset = ((cropMinY + y) * width + cropMinX + x) * 4
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
    guard let provider = CGDataProvider(
              data: Data(bytes: rawBuffer.baseAddress!, count: rawBuffer.count) as CFData),
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
guard encoded else { fail("could not write \(outputURL.path)") }
print("trimmed \(inputURL.lastPathComponent) -> \(outputURL.path) (\(outputWidth)x\(outputHeight), keep-height=\(visibleHeight))")
