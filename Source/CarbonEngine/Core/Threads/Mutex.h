/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * A recursive mutex synchronization primitive. This class has no public methods because all operations on it are done
 * via the ScopedMutexLock class to ensure correct pairing of mutex acquires and releases.
 */
class CARBON_API Mutex
{
    std::recursive_mutex mutex_;

    friend class ScopedMutexLock;
};

/**
 * This class assists with correct acquire and release of the Mutex class by acquiring a mutex in its constructor and
 * releasing it in its destructor. This ensures correct acquire/release pairing. There are also
 * ScopedMutexLock::acquire() and ScopedMutexLock::release() methods that provide finer control.
 */
class CARBON_API ScopedMutexLock : private Noncopyable
{
public:

    /**
     * This constructor takes a Mutex and acquires it, it is then released by this class' destructor.
     */
    ScopedMutexLock(Mutex& mutex) : mutex_(mutex.mutex_)
    {
        mutex_.lock();
        acquireCount_ = 1;
    }

    /**
     * Move constructor.
     */
    ScopedMutexLock(ScopedMutexLock&& other) : mutex_(other.mutex_), acquireCount_(other.acquireCount_)
    {
        other.acquireCount_ = 0;
    }

    ~ScopedMutexLock()
    {
        for (auto i = 0U; i < acquireCount_; i++)
            mutex_.unlock();
    }

    /**
     * Releases the mutex that was passed to the constructor. This can be used to release the mutex prior to this class
     * falling out of scope, and can also be used in combination with ScopedMutexLock::acquire() to temporarily release
     * then re-acquire the mutex. An internal counter is used to make sure that the correct number of mutex releases
     * occur in the destructor.
     */
    void release()
    {
        assert(acquireCount_ && "Attempted to release a scoped mutex that has an acquire count of zero");

        if (acquireCount_)
        {
            mutex_.unlock();
            acquireCount_--;
        }
    }

    /**
     * Acquires the mutex that was passed to the constructor. This can be used in combination with
     * ScopedMutexLock::release() to gain finer control over the mutex. An internal counter is used to make sure that
     * the correct number of mutex releases occur in the destructor.
     */
    void acquire()
    {
        mutex_.lock();
        acquireCount_++;
    }

private:

    std::recursive_mutex& mutex_;
    unsigned int acquireCount_ = 0;
};

}
