/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Memory/BlockAllocator.h"
#include "CarbonEngine/Core/Memory/BlockAllocatorSet.h"
#include "CarbonEngine/Globals.h"

namespace Carbon
{

void BlockAllocatorSet::create(const unsigned int config[][2], const std::function<void*(size_t size)>& fnAlloc,
                               const std::function<void(void* p)>& fnFree)
{
    if (isCreated())
        return;

    // Determine the block allocator count
    blockAllocatorCount_ = 0;
    while (config[blockAllocatorCount_][0])
        blockAllocatorCount_++;

    // Allocate space for the block allocators
    blockAllocators_ = reinterpret_cast<BlockAllocator*>(fnAlloc(blockAllocatorCount_ * sizeof(BlockAllocator)));

    // Setup the block allocators
    for (auto i = 0U; i < blockAllocatorCount_; i++)
    {
        auto blockSize = config[i][0];
        auto chunkSize = config[i][1];

        if (blockSize % 32)
        {
            Globals::debugLog("Error: block allocator block size is not a multiple of 32: %u", blockSize);
            assert(false);
        }

        if (i > 0 && blockSize <= config[i - 1][0])
        {
            Globals::debugLog("Error: block allocator block sizes must be ordered by increasing size");
            assert(false);
        }

        // Construct the block allocator
        placement_new<BlockAllocator>(&blockAllocators_[i], chunkSize, blockSize, 64, fnAlloc, fnFree);
    }
}

void* BlockAllocatorSet::allocate(unsigned int size)
{
    for (auto i = 0U; i < blockAllocatorCount_; i++)
    {
        if (size <= blockAllocators_[i].getBlockSize())
        {
            auto p = blockAllocators_[i].allocate();
            if (p)
                return p;
        }
    }

    return nullptr;
}

bool BlockAllocatorSet::free(void* block)
{
    for (auto i = 0U; i < blockAllocatorCount_; i++)
    {
        if (blockAllocators_[i].hasAllocation(block))
        {
            blockAllocators_[i].free(block);
            return true;
        }
    }

    return false;
}

void BlockAllocatorSet::printInfo() const
{
    for (auto i = 0U; i < blockAllocatorCount_; i++)
        blockAllocators_[i].printInfo();
}

}
