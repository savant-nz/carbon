/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CARBON_INCLUDE_FREEIMAGE

#include <LibJPEG/jpeglib.h>

namespace Carbon
{

/**
 * Provides JPEG image reading support through LibJPEG. Note that this uses the copy of LibJPEG that is part of FreeImage.
 */
class LibJPEG
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        auto jpegInfo = jpeg_decompress_struct();
        auto jpegError = jpeg_error_mgr();

        try
        {
            if (imageIndex)
                LOG_WARNING << "Image indexing not supported for this format";

            image.clear();

            // Setup error handling
            jpegInfo.err = jpeg_std_error(&jpegError);
            jpegError.error_exit = onLibJPEGError;

            // Initialize a JPEG decompression object
            jpeg_CreateDecompress(&jpegInfo, JPEG_LIB_VERSION, sizeof(jpeg_decompress_struct));

            // Read the file data into a buffer
            auto fileDataStorage = Vector<byte_t>();
            auto fileData = file.getData(fileDataStorage);
            jpeg_mem_src(&jpegInfo, fileData, file.getSize());

            // Read JPEG header
            jpeg_read_header(&jpegInfo, true);

            // Initialize output image
            if (!image.initialize(jpegInfo.image_width, jpegInfo.image_height, 1, Image::RGB8, false, 1))
                throw Exception("Failed initializing image");

            // Start the decompression
            jpegInfo.out_color_space = JCS_RGB;
            jpegInfo.dct_method = JDCT_FLOAT;
            jpegInfo.dither_mode = JDITHER_FS;
            jpegInfo.do_fancy_upsampling = FALSE;
            jpeg_start_decompress(&jpegInfo);

            // Calculate the size of an individual row
            auto rowSize = Image::getImageDataSize(image.getWidth(), 1, image.getDepth(), image.getPixelFormat());

            // Read all the rows in the image, converting to bottom-left origin
            auto scanline = Vector<JSAMPLE>(jpegInfo.output_width * jpegInfo.output_components);
            for (auto y = 0U; y < jpegInfo.output_height; y++)
            {
                auto ptr = scanline.getData();
                jpeg_read_scanlines(&jpegInfo, &ptr, 1);

                memcpy(image.getDataForFrame(0) + (image.getHeight() - y - 1) * rowSize, scanline.getData(), rowSize);
            }

            // Clean up
            jpeg_finish_decompress(&jpegInfo);
            jpeg_destroy_decompress(&jpegInfo);

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;

            jpeg_finish_decompress(&jpegInfo);
            jpeg_destroy_decompress(&jpegInfo);

            return false;
        }
    }

private:

    static void onLibJPEGError(j_common_ptr jpegInfo)
    {
        auto buffer = std::array<char, JMSG_LENGTH_MAX>();
        (*jpegInfo->err->format_message)(jpegInfo, buffer.data());

        throw Exception() << "LibJPEG error: " << buffer.data();
    }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(jpg, LibJPEG::load, nullptr)
CARBON_REGISTER_IMAGE_FILE_FORMAT(jpeg, LibJPEG::load, nullptr)

}

#endif
