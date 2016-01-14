/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef ANDROID

#include <android/log.h>

#include "CarbonEngine/Core/Memory/MemoryLeakDetector.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/Android/PlatformAndroid.h"

namespace Carbon
{

#ifdef CARBON_INCLUDE_LOGGING

void Globals::debugLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    __android_log_vprint(ANDROID_LOG_DEBUG, Globals::getClientNameBuffer(), format, args);
}

#endif

PlatformAndroid::PlatformAndroid()
{
    setTicksPerSecond(1000000);
}

TimeValue PlatformAndroid::getTime() const
{
    auto now = timespec();
    clock_gettime(CLOCK_MONOTONIC, &now);

    return TimeValue(int64_t(now.tv_sec) * 1000000 + int64_t(now.tv_nsec) / 1000);
}

void MemoryLeakDetector::writeMemoryLeaksReportFile()
{
    // TODO
}

}

#endif
