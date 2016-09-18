/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

/**
 * This hooks the native Image class save and load up to the ImageFormatRegistry mechanism so that .image files save and
 * load like all other supported image formats.
 */
class NativeImage
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        try
        {
            if (imageIndex)
                LOG_WARNING << "Image indexing not supported for this format";

            image.load(file);
            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;
            return false;
        }
    }

    static bool save(FileWriter& file, const Image& image)
    {
        try
        {
            image.save(file);
            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;
            return false;
        }
    }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(image, NativeImage::load, NativeImage::save)

}
