/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

/**
 * Cubemap texture object that inherits from the base Texture class.
 */
class CARBON_API TextureCubemap : public Texture
{
public:

    /**
     * Returns the size of the cubemap texture. Since cubemaps must be square, this is the width and height of each
     * face.
     */
    unsigned int getSize() const { return getImage().getWidth(); }

    GraphicsInterface::TextureType getTextureType() const override { return GraphicsInterface::TextureCubemap; }
    bool upload() override;

private:

    void getCubeVector(unsigned int size, unsigned int index, unsigned int x, unsigned int y, Vec3& v) const;
};

}
