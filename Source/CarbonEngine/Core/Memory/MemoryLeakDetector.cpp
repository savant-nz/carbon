/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Memory/MemoryLeakDetector.h"
#include "CarbonEngine/Core/Memory/MemoryValidator.h"
#include "CarbonEngine/Core/Threads/Mutex.h"

namespace Carbon
{

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    bool MemoryLeakDetector::isEnabled_ = true;
#else
    bool MemoryLeakDetector::isEnabled_ = false;
#endif

Mutex* MemoryLeakDetector::mutex_;

unsigned int MemoryLeakDetector::activeAllocationCount_;
MemoryLeakDetector::AllocationInfo* MemoryLeakDetector::allocations_[MemoryLeakDetector::HashTableSize];
MemoryLeakDetector::AllocationInfo* MemoryLeakDetector::allocationInfoReservoir_;
MemoryLeakDetector::AllocationInfo** MemoryLeakDetector::allocationInfoReservoirAllocations_;
unsigned int MemoryLeakDetector::allocationInfoReservoirAllocationCount_;

void MemoryLeakDetector::setupMutex()
{
    if (mutex_)
        return;

    mutex_ = placement_new<Mutex>(MemoryInterceptor::untrackedAllocate(sizeof(Mutex), 0, true));
}

void MemoryLeakDetector::addAllocation(const void* address, size_t size, const char* file, unsigned int line, uint64_t index)
{
    if (!isEnabled_)
        return;

    setupMutex();
    auto lock = ScopedMutexLock(*mutex_);

    // If the reservoir of AllocationInfo instances is empty then allocate a new set and fill up the reservoir
    if (!allocationInfoReservoir_)
    {
        auto allocationInfoCount = uint(ReservoirAllocationSize / sizeof(AllocationInfo));

        allocationInfoReservoir_ = MemoryInterceptor::allocate<AllocationInfo>(allocationInfoCount, 0, false, false);
        auto newAllocationInfoReservoirAllocations =
            MemoryInterceptor::allocate<AllocationInfo*>(allocationInfoReservoirAllocationCount_ + 1, 0, false, false);
        if (!allocationInfoReservoir_ || !newAllocationInfoReservoirAllocations)
        {
            MemoryValidator::reportError("Internal memory leak detector allocation failed");
            return;
        }

        // Set all the next pointers on the new reservoir items
        allocationInfoReservoir_[allocationInfoCount - 1].next = nullptr;
        for (auto i = 0U; i < allocationInfoCount - 1; i++)
            allocationInfoReservoir_[i].next = &allocationInfoReservoir_[i + 1];

        // Update the list of allocations used to create the reservoir, these are then able to be freed on shutdown
        memcpy(newAllocationInfoReservoirAllocations, allocationInfoReservoirAllocations_,
               allocationInfoReservoirAllocationCount_ * sizeof(AllocationInfo*));
        MemoryInterceptor::free(allocationInfoReservoirAllocations_, 0, false, false);
        allocationInfoReservoirAllocations_ = newAllocationInfoReservoirAllocations;
        allocationInfoReservoirAllocations_[allocationInfoReservoirAllocationCount_++] = allocationInfoReservoir_;
    }

    // Take an AllocationInfo instance out of the reservoir
    auto a = allocationInfoReservoir_;
    allocationInfoReservoir_ = a->next;

    // Store the details about this allocation
    a->address = address;
    a->size = size;
    a->file = file;
    a->line = line;
    a->index = index;
    a->inStaticInitialization = Globals::isInStaticInitialization();

    // Store this new allocation
    auto hashIndex = getAllocationAddressHash(a->address);
    a->next = allocations_[hashIndex];
    allocations_[hashIndex] = a;

    activeAllocationCount_++;
}

bool MemoryLeakDetector::removeAllocation(const void* address)
{
    if (!isEnabled_)
        return false;

    setupMutex();
    auto lock = ScopedMutexLock(*mutex_);

    // Find this allocation in the global linked list
    auto hashIndex = getAllocationAddressHash(address);
    auto allocation = allocations_[hashIndex];
    auto previousAllocation = pointer_to<AllocationInfo>::type();

    while (allocation)
    {
        if (allocation->address == address)
            break;

        previousAllocation = allocation;
        allocation = allocation->next;
    }

    if (!allocation)
    {
        MemoryValidator::reportError("Allocation at %p is unknown to the memory leak detector, possible double free", address);
        return false;
    }

    // Remove this allocation
    if (allocation->next && previousAllocation)
        previousAllocation->next = allocation->next;
    else
    {
        if (allocation->next)
            allocations_[hashIndex] = allocation->next;
        else if (previousAllocation)
            previousAllocation->next = nullptr;
        else
            allocations_[hashIndex] = nullptr;
    }

    auto isFileAndLineKnown = allocation->file && allocation->file[0];

    // Put the AllocationInfo instance back into the reservoir so it can be reused
    allocation->next = allocationInfoReservoir_;
    allocationInfoReservoir_ = allocation;

    // If there are now no allocations being tracked then free up the reservoir of AllocationInfo instances, without this they
    // would be leaked on shutdown
    if (--activeAllocationCount_ == 0)
        freeReservoir();

    return isFileAndLineKnown;
}

void MemoryLeakDetector::freeReservoir()
{
    MemoryInterceptor::free(allocationInfoReservoirAllocations_, 0, false, false);
    allocationInfoReservoirAllocations_ = nullptr;
    allocationInfoReservoirAllocationCount_ = 0;
    allocationInfoReservoir_ = nullptr;

    activeAllocationCount_ = 0;

    for (auto& hashEntry : allocations_)
        hashEntry = nullptr;
}

bool MemoryLeakDetector::onGatherMemorySummaryEvent(const GatherMemorySummaryEvent& e)
{
    // Tell GatherMemorySummaryEvent about the memory leak detector's allocations.

    for (auto i = 0U; i < allocationInfoReservoirAllocationCount_; i++)
        e.addAllocation("Memory leak detector", "", allocationInfoReservoirAllocations_[i], ReservoirAllocationSize);

    return true;
}
CARBON_REGISTER_EVENT_HANDLER_FUNCTION(GatherMemorySummaryEvent, MemoryLeakDetector::onGatherMemorySummaryEvent)

void MemoryLeakDetector::enumerateAllocations(const MemoryInterceptor::AllocationCallback& fnCallback)
{
    if (!mutex_)
        return;

    auto lock = ScopedMutexLock(*mutex_);

    for (auto a : allocations_)
    {
        while (a)
        {
            fnCallback(a->address, a->size, a->file, a->line, a->index);
            a = a->next;
        }
    }
}

void MemoryLeakDetector::disable()
{
    setupMutex();
    auto lock = ScopedMutexLock(*mutex_);

    isEnabled_ = false;
    freeReservoir();
}

static byte_t leaksReportFilename[2048];

const byte_t* MemoryLeakDetector::getMemoryLeaksReportFilename()
{
    return leaksReportFilename;
}

// On startup get the name of the memory leaks report file to use, and delete it if it currently exists.
static void setupMemoryLeaksReportFile()
{
    auto filename = UnicodeString(Globals::getClientName()) + " Memory Leaks";

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    filename = Logfile::getFilename(filename);
    FileSystem::deleteLocalFile(filename);
#endif

    auto utf8 = filename.toUTF8();

    memcpy(leaksReportFilename, utf8.getData(), utf8.getDataSize());
}
CARBON_REGISTER_STARTUP_FUNCTION(setupMemoryLeaksReportFile, 0)

unsigned int MemoryLeakDetector::enumerateMemoryLeaks(
    bool includeStaticInitializationLeaks, const std::function<void(const char* file)>& fnBeginLeaksForFile,
    const std::function<bool(size_t size, const char* file, unsigned int line, uint64_t index)>& fnReportLeak,
    const std::function<void(const char* file)>& fnEndLeaksForFile)
{
    if (!isEnabled_)
        return 0;

    // Mark all allocations as not having been reported yet
    for (auto a : allocations_)
    {
        while (a)
        {
            a->reported = false;
            a = a->next;
        }
    }

    auto memoryLeakCount = 0U;

    // Go through all allocations
    for (auto i = 0U; i < HashTableSize; i++)
    {
        auto a = allocations_[i];
        while (a)
        {
            if (!a->reported && (!a->inStaticInitialization || includeStaticInitializationLeaks))
            {
                // Memory leaks are grouped by source file, start a new subsection for this allocation's source file
                fnBeginLeaksForFile(a->file);

                // Find all other leaks in this source file
                for (auto j = i; j < HashTableSize; j++)
                {
                    auto a2 = (j == i) ? a : allocations_[j];
                    while (a2)
                    {
                        if ((!a->file && !a2->file) || (a->file && a2->file && strcmp(a->file, a2->file) == 0))
                        {
                            a2->reported = true;

                            if (!a2->inStaticInitialization || includeStaticInitializationLeaks)
                            {
                                if (fnReportLeak(a2->size, a2->file, a2->line, a2->index))
                                    memoryLeakCount++;
                            }
                        }
                        a2 = a2->next;
                    }
                }

                // Close off this section
                fnEndLeaksForFile(a->file);
            }

            a = a->next;
        }
    }

    return memoryLeakCount;
}

unsigned int MemoryLeakDetector::buildMemoryLeaksReportHTMLContent(bool includeStaticInitializationLeaks,
                                                                   PFnFormattedPrint fnPrintf)
{
    if (!isEnabled_)
        return 0;

    // Construct the title string based on the client name
    auto title = std::array<char, 256>();
    strcpy(title.data(), Globals::getClientNameBuffer());
    strcat(title.data(), " - Memory Leak Report");

    // Construct the subtitle
    auto subtitle = std::array<char, 512>();
    strcpy(subtitle.data(), "Created on ");
    FileSystem::getDateTime(subtitle.data() + strlen(subtitle.data()), uint(subtitle.size() - strlen(subtitle.data())));

    // Write the HTML header
    fnPrintf(Logfile::LogfileHeader, title.data(), title.data(), subtitle.data());

    auto memoryLeakCount = 0U;

#ifdef CARBON_DEBUG
    // If there are resource leaks then report their details and don't log any detected memory leaks
    if (Globals::getLeakedResourceCount())
    {
        fnPrintf(
            "<div class='info'>%i resource leak%s detected. These are listed near the end of the application logfile.<br/><br/>"
            "Resource leaks must be fixed before memory leaks can be accurately logged.</div>",
            Globals::getLeakedResourceCount(), Globals::getLeakedResourceCount() == 1 ? "" : "s");

        memoryLeakCount = Globals::getLeakedResourceCount();
    }
    else
#endif
    {
        // Write a placeholder line that will state the number of memory leaks found, this is filled in at the end by a little
        // piece of JavaScript
        fnPrintf("<div><br/></div><div class='info' id='summary'></div><div><br/><br/></div>");

        memoryLeakCount = enumerateMemoryLeaks(
            includeStaticInitializationLeaks,
            [&](const char* file) {
                if (file)
                    fnPrintf("<div class='info'>Leaks in %s:</div><div style='text-indent: 2.0em'><br/>", file);
            },
            [&](size_t size, const char* file, unsigned int line, uint64_t index) {
                if (!file)
                    return false;

                fnPrintf("<div class='info'>%llu bytes on line %u. (#%llu)</div>", uint64_t(size), line, index);

                return true;
            },
            [&](const char* file) {
                if (file)
                    fnPrintf("<br/></div>");
            });

        // Add some JavaScript to display the number of memory leaks at the top of the leak report file
        fnPrintf("<script type='text/javascript'>"
                 "document.getElementById('summary').innerHTML = 'Detected %u memory leak%s.';"
                 "</script>",
                 memoryLeakCount, memoryLeakCount == 1 ? "" : "s");
    }

    // Write the footer
    fnPrintf(Logfile::LogfileFooter);

    return memoryLeakCount;
}

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

// Printf-style callback function used when writing the HTML memory leaks file on platforms that support local filesystem
// access.
static FILE* memoryLeaksReportFile;
static int filePrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    auto buffer = std::array<char, 4096>();
    vsnprintf(buffer.data(), buffer.size() - 1, format, args);

    fwrite(buffer.data(), strlen(buffer.data()), 1, memoryLeaksReportFile);

    return 1;
}

void MemoryLeakDetector::writeMemoryLeaksReportFile()
{
    if (!isEnabled_)
        return;

#ifdef WINDOWS
    auto utf16 = std::array<wchar_t, MAX_PATH>();
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(getMemoryLeaksReportFilename()), -1, utf16.data(), utf16.size());
    memoryLeaksReportFile = _wfopen(utf16.data(), L"wb");
#else
    memoryLeaksReportFile = fopen(reinterpret_cast<const char*>(getMemoryLeaksReportFilename()), "wb");
#endif
    if (!memoryLeaksReportFile)
        return;

    buildMemoryLeaksReportHTMLContent(true, filePrintf);

    fclose(memoryLeaksReportFile);
    memoryLeaksReportFile = nullptr;
}

#endif
}
