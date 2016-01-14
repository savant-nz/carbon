/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/Exporters/ExportRunners.h"
#include "CarbonEngine/Exporters/Maya/MayaGeometryHelper.h"
#include "CarbonEngine/Exporters/Maya/MayaHelper.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"

namespace Carbon
{

namespace Maya
{

class StaticMeshExporter : public MPxFileTranslator
{
public:

    bool canBeOpened() const override { return true; }
    bool haveReadMethod() const override { return false; }
    bool haveWriteMethod() const override { return true; }

    MString defaultExtension() const override { return toMString(Mesh::MeshExtension.substr(1)); }

    MString filter() const override { return toMString("*" + Mesh::MeshExtension); }

    MPxFileTranslator::MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const override
    {
        if (MStringToString(fileName.name()).asLower().endsWith(Mesh::MeshExtension))
            return kIsMyFileType;

        return kNotMyFileType;
    }

    MStatus writer(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override
    {
        onlyExportSelected = (mode == kExportActiveAccessMode);

        Globals::initializeEngine(getMayaClientName());

        auto runner = MeshExportRunner(fromUTF8(file.fullName().asChar()), extractAllMeshes, Helper::exportAllMaterials);

        ProgressDialog(StaticMeshExporterTitle).show(runner, M3dView::applicationShell());

        Globals::uninitializeEngine();

        return MS::kSuccess;
    }

    static bool extractAllMeshes(TriangleArraySet& triangleSet, Runnable& r)
    {
        return GeometryHelper::extractAllMeshes(triangleSet, r);
    }
};

void* createStaticMeshExporter()
{
    return new StaticMeshExporter;
}

}

}

#endif
