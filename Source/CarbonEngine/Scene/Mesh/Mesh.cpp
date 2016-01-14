/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"

namespace Carbon
{

const UnicodeString Mesh::MeshDirectory = "Meshes/";
const UnicodeString Mesh::MeshExtension = ".mesh";

const auto MeshHeaderID = FileSystem::makeFourCC("cmsh");
const auto MeshVersionInfo = VersionInfo(1, 1);

void Mesh::clear()
{
    name_.clear();
    meshComponents_.clear();

    physics().deleteBodyTemplate(physicsBodyTemplate_);
    physicsBodyTemplate_ = nullptr;
}

bool Mesh::setupFromTriangles(TriangleArraySet& triangleSet, Runnable& r)
{
    auto triangleCount = triangleSet.getTriangleCount();

    meshComponents_.clear();

    // Group triangles in each set by their material
    auto groupedTriangles = Vector<std::pair<String, Vector<const Triangle*>>>();
    r.beginTask("gathering materials", 5);
    for (auto i = 0U; i < triangleSet.size(); i++)
    {
        auto& triangleArray = triangleSet[i];

        auto materialTriangles = std::unordered_map<String, Vector<const Triangle*>>();
        for (auto& triangle : triangleArray)
            materialTriangles[triangle.getMaterial()].append(&triangle);

        for (const auto& triangles : materialTriangles)
            groupedTriangles.emplace(triangles.first, triangles.second);

        if (r.setTaskProgress(i + 1, triangleSet.size()))
            return false;
    }
    r.endTask();

    // Create mesh components
    meshComponents_.reserve(groupedTriangles.size());
    for (auto& entry : groupedTriangles)
    {
        meshComponents_.enlarge(1);

        auto& meshComponent = meshComponents_.back();
        auto& triangles = entry.second;

        meshComponent.material_ = entry.first;

        r.beginTask(String() + "component " + meshComponents_.size() + "/" + meshComponents_.size() + " with " +
                        triangles.size() + " triangles",
                    95.0f * float(triangles.size()) / float(triangleCount));

        auto triangleArray = triangles[0]->getParentTriangleArray();

        // Setup the geometry chunk
        meshComponent.geometryChunk_.setVertexStreams(triangleArray->getVertexStreams());
        meshComponent.geometryChunk_.setVertexCount(triangles.size() * 3);

        auto vertexSize = meshComponent.geometryChunk_.getVertexSize();

        // Fill with raw vertex data
        auto lockedVertexData = meshComponent.geometryChunk_.lockVertexData();
        for (auto triangle : triangles)
        {
            for (auto index : triangle->getIndices())
            {
                memcpy(lockedVertexData, triangleArray->getVertexData(index), vertexSize);
                lockedVertexData += vertexSize;
            }
        }
        meshComponent.geometryChunk_.unlockVertexData();

        triangles.clear();

        // Validate the vertex position data
        if (!meshComponent.geometryChunk_.validateVertexPositionData())
        {
            LOG_ERROR << "Mesh vertex positions are not valid";
            meshComponents_.clear();
            return false;
        }

        // Fill with raw index data
        meshComponent.geometryChunk_.setIndexDataStraight();

        // Do all the mesh preparation
        r.beginTask("optimizing vertex array", 5);
        if (!meshComponent.geometryChunk_.optimizeVertexData(r))
        {
            meshComponents_.clear();
            return false;
        }
        r.endTask();

        if (!meshComponent.geometryChunk_.hasVertexStream(VertexStream::Tangent))
        {
            r.beginTask("calculating tangent bases", 10);
            if (!meshComponent.geometryChunk_.calculateTangentBases())
            {
                meshComponents_.clear();
                return false;
            }
            r.endTask();

            r.beginTask("optimizing vertex array", 5);
            if (!meshComponent.geometryChunk_.optimizeVertexData(r))
            {
                meshComponents_.clear();
                return false;
            }
            r.endTask();
        }

        r.beginTask("calculating triangle strips", 75);
        if (!meshComponent.geometryChunk_.generateTriangleStrips(r))
        {
            meshComponents_.clear();
            return false;
        }
        r.endTask();

        r.beginTask("optimizing vertex array", 5);
        if (!meshComponent.geometryChunk_.optimizeVertexData(r))
        {
            meshComponents_.clear();
            return false;
        }
        r.endTask();

        // Register this mesh component's chunk with the renderer unless we're running in an exporter (i.e. Max or Maya)
        if (!BuildInfo::isExporterBuild())
            meshComponent.geometryChunk_.registerWithRenderer();

        r.endTask();
    }

    return true;
}

bool Mesh::getTriangles(TriangleArraySet& triangleSet) const
{
    triangleSet.clear();

    for (auto& meshComponent : meshComponents_)
    {
        // Get the triangles in this mesh component
        auto triangles = new TriangleArray;
        triangleSet.append(triangles);
        meshComponent.getGeometryChunk().getTriangles(*triangles);

        // Remove degenerates, this is needed because some of the triangles in compiled geometry chunks may be zero area
        // triangles that stitch separate triangle strips together
        triangles->removeDegenerateTriangles();

        // Set material index for the triangles in this array
        for (auto j = 0U; j < triangles->size(); j++)
            (*triangles)[j].setMaterial(meshComponent.getMaterial());
    }

    return true;
}

void Mesh::setParameter(const String& name, const Parameter& value)
{
    for (auto& meshComponent : meshComponents_)
        meshComponent.geometryChunk_.getParameters().set(name, value);
}

void Mesh::save(FileWriter& file) const
{
    file.write(MeshHeaderID);

    file.beginVersionedSection(MeshVersionInfo);
    file.write(meshComponents_, ExportInfo::get());
    file.endVersionedSection();
}

bool Mesh::save(const String& name) const
{
    try
    {
        auto file = FileWriter();
        fileSystem().open(MeshDirectory + name + MeshExtension, file);

        save(file);

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

void Mesh::load(FileReader& file)
{
    try
    {
        clear();

        // Check header
        if (file.readFourCC() != MeshHeaderID)
            throw Exception("Not a mesh file");

        // Read version
        auto readVersion = file.beginVersionedSection(MeshVersionInfo);

        // Read components
        file.read(meshComponents_);

        // v1.1, export info
        if (readVersion.getMinor() >= 1)
        {
            auto exportInfo = ExportInfo();
            file.read(exportInfo);
        }

        file.endVersionedSection();

        for (auto& meshComponent : meshComponents_)
            meshComponent.geometryChunk_.registerWithRenderer();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

void Mesh::intersectRay(const Ray& ray, Vector<IntersectionResult>& results) const
{
    for (auto& meshComponent : meshComponents_)
    {
        auto meshResults = Vector<GeometryChunk::IntersectionResult>();
        meshComponent.getGeometryChunk().intersect(ray, meshResults);

        for (const auto& result : meshResults)
        {
            results.emplace(result.getDistance(), ray.getPoint(result.getDistance()), result.getNormal(), nullptr,
                            meshComponent.getMaterial());
        }
    }
}

unsigned int Mesh::getTriangleCount() const
{
    auto count = 0U;

    for (auto& meshComponent : meshComponents_)
        count += meshComponent.getGeometryChunk().getTriangleCount();

    return count;
}

AABB Mesh::getAABB() const
{
    if (meshComponents_.empty())
        return {};

    auto aabb = meshComponents_[0].getGeometryChunk().getAABB();

    for (auto i = 1U; i < meshComponents_.size(); i++)
        aabb.merge(meshComponents_[i].getGeometryChunk().getAABB());

    return aabb;
}

Sphere Mesh::getSphere() const
{
    if (meshComponents_.empty())
        return {};

    auto sphere = meshComponents_[0].getGeometryChunk().getSphere();

    for (auto i = 1U; i < meshComponents_.size(); i++)
        sphere.merge(meshComponents_[i].getGeometryChunk().getSphere());

    return sphere;
}

PhysicsInterface::BodyTemplateObject Mesh::getPhysicsBodyTemplate() const
{
    if (!physicsBodyTemplate_)
    {
        auto triangleSet = TriangleArraySet();
        getTriangles(triangleSet);

        auto bodyVertices = Vector<Vec3>();
        auto bodyTriangles = Vector<RawIndexedTriangle>();

        for (auto triangles : triangleSet)
        {
            auto indexOffset = bodyVertices.size();

            auto itPosition =
                triangles->getVertexDataGeometryChunk().getVertexStreamConstIterator<Vec3>(VertexStream::Position);
            for (auto j = 0U; j < triangles->getVertexDataGeometryChunk().getVertexCount(); j++, itPosition++)
                bodyVertices.append(*itPosition);

            for (auto& triangle : *triangles)
            {
                bodyTriangles.emplace(indexOffset + triangle.getIndex(0), indexOffset + triangle.getIndex(1),
                                      indexOffset + triangle.getIndex(2));
            }
        }

        physicsBodyTemplate_ = physics().createBodyTemplateFromGeometry(bodyVertices, bodyTriangles);
    }

    return physicsBodyTemplate_;
}

}
