/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file
 *
 * This is a special header file that defines the application entry point for the current platform. It requires that the
 * `CARBON_ENTRY_POINT_CLASS` macro is set to the application's primary class and it must be included in exactly one of the
 * application's source files.
 */

#ifndef DOXYGEN

#ifndef CARBON_ENTRY_POINT_CLASS
    #error CARBON_ENTRY_POINT_CLASS must be defined before including CarbonEngine/EntryPoint.h
#endif

#ifndef iOS

namespace Carbon
{

// This is the bulk of the entry point function on all platforms except iOS, it calls core engine setup and passes control to
// the specified application-defined class.
static int main()
{
    if (!Globals::initializeEngine(CARBON_QUOTE_MACRO_VALUE(CARBON_ENTRY_POINT_CLASS)))
        return 1;

    // Instantiate and run the application
    if (!CARBON_ENTRY_POINT_CLASS().run())
    {
        // Ensure the exit code is non zero if Application::run() failed
        if (Globals::getExitCode() == 0)
            Globals::setExitCode(1);
    }

    Globals::uninitializeEngine();

    return Globals::getExitCode();
}

}

#endif

#ifdef WINDOWS

// Use wWinMain() on Windows
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
    Carbon::Globals::setInStaticInitialization(false);
    Carbon::Globals::setCommandLineParameters(fromUTF16(lpCmdLine));
    return Carbon::main();
}

#elif defined(LINUX)

// Use main() on Linux
int main(int argc, const char* argv[])
{
    Carbon::Globals::setInStaticInitialization(false);
    Carbon::Globals::setCommandLineParameters(argc, argv);
    return Carbon::main();
}

#elif defined(MACOSX)

namespace Carbon
{

extern CARBON_API int runMacOSXApplication(const std::function<int()>& fnMain, const UnicodeString& applicationName);

}

int main(int argc, const char* argv[])
{
    Carbon::Globals::setInStaticInitialization(false);
    Carbon::Globals::setCommandLineParameters(argc, argv);
    return Carbon::runMacOSXApplication(Carbon::main, CARBON_QUOTE_MACRO_VALUE(CARBON_ENTRY_POINT_CLASS));
}

#elif defined(iOS)

namespace Carbon
{

String iOSGetApplicationName();
Application* iOSCreateApplication();

String iOSGetApplicationName()
{
    return CARBON_QUOTE_MACRO_VALUE(CARBON_ENTRY_POINT_CLASS);
}

Application* iOSCreateApplication()
{
    return new CARBON_ENTRY_POINT_CLASS;
}

}

#else

// External platform modules can hook in their application entry point here

#endif

// Add operator new and delete overloads directly into the application binary and then restore the operator new macro
#if defined(CARBON_INCLUDE_MEMORY_INTERCEPTOR) && !defined(CARBON_STATIC_LIBRARY)
    #include "CarbonEngine/Core/Memory/DefineGlobalOperatorNewDelete.h"
    #include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
#endif

#endif
