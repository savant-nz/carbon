/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/HashFunctions.h"

namespace Carbon
{

/**
 * This is a static helper class that is responsible for tracking all memory allocations and then reporting memory leaks
 * on shutdown, it is used by the MemoryInterceptor class.
 */
class CARBON_API MemoryLeakDetector
{
public:

    /**
     * Adds an allocation to the memory leak detector. This method is thread safe.
     */
    static void addAllocation(const void* address, size_t size, const char* file, unsigned int line, uint64_t index);

    /**
     * Removes an allocation from the memory leak detector. Returns whether or not the file and line information for it
     * is known. This method is thread safe.
     */
    static bool removeAllocation(const void* address);

    /**
     * Calls the specified callback function once for every allocation currently known to the memory leak detector. The
     * callback function must not interact with any allocation routines. This method is thread safe and only works if
     * the memory leak detector is currently enabled (see MemoryLeakDetector::isEnabled()).
     */
    static void enumerateAllocations(const MemoryInterceptor::AllocationCallback& fnCallback);

    /**
     * Returns whether the memory leak detector is currently enabled and active. The memory leak detector is enabled on
     * startup in builds which include the memory interceptor (i.e. those that define CARBON_INCLUDE_MEMORY_INTERCEPTOR,
     * which happens by default in debug builds). The memory leak detector is always disabledin builds which do not
     * include the memory interceptor. If the memory leak detector is enabled it can be switched off by calling
     * MemoryLeakDetector::disable(), however once it is off it can't be turned back on.
     */
    static bool isEnabled() { return isEnabled_; }

    /**
     * Turns off the memory leak detector if it is currently enabled and frees any internal memory it is using, this can
     * be useful to eliminate the performance and memory overhead associated with using the memory leak detector, while
     * still getting the benefit of the other memory debugging tools. Once the memory leak detector is turned off it
     * cannot be turned back on, and no memory leak reports file will be written on shutdown. See
     * MemoryLeakDetector::isEnabled() for more details.
     */
    static void disable();

    /**
     * Registers any internal memory leak detector allocations with the passed GatherMemorySummaryEvent instance.
     */
    static bool onGatherMemorySummaryEvent(const GatherMemorySummaryEvent& e);

    /**
     * On platforms that support local filesystem access this returns the full path to the memory leaks report file. On
     * other platforms it returns "<client name> Memory Leaks". This is a UTF-8 encoded string.
     */
    static const byte_t* getMemoryLeaksReportFilename();

    /**
     * Writes details on all leaked allocations to the file specified by
     * MemoryLeakDetector::getMemoryLeaksReportFilename(), leaked allocations are all those which have been added using
     * MemoryLeakDetector::addAllocation() but not removed with MemoryLeakDetector::removeAllocation(). If
     * Globals::getLeakedResourceCount() returns a non-zero value then this method will not log individual memory leaks
     * and the resulting file will just have a message saying that the resource leaks must be cleaned up first prior to
     * memory leaks being logged. This method should only be called during static deinitialization. Platforms that don't
     * support local filesystem access must provide their own implementation of this method.
     */
    static void writeMemoryLeaksReportFile();

private:

    static bool isEnabled_;

    static Mutex* mutex_;
    static void setupMutex();

    // Holds the number of allocations currently being tracked by the memory leak detector
    static unsigned int activeAllocationCount_;

    // This structure holds details on allocations using a singly linked list
    struct AllocationInfo
    {
        // Unique index for every allocation
        uint64_t index;

        // Singly linked list
        AllocationInfo* next;

        // Basic allocation information
        const void* address;
        size_t size;
        const char* file;
        unsigned int line;
        bool inStaticInitialization;

        // When reporting memory leaks this value is used to help with the grouping of leaks by source file
        bool reported;
    };

    // Hash table that tracks all currently active allocations
    static const auto HashTableSize = 8191U;
    static AllocationInfo* allocations_[HashTableSize];

    // Returns the index into the allocations_ hash table for the specified allocation address
    static unsigned int getAllocationAddressHash(const void* address)
    {
        return HashFunctions::hash(address) % HashTableSize;
    }

    // Linked list of AllocationInfo objects that are not currently in use
    static AllocationInfo* allocationInfoReservoir_;

    // The reservoir of AllocationInfo instances is filled by making large untracked allocations and then setting up the
    // next pointers appropriately and putting the resulting linked list into the reservoir. These allocations
    // themselves need to be tracked so they can be freed on shutdown.
    static const auto ReservoirAllocationSize = 1024U * 1024U;
    static AllocationInfo** allocationInfoReservoirAllocations_;
    static unsigned int allocationInfoReservoirAllocationCount_;
    static void freeReservoir();

    // Groups memory leaks and enumerates them via the specified callbacks
    static unsigned int enumerateMemoryLeaks(
        bool includeStaticInitializationLeaks, const std::function<void(const char* file)>& fnBeginLeaksForFile,
        const std::function<bool(size_t size, const char* file, unsigned int line, uint64_t index)>& fnReportLeak,
        const std::function<void(const char* file)>& fnEndLeaksForFile);

    typedef int (*PFnFormattedPrint)(const char* format, ...);

    static unsigned int buildMemoryLeaksReportHTMLContent(bool includeStaticInitializationLeaks,
                                                          PFnFormattedPrint fnPrintf);
};

}
