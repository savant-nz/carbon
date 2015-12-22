/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/Exporters/Maya/MayaPlugin.h"
#include "CarbonEngine/Math/Matrix3.h"
#include <maya/MFnPlugin.h>

#if MAYA_API_VERSION >= 201400 && !defined(CARBON_64BIT)
    #error This version of Maya does not support 32-bit platforms
#endif

#if MAYA_API_VERSION < 200900 && defined(CARBON_64BIT)
    #error This version of Maya does not support 64-bit platforms
#endif

#ifdef _MSC_VER
    #pragma comment(lib, "Foundation.lib")
    #pragma comment(lib, "OpenMaya.lib")
    #pragma comment(lib, "OpenMayaAnim.lib")
    #pragma comment(lib, "OpenMayaUI.lib")
#endif

namespace Carbon
{

namespace Maya
{

bool onlyExportSelected;

extern void* createStaticMeshExporter();
extern void* createSceneExporter();
extern void* createSkeletalAnimationExporter();
extern void* createSkeletalMeshExporter();

String getMayaClientName()
{
    auto name = "CarbonExporterMaya" + String(MAYA_API_VERSION / 100);

#ifdef CARBON_64BIT
    name << "64";
#endif

    return name;
}

// Maya type conversion functions
Vec3 MPointToVec3(const MPoint& p)
{
    return {float(p.x), float(p.y), float(p.z)};
}

Vec3 MVectorToVec3(const MVector& v)
{
    return {float(v.x), float(v.y), float(v.z)};
}

Quaternion MQuaternionToQuaternion(const MQuaternion& mq)
{
    return {float(mq.x), float(mq.y), float(mq.z), float(mq.w)};
}

SimpleTransform MMatrixToAffineTransform(const MMatrix& matrix)
{
    return {Vec3(float(matrix[3][0]), float(matrix[3][1]), float(matrix[3][2])),
            Quaternion::createFromRotationMatrix({float(matrix[0][0]), float(matrix[1][0]), float(matrix[2][0]),
                                                  float(matrix[0][1]), float(matrix[1][1]), float(matrix[2][1]),
                                                  float(matrix[0][2]), float(matrix[1][2]), float(matrix[2][2])})};
}

Color MColorToColor(const MColor& color)
{
    return {color.r, color.g, color.b, color.a};
}

UnicodeString MStringToString(const MString& string)
{
    return fromUTF8(string.asUTF8());
}

MString toMString(const UnicodeString& string)
{
    return reinterpret_cast<const wchar_t*>(string.toUTF16().getData());
}

}

}

using namespace Carbon;

CARBON_EXPORTER_API MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, Globals::getDeveloperName().cStr(), BuildInfo::getVersion().cStr());

    static auto icon = std::array<char, 1>();

    // Register the exporters with Maya
    if (plugin.registerFileTranslator(StaticMeshExporterFileType.cStr(), icon.data(), Maya::createStaticMeshExporter) &&
        plugin.registerFileTranslator(SceneExporterFileType.cStr(), icon.data(), Maya::createSceneExporter) &&
        plugin.registerFileTranslator(SkeletalAnimationExporterFileType.cStr(), icon.data(),
                                      Maya::createSkeletalAnimationExporter) &&
        plugin.registerFileTranslator(SkeletalMeshExporterFileType.cStr(), icon.data(), Maya::createSkeletalMeshExporter))
        return MS::kSuccess;
    else
        return MS::kFailure;
}

CARBON_EXPORTER_API MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, Globals::getDeveloperName().cStr(), BuildInfo::getVersion().cStr());

    // Remove the exporters from Maya
    if (plugin.deregisterFileTranslator(StaticMeshExporterFileType.cStr()) &&
        plugin.deregisterFileTranslator(SceneExporterFileType.cStr()) &&
        plugin.deregisterFileTranslator(SkeletalAnimationExporterFileType.cStr()) &&
        plugin.deregisterFileTranslator(SkeletalMeshExporterFileType.cStr()))
        return MS::kSuccess;
    else
        return MS::kFailure;
}

#endif
