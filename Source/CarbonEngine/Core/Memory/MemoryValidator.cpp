/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/Memory/MemoryValidator.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

bool MemoryValidator::EnableRandomWipe = false;
bool MemoryValidator::EnableStressTest = false;

std::function<void(const char* message)> MemoryValidator::fnErrorCallback;

void MemoryValidator::reportError(const char* format, ...)
{
    auto message = std::array<char, 1024>();

    va_list args;
    va_start(args, format);
    vsnprintf(message.data(), message.size() - 1, format, args);

    if (fnErrorCallback)
        fnErrorCallback(message.data());
    else
    {
        // Default error handler
        Globals::debugLog("Memory error: %s", message.data());
        assert(false && "Memory error detected, check stdout or the debug output for details");
    }
}

size_t MemoryValidator::beforeAllocation(size_t size)
{
    // Increase the allocation size so that there is room for the prefix and suffix areas
    return size + PrefixAreaSize + SuffixAreaSize;
}

byte_t* MemoryValidator::afterAllocation(byte_t* block, size_t size, uint64_t index, bool isArray)
{
    if (!block)
        return nullptr;

    // Fill the suffix and prefix with the unique byte values that will be used to check for overrun and underrun when this
    // block is freed
    memset(block, PrefixAreaValue, PrefixAreaSize);
    memset(block + size - SuffixAreaSize, SuffixAreaValue, SuffixAreaSize);

    // Put the total allocation size right at the start of the prefix area along with the index and isArray flag
    *(reinterpret_cast<size_t*>(block)) = size;
    *(reinterpret_cast<uint64_t*>(block + sizeof(size_t))) = index;
    *(reinterpret_cast<bool*>(block + sizeof(size_t) + sizeof(uint64_t))) = isArray;

    // Move the returned block address past the prefix area
    block += PrefixAreaSize;

    // Set the initial contents of the new allocation
    if (EnableRandomWipe)
        setToRandomData(block, size - PrefixAreaSize - SuffixAreaSize);
    else
        memset(block, UnusedAreaValue, size - PrefixAreaSize - SuffixAreaSize);

    return block;
}

void* MemoryValidator::beforeFree(byte_t* block, size_t& size, bool isArray, bool verifyIsArray)
{
    // Validate block prior to freeing
    size = validateAllocation(block);

    // Wipe the contents of the block prior to freeing
    if (EnableRandomWipe)
        setToRandomData(block, size);
    else
        memset(block, FreedAreaValue, size);

    // Adjust returned address to the true address of the allocation
    block -= PrefixAreaSize;

    // Check that the isArray flag matches up. This is only done for allocations that have a known origin because catching all
    // of these errors seems to expose bugs in system libraries on some platforms.
    if (verifyIsArray && *(reinterpret_cast<bool*>(block + sizeof(size_t) + sizeof(uint64_t))) != isArray)
        reportError("Mismatched array new/delete on allocation at %p", block + PrefixAreaSize);

    return block;
}

size_t MemoryValidator::validateAllocation(const byte_t* block)
{
    auto reportedAddress = block;

    // Move to the true start of the allocation
    block -= PrefixAreaSize;

    // Read the allocation size out of the start of the prefix area
    auto size = *reinterpret_cast<const size_t*>(block);

    // Check the size is at least enough for the prefix and suffix areas
    if (size < PrefixAreaSize + SuffixAreaSize)
        reportError("Allocation at %p has a corrupted size, possibly caused by an underrun", reportedAddress);

    auto reportedSize = size - PrefixAreaSize - SuffixAreaSize;

    // Check that the prefix area hasn't been altered, if it has then there must have been some corruption
    for (auto i = sizeof(size_t) + sizeof(uint64_t) + sizeof(bool); i < PrefixAreaSize; i++)
    {
        if (block[i] != PrefixAreaValue)
        {
            reportError(
                "Allocation at %p of size %llu has a corrupted prefix area of size %u bytes, possibly caused by an underrun",
                reportedAddress, uint64_t(reportedSize), PrefixAreaSize - i);
            break;
        }
    }

    // Check that the suffix area hasn't been altered, if it has then there must have been some corruption
    auto suffixAreaOffset = size - SuffixAreaSize;
    for (auto i = size_t(); i < SuffixAreaSize; i++)
    {
        auto index = SuffixAreaSize - i - 1;
        if (block[suffixAreaOffset + index] != SuffixAreaValue)
        {
            reportError(
                "Allocation at %p of size %llu has a corrupted suffix area of size %u bytes, possibly caused by an overrun",
                reportedAddress, uint64_t(reportedSize), index + 1);
            break;
        }
    }

    return reportedSize;
}

void MemoryValidator::setToRandomData(byte_t* block, size_t size)
{
    for (auto i = size_t(); i < size; i++)
        block[i] = byte_t(Math::random(0, 255));
}

}
