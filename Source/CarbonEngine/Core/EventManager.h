/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Implements a global event dispatch mechanism that allows classes to register handlers for specific Event subclasses and then
 * have their EventHandler::processEvent() implementation called whenever and Event of that type is sent. Classes do this by
 * inheriting from EventHandler, implementing EventHandler::processEvent(), and then calling EventManager::addHandler(). Events
 * are sent using EventManager::dispatchEvent().
 *
 * Events can only be received on the main thread, however they can be dispatched from worker threads through the
 * EventManager::dispatchEvent() and EventManager::queueEvent() methods. The former blocks the calling thread until the main
 * thread has dispatched the event, and the latter returns immediately but the event will not be dispatched until the start of
 * the next frame.
 */
class CARBON_API EventManager : private Noncopyable
{
public:

    /**
     * Controls whether or not events are logged with LOG_DEBUG() before they are dispatched, this is false by default but can
     * be enabled in order to analyze the flow of events for debugging purposes. Only events that return true from
     * Event::isLoggable() will be logged.
     */
    static bool LogEvents;

    /**
     * Registers a new handler for the specified event type. If \a first is true then it is put at the front of the handler
     * list, and if \a first is false then it is put at the end of the handler list for the event type. If the given handler is
     * already registered for this event then this method does nothing.
     */
    void addHandler(EventHandler* handler, unsigned int eventTypeID, bool first);

    /**
     * \copydoc EventManager::addHandler(EventHandler *, unsigned int, bool)
     */
    template <typename EventType> void addHandler(EventHandler* handler, bool first = false)
    {
        addHandler(handler, getEventTypeID<EventType>(), first);
    }

    /**
     * Removes the specified event handler from all events that it is currently registered for.
     */
    void removeHandler(EventHandler* handler);

    /**
     * Removes the specified event handler from the list of handlers for the specified event type.
     */
    void removeHandler(EventHandler* handler, unsigned int eventTypeID);

    /**
     * \copydoc EventManager::removeHandler(EventHandler*)
     */
    template <typename EventType> void removeHandler(EventHandler* handler)
    {
        removeHandler(handler, getEventTypeID<EventType>());
    }

    /**
     * Returns whether the specified event handler is registered for the specified event type.
     */
    bool isRegistered(EventHandler* handler, unsigned int eventTypeID) const;

    /**
     * Returns whether the specified event handler is registered for any event.
     */
    bool isRegistered(EventHandler* handler) const;

    /**
     * When called from the main thread this method sends the passed Event to all handlers that have registered to receive it.
     * If the passed Event has been disallowed using EventManager::setEventAllowed() then this method will do nothing. When
     * called from a thread that is not the main threadthe calling thread will block until the main thread is free to dispatch
     * the passed Event. The return value is true if all registered handlers for the passed event type were called, and false if
     * one of the registered handlers swallowed the event.
     */
    bool dispatchEvent(Event& e);

    /**
     * \copydoc EventManager::dispatchEvent(Event&)
     */
    template <typename T> bool dispatchEvent(T&& e)
    {
        auto& reference = e;
        return dispatchEvent(static_cast<Event&>(reference));
    }

    /**
     * Queues the passed Event instance for dispatch on the main thread at the start of the next frame. Once the main thread has
     * dispatched the passed event it will be deleted. This method is useful because it allows worker threads to send
     * notification events without waiting on the main thread to perform the actual dispatch (as happens when
     * EventManager::dispatchEvent() is called from a worker thread). The trade-off is that any result or data gathered during
     * the event dispatch will never be available to the worker thread.
     */
    void queueEvent(Event* event);

    /**
     * When EventManager::queueEvent() is called or EventManager::dispatchEvent() is called from a thread other than the main
     * thread, the passed Event instance is queued and dispatched on the main thread at the start of the next frame. This method
     * is responsible for running these dispatches and is called automatically by Application::mainLoop() every frame. It can
     * only ever be called from the main thread. The return value is the number of worker threads that are currently blocking in
     * a call to EventManager::dispatchEvent().
     */
    unsigned int dispatchQueuedEvents();

    /**
     * Returns whether the specified event type is currently allowed to be dispatched, all events are allowed by default but can
     * be blocked from being dispatched using the EventManager::setEventAllowed() method.
     */
    bool isEventAllowed(unsigned int eventTypeID) const;

    /**
     * Returns whether the specified event type is currently allowed to be dispatched, all events are allowed by default but can
     * be blocked from being dispatched using the EventManager::setEventAllowed() method.
     */
    template <typename EventType> bool isEventAllowed() const { return isEventAllowed(getEventTypeID<EventType>()); }

    /**
     * Sets whether the specified event type is allowed to be dispatched by the EventManager::dispatchEvent() method. By default
     * all event types are allowed to be dispatched.
     */
    void setEventAllowed(unsigned int eventTypeID, bool allowed);

    /**
     * \copydoc setEventAllowed(unsigned int, bool)
     */
    template <typename EventType> void setEventAllowed(bool allowed) { setEventAllowed(getEventTypeID<EventType>(), allowed); }

    /**
     * Returns the unique ID for the passed std::type_info, this is used to turn an Event subclass into a number.
     */
    unsigned int getEventTypeID(const std::type_info& typeInfo) const;

    /**
     * Returns the unique ID for the specified event type, this is used to turn an Event subclass type into a number.
     */
    template <typename EventType> unsigned int getEventTypeID() const { return getEventTypeID(typeid(EventType)); }

private:

    EventManager();
    ~EventManager();
    friend class Globals;

    class Members;
    Members* m = nullptr;

    static Vector<String> eventTypes_;
};

/**
 * \file
 */

/**
 * This macro contains boilerplate code that will cause the specified \a HandlerFunction function to be called whenever an event
 * of the specified \a EventClass is dispatched. This is useful as it handles setting up an EventHandler subclass automatically.
 */
#define CARBON_REGISTER_EVENT_HANDLER_FUNCTION(EventClass, HandlerFunction)                                           \
    CARBON_UNIQUE_NAMESPACE                                                                                           \
    {                                                                                                                 \
        static class Handler : public EventHandler                                                                    \
        {                                                                                                             \
        public:                                                                                                       \
            bool processEvent(const Event& e) override { return HandlerFunction(static_cast<const EventClass&>(e)); } \
        } eventHandler;                                                                                               \
        static void registerHandler() { Carbon::events().addHandler<EventClass>(&eventHandler); }                     \
        static void unregisterHandler() { Carbon::events().removeHandler<EventClass>(&eventHandler); }                \
        CARBON_REGISTER_STARTUP_FUNCTION(registerHandler, 0)                                                          \
        CARBON_REGISTER_SHUTDOWN_FUNCTION(unregisterHandler, 0)                                                       \
    }                                                                                                                 \
    CARBON_UNIQUE_NAMESPACE_END
}
