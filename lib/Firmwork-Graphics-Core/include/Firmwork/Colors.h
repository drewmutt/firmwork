//
// Created by Junie (AI) on 9/30/25.
//

#ifndef FIRMWORK_GRAPHICSHELPER_H
#define FIRMWORK_GRAPHICSHELPER_H

#include <stdint.h>
#include <math.h>
#include <Firmwork/GraphicsTypes.h> // for Color and ColorRGB

class Colors {
public:
    // Convert 8-bit-per-channel RGB to Color in RGB888 hex form 0xRRGGBB (stored in uint32_t)
    static inline Color fromRGB(uint8_t r, uint8_t g, uint8_t b) {
        return (static_cast<Color>(r) << 16)
             | (static_cast<Color>(g) << 8)
             | (static_cast<Color>(b));
    }

    // Overload accepting ints (clamped to [0,255])
    static inline Color fromRGB(int r, int g, int b) {
        if (r < 0) r = 0; else if (r > 255) r = 255;
        if (g < 0) g = 0; else if (g > 255) g = 255;
        if (b < 0) b = 0; else if (b > 255) b = 255;
        return fromRGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    }

    // Overload accepting ColorRGB struct
    static inline Color fromRGB(ColorRGB rgb) {
        return fromRGB(rgb.r, rgb.g, rgb.b);
    }

    // Convert Color (RGB888 0xRRGGBB stored in 32-bit) to 8-bit-per-channel ColorRGB
    static inline ColorRGB toRGB(Color color) {
        ColorRGB out;
        out.r = static_cast<uint8_t>((color >> 16) & 0xFFu);
        out.g = static_cast<uint8_t>((color >> 8)  & 0xFFu);
        out.b = static_cast<uint8_t>( color        & 0xFFu);
        return out;
    }

    // Convert HSV (h:0-360, s:0-1, v:0-1) to Color in RGB888 (0xRRGGBB)
    static inline Color fromHSV(float h, float s, float v) {
        // Clamp inputs
        if (s < 0.0f) s = 0.0f; else if (s > 1.0f) s = 1.0f;
        if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;
        if (!isfinite(h)) h = 0.0f;
        // Wrap hue into [0,360)
        h = fmodf(h, 360.0f);
        if (h < 0.0f) h += 360.0f;

        float r = v, g = v, b = v;
        if (s <= 0.0f) {
            // Achromatic (grey)
            uint8_t R = static_cast<uint8_t>(r * 255.0f + 0.5f);
            uint8_t G = static_cast<uint8_t>(g * 255.0f + 0.5f);
            uint8_t B = static_cast<uint8_t>(b * 255.0f + 0.5f);
            return fromRGB(R, G, B);
        }

        float c = v * s;
        float hh = h / 60.0f;
        int    sector = static_cast<int>(floorf(hh)); // 0..5
        float  f = hh - sector;
        float  x = c * (1.0f - fabsf(2.0f * f - 1.0f));
        float rp = 0.0f, gp = 0.0f, bp = 0.0f;
        switch (sector) {
            case 0: rp = c; gp = x; bp = 0.0f; break;
            case 1: rp = x; gp = c; bp = 0.0f; break;
            case 2: rp = 0.0f; gp = c; bp = x; break;
            case 3: rp = 0.0f; gp = x; bp = c; break;
            case 4: rp = x; gp = 0.0f; bp = c; break;
            default: // 5
                rp = c; gp = 0.0f; bp = x; break;
        }
        float m = v - c;
        r = rp + m; g = gp + m; b = bp + m;

        // Convert to 0..255 with rounding and clamp
        auto to8 = [](float comp) -> uint8_t {
            if (comp <= 0.0f) return 0;
            if (comp >= 1.0f) return 255;
            return static_cast<uint8_t>(comp * 255.0f + 0.5f);
        };
        return fromRGB(to8(r), to8(g), to8(b));
    }

    // Overload accepting ColorHSV
    static inline Color fromHSV(ColorHSV hsv) {
        return fromHSV(hsv.h, hsv.s, hsv.v);
    }

