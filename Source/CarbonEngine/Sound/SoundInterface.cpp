/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Sound/OpenAL/OpenAL.h"
#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

const UnicodeString SoundInterface::SoundDirectory = "Sounds/";

CARBON_DEFINE_INTERFACE_REGISTRY(SoundInterface)
{
    return i->isAvailable();
}

typedef SoundInterface NullInterface;
CARBON_REGISTER_INTERFACE_IMPLEMENTATION(SoundInterface, NullInterface, 0)

#ifdef CARBON_INCLUDE_OPENAL
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(SoundInterface, OpenAL, 100)
#endif

const auto MasterVolumeSetting = String("MasterVolume");
const auto MutedSetting = String("Muted");

bool SoundInterface::setup()
{
    masterVolume_ = settings().getFloat(MasterVolumeSetting, 1.0f);
    isMuted_ = settings().getBoolean(MutedSetting);

    return true;
}

void SoundInterface::clear()
{
    // Save settings
    settings().set(MasterVolumeSetting, masterVolume_);
    settings().set(MutedSetting, isMuted_);
}

}
