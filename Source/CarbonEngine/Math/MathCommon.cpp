/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/HashFunctions.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/RandomNumberGenerator.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Math/Vec2.h"
#include "CarbonEngine/Math/Vec2i.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

std::array<float, 256> Math::byteToFloat_;

const SimpleTransform SimpleTransform::Identity(Vec3(0.0f, 0.0f, 0.0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

const Vec2 Vec2::Zero;
const Vec2 Vec2::One(1.0f, 1.0f);
const Vec2 Vec2::Half(0.5f, 0.5f);
const Vec2 Vec2::UnitX(1.0f, 0.0f);
const Vec2 Vec2::UnitY(0.0f, 1.0f);

const Vec2i Vec2i::Zero;
const Vec2i Vec2i::One(1, 1);
const Vec2i Vec2i::UnitX(1, 0);
const Vec2i Vec2i::UnitY(0, 1);

const Vec3 Vec3::Zero;
const Vec3 Vec3::One(1.0f, 1.0f, 1.0f);
const Vec3 Vec3::Half(0.5f, 0.5f, 0.5f);
const Vec3 Vec3::UnitX(1.0f, 0.0f, 0.0f);
const Vec3 Vec3::UnitY(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::UnitZ(0.0f, 0.0f, 1.0f);
const Vec3 Vec3::NegateZ(1.0f, 1.0f, -1.0f);

bool Math::isPowerOfTwo(unsigned int n)
{
    return n != 0 && ((n & (n - 1)) == 0);
}

unsigned int Math::getNextPowerOfTwo(unsigned int n)
{
    if (n >= 0x80000000)
        return n;

    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);

    return n + 1;
}

unsigned int Math::getPreviousPowerOfTwo(unsigned int n)
{
    if (n == 0)
        return 0;

    auto p = 1U << 31;

    while (p >= n)
        p /= 2;

    return p;
}

int Math::random(int lower, int upper)
{
    if (upper < lower)
        return lower;

    return lower + int(double(upper - lower + 1) * double(RandomNumberGenerator::run()) / (double(UINT_MAX) + 1.0));
}

float Math::random(float lower, float upper)
{
    if (upper < lower)
        return lower;

    return lower + (upper - lower) * float(double(RandomNumberGenerator::run()) / double(UINT_MAX));
}

int Math::positiveModulus(int value, int modulus)
{
    if (!modulus)
        return value;

    return ((value % modulus) + modulus) % modulus;
}

float Math::normalDistribution(float x, float mean, float standardDeviation)
{
    static const auto rootTwoPi = sqrtf(Math::TwoPi);

    // Compute squared distance from the mean
    auto d = (x - mean) * (x - mean);

    return expf(-d / (2 * standardDeviation * standardDeviation)) / (standardDeviation * rootTwoPi);
}

String Math::createGUID()
{
    // Generate a random string, start with the date and time and then append a set of random characters to it
    auto random = FileSystem::getShortDateTime();
    for (auto i = 0U; i < 32; i++)
        random.append(char(Math::random(int(' '), int('~'))));

    // Hash and turn into an 8 digit hex string
    return String::toHex(random.hash());
}

void Math::initializeByteToFloatLookupTable()
{
    for (auto i = 0U; i < 256; i++)
        byteToFloat_[i] = float(i) / 255.0f;
}
CARBON_REGISTER_STARTUP_FUNCTION(Math::initializeByteToFloatLookupTable, 0)

}
