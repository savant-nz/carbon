/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

// There are three color component types used in uncompressed images: byte_t, uint16_t (for 16-bit floats), and 32-bit float.
// Each of these is available as a specialization of the ColorComponentType class and there are methods for converting each
// to/from a 32-bit float, this allows image conversion routines to abstractly deal with a variety of different image types by
// way of a common 32-bit float intermediate representation. More complicated formats such as packed 16-bit RGB formats and
// compressed formats use custom read/write pixel functions that are implemented separately and so do not use the
// ColorComponentType class.
template <typename T> class ColorComponentType
{
public:

    static const T One;    // The value of a maximally saturated color component of this type

    static float toFloat(T value);
    static T fromFloat(float value);

    static Color toColor(T value) { return {toFloat(value)}; }

    static Color toColor(T rgb, T alpha)
    {
        auto f = toFloat(rgb);

        return {f, f, f, toFloat(alpha)};
    }

    static Color toColor(T red, T green, T blue) { return {toFloat(red), toFloat(green), toFloat(blue), 1.0f}; }

    static Color toColor(T red, T green, T blue, T alpha)
    {
        return {toFloat(red), toFloat(green), toFloat(blue), toFloat(alpha)};
    }

    static unsigned int getPixelOffset(unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                                       unsigned int y, unsigned int z, unsigned int componentCount)
    {
        return ((width * height * z) + width * y + x) * sizeof(T) * componentCount;
    }

    template <unsigned int RedOffset, unsigned int GreenOffset, unsigned int BlueOffset>
    static Color readRGBPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                              unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 3));

        return toColor(pixel[RedOffset], pixel[GreenOffset], pixel[BlueOffset]);
    }

    template <unsigned int RedOffset, unsigned int GreenOffset, unsigned int BlueOffset>
    static void writeRGBPixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                              unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 3));

        pixel[RedOffset] = fromFloat(color.r);
        pixel[GreenOffset] = fromFloat(color.g);
        pixel[BlueOffset] = fromFloat(color.b);
    }

    template <unsigned int RedOffset, unsigned int GreenOffset, unsigned int BlueOffset, unsigned int AlphaOffset>
    static Color readRGBAPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 4));
        return toColor(pixel[RedOffset], pixel[GreenOffset], pixel[BlueOffset], pixel[AlphaOffset]);
    }

    template <unsigned int RedOffset, unsigned int GreenOffset, unsigned int BlueOffset, unsigned int AlphaOffset>
    static void writeRGBAPixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 4));

        pixel[RedOffset] = fromFloat(color.r);
        pixel[GreenOffset] = fromFloat(color.g);
        pixel[BlueOffset] = fromFloat(color.b);
        pixel[AlphaOffset] = fromFloat(color.a);
    }

    static Color readRedPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                              unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        return toColor(*pixel, T(), T(), One);
    }

    static void writeRedPixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                              unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        pixel[0] = fromFloat(color.r);
    }

    static Color readRedGreenPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        return toColor(pixel[0], pixel[1], T(), One);
    }

    static void writeRedGreenPixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                                   unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        pixel[0] = fromFloat(color.r);
        pixel[1] = fromFloat(color.g);
    }

    static Color readAlphaPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                                unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        return toColor(One, *pixel);
    }

    static void writeAlphaPixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                                unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        pixel[0] = fromFloat(color.a);
    }

    static Color readLuminancePixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                    unsigned int x, unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        return toColor(*pixel, One);
    }

    static void writeLuminancePixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                                    unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 1));
        pixel[0] = fromFloat(color.getRGBLuminance());
    }

    static Color readLuminanceAlphaPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                         unsigned int x, unsigned int y, unsigned int z)
    {
        auto pixel = reinterpret_cast<const T*>(data + getPixelOffset(width, height, depth, x, y, z, 2));
        return toColor(pixel[0], pixel[1]);
    }

    static void writeLuminanceAlphaPixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                         unsigned int x, unsigned int y, unsigned int z, const Color& color)
    {
        auto pixel = reinterpret_cast<T*>(data + getPixelOffset(width, height, depth, x, y, z, 2));

        pixel[0] = fromFloat(color.getRGBLuminance());
        pixel[1] = fromFloat(color.a);
    }
};

// Implement ColorComponentType for the byte_t type.
template <> const byte_t ColorComponentType<byte_t>::One = 0xFF;

