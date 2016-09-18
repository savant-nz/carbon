/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

/**
 * This class adds support for loading DDS images. It supports uncompressed images and images compressed with DXT1, DXT3
 * or DXT5 compression. Supports mipmapped and non-mipmapped 2D, 3D and cubemap images.
 *
 * \note The DDS format is documented in the DirectX SDK. DDS image data is stored with an upper left origin, so an
 * Image::flipVertical() call is required to convert to the lower left origin used by the Image class.
 */
class DDS
{
public:

    // DDS structures and flags
    struct DDCAPS2
    {
        uint32_t dwCaps1;
        uint32_t dwCaps2;
        uint32_t dwReserved[2];
    };
    struct DDPIXELFORMAT
    {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwRGBAlphaBitMask;
    };
    struct DDSURFACEDESC2
    {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        uint32_t dwPitchOrLinearSize;
        uint32_t dwDepth;
        uint32_t dwMipmapCount;
        uint32_t dwReserved[11];

        DDPIXELFORMAT ddpfPixelFormat;
        DDCAPS2 ddsCaps;

        uint32_t dwReserved2;
    };
    struct DDSHeader
    {
        uint32_t dwMagic;

        DDSURFACEDESC2 ddsd;
    };

    static const auto DDSD_CAPS = 0x01;
    static const auto DDSD_HEIGHT = 0x02;
    static const auto DDSD_WIDTH = 0x04;
    static const auto DDSD_PITCH = 0x08;
    static const auto DDSD_PIXELFORMAT = 0x1000;
    static const auto DDSD_MIPMAPCOUNT = 0x020000;
    static const auto DDSD_LINEARSIZE = 0x080000;
    static const auto DDSD_DEPTH = 0x800000;
    static const auto DDPF_ALPHAPIXELS = 0x01;
    static const auto DDPF_ALPHA = 0x02;
    static const auto DDPF_FOURCC = 0x04;
    static const auto DDPF_RGB = 0x40;
    static const auto DDSCAPS_COMPLEX = 0x08;
    static const auto DDSCAPS_MIPMAP = 0x400000;
    static const auto DDSCAPS_TEXTURE = 0x1000;
    static const auto DDSCAPS2_CUBEMAP = 0x0200;
    static const auto DDSCAPS2_VOLUME = 0x200000;
    static const auto DDSCAPS2_CUBEMAP_POSITIVEX = 0x0400;
    static const auto DDSCAPS2_CUBEMAP_NEGATIVEX = 0x0800;
    static const auto DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000;
    static const auto DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000;
    static const auto DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000;
    static const auto DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000;

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        try
        {
            image.clear();

            if (imageIndex)
                LOG_WARNING << "Image indexing not supported for this format";

            // Read header
            auto header = DDSHeader();
            file.read(header.dwMagic, header.ddsd.dwSize, header.ddsd.dwFlags, header.ddsd.dwHeight,
                      header.ddsd.dwWidth, header.ddsd.dwPitchOrLinearSize, header.ddsd.dwDepth,
                      header.ddsd.dwMipmapCount);
            file.skip(44);
            file.read(header.ddsd.ddpfPixelFormat.dwSize, header.ddsd.ddpfPixelFormat.dwFlags,
                      header.ddsd.ddpfPixelFormat.dwFourCC, header.ddsd.ddpfPixelFormat.dwRGBBitCount,
                      header.ddsd.ddpfPixelFormat.dwRBitMask, header.ddsd.ddpfPixelFormat.dwGBitMask,
                      header.ddsd.ddpfPixelFormat.dwBBitMask, header.ddsd.ddpfPixelFormat.dwRGBAlphaBitMask,
                      header.ddsd.ddsCaps.dwCaps1, header.ddsd.ddsCaps.dwCaps2);
            file.skip(12);

            // Check header
            if (header.dwMagic != FileSystem::makeFourCC("DDS "))
                throw Exception("Not a DDS file");

            if (header.ddsd.dwSize != sizeof(DDSURFACEDESC2) ||
                header.ddsd.ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT))
                throw Exception("Invalid header");

            auto flags = header.ddsd.dwFlags;
            if (!(flags & DDSD_CAPS) || !(flags & DDSD_PIXELFORMAT) || !(flags & DDSD_WIDTH) || !(flags & DDSD_HEIGHT))
                throw Exception("Missing required flags");

            if (!(header.ddsd.ddsCaps.dwCaps1 & DDSCAPS_TEXTURE))
                throw Exception("Missing texture flag");

            if (header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            {
                if (!loadCubemap(file, image, header))
                    throw Exception("Failed loading cubemap image");
            }
            else
            {
                if (!loadNormal(file, image, header))
                    throw Exception("Failed loading image");
            }

            if (!file.isEOF())
                LOG_WARNING << file.getName() << " - DDS load was successful, but not all data in the file was read";

            if (!image.isCubemap() && !image.flipVertical())
                LOG_WARNING << file.getName() << " - Failed flipping image vertically";

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << file.getName() << " - " << e;

            image.clear();

            return false;
        }
    }

