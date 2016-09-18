/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/Quaternion.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

const Matrix4 Matrix4::Identity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f);
const Matrix4 Matrix4::Half(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f,
                            1.0f);

Matrix4 Matrix4::operator*(const Matrix4& other) const
{
    return {m_[0] * other[0] + m_[4] * other[1] + m_[8] * other[2] + m_[12] * other[3],
            m_[1] * other[0] + m_[5] * other[1] + m_[9] * other[2] + m_[13] * other[3],
            m_[2] * other[0] + m_[6] * other[1] + m_[10] * other[2] + m_[14] * other[3],
            m_[3] * other[0] + m_[7] * other[1] + m_[11] * other[2] + m_[15] * other[3],
            m_[0] * other[4] + m_[4] * other[5] + m_[8] * other[6] + m_[12] * other[7],
            m_[1] * other[4] + m_[5] * other[5] + m_[9] * other[6] + m_[13] * other[7],
            m_[2] * other[4] + m_[6] * other[5] + m_[10] * other[6] + m_[14] * other[7],
            m_[3] * other[4] + m_[7] * other[5] + m_[11] * other[6] + m_[15] * other[7],
            m_[0] * other[8] + m_[4] * other[9] + m_[8] * other[10] + m_[12] * other[11],
            m_[1] * other[8] + m_[5] * other[9] + m_[9] * other[10] + m_[13] * other[11],
            m_[2] * other[8] + m_[6] * other[9] + m_[10] * other[10] + m_[14] * other[11],
            m_[3] * other[8] + m_[7] * other[9] + m_[11] * other[10] + m_[15] * other[11],
            m_[0] * other[12] + m_[4] * other[13] + m_[8] * other[14] + m_[12] * other[15],
            m_[1] * other[12] + m_[5] * other[13] + m_[9] * other[14] + m_[13] * other[15],
            m_[2] * other[12] + m_[6] * other[13] + m_[10] * other[14] + m_[14] * other[15],
            m_[3] * other[12] + m_[7] * other[13] + m_[11] * other[14] + m_[15] * other[15]};
}

Plane Matrix4::operator*(const Plane& plane) const
{
    return {operator*(-plane.getNormal() * plane.getDistance()),
            (operator*(plane.getNormal()) - Vec3(m_[12], m_[13], m_[14])).normalized()};
}

