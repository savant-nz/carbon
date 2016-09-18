/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/Threads/Mutex.h"
#include "CarbonEngine/Core/Threads/Thread.h"

namespace Carbon
{

bool EventManager::LogEvents = false;
Vector<String> EventManager::eventTypes_;

class EventManager::Members
{
public:

    // The registered handlers for each event are managed by this helper class. Handlers registered as needing to
    // receive an event first before other handlers go in firstHandlers_, all other handlers go in normalHandlers_.
    class EventHandlerSet
    {
    public:

        void registerFirstHandler(EventHandler* handler) { firstHandlers_.insert(handler); }

        void registerLastHandler(EventHandler* handler) { normalHandlers_.insert(handler); }

        void removeHandler(EventHandler* handler)
        {
            firstHandlers_.erase(handler);
            normalHandlers_.erase(handler);
        }

        bool hasHandler(EventHandler* handler)
        {
            return firstHandlers_.find(handler) != firstHandlers_.end() ||
                normalHandlers_.find(handler) != normalHandlers_.end();
        }

        bool each(const std::function<bool(EventHandler* handler)>& predicate)
        {
            for (auto handler : firstHandlers_)
            {
                if (!predicate(handler))
                    return false;
            }

            for (auto handler : normalHandlers_)
            {
                if (!predicate(handler))
                    return false;
            }

            return true;
        }

    private:

        std::unordered_set<EventHandler*> firstHandlers_;
        std::unordered_set<EventHandler*> normalHandlers_;
    };

    // Events are referenced using a unique integer ID value that is can be looked up directly in this array to get the
    // set of handlers for the event. The sets are created on demand.
    std::vector<std::unique_ptr<EventHandlerSet>> eventHandlerSets;
    EventHandlerSet& getEventHandlerSet(unsigned int eventTypeID)
    {
        if (eventTypeID >= eventHandlerSets.size())
            eventHandlerSets.resize(eventTypeID + 1);

        if (!eventHandlerSets[eventTypeID])
            eventHandlerSets[eventTypeID].reset(new EventHandlerSet);

        return *eventHandlerSets[eventTypeID];
    }

    std::set<unsigned int> disallowedEvents;

    // Holds the type of all the events that are currently in the process of being dispatched
    Vector<unsigned int> currentEventStack;
    bool isEventBeingDispatched(unsigned int eventTypeID) { return currentEventStack.has(eventTypeID); }

    // Any alterations to currently registered event handlers are queued up until all the dispatches are finished
    struct PendingEventHandlerChange
    {
        enum ChangeType
        {
            RegisterFirstHandler,
            RegisterLastHandler,
            RemoveHandler
        } type;

        unsigned int eventTypeID = 0;
        EventHandler* handler = nullptr;

        PendingEventHandlerChange(ChangeType type_, unsigned int eventTypeID_, EventHandler* handler_)
            : type(type_), eventTypeID(eventTypeID_), handler(handler_)
        {
        }
    };
    Vector<PendingEventHandlerChange> pendingHandlerChanges;

    bool isHandlerAwaitingRemoval(const EventHandler* handler, unsigned int eventTypeID) const
    {
        auto result = false;

        for (const auto& change : pendingHandlerChanges)
        {
            if (change.handler == handler && change.eventTypeID == eventTypeID)
                result = (change.type == Members::PendingEventHandlerChange::RemoveHandler);
        }

        return result;
    }

    // Events dispatched by worker threads are queued in this vector then dispatched on the main thread, the boolean
    // value indicates whether the queued Event instance needs to be explicitly deleted once the event has been sent,
    // i.e. it was added via EventManager::queueEvent()
    struct QueuedEvent
    {
        Event* event = nullptr;
        bool deleteAfterDispatch = false;

        QueuedEvent() {}
        QueuedEvent(Event* event_, bool deleteAfterDispatch_) : event(event_), deleteAfterDispatch(deleteAfterDispatch_)
        {
        }
    };
    Vector<QueuedEvent> queuedEvents;

    // The number of worker threads currently blocked waiting for a call to EventManager::dispatchQueuedEvents() on the
    // main thread to occur
    unsigned int workerThreadsWaitingForDispatchCount = 0;

