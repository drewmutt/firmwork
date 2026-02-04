//
// Created by Andrew Simmons on 10/25/25.
//

#ifndef FIRMWORK_ROTARYSELECTORTOMENUCONNECTOR_H
#define FIRMWORK_ROTARYSELECTORTOMENUCONNECTOR_H
#include <RotaryEncoder.h>
#include <Firmwork/ui/Selectable.h>
#include <optional>

class RotaryEncoderToSelectableConnector
{
public:
    RotaryEncoder *encoder;
    Selectable *selectable;
    bool loopAround = true;

    RotaryEncoderToSelectableConnector(RotaryEncoder* encoder, Selectable* selectable)
        : encoder(encoder),
          selectable(selectable)
    {

    }

    void onEncoderStep(int step)
    {
        std::optional<unsigned int> selectedItemIndex = selectable->getSelectedItemIndex();
        if (!selectedItemIndex.has_value())
            return;

        int totalItems = selectable->getTotalItems();
        int itemIndex = selectedItemIndex.value() + step;


        if (loopAround)
        {
            if (itemIndex < 0)
                itemIndex = totalItems - 1;
            itemIndex = itemIndex % totalItems;
        }
        else
            itemIndex = itemIndex < 0 ? 0 : itemIndex >= totalItems ? totalItems - 1 : itemIndex;

        selectable->setSelectedItemIndex(itemIndex);
    }
};

#endif //FIRMWORK_ROTARYSELECTORTOMENUCONNECTOR_H