/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
#include "CarbonEngine/Core/Memory/MemoryLeakDetector.h"
#include "CarbonEngine/Core/Memory/MemoryStatistics.h"
#include "CarbonEngine/Core/Memory/MemoryValidator.h"
#include "CarbonEngine/Core/Threads/Mutex.h"

#include "CarbonEngine/Core/Memory/DefineGlobalOperatorNewDelete.h"

namespace Carbon
{

bool MemoryInterceptor::EnableLogging = false;

MemoryInterceptor::AllocationCallback MemoryInterceptor::fnAllocationCallback;
MemoryInterceptor::FreeCallback MemoryInterceptor::fnFreeCallback;

Mutex* MemoryInterceptor::mutex_;

void MemoryInterceptor::setup()
{
    if (mutex_)
        return;

    mutex_ = placement_new<Mutex>(untrackedAllocate(sizeof(Mutex), 0, true));

    setupThreadLocalStorage();
}

void MemoryInterceptor::start(const char* file, unsigned int line)
{
#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    setup();
    setCurrentThreadsFileAndLine(file, line);
#endif
}

void* MemoryInterceptor::allocate(size_t size, unsigned int flags, bool isArray, bool canThrowBadAlloc, bool doLeakDetection)
{
#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    setup();

    // Issue a warning for allocation requests over 2GB, these are almost always a bug
    if (size >= (1U << 31) - 1)
        Globals::debugLog("Warning: Allocation request exceeds 2GB, size: %lluMB", uint64_t(size / 1024 / 1024));

    // Get index for this allocation
    auto index = getNextAllocationIndex();

    // Get the file and line information for this allocation, note that these are only set for allocations from code that
    // actually saw the relevant operator new overload.
    auto file = pointer_to<const char>::type();
    auto line = 0U;
    getCurrentThreadsFileAndLine(file, line);

    // Wipe the current file and line information, if it stayed around and the next allocation to be processed was from code
    // that hadn't seen the operator new oveload then this file and line information would get reused incorrectly
    setCurrentThreadsFileAndLine(nullptr, 0);

    // If stress testing is enabled then do a full validation of all current allocations
    if (MemoryValidator::EnableStressTest)
        validateAllAllocations();

    auto reportedBlock = pointer_to<byte_t>::type();

    {
        auto lock = ScopedMutexLock(*mutex_);

        // Allow adjustment of the allocated size
        auto actualSize = beforeTrackedAllocate(size, flags);

        // Do the actual allocation
        auto actualBlock = pointer_to<void>::type();
        try
        {
            actualBlock = untrackedAllocate(actualSize, flags, true);
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }

        // Allow adjustment of the returned address
        reportedBlock = afterTrackedAllocate(actualSize, index, flags, actualBlock, isArray);

        // Update memory statistics
        MemoryStatistics::addAllocation(size);

        // If logging is enabled then write out details on this allocation
        if (EnableLogging)
        {
            Globals::debugLog("Allocation   %0*p\t\t%9llu bytes from %s:%u%s (#%llu)", sizeof(reportedBlock) * 2 + 2,
                              reportedBlock, uint64_t(size), file ? file : "?", line, isArray ? " (array)" : "", index);
        }

        // Run callback function if one is specified
        if (fnAllocationCallback)
            fnAllocationCallback(reportedBlock, size, file, line, index);
    }

    // Update the leak detector
    if (doLeakDetection)
        MemoryLeakDetector::addAllocation(reportedBlock, size, file, line, index);
#else
    void* reportedBlock = untrackedAllocate(size, flags, canThrowBadAlloc);
#endif

    return reportedBlock;
}

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR

// Reporting of memory leaks on shutdown is done by detecting when static deinitialization is in process using the following
// class and then writing the memory leaks report file straight away. It is then rewritten on every subsequent invocation of
// MemoryInterceptor::free(). This inefficient system is required because there is no way to know when static deinitialization
// has finished and then write the memory leaks logfile just once.
static bool isInStaticDeinitialization;
static struct StaticDeinitializationDetector
{
    ~StaticDeinitializationDetector()
    {
        isInStaticDeinitialization = true;
        MemoryLeakDetector::writeMemoryLeaksReportFile();
    }
} staticDeinitializationDetector;

#endif

void MemoryInterceptor::free(void* block, unsigned int flags, bool isArray, bool doLeakDetection)
{
    if (!block)
        return;

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    // Update the leak detector
    auto isFileAndLineKnown = doLeakDetection && MemoryLeakDetector::removeAllocation(block);

    // If stress testing is enabled then do a full validation of all current allocations
    if (MemoryValidator::EnableStressTest)
        validateAllAllocations();

    setup();
    auto lock = ScopedMutexLock(*mutex_);

    // Allow adjustment of the block address and retrieve the size of this allocation
    auto size = size_t();
    auto actualBlock = beforeTrackedFree(reinterpret_cast<byte_t*>(block), flags, size, isArray, isFileAndLineKnown);

    // Write the memory leaks report file if we're in the middle of static deinitialization
    if (isInStaticDeinitialization)
        MemoryLeakDetector::writeMemoryLeaksReportFile();

    // Update memory statistics
    MemoryStatistics::removeAllocation(size);

    // Do the actual deallocation
    untrackedFree(actualBlock, flags);

    // If logging is enabled write out details on this free
    if (EnableLogging)
    {
        Globals::debugLog("Free         %0*p\t\t%9i bytes%s", sizeof(block) * 2 + 2, block, int(size),
                          isArray ? " (array)" : "");
    }

    // Run callback function if one is specified
    if (fnFreeCallback)
        fnFreeCallback(block, size);
#else
    untrackedFree(block, flags);
#endif
}

uint64_t MemoryInterceptor::getNextAllocationIndex()
{
    static auto nextAllocationIndex = uint64_t(1);

    setup();
    auto lock = ScopedMutexLock(*mutex_);

    return nextAllocationIndex++;
}

static auto memoryValidationErrorCount = 0U;
unsigned int MemoryInterceptor::validateAllAllocations()
{
    setup();
    auto lock = ScopedMutexLock(*mutex_);

    // Reset the error count
    memoryValidationErrorCount = 0;

    // The memory error callback is redirected so that all errors are reported without asserts, and also so that any errors
    // detected can be grouped by allocation
    auto originalMemoryErrorCallback = MemoryValidator::fnErrorCallback;
    MemoryValidator::fnErrorCallback = validateAllAllocationsMemoryErrorCallback;

    MemoryLeakDetector::enumerateAllocations(validateAllAllocationsAllocationCallback);

    MemoryValidator::fnErrorCallback = originalMemoryErrorCallback;

    assert(memoryValidationErrorCount == 0);

#ifdef _MSC_VER
    // On MSVC, validate the CRT heap as well
    assert(_CrtCheckMemory());
#endif

    return memoryValidationErrorCount;
}

static auto currentAddress = pointer_to<const void>::type();
static auto currentSize = size_t();
static auto currentFile = pointer_to<const char>::type();
static auto currentLine = 0U;
static auto currentIndex = uint64_t();

void MemoryInterceptor::validateAllAllocationsAllocationCallback(const void* address, size_t size, const char* file,
                                                                 unsigned int line, uint64_t index)
{
    currentAddress = address;
    currentSize = size;
    currentFile = file;
    currentLine = line;
    currentIndex = index;

    // Validation for a specific allocation is implemented in the memory interceptor backend. Any errors detected will end up in
    // the callback below
    validateSingleAllocation(address, size, file, line, index);
}

void MemoryInterceptor::validateAllAllocationsMemoryErrorCallback(const char* errorMessage)
{
    if (currentAddress)
    {
        Globals::debugLog("Error: Allocation at %p of size %llu was made by %s:%u (#%llu)", currentAddress,
                          uint64_t(currentSize), currentFile ? currentFile : "[unknown]", currentLine, currentIndex);
        currentAddress = nullptr;
    }

    Globals::debugLog("Error: %s", errorMessage);

    memoryValidationErrorCount++;
}

}
