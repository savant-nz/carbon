/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Scripting/ScriptManager.h"

namespace Carbon
{

// Container namespace for the built-in scripting functions.
namespace ScriptFunctions
{

static void print(const String& s)
{
    LOG_DEBUG << s;
}

static void sleep(float seconds)
{
    if (seconds > 0.0f)
        scripts().suspend(scripts().getCurrentScript(), seconds);
}

static void suspend()
{
    scripts().suspend(scripts().getCurrentScript());
}

static void exit()
{
    events().dispatchEvent(ShutdownRequestEvent());
}

}

void ScriptManager::registerBuiltInFunctions()
{
    scripts().registerGlobalFunction("void print(const String& in)", ScriptFunctions::print);
    scripts().registerGlobalFunction("void sleep(float)", ScriptFunctions::sleep);
    scripts().registerGlobalFunction("void suspend()", ScriptFunctions::suspend);
    scripts().registerGlobalFunction("void exit()", ScriptFunctions::exit);
}

}
