/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/SharedLibrary.h"

namespace Carbon
{

#ifdef WINDOWS

bool SharedLibrary::load(const UnicodeString& name)
{
    unload();

    library_ = LoadLibraryW(name.toUTF16().as<wchar_t>());
    if (!library_)
        return false;

    name_ = name;

    return true;
}

VoidFunction SharedLibrary::mapFunction(const String& name) const
{
    if (!library_)
        return nullptr;

    return reinterpret_cast<VoidFunction>(GetProcAddress(library_, name.cStr()));
}

void SharedLibrary::unload()
{
    name_.clear();

    FreeLibrary(library_);
    library_ = nullptr;
}

#elif defined(POSIX)

bool SharedLibrary::load(const UnicodeString& name)
{
    unload();

    library_ = dlopen(name.toUTF8().as<char>(), RTLD_LAZY | RTLD_GLOBAL);
    if (!library_)
        return false;

    name_ = name;

    return true;
}

VoidFunction SharedLibrary::mapFunction(const String& name) const
{
    if (!library_)
        return nullptr;

    auto fn = dlsym(library_, name.cStr());

    return reinterpret_cast<VoidFunction>(reinterpret_cast<uintptr_t>(fn));
}

void SharedLibrary::unload()
{
    name_.clear();

    if (library_)
    {
        dlclose(library_);
        library_ = nullptr;
    }
}

#else

bool SharedLibrary::load(const UnicodeString& name)
{
    return false;
}

VoidFunction SharedLibrary::mapFunction(const String& name) const
{
    return nullptr;
}

void SharedLibrary::unload()
{
}

#endif
}
