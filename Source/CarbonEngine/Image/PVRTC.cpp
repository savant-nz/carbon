/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Endian.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Math/MathCommon.h"

// The PVRTC decompression code in this file is based on the reference implementation that comes with the PowerVR SDK.

namespace Carbon
{

struct RGBA8
{
    byte_t red = 0;
    byte_t green = 0;
    byte_t blue = 0;
    byte_t alpha = 0;

    RGBA8() {}
    RGBA8(byte_t r, byte_t g, byte_t b, byte_t a) : red(r), green(g), blue(b), alpha(a) {}

    Color toColor() const
    {
        return {Math::byteToFloat(red), Math::byteToFloat(green), Math::byteToFloat(blue), Math::byteToFloat(alpha)};
    }
};

struct Pixel128S
{
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 0;

    Pixel128S() {}
    Pixel128S(int red_, int green_, int blue_, int alpha_) : red(red_), green(green_), blue(blue_), alpha(alpha_) {}
    Pixel128S(const RGBA8& pixel) : red(pixel.red), green(pixel.green), blue(pixel.blue), alpha(pixel.alpha) {}

    Pixel128S& operator+=(const Pixel128S& p)
    {
        red += p.red;
        green += p.green;
        blue += p.blue;
        alpha += p.alpha;

        return *this;
    }

    Pixel128S& operator*=(int i)
    {
        red *= i;
        green *= i;
        blue *= i;
        alpha *= i;

        return *this;
    }

    Pixel128S operator-(const Pixel128S& p) const { return {red - p.red, green - p.green, blue - p.blue, alpha - p.alpha}; }

