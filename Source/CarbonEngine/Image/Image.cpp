/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Math/HashFunctions.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

const UnicodeString Image::ImageExtension(".image");

const auto ImageVersionInfo = VersionInfo(3, 0);
const auto ImageHeaderID = FileSystem::makeFourCC("cimg");

std::array<Image::PixelFormatDetails, Image::LastPixelFormat> Image::pixelFormatDetails_ = {};

void Image::setupPixelFormats()
{
    pixelFormatDetails_[ABGR4444].isAlphaAware = true;
    pixelFormatDetails_[ABGR4444].name = "ABGR4444";
    pixelFormatDetails_[ABGR4444].pixelSize = 2;

    pixelFormatDetails_[ABGR8].isAlphaAware = true;
    pixelFormatDetails_[ABGR8].name = "ABGR8";
    pixelFormatDetails_[ABGR8].pixelSize = 4;

    pixelFormatDetails_[Alpha8].isAlphaAware = true;
    pixelFormatDetails_[Alpha8].name = "Alpha8";
    pixelFormatDetails_[Alpha8].pixelSize = 1;

    pixelFormatDetails_[ARGB1555].isAlphaAware = true;
    pixelFormatDetails_[ARGB1555].name = "ARGB1555";
    pixelFormatDetails_[ARGB1555].pixelSize = 2;

    pixelFormatDetails_[ARGB4444].isAlphaAware = true;
    pixelFormatDetails_[ARGB4444].name = "ARGB4444";
    pixelFormatDetails_[ARGB4444].pixelSize = 2;

    pixelFormatDetails_[BGR565].name = "BGR565";
    pixelFormatDetails_[BGR565].pixelSize = 2;

    pixelFormatDetails_[BGR8].name = "BGR8";
    pixelFormatDetails_[BGR8].pixelSize = 3;

    pixelFormatDetails_[BGRA8].isAlphaAware = true;
    pixelFormatDetails_[BGRA8].name = "BGRA8";
    pixelFormatDetails_[BGRA8].pixelSize = 4;

    pixelFormatDetails_[Depth24Stencil8].isDepthAware = true;
    pixelFormatDetails_[Depth24Stencil8].name = "Depth24Stencil8";
    pixelFormatDetails_[Depth24Stencil8].pixelSize = 4;

    pixelFormatDetails_[Depth].isDepthAware = true;
    pixelFormatDetails_[Depth].name = "Depth";
    pixelFormatDetails_[Depth].pixelSize = 4;

    pixelFormatDetails_[DXT1].compressionBlockSize = 8;
    pixelFormatDetails_[DXT1].isAlphaAware = true;
    pixelFormatDetails_[DXT1].isDXTCompressed = true;
    pixelFormatDetails_[DXT1].name = "DXT1";

    pixelFormatDetails_[DXT3].compressionBlockSize = 16;
    pixelFormatDetails_[DXT3].isAlphaAware = true;
    pixelFormatDetails_[DXT3].isDXTCompressed = true;
    pixelFormatDetails_[DXT3].name = "DXT3";

    pixelFormatDetails_[DXT5].compressionBlockSize = 16;
    pixelFormatDetails_[DXT5].isAlphaAware = true;
    pixelFormatDetails_[DXT5].isDXTCompressed = true;
    pixelFormatDetails_[DXT5].name = "DXT5";

    pixelFormatDetails_[Luminance8].name = "Luminance8";
    pixelFormatDetails_[Luminance8].pixelSize = 1;

    pixelFormatDetails_[LuminanceAlpha8].isAlphaAware = true;
    pixelFormatDetails_[LuminanceAlpha8].name = "LuminanceAlpha8";
    pixelFormatDetails_[LuminanceAlpha8].pixelSize = 2;

    pixelFormatDetails_[PVRTC2BitRGB].compressionBlockSize = 8;
    pixelFormatDetails_[PVRTC2BitRGB].isPVRTCCompressed = true;
    pixelFormatDetails_[PVRTC2BitRGB].name = "PVRTC2BitRGB";

    pixelFormatDetails_[PVRTC2BitRGBA].compressionBlockSize = 8;
    pixelFormatDetails_[PVRTC2BitRGBA].isAlphaAware = true;
    pixelFormatDetails_[PVRTC2BitRGBA].isPVRTCCompressed = true;
    pixelFormatDetails_[PVRTC2BitRGBA].name = "PVRTC2BitRGBA";

    pixelFormatDetails_[PVRTC4BitRGB].compressionBlockSize = 8;
    pixelFormatDetails_[PVRTC4BitRGB].isPVRTCCompressed = true;
    pixelFormatDetails_[PVRTC4BitRGB].name = "PVRTC4BitRGB";

    pixelFormatDetails_[PVRTC4BitRGBA].compressionBlockSize = 8;
    pixelFormatDetails_[PVRTC4BitRGBA].isAlphaAware = true;
    pixelFormatDetails_[PVRTC4BitRGBA].isPVRTCCompressed = true;
    pixelFormatDetails_[PVRTC4BitRGBA].name = "PVRTC4BitRGBA";

    pixelFormatDetails_[Red16f].isFloatingPoint = true;
    pixelFormatDetails_[Red16f].name = "Red16f";
    pixelFormatDetails_[Red16f].pixelSize = 2;

    pixelFormatDetails_[Red32f].isFloatingPoint = true;
    pixelFormatDetails_[Red32f].name = "Red32f";
    pixelFormatDetails_[Red32f].pixelSize = 4;

    pixelFormatDetails_[RedGreen16f].isFloatingPoint = true;
    pixelFormatDetails_[RedGreen16f].name = "RedGreen16f";
    pixelFormatDetails_[RedGreen16f].pixelSize = 4;

    pixelFormatDetails_[RedGreen32f].isFloatingPoint = true;
    pixelFormatDetails_[RedGreen32f].name = "RedGreen32f";
    pixelFormatDetails_[RedGreen32f].pixelSize = 8;

    pixelFormatDetails_[RGB16f].isFloatingPoint = true;
    pixelFormatDetails_[RGB16f].name = "RGB16f";
    pixelFormatDetails_[RGB16f].pixelSize = 6;

    pixelFormatDetails_[RGB32f].isFloatingPoint = true;
    pixelFormatDetails_[RGB32f].name = "RGB32f";
    pixelFormatDetails_[RGB32f].pixelSize = 12;

    pixelFormatDetails_[RGB565].name = "RGB565";
    pixelFormatDetails_[RGB565].pixelSize = 2;

    pixelFormatDetails_[RGB8].name = "RGB8";
    pixelFormatDetails_[RGB8].pixelSize = 3;

    pixelFormatDetails_[RGBA16f].isAlphaAware = true;
    pixelFormatDetails_[RGBA16f].isFloatingPoint = true;
    pixelFormatDetails_[RGBA16f].name = "RGBA16f";
    pixelFormatDetails_[RGBA16f].pixelSize = 8;

    pixelFormatDetails_[RGBA32f].isAlphaAware = true;
    pixelFormatDetails_[RGBA32f].isFloatingPoint = true;
    pixelFormatDetails_[RGBA32f].name = "RGBA32f";
    pixelFormatDetails_[RGBA32f].pixelSize = 16;

    pixelFormatDetails_[RGBA4444].isAlphaAware = true;
    pixelFormatDetails_[RGBA4444].name = "RGBA4444";
    pixelFormatDetails_[RGBA4444].pixelSize = 2;

    pixelFormatDetails_[RGBA5551].isAlphaAware = true;
    pixelFormatDetails_[RGBA5551].name = "RGBA5551";
    pixelFormatDetails_[RGBA5551].pixelSize = 2;

    pixelFormatDetails_[RGBA8].isAlphaAware = true;
    pixelFormatDetails_[RGBA8].name = "RGBA8";
    pixelFormatDetails_[RGBA8].pixelSize = 4;

    pixelFormatDetails_[SRGB8].name = "SRGB8";
    pixelFormatDetails_[SRGB8].pixelSize = 3;

    pixelFormatDetails_[SRGBA8].isAlphaAware = true;
    pixelFormatDetails_[SRGBA8].name = "SRGBA8";
    pixelFormatDetails_[SRGBA8].pixelSize = 4;

    pixelFormatDetails_[UnknownPixelFormat].name = "Unknown";
}
CARBON_REGISTER_STARTUP_FUNCTION(Image::setupPixelFormats, 0)

