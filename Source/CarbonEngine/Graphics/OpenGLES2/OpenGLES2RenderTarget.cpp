/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGLES2

#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

bool OpenGLES2::isRenderTargetSupported() const
{
    return true;
}

GraphicsInterface::RenderTargetObject OpenGLES2::createRenderTarget()
{
    auto glFramebuffer = GLuint();
    glGenFramebuffers(1, &glFramebuffer);
    CARBON_CHECK_OPENGL_ERROR(glGenFramebuffers);

    renderTargets_.append(new RenderTarget(glFramebuffer, getMaximumRenderTargetColorTextures()));

    return renderTargets_.back();
}

void OpenGLES2::deleteRenderTarget(RenderTargetObject renderTargetObject)
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

bool OpenGLES2::setRenderTargetColorBufferTextures(RenderTargetObject renderTargetObject, const Vector<TextureObject>& textures,
                                                   const Vector<int>& cubemapFaces)
{
    if (!renderTargetObject)
        return false;

    if (textures.size() > getMaximumRenderTargetColorTextures())
    {
        LOG_ERROR << "Draw buffer configuration not supported";
        return false;
    }

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    // Attach the textures to the framebuffer object
    auto texture = textures.size() ? textures[0] : nullptr;

    auto textureTarget = GLenum(GL_TEXTURE_2D);

    // If this output is going into a cubemap face then set the corresponding texture target. Only cubemapFaces[0] is
    // checked here because OpenGL ES 2 doesn't support single-pass render-to-cubemap.
    if (cubemapFaces.size() && cubemapFaces[0] >= 0 && cubemapFaces[0] < 6)
        textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubemapFaces[0];

    if (renderTarget->colorTextures[0] != texture || renderTarget->colorTextureTargets[0] != textureTarget)
    {
        States::RenderTarget.pushSetFlushPop(renderTargetObject);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, getOpenGLTexture(texture), 0);
        CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2D);

        renderTarget->colorTextures[0] = texture;
        renderTarget->colorTextureTargets[0] = textureTarget;
    }

    return true;
}

unsigned int OpenGLES2::getMaximumRenderTargetColorTextures() const
{
    return 1;
}

bool OpenGLES2::setRenderTargetDepthBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture)
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

bool OpenGLES2::setRenderTargetStencilBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture)
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

bool OpenGLES2::isRenderTargetValid(RenderTargetObject renderTargetObject) const
{
    if (!renderTargetObject)
        return false;

    States::RenderTarget.pushSetFlushPop(renderTargetObject);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    CARBON_CHECK_OPENGL_ERROR(glCheckFramebufferStatus);

#ifdef CARBON_DEBUG
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        auto statusString = "";

        if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
        else if (status == GL_FRAMEBUFFER_UNSUPPORTED)
            statusString = "GL_FRAMEBUFFER_UNSUPPORTED";

        LOG_DEBUG << "OpenGL render target is not valid, status: " << statusString;
    }
#endif

    return status == GL_FRAMEBUFFER_COMPLETE;
}

void OpenGLES2::setRenderTarget(RenderTargetObject renderTargetObject)
{
    glBindFramebuffer(GL_FRAMEBUFFER,
                      renderTargetObject ? reinterpret_cast<RenderTarget*>(renderTargetObject)->glFramebuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindFramebuffer);
}

}

#endif
