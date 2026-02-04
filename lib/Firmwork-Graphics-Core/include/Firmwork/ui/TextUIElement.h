//
// Created by Andrew Simmons on 10/25/25.
//

#ifndef FIRMWORK_TEXTUIELEMENT_H
#define FIRMWORK_TEXTUIELEMENT_H
#include <WString.h>
#include <Firmwork/Graphics.h>

#include "UIElement.h"
#include "Firmwork/Colors.h"
#include "Log.h"

class TextUIElement:public UIElement
{
protected:
    String text;
    Color textColor;

public:
    TextUIElement() : text(""), textColor(Colors::WHITE) {}
    explicit TextUIElement(String text) : text(std::move(text)), textColor(Colors::WHITE){}
    TextUIElement(String text, Color textColor) : text(std::move(text)), textColor(textColor){}

    String getText() const
    {
        return text;
    }

    void setText(const String& text)
    {
        this->text = text;
    }

    void updateBoundsFromLabel(Graphics* graphics)
    {
        auto textBoundSize = graphics->getTextBoundSize(this->text);
        bounds.size = textBoundSize;
    }

    Bounds getBounds(Graphics *graphics) override
    {
        if (IS_PIXEL_SIZE_ZERO(this->bounds.size))
            updateBoundsFromLabel(graphics);

        return UIElement::getBounds(graphics);
    }

    Color getTextColor() const { return textColor; }
    void setTextColor(Color color) { textColor = color; }

    void _drawSelf(Graphics *graphics, Bounds inBounds) override
    {
        graphics->drawTextString(inBounds.pt, text, textColor);
    }
};


#endif //FIRMWORK_TEXTUIELEMENT_H