Image::Image(const Image& other)
    : width_(other.width_),
      height_(other.height_),
      depth_(other.depth_),
      pixelFormat_(other.pixelFormat_),
      hasMipmaps_(other.hasMipmaps_),
      isCubemap_(other.isCubemap_)
{
    for (auto frame : other.frames_)
        frames_.append(new SingleFrame(*frame));
}

Image::Image(Image&& other) noexcept : width_(other.width_),
                                       height_(other.height_),
                                       depth_(other.depth_),
                                       pixelFormat_(other.pixelFormat_),
                                       hasMipmaps_(other.hasMipmaps_),
                                       isCubemap_(other.isCubemap_),
                                       frames_(std::move(other.frames_))
{
    other.clear();
}

void swap(Image& first, Image& second)
{
    using std::swap;

    swap(first.width_, second.width_);
    swap(first.height_, second.height_);
    swap(first.depth_, second.depth_);
    swap(first.pixelFormat_, second.pixelFormat_);
    swap(first.hasMipmaps_, second.hasMipmaps_);
    swap(first.isCubemap_, second.isCubemap_);
    swap(first.frames_, second.frames_);
}

void Image::clear()
{
    width_ = 0;
    height_ = 0;
    depth_ = 0;
    pixelFormat_ = UnknownPixelFormat;
    hasMipmaps_ = false;
    isCubemap_ = false;

    for (auto frame : frames_)
        delete frame;
    frames_.clear();
}