bool Matrix4::getInverse(Matrix4& result) const
{
    auto temp = std::array<std::array<float, 8>, 4>{{{{m_[0], m_[4], m_[8], m_[12], 1.0f, 0.0f, 0.0f, 0.0f}},
                                                     {{m_[1], m_[5], m_[9], m_[13], 0.0f, 1.0f, 0.0f, 0.0f}},
                                                     {{m_[2], m_[6], m_[10], m_[14], 0.0f, 0.0f, 1.0f, 0.0f}},
                                                     {{m_[3], m_[7], m_[11], m_[15], 0.0f, 0.0f, 0.0f, 1.0f}}}};

    auto r0 = temp[0].data();
    auto r1 = temp[1].data();
    auto r2 = temp[2].data();
    auto r3 = temp[3].data();

    // Choose pivot otherwise fail
    if (fabsf(r3[0]) > fabsf(r2[0]))
        std::swap(r3, r2);
    if (fabsf(r2[0]) > fabsf(r1[0]))
        std::swap(r2, r1);
    if (fabsf(r1[0]) > fabsf(r0[0]))
        std::swap(r1, r0);
    if (r0[0] == 0.0f)
        return false;

    // Eliminate first variable
    auto m1 = r1[0] / r0[0];
    auto m2 = r2[0] / r0[0];
    auto m3 = r3[0] / r0[0];
    auto s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0f)
    {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0f)
    {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0f)
    {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0f)
    {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    // Choose pivot otherwise fail
    if (fabsf(r3[1]) > fabsf(r2[1]))
        std::swap(r3, r2);
    if (fabsf(r2[1]) > fabsf(r1[1]))
        std::swap(r2, r1);
    if (r1[1] == 0.0f)
        return false;

    // Eliminate second variable
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (s != 0.0f)
    {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (s != 0.0f)
    {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (s != 0.0f)
    {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (s != 0.0f)
    {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    // Choose pivot otherwise fail
    if (fabsf(r3[2]) > fabsf(r2[2]))
        std::swap(r3, r2);
    if (r2[2] == 0.0f)
        return false;

    // Eliminate third variable
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3];
    r3[4] -= m3 * r2[4];
    r3[5] -= m3 * r2[5];
    r3[6] -= m3 * r2[6];
    r3[7] -= m3 * r2[7];

    // Last check
    if (r3[3] == 0.0f)
        return false;

    // Now back substitute row 3
    s = 1.0f / r3[3];
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;

    // Now back substitute row 2
    m2 = r2[3];
    s = 1.0f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2);
    r2[5] = s * (r2[5] - r3[5] * m2);
    r2[6] = s * (r2[6] - r3[6] * m2);
    r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1;
    r1[5] -= r3[5] * m1;
    r1[6] -= r3[6] * m1;
    r1[7] -= r3[7] * m1;
    float m0 = r0[3];
    r0[4] -= r3[4] * m0;
    r0[5] -= r3[5] * m0;
    r0[6] -= r3[6] * m0;
    r0[7] -= r3[7] * m0;

    // Now back substitute row 1
    m1 = r1[2];
    s = 1.0f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1);
    r1[5] = s * (r1[5] - r2[5] * m1);
    r1[6] = s * (r1[6] - r2[6] * m1);
    r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0;
    r0[5] -= r2[5] * m0;
    r0[6] -= r2[6] * m0;
    r0[7] -= r2[7] * m0;

    // Now back substitute row 0
    m0 = r0[1];
    s = 1.0f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0);
    r0[5] = s * (r0[5] - r1[5] * m0);
    r0[6] = s * (r0[6] - r1[6] * m0);
    r0[7] = s * (r0[7] - r1[7] * m0);

    result[0] = r0[4];
    result[1] = r1[4];
    result[2] = r2[4];
    result[3] = r3[4];
    result[4] = r0[5];
    result[5] = r1[5];
    result[6] = r2[5];
    result[7] = r3[5];
    result[8] = r0[6];
    result[9] = r1[6];
    result[10] = r2[6];
    result[11] = r3[6];
    result[12] = r0[7];
    result[13] = r1[7];
    result[14] = r2[7];
    result[15] = r3[7];

    return true;
}

void Matrix4::transpose()
{
    std::swap(m_[1], m_[4]);
    std::swap(m_[2], m_[8]);
    std::swap(m_[3], m_[12]);
    std::swap(m_[6], m_[9]);
    std::swap(m_[7], m_[13]);
    std::swap(m_[11], m_[14]);
}

void Matrix4::modifyProjectionMatrix(const Plane& clipPlane)
{
    // Calculate the clip-space corner point opposite the clipping plane as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1)
    // and transform it into camera space by multiplying it by the inverse of the projection matrix

    auto q = Vec3((Math::getSign(clipPlane.getNormal().x) + m_[8]) / m_[0],
                  (Math::getSign(clipPlane.getNormal().y) + m_[9]) / m_[5], -1.0f);
    auto qw = (1.0f - m_[10]) / m_[14];

    // Calculate the scaled plane vector
    auto scale = 1.0f / (clipPlane.getNormal().dot(q) + clipPlane.getDistance() * qw);

    // Replace the third row of the projection matrix
    m_[2] = clipPlane.getNormal().x * scale;
    m_[6] = clipPlane.getNormal().y * scale;
    m_[10] = clipPlane.getNormal().z * scale + 1.0f;
    m_[14] = clipPlane.getDistance() * scale;
}

