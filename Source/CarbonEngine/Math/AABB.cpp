/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Math/Line.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Ray.h"

namespace Carbon
{

const AABB AABB::Max(Vec3(-std::numeric_limits<float>::max()), Vec3(std::numeric_limits<float>::max()));

AABB::AABB(const AABB& aabb, const SimpleTransform& transform)
{
    auto corners = std::array<Vec3, 8>();
    aabb.getCorners(corners);

    if (transform.getOrientation() != Quaternion::Identity)
    {
        for (auto& corner : corners)
            corner = transform * corner;
    }
    else
    {
        if (transform.getPosition() != Vec3::Zero)
        {
            for (auto& corner : corners)
                corner += transform.getPosition();
        }
    }

    // Set this AABB
    minimum_ = corners[0];
    maximum_ = corners[0];
    for (auto i = 1U; i < 8; i++)
        addPoint(corners[i]);
}

void AABB::addPoint(const Vec3& p)
{
    minimum_.x = std::min(p.x, minimum_.x);
    minimum_.y = std::min(p.y, minimum_.y);
    minimum_.z = std::min(p.z, minimum_.z);

    maximum_.x = std::max(p.x, maximum_.x);
    maximum_.y = std::max(p.y, maximum_.y);
    maximum_.z = std::max(p.z, maximum_.z);
}

void AABB::addPoints(const Vector<Vec3>& points)
{
    for (auto point : points)
        addPoint(point);
}

void AABB::addSphere(const Sphere& sphere)
{
    minimum_.x = std::min(sphere.getOrigin().x - sphere.getRadius(), minimum_.x);
    minimum_.y = std::min(sphere.getOrigin().y - sphere.getRadius(), minimum_.y);
    minimum_.z = std::min(sphere.getOrigin().z - sphere.getRadius(), minimum_.z);

    maximum_.x = std::max(sphere.getOrigin().x + sphere.getRadius(), maximum_.x);
    maximum_.y = std::max(sphere.getOrigin().y + sphere.getRadius(), maximum_.y);
    maximum_.z = std::max(sphere.getOrigin().z + sphere.getRadius(), maximum_.z);
}

void AABB::getCorners(std::array<Vec3, 8>& corners, const SimpleTransform& transform, const Vec3& scale) const
{
    corners[0] = minimum_;
    corners[1].setXYZ(minimum_.x, minimum_.y, maximum_.z);
    corners[2].setXYZ(minimum_.x, maximum_.y, minimum_.z);
    corners[3].setXYZ(minimum_.x, maximum_.y, maximum_.z);
    corners[4].setXYZ(maximum_.x, minimum_.y, minimum_.z);
    corners[5].setXYZ(maximum_.x, minimum_.y, maximum_.z);
    corners[6].setXYZ(maximum_.x, maximum_.y, minimum_.z);
    corners[7] = maximum_;

    if (transform != SimpleTransform::Identity || scale != Vec3::One)
    {
        for (auto& corner : corners)
            corner = transform * (corner * scale);
    }
}

void AABB::getEdges(std::array<Line, 12>& edges, const SimpleTransform& transform) const
{
    auto corners = std::array<Vec3, 8>();
    getCorners(corners, transform);

    static const auto edgeIndices = std::array<std::pair<unsigned int, unsigned int>, 12>{
        {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {4, 5}, {4, 6}, {5, 7}, {6, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}}};

    for (auto i = 0U; i < 12; i++)
    {
        edges[i].setOrigin(corners[edgeIndices[i].first]);
        edges[i].setEnd(corners[edgeIndices[i].second]);
    }
}

float AABB::getVolume() const
{
    if (maximum_.x < minimum_.x || maximum_.y < minimum_.y || maximum_.z < minimum_.z)
        return 0.0f;

    return (maximum_.x - minimum_.x) * (maximum_.y - minimum_.y) * (maximum_.z - minimum_.z);
}

void AABB::setFromTriangles(const TriangleArray& triangles)
{
    if (triangles.empty())
        return;

    minimum_ = triangles[0].getVertexPosition(0);
    maximum_ = minimum_;

    for (auto& triangle : triangles)
    {
        for (auto i = 0U; i < 3; i++)
            addPoint(triangle.getVertexPosition(i));
    }
}

void AABB::setFromTriangleSet(const TriangleArraySet& triangleSet)
{
    if (triangleSet.empty())
        return;

    minimum_ = triangleSet[0][0].getVertexPosition(0);
    maximum_ = minimum_;

    for (auto triangles : triangleSet)
    {
        for (auto& triangle : *triangles)
        {
            for (auto i = 0U; i < 3; i++)
                addPoint(triangle.getVertexPosition(i));
        }
    }
}

Vec3 AABB::clipPoint(const Vec3& p) const
{
    return {Math::clamp(p.x, minimum_.x, maximum_.x), Math::clamp(p.y, minimum_.y, maximum_.y),
            Math::clamp(p.z, minimum_.z, maximum_.z)};
}

void AABB::merge(const AABB& aabb, const SimpleTransform& transform, const Vec3& scale)
{
    auto corners = std::array<Vec3, 8>();
    aabb.getCorners(corners);

    for (auto& corner : corners)
        addPoint(transform * (corner * scale));
}

ConvexHull AABB::getConvexHull() const
{
    auto convexHull = ConvexHull();

    convexHull.setPlaneCount(6);

    convexHull.setPlane(0, Plane(minimum_, -Vec3::UnitX));
    convexHull.setPlane(1, Plane(minimum_, -Vec3::UnitY));
    convexHull.setPlane(2, Plane(minimum_, -Vec3::UnitZ));
    convexHull.setPlane(3, Plane(maximum_, Vec3::UnitX));
    convexHull.setPlane(4, Plane(maximum_, Vec3::UnitY));
    convexHull.setPlane(5, Plane(maximum_, Vec3::UnitZ));

    return convexHull;
}

bool AABB::intersect(const Ray& ray, float* t) const
{
    if (intersect(ray.getOrigin()))
        return true;

    // Ray-box intersection using IEEE numerical properties to ensure that the test is both robust and efficient, as described
    // in "An Efficient and Robust Ray-Box Intersection Algorithm" by Amy Williams, Steve Barrus, R. Keith Morley, and Peter
    // Shirley. Journal of graphics tools, 10(1):49-54, 2005.

    auto bbox = std::array<Vec3, 2>{{minimum_, maximum_}};

    auto inverseDirection = Vec3::One / ray.getDirection();

    auto sign = std::array<int, 3>{
        {inverseDirection.x < 0.0f ? 1 : 0, inverseDirection.y < 0.0f ? 1 : 0, inverseDirection.z < 0.0f ? 1 : 0}};

    auto tmin = (bbox[sign[0]].x - ray.getOrigin().x) * inverseDirection.x;
    auto tmax = (bbox[1 - sign[0]].x - ray.getOrigin().x) * inverseDirection.x;

    auto tymin = (bbox[sign[1]].y - ray.getOrigin().y) * inverseDirection.y;
    auto tymax = (bbox[1 - sign[1]].y - ray.getOrigin().y) * inverseDirection.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    auto tzmin = (bbox[sign[2]].z - ray.getOrigin().z) * inverseDirection.z;
    auto tzmax = (bbox[1 - sign[2]].z - ray.getOrigin().z) * inverseDirection.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tmin < 0.0f)
        return false;

