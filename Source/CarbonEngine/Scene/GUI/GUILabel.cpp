/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Scene/GUI/GUILabel.h"

namespace Carbon
{

GUILabel::~GUILabel()
{
    onDestruct();
    clear();
}

void GUILabel::clear()
{
    GUIWindow::clear();

    setTextAlignment(Font::AlignCenterLeft);
}

Color GUILabel::getFillColor() const
{
    return adjustColorAlpha(useCustomFillColor_ ? fillColor_ : Color::Zero);
}

Color GUILabel::getBorderColor() const
{
    return adjustColorAlpha(useCustomBorderColor_ ? borderColor_ : Color::Zero);
}

}
