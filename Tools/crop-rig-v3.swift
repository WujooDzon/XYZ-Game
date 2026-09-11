import CoreGraphics
import Darwin
import Foundation
import ImageIO
import UniformTypeIdentifiers

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("crop-rig-v3: \(message)\n".utf8))
    exit(2)
}

guard CommandLine.arguments.count == 7 || CommandLine.arguments.count == 8 else {
    fail("usage: swift Tools/crop-rig-v3.swift input.png output.png x y width height [padding]")
}

let inputURL = URL(fileURLWithPath: CommandLine.arguments[1])
let outputURL = URL(fileURLWithPath: CommandLine.arguments[2])
guard let x = Int(CommandLine.arguments[3]),
      let y = Int(CommandLine.arguments[4]),
      let requestedWidth = Int(CommandLine.arguments[5]),
      let requestedHeight = Int(CommandLine.arguments[6]),
      requestedWidth > 0,
      requestedHeight > 0 else {
    fail("crop rectangle must contain positive integer values")
}
let padding = CommandLine.arguments.count == 8
    ? (Int(CommandLine.arguments[7]) ?? 24)
    : 24
guard padding >= 0 else { fail("padding must not be negative") }

guard let source = CGImageSourceCreateWithURL(inputURL as CFURL, nil),
      let image = CGImageSourceCreateImageAtIndex(source, 0, nil) else {
    fail("could not read \(inputURL.path)")
}

let sourceWidth = image.width
let sourceHeight = image.height
var sourcePixels = [UInt8](repeating: 0, count: sourceWidth * sourceHeight * 4)
let rendered = sourcePixels.withUnsafeMutableBytes { rawBuffer -> Bool in
    guard let baseAddress = rawBuffer.baseAddress,
          let context = CGContext(
              data: baseAddress,
              width: sourceWidth,
              height: sourceHeight,
              bitsPerComponent: 8,
              bytesPerRow: sourceWidth * 4,
              space: CGColorSpaceCreateDeviceRGB(),
              bitmapInfo: CGBitmapInfo(rawValue: CGBitmapInfo.byteOrder32Big.rawValue
                  | CGImageAlphaInfo.premultipliedLast.rawValue)) else {
        return false
    }
    context.setBlendMode(.copy)
    context.interpolationQuality = .none
    context.draw(image, in: CGRect(x: 0, y: 0, width: sourceWidth, height: sourceHeight))
    return true
}
guard rendered else { fail("could not decode input into RGBA") }

let cropX = max(0, min(x, sourceWidth - 1))
let cropY = max(0, min(y, sourceHeight - 1))
let cropWidth = min(requestedWidth, sourceWidth - cropX)
let cropHeight = min(requestedHeight, sourceHeight - cropY)
guard cropWidth > 0, cropHeight > 0 else { fail("crop rectangle is outside the image") }

let outputWidth = cropWidth + padding * 2
let outputHeight = cropHeight + padding * 2
var outputPixels = [UInt8](repeating: 0, count: outputWidth * outputHeight * 4)
for row in 0..<cropHeight {
    for column in 0..<cropWidth {
        let sourceOffset = ((cropY + row) * sourceWidth + cropX + column) * 4
        let outputOffset = ((row + padding) * outputWidth + column + padding) * 4
        outputPixels[outputOffset] = sourcePixels[sourceOffset]
        outputPixels[outputOffset + 1] = sourcePixels[sourceOffset + 1]
        outputPixels[outputOffset + 2] = sourcePixels[sourceOffset + 2]
        outputPixels[outputOffset + 3] = sourcePixels[sourceOffset + 3]
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
print("cropped \(inputURL.lastPathComponent) -> \(outputURL.path) (\(outputWidth)x\(outputHeight), padding=\(padding))")
