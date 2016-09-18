/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUIEditbox.h"

namespace Carbon
{

const UnicodeString GUIEditbox::PasswordCharacter = "*";

GUIEditbox::GUIEditbox() : textInput_(text_)
{
    clear();
}

GUIEditbox::~GUIEditbox()
{
    onDestruct();
    clear();
}

bool GUIEditbox::isInteractive() const
{
    return true;
}

Color GUIEditbox::getTextColor() const
{
    if (useCustomTextColor_)
        return adjustColorAlpha(textColor_);

    if (isEnabled() && (hasFocus() || isMouseInWindow()))
        return adjustColorAlpha(theme()["TextHighlightColor"]);

    return adjustColorAlpha(theme()["TextColor"]);
}

void GUIEditbox::setText(const UnicodeString& text)
{
    GUIWindow::setText(text);
    textInput_.setCursorPosition(text.length());
}

bool GUIEditbox::processEvent(const Event& e)
{
    if (isEnabled() && isVisibleIgnoreAlpha() && hasFocus())
    {
        if (auto kde = e.as<KeyDownEvent>())
        {
            if (textInput_.onKeyDownEvent(*kde))
                areLinesCurrent_ = false;
        }
        else if (auto cie = e.as<CharacterInputEvent>())
        {
            if (textInput_.onCharacterInputEvent(*cie))
                areLinesCurrent_ = false;
        }
        else if (auto mbde = e.as<MouseButtonDownEvent>())
        {
            // A click in an editbox sets the position of the cursor
            if (isMouseInWindow() && mbde->getButton() == LeftMouseButton)
            {
                setCursorPositionFromLocalPoint(worldToLocal(screenToWorld(mbde->getPosition())).toVec2());
                textInput_.restartCursorBlink();
            }
        }
    }

    return GUIWindow::processEvent(e);
}

void GUIEditbox::clear()
{
    characterOffset_ = 0;
    isPasswordEditbox_ = false;

    GUIWindow::clear();

    setTextAlignment(Font::AlignCenterLeft);
}

void GUIEditbox::updateLines()
{
    // Draw the contents of the editbox as one line, no multiline editing is supported. Calculate and use the character
    // offset to know which character to start on, then add as many as will fit inside the margins.

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);
    auto textMargins = getTextMarginsToUse();
    auto maxWidth = getWidth() - textMargins.getLeft() - textMargins.getRight();

    calculateCharacterOffset();

    auto clippedText = UnicodeString();
    auto currentWidth = 0.0f;
    for (auto i = characterOffset_; i < text_.length(); i++)
    {
        auto c = isPasswordEditbox_ ? PasswordCharacter.at(0) : text_.at(i);
        auto characterWidth = font->getWidth(c, fontSize);

        if (currentWidth + characterWidth > maxWidth)
            break;

        clippedText.append(c);
        currentWidth += characterWidth;
    }

    lines_.clear();
    lines_.append(clippedText);
    lines_.back().setVisible(true);

    updateCursorDrawPosition();
}

bool GUIEditbox::gatherGeometry(GeometryGather& gather)
{
    if (!GUIWindow::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        if (hasFocus() && textInput_.isCursorOn() && lines_[0].isVisible())
        {
            gather.changePriority(getRenderPriority());

            // Draw cursor
            queueText(gather, cursorDrawPosition_, "|", getTextColor());
        }
    }

    return true;
}

void GUIEditbox::calculateCharacterOffset()
{
    // The editbox scrolls horizontally when its content does not all fit in. This method works out the correct
    // character offset to use for the scrolling based on the cursor position. The character offset is the index of the
    // first character of the text_ string to be drawn when rendering the editbox.

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);
    auto maxWidth = getWidth() - getTextMarginsToUse().getLeft() - getTextMarginsToUse().getRight();

    // If the cursor is off the left side of the window adjust the character offset so it becomes visible.
    if (characterOffset_ > textInput_.getCursorPosition())
    {
        characterOffset_ = textInput_.getCursorPosition();

        // The standard behavior of an editbox when moving the cursor off the left is to jump back a certain number of
        // characters. So this code jumps back either by 8 characters or roughly half the width of the editbox,
        // whichever is smaller.
        auto jump = uint((maxWidth * 0.5f) / font->getMaximumCharacterWidth(fontSize));
        if (jump > 8)
            jump = 8;

        if (characterOffset_ < jump)
            characterOffset_ = 0;
        else
            characterOffset_ -= jump;

        return;
    }

    // Loop round increasing the character offset until the cursor is visible
    while (true)
    {
        if (isPasswordEditbox_)
        {
            if (font->getWidth(PasswordCharacter, fontSize) * (textInput_.getCursorPosition() - characterOffset_) <=
                maxWidth)
                break;
        }
        else
        {
            auto s = text_.substr(characterOffset_, textInput_.getCursorPosition() - characterOffset_);
            if (font->getWidth(s, fontSize) <= maxWidth)
                break;
        }

        if (characterOffset_ == text_.length() - 1)
            break;

        characterOffset_++;
    }
}

void GUIEditbox::setCursorPositionFromLocalPoint(const Vec2& p)
{
    // This method translates a mouse click position in the editbox into the new position for the cursor

    if (lines_.empty())
        return;

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);
    const auto& line = lines_[0];

    areLinesCurrent_ = false;

    textInput_.setCursorPosition(characterOffset_);

    auto offset = p.x - line.getPosition().x;
    auto total = 0.0f;
    for (auto i = 0U; i < line.getText().length(); i++)
    {
        auto c = isPasswordEditbox_ ? PasswordCharacter.at(0) : line.getText().at(i);
        auto halfCharacterWidth = font->getWidth(c, fontSize) * 0.5f;

        total += halfCharacterWidth;

        if (offset > total)
            textInput_.setCursorPosition(textInput_.getCursorPosition() + 1);
        else
            return;

        total += halfCharacterWidth;
    }
}

void GUIEditbox::updateCursorDrawPosition()
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    cursorDrawPosition_ = lines_[0].getPosition();
    cursorDrawPosition_.x -= font->getCharacterPreMove('|', fontSize);

    if (isPasswordEditbox_)
    {
        cursorDrawPosition_.x +=
            font->getWidth(PasswordCharacter, fontSize) * (textInput_.getCursorPosition() - characterOffset_);
    }
    else
    {
        auto s = text_.substr(characterOffset_, textInput_.getCursorPosition() - characterOffset_);
        cursorDrawPosition_.x += font->getWidth(s, fontSize);
    }
}

void GUIEditbox::setAllowedCharacters(const UnicodeString& characters)
{
    textInput_.setAllowedCharacters(characters);
}

void GUIEditbox::setPasswordEditbox(bool passwordEditbox)
{
    isPasswordEditbox_ = passwordEditbox;
    areLinesCurrent_ = false;
}

}