Rect Matrix4::getProjectedSphereBounds(const Vec3& viewSpacePosition, float radius, float nearPlaneDistance) const
{
    // Start with a rect that covers the entire viewport
    auto rect = Rect(-1.0f, -1.0f, 1.0f, 1.0f);

    auto radiusSquared = radius * radius;

    // This method doesn't handle the case when the sphere intersects the near plane
    if (viewSpacePosition.z >= 0.0f || viewSpacePosition.lengthSquared() <= radiusSquared)
        return rect;

    // This matrix is the projection matrix
    auto& projection = *this;

    auto lightXZ = powf(viewSpacePosition.x, 2.0f) + powf(viewSpacePosition.z, 2.0f);
    auto lightYZ = powf(viewSpacePosition.y, 2.0f) + powf(viewSpacePosition.z, 2.0f);

    // Find the tangent planes to the sphere, XZ first
    // Calculate quadratic discriminant: b*b - 4ac
    // x = Nx
    // a = Lx^2 + Lz^2
    // b = -2rLx
    // c = r^2 - Lz^2

    auto a = lightXZ;
    auto b = -2.0f * radius * viewSpacePosition.x;
    auto c = radiusSquared - powf(viewSpacePosition.z, 2.0f);
    auto discriminant = b * b - 4.0f * a * c;

    // Two possible solutions if D is positive
    if (discriminant > 0.0f)
    {
        discriminant = sqrtf(discriminant);

        // Solve the quadratic to get the components of the normal
        auto normalX0 = (-b + discriminant) / (2.0f * a);
        auto normalX1 = (-b - discriminant) / (2.0f * a);

        // Derive Z from this
        auto normalZ0 = (radius - normalX0 * viewSpacePosition.x) / viewSpacePosition.z;
        auto normalZ1 = (radius - normalX1 * viewSpacePosition.x) / viewSpacePosition.z;

        // Get the point of tangency, only consider points of tangency in front of the camera
        auto pointZ0 =
            (lightXZ - radiusSquared) / (viewSpacePosition.z - ((normalZ0 / normalX0) * viewSpacePosition.x));
        if (pointZ0 < 0.0f)
        {
            // Project point onto near plane
            auto nearX0 = (normalZ0 * nearPlaneDistance) / normalX0;

            // Map this to viewport coords using the projection matrix
            auto relX0 = projection * Vec3(nearX0, 0.0f, -nearPlaneDistance);

            // Find out whether this is a left side or right side
            auto pointX0 = -(pointZ0 * normalZ0) / normalX0;
            if (pointX0 > viewSpacePosition.x)
                rect.setRight(std::min(rect.getRight(), relX0.x));
            else
                rect.setLeft(std::max(rect.getLeft(), relX0.x));
        }

        auto pointZ1 =
            (lightXZ - radiusSquared) / (viewSpacePosition.z - ((normalZ1 / normalX1) * viewSpacePosition.x));
        if (pointZ1 < 0.0f)
        {
            // Project point onto near plane
            auto nearX1 = (normalZ1 * nearPlaneDistance) / normalX1;

            // Map this to viewport coords using the projection matrix
            auto relX1 = projection * Vec3(nearX1, 0.0f, -nearPlaneDistance);

            // Find out whether this is a left side or right side
            auto pointX1 = -(pointZ1 * normalZ1) / normalX1;
            if (pointX1 > viewSpacePosition.x)
                rect.setRight(std::min(rect.getRight(), relX1.x));
            else
                rect.setLeft(std::max(rect.getLeft(), relX1.x));
        }
    }

    // Now find the Now YZ tangent plane to the sphere
    // Calculate quadratic discriminant: b*b - 4ac
    // x = Ny
    // a = Ly^2 + Lz^2
    // b = -2rLy
    // c = r^2 - Lz^2

    a = lightYZ;
    b = -2.0f * radius * viewSpacePosition.y;
    c = radiusSquared - powf(viewSpacePosition.z, 2.0f);
    discriminant = b * b - 4.0f * a * c;

    // Two possible solutions if D is positive
    if (discriminant > 0.0f)
    {
        discriminant = sqrtf(discriminant);

        // Solve the quadratic to get the components of the normal
        auto normalY0 = (-b + discriminant) / (2.0f * a);
        auto normalY1 = (-b - discriminant) / (2.0f * a);

        // Derive Z from this
        auto normalZ0 = (radius - normalY0 * viewSpacePosition.y) / viewSpacePosition.z;
        auto normalZ1 = (radius - normalY1 * viewSpacePosition.y) / viewSpacePosition.z;

        // Get the point of tangency, only consider points of tangency in front of the camera
        auto pointZ0 =
            (lightYZ - radiusSquared) / (viewSpacePosition.z - ((normalZ0 / normalY0) * viewSpacePosition.y));
        if (pointZ0 < 0)
        {
            // Project point onto near plane
            auto nearY0 = (normalZ0 * nearPlaneDistance) / normalY0;

            // Map this to viewport coords using the projection matrix
            auto relY0 = projection * Vec3(0.0f, nearY0, -nearPlaneDistance);

            // Find out whether this is a top side or bottom side
            auto pointY0 = -(pointZ0 * normalZ0) / normalY0;
            if (pointY0 > viewSpacePosition.y)
                rect.setTop(std::min(rect.getTop(), relY0.y));
            else
                rect.setBottom(std::max(rect.getBottom(), relY0.y));
        }

        auto pointZ1 =
            (lightYZ - radiusSquared) / (viewSpacePosition.z - ((normalZ1 / normalY1) * viewSpacePosition.y));
        if (pointZ1 < 0)
        {
            // Project point onto near plane
            auto nearY1 = (normalZ1 * nearPlaneDistance) / normalY1;

            // Map this to viewport coords using the projection matrix
            auto relY1 = projection * Vec3(0.0f, nearY1, -nearPlaneDistance);

            // Find out whether this is a top side or bottom side
            auto pointY1 = -(pointZ1 * normalZ1) / normalY1;
            if (pointY1 > viewSpacePosition.y)
                rect.setTop(std::min(rect.getTop(), relY1.y));
            else
                rect.setBottom(std::max(rect.getBottom(), relY1.y));
        }
    }

    return rect;
}