bool Image::initialize(unsigned int width, unsigned int height, unsigned int depth, PixelFormat pixelFormat, bool hasMipmaps,
                       unsigned int frameCount)
{
    clear();

    width_ = width;
    height_ = height;
    depth_ = depth;
    pixelFormat_ = pixelFormat;
    hasMipmaps_ = hasMipmaps;

    if (!setFrameCount(frameCount) || !isValidImage())
    {
        clear();
        return false;
    }

    return true;
}

bool Image::initializeCubemap(unsigned int size, PixelFormat pixelFormat, bool hasMipmaps, unsigned int frameCount)
{
    clear();

    width_ = size;
    height_ = size;
    depth_ = 1;
    pixelFormat_ = pixelFormat;
    hasMipmaps_ = hasMipmaps;
    isCubemap_ = true;

    return setFrameCount(frameCount);
}

bool Image::initializeCubemap(std::array<Image, 6>& faces)
{
    // Check faces are all valid and identical
    for (auto& face : faces)
    {
        if (!face.isValid2DImage() || face.getWidth() != faces[0].getWidth() || face.getHeight() != faces[0].getHeight() ||
            face.hasMipmaps() != faces[0].hasMipmaps() || face.getFrameCount() != faces[0].getFrameCount() ||
            face.getPixelFormat() != faces[0].getPixelFormat())
            return false;
    }

    // Setup new cubemap image
    clear();
    width_ = faces[0].getWidth();
    height_ = faces[0].getHeight();
    depth_ = 1;
    pixelFormat_ = faces[0].pixelFormat_;
    hasMipmaps_ = faces[0].hasMipmaps_;
    isCubemap_ = true;

    // Move the data from the source images onto the cubemap
    for (auto i = 0U; i < faces[0].getFrameCount(); i++)
    {
        frames_.append(new SingleFrame);
        for (auto j = 0U; j < 6; j++)
            frames_.back()->cubemapData[j] = std::move(faces[j].frames_[i]->data);
    }

    return true;
}

bool Image::isValid2DImage() const
{
    return width_ && height_ && depth_ == 1 && pixelFormat_ != UnknownPixelFormat && !isCubemap_ && !frames_.empty();
}

bool Image::isValid3DImage() const
{
    return width_ && height_ && depth_ == 1 && pixelFormat_ != UnknownPixelFormat && !isCubemap_ && !frames_.empty() &&
        !isPixelFormatDXTCompressed(pixelFormat_) && !isPixelFormatPVRTCCompressed(pixelFormat_);
}

