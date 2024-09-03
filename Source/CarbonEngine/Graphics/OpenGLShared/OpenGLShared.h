/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLBase.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"

// In debug builds the OpenGL error state is checked after every relevant GL call and any errors are logged
#ifdef CARBON_DEBUG
    #define CARBON_CHECK_OPENGL_ERROR(FunctionName)                                            \
        do                                                                                     \
        {                                                                                      \
            auto glError = glGetError();                                                       \
            if (glError)                                                                       \
            {                                                                                  \
                LOG_ERROR << "OpenGL error " << Carbon::OpenGLShared::glErrorToString(glError) \
                          << " occurred in " #FunctionName;                                    \
                /* assert(false && "An OpenGL error occurred"); */                             \
            }                                                                                  \
            graphics().incrementAPICallCount();                                                \
        } while (false)
#else
    #define CARBON_CHECK_OPENGL_ERROR(FunctionName) ((void)0)
#endif

// Macros for declaring, defining and mapping OpenGL extension functions
#define CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(Function) extern PFn##Function Function
#define CARBON_OPENGL_DEFINE_EXTENSION_FUNCTION(Function) PFn##Function Function = nullptr
#define CARBON_OPENGL_MAP_EXTENSION_FUNCTION(Function) \
    Function = platform().getOpenGLFunctionAddress<PFn##Function>(#Function)

namespace Carbon
{

/**
 * This is a partial graphics interface backend that contains code shared by all of the OpenGL and OpenGL ES backends.
 */
class CARBON_API OpenGLShared : public OpenGLBase
{
public:

    bool setup() override
    {
        // Use tightly packed pixel alignments everywhere
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        CARBON_CHECK_OPENGL_ERROR(glPixelStorei);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        CARBON_CHECK_OPENGL_ERROR(glPixelStorei);

        activeTextureUnit_ = 0;
        activeVertexDataBuffer_ = nullptr;
        activeIndexDataBuffer_.clear();

        setupTextureFormats();

        return OpenGLBase::setup();
    }

    void clearBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) override
    {
        States::RenderTarget.flush();
        States::ScissorEnabled.flush();
        States::ScissorRectangle.flush();

        if (colorBuffer)
            States::ClearColor.flush();

        if (depthBuffer)
        {
            States::DepthClearValue.flush();
            States::DepthWriteEnabled.pushSetFlushPop(true);
        }

        if (stencilBuffer)
        {
            States::StencilClearValue.flush();
            States::StencilWriteEnabled.pushSetFlushPop(true);
        }

        glClear((colorBuffer ? GL_COLOR_BUFFER_BIT : 0) | (depthBuffer ? GL_DEPTH_BUFFER_BIT : 0) |
                (stencilBuffer ? GL_STENCIL_BUFFER_BIT : 0));
        CARBON_CHECK_OPENGL_ERROR(glClear);
    }

    void setClearColor(const Color& color) override
    {
        glClearColor(color.r, color.g, color.b, color.a);
        CARBON_CHECK_OPENGL_ERROR(glClearColor);
    }

    void setDepthTestEnabled(bool enabled) override { setEnabled(GL_DEPTH_TEST, enabled); }

    void setDepthWriteEnabled(bool enabled) override
    {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
        CARBON_CHECK_OPENGL_ERROR(glDepthMask);
    }

    void setDepthCompareFunction(States::CompareFunction compare) override
    {
        glDepthFunc(glCompareFunctionEnum[compare]);
        CARBON_CHECK_OPENGL_ERROR(glDepthFunc);
    }

    void setCullMode(States::CullingMode mode) override
    {
        if ((mode == States::CullingDisabled) == isCullingEnabled_)
        {
            isCullingEnabled_ = (mode != States::CullingDisabled);
            setEnabled(GL_CULL_FACE, isCullingEnabled_);
        }

        if (mode == States::CullFrontFaces)
            glCullFace(GL_FRONT);
        else if (mode == States::CullBackFaces)
            glCullFace(GL_BACK);

        CARBON_CHECK_OPENGL_ERROR(glCullFace);
    }

    void setBlendEnabled(bool enabled) override { setEnabled(GL_BLEND, enabled); }

    void setBlendFunction(const States::BlendFunctionSetup& function) override
    {
        glBlendFunc(glBlendFactorEnum[function.getSourceFactor()], glBlendFactorEnum[function.getDestinationFactor()]);
        CARBON_CHECK_OPENGL_ERROR(glBlendFunc);
    }

