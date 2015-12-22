/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Scene/ABTCompiler.h"
#include "CarbonEngine/Scene/CullingNode.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Mesh/MeshManager.h"
#include "CarbonEngine/Scene/Region.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

unsigned int ABTCompiler::triangleRecursionThreshold_ = ABTCompiler::DefaultTriangleRecursionThreshold;
float ABTCompiler::maxOvergrowth_ = ABTCompiler::DefaultMaxOvergrowth;
ABTCompiler::LightingType ABTCompiler::lightingType_ = ABTCompiler::LightingPerPixel;
unsigned int ABTCompiler::initialTriangleCount_;
Vector<ABTCompiler::CompileNode*> ABTCompiler::finalNodes_;

// Chooses a splitting plane given an AABB and a set of triangles inside that AABB.
static Plane chooseSplitPlane(const AABB& aabb, const TriangleArraySet& triangleSet)
{
    // TODO: implement this correctly using heuristics

    auto xSize = aabb.getMaximum().x - aabb.getMinimum().x;
    auto ySize = aabb.getMaximum().y - aabb.getMinimum().y;
    auto zSize = aabb.getMaximum().z - aabb.getMinimum().z;

    auto n = Vec3();

    if (xSize > ySize && xSize > zSize)
        n = Vec3::UnitX;
    else if (ySize > xSize && ySize > zSize)
        n = Vec3::UnitY;
    else
        n = Vec3::UnitZ;

    return {aabb.getMinimum() + (aabb.getMaximum() - aabb.getMinimum()) * 0.5f, n};
}

static bool divideTriangles(const Plane& plane, const TriangleArraySet& triangleSet, TriangleArraySet& frontSet,
                            TriangleArraySet& backSet, bool allowOvergrowth, float maxOvergrowth, Runnable& r)
{
    for (auto triangles : triangleSet)
    {
        auto frontArray = frontSet.findOrCreateArrayByVertexStreamLayout(triangles->getVertexStreams());
        auto backArray = backSet.findOrCreateArrayByVertexStreamLayout(triangles->getVertexStreams());

        for (auto i = 0U; i < triangles->size(); i++)
        {
            auto& tri = (*triangles)[i];

            // Classify the triangle against the splitting plane
            auto cr = tri.classify(plane);

            auto arrayToTransferTriangleTo = pointer_to<TriangleArray>::type();

            // Deal with the trivial cases, triangle fully on one side of the plane
            if (cr == Plane::Back)
                arrayToTransferTriangleTo = backArray;
            else if (cr == Plane::Coincident || cr == Plane::Front)
                arrayToTransferTriangleTo = frontArray;
            else if (allowOvergrowth)
            {
                // The triangle is spanning the split plane Instead of just blindly splitting the triangle, work out whether
                // shifting the splitting plane along its normal by a maximum of +/- the overgrowth value will mean the triangle
                // doesn't have to be split. If this is possible then we use the smallest shift possible that avoids splitting
                // the triangle

                auto maxBackDist = 0.0f;
                auto maxFrontDist = 0.0f;

                for (auto j = 0U; j < 3; j++)
                {
                    auto d = plane.distance(tri.getVertexPosition(j));
                    if (d < 0.0f && -d > maxBackDist)
                        maxBackDist = -d;
                    else if (d > 0.0f && d > maxFrontDist)
                        maxFrontDist = d;
                }

                if (maxBackDist < maxFrontDist && maxBackDist < maxOvergrowth)
                    arrayToTransferTriangleTo = frontArray;
                else if (maxFrontDist < maxOvergrowth)
                    arrayToTransferTriangleTo = backArray;
            }

            if (arrayToTransferTriangleTo)
            {
                if (!arrayToTransferTriangleTo->addTriangle(tri.getVertexData(0), tri.getVertexData(1), tri.getVertexData(2),
                                                            tri.getMaterial(), tri.getLightmap()))
                    return false;

                continue;
            }

            // Overgrowing failed to account for the spanning triangle so it must be split. If the triangle has a vertex within
            // the overgrowing region then we split along the plane that vertex lies in in order to reduce the number of
            // triangles produced by the split from 3 to 2

            // Find closest vertex to split plane
            auto closestVertex = 0U;
            auto closestVertexDistance = FLT_MAX;

            if (allowOvergrowth)
            {
                for (auto j = 0U; j < 3; j++)
                {
                    auto d = fabsf(plane.distance(tri.getVertexPosition(j)));
                    if (d < closestVertexDistance)
                    {
                        closestVertexDistance = d;
                        closestVertex = j;
                    }
                }
            }

            // Storage for front and back tris resulting from the split
            auto frontPieces = TriangleArray();
            auto backPieces = TriangleArray();

            if (closestVertexDistance < maxOvergrowth)
                tri.split(Plane(tri.getVertexPosition(closestVertex), plane.getNormal()), frontPieces, backPieces);
            else
                tri.split(plane, frontPieces, backPieces);

            // Add front and back tris to the main lists
            for (auto& piece : frontPieces)
            {
                if (!frontArray->addTriangle(piece.getVertexData(0), piece.getVertexData(1), piece.getVertexData(2),
                                             tri.getMaterial(), tri.getLightmap()))
                    return false;
            }
            for (auto& piece : backPieces)
            {
                if (!backArray->addTriangle(piece.getVertexData(0), piece.getVertexData(1), piece.getVertexData(2),
                                            tri.getMaterial(), tri.getLightmap()))
                    return false;
            }

            if (r.isCancelled())
                return false;
        }

        if (frontArray->empty())
            frontSet.remove(frontArray);

        if (backArray->empty())
            backSet.remove(backArray);
    }

    return true;
}

