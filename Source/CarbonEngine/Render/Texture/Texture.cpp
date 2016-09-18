/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Render/RenderEvents.h"
#include "CarbonEngine/Render/Texture/Texture.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

const UnicodeString Texture::TextureDirectory = "Textures/";

Texture::Texture()
{
    events().addHandler<GatherMemorySummaryEvent>(this);
    clear();
}

Texture::~Texture()
{
    clear();
    events().removeHandler(this);
}

void Texture::setCurrentFrame(unsigned int frame)
{
    if (image_.getFrameCount())
        currentFrame_ = frame % image_.getFrameCount();
    else
        currentFrame_ = 0;
}

void Texture::clear()
{
    name_.clear();
    isLoadedFromFile_ = false;

    deupload();

    image_.clear();
    currentFrame_ = 0;
    textureObjects_.clear();
    properties_ = TextureProperties();
    group_.clear();
    state_ = Uninitialized;
    videoMemoryUsed_ = 0;
    isImageDataLocked_ = false;
}

bool Texture::load(const String& name, const String& group)
{
    if (isImageDataLocked_)
    {
        LOG_ERROR << "Image data is locked";
        return false;
    }

    clear();

    name_ = name;
    isLoadedFromFile_ = true;

    group_ = group;
    if (group_.length())
        properties_ = textures().getGroupProperties(group_);

    state_ = ImageLoadPending;

    return true;
}

bool Texture::load(const String& name, Image image, const String& group)
{
    try
    {
        if (isImageDataLocked_)
            throw Exception("Texture image data is locked");

        if (!image.isValidImage())
            throw Exception("Invalid source image");

        clear();

        // Copy the image
        image_ = std::move(image);

        name_ = name;
        group_ = group;
        if (group_.length())
            properties_ = textures().getGroupProperties(group);

        sendTextureLoadedEvent(name_, image_, getTextureType());

        state_ = UploadPending;

        LOG_INFO << "Texture loaded - '" << name_ << "' - " << *this;

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        return false;
    }
}

void Texture::sendTextureLoadedEvent(const String& name, Image& image, GraphicsInterface::TextureType type)
{
    // System textures begin with a '.' and don't trigger TextureLoadedEvent
    if (name.startsWith(String::Period))
        return;

    auto textureLoadedEvent = TextureLoadedEvent(name, image, type);
    events().dispatchEvent(textureLoadedEvent);

    // Convert texture image if needed
    if (image.getPixelFormat() != textureLoadedEvent.getNewPixelFormat())
    {
        if (!image.setPixelFormat(textureLoadedEvent.getNewPixelFormat()))
        {
            LOG_INFO << "Failed converting texture '" << name << "' pixel format to "
                     << Image::getPixelFormatString(textureLoadedEvent.getNewPixelFormat());
        }
    }
}

void Texture::setProperties(const TextureProperties& properties)
{
    auto runUploadImmediately = false;

    // Changes in texture quality necessitate a re-upload of this texture if it has already been uploaded to the
    // graphics interface
    if (properties.getQuality() != properties_.getQuality())
    {
        if (state_ == Ready)
        {
            runUploadImmediately = true;
            state_ = UploadPending;
        }
    }

    const auto textureType = getTextureType();
    const auto anisotropy = std::min(properties.getAnisotropy(), graphics().getMaximumTextureAnisotropy(textureType));

    for (auto t : textureObjects_)
    {
        if (image_.hasMipmaps())
        {
            if (properties.getFilter() == TextureProperties::NearestFilter)
                graphics().setTextureFilter(t, textureType, GraphicsInterface::FilterNearestMipmapNearest,
                                            GraphicsInterface::FilterNearest);
            else if (properties.getFilter() == TextureProperties::BilinearFilter)
                graphics().setTextureFilter(t, textureType, GraphicsInterface::FilterNearestMipmapLinear,
                                            GraphicsInterface::FilterLinear);
            else
                graphics().setTextureFilter(t, textureType, GraphicsInterface::FilterLinearMipmapLinear,
                                            GraphicsInterface::FilterLinear);
        }
        else
        {
            if (properties.getFilter() == TextureProperties::NearestFilter)
                graphics().setTextureFilter(t, textureType, GraphicsInterface::FilterNearest,
                                            GraphicsInterface::FilterNearest);
            else
                graphics().setTextureFilter(t, textureType, GraphicsInterface::FilterLinear,
                                            GraphicsInterface::FilterLinear);
        }

        graphics().setTextureWrap(t, textureType, properties.getWrap());
        graphics().setTextureAnisotropy(t, textureType, anisotropy);
    }

    properties_ = properties;

    if (runUploadImmediately)
        upload();
}

