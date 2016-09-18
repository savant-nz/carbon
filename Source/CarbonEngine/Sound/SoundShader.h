/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

/**
 * Sound shaders are used to describe a sound that can be played, they consist of a source audio file as well as a
 * number of properties that affect how the audio file is played at runtime such as volume adjustment, pitch adjustment,
 * radius (for positional audio), and other properties. They are specified in .soundshader files that are loaded from
 * the SoundShaders/ directory and are activated by applying them to a sound source with
 * SoundInterface::setSourceSoundShader().
 */
class CARBON_API SoundShader : private Noncopyable
{
public:

    SoundShader() { clear(); }
    ~SoundShader() { clear(); }

    /**
     * Returns the name of this sound shader, this will be the name of the file it was loaded from.
     */
    const String& getName() const { return name_; }

    /**
     * Returns the description string for this sound shader.
     */
    const String& getDescription() const { return description_; }

    /**
     * Returns the name of source audio file that this sound shader is using.
     */
    const String& getFile() const { return file_; }

    /**
     * Returns the volume of this sound shader in the range 0 - 1. Defaults to 1.0.
     */
    float getVolume() const { return volume_; }

    /**
     * Sets the volume of this sound shader, the volume should be in the range 0 - 1. Setting this value will cause all
     * sound sources that are using this sound shader to be updated.
     */
    void setVolume(float volume);

    /**
     * Returns the pitch adjustment of this sound shader. Defaults to 1.0 which means no pitch adjustment will be done.
     * See SoundShader::setPitch() for details.
     */
    float getPitch() const { return pitch_; }

    /**
     * Sets the the pitch adjustment of this sound shader, the pitch adjustment should be greater than zero. A pitch
     * adjustment of 1.0 means no pitch adjustment will be applied. Each reduction in the pitch adjustment by 50% equals
     * a pitch shift of -12 semitones (down one octave), and each doubling equals a pitch shift of 12 semitones (up one
     * octave). Setting this value will cause all sound sources that are using this sound shader to be updated.
     */
    void setPitch(float pitch);

    /**
     * Returns the radius of this sound shader that is used when doing positional audio. Defaults to 10.0.
     */
    float getRadius() const { return radius_; }

    /**
     * Sets the radius of this sound shader that is used when doing positional audio. Defaults to 10.0. Setting this
     * value will cause all sound sources that are using this sound shader to be updated.
     */
    void setRadius(float radius);

    /**
     * Returns whether this sound is to loop continuously.
     */
    bool isLooping() const { return isLooping_; }

    /**
     * Sets whether this sound should loop continuously. Setting this value will cause all sound sources that are using
     * this shader to be updated.
     */
    void setLooping(bool looping);

    /**
     * Returns whether this sound should always load in the background and only start playing once its load has
     * completed. Defaults to false. Although background loading is enabled for all sound files, if an attempt is made
     * to play a sound that has yet to be background loaded then the load will be performed immediately on the main
     * thread, so that the sound is able to be played. This behavior can be disabled by specifying that a background
     * load is required for this sound, which means that any attempts to play it will be queued and only actioned once
     * the underlying sound file has been loaded in the background.
     */
    bool isBackgroundLoadRequired() const { return isBackgroundLoadRequired_; }

    /**
     * Sets whether this sound should always load in the background and only start playing once its load has completed.
     * See SoundShader::isBackgroundLoadRequired() for more details.
     */
    void setBackgroundLoadRequired(bool required) { isBackgroundLoadRequired_ = required; }

    /**
     * Returns whether this sound shader loaded successfully.
     */
    bool isLoaded() const { return isLoaded_; }

    /**
     * Clears the contents of this sound shader.
     */
    void clear();

    /**
     * Loads this sound shader from the given sound shader file.
     */
    bool load(const String& name);

    /**
     * Returns the internal buffer object used by this sound shader.
     */
    SoundInterface::BufferObject getBufferObject() const { return bufferObject_; }

private:

    String name_;
    String description_;

    String file_;
    float volume_ = 0.0f;
    float pitch_ = 0.0f;
    float radius_ = 0.0f;
    bool isLooping_ = false;
    bool isBackgroundLoadRequired_ = false;

    bool isLoaded_ = false;

    SoundInterface::BufferObject bufferObject_ = nullptr;

    void sendSoundShaderChangedEvent();

    unsigned int referenceCount_ = 0;
    friend class SoundShaderManager;
};

}
