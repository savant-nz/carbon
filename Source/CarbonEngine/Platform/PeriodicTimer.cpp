/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/PeriodicTimer.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

// internalTimers is a vector of all the currently active internal timer objects created by PeriodicTimer::createSingle(). These
// timers automatically delete themselves when they fire, and any that are yet to fire when the engine shuts down are deleted by
// the internalTimersDeleter() function in order to avoid leaking memory.
static auto internalTimers = Vector<PeriodicTimer*>();
static void internalTimersDeleter()
{
    for (auto internalTimer : internalTimers)
        delete internalTimer;

    internalTimers.clear();
}
CARBON_REGISTER_SHUTDOWN_FUNCTION(internalTimersDeleter, 0)

PeriodicTimer::PeriodicTimer(TimeValue timeout, bool repeat) : onTimerEvent(this)
{
    clear();

    setTimeout(timeout);
    setRepeat(repeat);
}

void PeriodicTimer::clear()
{
    setTimeout(TimeValue(1.0f));
    setRepeat(false);

    lastFireTime_.clear();

    events().removeHandler<UpdateEvent>(this);
}

void PeriodicTimer::start()
{
    lastFireTime_ = platform().getTime();
    events().addHandler<UpdateEvent>(this);
}

void PeriodicTimer::stop()
{
    events().removeHandler<UpdateEvent>(this);
}

bool PeriodicTimer::processEvent(const Event& e)
{
    if (e.as<UpdateEvent>())
    {
        // Work out how many timer events need to be sent based on how much time has elapsed since the last event was sent
        auto eventCount = lastFireTime_.getTimeSince() / timeout_;
        if (eventCount > 0)
        {
            lastFireTime_ += timeout_ * eventCount;

            // Fire the timer events
            while (eventCount--)
                onTimerEvent.fire(getTimeout());

            // If this timer is not set to repeat then disable the event handler
            if (!repeat_)
            {
                events().removeHandler<UpdateEvent>(this);

                // If this was an internal timer then delete it because it has now fired its single event
                if (internalTimers.unorderedEraseValue(this))
                    delete this;
            }
        }
    }

    return true;
}

PeriodicTimer* PeriodicTimer::createSingle(TimeValue timeout)
{
    auto timer = new PeriodicTimer(timeout, false);
    internalTimers.append(timer);

    timer->start();

    return timer;
}

}
