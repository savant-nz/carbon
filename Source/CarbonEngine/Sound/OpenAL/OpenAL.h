/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_OPENAL

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/SharedLibrary.h"
#include "CarbonEngine/Core/Threads/Thread.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Sound/SoundShader.h"

// Include the OpenAL headers for this platform.
#ifdef WINDOWS
    #define AL_LIBTYPE_STATIC
#endif

#ifdef APPLE
    #include <OpenAL/al.h>
    #include <OpenAL/alc.h>
#else
    #include <AL/al.h>
    #include <AL/alc.h>
#endif

#ifndef AL_VERSION_1_1
    #error OpenAL version must be v1.1
#endif

#ifdef _MSC_VER
    #pragma comment(lib, "OpenALSoft" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
    #pragma comment(lib, "WinMM.lib")
#endif

namespace Carbon
{

/**
 * OpenAL sound backend.
 */
class OpenAL : public SoundInterface, public EventHandler
{
public:

    OpenAL();
    ~OpenAL() override;

    bool processEvent(const Event& e) override;

    bool isAvailable() const override;
    bool setup() override;
    void clear() override;
    SourceObject createSource() override;
    void deleteSource(SourceObject sourceObject) override;
    void setSourceTransform(SourceObject sourceObject, const SimpleTransform& transform) override;
    float getSourceVolume(SourceObject sourceObject) const override;
    void setSourceVolume(SourceObject sourceObject, float volume) override;
    float getSourcePitch(SourceObject sourceObject) const override;
    void setSourcePitch(SourceObject sourceObject, float pitch) override;
    void setSourceVelocity(SourceObject sourceObject, const Vec3& velocity) override;
    bool setSourceSoundShader(SourceObject sourceObject, const String& shader) override;
    const String& getSourceSoundShader(SourceObject sourceObject) const override;
    void setSourceState(SourceObject sourceObject, SourceState state) override;
    SourceState getSourceState(SourceObject sourceObject) const override;
    void rewindSource(SourceObject sourceObject) override;
    void setListenerTransform(const SimpleTransform& transform) override;
    void setListenerVelocity(const Vec3& velocity) override;
    BufferObject setupBuffer(const String& name) override;
    void releaseBuffer(BufferObject bufferObject) override;
    SourceObject playShaderStraight(const String& shaderName) override;
    bool isSoundLoadThreadRunning() const override;
    bool isSoundLoadThreadActive() const override;
    void shutdownSoundLoadThread() override;

private:

    // Wrapper around an OpenAL sound source.
    class Source
    {
    public:

        // OpenAL source object
        ALuint alID = 0;

        // Properties of this sound source
        float volume = 1.0f;
        float pitch = 1.0f;
        SimpleTransform transform;
        Vec3 velocity;
        SourceState state = Stopped;

        // The sound shader that this source is using
        const SoundShader* soundShader = nullptr;

        // Temporary sources are automatically deleted in processEvent() once they finish playing
        bool temporary = false;

        // Set when the source has been successfully set up and is ready for use
        bool valid = false;

        bool isWaitingForBufferLoad = false;
    };

    void updateSourceALVolume(Source* source)
    {
        float gain = source->volume * (isMuted_ ? 0.0f : masterVolume_);

        if (source->soundShader)
            gain *= source->soundShader->getVolume();

        // Distance attenuation
        if (!source->temporary && source->soundShader)
        {
            gain *=
                powf(1.0f - Math::clamp01(source->transform.getPosition().distance(listenerTransform_.getPosition()) /
                                          source->soundShader->getRadius()),
                     2.0f);
        }

        alSourcef(source->alID, AL_GAIN, gain);
    }

    // Wrapper around an OpenAL sound buffer.
    class Buffer
    {
    public:

        Buffer(String name_) : name(std::move(name_)) {}

        String name;
        ALuint alID = 0;

        unsigned int referenceCount = 1;

        enum
        {
            LoadPending,
            Ready,
            Error
        } state = LoadPending;
    };
    ALuint createOpenALBuffer(const String& name);

    // Primary OpenAL objects
    ALCdevice* alcDevice_;
    ALCcontext* alcContext_;

    void logALString(ALenum name, const String& nameString);

    Vector<Source*> sources_;
    void updateSourcePropertiesFromSoundShader(Source* source);

    SimpleTransform listenerTransform_;
    Vec3 listenerVelocity_;

    Vector<Buffer*> buffers_;

    Vector<ALuint> alSourceDeleteQueue_;
    Vector<ALuint> alBufferDeleteQueue_;
    void clearALDeleteQueues();

    // Sound load thread, this just continuously polls getNextSoundLoadThreadJob() and then runs
    // Buffer::createOpenALBuffer() on any jobs it finds, then returns the OpenAL buffer ID using
    // setSoundLoadThreadJobResult()
    class SoundLoadThread : public Thread
    {
    public:

        OpenAL* openAL = nullptr;

        SoundLoadThread() : Thread("SoundLoadThread") {}

        void main() override;
    } soundLoadThread_;

    // When the sound load thread finishes a job it goes into this list of completed jobs for followup processing on the
    // main thread in OpenAL::processEvent()
    mutable Mutex mutex_;
    std::unordered_map<String, ALuint> completedSoundLoadThreadJobs_;
    String getNextSoundLoadThreadJob() const;
    void setSoundLoadThreadJobResult(const String& name, ALuint alID);
};

}

#endif
