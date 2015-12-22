/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Useful interpolation functions.
 */
class CARBON_API Interpolate
{
public:

    /**
     * Linear interpolation between two values.
     */
    template <typename T> static T linear(const T& start, const T& end, float t) { return T(start * (1.0f - t) + end * t); }

    /**
     * Cosine interpolation between two values.
     */
    template <typename T> static T cosine(const T& start, const T& end, float t)
    {
        t = 0.5f - 0.5f * cosf(t * Math::Pi);
        return start * (1.0f - t) + end * t;
    }

    /**
     * Fast and smooth interpolation between two values.
     */
    template <typename T> static T fast(const T& start, const T& end, float t)
    {
        auto omt = 1.0f - t;

        return start * (3.0f * omt * omt - 2.0f * omt * omt * omt) + end * (3.0f * t * t - 2.0f * t * t * t);
    }

    /**
     * Does smooth cubic interpolation between the specified set of points. Note that control points are automatically
     * calculated to ensure that the returned path will pass through each point. The passed \a t value should be in the range
     * 0-1.
     */
    static Vec3 cubic(const Vector<Vec3>& points, float t)
    {
        if (points.size() < 3)
            return Vec3::Zero;

        t = Math::clamp01(t);

        // Get the 4 points that define this segment of the curve
        auto index = int(t * float(points.size()));
        auto p = std::array<Vec3, 4>{{points[(index + points.size() - 1) % points.size()], points[index % points.size()],
                                      points[(index + 1) % points.size()], points[(index + 2) % points.size()]}};

        // Calculate midpoints
        auto m = std::array<Vec3, 3>{{(p[0] + p[1]) * 0.5f, (p[1] + p[2]) * 0.5f, (p[2] + p[3]) * 0.5f}};

        // Calculate control points
        auto c =
            std::array<Vec3, 2>{{p[1] + (m[1] - m[0]) * (p[1].distance(p[2]) / (p[0].distance(p[1]) + p[1].distance(p[2]))),
                                 p[2] - (m[2] - m[1]) * (p[2].distance(p[3]) / (p[1].distance(p[2]) + p[2].distance(p[3])))}};

        // Get interpolation fraction
        t = t * float(points.size()) - float(index);

        // Do the cubic interpolation
        auto result = p[1] * (1.0f - t) * (1.0f - t) * (1.0f - t);
        result += c[0] * 3 * (1.0f - t) * (1.0f - t) * t;
        result += c[1] * 3 * (1.0f - t) * t * t;
        result += p[2] * t * t * t;

        return result;
    }
};

}
