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
 * Three component vector.
 */
class CARBON_API Vec3
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

    /**
     * The z component of this vector.
     */
    float z = 0.0f;

    /**
     * Default constructor. Sets \a x, \a y and \a z to zero.
     */
    Vec3() {}

    /**
     * Scalar constructor.
     */
    explicit Vec3(float f) : x(f), y(f), z(f) {}

    /**
     * Component constructor.
     */
    Vec3(float x_, float y_, float z_ = 0.0f) : x(x_), y(y_), z(z_) {}

    /**
     * Copies the \a x and \a y values from a Vec2 instance. \a z is set directly.
     */
    Vec3(const Vec2& v, float z_ = 0.0f) : x(v.x), y(v.y), z(z_) {}

    /**
     * Component equality. This is not a fuzzy comparison.
     */
    bool operator==(const Vec3& other) const { return x == other.x && y == other.y && z == other.z; }

    /**
     * Component inequality. This is not a fuzzy comparison.
     */
    bool operator!=(const Vec3& other) const { return x != other.x || y != other.y || z != other.z; }

    /**
     * Component comparison. Allows this class to be sorted.
     */
    bool operator<(const Vec3& other) const
    {
        if (x == other.x)
        {
            if (y == other.y)
                return z < other.z;

            return y < other.y;
        }

        return x < other.x;
    }

    /**
     * Component addition.
     */
    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }

    /**
     * Component addition in place.
     */
    Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;

        return *this;
    }

    /**
     * Component negation.
     */
    Vec3 operator-() const { return {-x, -y, -z}; }

    /**
     * Component subtraction.
     */
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }

    /**
     * Component subtraction in place.
     */
    Vec3& operator-=(const Vec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;

        return *this;
    }

    /**
     * Scalar multiplication.
     */
    Vec3 operator*(float f) const { return {x * f, y * f, z * f}; }

    /**
     * Component multiplication.
     */
    Vec3 operator*(const Vec3& other) const { return {x * other.x, y * other.y, z * other.z}; }

    /**
     * Scalar multiplication in place
     */
    Vec3& operator*=(float f)
    {
        x *= f;
        y *= f;
        z *= f;

        return *this;
    }

    /**
     * Component multiplication in place.
     */
    Vec3& operator*=(const Vec3& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;

        return *this;
    }

    /**
     * Scalar division.
     */
    Vec3 operator/(float f) const
    {
        f = 1.0f / f;

        return {x * f, y * f, z * f};
    }

    /**
     * Component division.
     */
    Vec3 operator/(const Vec3& other) const { return {x / other.x, y / other.y, z / other.z}; }

    /**
     * Scalar division in place.
     */
    Vec3& operator/=(float f)
    {
        f = 1.0f / f;

        x *= f;
        y *= f;
        z *= f;

        return *this;
    }

    /**
     * Component division in place.
     */
    Vec3& operator/=(const Vec3& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;

        return *this;
    }

    /**
     * Returns the pitch of this vector above the XZ plane.
     */
    float getPitch() const { return -atan2f(y, sqrtf(x * x + z * z)); }

    /**
     * Returns the yaw of this vector around the Y axis.
     */
    float getYaw() const { return atan2f(x, -z); }

    /**
     * Calculates the dot product between two vectors.
     */
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    /**
     * Calculates the cross product between two vectors.
     */
    Vec3 cross(const Vec3& v) const { return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x}; }

    /**
     * Calculates the length (magnitude) of this vector.
     */
    float length() const { return sqrtf(x * x + y * y + z * z); }

    /**
     * Calculates the squared length (magnitude) of this vector.
     */
    float lengthSquared() const { return x * x + y * y + z * z; }

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
    Vec3 normalized() const
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
    float distance(const Vec3& v) const { return sqrtf((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z)); }

    /**
     * Calculates the distance between two points.
     */
    float distanceSquared(const Vec3& v) const { return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z); }

    /**
     * Returns a vector with the direction of this vector but with the given length
     */
    Vec3 ofLength(float f) const
    {
        auto l = length();

        if (l < Math::Epsilon)
            return Vec3::Zero;

        return operator*(f / l);
    }

    /**
     * Scales this vector so that its length falls between the given minimum and maximum values.
     */
    void clampLength(float minLength, float maxLength)
    {
        if (maxLength < Math::Epsilon)
            *this = Zero;

        auto initialLength = length();

        if (initialLength < Math::Epsilon)
            return;

        operator*=(Math::clamp(initialLength, minLength, maxLength) / initialLength);
    }

    /**
     * Returns the vector found by raising each component of this vector to the given power.
     */
    Vec3 pow(float f) const { return {powf(x, f), powf(y, f), powf(z, f)}; }

    /**
     * Returns the vector found by exponentiating each component of this vector.
     */
    Vec3 exp() const { return {expf(x), expf(y), expf(z)}; }

    /**
     * Rounds each component to the closes whole number using Math::round().
     */
    void round()
    {
        x = Math::round(x);
        y = Math::round(y);
        z = Math::round(z);
    }

    /**
     * Sets the components of this vector to the given values.
     */
    void setXYZ(float x_, float y_, float z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }

    /**
     * Returns the value of the smallest component of this vector.
     */
    float getSmallestComponent() const { return x < y ? (x < z ? x : z) : (y < z ? y : z); }

    /**
     * Returns the value of the largest component of this vector.
     */
    float getLargestComponent() const { return x > y ? (x > z ? x : z) : (y > z ? y : z); }

    /**
     * Returns this Vec3 value as a float[3] array.
     */
    const float* asArray() const { return &x; }

    /**
     * Returns this vector as a string in the form "x y z".
     */
    operator UnicodeString() const { return UnicodeString() << x << " " << y << " " << z; }

    /**
     * Converts this vector to a two component vector by leaving out the z component.
     */
    Vec2 toVec2() const { return {x, y}; }

    /**
     * Returns a Vec2 made up of the x and z components of this vector.
     */
    Vec2 getXZ() const { return {x, z}; }

    /**
     * Assumes this vector is normalized and converts its components into unsigned bytes that are written to \a output.
     */
    void toNormalizedRGB8(byte_t* output) const
    {
        output[0] = byte_t(((x + 1.0f) * 0.5f * 255.0f));
        output[1] = byte_t(((y + 1.0f) * 0.5f * 255.0f));
        output[2] = byte_t(((z + 1.0f) * 0.5f * 255.0f));
    }

    /**
     * Returns whether the components of this vector are finite.
     */
    bool isFinite() const { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z); }

    /**
     * Returns two normalized vectors that together with this vector define a basis with this vector along its positive Z axis.
     * This vector must already be normalized.
     */
    void constructBasis(Vec3& v0, Vec3& v1) const
    {
        assert(isNormalized());

        v1 = fabsf(dot(Vec3::UnitY)) > 0.99f ? Vec3::UnitX : Vec3::UnitY;
        v0 = v1.cross(*this).normalized();
        v1 = cross(v0).normalized();
    }

    /**
     * Returns a vector that represents a 2D offset in the XY plane of the local basis of this vector that is calculated by the
     * Vec3::constructBasis() method.
     */
    Vec3 getLocalOffset(float xOffset, float yOffset) const
    {
        auto v0 = Vec3();
        auto v1 = Vec3();

        constructBasis(v0, v1);

        return v0 * xOffset + v1 * yOffset;
    }

    /**
     * Returns the given vector reflected in this vector, assumes that this vector is normalized.
     */
    Vec3 reflect(const Vec3& v) const { return v - (*this) * dot(v) * 2.0f; }

    /**
     * Saves this vector to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(x, y, z); }

    /**
     * Loads this vector from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(x, y, z); }

    /**
     * Vec3 instance with all components set to zero.
     */
    static const Vec3 Zero;

    /**
     * Vec3 instance with all components set to one.
     */
    static const Vec3 One;

    /**
     * Vec3 instance with all components set to 0.5.
     */
    static const Vec3 Half;

    /**
     * Vec3 instance with the x component set to one and the other components set to zero.
     */
    static const Vec3 UnitX;

    /**
     * Vec3 instance with the y component set to one and the other components set to zero.
     */
    static const Vec3 UnitY;

    /**
     * Vec3 instance with the z component set to one and the other components set to zero.
     */
    static const Vec3 UnitZ;

    /**
     * Vec3 instance with the x and y components set to one and the z component set to negative one.
     */
    static const Vec3 NegateZ;

    /**
     * Returns a vector where the x, y and z values are randomly generated in the range -1 to 1. Note that the returned vector
     * is not normalized.
     */
    static Vec3 random() { return {Math::random(-1.0f, 1.0f), Math::random(-1.0f, 1.0f), Math::random(-1.0f, 1.0f)}; }
};

}