void Matrix4::save(FileWriter& file) const
{
    for (auto element : m_)
        file.write(element);
}

void Matrix4::load(FileReader& file)
{
    for (auto& element : m_)
        file.read(element);
}

Matrix4 Matrix4::getScale(const Vec3& v)
{
    return {v.x, 0.0f, 0.0f, 0.0f, 0.0f, v.y, 0.0f, 0.0f, 0.0f, 0.0f, v.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

Matrix4 Matrix4::getTranslation(const Vec3& p)
{
    return {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, p.x, p.y, p.z, 1.0f};
}

Matrix4 Matrix4::getScaleAndTranslation(const Vec3& scale, const Vec3& translation)
{
    return {scale.x, 0.0f, 0.0f,    0.0f, 0.0f,          scale.y,       0.0f,          0.0f,
            0.0f,    0.0f, scale.z, 0.0f, translation.x, translation.y, translation.z, 1.0f};
}

Matrix4 Matrix4::getRotationX(float radians)
{
    auto sinr = sinf(radians);
    auto cosr = cosf(radians);

    return {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, cosr, sinr, 0.0f, 0.0f, -sinr, cosr, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

Matrix4 Matrix4::getRotationY(float radians)
{
    auto sinr = sinf(radians);
    auto cosr = cosf(radians);

    return {cosr, 0.0f, -sinr, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, sinr, 0.0f, cosr, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

Matrix4 Matrix4::getRotationZ(float radians)
{
    auto sinr = sinf(radians);
    auto cosr = cosf(radians);

    return {cosr, sinr, 0.0f, 0.0f, -sinr, cosr, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

Matrix4 Matrix4::getReflection(const Plane& plane)
{
    return {-2.0f * plane.getNormal().x * plane.getNormal().x + 1.0f,
            -2.0f * plane.getNormal().x * plane.getNormal().y,
            -2.0f * plane.getNormal().x * plane.getNormal().z,
            0.0f,

            -2.0f * plane.getNormal().y * plane.getNormal().x,
            -2.0f * plane.getNormal().y * plane.getNormal().y + 1.0f,
            -2.0f * plane.getNormal().y * plane.getNormal().z,
            0.0f,

            -2.0f * plane.getNormal().z * plane.getNormal().x,
            -2.0f * plane.getNormal().z * plane.getNormal().y,
            -2.0f * plane.getNormal().z * plane.getNormal().z + 1.0f,
            0.0f,

            -2.0f * plane.getDistance() * plane.getNormal().x,
            -2.0f * plane.getDistance() * plane.getNormal().y,
            -2.0f * plane.getDistance() * plane.getNormal().z,
            1.0f};
}

Matrix4 Matrix4::getOrthographicProjection(const Rect& rect, float nearPlaneDistance, float farPlaneDistance)
{
    auto m = Matrix4();

    m[0] = 2.0f / rect.getWidth();
    m[5] = 2.0f / rect.getHeight();
    m[10] = -2.0f / (farPlaneDistance - nearPlaneDistance);
    m[12] = -(rect.getRight() + rect.getLeft()) / rect.getWidth();
    m[13] = -(rect.getTop() + rect.getBottom()) / rect.getHeight();
    m[14] = -(farPlaneDistance + nearPlaneDistance) / (farPlaneDistance - nearPlaneDistance);
    m[15] = 1.0f;

    return m;
}

Matrix4 Matrix4::getPerspectiveProjection(float fieldOfView, float aspectRatio, float nearPlaneDistance,
                                          float farPlaneDistance)
{
    auto m = Matrix4();

    auto f = 1.0f / tanf(fieldOfView * 0.5f);

    m[0] = f / aspectRatio;
    m[5] = f;
    m[10] = (farPlaneDistance + nearPlaneDistance) / (nearPlaneDistance - farPlaneDistance);
    m[11] = -1.0f;
    m[14] = (2.0f * nearPlaneDistance * farPlaneDistance) / (nearPlaneDistance - farPlaneDistance);

    return m;
}

Vec3 Matrix4::unproject(const Vec3& p, const SimpleTransform& viewTransform, const Matrix4& projection,
                        const Rect& viewport)
{
    // Calculate inverse matrix
    auto mvpInverse = Matrix4();
    if (!(projection * viewTransform.getMatrix()).getInverse(mvpInverse))
        return Vec3::Zero;

    // Transform position into normalized device coordinates (-1 - 1 range)
    auto device = Vec3(((p.x - viewport.getLeft()) / viewport.getWidth()) * 2.0f - 1.0f,
                       ((p.y - viewport.getBottom()) / viewport.getHeight()) * 2.0f - 1.0f, p.z * 2.0f - 1.0f);

    // Transform by the inverse model-view-projection matrix
    auto result = mvpInverse * device;
    auto w = mvpInverse[3] * device.x + mvpInverse[7] * device.y + mvpInverse[11] * device.z + mvpInverse[15];

    if (fabsf(w) < Math::Epsilon)
        return Vec3::Zero;

    // Divide through by w to get the final result
    return result / w;
}

Matrix4::operator UnicodeString() const
{
    auto s = UnicodeString("[");

    for (auto i = 0U; i < 16; i += 4)
    {
        s << " " << m_[i] << " " << m_[i + 1] << " " << m_[i + 2] << " " << m_[i + 3] << " ";
        if (i != 12)
            s << "|";
    }

    return s << "]";
}

}
