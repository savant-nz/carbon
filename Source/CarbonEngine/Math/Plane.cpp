/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/Line.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Plane.h"
#include "CarbonEngine/Math/Ray.h"

namespace Carbon
{

Plane::ClassifyResult Plane::classify(const Vec3& p) const
{
    auto d = distance(p);

    if (d > Math::Epsilon)
        return Front;

    if (d < -Math::Epsilon)
        return Back;

    return Coincident;
}

Plane::ClassifyResult Plane::classify(const Sphere& sphere) const
{
    auto d = distance(sphere.getOrigin());

    if (d > sphere.getRadius())
        return Front;

    if (d < -sphere.getRadius())
        return Back;

    return Spanning;
}

Plane::ClassifyResult Plane::classify(const Vec3& v0, const Vec3& v1, const Vec3& v2) const
{
    auto frontCount = 0U;
    auto backCount = 0U;
    auto onPlaneCount = 0U;

    auto points = std::array<const Vec3*, 3>{{&v0, &v1, &v2}};

    for (auto point : points)
    {
        auto result = classify(*point);

        if (result == Front)
            frontCount++;
        else if (result == Back)
            backCount++;
        else if (result == Coincident)
            onPlaneCount++;
    }

    if (onPlaneCount == 3)
        return Coincident;

    if (frontCount && !backCount)
        return Front;

    if (backCount && !frontCount)
        return Back;

    return Spanning;
}

bool Plane::intersect(const Ray& ray, float& t) const
{
    auto denom = normal_.dot(ray.getDirection());
    if (fabsf(denom) < Math::Epsilon)
        return false;

    t = -distance(ray.getOrigin()) / denom;

    return true;
}

bool Plane::intersect(const Ray& ray, Vec3& p) const
{
    auto t = 0.0f;
    if (!intersect(ray, t))
        return false;

    p = ray.getOrigin() + ray.getDirection() * t;

    return true;
}

bool Plane::intersect(const Line& line, float& t) const
{
    auto ray = Ray(line.getOrigin(), line.getDirection());

    if (!intersect(ray, t))
        return false;

    t /= line.calculateLength();

    return t < 1.0f;
}

bool Plane::intersect(const Line& line, Vec3& p) const
{
    if (!intersect(Ray(line.getOrigin(), line.getDirection()), p))
        return false;

    return line.getOrigin().distance(p) <= line.calculateLength();
}

bool Plane::clipConvexPolygon(Vector<Vec3>& vertices) const
{
    if (vertices.size() < 3)
        return false;

    // Classify all the vertices against this plane
    auto classifications = Vector<Plane::ClassifyResult>(vertices.size());
    auto frontCount = 0U;
    auto backCount = 0U;
    for (auto j = 0U; j < vertices.size(); j++)
    {
        classifications[j] = classify(vertices[j]);

        if (classifications[j] == Plane::Front)
            frontCount++;
        else if (classifications[j] == Plane::Back)
            backCount++;
    }

    // If nothing is in front of this plane then there is nothing to do
    if (frontCount == 0)
        return true;

    // If everything is in front of this plane then the whole polygon is clipped
    if (backCount == 0)
    {
        vertices.clear();
        return false;
    }

    auto newVertices = Vector<Vec3>();
    auto intersectionPoint = Vec3();

    // Clip the polygon against this plane
    for (auto j = 0U; j < vertices.size(); j++)
    {
        auto previous = (j + vertices.size() - 1) % vertices.size();

        if (classifications[j] == Plane::Front)
        {
            if (classifications[previous] == Plane::Back)
            {
                intersect(Line(vertices[j], vertices[previous]), intersectionPoint);
                newVertices.append(intersectionPoint);
            }
        }
        else
        {
            if (classifications[j] == Plane::Back && classifications[previous] == Plane::Front)
            {
                intersect(Line(vertices[j], vertices[previous]), intersectionPoint);
                newVertices.append(intersectionPoint);
            }

            newVertices.append(vertices[j]);
        }
    }

    swap(vertices, newVertices);

    return vertices.size() >= 3;
}

}
