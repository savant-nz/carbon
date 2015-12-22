/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Exporters/Maya/MayaPlugin.h"
#include "CarbonEngine/Scene/SkeletalMesh.h"

namespace Carbon
{

namespace Maya
{

/**
 * Helper methods for the Maya exporters.
 */
class GeometryHelper
{
public:

    /**
     * Constructs the vertex stream layout to use for exporting the geometry in the given Maya mesh. Returns details on the
     * stream layout as well as the UV and color data. If \a addSkeletalVertexStreams is true then two extra vertex streams for
     * bone indices and weights are added immediately after the initial position vertex stream.
     */
    static void getMFnMeshVertexStreamLayout(MFnMesh& fnMesh, Vector<VertexStream>& meshVertexStreams,
                                             unsigned int& meshVertexSize, MStringArray& uvSetNames,
                                             Vector<MColorArray>& colorArrays, bool addSkeletalVertexStreams);

    /**
     * Exports mesh data from a given DAG path to a mesh. Returns the data in a triangle set.
     */
    static bool exportMeshAtPath(const MDagPath& dagPath, TriangleArraySet& triangleSet, Runnable& r);

    /**
     * Exports mesh data from a given DAG path to a mesh and the actual mesh object to export. Returns the data in a triangle
     * set. If \a skeletalVertices is not null then it is used as the source data for adding extra vertex streams containing
     * skeletal animation data (for bone indices and weights).
     */
    static bool exportMFnMesh(const MDagPath& dagPath, MFnMesh& fnMesh, MObject shadersMesh, TriangleArraySet& triangleSet,
                              const Vector<Vector<SkeletalMesh::VertexWeight>>* skeletalVertices, Runnable& r);

    /**
     * Similar to GeometryHelper::exportMFnMesh() except that only raw vertex positions and an indexed triangle array is
     * exported.
     */
    static bool exportMFnMeshRaw(const MDagPath& dagPath, Vector<Vec3>& vertices, Vector<RawIndexedTriangle>& triangles,
                                 MSpace::Space space = MSpace::kWorld);

    /**
     * Extracts all meshes from the current DAG. Returns the data in a triangle set.
     */
    static bool extractAllMeshes(TriangleArraySet& triangleSet, Runnable& r);

    /**
     * Mesh callback function definition used by GeometryHelper::extractAllMeshes(), it is passed the DAG path to a mesh and the
     * return value indicates whether or not its data should be included in the export.
     */
    typedef bool (*ExportMeshCallback)(const MDagPath& dagPath);

    /**
     * Extracts all meshes from the current DAG. Returns the data in a triangle set. If the mesh callback is not null then it
     * will be called for each mesh prior to its data being exported. If the callback returns false then the mesh will not be
     * exported.
     */
    static bool extractAllMeshes(TriangleArraySet& triangleSet, Runnable& r, ExportMeshCallback meshCallback);

private:

    static bool getConnectedShaders(const MDagPath& dagPath, MFnMesh& fnMesh, MObjectArray& shaders, MIntArray& indices);
};

}

}

#endif
