/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Color.h"

namespace Carbon
{

/**
 * Class used to store image and texture data. Can handle 2D, 3D and cubemap images with mipmaps in a variety of pixel
 * formats including animations. Pixel data is stored with a bottom left origin. A number of image conversion and
 * transformations are supported, and this class forms the base of texture image data handling.
 */
class CARBON_API Image
{
public:

    /**
     * This is the extension to use when saving or loading an Image. Currently this is ".image".
     */
    static const UnicodeString ImageExtension;

    /**
     * Enumeration of all the supported pixel formats that image data can be stored in.
     */
    enum PixelFormat
    {
        /**
         * Unknown or unspecified pixel format.
         */
        UnknownPixelFormat,

        /**
         * Alpha stored as an 8-bit unsigned integer value.
         */
        Alpha8,

        /**
         * Luminance stored as an 8-bit unsigned integer value, a.k.a. grayscale.
         */
        Luminance8,

        /**
         * Luminance and alpha stored as an 8-bit unsigned integer value per component.
         */
        LuminanceAlpha8,

        /**
         * RGB stored as an 8-bit unsigned integer value per component.
         */
        RGB8,

        /**
         * BGR stored as an 8-bit unsigned integer value per component.
         */
        BGR8,

        /**
         * RGBA stored as an 8-bit unsigned integer value per component.
         */
        RGBA8,

        /**
         * BGRA stored as an 8-bit unsigned integer value per component.
         */
        BGRA8,

        /**
         * ABGR stored as an 8-bit unsigned integer value per component.
         */
        ABGR8,

        /**
         * RGB stored as an 8-bit unsigned integer value per component in the sRGB color space.
         */
        SRGB8,

        /**
         * RGB stored as an 8-bit unsigned integer value per component in the sRGB color space as well as a linear 8-bit
         * alpha.
         */
        SRGBA8,

        /**
         * RGB stored in an unsigned 16-bit little endian 565 arrangement per pixel.
         */
        RGB565,

        /**
         * BGR stored in an unsigned 16-bit little endian 565 arrangement per pixel.
         */
        BGR565,

        /**
         * RGBA stored in an unsigned 16-bit little endian 5551 arrangement per pixel.
         */
        RGBA5551,

        /**
         * RGBA stored in an unsigned 16-bit little endian 4444 arrangement per pixel.
         */
        RGBA4444,

        /**
         * ARGB stored in an unsigned 16-bit little endian 1555 arrangement per pixel.
         */
        ARGB1555,

        /**
         * ARGB stored in an unsigned 16-bit little endian 4444 arrangement per pixel.
         */
        ARGB4444,

        /**
         * ABGR stored in an unsigned 16-bit little endian 4444 arrangement per pixel.
         */
        ABGR4444,

        /**
         * One 16-bit half-precision floating point value per pixel.
         */
        Red16f,

        /**
         * Two 16-bit half-precision floating point values per pixel.
         */
        RedGreen16f,

        /**
         * RGB stored as a 16-bit half-precision floating point value per component.
         */
        RGB16f,

        /**
         * RGBA stored as a 16-bit half-precision floating point value per component.
         */
        RGBA16f,

        /**
         * One 32-bit full-precision floating point value per pixel.
         */
        Red32f,

        /**
         * Two 32-bit full-precision floating point values per pixel.
         */
        RedGreen32f,

        /**
         * RGB stored as a 32-bit full-precision floating point value per component.
         */
        RGB32f,

        /**
         * RGBA stored as a 32-bit full-precision floating point value per component.
         */
        RGBA32f,

        /**
         * Depth stored as a 24-bit floating point value.
         */
        Depth,

        /**
         * Combined 24-bit depth and 8-bit stencil giving 32 bits per pixel.
         */
        Depth24Stencil8,

        /**
         * DXT1 compression. Only supported on 2D and cubemap images.
         */
        DXT1,

        /**
         * DXT3 compression. Only supported on 2D and cubemap images.
         */
        DXT3,

        /**
         * DXT5 compression. Only supported on 2D and cubemap images.
         */
        DXT5,

        /**
         * 2-bit RGB PVRTC compression. Only supported on square 2D and cubemap images.
         */
        PVRTC2BitRGB,

