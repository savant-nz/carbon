/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"

namespace Carbon
{

/**
 * This class is responsible for organizing the storage of all vertex and index data into graphics memory.
 */
class CARBON_API DataBufferManager : private Noncopyable
{
public:

    /**
     * Opaque video memory allocation object. Null is reserved for 'no allocation'.
     */
    typedef void* AllocationObject;

    /**
     * This method is used by the renderer to notify the data buffer manager of a RecreateWindowEvent that it needs to
     * process.
     */
    void onRecreateWindowEvent(const RecreateWindowEvent& rwe);

    /**
     * Allocates video memory and returns the new allocation. \a type specifies the type of memory to be allocated, and
     * must be one of VertexDataBuffer or IndexDataBuffer. \a size is the size in bytes of the allocation being
     * requested. \a data is the pointer to source the data from. This pointer is normally a geometry chunk's internal
     * vertex or index data pointer. The data at this pointer is not copied by this method, and the caller is
     * responsible for ensuring that the pointer stays valid until the allocation is freed with
     * DataBufferManager::deallocate(). If the contents of the allocation are likely to change frequently then
     * \a isDynamic should be set to true so that the allocator can optimize for dynamic data. If the allocation fails
     * for any reason then null is returned.
     */
    AllocationObject allocate(GraphicsInterface::DataBufferType type, unsigned int size, const byte_t* data,
                              bool isDynamic);

    /**
     * Frees an allocation done by DataBufferManager::allocate(). This method does nothing if null is passed. Returns
     * success flag.
     */
    bool free(AllocationObject allocationObject);

    /**
     * Tells the data buffer manager that the data for the given allocation has changed and needs to be updated. The
     * updated data will be read from the \a data pointer that was passed to the DataBufferManager::allocate() method
     * when the allocation object was created. Returns success flag.
     */
    bool updateData(AllocationObject allocationObject);

    /**
     * Returns the graphics interface data buffer object to use when rendering the data in the given allocation. The \a
     * offset parameter is set to the number of bytes from the start of the data buffer object to the start of the
     * allocation in the returned data buffer object.
     */
    GraphicsInterface::DataBufferObject getAllocationBufferObject(AllocationObject allocationObject, uintptr_t& offset);

    /**
     * Returns an information string with vertex memory statistics useful for debugging.
     */
    String getMemoryStatistics() const;

private:

    DataBufferManager() {}
    ~DataBufferManager();
    friend class Globals;

    struct StaticDataGroup
    {
        const GraphicsInterface::DataBufferType type = GraphicsInterface::VertexDataBuffer;
        GraphicsInterface::DataBufferObject buffer = nullptr;    // The data buffer object for this group

        unsigned int blockSize = 0;
        unsigned int blockCount = 0;
        unsigned int allocatedBlockCount = 0;
        Vector<bool> isBlockAllocated;

        struct Allocation
        {
            unsigned int firstBlock = 0;
            unsigned int blockCount = 0;
            const byte_t* data = nullptr;
            unsigned int dataSize = 0;

            Allocation() {}
            Allocation(unsigned int firstBlock_, unsigned int blockCount_, const byte_t* data_, unsigned int dataSize_)
                : firstBlock(firstBlock_), blockCount(blockCount_), data(data_), dataSize(dataSize_)
            {
            }
        };

        Vector<Allocation*> allocations;

        bool isDirty = false;

        StaticDataGroup(GraphicsInterface::DataBufferType type_, unsigned int blockSize_, unsigned int blockCount_)
            : type(type_), blockSize(blockSize_), blockCount(blockCount_), isBlockAllocated(blockCount_, false)
        {
        }

        ~StaticDataGroup();

        Allocation* allocate(unsigned int size, const byte_t* data);
        void free(Allocation* allocation);
        unsigned int getActiveBlockCount() const;
    };

    struct Allocation
    {
        GraphicsInterface::DataBufferType type = GraphicsInterface::VertexDataBuffer;
        bool isDynamic = false;
        unsigned int size = 0;
        const byte_t* data = nullptr;

        // The data buffer object used if this is a dynamic allocation
        GraphicsInterface::DataBufferObject buffer = nullptr;

        // For static allocations
        StaticDataGroup* group = nullptr;
        StaticDataGroup::Allocation* allocation = nullptr;

        Allocation() {}
        Allocation(GraphicsInterface::DataBufferType type_, bool isDynamic_, unsigned int size_, const byte_t* data_)
            : type(type_), isDynamic(isDynamic_), size(size_), data(data_)
        {
        }
    };

    Vector<StaticDataGroup*> staticDataGroups_;
    Vector<Allocation*> allocations_;
};

}
