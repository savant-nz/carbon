/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAX_EXPORTER

#include "CarbonEngine/Exporters/Max/MaxGeometryExporter.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Scene/ABTCompiler.h"
#include "CarbonEngine/Scene/MaterialManager.h"

namespace Carbon
{

namespace Max
{

bool GeometryExporter::exportGeometry(TriangleArraySet& triangleSet, Runnable& r)
{
    auto nodes = Vector<INode*>();

    if (!gatherNodes(ip->GetRootNode(), nodes, r))
        return false;

    // Export triangles
    for (auto i = 0U; i < nodes.size(); i++)
    {
        if (!exportGeomObject(nodes[i], triangleSet))
            return false;

        if (r.setTaskProgress(i + 1, nodes.size()))
            return false;
    }

    return true;
}

bool GeometryExporter::gatherNodes(INode* node, Vector<INode*>& nodes, Runnable& r)
{
    if (r.isCancelled())
        return false;

    if (!onlyExportSelected || (onlyExportSelected && node->Selected()))
        nodes.append(node);

    for (auto i = 0; i < node->NumberOfChildren(); i++)
    {
        if (!gatherNodes(node->GetChildNode(i), nodes, r))
            return false;
    }

    return true;
}

bool GeometryExporter::exportGeomObject(INode* node, TriangleArraySet& triangleSet)
{
    // Evaluate node state
    auto currentTime = ip->GetTime();
    auto os = node->EvalWorldState(currentTime);

    // Check this is a geomobject node
    if (!os.obj || os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID)
        return true;

    // Target camera "targets" are actually geomobjects, so we skip them here
    if (os.obj->ClassID() == Class_ID(TARGET_CLASS_ID, 0))
        return true;

    // Check we can convert to a TriObject
    auto obj = os.obj;
    if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
        return true;
    auto triObject = static_cast<TriObject*>(obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0)));
    if (!triObject)
        return true;

    // Get mesh pointer
    auto mesh = &(triObject->GetMesh());
    if (!mesh)
        return true;

    LOG_INFO << "Exporting node: '" << node->GetName() << "'";

    // Get material name
    auto material = String();
    auto mtl = node->GetMtl();
    auto isMultiMaterial = false;
    if (mtl)
    {
        isMultiMaterial = (mtl->IsMultiMtl() == TRUE);
        material = mtl->GetName().data();
    }

    // Get node transform to transform the object-space vertices by
    auto transform = node->GetObjTMAfterWSM(currentTime);

    // Create transformed vertex list
    auto vertices = Vector<Vec3>(mesh->getNumVerts());
    for (auto i = 0U; i < vertices.size(); i++)
    {
        auto vertex = transform * mesh->verts[i];
        vertices[i].setXYZ(vertex.x, vertex.z, -vertex.y);
    }

    // Work out the winding order to use, a mirroring transform on the node flips the winding order and so we need to
    // compensate
    auto winding = std::array<unsigned int, 3>{{0, 1, 2}};
    if (DotProd(CrossProd(transform.GetRow(0), transform.GetRow(1)), transform.GetRow(2)) < 0.0)
        std::swap(winding[1], winding[2]);

    // Get access to user-specified mesh normals if they are present
    auto meshNormals = mesh->GetSpecifiedNormals();

    // Construct vertex stream layout
    auto meshVertexStreams =
        Vector<VertexStream>{{VertexStream::Position, 3}, {VertexStream::DiffuseTextureCoordinate, 2}};
    if (meshNormals)
        meshVertexStreams.emplace(VertexStream::Normal, 3);

    // Get vertex size in bytes
    auto meshVertexSize = VertexStream::getVertexSize(meshVertexStreams);

    // Get the triangle array on this set that has the vertex layout needed for this mesh
    auto triangles = triangleSet.findOrCreateArrayByVertexStreamLayout(meshVertexStreams);

    auto newTriangleVertexData = Vector<byte_t>(meshVertexSize * 3);
    auto newVertices = std::array<byte_t*, 3>{{&newTriangleVertexData[meshVertexSize * winding[0]],
                                               &newTriangleVertexData[meshVertexSize * winding[1]],
                                               &newTriangleVertexData[meshVertexSize * winding[2]]}};

    for (auto i = 0; i < mesh->getNumFaces(); i++)
    {
        // Zero vertex data
        for (auto vertex : newVertices)
            memset(vertex, 0, meshVertexSize);

        auto currentOffset = 0U;

        // Get position data
        for (auto v = 0U; v < 3; v++)
            *reinterpret_cast<Vec3*>(newVertices[v]) = vertices[mesh->faces[i].v[v]];
        currentOffset += 12;

        // Get ST data out if it's specified
        if (mesh->numTVerts)
        {
            for (auto v = 0U; v < 3; v++)
            {
                reinterpret_cast<Vec2*>(&newVertices[v][currentOffset])
                    ->setXY(mesh->tVerts[mesh->tvFace[i].t[v]].x, mesh->tVerts[mesh->tvFace[i].t[v]].y);
            }
        }
        currentOffset += 8;

        // Get user-specified vertex normals
        if (meshNormals)
        {
            for (auto v = 0U; v < 3; v++)
            {
                *reinterpret_cast<Vec3*>(&newVertices[v][currentOffset]) =
                    maxPoint3ToVec3(meshNormals->GetNormal(i, v));
            }
        }

        // If there was a multi-material assigned to this mesh then we need to get the actual submaterial being used on
        // this face
        auto materialName = material;
        if (isMultiMaterial)
        {
            auto subMaterial = mtl->GetSubMtl(mesh->faces[i].getMatID() % mtl->NumSubMtls());
            if (subMaterial)
                materialName = material + "/" + subMaterial->GetName().data();
            else
                LOG_WARNING << "Null submaterial found, using parent material name: " << material;
        }

        // Fall back if no material was found
        if (materialName.length() == 0)
            materialName = MaterialManager::ExporterNoMaterialFallback;

        if (!triangles->getMaterials().has(materialName))
            LOG_INFO << "New material: " << materialName;

        if (!triangles->addTriangle(newVertices[0], newVertices[1], newVertices[2], materialName))
            return false;
    }

    // Delete triobject if it was allocated specifically for us
    if (obj != triObject)
    {
        delete triObject;
        triObject = nullptr;
    }

    return true;
}

}

}

#endif
