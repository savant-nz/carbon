/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Holds state of a game controller input device such as a joystick.
 */
class CARBON_API GameControllerState
{
public:

    /**
     * An empty instance of the controller state class.
     */
    static const GameControllerState Empty;

    /**
     * The current XYZ axis positions of this game controller, each in the range -1.0 to 1.0. X is typically left-right
     * movement, Y is typically backwards-forwards movement, and Z is typically a throttle.
     */
    Vec3 axisPosition;

    /**
     * Button pressed state for every button on this game controller.
     */
    Vector<bool> isButtonPressed;

    GameControllerState() {}
};

}
