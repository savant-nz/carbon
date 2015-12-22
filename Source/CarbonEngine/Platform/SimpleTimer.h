/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * This is a simple timer than can be started, stopped, reset, and queried for how much time has elapsed.
 */
class CARBON_API SimpleTimer
{
public:

    /**
     * Initializes this simple timer, by default it will be started immediately but this can be overridden if desired.
     */
    explicit SimpleTimer(bool startImmediately = true);

    /**
     * Starts this timer if it isn't already running.
     */
    void start();

    /**
     * Stops this timer if it is currently running.
     */
    void stop();

    /**
     * Stops this timer and resets its cumulative elapsed time to zero.
     */
    void reset();

    /**
     * Returns whether or not this timer is currently running.
     */
    bool isRunning() const { return startTime_.isSet(); }

    /**
     * Returns the amount of time that this timer has been active since it was last reset.
     */
    TimeValue getElapsedTime() const;

    /**
     * Returns this timer as a string formatted as "<elapsed milliseconds>ms".
     */
    operator UnicodeString() const;

private:

    TimeValue elapsedTime_;
    TimeValue startTime_;
};

}
