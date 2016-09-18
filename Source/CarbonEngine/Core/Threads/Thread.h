/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Threads/Mutex.h"

namespace Carbon
{

/**
 * Threads in the engine and application are implemented by subclassing this Thread class, implementing the
 * Thread::main() method, and then calling Thread::run(). Supports thread naming, thread priorities, and other threading
 * support functionality.
 */
class CARBON_API Thread : private Noncopyable
{
public:

    /**
     * Constructs this thread object with the given name, a thread's name is used to help identify it when debugging but
     * is otherwise unused.
     */
    Thread(String name) : name_(std::move(name)) {}

    virtual ~Thread();

    /**
     * Returns the name of this thread that was passed to its constructor.
     */
    const String& getName() const { return name_; }

    /**
     * Starts this thread's execution and assigns it the given priority level. Thread priorities range from the lowest
     * priority of 0 to the highest priority of 1, and the default priority is 0.5. Returns success flag.
     */
    bool run(float priority = 0.5f);

    /**
     * Causes the calling thread to block until this thread's execution has completed.
     */
    void wait();

    /**
     * This method is for use on the main thread to wait for this thread to terminate, it is different to Thread::wait()
     * in that it loops around checking the return value of Thread::isRunning() while also calling
     * EventManager::dispatchQueuedEvents(). The latter call ensures that this thread's execution doesn't hang
     * indefinitely in a call to EventManager::dispatchEvent(). The \a sleepTime parameter is how many milliseconds to
     * sleep for every time around the wait loop.
     */
    void waitWithQueuedEventDispatching(unsigned int sleepTime = 1);

    /**
     * Returns whether or not this thread is currently executing.
     */
    bool isRunning() const
    {
        auto lock = ScopedMutexLock(mutex_);
        return isRunning_;
    }

    /**
     * Sets this thread's exit flag which is a signal to the thread that it should terminate cleanly as soon as
     * possible. Individual implementations of Thread::main() are responsible for obeying this flag. The state of the
     * exit flag can be retrieved by the thread using Thread::shouldExit(). There is no way to forcibly terminate a
     * running thread.
     */
    void setExitFlag()
    {
        auto lock = ScopedMutexLock(mutex_);
        exitFlag_ = true;
    }

    /**
     * Causes the calling thread to sleep for the given number of milliseconds.
     */
    static void sleep(unsigned int milliseconds);

    /**
     * This method returns true when called from the main thread, when called from any other thread it returns false.
     */
    static bool isRunningInMainThread();

protected:

    /**
     * This is the execution entry point for the thread and must be implemented by subclasses.
     */
    virtual void main() = 0;

    /**
     * Returns whether the exit flag has been set on this thread, if the return value is true then the thread should
     * terminate cleanly as soon as possible. Individual Thread::main() implementations are responsible for checking and
     * obeying this flag.
     */
    bool shouldExit() const
    {
        auto lock = ScopedMutexLock(mutex_);
        return exitFlag_;
    }

private:

    std::thread* thread_ = nullptr;

    const String name_;
    float priority_ = 0.0f;

    mutable Mutex mutex_;

    bool isRunning_ = false;
    bool exitFlag_ = false;

    // The Thread::start() method is private and handles setup and teardown for a thread, most of the thread execution
    // occurs in the call to Thread::main()
    void start();

#ifdef APPLE
    void startWithAutoReleasePool();
#endif

    void setName();
    void setPriority();
};

}
