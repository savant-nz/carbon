/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Memory/BlockAllocator.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

BlockAllocator::BlockAllocator(unsigned int chunkSize, unsigned int blockSize, unsigned int freeBlockCacheSize,
                               const std::function<void*(size_t size)>& fnAlloc,
                               const std::function<void(void* p)>& fnFree)
    : blockSize_(blockSize),
      blockCount_(chunkSize / blockSize),
      chunk_(reinterpret_cast<byte_t*>(fnAlloc(chunkSize))),
      chunkSize_(chunkSize),
      usedBlocksArraySize_(blockCount_ / 8),
      usedBlocks_(reinterpret_cast<byte_t*>(fnAlloc(usedBlocksArraySize_))),
      freeBlockCacheSize_(freeBlockCacheSize),
      freeBlockCache_(reinterpret_cast<unsigned int*>(fnAlloc(freeBlockCacheSize * sizeof(unsigned int)))),
      fnFree_(fnFree)
{
    // The chunk size should be a multiple of the block size
    if (chunkSize % blockSize)
    {
        Globals::debugLog("Error: block allocator chunk size is not a multiple of the block size");
        assert(false);
    }

    // There is one bit per block in usedBlocks_[] which means that the block count shohuld be a multiple of eight
    if (blockCount_ % 8)
    {
        Globals::debugLog("Error: block allocator block count is not a multiple of eight");
        assert(false);
    }

    // Mark all blocks as free
    memset(usedBlocks_, 0, usedBlocksArraySize_);
}

BlockAllocator::~BlockAllocator()
{
    fnFree_(chunk_);
    fnFree_(usedBlocks_);
    fnFree_(freeBlockCache_);
}

void* BlockAllocator::allocate()
{
    auto lock = ScopedMutexLock(mutex_);

    if (allocatedBlockCount_ == blockCount_)
    {
        // Issue a warning when the block allocator fills up for the first time
        if (!hasFullWarningBeenIssued_)
        {
            hasFullWarningBeenIssued_ = true;
            Globals::debugLog("Warning: the block allocator with block size %u is full", blockSize_);
        }

        return nullptr;
    }

    // If the free block cache is empty then repopulate it
    if (!freeBlockCacheEntryCount_)
    {
        repopulateFreeBlockCache();

        // We confirmed earlier in this method that there are unallocated blocks, which means that searching for free
        // blocks with which to repopulate the free block cache must succeed in finding at least one free block that can
        // be allocated. If this doesn't happen then there is some inconsistency, corruption or other problem within the
        // block allocator.

        assert(freeBlockCacheEntryCount_ && "Block allocator internal consistency failure");
    }

    // Get the index of the block to allocate and remove it from the free block cache
    auto blockIndex = freeBlockCache_[--freeBlockCacheEntryCount_];

    // Get the address of the block
    auto block = &chunk_[blockIndex * blockSize_];

    // Mask for this block in usedBlocks_[]
    auto usedMask = byte_t(1 << (blockIndex & 0x7));

    // Check that this block is marked as free in usedBlocks_[]
    assert((usedBlocks_[blockIndex / 8] & usedMask) == 0 && "Block to be allocated is not currently marked as free");

    // Allocate this block
    usedBlocks_[blockIndex / 8] |= usedMask;
    allocatedBlockCount_++;

    // Keep track of the maximum number of blocks that were allocated at any one time
    highestAllocatedBlockCount_ = std::max(allocatedBlockCount_, highestAllocatedBlockCount_);

    return block;
}

bool BlockAllocator::free(void* block)
{
    auto lock = ScopedMutexLock(mutex_);

    assert(hasAllocation(block) && "Address does not lie in this block allocator");

    // Get byte offset to allocation in chunk_
    auto offset = reinterpret_cast<byte_t*>(block) - chunk_;

    // Check that the offset is a multiple of the block size, if it isn't then this is an invalid free
    assert((offset % blockSize_ == 0) && "Address does not point to the start of a block");

    // Convert offset to a block index
    auto blockIndex = uint(offset) / blockSize_;

    // Mask for this block in usedBlocks_[]
    auto usedMask = byte_t(1 << (blockIndex & 0x7));

    // Check that this block is currently allocated, if this assert triggers then a block was probably double-freed
    assert((usedBlocks_[blockIndex / 8] & usedMask) && "Block not marked as allocated, possible double free");

    // Deallocate this item
    usedBlocks_[blockIndex / 8] &= ~usedMask;

    // Decrement the block count
    assert(allocatedBlockCount_ && "Allocated block count is zero");
    allocatedBlockCount_--;

    // Put this block into the free block cache if there's room
    if (freeBlockCacheEntryCount_ < freeBlockCacheSize_)
        freeBlockCache_[freeBlockCacheEntryCount_++] = blockIndex;

    return true;
}

void BlockAllocator::printInfo() const
{
    LOG_DEBUG << "Usage details for the " << blockSize_ << " byte block allocator:";
    LOG_DEBUG << "    Block count          " << blockCount_;
    LOG_DEBUG << "    Chunk size           " << FileSystem::formatByteSize(chunkSize_);
    LOG_DEBUG << "    Current usage        " << FileSystem::formatByteSize(allocatedBlockCount_ * blockSize_) << " ("
              << String::formatPercentage(allocatedBlockCount_, blockCount_) << ")";
    LOG_DEBUG << "    Highest usage        " << FileSystem::formatByteSize(highestAllocatedBlockCount_ * blockSize_)
              << " (" << String::formatPercentage(highestAllocatedBlockCount_, blockCount_) << ")";
}

unsigned int BlockAllocator::getMemoryUsage() const
{
    return chunkSize_ + usedBlocksArraySize_ + freeBlockCacheSize_ * sizeof(unsigned int) + sizeof(*this);
}

void BlockAllocator::processGatherMemorySummaryEvent(const GatherMemorySummaryEvent& gmse) const
{
    gmse.addAllocation("Block allocator", "Chunk", chunk_, chunkSize_);
    gmse.addAllocation("Block allocator", "Internal", usedBlocks_, usedBlocksArraySize_);
    gmse.addAllocation("Block allocator", "Internal", freeBlockCache_, usedBlocksArraySize_);
}

void BlockAllocator::repopulateFreeBlockCache()
{
    // Try and start in a place where the search is likely to find free blocks faster than if it started at the
    // beginning and worked its way forwards from there

    auto start = allocatedBlockCount_ / 8;
    auto end = start + usedBlocksArraySize_;

    for (auto i = start; i < end; i++)
    {
        auto index = i % usedBlocksArraySize_;

        if (usedBlocks_[index] == 0xFF)
            continue;

        for (auto j = 0U; j < 8; j++)
        {
            // If this block is not allocated then put it into the free block cache
            if (((usedBlocks_[index] >> j) & 1) == 0)
            {
                freeBlockCache_[freeBlockCacheEntryCount_++] = index * 8 + j;

                // Terminate if the free block cache is full
                if (freeBlockCacheEntryCount_ == freeBlockCacheSize_)
                    return;
            }
        }
    }
}

}
