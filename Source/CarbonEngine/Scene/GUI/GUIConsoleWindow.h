/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * GUIWindow subclass that provides an interface to the engine console.
 */
class CARBON_API GUIConsoleWindow : public GUIWindow
{
public:

    GUIConsoleWindow();
    ~GUIConsoleWindow() override;

    /**
     * Returns whether this console window is actively accepting input, which means it is visible, has focus, and has an
     * alpha that is greater than zero.
     */
    bool isActive() const { return hasFocus() && isVisible(); }

    bool isInteractive() const override { return true; }
    bool processEvent(const Event& e) override;

    void clear() override;
    bool gatherGeometry(GeometryGather& gather) override;

protected:

    void updateLines() override;

private:

    Vec2 cursorDrawPosition_;
    void updateCursorDrawPosition();
};

}
