/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CARBON_INCLUDE_FREEIMAGE

#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Image/Formats/FreeImageIncludeWrapper.h"
#include "CarbonEngine/Math/MathCommon.h"

#ifndef CARBON_INCLUDE_ZLIB
    #error Using FreeImage requires that ZLib is included in the build
#endif

#ifdef _MSC_VER
    #pragma comment(lib, "FreeImage" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
    #pragma comment(lib, "ZLib" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
#endif

namespace Carbon
{

/**
 * Helper class that loads images using the FreeImage library.
 */
template <FREE_IMAGE_FORMAT FreeImageFormat> class CARBON_API FreeImage
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        auto bitmap = pointer_to<FIBITMAP>::type();
        auto multiBitmap = pointer_to<FIMULTIBITMAP>::type();
        auto memory = pointer_to<FIMEMORY>::type();

        try
        {
            image.clear();

            if (imageIndex)
                LOG_WARNING << "Image indexing not supported for this format";

            if (FreeImageFormat == FIF_GIF)
            {
                // Read the GIF file data into a local memory buffer
                auto fileDataStorage = Vector<byte_t>();
                auto fileData = file.getData(fileDataStorage);

                // Load the GIF file into a FreeImage multi-bitmap
                memory = FreeImage_OpenMemory(fileData, file.getSize());
                multiBitmap = FreeImage_LoadMultiBitmapFromMemory(FreeImageFormat, memory, 0);
                if (!multiBitmap)
                    throw Exception("Failed loading image");

                // Read each individual page of the GIF image
                auto pageCount = uint(FreeImage_GetPageCount(multiBitmap));
                for (auto i = 0U; i < pageCount; i++)
                {
                    // Get access to the bitmap data on this page
                    auto pageBitmap = FreeImage_LockPage(multiBitmap, i);
                    if (!pageBitmap)
                        throw Exception("Failed loading image");

                    auto original = pageBitmap;

                    try
                    {
                        // If this is the first frame just read it into the output image
                        if (i == 0)
                            loadFIBITMAP(file, pageBitmap, image, true);
                        else
                        {
                            // Subsequent frames just have their data appended to the output image's frames
                            auto pageImage = Image();
                            loadFIBITMAP(file, pageBitmap, pageImage, true);

                            if (!image.append(pageImage))
                                throw Exception("Failed appending GIF frame, check that all frames are the same size");
                        }

                        // Clean up this page
                        if (pageBitmap != original)
                            FreeImage_Unload(pageBitmap);
                        FreeImage_UnlockPage(multiBitmap, original, FALSE);
                    }
                    catch (const Exception&)
                    {
                        // Clean up this page
                        if (pageBitmap != original)
                            FreeImage_Unload(pageBitmap);
                        FreeImage_UnlockPage(multiBitmap, original, FALSE);

                        throw;
                    }
                }
            }
            else
            {
                // Load the image file into a FreeImage bitmap
                auto ioProcs = FreeImageIO{readProc, nullptr, readerSeekProc, readerTellProc};
                bitmap = FreeImage_LoadFromHandle(FreeImageFormat, &ioProcs, reinterpret_cast<fi_handle>(&file), 0);
                if (!bitmap)
                    throw Exception("Failed loading image");

                loadFIBITMAP(file, bitmap, image, false);
            }

            // Clear up all FreeImage objects
            if (bitmap)
                FreeImage_Unload(bitmap);
            if (multiBitmap)
                FreeImage_CloseMultiBitmap(multiBitmap);
            if (memory)
                FreeImage_CloseMemory(memory);

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;

            image.clear();

            // Clear up all FreeImage objects
            if (bitmap)
                FreeImage_Unload(bitmap);
            if (multiBitmap)
                FreeImage_CloseMultiBitmap(multiBitmap);
            if (memory)
                FreeImage_CloseMemory(memory);

            return false;
        }
    }

    static bool save(FileWriter& file, const Image& image)
    {
        auto bitmap = pointer_to<FIBITMAP>::type();

        FreeImage_Initialise();

        try
        {
            if (!image.isValid2DImage())
                throw Exception("Invalid image data");

            if (image.getFrameCount() != 1)
                LOG_WARNING << "Saving animated images is not supported, only the first frame will be saved";

            if (image.hasMipmaps())
                LOG_WARNING << "Saving mipmaps is not supported, only the base level mipmap will be saved";

            // Check whether this image has an alpha channel and whether or not it can be saved in this image format
            auto saveAlpha = false;
            if (Image::isPixelFormatAlphaAware(image.getPixelFormat()))
            {
                if (FreeImage_FIFSupportsExportBPP(FreeImageFormat, 32))
                    saveAlpha = true;
                else
                    LOG_WARNING << "Target image format doesn't support alpha, only RGB data will be saved";
            }

            // Make a copy of the image data and convert it to RGBA8
            auto copy = image;
            if (!copy.setPixelFormat(Image::RGBA8))
                throw Exception("Failed converting image to RGBA8");

            // Create FreeImage bitmap
            bitmap = FreeImage_Allocate(image.getWidth(), image.getHeight(), saveAlpha ? 32 : 24, 0x000000FF,
                                        0x0000FF00, 0x00FF0000);
            if (!bitmap)
                throw Exception("Failed allocating bitmap");
            if (saveAlpha)
                FreeImage_SetTransparent(bitmap, TRUE);

            // Copy image data into FreeImage bitmap
            auto pixel = copy.getDataForFrame(0);
            for (auto y = 0U; y < image.getHeight(); y++)
            {
                for (auto x = 0U; x < image.getWidth(); x++)
                {
                    auto rgba = RGBQUAD();
                    rgba.rgbRed = *pixel++;
                    rgba.rgbGreen = *pixel++;
                    rgba.rgbBlue = *pixel++;
                    rgba.rgbReserved = *pixel++;

                    if (!FreeImage_SetPixelColor(bitmap, x, y, &rgba))
                        throw Exception("Failed setting pixel color");
                }
            }

            // Save image
            auto ioProcs = FreeImageIO{nullptr, writeProc, writerSeekProc, writerTellProc};
            if (!FreeImage_SaveToHandle(FreeImageFormat, bitmap, &ioProcs, fi_handle(&file), 0))
                throw Exception("Failed saving image");

            // Clean up
            FreeImage_Unload(bitmap);
            FreeImage_DeInitialise();

            return true;
        }
        catch (const Exception& e)
        {
            if (bitmap)
                FreeImage_Unload(bitmap);

            LOG_ERROR << e;

            FreeImage_DeInitialise();

            return false;
        }
    }

