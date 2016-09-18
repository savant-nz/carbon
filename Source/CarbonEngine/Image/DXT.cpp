/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Image/Image.h"

namespace Carbon
{

void Image::decompressDXTCBlock(const byte_t* blockData, std::array<std::array<byte_t, 4>, 16>& output,
                                PixelFormat pixelFormat)
{
    // Decompress alpha block if present
    if (pixelFormat == DXT3)
    {
        auto alphas = std::array<byte_t, 16>();
        decompressDXT3Alpha(blockData, alphas);

        for (auto i = 0U; i < 16; i++)
            output[i][3] = alphas[i];

        blockData += 8;
    }
    else if (pixelFormat == DXT5)
    {
        auto alphas = std::array<byte_t, 16>();
        decompressDXT5Alpha(blockData, alphas);

        for (auto i = 0U; i < 16; i++)
            output[i][3] = alphas[i];

        blockData += 8;
    }

    // RGB8 colors for this color block.
    auto colors = std::array<std::array<byte_t, 3>, 4>();

    colors[0][0] = rgb565GetR(blockData);
    colors[0][1] = rgb565GetG(blockData);
    colors[0][2] = rgb565GetB(blockData);
    colors[1][0] = rgb565GetR(blockData + 2);
    colors[1][1] = rgb565GetG(blockData + 2);
    colors[1][2] = rgb565GetB(blockData + 2);

    auto hasDXT1Alpha = false;
    if (pixelFormat == DXT1)
    {
        // Look at the color values to see whether this DXT1 block is using a 1-bit alpha
        auto c0 = *reinterpret_cast<const uint16_t*>(blockData);
        auto c1 = *reinterpret_cast<const uint16_t*>(blockData + 2);

#ifdef CARBON_BIG_ENDIAN
        Endian::convert(c0);
        Endian::convert(c1);
#endif

        hasDXT1Alpha = (c0 <= c1);
    }

    if (!hasDXT1Alpha)
    {
        // Standard DXTC color block
        colors[2][0] = (2 * colors[0][0] + colors[1][0] + 1) / 3;
        colors[2][1] = (2 * colors[0][1] + colors[1][1] + 1) / 3;
        colors[2][2] = (2 * colors[0][2] + colors[1][2] + 1) / 3;
        colors[3][0] = (colors[0][0] + 2 * colors[1][0] + 1) / 3;
        colors[3][1] = (colors[0][1] + 2 * colors[1][1] + 1) / 3;
        colors[3][2] = (colors[0][2] + 2 * colors[1][2] + 1) / 3;
    }
    else
    {
        // 1 bit DXT1 alpha so colors[3] actually means RGBA = (0, 0, 0, 0)
        colors[2][0] = (colors[0][0] + colors[1][0]) / 2;
        colors[2][1] = (colors[0][1] + colors[1][1]) / 2;
        colors[2][2] = (colors[0][2] + colors[1][2]) / 2;
        colors[3][0] = 0;
        colors[3][1] = 0;
        colors[3][2] = 0;
    }

    for (auto i = 0U; i < 16; i++)
    {
        auto interp = getDXTRGBInterp(blockData, i % 4, i / 4);

        output[i][0] = colors[interp][0];
        output[i][1] = colors[interp][1];
        output[i][2] = colors[interp][2];

        if (pixelFormat == DXT1)
            output[i][3] = (hasDXT1Alpha && interp == 3) ? 0 : 0xFF;
    }
}

void Image::decompressDXT3Alpha(const byte_t* alphaData, std::array<byte_t, 16>& output)
{
    // 4 bit alpha per pixel
    for (auto y = 0U; y < 4; y++)
    {
        for (auto x = 0U; x < 4; x++)
        {
            output[y * 4 + x] = getDXT3Alpha(alphaData, x, y);
            output[y * 4 + x] |= output[y * 4 + x] << 4;
        }
    }
}

void Image::decompressDXT5Alpha(const byte_t* alphaData, std::array<byte_t, 16>& output)
{
    // 3 bit interpolated alpha per pixel

    auto alphas = std::array<byte_t, 8>();

    alphas[0] = *reinterpret_cast<const byte_t*>(alphaData);
    alphas[1] = *reinterpret_cast<const byte_t*>(alphaData + 1);

    // Interpolated alpha colors
    if (alphas[0] > alphas[1])
    {
        alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;    // Bit code 010
        alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;    // Bit code 011
        alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;    // Bit code 100
        alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;    // Bit code 101
        alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;    // Bit code 110
        alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;    // Bit code 111
    }
    else
    {
        alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;    // Bit code 010
        alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;    // Bit code 011
        alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;    // Bit code 100
        alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;    // Bit code 101
        alphas[6] = 0;                                          // Bit code 110
        alphas[7] = 0xFF;                                       // Bit code 111
    }

    for (auto y = 0U; y < 4; y++)
    {
        for (auto x = 0U; x < 4; x++)
            output[y * 4 + x] = alphas[getDXT5AlphaInterp(alphaData, x, y)];
    }
}

byte_t Image::getDXTRGBInterp(const byte_t* rgbData, unsigned int x, unsigned int y)
{
    return byte_t((rgbData[4 + y] >> (x * 2)) & 3);
}

void Image::setDXTRGBInterp(byte_t* rgbData, unsigned int x, unsigned int y, byte_t interp)
{
    rgbData[4 + y] &= ~(3 << (x * 2));
    rgbData[4 + y] |= (interp & 3) << (x * 2);
}

byte_t Image::getDXT3Alpha(const byte_t* alphaData, unsigned int x, unsigned int y)
{
    if (x == 0)
        return alphaData[y * 2] & 0xF;

    if (x == 1)
        return (alphaData[y * 2] >> 4) & 0xF;

    if (x == 2)
        return (alphaData[y * 2 + 1] >> 0) & 0xF;

    if (x == 3)
        return (alphaData[y * 2 + 1] >> 4) & 0xF;

    return 0;
}

void Image::setDXT3Alpha(byte_t* alphaData, unsigned int x, unsigned int y, byte_t alpha)
{
    alpha &= 0xF;

    if (x == 0)
        alphaData[y * 2] = (alphaData[y * 2] & 0xF0) | alpha;

    if (x == 1)
        alphaData[y * 2] = (alphaData[y * 2] & 0x0F) | byte_t(alpha << 4);

    if (x == 2)
        alphaData[y * 2 + 1] = (alphaData[y * 2 + 1] & 0xF0) | alpha;

    if (x == 3)
        alphaData[y * 2 + 1] = (alphaData[y * 2 + 1] & 0x0F) | byte_t(alpha << 4);
}

byte_t Image::getDXT5AlphaInterp(const byte_t* alphaData, unsigned int x, unsigned int y)
{
    alphaData += y < 2 ? 2 : 5;

    auto r = alphaData[0] | (alphaData[1] << 8) | (alphaData[2] << 16);
    auto offset = (y % 2 * 4 + x) * 3;

    return byte_t((r >> offset) & 7);
}

void Image::setDXT5AlphaInterp(byte_t* alphaData, unsigned int x, unsigned int y, byte_t interp)
{
    alphaData += y < 2 ? 2 : 5;

    auto r = alphaData[0] | (alphaData[1] << 8) | (alphaData[2] << 16);
    auto offset = (y % 2 * 4 + x) * 3;

    r &= ~(7 << offset);
    r |= (interp & 7) << offset;

    alphaData[0] = r & 0xFF;
    alphaData[1] = (r >> 8) & 0xFF;
    alphaData[2] = (r >> 16) & 0xFF;
}

}
