/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Render/DataBufferManager.h"
#include "CarbonEngine/Render/Renderer.h"

namespace Carbon
{

DataBufferManager::StaticDataGroup::~StaticDataGroup()
{
    for (auto allocation : allocations)
        delete allocation;

    graphics().deleteDataBuffer(buffer);
    buffer = nullptr;
}

DataBufferManager::StaticDataGroup::Allocation* DataBufferManager::StaticDataGroup::allocate(unsigned int size,
                                                                                               const byte_t* data)
{
    // The number of blocks needed
    auto blocksNeeded = (size + blockSize - 1) / blockSize;

    // Check there are enough free blocks
    if (blocksNeeded > blockCount - allocatedBlockCount)
        return nullptr;

    // Search for the required number of consecutive free blocks
    auto start = 0U;
    for (auto i = 0U; i < blockCount; i++)
    {
        if (isBlockAllocated[i])
            start = i + 1;
        else if (i - start + 1 == blocksNeeded)
        {
            // Mark the newly allocated blocks as taken
            for (auto j = 0U; j < blocksNeeded; j++)
                isBlockAllocated[start + j] = true;

            isDirty = true;
            allocatedBlockCount += blocksNeeded;

            allocations.append(new Allocation(start, blocksNeeded, data, size));
            return allocations.back();
        }
    }

    return nullptr;
}

void DataBufferManager::StaticDataGroup::free(Allocation* allocation)
{
    if (!allocations.unorderedEraseValue(allocation))
        return;

    // Mark the allocation's blocks as free
    for (auto i = 0U; i < allocation->blockCount; i++)
        isBlockAllocated[allocation->firstBlock + i] = false;

    allocatedBlockCount -= allocation->blockCount;

    delete allocation;
}

unsigned int DataBufferManager::StaticDataGroup::getActiveBlockCount() const
{
    auto activeBlockCount = blockCount;

    while (activeBlockCount && !isBlockAllocated[activeBlockCount - 1])
        activeBlockCount--;

    return activeBlockCount;
}

DataBufferManager::~DataBufferManager()
{
    for (auto group : staticDataGroups_)
        delete group;

    for (auto allocation : allocations_)
    {
        if (allocation->isDynamic)
            graphics().deleteDataBuffer(allocation->buffer);

        delete allocation;
    }
}

void DataBufferManager::onRecreateWindowEvent(const RecreateWindowEvent& rwe)
{
    if (rwe.getWindowEventType() == RecreateWindowEvent::CloseWindow)
    {
        // Flag all static groups as dirty and delete their buffers. This will force the buffer to be recreated and uploaded
        // next time they are accessed.
        for (auto group : staticDataGroups_)
        {
            graphics().deleteDataBuffer(group->buffer);
            group->buffer = nullptr;

            group->isDirty = true;
        }

        // Delete dynamic buffers
        for (auto allocation : allocations_)
        {
            if (allocation->isDynamic)
            {
                graphics().deleteDataBuffer(allocation->buffer);
                allocation->buffer = nullptr;
            }
        }
    }
    else if (rwe.getWindowEventType() == RecreateWindowEvent::NewWindow)
    {
        // For dynamic allocations, just recreate the data buffer
        for (auto a : allocations_)
        {
            if (a->isDynamic)
            {
                a->buffer = graphics().createDataBuffer();
                if (!a->buffer || !graphics().uploadDynamicDataBuffer(a->buffer, a->type, a->size, a->data))
                    LOG_ERROR << "Failed creating dynamic buffer";
            }
        }
    }
}

DataBufferManager::AllocationObject DataBufferManager::allocate(GraphicsInterface::DataBufferType type, unsigned int size,
                                                                  const byte_t* data, bool isDynamic)
{
    if (!data)
        return nullptr;

    if (size == 0)
        size = 1;

    // Create a new video memory allocation object
    auto allocation = new Allocation(type, isDynamic, size, data);
    allocations_.append(allocation);

    // Dynamic data goes straight into its own buffer if the buffer type is supported, whereas static data is combined into a
    // small number of shared buffers
    if (isDynamic)
    {
        allocation->buffer = graphics().createDataBuffer();
        if (!allocation->buffer || !graphics().uploadDynamicDataBuffer(allocation->buffer, allocation->type, size, data))
            LOG_ERROR << "Failed creating dynamic buffer";

        return allocation;
    }

    // This is a static data allocation, look through all the static data groups already created and see if there's room for
    // this allocation in any of them and if there is then allocate directly from there
    for (auto group : staticDataGroups_)
    {
        if (group->type == type)
        {
            allocation->allocation = group->allocate(size, data);
            if (allocation->allocation)
            {
                allocation->group = group;
                return allocation;
            }
        }
    }

    // The allocation doesn't fit in any existing static data group so make a new one. If this allocation is larger than the
    // default static data group size then it is sized especially for this allocation.

    const auto defaultBlockSize = 16384U;
    const auto defaultBlockCount = 128U;

    if (size >= defaultBlockSize * defaultBlockCount)
        allocation->group = new StaticDataGroup(type, size, 1);
    else
        allocation->group = new StaticDataGroup(type, defaultBlockSize, defaultBlockCount);

    staticDataGroups_.append(allocation->group);

    // Allocate out of the new static data group
    allocation->allocation = allocation->group->allocate(size, data);

    return allocation;
}

bool DataBufferManager::free(AllocationObject allocationObject)
{
    auto allocation = reinterpret_cast<Allocation*>(allocationObject);

    if (!allocation || !allocations_.unorderedEraseValue(allocation))
        return false;

    if (allocation->isDynamic)
    {
        // Delete the dynamic allocation's buffer, if any is present
        graphics().deleteDataBuffer(allocation->buffer);
        allocation->buffer = nullptr;
    }
    else
    {
        auto group = allocation->group;

        // Free the space being used by this static allocation
        group->free(allocation->allocation);

        // Delete the static data group if it is now empty
        if (group->allocations.empty())
        {
            staticDataGroups_.unorderedEraseValue(group);
            delete group;

            LOG_INFO << "Deleted static " << (allocation->type == GraphicsInterface::VertexDataBuffer ? "vertex" : "index")
                     << " data group " << allocation->group;
        }
    }

    // Delete the allocation
    delete allocation;
    allocation = nullptr;

    return true;
}

bool DataBufferManager::updateData(AllocationObject allocationObject)
{
    if (!allocationObject)
        return false;

    auto allocation = reinterpret_cast<const Allocation*>(allocationObject);

    if (allocation->isDynamic)
    {
        // For dynamic allocations, update the buffer contents
        if (allocation->buffer)
            graphics().updateDataBuffer(allocation->buffer, allocation->type, allocation->data);
    }
    else
    {
        // For static allocations setting the dirty flag will force an upload the next time it is accessed, so there's no need
        // to do anything right now
        allocation->group->isDirty = true;
    }

    return true;
}

GraphicsInterface::DataBufferObject DataBufferManager::getAllocationBufferObject(AllocationObject allocationObject,
                                                                                  uintptr_t& offset)
{
    if (!allocationObject)
        return nullptr;

    auto allocation = reinterpret_cast<const Allocation*>(allocationObject);

    if (allocation->isDynamic)
    {
        // For dynamic allocations simply return its data buffer with no offset
        offset = 0;
        return allocation->buffer;
    }

    auto group = allocation->group;

    // Return the offset to this allocation from the start of the data buffer
    offset = allocation->allocation->firstBlock * group->blockSize;

    // Dirty static data groups need to be reuploaded to the graphics interface
    if (group->isDirty)
    {
        // No longer dirty
        group->isDirty = false;

        // Create data buffer if needed
        if (!group->buffer)
        {
            group->buffer = graphics().createDataBuffer();
            if (!group->buffer)
            {
                LOG_ERROR << "Failed creating static data buffer";
                return nullptr;
            }
        }

        // Allocate a buffer to ready to receive the contents of this static data group
        auto uploadBuffer = Vector<byte_t>();
        try
        {
            uploadBuffer.resize(group->getActiveBlockCount() * group->blockSize);
        }
        catch (const std::bad_alloc&)
        {
            LOG_ERROR << "Failed allocating memory for the upload buffer";
            return nullptr;
        }

        // Fill the upload buffer
        for (auto i = 0U; i < group->allocations.size(); i++)
        {
            auto a = group->allocations[i];
            memcpy(&uploadBuffer[a->firstBlock * group->blockSize], a->data, a->dataSize);
        }

        // Upload to the graphics interface
        auto timer = SimpleTimer();
        graphics().uploadStaticDataBuffer(group->buffer, allocation->type, uploadBuffer.size(), uploadBuffer.getData());

        LOG_INFO << "Updated static " << (allocation->type == GraphicsInterface::VertexDataBuffer ? "vertex" : "index")
                 << " data group " << group << ", size: " << FileSystem::formatByteSize(uploadBuffer.size())
                 << ", time: " << timer;
    }

    return group->buffer;
}

String DataBufferManager::getMemoryStatistics() const
{
    auto dynamicBufferCount = 0U;
    auto totalBytes = 0U;

    for (auto allocation : allocations_)
    {
        if (allocation->isDynamic)
        {
            dynamicBufferCount++;
            totalBytes += allocation->size;
        }
        else if (allocation->group->buffer)
            totalBytes += allocation->size;
    }

    return String() << staticDataGroups_.size() << " static, " << dynamicBufferCount << " dynamic, " << allocations_.size()
                    << " allocations, " << FileSystem::formatByteSize(totalBytes);
}

}
