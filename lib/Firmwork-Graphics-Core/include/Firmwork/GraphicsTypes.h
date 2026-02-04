//
// Shared graphics primitive types to avoid circular includes
//
#ifndef FIRMWORK_GRAPHICSTYPES_H
#define FIRMWORK_GRAPHICSTYPES_H

#include <stdint.h>

// Basic geometry types
#define IS_PIXEL_POINT_ZERO(pp) pp.x == 0 && pp.y == 0
struct PixelPoint {
    int x;
    int y;
    PixelPoint(int x, int y);
    [[nodiscard]] PixelPoint add(PixelPoint point) const { return PixelPoint{this->x + point.x, this->y + point.y}; }
    [[nodiscard]] PixelPoint subtract(PixelPoint point) const { return PixelPoint{this->x - point.x, this->y - point.y}; }
};

#define IS_PIXEL_SIZE_ZERO(ps) ps.w == 0 && ps.h == 0
struct PixelSize {
    int w;
    int h;
    PixelSize(int w, int h);
};


#define COLOR_IS_TRANSPARENT(col) (col > 16777215) // > 0xFFFFFF
// Color types
using Color = uint32_t; // RGB888 stored as 0xRRGGBB
struct ColorRGB { uint8_t r, g, b; };

// h in degrees [0,360), s in [0,1], v in [0,1]
struct ColorHSV { float h, s, v; };

using FontSize = float;

#endif // FIRMWORK_GRAPHICSTYPES_H
