/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Simple helper window type that follows the position of the mouse like a standard pointer or cursor. Only the x and y position
 * is affected, the z position is not touched, this enables the pointer to be placed at a specific z level. This entity also
 * defaults to a material of "MousePointer", which is a standard material resource. This can be changed with a call to
 * GUIMousePointer::setMaterial().
 */
class CARBON_API GUIMousePointer : public GUIWindow
{
public:

    GUIMousePointer() { clear(); }
    ~GUIMousePointer() override;

    /**
     * Returns the current position in local GUIWindow space which will be held at the position of the mouse pointer. Note that
     * this position is normalized in x and y such that 1.0 is equal to the width and height respectively. The default value is
     * (0, 1) which will keep the top left corner of this GUIMousePointer on the mouse position. Rotated mouse pointers are also
     * supported.
     */
    const Vec2& getLocalPointerOrigin() const;

    /**
     * Sets the position on this GUIMousePointer which will stay at the position of the mouse pointer. See
     * GUIMousePointer::getLocalPointerOrigin() for details.
     */
    void setLocalPointerOrigin(const Vec2& origin);

    void clear() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    void update() override;
    bool isPerFrameUpdateRequired() const override { return true; }

private:

    Vec2 localPointerOrigin_;
};

}
