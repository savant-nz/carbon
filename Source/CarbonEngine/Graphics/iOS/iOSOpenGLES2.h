/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef iOS

#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

/**
 * A slightly modified version of the generic OpenGL ES 2 graphics backend for use on iOS.
 */
class CARBON_API iOSOpenGLES2 : public OpenGLES2
{
public:

    // OpenGLES2::setRenderTarget() is overridden here so that when a null RenderTargetObject is set it can be mapped to the
    // framebuffer that was created in PlatformiOS.mm

    void setRenderTarget(RenderTargetObject renderTargetObject) override
    {
        auto fbo = GLuint();

        if (renderTargetObject)
            fbo = reinterpret_cast<RenderTarget*>(renderTargetObject)->glFramebuffer;
        else
            fbo = GLuint(platform().getPlatformSpecificValue(PlatformInterface::iOSOpenGLESFramebuffer));

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        CARBON_CHECK_OPENGL_ERROR(glBindFramebuffer);
    }

    // Support framebuffer discard on iOS in order to save memory bandwidth

    void discardRenderTargetBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) override
    {
        auto attachments = Vector<GLenum>();

        if (colorBuffer)
            attachments.append(GL_COLOR_ATTACHMENT0);
        if (depthBuffer)
            attachments.append(GL_DEPTH_ATTACHMENT);
        if (stencilBuffer)
            attachments.append(GL_STENCIL_ATTACHMENT);

        glDiscardFramebufferEXT(GL_FRAMEBUFFER, attachments.size(), attachments.getData());
        CARBON_CHECK_OPENGL_ERROR(glDiscardFramebufferEXT);
    }
};

}

#endif
