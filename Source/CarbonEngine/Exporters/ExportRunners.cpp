/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Exporters/ExportRunners.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Scene/ABTCompiler.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Mesh/MeshManager.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

SceneExportRunner::SceneExportRunner(UnicodeString filename, ExportTrianglesFunction fnExportTriangles,
                                     ExportMaterialsFunction fnExportMaterials, ExportEntitiesFunction fnExportEntities)
    : filename_(std::move(filename)),
      fnExportTriangles_(fnExportTriangles),
      fnExportMaterials_(fnExportMaterials),
      fnExportEntities_(fnExportEntities)
{
}

bool SceneExportRunner::run()
{
    // Call function to export triangle data
    beginTask("Exporting triangles", 2);
    auto triangleSet = TriangleArraySet();
    if (!fnExportTriangles_(triangleSet, *this))
    {
        if (!isCancelled())
            LOG_ERROR_WITHOUT_CALLER << "Failed exporting triangles";

        return false;
    }
    LOG_INFO << "Exported " << triangleSet.getTriangleCount() << " triangles";
    endTask();

    // Compile scene
    beginTask("Compiling", 95);
    auto scene = Scene();
    if (!ABTCompiler::compile(scene, triangleSet, *this))
    {
        if (!isCancelled())
            LOG_ERROR_WITHOUT_CALLER << "Failed compiling scene";

        return false;
    }

    // Export entities
    if (fnExportEntities_ && !fnExportEntities_(scene, *this))
    {
        if (!isCancelled())
            LOG_ERROR_WITHOUT_CALLER << "Failed exporting scene entities";

        return false;
    }

    endTask();

    // Set a sensible scene name from the given filename
    auto name = filename_;
    auto index = name.findLastOf("/\\");
    if (index != -1)
        name = name.substr(index + 1);
    index = name.findLastOf(UnicodeString::Period);
    if (index != -1)
        name = name.substr(0, index);
    scene.setName(A(name));

    // Save scene
    beginTask("Saving file", 3);
    try
    {
        auto file = FileWriter(filename_);
        scene.save(file);
    }
    catch (const Exception& e)
    {
        LOG_ERROR_WITHOUT_CALLER << "Failed saving scene file - " << e;
        return false;
    }
    endTask();

    if (fnExportMaterials_)
        fnExportMaterials_(triangleSet, FileSystem::getDirectory(filename_));

    return true;
}

MeshExportRunner::MeshExportRunner(UnicodeString filename, ExportTrianglesFunction fnExportTriangles,
                                   ExportMaterialsFunction fnExportMaterials)
    : filename_(std::move(filename)), fnExportTriangles_(fnExportTriangles), fnExportMaterials_(fnExportMaterials)
{
}

bool MeshExportRunner::run()
{
    // Call function to export triangle data
    beginTask("Exporting triangles", 3);
    auto triangleSet = TriangleArraySet();
    if (!fnExportTriangles_(triangleSet, *this))
    {
        if (!isCancelled())
            LOG_ERROR_WITHOUT_CALLER << "Failed exporting triangles";

        return false;
    }
    if (triangleSet.empty())
    {
        LOG_ERROR_WITHOUT_CALLER << "No triangles were found to export";
        return false;
    }
    LOG_INFO << "Exported " << triangleSet.getTriangleCount() << " triangles";
    endTask();

    // Compile mesh
    beginTask("Compiling", 95);
    auto mesh = meshes().createMesh();
    if (!mesh->setupFromTriangles(triangleSet, *this))
    {
        if (!isCancelled())
            LOG_ERROR_WITHOUT_CALLER << "Failed compiling mesh";

        meshes().releaseMesh(mesh);
        return false;
    }
    endTask();

    // Save mesh
    beginTask("Saving file", 2);
    try
    {
        auto file = FileWriter(filename_);
        mesh->save(file);
    }
    catch (const Exception& e)
    {
        meshes().releaseMesh(mesh);

        LOG_ERROR_WITHOUT_CALLER << "Failed saving mesh file - " << e;
        return false;
    }
    endTask();

    meshes().releaseMesh(mesh);

    if (fnExportMaterials_)
        fnExportMaterials_(triangleSet, FileSystem::getDirectory(filename_));

    return true;
}

}
