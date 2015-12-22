/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * Manages all texture objects in the engine with reference counting
 */
class CARBON_API TextureManager : public EventHandler, private Noncopyable
{
public:

    /**
     * Sets up the texture manager.
     */
    void setup();

    /**
     * This method is used by the renderer to notify the texture manager of a RecreateWindowEvent that it needs to process.
     */
    void onRecreateWindowEvent(const RecreateWindowEvent& rwe);

    /**
     * Returns a new texture of the given type with a reference count of 1.
     */
    Texture* createTexture(GraphicsInterface::TextureType type);

    /**
     * Returns a new 2D texture with a reference count of 1.
     */
    Texture2D* create2DTexture();

    /**
     * Returns a new 3D texture with a reference count of 1.
     */
    Texture3D* create3DTexture();

    /**
     * Returns a new cubemap texture with a reference count of 1.
     */
    TextureCubemap* createCubemapTexture();

    /**
     * Returns the texture with the given name, or null if there is no texture with the given name.
     */
    Texture* getTexture(const String& name);

    /**
     * Decreases the reference count of the given texture. Once the reference count is zero the texture is removed. See
     * TextureManager::enableTextureDeletion() and TextureManager::disableTextureDeletion() for more details.
     */
    void releaseTexture(const Texture* texture);

    /**
     * Returns whether the two passed texture names will resolve to the same texture, this accounts for the possibility of
     * automatic image format detection as well as explicit image format extensions that may be present in the texture names.
     */
    static bool areTextureNamesEquivalent(const String& name0, const String& name1);

    /**
     * Enables automatic deletion of textures in TextureManager::releaseTexture() when they reach a reference count of zero.
     * This also checks through every loaded texture and deletes any with a reference count of zero. This is enabled by default,
     * but may be disabled for brief periods to avoid excessive unloading and reloading of textures during certain operations.
     */
    void enableTextureDeletion();

    /**
     * This disables the deleting of textures in TextureManager::releaseTexture() when they reach a reference count of zero. See
     * TextureManager::enableTextureDeletion() for details.
     */
    void disableTextureDeletion();

    /**
     * Returns a vector containing the names of all the texture groups currently known. The texture groups used by the engine
     * are currently 'Font', 'PostProcess', 'Sprite', 'Sky', 'WorldDiffuse', 'WorldEnvironmentMap', 'WorldGloss', 'WorldNormal',
     * 'WorldOpacity', and 'WorldLightmap'.
     */
    Vector<String> getTextureGroups() const;

    /**
     * Sets the texture properties of all textures that have the specified group. The texture groups used by the engine are
     * listed with the TextureManager::getTextureGroups() method, and one of these should be passed as \a group unless the
     * application is using its own custom texture groups.
     */
    void setGroupProperties(const String& group, const TextureProperties& properties);

    /**
     * Returns the texture properties of the group specified.
     */
    TextureProperties getGroupProperties(const String& group);

    /**
     * Sets up a texture for use and returns a reference to it. If the texture is already loaded then its reference count will
     * be increased and it will be returned, otherwise an attempt will be made to load it. The reference to the texture that is
     * taken by calling this method must be released with TextureManager::releaseTexture(). If the texture needs to be loaded or
     * has not had a texture group assigned to it then it will be assigned the specified texture group, the \a group parameter
     * is otherwise ignored.
     */
    const Texture* setupTexture(GraphicsInterface::TextureType type, const String& name, const String& group = String::Empty);

    /**
     * Returns a new 1x1 2D texture with the given name and RGBA color value.
     */
    const Texture2D* create1x12DTexture(const String& name, const Color& color);

    /**
     * Returns a new 2D perlin noise texture created with the given parameters, or null on failure.
     */
    const Texture2D* create2DPerlinNoiseTexture(const String& name, unsigned int width, unsigned int height,
                                                unsigned int octaves, float persistence, float zoom);

    /**
     * Returns a new cubemap texture with the given name where each face is 1x1 and has the given RGBA color value.
     */
    const TextureCubemap* create1x1CubemapTexture(const String& name, const Color& color);

    /**
     * Returns the built in normalization cubemap texture.
     */
    const TextureCubemap* getNormalizationCubemap();

    /**
     * Returns a list of the names of all currently loaded textures.
     */
    Vector<String> getTextureNames() const;

    /**
     * Reloads all the currently loaded textures that were originally read in from a file.
     */
    void reloadTextures();

    /**
     * Calls Texture::upload() on all textures, this only affects textures that are in the UploadPending state.
     */
    void uploadTextures();

    /**
     * This is a helper method for creating a 2D texture ready for custom rendering via RTT or dynamic texture upload through
     * Texture::lockImageData(). Internally it just creates a 2D texture, loads it with an empty image that has the passed
     * dimensions, and then returns the texture. Returns null on failure. Note that the caller is responsible for cleaning up
     * the returned texture reference using TextureManager::releaseTexture(). An alpha channel can be included in the texture if
     * required.
     */
    Texture2D* create2DTexture(const String& name, unsigned int width, unsigned int height, bool includeAlpha = false);

    /**
     * This is a helper method for creating a cubemap texture ready for custom rendering via render-to-texture or dynamic
     * texture upload through Texture::lockImageData(). Internally it just creates a cubemap texture, loads it with an empty
     * image that has the passed dimensions, and then returns the texture. Returns null on failure. Note that the caller is
     * responsible for cleaning up the returned texture reference using TexureManager::releaseTexture(). An alpha channel can be
     * included in the texture if required.
     */
    TextureCubemap* createCubemapTexture(const String& name, unsigned int size, bool includeAlpha = false);

    /**
     * Returns whether the texture load thread is currently executing, this will be true unless there was some problem starting
     * the thread or the application has called TextureManager::shutdownTextureLoadThread(). To query whether the texture load
     * thread is currently working on loading texture data in the background use TextureManager::isTextureLoadThreadActive().
     */
    bool isTextureLoadThreadRunning() const;

    /**
     * Returns whether the texture load thread is still working in the background to get textures ready for rendering. This can
     * be used to wait on a loading screen for all textures to be ready.
     */
    bool isTextureLoadThreadActive() const;

    /**
     * Shuts down the texture load thread if it is currently running.
     */
    void shutdownTextureLoadThread();

    bool processEvent(const Event& e) override;

private:

    TextureManager();
    ~TextureManager() override;
    friend class Globals;

    // The following two methods are where the texture load thread hooks into the texture manager
    bool getNextTextureLoadThreadJob(String& name, GraphicsInterface::TextureType& type) const;
    void setTextureLoadThreadJobResult(const String& name, Image&& image, TimeValue time);
    friend class TextureLoadThread;

    class Members;
    Members* m = nullptr;
};

}
