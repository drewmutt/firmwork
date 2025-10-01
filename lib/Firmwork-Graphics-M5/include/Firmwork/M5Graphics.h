//
// Created by Andrew Simmons on 9/30/25.
//

#ifndef FIRMWORK_GRAPHICSM5_H
#define FIRMWORK_GRAPHICSM5_H
#include <Arduino.h> // for String

#include <M5GFX.h>
#include <Firmwork/Graphics.h>

class M5Graphics final : public Graphics {
public:
    explicit M5Graphics(M5GFX& gfx) : gfx_(gfx) {}

    // Basic pixels and lines
    void drawPixel        (PixelPoint pt, Color color) override;
    void drawFastVLine    (PixelPoint start, int h, Color color) override;
    void drawFastHLine    (PixelPoint start, int w, Color color) override;

    // Rectangles
    void fillRect         (PixelRect rect, Color color) override;
    void drawRect         (PixelRect rect, Color color) override;
    void drawRoundRect    (PixelRect rect, int r, Color color) override;
    void fillRoundRect    (PixelRect rect, int r, Color color) override;

    // Circles & Ellipses
    void drawCircle       (PixelPoint center, int r, Color color) override;
    void fillCircle       (PixelPoint center, int r, Color color) override;
    void drawEllipse      (PixelPoint center, PixelSize radii, Color color) override;
    void fillEllipse      (PixelPoint center, PixelSize radii, Color color) override;

    // Lines & polygons
    void drawLine         (PixelPoint p0, PixelPoint p1, Color color) override;
    void drawTriangle     (PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) override;
    void fillTriangle     (PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) override;

    // Bezier curves
    void drawBezier       (PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) override;                 // Quadratic
    void drawBezier       (PixelPoint p0, PixelPoint p1, PixelPoint p2, PixelPoint p3, Color color) override;  // Cubic

    // Arcs
    void drawEllipseArc   (PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) override;
    void fillEllipseArc   (PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) override;
    void drawArc          (PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) override;
    void fillArc          (PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) override;

    // Text & fills
    void drawString       (PixelPoint pt, const char* text) override;
    void floodFill        (PixelPoint seed, Color color) override;
    void drawGradientLine (PixelPoint p0, PixelPoint p1, Color colorStart, Color colorEnd) override;

private:
    M5GFX& gfx_;
};
#endif // FIRMWORK_GRAPHICS_H
