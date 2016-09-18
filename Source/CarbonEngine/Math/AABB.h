/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/ConvexHull.h"
#include "CarbonEngine/Math/Rect.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * Axis aligned bounding box. Used as coarse bounding of scene objects for culling.
 */
class CARBON_API AABB
{
public:

    /**
     * The largest possible AABB.
     */
    static const AABB Max;

    /**
     * Default constructor. Sets the minimum vector to `FLT_MAX` and the maximum vector to `-FLT_MAX`.
     */
    AABB() : minimum_(FLT_MAX), maximum_(-FLT_MAX) {}

    /**
     * Constructor that sets the minimum and maximum vectors to the given value.
     */
    AABB(const Vec3& initial) : minimum_(initial), maximum_(initial) {}

    /**
     * Constructor that sets the minimum and maximum vectors.
     */
    AABB(const Vec3& minimum, const Vec3& maximum) : minimum_(minimum), maximum_(maximum) {}

    /**
     * Constructor that initializes this AABB to enclose the given AABB after it has undergone the passed
     * transformation.
     */
    AABB(const AABB& aabb, const SimpleTransform& transform);

    /**
     * Constructor that initializes this AABB to enclose the specified points.
     */
    AABB(const Vector<Vec3>& points) : AABB() { addPoints(points); }

    /**
     * AABB equality operator.
     */
    bool operator==(const AABB& other) const { return minimum_ == other.minimum_ && maximum_ == other.maximum_; }

    /**
     * AABB inequality operator.
     */
    bool operator!=(const AABB& other) const { return minimum_ != other.minimum_ || maximum_ != other.maximum_; }

    /**
     * Returns a copy of this AABB offset by the given vector.
     */
    AABB operator+(const Vec3& v) const { return {minimum_ + v, maximum_ + v}; }

    /**
     * Returns a copy of this AABB scaled by the passed values.
     */
    AABB operator*(const Vec3& v) const { return {minimum_ * v, maximum_ * v}; }

    /**
     * Returns the minimum vector for this AABB.
     */
    const Vec3& getMinimum() const { return minimum_; }

    /**
     * Returns the maximum vector for this AABB.
     */
    const Vec3& getMaximum() const { return maximum_; }

    /**
     * Adds a point to include in this AABB. The AABB will expand as required in order to accomodate the point.
     */
    void addPoint(const Vec3& p);

    /**
     * Calls AABB::addPoint() with every point in the passed vector.
     */
    void addPoints(const Vector<Vec3>& points);

    /**
     * Adds a sphere to include in this AABB. The AABB will expand as required in order to accomodate the sphere.
     */
    void addSphere(const Sphere& sphere);

    /**
     * Returns the eight corners of this AABB, transformed by the passed transform.
     */
    void getCorners(std::array<Vec3, 8>& corners, const SimpleTransform& transform = SimpleTransform::Identity,
                    const Vec3& scale = Vec3::One) const;

    /**
     * Returns the twelve edges of this AABB.
     */
    void getEdges(std::array<Line, 12>& edges, const SimpleTransform& transform = SimpleTransform::Identity) const;

    /**
     * Returns the center point of this AABB
     */
    Vec3 getCenter() const { return (minimum_ + maximum_) * 0.5f; }

    /**
     * Returns the width of this AABB.
     */
    float getWidth() const { return maximum_.x - minimum_.x; }

    /**
     * Returns the height of this AABB.
     */
    float getHeight() const { return maximum_.y - minimum_.y; }

    /**
     * Returns the depth of this AABB.
     */
    float getDepth() const { return maximum_.z - minimum_.z; }

    /**
     * Returns the volume of this AABB.
     */
    float getVolume() const;

    /**
     * Returns the width, height and depth of this AABB in a single Vec3.
     */
    Vec3 getDimensions() const { return maximum_ - minimum_; }

    /**
     * Builds the AABB from the given triangles.
     */
    void setFromTriangles(const TriangleArray& triangles);

    /**
     * Builds the AABB from the given triangle set.
     */
    void setFromTriangleSet(const TriangleArraySet& triangleSet);

    /**
     * Clips the given point into this AABB.
     */
    Vec3 clipPoint(const Vec3& p) const;

    /**
     * Enlarges this AABB as necessary in order to completely enclose the given AABB. The passed AABB can have a
     * transform applied to it prior to merging if desired.
     */
    void merge(const AABB& aabb, const SimpleTransform& transform = SimpleTransform::Identity,
               const Vec3& scale = Vec3::One);

    /**
     * Returns a convex hull for this AABB.
     */
    ConvexHull getConvexHull() const;

    /**
     * Returns whether there is an intersection between this AABB and the passed AABB.
     */
    bool intersect(const AABB& aabb) const
    {
        return minimum_.x <= aabb.getMaximum().x && maximum_.x >= aabb.getMinimum().x &&
            minimum_.y <= aabb.getMaximum().y && maximum_.y >= aabb.getMinimum().y &&
            minimum_.z <= aabb.getMaximum().z && maximum_.z >= aabb.getMinimum().z;
    }

    /**
     * Returns whether there is an intersection between this AABB and the passed point.
     */
    bool intersect(const Vec3& point) const
    {
        return minimum_.x <= point.x && maximum_.x >= point.x && minimum_.y <= point.y && maximum_.y >= point.y &&
            minimum_.z <= point.z && maximum_.z >= point.z;
    }

    /**
     * Returns whether there is an intersection between this AABB and the passed sphere.
     */
    bool intersect(const Sphere& sphere) const
    {
        auto& origin = sphere.getOrigin();
        auto radius = sphere.getRadius();

        return minimum_.x < origin.x + radius && maximum_.x > origin.x - radius && minimum_.y < origin.y + radius &&
            maximum_.y > origin.y - radius && minimum_.z < origin.z + radius && maximum_.z > origin.z - radius;
    }

    /**
     * Returns whether there is an intersection between this AABB and the passed ray, and if there is then the
     * intersection distance along the ray is returned in \a t.
     */
    bool intersect(const Ray& ray, float* t = nullptr) const;

    /**
     * Returns whether there is an intersection between this AABB and the passed triangle.
     */
    bool intersect(const Vec3& v0, const Vec3& v1, const Vec3& v2) const;

    /**
     * Checks intersection between two AABBs after each one has been transformed. This implementation uses a separating
     * axis algorithm and does not return any false positives.
     */
    bool orientedIntersect(const SimpleTransform& aabb0Transform, const AABB& aabb1,
                           const SimpleTransform& aabb1Transform) const;

    /**
     * Saves this AABB to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(minimum_, maximum_); }

    /**
     * Loads this AABB from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(minimum_, maximum_); }

    /**
     * Returns a string containing the min and max points that define this AABB, the format is "(min.x min.y, min.z)
     * (max.x max.y max.z)".
     */
    operator UnicodeString() const { return UnicodeString() << "(" << minimum_ << ") (" << maximum_ << ")"; }

    /**
     * Converts this AABB to a 2D rect by dropping the Z component.
     */
    Rect toRect() const { return {minimum_.x, minimum_.y, maximum_.x, maximum_.y}; }

private:

    Vec3 minimum_;
    Vec3 maximum_;
};

}