    // Convert Color (RGB565) to HSV (h in [0,360), s,v in [0,1])
    static inline ColorHSV toHSV(Color color) {
        ColorRGB rgb = toRGB(color);
        float rf = rgb.r / 255.0f;
        float gf = rgb.g / 255.0f;
        float bf = rgb.b / 255.0f;

        float cmax = rf;
        if (gf > cmax) cmax = gf;
        if (bf > cmax) cmax = bf;
        float cmin = rf;
        if (gf < cmin) cmin = gf;
        if (bf < cmin) cmin = bf;
        float delta = cmax - cmin;

        float h = 0.0f;
        if (delta > 0.0f) {
            if (cmax == rf) {
                h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
            } else if (cmax == gf) {
                h = 60.0f * (((bf - rf) / delta) + 2.0f);
            } else { // cmax == bf
                h = 60.0f * (((rf - gf) / delta) + 4.0f);
            }
            if (h < 0.0f) h += 360.0f;
        }

        float v = cmax;
        float s = (cmax <= 0.0f) ? 0.0f : (delta / cmax);

        ColorHSV out{h, s, v};
        return out;
    }

    // Convert Color (stored as RGB888 0xRRGGBB) to raw RGB565 value (e.g., 0xF81F)
    static inline uint16_t toRGB565(Color color) {
        uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFFu);
        uint8_t g = static_cast<uint8_t>((color >> 8)  & 0xFFu);
        uint8_t b = static_cast<uint8_t>( color        & 0xFFu);
        uint16_t v = (static_cast<uint16_t>(r >> 3) << 11)
                   | (static_cast<uint16_t>(g >> 2) << 5)
                   | (static_cast<uint16_t>(b >> 3));
        return v;
    }

    // Blend two colors with ratio (0.0 = color1, 1.0 = color2)
    static inline Color blend(Color color1, Color color2, float ratio) {
        if (ratio <= 0.0f) return color1;
        if (ratio >= 1.0f) return color2;

        ColorRGB rgb1 = toRGB(color1);
        ColorRGB rgb2 = toRGB(color2);

        return fromRGB(
            static_cast<uint8_t>(rgb1.r + (rgb2.r - rgb1.r) * ratio),
            static_cast<uint8_t>(rgb1.g + (rgb2.g - rgb1.g) * ratio),
            static_cast<uint8_t>(rgb1.b + (rgb2.b - rgb1.b) * ratio)
        );
    }


    // Named color constants in RGB888 (0xRRGGBB)
    static constexpr Color BLACK       = 0x000000u; //   0,   0,   0
    static constexpr Color NAVY        = 0x000080u; //   0,   0, 128
    static constexpr Color DARKGREEN   = 0x008000u; //   0, 128,   0
    static constexpr Color DARKCYAN    = 0x008080u; //   0, 128, 128
    static constexpr Color MAROON      = 0x800000u; // 128,   0,   0
    static constexpr Color PURPLE      = 0x800080u; // 128,   0, 128
    static constexpr Color OLIVE       = 0x808000u; // 128, 128,   0
    static constexpr Color LIGHTGREY   = 0xD3D3D3u; // 211, 211, 211
    static constexpr Color DARKGREY    = 0x808080u; // 128, 128, 128
    static constexpr Color BLUE        = 0x0000FFu; //   0,   0, 255
    static constexpr Color GREEN       = 0x00FF00u; //   0, 255,   0
    static constexpr Color CYAN        = 0x00FFFFu; //   0, 255, 255
    static constexpr Color RED         = 0xFF0000u; // 255,   0,   0
    static constexpr Color MAGENTA     = 0xFF00FFu; // 255,   0, 255
    static constexpr Color YELLOW      = 0xFFFF00u; // 255, 255,   0
    static constexpr Color WHITE       = 0xFFFFFFu; // 255, 255, 255
    static constexpr Color ORANGE      = 0xFFA500u; // 255, 165,   0 (approx from 255,180,0)
    static constexpr Color GREENYELLOW = 0xB4FF00u; // 180, 255,   0
    static constexpr Color PINK        = 0xFFC0CBu; // 255, 192, 203
    static constexpr Color BROWN       = 0x964B00u; // 150,  75,   0 (approx)
    static constexpr Color GOLD        = 0xFFD700u; // 255, 215,   0
    static constexpr Color SILVER      = 0xC0C0C0u; // 192, 192, 192
    static constexpr Color SKYBLUE     = 0x87CEEBu; // 135, 206, 235
    static constexpr Color VIOLET      = 0xB42EE2u; // 180,  46, 226
};

#endif // FIRMWORK_GRAPHICSHELPER_H