template <> float ColorComponentType<byte_t>::toFloat(byte_t value)
{
    return Math::byteToFloat(value);
}

template <> byte_t ColorComponentType<byte_t>::fromFloat(float value)
{
    return byte_t(Math::clamp01(value) * 255.0f);
}

// Implement ColorComponentType for the uint16_t type, this is used for 16-bit half-precision floating point images.
template <> const uint16_t ColorComponentType<uint16_t>::One = 0x3C00;

template <> float ColorComponentType<uint16_t>::toFloat(uint16_t value)
{
    // The format of a 16-bit half float is 1-bit sign, 5-bit exponent, 10-bit mantissa

    auto sign = uint((value & 0x8000) >> 15);
    auto exponent = uint((value & 0x7C00) >> 10);
    auto mantissa = uint(value & 0x03FF);

    // Detect zeroes, denormalized values get treated as zero
    if (exponent == 0)
        return sign ? -0.0f : 0.0f;

    // Detect infinities, NaNs get treated as infinities
    if (exponent == 31)
        return sign ? -FLT_MAX : FLT_MAX;

    return (sign ? -1.0f : 1.0f) * powf(2.0f, float(exponent) - 15.0f) * (1.0f + float(mantissa) / 1024.0f);
}

template <> uint16_t ColorComponentType<uint16_t>::fromFloat(float value)
{
    const auto halfNegativeZero = uint16_t(0x8000);
    const auto halfPositiveZero = uint16_t(0x0000);
    const auto halfNegativeInfinity = uint16_t(0xFC00);
    const auto halfPositiveInfinity = uint16_t(0x7C00);

    // The format of a 32-bit float is 1-bit sign, 8-bit exponent, 23-bit mantissa

    auto bits = 0U;
    memcpy(&bits, &value, 4);

    auto sign = uint((bits & 0x80000000) >> 31);
    auto exponent = uint((bits & 0x7F800000) >> 23);
    auto mantissa = uint(bits & 0x007FFFFF);

    // Detect zeroes, denormalized values get treated as zero
    if (exponent == 0)
        return sign ? halfNegativeZero : halfPositiveZero;

    // Detect infinities, NaNs get treated as infinites
    if (exponent == 255)
        return sign ? halfNegativeInfinity : halfPositiveInfinity;

    // If the number is too big or too small to fit into the 16-bit float then round to zero or infinity appropriately
    auto trueExponent = int(exponent) - 127;
    if (trueExponent < -14)
        return sign ? halfNegativeZero : halfPositiveZero;
    if (trueExponent > 15)
        return sign ? halfNegativeInfinity : halfPositiveInfinity;

    exponent = uint(trueExponent + 15);
    mantissa >>= 13;

    return uint16_t(((sign & 1) << 15) | ((exponent & 0x1F) << 10) | (mantissa & 0x3FF));
}

// Implement ColorComponentType for the float type.
template <> const float ColorComponentType<float>::One = 1.0f;

template <> float ColorComponentType<float>::toFloat(float value)
{
    return value;
}

template <> float ColorComponentType<float>::fromFloat(float value)
{
    return value;
}

bool Image::setPixelFormat(PixelFormat newPixelFormat)
{
    if (newPixelFormat == UnknownPixelFormat || getPixelFormat() == newPixelFormat)
        return true;

    if (!isValidImage())
        return false;

    // Check whether this image conversion is supported
    if (!canConvertToPixelFormat(newPixelFormat))
    {
        LOG_ERROR << "Converting images from " << getPixelFormatString(getPixelFormat()) << " to "
                  << getPixelFormatString(newPixelFormat) << " is not supported";

        return false;
    }

    auto sources = getAllData();

    // Allocate space to put the converted image
    auto targets = Vector<Vector<byte_t>>();

    try
    {
        auto dataSize = getImageDataSize(getWidth(), getHeight(), getDepth(), newPixelFormat, hasMipmaps());

        targets.resize(sources.size());
        for (auto& target : targets)
            target.resize(dataSize);
    }
    catch (const std::bad_alloc&)
    {
        LOG_ERROR << "Insufficient memory to convert this image";
        return false;
    }

    // Convert each source
    for (auto i = 0U; i < sources.size(); i++)
    {
        auto w = getWidth();
        auto h = getHeight();
        auto d = getDepth();

        auto sourceData = sources[i];
        auto targetData = targets[i].getData();

        for (auto j = 0U; j < getMipmapCount(); j++)
        {
            convertRawImage(sourceData, getPixelFormat(), targetData, newPixelFormat, w, h, d);

            sourceData += getImageDataSize(w, h, d, getPixelFormat(), false);
            targetData += getImageDataSize(w, h, d, newPixelFormat, false);

            getNextMipmapSize(w, h, d);
        }

        replaceFrameData(sources[i], targets[i]);
    }

    pixelFormat_ = newPixelFormat;

    return true;
}