bool Image::isValidCubemapImage() const
{
    return width_ && height_ && depth_ == 1 && width_ == height_ && pixelFormat_ != UnknownPixelFormat && isCubemap_ &&
        !frames_.empty();
}

bool Image::isValidImage() const
{
    return isValid2DImage() || isValid3DImage() || isValidCubemapImage();
}

bool Image::isNPOT() const
{
    return isValidImage() && (!Math::isPowerOfTwo(width_) || !Math::isPowerOfTwo(height_) || !Math::isPowerOfTwo(depth_));
}

unsigned int Image::getFrameDataSize() const
{
    return getImageDataSize(width_, height_, depth_, pixelFormat_, hasMipmaps_);
}

unsigned int Image::getDataSize() const
{
    auto size = 0U;

    for (auto frame : frames_)
    {
        if (!isCubemap_)
            size += frame->data.size();
        else
        {
            for (auto& cubemapData : frame->cubemapData)
                size += cubemapData.size();
        }
    }

    return size;
}

bool Image::setFrameCount(unsigned int frameCount)
{
    while (frames_.size() > frameCount)
        delete frames_.popBack();

    for (auto i = 0U; i < frameCount; i++)
    {
        auto newFrame = new SingleFrame;

        try
        {
            if (isCubemap())
            {
                for (auto& cubemapData : newFrame->cubemapData)
                    cubemapData.resize(Image::getFrameDataSize());
            }
            else
                newFrame->data.resize(Image::getFrameDataSize());
        }
        catch (const std::bad_alloc&)
        {
            delete newFrame;
            return false;
        }

        frames_.append(newFrame);
    }

    return true;
}

bool Image::append(Image& image)
{
    if (getWidth() != image.getWidth() || getHeight() != image.getHeight() || getDepth() != image.getDepth() ||
        getPixelFormat() != image.getPixelFormat() || hasMipmaps() != image.hasMipmaps() || isCubemap() != image.isCubemap())
    {
        LOG_ERROR << "Unable to append images with incompatible definitions";
        return false;
    }

    frames_.append(image.frames_);
    image.frames_.clear();

    return true;
}

void Image::replaceFrameData(const byte_t* current, Vector<byte_t>& replacement)
{
    if (!current)
        return;

    for (auto i = 0U; i < getFrameCount(); i++)
    {
        if (isCubemap())
        {
            for (auto j = 0U; j < 6; j++)
            {
                if (getCubemapDataForFrame(i, j) == current)
                {
                    frames_[i]->cubemapData[j].clear();
                    swap(frames_[i]->cubemapData[j], replacement);

                    return;
                }
            }
        }
        else
        {
            if (getDataForFrame(i) == current)
            {
                frames_[i]->data.clear();
                swap(frames_[i]->data, replacement);

                return;
            }
        }
    }
}

Vector<byte_t*> Image::getAllData()
{
    auto data = Vector<byte_t*>();

    for (auto i = 0U; i < getFrameCount(); i++)
    {
        if (!isCubemap())
            data.append(getDataForFrame(i));
        else
        {
            for (auto j = 0U; j < 6; j++)
                data.append(getCubemapDataForFrame(i, j));
        }
    }

    return data;
}

Vector<const byte_t*> Image::getAllData() const
{
    auto data = Vector<const byte_t*>();

    for (auto i = 0U; i < getFrameCount(); i++)
    {
        if (!isCubemap())
            data.append(getDataForFrame(i));
        else
        {
            for (auto j = 0U; j < 6; j++)
                data.append(getCubemapDataForFrame(i, j));
        }
    }

    return data;
}

Color Image::getPixelColor(unsigned int x, unsigned int y, unsigned int z, unsigned int frame) const
{
    auto fnRead = getPixelFormatReadFunction(pixelFormat_);
    if (!fnRead)
        return Color::Zero;

    return fnRead(frames_[frame]->data.getData(), width_, height_, depth_, x, y, z);
}

