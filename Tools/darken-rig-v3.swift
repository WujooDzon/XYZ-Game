import CoreGraphics
import Darwin
import Foundation
import ImageIO
import UniformTypeIdentifiers

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("darken-rig-v3: \(message)\n".utf8))
    exit(2)
}

guard CommandLine.arguments.count == 4,
      let factor = Float(CommandLine.arguments[3]),
      factor > 0.0,
      factor <= 1.0 else {
    fail("usage: swift Tools/darken-rig-v3.swift input.png output.png factor")
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

for offset in stride(from: 0, to: pixels.count, by: 4) {
    guard pixels[offset + 3] > 0 else { continue }
    pixels[offset] = UInt8(max(0, min(255, Int(Float(pixels[offset]) * factor))))
    pixels[offset + 1] = UInt8(max(0, min(255, Int(Float(pixels[offset + 1]) * factor))))
    pixels[offset + 2] = UInt8(max(0, min(255, Int(Float(pixels[offset + 2]) * factor))))
}

try? FileManager.default.createDirectory(
    at: outputURL.deletingLastPathComponent(),
    withIntermediateDirectories: true)

let encoded = pixels.withUnsafeBytes { rawBuffer -> Bool in
    guard let provider = CGDataProvider(
              data: Data(bytes: rawBuffer.baseAddress!, count: rawBuffer.count) as CFData),
          let outputImage = CGImage(
              width: width,
              height: height,
              bitsPerComponent: 8,
              bitsPerPixel: 32,
              bytesPerRow: width * 4,
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
print("darkened \(inputURL.lastPathComponent) -> \(outputURL.path) (factor=\(factor))")
