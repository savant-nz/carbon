/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Interface class for handling an event via the EventHandler::processEvent() method.
 */
class CARBON_API EventHandler
{
public:

    virtual ~EventHandler();

    /**
     * This method is called by the EventManager when an event that this class has registered for is sent. The return
     * value from this method controls propagation of the event to subsequent handlers that have registered to receive
     * the same event. A value of true will allow the event to proceed, and a value of false will 'swallow' the event.
     * Returning false will also result in the return value of the original EventManager::dispatchEvent() call that sent
     * out the event being false.
     */
    virtual bool processEvent(const Event& e) = 0;
};

}
