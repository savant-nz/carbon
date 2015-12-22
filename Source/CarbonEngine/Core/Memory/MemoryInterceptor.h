/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CARBONENGINE_CORE_MEMORY_MEMORYINTERCEPTOR_H
#define CARBONENGINE_CORE_MEMORY_MEMORYINTERCEPTOR_H

#if defined(CARBON_DEBUG) && !defined(CARBON_DISABLE_MEMORY_INTERCEPTOR) && !defined(CARBON_INCLUDE_MEMORY_INTERCEPTOR)
    #define CARBON_INCLUDE_MEMORY_INTERCEPTOR
#endif

namespace Carbon
{

/**
 * \file
 */

/**
 * Using placement new directly can lead to incorrect reporting of memory leaks when the MemoryInterceptor is in use because
 * because placement new can't be redefined in the same way that operator new can. This means that if placement new is used
 * directly the operator new macro would cause an initial call to MemoryInterceptor::start() to be made that is not then matched
 * by a subsequent call to MemoryInterceptor::allocate(), resulting in the next allocation being incorrectly labelled if it
 * comes directly from system code that is unaware of the MemoryInterceptor's redefinition of operator new (i.e. it doesn't know
 * to call MemoryInterceptor::start()).
 *
 * To fix this a custom placement_new() function is provided which avoids the effects of the operator new macro redefinition.
 */
template <typename T, typename... ArgTypes> static T* placement_new(void* p, ArgTypes&&... args)
{
    return new (p) T(std::forward<ArgTypes>(args)...);
}

/**
 * This class is used in debugging to intercept all of the process' memory allocation and free requests in order to detect
 * memory leaks and other common application memory errors. It uses the MemoryValidator, MemoryLeakDetector and MemoryStatistics
 * classes to do a lot of this work. MemoryInterceptor also allows for custom allocation systems to be used if desired, at
 * present this is only used on console platforms.
 *
 * The \a flags parameters that are passed to a number of MemoryInterceptor methods are solely for use by the active
 * MemoryInterceptor backend if it requires them, and should be zero unless some special allocation direction is being given to
 * the active backend.
 */
class CARBON_API MemoryInterceptor
{
public:

    /**
     * This method is used by the operator new macro at the bottom of this file in order to tell the memory interceptor the
     * relevant file and line information for allocations it is about to receive. If the MemoryInterceptor was not included in
     * the build then this method does nothing. This method is thread safe.
     */
    static void start(const char* file, unsigned int line);

    /**
     * This method is where all intercepted allocations come first. If the MemoryInterceptor was not included in the build then
     * this method passes straight through to MemoryInterceptor::untrackedAllocate(). The MemoryInterceptor::free() method must
     * be called to free any memory allocated using this method. This method is thread safe.
     */
    static void* allocate(size_t size, unsigned int flags = 0, bool isArray = false, bool canThrowBadAlloc = true,
                          bool doLeakDetection = true);

    /**
     * Templated variant of MemoryInterceptor::allocate() that casts the pointer to the specified type and also takes a count
     * specifying the number of items of that type to allocate. Otherwise identical to MemoryInterceptor::allocate().
     */
    template <typename T>
    static T* allocate(unsigned int count = 1, unsigned int flags = 0, bool canThrowBadAlloc = true,
                       bool doLeakDetection = true)
    {
        return reinterpret_cast<T*>(allocate(sizeof(T) * count, flags, false, canThrowBadAlloc, doLeakDetection));
    }

    /**
     * This method is where all intercepted deallocations come first. If the MemoryInterceptor was not included in the build
     * then this method passes straight through to MemoryInterceptor::untrackedFree(). This method is thread safe.
     */
    static void free(void* block, unsigned int flags = 0, bool isArray = false, bool doLeakDetection = true);

    /**
     * Returns the next allocation index to use, this is just a running counter that goes up by one for every allocation and can
     * be used to measure the number of allocations made during a specific time period or block of code. This method is thread
     * safe.
     */
    static uint64_t getNextAllocationIndex();

