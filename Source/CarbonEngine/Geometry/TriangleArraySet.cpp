/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"

namespace Carbon
{

TriangleArray* TriangleArraySet::findOrCreateArrayByVertexStreamLayout(const Vector<VertexStream>& streams)
{
    for (auto array : arrays_)
    {
        if (streams == array->vertexDataGeometryChunk_.getVertexStreams())
            return array;
    }

    // Create a new triangle array with the given vertex layout
    auto triangleArray = new TriangleArray;
    triangleArray->vertexDataGeometryChunk_.setVertexStreams(streams);
    append(triangleArray);

    return triangleArray;
}

unsigned int TriangleArraySet::getTriangleCount() const
{
    auto count = 0U;

    for (auto array : arrays_)
        count += array->size();

    return count;
}

void TriangleArraySet::clear()
{
    for (auto array : arrays_)
        delete array;

    arrays_.clear();
}

bool TriangleArraySet::remove(TriangleArray* array)
{
    auto index = arrays_.find(array);
    if (index == -1)
    {
        LOG_ERROR << "Unknown array";
        return false;
    }

    arrays_.erase(index);

    delete array;
    array = nullptr;

    return true;
}

bool TriangleArraySet::transformPositionData(const Matrix4& transform)
{
    bool result = true;

    for (auto array : arrays_)
        result &= array->vertexDataGeometryChunk_.transformVertexStream(VertexStream::Position, transform);

    return result;
}

void TriangleArraySet::transfer(TriangleArraySet& target)
{
    target.clear();
    target.arrays_ = arrays_;
    arrays_.clear();
}

Vector<String> TriangleArraySet::getMaterials() const
{
    auto materials = Vector<String>();

    for (auto array : arrays_)
    {
        for (auto& triangle : *array)
        {
            if (!materials.has(triangle.getMaterial()))
                materials.append(triangle.getMaterial());
        }
    }

    return materials;
}

}
