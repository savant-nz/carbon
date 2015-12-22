/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Base class for events that are sent through the EventManager class and can be handled by classes that register to receive
 * them. Actual Event subclasses derive from this class.
 */
class CARBON_API Event
{
public:

    Event() {}

    /**
     * Copy constructor.
     */
    Event(const Event& other) {}

    virtual ~Event() {}

    /**
     * Returns whether or not this event should be logged when event logging is enabled, this is on by default for all events
     * but some events that are sent every frame or would have problems with recursion can elect not to be shown in the event
     * log output.
     */
    virtual bool isLoggable() const { return true; }

    /**
     * When events are being logged this method is used to return more information about this event so that the logging output
     * is more useful.
     */
    virtual operator UnicodeString() const { return {}; }

    /**
     * Method template that is used to check the event type, null is returned if this event is not of the type `EventType`,
     * and if it is then a pointer to an `EventType` is returned. Internally this uses a `dynamic_cast`.
     */
    template <typename EventType> const EventType* as() const { return dynamic_cast<const EventType*>(this); }
};

}
