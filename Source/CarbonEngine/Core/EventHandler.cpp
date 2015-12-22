/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/EventManager.h"

namespace Carbon
{

EventHandler::~EventHandler()
{
#ifdef CARBON_DEBUG
    // Check that this event handler isn't still registered for events, if it is then assert to flag a bug
    if (Globals::isEngineInitialized())
    {
        if (events().isRegistered(this))
            assert(false && "The event handler being destructed is still registered for events, this is undefined behavior");
    }
#endif
}

}
