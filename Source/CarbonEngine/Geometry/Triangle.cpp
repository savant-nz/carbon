/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Geometry/Triangle.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Math/Line.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

Vec3 Triangle::getNormal() const
{
    return ((getVertexPosition(2) - getVertexPosition(0)).cross(getVertexPosition(1) - getVertexPosition(0)))
        .normalized();
}

void Triangle::split(const Plane& plane, TriangleArray& frontPieces, TriangleArray& backPieces) const
{
    // This algorithm works for splitting any convex polygon by a plane

    // Classify the vertices against the plane
    auto vertClassify = std::array<Plane::ClassifyResult, 3>{{plane.classify(getVertexPosition(0)),
                                                              plane.classify(getVertexPosition(1)),
                                                              plane.classify(getVertexPosition(2))}};

    // The front and back pieces after splitting
    auto front = Vector<Vector<byte_t>>();
    auto back = Vector<Vector<byte_t>>();

    for (auto j = 0U; j < 3; j++)
    {
        switch (vertClassify[j])
        {
            case Plane::Front:
                front.append(copyVertexData(j));
                break;

            case Plane::Back:
                back.append(copyVertexData(j));
                break;

            case Plane::Coincident:
                front.append(copyVertexData(j));
                back.append(copyVertexData(j));
                break;

            case Plane::Spanning:
                // Won't happen when classifying a point
                break;
        }

        // Get index of next vertex
        auto next = (j + 1) % 3;

        // Ignore case where one is on the plane and the other isn't
        if ((vertClassify[j] == Plane::Coincident && vertClassify[next] != Plane::Coincident) ||
            (vertClassify[next] == Plane::Coincident && vertClassify[j] != Plane::Coincident))
            continue;

        if (vertClassify[j] != vertClassify[next])
        {
            // Get the intersection point
            auto fraction = 0.0f;
            if (!plane.intersect(Line(getVertexPosition(j), getVertexPosition(next)), fraction))
                LOG_WARNING << "Error in triangle splitting";

            // Interpolate new vertex
            front.append(Vector<byte_t>());
            front.back().resize(array_->vertexDataGeometryChunk_.getVertexSize());
            VertexStream::interpolate(array_->vertexDataGeometryChunk_.getVertexStreams(), getVertexData(j),
                                      getVertexData(next), front.back().getData(), fraction);

            // Add new vertex to back list as well
            back.append(front.back());
        }
    }

    frontPieces.clear();
    backPieces.clear();

    if (front.size() > 2)
    {
        frontPieces.vertexDataGeometryChunk_.setVertexStreams(array_->getVertexStreams());

        frontPieces.addTriangle(front[0].getData(), front[1].getData(), front[2].getData());

        if (front.size() == 4)
            frontPieces.addTriangle(front[0].getData(), front[2].getData(), front[3].getData());
    }
    if (back.size() > 2)
    {
        backPieces.vertexDataGeometryChunk_.setVertexStreams(array_->getVertexStreams());

        backPieces.addTriangle(back[0].getData(), back[1].getData(), back[2].getData());

        if (back.size() == 4)
            backPieces.addTriangle(back[0].getData(), back[2].getData(), back[3].getData());
    }
}

float Triangle::calculateArea() const
{
    if (!getVertexPosition(0).isFinite() || !getVertexPosition(1).isFinite() || !getVertexPosition(2).isFinite())
        return FLT_MAX;

    // Side lengths
    auto a = (getVertexPosition(0) - getVertexPosition(1)).length();
    auto b = (getVertexPosition(1) - getVertexPosition(2)).length();
    auto c = (getVertexPosition(2) - getVertexPosition(0)).length();

    auto s = (a + b + c) * 0.5f;
    auto t = s * (s - a) * (s - b) * (s - c);

    return sqrtf(t);
}

Plane::ClassifyResult Triangle::classify(const Plane& plane) const
{
    return plane.classify(getVertexPosition(0), getVertexPosition(1), getVertexPosition(2));
}

const byte_t* Triangle::getVertexData(unsigned int v) const
{
    return array_ ? array_->getVertexData(indices_[v]) : nullptr;
}

const Vec3& Triangle::getVertexPosition(unsigned int v) const
{
    if (!array_ || v > 2)
    {
        LOG_ERROR << "No vertex position";
        return Vec3::Zero;
    }

    return *reinterpret_cast<const Vec3*>(getVertexData(v));
}

Vector<byte_t> Triangle::copyVertexData(unsigned int v) const
{
    auto data = Vector<byte_t>();

    if (array_)
    {
        data.resize(array_->vertexDataGeometryChunk_.getVertexSize());
        memcpy(data.getData(), array_->getVertexData(indices_[v]), array_->vertexDataGeometryChunk_.getVertexSize());
    }

    return data;
}

}