    operator RGBA8() const { return {byte_t(red), byte_t(green), byte_t(blue), byte_t(alpha)}; }
};

struct PVRTCBlock
{
    unsigned int modulationData = 0;
    unsigned int colorData = 0;
};

// Returns one of the two colors in a PVRTCBlock's color data.
static RGBA8 unpackColor(unsigned int colorData, unsigned int index)
{
    if (index == 0)
    {
        if (colorData & 0x8000)
        {
            // RGB554 => RGBA5554
            return {byte_t((colorData & 0x7C00) >> 10),                        // 5->5 bits
                    byte_t((colorData & 0x3E0) >> 5),                          // 5->5 bits
                    byte_t((colorData & 0x1E) | ((colorData & 0x1E) >> 4)),    // 4->5 bits
                    0x0F};                                                     // 0->4 bits
        }

        // ARGB 3443 => RGBA5554
        return {byte_t(((colorData & 0xF00) >> 7) | ((colorData & 0xF00) >> 11)),    // 4->5 bits
                byte_t(((colorData & 0xF0) >> 3) | ((colorData & 0xF0) >> 7)),       // 4->5 bits
                byte_t(((colorData & 0x0E) << 1) | ((colorData & 0x0E) >> 2)),       // 3->5 bits
                byte_t((colorData & 0x7000) >> 11)};                                 // 3->4 bits
    }

    if (colorData & 0x80000000)
    {
        // RGB 555 => RGBA5554
        return {byte_t((colorData & 0x7c000000) >> 26),    // 5->5 bits
                byte_t((colorData & 0x3e00000) >> 21),     // 5->5 bits
                byte_t((colorData & 0x1f0000) >> 16),      // 5->5 bits
                0x0F};                                     // 0->4 bits
    }

    // ARGB 3444 => RGBA5554
    return {byte_t(((colorData & 0xf000000) >> 23) | ((colorData & 0xf000000) >> 27)),    // 4->5 bits
            byte_t(((colorData & 0xf00000) >> 19) | ((colorData & 0xf00000) >> 23)),      // 4->5 bits
            byte_t(((colorData & 0xf0000) >> 15) | ((colorData & 0xf0000) >> 19)),        // 4->5 bits
            byte_t((colorData & 0x70000000) >> 27)};                                      // 3->4 bits
}

// Takes four low bit-rate color values for each PVRTCBlock and outputs a bilinear upscale from 2x2 pixels to 4x4/8x4 pixels
// depending on PVRTC bpp mode.
static void interpolateColors(const RGBA8& p, const RGBA8& q, const RGBA8& r, const RGBA8& s, Pixel128S* output,
                              unsigned int bpp)
{
    auto blockWidth = (bpp == 2 ? 8U : 4U);
    auto blockHeight = 4U;

    auto hP = Pixel128S(p);
    auto hQ = Pixel128S(q);
    auto hR = Pixel128S(r);
    auto hS = Pixel128S(s);

    // Get vectors
    auto qMinusP = hQ - hP;
    auto sMinusR = hS - hR;

    // Multiply colors
    hP *= blockWidth;
    hR *= blockWidth;

    if (bpp == 2)
    {
        for (auto x = 0U; x < blockWidth; x++)
        {
            auto result = Pixel128S(4 * hP.red, 4 * hP.green, 4 * hP.blue, 4 * hP.alpha);
            auto dY = Pixel128S(hR.red - hP.red, hR.green - hP.green, hR.blue - hP.blue, hR.alpha - hP.alpha);

            for (auto y = 0U; y < blockHeight; y++)
            {
                auto& pixel = output[y * blockWidth + x];

                pixel.red = byte_t((result.red >> 7) + (result.red >> 2));
                pixel.green = byte_t((result.green >> 7) + (result.green >> 2));
                pixel.blue = byte_t((result.blue >> 7) + (result.blue >> 2));
                pixel.alpha = byte_t((result.alpha >> 5) + (result.alpha >> 1));

                result += dY;
            }

            hP += qMinusP;
            hR += sMinusR;
        }
    }
    else
    {
        for (auto y = 0U; y < blockHeight; y++)
        {
            auto result = Pixel128S(4 * hP.red, 4 * hP.green, 4 * hP.blue, 4 * hP.alpha);
            auto dY = Pixel128S(hR.red - hP.red, hR.green - hP.green, hR.blue - hP.blue, hR.alpha - hP.alpha);

            for (auto x = 0U; x < blockWidth; x++)
            {
                auto& pixel = output[y * blockWidth + x];

                pixel.red = byte_t((result.red >> 6) + (result.red >> 1));
                pixel.green = byte_t((result.green >> 6) + (result.green >> 1));
                pixel.blue = byte_t((result.blue >> 6) + (result.blue >> 1));
                pixel.alpha = byte_t((result.alpha >> 4) + (result.alpha));

                result += dY;
            }

            hP += qMinusP;
            hR += sMinusR;
        }
    }
}

// Reads out and decodes the modulation values for the given PVRTCBlock, the results are put into modulationValues and
// modulationModes.
static void unpackModulations(const PVRTCBlock& block, int offsetX, int offsetY,
                              std::array<std::array<int, 8>, 16>& modulationValues,
                              std::array<std::array<int, 8>, 16>& modulationModes, unsigned int bpp)
{
    auto modulationData = block.modulationData;
    auto modulationMode = block.colorData & 1U;

    // Unpack differently depending on 2 or 4 bpp
    if (bpp == 2)
    {
        if (modulationMode)
        {
            // determine which of the three modes are in use:

            // If this is the either the H-only or V-only interpolation mode...
            if (modulationData & 1)
            {
                // look at the LSB for the centre texel, it is now used to indicate whether it's the horizontal only mode or
                // vertical only mode

                // The centre texel data is the at (x == 4, y == 2) and so its LSB is at bit 20
                if (modulationData & (1 << 20))
                    modulationMode = 3;    // This is the V-only mode
                else
                    modulationMode = 2;    // This is the H-only mode

                // Create an extra bit for the centre pixel so that it looks like we have 2 actual bits for this texel, this
                // makes later coding much easier
                if (modulationData & (1 << 21))
                    modulationData |= 1 << 20;    // Set it to produce code for 1.0
                else
                    modulationData &= ~(1 << 20);    // Clear it to produce 0.0 code
            }

            if (modulationData & 2)
                modulationData |= 1;    // Set it
            else
                modulationData &= ~1;    // Clear it

            // Run through all the pixels in the block. Note we can now treat all the "stored" values as if they have 2 bits
            // (even when they didn't!)
            for (auto y = 0; y < 4; y++)
            {
                for (auto x = 0; x < 8; x++)
                {
                    modulationModes[x + offsetX][y + offsetY] = modulationMode;

                    // If this is a stored value
                    if (((x ^ y) & 1) == 0)
                    {
                        modulationValues[x + offsetX][y + offsetY] = modulationData & 3;
                        modulationData >>= 2;
                    }
                }
            }
        }
        else    // Direct encoded 2 bit mode, i.e. 1 mode bit per pixel
        {
            for (auto y = 0; y < 4; y++)
            {
                for (auto x = 0; x < 8; x++)
                {
                    modulationModes[x + offsetX][y + offsetY] = modulationMode;

                    // Double the bits, so 0 => 00, and 1 => 11
                    modulationValues[x + offsetX][y + offsetY] = (modulationData & 1) ? 0x3 : 0x0;

                    modulationData >>= 1;
                }
            }
        }
    }
    else
    {
        if (modulationMode)
        {
            for (auto y = 0; y < 4; y++)
            {
                for (auto x = 0; x < 4; x++)
                {
                    modulationValues[y + offsetY][x + offsetX] = modulationData & 3;
                    if (modulationValues[y + offsetY][x + offsetX] == 1)
                        modulationValues[y + offsetY][x + offsetX] = 4;
                    else if (modulationValues[y + offsetY][x + offsetX] == 2)
                        modulationValues[y + offsetY][x + offsetX] = 14;    // +10 tells the decompressor to punch through alpha
                    else if (modulationValues[y + offsetY][x + offsetX] == 3)
                        modulationValues[y + offsetY][x + offsetX] = 8;

                    modulationData >>= 2;
                }
            }
        }
        else
        {
            for (auto y = 0; y < 4; y++)
            {
                for (auto x = 0; x < 4; x++)
                {
                    modulationValues[y + offsetY][x + offsetX] = modulationData & 3;
                    modulationValues[y + offsetY][x + offsetX] *= 3;
                    if (modulationValues[y + offsetY][x + offsetX] > 3)
                        modulationValues[y + offsetY][x + offsetX] -= 1;

                    modulationData >>= 2;
                }
            }
        }
    }
}

// Returns the effective modulation value for a given pixel.
static int getPixelModulationValue(const std::array<std::array<int, 8>, 16>& modulationValues,
                                   const std::array<std::array<int, 8>, 16>& modulationModes, unsigned int xPos,
                                   unsigned int yPos, unsigned int bpp)
{
    auto value = modulationValues[xPos][yPos];

    if (bpp == 2)
    {
        auto values = std::array<int, 4>{{0, 3, 5, 8}};

        // Extract the modulation value if a simple encoding
        if (modulationModes[xPos][yPos] == 0)
            value = values[value];
        else
        {
            // If this is a stored value then return it
            if (((xPos ^ yPos) & 1) == 0)
                value = values[value];
            else
            {
                // Otherwise average from the neighbors

                // Horizontal and vertical interpolation
                if (modulationModes[xPos][yPos] == 1)
                {
                    auto sum = 0;

                    sum += values[modulationValues[xPos][yPos - 1]] + values[modulationValues[xPos][yPos + 1]];
                    sum += values[modulationValues[xPos - 1][yPos]] + values[modulationValues[xPos + 1][yPos]];
                    sum += 2;

                    return sum / 4;
                }

                // Horizontal interpolation only
                if (modulationModes[xPos][yPos] == 2)
                    return (values[modulationValues[xPos - 1][yPos]] + values[modulationValues[xPos + 1][yPos]] + 1) / 2;

                // Vertical interpolation only
                return (values[modulationValues[xPos][yPos - 1]] + values[modulationValues[xPos][yPos + 1]] + 1) / 2;
            }
        }
    }

    return value;
}

// Takes the four blocks in the current decompression area and outputs final decompressed pixels.
static void getDecompressedPixels(PVRTCBlock p, PVRTCBlock q, PVRTCBlock r, PVRTCBlock s, RGBA8* output, unsigned int bpp)
{
#ifdef CARBON_BIG_ENDIAN
    Endian::convert(p.modulationData);
    Endian::convert(p.colorData);
    Endian::convert(q.modulationData);
    Endian::convert(q.colorData);
    Endian::convert(r.modulationData);
    Endian::convert(r.colorData);
    Endian::convert(s.modulationData);
    Endian::convert(s.colorData);
#endif

    // 4bpp only needs 8*8 values, but 2bpp needs 16*8
    auto modulationValues = std::array<std::array<int, 8>, 16>();
    auto modulationModes = std::array<std::array<int, 8>, 16>();

    // 4bpp only needs 16 values, but 2bpp needs 32
    auto upscaledColorA = std::array<Pixel128S, 32>();
    auto upscaledColorB = std::array<Pixel128S, 32>();

    auto blockWidth = (bpp == 2 ? 8U : 4U);
    auto blockHeight = 4U;

    // Get the modulations for each block
    unpackModulations(p, 0, 0, modulationValues, modulationModes, bpp);
    unpackModulations(q, blockWidth, 0, modulationValues, modulationModes, bpp);
    unpackModulations(r, 0, blockHeight, modulationValues, modulationModes, bpp);
    unpackModulations(s, blockWidth, blockHeight, modulationValues, modulationModes, bpp);

    // Bilinear upscale image data from 2x2 -> 4x4
    interpolateColors(unpackColor(p.colorData, 0), unpackColor(q.colorData, 0), unpackColor(r.colorData, 0),
                      unpackColor(s.colorData, 0), upscaledColorA.data(), bpp);
    interpolateColors(unpackColor(p.colorData, 1), unpackColor(q.colorData, 1), unpackColor(r.colorData, 1),
                      unpackColor(s.colorData, 1), upscaledColorB.data(), bpp);

    for (auto y = 0U; y < blockHeight; y++)
    {
        for (auto x = 0U; x < blockWidth; x++)
        {
            auto mod = getPixelModulationValue(modulationValues, modulationModes, x + blockWidth / 2, y + blockHeight / 2, bpp);
            auto punchthroughAlpha = mod > 10;
            if (punchthroughAlpha)
                mod -= 10;

            auto index = y * blockWidth + x;

            auto result = Pixel128S();

            result.red = (upscaledColorA[index].red * (8 - mod) + upscaledColorB[index].red * mod) / 8;
            result.green = (upscaledColorA[index].green * (8 - mod) + upscaledColorB[index].green * mod) / 8;
            result.blue = (upscaledColorA[index].blue * (8 - mod) + upscaledColorB[index].blue * mod) / 8;

            if (!punchthroughAlpha)
                result.alpha = (upscaledColorA[index].alpha * (8 - mod) + upscaledColorB[index].alpha * mod) / 8;

            // Convert the 32-bit precision result to 8-bit
            if (bpp == 2)
                output[y * blockWidth + x] = result;
            else if (bpp == 4)
                output[y + x * blockHeight] = result;
        }
    }
}

// Returns the twiddled offset of the specified pixel. Given the block (or pixel) coordinates and the dimension of the texture
// in blocks (or pixels) this returns the twiddled offset of the block (or pixel) from the start of the map. The dimensions of
// the texture must be a power of 2.
static unsigned int twiddleUV(unsigned int xSize, unsigned int ySize, unsigned int xPos, unsigned int yPos)
{
    // Initially assume X is the larger size
    auto minDimension = xSize;
    auto maxValue = yPos;
    auto twiddled = 0U;
    auto srcBitPos = 1U;
    auto dstBitPos = 1U;
    auto shiftCount = 0U;

    // If Y is the larger dimension - switch the min/max values.
    if (ySize < xSize)
    {
        minDimension = ySize;
        maxValue = xPos;
    }

    // Step through all the bits in the minimum dimension
    while (srcBitPos < minDimension)
    {
        if (yPos & srcBitPos)
            twiddled |= dstBitPos;

        if (xPos & srcBitPos)
            twiddled |= dstBitPos << 1;

        srcBitPos <<= 1;
        dstBitPos <<= 2;
        shiftCount++;
    }

    // Prepend any unused bits
    maxValue >>= shiftCount;
    twiddled |= maxValue << (2 * shiftCount);

    return twiddled;
}

// Wraps an index into the positive 0 => (size - 1) range.
static unsigned int wrapIndex(unsigned int size, int index)
{
    while (index < 0)
        index += size;

    return index % size;
}

// Takes a PVRTC image and decompresses a single pixel in it
static RGBA8 decompressSinglePVRTCPixel(const byte_t* compressedData, unsigned int width, unsigned int height, unsigned int bpp,
                                        unsigned int x, unsigned int y)
{
    auto blockWidth = (bpp == 2) ? 8U : 4U;
    auto blockHeight = 4U;

    auto blocks = reinterpret_cast<const PVRTCBlock*>(compressedData);

    // Calculate number of blocks
    auto xBlockCount = std::max(1U, width / blockWidth);
    auto yBlockCount = std::max(1U, height / blockHeight);

    auto pixels = Vector<RGBA8>(blockWidth * blockHeight);

    auto xBlock = int(x) - int(blockWidth / 2);
    auto yBlock = int(y) - int(blockHeight / 2);

    xBlock = xBlock < 0 ? -1 : (xBlock / blockWidth);
    yBlock = yBlock < 0 ? -1 : (yBlock / blockHeight);

    auto indices = std::array<unsigned int, 8>{{wrapIndex(xBlockCount, xBlock), wrapIndex(yBlockCount, yBlock),
                                                wrapIndex(xBlockCount, xBlock + 1), wrapIndex(yBlockCount, yBlock),
                                                wrapIndex(xBlockCount, xBlock), wrapIndex(yBlockCount, yBlock + 1),
                                                wrapIndex(xBlockCount, xBlock + 1), wrapIndex(yBlockCount, yBlock + 1)}};

    auto& p = blocks[twiddleUV(xBlockCount, yBlockCount, indices[0], indices[1])];
    auto& q = blocks[twiddleUV(xBlockCount, yBlockCount, indices[2], indices[3])];
    auto& r = blocks[twiddleUV(xBlockCount, yBlockCount, indices[4], indices[5])];
    auto& s = blocks[twiddleUV(xBlockCount, yBlockCount, indices[6], indices[7])];

    // Use the four blocks to compute decompressed pixels
    getDecompressedPixels(p, q, r, s, pixels.getData(), bpp);

    // Return the actual pixel we care about
    return pixels[wrapIndex(height, y - (yBlock * blockHeight + blockHeight / 2)) * blockWidth +
                  wrapIndex(width, x - (xBlock * blockWidth + blockWidth / 2))];
}

Color Image::readPVRTC2BitRGBPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z)
{
    return decompressSinglePVRTCPixel(data, width, height, 2, x, y).toColor();
}

Color Image::readPVRTC2BitRGBAPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                    unsigned int x, unsigned int y, unsigned int z)
{
    return decompressSinglePVRTCPixel(data, width, height, 2, x, y).toColor();
}

Color Image::readPVRTC4BitRGBPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z)
{
    return decompressSinglePVRTCPixel(data, width, height, 4, x, y).toColor();
}

Color Image::readPVRTC4BitRGBAPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                    unsigned int x, unsigned int y, unsigned int z)
{
    return decompressSinglePVRTCPixel(data, width, height, 4, x, y).toColor();
}

}
