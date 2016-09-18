/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Event.h"
#include "CarbonEngine/Sound/SoundShader.h"

namespace Carbon
{

/**
 * This event is sent when an existing sound shader has one of its properties changed, this is needed for the sound
 * layer to respond appropriately by updating existing sound sources that are using the sound shader concerned.
 */
class CARBON_API SoundShaderChangedEvent : public Event
{
public:

    /**
     * Initializes the contents of this console command event.
     */
    SoundShaderChangedEvent(const SoundShader* soundShader) : soundShader_(soundShader) {}

    /**
     * Returns the sound shader that was changed.
     */
    const SoundShader* getSoundShader() const { return soundShader_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "sound shader: " << getSoundShader()->getName();
    }

private:

    const SoundShader* const soundShader_ = nullptr;
};

}
