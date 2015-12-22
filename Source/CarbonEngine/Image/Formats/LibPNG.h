/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CARBON_INCLUDE_FREEIMAGE

#include <LibPNG/png.h>

namespace Carbon
{

/**
 * Provides PNG image reading support through LibPNG. Note that this uses the copy of LibPNG that is part of FreeImage.
 */
class LibPNG
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        auto png = pointer_to<png_struct>::type();
        auto pngInfo = pointer_to<png_info>::type();

        if (imageIndex)
            LOG_WARNING << "Image indexing not supported for this format";

        try
        {
            image.clear();

            png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, pngError, nullptr);
            if (!png)
                throw Exception("PNG library setup failed");

            pngInfo = png_create_info_struct(png);
            if (!pngInfo)
                throw Exception("PNG library setup failed");

            // Prepare for PNG read
            png_set_read_fn(png, &file, pngRead);
            png_read_info(png, pngInfo);
            png_set_expand(png);
            png_read_update_info(png, pngInfo);

            // Check the bit depth is 8-bit and that interlacing isn't used on this image
            if (png_get_bit_depth(png, pngInfo) != 8 || png_get_interlace_type(png, pngInfo) != PNG_INTERLACE_NONE)
                throw Exception("Only 8-bit non-interlaced images are supported");

            // Read image dimensions
            auto width = png_get_image_width(png, pngInfo);
            auto height = png_get_image_height(png, pngInfo);

            // Determine pixel format
            auto pixelFormat = Image::UnknownPixelFormat;
            switch (png_get_color_type(png, pngInfo))
            {
                case PNG_COLOR_TYPE_GRAY:
                    pixelFormat = Image::Luminance8;
                    break;
                case PNG_COLOR_TYPE_RGB:
                    pixelFormat = Image::RGB8;
                    break;
                case PNG_COLOR_TYPE_RGB_ALPHA:
                    pixelFormat = Image::RGBA8;
                    break;
                case PNG_COLOR_TYPE_GRAY_ALPHA:
                    pixelFormat = Image::LuminanceAlpha8;
                    break;
                default:
                    throw Exception("Unknown color type");
            }

            // Initialize output image
            if (!image.initialize(width, height, 1, pixelFormat, false, 1))
                throw Exception("Failed initializing image");

            // Calculate the size of an individual row
            auto rowSize = Image::getImageDataSize(image.getWidth(), 1, image.getDepth(), image.getPixelFormat());

            // Read all the rows in the image, converting to bottom-left origin
            for (auto y = 0U; y < image.getHeight(); y++)
            {
                auto row = image.getDataForFrame(0) + (image.getHeight() - y - 1) * rowSize;
                png_read_rows(png, &row, nullptr, 1);
            }

            // Clean up
            png_destroy_read_struct(&png, &pngInfo, nullptr);

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;

            png_destroy_read_struct(&png, &pngInfo, nullptr);

            return false;
        }
    }

private:

    static void pngRead(png_structp png, png_bytep data, png_size_t length)
    {
        try
        {
            reinterpret_cast<FileReader*>(png_get_io_ptr(png))->readBytes(data, uint(length));
        }
        catch (const Exception&)
        {
            LOG_ERROR << "Failed reading data from file";
        }
    }

    static void pngError(png_structp png, png_const_charp message) { throw Exception(message); }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(png, LibPNG::load, nullptr)

}

#endif
