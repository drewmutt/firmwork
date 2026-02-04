//
// Created by Andrew Simmons on 9/30/25.
//

#ifndef FIRMWORK_GRAPHICS_H
#define FIRMWORK_GRAPHICS_H
#include <Arduino.h> // for String
#include "GraphicsTypes.h"
#include "Bounds.h"
#include "Colors.h"

class Graphics {
public:
       virtual ~Graphics() = default;

    virtual void start();
    // Basic pixels and lines
    virtual void drawPixel        (PixelPoint pt, Color color) = 0;
    virtual void drawFastVLine    (PixelPoint start, int h, Color color) = 0;
    virtual void drawFastHLine    (PixelPoint start, int w, Color color) = 0;

    // Rectangles
    virtual void fillRect         (PixelPoint topLeft, PixelSize size, Color color) = 0;
    virtual void drawRect         (PixelPoint topLeft, PixelSize size, Color color) = 0;
    virtual void drawRoundRect    (PixelPoint topLeft, PixelSize size, int r, Color color) = 0;
    virtual void fillRoundRect    (PixelPoint topLeft, PixelSize size, int r, Color color) = 0;

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
    virtual void drawTextChars(PixelPoint pt, FontSize fontSize, const char* text, Color color) = 0;
    virtual void drawTextPrintf(PixelPoint pt, FontSize fontSize, Color color, const char* fmt, ...) = 0;

    // Convenience overloads
    void drawTextChars(PixelPoint pt, const char* text, Color color) { drawTextChars(pt, getDefaultFontSize(), text, color); }
    void drawTextChars(PixelPoint pt, const char* text) { drawTextChars(pt, getDefaultFontSize(), text, Colors::WHITE); }

    void drawTextString(PixelPoint pt, FontSize fontSize, String string, Color color) { drawTextChars(pt, fontSize, string.c_str(), color); }
    void drawTextString(PixelPoint pt, String string, Color color) { drawTextChars(pt, getDefaultFontSize(), string.c_str(), color); }
    void drawTextString(PixelPoint pt, String string) { drawTextChars(pt, getDefaultFontSize(), string.c_str(), Colors::WHITE); }

    void drawTextPrintf(PixelPoint pt, Color color, const char* fmt, ...) { va_list arg; va_start(arg, fmt); drawTextPrintf(pt, getDefaultFontSize(), color, fmt, arg); va_end(arg); }
    void drawTextPrintf(PixelPoint pt, const char* fmt, ...) { va_list arg; va_start(arg, fmt); drawTextPrintf(pt, getDefaultFontSize(), Colors::WHITE, fmt, arg); va_end(arg); }

    void drawTextStringInBounds(Bounds bounds, BoundsAnchor justify, FontSize fontSize, String string, Color color) {   auto textBounds = this->getTextBoundSize(fontSize, string); this->drawTextString(bounds.topLeftOf(textBounds, justify),fontSize, string, color);} 
    void drawTextStringInBounds(Bounds bounds, BoundsAnchor justify, String string, Color color) { drawTextStringInBounds(bounds, justify, getDefaultFontSize(), string, color); }
    void drawTextStringInBounds(Bounds bounds, BoundsAnchor justify, String string) { drawTextStringInBounds(bounds, justify, getDefaultFontSize(), string, Colors::WHITE); }

    virtual PixelSize getTextBoundSize(String string) = 0;
    virtual PixelSize getTextBoundSize(FontSize fontSize, String string) = 0;

    virtual void fillScreen        (Color color) = 0;
    virtual void drawGradientLine (PixelPoint p0, PixelPoint p1, Color colorStart, Color colorEnd) = 0;

    virtual void clearScreen () = 0;

    virtual FontSize getDefaultFontSize() = 0;

    virtual void update();
};

#endif // FIRMWORK_GRAPHICS_H
