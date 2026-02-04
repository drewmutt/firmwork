//
// Created by Andrew Simmons on 10/17/25.
//

#ifndef FIRMWORK_UIELEMENT_H
#define FIRMWORK_UIELEMENT_H
#include "UILayout.h"
#include "Firmwork/Bounds.h"
#include "Firmwork/Graphics.h"
#include <vector>
#include <Firmwork/LineStyle.h>




class UIElement
{
protected:
    std::vector<UIElement *> children = {};
    Bounds bounds = {{0, 0}, {0, 0}};
    int innerPadding = 3;
    LineStyle *outlineStyle = nullptr;
    UILayout *childLayout = nullptr;
    Color backgroundColor = Colors::TRANSPARENT;
public:
    [[nodiscard]] std::vector<UIElement*> getChildren() const { return children; }
    virtual Bounds getBounds(Graphics *graphics) { return bounds; }

    void setBounds(const Bounds& aBounds)
    {
        setPosition(aBounds.pt);
        setSize(aBounds.size);
    }

    void setPosition(PixelPoint pos) { bounds.pt = pos; }
    void setSize(PixelSize size) { bounds.size = size; }

    virtual ~UIElement() = default;
    void addChild(UIElement *child) { children.push_back(child); }

    void autoSizeToChildren(Graphics *graphics)
    {
        auto childBounds = this->layout(graphics);
        setSize(childBounds.size);
    }

    // Try not to override this
    virtual void draw(Graphics *graphics, Bounds inBounds)
    {
        Bounds childBounds = Bounds::translate(inBounds, bounds.pt);

        if (backgroundColor != Colors::TRANSPARENT)
            graphics->fillRect(childBounds.pt, bounds.size, backgroundColor);

        if (outlineStyle)
            graphics->drawRect(childBounds.pt, bounds.size, outlineStyle->color);

        layout(graphics);
        _layoutSelf(graphics, childBounds);
        _drawSelf(graphics, childBounds);

        if (innerPadding != 0)
            childBounds = Bounds::offset(inBounds, innerPadding);

        for (auto child:children)
        {
            child->draw(graphics, childBounds);
        }
    }

    [[nodiscard]] Color getBackgroundColor() const
    {
        return backgroundColor;
    }

    void setBackgroundColor(Color aBackgroundColor)
    {
        this->backgroundColor = aBackgroundColor;
    }

    [[nodiscard]] int getInnerPadding() const
    {
        return innerPadding;
    }

    void setInnerPadding(int aInnerPadding)
    {
        this->innerPadding = aInnerPadding;
    }

    [[nodiscard]] LineStyle* getOutlineStyle() const
    {
        return outlineStyle;
    }

    void setOutlineStyle(LineStyle* aOutlineStyle)
    {
        this->outlineStyle = aOutlineStyle;
    }

    [[nodiscard]] UILayout* getChildLayout() const
    {
        return childLayout;
    }

    void setChildLayout(UILayout* aChildLayout)
    {
        this->childLayout = aChildLayout;
    }

    virtual Bounds layout(Graphics *graphics)
    {
        if (childLayout)
            return childLayout->layout(children, graphics, bounds);

        return bounds;
    }

    // Implement drawing here in ancestors
    virtual void _layoutSelf(Graphics *graphics, Bounds inBounds)
    {

    }

    virtual void _drawSelf(Graphics *graphics, Bounds inBounds)
    {

    }


};


#endif //FIRMWORK_UIELEMENT_H