        /**
         * 2-bit RGBA PVRTC compression. Only supported on square 2D and cubemap images.
         */
        PVRTC2BitRGBA,

        /**
         * 4-bit RGB PVRTC compression. Only supported on square 2D and cubemap images.
         */
        PVRTC4BitRGB,

        /**
         * 4-bit RGBA PVRTC compression. Only supported on square 2D and cubemap images.
         */
        PVRTC4BitRGBA,

        /**
         * The size of this enumeration.
         */
        LastPixelFormat
    };

    Image() {}

    /**
     * Copy constructor.
     */
    Image(const Image& other);

    /**
     * Move constructor.
     */
    Image(Image&& other) noexcept;

    ~Image() { clear(); }

    /**
     * Assignment operator.
     */
    Image& operator=(Image other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * Swaps the contents of two Image instances.
     */
    friend void swap(Image& first, Image& second);

    /**
     * Clears this image's current contents and initializes it to hold a 2D/3D image of the specified dimensions, pixel
     * format, mipmap state, and frame count. If \a frameCount is zero then this method will not allocate any data. This
     * method returns false if \a frameCount is non-zero and an internal allocation fails.
     */
    bool initialize(unsigned int width, unsigned int height, unsigned int depth, PixelFormat pixelFormat,
                    bool hasMipmaps = false, unsigned int frameCount = 0);

    /**
     * Clears this image's current contents and initializes it to hold a cubemap image of the specified size, pixel
     * format, mipmap state, and frame count. If \a frameCount is zero then this method will not allocate any data. This
     * method returns false if \a frameCount is non-zero and an internal allocation fails.
     */
    bool initializeCubemap(unsigned int size, PixelFormat pixelFormat, bool hasMipmaps = false,
                           unsigned int frameCount = 0);

    /**
     * Clears this image's current contents and initializs it to be a cubemap built out of the six individual 2D images
     * specified. All six cubemap face images must have identical specifications. In order to avoid a copy of the image
     * data into the new cubemap, the contents of the passed images is transferred directly onto this image leaving the
     * source images empty after this call completes. Returns success flag.
     */
    bool initializeCubemap(std::array<Image, 6>& faces);

    /**
     * Erases this image definition and frees all associated data.
     */
    void clear();

    /**
     * Returns the width of this image.
     */
    unsigned int getWidth() const { return width_; }

    /**
     * The height of this image.
     */
    unsigned int getHeight() const { return height_; }

    /**
     * The depth of this image. This is used when working with 3D images, will be 1 for 2D and cubemap images.
     */
    unsigned int getDepth() const { return depth_; }

    /**
     * The pixel format used to store the image data.
     */
    PixelFormat getPixelFormat() const { return pixelFormat_; }

    /**
     * Converts this image to the specified pixel format, if the specified pixel format is the same as the current pixel
     * format then nothing is done. This method relies on being able to read and write the current and specified pixel
     * formats, which is supported for most but not all pixel formats. Returns success flag.
     */
    bool setPixelFormat(PixelFormat newPixelFormat);

    /**
     * Returns whether converting from a given pixel format into the target pixel format is supported by
     * Image::setPixelFormat().
     */
    bool canConvertToPixelFormat(PixelFormat pixelFormat) const;

    /**
     * Whether a full mipmap chain is stored with the image data. Incomplete mipmap chains are not allowed.
     */
    bool hasMipmaps() const { return hasMipmaps_; }

    /**
     * Whether this image is a cubemap. Cubemap images have a depth of 1 and equal widths and heights.
     */
    bool isCubemap() const { return isCubemap_; }

    /**
     * Returns the number of animation frames in this image, non-animated images have just one frame
     */
    unsigned int getFrameCount() const { return frames_.size(); }

    /**
     * Sets the number of frames in this image, this may internally alloate or deallocate memory for storing image data.
     * If an internal allocation fails then false is returned.
     */
    bool setFrameCount(unsigned int frameCount);

    /**
     * Returns the internal data pointer for the specified frame, returns null if this image is a cubemap.
     */
    const byte_t* getDataForFrame(unsigned int frameIndex) const { return frames_[frameIndex]->data.getData(); }

    /**
     * Returns the internal data pointer for the specified frame, returns null if this image is a cubemap.
     */
    byte_t* getDataForFrame(unsigned int frameIndex) { return frames_[frameIndex]->data.getData(); }

    /**
     * Returns the internal data pointer for the specified frame and cubemap side, returns null if this image is not a
     * cubemap. The \a faceIndex parameter must be in the range 0-5, the order of cubemap faces is positive x, negative
     * x, positive y, negative y, positive z, negative z.
     */
    const byte_t* getCubemapDataForFrame(unsigned int frameIndex, unsigned int faceIndex) const
    {
        assert(faceIndex < 6 && "Cubemap face index must be less than six");

        return frames_[frameIndex]->cubemapData[faceIndex].getData();
    }

    /**
     * Returns the internal data pointer for the specified frame and cubemap side, returns null if this image is not a
     * cubemap. The \a faceIndex parameter must be in the range 0-5, the order of cubemap faces is positive x, negative
     * x, positive y, negative y, positive z, negative z.
     */
    byte_t* getCubemapDataForFrame(unsigned int frameIndex, unsigned int faceIndex)
    {
        assert(faceIndex < 6 && "Cubemap face index must be less than six");

        return frames_[frameIndex]->cubemapData[faceIndex].getData();
    }

    /**
     * Returns pointers to all of the frame data in use by this image, in 2D and 3D images there is one block of data
     * per frame, and in cubemap images there are six blocks of data per frame.
     */
    Vector<byte_t*> getAllData();

    /**
     * Returns pointers to all of the frame data in use by this image, in 2D and 3D images there is one block of data
     * per frame, and in cubemap images there are six blocks of data per frame.
     */
    Vector<const byte_t*> getAllData() const;

    /**
     * Returns the number of mipmaps in this image's data. If Image::hasMipmaps() is true then the mipmap count is
     * calculated from this image's dimensions, otherwise 1 is returned.
     */
    unsigned int getMipmapCount() const
    {
        return hasMipmaps() ? getImageMipmapCount(getWidth(), getHeight(), getDepth()) : 1;
    }

    /**
     * Appends the frames in the passed image onto the end of this image. The image data is taken of the passed image
     * and so no copying or allocations are done. This method requires that this image and the passed image have the
     * same dimensions, pixel format, mipmap flag and cubemap flag. Returns success flag.
     */
    bool append(Image& image);

    /**
     * Returns whether this image is a valid 2D image.
     */
    bool isValid2DImage() const;

    /**
     * Returns whether this image is a valid 3D image.
     */
    bool isValid3DImage() const;

    /**
     * Returns whether this image is a valid cubemap image.
     */
    bool isValidCubemapImage() const;

    /**
     * Returns whether this is a valid Image object instance of any type, 2D, 3D or cubemap
     */
    bool isValidImage() const;

    /**
     * Returns whether any of the dimensions of this image are not powers of two.
     */
    bool isNPOT() const;

    /**
     * Returns the amount of memory used by each frame of this image. Note that for cubemap images this returns the
     * memory used by one face of the cubemap, not the full amount for all six cube faces. The return value is in bytes.
     */
    unsigned int getFrameDataSize() const;

    /**
     * Returns the total amount of memory being consumed by this image's data.
     */
    unsigned int getDataSize() const;

    /**
     * Returns the color of the pixel at the specified position. This is not supported on cubemap images or images
     * stored in a depth or depth/stencil format.
     */
    Color getPixelColor(unsigned int x, unsigned int y, unsigned int z = 0, unsigned int frame = 0) const;

    /**
     * Returns the color of the pixel at the given normalized texture coordinates. This is not supported on cubemap
     * images or images stored in a depth or depth/stencil format.
     */
    Color getPixelColor(float u, float v, float w = 0.0f, unsigned int frame = 0) const;

    /**
     * Sets the color of the pixel at the specified position. This is not supported on cubemap images or images stored
     * in a depth, depth/stencil or compressed pixel format. Returns success flag.
     */
    bool setPixelColor(unsigned int x, unsigned int y, unsigned int z, const Color& color);

    /**
     * \copydoc setPixelColor(unsigned int, unsigned int, unsigned int, const Color &)
     */
    void setPixelColor(unsigned int x, unsigned int y, const Color& color) { setPixelColor(x, y, 0, color); }

    /**
     * Writes this image to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Reads this image from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Returns a human-readable summary of the contents of this image, this includes dimensions, pixel format, etc...
     */
    operator UnicodeString() const;

    /**
     * Returns a hash value that covers all aspects of this image's properties and data.
     */
    unsigned int getHash() const;

    /**
     * Generates mipmaps for this image if it doesn't already have them. This is only supported for uncompressed pixel
     * formats. Returns success flag.
     */
    bool generateMipmaps();

    /**
     * Flips this image vertically, this operation is supported for most pixel formats. Returns success flag.
     */
    bool flipVertical();

    /**
     * Flips this image horizontally, this operation is supported for most pixel formats. Returns success flag.
     */
    bool flipHorizontal();

    /**
     * Rotates this image counter-clockwise 90 degrees, this operation is supported for most pixel formats. Returns
     * success flag.
     */
    bool rotateCCW();

    /**
     * Calls the passed fnImageOperation function once for every frame, cubemap face, and mipmap level in this image
     */
    bool enumerateImage(const std::function<bool(unsigned int, unsigned int, unsigned int, Image::PixelFormat,
                                                 byte_t*)>& fnImageOperation);

    /**
     * Does an in-place horizontal flip of the specified 2D/3D image data. Returns success flag.
     */
    static bool rawFlipHorizontal(unsigned int w, unsigned int h, unsigned int d, PixelFormat pf, byte_t* data);

    /**
     * Does an in-place vertical flip of the specified 2D/3D image data. Returns success flag.
     */
    static bool rawFlipVertical(unsigned int w, unsigned int h, unsigned int d, PixelFormat pf, byte_t* data);

    /**
     * Does an in-place counter-clockwise rotation by 90 degrees of the specified 2D/3D image data. Returns success
     * flag.
     */
    static bool rawRotateCCW(unsigned int w, unsigned int h, unsigned int d, PixelFormat pf, byte_t* data);

    /**
     * Returns the number of mipmaps required for the given image dimensions.
     */
    static unsigned int getImageMipmapCount(unsigned int width, unsigned int height, unsigned int depth);

    /**
     * Calculates the dimensions of the next smallest mipmap level from the dimensions of the current one.
     */
    static void getNextMipmapSize(unsigned int& width);

    /**
     * Calculates the dimensions of the next smallest mipmap level from the dimensions of the current one.
     */
    static void getNextMipmapSize(unsigned int& width, unsigned int& height);

    /**
     * Calculates the dimensions of the next smallest mipmap level from the dimensions of the current one.
     */
    static void getNextMipmapSize(unsigned int& width, unsigned int& height, unsigned int& depth);

    /**
     * Returns the number of bytes required to store an image of the given dimensions in a certain pixel format. The
     * size required to store this image as well as a full mipmap chain for it can be returned by setting \a hasMipmaps
     * to true.
     */
    static unsigned int getImageDataSize(unsigned int width, unsigned int height, unsigned int depth,
                                         PixelFormat pixelFormat, bool hasMipmaps = false);

    /**
     * Returns the size of a single compression block in the given pixel format. The pixel format must be compressed.
     */
    static unsigned int getCompressedPixelFormatBlockSize(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].compressionBlockSize;
    }

