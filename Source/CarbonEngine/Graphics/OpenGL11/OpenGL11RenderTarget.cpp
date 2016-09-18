/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL11

#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

using namespace OpenGL11Extensions;

bool OpenGL11::isRenderTargetSupported() const
{
    return extensions_.EXT_framebuffer_object;
}

GraphicsInterface::RenderTargetObject OpenGL11::createRenderTarget()
{
    if (!extensions_.EXT_framebuffer_object)
        return nullptr;

    auto glFramebuffer = GLuint();
    glGenFramebuffersEXT(1, &glFramebuffer);
    CARBON_CHECK_OPENGL_ERROR(glGenFramebuffersEXT);

    renderTargets_.append(new RenderTarget(glFramebuffer, maximumDrawBuffers_));

    return renderTargets_.back();
}

void OpenGL11::deleteRenderTarget(RenderTargetObject renderTargetObject)
{
    if (!extensions_.EXT_framebuffer_object || !renderTargetObject)
        return;

    States::RenderTarget.onGraphicsInterfaceObjectDelete(renderTargetObject);

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    if (!renderTargets_.unorderedEraseValue(renderTarget))
        LOG_WARNING << "Unknown render target object: " << renderTargetObject;

    glDeleteFramebuffersEXT(1, &renderTarget->glFramebuffer);
    CARBON_CHECK_OPENGL_ERROR(glDeleteFramebuffersEXT);

    delete renderTarget;
    renderTarget = nullptr;
}

bool OpenGL11::setRenderTargetColorBufferTextures(RenderTargetObject renderTargetObject,
                                                  const Vector<TextureObject>& textures,
                                                  const Vector<int>& cubemapFaces)
{
    if (!extensions_.EXT_framebuffer_object || !renderTargetObject)
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
            textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + cubemapFaces[i];

        // Check whether the hardware state needs updating
        if (renderTarget->colorTextures[i] != colorTexture || renderTarget->colorTextureTargets[i] != textureTarget)
        {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, textureTarget,
                                      getOpenGLTexture(colorTexture), 0);
            CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2DEXT);

            renderTarget->colorTextures[i] = colorTexture;
            renderTarget->colorTextureTargets[i] = textureTarget;
        }

        if (renderTarget->colorTextures[i])
            drawBuffers[i] = GL_COLOR_ATTACHMENT0_EXT + i;
    }

    // Update the draw buffers appropriately
    if (extensions_.ARB_draw_buffers)
    {
        glDrawBuffersARB(maximumDrawBuffers_, drawBuffers.getData());
        CARBON_CHECK_OPENGL_ERROR(glDrawBuffersARB);
    }
    else
    {
        glDrawBuffer(textures.size() ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE);
        CARBON_CHECK_OPENGL_ERROR(glDrawBuffer);
    }

    // Set the read buffer to the first color attachment (i.e. first texture) if it is specified, otherwise there is no
    // read buffer
    glReadBuffer(textures.size() ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE);
    CARBON_CHECK_OPENGL_ERROR(glReadBuffer);

    return true;
}

unsigned int OpenGL11::getMaximumRenderTargetColorTextures() const
{
    return maximumDrawBuffers_;
}

bool OpenGL11::setRenderTargetDepthBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture)
{
    if (!extensions_.EXT_framebuffer_object || !renderTargetObject)
        return false;

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    if (renderTarget->depthTexture != texture)
    {
        States::RenderTarget.pushSetFlushPop(renderTargetObject);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, getOpenGLTexture(texture),
                                  0);
        CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2DEXT);

        renderTarget->depthTexture = texture;
    }

    return true;
}

bool OpenGL11::setRenderTargetStencilBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture)
{
    if (!extensions_.EXT_framebuffer_object || !renderTargetObject)
        return false;

    auto renderTarget = reinterpret_cast<RenderTarget*>(renderTargetObject);

    if (renderTarget->stencilTexture != texture)
    {
        States::RenderTarget.pushSetFlushPop(renderTargetObject);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D,
                                  getOpenGLTexture(texture), 0);
        CARBON_CHECK_OPENGL_ERROR(glFramebufferTexture2DEXT);

        renderTarget->stencilTexture = texture;
    }

    return true;
}

