/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Image/LightmapPacker.h"
#include "CarbonEngine/Math/Rect.h"

namespace Carbon
{

LightmapPacker::LightmapPacker(unsigned int width, unsigned int height, Image::PixelFormat pixelFormat)
{
    image_.initialize(width, height, 1, pixelFormat, false, 1);

    // Allocate the taken array which tracks which image pixels have been allocated
    isUsed_.resize(width * height);
}

bool LightmapPacker::addLightmap(unsigned int width, unsigned int height, const byte_t* data, Rect& rect)
{
    // There is a 1 pixel border around each lightmap which gets filled with the border colors of the lightmap This avoids
    // texture filtering artifacts in texture sampling

    auto spacedWidth = width + 2;
    auto spacedHeight = height + 2;

    auto pixelSize = Image::getPixelFormatPixelSize(image_.getPixelFormat());

    if (spacedWidth > image_.getWidth() || spacedHeight > image_.getHeight())
        return false;

    for (auto y = 0U; y < image_.getHeight() - spacedHeight + 1; y++)
    {
        for (auto x = 0U; x < image_.getWidth() - spacedWidth + 1; x++)
        {
            auto addHere = true;

            // Try allocating it starting here
            for (auto ly = 0U; ly < spacedHeight; ly++)
            {
                for (auto lx = 0U; lx < spacedWidth; lx++)
                {
                    if (isUsed_[(y + ly) * image_.getWidth() + x + lx])
                    {
                        addHere = false;
                        break;
                    }
                }

                if (!addHere)
                    break;
            }

            if (addHere)
            {
                // Mark this area as allocated
                for (auto ly = 0U; ly < spacedHeight; ly++)
                    memset(&isUsed_[(y + ly) * image_.getWidth() + x], 0xFF, spacedWidth);

                // Move in one pixel to where the actual lightmap data will start
                x++;
                y++;

                // Copy in lightmap data
                for (auto ly = 0U; ly < height; ly++)
                {
                    memcpy(&image_.getDataForFrame(0)[((y + ly) * image_.getWidth() + x) * pixelSize],
                           data + ly * width * pixelSize, width * pixelSize);

                    // Copy the leftmost and rightmost pixels into the lightmap border
                    memcpy(&image_.getDataForFrame(0)[((y + ly) * image_.getWidth() + x - 1) * pixelSize],
                           data + ly * width * pixelSize, pixelSize);
                    memcpy(&image_.getDataForFrame(0)[((y + ly) * image_.getWidth() + x + width) * pixelSize],
                           data + (ly * width + width - 1) * pixelSize, pixelSize);
                }

                // Copy the top and bottom of the lightmap data into the lightmap border
                memcpy(&image_.getDataForFrame(0)[((y - 1) * image_.getWidth() + x) * pixelSize], data, width * pixelSize);
                memcpy(&image_.getDataForFrame(0)[((y + height) * image_.getWidth() + x) * pixelSize],
                       data + (height - 1) * width * pixelSize, width * pixelSize);

                // Copy the corners of the lightmap data into the lightmap border to complete
                memcpy(&image_.getDataForFrame(0)[((y - 1) * image_.getWidth() + x - 1) * pixelSize], data, pixelSize);
                memcpy(&image_.getDataForFrame(0)[((y - 1) * image_.getWidth() + x + width) * pixelSize],
                       data + (width - 1) * pixelSize, pixelSize);
                memcpy(&image_.getDataForFrame(0)[((y + height) * image_.getWidth() + x - 1) * pixelSize],
                       data + ((height - 1) * width) * pixelSize, pixelSize);
                memcpy(&image_.getDataForFrame(0)[((y + height) * image_.getWidth() + x + width) * pixelSize],
                       data + ((height - 1) * width + width - 1) * pixelSize, pixelSize);

                // Return texture coordinates for where the lightmap was placed
                rect = Rect(float(x) / float(image_.getWidth()), float(y) / float(image_.getHeight()),
                            float(x + width) / float(image_.getWidth()), float(y + height) / float(image_.getHeight()));

                return true;
            }
        }
    }

    return false;
}

}
