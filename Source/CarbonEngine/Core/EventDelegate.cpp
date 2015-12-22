/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventDelegate.h"

namespace Carbon
{

static std::unordered_set<EventDispatcherBase*>* allEventDispatchers;

EventDispatcherBase::EventDispatcherBase()
{
    if (!allEventDispatchers)
        allEventDispatchers = new std::unordered_set<EventDispatcherBase*>;

    allEventDispatchers->insert(this);
}

EventDispatcherBase::~EventDispatcherBase()
{
    allEventDispatchers->erase(this);

    if (allEventDispatchers->empty())
    {
        delete allEventDispatchers;
        allEventDispatchers = nullptr;
    }
}

void EventDispatcherBase::removeHandlerFromAllEventDispatchers(void* handler)
{
    if (allEventDispatchers)
    {
        for (auto e : *allEventDispatchers)
            e->remove(handler);
    }
}

}
