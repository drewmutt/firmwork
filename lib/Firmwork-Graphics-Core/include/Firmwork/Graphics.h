//
// Created by Andrew Simmons on 9/30/25.
//

#ifndef FIRMWORK_GRAPHICS_H
#define FIRMWORK_GRAPHICS_H
#include <Arduino.h> // for String

// New typedefs for clarity in graphics APIs
typedef struct PixelPoint { int x, y; PixelPoint() = default;
    PixelPoint(int x, int y);
    void offset();
} PixelPoint;

typedef struct PixelSize  { int w, h; PixelSize() = default;
    PixelSize(int w, int h);
} PixelSize;
typedef struct PixelRect  { PixelPoint p; PixelSize s; } PixelRect;
typedef uint32_t Color;
typedef struct ColorRGB{ uint8_t r, g, b; } ColorRGB;

// h in degrees [0,360), s in [0,1], v in [0,1]
typedef struct ColorHSV { float h, s, v; } ColorHSV;
typedef float FontSize;

class Graphics {
public:
       virtual ~Graphics() = default;

    // Basic pixels and lines
    virtual void drawPixel        (PixelPoint pt, Color color) = 0;
    virtual void drawFastVLine    (PixelPoint start, int h, Color color) = 0;
    virtual void drawFastHLine    (PixelPoint start, int w, Color color) = 0;

    // Rectangles
    virtual void fillRect         (PixelRect rect, Color color) = 0;
    virtual void drawRect         (PixelRect rect, Color color) = 0;
    virtual void drawRoundRect    (PixelRect rect, int r, Color color) = 0;
    virtual void fillRoundRect    (PixelRect rect, int r, Color color) = 0;

    // Circles & Ellipses
    virtual void drawCircle       (PixelPoint center, int r, Color color) = 0;
    virtual void fillCircle       (PixelPoint center, int r, Color color) = 0;
    virtual void drawEllipse      (PixelPoint center, PixelSize radii, Color color) = 0;
    virtual void fillEllipse      (PixelPoint center, PixelSize radii, Color color) = 0;

    // Lines & polygons
    virtual void drawLine         (PixelPoint p0, PixelPoint p1, Color color) = 0;
    virtual void drawTriangle     (PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) = 0;
    virtual void fillTriangle     (PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) = 0;

    // Bezier curves
    virtual void drawBezier       (PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) = 0;                 // Quadratic
    virtual void drawBezier       (PixelPoint p0, PixelPoint p1, PixelPoint p2, PixelPoint p3, Color color) = 0;  // Cubic

    // Arcs
    virtual void drawEllipseArc   (PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) = 0;
    virtual void fillEllipseArc   (PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) = 0;
    virtual void drawArc          (PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) = 0;
    virtual void fillArc          (PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) = 0;

    // Text & fills (prefer const char* to avoid Arduino dependency here)
    virtual void drawTextChars(PixelPoint pt, const char* text) = 0;
    virtual void drawTextChars(PixelPoint pt, FontSize fontSize, const char* text) = 0;
    virtual void drawTextString(PixelPoint pt, String string) = 0;
    virtual void drawTextString(PixelPoint pt, FontSize fontSize, String string) = 0;
    virtual void drawTextPrintf(PixelPoint pt, const char* fmt, ...) = 0;
    virtual void drawTextPrintf(PixelPoint pt, FontSize fontSize, const char* fmt, ...) = 0;

    virtual void floodFill        (PixelPoint seed, Color color) = 0;
    virtual void drawGradientLine (PixelPoint p0, PixelPoint p1, Color colorStart, Color colorEnd) = 0;

    virtual void clearScreen (Color color) = 0;
    virtual void clearScreen () = 0;

    virtual FontSize getDefaultFontSize() = 0;
};

#endif // FIRMWORK_GRAPHICS_H
