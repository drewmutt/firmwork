#include "Firmwork/GrayU8G2Graphics.h"
#include <Firmwork/Colors.h>

#include <cstdarg>
#include <cstdio>
#include <cmath>
#include <algorithm>

// Notes:
// - This backend targets a 1-buffer U8G2 instance which in this fork supports 16 grayscale levels (0..15).
// - We map RGB888 Color to 4-bit grayscale using Rec. 709 luma and clamp to [0,15].
// - Where U8G2 lacks primitives (ellipses, arcs, bezier, fill triangle), we approximate using pixels/lines.

namespace {
    static inline uint8_t colorToGray4(Color c)
    {
        // Allow native 4-bit grayscale (0..15) to be passed directly.
        // If the value fits in 4 bits, treat it as the grayscale level.
        if ((c & 0xFFFFFFF0u) == 0) {
            return (uint8_t)c; // 0..15 maps directly to U8G2 grayscale
        }
        // Otherwise, convert 0xRRGGBB to 4-bit grayscale using Rec. 709 luma.
        uint8_t r = (uint8_t)((c >> 16) & 0xFF);
        uint8_t g = (uint8_t)((c >> 8)  & 0xFF);
        uint8_t b = (uint8_t)(c & 0xFF);
        float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b; // 0..255
        int gray = (int)std::lround((luma / 255.0f) * 15.0f);
        if (gray < 0) gray = 0; else if (gray > 15) gray = 15;
        return (uint8_t)gray;
    }

    static inline void setColor(U8G2& u8, Color c)
    {
        u8.setDrawColor(colorToGray4(c));
    }

    static inline int isign(int v) { return (v > 0) - (v < 0); }

