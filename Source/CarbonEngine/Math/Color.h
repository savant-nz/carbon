/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * RGBA color.
 */
class CARBON_API Color
{
public:

    /**
     * Red component of this color.
     */
    float r = 0.0f;

    /**
     * Green component of this color.
     */
    float g = 0.0f;

    /**
     * Blue component of this color.
     */
    float b = 0.0f;

    /**
     * Alpha component of this color.
     */
    float a = 1.0f;

    Color() {}

    /**
     * Scalar constructor. The red, green and blue components are set to \a value. The alpha component is set to one.
     */
    explicit Color(float f) : r(f), g(f), b(f), a(1.0f) {}

    /**
     * Component constructor. The red, green, blue and alpha components are set to \a r, \a g, \a b and \a a respectively.
     */
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    /**
     * Component equality.
     */
    bool operator==(const Color& other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }

    /**
     * Component inequality.
     */
    bool operator!=(const Color& other) const { return r != other.r || g != other.g || b != other.b || a != other.a; }

    /**
     * Component addition.
     */
    Color operator+(const Color& other) const { return {r + other.r, g + other.g, b + other.b, a + other.a}; }

    /**
     * Component addition in place.
     */
    Color& operator+=(const Color& other)
    {
        r += other.r;
        g += other.g;
        b += other.b;
        a += other.a;

        return *this;
    }

    /**
     * Component negation.
     */
    Color operator-() const { return {-r, -g, -b, -a}; }

    /**
     * Component subtraction.
     */
    Color operator-(const Color& other) const { return {r - other.r, g - other.g, b - other.b, a - other.a}; }

    /**
     * Component subtraction in place.
     */
    Color& operator-=(const Color& other)
    {
        r -= other.r;
        g -= other.g;
        b -= other.b;
        a -= other.a;

        return *this;
    }

    /**
     * Scalar multiplication.
     */
    Color operator*(float f) const { return {r * f, g * f, b * f, a * f}; }

    /**
     * Component multiplication.
     */
    Color operator*(const Color& other) const { return {r * other.r, g * other.g, b * other.b, a * other.a}; }

    /**
     * Scalar multiplication in place.
     */
    Color& operator*=(float f)
    {
        r *= f;
        g *= f;
        b *= f;
        a *= f;

        return *this;
    }

    /**
     * Component multiplication in place.
     */
    Color& operator*=(const Color& other)
    {
        r *= other.r;
        g *= other.g;
        b *= other.b;
        a *= other.a;

        return *this;
    }

    /**
     * Scalar division.
     */
    Color operator/(float f) const
    {
        f = 1.0f / f;

        return {r * f, g * f, b * f, a * f};
    }

    /**
     * Component division.
     */
    Color operator/(const Color& other) const { return {r / other.r, g / other.g, b / other.b, a / other.a}; }

    /**
     * Scalar division in place.
     */
    Color& operator/=(float f)
    {
        f = 1.0f / f;

        r *= f;
        g *= f;
        b *= f;
        a *= f;

        return *this;
    }

    /**
     * Component division in place.
     */
    Color& operator/=(const Color& other)
    {
        r /= other.r;
        g /= other.g;
        b /= other.b;
        a /= other.a;

        return *this;
    }

    /**
     * Sets all the components of this color.
     */
    void setRGBA(float r_, float g_, float b_, float a_)
    {
        r = r_;
        g = g_;
        b = b_;
        a = a_;
    }

    /**
     * Clamps all component values in this color into the range \a lower to \a upper.
     */
    void clamp(float lower, float upper);

    /**
     * Converts the RGB portion of this color into a single luminance value using the standard RGB luminance formula.
     */
    float getRGBLuminance() const { return r * 0.3f + g * 0.59f + b * 0.11f; }

    /**
     * Converts this color into a single intensity value by multiplying the value of Color::getRGBLuminance() by the alpha.
     */
    float getIntensity() const { return getRGBLuminance() * a; }

    /**
     * Normalizes the RGB components so that at least one of them has the value 1.0.
     */
    Color normalized() const;

    /**
     * Converts this color to a 32-bit RGBA8 value. This method is endian-aware.
     */
    unsigned int toRGBA8() const
    {
        auto c = *this;
        c.clamp(0.0f, 1.0f);
        c *= 255.0f;

#ifdef CARBON_BIG_ENDIAN
        return uint(byte_t(c.r) << 24) | uint(byte_t(c.g) << 16) | uint(byte_t(c.b) << 8) | uint(byte_t(c.a));
#else
        return uint(byte_t(c.r)) | uint(byte_t(c.g) << 8) | uint(byte_t(c.b) << 16) | uint(byte_t(c.a) << 24);
#endif
    }

    /**
     * Returns this color value as a float[4] array.
     */
    const float* asArray() const { return &r; }

    /**
     * Returns this color as a string in the form "r g b a".
     */
    operator UnicodeString() const { return UnicodeString() << r << " " << g << " " << b << " " << a; }

#ifdef WINDOWS
    /**
     * Converts this color to a Windows API COLORREF value.
     */
    COLORREF toCOLORREF() const;
#endif

    /**
     * Saves this color to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(r, g, b, a); }

    /**
     * Loads this color from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(r, g, b, a); }

    /**
     * Color instance with all components set to zero
     */
    static const Color Zero;

    /**
     * Color instance with the red, green and blue components set to zero and the alpha component set to one.
     */
    static const Color Black;

    /**
     * Color instance with all components set to one.
     */
    static const Color White;

    /**
     * Color instance with the red and alpha components set to one and the other components set to zero.
     */
    static const Color Red;

    /**
     * Color instance with the green and alpha components set to one and the other components set to zero.
     */
    static const Color Green;

    /**
     * Color instance with the blue and alpha components set to one and the other components set to zero.
     */
    static const Color Blue;

    /**
     * Returns a color where all values are randomly generated in the range 0 to 1.
     */
    static Color random();

    /**
     * Returns a color where the RGB values are randomly generated in the range 0 to 1 and the alpha value is 1.
     */
    static Color randomRGB();
};

}
