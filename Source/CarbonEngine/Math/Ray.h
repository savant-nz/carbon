/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * 3D ray consisting of an origin and a direction.
 */
class CARBON_API Ray
{
public:

    Ray() {}

    /**
     * Constructs the ray from an origin and a direction, the direction vector is automatically normalized.
     */
    Ray(const Vec3& origin, const Vec3& direction) : origin_(origin), direction_(direction)
    {
        if (direction_ != Vec3::Zero)
            direction_.normalize();
    }

    /**
     * Returns the origin of this ray.
     */
    Vec3 getOrigin() const { return origin_; }

    /**
     * Sets the origin of this ray.
     */
    void setOrigin(const Vec3& origin) { origin_ = origin; }

    /**
     * Returns the normalized direction vector of the ray.
     */
    Vec3 getDirection() const { return direction_; }

    /**
     * Returns the point that is the given distance along this ray.
     */
    Vec3 getPoint(float distance) const { return origin_ + direction_ * distance; }

    /**
     * Returns whether there is an intersection between this ray and the passed triangle, and if there is then the distance
     * along this ray to the intersection is returned in \a t.
     */
    bool intersect(const Vec3& v0, const Vec3& v1, const Vec3& v2, float* t = nullptr) const
    {
        // Edge vectors
        auto edge1 = v1 - v0;
        auto edge2 = v2 - v0;

        // Begin calculating determinant
        auto pvec = direction_.cross(edge2);

        // If determinant is near zero, then the ray is on the triangle
        auto det = edge1.dot(pvec);
        if (det < Math::Epsilon)
            return false;

        // Calculate distance from vert0 to ray origin
        auto tvec = origin_ - v0;

        // Calculate U parameter and test bounds
        auto u = tvec.dot(pvec);
        if (u < 0.0f || u > det)
            return false;

        // Prepare to test V parameter
        auto qvec = tvec.cross(edge1);

        // Calculate V parameter and test bounds
        auto v = direction_.dot(qvec);
        if (v < 0.0f || u + v > det)
            return false;

        // Calculate t to find where ray intersects triangle
        auto tt = edge2.dot(qvec);
        if (t)
            *t = tt / det;

        return tt >= 0.0f;
    }

private:

    Vec3 origin_;
    Vec3 direction_;
};

}
