/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Runnable.h"

namespace Carbon
{

/**
 * Typedef for the triangle exporting function passed to the export runner classes below. It takes a TriangleSet to output the
 * result into and a Runnable which is used to check the cancelled state. Returns success flag.
 */
typedef std::function<bool(TriangleArraySet&, Runnable&)> ExportTrianglesFunction;

/**
 * Typedef for the material exporting function passed to the export runner classes below. It exports all the materials
 * referenced by the triangle set into real material files on disk in the given output directory.
 */
typedef std::function<void(const TriangleArraySet&, const UnicodeString& outputDirectory)> ExportMaterialsFunction;

/**
 * Runnable class for a Scene export.
 */
class CARBON_API SceneExportRunner : public Runnable
{
public:

    /**
     * Typedef for the entity exporting function passed to the constructor. This function is responsible for exporting all
     * non-geometric entities into the under construction Scene object.
     */
    typedef std::function<bool(Scene&, Runnable&)> ExportEntitiesFunction;

    SceneExportRunner(const SceneExportRunner&);

    SceneExportRunner(UnicodeString filename, ExportTrianglesFunction fnExportTriangles,
                      ExportMaterialsFunction fnExportMaterials = nullptr, ExportEntitiesFunction fnExportEntities = nullptr);

    bool run() override;

private:

    const UnicodeString filename_;
    ExportTrianglesFunction fnExportTriangles_;
    ExportMaterialsFunction fnExportMaterials_;
    ExportEntitiesFunction fnExportEntities_;
};

/**
 * Runnable class for a Mesh export.
 */
class CARBON_API MeshExportRunner : public Runnable
{
public:

    MeshExportRunner(const MeshExportRunner&);

    MeshExportRunner(UnicodeString filename, ExportTrianglesFunction fnExportTriangles,
                     ExportMaterialsFunction fnExportMaterials = nullptr);

    bool run() override;

private:

    const UnicodeString filename_;
    ExportTrianglesFunction fnExportTriangles_;
    ExportMaterialsFunction fnExportMaterials_;
};

}