private:

    // Loads a single FIBITMAP into a Carbon::Image object
    static void loadFIBITMAP(FileReader& file, FIBITMAP*& fiBitmap, Image& image, bool isMultiBitmap)
    {
        // If the image is not stored in 24 or 32 BPP then convert it
        auto bpp = FreeImage_GetBPP(fiBitmap);
        if (bpp != 24 && bpp != 32)
        {
            auto newBitmap = pointer_to<FIBITMAP>::type();
            if (FreeImage_IsTransparent(fiBitmap))
                newBitmap = FreeImage_ConvertTo32Bits(fiBitmap);
            else
                newBitmap = FreeImage_ConvertTo24Bits(fiBitmap);

            // Check image conversion was successful
            if (!newBitmap)
                throw Exception("Failed converting image to 24/32 BPP");

            if (!isMultiBitmap)
            {
                // Delete old bitmap
                FreeImage_Unload(fiBitmap);
            }

            fiBitmap = newBitmap;
            bpp = FreeImage_GetBPP(fiBitmap);
        }

        auto pixelFormat = Image::UnknownPixelFormat;
        if (bpp == 24)
            pixelFormat = Image::BGR8;
        else if (bpp == 32)
            pixelFormat = Image::BGRA8;

        // Setup output image
        if (!image.initialize(FreeImage_GetWidth(fiBitmap), FreeImage_GetHeight(fiBitmap), 1, pixelFormat, false, 1))
            throw Exception("Failed initializing image");

        // Copy pixel data in one scanline at a time because FreeImage aligns scanlines to a DWORD boundary
        auto imageData = image.getDataForFrame(0);
        auto rowSize = Image::getImageDataSize(image.getWidth(), 1, 1, image.getPixelFormat(), false);
        for (auto y = 0U; y < image.getHeight(); y++)
        {
            memcpy(imageData, FreeImage_GetScanLine(fiBitmap, y), rowSize);
            imageData += rowSize;
        }
    }

    static unsigned int readProc(void* buffer, unsigned int size, unsigned int count, fi_handle handle)
    {
        auto& file = *reinterpret_cast<FileReader*>(handle);

        auto bytesRead = 0U;
        try
        {
            file.readBytes(buffer, size * count, &bytesRead);
        }
        catch (const Exception&)
        {
        }

        return bytesRead / size;
    }

    static int readerSeekProc(fi_handle handle, long offset, int origin)
    {
        auto& file = *reinterpret_cast<FileReader*>(handle);

        auto newPosition = 0L;
        if (origin == SEEK_SET)
            newPosition = offset;
        else if (origin == SEEK_CUR)
            newPosition = file.getPosition() + offset;
        else if (origin == SEEK_END)
            newPosition = file.getSize() + offset;
        else
            return -1;

        try
        {
            file.setPosition(uint(newPosition));
            return 0;
        }
        catch (const Exception&)
        {
            return -1;
        }
    }

    static long readerTellProc(fi_handle handle) { return reinterpret_cast<FileReader*>(handle)->getPosition(); }

    static unsigned int writeProc(void* ptr, unsigned int size, unsigned int count, fi_handle handle)
    {
        try
        {
            reinterpret_cast<FileWriter*>(handle)->writeBytes(ptr, size * count);
        }
        catch (const Exception&)
        {
            return 0;
        }

        return size * count;
    }

    static int writerSeekProc(fi_handle handle, long offset, int origin)
    {
        auto& file = *reinterpret_cast<FileWriter*>(handle);

        auto newPosition = 0L;
        if (origin == SEEK_SET)
            newPosition = offset;
        else if (origin == SEEK_CUR)
            newPosition = file.getPosition() + offset;
        else
            return -1;

        try
        {
            file.setPosition(uint(newPosition));
            return 0;
        }
        catch (const Exception&)
        {
            return -1;
        }
    }

    static long writerTellProc(fi_handle handle) { return reinterpret_cast<FileWriter*>(handle)->getPosition(); }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(bmp, FreeImage<FIF_BMP>::load, FreeImage<FIF_BMP>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(gif, FreeImage<FIF_GIF>::load, FreeImage<FIF_GIF>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(ico, FreeImage<FIF_ICO>::load, FreeImage<FIF_ICO>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(mng, FreeImage<FIF_MNG>::load, FreeImage<FIF_MNG>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(psd, FreeImage<FIF_PSD>::load, FreeImage<FIF_PSD>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(tga, FreeImage<FIF_TARGA>::load, FreeImage<FIF_TARGA>::save)

// Prefer the native LibJPEG and LibPNG image loaders, they are faster and use less memory than FreeImage
CARBON_REGISTER_IMAGE_FILE_FORMAT(jpg, nullptr, FreeImage<FIF_JPEG>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(jpeg, nullptr, FreeImage<FIF_JPEG>::save)
CARBON_REGISTER_IMAGE_FILE_FORMAT(png, nullptr, FreeImage<FIF_PNG>::save)

// Register functions to startup and shutdown FreeImage
static void freeImageStartup()
{
    FreeImage_Initialise();
}
CARBON_REGISTER_STARTUP_FUNCTION(freeImageStartup, 0)

static void freeImageShutdown()
{
    FreeImage_DeInitialise();
}
CARBON_REGISTER_SHUTDOWN_FUNCTION(freeImageShutdown, 0)

}

#endif
