/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Memory/BlockAllocator.h"
#include "CarbonEngine/Core/Memory/BlockAllocatorSet.h"
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
#include "CarbonEngine/Core/Memory/MemoryLeakDetector.h"
#include "CarbonEngine/Core/Memory/MemoryValidator.h"
#include "CarbonEngine/Globals.h"

// This MemoryInterceptor backend is based on malloc/free. Platforms can implement their own allocator backend if
// desired.

#ifdef CARBON_INCLUDE_DEFAULT_MEMORY_INTERCEPTOR_BACKEND

// Current versions of iOS and Mac OS X do not support the C++11 thread_local storage class specifier. Mac OS X does
// support __thread but this is a non-standard extension. Use pthreads for TLS on both platforms.
#ifdef APPLE
    #define USE_PTHREADS_FOR_TLS
#endif

namespace Carbon
{

// On Windows a basic BlockAllocatorSet is used in debug builds in order to avoid most interaction with the slow debug
// heap. Search for details on the _NO_DEBUG_HEAP environment variable for further information. Putting a generously
// sized BlockAllocatorSet between the engine and the Windows Debug Heap improves performance a lot when debugging with
// an IDE.
#if defined(WINDOWS) && defined(CARBON_DEBUG)
    #define MEGABYTES(n) (n * 1024 * 1024)
    #define CARBON_BLOCK_ALLOCATOR_CONFIG \
        {32, MEGABYTES(4)}, {256, MEGABYTES(16)}, {1024, MEGABYTES(16)}, { 0, 0 }
#endif

#ifdef CARBON_BLOCK_ALLOCATOR_CONFIG
    static BlockAllocatorSet blockAllocators;
#endif

// Thread local storage for allocation file/line information
#ifdef USE_PTHREADS_FOR_TLS
    static pthread_key_t tlsFile, tlsLine;
#else
    thread_local static const char* tlsFile;
    thread_local static unsigned int tlsLine;
#endif

void MemoryInterceptor::setupThreadLocalStorage()
{
#ifdef USE_PTHREADS_FOR_TLS
    // Setup Pthread's TLS, this is needed to safely get file/line information through to the interceptor methods
    if (pthread_key_create(&tlsFile, nullptr) != 0 || pthread_key_create(&tlsLine, nullptr) != 0)
        MemoryValidator::reportError("Failed setting up Pthread thread local storage");
#endif
}

void MemoryInterceptor::getCurrentThreadsFileAndLine(const char*& file, unsigned int& line)
{
#ifdef USE_PTHREADS_FOR_TLS
    file = reinterpret_cast<const char*>(pthread_getspecific(tlsFile));
    line = uint(uintptr_t(pthread_getspecific(tlsLine)));
#else
    file = tlsFile;
    line = tlsLine;
#endif
}

void MemoryInterceptor::setCurrentThreadsFileAndLine(const char* file, unsigned int line)
{
#ifdef USE_PTHREADS_FOR_TLS
    pthread_setspecific(tlsFile, file);
    pthread_setspecific(tlsLine, reinterpret_cast<void*>(line));
#else
    tlsFile = file;
    tlsLine = line;
#endif
}

void* MemoryInterceptor::untrackedAllocate(size_t size, unsigned int flags, bool canThrowBadAlloc)
{
    auto block = pointer_to<void>::type();

#ifdef CARBON_BLOCK_ALLOCATOR_CONFIG
    if (!blockAllocators.isCreated())
    {
        unsigned int setup[][2] = {CARBON_BLOCK_ALLOCATOR_CONFIG};
        blockAllocators.create(setup, ::malloc, ::free);
    }

    block = blockAllocators.allocate(size);
    if (block)
        return block;
#endif

    block = ::malloc(size);
    if (block)
        return block;

    if (canThrowBadAlloc)
    {
        Globals::debugLog("Memory interceptor backend could not allocate %llu bytes, throwing std::bad_alloc",
                          uint64_t(size));
        throw std::bad_alloc();
    }
    else
        return nullptr;
}

void MemoryInterceptor::untrackedFree(void* block, unsigned int flags)
{
#ifdef CARBON_BLOCK_ALLOCATOR_CONFIG
    if (blockAllocators.free(block))
        return;
#endif

    ::free(block);
}

size_t MemoryInterceptor::beforeTrackedAllocate(size_t size, unsigned int flags)
{
    return MemoryValidator::beforeAllocation(size);
}

byte_t* MemoryInterceptor::afterTrackedAllocate(size_t actualSize, uint64_t index, unsigned int flags, void* block,
                                                bool isArray)
{
    return MemoryValidator::afterAllocation(reinterpret_cast<byte_t*>(block), actualSize, index, isArray);
}

void* MemoryInterceptor::beforeTrackedFree(byte_t* block, unsigned int flags, size_t& size, bool isArray,
                                           bool isFileAndLineKnown)
{
    return MemoryValidator::beforeFree(block, size, isArray, isFileAndLineKnown);
}

void MemoryInterceptor::validateSingleAllocation(const void* address, size_t size, const char* file, unsigned int line,
                                                 uint64_t index)
{
    MemoryValidator::validateAllocation(reinterpret_cast<const byte_t*>(address));
}

const BlockAllocatorSet* MemoryInterceptor::getBlockAllocators()
{
#ifdef CARBON_BLOCK_ALLOCATOR_CONFIG
    return &blockAllocators;
#else
    return nullptr;
#endif
}

}

#endif
