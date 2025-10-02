#include "Firmwork/M5Graphics.h"
#include <Firmwork/GraphicsHelper.h>

// Convert Firmwork Color (RGB565 in low 16 bits) to M5GFX color value
#ifndef FW_TO565
#define FW_TO565(c) (static_cast<uint32_t>(GraphicsHelper::toRGB565((c))))
#endif

// Basic pixels and lines
void M5Graphics::drawPixel(PixelPoint pt, Color color) {
    gfx_.drawPixel(pt.x, pt.y, FW_TO565(color));
}

void M5Graphics::drawFastVLine(PixelPoint start, int h, Color color) {
    gfx_.drawFastVLine(start.x, start.y, h, FW_TO565(color));
}

void M5Graphics::drawFastHLine(PixelPoint start, int w, Color color) {
    gfx_.drawFastHLine(start.x, start.y, w, FW_TO565(color));
}

// Rectangles
void M5Graphics::fillRect(PixelRect rect, Color color) {
    gfx_.fillRect(rect.p.x, rect.p.y, rect.s.w, rect.s.h, FW_TO565(color));
}

void M5Graphics::drawRect(PixelRect rect, Color color) {
    gfx_.drawRect(rect.p.x, rect.p.y, rect.s.w, rect.s.h, FW_TO565(color));
}

void M5Graphics::drawRoundRect(PixelRect rect, int r, Color color) {
    gfx_.drawRoundRect(rect.p.x, rect.p.y, rect.s.w, rect.s.h, r, FW_TO565(color));
}

void M5Graphics::fillRoundRect(PixelRect rect, int r, Color color) {
    gfx_.fillRoundRect(rect.p.x, rect.p.y, rect.s.w, rect.s.h, r, FW_TO565(color));
}

// Circles & Ellipses
void M5Graphics::drawCircle(PixelPoint center, int r, Color color) {
    gfx_.drawCircle(center.x, center.y, r, FW_TO565(color));
}

void M5Graphics::fillCircle(PixelPoint center, int r, Color color) {
    gfx_.fillCircle(center.x, center.y, r, FW_TO565(color));
}

void M5Graphics::drawEllipse(PixelPoint center, PixelSize radii, Color color) {
    gfx_.drawEllipse(center.x, center.y, radii.w, radii.h, FW_TO565(color));
}

void M5Graphics::fillEllipse(PixelPoint center, PixelSize radii, Color color) {
    gfx_.fillEllipse(center.x, center.y, radii.w, radii.h, FW_TO565(color));
}

// Lines & polygons
void M5Graphics::drawLine(PixelPoint p0, PixelPoint p1, Color color) {
    gfx_.drawLine(p0.x, p0.y, p1.x, p1.y, FW_TO565(color));
}

void M5Graphics::drawTriangle(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    gfx_.drawTriangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, FW_TO565(color));
}

void M5Graphics::fillTriangle(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    gfx_.fillTriangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, FW_TO565(color));
}

// Bezier curves
void M5Graphics::drawBezier(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    gfx_.drawBezier(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, FW_TO565(color));
}

void M5Graphics::drawBezier(PixelPoint p0, PixelPoint p1, PixelPoint p2, PixelPoint p3, Color color) {
    gfx_.drawBezier(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, FW_TO565(color));
}

// Arcs
void M5Graphics::drawEllipseArc(PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) {
    gfx_.drawEllipseArc(center.x, center.y, r0.w, r0.h, r1.w, r1.h, angle0, angle1, FW_TO565(color));
}

void M5Graphics::fillEllipseArc(PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) {
    gfx_.fillEllipseArc(center.x, center.y, r0.w, r0.h, r1.w, r1.h, angle0, angle1, FW_TO565(color));
}

void M5Graphics::drawArc(PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) {
    gfx_.drawArc(center.x, center.y, r0, r1, angle0, angle1, FW_TO565(color));
}

void M5Graphics::fillArc(PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) {
    gfx_.fillArc(center.x, center.y, r0, r1, angle0, angle1, FW_TO565(color));
}

// Text & fills
void M5Graphics::drawTextChars(PixelPoint pt, const char* text) {
    drawTextChars(pt, getDefaultFontSize(), text);
}

void M5Graphics::drawTextChars(PixelPoint pt, FontSize fontSize, const char* text)
{
    gfx_.setTextSize(fontSize);
    gfx_.setCursor(pt.x, pt.y);
    gfx_.print(text ? text : "");
}

void M5Graphics::drawTextString(PixelPoint pt, FontSize fontSize, String string)
{
    drawTextChars(pt, fontSize, string.c_str());
}

void M5Graphics::drawTextString(PixelPoint pt, String string)
{
    drawTextChars(pt, getDefaultFontSize(), string.c_str());
}

void M5Graphics::drawTextPrintf(PixelPoint pt, const char* fmt, ...) {

    va_list arg;
    va_start(arg, fmt);
    drawTextPrintf(pt, getDefaultFontSize(), fmt, arg);
    va_end(arg);
}

void M5Graphics::drawTextPrintf(PixelPoint pt, FontSize fontSize, const char* fmt, ...)
{
    gfx_.setTextSize(fontSize);
    gfx_.setCursor(pt.x, pt.y);
    va_list arg;
    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);
}

void M5Graphics::floodFill(PixelPoint seed, Color color) {
    gfx_.floodFill(seed.x, seed.y, FW_TO565(color));
}

void M5Graphics::drawGradientLine(PixelPoint p0, PixelPoint p1, Color colorStart, Color colorEnd) {
    // If supported, use native gradient line; otherwise fall back to uniform color line
#if defined(LGFX_VERSION_MAJOR)
    gfx_.drawGradientLine(p0.x, p0.y, p1.x, p1.y, FW_TO565(colorStart), FW_TO565(colorEnd));
#else
    (void)colorEnd;
    gfx_.drawLine(p0.x, p0.y, p1.x, p1.y, FW_TO565(colorStart));
#endif
}

void M5Graphics::clearScreen(Color color)
{
    gfx_.setBaseColor(FW_TO565(color));
    gfx_.clear();
}

void M5Graphics::clearScreen()
{
    gfx_.setBaseColor(BLACK);
    gfx_.clear();
}


