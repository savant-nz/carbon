/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

const auto FrameTimersEnabledSetting = String("FrameTimersEnabled");

EventDispatcher<FrameTimers, TimeValue> FrameTimers::OnSamplingDataReady(nullptr);

bool FrameTimers::Enabled = false;
bool FrameTimers::areTimersActive_ = false;
Vector<FrameTimers::FrameTimer*> FrameTimers::timerStack_;
TimeValue FrameTimers::lastActivityTime_;

const unsigned int FrameTimers::HistorySize;

// Persist the frame timers enabled setting
CARBON_PERSISTENT_SETTING(FrameTimersEnabled, Boolean, FrameTimers::Enabled, false)

static Vector<FrameTimers::FrameTimer*>* registeredTimers;

// The registeredTimers vector is allocated on the first call to FrameTimers::createTimer() in order to avoid issues
// with the order of construction of static objects in different translation units. The RegisteredTimersVectorDeleter
// class is used to deallocate the vector when the library is uninitialized in order to avoid a memory leak.
class RegisteredTimersVectorDeleter
{
public:

    ~RegisteredTimersVectorDeleter()
    {
        if (registeredTimers)
        {
            for (auto registeredTimer : *registeredTimers)
                delete registeredTimer;

            delete registeredTimers;
            registeredTimers = nullptr;
        }
    }
};
static RegisteredTimersVectorDeleter registeredTimersVectorDeleter;

FrameTimers::FrameTimer* FrameTimers::createTimer(const String& name, const Color& color)
{
    if (!registeredTimers)
        registeredTimers = new Vector<FrameTimer*>;

    // Create new frame timer
    auto timer = new FrameTimer(name.withoutSuffix("Timer"), color);

    // Sort the registered timers list by name
    auto i = 0U;
    for (; i < registeredTimers->size(); i++)
    {
        if ((*registeredTimers)[i]->getName() > timer->getName())
            break;
    }
    registeredTimers->insert(i, timer);

    return timer;
}

const Vector<FrameTimers::FrameTimer*>& FrameTimers::getRegisteredTimers()
{
    return *registeredTimers;
}

void FrameTimers::push(FrameTimer* timer)
{
    assert(registeredTimers->has(timer) && "Invalid frame timer");

    if (areTimersActive_)
    {
        auto currentTime = platform().getTime();

        // If there is a currently running timer then update its accumulated time before pushing the new timer type onto
        // the stack
        if (!timerStack_.empty())
            timerStack_.back()->accumulatedTime_ += currentTime - lastActivityTime_;

        // Record when the push happened so that the next push/pop can calculate the time that this timer spent on the
        // top of the stack
        lastActivityTime_ = currentTime;
    }

    // Push the new timer onto the stack
    timerStack_.append(timer);
}

void FrameTimers::pop()
{
    auto currentTime = platform().getTime();

    if (areTimersActive_)
    {
        // Update the total time for the timer on the top of the stack and then pop it off
        timerStack_.back()->accumulatedTime_ += currentTime - lastActivityTime_;

        // Record when the pop happened so that the next push/pop can calculate the time that the new topmost timer
        // spent on the top of the stack
        lastActivityTime_ = currentTime;
    }

    timerStack_.popBack();

    // If the timer stack is empty then look at whether the current sampling period is up and if so then make new
    // sampling data available
    if (timerStack_.empty() && areTimersActive_)
    {
        static auto lastSummaryTime = TimeValue();

        // Check whether the current sampling period is over
        if (currentTime - lastSummaryTime < TimeValue(1.0f / ReportingFrequency))
            return;

        lastSummaryTime = currentTime;

        // Get the total amount time recorded on all timers
        auto totalTimeAccumulated = TimeValue();
        for (auto registeredTimer : *registeredTimers)
            totalTimeAccumulated += registeredTimer->accumulatedTime_;

        // Compute a fractional time for each timer and add it to the history
        for (auto registeredTimer : *registeredTimers)
        {
            std::rotate(registeredTimer->fractionHistory_.begin(), registeredTimer->fractionHistory_.end() - 1,
                        registeredTimer->fractionHistory_.end());

            registeredTimer->fractionHistory_[0] = registeredTimer->accumulatedTime_ % totalTimeAccumulated.toSeconds();

            registeredTimer->accumulatedTime_.clear();
        }

        OnSamplingDataReady.fire(currentTime);
    }

    // Propagate the Enabled state to the areTimersActive state when the timer count hits zero, this ensures that the
    // timer stack pushes and pops stay matched in the event that Enabled is changed when the timer stack isn't empty
    if (timerStack_.empty())
        areTimersActive_ = Enabled;
}

}
