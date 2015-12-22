/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This class contains methods that do validations on memory allocations, it is used mainly by the MemoryInterceptor class. The
 * most important validation checks are for buffer underruns and overruns. New allocations and recently freed allocations can
 * also be optionally set to random data in order to improve error detection. These features help with the automatic detection
 * of a variety of memory errors, and when errors are encountered the error callback is called. See
 * MemoryValidator::fnErrorCallback for details on the handling of memory validation errors.
 *
 * This class both respects and ensures 32-byte alignment of addresses in order to maximize platform compatibility.
 */
class CARBON_API MemoryValidator
{
public:

    /**
     * This callback is called whenever this class detects any memory corruption or other problems. If this callback is null
     * then a default handler is called which logs the error to the standard error stream and then asserts. This behavior can be
     * overridden in order to alter the default response to the detection of memory errors.
     */
    static std::function<void(const char* message)> fnErrorCallback;

    /**
     * Reports a memory error through the current memory error callback, see MemoryValidator:fnErrorCallback for details.
     */
    static void reportError(const char* format, ...);

    /**
     * The size of the prefix area used on allocations. This is the additional space before each allocation that is reserved for
     * use by the MemoryValidator. The first 16 bytes of the prefix area are reserved for internal use. Must be a multiple of
     * 32.
     */
    static const size_t PrefixAreaSize = 32;

    /**
     * The size of the suffix area used on allocations. This is the additional space after each allocation that is reserved for
     * use by the MemoryValidator. Larger suffix areas will do a more reliable job of detecting overruns, but at the cost of
     * increased memory usage. Must be a multiple of 32.
     */
    static const size_t SuffixAreaSize = 32;

    /**
     * This is the value written into the prefix area of every new allocation. It is verified when deallocating in order to
     * detect underrun.
     */
    static const byte_t PrefixAreaValue = 0x55;

    /**
     * This is the value written into the suffix area of every new allocation. It is verified when deallocating in order to
     * detect overrun.
     */
    static const byte_t SuffixAreaValue = 0xAA;

    /**
     * This is the value written into fresh allocations.
     */
    static const byte_t UnusedAreaValue = 0xBC;

    /**
     * This is the value written into allocations just before they are freed.
     */
    static const byte_t FreedAreaValue = 0xDE;

    /**
     * Specifies whether new and freed allocations should be wiped with randomly generated data rather than using the
     * MemoryValidator::UnusedAreaValue and MemoryValidator::FreedAreaValue constants when performing the wipe. This defaults to
     * false but can be enabled to stress test an application further. Enabling this will increase the time taken to allocate
     * and free memory.
     */
    static bool EnableRandomWipe;

    /**
     * Specifies whether to do the maximum amount of testing of memory integrity, this can be useful when trying to narrow down
     * the cause of memory corruption. When this is enabled a full validation of all known memory allocations is performed
     * whenever an allocation or free occurs, and any issues encountered will trigger the error callback. This system can be
     * used to narrow down the source of a corruption problem, however this additional checking adds a very large amount of
     * overhead and so is disabled by default.
     */
    static bool EnableStressTest;

    /**
     * This method should be called immediately before an allocation so that the MemoryValidator can adjust its size in order to
     * add space for the prefix and suffix areas as well as any other per-allocation data it needs to store. Returns the
     * adjusted size.
     */
    static size_t beforeAllocation(size_t size);

    /**
     * This method should be called immediately after a successful allocation so that the MemoryValidator can initialize the new
     * allocation and return an adjusted base address.
     */
    static byte_t* afterAllocation(byte_t* block, size_t size, uint64_t index, bool isArray);

    /**
     * This method should be called immediately before an allocation is freed so that the MemoryValidator can validate it for
     * any corruption and return the original base address. The user-visible size of the allocation is returned in \a size. Any
     * errors encountered will trigger the error callback specified by MemoryValidator::fnErrorCallback.
     */
    static void* beforeFree(byte_t* block, size_t& size, bool isArray, bool verifyIsArray);

    /**
     * Checks the specified allocation for any corruption (e.g. overruns, underruns) and triggers the error callback if problems
     * are found. Returns the size of the allocation not including any extra bytes used by the MemoryValidator.
     */
    static size_t validateAllocation(const byte_t* block);

private:

    static void setToRandomData(byte_t* block, size_t size);
};

}
