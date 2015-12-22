/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

void TriangleArray::clear()
{
    triangles_.clear();

    vertexDataGeometryChunk_.clear();
    usedVertexCount_ = 0;
}

bool TriangleArray::addTriangle(unsigned int index0, unsigned int index1, unsigned int index2, const String& material,
                                const String& lightmap)
{
    if (index0 >= usedVertexCount_ || index1 >= usedVertexCount_ || index2 >= usedVertexCount_)
    {
        LOG_ERROR << "Vertex index out of range";
        return false;
    }

    triangles_.emplace(this, index0, index1, index2, material, lightmap);

    return true;
}

bool TriangleArray::addTriangle(const Triangle& triangle)
{
    if (vertexDataGeometryChunk_.getVertexStreams() != triangle.array_->vertexDataGeometryChunk_.getVertexStreams())
    {
        LOG_ERROR << "Vertex stream layout mismatch, can't add triangle to array";
        return false;
    }

    return addTriangle(triangle.getVertexData(0), triangle.getVertexData(1), triangle.getVertexData(2), triangle.getMaterial(),
                       triangle.getLightmap());
}

bool TriangleArray::addTriangle(const void* v0, const void* v1, const void* v2, const String& material, const String& lightmap)
{
    usedVertexCount_ += 3;

    if (usedVertexCount_ > vertexDataGeometryChunk_.getVertexCount())
    {
        if (!vertexDataGeometryChunk_.setVertexCount(std::max(64U, vertexDataGeometryChunk_.getVertexCount() * 2)))
        {
            LOG_ERROR << "Failed changing vertex count";
            return false;
        }
    }

    // Copy in the new vertex data
    auto vertexSize = vertexDataGeometryChunk_.getVertexSize();
    auto data = vertexDataGeometryChunk_.lockVertexData() + (usedVertexCount_ - 3) * vertexSize;
    memcpy(data, v0, vertexSize);
    memcpy(data + vertexSize, v1, vertexSize);
    memcpy(data + vertexSize * 2, v2, vertexSize);
    vertexDataGeometryChunk_.unlockVertexData();

    // Add the new triangle
    triangles_.emplace(this, usedVertexCount_ - 3, usedVertexCount_ - 2, usedVertexCount_ - 1, material, lightmap);

    return true;
}

bool TriangleArray::removeDegenerateTriangles(Runnable& r)
{
    auto degenerateTriangleCount = 0U;

    for (auto i = 0U; i < size(); i++)
    {
        auto area = triangles_[i].calculateArea();
        if (area < 0.000001f)
        {
            erase(i--);
            degenerateTriangleCount++;
        }

        if (r.setTaskProgress(i + 1, size()))
            return false;
    }

    if (degenerateTriangleCount)
        LOG_INFO << "Removed " << degenerateTriangleCount << " degenerate triangles from array";

    return true;
}

bool TriangleArray::setVertexStreams(const Vector<VertexStream>& streams)
{
    if (triangles_.size())
        return false;

    vertexDataGeometryChunk_.setVertexStreams(streams);

    return true;
}

const byte_t* TriangleArray::getVertexData(unsigned int index) const
{
    if (index >= usedVertexCount_)
    {
        LOG_ERROR << "Invalid vertex index";
        return nullptr;
    }

    return vertexDataGeometryChunk_.getVertexData() + index * vertexDataGeometryChunk_.getVertexSize();
}

bool TriangleArray::copyVertexStreamsAndDataFromGeometryChunk(const GeometryChunk& geometryChunk)
{
    if (vertexDataGeometryChunk_.getVertexCount())
    {
        LOG_ERROR << "Can only be called on empty triangle arrays";
        return false;
    }

    if (!vertexDataGeometryChunk_.setVertexStreams(geometryChunk.getVertexStreams()) ||
        !vertexDataGeometryChunk_.setVertexCount(geometryChunk.getVertexCount(), false))
    {
        LOG_ERROR << "Failed setting up geometry chunk";
        vertexDataGeometryChunk_.clear();
        return false;
    }

    // Copy vertex data across
    memcpy(vertexDataGeometryChunk_.lockVertexData(), geometryChunk.getVertexData(), geometryChunk.getVertexDataSize());
    vertexDataGeometryChunk_.unlockVertexData();

    usedVertexCount_ = geometryChunk.getVertexCount();

    return true;
}

Vector<String> TriangleArray::getMaterials() const
{
    auto materials = Vector<String>();

    for (auto& triangle : triangles_)
    {
        if (!materials.has(triangle.getMaterial()))
            materials.append(triangle.getMaterial());
    }

    return materials;
}

Vector<String> TriangleArray::getLightmaps() const
{
    auto lightmaps = Vector<String>();

    for (auto& triangle : triangles_)
    {
        if (!lightmaps.has(triangle.getLightmap()))
            lightmaps.append(triangle.getLightmap());
    }

    return lightmaps;
}

}