bool ABTCompiler::subdivide(Scene& scene, CullingNode* node, TriangleArraySet& triangleSet, Runnable& r)
{
    if (r.isCancelled())
        return false;

    auto totalDoneTriangles = 0U;
    for (auto finalNode : finalNodes_)
        totalDoneTriangles += finalNode->triangleSet.getTriangleCount();
    r.setTaskProgress(totalDoneTriangles, initialTriangleCount_);

    node->setIsWorldGeometry(true);

    // Calculate the bounding volumes for this node
    auto aabb = AABB();
    aabb.setFromTriangleSet(triangleSet);

    // Check if the triangle threshold has been reached
    if (triangleSet.getTriangleCount() <= ABTCompiler::getTriangleRecursionThreshold())
    {
        finalNodes_.append(new CompileNode);
        finalNodes_.back()->node = node;
        triangleSet.transfer(finalNodes_.back()->triangleSet);

        return true;
    }

    // Get node splitting plane
    auto splitPlane = chooseSplitPlane(aabb, triangleSet);

    // Limit the maximum allowed overgrowth to 10% of the smallest half of the split AABB
    auto distance1 = fabsf(splitPlane.distance(aabb.getMinimum()));
    auto distance2 = fabsf(splitPlane.distance(aabb.getMaximum()));
    auto maxOvergrowth = std::min(ABTCompiler::getMaxOvergrowth(), std::min(distance1, distance2) * 0.1f);

    // Divide triangles by the plane
    auto frontSet = TriangleArraySet();
    auto backSet = TriangleArraySet();
    if (!divideTriangles(splitPlane, triangleSet, frontSet, backSet, true, maxOvergrowth, r))
        return false;
    triangleSet.clear();

    // Recurse through front node
    if (frontSet.size())
    {
        auto front = node->addChild<CullingNode>();
        front->setIsInternalEntity(true);
        if (!subdivide(scene, front, frontSet, r))
            return false;
    }

    // Recurse through back node
    if (backSet.size())
    {
        auto back = node->addChild<CullingNode>();
        back->setIsInternalEntity(true);
        if (!subdivide(scene, back, backSet, r))
            return false;
    }

    return true;
}

