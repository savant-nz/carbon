/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL41

#include "CarbonEngine/Graphics/OpenGL41/OpenGL41.h"
#include "CarbonEngine/Graphics/OpenGL41/OpenGL41Extensions.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

bool OpenGL41::isRenderTargetSupported() const
{
    return true;
}

GraphicsInterface::RenderTargetObject OpenGL41::createRenderTarget()
{
    auto glFramebuffer = GLuint();
    glGenFramebuffers(1, &glFramebuffer);
    CARBON_CHECK_OPENGL_ERROR(glGenFramebuffers);

    renderTargets_.append(new RenderTarget(glFramebuffer, maximumDrawBuffers_));

    return renderTargets_.back();
}

void OpenGL41::deleteRenderTarget(RenderTargetObject renderTargetObject)
{
    if (!renderTargetObject)
        return;

    States::RenderTarget.onGraphicsInterfaceObjectDelete(renderTargetObject);

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    if (!renderTargets_.unorderedEraseValue(renderTarget))
        LOG_WARNING << "Unknown render target object: " << renderTargetObject;

    glDeleteFramebuffers(1, &renderTarget->glFramebuffer);
    CARBON_CHECK_OPENGL_ERROR(glDeleteFramebuffers);

    delete renderTarget;
    renderTarget = nullptr;
}

bool OpenGL41::setRenderTargetColorBufferTextures(RenderTargetObject renderTargetObject, const Vector<TextureObject>& textures,
                                                  const Vector<int>& cubemapFaces)
{
    if (!renderTargetObject)
        return false;

    if (textures.size() > maximumDrawBuffers_)
    {
        LOG_ERROR << "Draw buffer configuration not supported";
        return false;
    }

    States::RenderTarget.pushSetFlushPop(renderTargetObject);

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    // Attach the textures to the framebuffer object
    auto drawBuffers = Vector<GLenum>(maximumDrawBuffers_, GL_NONE);
    for (auto i = 0U; i < maximumDrawBuffers_; i++)
    {
        auto colorTexture = textures.size() > i ? textures[i] : nullptr;

        auto textureTarget = GLenum(GL_TEXTURE_2D);

        // If this output is going into a cubemap face then set the corresponding texture target
        if (i < cubemapFaces.size() && cubemapFaces[i] >= 0 && cubemapFaces[i] < 6)
            textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubemapFaces[i];

        // Check whether the hardware state needs updating
        if (renderTarget->colorTextures[i] != colorTexture || renderTarget->colorTextureTargets[i] != textureTarget)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textureTarget, getOpenGLTexture(colorTexture), 0);
            CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2D);

            renderTarget->colorTextures[i] = colorTexture;
            renderTarget->colorTextureTargets[i] = textureTarget;
        }

        if (renderTarget->colorTextures[i])
            drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    // Update the draw buffers appropriately
    glDrawBuffers(maximumDrawBuffers_, drawBuffers.getData());
    CARBON_CHECK_OPENGL_ERROR(glDrawBuffers);

    // Set the read buffer to the first color attachment (i.e. first texture) if it is specified, otherwise there is no read
    // buffer
    glReadBuffer(textures.size() ? GL_COLOR_ATTACHMENT0 : GL_NONE);
    CARBON_CHECK_OPENGL_ERROR(glReadBuffer);

    return true;
}

unsigned int OpenGL41::getMaximumRenderTargetColorTextures() const
{
    return maximumDrawBuffers_;
}

bool OpenGL41::setRenderTargetDepthBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture)
{
    if (!renderTargetObject)
        return false;

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    if (renderTarget->depthTexture != texture)
    {
        States::RenderTarget.pushSetFlushPop(renderTargetObject);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, getOpenGLTexture(texture), 0);
        CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2D);

        renderTarget->depthTexture = texture;
    }

    return true;
}

bool OpenGL41::setRenderTargetStencilBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture)
{
    if (!renderTargetObject)
        return false;

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    if (renderTarget->stencilTexture != texture)
    {
        States::RenderTarget.pushSetFlushPop(renderTargetObject);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, getOpenGLTexture(texture), 0);
        CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2D);

        renderTarget->stencilTexture = texture;
    }

    return true;
}

bool OpenGL41::isRenderTargetValid(RenderTargetObject renderTargetObject) const
{
    if (!renderTargetObject)
        return false;

    States::RenderTarget.pushSetFlushPop(renderTargetObject);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    CARBON_CHECK_OPENGL_ERROR(glCheckFramebufferStatus);

#ifdef CARBON_DEBUG
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        auto statusString = String();

        if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        else if (status == GL_FRAMEBUFFER_UNDEFINED)
            statusString = "GL_FRAMEBUFFER_UNDEFINED";
        else if (status == GL_FRAMEBUFFER_UNSUPPORTED)
            statusString = "GL_FRAMEBUFFER_UNSUPPORTED";

        LOG_DEBUG << "OpenGL render target is not valid, status: " << statusString;
    }
#endif

    return status == GL_FRAMEBUFFER_COMPLETE;
}

void OpenGL41::setRenderTarget(RenderTargetObject renderTargetObject)
{
    glBindFramebuffer(GL_FRAMEBUFFER,
                      renderTargetObject ? reinterpret_cast<RenderTarget*>(renderTargetObject)->glFramebuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindFramebuffer);
}

}

#endif
