/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/Plane.h"
#include "CarbonEngine/Math/Quaternion.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Holds a simple transform consisting of a position and orientation.
 */
class CARBON_API SimpleTransform
{
public:

    /**
     * The identity transform, has a zero position and an identity orientation quaternion.
     */
    static const SimpleTransform Identity;

    SimpleTransform() {}

    /**
     * Sets the position and orientation of this simple transform.
     */
    SimpleTransform(const Vec3& position, const Quaternion& orientation = Quaternion::Identity)
        : position_(position), orientation_(orientation)
    {
    }

    /**
     * Sets the orientation of this simple transform, the position is zeroed.
     */
    SimpleTransform(const Quaternion& orientation) : orientation_(orientation) {}

    /**
     * Returns the position of this simple transform.
     */
    const Vec3& getPosition() const { return position_; }

    /**
     * Sets the position of this simple transform.
     */
    void setPosition(const Vec3& position) { position_ = position; }

    /**
     * Returns the orientation of this simple transform.
     */
    const Quaternion& getOrientation() const { return orientation_; }

    /**
     * Sets the orientation of this simple transform.
     */
    void setOrientation(const Quaternion& orientation) { orientation_ = orientation; }

    /**
     * Equality operator.
     */
    bool operator==(const SimpleTransform& other) const
    {
        return position_ == other.position_ && orientation_ == other.orientation_;
    }

    /**
     * Inequality operator.
     */
    bool operator!=(const SimpleTransform& other) const
    {
        return position_ != other.position_ || orientation_ != other.orientation_;
    }

    /**
     * Position addition.
     */
    SimpleTransform operator+(const Vec3& p) const { return {position_ + p, orientation_}; }

    /**
     * Position subtraction.
     */
    SimpleTransform operator-(const Vec3& p) const { return {position_ - p, orientation_}; }

    /**
     * Position addition in-place.
     */
    SimpleTransform& operator+=(const Vec3& p)
    {
        position_ += p;
        return *this;
    }

    /**
     * Position subtraction in-place.
     */
    SimpleTransform& operator-=(const Vec3& p)
    {
        position_ -= p;
        return *this;
    }

    /**
     * Returns the product of this transform and the passed transform.
     */
    SimpleTransform operator*(const SimpleTransform& other) const
    {
        return {position_ + orientation_ * other.position_, other.orientation_ * orientation_};
    }

    /**
     * Transforms the passed Vec2.
     */
    Vec2 operator*(const Vec2& p) const { return (position_ + orientation_ * Vec3(p)).toVec2(); }

    /**
     * Transforms the passed Vec3.
     */
    Vec3 operator*(const Vec3& p) const { return position_ + orientation_ * p; }

    /**
     * Transforms the passed Plane.
     */
    Plane operator*(const Plane& plane) const
    {
        return {operator*(-plane.getNormal() * plane.getDistance()), orientation_ * plane.getNormal()};
    }

    /**
     * Transforms the passed Ray.
     */
    Ray operator*(const Ray& ray) const { return {operator*(ray.getOrigin()), orientation_ * ray.getDirection()}; }

    /**
     * Returns the inverse of this transform.
     */
    SimpleTransform getInverse() const { return {orientation_.getInverse() * -position_, orientation_.getInverse()}; }

    /**
     * Returns the direction of this transform, which is defined as its -Z vector.
     */
    Vec3 getDirection() const { return -getOrientation().getZVector(); }

    /**
     * Converts this transform into a 4x4 matrix and puts the result into \a matrix.
     */
    Matrix4 getMatrix() const { return orientation_.getMatrix4(position_); }

    /**
     * Interpolates linearly between this transform and the passed transform, \a t must be between zero and one.
     */
    SimpleTransform interpolate(const SimpleTransform& transform, float t) const
    {
        return {position_ * (1.0f - t) + transform.getPosition() * t, orientation_.slerp(transform.getOrientation(), t)};
    }

    /**
     * Saves this simple transform to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(orientation_, position_); }

    /**
     * Loads this simple transform from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(orientation_, position_); }

    /**
     * Converts this simple transform to a human-readable string.
     */
    operator UnicodeString() const { return UnicodeString() << "position: " << position_ << ", orientation: " << orientation_; }

private:

    Vec3 position_;
    Quaternion orientation_;
};

}
