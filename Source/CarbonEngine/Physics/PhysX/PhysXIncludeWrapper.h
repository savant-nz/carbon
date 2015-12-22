/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef __GNUC__
    #pragma GCC system_header
#endif

// PhysX requires _DEBUG, DEBUG is not enough
#if !defined(_DEBUG) && !defined(NDEBUG) && defined(DEBUG)
    #define _DEBUG
#endif

#undef new

using std::isfinite;

#define PX_EXTENSIONS_API
#include <PxPhysicsAPI.h>

#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
