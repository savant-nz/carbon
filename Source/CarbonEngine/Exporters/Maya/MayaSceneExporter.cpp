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
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Scene/Light.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/Terrain.h"

namespace Carbon
{

namespace Maya
{

class SceneExporter : public MPxFileTranslator
{
public:

    bool canBeOpened() const override { return true; }
    bool haveReadMethod() const override { return false; }
    bool haveWriteMethod() const override { return true; }

    MString defaultExtension() const override { return toMString(Scene::SceneExtension.substr(1).cStr()); }

    MString filter() const override { return toMString("*" + Scene::SceneExtension); }

    MPxFileTranslator::MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const override
    {
        if (MStringToString(fileName.name()).asLower().endsWith(Scene::SceneExtension))
            return kIsMyFileType;

        return kNotMyFileType;
    }

    MStatus writer(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override
    {
        onlyExportSelected = (mode == kExportActiveAccessMode);

        Globals::initializeEngine(getMayaClientName());

        heightmapDagPaths.clear();

        auto runner =
            SceneExportRunner(MStringToString(file.fullName()), extractAllMeshes, Helper::exportAllMaterials, exportEntites);
        ProgressDialog(SceneExporterTitle).show(runner, M3dView::applicationShell());

        Globals::uninitializeEngine();

        return MS::kSuccess;
    }

    static MDagPathArray heightmapDagPaths;

    // Extract all meshes in the Maya scene, with a callback to leave out any heightmaps
    static bool extractAllMeshes(TriangleArraySet& triangleSet, Runnable& r)
    {
        return GeometryHelper::extractAllMeshes(triangleSet, r, extractAllMeshesCallback);
    }

    static bool extractAllMeshesCallback(const MDagPath& dagPath)
    {
        auto name = String(dagPath.partialPathName().asChar());

        if (name.startsWith("heightmap_"))
        {
            heightmapDagPaths.append(dagPath);
            return false;
        }

        return true;
    }

    // Exports extra Maya constructs as entities in the exported scene.
    static bool exportEntites(Scene& scene, Runnable& runnable)
    {
        exportLocatorsAsNamedEntities(scene);
        exportLights(scene);

        return true;
    }

    // Exports Maya locators as Carbon::Entity instances in the exported scene.
    static void exportLocatorsAsNamedEntities(Scene& scene)
    {
        auto paths = MDagPathArray();
        Helper::getExportObjects(paths, MFn::kLocator);

        for (auto i = 0U; i < paths.length(); i++)
        {
            // Create entity for this locator and give it a name and transform

            auto name = String(MFnTransform(paths[i].transform()).name().asChar());
            name.replace("__", "/");

            auto entity = scene.addEntity<Entity>(name);
            Helper::getTransformAtDagPath(paths[i], *entity);

            LOG_INFO << "Exported locator '" << entity->getName() << "' at position " << entity->getLocalPosition()
                     << " and orientation " << entity->getLocalOrientation();
        }
    }

    // Exports Maya lights as Carbon::Light instances in the exported scene.
    static void exportLights(Scene& scene)
    {
        auto paths = MDagPathArray();
        Helper::getExportObjects(paths, MFn::kLight);

        for (auto i = 0U; i < paths.length(); i++)
        {
            MFnLight fnLight(paths[i]);

            // Create a new light
            auto light = scene.addEntity<Light>(fnLight.name().asChar());

            // Set the basic light properties
            light->setColor(MColorToColor(fnLight.color()));
            light->setSpecularEnabled(fnLight.lightSpecular());

            Helper::getTransformAtDagPath(paths[i], *light);

            // Correct for the fact that the Light class points spotlights down +Z but Maya is down -Z
            light->rotate(Quaternion::createFromAxisAngle(light->getWorldOrientation().getYVector(), Math::Pi));

            // The Maya light intensity is currently mapped directly to the radius. Maya lights use unbounded falloff and so
            // there is no direct radius value on the lights. It may be better to set the distance at which the Maya light is at
            // 5% of its maximum brightness as the exported radius, but doing that may not be very fantastic either.
            light->setRadius(fnLight.intensity());

            // Set the light type and export any light-type specific properties
            switch (paths[i].apiType())
            {
                case MFn::kAmbientLight:
                    light->setType(Light::AmbientLight);
                    break;

                case MFn::kDirectionalLight:
                    light->setType(Light::DirectionalLight);
                    break;

                case MFn::kPointLight:
                    light->setType(Light::PointLight);
                    break;

                case MFn::kSpotLight:
                {
                    MFnSpotLight fnSpotLight(paths[i]);

                    light->setType(Light::SpotLight);
                    light->setMaximumConeAngle(float(fnSpotLight.coneAngle()));
                    light->setMinimumConeAngle(light->getMaximumConeAngle() * 0.5f);

                    break;
                }

                default:
                    LOG_INFO << "Unsupported light type: " << light->getName();
                    break;
            }

            LOG_INFO << "Exported light: " << light->getName();
        }
    }
};

MDagPathArray SceneExporter::heightmapDagPaths;

void* createSceneExporter()
{
    return new SceneExporter;
}

}

}

#endif
