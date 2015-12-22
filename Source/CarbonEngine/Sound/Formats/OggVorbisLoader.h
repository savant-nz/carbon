/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CARBON_INCLUDE_VORBIS

#include "CarbonEngine/Sound/Formats/OggVorbisIncludeWrapper.h"

#ifdef _MSC_VER
    #pragma comment(lib, "Vorbis" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
#endif

namespace Carbon
{

/**
 * Adds support for OGG Vorbis sound files.
 */
class OGGVorbisLoader
{
public:

    static bool load(FileReader& file, SoundInterface::AudioFormat& format, unsigned int& channelCount, unsigned int& frequency,
                     Vector<byte_t>& data)
    {
        auto oggFile = OggVorbis_File();

        try
        {
            // Open the OGG Vorbis file
            auto oggCallbacks = ov_callbacks{oggRead, oggSeek, nullptr, oggTell};
            if (ov_open_callbacks(&file, &oggFile, nullptr, 0, oggCallbacks) < 0)
                throw Exception("Failed opening OGG file");

            auto vorbisInfo = ov_info(&oggFile, -1);

            // Determine the audio format
            format = SoundInterface::PCM16Bit;
            channelCount = vorbisInfo->channels;
            if (channelCount != 1 && channelCount != 2)
                throw Exception("Vorbis files must be either mono or stereo");

            // Read the sampling frequency
            frequency = uint(vorbisInfo->rate);

            // Read the amount of data in the file
            auto pcmSampleCount = int(ov_pcm_total(&oggFile, -1));
            if (pcmSampleCount < 0)
                throw Exception("Failed determining PCM sample count");

            // Allocate storage for the decompressed PCM audio data
            data.resize(pcmSampleCount * 2 * vorbisInfo->channels);

            auto bytesRead = 0U;

            // Read all the OGG packets
            while (true)
            {
                auto bitstream = 0;
                auto result = ov_read(&oggFile, data.as<char>() + bytesRead, data.size() - bytesRead, 0, vorbisInfo->channels,
                                      1, &bitstream);
                if (result < 0)
                    throw Exception("Invalid OGG data");
                else
                {
                    bytesRead += result;
                    if (result == 0 || bytesRead >= data.size())
                        break;
                }
            }

            ov_clear(&oggFile);

            return true;
        }
        catch (const Exception&)
        {
            data.clear();

            ov_clear(&oggFile);

            return false;
        }
    }

private:

    static size_t oggRead(void* ptr, size_t size, size_t nmemb, void* datasource)
    {
        auto bytesRead = 0U;

        try
        {
            reinterpret_cast<FileReader*>(datasource)->readBytes(ptr, uint(size * nmemb), &bytesRead);
        }
        catch (const Exception&)
        {
        }

        return bytesRead;
    }

    static int oggSeek(void* datasource, ogg_int64_t offset, int whence)
    {
        auto& file = *reinterpret_cast<FileReader*>(datasource);

        auto newPosition = ogg_int64_t();
        if (whence == SEEK_SET)
            newPosition = offset;
        else if (whence == SEEK_CUR)
            newPosition = file.getPosition() + offset;
        else if (whence == SEEK_END)
            newPosition = file.getSize() + offset;
        else
            return -1;

        try
        {
            file.setPosition(uint(newPosition));
            return 0;
        }
        catch (const Exception&)
        {
            return -1;
        }
    }

    static long oggTell(void* datasource) { return reinterpret_cast<FileReader*>(datasource)->getPosition(); }
};

CARBON_REGISTER_SOUND_FILE_FORMAT(ogg, OGGVorbisLoader::load, nullptr)
CARBON_REGISTER_SOUND_FILE_FORMAT(oga, OGGVorbisLoader::load, nullptr)

}

#endif
