/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/ShaderProgram.h"
#include "CarbonEngine/Graphics/States/StateTypes.h"
#include "CarbonEngine/Image/Image.h"

namespace Carbon
{

/**
 * Provides an interface over an underlying graphics API such as OpenGL. This is what allows graphics API independence, as all
 * setup and drawing commands issued by the renderer go through this layer. Implementations of this interface are registered for
 * use by the CARBON_REGISTER_INTERFACE_IMPLEMENTATION() macro, see the InterfaceRegistry class for details.
 */
class CARBON_API GraphicsInterface
{
public:

    virtual ~GraphicsInterface() {}

    /**
     * Opaque texture object. Null is reserved for 'no texture'.
     */
    typedef void* TextureObject;

    /**
     * The supported texture types. Hardware capabilities and restrictions for each texture type may differ.
     */
    enum TextureType
    {
        TextureNone,
        Texture2D,
        Texture3D,
        TextureCubemap
    };

    /**
     * Texture filtering modes, ones containing 'Mipmap' should only be used for minification.
     */
    enum TextureFilter
    {
        FilterNearest,
        FilterLinear,
        FilterNearestMipmapNearest,
        FilterNearestMipmapLinear,
        FilterLinearMipmapLinear
    };

    /**
     * Texture wrap modes.
     */
    enum TextureWrap
    {
        WrapRepeat,
        WrapClamp
    };

    /**
     * Opaque data buffer object for vertex and index data. Null is reserved for 'no buffer'.
     */
    typedef void* DataBufferObject;

    /**
     * Data buffer types, just vertex and index data is handled at present.
     */
    enum DataBufferType
    {
        BufferNone,
        VertexDataBuffer,
        IndexDataBuffer
    };

    /**
     * Returns whether this graphics interface is able to be used on the current platform.
     */
    virtual bool isSupported() const { return true; }

    /**
     * Initializes the graphics interface, returns success flag.
     */
    virtual bool setup();

    /**
     * This is called automatically during graphics interface setup and can be used by graphics interface implementations to
     * call States::StateCacher::disable() on any states that they don't support in order to avoid the overhead of the state
     * cacher having to manage them.
     */
    virtual void disableUnusedCachedStates() {}

    /**
     * Uninitializes the graphics interface. For internal use only.
     */
    virtual void shutdown() {}

    /**
     * Returns whether the given shader language is supported.
     */
    virtual bool isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const { return false; }

    /**
     * Returns the number of whether texture lookup in vertex programs is supported by the given shader language on this
     * hardware.
     */
    virtual unsigned int getVertexShaderTextureUnitCount(ShaderProgram::ShaderLanguage language) const { return 0; }

    /**
     * Returns whether geometry programs are supported in the given shader language on this hardware.
     */
    virtual bool isGeometryProgrammingSupported(ShaderProgram::ShaderLanguage language) const { return false; }

    /**
     * Creates a shader program that uses the given language, or null if the language is not supported by this graphics
     * interface.
     */
    virtual ShaderProgram* createShaderProgram(ShaderProgram::ShaderLanguage language) { return nullptr; }

    /**
     * Deletes a shader program created with GraphicsInterface::createShaderProgram(). Deleting a null shader program is a
     * no-op.
     */
    virtual void deleteShaderProgram(ShaderProgram* program) {}

    /**
     * Sets the shader program used in rendering, setting this to null deactivates any active shader program.
     */
    virtual void setShaderProgram(ShaderProgram* program) {}

    /**
     * Returns the maximum texture dimension for the given texture type.
     */
    virtual unsigned int getMaximumTextureSize(TextureType type) const { return 0; }

    /**
     * Returns the maximum allowable texture anisotropy setting for the given texture type.
     */
    virtual unsigned int getMaximumTextureAnisotropy(TextureType type) const { return 0; }

    /**
     * Returns the number of texture units that are available, this is the maximum number of textures that can be used by a
     * shader or draw call.
     */
    virtual unsigned int getTextureUnitCount() const { return 0; }

