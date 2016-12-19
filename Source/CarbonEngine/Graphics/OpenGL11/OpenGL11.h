/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_OPENGL11

#ifndef MACOS
    #include <GL/gl.h>
#else
    #include <OpenGL/gl.h>
#endif

#ifdef CARBON_INCLUDE_OCULUSRIFT
    #include "CarbonEngine/Platform/Windows/OculusRiftIncludeWrapper.h"
#endif

#ifdef _MSC_VER
    #pragma comment(lib, "OpenGL32.lib")
#endif

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShared.h"
#include "CarbonEngine/Graphics/OpenGL11/OpenGL11Extensions.h"

namespace Carbon
{

/**
 * Graphics interface backend that targets OpenGL 1.1 plus a number of extensions that are used to expose more modern
 * hardware functionality such as shaders, vertex buffer objects, cube maps, render targets, and so on.
 */
class CARBON_API OpenGL11 : public OpenGLShared
{
public:

    ~OpenGL11() { shutdown(); }

    bool setup() override;
    void shutdown() override;

    bool isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const override;
    unsigned int getVertexShaderTextureUnitCount(ShaderProgram::ShaderLanguage language) const override;
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
    void setStencilTestEnabled(bool enabled) override;
    void setStencilTestFunction(const States::StencilTestSetup& function) override;
    void setStencilWriteEnabled(bool enabled) override;
    void setStencilOperationsForFrontFaces(const States::StencilOperations& operations) override;
    void setStencilOperationsForBackFaces(const States::StencilOperations& operations) override;

    bool isDepthClampSupported() const override;
    void setDepthClampEnabled(bool enabled) override;

    unsigned int getVertexAttributeArrayCount() const override;
    bool setVertexAttributeArrayEnabled(unsigned int attributeIndex, bool enabled) override;
    bool setVertexAttributeArraySource(unsigned int attributeIndex, const ArraySource& source) override;

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

#ifdef CARBON_INCLUDE_OCULUSRIFT
    RenderTargetObject getOutputDestinationRenderTarget(OutputDestination destination) override;
    virtual Rect getOutputDestinationViewport(OutputDestination destination) const override;
    virtual void flushOutputDestination(OutputDestination destination) override;

    std::array<ovrSwapTextureSet*, 2> getOculusRiftSwapTextureSets()
    {
        return {{ oculusRiftEyes_[0].swapTextureSet, oculusRiftEyes_[1].swapTextureSet }};
    }
#endif

private:

    const std::array<GLenum, 3> glBufferTypeEnum = {{0, GL_ARRAY_BUFFER_ARB, GL_ELEMENT_ARRAY_BUFFER_ARB}};

    const std::array<GLenum, 8> glPrimitiveType = {
        {GL_POINTS, GL_LINES, 0, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, 0, 0}};

    const std::array<GLenum, 4> glTextureTypeEnum = {{0, GL_TEXTURE_2D, GL_TEXTURE_3D_EXT, GL_TEXTURE_CUBE_MAP_ARB}};

    const std::array<GLenum, 5> glTextureFilterEnum = {
        {GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR}};

    // The availability of each of these extensions is determined in OpenGL11::setup()
    struct
    {
        bool ARB_depth_clamp = false;
        bool ARB_depth_texture = false;
        bool ARB_draw_buffers = false;
        bool ARB_half_float_pixel = false;
        bool ARB_shadow = false;
        bool ARB_texture_float = false;
        bool ARB_texture_non_power_of_two = false;
        bool ARB_texture_rg = false;
        bool ARB_vertex_array_object = false;
        bool EXT_abgr = false;
        bool EXT_bgra = false;
        bool EXT_draw_range_elements = false;
        bool EXT_framebuffer_object = false;
        bool EXT_packed_depth_stencil = false;
        bool EXT_stencil_two_side = false;
        bool EXT_stencil_wrap = false;
        bool EXT_texture_3D = false;
        bool EXT_texture_compression_s3tc = false;
        bool EXT_texture_filter_anisotropic = false;
        bool EXT_texture_sRGB = false;
        bool SGIS_texture_lod = false;
    } extensions_;

    // Hardware limits
    unsigned int vertexTextureUnitCount_ = 0;
    unsigned int maximumDrawBuffers_ = 0;

    // Texture handling
    void setupTextureFormats() override;
    GLenum getTextureInternalFormat(Image::PixelFormat pixelFormat, TextureType textureType) const override;

    // VBO support
    void setVertexDataBuffer(const DataBuffer* dataBuffer);
    void setIndexDataBuffer(const DataBuffer* dataBuffer);

#ifdef CARBON_INCLUDE_OCULUSRIFT
    struct OculusRiftEye
    {
        RenderTargetObject renderTarget = nullptr;
        ovrSwapTextureSet* swapTextureSet = nullptr;
        TextureObject depthTexture = nullptr;
    };
    std::array<OculusRiftEye, 2> oculusRiftEyes_;
#endif
};

}

#endif
