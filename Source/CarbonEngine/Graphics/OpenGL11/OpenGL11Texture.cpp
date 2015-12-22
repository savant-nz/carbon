/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL11

#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

using namespace OpenGL11Extensions;

void OpenGL11::setupTextureFormats()
{
    textureFormats_[Image::Alpha8] = {GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA8};
    textureFormats_[Image::Luminance8] = {GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE8};
    textureFormats_[Image::LuminanceAlpha8] = {GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE8_ALPHA8};
    textureFormats_[Image::RGB8] = {GL_RGB, GL_UNSIGNED_BYTE, GL_RGB8};
    textureFormats_[Image::RGBA8] = {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8};
    textureFormats_[Image::RGB565] = {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB};
    textureFormats_[Image::RGBA5551] = {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA};
    textureFormats_[Image::RGBA4444] = {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4};
    textureFormats_[Image::Depth] = {GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT};

    if (extensions_.ARB_texture_float)
    {
        if (extensions_.ARB_half_float_pixel)
        {
            textureFormats_[Image::RGB16f] = {GL_RGB, GL_HALF_FLOAT_ARB, GL_RGB16F_ARB};
            textureFormats_[Image::RGBA16f] = {GL_RGBA, GL_HALF_FLOAT_ARB, GL_RGBA16F_ARB};

            if (extensions_.ARB_texture_rg)
            {
                textureFormats_[Image::Red16f] = {GL_RED_ARB, GL_HALF_FLOAT_ARB, GL_R16F_ARB};
                textureFormats_[Image::RedGreen16f] = {GL_RG_ARB, GL_HALF_FLOAT_ARB, GL_RG16F_ARB};
            }
        }

        textureFormats_[Image::RGB32f] = {GL_RGB, GL_FLOAT, GL_RGB32F_ARB};
        textureFormats_[Image::RGBA32f] = {GL_RGBA, GL_FLOAT, GL_RGBA32F_ARB};

        if (extensions_.ARB_texture_rg)
        {
            textureFormats_[Image::Red32f] = {GL_RED, GL_FLOAT, GL_R32F_ARB};
            textureFormats_[Image::RedGreen32f] = {GL_RG_ARB, GL_FLOAT, GL_RG32F_ARB};
        }
    }

    if (extensions_.EXT_abgr)
        textureFormats_[Image::ABGR8] = {GL_ABGR_EXT, GL_UNSIGNED_BYTE, GL_RGBA8};

    if (extensions_.EXT_bgra)
    {
        textureFormats_[Image::BGR8] = {GL_BGR_EXT, GL_UNSIGNED_BYTE, GL_RGB8};
        textureFormats_[Image::BGRA8] = {GL_BGRA_EXT, GL_UNSIGNED_BYTE, GL_RGBA8};
        textureFormats_[Image::ARGB1555] = {GL_BGRA_EXT, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_RGBA};
        textureFormats_[Image::ARGB4444] = {GL_BGRA_EXT, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_RGBA4};
    }

    if (extensions_.EXT_packed_depth_stencil)
        textureFormats_[Image::Depth24Stencil8] = {GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, GL_DEPTH24_STENCIL8_EXT};

    if (extensions_.EXT_texture_compression_s3tc)
    {
        textureFormats_[Image::DXT1] = {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT};
        textureFormats_[Image::DXT3] = {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT};
        textureFormats_[Image::DXT5] = {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT};
    }

    if (extensions_.EXT_texture_sRGB)
    {
        textureFormats_[Image::SRGB8] = {GL_RGB, GL_UNSIGNED_BYTE, GL_SRGB8_EXT};
        textureFormats_[Image::SRGBA8] = {GL_RGBA, GL_UNSIGNED_BYTE, GL_SRGB8_ALPHA8_EXT};
    }
}

unsigned int OpenGL11::getMaximumTextureSize(TextureType type) const
{
    if (type == Texture2D)
        return glGetUnsignedInteger(GL_MAX_TEXTURE_SIZE);

    if (type == Texture3D && extensions_.EXT_texture_3D)
        return glGetUnsignedInteger(GL_MAX_3D_TEXTURE_SIZE_EXT);

    if (type == TextureCubemap)
        return glGetUnsignedInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB);

    return 0;
}

