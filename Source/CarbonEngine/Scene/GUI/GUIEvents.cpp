/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Scene/GUI/GUICombobox.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/GUI/GUISlider.h"
#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

const Scene* GUIEventDetails::getScene() const
{
    return window_ ? window_->getScene() : nullptr;
}

const String& GUIEventDetails::getWindowName() const
{
    return window_->getName();
}

GUIComboboxItemSelectEvent::GUIComboboxItemSelectEvent(GUICombobox* combobox, unsigned int item)
    : GUIEventDetails(combobox), item_(item)
{
}

const GUICombobox* GUIComboboxItemSelectEvent::getCombobox() const
{
    return static_cast<const GUICombobox*>(getWindow());
}

GUISliderChangedEvent::GUISliderChangedEvent(GUISlider* slider) : GUIEventDetails(slider)
{
}

GUISlider* GUISliderChangedEvent::getSlider() const
{
    return static_cast<GUISlider*>(getWindow());
}

}
