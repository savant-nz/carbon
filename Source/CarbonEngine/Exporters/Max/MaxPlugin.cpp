/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAX_EXPORTER

#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Exporters/ExportInfo.h"
#include "CarbonEngine/Exporters/Max/MaxPlugin.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/Matrix3.h"

#if MAX_PRODUCT_VERSION_MAJOR >= 2013 && !defined(CARBON_64BIT)
    #error This version of Max does not support 32-bit platforms
#endif

#if MAX_PRODUCT_VERSION_MAJOR <= 9 && defined(CARBON_64BIT)
    #error This version of Max does not support 64-bit platforms
#endif

namespace Carbon
{

namespace Max
{

Interface* ip;
bool onlyExportSelected;

// Declare ClassDesc retrieval functions for the exporter classes
extern ClassDesc* getSceneExporterClassDesc();
extern ClassDesc* getSkeletalAnimationExporterClassDesc();
extern ClassDesc* getSkeletalMeshExporterClassDesc();
extern ClassDesc* getStaticMeshExporterClassDesc();

Vec3 maxPoint3ToVec3(::Point3 p)
{
    return {p.x, p.y, p.z};
}

SimpleTransform maxMatrix3ToSimpleTransform(const ::Matrix3 mm)
{
    return {maxPoint3ToVec3(mm.GetTrans()),
            Quaternion::createFromRotationMatrix(
                Matrix3(mm[0].x, mm[1].x, mm[2].x, mm[0].y, mm[1].y, mm[2].y, mm[0].z, mm[1].z, mm[2].z))};
}

Color maxColorToColor(::Color color)
{
    return {color.r, color.g, color.b};
}

String fixMaxFilename(const String& filename, const String& extension)
{
    if (filename.length() < extension.length())
        return filename;

    if (filename.substr(filename.length() - extension.length()).asLower() != extension.asLower())
        return filename;

    return filename.substr(0, filename.length() - extension.length()) + extension;
}

String getMaxClientName()
{
    auto name = String("CarbonExporterMax");

    name += MAX_PRODUCT_VERSION_MAJOR;

#ifdef CARBON_64BIT
    name += "64";
#endif

    return name;
}

}

}

using namespace Carbon;

extern "C" CARBON_EXPORTER_API const char* LibDescription()
{
    static const auto description = A(ExportInfo::get());

    return description.cStr();
}

extern "C" CARBON_EXPORTER_API int LibNumberClasses()
{
    return 4;
}

extern "C" CARBON_EXPORTER_API ClassDesc* LibClassDesc(int i)
{
    switch (i)
    {
        case 0:
            return Max::getSceneExporterClassDesc();
        case 1:
            return Max::getSkeletalAnimationExporterClassDesc();
        case 2:
            return Max::getSkeletalMeshExporterClassDesc();
        case 3:
            return Max::getStaticMeshExporterClassDesc();
        default:
            return nullptr;
    }
}

extern "C" CARBON_EXPORTER_API ULONG LibVersion()
{
    return VERSION_3DSMAX;
}

#endif
