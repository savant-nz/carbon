/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Exporters/Maya/MayaPlugin.h"

namespace Carbon
{

namespace Maya
{

/**
 * Helper methods for the Maya exporters.
 */
class Helper
{
public:

    /**
     * Returns whether the given node is flagged visible.
     */
    static bool isNodeVisible(const MFnDagNode& fnDagNode);

    /**
     * Returns all the objects of the given type that should be exported. If Maya::onlyExportSelected is true then only the
     * selected objects of the given type are returned.
     */
    static void getExportObjects(MDagPathArray& objects, MFn::Type type);

    /**
     * Returns whether the given object is selected.
     */
    static bool isObjectSelected(MObject& obj);

    /**
     * Puts the transform at the given DAG path into \a position and \a orientation. Returns success flag.
     */
    static bool getTransformAtDagPath(const MDagPath& dagPath, SimpleTransform& transform);

    /**
     * Puts the transform at the given DAG path onto the passed entity. Returns success flag.
     */
    static bool getTransformAtDagPath(const MDagPath& dagPath, Entity& entity);

    /**
     * This method is used in tandem with the arrays returned by MFnMesh::getConnectedShaders() and returns the material name
     * for the polygon at the given index, based on the \a shaders and \a indices arrays provided by that method. This also
     * handles falling back to MaterialManager::ExporterNoMaterialFallback if no material is specified for a polygon and will
     * replace two consecutive underscores in a material name with a forward slash. This provides a way to specify a '/' in a
     * material name that gets around Maya not allowing special characters.
     */
    static String getMaterialName(int polygonIndex, const MObjectArray& shaders, const MIntArray& indices);

    /**
     * Exports all materials referenced by the triangle set as material files in the given directory. Some material properties
     * from Maya are preserved where possible. Does not overwrite any existing files.
     */
    static void exportAllMaterials(const TriangleArraySet& triangleSet, const UnicodeString& outputDirectory);

private:

    static String processMaterialName(const String& name);
    static String extractMaterialTexture(const MFnDependencyNode& fn, const String& colorName,
                                         const String& fallback = String::Empty);
    static String extractMaterialNormalMap(const MFnDependencyNode& fn);
    static String getTextureOutputName(MObject node);
};

}

}

#endif
