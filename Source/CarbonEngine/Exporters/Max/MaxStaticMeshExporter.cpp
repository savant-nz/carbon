/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAX_EXPORTER

#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/Exporters/ExportRunners.h"
#include "CarbonEngine/Exporters/Max/MaxGeometryExporter.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"

namespace Carbon
{

namespace Max
{

class StaticMeshExporter : public SceneExport
{
public:

    unsigned int Version() override { return BuildInfo::getVersion().asInteger(); }
    const char* ShortDesc() override { return StaticMeshExporterFileType.cStr(); }
    const char* LongDesc() override { return ShortDesc(); }
    const char* AuthorName() override { return Globals::getDeveloperName().cStr(); }
    const char* CopyrightMessage() override { return ""; }
    const char* OtherMessage1() override { return ""; }
    const char* OtherMessage2() override { return ""; }
    void ShowAbout(HWND hWnd) override {}
    int ExtCount() override { return 1; }

    const char* Ext(int n) override
    {
        static const auto extension = A(Mesh::MeshExtension.substr(1));

        return extension.cStr();
    }

    // Returns whether we support the extended exporter options. There is only one option currently.
    virtual BOOL SupportsOptions(int ext, DWORD options) override { return options == SCENE_EXPORT_SELECTED; }

    int DoExport(const char* name, ExpInterface* ei, Interface* pIp, BOOL suppressPrompts, DWORD options) override
    {
        ip = pIp;
        onlyExportSelected = (options & SCENE_EXPORT_SELECTED);

        Globals::initializeEngine(getMaxClientName());

        auto runner = MeshExportRunner(fixMaxFilename(name, Ext(0)), GeometryExporter::exportGeometry);
        ProgressDialog(StaticMeshExporterTitle).show(runner, ip->GetMAXHWnd());

        Globals::uninitializeEngine();

        return 1;
    }
};

class StaticMeshExporterClassDesc : public ClassDesc2
{
public:

    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading) override { return new StaticMeshExporter; }
    const char* ClassName() override { return "StaticMeshExporterClassDesc"; }
    SClass_ID SuperClassID() override { return SCENE_EXPORT_CLASS_ID; }
    Class_ID ClassID() override { return Class_ID(0x7e041d38, 0x307652da); }
    const char* Category() override { return ""; }
    const char* InternalName() override { return "CarbonStaticMeshExporter"; }
    HINSTANCE HInstance() override { return Globals::getHInstance(); }
};

ClassDesc* getStaticMeshExporterClassDesc()
{
    static auto desc = StaticMeshExporterClassDesc();
    return &desc;
}

}

}

#endif
