/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Threads/Mutex.h"

namespace Carbon
{

/**
 * This class manages fixed-size allocations in a chunk of memory and is intended for internal use in accelerating allocation
 * speed. This class is thread-safe.
 */
class CARBON_API BlockAllocator : private Noncopyable
{
public:

    /**
     * Sets up this block allocator to use the given chunk size and block size. \a fnAlloc and \a fnFree must point to functions
     * that can be used by this class to allocate and free any memory that the block allocator needs.
     */
    BlockAllocator(unsigned int chunkSize, unsigned int blockSize, unsigned int freeBlockCacheSize,
                   const std::function<void*(size_t size)>& fnAlloc, const std::function<void(void* p)>& fnFree);

    ~BlockAllocator();

    /**
     * Returns the size of an individual block in this block allocator.
     */
    unsigned int getBlockSize() const { return blockSize_; }

    /**
     * Returns the number of individual blocks in this block allocator
     */
    unsigned int getBlockCount() const { return blockCount_; }

    /**
     * Returns the base address of the chunk of memory being managed by this class.
     */
    const byte_t* getChunk() const { return chunk_; }

    /**
     * Returns the number of blocks that are currently allocated by this block allocator.
     */
    unsigned int getAllocatedBlockCount() const { return allocatedBlockCount_; }

    /**
     * Returns the highest number of blocks that have ever been concurrently allocated by this block allocator.
     */
    unsigned int getHighestAllocatedBlockCount() const { return highestAllocatedBlockCount_; }

    /**
     * Returns a pointer to a freshly allocated block, or null if there are no free blocks. Blocks allocated using this method
     * must be freed using the BlockAllocator::free() method.
     */
    void* allocate();

    /**
     * Frees a block allocated by BlockAllocator::allocate().
     */
    bool free(void* block);

    /**
     * Returns whether or not the given allocation lies inside this block allocator.
     */
    bool hasAllocation(const void* block) const { return block < chunk_ + chunkSize_ && block >= chunk_; }

    /**
     * Logs usage details on this block allocator.
     */
    void printInfo() const;

    /**
     * Returns the amount of memory being used by this block allocator.
     */
    unsigned int getMemoryUsage() const;

    /**
     * Adds information on this block allocator's internal allocations to the passed GatherMemorySummaryEvent.
     */
    void processGatherMemorySummaryEvent(const GatherMemorySummaryEvent& gmse) const;

private:

    mutable Mutex mutex_;

    const unsigned int blockSize_ = 0;
    const unsigned int blockCount_ = 0;

    // The chunk of memory which is being managed by this block allocator
    byte_t* const chunk_ = nullptr;
    const unsigned int chunkSize_ = 0;

    // Tracks which blocks have been allocated, 1 bit per block
    const unsigned int usedBlocksArraySize_ = 0;
    byte_t* const usedBlocks_ = nullptr;

    // Tracks the number of allocated blocks
    unsigned int allocatedBlockCount_ = 0;

    // Tracks the largest number of blocks that have been allocated at any one time
    unsigned int highestAllocatedBlockCount_ = 0;
    bool hasFullWarningBeenIssued_ = false;

    // A small cache of free blocks is kept to improve allocation speed
    const unsigned int freeBlockCacheSize_ = 0;
    unsigned int* const freeBlockCache_ = 0;
    unsigned int freeBlockCacheEntryCount_ = 0;

    // If the free block cache is empty it is refilled by BlockAllocator::allocate() using this method
    void repopulateFreeBlockCache();

    // This is the function used in the destructor to free the allocations made in the constructor
    std::function<void(void* p)> fnFree_ = nullptr;
};

}
