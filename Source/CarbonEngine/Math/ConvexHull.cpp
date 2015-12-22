/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Math/ConvexHull.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Math/Sphere.h"

namespace Carbon
{

ConvexHull::ConvexHull(const Matrix4& projectionMatrix, const Matrix4& viewMatrix)
{
    setFromViewProjectionMatrix(projectionMatrix * viewMatrix);
}

void ConvexHull::setFromViewProjectionMatrix(const Matrix4& viewProjection)
{
    const auto& vp = viewProjection;

    setPlaneCount(6);

    // Right side
    planes_[0].setNormal(Vec3(vp[3] - vp[0], vp[7] - vp[4], vp[11] - vp[8]));
    planes_[0].setDistance(vp[15] - vp[12]);
    planes_[0].flip();
    planes_[0].normalize();

    // Left side
    planes_[1].setNormal(Vec3(vp[3] + vp[0], vp[7] + vp[4], vp[11] + vp[8]));
    planes_[1].setDistance(vp[15] + vp[12]);
    planes_[1].flip();
    planes_[1].normalize();

    // Bottom side
    planes_[2].setNormal(Vec3(vp[3] + vp[1], vp[7] + vp[5], vp[11] + vp[9]));
    planes_[2].setDistance(vp[15] + vp[13]);
    planes_[2].flip();
    planes_[2].normalize();

    // Top side
    planes_[3].setNormal(Vec3(vp[3] - vp[1], vp[7] - vp[5], vp[11] - vp[9]));
    planes_[3].setDistance(vp[15] - vp[13]);
    planes_[3].flip();
    planes_[3].normalize();

    // Far side
    planes_[4].setNormal(Vec3(vp[3] - vp[2], vp[7] - vp[6], vp[11] - vp[10]));
    planes_[4].setDistance(vp[15] - vp[14]);
    planes_[4].flip();
    planes_[4].normalize();

    // Near side
    planes_[5].setNormal(Vec3(vp[3] + vp[2], vp[7] + vp[6], vp[11] + vp[10]));
    planes_[5].setDistance(vp[15] + vp[14]);
    planes_[5].flip();
    planes_[5].normalize();
}

ConvexHull ConvexHull::operator*(const SimpleTransform& transform) const
{
    return {planes_.map<Plane>([&](const Plane& plane) { return transform * plane; })};
}

bool ConvexHull::intersect(const Sphere& sphere) const
{
    return !planes_.has([&](const Plane& plane) { return plane.classify(sphere) == Plane::Front; });
}

bool ConvexHull::intersect(const Vec3& point) const
{
    return !planes_.has([&](const Plane& plane) { return plane.classify(point) == Plane::Front; });
}

bool ConvexHull::intersect(const AABB& aabb, const SimpleTransform& transform, const Vec3& scale) const
{
    auto corners = std::array<Vec3, 8>();
    aabb.getCorners(corners, transform, scale);

    for (auto& plane : planes_)
    {
        // Check if the AABB lies on the front of this plane, if so then there is no intersection with this convex hull
        for (auto j = 0U; j < 8; j++)
        {
            if (plane.distance(corners[j]) < 0.0f)
                break;

            if (j == 7)
                return false;
        }
    }

    return true;
}

}
