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
#include "CarbonEngine/Scene/Light.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

namespace Max
{

class SceneExporter : public SceneExport
{
public:

    unsigned int Version() override { return BuildInfo::getVersion().asInteger(); }
    const char* ShortDesc() override { return SceneExporterFileType.cStr(); }
    const char* LongDesc() override { return ShortDesc(); }
    const char* AuthorName() override { return Globals::getDeveloperName().cStr(); }
    const char* CopyrightMessage() override { return ""; }
    const char* OtherMessage1() override { return ""; }
    const char* OtherMessage2() override { return ""; }
    void ShowAbout(HWND hWnd) override {}
    int ExtCount() override { return 1; }

    const char* Ext(int n) override
    {
        static const auto extension = A(Carbon::Scene::SceneExtension.substr(1));

        return extension.cStr();
    }

    BOOL SupportsOptions(int ext, DWORD options) override { return options == SCENE_EXPORT_SELECTED; }

    int DoExport(const char* name, ExpInterface* ei, Interface* pIp, BOOL suppressPrompts, DWORD options) override
    {
        ip = pIp;
        onlyExportSelected = (options & SCENE_EXPORT_SELECTED);

        Globals::initializeEngine(getMaxClientName());

        auto runner =
            SceneExportRunner(fixMaxFilename(name, Ext(0)), GeometryExporter::exportGeometry, nullptr, exportEntities);
        ProgressDialog(SceneExporterTitle).show(runner, ip->GetMAXHWnd());

        Globals::uninitializeEngine();

        return 1;
    }

    static bool exportEntities(Scene& scene, Runnable& r)
    {
        // Export ambient light color if there is one
        auto interval = Interval(::TimeValue(0x80000000), ::TimeValue(0x7fffffff));
        auto ambientLightColor = ::Color(ip->GetAmbient(ip->GetTime(), interval));
        if (ambientLightColor.r > 0.0f || ambientLightColor.g > 0.0f || ambientLightColor.b > 0.0f)
            scene.addEntity<Light>()->setAmbientLight(maxColorToColor(ambientLightColor));

        // Enumerate through scene nodes
        return enumerateNodes(ip->GetRootNode(), scene, r);
    }

    // Enumerates all nodes in the scene starting at the specified node
    static bool enumerateNodes(INode* node, Scene& scene, Runnable& r)
    {
        if (!node || r.isCancelled())
            return false;

        if (!onlyExportSelected || (onlyExportSelected && node->Selected()))
            exportNode(node, scene);

        // Recurse through all child nodes
        for (auto i = 0; i < node->NumberOfChildren(); i++)
        {
            if (!enumerateNodes(node->GetChildNode(i), scene, r))
                return false;
        }

        return true;
    }

    // Gets called for every node in the scene. Currently just exports lights into entities in the final scene.
    static void exportNode(INode* node, Scene& scene)
    {
        // Evaluate node state
        auto currentTime = ip->GetTime();
        auto os = node->EvalWorldState(currentTime);

        if (!os.obj)
            return;

        if (os.obj->SuperClassID() == LIGHT_CLASS_ID)
        {
            // Export light entity

            auto genLight = static_cast<GenLight*>(os.obj);

            // Get light type
            auto lightType = Light::LightType();
            switch (genLight->Type())
            {
                case OMNI_LIGHT:
                    lightType = Light::PointLight;
                    break;

                case DIR_LIGHT:
                    lightType = Light::DirectionalLight;
                    break;

                case TSPOT_LIGHT:
                    lightType = Light::SpotLight;
                    break;

                default:
                    LOG_WARNING_WITHOUT_CALLER << "Skipping unsupported light type on light '" << node->GetName();
                    return;
            }

            auto light = scene.addEntity<Light>();
            light->setType(lightType);

            // Get light color
            auto ls = LightState();
            auto interval = Interval(::TimeValue(0x80000000), ::TimeValue(0x7fffffff));
            genLight->EvalLightState(currentTime, interval, &ls);
            light->setColor(maxColorToColor(ls.color) * ls.intens);

            light->setSpecularEnabled(ls.affectSpecular == TRUE);

            // Get light radius
            if (lightType == Light::PointLight || lightType == Light::SpotLight)
                light->setRadius(ls.attenEnd);

            // Get spotlight properties
            if (lightType == Light::SpotLight)
            {
                light->setMaximumConeAngle(ls.fallsize * 0.5f);
                light->setMinimumConeAngle(ls.fallsize * 0.25f);
            }

            // Get light position and orientation
            light->setWorldTransform(maxMatrix3ToSimpleTransform(node->GetNodeTM(currentTime)));

            // Get whether the light is on
            light->setVisible(ls.on == TRUE);

            LOG_INFO << "Exported light: " << node->GetName();
        }
    }
};

class SceneExporterClassDesc : public ClassDesc2
{
public:

    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading) override { return new SceneExporter; }
    const char* ClassName() override { return "SceneExporterClassDesc"; }
    SClass_ID SuperClassID() override { return SCENE_EXPORT_CLASS_ID; }
    Class_ID ClassID() override { return Class_ID(0x248725c1, 0x367466a8); }
    const char* Category() override { return ""; }
    const char* InternalName() override { return "CarbonSceneExporter"; }
    HINSTANCE HInstance() override { return Globals::getHInstance(); }
};

ClassDesc* getSceneExporterClassDesc()
{
    static auto desc = SceneExporterClassDesc();
    return &desc;
}

}

}

#endif