    /**
     * Returns a string identifier for the given pixel format.
     */
    static const String& getPixelFormatString(PixelFormat pixelFormat) { return pixelFormatDetails_[pixelFormat].name; }

    /**
     * Converts the given pixel format name into a pixel format enumeration value. Returns `UnknownPixelFormat` if the
     * passed string is not a recognized pixel format name. Case insensitive.
     */
    static PixelFormat getPixelFormatFromString(const String& pixelFormatName);

    /**
     * Returns the size in bytes required for a single pixel in the given pixel format. If the pixel format is
     * compressed then there is no per-pixel size and so zero will be returned.
     */
    static unsigned int getPixelFormatPixelSize(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].pixelSize;
    }

    /**
     * Returns whether the given pixel format is able to store alpha data in some form.
     */
    static bool isPixelFormatAlphaAware(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].isAlphaAware;
    }

    /**
     * Returns whether the given pixel format is an uncompressed format, i.e anything except PVRTC or DXT formats.
     */
    static bool isPixelFormatUncompressed(PixelFormat pixelFormat) { return !isPixelFormatCompressed(pixelFormat); }

    /**
     * Returns whether the given pixel format is a compressed format, ie. DXT or PVRTC.
     */
    static bool isPixelFormatCompressed(PixelFormat pixelFormat)
    {
        return isPixelFormatDXTCompressed(pixelFormat) || isPixelFormatPVRTCCompressed(pixelFormat);
    }

    /**
     * Returns whether the given pixel format is a DXT/S3TC compressed format
     */
    static bool isPixelFormatDXTCompressed(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].isDXTCompressed;
    }

    /**
     * Returns whether the given pixel format is a PVRTC compressed format
     */
    static bool isPixelFormatPVRTCCompressed(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].isPVRTCCompressed;
    }

    /**
     * Returns whether the given pixel format is a floating point format.
     */
    static bool isPixelFormatFloatingPoint(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].isFloatingPoint;
    }

    /**
     * Returns whether the given pixel format can be used to store depth buffer data.
     */
    static bool isPixelFormatDepthAware(PixelFormat pixelFormat)
    {
        return pixelFormatDetails_[pixelFormat].isDepthAware;
    }

    /**
     * This is a lower level image conversion routine than Image::setPixelFormat() that works with a single piece of
     * image data. In general the Image::setPixelFormat() method should be used instead of this method, however lower
     * level conversions can prove useful under certain circumstances. Note that Image::setPixelFormat() internally uses
     * this method to do conversions.
     */
    static bool convertRawImage(const byte_t* source, PixelFormat sourcePixelFormat, byte_t* destination,
                                PixelFormat destinationPixelFormat, unsigned int width, unsigned int height,
                                unsigned int depth);

    /**
     * Function typedef for reading the color of a single pixel in an image, this is implemented separately for each
     * pixel format.
     */
    typedef std::function<Color(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                unsigned int x, unsigned int y, unsigned int z)>
        ReadPixelFunction;

    /**
     * Function typedef for writing the color of a single pixel in an image, this is implemented separately for each
     * pixel format.
     */
    typedef std::function<void(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                               unsigned int x, unsigned int y, unsigned int z, const Color& color)>
        WritePixelFunction;

    /**
     * Returns a function for reading a single pixel value out of a piece of raw image data in the specified pixel
     * format, or null if reading individual pixels in that pixel format isn't supported.
     */
    static ReadPixelFunction getPixelFormatReadFunction(PixelFormat pixelFormat)
    {
        return pixelFormatReadWriteFunctions_[pixelFormat].first;
    }

    /**
     * Returns a function for writing a single pixel value to a piece of raw image data in the specified pixel format,
     * or null if writing individual pixels in that pixel format isn't supported.
     */
    static WritePixelFunction getPixelFormatWriteFunction(PixelFormat pixelFormat)
    {
        return pixelFormatReadWriteFunctions_[pixelFormat].second;
    }

    /**
     * This method is called automatically on startup to initialize the supported pixel formats.
     */
    static void setupPixelFormats();

    /**
     * This method is called automatically on startup to initialize the supported pixel format read and write functions.
     */
    static void setupPixelFormatReadWriteFunctions();

