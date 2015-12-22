/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Simple line primitive with origin and end points.
 */
class CARBON_API Line
{
public:

    Line() {}

    /**
     * Constructs this line with the given origin and end point.
     */
    Line(const Vec3& origin, const Vec3& end) : origin_(origin), end_(end) {}

    /**
     * Returns the origin of this line.
     */
    const Vec3& getOrigin() const { return origin_; }

    /**
     * Sets the origin of this line.
     */
    void setOrigin(const Vec3& origin) { origin_ = origin; }

    /**
     * Returns the end of this line.
     */
    const Vec3& getEnd() const { return end_; }

    /**
     * Sets the end of this line.
     */
    void setEnd(const Vec3& end) { end_ = end; }

    /**
     * Calculates the length of this line.
     */
    float calculateLength() const { return origin_.distance(end_); }

    /**
     * Calculates the normalized direction of this line.
     */
    Vec3 getDirection() const { return (end_ - origin_).normalized(); }

    /**
     * Returns the closest point on this line to the point \a p.
     */
    Vec3 getClosestPoint(const Vec3& p) const
    {
        auto direction = end_ - origin_;
        auto denom = direction.dot(direction);

        // Project the point p onto this line
        return denom == 0.0f ? origin_ : (origin_ + direction * Math::clamp01(direction.dot(p - origin_) / denom));
    }

private:

    Vec3 origin_;
    Vec3 end_;
};

}
