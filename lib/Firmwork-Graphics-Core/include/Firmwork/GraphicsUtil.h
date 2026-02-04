//
// Created by Andrew Simmons on 10/17/25.
//

#ifndef FIRMWORK_GRAPHICSUTIL_H
#define FIRMWORK_GRAPHICSUTIL_H

class GraphicsUtil
{
public:
    void static drawRectWidthInside(Graphics *graphics, PixelPoint topLeft, PixelSize size, Color color, unsigned int width)
    {
        for (int i = 0; i < width; i++)
        {
            PixelPoint drawPoint = {topLeft.x + i, topLeft.y + i};
            PixelSize drawSize = {size.w - i * 2, size.h - i * 2};
            graphics->drawRect(drawPoint, drawSize, color);
        }
    }
};
#endif //FIRMWORK_GRAPHICSUTIL_H