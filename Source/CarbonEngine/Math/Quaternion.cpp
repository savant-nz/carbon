/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/Quaternion.h"

namespace Carbon
{

const Quaternion Quaternion::Identity;

Matrix3 Quaternion::getMatrix3() const
{
    auto m = Matrix3();

    auto xx = x * x;
    auto yy = y * y;
    auto zz = z * z;
    auto xy = x * y;
    auto xz = x * z;
    auto xw = x * w;
    auto yz = y * z;
    auto yw = y * w;
    auto zw = z * w;

    m[0] = 1.0f - 2.0f * (yy + zz);
    m[1] = 2.0f * (xy - zw);
    m[2] = 2.0f * (xz + yw);
    m[3] = 2.0f * (xy + zw);
    m[4] = 1.0f - 2.0f * (xx + zz);
    m[5] = 2.0f * (yz - xw);
    m[6] = 2.0f * (xz - yw);
    m[7] = 2.0f * (yz + xw);
    m[8] = 1.0f - 2.0f * (xx + yy);

    return m;
}

Matrix4 Quaternion::getMatrix4(const Vec3& position) const
{
    auto m = Matrix4();

    auto xx = x * x;
    auto yy = y * y;
    auto zz = z * z;
    auto xy = x * y;
    auto xz = x * z;
    auto xw = x * w;
    auto yz = y * z;
    auto yw = y * w;
    auto zw = z * w;

    m[0] = 1.0f - 2.0f * (yy + zz);
    m[1] = 2.0f * (xy - zw);
    m[2] = 2.0f * (xz + yw);
    m[3] = 0.0f;
    m[4] = 2.0f * (xy + zw);
    m[5] = 1.0f - 2.0f * (xx + zz);
    m[6] = 2.0f * (yz - xw);
    m[7] = 0.0f;
    m[8] = 2.0f * (xz - yw);
    m[9] = 2.0f * (yz + xw);
    m[10] = 1.0f - 2.0f * (xx + yy);
    m[11] = 0.0f;
    m[12] = position.x;
    m[13] = position.y;
    m[14] = position.z;
    m[15] = 1.0f;

    return m;
}

void Quaternion::convertToAxisAngle(Vec3& axis, float& angle) const
{
    angle = 2.0f * acosf(w);

    auto s = sqrtf(1.0f - w * w);
    if (s < Math::Epsilon)
        s = 1.0f;

    s = 1.0f / s;

    axis.setXYZ(x * s, y * s, z * s);
}

Quaternion Quaternion::slerp(const Quaternion& q, float t) const
{
    auto q0 = Quaternion();

    auto cosTheta = x * q.x + y * q.y + z * q.z + w * q.w;

    if (cosTheta < 0.0f)
    {
        q0 = getInverse();
        cosTheta = -cosTheta;
    }
    else
        q0 = *this;

    auto a = 0.0f;
    auto b = 0.0f;

    if (1.0f - cosTheta > Math::Epsilon)
    {
        // Use spherical interpolation
        auto theta = acosf(cosTheta);
        auto oosinTheta = 1.0f / sinf(theta);
        a = sinf(theta * t) * oosinTheta;
        b = sinf(theta * (1.0f - t)) * oosinTheta;
    }
    else
    {
        // Use linear interpolation
        a = t;
        b = 1.0f - t;
    }

    // Do the interpolation
    return {q0.x * b + q.x * a, q0.y * b + q.y * a, q0.z * b + q.z * a, q0.w * b + q.w * a};
}

void Quaternion::save(FileWriter& file) const
{
    file.write(x, y, z, w);
}

void Quaternion::load(FileReader& file)
{
    file.read(x, y, z, w);
}

Quaternion Quaternion::createRotationX(float radians)
{
    radians *= 0.5f;
    return {sinf(radians), 0.0f, 0.0f, cosf(radians)};
}

Quaternion Quaternion::createRotationY(float radians)
{
    radians *= 0.5f;
    return {0.0f, sinf(radians), 0.0f, cosf(radians)};
}

Quaternion Quaternion::createRotationZ(float radians)
{
    radians *= 0.5f;
    return {0.0f, 0.0f, sinf(radians), cosf(radians)};
}

Quaternion Quaternion::createRotationXY(float xRadians, float yRadians)
{
    xRadians *= 0.5f;
    yRadians *= 0.5f;

    auto sinx = sinf(xRadians);
    auto cosx = cosf(xRadians);
    auto siny = sinf(yRadians);
    auto cosy = cosf(yRadians);

    return {sinx * cosy, cosx * siny, sinx * siny, cosx * cosy};
}

Quaternion Quaternion::createFromAxisAngle(const Vec3& axis, float radians)
{
    auto v = axis.normalized();

    radians *= 0.5f;

    auto sinAngle = sinf(radians);

    return {v.x * sinAngle, v.y * sinAngle, v.z * sinAngle, cosf(radians)};
}

Quaternion Quaternion::createFromRotationMatrix(const Matrix3& m)
{
    auto q = Quaternion();

    auto t = m[0] + m[4] + m[8] + 1.0f;

    if (t > Math::Epsilon)
    {
        auto s = sqrtf(t) * 2.0f;

        q.x = (m[5] - m[7]) / s;
        q.y = (m[6] - m[2]) / s;
        q.z = (m[1] - m[3]) / s;
        q.w = 0.25f * s;
    }
    else
    {
        if (m[0] > m[4] && m[0] > m[8])
        {
            auto s = 2.0f * sqrtf(1.0f + m[0] - m[4] - m[8]);

            q.x = 0.25f * s;
            q.y = (m[3] + m[1]) / s;
            q.z = (m[6] + m[2]) / s;
            q.w = (m[7] - m[5]) / s;
        }
        else if (m[4] > m[8])
        {
            auto s = 2.0f * sqrtf(1.0f + m[4] - m[0] - m[8]);

            q.x = (m[3] + m[1]) / s;
            q.y = 0.25f * s;
            q.z = (m[7] + m[5]) / s;
            q.w = (m[6] - m[2]) / s;
        }
        else
        {
            auto s = 2.0f * sqrtf(1.0f + m[8] - m[0] - m[4]);

            q.x = (m[6] + m[2]) / s;
            q.y = (m[7] + m[5]) / s;
            q.z = 0.25f * s;
            q.w = (m[3] - m[1]) / s;
        }
    }

    q.normalize();

    return q;
}

Quaternion Quaternion::createFromVectorToVector(const Vec3& from, const Vec3& to)
{
    auto axis = from.cross(to);

    auto q = Quaternion(axis.x, axis.y, axis.z, sqrtf(from.lengthSquared() * to.lengthSquared()) + from.dot(to));

    q.normalize();

    return q;
}

Quaternion Quaternion::random()
{
    auto axis = Vec3::random().normalized();

    auto angle = Math::random(0.0f, 2 * Math::Pi);

    return Quaternion::createFromAxisAngle(axis, angle);
}

}
