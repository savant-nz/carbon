/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Base templated class that describes an interface for invoking an event delegate.
 */
template <typename Sender, typename EventData> class Delegate
{
public:

    virtual ~Delegate() {}

    /**
     * Calls the delegate method with the given sender reference and event data.
     */
    virtual bool invoke(Sender& sender, EventData data) const = 0;

    /**
     * Returns whether this delegate passes events to the given handler.
     */
    virtual bool hasHandler(void* handler) const = 0;
};

/**
 * Delegate subclass for calling a specific member function of a specific class type. This is specialized for void and bool
 * return types.
 */
template <typename Handler, typename Return, typename Sender, typename EventData> class EventDelegate;

/**
 * EventDelegate specialized for a void return value from the callback.
 */
template <typename Handler, typename Sender, typename EventData>
class EventDelegate<Handler, void, Sender, EventData> : public Delegate<Sender, EventData>
{
public:

    /**
     * Event delegate callback typedef that takes a reference to the object sending the event as well as the event's data.
     */
    typedef void (Handler::*CallbackMethod)(Sender&, EventData);

    /**
     * Initializes this event delegate for the given object and callback method
     */
    EventDelegate(Handler* handler, CallbackMethod callback) : handler_(handler), callback_(callback) {}

    /**
     * Returns the instance of the handler class that this delegate calls in EventDelegate::invoke().
     */
    Handler* getHandler() const { return handler_; }

    /**
     * Returns the instance method that this delegate calls in EventDelegate::invoke().
     */
    CallbackMethod getCallback() const { return callback_; }

    bool invoke(Sender& sender, EventData data) const override
    {
        (handler_->*callback_)(sender, data);
        return true;
    }

    bool hasHandler(void* handler) const override { return handler == reinterpret_cast<void*>(handler_); }

private:

    Handler* const handler_ = nullptr;
    const CallbackMethod callback_ = nullptr;
};

/**
 * EventDelegate specialized for a bool return value from the callback.
 */
template <typename Handler, typename Sender, typename EventData>
class EventDelegate<Handler, bool, Sender, EventData> : public Delegate<Sender, EventData>
{
public:

    /**
     * Event delegate callback typedef that takes a reference to the object sending the event as well as the event's data.
     */
    typedef bool (Handler::*CallbackMethod)(Sender&, EventData);

    /**
     * Initializes this event delegate for the given object and instance method
     */
    EventDelegate(Handler* handler, CallbackMethod callback) : handler_(handler), callback_(callback) {}

    /**
     * Returns the instance of the handler class that this delegate calls in EventDelegate::invoke().
     */
    Handler* getHandler() const { return handler_; }

    /**
     * Returns the instance method that this delegate calls in EventDelegate::invoke().
     */
    CallbackMethod getCallback() const { return callback_; }

    bool invoke(Sender& sender, EventData data) const override { return (handler_->*callback_)(sender, data); }

    bool hasHandler(void* handler) const override { return handler == reinterpret_cast<void*>(handler_); }

private:

    Handler* const handler_ = nullptr;
    const CallbackMethod callback_ = nullptr;
};

/**
 * Base class for EventDispatcher that is needed to allow a handler to be removed from all existing EventDispatcher instances in
 * one call. To do this the handler pointer is treated as a void * rather than a typed value, see the implementation of
 * EventDispatcher::remove() for more details.
 */
class EventDispatcherBase
{
public:

    EventDispatcherBase();
    virtual ~EventDispatcherBase();

    /**
     * Calls EventDispatcher::removeHandler() on all EventDispatcher instances.
     */
    static void removeHandlerFromAllEventDispatchers(void* handler);

protected:

    /**
     * Removes the given handler, implemented in EventDispatcher.
     */
    virtual void remove(void* handler) = 0;
};

/**
 * The global event system implemented by the Event, EventHandler and EventManager classes isn't well-suited to events that have
 * an originating object such as GUI events which occur on a specific window. In such cases a delegate system is more
 * appropriate, and this is provided by the EventDispatcher class. An object can define multiple instance-specific events and
 * then interested parties can register their own instance methods using EventDispatcher::addHandler() to be run when the event
 * is fired on that specific object by a call to EventDispatcher::fire().
 */
