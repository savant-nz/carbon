/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Vec2.h"

namespace Carbon
{

/**
 * Two component integer vector.
 */
class CARBON_API Vec2i
{
public:

    /**
     * The x component of this vector.
     */
    int x = 0;

    /**
     * The y component of this vector.
     */
    int y = 0;

    Vec2i() {}

    /**
     * Scalar constructor. Sets \a x and \a y to \a i.
     */
    explicit Vec2i(int i) : x(i), y(i) {}

    /**
     * Constructs this integer vector from the passed floating point vector, the x and y values are cast to integers.
     */
    explicit Vec2i(const Vec2& v) : x(int(v.x)), y(int(v.y)) {}

    /**
     * Component constructor. Sets \a x and \a y.
     */
    Vec2i(int x_, int y_) : x(x_), y(y_) {}

    /**
     * Component equality.
     */
    bool operator==(const Vec2i& other) const { return x == other.x && y == other.y; }

    /**
     * Component inequality.
     */
    bool operator!=(const Vec2i& other) const { return x != other.x || y != other.y; }

    /**
     * Component comparison. Allows this class to be sorted.
     */
    bool operator<(const Vec2i& other) const { return x == other.x ? y < other.y : x < other.x; }

    /**
     * Component addition.
     */
    Vec2i operator+(const Vec2i& other) const { return {x + other.x, y + other.y}; }

    /**
     * Component addition in place.
     */
    Vec2i& operator+=(const Vec2i& other)
    {
        x += other.x;
        y += other.y;

        return *this;
    }

    /**
     * Component negation.
     */
    Vec2i operator-() const { return {-x, -y}; }

    /**
     * Component subtraction.
     */
    Vec2i operator-(const Vec2i& other) const { return {x - other.x, y - other.y}; }

    /**
     * Component subtraction in place.
     */
    Vec2i& operator-=(const Vec2i& other)
    {
        x -= other.x;
        y -= other.y;

        return *this;
    }

    /**
     * Scalar multiplication.
     */
    Vec2i operator*(int i) const { return {x * i, y * i}; }

    /**
     * Component multiplication.
     */
    Vec2i operator*(const Vec2i& other) const { return {x * other.x, y * other.y}; }

    /**
     * Scalar multiplication in place.
     */
    Vec2i& operator*=(int i)
    {
        x *= i;
        y *= i;

        return *this;
    }

    /**
     * Component multiplication in place.
     */
    Vec2i& operator*=(const Vec2i& other)
    {
        x *= other.x;
        y *= other.y;

        return *this;
    }

    /**
     * Scalar division.
     */
    Vec2i operator/(int i) const { return {x / i, y / i}; }

    /**
     * Component division.
     */
    Vec2i operator/(const Vec2i& other) const { return {x / other.x, y / other.y}; }

    /**
     * Scalar division in place.
     */
    Vec2i& operator/=(int i)
    {
        x /= i;
        y /= i;

        return *this;
    }

    /**
     * Component division in place.
     */
    Vec2i& operator/=(const Vec2i& other)
    {
        x /= other.x;
        y /= other.y;

        return *this;
    }

    /**
     * Calculates the dot product between two vectors.
     */
    float dot(const Vec2i& v) const { return float(x * v.x + y * v.y); }

    /**
     * Calculates the length of this vector.
     */
    float length() const { return sqrtf(float(x * x + y * y)); }

    /**
     * Calculates the squared length of this vector.
     */
    float lengthSquared() const { return float(x * x + y * y); }

    /**
     * Calculates the distance between two points.
     */
    float distance(const Vec2i& v) const { return sqrtf(float((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y))); }

    /**
     * Calculates the distance between two points.
     */
    float distanceSquared(const Vec2i& v) const { return float((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y)); }

    /**
     * Sets the components of this vector to the given values.
     */
    void setXY(int x_, int y_)
    {
        x = x_;
        y = y_;
    }

    /**
     * Returns the value of the largest component of this vector.
     */
    int getLargestComponent() const { return y > x ? y : x; }

    /**
     * Returns this vector as a string in the form "x y".
     */
    operator UnicodeString() const { return UnicodeString() << x << " " << y; }

    /**
     * Automatic conversion to floating point Vec2.
     */
    operator Vec2() const { return {float(x), float(y)}; }

    /**
     * Saves this vector to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(x, y); }

    /**
     * Loads this vector from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(x, y); }

    /**
     * Vec2i instance with all components set to zero.
     */
    static const Vec2i Zero;

    /**
     * Vec2i instance with all components set to one.
     */
    static const Vec2i One;

    /**
     * Vec2i instance with the x component set to one and the y component set to zero.
     */
    static const Vec2i UnitX;

    /**
     * Vec2i instance with the y component set to one and the x component set to zero.
     */
    static const Vec2i UnitY;
};

}
