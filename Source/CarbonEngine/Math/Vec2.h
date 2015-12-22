/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

/**
 * Two component vector.
 */
class CARBON_API Vec2
{
public:

    /**
     * The x component of this vector.
     */
    float x = 0.0f;

    /**
     * The y component of this vector.
     */
    float y = 0.0f;

    Vec2() {}

    /**
     * Scalar constructor. Sets \a x and \a y to \a f.
     */
    explicit Vec2(float f) : x(f), y(f) {}

    /**
     * Component constructor. Sets \a x and \a y.
     */
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    /**
     * Component equality. This is not a fuzzy comparison.
     */
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }

    /**
     * Component inequality. This is not a fuzzy comparison.
     */
    bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }

    /**
     * Component comparison. Allows this class to be sorted.
     */
    bool operator<(const Vec2& other) const { return x == other.x ? y < other.y : x < other.x; }

    /**
     * Component addition.
     */
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }

    /**
     * Component addition in place.
     */
    Vec2& operator+=(const Vec2& other)
    {
        x += other.x;
        y += other.y;

        return *this;
    }

    /**
     * Component negation.
     */
    Vec2 operator-() const { return {-x, -y}; }

    /**
     * Component subtraction.
     */
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }

    /**
     * Component subtraction in place.
     */
    Vec2& operator-=(const Vec2& other)
    {
        x -= other.x;
        y -= other.y;

        return *this;
    }

    /**
     * Scalar multiplication.
     */
    Vec2 operator*(float f) const { return {x * f, y * f}; }

    /**
     * Component multiplication.
     */
    Vec2 operator*(const Vec2& other) const { return {x * other.x, y * other.y}; }

    /**
     * Scalar multiplication in place.
     */
    Vec2& operator*=(float f)
    {
        x *= f;
        y *= f;

        return *this;
    }

    /**
     * Component multiplication in place.
     */
    Vec2& operator*=(const Vec2& other)
    {
        x *= other.x;
        y *= other.y;

        return *this;
    }

    /**
     * Scalar division.
     */
    Vec2 operator/(float f) const
    {
        f = 1.0f / f;

        return {x * f, y * f};
    }

    /**
     * Component division.
     */
    Vec2 operator/(const Vec2& other) const { return {x / other.x, y / other.y}; }

    /**
     * Scalar division in place.
     */
    Vec2& operator/=(float f)
    {
        f = 1.0f / f;

        x *= f;
        y *= f;

        return *this;
    }

    /**
     * Component division in place.
     */
    Vec2& operator/=(const Vec2& other)
    {
        x /= other.x;
        y /= other.y;

        return *this;
    }

    /**
     * Calculates the dot product between two vectors.
     */
    float dot(const Vec2& v) const { return x * v.x + y * v.y; }

    /**
     * Calculates the length of this vector.
     */
    float length() const { return sqrtf(x * x + y * y); }

    /**
     * Calculates the squared length of this vector.
     */
    float lengthSquared() const { return x * x + y * y; }

    /**
     * Normalizes this vector.
     */
    void normalize()
    {
        auto l = length();

        if (l == 0.0f)
        {
#ifdef CARBON_DEBUG
            LOG_WARNING << "Vector has a length of zero";
#endif
            return;
        }

        operator*=(1.0f / l);
    }

    /**
     * Returns this vector normalized.
     */
    Vec2 normalized() const
    {
        auto l = length();

        if (l == 0.0f)
        {
#ifdef CARBON_DEBUG
            LOG_WARNING << "Vector has a length of zero";
#endif
            return Zero;
        }

        return operator*(1.0f / l);
    }

    /**
     * Returns whether or not this vector is currently normalized.
     */
    bool isNormalized() const { return fabsf(lengthSquared() - 1.0f) < Math::Epsilon; }

    /**
     * Calculates the distance between two points.
     */
    float distance(const Vec2& v) const { return sqrtf((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y)); }

    /**
     * Calculates the distance between two points.
     */
    float distanceSquared(const Vec2& v) const { return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y); }

    /**
     * Returns a vector with the direction of this vector but with the given length
     */
    Vec2 ofLength(float f) const
    {
        auto l = length();

        if (l < Math::Epsilon)
            return Vec2::Zero;

        return operator*(f / l);
    }

    /**
     * Sets all the components of this vector to the same value.
     */
    void setXY(float value) { x = y = value; }

    /**
     * Sets the components of this vector to the given values.
     */
    void setXY(float x_, float y_)
    {
        x = x_;
        y = y_;
    }

    /**
     * Returns the value of the largest component of this vector.
     */
    float getLargestComponent() const { return y > x ? y : x; }

    /**
     * Returns this Vec2 value as a float[2] array.
     */
    const float* asArray() const { return &x; }

    /**
     * Returns this vector as a string in the form "x y".
     */
    operator UnicodeString() const { return UnicodeString() << x << " " << y; }

    /**
     * Returns whether both the components of this vector are finite.
     */
    bool isFinite() const { return std::isfinite(x) && std::isfinite(y); }

    /**
     * Creates a vector in cartesian coordinates from the given polar coordinates.
     */
    static Vec2 fromPolarCoordinates(float radius, float angle) { return {radius * cosf(angle), radius * sinf(angle)}; }

    /**
     * Converts this vector into polar coordinates, the result is returned in \a radius and \a angle.
     */
    void toPolarCoordinates(float& radius, float& angle) const
    {
        radius = sqrtf(y * y + x * x);
        angle = atan2f(y, x);
    }

    /**
     * Saves this vector to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(x, y); }

    /**
     * Loads this vector from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(x, y); }

    /**
     * Vec2 instance with all components set to zero.
     */
    static const Vec2 Zero;

    /**
     * Vec2 instance with all components set to one.
     */
    static const Vec2 One;

    /**
     * Vec2 instance with all components set to 0.5.
     */
    static const Vec2 Half;

    /**
     * Vec2 instance with the x component set to one and the y component set to zero.
     */
    static const Vec2 UnitX;

    /**
     * Vec2 instance with the y component set to one and the x component set to zero.
     */
    static const Vec2 UnitY;
};

}
