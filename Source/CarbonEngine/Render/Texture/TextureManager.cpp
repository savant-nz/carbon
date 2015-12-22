/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Threads/Mutex.h"
#include "CarbonEngine/Core/Threads/Thread.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"
#include "CarbonEngine/Math/Noise.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/Texture3D.h"
#include "CarbonEngine/Render/Texture/TextureCubemap.h"

namespace Carbon
{

// The texture load thread is responsible for background texture loading, it takes textures that are in the ImageLoadPending
// state and does the file system read, image load/decompression, any image conversion requested by a TextureLoadedEvent
// handler, and then passes the resulting Image instance back to TextureManager where it is matched up with the correct texture.
//
// Applications can wait for all texture loading to be completed by waiting until TextureManager::isTextureLoadThreadActive()
// returns false.
class TextureLoadThread : public Thread
{
public:

    TextureLoadThread() : Thread("TextureLoadThread") {}

    void main() override
    {
        LOG_INFO << "Texture load thread started";

        auto loadedTextureCount = 0U;

        while (!shouldExit())
        {
            // Get the next texture to load
            auto name = String();
            auto type = GraphicsInterface::TextureType();
            if (!textures().getNextTextureLoadThreadJob(name, type))
            {
                // If nothing to do then sleep briefly and loop round again
                sleep(20);
                continue;
            }

            auto timer = SimpleTimer();

            // Load the image for this texture
            auto image = Image();
            if (Texture::loadTextureImage(name, image))
            {
                // If a texture image was loaded then send a TextureLoadedEvent, the event itself will be dispatched on the main
                // thread, but any texture image conversion requested will be run in this texture load thread
                Texture::sendTextureLoadedEvent(name, image, type);
            }
            else
                image.clear();

            // This texture load is complete, pass the result back through to the texture manager
            textures().setTextureLoadThreadJobResult(name, std::move(image), timer.getElapsedTime());
            loadedTextureCount++;
        }

        LOG_INFO << "Texture load thread stopped - " << loadedTextureCount << " textures were loaded";
    }
};

class TextureManager::Members
{
public:

    mutable Mutex mutex;

    Vector<Texture*> textures;
    std::unordered_map<String, TextureProperties> groups;

    bool enableTextureDeletion = true;

    TextureLoadThread textureLoadThread;

    // When the texture load thread finishes a job it goes into this list of completed jobs for followup processing on the main
    // thread in TextureManager::processEvent()
    struct CompletedTextureLoadThreadJob
    {
        String name;
        Image image;
        TimeValue time;