bool OpenGL11::isRenderTargetValid(RenderTargetObject renderTargetObject) const
{
    if (!extensions_.EXT_framebuffer_object || !renderTargetObject)
        return false;

    States::RenderTarget.pushSetFlushPop(renderTargetObject);

    auto status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    CARBON_CHECK_OPENGL_ERROR(glCheckFramebufferStatusEXT);

#ifdef CARBON_DEBUG
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        auto statusString = String();

        if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT)
            statusString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT";
        else if (status == GL_FRAMEBUFFER_UNSUPPORTED_EXT)
            statusString = "GL_FRAMEBUFFER_UNSUPPORTED_EXT";

        LOG_DEBUG << "OpenGL render target is not valid, status: " << statusString;
    }
#endif

    return status == GL_FRAMEBUFFER_COMPLETE_EXT;
}

void OpenGL11::setRenderTarget(RenderTargetObject renderTargetObject)
{
    if (!extensions_.EXT_framebuffer_object)
        return;

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                         renderTargetObject ? reinterpret_cast<RenderTarget*>(renderTargetObject)->glFramebuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindFramebufferEXT);
}

#ifdef CARBON_INCLUDE_OCULUSRIFT

GraphicsInterface::RenderTargetObject OpenGL11::getOutputDestinationRenderTarget(OutputDestination destination)
{
    if (destination == OutputDefault)
        return nullptr;

    auto session = platform().getPlatformSpecificValue<ovrSession>(PlatformInterface::OculusRiftSession);
    auto viewport = getOutputDestinationViewport(destination);

    for (auto& eye : oculusRiftEyes_)
    {
        if (!eye.swapTextureSet)
        {
            auto result = ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8_EXT, int(viewport.getWidth()),
                                                     int(viewport.getHeight()), &eye.swapTextureSet);

            if (!OVR_SUCCESS(result))
            {
                LOG_ERROR << "Failed creating Oculus Rift swap texture set with dimensions " << viewport.getWidth()
                          << "x" << viewport.getHeight();
                continue;
            }
            else
            {
                for (auto i = 0; i < eye.swapTextureSet->TextureCount; i++)
                {
                    auto texture = reinterpret_cast<ovrGLTexture*>(&eye.swapTextureSet->Textures[i]);
                    glBindTexture(GL_TEXTURE_2D, texture->OGL.TexId);

                    // Set texture filter and wrap modes
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE_EXT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE_EXT);

                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
        }

        if (!eye.depthTexture)
        {
            eye.depthTexture = createTexture();
            if (!uploadTexture(eye.depthTexture, Texture2D, Image::Depth,
                               {{uint(viewport.getWidth()), uint(viewport.getHeight()), 1U, nullptr, 0U}}))
                LOG_WARNING << "Failed creating depth texture";
        }

        if (!eye.renderTarget)
        {
            eye.renderTarget = createRenderTarget();
            setRenderTargetDepthBufferTexture(eye.renderTarget, eye.depthTexture);
        }
    }

    if (destination == OutputOculusRiftLeftEye || destination == OutputOculusRiftRightEye)
    {
        auto& eye = oculusRiftEyes_[destination == OutputOculusRiftLeftEye ? ovrEye_Left : ovrEye_Right];

        auto texture = Texture(
            reinterpret_cast<ovrGLTexture*>(&eye.swapTextureSet->Textures[eye.swapTextureSet->CurrentIndex])->OGL.TexId,
            GraphicsInterface::Texture2D);

        setRenderTargetColorBufferTextures(eye.renderTarget, {&texture}, {});

        return eye.renderTarget;
    }

    return nullptr;
}

Rect OpenGL11::getOutputDestinationViewport(OutputDestination destination) const
{
    if (destination == OutputOculusRiftLeftEye || destination == OutputOculusRiftRightEye)
        return platform().getOculusRiftTextureDimensions();

    return OpenGLShared::getOutputDestinationViewport(destination);
}

void OpenGL11::flushOutputDestination(OutputDestination destination)
{
    for (auto& eye : oculusRiftEyes_)
    {
        if (eye.renderTarget)
            setRenderTargetColorBufferTextures(eye.renderTarget, {}, {});
    }
}

#endif
}

#endif