bool Image::convertRawImage(const byte_t* source, PixelFormat sourcePixelFormat, byte_t* destination,
                            PixelFormat destinationPixelFormat, unsigned int width, unsigned int height, unsigned int depth)
{
    // Check that the source and destination buffers are different, in-place conversion is not supported
    if (source == destination)
    {
        LOG_ERROR << "Source and destination buffers cannot be the same";
        return false;
    }

    // Get the reader and writer functions to use
    auto fnReadPixel = getPixelFormatReadFunction(sourcePixelFormat);
    auto fnWritePixel = getPixelFormatWriteFunction(destinationPixelFormat);
    if (!fnReadPixel)
    {
        LOG_ERROR << "Can't read from " << getPixelFormatString(sourcePixelFormat);
        return false;
    }
    if (!fnWritePixel)
    {
        LOG_ERROR << "Can't write to " << getPixelFormatString(destinationPixelFormat);
        return false;
    }

    // If the pixel formats are the same then just do a straight copy
    if (sourcePixelFormat == destinationPixelFormat)
        memcpy(destination, source, getImageDataSize(width, height, depth, destinationPixelFormat, false));
    else
    {
        for (auto z = 0U; z < depth; z++)
        {
            for (auto y = 0U; y < height; y++)
            {
                for (auto x = 0U; x < width; x++)
                {
                    fnWritePixel(destination, width, height, depth, x, y, z,
                                 fnReadPixel(source, width, height, depth, x, y, z));
                }
            }
        }
    }

    return true;
}

bool Image::canConvertToPixelFormat(PixelFormat pixelFormat) const
{
    // Check whether there are read/write functions
    return getPixelFormatReadFunction(pixelFormat_) && getPixelFormatWriteFunction(pixelFormat);
}

Color Image::readRGB565Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                             unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(rgb565GetR(data)), ColorComponentType<byte_t>::toFloat(rgb565GetG(data)),
            ColorComponentType<byte_t>::toFloat(rgb565GetB(data)), 1.0f};
}

void Image::writeRGB565Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                             unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 3;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 2;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 3;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((red << 11) | (green << 5) | blue);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(data, 2);
#endif
}

Color Image::readBGR565Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                             unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(rgb565GetB(data)), ColorComponentType<byte_t>::toFloat(rgb565GetG(data)),
            ColorComponentType<byte_t>::toFloat(rgb565GetR(data)), 1.0f};
}

void Image::writeBGR565Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                             unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 3;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 2;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 3;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((blue << 11) | (green << 5) | red);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(data, 2);
#endif
}

Color Image::readRGBA5551Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0xF800, 10, 5)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x07C0, 5, 5)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x003E, 0, 5)), (*data & 0x01) ? 1.0f : 0.0f};
}

void Image::writeRGBA5551Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 3;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 3;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 3;
    auto alpha = ColorComponentType<byte_t>::fromFloat(color.a) >> 7;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((red << 11) | (green << 6) | (blue << 1) | alpha);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(data, 2);
#endif
}

Color Image::readARGB1555Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x7C00, 10, 5)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x03E0, 5, 5)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x001F, 0, 5)), (*data & 0x80) ? 1.0f : 0.0f};
}

void Image::writeARGB1555Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 3;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 3;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 3;
    auto alpha = ColorComponentType<byte_t>::fromFloat(color.a) >> 7;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((alpha << 15) | (red << 10) | (green << 5) | blue);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(data, 2);
#endif
}

Color Image::readRGBA4444Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0xF000, 12, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x0F00, 8, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x00F0, 4, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x000F, 0, 4))};
}

void Image::writeRGBA4444Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 4;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 4;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 4;
    auto alpha = ColorComponentType<byte_t>::fromFloat(color.a) >> 4;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((red << 12) | (green << 8) | (blue << 4) | alpha);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(data, 2);
#endif
}