    if (t)
        *t = tmin;

    return true;
}

bool AABB::intersect(const Vec3& v0, const Vec3& v1, const Vec3& v2) const
{
    auto planes = std::array<Plane, 6>{{{minimum_, -Vec3::UnitX},
                                        {minimum_, -Vec3::UnitY},
                                        {minimum_, -Vec3::UnitZ},
                                        {maximum_, Vec3::UnitX},
                                        {maximum_, Vec3::UnitY},
                                        {maximum_, Vec3::UnitZ}}};

    auto triangle = RawTriangle(v0, v1, v2);

    for (auto& plane : planes)
    {
        if (triangle.classify(plane) == Plane::Front)
            return false;
    }

    return true;
}

bool AABB::orientedIntersect(const SimpleTransform& aabb0Transform, const AABB& aabb1,
                             const SimpleTransform& aabb1Transform) const
{
    auto& aabb0 = *this;

    auto corners = std::array<Vec3, 8>();

    // Get corners of aabb1 in the local space of aabb0
    aabb1.getCorners(corners, aabb0Transform.getInverse() * aabb1Transform);

    // Separating axis test
    if ((corners[0].x < aabb0.getMinimum().x && corners[1].x < aabb0.getMinimum().x && corners[2].x < aabb0.getMinimum().x &&
         corners[3].x < aabb0.getMinimum().x && corners[4].x < aabb0.getMinimum().x && corners[5].x < aabb0.getMinimum().x &&
         corners[6].x < aabb0.getMinimum().x && corners[7].x < aabb0.getMinimum().x) ||

        (corners[0].x > aabb0.getMaximum().x && corners[1].x > aabb0.getMaximum().x && corners[2].x > aabb0.getMaximum().x &&
         corners[3].x > aabb0.getMaximum().x && corners[4].x > aabb0.getMaximum().x && corners[5].x > aabb0.getMaximum().x &&
         corners[6].x > aabb0.getMaximum().x && corners[7].x > aabb0.getMaximum().x) ||

        (corners[0].y < aabb0.getMinimum().y && corners[1].y < aabb0.getMinimum().y && corners[2].y < aabb0.getMinimum().y &&
         corners[3].y < aabb0.getMinimum().y && corners[4].y < aabb0.getMinimum().y && corners[5].y < aabb0.getMinimum().y &&
         corners[6].y < aabb0.getMinimum().y && corners[7].y < aabb0.getMinimum().y) ||

        (corners[0].y > aabb0.getMaximum().y && corners[1].y > aabb0.getMaximum().y && corners[2].y > aabb0.getMaximum().y &&
         corners[3].y > aabb0.getMaximum().y && corners[4].y > aabb0.getMaximum().y && corners[5].y > aabb0.getMaximum().y &&
         corners[6].y > aabb0.getMaximum().y && corners[7].y > aabb0.getMaximum().y) ||

        (corners[0].z < aabb0.getMinimum().z && corners[1].y < aabb0.getMinimum().z && corners[2].z < aabb0.getMinimum().z &&
         corners[3].z < aabb0.getMinimum().z && corners[4].z < aabb0.getMinimum().z && corners[5].y < aabb0.getMinimum().z &&
         corners[6].z < aabb0.getMinimum().z && corners[7].z < aabb0.getMinimum().z) ||

        (corners[0].z > aabb0.getMaximum().z && corners[1].y > aabb0.getMaximum().z && corners[2].z > aabb0.getMaximum().z &&
         corners[3].z > aabb0.getMaximum().z && corners[4].z > aabb0.getMaximum().z && corners[5].y > aabb0.getMaximum().z &&
         corners[6].z > aabb0.getMaximum().z && corners[7].z > aabb0.getMaximum().z))
        return false;

    // Get corners of aabb0 in the local space of aabb1
    aabb0.getCorners(corners, aabb1Transform.getInverse() * aabb0Transform);

    // Separating axis test
    return !(
        (corners[0].x < aabb1.getMinimum().x && corners[1].x < aabb1.getMinimum().x && corners[2].x < aabb1.getMinimum().x &&
         corners[3].x < aabb1.getMinimum().x && corners[4].x < aabb1.getMinimum().x && corners[5].x < aabb1.getMinimum().x &&
         corners[6].x < aabb1.getMinimum().x && corners[7].x < aabb1.getMinimum().x) ||

        (corners[0].x > aabb1.getMaximum().x && corners[1].x > aabb1.getMaximum().x && corners[2].x > aabb1.getMaximum().x &&
         corners[3].x > aabb1.getMaximum().x && corners[4].x > aabb1.getMaximum().x && corners[5].x > aabb1.getMaximum().x &&
         corners[6].x > aabb1.getMaximum().x && corners[7].x > aabb1.getMaximum().x) ||

        (corners[0].y < aabb1.getMinimum().y && corners[1].y < aabb1.getMinimum().y && corners[2].y < aabb1.getMinimum().y &&
         corners[3].y < aabb1.getMinimum().y && corners[4].y < aabb1.getMinimum().y && corners[5].y < aabb1.getMinimum().y &&
         corners[6].y < aabb1.getMinimum().y && corners[7].y < aabb1.getMinimum().y) ||

        (corners[0].y > aabb1.getMaximum().y && corners[1].y > aabb1.getMaximum().y && corners[2].y > aabb1.getMaximum().y &&
         corners[3].y > aabb1.getMaximum().y && corners[4].y > aabb1.getMaximum().y && corners[5].y > aabb1.getMaximum().y &&
         corners[6].y > aabb1.getMaximum().y && corners[7].y > aabb1.getMaximum().y) ||

        (corners[0].z < aabb1.getMinimum().z && corners[1].y < aabb1.getMinimum().z && corners[2].z < aabb1.getMinimum().z &&
         corners[3].z < aabb1.getMinimum().z && corners[4].z < aabb1.getMinimum().z && corners[5].y < aabb1.getMinimum().z &&
         corners[6].z < aabb1.getMinimum().z && corners[7].z < aabb1.getMinimum().z) ||

        (corners[0].z > aabb1.getMaximum().z && corners[1].y > aabb1.getMaximum().z && corners[2].z > aabb1.getMaximum().z &&
         corners[3].z > aabb1.getMaximum().z && corners[4].z > aabb1.getMaximum().z && corners[5].y > aabb1.getMaximum().z &&
         corners[6].z > aabb1.getMaximum().z && corners[7].z > aabb1.getMaximum().z));
}

}
