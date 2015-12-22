/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Button GUI item. A GUIButton's text highlights when the mouse is over it. Text alignment defaults to Font::AlignCenter.
 */
class CARBON_API GUIButton : public GUIWindow
{
public:

    GUIButton() { clear(); }
    ~GUIButton() override;

    /**
     * Returns whether this button is a toggle button. Defaults to false.
     */
    bool isToggle() const { return isToggle_; }

    /**
     * Sets whether this button is a toggle button. Defaults to false.
     */
    void setToggle(bool isToggle) { isToggle_ = isToggle; }

    /**
     * If this button is a toggle button then this returns the current toggled state.
     */
    bool isToggled() const { return isToggled_; }

    /**
     * If this button is a toggle button then this sets the current toggled state.
     */
    void setToggled(bool isToggled) { isToggled_ = isToggled; }

    bool isInteractive() const override { return true; }
    Color getFillColor() const override;
    Color getTextColor() const override;

    void clear() override;

protected:

    void onBeforeGUIMouseButtonDownEvent(const GUIMouseButtonDownEvent& gmbde) override;

private:

    bool isToggle_ = false;
    bool isToggled_ = false;
};

}
