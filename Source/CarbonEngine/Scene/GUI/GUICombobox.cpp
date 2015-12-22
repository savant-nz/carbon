/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUICombobox.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

const auto GUIComboboxVersionInfo = VersionInfo(1, 0);

GUICombobox::GUICombobox() : onItemSelectEvent(this)
{
    clear();
}

GUICombobox::~GUICombobox()
{
    onDestruct();
    clear();
}

Color GUICombobox::getTextColor() const
{
    if (useCustomTextColor_)
        return adjustColorAlpha(textColor_);

    if (isEnabled() && isMouseInWindow())
        return adjustColorAlpha(theme()["TextHighlightColor"]);

    return adjustColorAlpha(theme()["TextColor"]);
}

Color GUICombobox::getFillColor() const
{
    if (useCustomFillColor_)
        return adjustColorAlpha(fillColor_);

    if (isInteractive() && ((isEnabled() && isMouseInWindow()) || isExpanded_))
        return adjustColorAlpha(theme()["HoverFillColor"]);

    return adjustColorAlpha(theme()["FillColor"]);
}

bool GUICombobox::processEvent(const Event& e)
{
    if (isEnabled() && isVisibleIgnoreAlpha())
    {
        if (hasFocus())
        {
            if (auto mbde = e.as<MouseButtonDownEvent>())
            {
                if (mbde->getButton() == LeftMouseButton)
                {
                    // Eat button pressed events that occur over the dropdown window
                    if (getMouseoverItem() != -1)
                        return false;
                }
            }
            else if (auto mbue = e.as<MouseButtonUpEvent>())
            {
                if (mbue->getButton() == LeftMouseButton)
                {
                    if (isExpanded_)
                    {
                        // See if an item in the dropdown was clicked
                        auto itemIndex = getMouseoverItem();
                        isExpanded_ = false;
                        if (itemIndex != -1)
                        {
                            setSelectedItem(itemIndex);

                            // Send a mousemove event to update any windows under the dropdown that haven't been getting
                            // mousemove events due to the combobox absorbing them all
                            events().dispatchEvent(MouseMoveEvent(platform().getMousePosition()));

                            // We've handled the mouse button event, so swallow it
                            return false;
                        }
                    }
                    else
                    {
                        // Expand the dropdown on a mouseclick anywhere in the window
                        if (isMouseInWindow())
                            isExpanded_ = true;
                    }
                }
            }

            // Eat mouse move and mouse wheel events over the dropdown window
            else if (e.as<MouseMoveEvent>() || e.as<MouseWheelEvent>())
            {
                if (getMouseoverItem() != -1 && !isBeingDragged())
                    return false;
            }

            // Make the up and down arrows move through the combobox items
            else if (auto kde = e.as<KeyDownEvent>())
            {
                if (kde->getKey() == KeyUpArrow)
                {
                    if (selectedItem_ > 0)
                        setSelectedItem(selectedItem_ - 1);

                    return false;
                }

                if (kde->getKey() == KeyDownArrow)
                {
                    setSelectedItem(selectedItem_ + 1);
                    return false;
                }
            }
        }

        // Make the mouse wheel scroll through the items
        if (auto mwe = e.as<MouseWheelEvent>())
        {
            if (isMouseInWindow())
            {
                if (mwe->getDirection() == MouseWheelEvent::AwayFromUser)
                {
                    if (selectedItem_ > 0)
                        setSelectedItem(selectedItem_ - 1);
                }
                else if (mwe->getDirection() == MouseWheelEvent::TowardsUser)
                {
                    if (items_.size() && selectedItem_ < int(items_.size()) - 1)
                        setSelectedItem(selectedItem_ + 1);
                }

                return false;
            }

            // Eat mouse wheel events over the dropdown window
            if (getMouseoverItem() != -1)
                return false;
        }
    }

    return GUIWindow::processEvent(e);
}

void GUICombobox::clear()
{
    GUIWindow::clear();

    setTextAlignment(Font::AlignCenterLeft);

    items_.clear();
    selectedItem_ = -1;
    isExpanded_ = false;
}

