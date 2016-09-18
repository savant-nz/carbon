/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

namespace States
{

/**
 * Provides a single point of access for flushing cached states to the graphics interface, and for pushing and popping
 * all cached states.
 */
class CARBON_API StateCacher : private Noncopyable
{
public:

    /**
     * Sets up the state cacher, this is called following graphics interface setup.
     */
    static void setup();

    /**
     * Disables the specified state, flushing it will now be a no-op and will never result in a call through to the
     * graphics interface.
     */
    static void disable(CachedState& state);

    /**
     * Calls CachedState::setDirty(true) on all cached states.
     */
    static void setDirty();

    /**
     * Calls CachedState::flush() on all cached states.
     */
    static void flush();

    /**
     * Calls CachedState::push() on all cached states.
     */
    static void push();

    /**
     * Calls CachedState::pop() on all cached states.
     */
    static void pop();

    /**
     * Returns a list of all the cached states known to the state cacher.
     */
    static const Vector<CachedState*>& getCachedStates() { return allStates_; }

    /**
     * Returns a vector of strings containing a human-readable description of all current cached state values.
     */
    static Vector<UnicodeString> getCurrentState();

    /**
     * Calls CachedState::resetGraphicsInterfaceStateUpdateCount() on all cached states.
     */
    static void resetGraphicsInterfaceStateUpdateCount();

private:

    static Vector<CachedState*> allStates_;
    static Vector<CachedState*> activeStates_;
};

}

}
