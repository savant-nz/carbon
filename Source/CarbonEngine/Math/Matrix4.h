/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Math/Rect.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * 4x4 matrix stored in column major format.
 */
class CARBON_API Matrix4
{
public:

    /**
     * Default constructor, the matrix is initially zeroed.
     */
    Matrix4() {}

    /**
     * Initializes the inner 3x3 portion of this matrix from the given 3x3 matrix.
     */
    Matrix4(const Matrix3& other)
        : m_{{other[0], other[1], other[2], 0.0f, other[3], other[4], other[5], 0.0f, other[6], other[7], other[8],
              0.0f, 0.0f, 0.0f, 0.0f, 1.0f}}
    {
    }

    /**
     * Component constructor.
     */
    Matrix4(float m0, float m1, float m2, float m3, float m4, float m5, float m6, float m7, float m8, float m9,
            float m10, float m11, float m12, float m13, float m14, float m15)
        : m_{{m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15}}
    {
    }

    /**
     * Returns a matrix element.
     */
    float& operator[](unsigned int index)
    {
        assert(index < 16);
        return m_[index];
    }

    /**
     * Returns a matrix element.
     */
    float operator[](unsigned int index) const
    {
        assert(index < 16);
        return m_[index];
    }

    /**
     * Matrix comparison.
     */
    bool operator==(const Matrix4& other) const
    {
        for (auto i = 0U; i < 16; i++)
        {
            if (m_[i] != other[i])
                return false;
        }

        return true;
    }

    /**
     * Matrix comparison.
     */
    bool operator!=(const Matrix4& other) const { return !operator==(other); }

    /**
     * Matrix multiplication.
     */
    Matrix4 operator*(const Matrix4& other) const;

    /**
     * Matrix and vector multiplication.
     */
    Vec3 operator*(const Vec3& v) const
    {
        return {m_[0] * v.x + m_[4] * v.y + m_[8] * v.z + m_[12], m_[1] * v.x + m_[5] * v.y + m_[9] * v.z + m_[13],
                m_[2] * v.x + m_[6] * v.y + m_[10] * v.z + m_[14]};
    }

    /**
     * Matrix and plane multiplication.
     */
    Plane operator*(const Plane& plane) const;

    /**
     * Matrix multiplication in place.
     */
    Matrix4& operator*=(const Matrix4& other)
    {
        *this = operator*(other);
        return *this;
    }

    /**
     * Fills the given Matrix3 with the inner 3x3 portion of this 4x4 matrix.
     */
    Matrix3 getMatrix3() const { return {m_[0], m_[1], m_[2], m_[4], m_[5], m_[6], m_[8], m_[9], m_[10]}; }

    /**
     * Calculates the inverse of this matrix. Returns true on success, and the inverse matrix is returned in \a result.
     */
    bool getInverse(Matrix4& result) const;

    /**
     * Transposes this matrix
     */
    void transpose();

    /**
     * Modifies this projection matrix so that the near plane coincides with a given arbitrary plane. The far plane is
     * adjusted so that the resulting view frustum has the best shape possible. This method assumes that this matrix is
     * a perspective projection and that \a clipPlane is in view space.
     */
    void modifyProjectionMatrix(const Plane& clipPlane);

    /**
     * Assuming that this is a projection matrix, this method returns the rectangle around the given view space sphere
     * that can be used as a scissor rectangle in rendering. The returned rect is in normalized device coordinates (from
     * -1 to 1).
     */
    Rect getProjectedSphereBounds(const Vec3& viewSpacePosition, float radius, float nearPlaneDistance) const;

    /**
     * Returns this matrix as a float[16] array.
     */
    const float* asArray() const { return m_.data(); }

    /**
     * In-place x translation.
     */
    void translate(float x)
    {
        m_[12] += m_[0] * x;
        m_[13] += m_[1] * x;
        m_[14] += m_[2] * x;
        m_[15] += m_[3] * x;
    }

