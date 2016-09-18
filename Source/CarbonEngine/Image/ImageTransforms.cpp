/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

bool Image::flipHorizontal()
{
    if (!enumerateImage(rawFlipHorizontal))
    {
        LOG_ERROR << "Failed flipping image horizontally";
        return false;
    }

    return true;
}

bool Image::flipVertical()
{
    if (!enumerateImage(rawFlipVertical))
    {
        LOG_ERROR << "Failed flipping image vertically";
        return false;
    }

    return true;
}

bool Image::rotateCCW()
{
    if (!enumerateImage(rawRotateCCW))
    {
        LOG_ERROR << "Failed rotating image counter-clockwise";
        return false;
    }

    std::swap(width_, height_);

    return true;
}

bool Image::enumerateImage(
    const std::function<bool(unsigned int, unsigned int, unsigned int, Image::PixelFormat, byte_t*)>& fnImageOperation)
{
    if (!isValidImage())
        return false;

    for (auto source : getAllData())
    {
        auto w = getWidth();
        auto h = getHeight();
        auto d = getDepth();
        auto dataOffset = 0U;

        for (auto j = 0U; j < getMipmapCount(); j++)
        {
            if (!fnImageOperation(w, h, d, getPixelFormat(), source + dataOffset))
                return false;

            dataOffset += getImageDataSize(w, h, d, getPixelFormat(), false);

            getNextMipmapSize(w, h, d);
        }
    }

    return true;
}

bool Image::rawFlipHorizontal(unsigned int w, unsigned int h, unsigned int d, PixelFormat pf, byte_t* data)
{
    if (w == 1)
        return true;

    if (isPixelFormatUncompressed(pf))
    {
        auto pixelSize = getPixelFormatPixelSize(pf);
        auto pixel = Vector<byte_t>(pixelSize);

        auto offset = 0U;
        for (auto z = 0U; z < d; z++)
        {
            for (auto y = 0U; y < h; y++)
            {
                for (auto x = 0U; x < w / 2; x++)
                {
                    // Swap pixels
                    memcpy(pixel.getData(), &data[(offset + x) * pixelSize], pixelSize);
                    memcpy(&data[(offset + x) * pixelSize], &data[(offset + w - x - 1) * pixelSize], pixelSize);
                    memcpy(&data[(offset + w - x - 1) * pixelSize], pixel.getData(), pixelSize);
                }

                offset += w;
            }
        }
    }
    else
    {
        if (isPixelFormatDXTCompressed(pf))
        {
            // Flip all the blocks
            auto xBlockCount = (w + 3) / 4;
            auto yBlockCount = (h + 3) / 4;
            auto blockSize = getCompressedPixelFormatBlockSize(pf);
            auto block = std::array<byte_t, 16>();

            auto offset = 0U;
            for (auto y = 0U; y < yBlockCount; y++)
            {
                for (auto x = 0U; x < xBlockCount / 2; x++)
                {
                    // Swap blocks
                    memcpy(block.data(), &data[(offset + x) * blockSize], blockSize);
                    memcpy(&data[(offset + x) * blockSize], &data[(offset + xBlockCount - x - 1) * blockSize],
                           blockSize);
                    memcpy(&data[(offset + xBlockCount - x - 1) * blockSize], block.data(), blockSize);
                }

                offset += xBlockCount;
            }

            // Flip the data in the blocks
            auto ptr = data;
            auto totalBlockCount = d * xBlockCount * yBlockCount;
            for (auto i = 0U; i < totalBlockCount; i++)
            {
                if (w == 2)
                {
                    if (pf == DXT3)
                    {
                        for (auto j = 0U; j < 4; j++)
                        {
                            auto alpha = getDXT3Alpha(ptr, 0, j);
                            setDXT3Alpha(ptr, 0, j, getDXT3Alpha(ptr, 1, j));
                            setDXT3Alpha(ptr, 1, j, alpha);
                        }

                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        for (auto j = 0U; j < 4; j++)
                        {
                            auto alpha = getDXT5AlphaInterp(ptr, 0, j);
                            setDXT5AlphaInterp(ptr, 0, j, getDXT5AlphaInterp(ptr, 1, j));
                            setDXT5AlphaInterp(ptr, 1, j, alpha);
                        }

                        ptr += 8;
                    }

                    // RGB block
                    for (auto j = 0U; j < 4; j++)
                    {
                        auto alpha = getDXTRGBInterp(ptr, 0, j);
                        setDXTRGBInterp(ptr, 0, j, getDXTRGBInterp(ptr, 1, j));
                        setDXTRGBInterp(ptr, 1, j, alpha);
                    }

                    ptr += 8;
                }
                else
                {
                    if (pf == DXT3)
                    {
                        for (auto j = 0U; j < 4; j++)
                        {
                            auto alpha = getDXT3Alpha(ptr, 0, j);
                            setDXT3Alpha(ptr, 0, j, getDXT3Alpha(ptr, 3, j));
                            setDXT3Alpha(ptr, 3, j, alpha);

                            alpha = getDXT3Alpha(ptr, 1, j);
                            setDXT3Alpha(ptr, 1, j, getDXT3Alpha(ptr, 2, j));
                            setDXT3Alpha(ptr, 2, j, alpha);
                        }

                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        for (auto j = 0U; j < 4; j++)
                        {
                            auto alpha = getDXT5AlphaInterp(ptr, 0, j);
                            setDXT5AlphaInterp(ptr, 0, j, getDXT5AlphaInterp(ptr, 3, j));
                            setDXT5AlphaInterp(ptr, 3, j, alpha);

                            alpha = getDXT5AlphaInterp(ptr, 1, j);
                            setDXT5AlphaInterp(ptr, 1, j, getDXT5AlphaInterp(ptr, 2, j));
                            setDXT5AlphaInterp(ptr, 2, j, alpha);
                        }

                        ptr += 8;
                    }

                    // RGB block
                    for (auto j = 0U; j < 4; j++)
                    {
                        auto interp = getDXTRGBInterp(ptr, 0, j);
                        setDXTRGBInterp(ptr, 0, j, getDXTRGBInterp(ptr, 3, j));
                        setDXTRGBInterp(ptr, 3, j, interp);

                        interp = getDXTRGBInterp(ptr, 1, j);
                        setDXTRGBInterp(ptr, 1, j, getDXTRGBInterp(ptr, 2, j));
                        setDXTRGBInterp(ptr, 2, j, interp);
                    }

                    ptr += 8;
                }
            }
        }
        else
            return false;
    }

    return true;
}

