#include "Firmwork/Graphics.h"

// Define simple constructors so they are available to all translation units
PixelPoint::PixelPoint(int x, int y) : x(x), y(y) {}
PixelSize::PixelSize(int w, int h) : w(w), h(h) {}