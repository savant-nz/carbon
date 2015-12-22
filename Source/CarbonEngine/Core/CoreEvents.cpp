/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Memory/MemoryStatistics.h"

namespace Carbon
{

String FileSystemErrorEvent::getErrorString() const
{
    return FileSystem::errorToString(error_);
}

void GatherMemorySummaryEvent::report()
{
    auto gmse = GatherMemorySummaryEvent();
    events().dispatchEvent(gmse);

    auto totalSize = size_t();

    // Sum the total size for all allocations of the same type
    auto collated = std::map<String, size_t>();
    for (auto& allocation : gmse.getAllocations())
    {
        collated[allocation.getType() + (allocation.getDetails().length() ? " (" + allocation.getDetails() + ")" : "")] +=
            allocation.getSize();

        totalSize += allocation.getSize();
    }

    // Report usage for each type
    for (const auto& item : collated)
        LOG_DEBUG << item.first << " - " << FileSystem::formatByteSize(item.second);

    // Print a summary line
    LOG_DEBUG << "";
#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    LOG_DEBUG << "These allocations use " << FileSystem::formatByteSize(totalSize) << " in total, which is "
              << String::formatPercentage(totalSize, MemoryStatistics::getAllocationSizeTotal(), 0)
              << " of all allocated memory";
#else
    LOG_DEBUG << "These allocations use " << FileSystem::formatByteSize(totalSize) << " in total";
#endif
}

}
