//
// Created by Andrew Simmons on 2/11/26.
//

#ifndef FIRMWORK_TITLEDUINELEMENT_H
#define FIRMWORK_TITLEDUINELEMENT_H

#include <optional>
#include <WString.h>
#include "UIElement.h"
#include "TextUIElement.h"
#include "Firmwork/Colors.h"

class TitleHeaderUIElement : public TextUIElement
{
    int horizontalPadding = 0;
    int verticalPadding = 0;
    int bottomSpacing = 2;

public:
    TitleHeaderUIElement() : TextUIElement("")
    {
        setTextColor(Colors::BLACK);
        setBackgroundColor(Colors::WHITE);
    }

    void clearTitle()
    {
        setText("");
        setSize({0, 0});
    }

    void setTitle(const String& title)
    {
        setText(title);
        setSize({0, 0});
    }

    Bounds getBounds(Graphics *graphics) override
    {
        if (text.isEmpty())
        {
            setSize({0, 0});
            return UIElement::getBounds(graphics);
        }

        auto textSize = graphics->getTextBoundSize(text);
        int contentWidth = textSize.w + (horizontalPadding * 2);
        int width = bounds.size.w > 0 ? bounds.size.w : contentWidth;
        setSize({
            width,
            textSize.h + (verticalPadding * 2) + bottomSpacing
        });
        return UIElement::getBounds(graphics);
    }

    void _drawSelf(Graphics *graphics, Bounds inBounds) override
    {
        if (text.isEmpty())
            return;

        graphics->drawTextString(
            {inBounds.pt.x + horizontalPadding, inBounds.pt.y + verticalPadding},
            text,
            textColor
        );
    }
};

class TitledUIElement : public UIElement
{
    std::optional<String> title = std::nullopt;
    TitleHeaderUIElement *titleUIElement = nullptr;

protected:
    void initTitleHeader()
    {
        if (titleUIElement)
            return;

        titleUIElement = new TitleHeaderUIElement();
        addChild(titleUIElement);
    }

public:
    Bounds layout(Graphics *graphics) override
    {
        if (titleUIElement && bounds.size.w > 0)
        {
            int contentWidth = bounds.size.w - (getInnerPadding() * 2);
            if (contentWidth < 0)
                contentWidth = 0;

            auto titleBounds = titleUIElement->getBounds(graphics);
            titleUIElement->setSize({contentWidth, titleBounds.size.h});
        }

        return UIElement::layout(graphics);
    }

    std::optional<String> getTitle() const
    {
        return title;
    }

    void setTitle(const String& aTitle)
    {
        setTitle(std::optional<String>(aTitle));
    }

    void setTitle(const char* aTitle)
    {
        setTitle(String(aTitle));
    }

    void setTitle(const std::optional<String>& aTitle)
    {
        initTitleHeader();

        if (!aTitle.has_value() || aTitle.value().isEmpty())
        {
            title = std::nullopt;
            titleUIElement->clearTitle();
            return;
        }

        title = aTitle;
        titleUIElement->setTitle(aTitle.value());
    }

    void clearTitle()
    {
        setTitle(std::nullopt);
    }

    Color getTitleColor() const
    {
        if (!titleUIElement)
            return Colors::BLACK;

        return titleUIElement->getTextColor();
    }

    void setTitleColor(Color color)
    {
        initTitleHeader();
        titleUIElement->setTextColor(color);
    }

    Color getTitleBackgroundColor() const
    {
        if (!titleUIElement)
            return Colors::WHITE;

        return titleUIElement->getBackgroundColor();
    }

    void setTitleBackgroundColor(Color color)
    {
        initTitleHeader();
        titleUIElement->setBackgroundColor(color);
    }

    // Backward compatibility from divider-style titles.
    Color getTitleDividerColor() const
    {
        return getTitleBackgroundColor();
    }

    void setTitleDividerColor(Color color)
    {
        setTitleBackgroundColor(color);
    }
};

#endif //FIRMWORK_TITLEDUINELEMENT_H
