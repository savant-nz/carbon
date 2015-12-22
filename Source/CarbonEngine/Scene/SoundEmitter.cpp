/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/SoundEmitter.h"
#include "CarbonEngine/Sound/SoundInterface.h"
#include "CarbonEngine/Sound/SoundShader.h"
#include "CarbonEngine/Sound/SoundShaderManager.h"

namespace Carbon
{

const auto SoundEmitterVersionInfo = VersionInfo(1, 0);

SoundEmitter::~SoundEmitter()
{
    onDestruct();
    clear();
}

void SoundEmitter::clear()
{
    sounds().deleteSource(sourceObject_);
    sourceObject_ = nullptr;

    volume_ = 1.0f;
    pitch_ = 1.0f;

    soundShaders().releaseSoundShader(soundShader_);
    soundShader_ = nullptr;

    Entity::clear();
}

bool SoundEmitter::isPerFrameUpdateRequired() const
{
    if (sourceObject_)
        return true;

    return Entity::isPerFrameUpdateRequired();
}

void SoundEmitter::update()
{
    if (sourceObject_)
    {
        sounds().setSourceTransform(sourceObject_, getWorldTransform());
        sounds().setSourceVelocity(sourceObject_, Vec3::Zero);

        // Detect when this sound emitter's source is no longer playing a sound and release the source object
        if (sounds().getSourceState(sourceObject_) == SoundInterface::Stopped)
            stop();
    }

    Entity::update();
}

void SoundEmitter::save(FileWriter& file) const
{
    Entity::save(file);

    file.beginVersionedSection(SoundEmitterVersionInfo);

    file.write(getSoundShader());
    file.writeEnum(sounds().getSourceState(sourceObject_));
    file.write(getVolume(), getPitch());

    file.endVersionedSection();
}

void SoundEmitter::load(FileReader& file)
{
    try
    {
        Entity::load(file);

        file.beginVersionedSection(SoundEmitterVersionInfo);

        // Read sound shader and state
        auto soundShader = String();
        auto state = SoundInterface::SourceState();
        file.read(soundShader);
        file.readEnum(state, SoundInterface::LastSourceState);
        file.read(volume_, pitch_);

        file.endVersionedSection();

        // Setup the sound source
        setSoundShader(soundShader);
        setVolume(volume_);
        setPitch(pitch_);

        // TODO: restore state correctly, to do this properly would involve also saving the current play position (if any)
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

const String& SoundEmitter::getSoundShader() const
{
    return soundShader_ ? soundShader_->getName() : String::Empty;
}

bool SoundEmitter::setSoundShader(const String& shader)
{
    soundShaders().releaseSoundShader(soundShader_);

    if (!shader.length())
        return true;

    soundShader_ = soundShaders().setupSoundShader(shader);
    if (!soundShader_)
        return false;

    if (sourceObject_ && !sounds().setSourceSoundShader(sourceObject_, soundShader_->getName()))
        return false;

    return true;
}

void SoundEmitter::play()
{
    if (!sourceObject_)
    {
        sourceObject_ = sounds().createSource();
        if (!sourceObject_)
            return;

        sounds().setSourceVolume(sourceObject_, volume_);
        sounds().setSourcePitch(sourceObject_, pitch_);
        sounds().setSourceSoundShader(sourceObject_, getSoundShader());

        recheckIsPerFrameUpdateRequired();
    }

    sounds().setSourceState(sourceObject_, SoundInterface::Playing);
}

void SoundEmitter::pause()
{
    sounds().setSourceState(sourceObject_, SoundInterface::Paused);
}

void SoundEmitter::stop()
{
    if (!sourceObject_)
        return;

    sounds().setSourceState(sourceObject_, SoundInterface::Stopped);
    sounds().deleteSource(sourceObject_);
    sourceObject_ = nullptr;

    recheckIsPerFrameUpdateRequired();
}

void SoundEmitter::rewind()
{
    sounds().rewindSource(sourceObject_);
}

void SoundEmitter::setVolume(float volume)
{
    volume_ = volume;

    sounds().setSourceVolume(sourceObject_, volume);
}

void SoundEmitter::setPitch(float pitch)
{
    pitch_ = pitch;

    sounds().setSourcePitch(sourceObject_, pitch);
}

SoundInterface::SourceState SoundEmitter::getState() const
{
    return sounds().getSourceState(sourceObject_);
}

}
