/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/SimpleTimer.h"

namespace Carbon
{

SimpleTimer::SimpleTimer(bool startImmediately)
{
    if (Globals::isEngineInitialized())
    {
        if (startImmediately)
            start();
    }
}

void SimpleTimer::start()
{
    if (startTime_.isSet())
        return;

    startTime_ = platform().getTime();
}

void SimpleTimer::stop()
{
    if (!startTime_.isSet())
        return;

    elapsedTime_ += startTime_.getTimeSince();
    startTime_.clear();
}

void SimpleTimer::reset()
{
    stop();

    elapsedTime_.clear();
}

TimeValue SimpleTimer::getElapsedTime() const
{
    auto elapsedTime = elapsedTime_;

    if (startTime_.isSet())
        elapsedTime += startTime_.getTimeSince();

    return elapsedTime;
}

SimpleTimer::operator UnicodeString() const
{
    return UnicodeString(int(getElapsedTime().toMilliseconds())) + "ms";
}

}
