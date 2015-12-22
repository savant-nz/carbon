/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix3.h"

namespace Carbon
{

const Matrix3 Matrix3::Identity(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

Matrix3 Matrix3::operator*(const Matrix3& other) const
{
    return {m_[0] * other[0] + m_[3] * other[1] + m_[6] * other[2], m_[1] * other[0] + m_[4] * other[1] + m_[7] * other[2],
            m_[2] * other[0] + m_[5] * other[1] + m_[8] * other[2], m_[0] * other[3] + m_[3] * other[4] + m_[6] * other[5],
            m_[1] * other[3] + m_[4] * other[4] + m_[7] * other[5], m_[2] * other[3] + m_[5] * other[4] + m_[8] * other[5],
            m_[0] * other[6] + m_[3] * other[7] + m_[6] * other[8], m_[1] * other[6] + m_[4] * other[7] + m_[7] * other[8],
            m_[2] * other[6] + m_[5] * other[7] + m_[8] * other[8]};
}

bool Matrix3::getInverse(Matrix3& inv) const
{
    inv[0] = m_[4] * m_[8] - m_[5] * m_[7];
    inv[1] = m_[1] * m_[8] - m_[2] * m_[7];
    inv[2] = m_[1] * m_[5] - m_[2] * m_[4];

    auto det = m_[0] * inv[0] - m_[3] * inv[1] + m_[6] * inv[2];

    if (fabsf(det) < Math::Epsilon)
        return false;

    det = 1.0f / det;

    inv[0] *= det;
    inv[1] *= -det;
    inv[2] *= det;
    inv[3] = det * -(m_[3] * m_[8] - m_[5] * m_[6]);
    inv[4] = det * (m_[0] * m_[8] - m_[2] * m_[6]);
    inv[5] = det * -(m_[0] * m_[5] - m_[2] * m_[3]);
    inv[6] = det * (m_[3] * m_[7] - m_[4] * m_[6]);
    inv[7] = det * -(m_[0] * m_[7] - m_[1] * m_[6]);
    inv[8] = det * (m_[0] * m_[4] - m_[1] * m_[3]);

    return true;
}

void Matrix3::transpose()
{
    std::swap(m_[1], m_[3]);
    std::swap(m_[2], m_[6]);
    std::swap(m_[5], m_[7]);
}

void Matrix3::save(FileWriter& file) const
{
    for (auto element : m_)
        file.write(element);
}

void Matrix3::load(FileReader& file)
{
    for (auto& element : m_)
        file.read(element);
}

Matrix3::operator UnicodeString() const
{
    auto s = UnicodeString("[");

    for (auto i = 0U; i < 9; i += 3)
    {
        s << " " << m_[i] << " " << m_[i + 1] << " " << m_[i + 2] << " ";
        if (i != 6)
            s << "|";
    }

    return s << "]";
}

Matrix3 Matrix3::getScale(const Vec3& v)
{
    return {v.x, 0.0f, 0.0f, 0.0f, v.y, 0.0f, 0.0f, 0.0f, v.z};
}

Matrix3 Matrix3::getRotationX(float radians)
{
    auto sinr = sinf(radians);
    auto cosr = cosf(radians);

    return {1.0f, 0.0f, 0.0f, 0.0f, cosr, sinr, 0.0f, -sinr, cosr};
}

Matrix3 Matrix3::getRotationY(float radians)
{
    auto sinr = sinf(radians);
    auto cosr = cosf(radians);

    return {cosr, 0.0f, -sinr, 0.0f, 1.0f, 0.0f, sinr, 0.0f, cosr};
}

Matrix3 Matrix3::getRotationZ(float radians)
{
    auto sinr = sinf(radians);
    auto cosr = cosf(radians);

    return {cosr, sinr, 0.0f, -sinr, cosr, 0.0f, 0.0f, 0.0f, 1.0f};
}

}