    // Bresenham line iterator to avoid heavy float usage
    static void drawLineBresenham(U8G2& u8, int x0, int y0, int x1, int y1)
    {
        int dx = std::abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;  // error value e_xy
        for(;;) {
            u8.drawPixel(x0, y0);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    // Midpoint circle/ellipse helpers
    static void plot4EllipsePoints(U8G2& u8, int cx, int cy, int x, int y)
    { u8.drawPixel(cx + x, cy + y); u8.drawPixel(cx - x, cy + y); u8.drawPixel(cx + x, cy - y); u8.drawPixel(cx - x, cy - y); }

    static void hline(U8G2& u8, int x0, int x1, int y)
    { if (x1 < x0) std::swap(x0, x1); u8.drawHLine(x0, y, x1 - x0 + 1); }

    static void vline(U8G2& u8, int x, int y0, int y1)
    { if (y1 < y0) std::swap(y0, y1); u8.drawVLine(x, y0, y1 - y0 + 1); }

    static void drawEllipseOutline(U8G2& u8, int cx, int cy, int rx, int ry)
    {
        if (rx <= 0 || ry <= 0) return;
        long rx2 = (long)rx * rx;
        long ry2 = (long)ry * ry;
        long twoRx2 = 2 * rx2;
        long twoRy2 = 2 * ry2;
        long x = 0;
        long y = ry;
        long px = 0;
        long py = twoRx2 * y;

        // Region 1
        long p = std::lround(ry2 - (rx2 * ry) + (0.25 * rx2));
        while (px < py) {
            plot4EllipsePoints(u8, cx, cy, (int)x, (int)y);
            x++;
            px += twoRy2;
            if (p < 0) {
                p += ry2 + px;
            } else {
                y--;
                py -= twoRx2;
                p += ry2 + px - py;
            }
        }
        // Region 2
        p = std::lround(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
        while (y >= 0) {
            plot4EllipsePoints(u8, cx, cy, (int)x, (int)y);
            y--;
            py -= twoRx2;
            if (p > 0) {
                p += rx2 - py;
            } else {
                x++;
                px += twoRy2;
                p += rx2 - py + px;
            }
        }
    }

    static void fillEllipseMidpoint(U8G2& u8, int cx, int cy, int rx, int ry)
    {
        if (rx <= 0 || ry <= 0) return;
        long rx2 = (long)rx * rx;
        long ry2 = (long)ry * ry;
        long twoRx2 = 2 * rx2;
        long twoRy2 = 2 * ry2;
        long x = 0;
        long y = ry;
        long px = 0;
        long py = twoRx2 * y;

        long p = std::lround(ry2 - (rx2 * ry) + (0.25 * rx2));
        while (px < py) {
            hline(u8, cx - (int)x, cx + (int)x, cy + (int)y);
            hline(u8, cx - (int)x, cx + (int)x, cy - (int)y);
            x++;
            px += twoRy2;
            if (p < 0) {
                p += ry2 + px;
            } else {
                y--;
                py -= twoRx2;
                p += ry2 + px - py;
            }
        }
        p = std::lround(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
        while (y >= 0) {
            hline(u8, cx - (int)x, cx + (int)x, cy + (int)y);
            hline(u8, cx - (int)x, cx + (int)x, cy - (int)y);
            y--;
            py -= twoRx2;
            if (p > 0) {
                p += rx2 - py;
            } else {
                x++;
                px += twoRy2;
                p += rx2 - py + px;
            }
        }
    }

    // Filled triangle via scanline rasterization
    static void fillTriangleImpl(U8G2& u8, int x0, int y0, int x1, int y1, int x2, int y2)
    {
        // Sort vertices by y
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        auto drawSpan = [&](int y, int xa, int xb){ hline(u8, xa, xb, y); };
        auto interp = [](int y, int y0, int y1, int x0, int x1)->int {
            if (y1 == y0) return x0;
            return x0 + (int)std::lround((double)(x1 - x0) * (double)(y - y0) / (double)(y1 - y0));
        };
        // y0..y1
        for (int y = y0; y <= y1; ++y) {
            int xa = interp(y, y0, y2, x0, x2);
            int xb = interp(y, y0, y1, x0, x1);
            drawSpan(y, xa, xb);
        }
        // y1..y2
        for (int y = y1; y <= y2; ++y) {
            int xa = interp(y, y0, y2, x0, x2);
            int xb = interp(y, y1, y2, x1, x2);
            drawSpan(y, xa, xb);
        }
    }

    // Bezier helpers
    static inline void quadBezierPoint(float t, int x0,int y0,int x1,int y1,int x2,int y2, int &x,int &y){
        float it = 1.0f - t;
        float a = it*it; float b = 2*it*t; float c = t*t;
        x = (int)std::lround(a*x0 + b*x1 + c*x2);
        y = (int)std::lround(a*y0 + b*y1 + c*y2);
    }

    static inline void cubicBezierPoint(float t, int x0,int y0,int x1,int y1,int x2,int y2,int x3,int y3, int &x,int &y){
        float it = 1.0f - t;
        float a = it*it*it; float b = 3*it*it*t; float c = 3*it*t*t; float d = t*t*t;
        x = (int)std::lround(a*x0 + b*x1 + c*x2 + d*x3);
        y = (int)std::lround(a*y0 + b*y1 + c*y2 + d*y3);
    }
}

void GrayU8G2Graphics::start()
{
    gfx_.begin();
}
// Basic pixels and lines
void GrayU8G2Graphics::drawPixel(PixelPoint pt, Color color) {
    setColor(gfx_, color);
    gfx_.drawPixel(pt.x, pt.y);
}

void GrayU8G2Graphics::drawFastVLine(PixelPoint start, int h, Color color) {
    setColor(gfx_, color);
    gfx_.drawVLine(start.x, start.y, h);
}

void GrayU8G2Graphics::drawFastHLine(PixelPoint start, int w, Color color) {
    setColor(gfx_, color);
    gfx_.drawHLine(start.x, start.y, w);
}

// Rectangles
void GrayU8G2Graphics::fillRect(PixelPoint topLeft, PixelSize size, Color color) {
    setColor(gfx_, color);
    gfx_.drawBox(topLeft.x, topLeft.y, size.w, size.h);
}

void GrayU8G2Graphics::drawRect(PixelPoint topLeft, PixelSize size, Color color) {
    setColor(gfx_, color);
    gfx_.drawFrame(topLeft.x, topLeft.y, size.w, size.h);
}

void GrayU8G2Graphics::drawRoundRect(PixelPoint topLeft, PixelSize size, int r, Color color) {
    setColor(gfx_, color);
    gfx_.drawRFrame(topLeft.x, topLeft.y, size.w, size.h, r);
}

void GrayU8G2Graphics::fillRoundRect(PixelPoint topLeft, PixelSize size, int r, Color color) {
    setColor(gfx_, color);
    gfx_.drawRBox(topLeft.x, topLeft.y, size.w, size.h, r);
}

// Circles & Ellipses
void GrayU8G2Graphics::drawCircle(PixelPoint center, int r, Color color) {
    setColor(gfx_, color);
    gfx_.drawCircle(center.x, center.y, r, U8G2_DRAW_ALL);
}

void GrayU8G2Graphics::fillCircle(PixelPoint center, int r, Color color) {
    setColor(gfx_, color);
    gfx_.drawDisc(center.x, center.y, r, U8G2_DRAW_ALL);
}

void GrayU8G2Graphics::drawEllipse(PixelPoint center, PixelSize radii, Color color) {
    setColor(gfx_, color);
    // U8G2 does not uniformly expose ellipse in all drivers; use midpoint algorithm
    drawEllipseOutline(gfx_, center.x, center.y, radii.w, radii.h);
}

void GrayU8G2Graphics::fillEllipse(PixelPoint center, PixelSize radii, Color color) {
    setColor(gfx_, color);
    fillEllipseMidpoint(gfx_, center.x, center.y, radii.w, radii.h);
}

// Lines & polygons
void GrayU8G2Graphics::drawLine(PixelPoint p0, PixelPoint p1, Color color) {
    setColor(gfx_, color);
    gfx_.drawLine(p0.x, p0.y, p1.x, p1.y);
}

void GrayU8G2Graphics::drawTriangle(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    setColor(gfx_, color);
    // Outline triangle via lines for consistent behavior
    gfx_.drawLine(p0.x, p0.y, p1.x, p1.y);
    gfx_.drawLine(p1.x, p1.y, p2.x, p2.y);
    gfx_.drawLine(p2.x, p2.y, p0.x, p0.y);
}

void GrayU8G2Graphics::fillTriangle(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    setColor(gfx_, color);
    fillTriangleImpl(gfx_, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y);
}

// Bezier curves
void GrayU8G2Graphics::drawBezier(PixelPoint p0, PixelPoint p1, PixelPoint p2, Color color) {
    setColor(gfx_, color);
    // Approximate with short line segments
    int lastX = p0.x, lastY = p0.y;
    const int segments = 32;
    for (int i = 1; i <= segments; ++i) {
        int x, y; quadBezierPoint(i/(float)segments, p0.x,p0.y,p1.x,p1.y,p2.x,p2.y, x,y);
        drawLineBresenham(gfx_, lastX, lastY, x, y);
        lastX = x; lastY = y;
    }
}

void GrayU8G2Graphics::drawBezier(PixelPoint p0, PixelPoint p1, PixelPoint p2, PixelPoint p3, Color color) {
    setColor(gfx_, color);
    int lastX = p0.x, lastY = p0.y;
    const int segments = 32;
    for (int i = 1; i <= segments; ++i) {
        int x, y; cubicBezierPoint(i/(float)segments, p0.x,p0.y,p1.x,p1.y,p2.x,p2.y,p3.x,p3.y, x,y);
        drawLineBresenham(gfx_, lastX, lastY, x, y);
        lastX = x; lastY = y;
    }
}

// Arcs
void GrayU8G2Graphics::drawEllipseArc(PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) {
    setColor(gfx_, color);
    if (angle1 < angle0) std::swap(angle0, angle1);
    const float toRad = (float)M_PI / 180.0f;
    float step = 1.0f; // degrees
    int prevOx=-1, prevOy=-1, prevIx=-1, prevIy=-1;
    for (float a = angle0; a <= angle1; a += step) {
        float rad = a * toRad;
        int ox = center.x + (int)std::lround(r1.w * std::cos(rad));
        int oy = center.y + (int)std::lround(r1.h * std::sin(rad));
        int ix = center.x + (int)std::lround(r0.w * std::cos(rad));
        int iy = center.y + (int)std::lround(r0.h * std::sin(rad));
        // Draw outer and inner along arc
        if (prevOx>=0) drawLineBresenham(gfx_, prevOx, prevOy, ox, oy);
        if (prevIx>=0) drawLineBresenham(gfx_, prevIx, prevIy, ix, iy);
        prevOx = ox; prevOy = oy; prevIx = ix; prevIy = iy;
    }
}

void GrayU8G2Graphics::fillEllipseArc(PixelPoint center, PixelSize r0, PixelSize r1, float angle0, float angle1, Color color) {
    setColor(gfx_, color);
    if (angle1 < angle0) std::swap(angle0, angle1);
    const float toRad = (float)M_PI / 180.0f;
    float step = 1.0f; // degrees
    for (float a = angle0; a <= angle1; a += step) {
        float rad = a * toRad;
        int ox = center.x + (int)std::lround(r1.w * std::cos(rad));
        int oy = center.y + (int)std::lround(r1.h * std::sin(rad));
        int ix = center.x + (int)std::lround(r0.w * std::cos(rad));
        int iy = center.y + (int)std::lround(r0.h * std::sin(rad));
        drawLineBresenham(gfx_, ix, iy, ox, oy);
    }
}

void GrayU8G2Graphics::drawArc(PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) {
    setColor(gfx_, color);
    if (angle1 < angle0) std::swap(angle0, angle1);
    const float toRad = (float)M_PI / 180.0f;
    float step = 1.0f;
    int prevOx=-1, prevOy=-1, prevIx=-1, prevIy=-1;
    for (float a = angle0; a <= angle1; a += step) {
        float rad = a * toRad;
        int ox = center.x + (int)std::lround(r1 * std::cos(rad));
        int oy = center.y + (int)std::lround(r1 * std::sin(rad));
        int ix = center.x + (int)std::lround(r0 * std::cos(rad));
        int iy = center.y + (int)std::lround(r0 * std::sin(rad));
        if (prevOx>=0) drawLineBresenham(gfx_, prevOx, prevOy, ox, oy);
        if (prevIx>=0) drawLineBresenham(gfx_, prevIx, prevIy, ix, iy);
        prevOx = ox; prevOy = oy; prevIx = ix; prevIy = iy;
    }
}

void GrayU8G2Graphics::fillArc(PixelPoint center, int r0, int r1, float angle0, float angle1, Color color) {
    setColor(gfx_, color);
    if (angle1 < angle0) std::swap(angle0, angle1);
    const float toRad = (float)M_PI / 180.0f;
    float step = 1.0f;
    for (float a = angle0; a <= angle1; a += step) {
        float rad = a * toRad;
        int ox = center.x + (int)std::lround(r1 * std::cos(rad));
        int oy = center.y + (int)std::lround(r1 * std::sin(rad));
        int ix = center.x + (int)std::lround(r0 * std::cos(rad));
        int iy = center.y + (int)std::lround(r0 * std::sin(rad));
        drawLineBresenham(gfx_, ix, iy, ox, oy);
    }
}

// Text & fills
void GrayU8G2Graphics::drawTextChars(PixelPoint pt, FontSize fontSize, const char* text, Color color)
{
    // U8G2 does not support arbitrary font scaling; we ignore fontSize here.
    // (void)fontSize;
    setColor(gfx_, color);
    gfx_.setFontPosTop(); // so y is top-aligned
    gfx_.drawStr(pt.x, pt.y, text ? text : "");
}

void GrayU8G2Graphics::drawTextPrintf(PixelPoint pt, FontSize fontSize, Color color, const char* fmt, ...)
{
    (void)fontSize;
    setColor(gfx_, color);
    gfx_.setFontPosTop();

    char buf[256];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt ? fmt : "", arg);
    va_end(arg);

    gfx_.drawStr(pt.x, pt.y, buf);
}

void GrayU8G2Graphics::fillScreen(Color color) {
    // Without pixel readback, provide a simple filled disc as a reasonable stand-in
    // Fill entire buffer with the given grayscale color
    int w = gfx_.getDisplayWidth();
    int h = gfx_.getDisplayHeight();
    gfx_.clearBuffer();
    setColor(gfx_, color);
    gfx_.drawBox(0, 0, w, h);
    gfx_.sendBuffer();
}

void GrayU8G2Graphics::drawGradientLine(PixelPoint p0, PixelPoint p1, Color colorStart, Color colorEnd) {
    // Interpolate in 4-bit grayscale and step with Bresenham
    int x0 = p0.x, y0 = p0.y, x1 = p1.x, y1 = p1.y;
    uint8_t g0 = colorToGray4(colorStart);
    uint8_t g1 = colorToGray4(colorEnd);

    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int len = std::max(dx, -dy) + 1;
    int step = 0;

    while (true) {
        uint8_t g = (uint8_t)std::lround(g0 + (g1 - g0) * (step / (float)std::max(1, len - 1)));
        gfx_.setDrawColor(g);
        gfx_.drawPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
        ++step;
    }
}

void GrayU8G2Graphics::clearScreen()
{
    gfx_.clearBuffer();
    gfx_.sendBuffer();
}

PixelSize GrayU8G2Graphics::getTextBoundSize(String string)
{
    // Width via U8G2; height via ascent-descent
    int w = gfx_.getStrWidth(string.c_str());
    int h = gfx_.getAscent() - gfx_.getDescent();
    if (h < 0) h = -h;
    return { w, h };
}

PixelSize GrayU8G2Graphics::getTextBoundSize(FontSize fontSize, String string)
{
    (void)fontSize; // Not supported by U8G2; return current font metrics
    return getTextBoundSize(string);
}

void GrayU8G2Graphics::update()
{
    gfx_.sendBuffer();
}
