/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Memory/MemoryStatistics.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

size_t MemoryStatistics::totalAllocationSize_;

// This class tracks allocation statistics for all the allocations which have a size within a specific range.
class AllocationStatistics
{
public:

    // The cutoff allocation size for this set of statistics
    size_t cutoff;

    // Current counts for this range of allocation sizes
    size_t activeCount;
    size_t peakCount;

    // Called when an allocation affecting this allocation size range is made
    void onAllocate()
    {
        activeCount++;
        peakCount = std::max(peakCount, activeCount);
    }

    // Called when an allocation affecting this allocation size range is freed
    void onFree()
    {
        assert(activeCount && "No allocations");
        activeCount--;
    }
};

// This array holds all the statistics for allocations grouped into the specified size ranges. Each instance in this
// array holds the statistics for allocations with a size <= to the cutoff and greater than the cutoff of the previous
// entry. This array can be adjusted to get different groupings of allocation sizes.
const size_t AllocationStatisticsCutoffSizes[] = {1, 2, 4, 8, 16, 32, 64, 96, 128, 256, 384, 512, 1024, 1536, 1024 * 2,
                                                  1024 * 4, 1024 * 8, 1024 * 16, 1024 * 32, 1024 * 64, 1024 * 128,
                                                  1024 * 256, 1024 * 512, 1024 * 1024, 1024 * 1024 * 2, 1024 * 1024 * 4,
                                                  1024 * 1024 * 8, 1024 * 1024 * 16, 1024 * 1024 * 32, 1024 * 1024 * 64,

                                                  // This is a catch-all for all allocation sizes above 64MB
                                                  ~size_t(0)};

static AllocationStatistics allocationStatistics[sizeof(AllocationStatisticsCutoffSizes) / sizeof(size_t)];

// Returns the set of allocation statistics pertaining to an allocation of the given size.
static AllocationStatistics& getStatisticsForAllocationSize(size_t size)
{
    // Set up the allocation statistics instances the first time this method is called, an AllocationStatistics
    // constructor can't be used because that would introduce static initialization order dependencies.
    static auto isSetup = false;
    if (!isSetup)
    {
        isSetup = true;

        auto nextCutoffSize = AllocationStatisticsCutoffSizes;
        for (auto& stats : allocationStatistics)
            stats.cutoff = *nextCutoffSize++;
    }

    // Move through the sets of statistics until the cutoff is exceeded
    auto stats = allocationStatistics;
    while (size > stats->cutoff)
        stats++;

    return *stats;
}

void MemoryStatistics::addAllocation(size_t size)
{
    getStatisticsForAllocationSize(size).onAllocate();

    // Update the total size of all allocations
    totalAllocationSize_ += size;
}

void MemoryStatistics::removeAllocation(size_t size)
{
    getStatisticsForAllocationSize(size).onFree();

    // Update the total size of all allocations
    assert(size <= totalAllocationSize_ && "Allocation size exceeds the total size of all allocations");
    totalAllocationSize_ -= size;
}

void MemoryStatistics::logAllocationDetails()
{
#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    auto totalAllocationCount = size_t(0);

    for (const auto& stats : allocationStatistics)
        totalAllocationCount += stats.activeCount;

    LOG_DEBUG << FileSystem::formatByteSize(getAllocationSizeTotal()) << " used by " << totalAllocationCount
              << " allocations";

    for (const auto& stats : allocationStatistics)
    {
        LOG_DEBUG << "    <= " << FileSystem::formatByteSize(stats.cutoff).prePadToLength(10)
                  << "    active: " << String(stats.activeCount).padToLength(6) << " peak: " << stats.peakCount;
    }
#endif
}

}