    void setViewport(const Rect& viewport) override
    {
        // Use a floor function on the viewport coordinates to avoid roundoff errors, this ensures that viewports right
        // next to each other on subpixel boundaries will not have any pixel cracks between them

        auto left = floorf(viewport.getLeft());
        auto bottom = floorf(viewport.getBottom());
        auto width = floorf(viewport.getRight()) - left;
        auto height = floorf(viewport.getTop()) - bottom;

        glViewport(GLint(left), GLint(bottom), GLsizei(width), GLsizei(height));
        CARBON_CHECK_OPENGL_ERROR(glViewport);
    }

    void setScissorEnabled(bool enabled) override { setEnabled(GL_SCISSOR_TEST, enabled); }

    void setScissorRectangle(const Rect& scissor) override
    {
        auto left = floorf(scissor.getLeft());
        auto bottom = floorf(scissor.getBottom());
        auto width = floorf(scissor.getRight()) - left;
        auto height = floorf(scissor.getTop()) - bottom;

        glScissor(GLint(left), GLint(bottom), GLsizei(width), GLsizei(height));
        CARBON_CHECK_OPENGL_ERROR(glScissor);
    }

    void setColorWriteEnabled(bool enabled) override
    {
        glColorMask(enabled, enabled, enabled, enabled);
        CARBON_CHECK_OPENGL_ERROR(glColorMask);
    }

    void setStencilTestEnabled(bool enabled) override
    {
        if (!isStencilBufferSupported())
            return;

        setEnabled(GL_STENCIL_TEST, enabled);
    }

    void setStencilTestFunction(const States::StencilTestSetup& function) override
    {
        if (!isStencilBufferSupported())
            return;

        glStencilFunc(glCompareFunctionEnum[function.getCompareFunction()], function.getReferenceValue(),
                      function.getMask());
        CARBON_CHECK_OPENGL_ERROR(glStencilFunc);
    }

    void setStencilWriteEnabled(bool enabled) override
    {
        if (!isStencilBufferSupported())
            return;

        glStencilMask(enabled ? ~GLuint(0) : 0);
        CARBON_CHECK_OPENGL_ERROR(glStencilMask);
    }

    void setStencilClearValue(unsigned int clearValue) override
    {
        if (!isStencilBufferSupported())
            return;

        glClearStencil(clearValue);
        CARBON_CHECK_OPENGL_ERROR(glClearStencil);
    }

    static String glErrorToString(GLenum glError)
    {
        switch (glError)
        {
            case GL_NO_ERROR:
                return "GL_NO_ERROR";
            case GL_INVALID_ENUM:
                return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:
                return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:
                return "GL_INVALID_OPERATION";
            case GL_OUT_OF_MEMORY:
                return "GL_OUT_OF_MEMORY";

#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                return "GL_INVALID_FRAMEBUFFER_OPERATION";
#elif defined(GL_INVALID_FRAMEBUFFER_OPERATION_EXT)
            case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
                return "GL_INVALID_FRAMEBUFFER_OPERATION_EXT";
#endif

            default:
                return glError;
        }
    }

    bool isPixelFormatSupported(Image::PixelFormat pixelFormat, TextureType type) const override
    {
        return getTextureInternalFormat(pixelFormat, type) != 0;
    }

#ifdef GL_TEXTURE_WIDTH
    bool downloadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat, Image& image) override
    {
        // TODO: this method is not compatible with OpenGL ES 2+, it needs to be reworked to attach the specified
        // texture to a temporary render target and then read the contents back using glReadPixels().

        image.clear();

        // Can't download into compressed formats
        if (Image::isPixelFormatCompressed(pixelFormat))
            return false;

        States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

        if (type == Texture2D)
        {
            auto width = GLint();
            auto height = GLint();
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            CARBON_CHECK_OPENGL_ERROR(glGetTexLevelParameteriv);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
            CARBON_CHECK_OPENGL_ERROR(glGetTexLevelParameteriv);

            if (!image.initialize(width, height, 1, pixelFormat, false, 1))
                return false;

            glGetTexImage(GL_TEXTURE_2D, 0, textureFormats_[pixelFormat].glDataFormat,
                          textureFormats_[pixelFormat].glDataType, image.getDataForFrame(0));
            CARBON_CHECK_OPENGL_ERROR(glGetTexImage);
        }
        else
            return false;

        return true;
    }
#endif

protected:

    const std::array<GLenum, 10> glBlendFactorEnum = {{GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
                                                       GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,
                                                       GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA}};

    const std::array<GLenum, 8> glCompareFunctionEnum = {
        {GL_NEVER, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS}};

    const std::array<GLenum, 11> glDataTypeEnum = {{0, GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT,
                                                    GL_UNSIGNED_INT, 0, 0, GL_FLOAT, GL_DOUBLE}};

