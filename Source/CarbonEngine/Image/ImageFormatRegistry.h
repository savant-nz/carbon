/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileFormatRegistry.h"
#include "CarbonEngine/Image/Image.h"

namespace Carbon
{

/**
 * Typedef for an image file reading function.
 */
typedef std::function<bool(FileReader& file, Image& image, unsigned int imageIndex,
                           Image::PixelFormat targetPixelFormat)>
    ReadImageFormatFunction;

/**
 * Typedef for an image file writing function.
 */
typedef std::function<bool(FileWriter& file, const Image& image)> WriteImageFormatFunction;

/**
 * Handles the registration of supported image formats and provides access to the reading and writing functions for each
 * supported format. Image formats can be registered with the CARBON_REGISTER_IMAGE_FILE_FORMAT() macro.
 */
class CARBON_API ImageFormatRegistry : public FileFormatRegistry<ReadImageFormatFunction, WriteImageFormatFunction>
{
public:

    /**
     * Given a filename that may or may not have an extension this method tries to load an image out of it. If the given
     * filename contains an extension then that format will be assumed, otherwise the filesystem will be searched for a
     * matching filename with an extension that has a reader function available. If one is found then it will be used to
     * read the image.
     *
     * If the filename ends with a number enclosed in square brackets (such as 'my_texture[1]' or 'my_texture.png[1]')
     * then that will be stripped off prior to load and the number will be interpreted as an image index into the
     * specified image. This allows the use of image formats which contain multiple images, because the desired image
     * can be chosen using the appropriate index value. Note that this is only useful if the target image format
     * supports multiple images, if it does not then any index will be ignored and a warning will be emitted.
     *
     * If \a targetPixelFormat is set to something other than \a UnknownPixelFormat then the image will be converted to
     * that pixel format before being returned. The advantage of using the target pixel format parameter is that it is
     * passed through to image loading methods which can then use it to load the image file directly into the desired
     * format rather than having to go through an intermediate format before being converted to the target pixel format.
     * If an image load method doesn't support loading directly into the specified target pixel format then
     * Image::setPixelFormat() will be used after the image has been loaded.
     *
     * Returns success flag.
     */
    static bool loadImageFile(UnicodeString filename, Image& image,
                              Image::PixelFormat targetPixelFormat = Image::UnknownPixelFormat);

    /**
     * Saves the passed image to a file, the format of the image file is determined by the extension present on the
     * passed filename. Note that many supported image formats do not support all of the features of the Image class
     * (e.g. PNG does not support animations or cubemaps), and so various internal conversions may have to be done or
     * certain parts of the image data skipped depending on the target format. The only image format which is guaranteed
     * to be able to persist an Image instance without losing any data is the native '.image' format, this is because
     * the '.image' format is a memory dump of the entire contents of an Image instance. However, because it is a native
     * format, '.image' files are not supported by other image viewing or editing applications. Returns success flag.
     */
    static bool saveImageFile(const UnicodeString& filename, const Image& image);

private:

    // Looks for an image index at the end of the passed name in the form "imagename[<index>]" and if one is found it
    // returns the index value and strips the image index from the passed name. Returns 0 if there is no valid image
    // index.
    static unsigned int detectImageIndex(UnicodeString& imageName);
};

#ifndef DOXYGEN

CARBON_DECLARE_FILE_FORMAT_REGISTRY(ReadImageFormatFunction, WriteImageFormatFunction);

#endif

/**
 * \file
 */

/**
 * Registers reading and writing functions for the image file format with the given extension. If a null function
 * pointer is specified it will be ignored.
 */
#define CARBON_REGISTER_IMAGE_FILE_FORMAT(Extension, ReaderFunction, WriterFunction) \
    CARBON_REGISTER_FILE_FORMAT(Carbon::ImageFormatRegistry, Extension, ReaderFunction, WriterFunction)
}
