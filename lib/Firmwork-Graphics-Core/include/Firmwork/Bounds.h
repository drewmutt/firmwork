//
// Created by Andrew Simmons on 10/1/25.
//

#ifndef FIRMWORK_BOUNDS_H
#define FIRMWORK_BOUNDS_H

#include "GraphicsTypes.h"

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
        static PixelPoint translateToPixelPoint(Bounds bounds, PixelPoint offset) { return PixelPoint{bounds.pt.x + offset.x, bounds.pt.y + offset.y}; }
        static Bounds translate(Bounds bounds, PixelPoint offset) { return Bounds{{bounds.pt.x + offset.x, bounds.pt.y + offset.y}, {bounds.size.w, bounds.size.h}}; }
        static Bounds offset(Bounds bounds, int offset) { return Bounds{{bounds.pt.x + offset, bounds.pt.y + offset}, {bounds.size.w - (offset * 2), bounds.size.h - (offset * 2)}}; }

        PixelPoint pt = {0, 0};
        PixelSize size = {0, 0};
        BoundsAnchor anchor = BoundsAnchor::TOP_LEFT;
    
        Bounds(PixelSize size) : size(size) {};
        Bounds() = default;
        Bounds(PixelPoint origin, PixelSize size): pt(origin), size(size) {};

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
            
            int x = pt.x;
            int y = pt.y;

            // First convert to TOP_LEFT
            switch (fromAnchor)
            {
            case BoundsAnchor::TOP_RIGHT:
                x -= size.w;
                break;
            case BoundsAnchor::TOP_CENTER:
                x -= size.w / 2;
                break;
            case BoundsAnchor::MIDDLE_LEFT:
                y -= size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_RIGHT:
                x -= size.w;
                y -= size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_CENTER:
                x -= size.w / 2;
                y -= size.h / 2;
                break;
            case BoundsAnchor::BOTTOM_LEFT:
                y -= size.h;
                break;
            case BoundsAnchor::BOTTOM_RIGHT:
                x -= size.w;
                y -= size.h;
                break;
            case BoundsAnchor::BOTTOM_CENTER:
                x -= size.w / 2;
                y -= size.h;
                break;
            default:
                break;
            }

            // Then convert from TOP_LEFT to target anchor
            switch (toAnchor)
            {
            case BoundsAnchor::TOP_RIGHT:
                x += size.w;
                break;
            case BoundsAnchor::TOP_CENTER:
                x += size.w / 2;
                break;
            case BoundsAnchor::MIDDLE_LEFT:
                y += size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_RIGHT:
                x += size.w;
                y += size.h / 2;
                break;
            case BoundsAnchor::MIDDLE_CENTER:
                x += size.w / 2;
                y += size.h / 2;
                break;
            case BoundsAnchor::BOTTOM_LEFT:
                y += size.h;
                break;
            case BoundsAnchor::BOTTOM_RIGHT:
                x += size.w;
                y += size.h;
                break;
            case BoundsAnchor::BOTTOM_CENTER:
                x += size.w / 2;
                y += size.h;
                break;
            default:
                break;
            }

            return PixelPoint{x, y};
        }

};

#endif //FIRMWORK_BOUNDS_H