bool ABTCompiler::compile(Scene& scene, TriangleArraySet& triangleSet, Runnable& r)
{
    auto rawRegions = std::unordered_map<String, std::unique_ptr<TriangleArray>>();

    try
    {
        scene.clear();

        auto guid = Math::createGUID();

        if (triangleSet.empty())
            return true;

        // Separate out region definitions
        for (auto triangles : triangleSet)
        {
            for (auto& triangle : *triangles)
            {
                if (triangle.getMaterial().startsWith(Region::RegionMaterialPrefix))
                {
                    if (rawRegions.find(triangle.getMaterial()) == rawRegions.end())
                    {
                        rawRegions[triangle.getMaterial()].reset(new TriangleArray);

                        // Regions only need vertex position data
                        if (rawRegions[triangle.getMaterial()]->getVertexStreams().empty())
                        {
                            auto vertexStreams = Vector<VertexStream>{{VertexStream::Position, 3}};
                            rawRegions[triangle.getMaterial()]->setVertexStreams(vertexStreams);
                        }
                    }

                    if (!rawRegions[triangle.getMaterial()]->addTriangle(triangle))
                        throw Exception("Failed adding triangle to region");
                }

                if (r.isCancelled())
                    throw Exception();
            }
        }

        // Setup collision triangles
        r.beginTask("initializing collision geometry", 5);
        if (!scene.setupCollisionTriangles(triangleSet, r))
            throw Exception();
        r.endTask();

        initialTriangleCount_ = triangleSet.getTriangleCount();

        // Subdivide the scene triangle array down into localised areas. This builds the scene-graph culling structure and fills
        // finalNodes_ with a list of nodes and the triangles associated with that node.
        r.beginTask("subdividing", 15);
        auto root = scene.addEntity<CullingNode>();
        if (!subdivide(scene, root, triangleSet, r))
            throw Exception();
        r.endTask();

        // Get the total number of triangles in all the nodes so we can allocate compilation time percentages
        auto totalNodeTriangles = 0U;
        for (auto node : finalNodes_)
            totalNodeTriangles += node->triangleSet.getTriangleCount();

        // The subdivision step above has produced a list of nodes in the scene graph and a list of triangles that are to be
        // drawn for that node. Go through the list of nodes and compile the geometry for each one
        for (auto i = 0U; i < finalNodes_.size(); i++)
        {
            auto node = finalNodes_[i]->node;
            auto& nodeTriangleSet = finalNodes_[i]->triangleSet;

            auto nodeTriangleCount = nodeTriangleSet.getTriangleCount();

            r.beginTask(String() + "node " + (i + 1) + "/" + finalNodes_.size() + " with " + nodeTriangleCount + " triangles",
                        80.0f * float(nodeTriangleCount) / float(totalNodeTriangles));

            if (lightingType_ == ABTCompiler::LightingPerPixel)
            {
                auto mesh = meshes().createMesh();
                if (!mesh->setupFromTriangles(nodeTriangleSet, r))
                {
                    meshes().releaseMesh(mesh);
                    throw Exception();
                }

                // Embed this mesh as a file in the scene
                auto meshName = ".scene/" + guid + "/" + i;
                scene.addEmbeddedResource(Mesh::MeshDirectory + meshName + Mesh::MeshExtension, *mesh);
                node->attachMesh(meshName);

                meshes().releaseMesh(mesh);
            }
            else
            {
                auto nextEmbeddedMeshIndex = 0U;

                for (auto nodeTriangles : nodeTriangleSet)
                {
                    // Separate triangles by their lightmap
                    for (auto& lightmapName : nodeTriangles->getLightmaps())
                    {
                        auto lightmapTriangleSet = TriangleArraySet();

                        auto lightmapTriangles =
                            lightmapTriangleSet.findOrCreateArrayByVertexStreamLayout(nodeTriangles->getVertexStreams());

                        for (auto& triangle : *nodeTriangles)
                        {
                            if (triangle.getLightmap() == lightmapName)
                            {
                                if (!lightmapTriangles->addTriangle(triangle))
                                    throw Exception("Failed adding triangle to lightmap triangle array");
                            }
                        }

                        auto mesh = meshes().createMesh();
                        if (!mesh->setupFromTriangles(lightmapTriangleSet, r))
                        {
                            meshes().releaseMesh(mesh);
                            throw Exception() << "Mesh setup failed on node " << (i + 1);
                        }

                        // Set lightmap texture parameter on the geometry chunks in this mesh
                        mesh->setParameter("lightMap", lightmapName);

                        auto meshName = ".scene/" + guid + "/" + i + "_" + nextEmbeddedMeshIndex++;
                        scene.addEmbeddedResource(Mesh::MeshDirectory + meshName + Mesh::MeshExtension, *mesh);
                        node->attachMesh(meshName);

                        meshes().releaseMesh(mesh);
                    }
                }
            }

            r.endTask();

            // Free this node
            delete finalNodes_[i];
            finalNodes_[i] = nullptr;
        }

        finalNodes_.clear();

        // Compile regions
        for (auto& rawRegion : rawRegions)
        {
            LOG_INFO << "Compiling region: '" << rawRegion.first << "' with " << rawRegion.second->size() << " triangles";

            scene.addEntity<Region>(rawRegion.first)->setup(*rawRegion.second);
        }

        return true;
    }
    catch (const Exception& e)
    {
        scene.clear();

        // Clean up any remaining nodes
        for (auto node : finalNodes_)
            delete node;
        finalNodes_.clear();

        if (e.get().length())
            LOG_ERROR << e.get();

        return false;
    }
}

}
