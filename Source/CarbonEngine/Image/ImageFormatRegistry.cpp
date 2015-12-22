/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"

namespace Carbon
{

CARBON_DEFINE_FILE_FORMAT_REGISTRY(ReadImageFormatFunction, WriteImageFormatFunction)

bool ImageFormatRegistry::loadImageFile(UnicodeString filename, Image& image, Image::PixelFormat targetPixelFormat)
{
    // Get any image index that may be present, will be zero if none is specified
    auto imageIndex = detectImageIndex(filename);

    auto file = FileReader();
    auto fnReader = loadFile(filename, file);

    return fnReader && fnReader(file, image, imageIndex, targetPixelFormat) && image.isValidImage() &&
        image.setPixelFormat(targetPixelFormat);
}

bool ImageFormatRegistry::saveImageFile(const UnicodeString& filename, const Image& image)
{
    // Check that the image is valid
    if (!image.isValidImage())
    {
        LOG_ERROR << "Invalid image";
        return false;
    }

    auto file = FileWriter();
    auto fnWriter = saveFile(filename, file);

    return fnWriter && fnWriter(file, image);
}

unsigned int ImageFormatRegistry::detectImageIndex(UnicodeString& imageName)
{
    auto index = imageName.getIndexInBrackets();
    if (index < 0)
        return 0;

    // Chop off the image index suffix from the image name
    imageName = imageName.withoutIndexInBrackets();

    // Return the image index
    return uint(index);
}

}

// Include all the built-in image formats
#include "CarbonEngine/Image/Formats/DDS.h"
#include "CarbonEngine/Image/Formats/FreeImageLoader.h"
#include "CarbonEngine/Image/Formats/LibJPEG.h"
#include "CarbonEngine/Image/Formats/LibPNG.h"
#include "CarbonEngine/Image/Formats/Meta.h"
#include "CarbonEngine/Image/Formats/NativeImage.h"
#include "CarbonEngine/Image/Formats/PVR.h"
#include "CarbonEngine/Image/Formats/VTF.h"