    // When an event queued in EventManager::dispatchEvent() by a worker thread is disptched on the main thread, the
    // return value from the call to EventManager::dispatchEvent() on the main thread is stored here for retrieval by
    // the worker thread that is waiting for the result
    std::unordered_map<Event*, bool> queuedEventDispatchResults;

    // This mutex is used to protect the above three members
    mutable Mutex queuedEventsMutex;
};

EventManager::EventManager()
{
    m = new Members;
}

EventManager::~EventManager()
{
    // Before shutting down, dispatch all queued events and wait for there to be no worker threads waiting on a dispatch
    // to complete
    while (true)
    {
        if (dispatchQueuedEvents() == 0)
            break;

        Thread::sleep(2);
    }

    delete m;
    m = nullptr;
}

void EventManager::addHandler(EventHandler* handler, unsigned int eventTypeID, bool first)
{
    assert(Thread::isRunningInMainThread());

    if (first)
    {
        if (m->isEventBeingDispatched(eventTypeID))
            m->pendingHandlerChanges.emplace(Members::PendingEventHandlerChange::RegisterFirstHandler, eventTypeID,
                                             handler);
        else if (!isRegistered(handler, eventTypeID))
            m->getEventHandlerSet(eventTypeID).registerFirstHandler(handler);
    }
    else
    {
        if (m->isEventBeingDispatched(eventTypeID))
            m->pendingHandlerChanges.emplace(Members::PendingEventHandlerChange::RegisterLastHandler, eventTypeID,
                                             handler);
        else if (!isRegistered(handler, eventTypeID))
            m->getEventHandlerSet(eventTypeID).registerLastHandler(handler);
    }
}

void EventManager::removeHandler(EventHandler* handler)
{
    assert(Thread::isRunningInMainThread());

    for (auto i = 0U; i < m->eventHandlerSets.size(); i++)
    {
        if (isRegistered(handler, i))
            removeHandler(handler, i);
    }
}

void EventManager::removeHandler(EventHandler* handler, unsigned int eventTypeID)
{
    assert(Thread::isRunningInMainThread());

    if (m->isEventBeingDispatched(eventTypeID))
        m->pendingHandlerChanges.emplace(Members::PendingEventHandlerChange::RemoveHandler, eventTypeID, handler);
    else
        m->getEventHandlerSet(eventTypeID).removeHandler(handler);
}

bool EventManager::isRegistered(EventHandler* handler, unsigned int eventTypeID) const
{
    assert(Thread::isRunningInMainThread());

    bool result = m->getEventHandlerSet(eventTypeID).hasHandler(handler);

    // Look for any pending event handler changes that affect the result of this method
    for (auto& change : m->pendingHandlerChanges)
    {
        if (change.handler == handler && change.eventTypeID == eventTypeID)
            result = (change.type != Members::PendingEventHandlerChange::RemoveHandler);
    }

    return result;
}

bool EventManager::isRegistered(EventHandler* handler) const
{
    assert(Thread::isRunningInMainThread());

    for (auto i = 0U; i < m->eventHandlerSets.size(); i++)
    {
        if (isRegistered(handler, i))
            return true;
    }

    return false;
}

bool EventManager::dispatchEvent(Event& e)
{
    // Dispatches that don't come from the main thread are queued for processing on the main thread at the start of the
    // next frame, the calling thread blocks waiting for the result of the dispatch
    if (!Thread::isRunningInMainThread())
    {
        auto queueEntry = Members::QueuedEvent(&e, false);

        // Queue the event for dispatch
        {
            auto lock = ScopedMutexLock(m->queuedEventsMutex);
            m->queuedEvents.append(queueEntry);
            m->workerThreadsWaitingForDispatchCount++;
        }

        // Wait until the event has been dispatched by the main thread, this is indicated by the event no longer being
        // in m->queuedEvents TODO: the synchronization here should really use a condition variable, but those aren't
        // part of the threading layer at present
        auto result = false;
        while (true)
        {
            Thread::sleep(2);

            auto lock = ScopedMutexLock(m->queuedEventsMutex);
            if (!m->queuedEvents.has([&](const Members::QueuedEvent& queuedEvent) { return queuedEvent.event == &e; }))
            {
                // The main thread has dispatched the event, get the result
                result = m->queuedEventDispatchResults[queueEntry.event];
                m->queuedEventDispatchResults.erase(queueEntry.event);

                m->workerThreadsWaitingForDispatchCount--;
                break;
            }
        }

        return result;
    }

    const auto eventTypeID = getEventTypeID(typeid(e));

    // Log this event if event logging is on and the event is loggable
    if (LogEvents && e.isLoggable())
    {
        auto stack = UnicodeString();
        for (auto currentEventID : m->currentEventStack)
            stack << eventTypes_[currentEventID - 1] << " => ";

        stack << typeid(e).name();

        // If there is additional information available for this event then tag it on the end
        auto eventDescription = UnicodeString(e);
        if (eventDescription.length())
            stack << " - " << eventDescription;

        if (isEventAllowed(eventTypeID))
            LOG_DEBUG << stack;
        else
            LOG_DEBUG << stack << " (DISALLOWED)";
    }

    if (!isEventAllowed(eventTypeID))
        return false;

    m->currentEventStack.append(eventTypeID);

    auto& eventHandlerSet = m->getEventHandlerSet(eventTypeID);

    // Call the event handlers, each one is checked that it doesn't have a removal pending before it is called
    auto result = eventHandlerSet.each([&](EventHandler* handler) {
        return m->isHandlerAwaitingRemoval(handler, eventTypeID) || handler->processEvent(e);
    });

    m->currentEventStack.popBack();

    // If the event stack is now empty then we can go ahead and clear the list of pending event handler changes
    if (m->currentEventStack.empty())
    {
        for (const auto& change : m->pendingHandlerChanges)
        {
            if (change.type == Members::PendingEventHandlerChange::RemoveHandler)
                removeHandler(change.handler, change.eventTypeID);
            else
            {
                auto& handlers = m->getEventHandlerSet(change.eventTypeID);

                if (change.type == Members::PendingEventHandlerChange::RegisterFirstHandler)
                {
                    if (!handlers.hasHandler(change.handler))
                        handlers.registerFirstHandler(change.handler);
                }
                else if (change.type == Members::PendingEventHandlerChange::RegisterLastHandler)
                {
                    if (!handlers.hasHandler(change.handler))
                        handlers.registerLastHandler(change.handler);
                }
            }
        }

        m->pendingHandlerChanges.clear();
    }

    return result;
}

void EventManager::queueEvent(Event* event)
{
    if (!event)
        return;

    auto lock = ScopedMutexLock(m->queuedEventsMutex);
    m->queuedEvents.emplace(event, true);
}

unsigned int EventManager::dispatchQueuedEvents()
{
    assert(Thread::isRunningInMainThread() && "Queued events can only be dispatched from the main thread");

    // Dispatch all queued events
    auto lock = ScopedMutexLock(m->queuedEventsMutex);
    for (auto& queuedEvent : m->queuedEvents)
    {
        auto result = events().dispatchEvent(*queuedEvent.event);

        if (queuedEvent.deleteAfterDispatch)
        {
            // This event came through EventManager::queueEvent() so the result is discarded and the Event instance is
            // deleted now that it has been dispatched
            delete queuedEvent.event;
        }
        else
        {
            // This event came via a call to EventManager::dispatchEvent() on a worker thread, which means there is
            // currently a thread waiting for this dispatch to be completed, the result of the dispatch goes into
            // m->queuedEventDispatchResults
            m->queuedEventDispatchResults[queuedEvent.event] = result;
        }
    }

    m->queuedEvents.clear();

    return m->workerThreadsWaitingForDispatchCount;
}

bool EventManager::isEventAllowed(unsigned int eventTypeID) const
{
    assert(Thread::isRunningInMainThread());

    return eventTypeID != 0 && m->disallowedEvents.find(eventTypeID) == m->disallowedEvents.end();
}

void EventManager::setEventAllowed(unsigned int eventTypeID, bool allowed)
{
    assert(Thread::isRunningInMainThread());

    if (allowed)
        m->disallowedEvents.erase(eventTypeID);
    else
        m->disallowedEvents.insert(eventTypeID);
}

unsigned int EventManager::getEventTypeID(const std::type_info& typeInfo) const
{
    assert(Thread::isRunningInMainThread());

    auto typeInfoName = String(typeInfo.name());

    for (auto i = 0U; i < eventTypes_.size(); i++)
    {
        if (eventTypes_[i] == typeInfoName)
            return i + 1;
    }

    eventTypes_.append(typeInfoName);

    return eventTypes_.size();
}

}
