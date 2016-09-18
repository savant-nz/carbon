/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL41

#include "CarbonEngine/Graphics/OpenGL41/OpenGL41.h"
#include "CarbonEngine/Graphics/OpenGL41/OpenGL41Extensions.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

void OpenGL41::setupTextureFormats()
{
    textureFormats_[Image::RGB8] = {GL_RGB, GL_UNSIGNED_BYTE, GL_RGB8};
    textureFormats_[Image::BGR8] = {GL_BGR, GL_UNSIGNED_BYTE, GL_RGB8};
    textureFormats_[Image::RGBA8] = {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8};
    textureFormats_[Image::BGRA8] = {GL_BGRA, GL_UNSIGNED_BYTE, GL_RGBA8};
    textureFormats_[Image::RGB565] = {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB};
    textureFormats_[Image::RGBA5551] = {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA};
    textureFormats_[Image::ARGB1555] = {GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_RGBA};
    textureFormats_[Image::RGBA4444] = {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4};
    textureFormats_[Image::ARGB4444] = {GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_RGBA4};
    textureFormats_[Image::SRGB8] = {GL_RGB, GL_UNSIGNED_BYTE, GL_SRGB8};
    textureFormats_[Image::SRGBA8] = {GL_RGBA, GL_UNSIGNED_BYTE, GL_SRGB8_ALPHA8};
    textureFormats_[Image::RGB16f] = {GL_RGB, GL_HALF_FLOAT, GL_RGB16F};
    textureFormats_[Image::RGBA16f] = {GL_RGBA, GL_HALF_FLOAT, GL_RGBA16F};
    textureFormats_[Image::Red16f] = {GL_RED, GL_HALF_FLOAT, GL_R16F};
    textureFormats_[Image::RedGreen16f] = {GL_RG, GL_HALF_FLOAT, GL_RG16F};
    textureFormats_[Image::RGB32f] = {GL_RGB, GL_FLOAT, GL_RGB32F};
    textureFormats_[Image::RGBA32f] = {GL_RGBA, GL_FLOAT, GL_RGBA32F};
    textureFormats_[Image::Red32f] = {GL_RED, GL_FLOAT, GL_R32F};
    textureFormats_[Image::RedGreen32f] = {GL_RG, GL_FLOAT, GL_RG32F};
    textureFormats_[Image::Depth] = {GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT};
    textureFormats_[Image::Depth24Stencil8] = {GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8};

    if (extensions_.EXT_texture_compression_s3tc)
    {
        textureFormats_[Image::DXT1] = {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT};
        textureFormats_[Image::DXT3] = {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT};
        textureFormats_[Image::DXT5] = {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT};
    }
}

unsigned int OpenGL41::getMaximumTextureSize(TextureType type) const
{
    if (type == Texture2D)
        return glGetUnsignedInteger(GL_MAX_TEXTURE_SIZE);

    if (type == Texture3D)
        return glGetUnsignedInteger(GL_MAX_3D_TEXTURE_SIZE);

    if (type == TextureCubemap)
        return glGetUnsignedInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE);

    return 0;
}

unsigned int OpenGL41::getMaximumTextureAnisotropy(TextureType type) const
{
    if (!extensions_.EXT_texture_filter_anisotropic)
        return 1;

    return glGetUnsignedInteger(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
}

unsigned int OpenGL41::getTextureUnitCount() const
{
    return textureUnitCount_;
}

bool OpenGL41::isTextureSupported(TextureType type, const Image& image) const
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

bool OpenGL41::isNonPowerOfTwoTextureSupported(TextureType type) const
{
    return true;
}

GraphicsInterface::TextureObject OpenGL41::createTexture()
{
    auto glTexture = GLuint();
    glGenTextures(1, &glTexture);
    CARBON_CHECK_OPENGL_ERROR(glGenTextures);
    return new Texture(glTexture);
}

void OpenGL41::deleteTexture(TextureObject texture)
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

bool OpenGL41::setTexture(unsigned int textureUnit, TextureObject textureObject)
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

bool OpenGL41::uploadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
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
                glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(), 0,
                                       data[i].getDataSize(), data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage2D);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(), 0,
                             glDataFormat, glDataType, data[i].getData());
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
                glCompressedTexImage3D(GL_TEXTURE_3D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(),
                                       data[i].getDepth(), 0, data[i].getDataSize(), data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage3D);
            }
            else
            {
                glTexImage3D(GL_TEXTURE_3D, i, glInternalFormat, data[i].getWidth(), data[i].getHeight(),
                             data[i].getDepth(), 0, glDataFormat, glDataType, data[i].getData());
                CARBON_CHECK_OPENGL_ERROR(glTexImage3D);
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
                    glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, glInternalFormat, d.getWidth(),
                                           d.getHeight(), 0, d.getDataSize(), d.getData());
                    CARBON_CHECK_OPENGL_ERROR(glCompressedTexImage2D);
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, j, glInternalFormat, d.getWidth(), d.getHeight(),
                                 0, glDataFormat, glDataType, d.getData());
                    CARBON_CHECK_OPENGL_ERROR(glTexImage2D);
                }
            }
        }
    }
    else
        return false;

    return true;
}

void OpenGL41::setTextureFilter(TextureObject texture, TextureType type, TextureFilter minFilter,
                                TextureFilter magFilter)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MIN_FILTER, glTextureFilterEnum[minFilter]);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAG_FILTER, glTextureFilterEnum[magFilter]);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGL41::setTextureWrap(TextureObject texture, TextureType type, TextureWrap wrap)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    auto glWrap = GLenum();
    if (wrap == WrapClamp)
        glWrap = GL_CLAMP_TO_EDGE;
    else if (wrap == WrapRepeat)
        glWrap = GL_REPEAT;

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_S, glWrap);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_T, glWrap);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);

    if (type == Texture3D)
    {
        glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_WRAP_R, glWrap);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    }
}

void OpenGL41::setTextureAnisotropy(TextureObject texture, TextureType type, unsigned int anisotropy)
{
    if (!extensions_.EXT_texture_filter_anisotropic)
        return;

    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAX_ANISOTROPY_EXT,
                    Math::clamp<unsigned int>(anisotropy, 1, getMaximumTextureAnisotropy(type)));
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGL41::setTextureBaseAndMaximumMipmapLevels(TextureObject texture, TextureType type, unsigned int baseLevel,
                                                    unsigned int maximumLevel)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_BASE_LEVEL, baseLevel);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    glTexParameteri(glTextureTypeEnum[type], GL_TEXTURE_MAX_LEVEL, maximumLevel);
    CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
}

void OpenGL41::setTextureIsShadowMap(TextureObject texture, bool isShadowMap)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);

    if (isShadowMap)
    {
        // Setup shadow map compare function
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        CARBON_CHECK_OPENGL_ERROR(glTexParameteri);
    }
}

}

#endif
