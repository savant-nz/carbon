/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This class handles loading dynamic libraries and mapping the functions they export, supports DLLs on Windows, DyLibs on Mac
 * OS X and SOs on Linux.
 */
class CARBON_API SharedLibrary : private Noncopyable
{
public:

    SharedLibrary() {}

    /**
     * Copy constructor (not implemented).
     */
    SharedLibrary(const SharedLibrary& other);

    ~SharedLibrary() { unload(); }

    /**
     * Loads the given dynamic library. Returns success flag.
     */
    bool load(const UnicodeString& name);

    /**
     * Returns the address of a function in the loaded shared library. Returns null if the given function isn't present or no
     * library is currently loaded.
     */
    VoidFunction mapFunction(const String& name) const;

    /**
     * Returns the address of a function in the loaded shared library typecast to the specified function type. Returns null if
     * the given function isn't present or no library is currently loaded.
     */
    template <typename FunctionType> FunctionType mapFunction(const String& name) const
    {
        return reinterpret_cast<FunctionType>(mapFunction(name));
    }

    /**
     * Unloads the current shared library.
     */
    void unload();

    /**
     * Returns the name of the currently loaded shared library, or an empty string if none is currently loaded.
     */
    const UnicodeString& getName() const { return name_; }

private:

    UnicodeString name_;

#ifdef WINDOWS
    HMODULE library_ = nullptr;
#else
    void* library_ = nullptr;
#endif
};

}
