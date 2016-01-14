/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// The Max headers trigger a number of warnings which are disabled briefly while they are included
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4100)    // unreferenced formal parameter
    #pragma warning(disable : 4201)    // nonstandard extension used : nameless struct/union
    #pragma warning(disable : 4238)    // nonstandard extension used : class rvalue used as lvalue
    #pragma warning(disable : 4239)    // nonstandard extension used : 'token' : conversion from 'type' to 'type'
    #pragma warning(disable : 4245)    // conversion from 'type1' to 'type2', signed/unsigned mismatch
    #pragma warning(disable : 4996)    // 'function' was declared deprecated

    #pragma conform(forScope, push)
    #pragma conform(forScope, off)
#endif

// The Max headers require a global min() function to be defined
template <typename T> static const T& min(const T& a, const T& b)
{
    return a < b ? a : b;
}

// Include Max headers and libraries
#include <CS/BipExp.H>
#include <CS/PhyExp.H>
#include <iparamm2.h>
#include <iskin.h>
#include <max.h>
#include <MeshNormalSpec.h>
#ifdef _MSC_VER
    #pragma comment(lib, "core.lib")
    #pragma comment(lib, "geom.lib")
    #pragma comment(lib, "maxutil.lib")
    #pragma comment(lib, "mesh.lib")
    #pragma comment(lib, "Paramblk2.lib")
#endif

// Restore compiler state
#ifdef _MSC_VER
    #pragma warning(pop)
    #pragma conform(forScope, pop)
#endif

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

namespace Max
{

extern Interface* ip;
extern bool onlyExportSelected;

/**
 * Converts a Max Point3 to a native Vec3.
 */
extern Vec3 maxPoint3ToVec3(::Point3 p);

/**
 * Converts a Max Matrix3 to a native SimpleTransform.
 */
extern SimpleTransform maxMatrix3ToSimpleTransform(::Matrix3 matrix);

/**
 * Converts a Max Color to a native Color.
 */
extern Color maxColorToColor(::Color color);

/**
 * This function fixes Max's habit of uppercasing the file extension. Parameters are the export filename received from Max and
 * the extension being used for the export.
 */
extern String fixMaxFilename(const String& filename, const String& extension);

/**
 * Returns the client name for the Max exporters to pass to Globals::initializeEngine() when starting an export.
 */
extern String getMaxClientName();

}

}