Color Image::getPixelColor(float u, float v, float w, unsigned int frame) const
{
    if (!isValidImage())
        return Color::Zero;

    auto x = Math::clamp(uint(Math::clamp01(u) * float(getWidth() - 1) + 0.5f), 0U, getWidth() - 1);
    auto y = Math::clamp(uint(Math::clamp01(v) * float(getHeight() - 1) + 0.5f), 0U, getHeight() - 1);
    auto z = Math::clamp(uint(Math::clamp01(w) * float(getHeight() - 1) + 0.5f), 0U, getDepth() - 1);

    return getPixelColor(x, y, z, frame);
}

bool Image::setPixelColor(unsigned int x, unsigned int y, unsigned int z, const Color& color)
{
    auto fnWrite = getPixelFormatWriteFunction(pixelFormat_);
    if (!fnWrite)
        return false;

    fnWrite(frames_[0]->data.getData(), width_, height_, depth_, x, y, z, color);

    return true;
}

void Image::save(FileWriter& file) const
{
    if (!isValidImage())
        throw Exception("Can't save invalid or incomplete image");

    // Write header ID
    file.write(ImageHeaderID);

    file.beginVersionedSection(ImageVersionInfo);

    // Write image specifications
    file.write(width_, height_, depth_, getPixelFormatString(pixelFormat_), hasMipmaps_, isCubemap_, frames_.size());

    // Write image data
    for (auto frame : frames_)
    {
        if (isCubemap())
        {
            for (auto& cubemapData : frame->cubemapData)
                file.writeBytes(cubemapData.getData(), getFrameDataSize());
        }
        else
            file.writeBytes(frame->data.getData(), getFrameDataSize());
    }

    file.endVersionedSection();
}

