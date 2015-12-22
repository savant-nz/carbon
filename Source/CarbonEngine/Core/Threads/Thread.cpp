/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/Threads/Mutex.h"
#include "CarbonEngine/Core/Threads/Thread.h"
#include "CarbonEngine/Math/Interpolate.h"

#ifdef POSIX
    #include <pthread.h>
#endif

namespace Carbon
{

// Stores the thread ID for the main thread on startup for use in Thread::isRunningInMainThread().
static std::thread::id mainThreadID;
static bool isMainThreadIDKnown;
static void initializeMainThreadID()
{
    mainThreadID = std::this_thread::get_id();
    isMainThreadIDKnown = true;
}
CARBON_REGISTER_STARTUP_FUNCTION(initializeMainThreadID, ~0U)

Thread::~Thread()
{
    if (isRunning())
        Globals::debugLog("Thread is still running, destructing the Thread instance will likely result in a crash");

    if (thread_)
    {
        if (thread_->joinable())
            thread_->detach();

        delete thread_;
        thread_ = nullptr;
    }
}

bool Thread::run(float priority)
{
    if (isRunning())
    {
        LOG_ERROR << "Thread is already running";
        return false;
    }

    priority_ = priority;

    delete thread_;
    thread_ = nullptr;

#ifdef APPLE
    thread_ = new std::thread(&Thread::startWithAutoReleasePool, this);
#else
    thread_ = new std::thread(&Thread::start, this);
#endif

    return true;
}

void Thread::setName()
{
#ifdef WINDOWS
    const auto MS_VC_EXCEPTION = DWORD();

    __try
    {
        auto info = std::array<ULONG_PTR, 4>{{0x1000, ULONG_PTR(getName().cStr()), GetCurrentThreadId(), 0}};
        RaiseException(MS_VC_EXCEPTION, 0, info.size(), info.data());
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#elif defined(APPLE)
    pthread_setname_np(getName().cStr());
#else
    pthread_setname_np(pthread_self(), getName().cStr());
#endif
}

void Thread::setPriority()
{
#ifdef WINDOWS
    SetThreadPriority(GetCurrentThread(), Interpolate::linear(THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL, priority_));
#else
    auto policy = SCHED_FIFO;

    auto param = sched_param();
    param.sched_priority = Interpolate::linear(sched_get_priority_min(policy), sched_get_priority_max(policy), priority_);
    pthread_setschedparam(thread_->native_handle(), policy, &param);
#endif
}

void Thread::start()
{
    setName();
    setPriority();

    {
        auto lock = ScopedMutexLock(mutex_);
        exitFlag_ = false;
        isRunning_ = true;
    }

    main();

    {
        auto lock = ScopedMutexLock(mutex_);
        isRunning_ = false;
        exitFlag_ = false;
    }
}

void Thread::waitWithQueuedEventDispatching(unsigned int sleepTime)
{
    assert(isRunningInMainThread() && "Thread::waitWithQueuedEventDispatching() can only be called from the main thread");

    while (isRunning())
    {
        events().dispatchQueuedEvents();
        sleep(sleepTime);
    }
}

void Thread::wait()
{
    if (isRunning())
        thread_->join();
}

void Thread::sleep(unsigned int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::duration<unsigned int, std::milli>(milliseconds));
}

bool Thread::isRunningInMainThread()
{
    return !isMainThreadIDKnown || std::this_thread::get_id() == mainThreadID;
}

}
