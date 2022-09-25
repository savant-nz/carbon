/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileFormatRegistry.h"
#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

/**
 * Typedef for a sound file reading function.
 */
typedef std::function<bool(FileReader& file, SoundInterface::AudioFormat& format, unsigned int& channelCount,
                           unsigned int& frequency, Vector<byte_t>& data)>
    ReadSoundFormatFunction;

/**
 * Typedef for a sound file writing function.
 */
typedef std::function<bool(FileWriter& file, SoundInterface::AudioFormat format, unsigned int channelCount,
                           unsigned int frequency, const Vector<byte_t>& data)>
    WriteSoundFormatFunction;

/**
 * Handles the registration of supported sound formats and provides access to the reading and writing functions for each
 * supported format. Sound formats can be registered with the CARBON_REGISTER_SOUND_FILE_FORMAT() macro.
 */
class CARBON_API SoundFormatRegistry : public FileFormatRegistry<ReadSoundFormatFunction, WriteSoundFormatFunction>
{
public:

    /**
     * Given a filename that may or may not have an extension this method tries to load a sound out of it. If the given
     * filename contains an extension then that format will be assumed, otherwise the filesystem will be searched for a
     * matching filename with an extension that has a reader function available. If one is found then it will be used to
     * read the sound. Returns success flag.
     */
    static bool loadSoundFile(const UnicodeString& filename, SoundInterface::AudioFormat& format,
                              unsigned int& channelCount, unsigned int& frequency, Vector<byte_t>& data);

    /**
     * Saves the passed sound data to a file, the format of the image file is determined by the extension present on the
     * passed filename. Returns success flag.
     */
    static bool saveSoundFile(const UnicodeString& filename, SoundInterface::AudioFormat format,
                              unsigned int channelCount, unsigned int frequency, const Vector<byte_t>& data);
};

#ifndef DOXYGEN

CARBON_DECLARE_FILE_FORMAT_REGISTRY(ReadSoundFormatFunction, WriteSoundFormatFunction);

#endif

/**
 * \file
 */

/**
 * Registers reading and writing functions for the sound file format with the given extension. If a null function
 * pointer is specified it will be ignored.
 */
#define CARBON_REGISTER_SOUND_FILE_FORMAT(Extension, ReaderFunction, WriterFunction) \
    CARBON_REGISTER_FILE_FORMAT(Carbon::SoundFormatRegistry, Extension, ReaderFunction, WriterFunction)
}
