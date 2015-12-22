/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

/**
 * 3D texture object that inherits from the base Texture class.
 */
class CARBON_API Texture3D : public Texture
{
public:

    /**
     * Returns the width of this 3D texture.
     */
    unsigned int getWidth() const { return getImage().getWidth(); }

    /**
     * Returns the height of this 3D texture.
     */
    unsigned int getHeight() const { return getImage().getHeight(); }

    /**
     * Returns the depth of this 3D texture.
     */
    unsigned int getDepth() const { return getImage().getDepth(); }

    GraphicsInterface::TextureType getTextureType() const override { return GraphicsInterface::Texture3D; }
    bool upload() override;
};

}