bool GUICombobox::removeItem(unsigned int index)
{
    if (index >= items_.size())
        return false;

    // Erase the item
    items_.erase(index);

    // Update the selected item
    if (items_.empty())
        setSelectedItem(-1);
    else if (int(index) < selectedItem_)
        setSelectedItem(selectedItem_ - 1);
    else if (selectedItem_ == int(items_.size()))
        setSelectedItem(items_.size() - 1);
    else
        setSelectedItem(getSelectedItem());

    return true;
}

void GUICombobox::setItems(const Vector<UnicodeString>& items, int selectedItem)
{
    items_ = items;
    setSelectedItem(selectedItem);
}

bool GUICombobox::setSelectedItem(int index, bool fireEvent)
{
    if (index >= int(items_.size()) || index < -1)
        return false;

    if (selectedItem_ == index)
        return true;

    // Set the new item
    selectedItem_ = index;

    // Update the displayed text
    areLinesCurrent_ = false;
    if (selectedItem_ == -1)
        text_.clear();
    else
        text_ = items_[selectedItem_];

    // Fire select event
    if (fireEvent)
    {
        auto event = GUIComboboxItemSelectEvent(this, uint(selectedItem_));
        events().dispatchEvent(event);
        onItemSelectEvent.fire(event);
    }

    return true;
}

void GUICombobox::setText(const UnicodeString& text, bool fireEvent)
{
    // Search for an item that matches and select it if one is found
    auto index = items_.find(text);
    if (index != -1)
        setSelectedItem(uint(index), fireEvent);
}

bool GUICombobox::gatherGeometry(GeometryGather& gather)
{
    if (!GUIWindow::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        // The combobox can only be expanded when it has focus
        if (!hasFocus())
            isExpanded_ = false;

        // Get the width of the arrow box
        auto arrowBoxWidth = getArrowBoxWidth();
        auto width = getWidth();
        auto height = getHeight();

        // Draw the divider line and the arrow when there is no custom material on the combobox
        if (material_ == String::Empty)
        {
            gather.changePriority(getRenderPriority() + 1);
            gather.changeTransformation(getWorldTransform());

            gather.addImmediateTriangles(2);

            auto left = width - arrowBoxWidth - getBorderSize() * 2.0f;

            gather.addImmediateTriangle(Vec3(left, getBorderSize()), Vec3(left + getBorderSize(), getBorderSize()),
                                        Vec3(left, height - getBorderSize()), getBorderColor());
            gather.addImmediateTriangle(Vec3(left, height - getBorderSize()), Vec3(left + getBorderSize(), getBorderSize()),
                                        Vec3(left + getBorderSize(), height - getBorderSize()), getBorderColor());

            // Draw the arrow, use the highlight color when the mouse is in the window. The sides of the arrow are always at 45
            // degrees, so the height of the arrow is half its width
            gather.addImmediateTriangles(1);

            auto horizontalMargin = getBorderSize() + getTextMargins().getRight();
            auto arrowHeight = (arrowBoxWidth - horizontalMargin) * 0.5f;
            auto verticalMargin = (height - arrowHeight) * 0.5f;

            gather.addImmediateTriangle(
                Vec3(width - getBorderSize() - arrowBoxWidth + horizontalMargin, verticalMargin + arrowHeight),
                Vec3(width - getBorderSize() - arrowBoxWidth * 0.5f, verticalMargin),
                Vec3(width - getBorderSize() - horizontalMargin, verticalMargin + arrowHeight), getBorderColor());
        }

        // Draw the dropdown
        if (isExpanded_)
        {
            auto font = getFontToUse();
            auto fontSize = getFontSizeToUse(font);
            auto dropdownWidth = getDropdownWidth();
            auto dropdownHeight = getDropdownHeight();
            auto p = getDropdownPosition();

            // Draw the dropdown window
            gather.changePriority(getRenderPriority());
            gather.changeTransformation(localToWorld(p), getWorldOrientation());
            queueWindow(gather, dropdownWidth, dropdownHeight, getBorderSize(), getFillColor(), getBorderColor());

            // Store the item the mouse is over
            auto mouseoverItem = getMouseoverItem();

            for (auto i = 0U; i < items_.size(); i++)
            {
                auto itemY = p.y + getBorderSize() + (items_.size() - i - 1) * fontSize;

                // Draw a selection highlight on the item the mouse is over
                if (int(i) == mouseoverItem)
                {
                    auto& c = theme()["HighlightColor"];

                    gather.changePriority(getRenderPriority() + 1);
                    gather.changeTransformation(localToWorld(Vec3(p.x + getBorderSize(), itemY)), getWorldOrientation());
                    queueWindow(gather, dropdownWidth - getBorderSize() * 2.0f, fontSize, getBorderSize(), c, c);
                }

                // Work out the horizontal offset needed to align the text for this item
                auto xOffset = 0.0f;
                if (getTextAlignment() != Font::AlignTopLeft && getTextAlignment() != Font::AlignCenterLeft &&
                    getTextAlignment() != Font::AlignBottomLeft)
                {
                    xOffset = dropdownWidth - getTextMargins().getLeft() - getTextMargins().getRight() -
                        font->getWidth(items_[i], fontSize);

                    if (getTextAlignment() == Font::AlignCenterLeft || getTextAlignment() == Font::AlignCenter ||
                        getTextAlignment() == Font::AlignCenterRight)
                        xOffset *= 0.5f;
                }

                // Get the color to draw the text with. Highlight item text when the mouse is over it
                auto color = Color();
                if (useCustomTextColor_)
                    color = adjustColorAlpha(textColor_);
                else
                {
                    if (int(i) == mouseoverItem)
                        color = adjustColorAlpha(theme()["TextHighlightColor"]);
                    else
                        color = adjustColorAlpha(theme()["TextColor"]);
                }

                // Draw the item text
                gather.changePriority(getRenderPriority() + 2);
                queueText(gather, Vec2(p.x + getBorderSize() + getTextMargins().getLeft() + xOffset, itemY), items_[i], color);
            }
        }
    }

    return true;
}

