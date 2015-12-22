/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"

namespace Carbon
{

/**
 * A thin wrapper around the offscreen rendering capabilities provided by GraphicsInterface.
 */
class CARBON_API RenderTarget : private Noncopyable
{
public:

    ~RenderTarget() { clear(); }

    /**
     * Clears and releases this render target.
     */
    void clear();

    /**
     * Creates a new render target, this will only succeed if the GraphicsInterface supports render targets. Returns success
     * flag.
     */
    bool create();

    /**
     * Returns the internal GraphicsInterface::RenderTargetObject used by this render target.
     */
    operator GraphicsInterface::RenderTargetObject() const { return renderTargetObject_; }

    /**
     * Returns whether this render target is a valid target for rendering in its current state.
     */
    bool isValid() const { return graphics().isRenderTargetValid(renderTargetObject_); }

    /**
     * Returns the current color texture being used by this render target, or null if no color texture is being used.
     */
    const Texture* getColorTexture(unsigned int index = 0) const
    {
        return colorTextures_.empty() ? nullptr : colorTextures_[index];
    }

    /**
     * Sets the color textures for this render target to render into. If the list is empty then this render target will not
     * render to a color buffer, i.e. standard color buffer rendering will be disabled for this render target.
     * RenderTarget::removeColorTextures() provides a shortcut for doing this. Some graphics hardware supports more than one
     * output texture which enables shaders to output data into various different textures, to use this feature pass the
     * textures to use into this method. The maximum number of simultaneous textures that can be rendered into is given by
     * GraphicsInterface::getMaximumRenderTargetColorTextures(). If one of the specified textures is a cubemap then the cubemap
     * face index to render into must be specified in \a cubemapFaces. Returns success flag.
     */
    bool setColorTextures(const Vector<const Texture*>& textures, const Vector<int>& cubemapFaces);

    /**
     * Shortcut for calling RenderTarget::setColorTextures() with a single texture.
     */
    bool setColorTexture(const Texture* texture) { return setColorTextures({texture}, {}); }

    /**
     * Clears all color output textures from this render target, this is equivalent to calling RenderTarget::setColorTextures()
     * with an empty vector.
     */
    bool removeColorTextures() { return setColorTextures({}, {}); }

    /**
     * Returns the current depth texture being used by this render target, or null if no depth texture is being used.
     */
    const Texture* getDepthTexture() const { return depthTexture_; }

    /**
     * Sets the depth texture for this render target to render into. The texture must have the pixel format Image::Depth or
     * Image::Depth24Stencil8. Passing a null texture indicates this render target should not render to a depth buffer, and so
     * standard depth buffer rendering will be disabled for this render target (this includes both depth testing and depth
     * writing). Returns success flag.
     */
    bool setDepthTexture(const Texture* texture);

    /**
     * Returns the current stencil texture being used by this render target, or null if no stencil texture is being used.
     */
    const Texture* getStencilTexture() const { return stencilTexture_; }

    /**
     * Sets the stencil texture for this render target to render into. The texture must have the pixel format
     * Image::Depth24Stencil8. Passing a null texture indicates this render target should not render to a depth buffer, and so
     * stencil buffer rendering will be disabled for this render target. If stencil buffering is not supported then this method
     * does nothing and returns true. Returns success flag.
     */
    bool setStencilTexture(const Texture* texture);

    /**
     * Enables a single color output texture for this render target into a single 2D texture, along with a depth texture and
     * stencil texture. Internally this just calls RenderTarget::setColorTextures(), RenderTarget::setDepthTexture() and
     * RenderTarget::setStencilTexture(). Returns success flag.
     */
    bool setTextures(const Texture* colorTexture, const Texture* depthTexture, const Texture* stencilTexture)
    {
        return setColorTextures({colorTexture}, {}) && setDepthTexture(depthTexture) && setStencilTexture(stencilTexture);
    }

    /**
     * Enables a single color output texture for this render target into a single face of a cubemap, along with a depth texture
     * and stencil texture. Internally this just calls RenderTarget::setColorTextures(), RenderTarget::setDepthTexture() and
     * RenderTarget::setStencilTexture(). Returns success flag.
     */
    bool setTextures(const Texture* colorTexture, unsigned int cubemapFace, const Texture* depthTexture,
                     const Texture* stencilTexture)
    {
        return setColorTextures({colorTexture}, {int(cubemapFace)}) && setDepthTexture(depthTexture) &&
            setStencilTexture(stencilTexture);
    }

    /**
     * Removes all color, depth and stencil textures from this render target.
     */
    void removeTextures()
    {
        removeColorTextures();
        setDepthTexture(nullptr);
        setStencilTexture(nullptr);
    }

private:

    GraphicsInterface::RenderTargetObject renderTargetObject_ = nullptr;

    Vector<const Texture*> colorTextures_;
    const Texture* depthTexture_ = nullptr;
    const Texture* stencilTexture_ = nullptr;

    friend class Renderer;
    mutable bool inUse_ = false;
};

}