    /**
     * Allocates a chunk of memory that will not be tracked, padded, or leak checked. This is the lowest level allocation that
     * can be done through the MemoryInterceptor. The MemoryInterceptor::untrackedFree() method must be used to free memory
     * allocated by this method. Throws `std::bad_alloc` if the allocation fails.
     */
    static void* untrackedAllocate(size_t size, unsigned int flags, bool canThrowBadAlloc);

    /**
     * Frees memory allocated by MemoryInterceptor::untrackedAllocate().
     */
    static void untrackedFree(void* block, unsigned int flags);

    /**
     * This can be used to have lines written to the debug output via Globals::debugLog() that give details on every allocation
     * and deallocation that passes through the MemoryInterceptor. This is particularly useful when debugging or needing to see
     * more details on the memory usage patterns of a piece of code. This functionality could be replicated by specifying
     * allocation and free callback functions, but using the built in logging is more convenient if the finer control offered by
     * the MemoryInterceptor callback functions is not required.
     */
    static bool EnableLogging;

    /**
     * Typedef for a callback function which receives details about a single allocation.
     */
    typedef std::function<void(const void* address, size_t size, const char* file, unsigned int line, uint64_t index)>
        AllocationCallback;

    /**
     * Typedef for a callback function which receives details about a single free.
     */
    typedef std::function<void(const void* address, size_t size)> FreeCallback;

    /**
     * This callback is for use when debugging memory leaks, when not null it is called after every single allocation that
     * passes through the MemoryInterceptor and can be used during development to break on specific allocations or to analyze
     * memory usage patterns. One possible use of this callback is to break based on the passed index value, e.g. if allocation
     * 7822 is reported to be leaking then this callback can be used to break on allocation 7822 in order to isolate the
     * problem. Note that allocation callback functions must not directly or indirectly allocate or free any memory except
     * through MemoryInterceptor::untrackedAllocate() and MemoryInterceptor::untrackedFree(). Memory deallocations can also be
     * analyzed in a similar way using MemoryInterceptor::fnFreeCallback.
     */
    static AllocationCallback fnAllocationCallback;

    /**
     * This is the deallocation callback equivalent to MemoryInterceptor::fnAllocationCallback.
     */
    static FreeCallback fnFreeCallback;

    /**
     * Runs full validation on all active allocations. Any errors detected are printed on the debug output, and if there are
     * errors then an assert is thrown. The return value is the number of errors that were found.
     */
    static unsigned int validateAllAllocations();

    /**
     * If a BlockAllocatorSet is in use by the current memory interceptor backend then it will be returned by this method,
     * otherwise null is returned.
     */
    static const BlockAllocatorSet* getBlockAllocators();

private:

    static void setup();

    static Mutex* mutex_;

    // Internal methods used by MemoryInterceptor::validateAllAllocations()
    static void validateAllAllocationsMemoryErrorCallback(const char* errorMessage);
    static void validateAllAllocationsAllocationCallback(const void* address, size_t size, const char* file, unsigned int line,
                                                         uint64_t index);

    // Everything below here is defined as the MemoryInterceptor backend, and must be reimplemented on console platforms

    static void setupThreadLocalStorage();
    static void getCurrentThreadsFileAndLine(const char*& file, unsigned int& line);
    static void setCurrentThreadsFileAndLine(const char* file, unsigned int line);

    // These methods are called by MemoryInterceptorallocate() and MemoryInterceptorfree() in order for the various size,
    // address and memory checks to be done
    static size_t beforeTrackedAllocate(size_t size, unsigned int flags);
    static byte_t* afterTrackedAllocate(size_t actualSize, uint64_t index, unsigned int flags, void* block, bool isArray);
    static void* beforeTrackedFree(byte_t* block, unsigned int flags, size_t& size, bool isArray, bool isFileAndLineKnown);

    // Backend validation function used by MemoryInterceptor::validateAllAllocations()
    static void validateSingleAllocation(const void* address, size_t size, const char* file, unsigned int line, uint64_t index);
};

}

#endif

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR

    // Redefine operator new as a macro that passes file and line information to MemoryInterceptor::start() prior to actually
    // calling operator new.

    #undef new
    #define new (Carbon::MemoryInterceptor::start(__FILE__, __LINE__), false) ? nullptr : new

#endif
