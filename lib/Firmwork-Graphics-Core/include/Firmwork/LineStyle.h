//
// Created by Andrew Simmons on 10/17/25.
//

#ifndef FIRMWORK_LINESTYLE_H
#define FIRMWORK_LINESTYLE_H
#include "Colors.h"


class LineStyle
{
public:
    LineStyle(unsigned int width, Color color)
        : width(width),
          color(color)
    {
    }

    unsigned int width;
    Color color;
};


#endif //FIRMWORK_LINESTYLE_H