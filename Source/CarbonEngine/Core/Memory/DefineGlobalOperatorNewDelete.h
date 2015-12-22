/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Including this header defines global operator new and delete that pass directly off to MemoryInterceptor, it is not intended
// to be used a standard #include.

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR

#undef new

void* operator new(size_t size)
{
    return Carbon::MemoryInterceptor::allocate(size, 0, false, true);
}

void* operator new[](size_t size)
{
    return Carbon::MemoryInterceptor::allocate(size, 0, true, true);
}

void operator delete(void* p) noexcept
{
    Carbon::MemoryInterceptor::free(p, 0, false);
}

void operator delete[](void* p) noexcept
{
    Carbon::MemoryInterceptor::free(p, 0, true);
}

void* operator new(size_t size, const std::nothrow_t& unused) noexcept
{
    return Carbon::MemoryInterceptor::allocate(size, 0, false, false);
}

void* operator new[](size_t size, const std::nothrow_t& unused) noexcept
{
    return Carbon::MemoryInterceptor::allocate(size, 0, true, false);
}

void operator delete(void* p, const std::nothrow_t& unused) noexcept
{
    Carbon::MemoryInterceptor::free(p, 0, false);
}

void operator delete[](void* p, const std::nothrow_t& unused) noexcept
{
    Carbon::MemoryInterceptor::free(p, 0, true);
}

// Define the sized global operator delete functions introduced in C++14
#if __cplusplus >= 201402L
void operator delete(void* p, std::size_t size)
{
    Carbon::MemoryInterceptor::free(p, 0, false);
}

void operator delete[](void* p, std::size_t size)
{
    Carbon::MemoryInterceptor::free(p, 0, true);
}

void operator delete(void* p, std::size_t size, const std::nothrow_t&) noexcept
{
    Carbon::MemoryInterceptor::free(p, 0, false);
}

void operator delete[](void* p, std::size_t size, const std::nothrow_t&) noexcept
{
    Carbon::MemoryInterceptor::free(p, 0, true);
}
#endif

#endif
