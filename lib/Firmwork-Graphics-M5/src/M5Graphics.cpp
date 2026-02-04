#include "Firmwork/M5Graphics.h"
#include <Firmwork/Colors.h>

// Basic pixels and lines
void M5Graphics::drawPixel(PixelPoint pt, Color color) {
    gfx_.drawPixel(pt.x, pt.y, color);
}

void M5Graphics::drawFastVLine(PixelPoint start, int h, Color color) {
    gfx_.drawFastVLine(start.x, start.y, h, color);
}

void M5Graphics::drawFastHLine(PixelPoint start, int w, Color color) {
    gfx_.drawFastHLine(start.x, start.y, w, color);
}

// Rectangles
void M5Graphics::fillRect(PixelPoint topLeft, PixelSize size, Color color) {
    gfx_.fillRect(topLeft.x, topLeft.y, size.w, size.h, color);
}

void M5Graphics::drawRect(PixelPoint topLeft, PixelSize size, Color color) {
    gfx_.drawRect(topLeft.x, topLeft.y, size.w, size.h, color);
}

void M5Graphics::drawRoundRect(PixelPoint topLeft, PixelSize size, int r, Color color) {
    gfx_.drawRoundRect(topLeft.x, topLeft.y, size.w, size.h, r, color);
}

void M5Graphics::fillRoundRect(PixelPoint topLeft, PixelSize size, int r, Color color) {
    gfx_.fillRoundRect(topLeft.x, topLeft.y, size.w, size.h, r, color);
}

// Circles & Ellipses
void M5Graphics::drawCircle(PixelPoint center, int r, Color color) {
    gfx_.drawCircle(center.x, center.y, r, color);
}

void M5Graphics::fillCircle(PixelPoint center, int r, Color color) {
    gfx_.fillCircle(center.x, center.y, r, color);
}

void M5Graphics::drawEllipse(PixelPoint center, PixelSize radii, Color color) {
    gfx_.drawEllipse(center.x, center.y, radii.w, radii.h, color);
}

void M5Graphics::fillEllipse(PixelPoint center, PixelSize radii, Color color) {
    gfx_.fillEllipse(center.x, center.y, radii.w, radii.h, color);
}

// Lines & polygons
void M5Graphics::drawLine(PixelPoint p0, PixelPoint p1, Color color) {
    gfx_.drawLine(p0.x, p0.y, p1.x, p1.y, color);
}

void M5Graphics::drawTriangle(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    gfx_.drawTriangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
}

void M5Graphics::fillTriangle(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    gfx_.fillTriangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
}

// Bezier curves
void M5Graphics::drawBezier(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    gfx_.drawBezier(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, color);
}

void M5Graphics::drawBezier(PixelPoint p0, PixelPoint p1, PixelPoint p2, PixelPoint p3, Color color) {
    gfx_.drawBezier(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, color);
}

// Arcs
void M5Graphics::drawEllipseArc(PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) {
    gfx_.drawEllipseArc(center.x, center.y, r0.w, r0.h, r1.w, r1.h, angle0, angle1, color);
}

void M5Graphics::fillEllipseArc(PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) {
    gfx_.fillEllipseArc(center.x, center.y, r0.w, r0.h, r1.w, r1.h, angle0, angle1, color);
}

void M5Graphics::drawArc(PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) {
    gfx_.drawArc(center.x, center.y, r0, r1, angle0, angle1, color);
}

void M5Graphics::fillArc(PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) {
    gfx_.fillArc(center.x, center.y, r0, r1, angle0, angle1, color);
}

// Text & fills
void M5Graphics::drawTextChars(PixelPoint pt, FontSize fontSize, const char* text, Color color)
{
    gfx_.setTextDatum(top_left);
    gfx_.setTextSize(fontSize);
    gfx_.setTextColor(color, 0);
    gfx_.setCursor(pt.x, pt.y);
    gfx_.print(text ? text : "");
}


void M5Graphics::drawTextPrintf(PixelPoint pt, FontSize fontSize, Color color, const char* fmt, ...)
{
    gfx_.setTextDatum(top_left);
    gfx_.setTextSize(fontSize);
    gfx_.setTextColor(color, 0);

    gfx_.setCursor(pt.x, pt.y);
    va_list arg;
    va_start(arg, fmt);
    gfx_.vprintf(fmt, arg);
    va_end(arg);
}

void M5Graphics::fillScreen(Color color) {
    gfx_.fillScreen(color);
}

void M5Graphics::drawGradientLine(PixelPoint p0, PixelPoint p1, Color colorStart, Color colorEnd) {
    // If supported, use native gradient line; otherwise fall back to uniform color line
#if defined(LGFX_VERSION_MAJOR)
    gfx_.drawGradientLine(p0.x, p0.y, p1.x, p1.y, colorStart, colorEnd);
#else
    (void)colorEnd;
    gfx_.drawLine(p0.x, p0.y, p1.x, p1.y, FW_TO565(colorStart));
#endif
}

void M5Graphics::clearScreen()
{
    gfx_.setBaseColor(BLACK);
    gfx_.clear();
}

PixelSize M5Graphics::getTextBoundSize(String string)
{
    int32_t textWidth  = gfx_.textWidth(string.c_str());
    int32_t textHeight = gfx_.textLength(string.c_str(), textWidth);

    return {textWidth, textHeight};
}

PixelSize M5Graphics::getTextBoundSize(FontSize fontSize, String string)
{
    float textSizeX = gfx_.getTextSizeX();
    float textSizeY = gfx_.getTextSizeY();
    gfx_.setTextSize(fontSize);
    auto textBoundSize = getTextBoundSize(string);
    gfx_.setTextSize(textSizeX, textSizeY);
    return textBoundSize;
}


