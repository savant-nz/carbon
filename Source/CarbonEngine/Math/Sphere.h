/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Spherical bounding volume.
 */
class CARBON_API Sphere
{
public:

    Sphere() {}

    /**
     * Constructs the sphere from an origin and a radius.
     */
    Sphere(const Vec3& origin, float radius) : origin_(origin), radius_(radius) {}

    /**
     * Constructs the sphere so that it bounds the passed AABB.
     */
    explicit Sphere(const AABB& aabb);

    /**
     * Returns the origin of this sphere.
     */
    const Vec3& getOrigin() const { return origin_; }

    /**
     * Sets the origin of this sphere.
     */
    void setOrigin(const Vec3& origin) { origin_ = origin; }

    /**
     * Returns the radius of this sphere.
     */
    float getRadius() const { return radius_; }

    /**
     * Sets the radius of this sphere.
     */
    void setRadius(float radius) { radius_ = radius; }

    /**
     * Returns the diameter of this sphere.
     */
    float getDiameter() const { return radius_ * 2.0f; }

    /**
     * Sets the diameter of this sphere.
     */
    void setDiameter(float diameter) { radius_ = diameter * 0.5f; }

    /**
     * Returns a copy of this sphere transformed and scaled by the specified values. The radius is scaled by the largest
     * component of \a scale.
     */
    Sphere getTransformedAndScaled(const SimpleTransform& transform, const Vec3& scale) const;

    /**
     * Returns a copy of this sphere offset by the given vector.
     */
    Sphere operator+(const Vec3& v) const { return {origin_ + v, radius_}; }

    /**
     * Saves this sphere to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(origin_, radius_); }

    /**
     * Loads this sphere from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(origin_, radius_); }

    /**
     * Enlarges this sphere so that the given point is contained within it.
     */
    void merge(const Vec3& p);

    /**
     * Enlarges this sphere so that the given sphere is completely contained within it. If the given sphere is already contained
     * in this sphere then this method does nothing. The given sphere can have a scale factor applied prior to merging.
     */
    void merge(const Sphere& sphere, float scale = 1.0f);

    /**
     * Emits a warning if this sphere is not well formed, meaning that one or more of its parts of its definition are not finite
     * or are huge unrealistic values.
     */
    void warnIfNotWellFormed() const;

    /**
     * Returns whether there is an intersection between this sphere and the passed ray, and if there is the distance along the
     * ray to the intersection point is returned in \a t.
     */
    bool intersect(const Ray& ray, float* t = nullptr) const;

    /**
     * Returns whether there is an intersection between this sphere and the passed sphere.
     */
    bool intersect(const Sphere& sphere) const { return origin_.distance(sphere.getOrigin()) < (radius_ + sphere.getRadius()); }

    /**
     * Returns whether there is an intersection between this sphere and the passed point.
     */
    bool intersect(const Vec3& point) { return (origin_ - point).length() < radius_; }

    /**
     * Returns a string containing this sphere's origin and radius, the format is "(origin.x origin.y, origin.z) radius".
     */
    operator UnicodeString() const { return UnicodeString() << "(" << origin_ << ") " << radius_; }

private:

    Vec3 origin_;
    float radius_ = 0.0f;
};

}
