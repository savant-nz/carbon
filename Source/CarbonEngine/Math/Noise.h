/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Helper methods for generating noise.
 */
class CARBON_API Noise
{
public:

    /**
     * Simple 2D noise function that generates noise in the range -1.0 - 1.0.
     */
    static float noise(int x, int y);

    /**
     * Interpolates the values of Noise::noise() in 2D.
     */
    static float interpolatedNoise(float x, float y);

    /**
     * 2D perlin noise generation.
     */
    static float perlin(float x, float y, unsigned int octaves, float persistence);
};

}