void Texture::deupload()
{
    for (auto textureObject : textureObjects_)
        graphics().deleteTexture(textureObject);

    textureObjects_.clear();

    state_ = UploadPending;
}

void Texture::ensureImageIsLoaded()
{
    if (state_ == ImageLoadPending)
    {
        auto timer = SimpleTimer();

        if (loadTextureImage(name_, image_))
        {
            sendTextureLoadedEvent(name_, image_, getTextureType());
            state_ = UploadPending;

            LOG_INFO << "Texture loaded - '" << name_ << "' - " << *this << ", time: " << timer << " (main thread)";
        }
        else
        {
            state_ = Error;
            image_.clear();

            LOG_ERROR_WITHOUT_CALLER << "Missing texture: " << name_;
        }
    }
}

Image* Texture::lockImageData()
{
    if (isImageDataLocked_ || (state_ != Ready && state_ != UploadPending))
        return nullptr;

    isImageDataLocked_ = true;

    return &image_;
}

bool Texture::unlockImageData()
{
    if (!isImageDataLocked_)
        return false;

    isImageDataLocked_ = false;

    if (state_ == Ready)
        state_ = UploadPending;

    upload();

    return true;
}

const Image& Texture::getUploadableImage(Image& temporaryImage) const
{
    auto image = &image_;

    if (!graphics().isPixelFormatSupported(image->getPixelFormat(), getTextureType()))
    {
        temporaryImage = *image;
        auto newPixelFormat = graphics().getFallbackPixelFormat(getTextureType(), image->getPixelFormat());
        if (!temporaryImage.setPixelFormat(newPixelFormat))
            throw Exception() << "Failed converting image to a supported pixel format: " << *image;

        image = &temporaryImage;
    }
    else
        image = &getImage();

    if (!graphics().isTextureSupported(getTextureType(), *image))
        throw Exception() << "Hardware does not support this texture image: " << *image;

    return *image;
}

bool Texture::downloadAndSaveToFile(const UnicodeString& filename, Image::PixelFormat pixelFormat) const
{
    auto image = Image();
    if (!graphics().downloadTexture(getActiveTextureObject(), getTextureType(), pixelFormat, image))
    {
        LOG_ERROR << "Failed downloading texture from the graphics interface";
        return false;
    }

    return ImageFormatRegistry::saveImageFile(filename, image);
}

unsigned int Texture::calculateFirstMipmapLevel()
{
    // Use the primary image if there is no mipmap chain
    if (!image_.hasMipmaps())
        return 0;

    // If texture quality is set to the maximum then start with the first mipmap level
    if (properties_.getQuality() == TextureProperties::TextureQualityMaximum)
        return 0;

    auto firstMipmap = 0U;
    auto largest = std::max(std::max(image_.getWidth(), image_.getHeight()), image_.getDepth());

    while (largest > properties_.getQuality())
    {
        Image::getNextMipmapSize(largest);
        firstMipmap++;
    }

    return firstMipmap;
}

bool Texture::doesTextureFileExist(const String& name)
{
    auto fullName = String();

    // Prefix with TextureDirectory unless the passed name starts with a forward slash indicating an absolute path, or a
    // $ which would indicate that the $<volume name>$/ syntax is in use
    if (name.at(0) != '/' && name.at(0) != '$')
        fullName = A(TextureDirectory) + name;
    else
        fullName = name;

    return ImageFormatRegistry::doesFileExist(fullName);
}

