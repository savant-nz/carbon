/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

/**
 * Loads a VTF (Valve Texture Format) file. The texture thumbnail is ignored. Animations, cubemaps, mipmaps, and all common
 * pixel formats are supported.
 */
class VTF
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        try
        {
            image.clear();

            if (imageIndex)
                LOG_WARNING << "Image indexing not supported for this format";

            // Read file ID, versions, and header size
            auto majorVersion = uint32_t();
            auto minorVersion = uint32_t();
            auto headerSize = uint32_t();
            if (file.readFourCC() != FileSystem::makeFourCC("VTF\0"))
                throw Exception("Not a valid VTF file");
            file.read(majorVersion, minorVersion, headerSize);
            if (majorVersion != 7 || minorVersion > 2)
                throw Exception("Unsupported file version, only VTF 7.0 to 7.2 is supported");
            if (headerSize != 64)
                throw Exception() << "Incorrect header size: " << headerSize;

            auto width = uint16_t();
            auto height = uint16_t();
            auto frameCount = uint16_t();
            auto flags = uint32_t();
            auto format = uint32_t();
            file.read(width, height, flags, frameCount);
            file.skip(26);    // Skip unknown data
            file.read(format);

            // Convert to native pixel format
            auto pixelFormat = Image::UnknownPixelFormat;
            if (format == 0)
                pixelFormat = Image::RGBA8;
            else if (format == 1)
                pixelFormat = Image::ABGR8;
            else if (format == 2)
                pixelFormat = Image::RGB8;
            else if (format == 3)
                pixelFormat = Image::BGR8;
            else if (format == 5)
                pixelFormat = Image::Luminance8;
            else if (format == 6)
                pixelFormat = Image::LuminanceAlpha8;
            else if (format == 8)
                pixelFormat = Image::Alpha8;
            else if (format == 12)
                pixelFormat = Image::BGRA8;
            else if (format == 13)
                pixelFormat = Image::DXT1;
            else if (format == 14)
                pixelFormat = Image::DXT3;
            else if (format == 15)
                pixelFormat = Image::DXT5;
            else if (format == 20)
                pixelFormat = Image::DXT1;
            else if (format == 24)
                pixelFormat = Image::RGBA16f;
            else
                throw Exception() << "Failed converting format: " << format;

            // Read mipmap count and check the mipmap chain is complete
            auto mipmapCount = uint8_t();
            file.read(mipmapCount);
            if (mipmapCount > 1 && mipmapCount != Image::getImageMipmapCount(width, height, 1))
                throw Exception("Incomplete mipmap chain");

            // Read the thumbnail definition
            auto thumbnailFormat = uint32_t();
            auto thumbnailWidth = uint8_t();
            auto thumbnailHeight = uint8_t();
            file.read(thumbnailFormat, thumbnailWidth, thumbnailHeight);
            file.skip(1);

            // Skip thumbnail data
            if (thumbnailWidth && thumbnailHeight)
                file.skip(Image::getImageDataSize(thumbnailWidth, thumbnailHeight, 1, Image::DXT1));

            // Initialize the image
            if (flags & 0x4000)
            {
                if (!image.initializeCubemap(width, pixelFormat, mipmapCount > 1, frameCount))
                    throw Exception("Failed initializing image");
            }
            else
            {
                if (!image.initialize(width, height, 1, pixelFormat, mipmapCount > 1, frameCount))
                    throw Exception("Failed initializing image");
            }

            // The VTF file stores the mipmaps from smallest to largest, but we need them from largest to smallest in the output
            // image. Animation frames are stored consecutively at each mipmap level. For cubemap images, each mipmap level for
            // each face is stored consecutively. The overall VTF on-disk layout is therefore:
            //
            //      Loop through mipmap levels (smallest to largest)
            //          Loop through animation frames
            //              Loop through cubemap faces (if reading a cubemap)
            //                  <read face pixel data>

            for (auto i = 0U; i < mipmapCount; i++)
            {
                auto mipmapWidth = image.getWidth();
                auto mipmapHeight = image.getHeight();

                // Because VTFs store their mipmaps from smallest to largest we need to work out where to read the next chunk of
                // data to, as the Image class expects them from largest to smallest
                auto offset = 0U;
                auto trueMipmapLevel = mipmapCount - i - 1;
                while (trueMipmapLevel--)
                {
                    offset += Image::getImageDataSize(mipmapWidth, mipmapHeight, 1, image.getPixelFormat());
                    Image::getNextMipmapSize(mipmapWidth, mipmapHeight);
                }

                for (auto j = 0U; j < image.getFrameCount(); j++)
                {
                    if (image.isCubemap())
                    {
                        for (auto k = 0U; k < 6; k++)
                        {
                            // Swap y and z faces
                            static const auto faceMapping = std::array<unsigned int, 6>{{0, 1, 4, 5, 2, 3}};

                            file.readBytes(&image.getCubemapDataForFrame(j, faceMapping[k])[offset],
                                           Image::getImageDataSize(mipmapWidth, mipmapHeight, 1, image.getPixelFormat()));
                        }
                    }
                    else
                    {
                        file.readBytes(&image.getDataForFrame(j)[offset],
                                       Image::getImageDataSize(mipmapWidth, mipmapHeight, 1, image.getPixelFormat()));
                    }
                }
            }

            // Cubemaps need further transforms of their image data in order to line up correctly
            if (image.isCubemap())
            {
                for (auto i = 0U; i < image.getFrameCount(); i++)
                {
                    for (auto j = 0U; j < 6; j++)
                    {
                        auto data = image.getCubemapDataForFrame(i, j);

                        if (j == 0)
                        {
                            Image::rawRotateCCW(image.getWidth(), image.getHeight(), 1, image.getPixelFormat(), data);
                            Image::rawFlipHorizontal(image.getWidth(), image.getHeight(), 1, image.getPixelFormat(), data);
                        }
                        else if (j == 1)
                        {
                            Image::rawRotateCCW(image.getWidth(), image.getHeight(), 1, image.getPixelFormat(), data);
                            Image::rawFlipVertical(image.getWidth(), image.getHeight(), 1, image.getPixelFormat(), data);
                        }
                        else if (j == 2 || j == 4)
                            Image::rawFlipVertical(image.getWidth(), image.getHeight(), 1, image.getPixelFormat(), data);
                        else if (j == 3 || j == 5)
                            Image::rawFlipHorizontal(image.getWidth(), image.getHeight(), 1, image.getPixelFormat(), data);
                    }
                }
            }
            else
                image.flipVertical();

            if (!file.isEOF())
                LOG_WARNING << "VTF load was successful, but not all data in the file was read";

            return true;
        }
        catch (const Exception& e)
        {
            image.clear();

            LOG_ERROR << e;

            return false;
        }
    }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(vtf, VTF::load, nullptr)

}
