//
// Created by Andrew Simmons on 10/25/25.
//

#ifndef FIRMWORK_SELECTABLE_H
#define FIRMWORK_SELECTABLE_H
#include <optional>
class Selectable
{
public:
    virtual void setSelectedItemIndex(int index) = 0;
    virtual std::optional<unsigned int> getSelectedItemIndex() = 0;
    virtual unsigned int getTotalItems() = 0;
    virtual void chooseItemAtSelectedIndex() = 0;
};
#endif //FIRMWORK_SELECTABLE_H