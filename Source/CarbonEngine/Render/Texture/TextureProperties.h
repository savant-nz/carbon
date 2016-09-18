/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"

namespace Carbon
{

/**
 * Holds texture properties such as filtering, wrapping mode, anisotropy and quality.
 */
class CARBON_API TextureProperties
{
public:

    /**
     * Texture filtering modes.
     */
    enum TextureFilter
    {
        /**
         * Nearest filtering.
         */
        NearestFilter,

        /**
         * Bilinear filtering.
         */
        BilinearFilter,

        /**
         * Trilinear filtering. This requires the texture to have mipmaps, otherwise BilinearFilter will be used
         * instead.
         */
        TrilinearFilter
    };

    /**
     * The low texture quality value, currently 256.
     */
    static const auto TextureQualityLow = 256U;

    /**
     * The medium texture quality value, currently 512.
     */
    static const auto TextureQualityMedium = 512U;

    /**
     * The high texture quality value, currently 1024.
     */
    static const auto TextureQualityHigh = 1024U;

    /**
     * The maximum texture quality value, currently zero.
     */
    static const auto TextureQualityMaximum = 0U;

    TextureProperties() {}

    /**
     * Constructs these texture properties with the specified texture filter and wrap.
     */
    TextureProperties(TextureFilter filter, GraphicsInterface::TextureWrap wrap = GraphicsInterface::WrapClamp)
        : filter_(filter), wrap_(wrap)
    {
    }

    /**
     * Returns the current texture filter.
     */
    TextureFilter getFilter() const { return filter_; }

    /**
     * Sets the current texture filter.
     */
    void setFilter(TextureFilter filter) { filter_ = filter; }

    /**
     * Returns the current texture wrap mode.
     */
    GraphicsInterface::TextureWrap getWrap() const { return wrap_; }

    /**
     * Sets the current texture wrap mode.
     */
    void setWrap(GraphicsInterface::TextureWrap wrap) { wrap_ = wrap; }

    /**
     * Returns the current texture quality level. The texture quality value sets the maximum size for a texture, any
     * textures larger than the maximum size are scaled down. A quality value of zero means there is no imposed limit on
     * texture size.
     */
    unsigned int getQuality() const { return quality_; }

    /**
     * Sets the current texture quality level. See TextureProperties::getQuality() for details.
     */
    void setQuality(unsigned int quality) { quality_ = quality; }

    /**
     * Returns the current level of anisotropy.
     */
    unsigned int getAnisotropy() const { return anisotropy_; }

    /**
     * Sets the current level of anisotropy. This clamps to between 1 and the maximum value the hardware allows for
     * anisotropy. The maximum allowable anisotropy value can be retrieved with
     * GraphicsInterface::getMaximumTextureAnisotropy().
     */
    void setAnisotropy(unsigned int anisotropy) { anisotropy_ = anisotropy; }

private:

    TextureFilter filter_ = NearestFilter;
    GraphicsInterface::TextureWrap wrap_ = GraphicsInterface::WrapClamp;
    unsigned int quality_ = 0;
    unsigned int anisotropy_ = 1;
};

}
