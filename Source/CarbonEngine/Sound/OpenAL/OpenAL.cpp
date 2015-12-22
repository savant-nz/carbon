/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENAL

#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/Endian.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Sound/SoundEvents.h"
#include "CarbonEngine/Sound/SoundFormatRegistry.h"
#include "CarbonEngine/Sound/SoundInterface.h"
#include "CarbonEngine/Sound/SoundShaderManager.h"
#include "CarbonEngine/Sound/OpenAL/OpenAL.h"

namespace Carbon
{

// Checks the OpenAL error state and reports any errors
#define CHECK_OPENAL_ERROR(message)                                    \
    do                                                                 \
    {                                                                  \
        auto error = alGetError();                                     \
        if (error != AL_NO_ERROR)                                      \
            LOG_ERROR << message << " (" << alGetString(error) << ")"; \
    } while (false)

#ifdef CARBON_DEBUG
    #define VERIFY_SOURCE_OBJECT assert(sources_.has(reinterpret_cast<Source *>(sourceObject)) && "Unknown source object");
    #define VERIFY_BUFFER_OBJECT assert(buffers_.has(reinterpret_cast<Buffer *>(bufferObject)) && "Unknown buffer object");
#else
    #define VERIFY_SOURCE_OBJECT
    #define VERIFY_BUFFER_OBJECT
#endif

void OpenAL::SoundLoadThread::main()
{
    LOG_INFO << "Sound load thread started";

    auto loadedSoundCount = 0U;

    while (!shouldExit())
    {
        // Get the next sound to load
        auto name = openAL->getNextSoundLoadThreadJob();

        // If nothing to do then sleep briefly and loop round again
        if (!name.length())
        {
            sleep(20);
            continue;
        }

        // Create an OpenAL buffer for this sound
        auto alID = openAL->createOpenALBuffer(name);

        // Pass the sound load result back through to the OpenAL instance
        openAL->setSoundLoadThreadJobResult(name, alID);
        loadedSoundCount++;
    }

    LOG_INFO << "Sound load thread stopped - " << loadedSoundCount << " sounds were loaded";
}

OpenAL::OpenAL()
{
    alcDevice_ = nullptr;
    alcContext_ = nullptr;

    events().addHandler<UpdateEvent>(this);
    events().addHandler<SoundShaderChangedEvent>(this);
    events().addHandler<GatherMemorySummaryEvent>(this);
}

OpenAL::~OpenAL()
{
    events().removeHandler(this);
}

void OpenAL::logALString(ALenum name, const String& nameString)
{
    if (alGetString(name))
        LOG_INFO << nameString << reinterpret_cast<const char*>(alGetString(name));
    else
        LOG_INFO << nameString << "null";
}

bool OpenAL::isAvailable() const
{
    // Determine the availability of OpenAL based on whether a device and context can be successfully created

    auto alcDevice = alcOpenDevice(nullptr);
    if (!alcDevice)
        return false;

    auto alcContext = alcCreateContext(alcDevice, nullptr);
    if (alcContext)
        alcDestroyContext(alcContext);

    alcCloseDevice(alcDevice);

    return alcContext != nullptr;
}

bool OpenAL::setup()
{
    // Initialize OpenAL
    try
    {
        // Open device
        alcDevice_ = alcOpenDevice(nullptr);
        if (!alcDevice_)
            throw Exception("Failed opening OpenAL device");

        // Create context
        alcContext_ = alcCreateContext(alcDevice_, nullptr);
        if (!alcContext_)
            throw Exception("Failed creating OpenAL context");

        // Activate context
        alcMakeContextCurrent(alcContext_);
        if (alcGetError(alcDevice_) != ALC_NO_ERROR)
            throw Exception("Failed making OpenAL context current");
    }
    catch (const Exception& e)
    {
        clear();

        LOG_ERROR << e;

        return false;
    }

    // Log OpenAL driver strings
    logALString(AL_VENDOR, "OpenAL Vendor: ");
    logALString(AL_VERSION, "OpenAL Version: ");
    logALString(AL_RENDERER, "OpenAL Renderer: ");

    // Set distance model
    alDistanceModel(AL_NONE);
    CHECK_OPENAL_ERROR("Failed setting distance model");

    SoundInterface::setup();

    soundLoadThread_.openAL = this;
    soundLoadThread_.run();

    return true;
}

void OpenAL::clear()
{
    shutdownSoundLoadThread();

    // Clean up any temporary sources
    for (auto i = 0U; i < sources_.size(); i++)
    {
        if (sources_[i]->temporary)
            deleteSource(sources_[i--]);
    }

    // Report any leaked sources and buffers
    Globals::increaseLeakedResourceCount(sources_.size() + buffers_.size());
    if (!sources_.empty())
        LOG_WARNING << "Unreleased source count: " << sources_.size();
    if (!buffers_.empty())
        LOG_WARNING << "Unreleased buffer count: " << buffers_.size();

    // Clear out the sound shaders
    soundShaders().clear();

    sources_.clear();
    buffers_.clear();

    clearALDeleteQueues();

    // Close OpenAL
    if (alcContext_)
    {
        alcDestroyContext(alcContext_);
        alcContext_ = nullptr;
    }
    if (alcDevice_)
    {
        alcCloseDevice(alcDevice_);
        alcDevice_ = nullptr;
    }

    SoundInterface::clear();
}

String OpenAL::getNextSoundLoadThreadJob() const
{
    auto lock = ScopedMutexLock(mutex_);

    // Find something for the sound load thread to do - it wants buffers that are in the LoadPending state and which have not
    // already been loaded but are still awaiting processing (i.e. they are done and waiting in completedSoundLoadThreadJobs_).
    for (auto buffer : buffers_)
    {
        if (buffer->state == Buffer::LoadPending &&
            completedSoundLoadThreadJobs_.find(buffer->name) == completedSoundLoadThreadJobs_.end())
            return buffer->name;
    }

    return {};
}

void OpenAL::setSoundLoadThreadJobResult(const String& name, ALuint alID)
{
    auto lock = ScopedMutexLock(mutex_);

    completedSoundLoadThreadJobs_[name] = alID;
}

bool OpenAL::processEvent(const Event& e)
{
    if (e.as<UpdateEvent>())
    {
        {
            // Loop over all the jobs completed by the sound load thread
            auto lock = ScopedMutexLock(mutex_);

            for (const auto& completedJob : completedSoundLoadThreadJobs_)
            {
                auto alID = completedJob.second;

                // Find the relevant buffer
                auto buffer = buffers_.detect([&](const Buffer* b) { return b->name == completedJob.first; }, nullptr);

                // Check the buffer exists and is currently waiting for its audio data to be loaded
                if (buffer && buffer->state == Buffer::LoadPending)
                {
                    if (alID)
                    {
                        buffer->alID = alID;
                        buffer->state = Buffer::Ready;
                    }
                    else
                        buffer->state = Buffer::Error;
                }
                else
                {
                    // Something happened which means the result of the sound load thread isn't usable - maybe the sound it was
                    // loading had its data loaded JIT on the main thread, or the sound was deleted. Either way it is no longer
                    // waiting in the LoadPending state, so throw out the work done by the sound load thread.

                    alDeleteBuffers(1, &alID);
                }
            }

            completedSoundLoadThreadJobs_.clear();
        }

        // Update listener position, velocity and orientation
        alListenerfv(AL_POSITION, listenerTransform_.getPosition().asArray());
        alListenerfv(AL_VELOCITY, listenerVelocity_.asArray());
        auto matrix = listenerTransform_.getOrientation().getMatrix3();
        auto f = std::array<float, 6>{{-matrix[6], -matrix[7], -matrix[8], matrix[3], matrix[4], matrix[5]}};
        alListenerfv(AL_ORIENTATION, f.data());

        Vector<Source*> sourcesToDelete;

        for (auto source : sources_)
        {
            if (!source->soundShader)
                continue;

            if (source->isWaitingForBufferLoad)
            {
                auto buffer = reinterpret_cast<Buffer*>(source->soundShader->getBufferObject());
                if (buffer->state != Buffer::LoadPending)
                {
                    if (buffer->state == Buffer::Ready)
                    {
                        if (setSourceSoundShader(source, source->soundShader->getName()))
                            setSourceState(source, source->state);
                    }

                    source->isWaitingForBufferLoad = false;
                }
            }

            auto pitch = Math::clamp(source->pitch * source->soundShader->getPitch(), 0.5f, 2.0f);

            if (source->temporary)
            {
                // Position source directly on top of the listener
                alSourcefv(source->alID, AL_POSITION, listenerTransform_.getPosition().asArray());
                alSourcefv(source->alID, AL_VELOCITY, listenerVelocity_.asArray());
                alSourcefv(source->alID, AL_DIRECTION, f.data());

                // Delete temporary sources that aren't playing
                auto state = ALint(0);
                alGetSourcei(source->alID, AL_SOURCE_STATE, &state);
                if (state == AL_STOPPED)
                    sourcesToDelete.append(source);
            }
            else
            {
                // Update the state of the source object
                auto state = ALint(0);
                alGetSourcei(source->alID, AL_SOURCE_STATE, &state);
                if (state == AL_PLAYING)
                    source->state = Playing;
                if (state == AL_PAUSED)
                    source->state = Paused;
                else if (state == AL_STOPPED || state == AL_INITIAL)
                    source->state = Stopped;
            }

            // Update gain and pitch
            updateSourceALVolume(source);
            alSourcef(source->alID, AL_PITCH, pitch);
        }

        for (auto source : sourcesToDelete)
            deleteSource(source);

        clearALDeleteQueues();
    }
    else if (auto ssce = e.as<SoundShaderChangedEvent>())
    {
        for (auto source : sources_)
        {
            if (source->soundShader == ssce->getSoundShader())
                updateSourcePropertiesFromSoundShader(source);
        }
    }
    else if (auto gmse = e.as<GatherMemorySummaryEvent>())
    {
        for (auto buffer : buffers_)
        {
            auto size = ALint(0);
            alGetBufferiv(buffer->alID, AL_SIZE, &size);
            gmse->addAllocation("SoundBuffer", buffer->name, buffer, size);
        }
    }

    return true;
}

void OpenAL::clearALDeleteQueues()
{
    for (auto source : alSourceDeleteQueue_)
    {
        alGetError();
        alDeleteSources(1, &source);
        CHECK_OPENAL_ERROR("Failed deleting source " << source);
    }
    alSourceDeleteQueue_.clear();

    for (auto buffer : alBufferDeleteQueue_)
    {
        alGetError();
        alDeleteBuffers(1, &buffer);
        CHECK_OPENAL_ERROR("Failed deleting buffer " << buffer);
    }
    alBufferDeleteQueue_.clear();
}

SoundInterface::SourceObject OpenAL::createSource()
{
    auto alSourceID = ALuint(0);

    try
    {
        alGetError();
        alGenSources(1, &alSourceID);
        if (alGetError() != AL_NO_ERROR || !alSourceID)
            throw Exception("Failed creating source");

        // Allocate new source
        auto source = new Source;
        source->alID = alSourceID;

        sources_.append(source);

        LOG_INFO << "Created OpenAL sound source: " << alSourceID << ", source count: " << sources_.size();

        return source;
    }
    catch (const Exception& e)
    {
        if (alSourceID)
            alDeleteSources(1, &alSourceID);

        LOG_ERROR << e;

        return nullptr;
    }
}

void OpenAL::deleteSource(SourceObject sourceObject)
{
    if (!sourceObject)
        return;

    VERIFY_SOURCE_OBJECT

    // Stop the source before deleting it
    setSourceState(sourceObject, Stopped);

    auto source = reinterpret_cast<Source*>(sourceObject);

    alSourceDeleteQueue_.append(source->alID);
    soundShaders().releaseSoundShader(source->soundShader);

    sources_.unorderedEraseValue(source);

    delete source;
    source = nullptr;
}

void OpenAL::setSourceTransform(SourceObject sourceObject, const SimpleTransform& transform)
{
    if (!sourceObject)
        return;

    VERIFY_SOURCE_OBJECT

    auto source = reinterpret_cast<Source*>(sourceObject);

    alSourcefv(source->alID, AL_POSITION, transform.getPosition().asArray());
    alSourcefv(source->alID, AL_DIRECTION, (-transform.getOrientation().getZVector()).asArray());
    source->transform = transform;
}

float OpenAL::getSourceVolume(SourceObject sourceObject) const
{
    if (!sourceObject)
        return 0.0f;

    VERIFY_SOURCE_OBJECT

    return reinterpret_cast<Source*>(sourceObject)->volume;
}

void OpenAL::setSourceVolume(SourceObject sourceObject, float volume)
{
    if (!sourceObject)
        return;

    VERIFY_SOURCE_OBJECT

    auto source = reinterpret_cast<Source*>(sourceObject);
    source->volume = Math::clamp01(volume);
    updateSourceALVolume(source);
}

float OpenAL::getSourcePitch(SourceObject sourceObject) const
{
    if (!sourceObject)
        return 0.0f;

    VERIFY_SOURCE_OBJECT

    return reinterpret_cast<Source*>(sourceObject)->pitch;
}

void OpenAL::setSourcePitch(SourceObject sourceObject, float pitch)
{
    if (!sourceObject)
        return;

    VERIFY_SOURCE_OBJECT

    reinterpret_cast<Source*>(sourceObject)->pitch = pitch;
}

void OpenAL::setSourceVelocity(SourceObject sourceObject, const Vec3& velocity)
{
    if (!sourceObject || !velocity.isFinite())
        return;

    VERIFY_SOURCE_OBJECT

    auto source = reinterpret_cast<Source*>(sourceObject);

    alSourcefv(source->alID, AL_VELOCITY, velocity.asArray());
    source->velocity = velocity;
}

bool OpenAL::setSourceSoundShader(SourceObject sourceObject, const String& shader)
{
    if (!sourceObject)
        return false;

    VERIFY_SOURCE_OBJECT

    auto source = reinterpret_cast<Source*>(sourceObject);

    if (source->soundShader && source->soundShader->getName() == shader)
    {
        if (!source->isWaitingForBufferLoad)
            return true;
    }
    else
    {
        // Release reference to current sound shader
        soundShaders().releaseSoundShader(source->soundShader);

        // Take reference to new sound shader
        source->soundShader = soundShaders().setupSoundShader(shader);
    }

    // Check the sound shader has a valid audio buffer
    auto buffer = reinterpret_cast<Buffer*>(source->soundShader->getBufferObject());
    if (!buffer)
        return false;

    // Handle the case when the load of this buffer has not yet occurred
    if (buffer->state == Buffer::LoadPending)
    {
        // If the sound shader does not want its sound file loaded on the main thread then flag it as currently waiting for its
        // buffer to be loaded, this state is checked every frame in OpenAL::processEvent()
        if (source->soundShader->isBackgroundLoadRequired())
        {
            source->isWaitingForBufferLoad = true;
            return true;
        }

        // Load this buffer on the main thread
        buffer->alID = createOpenALBuffer(buffer->name);
        buffer->state = buffer->alID ? Buffer::Ready : Buffer::Error;
    }

    // The buffer failed to load
    if (buffer->state == Buffer::Error)
        return false;

    // Store current state
    auto previousState = source->state;
    alSourceStop(source->alID);

    // Set all the AL values for this source using the contents of the sound shader
    alGetError();
    alSourcei(source->alID, AL_BUFFER, buffer->alID);
    CHECK_OPENAL_ERROR("Failed binding buffer to source");

    alSourcef(source->alID, AL_GAIN, 0.0f);
    updateSourcePropertiesFromSoundShader(source);

    source->valid = true;

    // Set it to playing if it was already playing prior to updating everything
    if (previousState == Playing)
        setSourceState(sourceObject, Playing);
    else
        setSourceState(sourceObject, Stopped);

    return true;
}

const String& OpenAL::getSourceSoundShader(SourceObject sourceObject) const
{
    if (!sourceObject)
        return String::Empty;

    VERIFY_SOURCE_OBJECT

    auto soundShader = reinterpret_cast<Source*>(sourceObject)->soundShader;

    return soundShader ? soundShader->getName() : String::Empty;
}

void OpenAL::updateSourcePropertiesFromSoundShader(Source* source)
{
    if (!source || !source->soundShader)
        return;

    alSourcei(source->alID, AL_LOOPING, source->soundShader->isLooping());
}

void OpenAL::setSourceState(SourceObject sourceObject, SourceState state)
{
    if (!sourceObject)
        return;

    VERIFY_SOURCE_OBJECT

    auto source = reinterpret_cast<Source*>(sourceObject);

    if (source->valid && source->alID)
    {
        if (state == Playing)
            alSourcePlay(source->alID);
        else if (state == Paused)
            alSourcePause(source->alID);
        else if (state == Stopped)
            alSourceStop(source->alID);
    }

    source->state = state;
}

SoundInterface::SourceState OpenAL::getSourceState(SourceObject sourceObject) const
{
    if (!sourceObject)
        return Stopped;

    VERIFY_SOURCE_OBJECT

    return reinterpret_cast<Source*>(sourceObject)->state;
}

void OpenAL::rewindSource(SourceObject sourceObject)
{
    if (!sourceObject)
        return;

    VERIFY_SOURCE_OBJECT

    auto source = reinterpret_cast<Source*>(sourceObject);

    alSourceRewind(source->alID);
    source->state = Stopped;
}

void OpenAL::setListenerTransform(const SimpleTransform& transform)
{
    // This will be set in processEvent()
    listenerTransform_ = transform;
}

void OpenAL::setListenerVelocity(const Vec3& velocity)
{
    if (!velocity.isFinite())
        return;

    // This will be set in processEvent()
    listenerVelocity_ = velocity;
}

SoundInterface::BufferObject OpenAL::setupBuffer(const String& name)
{
    // See if the buffer is already loaded
    auto buffer = buffers_.detect([&](const Buffer* b) { return b->name == name; }, nullptr);

    if (buffer)
        buffer->referenceCount++;
    else
    {
        // Create new internal sound buffer object
        buffer = new Buffer(name);
        buffers_.append(buffer);
    }

    return buffer;
}

ALuint OpenAL::createOpenALBuffer(const String& name)
{
    SimpleTimer timer;

    AudioFormat format;
    unsigned int channelCount, frequency;
    Vector<byte_t> data;
    if (!SoundFormatRegistry::loadSoundFile(SoundDirectory + name, format, channelCount, frequency, data))
    {
        LOG_ERROR_WITHOUT_CALLER << "Failed loading sound buffer: " << name;
        return 0;
    }

    if ((format != PCM8Bit && format != PCM16Bit) || channelCount > 2)
    {
        LOG_ERROR_WITHOUT_CALLER << "Unsupported audio format: " << name;
        return 0;
    }

#ifdef CARBON_BIG_ENDIAN
    if (format == PCM16Bit)
    {
        // On big endian platforms 16-bit audio needs manual conversion to big endian
        for (auto i = 0U; i < data.size(); i += 2)
            Endian::convert(&data[i], 2);
    }
#endif

    auto alID = ALuint(0);

    // Create a buffer and upload the data
    alGetError();
    alGenBuffers(1, &alID);
    if (alGetError() != AL_NO_ERROR)
    {
        LOG_ERROR_WITHOUT_CALLER << "Failed creating buffer: " << name;
        return 0;
    }

    // Get OpenAL format
    auto alFormat = ALenum(0);
    if (format == PCM8Bit)
        alFormat = (channelCount == 2) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
    else
        alFormat = (channelCount == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

    alBufferData(alID, alFormat, data.getData(), data.size(), frequency);
    if (alGetError() != AL_NO_ERROR)
    {
        LOG_ERROR_WITHOUT_CALLER << "Failed setting buffer data: " << name;
        alDeleteBuffers(1, &alID);
        return 0;
    }

    LOG_INFO << "Loaded sound buffer - '" << name << "', channels: " << channelCount
             << ", size: " << FileSystem::formatByteSize(data.size()) << ", time: " << timer
             << (Thread::isRunningInMainThread() ? " (main thread)" : " (background load)");

    return alID;
}

void OpenAL::releaseBuffer(BufferObject bufferObject)
{
    if (!bufferObject)
        return;

    VERIFY_BUFFER_OBJECT

    auto buffer = reinterpret_cast<Buffer*>(bufferObject);

    if (buffer->referenceCount < 2)
    {
        LOG_INFO << "Sound buffer deleted - '" << buffer->name << "'";

        if (buffer->alID)
            alBufferDeleteQueue_.append(buffer->alID);
        buffers_.unorderedEraseValue(buffer);

        delete buffer;
        buffer = nullptr;
    }
    else
        buffer->referenceCount--;
}

SoundInterface::SourceObject OpenAL::playShaderStraight(const String& shaderName)
{
    auto source = createSource();
    if (!source)
        return nullptr;

    reinterpret_cast<Source*>(source)->temporary = true;

    if (!setSourceSoundShader(source, shaderName))
    {
        deleteSource(source);
        return nullptr;
    }

    setSourceState(source, Playing);

    return source;
}

bool OpenAL::isSoundLoadThreadRunning() const
{
    return soundLoadThread_.isRunning();
}

bool OpenAL::isSoundLoadThreadActive() const
{
    {
        auto lock = ScopedMutexLock(mutex_);
        if (completedSoundLoadThreadJobs_.size())
            return true;
    }

    return buffers_.has([](const Buffer* b) { return b->state == Buffer::LoadPending; });
}

void OpenAL::shutdownSoundLoadThread()
{
    if (soundLoadThread_.isRunning())
    {
        soundLoadThread_.setExitFlag();
        soundLoadThread_.waitWithQueuedEventDispatching();
    }
}

}

#endif
