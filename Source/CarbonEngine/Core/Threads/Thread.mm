/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Threads/Thread.h"

namespace Carbon
{

void Thread::startWithAutoReleasePool()
{
    // Start the thread with an autorelease pool in place
    @autoreleasepool
    {
        [[NSThread currentThread] setName:getName().toNSString()];

        start();
    }
}

}