        CompletedTextureLoadThreadJob() {}
        CompletedTextureLoadThreadJob(String name_, Image image_, TimeValue time_)
            : name(std::move(name_)), image(std::move(image_)), time(time_)
        {
        }
    };
    Vector<CompletedTextureLoadThreadJob> completedTextureLoadThreadJobs;
};

TextureManager::TextureManager()
{
    m = new Members;
}

void TextureManager::setup()
{
    auto lock = ScopedMutexLock(m->mutex);

    // Setup texturing groups with default settings
    m->groups["Font"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapClamp);
    m->groups["PostProcess"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapClamp);
    m->groups["Sprite"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapClamp);
    m->groups["Sky"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapClamp);
    m->groups["WorldDiffuse"] = TextureProperties(TextureProperties::TrilinearFilter, GraphicsInterface::WrapRepeat);
    m->groups["WorldEnvironmentMap"] = TextureProperties(TextureProperties::TrilinearFilter, GraphicsInterface::WrapClamp);
    m->groups["WorldGloss"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapRepeat);
    m->groups["WorldNormal"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapRepeat);
    m->groups["WorldOpacity"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapRepeat);
    m->groups["WorldLightmap"] = TextureProperties(TextureProperties::BilinearFilter, GraphicsInterface::WrapClamp);

    // Start the texture load thread
    m->textureLoadThread.run();

    events().addHandler<UpdateEvent>(this);
}

TextureManager::~TextureManager()
{
    events().removeHandler(this);

    shutdownTextureLoadThread();

    m->completedTextureLoadThreadJobs.clear();

    enableTextureDeletion();

    for (auto texture : m->textures)
    {
        LOG_WARNING << "Unreleased texture, name: " << texture->getName() << ", reference count: " << texture->referenceCount_;
        delete texture;
    }

    delete m;
    m = nullptr;
}

bool TextureManager::getNextTextureLoadThreadJob(String& name, GraphicsInterface::TextureType& type) const
{
    auto lock = ScopedMutexLock(m->mutex);

    // Find something for the texture load thread to do - it wants textures that are in the ImageLoadPending state and which
    // have not already been loaded but still awaiting processing (i.e. they are done and waiting in
    // completedTextureLoadThreadJobs).
    for (auto texture : m->textures)
    {
        if (texture->getState() == Texture::ImageLoadPending)
        {
            if (!m->completedTextureLoadThreadJobs.has(
                    [&](const Members::CompletedTextureLoadThreadJob& job) { return job.name == texture->getName(); }))
            {
                name = texture->getName();
                type = texture->getTextureType();

                return true;
            }
        }
    }

    return false;
}

void TextureManager::setTextureLoadThreadJobResult(const String& name, Image&& image, TimeValue time)
{
    auto lock = ScopedMutexLock(m->mutex);

    // Store the resulting image from the texture load thread
    m->completedTextureLoadThreadJobs.emplace(name, std::move(image), time);
}

bool TextureManager::isTextureLoadThreadRunning() const
{
    return m->textureLoadThread.isRunning();
}

bool TextureManager::isTextureLoadThreadActive() const
{
    auto lock = ScopedMutexLock(m->mutex);

    if (m->completedTextureLoadThreadJobs.size())
        return true;

    return m->textures.has([](const Texture* texture) { return texture->getState() == Texture::ImageLoadPending; });
}

void TextureManager::shutdownTextureLoadThread()
{
    if (m->textureLoadThread.isRunning())
    {
        // Stop the texture load thread, it is important to keep dispatching queued events to avoid the texture load thread
        // getting stuck waiting for an event to be dispatched on the main thread
        m->textureLoadThread.setExitFlag();
        m->textureLoadThread.waitWithQueuedEventDispatching();
    }
}

bool TextureManager::processEvent(const Event& e)
{
    if (e.as<UpdateEvent>())
    {
        auto lock = ScopedMutexLock(m->mutex);

        auto timer = SimpleTimer();

        // Loop over all the jobs completed by the texture load thread
        while (!m->completedTextureLoadThreadJobs.empty())
        {
            auto job = m->completedTextureLoadThreadJobs.popFront();

            // Find the relevant texture
            auto texture = getTexture(job.name);

            // Check the texture exists and is currently waiting for its image data to be loaded
            if (texture && texture->getState() == Texture::ImageLoadPending)
            {
                if (job.image.isValidImage())
                {
                    // Take the image data loaded by the texture load thread and give it to the texture
                    swap(job.image, texture->image_);
                    texture->state_ = Texture::UploadPending;

                    // Upload the texture
                    texture->upload();

                    LOG_INFO << "Texture loaded - '" << texture->getName() << "' - " << *texture
                             << ", time: " << int(job.time.toMilliseconds()) << "ms (background load)";
                }
                else
                {
                    texture->state_ = Texture::Error;
                    LOG_ERROR_WITHOUT_CALLER << "Missing texture: " << texture->getName();
                }
            }
            else
            {
                // Something happened which means the result of the texture load thread isn't usable - maybe the texture it was
                // loading had its image data loaded JIT on the main thread, or the texture was deleted. Either way it is no
                // longer waiting in the ImageLoadPending state, so the work done by the texture load thread can't be used.
            }

            // Break out if more than 100ms has been spent processing completed texture loads this time around, any remaining
            // items can be processed in subsequent frames, no need to get it all done now
            if (timer.getElapsedTime().toMilliseconds() > 100.0f)
                break;
        }
    }

    return true;
}

void TextureManager::onRecreateWindowEvent(const RecreateWindowEvent& rwe)
{
    auto lock = ScopedMutexLock(m->mutex);

    // When the window is recreated all textures need to be reuploaded

    if (rwe.getWindowEventType() == RecreateWindowEvent::CloseWindow)
    {
        // Destroy all graphics interface texture objects
        for (auto texture : m->textures)
        {
            if (texture->getState() == Texture::Ready)
                texture->deupload();

            // Textures that have a valid image but had an error uploading will get another go under the new render window
            else if (texture->getState() == Texture::Error && texture->getImage().isValidImage())
                texture->state_ = Texture::UploadPending;
        }
    }
    else if (rwe.getWindowEventType() == RecreateWindowEvent::NewWindow)
    {
        // Upload textures that need uploading, this only affects textures that are in the UploadPending state
        for (auto texture : m->textures)
            texture->upload();
    }
}

Texture* TextureManager::createTexture(GraphicsInterface::TextureType type)
{
    auto lock = ScopedMutexLock(m->mutex);

    auto texture = pointer_to<Texture>::type();

    if (type == GraphicsInterface::Texture2D)
        texture = new Texture2D;
    else if (type == GraphicsInterface::Texture3D)
        texture = new Texture3D;
    else if (type == GraphicsInterface::TextureCubemap)
        texture = new TextureCubemap;
    else
        return nullptr;

    // Put new texture into the texture list
    texture->referenceCount_ = 1;
    m->textures.append(texture);

    return texture;
}

Texture2D* TextureManager::create2DTexture()
{
    return static_cast<Texture2D*>(createTexture(GraphicsInterface::Texture2D));
}

Texture3D* TextureManager::create3DTexture()
{
    return static_cast<Texture3D*>(createTexture(GraphicsInterface::Texture3D));
}

TextureCubemap* TextureManager::createCubemapTexture()
{
    return static_cast<TextureCubemap*>(createTexture(GraphicsInterface::TextureCubemap));
}

Texture* TextureManager::getTexture(const String& name)
{
    auto lock = ScopedMutexLock(m->mutex);

    return m->textures.detect([&](Texture* texture) { return areTextureNamesEquivalent(texture->getName(), name); }, nullptr);
}

void TextureManager::releaseTexture(const Texture* texture)
{
    if (!texture)
        return;

    auto lock = ScopedMutexLock(m->mutex);

    auto nonConstTexture = const_cast<Texture*>(texture);

    if (texture->referenceCount_ < 2)
    {
        nonConstTexture->referenceCount_ = 0;

        // Remove texture if there are no other users of it
        if (m->enableTextureDeletion)
        {
            LOG_INFO << "Texture deleted - '" << texture->getName() << "'";

            m->textures.unorderedEraseValue(nonConstTexture);

            delete nonConstTexture;
        }
    }
    else
        nonConstTexture->referenceCount_--;
}

bool TextureManager::areTextureNamesEquivalent(const String& name0, const String& name1)
{
    if (name0 == name1)
        return true;

    // Check whether the names match after stripping off any extensions that may be present
    if (ImageFormatRegistry::stripSupportedExtension(name0) == ImageFormatRegistry::stripSupportedExtension(name1))
    {
        // If the base texture names match then make sure that different extensions haven't been explicitly specified on the two
        // textures, if this checks out then the texture names are considered to be equivalent

        auto& extension0 = ImageFormatRegistry::getSupportedExtension(name0);
        auto& extension1 = ImageFormatRegistry::getSupportedExtension(name1);

        if (!extension0.length() || !extension1.length() || extension0 == extension1)
            return true;
    }

    return false;
}

void TextureManager::enableTextureDeletion()
{
    auto lock = ScopedMutexLock(m->mutex);

    m->enableTextureDeletion = true;

    auto deleteList = m->textures.select([](Texture* texture) { return texture->referenceCount_ == 0; });

    for (auto texture : deleteList)
        releaseTexture(texture);
}

void TextureManager::disableTextureDeletion()
{
    auto lock = ScopedMutexLock(m->mutex);

    m->enableTextureDeletion = false;
}

Vector<String> TextureManager::getTextureGroups() const
{
    auto groups = Vector<String>();

    for (auto& group : m->groups)
        groups.append(group.first);

    return groups;
}

void TextureManager::setGroupProperties(const String& group, const TextureProperties& properties)
{
    auto lock = ScopedMutexLock(m->mutex);

    if (m->groups.find(group) == m->groups.end())
        LOG_INFO << "Custom texture group created: " << group;

    m->groups[group] = properties;

    for (auto texture : m->textures)
    {
        if (texture->getGroup() == group)
            texture->setProperties(properties);
    }
}

TextureProperties TextureManager::getGroupProperties(const String& group)
{
    auto lock = ScopedMutexLock(m->mutex);

    return m->groups[group];
}

const Texture* TextureManager::setupTexture(GraphicsInterface::TextureType type, const String& name, const String& group)
{
    auto lock = ScopedMutexLock(m->mutex);

    // Search for this texture, if it is already loaded just increment the reference count and return it
    for (auto texture : m->textures)
    {
        if (texture->getTextureType() == type && areTextureNamesEquivalent(texture->getName(), name))
        {
            // If this texture isn't in a group and a valid group has been passed then assign this texture into that group
            if (texture->getGroup() == String::Empty && group != String::Empty)
            {
                texture->group_ = group;
                texture->setProperties(m->groups[group]);
            }
            else if (m->groups[texture->getGroup()].getWrap() != m->groups[group].getWrap())
            {
                LOG_WARNING_WITHOUT_CALLER << "Texture '" << name
                                           << "' is used in texture groups that have differing wrap modes, "
                                              "this may cause rendering problems";
            }

            texture->referenceCount_++;
            return texture;
        }
    }

    // Create the new texture and put it into the ImageLoadPending state
    auto texture = createTexture(type);
    texture->load(name, group);

    return texture;
}

Vector<String> TextureManager::getTextureNames() const
{
    auto lock = ScopedMutexLock(m->mutex);

    return m->textures.map<String>([](const Texture* t) { return t->getName(); }).sorted();
}

void TextureManager::reloadTextures()
{
    auto lock = ScopedMutexLock(m->mutex);

    for (auto texture : m->textures)
    {
        if (!texture->isLoadedFromFile())
            continue;

        // Reload this texture from file

        // Store previous name and properties
        auto name = texture->getName();
        auto properties = texture->getProperties();

        // Load the texture
        auto loadSucceeded = texture->load(name, texture->getGroup());
        texture->ensureImageIsLoaded();

        // Update properties
        texture->setProperties(properties);

        // Upload the texture if the reload was successful
        if (loadSucceeded)
            texture->upload();
    }
}

void TextureManager::uploadTextures()
{
    auto lock = ScopedMutexLock(m->mutex);

    for (auto& texture : m->textures)
        texture->upload();
}

Texture2D* TextureManager::create2DTexture(const String& name, unsigned int width, unsigned int height, bool includeAlpha)
{
    auto image = Image();
    if (!image.initialize(width, height, 1, includeAlpha ? Image::RGBA8 : Image::RGB8, false, 1))
        return nullptr;

    auto texture = textures().create2DTexture();
    if (!texture->load(name, std::move(image), "Sprite") || !texture->upload())
    {
        releaseTexture(texture);
        return nullptr;
    }

    return texture;
}

TextureCubemap* TextureManager::createCubemapTexture(const String& name, unsigned int size, bool includeAlpha)
{
    auto image = Image();
    if (!image.initializeCubemap(size, includeAlpha ? Image::RGBA8 : Image::RGB8, false, 1))
        return nullptr;

    auto texture = textures().createCubemapTexture();
    if (!texture->load(name, std::move(image), "WorldEnvironmentMap") || !texture->upload())
    {
        releaseTexture(texture);
        return nullptr;
    }

    return texture;
}

const Texture2D* TextureManager::create1x12DTexture(const String& name, const Color& color)
{
    // Setup 1x1 image
    auto image = Image();
    image.initialize(1, 1, 1, Image::RGBA8, false, 1);
    *reinterpret_cast<unsigned int*>(image.getDataForFrame(0)) = color.toRGBA8();

    // Load and upload the texture
    auto texture = textures().create2DTexture();
    if (!texture->load(name, std::move(image)) || !texture->upload())
    {
        releaseTexture(texture);
        return nullptr;
    }

    return texture;
}

const Texture2D* TextureManager::create2DPerlinNoiseTexture(const String& name, unsigned int width, unsigned int height,
                                                            unsigned int octaves, float persistence, float zoom)
{
    if (getTexture(name))
    {
        LOG_ERROR << "Texture name is already in use";
        return nullptr;
    }

    // Fill an image with 2D perlin noise
    auto image = Image();
    image.initialize(width, height, 1, Image::Luminance8, false, 1);

    auto data = image.getDataForFrame(0);
    for (auto y = 0U; y < height; y++)
    {
        for (auto x = 0U; x < width; x++)
            *data++ = byte_t(Math::clamp01(Noise::perlin(x * zoom, y * zoom, octaves, persistence) * 0.5f + 0.5f) * 255.0f);
    }

    // Create a new texture from the perlin noise
    auto texture = create2DTexture();
    if (!texture->load(name, std::move(image), "WorldDiffuse") || !texture->upload())
    {
        releaseTexture(texture);
        return nullptr;
    }

    return texture;
}

const TextureCubemap* TextureManager::create1x1CubemapTexture(const String& name, const Color& color)
{
    // Setup 1x1 cubemap image
    auto image = Image();
    image.initializeCubemap(1, Image::RGBA8, false, 1);

    for (auto i = 0U; i < 6; i++)
        *reinterpret_cast<unsigned int*>(image.getCubemapDataForFrame(0, i)) = color.toRGBA8();

    // Load and upload the texture
    auto texture = textures().createCubemapTexture();
    if (!texture->load(name, std::move(image), "WorldDiffuse") || !texture->upload())
    {
        releaseTexture(texture);
        return nullptr;
    }

    return texture;
}

}
