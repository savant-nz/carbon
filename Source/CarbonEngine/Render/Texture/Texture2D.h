/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Rect.h"
#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

/**
 * 2D texture object that inherits from the base Texture class.
 */
class CARBON_API Texture2D : public Texture
{
public:

    /**
     * Returns the width of this 2D texture.
     */
    unsigned int getWidth() const { return getImage().getWidth(); }

    /**
     * Returns the height of this 2D texture.
     */
    unsigned int getHeight() const { return getImage().getHeight(); }

    /**
     * Samples this texture at the given normalized u,v offset.
     */
    Color sampleNearestTexel(float u, float v, unsigned int frame = 0) const;

    GraphicsInterface::TextureType getTextureType() const override { return GraphicsInterface::Texture2D; }
    bool upload() override;
};

}