float GUICombobox::getDropdownWidth() const
{
    // The width of the dropdown is the width of the widest item plus text margins The minimum is the width of the text area in
    // the combobox

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    auto dropdownWidth = getWidth() - getArrowBoxWidth() - getBorderSize();
    auto margins = getTextMargins().getLeft() + getTextMargins().getRight();

    for (auto& item : items_)
        dropdownWidth = std::max(dropdownWidth, font->getWidth(item, fontSize) + margins);

    return dropdownWidth;
}

float GUICombobox::getDropdownHeight() const
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    return items_.size() * fontSize + getBorderSize() * 2.0f;
}

Vec2 GUICombobox::getDropdownPosition() const
{
    // This method calculates where the dropdown should be put so that it can be easily seen. The placement options used in
    // order of preference are: below the combobox window, above the combobox window, on the right side of the combobox window,
    // on the left side of the combobox window. This code doesn't currently account for any rotation that might be on this
    // combobox.

    auto orthographicRect = getScene()->getDefaultCameraOrthographicRect();

    auto spaceBelow = getWorldPosition().y;
    auto spaceAbove = orthographicRect.getHeight() - spaceBelow - getHeight();
    auto dropdownWidth = getDropdownWidth();
    auto dropdownHeight = getDropdownHeight();

    auto position = Vec2();

    if (spaceBelow >= dropdownHeight)
        position.y = -dropdownHeight + getBorderSize();
    else if (spaceAbove >= dropdownHeight)
        position.y = getHeight() - getBorderSize();
    else
    {
        // The dropdown has to go to one side. If the right side has enough space then go there, otherwise use the side with the
        // most space

        auto spaceOnRight = orthographicRect.getWidth() - getWorldPosition().x - getWidth();
        auto spaceOnLeft = getWorldPosition().x;

        if (spaceOnRight >= dropdownWidth || spaceOnRight >= spaceOnLeft)
        {
            position.x = getWidth() - getBorderSize();
            position.y = -spaceBelow + (orthographicRect.getHeight() - dropdownHeight) * 0.5f;
        }
        else
        {
            position.x = -dropdownWidth + getBorderSize();
            position.y = -spaceBelow + (orthographicRect.getHeight() - dropdownHeight) * 0.5f;
        }
    }

    // If the dropdown is off the right side of the screen then align its right side with the right side of the combobox
    if (getWorldPosition().x + position.x + dropdownWidth > orthographicRect.getWidth())
        position.x = getWidth() - dropdownWidth;

    return position;
}

