/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Sound/SoundFormatRegistry.h"

namespace Carbon
{

CARBON_DEFINE_FILE_FORMAT_REGISTRY(ReadSoundFormatFunction, WriteSoundFormatFunction)

bool SoundFormatRegistry::loadSoundFile(const UnicodeString& filename, SoundInterface::AudioFormat& format,
                                        unsigned int& channelCount, unsigned int& frequency, Vector<byte_t>& data)
{
    auto file = FileReader();
    auto fnReader = loadFile(filename, file);

    format = SoundInterface::UnknownAudioFormat;
    frequency = 0;
    data.clear();

    return fnReader && fnReader(file, format, channelCount, frequency, data);
}

bool SoundFormatRegistry::saveSoundFile(const UnicodeString& filename, SoundInterface::AudioFormat format,
                                        unsigned int channelCount, unsigned int frequency, const Vector<byte_t>& data)
{
    auto file = FileWriter();
    auto fnWriter = saveFile(filename, file);

    return fnWriter && fnWriter(file, format, channelCount, frequency, data);
}

}

#include "CarbonEngine/Sound/Formats/OggVorbisLoader.h"
#include "CarbonEngine/Sound/Formats/WavLoader.h"
