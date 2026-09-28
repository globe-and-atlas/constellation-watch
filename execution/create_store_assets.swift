// Native vector rendering of the Constellation store icon, matching store/icon.svg.
import AppKit

let root = URL(fileURLWithPath: CommandLine.arguments[1])
for size in [80, 144] {
    let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: size, pixelsHigh: size,
        bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
        colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
    let context = NSGraphicsContext(bitmapImageRep: bitmap)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = context
    let c = context.cgContext
    c.scaleBy(x: CGFloat(size)/144.0, y: CGFloat(size)/144.0)
    c.translateBy(x: 0, y: 144)
    c.scaleBy(x: 1, y: -1)

    func color(_ r: CGFloat, _ g: CGFloat, _ b: CGFloat, _ a: CGFloat = 1.0) -> CGColor {
        return CGColor(red: r/255.0, green: g/255.0, blue: b/255.0, alpha: a)
    }

    // 1. Dark navy background with rounded corners
    let bgRect = CGRect(x: 0, y: 0, width: 144, height: 144)
    let bgPath = CGPath(roundedRect: bgRect, cornerWidth: 28, cornerHeight: 28, transform: nil)
    c.addPath(bgPath)
    c.setFillColor(color(6, 27, 43))
    c.fillPath()

    // 2. Outer Celestial Reference Rings
    c.setStrokeColor(color(19, 44, 63))
    c.setLineWidth(1.5)
    c.setLineDash(phase: 0, lengths: [2, 4])
    c.strokeEllipse(in: CGRect(x: 72 - 62, y: 72 - 62, width: 124, height: 124))

    c.setStrokeColor(color(22, 56, 79))
    c.setLineDash(phase: 0, lengths: [4, 4])
    c.strokeEllipse(in: CGRect(x: 72 - 50, y: 72 - 50, width: 100, height: 100))
    c.setLineDash(phase: 0, lengths: [])

    // 3. GLONASS Orbit Ring (70 deg rotation)
    c.saveGState()
    c.translateBy(x: 72, y: 72)
    c.rotate(by: 70 * .pi / 180.0)
    c.setStrokeColor(color(85, 255, 170, 0.7))
    c.setLineWidth(2.0)
    c.strokeEllipse(in: CGRect(x: -54, y: -17, width: 108, height: 34))
    c.restoreGState()

    // 4. GPS Orbit Ring (-28 deg rotation)
    c.saveGState()
    c.translateBy(x: 72, y: 72)
    c.rotate(by: -28 * .pi / 180.0)
    c.setStrokeColor(color(255, 170, 0, 0.9))
    c.setLineWidth(2.5)
    c.strokeEllipse(in: CGRect(x: -55, y: -19, width: 110, height: 38))
    c.restoreGState()

    // 5. Galileo Orbit Ring (+32 deg rotation)
    c.saveGState()
    c.translateBy(x: 72, y: 72)
    c.rotate(by: 32 * .pi / 180.0)
    c.setStrokeColor(color(0, 255, 255, 0.9))
    c.setLineWidth(2.5)
    c.strokeEllipse(in: CGRect(x: -55, y: -19, width: 110, height: 38))
    c.restoreGState()

    // 6. Central Earth Sphere
    let earthRect = CGRect(x: 72 - 26, y: 72 - 26, width: 52, height: 52)
    c.setFillColor(color(11, 57, 53))
    c.fillEllipse(in: earthRect)
    c.setStrokeColor(color(77, 119, 120))
    c.setLineWidth(2.5)
    c.strokeEllipse(in: earthRect)

    // Earth Graticule Lines
    c.setStrokeColor(color(26, 93, 87))
    c.setLineWidth(1.5)
    // Equator
    c.move(to: CGPoint(x: 46, y: 72)); c.addLine(to: CGPoint(x: 98, y: 72)); c.strokePath()
    // Meridian
    c.strokeEllipse(in: CGRect(x: 72 - 14, y: 72 - 26, width: 28, height: 52))

    // Stylized landmasses
    c.setFillColor(color(27, 110, 95))
    let land1 = CGMutablePath()
    land1.move(to: CGPoint(x: 64, y: 56))
    land1.addQuadCurve(to: CGPoint(x: 76, y: 56), control: CGPoint(x: 70, y: 52))
    land1.addQuadCurve(to: CGPoint(x: 80, y: 66), control: CGPoint(x: 82, y: 60))
    land1.addQuadCurve(to: CGPoint(x: 64, y: 56), control: CGPoint(x: 72, y: 67))
    c.addPath(land1)
    c.fillPath()

    let land2 = CGMutablePath()
    land2.move(to: CGPoint(x: 68, y: 76))
    land2.addQuadCurve(to: CGPoint(x: 76, y: 84), control: CGPoint(x: 78, y: 74))
    land2.addQuadCurve(to: CGPoint(x: 66, y: 82), control: CGPoint(x: 70, y: 88))
    land2.closeSubpath()
    c.addPath(land2)
    c.fillPath()

    // 7. Active Satellite Nodes with Halos
    func drawSat(x: CGFloat, y: CGFloat, r: CGFloat, cr: CGFloat, cg: CGFloat, cb: CGFloat) {
        // Halo
        c.setFillColor(color(cr, cg, cb, 0.25))
        c.fillEllipse(in: CGRect(x: x - r - 2.5, y: y - r - 2.5, width: (r + 2.5)*2, height: (r + 2.5)*2))
        // Solid body
        c.setFillColor(color(cr, cg, cb, 1.0))
        c.fillEllipse(in: CGRect(x: x - r, y: y - r, width: r*2, height: r*2))
        // Specular core
        c.setFillColor(color(255, 255, 255, 0.9))
        c.fillEllipse(in: CGRect(x: x - r*0.35, y: y - r*0.35, width: r*0.7, height: r*0.7))
    }

    // GPS Sat (Amber)
    drawSat(x: 116, y: 48, r: 4.2, cr: 255, cg: 170, cb: 0)
    // Galileo Sat (Cyan)
    drawSat(x: 30, y: 46, r: 4.2, cr: 0, cg: 255, cb: 255)
    // GLONASS Sat (Mint)
    drawSat(x: 76, y: 18, r: 3.5, cr: 85, cg: 255, cb: 170)
    // BeiDou Sat (Rose / Magenta)
    drawSat(x: 104, y: 98, r: 3.5, cr: 255, cg: 85, cb: 170)

    // Observer Zenith Pulse (Observer location on Earth surface)
    c.setFillColor(color(255, 255, 255, 1.0))
    c.fillEllipse(in: CGRect(x: 72 - 3.5, y: 62 - 3.5, width: 7, height: 7))
    c.setStrokeColor(color(255, 255, 255, 0.6))
    c.setLineWidth(1.0)
    c.strokeEllipse(in: CGRect(x: 72 - 7, y: 62 - 7, width: 14, height: 14))

    NSGraphicsContext.restoreGraphicsState()
    let name = size == 80 ? "icon-small.png" : "icon-large.png"
    try bitmap.representation(using: .png, properties: [:])!.write(to: root.appendingPathComponent(name))
    print("Rendered \(name) at \(size)x\(size)")
}
