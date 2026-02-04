//
// Created by Andrew Simmons on 10/17/25.
//

#ifndef FIRMWORK_MENU_H
#define FIRMWORK_MENU_H
#include "MenuItemUIElement.h"
#include "UILayout.h"
#include "Firmwork/Graphics.h"
#include "Firmwork/LineStyle.h"
#include "Firmwork/Colors.h"
#include "UIElement.h"
#include <optional>

#include "Selectable.h"

class MenuUIElement: public UIElement, public Selectable
{
    std::vector<MenuItemUIElement *> menuItems = {};
    int selectedItemIndex = -1;
    Color selectedItemColor = Colors::WHITE;
    Color unselectedItemColor = Colors::LIGHTGREY;
public:

    MenuUIElement()
    {
        outlineStyle = new LineStyle(1, Colors::WHITE);
        childLayout = new VerticalUILayout();
    }

    void chooseItemAtSelectedIndex() override
    {
        if (this->getSelectedItem().has_value())
            this->getSelectedItem().value()->choose();
    }

    Color getSelectedItemColor() const
    {
        return selectedItemColor;
    }

    void setSelectedItemColor(Color selectedItemColor)
    {
        this->selectedItemColor = selectedItemColor;
        updateMenuItemsDisplay();
    }

    Color getUnselectedItemColor() const
    {
        return unselectedItemColor;
    }

    void setUnselectedItemColor(Color unselectedItemColor)
    {
        this->unselectedItemColor = unselectedItemColor;
        updateMenuItemsDisplay();
    }

    std::optional<unsigned int>  getSelectedItemIndex()  override
    {
        if (selectedItemIndex < 0)
            return std::nullopt;

        return selectedItemIndex;
    }

    unsigned int getTotalItems() override { return menuItems.size(); };

    void updateMenuItemsDisplay()
    {
        for (auto menuItem:menuItems)
        {
            menuItem->setTextColor(unselectedItemColor);
        }

        auto menuItemUiElement = getSelectedItem();
        if (menuItemUiElement.has_value())
            menuItemUiElement.value()->setTextColor(selectedItemColor);
    }

    void setSelectedItemIndex(int index) override
    {
        if (index == selectedItemIndex)
            return;

        if (getSelectedItem().has_value())
            getSelectedItem().value()->setSelected(false);

        selectedItemIndex = index;

        if (getSelectedItem().has_value())
            getSelectedItem().value()->setSelected(true);

        updateMenuItemsDisplay();
    }

    std::optional<MenuItemUIElement*> getSelectedItem()
    {
        if (menuItems.empty() || selectedItemIndex < 0)
            return std::nullopt;

        return menuItems[selectedItemIndex];
    }

    void addMenuItem(MenuItemUIElement* menuItem)
    {
        menuItems.push_back(menuItem);

        if (selectedItemIndex == -1)
            setSelectedItemIndex(0);
        else
            updateMenuItemsDisplay();

        this->addChild(menuItem);
    }

};


#endif //FIRMWORK_MENU_H