int GUICombobox::getMouseoverItem() const
{
    // This method calculates the index of the dropdown item that the mouse is currently over or -1 if the mouse is not
    // currently over any dropdown item

    if (!isExpanded_)
        return -1;

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    // Transform the mouse position into window space
    auto local = worldToLocal(Vec3(screenToWorld(platform().getMousePosition())));

    // Adjust for the position of the dropdown
    local -= getDropdownPosition();
    local.y -= getBorderSize();

    // See if the mouse is off the side or the bottom
    if (local.x < 0.0f || local.x > getDropdownWidth() || local.y < 0.0f)
        return -1;

    // Work out which item the mouse is over
    auto index = int(local.y / fontSize);
    if (index < 0 || index >= int(items_.size()))
        return -1;

    return items_.size() - index - 1;
}

float GUICombobox::getArrowBoxWidth() const
{
    return getHeight() - getBorderSize() * 2.0f;
}

void GUICombobox::updateLines()
{
    // Have one line which is the text of the selected item, and draw as many characters as will fit before running out of space

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    auto textMargins = getTextMarginsToUse();
    auto maxWidth = getWidth() - textMargins.getLeft() - textMargins.getRight();

    auto clippedText = UnicodeString();
    auto currentWidth = 0.0f;
    for (auto i = 0U; i < text_.length(); i++)
    {
        auto characterWidth = font->getWidth(text_.at(i), fontSize);
        if (currentWidth + characterWidth > maxWidth)
            break;

        clippedText.append(text_.at(i));
        currentWidth += characterWidth;
    }

    lines_.clear();
    lines_.append(clippedText);
    lines_.back().setVisible(true);
}

Rect GUICombobox::getTextMarginsToUse() const
{
    auto rect = GUIWindow::getTextMarginsToUse();

    rect.setRight(rect.getRight() + getArrowBoxWidth());

    return rect;
}

void GUICombobox::autosize()
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    setHeight(getTextMargins().getTop() + getTextMargins().getBottom() + fontSize);

    auto baseWidth = getArrowBoxWidth() + getTextMargins().getLeft() + getTextMargins().getRight();
    setWidth(baseWidth);

    if (items_.empty())
        return;

    // Make sure the combobox is wide enough to display its longest item
    for (auto& item : items_)
    {
        auto itemWidth = baseWidth + getFontToUse()->getWidth(item, fontSize);
        if (itemWidth > getWidth())
            setWidth(itemWidth);
    }
}

bool GUICombobox::intersect(const Vec2& position) const
{
    if (GUIWindow::intersect(position))
        return true;

    if (!isExpanded_)
        return false;

    // Check for intersection with the dropdown
    auto p = getDropdownPosition();

    return Rect(p.x, p.y, p.x + getDropdownWidth(), p.y + getDropdownHeight()).intersect(worldToLocal(position).toVec2());
}

void GUICombobox::save(FileWriter& file) const
{
    GUIWindow::save(file);

    file.beginVersionedSection(GUIComboboxVersionInfo);
    file.write(items_, selectedItem_);
    file.endVersionedSection();
}

void GUICombobox::load(FileReader& file)
{
    try
    {
        GUIWindow::load(file);

        file.beginVersionedSection(GUIComboboxVersionInfo);
        file.read(items_, selectedItem_);
        file.endVersionedSection();

        setSelectedItem(selectedItem_);
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

}