Color Image::readARGB4444Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x0F00, 8, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x00F0, 4, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x000F, 0, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0xF000, 12, 4))};
}

void Image::writeARGB4444Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 4;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 4;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 4;
    auto alpha = ColorComponentType<byte_t>::fromFloat(color.a) >> 4;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((alpha << 12) | (red << 8) | (green << 4) | blue);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(data, 2);
#endif
}

Color Image::readABGR4444Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 2;

    return {ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x000F, 0, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x00F0, 4, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0x0F00, 8, 4)),
            ColorComponentType<byte_t>::toFloat(extract16BitValue(data, 0xF000, 12, 4))};
}

void Image::writeABGR4444Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                               unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 2;

    auto red = ColorComponentType<byte_t>::fromFloat(color.r) >> 4;
    auto green = ColorComponentType<byte_t>::fromFloat(color.g) >> 4;
    auto blue = ColorComponentType<byte_t>::fromFloat(color.b) >> 4;
    auto alpha = ColorComponentType<byte_t>::fromFloat(color.a) >> 4;

    *reinterpret_cast<uint16_t*>(data) = uint16_t((alpha << 12) | (blue << 8) | (green << 4) | red);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(*reinterpret_cast<uint16_t*>(data));
#endif
}

template <Image::PixelFormat SourcePixelFormat>
Color Image::readDXTPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                          unsigned int y, unsigned int z)
{
    auto output = std::array<std::array<byte_t, 4>, 16>();

    decompressDXTCBlock(data + ((y / 4) * (width / 4) + (x / 4)) * getCompressedPixelFormatBlockSize(SourcePixelFormat), output,
                        SourcePixelFormat);

    auto& rgba = output[(y % 4) * 4 + (x % 4)];

    return {ColorComponentType<byte_t>::toFloat(rgba[0]), ColorComponentType<byte_t>::toFloat(rgba[1]),
            ColorComponentType<byte_t>::toFloat(rgba[2]), ColorComponentType<byte_t>::toFloat(rgba[3])};
}

std::array<std::pair<Image::ReadPixelFunction, Image::WritePixelFunction>, Image::LastPixelFormat>
    Image::pixelFormatReadWriteFunctions_;

