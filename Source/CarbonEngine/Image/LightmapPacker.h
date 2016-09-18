/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Image/Image.h"

namespace Carbon
{

/**
 * Helper class that packs individual lightmaps into one large lightmap texture.
 */
class CARBON_API LightmapPacker
{
public:

    /**
     * Initializes this lightmap packer with the given lightmap texture dimensions and pixel format. The pixel format
     * can not be a compressed format.
     */
    LightmapPacker(unsigned int width, unsigned int height, Image::PixelFormat pixelFormat);

    /**
     * Packs a new lightmap into the final image. The data must be of the format RGBA8. The area of the final image
     * where this lightmap gets placed is returned in \a rect. Returns success flag.
     */
    bool addLightmap(unsigned int width, unsigned int height, const byte_t* data, Rect& rect);

    /**
     * Returns the internal Image object that is being filled with lightmap data by the calls to
     * LightmapPacker::addLightmap().
     */
    const Image& getImage() const { return image_; }

private:

    Image image_;
    Vector<bool> isUsed_;
};

}
