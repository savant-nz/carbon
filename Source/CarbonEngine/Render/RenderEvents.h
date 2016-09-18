/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Event.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Render/Font.h"

namespace Carbon
{

/**
 * A shader change event is sent after a change of effect-to-shader linkage occurs in Effect::updateActiveShader().
 */
class CARBON_API ShaderChangeEvent : public Event
{
public:

    /**
     * Constructs this shader change event from an effect name and two shader pointers. The shader pointers are allowed
     * to be null.
     */
    ShaderChangeEvent(String effectName, const Shader* oldShader, const Shader* newShader)
        : effectName_(std::move(effectName)), oldShader_(oldShader), newShader_(newShader)
    {
    }

    /**
     * Returns the effect that has had a shader change
     */
    const String& getEffectName() const { return effectName_; }

    /**
     * Returns the old shader for the effect that has just been removed.
     */
    const Shader* getOldShader() const { return oldShader_; }

    /**
     * Returns the new shader for the effect.
     */
    const Shader* getNewShader() const { return newShader_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "effect: " << getEffectName() << ", previous: " << getOldShader()
                               << ", new: " << getNewShader();
    }

private:

    const String effectName_;
    const Shader* oldShader_ = nullptr;
    const Shader* newShader_ = nullptr;
};

/**
 * This event is sent before loading an image file which will then be used as a texture. Applications can use this event
 * to control the pixel format that the image file will target, see BeforeTextureImageLoadEvent::setTargetPixelFormat()
 * for details. Note that TextureLoadedEvent is sent after a successful texture image load and can also be used to set
 * the runtime pixel format for a texture, however if the engine can be notified about the target pixel format prior to
 * loading the image file then it may be able to consume less memory or other resources when loading the image file.
 */
class CARBON_API BeforeTextureImageLoadEvent : public Event
{
public:

    /**
     * Default copy constructor.
     */
    BeforeTextureImageLoadEvent(const BeforeTextureImageLoadEvent& other) = default;

    /**
     * Constructs this before texture image loaded event with the given image name.
     */
    BeforeTextureImageLoadEvent(String imageName) : imageName_(std::move(imageName)) {}

    /**
     * Returns the name of the image that is about to be loaded for use as a texture and which caused this event to be
     * sent.
     */
    const String& getImageName() const { return imageName_; }

    /**
     * Sets the image name for this before texture image loaded event.
     */
    void setImageName(const String& imageName) { imageName_ = imageName; }

    /**
     * Returns the pixel format that the image will be loaded as, see
     * BeforeTextureImageLoadEvent::setTargetPixelFormat() for details.
     */
    Image::PixelFormat getTargetPixelFormat() const { return targetPixelFormat_; }

    /**
     * Sets the target pixel format that the image should be loaded as, see ImageFormatRegistry::loadImageFile() for
     * details. The default value is \a Image::UnknownPixelFormat which means the image load will load the texture in
     * whatever pixel format it decides is most appropriate based on the contents of the image file.
     */
    void setTargetPixelFormat(Image::PixelFormat targetPixelFormat) const { targetPixelFormat_ = targetPixelFormat; }

    operator UnicodeString() const override { return UnicodeString() << "name: " << getImageName(); }

private:

    String imageName_;

    mutable Image::PixelFormat targetPixelFormat_ = Image::UnknownPixelFormat;
};

/**
 * A texture loaded event is sent immediately after the image data for a texture has been loaded, applications can use
 * this event to control the runtime pixel format used by the texture, see TextureLoadedEvent::setNewPixelFormat() for
 * details.
 */
class CARBON_API TextureLoadedEvent : public Event
{
public:

    /**
     * Constructs this texture loaded event from a texture name, type and image instance.
     */
    TextureLoadedEvent(String name, const Image& image, GraphicsInterface::TextureType type)
        : name_(std::move(name)), image_(image), type_(type), newPixelFormat_(image.getPixelFormat())
    {
    }

    /**
     * Returns the name of the texture that was loaded and caused this event to be sent.
     */
    const String& getTextureName() const { return name_; }

    /**
     * Returns the type of the texture that was loaded and caused this event to be sent.
     */
    GraphicsInterface::TextureType getTextureType() const { return type_; }

    /**
     * Returns the Image instance that holds the image data for the texture that has just been loaded.
     */
    const Image& getImage() const { return image_; }

    /**
     * Returns the pixel format that the loaded texture is going to be converted to. If this is the same as the image's
     * current pixel format then no conversion will be done.
     */
    Image::PixelFormat getNewPixelFormat() const { return newPixelFormat_; }

    /**
     * Sets the pixel format that the loaded texture should be converted to. This allows control over the runtime pixel
     * format of the texture regardless of what format it is originally loaded as. If this is the same as the image's
     * current pixel format then no conversion will be done.
     */
    void setNewPixelFormat(Image::PixelFormat newPixelFormat) const { newPixelFormat_ = newPixelFormat; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "name: " << getTextureName() << ", image: " << getImage();
    }

private:

    const String name_;
    const Image& image_;
    const GraphicsInterface::TextureType type_ = GraphicsInterface::TextureNone;

    mutable Image::PixelFormat newPixelFormat_;
};

/**
 * A font loaded event is sent immediately after a new font is loaded, applications can use this event to setup various
 * properties for the newly loaded font.
 */
class CARBON_API FontLoadedEvent : public Event
{
public:

    /**
     * Constructs this font loaded event from the given Font pointer.
     */
    FontLoadedEvent(Font* font) : font_(font) {}

    /**
     * Returns the name of the font that was loaded and caused this event to be sent.
     */
    const String& getFontName() const { return font_->getName(); }

    /**
     * Returns the Font instance that has just been loaded.
     */
    Font* getFont() const { return font_; }

    operator UnicodeString() const override { return UnicodeString() << "name: " << getFontName(); }

private:

    mutable Font* font_ = nullptr;
};

}
