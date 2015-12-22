/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * The TimeValue class is the basis of all time and duration manipulation, and forms the basis of the SimpleTimer, ScopedTimer
 * and PeriodicTimer classes. Internally the TimeValue class uses an integer tick count, but this is an implementation detail
 * and precise timing accuracy varies between platforms, however accuracy down to at least 1ms is guaranteed and most platforms
 * provide at least 0.1ms accuracy or better. The TimeValue for 'right now' is returned by PlatformInterface::getTime().
 * PlatformInterface::getTimePassed() and PlatformInterface::getSecondsPassed() are available for querying the amount of time
 * taken for the last frame.
 *
 * TimeValue instances can be directly manipulated with operations such as addition, subtraction, modulo and division, and can
 * also be converted to seconds with TimeValue::toSeconds() and milliseconds with TimeValue::toMilliseconds(). In order to
 * ensure a high level of accuracy all calculations that involve TimeValue instances should only convert into seconds as a last
 * step, and should do all calculations using TimeValue instances, as well as storing long-lived timing values in a TimeValue
 * instance.
 */
class CARBON_API TimeValue
{
public:

    TimeValue() {}

    /**
     * Constructs this time value with the given tick count.
     */
    explicit TimeValue(int64_t ticks) : ticks_(ticks) {}

    /**
     * Constructs this time value with the given number of seconds.
     */
    explicit TimeValue(float seconds) { *this = fromSeconds(seconds); }

    /**
     * Constructs this time value with the integer value from the given Parameter instance.
     */
    explicit TimeValue(const Parameter& parameter);

    /**
     * Clears this time value to zero, after calling this method the TimeValue::isSet() method will return false.
     */
    void clear() { ticks_ = 0; }

    /**
     * Returns whether this time value is currently set, i.e. its internal time value is non-zero.
     */
    bool isSet() const { return ticks_ != 0; }

    /**
     * Sets this time value to the specified number of seconds.
     */
    TimeValue& operator=(float seconds)
    {
        *this = fromSeconds(seconds);
        return *this;
    }

    /**
     * Adds another time value to this time value and returns the result.
     */
    TimeValue operator+(TimeValue other) const { return TimeValue(ticks_ + other.ticks_); }

    /**
     * Adds the specified number of seconds to this time value and returns the result.
     */
    TimeValue operator+(float seconds) const { return operator+(fromSeconds(seconds)); }

    /**
     * Subtracts another time value from this time value and returns the result, this can be used to work out time value deltas.
     */
    TimeValue operator-(TimeValue other) const { return TimeValue(ticks_ - other.ticks_); }

    /**
     * Subtracts the specified number of seconds from this time value and returns the result.
     */
    TimeValue operator-(float seconds) const { return operator-(fromSeconds(seconds)); }

    /**
     * Multiplies this time value by the given factor and returns the result.
     */
    TimeValue operator*(int64_t factor) const { return TimeValue(ticks_ * factor); }

    /**
     * Less-than comparison for time values.
     */
    bool operator<(TimeValue other) const { return ticks_ < other.ticks_; }

    /**
     * Less-than comparison against the given number of seconds.
     */
    bool operator<(float seconds) const { return operator<(TimeValue(seconds)); }

    /**
     * Less-than or equal comparison for time values.
     */
    bool operator<=(TimeValue other) const { return ticks_ <= other.ticks_; }

    /**
     * Less-than or equal comparison against the given number of seconds.
     */
    bool operator<=(float seconds) const { return operator<=(TimeValue(seconds)); }

    /**
     * Greater-than comparison for time values.
     */
    bool operator>(TimeValue other) const { return ticks_ > other.ticks_; }

    /**
     * Greater-than comparison against the given number of seconds.
     */
    bool operator>(float seconds) const { return operator>(TimeValue(seconds)); }

    /**
     * Greater-than or equal comparison for time values.
     */
    bool operator>=(TimeValue other) const { return ticks_ >= other.ticks_; }

    /**
     * Greater-than or equal comparison against the given number of seconds.
     */
    bool operator>=(float seconds) const { return operator>=(TimeValue(seconds)); }

    /**
     * In-place addition of time values.
     */
    TimeValue& operator+=(TimeValue other)
    {
        ticks_ += other.ticks_;
        return *this;
    }

    /**
     * Adds the specified number of seconds to this time value.
     */
    TimeValue& operator+=(float seconds) { return operator+=(fromSeconds(seconds)); }

    /**
     * In-place subtraction of time values.
     */
    TimeValue& operator-=(TimeValue other)
    {
        ticks_ -= other.ticks_;
        return *this;
    }

    /**
     * Subtracts the specified number of seconds from this time value.
     */
    TimeValue& operator-=(float seconds) { return operator-=(fromSeconds(seconds)); }

    /**
     * Divides this time value by the passed time value and returns the result as an integer.
     */
    int operator/(TimeValue other) const { return int(ticks_ / other.ticks_); }

    /**
     * Returns the remainder of this time value modulo another time.
     */
    TimeValue operator%(TimeValue other) const { return TimeValue(ticks_ % other.ticks_); }

    /**
     * Returns the normalized remainder of this time value under the given modulus in seconds, this is particularly useful in
     * conjunction with `platform().getTime()` for getting a value in range 0-1 for a given period. For example, the value of
     * `platform().getTime() % 2.2f` will go from 0 to 1 over a period of 2.2 seconds, and then wrap around back to zero and
     * repeat. This operator does not lose accuracy when internal tick counts are extremely large.
     */
    float operator%(float seconds) const
    {
        TimeValue s(seconds);

        return float(double(ticks_ % s.ticks_) / double(s.ticks_));
    }

    /**
     * Returns this time value in seconds.
     */
    float toSeconds() const
    {
        return float(ticks_ / ticksPerSecond_) + float(ticks_ % ticksPerSecond_) / float(ticksPerSecond_);
    }

    /**
     * Returns this time value in milliseconds.
     */
    float toMilliseconds() const { return toSeconds() * 1000.0f; }

    /**
     * Returns the amount of time that has passed since the time stored in this time value.
     */
    TimeValue getTimeSince() const;

    /**
     * Returns the number of seconds that have passed since this time value.
     */
    float getSecondsSince() const { return getTimeSince().toSeconds(); }

    /**
     * Returns this time value as a string containing the internal tick count.
     */
    operator UnicodeString() const { return ticks_; }

    /**
     * Returns this time value as a Parameter that stores the internal tick counter, this is used to store a TimeValue in a
     * ParameterArray.
     */
    operator Parameter() const;

    /**
     * Converts the passed number of seconds into a time value.
     */
    static TimeValue fromSeconds(float seconds) { return TimeValue(int64_t(double(ticksPerSecond_) * double(seconds))); }

    /**
     * Saves this time value to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const
    {
        file.write(double(ticks_ / ticksPerSecond_) + double(ticks_ % ticksPerSecond_) / double(ticksPerSecond_));
    }

    /**
     * Loads this time value from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file)
    {
        double seconds;
        file.read(seconds);

        ticks_ = int64_t(double(ticksPerSecond_) * seconds);
    }

private:

    int64_t ticks_ = 0;

    // The ticks-per-second value is set via the active PlatformInterface backend when it starts up
    friend class PlatformInterface;
    static int64_t ticksPerSecond_;
};

}
