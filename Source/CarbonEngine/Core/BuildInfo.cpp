/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/BuildInfo.h"

namespace Carbon
{

Vector<String> BuildInfo::getBuildInfo()
{
    auto result = Vector<String>();

#ifdef WINDOWS
    result.append("Platform:           Windows");
#elif defined(ANDROID)
    result.append("Platform:           Android");
#elif defined(LINUX)
    result.append("Platform:           Linux");
#elif defined(iOS)
    result.append("Platform:           iOS");
#elif defined(MACOSX)
    result.append("Platform:           Mac OS X");
#elif defined(CONSOLE)
    result.append("Platform:           Console");
#else
    result.append("Platform:           Unknown");
#endif

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
    result.append("Architecture:       x64");
#elif defined(__arm64__) || defined(__aarch64__)
    result.append("Architecture:       ARM64");
#elif defined(__ppc__) || defined(__PPC__) || defined(_M_PPC)
    result.append("Architecture:       PowerPC");
#elif defined(__mips__) || defined(__mips)
    result.append("Architecture:       MIPS");
#elif defined(__i386__) || defined(_M_IX86)
    result.append("Architecture:       x86");
#elif defined(__ARM_ARCH_7A__)
    result.append("Architecture:       ARMv7");
#elif defined(__arm__)
    result.append("Architecture:       ARM");
#else
    result.append("Architecture:       Unknown");
#endif

#ifdef _MSC_VER
    result.append("Compiler:           Visual C++ " CARBON_MSVC_VERSION);
    result.append("MSC Version:        ");
    result.back() << _MSC_FULL_VER;
#elif defined(__SNC__)
    result.append("Compiler:           SNC " + String(__SN_VER__));
#elif defined(__clang__)
    result.append("Compiler:           Clang " __clang_version__);
#elif defined(__GNUC__)
    result.append("Compiler:           GCC " __VERSION__);
#elif defined(__MWERKS__)
    result.append("Compiler:           Metrowerks " + String(__MWERKS__));
#else
    result.append("Compiler:           Unknown");
#endif

#ifdef DEBUG
    result.append("Build Type:         Debug");
#else
    result.append("Build Type:         Release");
#endif

    result.append("Version:            ");
    result.back() << getVersion();

    result.append("");

    // Log included and excluded defines
    Vector<String> includedDefines, excludedDefines;
#ifdef CARBON_STATIC_LIBRARY
    includedDefines.append("CARBON_STATIC_LIBRARY");
#else
    excludedDefines.append("CARBON_STATIC_LIBRARY");
#endif
#ifdef CARBON_DEBUG
    includedDefines.append("CARBON_DEBUG");
#else
    excludedDefines.append("CARBON_DEBUG");
#endif
#ifdef CARBON_BIG_ENDIAN
    includedDefines.append("CARBON_BIG_ENDIAN");
#else
    excludedDefines.append("CARBON_BIG_ENDIAN");
#endif
#ifdef CARBON_LITTLE_ENDIAN
    includedDefines.append("CARBON_LITTLE_ENDIAN");
#else
    excludedDefines.append("CARBON_LITTLE_ENDIAN");
#endif
#ifdef CARBON_INCLUDE_ANGELSCRIPT
    includedDefines.append("CARBON_INCLUDE_ANGELSCRIPT");
#else
    excludedDefines.append("CARBON_INCLUDE_ANGELSCRIPT");
#endif
#ifdef CARBON_INCLUDE_BULLET
    includedDefines.append("CARBON_INCLUDE_BULLET");
#else
    excludedDefines.append("CARBON_INCLUDE_BULLET");
#endif
#ifdef CARBON_INCLUDE_CONSOLE_COMMANDS
    includedDefines.append("CARBON_INCLUDE_CONSOLE_COMMANDS");
#else
    excludedDefines.append("CARBON_INCLUDE_CONSOLE_COMMANDS");
#endif
#ifdef CARBON_INCLUDE_FREEIMAGE
    includedDefines.append("CARBON_INCLUDE_FREEIMAGE");
#else
    excludedDefines.append("CARBON_INCLUDE_FREEIMAGE");
#endif
#ifdef CARBON_INCLUDE_FREETYPE
    includedDefines.append("CARBON_INCLUDE_FREETYPE");
#else
    excludedDefines.append("CARBON_INCLUDE_FREETYPE");
#endif

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    includedDefines.append("CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS");
#else
    excludedDefines.append("CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS");
#endif
#ifdef CARBON_INCLUDE_LOGGING
    includedDefines.append("CARBON_INCLUDE_LOGGING");
#else
    excludedDefines.append("CARBON_INCLUDE_LOGGING");
#endif
#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    includedDefines.append("CARBON_INCLUDE_MEMORY_INTERCEPTOR");
#else
    excludedDefines.append("CARBON_INCLUDE_MEMORY_INTERCEPTOR");
#endif
#ifdef CARBON_INCLUDE_MAX_EXPORTER
    includedDefines.append("CARBON_INCLUDE_MAX_EXPORTER");
#else
    excludedDefines.append("CARBON_INCLUDE_MAX_EXPORTER");
#endif
#ifdef CARBON_INCLUDE_MAYA_EXPORTER
    includedDefines.append("CARBON_INCLUDE_MAYA_EXPORTER");
#else
    excludedDefines.append("CARBON_INCLUDE_MAYA_EXPORTER");
#endif
#ifdef CARBON_INCLUDE_OPENAL
    includedDefines.append("CARBON_INCLUDE_OPENAL");
#else
    excludedDefines.append("CARBON_INCLUDE_OPENAL");
#endif
#ifdef CARBON_INCLUDE_OPENASSETIMPORT
    includedDefines.append("CARBON_INCLUDE_OPENASSETIMPORT");
#else
    excludedDefines.append("CARBON_INCLUDE_OPENASSETIMPORT");
#endif
#ifdef CARBON_INCLUDE_OCULUSRIFT
    includedDefines.append("CARBON_INCLUDE_OCULUSRIFT");
#else
    excludedDefines.append("CARBON_INCLUDE_OCULUSRIFT");
#endif
#ifdef CARBON_INCLUDE_OPENGL11
    includedDefines.append("CARBON_INCLUDE_OPENGL11");
#else
    excludedDefines.append("CARBON_INCLUDE_OPENGL11");
#endif
#ifdef CARBON_INCLUDE_OPENGL41
    includedDefines.append("CARBON_INCLUDE_OPENGL41");
#else
    excludedDefines.append("CARBON_INCLUDE_OPENGL41");
#endif
#ifdef CARBON_INCLUDE_OPENGLES2
    includedDefines.append("CARBON_INCLUDE_OPENGLES2");
#else
    excludedDefines.append("CARBON_INCLUDE_OPENGLES2");
#endif
#ifdef CARBON_INCLUDE_PHYSX
    includedDefines.append("CARBON_INCLUDE_PHYSX");
#else
    excludedDefines.append("CARBON_INCLUDE_PHYSX");
#endif
#ifdef CARBON_INCLUDE_PLATFORM_MACOSX
    includedDefines.append("CARBON_INCLUDE_PLATFORM_MACOSX");
#else
    excludedDefines.append("CARBON_INCLUDE_PLATFORM_MACOSX");
#endif
#ifdef CARBON_INCLUDE_PLATFORM_SDL
    includedDefines.append("CARBON_INCLUDE_PLATFORM_SDL");
#else
    excludedDefines.append("CARBON_INCLUDE_PLATFORM_SDL");
#endif
#ifdef CARBON_INCLUDE_PLATFORM_WINDOWS
    includedDefines.append("CARBON_INCLUDE_PLATFORM_WINDOWS");
#else
    excludedDefines.append("CARBON_INCLUDE_PLATFORM_WINDOWS");
#endif
#ifdef CARBON_INCLUDE_VORBIS
    includedDefines.append("CARBON_INCLUDE_VORBIS");
#else
    excludedDefines.append("CARBON_INCLUDE_VORBIS");
#endif
#ifdef CARBON_INCLUDE_ZLIB
    includedDefines.append("CARBON_INCLUDE_ZLIB");
#else
    excludedDefines.append("CARBON_INCLUDE_ZLIB");
#endif

    result.append("Included defines:");
    result.append(includedDefines);
    result.append("");
    result.append("Excluded defines:");
    result.append(excludedDefines);

    return result;
}

String BuildInfo::getVersion()
{
#ifdef CARBON_VERSION
    return CARBON_QUOTE_MACRO_VALUE(CARBON_VERSION);
#else
    return "Unknown";
#endif
}

bool BuildInfo::isExporterBuild()
{
#if defined(CARBON_INCLUDE_MAX_EXPORTER) || defined(CARBON_INCLUDE_MAYA_EXPORTER)
    return true;
#else
    return false;
#endif
}

}
