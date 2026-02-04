#include "Firmwork/ui/UILayout.h"
#include "Firmwork/ui/UIElement.h"
#include "Firmwork/Graphics.h"

Bounds VerticalUILayout::layout(const std::vector<UIElement*>& elements,
                              Graphics *graphics,
                              Bounds bounds)
{
    Bounds outBounds = bounds;
    int currentY = bounds.pt.y;
    for (auto element : elements) {
        auto elementBounds = element->getBounds(graphics);
        element->setPosition({bounds.pt.x, currentY});

        if (layoutDetails.setElementWidthToBounds)
        {
            element->setSize({bounds.size.w, elementBounds.size.h});
        }

        currentY += elementBounds.size.h + layoutDetails.verticalPadding;
    }

    if (layoutDetails.setElementWidthToBounds)
        outBounds.size.w = bounds.size.w;

    outBounds.size.h = currentY - bounds.pt.y;

    return outBounds;
}