unsigned int OpenGL11::getMaximumTextureAnisotropy(TextureType type) const
{
    if (!extensions_.EXT_texture_filter_anisotropic)
        return 1;

    return glGetUnsignedInteger(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
}

unsigned int OpenGL11::getTextureUnitCount() const
{
    return textureUnitCount_;
}

bool OpenGL11::isTextureSupported(TextureType type, const Image& image) const
{
    if (!glTextureTypeEnum[type])
        return false;

    if ((type == Texture2D && !image.isValid2DImage()) || (type == Texture3D && !image.isValid3DImage()) ||
        (type == TextureCubemap && !image.isValidCubemapImage()))
        return false;

    auto maximumTextureSize = getMaximumTextureSize(type);
    if (image.getWidth() > maximumTextureSize || image.getHeight() > maximumTextureSize ||
        image.getDepth() > maximumTextureSize)
        return false;

    if (image.isNPOT() && !isNonPowerOfTwoTextureSupported(type))
        return false;

    if (!isPixelFormatSupported(image.getPixelFormat(), type))
        return false;

    return true;
}

bool OpenGL11::isNonPowerOfTwoTextureSupported(TextureType type) const
{
    return extensions_.ARB_texture_non_power_of_two;
}

GraphicsInterface::TextureObject OpenGL11::createTexture()
{
    auto glTexture = GLuint();
    glGenTextures(1, &glTexture);
    CARBON_CHECK_OPENGL_ERROR(glGenTextures);
    return new Texture(glTexture);
}

void OpenGL11::deleteTexture(TextureObject texture)
{
    if (!texture)
        return;

    States::Texture.onGraphicsInterfaceObjectDelete(texture);

    // Clear the texture out of the render target texture caches
    for (auto renderTarget : renderTargets_)
    {
        for (auto& colorTexture : renderTarget->colorTextures)
        {
            if (colorTexture == texture)
                colorTexture = nullptr;
        }

        if (renderTarget->depthTexture == texture)
            renderTarget->depthTexture = nullptr;

        if (renderTarget->stencilTexture == texture)
            renderTarget->stencilTexture = nullptr;
    }

    // Delete the texture
    glDeleteTextures(1, &reinterpret_cast<Texture*>(texture)->glTexture);
    CARBON_CHECK_OPENGL_ERROR(glDeleteTextures);

    delete reinterpret_cast<Texture*>(texture);
}

bool OpenGL11::setTexture(unsigned int textureUnit, TextureObject textureObject)
{
    if (!textureObject)
        return true;

    if (activeTextureUnit_ != textureUnit)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + textureUnit);
        CARBON_CHECK_OPENGL_ERROR(glActiveTextureARB);

        activeTextureUnit_ = textureUnit;
    }

    auto texture = reinterpret_cast<Texture*>(textureObject);

    glBindTexture(glTextureTypeEnum[texture->type], texture->glTexture);
    CARBON_CHECK_OPENGL_ERROR(glBindTexture);

    return true;
}

bool OpenGL11::uploadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
                             const Vector<TextureData>& data)
{
    auto glInternalFormat = GLenum();
    auto glDataFormat = GLenum();
    auto glDataType = GLenum();
    if (!beginTextureUpload(texture, type, pixelFormat, glInternalFormat, glDataFormat, glDataType))
        return false;

    if (type == Texture2D)
    {
        for (auto i = 0U; i < data.size(); i++)
        {
            if (Image::isPixelFormatCompressed(pixelFormat))
            {
                glCompressedTexImage2DARB(GL_TEXTURE_2D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(), 0,
                                          data[i].getDataSize(), data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage2DARB);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(), 0, glDataFormat,
                             glDataType, data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glTexImage2D);
            }
        }
    }
    else if (type == Texture3D)
    {
        for (auto i = 0U; i < data.size(); i++)
        {
            if (Image::isPixelFormatCompressed(pixelFormat))
            {
                glCompressedTexImage3DARB(GL_TEXTURE_3D_EXT, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(),
                                          data[i].getDepth(), 0, data[i].getDataSize(), data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage3DARB);
            }
            else
            {
                glTexImage3DEXT(GL_TEXTURE_3D_EXT, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(),
                                data[i].getDepth(), 0, glDataFormat, glDataType, data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glTexImage3DEXT);
            }
        }
    }
    else if (type == TextureCubemap)
    {
        if (data.size() % 6)
            return false;

        auto mipmapCount = data.size() / 6;

        for (auto i = 0U; i < 6; i++)
        {
            for (auto j = 0U; j < mipmapCount; j++)
            {
                auto& d = data[i * mipmapCount + j];

                if (Image::isPixelFormatCompressed(pixelFormat))
                {
                    glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, j, glInternalFormat, d.getWidth(),
                                              d.getHeight(), 0, d.getDataSize(), d.getData());
                    CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage2DARB);
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, j, glInternalFormat, d.getWidth(), d.getHeight(), 0,
                                 glDataFormat, glDataType, d.getData());
                    CARBON_CHECK_OPENGL_ERROR(glTexImage2D);
                }
            }
        }
    }
    else
        return false;

    return true;
}

void OpenGL11::setTextureFilter(TextureObject texture, TextureType type, TextureFilter minFilter, TextureFilter magFilter)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MIN_FILTER, glTextureFilterEnum[minFilter]);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAG_FILTER, glTextureFilterEnum[magFilter]);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGL11::setTextureWrap(TextureObject texture, TextureType type, TextureWrap wrap)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    auto glWrap = GLenum();
    if (wrap == WrapClamp)
        glWrap = GL_CLAMP_TO_EDGE_EXT;
    else if (wrap == WrapRepeat)
        glWrap = GL_REPEAT;

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_S, glWrap);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_T, glWrap);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);

    if (type == Texture3D)
    {
        glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_R_EXT, glWrap);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    }
}

void OpenGL11::setTextureAnisotropy(TextureObject texture, TextureType type, unsigned int anisotropy)
{
    if (!extensions_.EXT_texture_filter_anisotropic)
        return;

    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    Math::clamp<unsigned int>(anisotropy, 1, getMaximumTextureAnisotropy(type)));
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGL11::setTextureBaseAndMaximumMipmapLevels(TextureObject texture, TextureType type, unsigned int baseLevel,
                                                    unsigned int maximumLevel)
{
    if (!extensions_.SGIS_texture_lod)
        return;

    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_BASE_LEVEL_SGIS, baseLevel);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAX_LEVEL_SGIS, maximumLevel);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGL11::setTextureIsShadowMap(TextureObject texture, bool isShadowMap)
{
    if (!extensions_.ARB_shadow)
        return;

    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    if (isShadowMap)
    {
        // Setup shadow map compare function
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    }
}

GLenum OpenGL11::getTextureInternalFormat(Image::PixelFormat pixelFormat, TextureType textureType) const
{
    // Check for hardware 3D texture support
    if (textureType == Texture3D && !extensions_.EXT_texture_3D)
        return 0;

    return OpenGLShared::getTextureInternalFormat(pixelFormat, textureType);
}

}

#endif
