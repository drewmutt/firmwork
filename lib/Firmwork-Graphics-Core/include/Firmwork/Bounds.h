//
// Created by Andrew Simmons on 10/1/25.
//

#ifndef FIRMWORK_BOUNDS_H
#define FIRMWORK_BOUNDS_H

#include "Graphics.h"

enum class BoundsAnchor {
    TOP_LEFT,
    TOP_RIGHT,
    TOP_CENTER,
    MIDDLE_LEFT,
    MIDDLE_RIGHT,
    MIDDLE_CENTER,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    BOTTOM_CENTER
};

struct Bounds {
        PixelPoint pt = {0, 0};
        PixelSize size;
        BoundsAnchor anchor = BoundsAnchor::TOP_LEFT;
    
        explicit Bounds(PixelSize size) : size(size) {};
        explicit Bounds(PixelPoint origin, PixelSize size): pt(origin), size(size) {};

        PixelPoint middleCenter() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::MIDDLE_CENTER); }
        PixelPoint middleLeft() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::MIDDLE_LEFT); }
        PixelPoint middleRight() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::MIDDLE_RIGHT); }
        PixelPoint topLeft() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::TOP_LEFT); }
        PixelPoint topRight() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::TOP_RIGHT); }
        PixelPoint topCenter() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::TOP_CENTER); }
        PixelPoint bottomLeft() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::BOTTOM_LEFT); }
        PixelPoint bottomRight() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::BOTTOM_RIGHT); }
        PixelPoint bottomCenter() { return convertAnchorPoint(pt, size, anchor, BoundsAnchor::BOTTOM_CENTER); }

        // To justify boxes inside boxes
        // To get the top left of a 100x200px box I want to place in the center of this Bound..
        // b.topLeftOf({100, 200}, MIDDLE_CENTER);
        PixelPoint topLeftOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::TOP_LEFT); }
        PixelPoint topRightOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::TOP_RIGHT); }
        PixelPoint topCenterOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::TOP_CENTER); }
        PixelPoint middleLeftOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::MIDDLE_LEFT); }
        PixelPoint middleRightOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::MIDDLE_RIGHT); }
        PixelPoint middleCenterOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::MIDDLE_CENTER); }
        PixelPoint bottomLeftOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::BOTTOM_LEFT); }
        PixelPoint bottomRightOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::BOTTOM_RIGHT); }
        PixelPoint bottomCenterOf(PixelSize size, BoundsAnchor placeAnchor) { return convertAnchorPoint(convertAnchorPoint(pt, this->size, anchor, placeAnchor), size, placeAnchor, BoundsAnchor::BOTTOM_CENTER); }


        static PixelPoint convertAnchorPoint(PixelPoint pt, PixelSize size, BoundsAnchor fromAnchor, BoundsAnchor toAnchor)
        {
            if (fromAnchor == toAnchor)
                return pt;
            
            PixelPoint result = pt;

            // First convert to TOP_LEFT
            switch (fromAnchor)
            {
            case BoundsAnchor::TOP_RIGHT:
                result.x -= size.w;
                break;
            case BoundsAnchor::TOP_CENTER:
                result.x -= size.w / 2;
                break;
            case BoundsAnchor::MIDDLE_LEFT:
                result.y -= size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_RIGHT:
                result.x -= size.w;
                result.y -= size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_CENTER:
                result.x -= size.w / 2;
                result.y -= size.h / 2;
                break;
            case BoundsAnchor::BOTTOM_LEFT:
                result.y -= size.h;
                break;
            case BoundsAnchor::BOTTOM_RIGHT:
                result.x -= size.w;
                result.y -= size.h;
                break;
            case BoundsAnchor::BOTTOM_CENTER:
                result.x -= size.w / 2;
                result.y -= size.h;
                break;
            default:
                break;
            }

            // Then convert from TOP_LEFT to target anchor
            switch (toAnchor)
            {
            case BoundsAnchor::TOP_RIGHT:
                result.x += size.w;
                break;
            case BoundsAnchor::TOP_CENTER:
                result.x += size.w / 2;
                break;
            case BoundsAnchor::MIDDLE_LEFT:
                result.y += size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_RIGHT:
                result.x += size.w;
                result.y += size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_CENTER:
                result.x += size.w / 2;
                result.y += size.h / 2;
                break;
            case BoundsAnchor::BOTTOM_LEFT:
                result.y += size.h;
                break;
            case BoundsAnchor::BOTTOM_RIGHT:
                result.x += size.w;
                result.y += size.h;
                break;
            case BoundsAnchor::BOTTOM_CENTER:
                result.x += size.w / 2;
                result.y += size.h;
                break;
            default:
                break;
            }

            return result;
        }

};

#endif //FIRMWORK_BOUNDS_H