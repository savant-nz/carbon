/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Math/Rect.h"
#include "CarbonEngine/Render/Texture/TextureProperties.h"

namespace Carbon
{

/**
 * Base texture class which supports animated textures and provides a standardized interface to the different texture
 * subclasses. The texture system is built on top of the image system which manages raw image data, pixel formats, image
 * transforms, and image file format support (see the Image and ImageFormatRegistry classes).
 */
class CARBON_API Texture : private EventHandler, private Noncopyable
{
public:

    /**
     * The directory which textures are stored under, currently "Textures/".
     */
    static const UnicodeString TextureDirectory;

    /**
     * Every texture is always in one of the states defined by this enumeration. All textures start in the \a Uninitialized
     * state. Splitting the texture load and upload process into multiple stages allows parts of this process to be offloaded
     * into worker threads.
     */
    enum TextureState
    {
        /**
         * This is the initial texture state and indicates that this texture has not yet been initialized. Depending on which
         * Texture::load() variant is called the next state will be ImageLoadPending or UploadPending.
         */
        Uninitialized,

        /**
         * This texture state indicates that an error occurred while loading or uploading this texture.
         */
        Error,

        /**
         * This texture state indicates that this texture has been setup but its image data has not yet been loaded from the
         * filesystem. If the image load succeeds then the next state will be UploadPending, if it fails then the next state
         * will be Error.
         */
        ImageLoadPending,

        /**
         * This texture state indicates that this texture's image data is loaded and ready but it has not yet been uploaded to
         * the graphics interface. If the upload succeeds then the next state will be Ready, if it fails then the next state
         * will be Error.
         */
        UploadPending,

        /**
         * This texture state indicates that this texture is ready for use in rendering. This means it was loaded successfully,
         * contains valid texture data, and has been successfully uploaded to the graphics interface.
         */
        Ready
    };

    Texture();
    ~Texture() override;

    /**
     * Returns the name of this texture.
     */
    const String& getName() const { return name_; }

    /**
     * Returns whether this texture was loaded from a file on the file system. If this is false then the texture was loaded
     * directly from an Image class instance.
     */
    bool isLoadedFromFile() const { return isLoadedFromFile_; }

    /**
     * Returns the number of frames of animation in this texture. Will be at least one if this texture is loaded.
     */
    unsigned int getFrameCount() const { return image_.getFrameCount(); }

    /**
     * Returns the currently active animation frame.
     */
    unsigned int getCurrentFrame() const { return currentFrame_; }

    /**
     * Sets the active animation frame. If the specified frame is greater than the total number of frames it will wrap around.
     */
    void setCurrentFrame(unsigned int frame);

    /**
     * Returns the texture object for the current frame of this texture, or null if there is no texture object available.
     */
    GraphicsInterface::TextureObject getActiveTextureObject() const
    {
        return currentFrame_ < textureObjects_.size() ? textureObjects_[currentFrame_] : nullptr;
    }

    /**
     * Returns the texture properties of this texture. This controls filtering and wrapping modes.
     */
    const TextureProperties& getProperties() const { return properties_; }

    /**
     * Returns the group of this texture.
     */
    const String& getGroup() const { return group_; }

    /**
     * Returns the current state that this texture is in, see the TextureState enumeration for details.
     */
    TextureState getState() const { return state_; }

    /**
     * Returns the internal Image object that contains the data for this texture. Note that this internal image data is only
     * correct once this texture is in the UploadPending or Ready states.
     */
    const Image& getImage() const { return image_; }

    /**
     * Returns the pixel format used by this texture's image data, this is the same as `Texture::getImage().pixelFormat`.
     */
    Image::PixelFormat getPixelFormat() const { return image_.getPixelFormat(); }

    /**
     * Returns a new Rect object initialized with the following values: 0, 0, width, height.
     */
    Rect getRect() const { return {0.0f, 0.0f, float(getImage().getWidth()), float(getImage().getHeight())}; }

    /**
     * Returns whether this texture's image data contains a full mipmap chain.
     */
    bool hasMipmaps() const { return image_.hasMipmaps(); }

    /**
     * Locks the image data of this texture so it can be updated. If the data is already locked then null will be returned. Once
     * the image data has been updated then Texture::unlockImageData() must be called for the changes to take effect. This
     * method only works once this texture has been successfully uploaded.
     */
    Image* lockImageData();

    /**
     * Unlocks the image data of this texture if it is currently locked. See Texture::lockImageData() for details. Once this
     * method has been called the pointer that was returned by Texture::lockImageData() is no longer valid and must be
     * discarded.
     */
    bool unlockImageData();