void Image::load(FileReader& file)
{
    try
    {
        clear();

        // Read header
        if (file.readFourCC() != ImageHeaderID)
            throw Exception("Not an image file");

        file.beginVersionedSection(ImageVersionInfo);

        // Read image specifications
        auto pixelFormatName = String();
        auto frameCount = 0U;
        file.read(width_, height_, depth_, pixelFormatName, hasMipmaps_, isCubemap_, frameCount);
        pixelFormat_ = getPixelFormatFromString(pixelFormatName);

        // Allocate frames
        if (!setFrameCount(frameCount))
            throw Exception() << "Failed allocating image data, frame count: " << frameCount;

        // Check image is valid
        if (!isValidImage())
            throw Exception("Invalid image");

        // Read image data
        for (auto i = 0U; i < frames_.size(); i++)
        {
            if (isCubemap())
            {
                for (auto j = 0U; j < 6; j++)
                    file.readBytes(getCubemapDataForFrame(i, j), getFrameDataSize());
            }
            else
                file.readBytes(getDataForFrame(i), getFrameDataSize());
        }

        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

Image::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    // Image type
    if (isValid2DImage())
        info.append("2D");
    else if (isValid3DImage())
        info.append("3D");
    else if (isValidCubemapImage())
        info.append("Cubemap");
    else
        info.append("Invalid");

    info.back() << " image";

    // Dimensions
    info.append(UnicodeString() + "dimensions: " + width_ + "x" + height_);
    if (depth_ > 1)
        info.back() << "x" << depth_;
    else if (isValidCubemapImage())
        info.append("x6");

    // Pixel format
    info.append("pixel format: " + Image::getPixelFormatString(pixelFormat_));

    // Mipmaps
    info.append(UnicodeString() + "mipmaps: " + hasMipmaps_);

    // Frames
    if (frames_.size() > 1)
        info.append(UnicodeString() + "frames: " + frames_.size());

    // Total data size
    info.append("data size: " + FileSystem::formatByteSize(getDataSize()));

    return UnicodeString(info);
}

unsigned int Image::getHash() const
{
    // Start by hashing the image properties
    auto imageDetails = std::array<unsigned int, 7>{
        {width_, height_, depth_, uint(pixelFormat_), uint(hasMipmaps_), uint(isCubemap_), frames_.size()}};

    auto hash = HashFunctions::hash(imageDetails);

    // Now add in hashes for any image data that may be present
    for (auto frame : frames_)
    {
        if (!isCubemap_)
            hash += HashFunctions::hash(frame->data);
        else
        {
            for (auto& cubemapData : frame->cubemapData)
                hash += HashFunctions::hash(cubemapData);
        }
    }

    return hash;
}

bool Image::generateMipmaps()
{
    if (hasMipmaps())
        return true;

    // Mipmap generation is only supported for uncompressed 8-bit per component linear textures
    if (getPixelFormat() != RGB8 && getPixelFormat() != RGBA8 && getPixelFormat() != BGR8 && getPixelFormat() != BGRA8 &&
        getPixelFormat() != ABGR8 && getPixelFormat() != Alpha8 && getPixelFormat() != Luminance8 &&
        getPixelFormat() != LuminanceAlpha8)
    {
        LOG_ERROR << "Unsupported pixel format: " << getPixelFormatString(getPixelFormat());
        return false;
    }

    // Store a few relevant values
    auto mipmapCount = getImageMipmapCount(getWidth(), getHeight(), getDepth());
    auto componentCount = getPixelFormatPixelSize(getPixelFormat());

    // Generate mipmaps for all the sources
    for (auto source : getAllData())
    {
        auto w = getWidth();
        auto h = getHeight();
        auto d = getDepth();

        // Allocate data for new mipmap chain
        auto mipmapChain = Vector<byte_t>();
        try
        {
            mipmapChain.resize(getImageDataSize(getWidth(), getHeight(), getDepth(), getPixelFormat(), true));
        }
        catch (const std::bad_alloc&)
        {
            LOG_ERROR << "Failed allocating memory for the mipmap data";
            return false;
        }

        // Copy over the image data for the base level
        auto mipmap = mipmapChain.getData();
        memcpy(mipmap, source, getImageDataSize(w, h, d, getPixelFormat(), false));

        // Calculate contents of subsequent mipmap levels
        for (auto j = 1U; j < mipmapCount; j++)
        {
            if (!w || !h || !d)
                break;

            // Store info on the previous level so we can downsample from it when generating this mipmap
            auto prevLevel = mipmap;
            auto prevWidth = w;
            auto prevHeight = h;
            auto prevDepth = d;

            // Move to start of this mipmap
            mipmap += getImageDataSize(w, h, d, getPixelFormat(), false);

            // Calculate the dimensions of the new mipmap
            getNextMipmapSize(w, h, d);

            // Calculate mipmap data by doing a 2x2x2 downsample of the previous level
            auto p = mipmap;
            for (auto z = 0U; z < d; z++)
            {
                auto zOff1 = ((z * 2) % prevDepth) * prevWidth * prevHeight;
                auto zOff2 = ((z * 2 + 1) % prevDepth) * prevWidth * prevHeight;

                for (auto y = 0U; y < h; y++)
                {
                    auto yOff1 = ((y * 2) % prevHeight) * prevWidth;
                    auto yOff2 = ((y * 2 + 1) % prevHeight) * prevWidth;

                    for (auto x = 0U; x < w; x++)
                    {
                        auto xOff1 = (x * 2) % prevWidth;
                        auto xOff2 = (x * 2 + 1) % prevWidth;

                        auto p0 = &prevLevel[(zOff1 + yOff1 + xOff1) * componentCount];
                        auto p1 = &prevLevel[(zOff1 + yOff1 + xOff2) * componentCount];
                        auto p2 = &prevLevel[(zOff1 + yOff2 + xOff1) * componentCount];
                        auto p3 = &prevLevel[(zOff1 + yOff2 + xOff2) * componentCount];
                        auto p4 = &prevLevel[(zOff2 + yOff1 + xOff1) * componentCount];
                        auto p5 = &prevLevel[(zOff2 + yOff1 + xOff2) * componentCount];
                        auto p6 = &prevLevel[(zOff2 + yOff2 + xOff1) * componentCount];
                        auto p7 = &prevLevel[(zOff2 + yOff2 + xOff2) * componentCount];

                        for (auto c = 0U; c < componentCount; c++)
                            *p++ = (*p0++ + *p1++ + *p2++ + *p3++ + *p4++ + *p5++ + *p6++ + *p7++) >> 3;
                    }
                }
            }
        }

        replaceFrameData(source, mipmapChain);
    }

    hasMipmaps_ = true;

    return true;
}

unsigned int Image::getImageMipmapCount(unsigned int width, unsigned int height, unsigned int depth)
{
    // Get largest component as this is what determines the number of mipmaps
    auto largest = std::max(width, std::max(height, depth));
    auto mipmapCount = 1U;

    while (largest > 1)
    {
        mipmapCount++;
        largest /= 2;
    }

    return mipmapCount;
}

void Image::getNextMipmapSize(unsigned int& width)
{
    width = std::max(width / 2, 1U);
}

void Image::getNextMipmapSize(unsigned int& width, unsigned int& height)
{
    width = std::max(width / 2, 1U);
    height = std::max(height / 2, 1U);
}

void Image::getNextMipmapSize(unsigned int& width, unsigned int& height, unsigned int& depth)
{
    width = std::max(width / 2, 1U);
    height = std::max(height / 2, 1U);
    depth = std::max(depth / 2, 1U);
}

unsigned int Image::getImageDataSize(unsigned int width, unsigned int height, unsigned int depth, PixelFormat pixelFormat,
                                     bool hasMipmaps)
{
    auto dataSize = 0U;

    auto mipmapCount = hasMipmaps ? getImageMipmapCount(width, height, depth) : 1U;

    for (auto i = 0U; i < mipmapCount; i++)
    {
        auto w = width;
        auto h = height;

        if (isPixelFormatUncompressed(pixelFormat))
            dataSize += w * h * depth * getPixelFormatPixelSize(pixelFormat);
        else
        {
            auto compressionBlockWidth = 4U;
            auto compressionBlockHeight = 4U;

            // 2-bit PVRTC formats use a 8x4 copmression block
            if (pixelFormat == PVRTC2BitRGB || pixelFormat == PVRTC2BitRGBA)
                compressionBlockWidth = 8;

            auto xBlocks = (w + compressionBlockWidth - 1) / compressionBlockWidth;
            auto yBlocks = (h + compressionBlockHeight - 1) / compressionBlockHeight;

            auto size = xBlocks * yBlocks * depth * getCompressedPixelFormatBlockSize(pixelFormat);

            // PVRTC formats have a minimum size of 32 bytes
            if (pixelFormat == PVRTC2BitRGB || pixelFormat == PVRTC2BitRGBA || pixelFormat == PVRTC4BitRGB ||
                pixelFormat == PVRTC4BitRGBA)
                size = std::max(size, 32U);

            dataSize += size;
        }

        getNextMipmapSize(width, height, depth);
    }

    return dataSize;
}

Image::PixelFormat Image::getPixelFormatFromString(const String& pixelFormatName)
{
    for (auto i = 0U; i < LastPixelFormat; i++)
    {
        if (pixelFormatDetails_[i].name == pixelFormatName.asLower())
            return PixelFormat(i);
    }

    return UnknownPixelFormat;
}

byte_t Image::extendToByte(unsigned int bits, unsigned int size)
{
    // Given a value that uses fewer than 8 bits and a size in bits of that value this method extends the value to an 8-bit
    // value by duplicating the original bits into the lower portion

    bits &= (1 << size) - 1;

    switch (size)
    {
        case 1:
            return byte_t(bits ? 0xFF : 0x00);
        case 2:
            return byte_t((bits << 6) | (bits << 4) | (bits << 2) | bits);
        case 3:
            return byte_t((bits << 5) | (bits << 2) | (bits >> 1));
        case 4:
            return byte_t((bits << 4) | bits);
        case 5:
            return byte_t((bits << 3) | (bits >> 2));
        case 6:
            return byte_t((bits << 2) | (bits >> 4));
        case 7:
            return byte_t((bits << 1) | (bits >> 6));
        default:
            assert(false && "Size in bits must be from 1 to 7");
            return byte_t(bits);
    }
}

byte_t Image::extract16BitValue(const byte_t* bits, unsigned int mask, unsigned int rightShift, unsigned int size)
{
    auto color = *reinterpret_cast<const uint16_t*>(bits);

#ifdef CARBON_BIG_ENDIAN
    Endian::convert(color);
#endif

    return extendToByte((color & mask) >> rightShift, size);
}

}
