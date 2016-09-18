/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventDelegate.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * Collates timing information for a set of frame timers and reports information about the fraction of processing time
 * consumed by each one. Timers are created by FrameTimers::createTimer() or the CARBON_DEFINE_FRAME_TIMER() macro.
 * During execution timers are managed using a stack, the total time consumed by a given timer is the total amount of
 * time it spent at the top of the timer stack. This means that pushing a new timer onto the stack stops the timing for
 * the previous timer that was at the top of the stack and starts it for the new timer. Timing results are accumulated
 * and made available at a frequency determined by FrameTimers::ReportingFrequency.
 */
class CARBON_API FrameTimers : private Noncopyable
{
public:

    /**
     * Specifies whether frame timers are enabled, this sets whether timer sampling should be done. Defaults to false.
     * This value is automatically persisted across executions of an application.
     */
    static bool Enabled;

    /**
     * The frequency with which new timing results will be made available by OnSamplingDataReady. The default is 3.0
     * which means new sampling data will be available 3 times every second. The timers are accumulated between each
     * sampling timestep and timer fractions are reported based on the entire elapsed time which helps smooth out
     * results and reduce errors caused by any timer aliasing that may be present.
     */
    static constexpr auto ReportingFrequency = 3.0f;

    /**
     * Returns the number of past results to keep around for each timer, currently this is set to 30. Timing results for
     * each individual timer can be retrieved using FrameTimers::FrameTimer::getHistoryEntry().
     */
    static const auto HistorySize = 30U;

    /**
     * This event is thrown every time a new set of sampling data becomes available. The associated data is the current
     * time. The sender for this event is undefined and should not be referenced.
     */
    static EventDispatcher<FrameTimers, TimeValue> OnSamplingDataReady;

    /**
     * Main frame timer object, these are created by FrameTimers::createTimer() and the list of all timers can be
     * retrieved using FrameTimers::getRegisteredTimers().
     */
    class FrameTimer
    {
    public:

        /**
         * Returns the name of this frame timer, this value was passed to FrameTimers::createTimer().
         */
        const String& getName() const { return name_; }

        /**
         * Returns the color to use when rendering this frame timer in a debug view, this value was passed to
         * FrameTimers::createTimer().
         */
        const Color& getColor() const { return color_; }

        /**
         * Returns the fraction of processing time used by this timer in a previous sampling period. The fraction at
         * index zero is the most recent sampling result. There will always be FrameTimers::HistorySize entries.
         */
        float getHistoryEntry(unsigned int index) const { return fractionHistory_[index]; }

    private:

        // The name of this timer
        const String name_;

        // The color to use when rendering this timer
        const Color color_;

        // The time accumulated for this timer since the beginning of the current sampling period
        TimeValue accumulatedTime_;

        // Fraction history for this timer
        std::array<float, HistorySize> fractionHistory_ = {};

        FrameTimer(String name, const Color& color) : name_(std::move(name)), color_(color) {}

        friend class FrameTimers;
    };

    /**
     * Creates and returns a new frame timer object with the given name and color. The name and color are used when
     * rendering the timer for debugging purposes and so should be unique for each timer to avoid confusion.
     */
    static FrameTimer* createTimer(const String& name, const Color& color);

    /**
     * Returns a list containing all the frame timers.
     */
    static const Vector<FrameTimer*>& getRegisteredTimers();

    /**
     * Pushes the given frame timer onto the top of the timer stack.
     */
    static void push(FrameTimer* timer);

    /**
     * Pops the current frame timer off the top of the stack.
     */
    static void pop();

private:

    static Vector<FrameTimer*> timerStack_;

    // The time at the last push or pop
    static TimeValue lastActivityTime_;

    // Changes to FrameTimers::Enabled don't take effect until the timer stack is next empty, the areTimersActive_ value
    // is the internal timer enabled/disabled toggle that is set to FrameTimers::Enabled whenever the timer stack
    // becomes empty. This system makes it safe to change the value of FrameTimers::Enabled at any time regardless of
    // what timers may be currently on the stack. The timer stack is still maintained when the frame timers are disabled
    // but no timing information is gathered.
    static bool areTimersActive_;

    FrameTimers();
};

/**
 * This is a helper class that pushes a frame timer onto the top of the timer stack in its constructor and pops it off
 * in its destructor. Useful for ensuring a 1:1 matching of pushes and pops.
 */
class CARBON_API ScopedFrameTimer : private Noncopyable
{
public:

    /**
     * Constructor that calls FrameTimers::push() with the passed timer, this is matched by a FrameTimers::pop() call in
     * the destructor.
     */
    ScopedFrameTimer(FrameTimers::FrameTimer* timer) : timer_(timer) { FrameTimers::push(timer); }

    /**
     * Move constructor.
     */
    ScopedFrameTimer(ScopedFrameTimer&& other) : timer_(other.timer_) { other.timer_ = nullptr; }

    ~ScopedFrameTimer()
    {
        if (timer_)
            FrameTimers::pop();
    }

private:

    FrameTimers::FrameTimer* timer_ = nullptr;
};

/**
 * Defines a frame timer with the given name and color, frame timers have the type FrameTimers::FrameTimer. This macro
 * makes it simpler to define a frame timer that can then be used at runtime: 'CARBON_DEFINE_FRAME_TIMER(RendererTimer,
 * Color::Red)'.
 */
#define CARBON_DEFINE_FRAME_TIMER(TimerName, TimerColor) \
    static auto TimerName = Carbon::FrameTimers::createTimer(#TimerName, TimerColor);
}