// Ensure GL_INCR_WRAP and GL_DECR_WRAP are available
#ifndef GL_INCR_WRAP
    #ifdef GL_INCR_WRAP_EXT
        #define GL_INCR_WRAP GL_INCR_WRAP_EXT
        #define GL_DECR_WRAP GL_DECR_WRAP_EXT
    #else
        #define GL_INCR_WRAP 0
        #define GL_DECR_WRAP 0
    #endif
#endif

    const std::array<GLenum, 8> glStencilOpEnum = {
        {GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INCR_WRAP, GL_DECR_WRAP, GL_INVERT}};

    // Hardware limits
    unsigned int textureUnitCount_ = 0;
    unsigned int vertexAttributeCount_ = 0;

    bool isCullingEnabled_ = false;
    unsigned int activeTextureUnit_ = 0;

    void logString(GLenum pname, const String& prefix)
    {
        auto string = reinterpret_cast<const char*>(glGetString(pname));
        CARBON_CHECK_OPENGL_ERROR(glGetString);

        LOG_INFO << prefix << (string ? string : "null");
    }

    unsigned int glGetUnsignedInteger(GLenum pname) const
    {
        auto i = GLint();
        glGetIntegerv(pname, &i);
        CARBON_CHECK_OPENGL_ERROR(glGetIntegerv);
        return uint(i);
    }

    void setEnabled(GLenum pname, bool enabled)
    {
        if (enabled)
        {
            glEnable(pname);
            CARBON_CHECK_OPENGL_ERROR(glEnable);
        }
        else
        {
            glDisable(pname);
            CARBON_CHECK_OPENGL_ERROR(glDisable);
        }
    }

    bool beginTextureUpload(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
                            GLenum& glInternalFormat, GLenum& glDataFormat, GLenum& glDataType)
    {
        glInternalFormat = getTextureInternalFormat(pixelFormat, type);
        if (!glInternalFormat)
            return false;

        if (Image::isPixelFormatUncompressed(pixelFormat))
        {
            // If the image is not compressed we need the format and data type of the data that is going to be passed to
            // OpenGL so that it knows how to interpret the data it gets given
            glDataFormat = textureFormats_[pixelFormat].glDataFormat;
            glDataType = textureFormats_[pixelFormat].glDataType;

            if (!glDataFormat || !glDataType)
                return false;
        }

        reinterpret_cast<Texture*>(texture)->type = type;

        States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

        return true;
    }

    struct TextureFormat
    {
        GLenum glDataFormat = 0;
        GLenum glDataType = 0;
        GLenum glInternalFormat = 0;

        TextureFormat() {}
        TextureFormat(GLenum glDataFormat_, GLenum glDataType_, GLenum glInternalFormat_)
            : glDataFormat(glDataFormat_), glDataType(glDataType_), glInternalFormat(glInternalFormat_)
        {
        }
    };

    std::array<TextureFormat, Image::LastPixelFormat> textureFormats_;

    // Subclasses can override this method to support different image formats for different texture types
    virtual GLenum getTextureInternalFormat(Image::PixelFormat pixelFormat, TextureType textureType) const
    {
        return textureFormats_[pixelFormat].glInternalFormat;
    }

    virtual void setupTextureFormats() = 0;

    // The internal texture object, maps to an OpenGL texture object
    struct Texture
    {
        GLuint glTexture = 0;
        TextureType type = TextureNone;

        Texture(GLuint glTexture_, TextureType type_ = TextureNone) : glTexture(glTexture_), type(type_) {}
    };

    static GLuint getOpenGLTexture(TextureObject texture)
    {
        return texture ? reinterpret_cast<Texture*>(texture)->glTexture : 0;
    }

    // The internal data buffer object, maps to an OpenGL vertex buffer object
    struct DataBuffer
    {
        GLuint glBuffer = 0;
        unsigned int size = 0;
        bool isDynamic = false;

        DataBuffer(GLuint glBuffer_) : glBuffer(glBuffer_) {}
    };

    // VBO and VAO support
    const DataBuffer* activeVertexDataBuffer_ = nullptr;
    std::unordered_map<VertexAttributeArrayConfigurationObject, const DataBuffer*> activeIndexDataBuffer_;

    // The internal render target object, maps to an OpenGL framebuffer object
    struct RenderTarget
    {
        GLuint glFramebuffer = 0;

        Vector<TextureObject> colorTextures;
        Vector<GLenum> colorTextureTargets;
        TextureObject depthTexture = nullptr;
        TextureObject stencilTexture = nullptr;

        RenderTarget(GLuint glFramebuffer_, unsigned int maximumDrawBuffers_)
            : glFramebuffer(glFramebuffer_),
              colorTextures(maximumDrawBuffers_),
              colorTextureTargets(maximumDrawBuffers_)
        {
        }
    };
    Vector<RenderTarget*> renderTargets_;
};

}