bool Image::rawFlipVertical(unsigned int w, unsigned int h, unsigned int d, PixelFormat pf, byte_t* data)
{
    if (h == 1)
        return true;

    if (isPixelFormatUncompressed(pf))
    {
        auto row = Vector<byte_t>(getImageDataSize(w, 1, 1, pf));

        auto offset = 0U;
        for (auto z = 0U; z < d; z++)
        {
            for (auto j = 0U; j < h / 2; j++)
            {
                memcpy(row.getData(), &data[(offset + j) * row.size()], row.size());
                memcpy(&data[(offset + j) * row.size()], &data[(offset + h - j - 1) * row.size()], row.size());
                memcpy(&data[(offset + h - j - 1) * row.size()], row.getData(), row.size());
            }

            offset += h;
        }
    }
    else
    {
        if (isPixelFormatDXTCompressed(pf))
        {
            // Flip all the blocks
            auto xBlockCount = (w + 3) / 4;
            auto yBlockCount = (h + 3) / 4;
            auto blockSize = getCompressedPixelFormatBlockSize(pf);
            auto row = Vector<byte_t>(getImageDataSize(w, 1, 1, pf));

            for (auto y = 0U; y < yBlockCount / 2; y++)
            {
                memcpy(row.getData(), &data[(y * xBlockCount) * blockSize], row.size());
                memcpy(&data[(y * xBlockCount) * blockSize], &data[((yBlockCount - y - 1) * xBlockCount) * blockSize],
                       row.size());
                memcpy(&data[((yBlockCount - y - 1) * xBlockCount) * blockSize], row.getData(), row.size());
            }

            // Flip the data in the blocks
            auto ptr = data;
            auto totalBlockCount = d * xBlockCount * yBlockCount;
            for (auto i = 0U; i < totalBlockCount; i++)
            {
                if (h == 2)
                {
                    if (pf == DXT3)
                    {
                        std::swap(ptr[0], ptr[2]);
                        std::swap(ptr[1], ptr[3]);
                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        for (auto x = 0U; x < 4; x++)
                        {
                            auto interp = getDXT5AlphaInterp(ptr, x, 0);
                            setDXT5AlphaInterp(ptr, x, 0, getDXT5AlphaInterp(ptr, x, 1));
                            setDXT5AlphaInterp(ptr, x, 1, interp);
                        }
                        ptr += 8;
                    }

                    // RGB block
                    std::swap(ptr[4], ptr[5]);
                    ptr += 8;
                }
                else
                {
                    if (pf == DXT3)
                    {
                        std::swap(ptr[0], ptr[6]);
                        std::swap(ptr[1], ptr[7]);
                        std::swap(ptr[2], ptr[4]);
                        std::swap(ptr[3], ptr[5]);
                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        // Take a copy of the original DXT5 alpha values for each row, shifted appropriately ready for
                        // the vertical flip
                        auto alpha0 = ptr[3] >> 4 | ptr[4] << 4;
                        auto alpha1 = ptr[2] << 12 | ptr[3] << 20;
                        auto alpha2 = ptr[6] >> 4 | ptr[7] << 4;
                        auto alpha3 = ptr[5] << 12 | ptr[6] << 20;

                        // Flip the alpha values for rows 0 and 1
                        auto flipped01 = 0U;
                        flipped01 |= (alpha0 & 0x000007) | (alpha1 & 0x007000) | (alpha0 & 0x000038);
                        flipped01 |= (alpha1 & 0x038000) | (alpha0 & 0x0001C0) | (alpha1 & 0x1C0000);
                        flipped01 |= (alpha0 & 0x000E00) | (alpha1 & 0xE00000);

                        // Flip the alpha values for rows 2 and 3
                        auto flipped23 = 0U;
                        flipped23 |= (alpha2 & 0x000007) | (alpha3 & 0x007000) | (alpha2 & 0x000038);
                        flipped23 |= (alpha3 & 0x038000) | (alpha2 & 0x0001C0) | (alpha3 & 0x1C0000);
                        flipped23 |= (alpha2 & 0x000E00) | (alpha3 & 0xE00000);

                        // Write the flipped rows over the original rows
                        ptr[2] = flipped23 & 0xFF;
                        ptr[3] = flipped23 >> 8 & 0xFF;
                        ptr[4] = flipped23 >> 16 & 0xFF;
                        ptr[5] = flipped01 & 0xFF;
                        ptr[6] = flipped01 >> 8 & 0xFF;
                        ptr[7] = flipped01 >> 16 & 0xFF;

                        ptr += 8;
                    }

                    // RGB block
                    std::swap(ptr[4], ptr[7]);
                    std::swap(ptr[5], ptr[6]);
                    ptr += 8;
                }
            }
        }
        else
            return false;
    }

    return true;
}