    /**
     * In-place x and y translation.
     */
    void translate(float x, float y)
    {
        m_[12] += m_[0] * x + m_[4] * y;
        m_[13] += m_[1] * x + m_[5] * y;
        m_[14] += m_[2] * x + m_[6] * y;
        m_[15] += m_[3] * x + m_[7] * y;
    }

    /**
     * In-place x and y translation.
     */
    void translate(const Vec2& v) { translate(v.x, v.y); }

    /**
     * In-place x, y and z translation.
     */
    void translate(float x, float y, float z)
    {
        m_[12] += m_[0] * x + m_[4] * y + m_[8] * z;
        m_[13] += m_[1] * x + m_[5] * y + m_[9] * z;
        m_[14] += m_[2] * x + m_[6] * y + m_[10] * z;
        m_[15] += m_[3] * x + m_[7] * y + m_[11] * z;
    }

    /**
     * In-place x, y and z translation.
     */
    void translate(const Vec3& v) { translate(v.x, v.y, v.z); }

    /**
     * In-place x scale.
     */
    void scale(float x)
    {
        m_[0] *= x;
        m_[1] *= x;
        m_[2] *= x;
        m_[3] *= x;
    }

    /**
     * In-place x and y scale.
     */
    void scale(float x, float y)
    {
        m_[0] *= x;
        m_[1] *= x;
        m_[2] *= x;
        m_[3] *= x;

        m_[4] *= y;
        m_[5] *= y;
        m_[6] *= y;
        m_[7] *= y;
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
        m_[3] *= x;

        m_[4] *= y;
        m_[5] *= y;
        m_[6] *= y;
        m_[7] *= y;

        m_[8] *= z;
        m_[9] *= z;
        m_[10] *= z;
        m_[11] *= z;
    }

    /**
     * In-place x, y and z scale.
     */
    void scale(const Vec3& xyz) { scale(xyz.x, xyz.y, xyz.z); }

    /**
     * Saves this matrix to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this matrix from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Returns this matrix as a string of 9 numbers enclosed by square brackets.
     */
    operator UnicodeString() const;

    /**
     * Zero matrix.
     */
    static const Matrix4 Zero;

    /**
     * Identity matrix.
     */
    static const Matrix4 Identity;

    /**
     * This matrix has an 0.5 xyz scale and an xyz translation of 0.5. This is used to map from -1 - 1 to 0 - 1 when
     * doing projective texturing.
     */
    static const Matrix4 Half;

    /**
     * Calculates a scale matrix.
     */
    static Matrix4 getScale(const Vec3& v);

    /**
     * Calculates a translation matrix.
     */
    static Matrix4 getTranslation(const Vec3& p);

    /**
     * Calculates a scale and translation matrix.
     */
    static Matrix4 getScaleAndTranslation(const Vec3& scale, const Vec3& translation);

    /**
     * Calculates an x rotation matrix.
     */
    static Matrix4 getRotationX(float radians);

    /**
     * Calculates a y rotation matrix.
     */
    static Matrix4 getRotationY(float radians);

    /**
     * Calculates a z rotation matrix.
     */
    static Matrix4 getRotationZ(float radians);

    /**
     * Calculates a reflection matrix for a plane.
     */
    static Matrix4 getReflection(const Plane& plane);

    /**
     * Calculates an orthographic projection matrix for a given rectangle.
     */
    static Matrix4 getOrthographicProjection(const Rect& rect, float nearPlaneDistance = -100.0f,
                                             float farPlaneDistance = 100.0f);

    /**
     * Calculates a perspective projection.
     */
    static Matrix4 getPerspectiveProjection(float fieldOfView, float aspectRatio, float nearPlaneDistance,
                                            float farPlaneDistance);

    /**
     * Transforms a 2D point in screen space into world space using the given view transform, projection matrix and
     * viewport. The \a p.z value should be the desired z depth of the returned point and must be in the range 0 - 1.
     */
    static Vec3 unproject(const Vec3& p, const SimpleTransform& viewTransform, const Matrix4& projection,
                          const Rect& viewport);

private:

    std::array<float, 16> m_ = {};
};

}