    /**
     * Returns whether the hardware supports using the given image as a source for the given texture type. Internally this will
     * check hardware limitations such as maximum texture sizes, NPOT support, pixel format support, and so on.
     */
    virtual bool isTextureSupported(TextureType type, const Image& image) const { return false; }

    /**
     * Returns whether the hardware supports the given pixel format when using the given texture type.
     */
    virtual bool isPixelFormatSupported(Image::PixelFormat pixelFormat, TextureType type) const { return false; }

    /**
     * If the given pixel format isn't supported by this graphics interface then this method returns a recommendation of an
     * alternative pixel format to use instead. If the specified pixel format is supported (i.e.
     * GraphicsInterface::isPixelFormatSupported() returns true) then it is returned unchanged. The default implementation of
     * this method falls back to RGBA8 or RGB8 depending on whether the original pixel format contains an alpha channel,
     * subclasses should alter this as needed.
     */
    virtual Image::PixelFormat getFallbackPixelFormat(TextureType type, Image::PixelFormat pixelFormat) const
    {
        if (isPixelFormatSupported(pixelFormat, type))
            return pixelFormat;

        auto isAlphaAware = Image::isPixelFormatAlphaAware(pixelFormat);
        auto isFloatingPoint = Image::isPixelFormatFloatingPoint(pixelFormat);

        if (isFloatingPoint)
        {
            if (isAlphaAware && isPixelFormatSupported(Image::RGBA32f, type))
                return Image::RGBA32f;

            if (!isAlphaAware && isPixelFormatSupported(Image::RGB32f, type))
                return Image::RGB32f;
        }

        if (isAlphaAware)
            return Image::RGBA8;

        return Image::RGB8;
    }

    /**
     * Returns whether the hardware supports dimensions that are not powers of two on the given texture type.
     */
    virtual bool isNonPowerOfTwoTextureSupported(TextureType type) const { return false; }

    /**
     * Creates a new texture object. Returns null on failure.
     */
    virtual TextureObject createTexture() { return nullptr; }

    /**
     * Deletes a texture object created with GraphicsInterface::createTexture(). Deleting a null texture object is a no-op.
     */
    virtual void deleteTexture(TextureObject texture) {}

    /**
     * Helper class used by GraphicsInterface::uploadTexture() to upload texture data to the graphics interface.
     */
    class TextureData
    {
    public:

        /**
         * Constructs this texture data with the passed values.
         */
        TextureData(unsigned int width = 0, unsigned int height = 0, unsigned int depth = 0, const byte_t* data = nullptr,
                    unsigned int dataSize = 0)
            : width_(width), height_(height), depth_(depth), data_(data), dataSize_(dataSize)
        {
        }

        /**
         * Returns the width in texels of this texture data.
         */
        unsigned int getWidth() const { return width_; }

        /**
         * Returns the height in texels of this texture data.
         */
        unsigned int getHeight() const { return height_; }

        /**
         * Returns the depth in texels of this texture data.
         */
        unsigned int getDepth() const { return depth_; }

        /**
         * Returns the raw texture data.
         */
        const byte_t* getData() const { return data_; }

        /**
         * Returns the size in bytes of the raw texture data returned by TextureData::getData().
         */
        unsigned int getDataSize() const { return dataSize_; }

    private:

        unsigned int width_ = 0;
        unsigned int height_ = 0;
        unsigned int depth_ = 0;
        const byte_t* data_ = nullptr;
        unsigned int dataSize_ = 0;
    };

    /**
     * Uploads the image data to use for the specified texture object. The \a data parameter holds all the individual mipmaps
     * with the base level at index 0. For cubemap textures the number of entries in \a data must be a multiple of 6 and consist
     * of the complete mipmap chain for each cubemap face in the order: positive X, negative X, positive Y, negative Y, positive
     * Z, negative Z. Returns success flag.
     */
    virtual bool uploadTexture(TextureObject texture, TextureType type, Image::PixelFormat pixelFormat,
                               const Vector<TextureData>& data)
    {
        return false;
    }

    /**
     * Downloads the current image data for the specified texture object and returns it in \a image. This can be used to read
     * back the result of render to texture effects. Returns success flag.
     */
    virtual bool downloadTexture(TextureObject texture, TextureType type, Image::PixelFormat targetPixelFormat, Image& image)
    {
        return false;
    }

