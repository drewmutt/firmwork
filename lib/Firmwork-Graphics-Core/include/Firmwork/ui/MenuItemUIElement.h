//
// Created by Andrew Simmons on 10/17/25.
//

#ifndef FIRMWORK_MENUITEM_H
#define FIRMWORK_MENUITEM_H
#include <WString.h>
#include <Firmwork/ui/UIElement.h>
#include <Firmwork/Bounds.h>
#include <Firmwork/Graphics.h>
#include <Firmwork/Colors.h>

#include "TextUIElement.h"

class MenuItemUIElement: public TextUIElement
{
public:
    explicit MenuItemUIElement(const String& text)
        : TextUIElement(text)
    {
    }

    bool isSelected = false;
    struct MenuItemData
    {
        MenuItemUIElement *menuItemUIElement;
    };
    std::function<void(MenuItemData)> onChoose = nullptr;
    std::function<void(MenuItemData)> onSelect = nullptr;
    std::function<void(MenuItemData)> onDeselect = nullptr;

    void choose()
    {
        MenuItemData data = {this};
        if (onChoose)
            onChoose(data);
    }

    void setSelected(bool selected)
    {
        if (isSelected == selected)
            return;

        MenuItemData data = {this};
        if (!isSelected && selected)
            if (onSelect)
                onSelect(data);
        else
            if (onDeselect)
                onDeselect(data);
    }
};


#endif //FIRMWORK_MENUITEM_H