/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGLES2

#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2Extensions.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

void OpenGLES2::setupTextureFormats()
{
    textureFormats_[Image::Alpha8] = {GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA};
    textureFormats_[Image::Luminance8] = {GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE};
    textureFormats_[Image::LuminanceAlpha8] = {GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE_ALPHA};
    textureFormats_[Image::RGB8] = {GL_RGB, GL_UNSIGNED_BYTE, GL_RGB};
    textureFormats_[Image::RGBA8] = {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA};
    textureFormats_[Image::RGB565] = {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB};
    textureFormats_[Image::RGBA5551] = {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA};
    textureFormats_[Image::RGBA4444] = {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA};

    if (extensions_.APPLE_texture_format_BGRA8888)
        textureFormats_[Image::RGBA4444] = {0x80E1, GL_UNSIGNED_BYTE, GL_RGBA};

    if (extensions_.EXT_sRGB)
    {
        textureFormats_[Image::SRGB8] = {GL_SRGB_EXT, GL_UNSIGNED_BYTE, GL_SRGB_EXT};
        textureFormats_[Image::SRGBA8] = {GL_SRGB_ALPHA_EXT, GL_UNSIGNED_BYTE, GL_SRGB_ALPHA_EXT};
    }

    if (extensions_.EXT_texture_compression_dxt1)
        textureFormats_[Image::Depth] = {0, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT};

    if (extensions_.IMG_texture_compression_pvrtc)
    {
        textureFormats_[Image::PVRTC2BitRGB] = {0, 0, GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG};
        textureFormats_[Image::PVRTC2BitRGBA] = {0, 0, GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG};
        textureFormats_[Image::PVRTC4BitRGB] = {0, 0, GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG};
        textureFormats_[Image::PVRTC4BitRGBA] = {0, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG};
    }

    if (extensions_.OES_depth_texture)
        textureFormats_[Image::Depth] = {GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT};

    if (extensions_.OES_packed_depth_stencil)
        textureFormats_[Image::Depth24Stencil8] = {GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES, GL_DEPTH_STENCIL_OES};
}

unsigned int OpenGLES2::getMaximumTextureSize(TextureType type) const
{
    if (type == Texture2D)
        return glGetUnsignedInteger(GL_MAX_TEXTURE_SIZE);

    if (type == TextureCubemap)
        return glGetUnsignedInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE);

    return 0;
}

unsigned int OpenGLES2::getTextureUnitCount() const
{
    return textureUnitCount_;
}

bool OpenGLES2::isTextureSupported(TextureType type, const Image& image) const
{
    if (!glTextureTypeEnum[type])
        return false;

    if ((type == Texture2D && !image.isValid2DImage()) || (type == TextureCubemap && !image.isValidCubemapImage()))
        return false;

    auto maximumTextureSize = getMaximumTextureSize(type);
    if (image.getWidth() > maximumTextureSize || image.getHeight() > maximumTextureSize)
        return false;

    if (!getTextureInternalFormat(image.getPixelFormat(), type))
        return false;

    if (image.isNPOT() && !isNonPowerOfTwoTextureSupported(type))
        return false;

    if (Image::isPixelFormatPVRTCCompressed(image.getPixelFormat()) && image.getWidth() != image.getHeight())
        return false;

    return true;
}

bool OpenGLES2::isNonPowerOfTwoTextureSupported(TextureType type) const
{
    // NOTE: NPOT requires that clamp wrapping be used and that minification filtering is either nearest or linear, these
    // restrictions mean that it's only really useful for doing offscreen rendering.

    return type == Texture2D;
}

GraphicsInterface::TextureObject OpenGLES2::createTexture()
{
    auto glTexture = GLuint();

    glGenTextures(1, &glTexture);
    CARBON_CHECK_OPENGL_ERROR(glGenTextures);

    return new Texture(glTexture);
}

void OpenGLES2::deleteTexture(TextureObject texture)
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

bool OpenGLES2::setTexture(unsigned int textureUnit, TextureObject textureObject)
{
    if (!textureObject)
        return true;

    if (activeTextureUnit_ != textureUnit)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        CARBON_CHECK_OPENGL_ERROR(glActiveTexture);

        activeTextureUnit_ = textureUnit;
    }

    auto texture = reinterpret_cast<Texture*>(textureObject);

    glBindTexture(glTextureTypeEnum[texture->type], texture->glTexture);
    CARBON_CHECK_OPENGL_ERROR(glBindTexture);

    return true;
}

bool OpenGLES2::uploadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
                              const Vector<TextureData>& data)
{
    auto glInternalFormat = GLenum();
    auto glDataFormat = GLenum();
    auto glDataType = GLenum();
    if (!beginTextureUpload(texture, Texture2D, pixelFormat, glInternalFormat, glDataFormat, glDataType))
        return false;

    if (type == Texture2D)
    {
        for (auto i = 0U; i < data.size(); i++)
        {
            if (Image::isPixelFormatCompressed(pixelFormat))
            {
                glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(), 0,
                                       data[i].getDataSize(), data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage2D);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(), 0, glDataFormat,
                             glDataType, data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glTexImage2D);
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
                    glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, glInternalFormat, d.getWidth(), d.getHeight(),
                                           0, d.getDataSize(), d.getData());
                    CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage2D);
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, glInternalFormat, d.getWidth(), d.getHeight(), 0,
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

void OpenGLES2::setTextureFilter(TextureObject texture, TextureType type, TextureFilter minFilter, TextureFilter magFilter)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MIN_FILTER, glTextureFilterEnum[minFilter]);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAG_FILTER, glTextureFilterEnum[magFilter]);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGLES2::setTextureWrap(TextureObject texture, TextureType type, TextureWrap wrap)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    auto glWrap = GLenum(wrap == WrapClamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_S, glWrap);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_T, glWrap);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

GLenum OpenGLES2::getTextureInternalFormat(Image::PixelFormat pixelFormat, TextureType textureType) const
{
    if (textureType != Texture2D && textureType != TextureCubemap)
        return 0;

    return OpenGLShared::getTextureInternalFormat(pixelFormat, textureType);
}

}

#endif
