/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

/**
 * Adds support for loading PVR image files, both the legacy version 2 and the newer version 3 formats are supported.
 */
class PVR
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        try
        {
            image.clear();

            auto pixelFormat = Image::UnknownPixelFormat;
            auto height = uint32_t();
            auto width = uint32_t();
            auto depth = uint32_t();
            auto flags = uint32_t();
            auto surfaceCount = uint32_t();
            auto mipmapCount = uint32_t();
            auto isPremultiplied = false;
            auto isCubemap = false;

            // Check whether this is a PVR version 3 image file
            if (file.readFourCC() == FileSystem::makeFourCC("PVR\03"))
            {
                auto rawPixelFormat = std::array<unsigned int, 2>();
                auto colorSpace = uint32_t();
                auto channelType = uint32_t();
                auto metaDataSize = uint32_t();
                auto faceCount = uint32_t();

                file.read(flags, rawPixelFormat[0], rawPixelFormat[1], colorSpace, channelType, height, width, depth);
                file.read(surfaceCount, faceCount, mipmapCount, metaDataSize);

                isPremultiplied = (flags & 0x02) != 0;

                if (rawPixelFormat[1])
                {
                    // This is an uncompressed pixel format

                    if (rawPixelFormat[0] == FileSystem::makeFourCC("rgb\0"))
                    {
                        if (rawPixelFormat[1] == 0x00080808 && channelType == 0)
                            pixelFormat = Image::RGB8;
                        else if (rawPixelFormat[1] == 0x00050605 && channelType == 4)
                            pixelFormat = Image::RGB565;
                        else if (rawPixelFormat[1] == 0x00202020 && channelType == 12)
                            pixelFormat = Image::RGB32f;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("bgr\0"))
                    {
                        if (rawPixelFormat[1] == 0x00080808 && channelType == 0)
                            pixelFormat = Image::BGR8;
                        else if (rawPixelFormat[1] == 0x00050605 && channelType == 4)
                            pixelFormat = Image::BGR565;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("rgba"))
                    {
                        if (rawPixelFormat[1] == 0x08080808 && channelType == 0)
                            pixelFormat = Image::RGBA8;
                        else if (rawPixelFormat[1] == 0x01050505 && channelType == 4)
                            pixelFormat = Image::RGBA5551;
                        else if (rawPixelFormat[1] == 0x04040404 && channelType == 4)
                            pixelFormat = Image::RGBA4444;
                        else if (rawPixelFormat[1] == 0x20202020 && channelType == 12)
                            pixelFormat = Image::RGBA32f;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("bgra"))
                    {
                        if (rawPixelFormat[1] == 0x08080808 && channelType == 0)
                            pixelFormat = Image::BGRA8;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("argb"))
                    {
                        if (rawPixelFormat[1] == 0x05050501 && channelType == 4)
                            pixelFormat = Image::ARGB1555;
                        else if (rawPixelFormat[1] == 0x04040404 && channelType == 4)
                            pixelFormat = Image::ARGB4444;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("abgr"))
                    {
                        if (rawPixelFormat[1] == 0x08080808 && channelType == 0)
                            pixelFormat = Image::ABGR8;
                        else if (rawPixelFormat[1] == 0x04040404 && channelType == 4)
                            pixelFormat = Image::ABGR4444;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("a\0\0\0"))
                    {
                        if (rawPixelFormat[1] == 0x00000008 && channelType == 0)
                            pixelFormat = Image::Alpha8;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("l\0\0\0"))
                    {
                        if (rawPixelFormat[1] == 0x00000008 && channelType == 0)
                            pixelFormat = Image::Luminance8;
                    }
                    else if (rawPixelFormat[0] == FileSystem::makeFourCC("la\0\0"))
                    {
                        if (rawPixelFormat[1] == 0x00000808 && channelType == 0)
                            pixelFormat = Image::LuminanceAlpha8;
                    }
                }
                else
                {
                    // This is a compressed pixel format
                    switch (rawPixelFormat[0])
                    {
                        case 0:
                            pixelFormat = Image::PVRTC2BitRGB;
                            break;
                        case 1:
                            pixelFormat = Image::PVRTC2BitRGBA;
                            break;
                        case 2:
                            pixelFormat = Image::PVRTC4BitRGB;
                            break;
                        case 3:
                            pixelFormat = Image::PVRTC4BitRGBA;
                            break;
                        case 7:
                            pixelFormat = Image::DXT1;
                            break;
                        case 8:
                            pixelFormat = Image::DXT3;
                            break;
                        case 9:
                            pixelFormat = Image::DXT5;
                            break;
                    }
                }

                if (pixelFormat == Image::UnknownPixelFormat)
                    throw Exception() << "Unsupported pixel format, details: " << String::toHex(rawPixelFormat[0])
                                      << ":" << String::toHex(rawPixelFormat[1]) << " with channel type "
                                      << channelType;

                if (faceCount == 6)
                    isCubemap = true;
                else if (faceCount != 1)
                    throw Exception() << "A face count of " << faceCount << " is not supported";

                // Skip past any metadata
                file.skip(metaDataSize);
            }
            else
            {
                // Check whether this is a legacy PVR version 2 image file
                file.setPosition(44);
                if (file.readFourCC() == FileSystem::makeFourCC("PVR!"))
                {
                    file.setPosition(0);

                    auto headerLength = uint32_t();
                    auto dataLength = uint32_t();
                    auto bpp = uint32_t();
                    auto bitmasks = std::array<uint32_t, 4>();
                    auto unused = uint32_t();

                    file.read(headerLength, height, width, mipmapCount, flags, dataLength, bpp);
                    file.read(bitmasks[0], bitmasks[1], bitmasks[2], bitmasks[3], unused, surfaceCount);

                    const auto textureFlagTypeMask = 255U;
                    const auto textureFlagTypePVRTC2Bit = 24U;
                    const auto textureFlagTypePVRTC4Bit = 25U;

                    // Work out the pixel format
                    if ((flags & textureFlagTypeMask) == textureFlagTypePVRTC4Bit)
                        pixelFormat = bitmasks[3] ? Image::PVRTC4BitRGBA : Image::PVRTC4BitRGB;
                    else if ((flags & textureFlagTypeMask) == textureFlagTypePVRTC2Bit)
                        pixelFormat = bitmasks[3] ? Image::PVRTC2BitRGBA : Image::PVRTC2BitRGB;

                    // Skip past the header
                    file.setPosition(headerLength);
                }
                else
                    throw Exception("Invalid file");
            }

            if (!surfaceCount)
                throw Exception("Image contains no surfaces");

            if (pixelFormat == Image::UnknownPixelFormat)
                throw Exception("Unsupported pixel format");

            // Check mipmap count makes sense
            if (mipmapCount > 1 && mipmapCount != Image::getImageMipmapCount(width, height, depth))
                throw Exception() << "Incorrect mipmap count: " << mipmapCount;

            // Setup image description
            if (isCubemap)
            {
                if (!image.initializeCubemap(width, pixelFormat, mipmapCount > 1, surfaceCount))
                    throw Exception("Failed initializing image");
            }
            else
            {
                if (!image.initialize(width, height, depth, pixelFormat, mipmapCount > 1, surfaceCount))
                    throw Exception("Failed initializing image");
            }

            // Read all the PVRTC image data
            auto w = width;
            auto h = height;
            auto d = depth;
            auto offset = 0U;
            for (auto i = 0U; i < mipmapCount; i++)
            {
                auto mipmapSize = Image::getImageDataSize(w, h, d, pixelFormat);

                for (auto j = 0U; j < surfaceCount; j++)
                {
                    if (isCubemap)
                    {
                        for (auto k = 0U; k < 6; k++)
                            file.readBytes(image.getCubemapDataForFrame(j, k) + offset, mipmapSize);
                    }
                    else
                        file.readBytes(image.getDataForFrame(j) + offset, mipmapSize);
                }

                Image::getNextMipmapSize(w, h, d);
                offset += mipmapSize;
            }

            // Warn about potential troubles rendering images with premultiplied alpha
            if (isPremultiplied)
                LOG_WARNING << "Premultiplied alpha is not supported, blending may render incorrectly: "
                            << file.getName();

            if (!file.isEOF())
                LOG_WARNING << file.getName() << " - PVR load was successful, but not all data in the file was read";

            if (!Image::isPixelFormatPVRTCCompressed(image.getPixelFormat()) && !image.flipVertical())
                LOG_WARNING << file.getName() << " - Failed flipping image vertically";

            return true;
        }
        catch (const Exception& e)
        {
            image.clear();

            LOG_ERROR << file.getName() << " - " << e;

            return false;
        }
    }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(pvr, PVR::load, nullptr)

}