bool Image::rawRotateCCW(unsigned int w, unsigned int h, unsigned int d, PixelFormat pf, byte_t* data)
{
    if (w == 1 && h == 1)
        return true;

    if (isPixelFormatUncompressed(pf))
    {
        auto pixelSize = getPixelFormatPixelSize(pf);
        auto newData = Vector<byte_t>(getImageDataSize(w, h, d, pf));

        auto offset = 0U;
        for (auto z = 0U; z < d; z++)
        {
            for (auto y = 0U; y < h; y++)
            {
                for (auto x = 0U; x < w; x++)
                {
                    memcpy(&newData[(offset + x * h + h - y - 1) * pixelSize], &data[(offset + y * w + x) * pixelSize],
                           pixelSize);
                }
            }
            offset += w * h;
        }

        memcpy(data, newData.getData(), newData.getDataSize());
    }
    else
    {
        if (isPixelFormatDXTCompressed(pf))
        {
            // Rotate all the blocks
            auto xBlockCount = (w + 3) / 4;
            auto yBlockCount = (h + 3) / 4;
            auto blockSize = getCompressedPixelFormatBlockSize(pf);

            auto newData = Vector<byte_t>(getImageDataSize(w, h, d, pf));

            for (auto y = 0U; y < yBlockCount; y++)
            {
                for (auto x = 0U; x < xBlockCount; x++)
                {
                    memcpy(&newData[(x * xBlockCount + yBlockCount - y - 1) * blockSize],
                           &data[(y * xBlockCount + x) * blockSize], blockSize);
                }
            }

            memcpy(data, newData.getData(), newData.getDataSize());

            std::swap(w, h);

            // Rotate the data in the blocks
            auto ptr = data;
            auto totalBlockCount = d * xBlockCount * yBlockCount;
            for (auto i = 0U; i < totalBlockCount; i++)
            {
                // A number of situations involving blocks with dimensions less than 4 need to be special cased, though
                // a couple still work with the standard block rotation code.

                if (w == 2 && h == 1)
                {
                    if (pf == DXT3)
                    {
                        setDXT3Alpha(ptr, 0, 1, getDXT3Alpha(ptr, 1, 0));
                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        setDXT5AlphaInterp(ptr, 0, 1, getDXT5AlphaInterp(ptr, 1, 0));
                        ptr += 8;
                    }

                    setDXTRGBInterp(ptr, 0, 1, getDXTRGBInterp(ptr, 1, 0));
                    ptr += 8;
                }
                else if (w == 4 && h == 1)
                {
                    if (pf == DXT3)
                    {
                        setDXT3Alpha(ptr, 0, 1, getDXT3Alpha(ptr, 1, 0));
                        setDXT3Alpha(ptr, 0, 2, getDXT3Alpha(ptr, 2, 0));
                        setDXT3Alpha(ptr, 0, 3, getDXT3Alpha(ptr, 3, 0));
                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        setDXT5AlphaInterp(ptr, 0, 1, getDXT5AlphaInterp(ptr, 1, 0));
                        setDXT5AlphaInterp(ptr, 0, 2, getDXT5AlphaInterp(ptr, 2, 0));
                        setDXT5AlphaInterp(ptr, 0, 3, getDXT5AlphaInterp(ptr, 3, 0));
                        ptr += 8;
                    }

                    setDXTRGBInterp(ptr, 0, 1, getDXTRGBInterp(ptr, 1, 0));
                    setDXTRGBInterp(ptr, 0, 2, getDXTRGBInterp(ptr, 2, 0));
                    setDXTRGBInterp(ptr, 0, 3, getDXTRGBInterp(ptr, 3, 0));
                    ptr += 8;
                }
                else if (w == 1 && h == 2)
                {
                    if (pf == DXT3)
                    {
                        setDXT3Alpha(ptr, 1, 0, getDXT3Alpha(ptr, 0, 1));
                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        setDXT5AlphaInterp(ptr, 1, 0, getDXT5AlphaInterp(ptr, 0, 1));
                        ptr += 8;
                    }

                    setDXTRGBInterp(ptr, 1, 0, getDXTRGBInterp(ptr, 0, 1));
                    ptr += 8;
                }
                else if ((w == 2 || w == 4) && h == 2)
                {
                    if (pf == DXT3)
                    {
                        auto alpha = getDXT3Alpha(ptr, 0, 0);
                        setDXT3Alpha(ptr, 0, 0, getDXT3Alpha(ptr, 0, 1));
                        setDXT3Alpha(ptr, 0, 1, getDXT3Alpha(ptr, 1, 1));
                        setDXT3Alpha(ptr, 1, 1, getDXT3Alpha(ptr, 1, 0));
                        setDXT3Alpha(ptr, 1, 0, alpha);
                        setDXT3Alpha(ptr, 0, 2, getDXT3Alpha(ptr, 2, 1));
                        setDXT3Alpha(ptr, 0, 3, getDXT3Alpha(ptr, 3, 1));
                        setDXT3Alpha(ptr, 1, 3, getDXT3Alpha(ptr, 3, 0));
                        setDXT3Alpha(ptr, 1, 2, getDXT3Alpha(ptr, 2, 0));
                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        auto interp = getDXT5AlphaInterp(ptr, 0, 0);
                        setDXT5AlphaInterp(ptr, 0, 0, getDXT5AlphaInterp(ptr, 0, 1));
                        setDXT5AlphaInterp(ptr, 0, 1, getDXT5AlphaInterp(ptr, 1, 1));
                        setDXT5AlphaInterp(ptr, 1, 1, getDXT5AlphaInterp(ptr, 1, 0));
                        setDXT5AlphaInterp(ptr, 1, 0, interp);
                        setDXT5AlphaInterp(ptr, 0, 2, getDXT5AlphaInterp(ptr, 2, 1));
                        setDXT5AlphaInterp(ptr, 0, 3, getDXT5AlphaInterp(ptr, 3, 1));
                        setDXT5AlphaInterp(ptr, 1, 3, getDXT5AlphaInterp(ptr, 3, 0));
                        setDXT5AlphaInterp(ptr, 1, 2, getDXT5AlphaInterp(ptr, 2, 0));
                        ptr += 8;
                    }

                    // RGB
                    auto interp = getDXTRGBInterp(ptr, 0, 0);
                    setDXTRGBInterp(ptr, 0, 0, getDXTRGBInterp(ptr, 0, 1));
                    setDXTRGBInterp(ptr, 0, 1, getDXTRGBInterp(ptr, 1, 1));
                    setDXTRGBInterp(ptr, 1, 1, getDXTRGBInterp(ptr, 1, 0));
                    setDXTRGBInterp(ptr, 1, 0, interp);
                    setDXTRGBInterp(ptr, 0, 2, getDXTRGBInterp(ptr, 2, 1));
                    setDXTRGBInterp(ptr, 0, 3, getDXTRGBInterp(ptr, 3, 1));
                    setDXTRGBInterp(ptr, 1, 3, getDXTRGBInterp(ptr, 3, 0));
                    setDXTRGBInterp(ptr, 1, 2, getDXTRGBInterp(ptr, 2, 0));
                    ptr += 8;
                }
                else
                {
                    // Rotate the whole block

                    if (pf == DXT3)
                    {
                        // Unpack the 4 bit alpha values
                        auto alphas = std::array<byte_t, 16>();
                        for (auto j = 0U; j < 16; j++)
                            alphas[j] = getDXT3Alpha(ptr, j % 4, j / 4);

                        // Rotate and recombine
                        for (auto y = 0U; y < 4; y++)
                        {
                            for (auto x = 0U; x < 4; x++)
                                setDXT3Alpha(ptr, x, y, alphas[12 - x * 4 + y]);
                        }

                        ptr += 8;
                    }
                    else if (pf == DXT5)
                    {
                        // Unpack the 3 bit alpha interp values
                        auto interps = std::array<byte_t, 16>();
                        for (auto j = 0U; j < 16; j++)
                            interps[j] = getDXT5AlphaInterp(ptr, j % 4, j / 4);

                        // Rotate and recombine
                        for (auto y = 0U; y < 4; y++)
                        {
                            for (auto x = 0U; x < 4; x++)
                                setDXT5AlphaInterp(ptr, x, y, interps[12 - x * 4 + y]);
                        }

                        ptr += 8;
                    }

                    // Unpack the 2 bit rgb interp values
                    auto interps = std::array<byte_t, 16>();
                    for (auto j = 0U; j < 16; j++)
                        interps[j] = getDXTRGBInterp(ptr, j % 4, j / 4);

                    // Rotate and recombine
                    for (auto y = 0U; y < 4; y++)
                    {
                        for (auto x = 0U; x < 4; x++)
                            setDXTRGBInterp(ptr, x, y, interps[12 - x * 4 + y]);
                    }

                    ptr += 8;
                }
            }
        }
        else
            return false;
    }

    return true;
}

}