private:

    static Image::PixelFormat getPixelFormat(const DDSHeader& header)
    {
        auto redMask = header.ddsd.ddpfPixelFormat.dwRBitMask;
        auto greenMask = header.ddsd.ddpfPixelFormat.dwGBitMask;
        auto blueMask = header.ddsd.ddpfPixelFormat.dwBBitMask;
        auto alphaMask = header.ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;

        if (header.ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        {
            // Compressed format
            auto fourCC = header.ddsd.ddpfPixelFormat.dwFourCC;

            if (fourCC == FileSystem::makeFourCC("DXT1"))
                return Image::DXT1;

            if (fourCC == FileSystem::makeFourCC("DXT3"))
                return Image::DXT3;

            if (fourCC == FileSystem::makeFourCC("DXT5"))
                return Image::DXT5;
        }
        else if (header.ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            if (header.ddsd.ddpfPixelFormat.dwRGBBitCount == 32)
            {
                // 32-bit RGBA formats
                if (header.ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
                {
                    if (redMask == 0xFF0000 && greenMask == 0xFF00 && blueMask == 0xFF && alphaMask == 0xFF000000)
                        return Image::BGRA8;

                    if (redMask == 0xFF && greenMask == 0xFF00 && blueMask == 0xFF0000 && alphaMask == 0xFF000000)
                        return Image::RGBA8;
                }
            }
            else if (header.ddsd.ddpfPixelFormat.dwRGBBitCount == 24)
            {
                // 24-bit RGB formats
                if (redMask == 0xFF0000 && greenMask == 0xFF00 && blueMask == 0xFF)
                    return Image::BGR8;

                if (blueMask == 0xFF0000 && greenMask == 0xFF00 && redMask == 0xFF)
                    return Image::RGB8;
            }
            else if (header.ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
            {
                // 16-bit RGB and RGBA formats
                if (redMask == 0xF800 && greenMask == 0x07E0 && blueMask == 0x1F)
                    return Image::RGB565;

                if (redMask == 0xF800 && greenMask == 0x07C0 && blueMask == 0x3E && alphaMask == 0x01)
                    return Image::RGBA5551;

                if (redMask == 0x7C00 && greenMask == 0x03E0 && blueMask == 0x1F && alphaMask == 0x8000)
                    return Image::ARGB1555;

                if (redMask == 0xF000 && greenMask == 0x0F00 && blueMask == 0xF0 && alphaMask == 0x0F)
                    return Image::RGBA4444;

                if (redMask == 0x0F00 && greenMask == 0xF0 && blueMask == 0x0F && alphaMask == 0xF000)
                    return Image::ARGB4444;
            }
        }
        else if (header.ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHA)
        {
            // 8-bit alpha format
            if (header.ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
            {
                if (redMask == 0 && greenMask == 0 && blueMask == 0 && alphaMask == 0xFF)
                    return Image::Alpha8;
            }
        }
        else
        {
            // 8-bit luminance format
            if (header.ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
            {
                if (redMask == 0xFF && greenMask == 0 && blueMask == 0)
                    return Image::Luminance8;
            }

            // 8-bit luminance alpha format
            else if (header.ddsd.ddpfPixelFormat.dwRGBBitCount == 16 && redMask == 0xFF && greenMask == 0 &&
                     blueMask == 0 && alphaMask == 0xFF00)
                return Image::LuminanceAlpha8;
        }

        return Image::UnknownPixelFormat;
    }

    static bool hasMipmaps(const DDSHeader& header)
    {
        return (header.ddsd.ddsCaps.dwCaps1 & DDSCAPS_MIPMAP) && (header.ddsd.dwFlags & DDSD_MIPMAPCOUNT) &&
            header.ddsd.dwMipmapCount != 0;
    }

    static bool loadNormal(FileReader& file, Image& image, const DDSHeader& header)
    {
        auto depth = 1U;

        if ((header.ddsd.dwFlags & DDSD_DEPTH) && (header.ddsd.ddsCaps.dwCaps1 & DDSCAPS_COMPLEX) &&
            (header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME))
            depth = header.ddsd.dwDepth;

        if (!image.initialize(header.ddsd.dwWidth, header.ddsd.dwHeight, depth, getPixelFormat(header),
                              hasMipmaps(header), 1))
            return false;

        file.readBytes(image.getDataForFrame(0), image.getFrameDataSize());

        return true;
    }

    static bool loadCubemap(FileReader& file, Image& image, const DDSHeader& header)
    {
        // Check the surface has the complex flag set
        if (!(header.ddsd.ddsCaps.dwCaps1 & DDSCAPS_COMPLEX))
            throw Exception("Surface not marked as complex");

        // Check all six cubemap faces are present
        if (!(header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) ||
            !(header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) ||
            !(header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) ||
            !(header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) ||
            !(header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) ||
            !(header.ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ))
            throw Exception("Must have all cubemap faces defined");

        if (!image.initializeCubemap(header.ddsd.dwWidth, getPixelFormat(header), hasMipmaps(header), 1))
            return false;

        // Read the cubemap face data
        for (auto i = 0U; i < 6; i++)
            file.readBytes(image.getCubemapDataForFrame(0, i), image.getFrameDataSize());

        return true;
    }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(dds, DDS::load, nullptr)

}
