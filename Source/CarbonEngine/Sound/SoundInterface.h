/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

/**
 * Interface for managing and controlling sound output.
 */
class CARBON_API SoundInterface : private Noncopyable
{
public:

    virtual ~SoundInterface() {}

    /**
     * Audio data formats.
     */
    enum AudioFormat
    {
        /**
         * Unknown/unspecified audio format.
         */
        UnknownAudioFormat,

        /**
         * Uncompressed 8-bit PCM audio data.
         */
        PCM8Bit,

        /**
         * Uncompressed 16-bit PCM audio data.
         */
        PCM16Bit,

        /**
         * Sony's 4-bit ADPCM compression, used in the .vag format.
         */
        SonyADPCM,

        /**
         * MP3 compressed audio data.
         */
        MP3,

        /**
         * ATRAC3 compressed audio data.
         */
        ATRAC3
    };

    /**
     * The directory which sounds are stored under, currently "Sounds/".
     */
    static const UnicodeString SoundDirectory;

    /**
     * Opaque sound source object. Null is reserved for 'no source'.
     */
    typedef void* SourceObject;

    /**
     * Opaque sound buffer object. Null is reserved for 'no buffer'.
     */
    typedef void* BufferObject;

    /**
     * State values for a sound source.
     */
    enum SourceState
    {
        Stopped,
        Playing,
        Paused,
        LastSourceState
    };

    /**
     * Returns whether this sound interface is available for use on the current system, for internal use only.
     */
    virtual bool isAvailable() const { return true; }

    /**
     * Initializes the sound interface. Returns success flag. The default implementation loads the MasterVolume and Muted
     * settings.
     */
    virtual bool setup();

    /**
     * Clears all sound source and buffers. Saves all settings.
     */
    virtual void clear();

    /**
     * Returns the current master volume level. This value is in the range 0.0 - 1.0.
     */
    float getMasterVolume() const { return masterVolume_; }

    /**
     * Sets the master volume level. This value is clamped to 0.0 - 1.0.
     */
    virtual void setMasterVolume(float volume) { masterVolume_ = Math::clamp01(volume); }

    /**
     * Returns whether sound output is currently muted.
     */
    bool isMuted() const { return isMuted_; }

    /**
     * Sets whether sound output is currently muted.
     */
    virtual void setMuted(bool muted) { isMuted_ = muted; }

    /**
     * Creates a new source object and returns it, or null if the create fails.
     */
    virtual SourceObject createSource() { return nullptr; }

    /**
     * Deletes the given source.
     */
    virtual void deleteSource(SourceObject sourceObject) {}

    /**
     * Sets the transform of a source.
     */
    virtual void setSourceTransform(SourceObject sourceObject, const SimpleTransform& transform) {}

    /**
     * Returns the current volume of a source, will be in the range 0.0 to 1.0. All sources default to a volume of 1.0. This
     * value is multiplied with the volume set on the source's sound shader to get the final source volume.
     */
    virtual float getSourceVolume(SourceObject sourceObject) const { return 0.0f; }

    /**
     * Sets the volume of a source, the value must be in the range 0.0 to 1.0. All sources default to a volume of 1.0. This
     * value is multiplied with the volume set on the source's sound shader to get the final source volume.
     */
    virtual void setSourceVolume(SourceObject sourceObject, float volume) {}

    /**
     * Returns the current pitch adjustment of a source, see SoundShader::setPitch() for details. This value is multiplied with
     * the pitch set on the source's sound shader to get the final source pitch. Defaults to 1.0.
     */
    virtual float getSourcePitch(SourceObject sourceObject) const { return 0.0f; }

    /**
     * Sets the pich adjustment of a source, see SoundShader::setPitch() for details. This value is multiplied with the pitch
     * set on the source's sound shader to get the final source pitch. Defaults to 1.0.
     */
    virtual void setSourcePitch(SourceObject sourceObject, float pitch) {}

    /**
     * Sets the velocity of a source.
     */
    virtual void setSourceVelocity(SourceObject sourceObject, const Vec3& velocity) {}

    /**
     * Sets the sound shader to use for a source. Returns success flag.
     */
    virtual bool setSourceSoundShader(SourceObject sourceObject, const String& shader) { return false; }

    /**
     * Returns the sound shader on a source.
     */
    virtual const String& getSourceSoundShader(SourceObject sourceObject) const { return String::Empty; }

    /**
     * Sets the state of a source.
     */
    virtual void setSourceState(SourceObject sourceObject, SourceState state) {}

    /**
     * Returns the current state of a source.
     */
    virtual SourceState getSourceState(SourceObject sourceObject) const { return Stopped; }

    /**
     * Rewinds a source. This resets the stream position to the beginning and sets the source state to "stopped".
     */
    virtual void rewindSource(SourceObject sourceObject) {}

    /**
     * Sets the transform of the listener.
     */
    virtual void setListenerTransform(const SimpleTransform& transform) {}

    /**
     * Sets the velocity of the listener.
     */
    virtual void setListenerVelocity(const Vec3& velocity) {}

    /**
     * Creates a buffer from the given sound file and returns it. If the given file is already loaded then its existing buffer
     * has its reference count increased, as each buffer only needs to be loaded once.
     */
    virtual BufferObject setupBuffer(const String& name) { return nullptr; }

    /**
     * Releases a reference to the given buffer. When the reference count gets to zero the buffer is deleted.
     */
    virtual void releaseBuffer(BufferObject bufferObject) {}

    /**
     * Creates a temporary source object and plays a sound shader on it then deletes the source. The source plays straight
     * through without any positional diminution of volume. When the shader finishes playing the temporary source object is
     * automatically deleted. However, if this is called on a shader that is specified to loop forever then the source will
     * obviously never be deleted. The return value is the temporary source and can be used to stop such infinitely looping
     * sounds. Returns null if an error occurs.
     */
    virtual SourceObject playShaderStraight(const String& shaderName) { return nullptr; }

    /**
     * Returns whether the sound load thread is currently executing, this will be true if the backend in use supports a sound
     * load thread, it was successfully started, and it hasn't been shut down using SoundInterface::shutdownSoundLoadThread().
     * To query whether the sound load thread is currently working on actually loading sound data in the background use
     * SoundInterface::isSoundLoadThreadActive().
     */
    virtual bool isSoundLoadThreadRunning() const { return false; }

    /**
     * Returns whether the sound load thread is still working in the background to get sounds ready for playback. This can be
     * used to wait on a loading screen for all sounds to be ready.
     */
    virtual bool isSoundLoadThreadActive() const { return false; }

    /**
     * Shuts down the sound load thread if it is currently running.
     */
    virtual void shutdownSoundLoadThread() {}

protected:

    friend class SoundShaderManager;

    /**
     * The master volume value, must be in the range 0-1.
     */
    float masterVolume_ = 1.0f;

    /**
     * Whether sound output is currently muted.
     */
    bool isMuted_ = false;
};

}
