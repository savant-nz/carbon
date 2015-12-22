/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * General math operations and constants.
 */
class CARBON_API Math
{
public:

    /**
     * The default epsilon value used in fuzzy comparisons. Currently set to 0.001f.
     */
    static constexpr float Epsilon = 0.001f;

    /**
     * Pi.
     */
    static constexpr float Pi = 3.1415927f;

    /**
     * Pi / 2.
     */
    static constexpr float HalfPi = 1.5707964f;

    /**
     * Pi / 4.
     */
    static constexpr float QuarterPi = 0.78539816f;

    /**
     * Pi * 2.
     */
    static constexpr float TwoPi = 6.2831855f;

    /**
     * Returns whether the passed value is a power of two.
     */
    static bool isPowerOfTwo(unsigned int n);

    /**
     * Returns the smallest power of two that is greater than the given value.
     */
    static unsigned int getNextPowerOfTwo(unsigned int n);

    /**
     * Returns the largest power of two that is smaller than the given value.
     */
    static unsigned int getPreviousPowerOfTwo(unsigned int n);

    /**
     * Templated clamping function. Returns the given value clamped to the given range.
     */
    template <typename T> static T clamp(const T& value, const T& lower, const T& upper)
    {
        return std::max(lower, std::min(value, upper));
    }

    /**
     * Returns the given floating point value clamped to the range 0-1.
     */
    static float clamp01(float value) { return clamp(value, 0.0f, 1.0f); }

    /**
     * Returns the given value clamped so that it lies between -limit and limit, i.e. its absolute value is less than \a limit.
     */
    template <typename T> static T absClamp(const T& value, const T& limit) { return clamp(value, -limit, limit); }

    /**
     * Returns the sign of the passed floating point value. Returns 1.0f for positive, -1.0f for negative, and 0.0f for 0.0f.
     */
    static float getSign(float value) { return float((0.0f < value) - (value < 0.0f)); }

    /**
     * Returns the sign of the passed integer. Returns 1 for positive, -1 for negative, and 0 for 0.
     */
    static int getSign(int value) { return (0 < value) - (value < 0); }

    /**
     * Returns a random integer in the given range. The range of return values is inclusive of \a lower and \a upper.
     */
    static int random(int lower, int upper);

    /**
     * Returns a random floating point number in the given range. The range of return values is inclusive of \a lower and \a
     * upper.
     */
    static float random(float lower, float upper);

    /**
     * Templated random function that can generate a random object of any type provided it defines addition, subtraction and
     * floating point multiplication operators.
     */
    template <typename T> static T random(const T& lower, const T& upper)
    {
        return lower + (upper - lower) * random(0.0f, 1.0f);
    }

    /**
     * Returns a random boolean value, the chance of the return value being true is specified by \a chance which should be
     * between zero and one and defaults to 0.5.
     */
    static bool randomBool(float chance = 0.5f) { return random(0.0f, 1.0f) <= clamp01(chance); }

    /**
     * Finds the largest and smallest elements in an array.
     */
    template <typename T> static void calculateBounds(const T* data, unsigned int size, T& lowest, T& highest)
    {
        if (size == 0)
            return;

        lowest = data[0];
        highest = data[0];

        for (auto i = 1U; i < size; i++)
        {
            lowest = std::min(lowest, data[i]);
            highest = std::max(highest, data[i]);
        }
    }

    /**
     * Returns the positive modulus of \a value mod \a modulus. This method returns the same result for positive values as the
     * standard modulus operator, but for negative values the result of the modulus will be shifted into the positive range. For
     * example, -1 mod 5 will return 4, whereas the standard modulus operator would return -1. If \a modulus is zero then \a
     * value is returned unchanged.
     */
    static int positiveModulus(int value, int modulus);

    /**
     * Rounds the given floating point number to the closest whole number value.
     */
    static float round(float f) { return floorf(f + 0.5f); }

    /**
     * Rounds the given unsigned integer up so that it is a multiple of a given factor.
     */
    static unsigned int roundUp(unsigned int value, unsigned int factor)
    {
        auto r = value % factor;

        if (r == 0)
            return value;

        return value + factor - r;
    }

    /**
     * Returns the fractional part of the given floating point number.
     */
    static float fract(float f) { return f - Math::getSign(f) * floorf(fabsf(f)); }

    /**
     * Converts an angle from degrees into radians.
     */
    static float degreesToRadians(float f) { return f * (Pi / 180.0f); }

    /**
     * Converts an angle from radians into degrees.
     */
    static float radiansToDegrees(float f) { return f * (180.0f / Pi); }

    /**
     * Calculates the normal distribution value at position \a x for the given mean and standard deviation.
     */
    static float normalDistribution(float x, float mean, float standardDeviation);

    /**
     * Returns a (probably) unique 8 character string that can be used as a unique identifier.
     */
    static String createGUID();

    /**
     * Fast lookup-based conversion from a byte to a floating point number in the range 0 - 1.
     */
    static float byteToFloat(byte_t value) { return byteToFloat_[value]; }

    /**
     * Initializes the lookup table used by Math::byteToFloat(), this is called automatically on startup.
     */
    static void initializeByteToFloatLookupTable();

private:

    static std::array<float, 256> byteToFloat_;
};

}