    /**
     * Sets the minification and magnification filters to use on the given texture object. The magnification filter can only be
     * FilterNearest or FilterLinear. The minification filter can be any of the TextureFilter enumeration values.
     */
    virtual void setTextureFilter(TextureObject texture, TextureType type, TextureFilter minFilter, TextureFilter magFilter) {}

    /**
     * Sets the wrap mode to use on the given texture object.
     */
    virtual void setTextureWrap(TextureObject texture, TextureType type, TextureWrap wrap) {}

    /**
     * Sets the anisotropic filtering level to use on the given texture object.
     */
    virtual void setTextureAnisotropy(TextureObject texture, TextureType type, unsigned int anisotropy) {}

    /**
     * Sets the base and maximum mipmap levels for the given texture object. \a baseLevel must not be greater than \a
     * maximumLevel.
     */
    virtual void setTextureBaseAndMaximumMipmapLevels(TextureObject texture, TextureType type, unsigned int baseLevel,
                                                      unsigned int maximumLevel)
    {
    }

    /**
     * Sets whether the given texture is a shadow map texture and should have hardware accelerated depth comparisons enabled on
     * it. The texture should be 2D and have been uploaded with a pixel format of Image::Depth or Image::Depth24Stencil8.
     */
    virtual void setTextureIsShadowMap(TextureObject texture, bool isShadowMap) {}

    /**
     * Creates a new data buffer object, these are used to store vertex and index data for use in rendering. Returns null on
     * failure.
     */
    virtual DataBufferObject createDataBuffer() { return nullptr; }

    /**
     * Deletes a data buffer object created with GraphicsInterface::createDataBuffer(). Deleting a null data buffer object is a
     * no-op.
     */
    virtual void deleteDataBuffer(DataBufferObject dataBufferObject) {}

    /**
     * Uploads data into a data buffer, the data will be optimized for static rendering, i.e. it will not be changing
     * frequently. Returns success flag.
     */
    virtual bool uploadStaticDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                        const byte_t* data)
    {
        return false;
    }

