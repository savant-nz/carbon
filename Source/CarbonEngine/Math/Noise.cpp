/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Math/Noise.h"

namespace Carbon
{

float Noise::noise(int x, int y)
{
    auto n = x + y * 57;
    n = (n << 13) ^ n;

    return 1.0f - float((n * (n * n * 15731 + 789221) + 1376312589) & 0x7FFFFFFF) / 1073741824.0f;
}

float Noise::interpolatedNoise(float x, float y)
{
    auto ix = int(x);
    auto iy = int(y);

    auto v1 = noise(ix, iy);
    auto v2 = noise(ix + 1, iy);
    auto v3 = noise(ix, iy + 1);
    auto v4 = noise(ix + 1, iy + 1);

    auto i1 = Interpolate::fast(v1, v2, x - ix);
    auto i2 = Interpolate::fast(v3, v4, x - ix);

    return Interpolate::fast(i1, i2, y - iy);
}

float Noise::perlin(float x, float y, unsigned int octaves, float persistence)
{
    auto total = 0.0f;
    auto frequency = 1.0f;
    auto amplitude = 1.0f;

    for (auto i = 0U; i < octaves; i++)
    {
        total += interpolatedNoise(x * frequency, y * frequency) * amplitude;

        frequency *= 2.0f;
        amplitude *= persistence;
    }

    return total;
}

}