    /**
     * Returns the type of this texture. Must be implemented by subclasses.
     */
    virtual GraphicsInterface::TextureType getTextureType() const = 0;

    /**
     * Clears this texture from system and video memory.
     */
    void clear();

    /**
     * Loads a texture from a file through the file system. A local path inside the textures directory should be specified, and
     * no extension added. This does not do any setup for rendering, that is done by Texture::upload().
     */
    bool load(const String& name, const String& group = String::Empty);

    /**
     * Loads a texture from an existing Image class instance.
     */
    bool load(const String& name, Image image, const String& group = String::Empty);

    /**
     * Sets the texture properties of this texture.
     */
    void setProperties(const TextureProperties& properties);

    /**
     * Uploads this texture to the graphics interface so it can be used in rendering. This method only works if this texture is
     * currently in the UploadPending state, if it is in any other state then nothing is done and false will be returned. If the
     * upload succeeds then the texture will be set to Ready.
     */
    virtual bool upload() = 0;

    /**
     * Removes this texture's graphics interface objects if it had uploaded any. This method only works if this texture is
     * currently in the Ready state, if it is in any other state then nothing is done. The new texture state will be
     * UploadPending.
     */
    void deupload();

    /**
     * If the current texture state is ImageLoadPending then this method will run the pending image load and the texture state
     * will be updated either to UploadPending or Error, depending on whether or not the image load process is successful.
     */
    void ensureImageIsLoaded();

    /**
     * Returns the amount of video memory this texture object is currently consuming. This will be zero if the texture is not
     * uploaded, and may vary based on the current texture quality setting.
     */
    unsigned int getVideoMemoryUsed() const { return videoMemoryUsed_; }

    /**
     * Downloads the contents of this texture using GraphicsInterface::downloadTexture() and writes the resulting image to the
     * specified file. This is useful for investigating the contents of temporary off-screen render target textures during
     * debugging. The specified file name should include a supported image file extension, e.g. "png", "jpg". Returns success
     * flag.
     */
    bool downloadAndSaveToFile(const UnicodeString& filename, Image::PixelFormat pixelFormat = Image::RGBA8) const;

    /**
     * Returns a short information string about this texture.
     */
    operator UnicodeString() const { return image_; }

    /**
     * Returns the mipmap level that an upload of this texture should start at. This value is based on the current texture
     * quality and the dimensions of the texture. Texture quality is controlled by changing which mipmap is uploaded as the
     * level 0 mipmap to the graphics interface.
     */
    unsigned int calculateFirstMipmapLevel();

    /**
     * Checks whether an image with the specified name exists and could potentially be loaded as a texture through
     * Texture::loadTextureImage().
     */
    static bool doesTextureFileExist(const String& name);

    /**
     * Loads the raw image data for the given texture name into the passed Image instance, the load is done through
     * ImageFormatRegistry. This method is thread-safe.
     */
    static bool loadTextureImage(const String& name, Image& image);

    /**
     * Converts a string to a texture type enum value. Returns TextureNone if the given string is not recognized as a texture
     * group.
     */
    static GraphicsInterface::TextureType convertStringToTextureType(const String& s);

    /**
     * Converts a texture type to a string.
     */
    static String convertTextureTypeToString(GraphicsInterface::TextureType type);

    /**
     * Sends a TextureLoadedEvent with the passed values, and performs any image conversion requested by the TextureLoadEvent
     * handler(s). This method is thread-safe.
     */
    static void sendTextureLoadedEvent(const String& name, Image& image, GraphicsInterface::TextureType type);

protected:

    /**
     * The current state of this texture, see the Texture::TextureState enumeration for more details.
     */
    TextureState state_ = Uninitialized;

    /**
     * Internal graphics interface texture objects created inside Texture::upload() implementations, there is one texture object
     * per frame.
     */
    Vector<GraphicsInterface::TextureObject> textureObjects_;

    /**
     * The amount of video memory currently being used by this texture in bytes.
     */
    unsigned int videoMemoryUsed_ = 0;

    /**
     * Helper method used by texture subclasses during upload that handles automatic conversion of image data to a pixel format
     * supported by the graphics hardware.
     */
    const Image& getUploadableImage(Image& temporaryImage) const;

private:

    String name_;

    Image image_;

    bool isLoadedFromFile_ = false;
    bool isImageDataLocked_ = false;

    TextureProperties properties_;
    String group_;

    unsigned int currentFrame_ = 0;

    bool processEvent(const Event& e) override;

    friend class TextureManager;
    unsigned int referenceCount_ = 0;
};

}
