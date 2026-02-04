//
// Created by Andrew Simmons on 10/27/25.
//

#ifndef FIRMWORK_UILAYOUT_H
#define FIRMWORK_UILAYOUT_H

#include <vector>
#include "../Bounds.h"

// Forward declarations to avoid circular dependency with UIElement.h
class UIElement;
class Graphics;

struct UILayout {
    virtual ~UILayout() = default;
    virtual Bounds layout(const std::vector<UIElement*>& elements,
                        Graphics *graphics,
                        Bounds bounds) = 0;
};

struct VerticalUILayout : UILayout {
    struct VerticalUILayoutDetails {
        int verticalPadding = 0;
        bool setElementWidthToBounds = false;
    } layoutDetails;

    explicit VerticalUILayout(const VerticalUILayoutDetails& layoutDetails): layoutDetails(layoutDetails){}
    VerticalUILayout() = default;
    Bounds layout(const std::vector<UIElement*>& elements,
                Graphics *graphics,
                Bounds bounds) override;
};

#endif //FIRMWORK_UILAYOUT_H