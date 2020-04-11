/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_OPENGL41

#ifdef MACOS
    #define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #error The location of the OpenGL 4.1 headers is not known on this platform
#endif

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShared.h"

namespace Carbon
{

/**
 * Graphics interface backend that targets the OpenGL 4.1 Core Profile with a few optional extensions. The supported
 * shader languages are GLSL 1.1 and GLSL 4.1.
 */
class CARBON_API OpenGL41 : public OpenGLShared
{
public:

    bool isSupported() const override;
    bool setup() override;
    void shutdown() override;

    bool isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const override;
    unsigned int getVertexShaderTextureUnitCount(ShaderProgram::ShaderLanguage language) const override;
    bool isGeometryProgrammingSupported(ShaderProgram::ShaderLanguage language) const override;
    ShaderProgram* createShaderProgram(ShaderProgram::ShaderLanguage language) override;
    void deleteShaderProgram(ShaderProgram* program) override;
    void setShaderProgram(ShaderProgram* program) override;

    unsigned int getMaximumTextureSize(TextureType type) const override;
    unsigned int getMaximumTextureAnisotropy(TextureType type) const override;
    unsigned int getTextureUnitCount() const override;
    bool isTextureSupported(TextureType type, const Image& image) const override;
    bool isNonPowerOfTwoTextureSupported(TextureType type) const override;

    TextureObject createTexture() override;
    void deleteTexture(TextureObject texture) override;
    bool uploadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
                       const Vector<TextureData>& data) override;
    void setTextureFilter(TextureObject texture, TextureType type, TextureFilter minFilter,
                          TextureFilter magFilter) override;
    void setTextureWrap(TextureObject texture, TextureType type, TextureWrap wrap) override;
    void setTextureAnisotropy(TextureObject texture, TextureType type, unsigned int anisotropy) override;
    void setTextureBaseAndMaximumMipmapLevels(TextureObject texture, TextureType type, unsigned int baseLevel,
                                              unsigned int maximumLevel) override;
    void setTextureIsShadowMap(TextureObject texture, bool isShadowMap) override;

    DataBufferObject createDataBuffer() override;
    void deleteDataBuffer(DataBufferObject dataBufferObject) override;
    bool uploadStaticDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                const byte_t* data) override;
    bool uploadDynamicDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                 const byte_t* data) override;
    bool updateDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, const byte_t* data) override;

    void setDepthClearValue(float clearValue) override;
    void setMultisampleEnabled(bool enabled) override;

    bool setTexture(unsigned int textureUnit, TextureObject textureObject) override;

    bool isStencilBufferSupported() const override;
    void setStencilOperationsForFrontFaces(const States::StencilOperations& operations) override;
    void setStencilOperationsForBackFaces(const States::StencilOperations& operations) override;

    bool isDepthClampSupported() const override;
    void setDepthClampEnabled(bool enabled) override;

    unsigned int getVertexAttributeArrayCount() const override;
    bool isVertexAttribtuteArrayConfigurationSupported() const override;
    VertexAttributeArrayConfigurationObject
        createVertexAttributeArrayConfiguration(const Vector<ArraySource>& sources) override;
    void deleteVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration) override;
    void setVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration) override;

    bool isPrimitiveTypeSupported(PrimitiveType primitiveType) const override;
    void drawIndexedPrimitives(PrimitiveType primitiveType, unsigned int lowestIndex, unsigned int highestIndex,
                               unsigned int indexCount, DataType indexDataType, DataBufferObject indexDataBufferObject,
                               uintptr_t indexOffset) override;

    void copyBackbufferTo2DTexture(TextureObject texture, unsigned int mipmapLevel, const Rect& rect) override;

    bool isRenderTargetSupported() const override;
    RenderTargetObject createRenderTarget() override;
    void deleteRenderTarget(RenderTargetObject renderTargetObject) override;
    bool setRenderTargetColorBufferTextures(RenderTargetObject renderTargetObject,
                                            const Vector<TextureObject>& textures,
                                            const Vector<int>& cubemapFaces) override;
    unsigned int getMaximumRenderTargetColorTextures() const override;
    bool setRenderTargetDepthBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture) override;
    bool setRenderTargetStencilBufferTexture(RenderTargetObject renderTargetObject, TextureObject texture) override;
    bool isRenderTargetValid(RenderTargetObject renderTargetObject) const override;
    void setRenderTarget(RenderTargetObject renderTargetObject) override;

private:

    const std::array<GLenum, 3> glBufferTypeEnum = {{0, GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER}};

    const std::array<GLenum, 8> glPrimitiveType = {{GL_POINTS, GL_LINES, 0, GL_LINE_STRIP, GL_TRIANGLES,
                                                    GL_TRIANGLE_STRIP, GL_TRIANGLES_ADJACENCY,
                                                    GL_TRIANGLE_STRIP_ADJACENCY}};

    const std::array<GLenum, 4> glTextureTypeEnum = {{0, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP}};

    const std::array<GLenum, 5> glTextureFilterEnum = {
        {GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR}};

    // The availability of each of these extensions is determined in OpenGL41::setup()
    struct
    {
        bool EXT_texture_compression_s3tc = false;
        bool EXT_texture_filter_anisotropic = false;
    } extensions_;

    // Hardware limits
    unsigned int vertexTextureUnitCount_ = 0;
    unsigned int maximumDrawBuffers_ = 0;

    // Texture handling
    void setupTextureFormats() override;

    // VBO support
    void setVertexDataBuffer(const DataBuffer* dataBuffer);
    void setIndexDataBuffer(const DataBuffer* dataBuffer);
};

}

#endif