private:

    unsigned int width_ = 0;
    unsigned int height_ = 0;
    unsigned int depth_ = 0;
    PixelFormat pixelFormat_ = UnknownPixelFormat;

    bool hasMipmaps_ = false;
    bool isCubemap_ = false;

    // Holds pixel data for a single frame of this image
    struct SingleFrame
    {
        // The pixel data for a single frame of a non-cubemap image. Will include mipmap data if Image::hasMipmaps() of
        // the owning Image class is set to true. Mipmap data is tagged onto the data array in order of decreasing size.
        Vector<byte_t> data;

        // The pixel data for a single frame of a cubemap image. This is in the same format as the FrameData::data
        // member but with one pixel data array for each cubemap face. The cubemap sides are in the order: positive x,
        // negative x, positive y, negative y, positive z, negative z.
        std::array<Vector<byte_t>, 6> cubemapData;
    };

    Vector<SingleFrame*> frames_;

    void replaceFrameData(const byte_t* current, Vector<byte_t>& replacement);

    // The following structure and array holds details on each pixel format
    struct PixelFormatDetails
    {
        String name;

        bool isAlphaAware = false;

        // Set for uncompressed pixel formats
        unsigned int pixelSize;
        bool isDepthAware = false;
        bool isFloatingPoint = false;

        // Set for compressed pixel formats
        bool isDXTCompressed = false;
        bool isPVRTCCompressed = false;
        unsigned int compressionBlockSize = 0;
    };
    static std::array<PixelFormatDetails, LastPixelFormat> pixelFormatDetails_;

    // Extends values up to 8 bits, size must be less than 8
    static byte_t extendToByte(unsigned int bits, unsigned int size);

    // Retrieves a set of bits inside a 16-bit value that was stored in little endian format. Endian safe.
    static byte_t extract16BitValue(const byte_t* bits, unsigned int mask, unsigned int rightShift, unsigned int size);

    // Extract individual components out of a 16 bit 565 rgb value that was stored in little endian format. Endian safe.
    static byte_t rgb565GetR(const byte_t* bits) { return extract16BitValue(bits, 0xF800, 11, 5); }
    static byte_t rgb565GetG(const byte_t* bits) { return extract16BitValue(bits, 0x07E0, 5, 6); }
    static byte_t rgb565GetB(const byte_t* bits) { return extract16BitValue(bits, 0x001F, 0, 5); }

    static std::array<std::pair<ReadPixelFunction, WritePixelFunction>, LastPixelFormat> pixelFormatReadWriteFunctions_;

    // Some pixel formats require custom read and write functions
    static Color readSRGB8Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                unsigned int x, unsigned int y, unsigned int z);
    static void writeSRGB8Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readSRGBA8Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                 unsigned int x, unsigned int y, unsigned int z);
    static void writeSRGBA8Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                 unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readRGB565Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                 unsigned int x, unsigned int y, unsigned int z);
    static void writeRGB565Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                 unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readBGR565Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                 unsigned int x, unsigned int y, unsigned int z);
    static void writeBGR565Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                 unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readRGBA5551Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z);
    static void writeRGBA5551Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readARGB1555Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z);
    static void writeARGB1555Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readRGBA4444Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z);
    static void writeRGBA4444Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readARGB4444Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z);
    static void writeARGB4444Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z, const Color& color);
    static Color readABGR4444Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z);
    static void writeABGR4444Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                   unsigned int x, unsigned int y, unsigned int z, const Color& color);

    template <Image::PixelFormat SourcePixelFormat>
    static Color readDXTPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                              unsigned int x, unsigned int y, unsigned int z);

    // Helper methods for working with DXT formats, implemented in DXT.cpp
    static void decompressDXTCBlock(const byte_t* blockData, std::array<std::array<byte_t, 4>, 16>& output,
                                    Image::PixelFormat pixelFormat);
    static void decompressDXT3Alpha(const byte_t* alphaData, std::array<byte_t, 16>& output);
    static void decompressDXT5Alpha(const byte_t* alphaData, std::array<byte_t, 16>& output);
    static byte_t getDXTRGBInterp(const byte_t* rgbData, unsigned int x, unsigned int y);
    static void setDXTRGBInterp(byte_t* rgbData, unsigned int x, unsigned int y, byte_t interp);
    static byte_t getDXT3Alpha(const byte_t* alphaData, unsigned int x, unsigned int y);
    static void setDXT3Alpha(byte_t* alphaData, unsigned int x, unsigned int y, byte_t alpha);
    static byte_t getDXT5AlphaInterp(const byte_t* alphaData, unsigned int x, unsigned int y);
    static void setDXT5AlphaInterp(byte_t* alphaData, unsigned int x, unsigned int y, byte_t interp);

    // Read methods for PVRTC pixel formats, these are implemented in PVRTC.cpp
    static Color readPVRTC2BitRGBPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                       unsigned int x, unsigned int y, unsigned int z);
    static Color readPVRTC2BitRGBAPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                        unsigned int x, unsigned int y, unsigned int z);
    static Color readPVRTC4BitRGBPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                       unsigned int x, unsigned int y, unsigned int z);
    static Color readPVRTC4BitRGBAPixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth,
                                        unsigned int x, unsigned int y, unsigned int z);
};

}
