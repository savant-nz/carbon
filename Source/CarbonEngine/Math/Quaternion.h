/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Quaternion rotation.
 */
class CARBON_API Quaternion
{
public:

    /**
     * The x component of this quaternion.
     */
    float x = 0.0f;

    /**
     * The y component of this quaternion.
     */
    float y = 0.0f;

    /**
     * The z component of this quaternion.
     */
    float z = 0.0f;

    /**
     * The w component of this quaternion.
     */
    float w = 1.0f;

    /**
     * Default constructor. Sets this quaternion to the identity quaternion.
     */
    Quaternion() {}

    /**
     * Constructs the quaternion from x, y, z and w values.
     */
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    /**
     * Quaternion component equality.
     */
    bool operator==(const Quaternion& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }

    /**
     * Quaternion component inequality.
     */
    bool operator!=(const Quaternion& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }

    /**
     * Quaternion multiplication. This is used to combine rotations.
     */
    Quaternion operator*(const Quaternion& other) const
    {
        if (*this == Identity)
            return other;

        return {w * other.x + x * other.w + y * other.z - z * other.y, w * other.y - x * other.z + y * other.w + z * other.x,
                w * other.z + x * other.y - y * other.x + z * other.w, w * other.w - x * other.x - y * other.y - z * other.z};
    }

    /**
     * Vector rotation. Rotates a vector using this quaternion.
     */
    Vec3 operator*(const Vec3& v) const
    {
        if (*this == Identity)
            return v;

        auto xx = x * x;
        auto yy = y * y;
        auto zz = z * z;
        auto xy = x * y;
        auto xz = x * z;
        auto xw = x * w;
        auto yz = y * z;
        auto yw = y * w;
        auto zw = z * w;

        return {(1.0f - 2.0f * (yy + zz)) * v.x + (2.0f * (xy + zw)) * v.y + (2.0f * (xz - yw)) * v.z,
                (2.0f * (xy - zw)) * v.x + (1.0f - 2.0f * (xx + zz)) * v.y + (2.0f * (yz + xw)) * v.z,
                (2.0f * (xz + yw)) * v.x + (2.0f * (yz - xw)) * v.y + (1.0f - 2.0f * (xx + yy)) * v.z};
    }

    /**
     * Quaternion multiplication. This is used to combine rotations.
     */
    Quaternion& operator*=(const Quaternion& other)
    {
        *this = operator*(other);
        return *this;
    }

    /**
     * Calculates the inverse of this quaternion, i.e. a rotation that reverses the rotation performed by this quaternion.
     */
    Quaternion getInverse() const { return {-x, -y, -z, w}; }

    /**
     * Normalizes this quaternion.
     */
    void normalize()
    {
        auto l = 1.0f / sqrtf(x * x + y * y + z * z + w * w);

        x *= l;
        y *= l;
        z *= l;
        w *= l;
    }

    /**
     * Converts this quaternion into a 3x3 rotation matrix. Assumes a unit quaternion,
     */
    Matrix3 getMatrix3() const;

    /**
     * Converts this quaternion into a 4x4 rotation matrix. Assumes a unit quaternion.
     */
    Matrix4 getMatrix4(const Vec3& position = Vec3::Zero) const;

    /**
     * Returns the result of rotating the unit (1, 0, 0) vector by this quaternion.
     */
    Vec3 getXVector() const { return {1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - z * w), 2.0f * (x * z + y * w)}; }

    /**
     * Returns the result of rotating the unit (0, 1, 0) vector by this quaternion.
     */
    Vec3 getYVector() const { return {2.0f * (x * y + z * w), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - x * w)}; }

    /**
     * Returns the result of rotating the unit (0, 0, 1) vector by this quaternion.
     */
    Vec3 getZVector() const { return {2.0f * (x * z - y * w), 2.0f * (y * z + x * w), 1.0f - 2.0f * (x * x + y * y)}; }

    /**
     * Converts this quaternion to an axis and an angle.
     */
    void convertToAxisAngle(Vec3& axis, float& angle) const;

    /**
     * Spherically interpolates between two rotations. A \a t value of zero will return this quaternion, and a value of one will
     * return \a q.
     */
    Quaternion slerp(const Quaternion& q, float t) const;

    /**
     * Returns whether all the components in this quaternion are finite.
     */
    bool isFinite() const { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(w); }

    /**
     * Saves this quaternion to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this quaternion from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Returns this quaternion as a string of the form "x y z w".
     */
    operator UnicodeString() const { return UnicodeString() << x << " " << y << " " << z << " " << w; }

    /**
     * The identity quaternion. The x, y and z components are zero and the w component is one.
     */
    static const Quaternion Identity;

    /**
     * Creates a quaternion with a rotation around the x axis.
     */
    static Quaternion createRotationX(float radians);

    /**
     * Creates a quaternion with a rotation around the y axis.
     */
    static Quaternion createRotationY(float radians);

    /**
     * Creates a quaternion with a rotation around the z axis.
     */
    static Quaternion createRotationZ(float radians);

    /**
     * Creates a quaternion with a rotation around the x and y axis. The order of multiplication is X * Y.
     */
    static Quaternion createRotationXY(float xRadians, float yRadians);

    /**
     * Creates a quaternion rotation from an axis and a rotation around that axis. The axis vector does not need to be
     * normalized.
     */
    static Quaternion createFromAxisAngle(const Vec3& axis, float radians);

    /**
     * Creates a quaternion rotation from a rotation matrix.
     */
    static Quaternion createFromRotationMatrix(const Matrix3& m);

    /**
     * Creates a quaternion rotation that rotates one vector onto another along the shortest arc, the passed vectors do not need
     * to be normalized.
     */
    static Quaternion createFromVectorToVector(const Vec3& from, const Vec3& to);

    /**
     * Creates a random normalized quaternion.
     */
    static Quaternion random();
};

}
