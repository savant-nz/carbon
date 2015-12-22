/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Render/RenderTarget.h"
#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

void RenderTarget::clear()
{
    graphics().deleteRenderTarget(renderTargetObject_);
    renderTargetObject_ = nullptr;

    colorTextures_.clear();
    depthTexture_ = nullptr;
    stencilTexture_ = nullptr;

    inUse_ = false;
}

bool RenderTarget::create()
{
    clear();

    if (!graphics().isRenderTargetSupported())
        return false;

    renderTargetObject_ = graphics().createRenderTarget();
    if (!renderTargetObject_)
    {
        LOG_ERROR << "Failed creating render target";
        return false;
    }

    // Initialize the new render target with no attached textures
    removeColorTextures();
    setDepthTexture(nullptr);
    setStencilTexture(nullptr);

    return true;
}

bool RenderTarget::setColorTextures(const Vector<const Texture*>& textures, const Vector<int>& cubemapFaces)
{
    if (!renderTargetObject_)
        return false;

    // Check that the textures all have the same pixel format
    for (auto texture : textures)
    {
        if (texture->getPixelFormat() != textures[0]->getPixelFormat())
        {
            LOG_ERROR << "All color textures should have the same pixel format";
            return false;
        }
    }

    // Can't render into compressed textures
    if (textures.size() && Image::isPixelFormatCompressed(textures[0]->getPixelFormat()))
    {
        LOG_ERROR << "Textures with compressed pixel formats can't be used as render targets";
        return false;
    }

    // Convert to a vector of TextureObjects
    auto textureObjects =
        textures.map<GraphicsInterface::TextureObject>([](const Texture* t) { return t->getActiveTextureObject(); });

    if (!graphics().setRenderTargetColorBufferTextures(renderTargetObject_, textureObjects, cubemapFaces))
        return false;

    colorTextures_ = textures;

    return true;
}

bool RenderTarget::setDepthTexture(const Texture* texture)
{
    if (!renderTargetObject_)
        return false;

    if (texture)
    {
        if (!Image::isPixelFormatDepthAware(texture->getImage().getPixelFormat()))
        {
            LOG_ERROR << "Only textures with a depth pixel format can be used as depth attachments";
            return false;
        }

        graphics().setRenderTargetDepthBufferTexture(renderTargetObject_, texture->getActiveTextureObject());
    }
    else
        graphics().setRenderTargetDepthBufferTexture(renderTargetObject_, nullptr);

    depthTexture_ = texture;

    return true;
}

bool RenderTarget::setStencilTexture(const Texture* texture)
{
    if (!renderTargetObject_)
        return false;

    if (!graphics().isStencilBufferSupported())
        return true;

    if (texture)
    {
        if (texture->getImage().getPixelFormat() != Image::Depth24Stencil8)
        {
            LOG_ERROR << "Only textures with a stencil pixel format can be used as stencil attachments";
            return false;
        }

        graphics().setRenderTargetStencilBufferTexture(renderTargetObject_, texture->getActiveTextureObject());
    }
    else
        graphics().setRenderTargetStencilBufferTexture(renderTargetObject_, nullptr);

    stencilTexture_ = texture;

    return true;
}

}
