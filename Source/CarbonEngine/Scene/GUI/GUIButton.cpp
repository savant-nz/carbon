/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Scene/GUI/GUIButton.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"

namespace Carbon
{

GUIButton::~GUIButton()
{
    onDestruct();
    clear();
}

Color GUIButton::getFillColor() const
{
    if (isToggle_ && isToggled_)
        return adjustColorAlpha(theme()["ButtonToggledFillColor"]);

    return GUIWindow::getFillColor();
}

Color GUIButton::getTextColor() const
{
    if (useCustomTextColor_)
        return adjustColorAlpha(textColor_);

    if (isMouseInWindow())
        return adjustColorAlpha(theme()["TextHighlightColor"]);

    return adjustColorAlpha(theme()["TextColor"]);
}

void GUIButton::clear()
{
    isToggle_ = false;
    isToggled_ = false;

    GUIWindow::clear();

    setTextAlignment(Font::AlignCenter);
}

void GUIButton::onBeforeGUIMouseButtonDownEvent(const GUIMouseButtonDownEvent& gmbde)
{
    if (gmbde.getButton() == LeftMouseButton)
    {
        if (isToggle_)
            isToggled_ = !isToggled_;
    }
}

}
