/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Plane.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * Convex hull bounding volume defined by a set of outward-facing planes, at least 4 planes are required to define a closed
 * volume.
 */
class CARBON_API ConvexHull
{
public:

    ConvexHull() {}

    /**
     * Constructs this convex hull from the given vector of planes.
     */
    ConvexHull(Vector<Plane> planes) : planes_(std::move(planes)) {}

    /**
     * Constructs this convex hull from the six world space frustum planes defined by the passed projection and view matrices.
     */
    ConvexHull(const Matrix4& projectionMatrix, const Matrix4& viewMatrix);

    /**
     * Constructs this convex hull from the six world space frustum planes defined by the passed view-projection matrix.
     */
    ConvexHull(const Matrix4& viewProjectionMatrix) { setFromViewProjectionMatrix(viewProjectionMatrix); }

    /**
     * Sets this convex hull to the six world space frustum planes defined by the passed view-projection matrix.
     */
    void setFromViewProjectionMatrix(const Matrix4& viewProjection);

    /**
     * Returns the number of planes in this convex hull.
     */
    unsigned int getPlaneCount() const { return planes_.size(); }

    /**
     * Sets the number of planes in this convex hull.
     */
    void setPlaneCount(unsigned int count) { planes_.resize(count); }

    /**
     * Returns the plane in this convex hull at the specified index.
     */
    const Plane& getPlane(unsigned int index) const { return planes_[index]; }

    /**
     * Sets the plane in this convex hull at the specified index.
     */
    void setPlane(unsigned int index, const Plane& plane) { planes_[index] = plane; }

    /**
     * Returns a copy of this this convex hull after having undergone the specified transformation.
     */
    ConvexHull operator*(const SimpleTransform& transform) const;

    /**
     * Checks for intersection between this convex hull and the passed sphere.
     */
    bool intersect(const Sphere& sphere) const;

    /**
     * Checks whether the passed point lies inside this convex hull.
     */
    bool intersect(const Vec3& point) const;

    /**
     * Checks for intersection between this convex hull and the passed AABB. The AABB can optionally be transformed and scaled
     * prior to the intersection test.
     */
    bool intersect(const AABB& aabb, const SimpleTransform& transform = SimpleTransform::Identity,
                   const Vec3& scale = Vec3::One) const;

    /**
     * Saves this convex hull to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(planes_); }

    /**
     * Loads this convex hull from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(planes_); }

private:

    Vector<Plane> planes_;
};

}
