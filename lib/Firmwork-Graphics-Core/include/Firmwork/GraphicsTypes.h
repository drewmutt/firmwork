//
// Shared graphics primitive types to avoid circular includes
//
#ifndef FIRMWORK_GRAPHICSTYPES_H
#define FIRMWORK_GRAPHICSTYPES_H

#include <stdint.h>

// Basic geometry types
struct PixelPoint {
    const int x;
    const int y;
    PixelPoint(int x, int y);
};

struct PixelSize {
    const int w;
    const int h;
    PixelSize(int w, int h);
};


// Color types
using Color = uint32_t; // RGB888 stored as 0xRRGGBB
struct ColorRGB { uint8_t r, g, b; };

// h in degrees [0,360), s in [0,1], v in [0,1]
struct ColorHSV { float h, s, v; };

using FontSize = float;

#endif // FIRMWORK_GRAPHICSTYPES_H
