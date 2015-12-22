/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_OPENGLES2

#ifdef APPLE
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
#elif defined(LINUX)
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#else
    #error OpenGL ES 2 header location not known for this platform
#endif

#ifndef GL_DOUBLE
    #define GL_DOUBLE 0
#endif

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShared.h"

namespace Carbon
{

/**
 * OpenGL ES 2 graphics backend.
 */
class CARBON_API OpenGLES2 : public OpenGLShared
{
public:

    bool setup() override;
    void disableUnusedCachedStates() override;

    unsigned int getMaximumTextureSize(TextureType type) const override;
    unsigned int getTextureUnitCount() const override;
    bool isTextureSupported(TextureType type, const Image& image) const override;
    bool isNonPowerOfTwoTextureSupported(TextureType type) const override;

    TextureObject createTexture() override;
    void deleteTexture(TextureObject texture) override;
    bool uploadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
                       const Vector<TextureData>& data) override;
    void setTextureFilter(TextureObject texture, TextureType type, TextureFilter minFilter, TextureFilter magFilter) override;
    void setTextureWrap(TextureObject texture, TextureType type, TextureWrap wrap) override;

    DataBufferObject createDataBuffer() override;
    void deleteDataBuffer(DataBufferObject dataBufferObject) override;
    bool uploadStaticDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                const byte_t* data) override;
    bool uploadDynamicDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                 const byte_t* data) override;
    bool updateDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, const byte_t* data) override;

    unsigned int getVertexAttributeArrayCount() const override;
    bool setVertexAttributeArrayEnabled(unsigned int attributeIndex, bool enabled) override;
    bool setVertexAttributeArraySource(unsigned int attributeIndex, const ArraySource& source) override;

    bool isVertexAttribtuteArrayConfigurationSupported() const override;
    VertexAttributeArrayConfigurationObject
        createVertexAttributeArrayConfiguration(const Vector<ArraySource>& sources) override;
    void deleteVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration) override;
    void setVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration) override;

    bool isStencilBufferSupported() const override;
    void setStencilOperationsForFrontFaces(const States::StencilOperations& operations) override;
    void setStencilOperationsForBackFaces(const States::StencilOperations& operations) override;

    void setDepthClearValue(float clearValue) override;

    bool setTexture(unsigned int textureUnit, TextureObject texture) override;

    bool isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const override;
    ShaderProgram* createShaderProgram(ShaderProgram::ShaderLanguage language) override;
    void deleteShaderProgram(ShaderProgram* program) override;
    void setShaderProgram(ShaderProgram* program) override;

    void drawIndexedPrimitives(PrimitiveType primitiveType, unsigned int lowestIndex, unsigned int highestIndex,
                               unsigned int indexCount, DataType indexDataType, DataBufferObject indexDataBufferObject,
                               uintptr_t indexOffset) override;

    void copyBackbufferTo2DTexture(TextureObject texture, unsigned int mipmapLevel, const Rect& rect) override;

    bool isRenderTargetSupported() const override;
    RenderTargetObject createRenderTarget() override;
    void deleteRenderTarget(RenderTargetObject renderTarget) override;
    bool setRenderTargetColorBufferTextures(RenderTargetObject renderTarget, const Vector<TextureObject>& textures,
                                            const Vector<int>& cubemapFaces) override;
    unsigned int getMaximumRenderTargetColorTextures() const override;
    bool setRenderTargetDepthBufferTexture(RenderTargetObject renderTarget, TextureObject texture) override;
    bool setRenderTargetStencilBufferTexture(RenderTargetObject renderTarget, TextureObject texture) override;
    bool isRenderTargetValid(RenderTargetObject renderTarget) const override;
    void setRenderTarget(RenderTargetObject renderTarget) override;

protected:

    const std::array<GLenum, 3> glBufferTypeEnum = {{0, GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER}};

    const std::array<GLenum, 8> glPrimitiveType = {
        {GL_POINTS, GL_LINES, 0, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, 0, 0}};

    const std::array<GLenum, 4> glTextureTypeEnum = {{0, GL_TEXTURE_2D, 0, GL_TEXTURE_CUBE_MAP}};

    const std::array<GLenum, 5> glTextureFilterEnum = {
        {GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR}};

    // The availability of each of these extensions is determined in OpenGLES2::setup()
    struct
    {
        bool APPLE_texture_format_BGRA8888 = false;
        bool EXT_sRGB = false;
        bool EXT_texture_compression_dxt1 = false;
        bool IMG_texture_compression_pvrtc = false;
        bool OES_depth_texture = false;
        bool OES_packed_depth_stencil = false;
        bool OES_vertex_array_object = false;
    } extensions_;

    // Texture handling
    void setupTextureFormats() override;
    GLenum getTextureInternalFormat(Image::PixelFormat pixelFormat, TextureType textureType) const override;

    // VBO support
    void setVertexDataBuffer(const DataBuffer* dataBufferObject);
    void setIndexDataBuffer(const DataBuffer* dataBufferObject);
};

}

#endif