void Image::setupPixelFormatReadWriteFunctions()
{
    // Most uncompressed formats use instantiations of the read and write methods on the ColorComponentType class, other more
    // complicated formats have custom read/write functions.

    pixelFormatReadWriteFunctions_[UnknownPixelFormat] = {nullptr, nullptr};

    pixelFormatReadWriteFunctions_[Alpha8] = {&ColorComponentType<byte_t>::readAlphaPixel,
                                              &ColorComponentType<byte_t>::writeAlphaPixel};
    pixelFormatReadWriteFunctions_[Luminance8] = {&ColorComponentType<byte_t>::readLuminancePixel,
                                                  &ColorComponentType<byte_t>::writeLuminancePixel};
    pixelFormatReadWriteFunctions_[LuminanceAlpha8] = {&ColorComponentType<byte_t>::readLuminanceAlphaPixel,
                                                       &ColorComponentType<byte_t>::writeLuminanceAlphaPixel};
    pixelFormatReadWriteFunctions_[RGB8] = {&ColorComponentType<byte_t>::readRGBPixel<0, 1, 2>,
                                            &ColorComponentType<byte_t>::writeRGBPixel<0, 1, 2>};
    pixelFormatReadWriteFunctions_[BGR8] = {&ColorComponentType<byte_t>::readRGBPixel<2, 1, 0>,
                                            &ColorComponentType<byte_t>::writeRGBPixel<2, 1, 0>};
    pixelFormatReadWriteFunctions_[RGBA8] = {&ColorComponentType<byte_t>::readRGBAPixel<0, 1, 2, 3>,
                                             &ColorComponentType<byte_t>::writeRGBAPixel<0, 1, 2, 3>};
    pixelFormatReadWriteFunctions_[BGRA8] = {&ColorComponentType<byte_t>::readRGBAPixel<2, 1, 0, 3>,
                                             &ColorComponentType<byte_t>::writeRGBAPixel<2, 1, 0, 3>};
    pixelFormatReadWriteFunctions_[ABGR8] = {&ColorComponentType<byte_t>::readRGBAPixel<3, 2, 1, 0>,
                                             &ColorComponentType<byte_t>::writeRGBAPixel<3, 2, 1, 0>};

    pixelFormatReadWriteFunctions_[SRGB8] = {&Image::readSRGB8Pixel, &Image::writeSRGB8Pixel};
    pixelFormatReadWriteFunctions_[SRGBA8] = {&Image::readSRGBA8Pixel, &Image::writeSRGBA8Pixel};

    pixelFormatReadWriteFunctions_[RGB565] = {&Image::readRGB565Pixel, &Image::writeRGB565Pixel};
    pixelFormatReadWriteFunctions_[BGR565] = {&Image::readBGR565Pixel, &Image::writeBGR565Pixel};
    pixelFormatReadWriteFunctions_[RGBA5551] = {&Image::readRGBA5551Pixel, &Image::writeRGBA5551Pixel};
    pixelFormatReadWriteFunctions_[RGBA4444] = {&Image::readRGBA4444Pixel, &Image::writeRGBA4444Pixel};
    pixelFormatReadWriteFunctions_[ARGB1555] = {&Image::readARGB1555Pixel, &Image::writeARGB1555Pixel};
    pixelFormatReadWriteFunctions_[ARGB4444] = {&Image::readARGB4444Pixel, &Image::writeARGB4444Pixel};
    pixelFormatReadWriteFunctions_[ABGR4444] = {&Image::readABGR4444Pixel, &Image::writeABGR4444Pixel};

    pixelFormatReadWriteFunctions_[Red16f] = {&ColorComponentType<uint16_t>::readRedPixel,
                                              &ColorComponentType<uint16_t>::writeRedPixel};
    pixelFormatReadWriteFunctions_[RedGreen16f] = {&ColorComponentType<uint16_t>::readRedGreenPixel,
                                                   &ColorComponentType<uint16_t>::writeRedGreenPixel};
    pixelFormatReadWriteFunctions_[RGB16f] = {&ColorComponentType<uint16_t>::readRGBPixel<0, 1, 2>,
                                              &ColorComponentType<uint16_t>::writeRGBPixel<0, 1, 2>};
    pixelFormatReadWriteFunctions_[RGBA16f] = {&ColorComponentType<uint16_t>::readRGBAPixel<0, 1, 2, 3>,
                                               &ColorComponentType<uint16_t>::writeRGBAPixel<0, 1, 2, 3>};
    pixelFormatReadWriteFunctions_[Red32f] = {&ColorComponentType<float>::readRedPixel,
                                              &ColorComponentType<float>::writeRedPixel};
    pixelFormatReadWriteFunctions_[RedGreen32f] = {&ColorComponentType<float>::readRedGreenPixel,
                                                   &ColorComponentType<float>::writeRedGreenPixel};
    pixelFormatReadWriteFunctions_[RGB32f] = {&ColorComponentType<float>::readRGBPixel<0, 1, 2>,
                                              &ColorComponentType<float>::writeRGBPixel<0, 1, 2>};
    pixelFormatReadWriteFunctions_[RGBA32f] = {&ColorComponentType<float>::readRGBAPixel<0, 1, 2, 3>,
                                               &ColorComponentType<float>::writeRGBAPixel<0, 1, 2, 3>};

    pixelFormatReadWriteFunctions_[Depth] = {nullptr, nullptr};
    pixelFormatReadWriteFunctions_[Depth24Stencil8] = {nullptr, nullptr};

    pixelFormatReadWriteFunctions_[DXT1] = {&Image::readDXTPixel<Image::DXT1>, nullptr};
    pixelFormatReadWriteFunctions_[DXT3] = {&Image::readDXTPixel<Image::DXT3>, nullptr};
    pixelFormatReadWriteFunctions_[DXT5] = {&Image::readDXTPixel<Image::DXT5>, nullptr};

    pixelFormatReadWriteFunctions_[PVRTC2BitRGB] = {&Image::readPVRTC2BitRGBPixel, nullptr};
    pixelFormatReadWriteFunctions_[PVRTC2BitRGBA] = {&Image::readPVRTC2BitRGBAPixel, nullptr};
    pixelFormatReadWriteFunctions_[PVRTC4BitRGB] = {&Image::readPVRTC4BitRGBPixel, nullptr};
    pixelFormatReadWriteFunctions_[PVRTC4BitRGBA] = {&Image::readPVRTC4BitRGBAPixel, nullptr};
}
CARBON_REGISTER_STARTUP_FUNCTION(Image::setupPixelFormatReadWriteFunctions, 0)

}
