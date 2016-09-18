/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This class contains methods for tracking memory usage and memory allocation statistics. It is called internally by
 * the MemoryInterceptor class and has methods which can be used to get information of memory usage patterns in the
 * application.
 */
class CARBON_API MemoryStatistics
{
public:

    /**
     * Adds an an allocation of the specified size to the memory statistics.
     */
    static void addAllocation(size_t size);

    /**
     * Removes an an allocation of the specified size from the memory statistics.
     */
    static void removeAllocation(size_t size);

    /**
     * Returns the total number of bytes in all current allocations, this does not include any system heap overhead or
     * other memory tracking overhead.
     */
    static size_t getAllocationSizeTotal() { return totalAllocationSize_; }

    /**
     * Logs details about all the active allocations.
     */
    static void logAllocationDetails();

private:

    static size_t totalAllocationSize_;
};

}