bool Texture::loadTextureImage(const String& name, Image& image)
{
    image.clear();

    if (!name.length())
        return false;

    auto prefixes = Vector<String>(1, String::Empty);

    // A prefix of TextureDirectory is preferred unless the passed name starts with a forward slash indicating an
    // absolute path, or a $ which would indicate that the $<volume name>$/ syntax is in use. A non-prefixed path is
    // always tried.
    if (name.at(0) != '/' && name.at(0) != '$')
        prefixes.prepend(A(TextureDirectory));

    for (auto& prefix : prefixes)
    {
        // Send a BeforeTextureImageLoadEvent to get the target pixel format to pass to
        // ImageFormatRegistry::loadImageFile()
        auto beforeTextureImageLoadEvent = BeforeTextureImageLoadEvent(prefix + name);
        events().dispatchEvent(beforeTextureImageLoadEvent);

        // Try and load the image file
        if (ImageFormatRegistry::loadImageFile(beforeTextureImageLoadEvent.getImageName(), image,
                                               beforeTextureImageLoadEvent.getTargetPixelFormat()))
            return true;

        // The above load didn't work, so try loading a cubemap from 6 individual image files, one for each face of the
        // cubemap. The first of the suffix configs below is the native format, the second config pattern is Quake 3's
        // setup.

        struct SuffixConfig
        {
            String suffix;
            bool flipVertical;
            bool flipHorizontal;
            bool rotateCCW;
        };

        static const auto suffixes =
            std::array<std::array<SuffixConfig, 2>, 6>{{{{{"Left", true, true, false}, {"_lf", true, false, false}}},
                                                        {{{"Right", true, true, false}, {"_rt", true, false, false}}},
                                                        {{{"Up", false, false, false}, {"_up", true, false, true}}},
                                                        {{{"Down", false, false, false}, {"_dn", false, true, true}}},
                                                        {{{"Front", true, true, false}, {"_ft", true, false, false}}},
                                                        {{{"Back", true, true, false}, {"_bk", true, false, false}}}}};

        auto cubemapFaces = std::array<Image, 6>();
        for (auto i = 0U; i < cubemapFaces.size(); i++)
        {
            auto& cubemapFace = cubemapFaces[i];

            auto suffix = pointer_to<const SuffixConfig>::type();

            for (auto& s : suffixes[i])
            {
                beforeTextureImageLoadEvent.setImageName(prefix + name + s.suffix);
                events().dispatchEvent(beforeTextureImageLoadEvent);

                if (ImageFormatRegistry::loadImageFile(beforeTextureImageLoadEvent.getImageName(), cubemapFace,
                                                       beforeTextureImageLoadEvent.getTargetPixelFormat()))
                {
                    suffix = &s;
                    break;
                }
            }

            // Bail if the image couldn't be loaded
            if (!suffix)
                break;

            // Apply any face orientation corrections that are needed
            if (suffix->flipVertical)
                cubemapFace.flipVertical();
            if (suffix->flipHorizontal)
                cubemapFace.flipHorizontal();
            if (suffix->rotateCCW)
                cubemapFace.rotateCCW();
        }

        // Try and combine the six images into a single cubemap
        if (image.initializeCubemap(cubemapFaces))
            return true;
    }

    return false;
}

GraphicsInterface::TextureType Texture::convertStringToTextureType(const String& s)
{
    auto lower = s.asLower();

    if (lower == "2d")
        return GraphicsInterface::Texture2D;
    if (lower == "3d")
        return GraphicsInterface::Texture3D;
    if (lower == "cubemap")
        return GraphicsInterface::TextureCubemap;

    return GraphicsInterface::TextureNone;
}

String Texture::convertTextureTypeToString(GraphicsInterface::TextureType type)
{
    if (type == GraphicsInterface::Texture2D)
        return "2D";
    if (type == GraphicsInterface::Texture3D)
        return "3D";
    if (type == GraphicsInterface::TextureCubemap)
        return "Cubemap";

    return "Unknown";
}

bool Texture::processEvent(const Event& e)
{
    if (auto gmse = e.as<GatherMemorySummaryEvent>())
    {
        for (auto i = 0U; i < image_.getFrameCount(); i++)
        {
            if (image_.isCubemap())
            {
                for (auto j = 0U; j < 6; j++)
                {
                    gmse->addAllocation("Texture", name_, image_.getCubemapDataForFrame(i, j),
                                        image_.getFrameDataSize());
                }
            }
            else
                gmse->addAllocation("Texture", name_, image_.getDataForFrame(i), image_.getFrameDataSize());
        }
    }

    return true;
}

}
