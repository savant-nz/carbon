/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This class handles allocations and frees on a set of BlockAllocator instances where each BlockAllocator has a different size.
 * This is useful in MemoryInterceptor backends that use BlockAllocator as part of a custom allocation configuration.
 *
 * \note There is deliberately no constructor for this class because it should always reside in static storage (which is set to
 * zero on process startup). This is required because BlockAllocatorSet needs to be available to serve allocations during
 * static object initialization and so if this class had a constructor then that constructor would end up being run after
 * the call to BlockAllocatorSet::create() made by the MemoryInterceptor backend. This would wipe the BlockAllocatorSet just
 * after it had been created, causing it to lose track of early allocations.
 */
class CARBON_API BlockAllocatorSet : private Noncopyable
{
public:

    /**
     * Creates this BlockAllocatorSet from the given \a config value which describes the block allocator layout that is to be
     * created. Each entry in \a config is a set of two values that describes a BlockAllocator, the first is the size in bytes
     * of an individual block in the BlockAllocator and the second is the size in bytes of the memory chunk that that
     * BlockAllocator should use. There is no limit on the number of BlockAllocator instnaces that can be created. The \a config
     * array must be explicitly terminated by a { 0, 0 } entry. The \a fnAlloc and \a fnFree parameters are the functions that
     * will be used to allocate and free memory needed by the BlockAllocator instances that are created by this method.
     */
    void create(const unsigned int config[][2], const std::function<void*(size_t size)>& fnAlloc,
                const std::function<void(void* p)>& fnFree);

    /**
     * Returns whether or not this BlockAllocatorSet has been created by a call to BlockAllocatorSet::create().
     */
    bool isCreated() const { return blockAllocators_ != nullptr; }

    /**
     * Returns the number of BlockAllocator instances in this BlockAllocatorSet.
     */
    unsigned int size() const { return blockAllocatorCount_; }

    /**
     * Returns an individual BlockAllocator, the index must be less than the value returned by BlockAllocatorSet::size().
     */
    const BlockAllocator& operator[](unsigned int index) const { return blockAllocators_[index]; }

    /**
     * Tries to allocate a block of memory from one of the BlockAllocator instances that is large enough for the specified
     * allocation size. Returns null on failure. This method is thread-safe.
     */
    void* allocate(unsigned int size);

    /**
     * Frees a block of memory allocated by BlockAllocatorSet::allocate(). The return value indicates whether the specified
     * block was able to be freed or not. This method is thread-safe.
     */
    bool free(void* block);

    /**
     * Logs usage details on all the BlockAllocator instances in this BlockAllocatorSet.
     */
    void printInfo() const;

private:

    unsigned int blockAllocatorCount_;
    BlockAllocator* blockAllocators_;
};

}