template <typename Sender, typename EventData> class EventDispatcher : public EventDispatcherBase, private Noncopyable
{
public:

    /**
     * Initializes this EventDispatcher to work with the given sender instance.
     */
    EventDispatcher(Sender* sender) : sender_(sender) {}

    ~EventDispatcher() override { clear(); }

    /**
     * Removes all registered handlers from this event.
     */
    void clear()
    {
        if (isFiring_)
            pendingUnhandles_ = delegates_;
        else
        {
            for (auto delegate : delegates_)
            {
                delegate->~DelegateType();
                MemoryInterceptor::free(delegate);
            }
            delegates_.clear();
        }
    }

    /**
     * Fires this event with the given event data, this calls all the delegates registered with EventDelegate::addHandler().
     */
    void fire(EventData data) const
    {
        // Don't allow nested fires
        if (isFiring_)
        {
            LOG_ERROR << "Nested firing of delegate events is not allowed";
            return;
        }

        // The firing state is tracked so that calls to EventDispatcher::removeHandler() while firing can be queued
        isFiring_ = true;

        // Store the size so that any calls to EventDispatcher::addHandler() made during these calls to EventDelegate::invoke()
        // don't have their new handlers called straight away for this event
        auto size = delegates_.size();

        for (auto i = 0U; i < size; i++)
        {
            if (!pendingUnhandles_.has(delegates_[i]))
            {
                if (!delegates_[i]->invoke(*sender_, data))
                    break;
            }
        }

        isFiring_ = false;

        // Process any pending removal of handlers that were queued while the delegates were being invoked
        for (auto pendingUnhandle : pendingUnhandles_)
        {
            if (delegates_.eraseValue(pendingUnhandle))
            {
                pendingUnhandle->~DelegateType();
                MemoryInterceptor::free(pendingUnhandle);
            }
        }

        pendingUnhandles_.clear();
    }

    /**
     * Calls EventDispatcher::fire() with an `EventData` instance constructed from the passed arguments.
     */
    template <typename... ArgTypes> void fireWith(ArgTypes&&... args) const
    {
        typename std::remove_reference<EventData>::type eventData(std::forward<ArgTypes>(args)...);

        fire(eventData);
    }

    /**
     * Registers a delegate callback method that will be invoked when this event is fired.
     */
    template <typename Handler, typename Return>
    void addHandler(Handler* handler, Return (Handler::*callback)(Sender&, EventData), bool first = false)
    {
        removeHandler(handler, callback);

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
        MemoryInterceptor::start(__FILE__, __LINE__);
#endif

        typedef EventDelegate<Handler, Return, Sender, EventData> EventDelegateType;

        auto delegate = MemoryInterceptor::allocate<EventDelegateType>();
        placement_new<EventDelegateType>(delegate, handler, callback);

        if (first)
            delegates_.prepend(delegate);
        else
            delegates_.append(delegate);
    }

    /**
     * Removes a delegate for this event that was added with the EventDispatcher::addHandler() method. Returns success flag.
     */
    template <typename Handler, typename Return>
    bool removeHandler(Handler* handler, Return (Handler::*callback)(Sender&, EventData))
    {
        typedef EventDelegate<Handler, Return, Sender, EventData> EventDelegateType;

        // Search for a registered delegate with the given instance and callback
        for (auto d : delegates_)
        {
            auto delegate = dynamic_cast<EventDelegateType*>(d);

            if (delegate && delegate->getHandler() == handler && delegate->getCallback() == callback)
            {
                if (isFiring_)
                {
                    if (!pendingUnhandles_.has(delegate))
                        pendingUnhandles_.append(delegate);
                }
                else
                {
                    delegates_.eraseValue(delegate);
                    delegate->~EventDelegateType();
                    MemoryInterceptor::free(delegate);
                }

                return true;
            }
        }

        return false;
    }

    /**
     * Removes the given handler instance from all events it is currently registered for regardless of the event type or class
     * method being used to handle each event.
     */
    template <typename Handler> static void removeFromAllEvents(Handler* handler)
    {
        EventDispatcherBase::removeHandlerFromAllEventDispatchers(handler);
    }

private:

    typedef Delegate<Sender, EventData> DelegateType;

    Sender* const sender_ = nullptr;

    mutable bool isFiring_ = false;

    mutable Vector<DelegateType*> delegates_;
    mutable Vector<DelegateType*> pendingUnhandles_;

    void remove(void* handler) override
    {
        for (auto delegate : delegates_)
        {
            if (delegate->hasHandler(handler))
            {
                if (isFiring_)
                    pendingUnhandles_.append(delegate);
                else
                {
                    delegates_.eraseValue(delegate);
                    delegate->~DelegateType();
                    MemoryInterceptor::free(delegate);
                    return;
                }
            }
        }
    }
};

}
