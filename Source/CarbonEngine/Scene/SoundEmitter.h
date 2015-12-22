/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/Entity.h"
#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

/**
 * A SoundEmitter entity is an entity which, when assigned a sound shader, can project a sound into the scene.
 */
class CARBON_API SoundEmitter : public Entity
{
public:

    SoundEmitter() { clear(); }

    ~SoundEmitter() override;

    /**
     * Initializer method intended for use by Scene::addEntity<>() and ComplexEntity::addChild<>(), it sets the sound shader.
     */
    void initialize(const String& soundShader) { setSoundShader(soundShader); }

    /**
     * Sets up this sound emitter to use the given sound shader.
     */
    bool setSoundShader(const String& shader);

    /**
     * Returns the name of the sound shader applied to this sound emitter.
     */
    const String& getSoundShader() const;

    /**
     * Starts this sound emitter playing.
     */
    void play();

    /**
     * Pauses this sound emitter if it's playing. Calling SoundEmitter::play() will resume from the paused position.
     */
    void pause();

    /**
     * Stops this sound emitter playing. This also rewinds, so a call to SoundEmitter::play() will start again from the
     * beginning.
     */
    void stop();

    /**
     * Rewinds this sound emitter so its current position is at the beginning.
     */
    void rewind();

    /**
     * Returns the current volume adjustment of the sound source associated with this sound emitter. Defaults to 1.0.
     */
    float getVolume() const { return volume_; }

    /**
     * Sets the volume adjustment of the sound source associated with this sound emitter. Defaults to 1.0.
     */
    void setVolume(float volume);

    /**
     * Returns the current pitch adjustment of the sound source associated with this sound emitter. Defaults to 1.0.
     */
    float getPitch() const { return pitch_; }

    /**
     * Sets the pitch adjustment of the sound source associated with this sound emitter. Defaults to 1.0.
     */
    void setPitch(float pitch);

    /**
     * Returns the current state of the sound source being used by this sound emitter.
     */
    SoundInterface::SourceState getState() const;

    void clear() override;
    bool isPerFrameUpdateRequired() const override;
    void update() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;

private:

    SoundInterface::SourceObject sourceObject_ = nullptr;

    float volume_ = 1.0f;
    float pitch_ = 1.0f;

    const SoundShader* soundShader_ = nullptr;
};

}
