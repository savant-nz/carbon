/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * 3x3 matrix. Stored in column major format.
 */
class CARBON_API Matrix3
{
public:

    /**
     * Default constructor, the matrix is initially zeroed.
     */
    Matrix3() {}

    /**
     * Component constructor.
     */
    Matrix3(float m0, float m1, float m2, float m3, float m4, float m5, float m6, float m7, float m8)
        : m_{{m0, m1, m2, m3, m4, m5, m6, m7, m8}}
    {
    }

    /**
     * Returns a matrix element.
     */
    float& operator[](unsigned int index)
    {
        assert(index < 9);
        return m_[index];
    }

    /**
     * Returns a matrix element.
     */
    float operator[](unsigned int index) const
    {
        assert(index < 9);
        return m_[index];
    }

    /**
     * Matrix comparison.
     */
    bool operator==(const Matrix3& other) const
    {
        for (auto i = 0U; i < 9; i++)
        {
            if (m_[i] != other[i])
                return false;
        }

        return true;
    }

    /**
     * Matrix comparison.
     */
    bool operator!=(const Matrix3& other) const { return !operator==(other); }

    /**
     * Multiplies two matrices.
     */
    Matrix3 operator*(const Matrix3& other) const;

    /**
     * Multiplies two matrices in place.
     */
    Matrix3& operator*=(const Matrix3& other)
    {
        *this = operator*(other);
        return *this;
    }

    /**
     * Multiplies a matrix by a vector.
     */
    Vec3 operator*(const Vec3& v) const
    {
        auto x = m_[0] * v.x + m_[3] * v.y + m_[6] * v.z;
        auto y = m_[1] * v.x + m_[4] * v.y + m_[7] * v.z;
        auto z = m_[2] * v.x + m_[5] * v.y + m_[8] * v.z;

        return {x, y, z};
    }

    /**
     * Calculates the inverse of this matrix. Returns success flag.
     */
    bool getInverse(Matrix3& inv) const;

    /**
     * Transposes this matrix
     */
    void transpose();

    /**
     * In-place x scale.
     */
    void scale(float x)
    {
        m_[0] *= x;
        m_[1] *= x;
        m_[2] *= x;
    }

    /**
     * In-place x and y scale.
     */
    void scale(float x, float y)
    {
        m_[0] *= x;
        m_[1] *= x;
        m_[2] *= x;

        m_[3] *= y;
        m_[4] *= y;
        m_[5] *= y;
    }

    /**
     * In-place x and y scale.
     */
    void scale(const Vec2& xy) { scale(xy.x, xy.y); }

    /**
     * In-place x, y and z scale.
     */
    void scale(float x, float y, float z)
    {
        m_[0] *= x;
        m_[1] *= x;
        m_[2] *= x;

        m_[3] *= y;
        m_[4] *= y;
        m_[5] *= y;

        m_[6] *= z;
        m_[7] *= z;
        m_[8] *= z;
    }

    /**
     * In-place x, y and z scale.
     */
    void scale(const Vec3& xyz) { scale(xyz.x, xyz.y, xyz.z); }

    /**
     * Returns this matrix as a float[9] array.
     */
    const float* asArray() const { return m_.data(); }

    /**
     * Saves this matrix to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this matrix from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Returns this matrix as a string of 16 numbers enclosed by square brackets.
     */
    operator UnicodeString() const;

    /**
     * Zero matrix.
     */
    static const Matrix3 Zero;

    /**
     * Identity matrix.
     */
    static const Matrix3 Identity;

    /**
     * Calculates a scale matrix.
     */
    static Matrix3 getScale(const Vec3& v);

    /**
     * Calculates an x rotation matrix.
     */
    static Matrix3 getRotationX(float radians);

    /**
     * Calculates a y rotation matrix.
     */
    static Matrix3 getRotationY(float radians);

    /**
     * Calculates a z rotation matrix.
     */
    static Matrix3 getRotationZ(float radians);

private:

    std::array<float, 9> m_ = {};
};

}
