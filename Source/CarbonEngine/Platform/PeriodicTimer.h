/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventDelegate.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * This class provides a periodic timer that triggers its PeriodicTimer::onTimerEvent at a regular interval. Periodic timers can
 * be repeating or just fire once. PeriodicTimer::createSingle() can be used to easily create a single-use timer that only needs
 * to fire once after a certain interval. Note that periodic timer accuracy and performance is influenced by the update rate of
 * the main loop.
 */
class CARBON_API PeriodicTimer : public EventHandler, private Noncopyable
{
public:

    /**
     * Timer event dispatcher for this timer, fired whenever this timer triggers. The second parameter is the timeout value.
     */
    EventDispatcher<PeriodicTimer, TimeValue> onTimerEvent;

    /**
     * Constructor that sets up the timer parameters, the timer is not started automatically by this constructor,
     * PeriodicTimer::start() must be called to start the timer.
     */
    PeriodicTimer(TimeValue timeout, bool repeat);

    ~PeriodicTimer() override { clear(); }

    /**
     * Stops this timer and clears it to the default values.
     */
    void clear();

    /**
     * Starts this timer. If the timer is already running then this restarts it
     */
    void start();

    /**
     * Stops this timer if it is active. This method has no effect if this timer is inactive.
     */
    void stop();

    /**
     * Returns the timeout length of this timer in seconds.
     */
    TimeValue getTimeout() const { return timeout_; }

    /**
     * Sets the timeout length of this timer.
     */
    void setTimeout(TimeValue timeout) { timeout_ = timeout; }

    /**
     * Returns whether or not this timer will repeatedly fire, non-repeating timers only fire once.
     */
    bool getRepeat() const { return repeat_; }

    /**
     * Sets whether or not this timer will repeatedly fire, non-repeating timers only fire once.
     */
    void setRepeat(bool repeat) { repeat_ = repeat; }

    bool processEvent(const Event& e) override;

    /**
     * Creates a timer managed by the engine that will fire its event once after \a timeout has elapsed. The timer will
     * automatically delete itself after firing its one event.
     */
    static PeriodicTimer* createSingle(TimeValue timeout);

private:

    TimeValue timeout_;
    TimeValue lastFireTime_;

    bool repeat_ = false;
};

}
