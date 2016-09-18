/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Exporters/Maya/MayaGeometryHelper.h"
#include "CarbonEngine/Exporters/Maya/MayaHelper.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Render/VertexStream.h"
#include "CarbonEngine/Scene/MaterialManager.h"

namespace Carbon
{

namespace Maya
{

void GeometryHelper::getMFnMeshVertexStreamLayout(MFnMesh& fnMesh, Vector<VertexStream>& meshVertexStreams,
                                                  unsigned int& meshVertexSize, MStringArray& uvSetNames,
                                                  Vector<MColorArray>& colorArrays, bool addSkeletalVertexStreams)
{
    meshVertexStreams.clear();
    meshVertexSize = 0;

    // Add position stream
    meshVertexStreams.emplace(VertexStream::Position, 3);
    meshVertexSize += 12;

    // Add skeletal mesh vertex streams if required
    if (addSkeletalVertexStreams)
    {
        meshVertexStreams.emplace(VertexStream::Bones, 4, TypeUInt8, false);
        meshVertexStreams.emplace(VertexStream::Weights, 4);
        meshVertexSize += 20;
    }

    // Add normal stream
    meshVertexStreams.emplace(VertexStream::Normal, 3);
    meshVertexSize += 12;

    // Check for any UV sets
    fnMesh.getUVSetNames(uvSetNames);
    for (auto i = 0U; i < uvSetNames.length(); i++)
    {
        auto streamName = String(i == 0 ? "Diffuse" : uvSetNames[i].asChar()) + "TextureCoordinate";
        meshVertexStreams.emplace(VertexStream::streamNameToType(streamName), 2);
        meshVertexSize += 8;
    }

    // Check for any color sets
    auto colorSetNames = MStringArray();
    fnMesh.getColorSetNames(colorSetNames);
    colorArrays.resize(colorSetNames.length());
    for (auto i = 0U; i < colorSetNames.length(); i++)
    {
        auto streamName = String((i == 0) ? "Color" : (String(colorSetNames[i].asChar()) + "Color"));
        meshVertexStreams.emplace(VertexStream::streamNameToType(streamName), 4, TypeUInt8);
        meshVertexSize += 4;

        fnMesh.getVertexColors(colorArrays[i], &colorSetNames[i]);
    }
}

bool GeometryHelper::exportMeshAtPath(const MDagPath& dagPath, TriangleArraySet& triangleSet, Runnable& r)
{
    auto status = MStatus();

    if (!dagPath.hasFn(MFn::kMesh))
    {
        LOG_WARNING_WITHOUT_CALLER << "DAG path does not lead to a mesh";
        return false;
    }

    MFnMesh fnMesh(dagPath);

    return exportMFnMesh(dagPath, fnMesh, MObject::kNullObj, triangleSet, nullptr, r);
}

bool GeometryHelper::getConnectedShaders(const MDagPath& dagPath, MFnMesh& fnMesh, MObjectArray& shaders,
                                         MIntArray& indices)
{
    if (fnMesh.getConnectedShaders(dagPath.instanceNumber(), shaders, indices) != MS::kSuccess)
    {
        LOG_WARNING_WITHOUT_CALLER << "Failed getting shaders connected to mesh '"
                                   << MStringToString(fnMesh.partialPathName())
                                   << "', no materials will be exported for it";
        return false;
    }

    return true;
}

bool GeometryHelper::exportMFnMesh(const MDagPath& dagPath, MFnMesh& fnMesh, MObject shadersMesh,
                                   TriangleArraySet& triangleSet,
                                   const Vector<Vector<SkeletalMesh::VertexWeight>>* skeletalVertices, Runnable& r)
{
    auto isSkeletal = (skeletalVertices != nullptr);

    auto status = MStatus();

    // Get the mesh point data
    auto meshPoints = MFloatPointArray();
    fnMesh.getPoints(meshPoints, MSpace::kWorld);

    // Get vertex layout details for this mesh
    auto meshVertexStreams = Vector<VertexStream>();
    auto meshVertexSize = 0U;
    auto uvSetNames = MStringArray();
    auto colorArrays = Vector<MColorArray>();
    getMFnMeshVertexStreamLayout(fnMesh, meshVertexStreams, meshVertexSize, uvSetNames, colorArrays, isSkeletal);

    // Get the triangle array on this set that has the vertex layout needed for this mesh
    auto triangles = triangleSet.findOrCreateArrayByVertexStreamLayout(meshVertexStreams);

    // Get the array of shaders used by this mesh with an index array specifying which shader each polygon uses
    auto shaders = MObjectArray();
    auto indices = MIntArray();
    if (shadersMesh.hasFn(MFn::kMesh))
    {
        MFnMesh fnShadersMesh(shadersMesh);
        getConnectedShaders(dagPath, fnShadersMesh, shaders, indices);
    }
    else
        getConnectedShaders(dagPath, fnMesh, shaders, indices);

    auto newTriangleVertexData = Vector<byte_t>(meshVertexSize * 3);
    auto newVertices =
        std::array<byte_t*, 3>{{&newTriangleVertexData[meshVertexSize * 0], &newTriangleVertexData[meshVertexSize * 1],
                                &newTriangleVertexData[meshVertexSize * 2]}};

    // Loop through polygons
    for (auto itPolygon = MItMeshPolygon(dagPath); !itPolygon.isDone(); itPolygon.next())
    {
        // Get object relative indices for the vertices in this polygon
        auto polygonVertices = MIntArray();
        itPolygon.getVertices(polygonVertices);

        // Material for this polygon
        auto materialName = Helper::getMaterialName(itPolygon.index(), shaders, indices);
        if (!triangles->getMaterials().has(materialName))
            LOG_INFO << "New material: " << materialName;

        // Get the number of triangles in the triangulation of this polygon
        auto triangleCount = 0;
        itPolygon.numTriangles(triangleCount);
        for (auto i = 0U; i < uint(triangleCount); i++)
        {
            auto triangleVertices = MIntArray();
            auto unused = MPointArray();

            if (itPolygon.getTriangle(i, unused, triangleVertices, MSpace::kWorld) == MStatus::kSuccess)
            {
                // Zero vertex data
                for (auto j = 0U; j < 3; j++)
                    memset(newVertices[j], 0, meshVertexSize);

                auto currentOffset = 0U;

                // Copy in vertex positions
                for (auto j = 0U; j < 3; j++)
                {
                    const auto& p = meshPoints[triangleVertices[j]];
                    reinterpret_cast<Vec3*>(&newVertices[j][currentOffset])->setXYZ(p.x, p.y, p.z);
                }
                currentOffset += 12;

                if (isSkeletal)
                {
                    // Copy in skeletal vertex stream data
                    for (auto j = 0U; j < 3; j++)
                    {
                        const auto& vertexWeights = (*skeletalVertices)[triangleVertices[j]];
                        for (auto v = 0U; v < vertexWeights.size(); v++)
                        {
                            newVertices[j][currentOffset + v] = vertexWeights[v].getBone();
                            *reinterpret_cast<float*>(&newVertices[j][currentOffset + 4 + v * 4]) =
                                vertexWeights[v].getWeight();
                        }
                    }
                    currentOffset += 20;
                }

                auto localIndices = MIntArray();

                // Convert to face-relative indices
                for (auto j = 0U; j < 3; j++)
                {
                    for (auto k = 0U; k < polygonVertices.length(); k++)
                    {
                        if (triangleVertices[j] == polygonVertices[k])
                        {
                            localIndices.append(k);
                            break;
                        }
                    }
                }

                // Get normals for the vertices
                for (auto j = 0U; j < 3; j++)
                {
                    auto normal = MVector();
                    itPolygon.getNormal(localIndices[j], normal, MSpace::kWorld);
                    *reinterpret_cast<Vec3*>(&newVertices[j][currentOffset]) = MVectorToVec3(normal);
                }
                currentOffset += 12;

                // Get uv set data for the vertices
                for (auto uvSet = 0U; uvSet < uvSetNames.length(); uvSet++)
                {
                    for (auto j = 0U; j < 3; j++)
                    {
                        float2 uvPoint;
                        itPolygon.getUV(localIndices[j], uvPoint, &uvSetNames[uvSet]);
                        reinterpret_cast<Vec2*>(&newVertices[j][currentOffset])->setXY(uvPoint[0], uvPoint[1]);
                    }
                    currentOffset += 8;
                }

                // Get color set data for the vertices
                for (auto colorSet = 0U; colorSet < colorArrays.size(); colorSet++)
                {
                    for (auto j = 0U; j < 3; j++)
                    {
                        auto color = colorArrays[colorSet][triangleVertices[j]];
                        auto data = &newVertices[j][currentOffset];
                        data[0] = byte_t(color.r * 255.0f);
                        data[1] = byte_t(color.g * 255.0f);
                        data[2] = byte_t(color.b * 255.0f);
                        data[3] = byte_t(color.a * 255.0f);
                    }
                    currentOffset += 4;
                }

                if (!triangles->addTriangle(newVertices[0], newVertices[1], newVertices[2], materialName))
                    return false;
            }
        }

        if (r.setTaskProgress(itPolygon.index() + 1, fnMesh.numPolygons()))
            return false;
    }

    return true;
}

bool GeometryHelper::extractAllMeshes(TriangleArraySet& triangleSet, Runnable& r)
{
    return extractAllMeshes(triangleSet, r, nullptr);
}

bool GeometryHelper::extractAllMeshes(TriangleArraySet& triangleSet, Runnable& r, ExportMeshCallback meshCallback)
{
    auto status = MStatus();

    auto meshDagPaths = MDagPathArray();
    Helper::getExportObjects(meshDagPaths, MFn::kMesh);

    // Remove meshes that aren't visible or have false returned from the mesh callback
    for (auto i = 0U; i < meshDagPaths.length(); i++)
    {
        if (!Helper::isNodeVisible(meshDagPaths[i]) || (meshCallback && !meshCallback(meshDagPaths[i])))
            meshDagPaths.remove(i--);
    }

    // Export all the meshes
    for (auto i = 0U; i < meshDagPaths.length(); i++)
    {
        r.beginTask("", 100.0f / float(meshDagPaths.length()));
        if (!exportMeshAtPath(meshDagPaths[i], triangleSet, r))
            return false;
        r.endTask();

        if (r.isCancelled())
            return false;
    }

    return true;
}

bool GeometryHelper::exportMFnMeshRaw(const MDagPath& dagPath, Vector<Vec3>& vertices,
                                      Vector<RawIndexedTriangle>& triangles, MSpace::Space space)
{
    if (!dagPath.hasFn(MFn::kMesh))
        return false;

    auto status = MStatus();

    // Get the mesh point data
    auto meshPoints = MFloatPointArray();
    MFnMesh(dagPath).getPoints(meshPoints, space);

    vertices.resize(meshPoints.length());
    for (auto i = 0U; i < meshPoints.length(); i++)
        vertices[i].setXYZ(meshPoints[i].x, meshPoints[i].y, meshPoints[i].z);

    // Loop through polygons
    for (auto itPolygon = MItMeshPolygon(dagPath); !itPolygon.isDone(); itPolygon.next())
    {
        // Get object relative indices for the vertices in this polygon
        auto polygonVertices = MIntArray();
        itPolygon.getVertices(polygonVertices);

        // Get the number of triangles in the triangulation of this polygon
        auto triangleCount = 0;
        itPolygon.numTriangles(triangleCount);
        for (auto i = 0U; i < uint(triangleCount); i++)
        {
            auto triangleVertices = MIntArray();
            auto unused = MPointArray();

            if (itPolygon.getTriangle(i, unused, triangleVertices, MSpace::kWorld) == MStatus::kSuccess)
                triangles.emplace(triangleVertices[0], triangleVertices[1], triangleVertices[2]);
        }
    }

    return true;
}

}

}

#endif
