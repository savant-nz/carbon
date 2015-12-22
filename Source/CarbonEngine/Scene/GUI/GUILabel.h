/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Label GUI item. A GUILabel does not draw the window rectangle border or fill unless a border or fill color has been
 * explicitly set. Text alignment defaults to Font::AlignCenterLeft.
 */
class CARBON_API GUILabel : public GUIWindow
{
public:

    GUILabel() { clear(); }
    ~GUILabel() override;

    Color getFillColor() const override;
    Color getBorderColor() const override;

    void clear() override;
};

}