    /**
     * Uploads data into a data buffer, the data will be optimized for dynamic rendering, i.e. it will be assumed to be changing
     * frequently. Returns success flag.
     */
    virtual bool uploadDynamicDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                         const byte_t* data)
    {
        return false;
    }

    /**
     * Updates the data in an existing data buffer, for performance reasons this should only be used on dynamic data buffers,
     * but will work for static buffers. The size should be the same as the full size of the buffer when it was created. Returns
     * success flag.
     */
    virtual bool updateDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, const byte_t* data) { return false; }

    /**
     * Sets the color that the backbuffer is cleared to by GraphicsInterface::clearBuffers().
     */
    virtual void setClearColor(const Color& color) {}

    /**
     * Sets whether depth testing is enabled.
     */
    virtual void setDepthTestEnabled(bool enabled) {}

    /**
     * Sets the value that the depth buffer is cleared to by GraphicsInterface::clearBuffers().
     */
    virtual void setDepthClearValue(float clearValue) {}

    /**
     * Sets whether depth buffer writes are enabled.
     */
    virtual void setDepthWriteEnabled(bool enabled) {}

    /**
     * Sets the comparison function to use that determines whether an incoming fragment passes the depth test.
     */
    virtual void setDepthCompareFunction(States::CompareFunction compare) {}

    /**
     * Sets the culling mode to use.
     */
    virtual void setCullMode(States::CullingMode mode) {}

    /**
     * Sets the blending mode to use.
     */
    virtual void setBlendEnabled(bool enabled) {}

    /**
     * Sets the blending function to use.
     */
    virtual void setBlendFunction(const States::BlendFunctionSetup& function) {}

    /**
     * Sets the current viewport area being rendered to.
     */
    virtual void setViewport(const Rect& viewport) {}

    /**
     * Sets whether scissoring should be enabled.
     */
    virtual void setScissorEnabled(bool enabled) {}

    /**
     * Sets the current scissor rectangle to use, when scissoring is enabled pixels outside this rectangle will not be affected
     * by rendering.
     */
    virtual void setScissorRectangle(const Rect& scissor) {}

    /**
     * Sets whether color buffer writes are enabled.
     */
    virtual void setColorWriteEnabled(bool enabled) {}

    /**
     * Sets the active texture for the given texture unit. Setting a null texture clears any currently active texture.
     */
    virtual bool setTexture(unsigned int textureUnit, TextureObject texture) { return false; }

    /**
     * Sets whether multisampling is enabled. Note that enabling multisampling will only have an effect when the backbuffer has
     * been set up with multisample support.
     */
    virtual void setMultisampleEnabled(bool enabled) {}

    /**
     * Returns whether this graphics interface supports stencil buffering and testing.
     */
    virtual bool isStencilBufferSupported() const { return false; }

    /**
     * Sets whether or not stencil testing and associated updates of the stencil buffer is enabled when rendering.
     */
    virtual void setStencilTestEnabled(bool enabled) {}

    /**
     * Sets the stencil test that is done on every fragment when stencil testing is enabled.
     */
    virtual void setStencilTestFunction(const States::StencilTestSetup& function) {}

    /**
     * Sets whether or not the stencil buffer is allowed to be updated when rendering geometry. When enabled, the updates that
     * are done will be determined by the current stencil compare function and the current stencil operations. When disabled,
     * fragments will still be culled based on the stencil compare function when stenciling is enabled, however there will be no
     * alterations to the stencil buffer.
     */
    virtual void setStencilWriteEnabled(bool enabled) {}

    /**
     * When stencil testing and stencil write is enabled this specifies what operations should be carried out on the stencil
     * buffer when the stencil test fails, when the depth test fails, and when both of them pass.
     */
    virtual void setStencilOperationsForFrontFaces(const States::StencilOperations& operations) {}

    /**
     * When stencil testing and stencil write is enabled this specifies what operations should be carried out on the stencil
     * buffer when the stencil test fails, when the depth test fails, and when both of them pass.
     */
    virtual void setStencilOperationsForBackFaces(const States::StencilOperations& operations) {}

    /**
     * Sets the value that the stencil buffer should be cleared to when calling GraphicsInterface::clearBuffers().
     */
    virtual void setStencilClearValue(unsigned int clearValue) {}

    /**
     * Returns whether this graphics interface supports depth clamping. Depth clamping clamps the z/w term of fragments to the
     * 0-1 range which effectively disables near and far plane clipping.
     */
    virtual bool isDepthClampSupported() const { return false; }

    /**
     * Sets whether depth claping is enabled when rendering, this is only available if depth clamping is supported (see
     * GraphicsInterface::isDepthClampSupported()).
     */
    virtual void setDepthClampEnabled(bool enabled) {}

    /**
     * Helper class that describes a vertex array source, this is made up of the data buffer object to read data from, an offset
     * into that buffer, as well as a stride, a component count and the data type that is to be read. There is also a flag
     * controlling normalization of fixed point integer data types which must be disabled in order to pass integers through
     * unchanged.
     */
    class ArraySource
    {
    public:

        /**
         * Constructs this array source with the passed values.
         */
        ArraySource(DataBufferObject dataBufferObject = nullptr, uintptr_t offset = 0, unsigned int stride = 0,
                    unsigned int componentCount = 0, DataType dataType = TypeNone, bool normalizeFixedPoint = false)
            : dataBufferObject_(dataBufferObject),
              offset_(offset),
              stride_(stride),
              componentCount_(componentCount),
              dataType_(dataType),
              normalizeFixedPoint_(normalizeFixedPoint)
        {
        }

        /**
         * Returns the data buffer object for this array source, if this is null then this source is invalid or not yet setup.
         */
        DataBufferObject getDataBufferObject() const { return dataBufferObject_; }

        /**
         * Returns the offset in bytes for this array source into its data buffer.
         */
        uintptr_t getOffset() const { return offset_; }

        /**
         * Returns the stride for this array source.
         */
        unsigned int getStride() const { return stride_; }

        /**
         * Returns the component count for this array source, must be in the range 1 - 4.
         */
        unsigned int getComponentCount() const { return componentCount_; }

        /**
         * Returns the data type for this array source.
         */
        DataType getDataType() const { return dataType_; }

        /**
         * Returns whether integer data types should be normalized into the 0-1 range.
         */
        bool getNormalizeFixedPoint() const { return normalizeFixedPoint_; }

        /**
         * Inequality operator.
         */
        bool operator!=(const ArraySource& other) const
        {
            return dataBufferObject_ != other.getDataBufferObject() || offset_ != other.getOffset() ||
                stride_ != other.getStride() || componentCount_ != other.getComponentCount() ||
                dataType_ != other.getDataType() || normalizeFixedPoint_ != other.getNormalizeFixedPoint();
        }

        /**
         * Returns whether this array source is valid for use in rendering.
         */
        bool isValid() const { return dataBufferObject_ && stride_ && componentCount_ >= 1 && componentCount_ <= 4; }

        /**
         * Converts this array source into a human-readable string.
         */
        operator UnicodeString() const
        {
            return UnicodeString() << "Data buffer: " << dataBufferObject_ << ", offset: " << uint64_t(offset_)
                                   << ", stride: " << stride_ << ", component count: " << componentCount_
                                   << ", data type: " << dataType_ << ", normalize fixed point: " << normalizeFixedPoint_;
        }

    private:

        DataBufferObject dataBufferObject_ = nullptr;
        uintptr_t offset_ = 0;
        unsigned int stride_ = 0;
        unsigned int componentCount_ = 0;
        DataType dataType_ = TypeNone;
        bool normalizeFixedPoint_ = false;
    };

    /**
     * Returns the number of vertex attribute arrays supported by this graphics interface. Vertex attribute arrays are used to
     * provide per-vertex attributes into the graphics hardware, e.g. position, texture coordinates, normals, or any other
     * per-vertex data that is required.
     */
    virtual unsigned int getVertexAttributeArrayCount() const { return 0; }

    /**
     * Sets whether to read vertex attribute data out of the corresponding vertex attribute array source specified by
     * GraphicsInterface::setVertexAttributeArraySource(). The upper limit for \a attributeIndex is returned by
     * GraphicsInterface::getVertexAttributeArrayCount(). Note that this state is ignored when vertex attribute array
     * configurations are supported as they should be used instead.
     */
    virtual bool setVertexAttributeArrayEnabled(unsigned int attributeIndex, bool enabled) { return true; }

    /**
     * Sets the data source for the specified vertex attribute array. The upper limit for \a attributeIndex is returned by
     * GraphicsInterface::getVertexAttributeArrayCount(). This source is only used if the vertex attribute array has been
     * enabled using GraphicsInterface::setVertexAttributeArrayEnabled(). Note that this state is ignored when vertex attribute
     * array configurations are supported as they should be used instead.
     */
    virtual bool setVertexAttributeArraySource(unsigned int attributeIndex, const ArraySource& source) { return true; }

    /**
     * Opaque vertex attribute array configuration object, this is used to store a configuration of predefined vertex attribute
     * array sources that can then be activated in one call to GraphicsInterface::setVertexAttributeArrayConfiguration().
     */
    typedef void* VertexAttributeArrayConfigurationObject;

    /**
     * Returns whether vertex attribute array configuration objects are supported by this graphics interface, when this is false
     * all calls to GraphicsInterface::createVertexAttributeArrayConfiguration() will return null.
     */
    virtual bool isVertexAttribtuteArrayConfigurationSupported() const { return false; }

    /**
     * Creates a new vertex attribute array configuration object from the specified sources. Using vertex attribute array
     * configuration objects is preferred to individually setting up each attribute array through calls to
     * GraphicsInterface::setVertexAttributeArrayEnabled() and GraphicsInterface::setVertexAttributeArraySource() as it reduces
     * CPU and graphics driver load. When a vertex attribute configuration is active all state setup by manual calls to
     * GraphicsInterface::setVertexAttributeArrayEnabled() and GraphicsInterface::setVertexAttributeArraySource() is ignored.
     * Returns null if an error occurs.
     */
    virtual VertexAttributeArrayConfigurationObject createVertexAttributeArrayConfiguration(const Vector<ArraySource>& sources)
    {
        return nullptr;
    }

    /**
     * Deletes a vertex attribute array configuration object created by a previous call to
     * GraphicsInterface::createVertexAttributeArrayConfiguration().
     */
    virtual void deleteVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration) {}

    /**
     * Activates the specified vertex attribute array configuration object for use in rendering.
     */
    virtual void setVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration) {}

    /**
     * Clears the color buffer, depth buffer, and stencil buffers of the currently active render target. If no render target is
     * currently active then the buffers for the primary display surface will be cleared. There are individual flags that
     * control which buffers should be cleared.
     */
    virtual void clearBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) {}

    /**
     * The list of primitive types that can be drawn by GraphicsInterface::drawIndexedPrimitives(). Whether or not the given
     * primitive type is supported by the active graphics interface can be checked using
     * GraphicsInterface::isPrimitiveTypeSupported(). Note that these enum values are allowed to be persisted and so their
     * integer values should not be altered.
     */
    enum PrimitiveType
    {
        LineList = 1,
        LineStrip = 3,
        TriangleList = 4,
        TriangleStrip = 5,
        TriangleListWithAdjacency = 6,
        TriangleStripWithAdjacency = 7,
        PrimitiveLast
    };

    /**
     * Returns whether this graphics interface supports the specified primitive type.
     */
    virtual bool isPrimitiveTypeSupported(PrimitiveType primitiveType) const { return false; }

    /**
     * Draws primitive data from the currently active vertex attribute arrays and the specified index data buffer. \a
     * indexDataType must one of TypeUInt16 or TypeUInt32. The base class implementation of this method gathers statistics on
     * draw calls and number of triangles rendered, so subclasses should only invoke the base implementation if they were able
     * to execute the passed draw command.
     */
    virtual void drawIndexedPrimitives(PrimitiveType primitiveType, unsigned int lowestIndex, unsigned int highestIndex,
                                       unsigned int indexCount, DataType indexDataType, DataBufferObject indexDataBufferObject,
                                       uintptr_t indexOffset)
    {
        drawCallCount_++;

        switch (primitiveType)
        {
            case TriangleList:
                triangleCount_ += indexCount / 3;
                break;
            case TriangleStrip:
                triangleCount_ += indexCount - 2;
                break;
            case TriangleListWithAdjacency:
                triangleCount_ += indexCount / 6;
                break;
            case TriangleStripWithAdjacency:
                triangleCount_ += indexCount / 2 - 2;
                break;
            default:
                break;
        }
    }

    /**
     * Copies the contents of the backbuffer of the current render target into the specified mipmap of a 2D texture. The width
     * and height of the rectangle should not exceed the width and height of the texture. The rectangle can be offset from the
     * origin in order to copy a specific region of the backbuffer.
     */
    virtual void copyBackbufferTo2DTexture(TextureObject texture, unsigned int mipmapLevel, const Rect& rect) {}

    /**
     * Opaque render target object. Null is reserved for 'no render target'. Render targets are used for off-screen rendering.
     */
    typedef void* RenderTargetObject;

    /**
     * Returns whether this graphics interface supports the use of render targets for off-screen rendering.
     */
    virtual bool isRenderTargetSupported() const { return false; }

    /**
     * Creates a new render target object. Returns null on failure.
     */
    virtual RenderTargetObject createRenderTarget() { return nullptr; }

    /**
     * Deletes a render target object created with GraphicsInterface::createRenderTarget(). Deleting a null render target object
     * is a no-op.
     */
    virtual void deleteRenderTarget(RenderTargetObject renderTarget) {}

    /**
     * Sets the texture(s) to use as the color output(s) for the given render target object. The number of simultaneous output
     * buffers is subject to hardware limitations (see GraphicsInterface::getMaximumRenderTargetColorTextures()). If an empty
     * vector is passed then no color will be output when using the render target but depth data can still be generated if a
     * depth texture is active (see GraphicsInterface::setRenderTargetDepthBufferTexture()). Cubemap faces can be used as render
     * targets by passing a cubemap TextureObject in \a textures and then specifying the cubemap face index to render into using
     * the \a cubemapFaces vector, cubemap indices must be between zero and five. Returns success flag.
     */
    virtual bool setRenderTargetColorBufferTextures(RenderTargetObject renderTarget, const Vector<TextureObject>& textures,
                                                    const Vector<int>& cubemapFaces)
    {
        return false;
    }

    /**
     * Returns the maximum number of color textures that can be set on a render target in order to output to multiple textures
     * from a single shader. This will be zero if render targets are not supported, and will be at least one when render targets
     * are supported.
     */
    virtual unsigned int getMaximumRenderTargetColorTextures() const { return 0; }

    /**
     * Sets the texture to use as the depth buffer for the given render target object. Returns success flag.
     */
    virtual bool setRenderTargetDepthBufferTexture(RenderTargetObject renderTarget, TextureObject texture) { return false; }

    /**
     * Sets the texture to use as the stencil buffer for the given render target object. Returns success flag.
     */
    virtual bool setRenderTargetStencilBufferTexture(RenderTargetObject renderTarget, TextureObject texture) { return false; }

    /**
     * Returns whether the given render target object is set up in such as a way that it is ready to be used as a target for
     * rendering.
     */
    virtual bool isRenderTargetValid(RenderTargetObject renderTarget) const { return false; }

    /**
     * Sets the render target to direct all rendering into. Setting this to null directs all rendering into the backbuffer.
     */
    virtual void setRenderTarget(RenderTargetObject renderTarget) {}

    /**
     * Tells the graphics interface that the contents of the specified buffers currently attached to the active render target
     * are not needed anymore. This information can be used on some platforms to avoid costly buffer writebacks to system
     * memory, particularly on tile-based deferred renderers.
     */
    virtual void discardRenderTargetBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) {}

    /**
     * The output destinations for final rendering that is ready for display, each of these can sbe checked for hardware support
     * using GraphicsInterface::isOutputDestinationSupported(). The render target and viewport for each output destination are
     * returned by GraphicsInterface::getOutputDestinationRenderTarget() and GraphicsInterface::getOutputDestinationViewport().
     */
    enum OutputDestination
    {
        /**
         * The default output destination, this is always supported and results in rendering to the system's primary display.
         */
        OutputDefault,

        /**
         * The output destination for rendering to the Oculus Rift's left eye.
         */
        OutputOculusRiftLeftEye,

        /**
         * The output destination for rendering to the Oculus Rift's right eye.
         */
        OutputOculusRiftRightEye
    };

    /**
     * Returns whether the spcecified output destination is supported by this graphics interface. The `OutputDefault`
     * destination is always supported.
     */
    virtual bool isOutputDestinationSupported(OutputDestination destination) const { return destination == OutputDefault; }

    /**
     * Returns the render target object to use for rendering into the specified output destination. This will always be null for
     * the default output.
     */
    virtual RenderTargetObject getOutputDestinationRenderTarget(OutputDestination destination) { return nullptr; }

    /**
     * Returns the viewport rectangle for the specified output destination. For the default output destination this method will
     * always return PlatformInterface::getWindowRect().
     */
    virtual Rect getOutputDestinationViewport(OutputDestination destination) const;

    /**
     * This method is called to signal that the engine has finished writing a frame to the specified output destination.
     */
    virtual void flushOutputDestination(OutputDestination destination) {}

    /**
     * Returns the number of draw calls that have been made since the graphics interface was initialized.
     */
    uint64_t getDrawCallCount() const { return drawCallCount_; }

    /**
     * Returns the number of triangles that have been drawn since the graphics interface was initialized.
     */
    uint64_t getTriangleCount() const { return triangleCount_; }

    /**
     * Returns the number of graphics API calls that have been made since this graphics interface was initialized, this is the
     * number of calls made into the underlying graphics API (e.g. OpenGL).
     */
    uint64_t getAPICallCount() const { return apiCallCount_; }

    /**
     * Increments the internal API call count by one.
     */
    void incrementAPICallCount() { apiCallCount_++; }

private:

    uint64_t drawCallCount_ = 0;
    uint64_t triangleCount_ = 0;
    uint64_t apiCallCount_ = 0;
};

}
