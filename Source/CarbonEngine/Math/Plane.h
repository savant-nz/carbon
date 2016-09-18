/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Sphere.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Plane class.
 */
class CARBON_API Plane
{
public:

    Plane() {}

    /**
     * Constructs this plane from a point and a normal. The normal must already be normalized.
     */
    Plane(const Vec3& p, const Vec3& normal) : normal_(normal), distance_(-normal_.dot(p)) {}

    /**
     * Constructs this plane from three points.
     */
    Plane(const Vec3& p0, const Vec3& p1, const Vec3& p2)
        : normal_(normalFromPoints(p0, p1, p2)), distance_(-normal_.dot(p0))
    {
    }

    /**
     * Returns the normal of this plane.
     */
    const Vec3& getNormal() const { return normal_; }

    /**
     * Sets the normal of this plane.
     */
    void setNormal(const Vec3& normal) { normal_ = normal; }

    /**
     * Returns the distance value of this plane.
     */
    float getDistance() const { return distance_; }

    /**
     * Sets the distance value of this plane.
     */
    void setDistance(float distance) { distance_ = distance; }

    /**
     * Equality operator.
     */
    bool operator==(const Plane& other) const
    {
        return normal_ == other.getNormal() && distance_ == other.getDistance();
    }

    /**
     * Returns the signed distance from a point to this plane.
     */
    float distance(const Vec3& p) const { return normal_.dot(p) + distance_; }

    /**
     * Geometric classification results.
     */
    enum ClassifyResult
    {
        Front,
        Back,
        Coincident,
        Spanning
    };

    /**
     * Classifies a point against this plane.
     */
    ClassifyResult classify(const Vec3& p) const;

    /**
     * Classifies a sphere against this plane.
     */
    ClassifyResult classify(const Sphere& sphere) const;

    /**
     * Classifies a triangle against this plane.
     */
    ClassifyResult classify(const Vec3& v0, const Vec3& v1, const Vec3& v2) const;

    /**
     * Offsets a point along the normal until it is a certain distance away from this plane.
     */
    Vec3 offset(const Vec3& p, float offset) const { return p - (normal_ * (distance(p) - offset)); }

    /**
     * Reflects the given point in this plane.
     */
    Vec3 reflect(const Vec3& p) const { return p - normal_ * distance(p) * 2.0f; }

    /**
     * Flips this plane, the normal and distance are both negated.
     */
    void flip()
    {
        normal_ = -normal_;
        distance_ = -distance_;
    }

    /**
     * Normalizes this plane. Normalizes the normal and scales the distance by the same amount.
     */
    void normalize()
    {
        auto inverseLength = 1.0f / normal_.length();
        normal_ *= inverseLength;
        distance_ *= inverseLength;
    }

    /**
     * Returns whether this plane intersects with the ray, if there is an intersection then the distance to the
     * intersection point is returned in \a t.
     */
    bool intersect(const Ray& ray, float& t) const;

    /**
     * Returns whether this plane intersects with the ray, if there is an intersection then the intersection point is
     * returned in \a p.
     */
    bool intersect(const Ray& ray, Vec3& p) const;

    /**
     * Returns whether this plane intersects with the passed line, if there is an intersection then the fraction along
     * the line that the intersection occurred is returned in \a t.
     */
    bool intersect(const Line& line, float& t) const;

    /**
     * Returns whether this plane intersects with the passed line, if there is an intersection then the intersection
     * point is returned in \a p.
     */
    bool intersect(const Line& line, Vec3& p) const;

    /**
     * Clips away all parts of the passed planar polygon that lie on the front of this plane. If the return value is
     * false then there was nothing left of the original polygon after clipping.
     */
    bool clipConvexPolygon(Vector<Vec3>& vertices) const;

    /**
     * Saves this plane to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(normal_, distance_); }

    /**
     * Loads this plane from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(normal_, distance_); }

    /**
     * Returns this plane as a string in the format "(nx ny nz) d".
     */
    operator UnicodeString() const { return UnicodeString() << "(" << normal_ << ") " << distance_; }

    /**
     * Calculates the normal vector for the plane defined by the given three points.
     */
    static Vec3 normalFromPoints(const Vec3& p0, const Vec3& p1, const Vec3& p2)
    {
        return (p2 - p1).cross(p0 - p1).normalized();
    }

private:

    Vec3 normal_;
    float distance_ = 0.0f;
};

}
