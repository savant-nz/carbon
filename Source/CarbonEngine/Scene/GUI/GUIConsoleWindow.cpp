/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUIConsoleWindow.h"

namespace Carbon
{

GUIConsoleWindow::GUIConsoleWindow()
{
    events().addHandler<ConsoleTextChangedEvent>(this);
    clear();
}

GUIConsoleWindow::~GUIConsoleWindow()
{
    onDestruct();
    clear();
    events().removeHandler(this);
}

void GUIConsoleWindow::clear()
{
    GUIWindow::clear();
    setTextAlignment(Font::AlignBottomLeft);
    setFont(fonts().getSystemMonospaceFont()->getName());
    setFontSize(0.0f);
    setWordWrapEnabled(false);
    processEvent(ConsoleTextChangedEvent());
}

void GUIConsoleWindow::updateLines()
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    auto textMargins = getTextMarginsToUse();
    auto height = getHeight() - textMargins.getBottom() - textMargins.getTop();
    auto visibleLines = uint(height / fontSize) - 1;
    auto initialLine = visibleLines < console().getHistorySize() ? console().getHistorySize() - visibleLines : 0;

    auto newText = UnicodeString();
    for (auto i = initialLine; i < console().getHistorySize(); i++)
        newText << console().getHistoryItem(i) << UnicodeString::Newline;

    newText << console().getPrompt() << console().getCurrentText();

    setText(newText);

    GUIWindow::updateLines();

    updateCursorDrawPosition();
}

void GUIConsoleWindow::updateCursorDrawPosition()
{
    if (lines_.empty())
        return;

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    cursorDrawPosition_ = lines_.back().getPosition();
    cursorDrawPosition_.x += font->getWidth(
        console().getPrompt() + lines_.back().getText().substr(0, console().getTextInput().getCursorPosition()), fontSize);
    cursorDrawPosition_.x -= font->getCharacterPreMove('|', fontSize);
}

bool GUIConsoleWindow::processEvent(const Event& e)
{
    if (e.as<ConsoleTextChangedEvent>())
        areLinesCurrent_ = false;

    if (isEnabled() && isActive())
    {
        if (auto kde = e.as<KeyDownEvent>())
            console().processKeyDownEvent(*kde);
        else if (auto cie = e.as<CharacterInputEvent>())
            console().processCharacterInputEvent(*cie);
    }

    return GUIWindow::processEvent(e);
}

bool GUIConsoleWindow::gatherGeometry(GeometryGather& gather)
{
    if (!GUIWindow::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        if (isActive() && lines_.size())
        {
            // Draw cursor
            if (console().getTextInput().isCursorOn() && lines_[0].isVisible())
            {
                gather.changePriority(getRenderPriority());
                queueText(gather, cursorDrawPosition_, "|", getTextColor());
            }
        }
    }

    return true;
}

}
