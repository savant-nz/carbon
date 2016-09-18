/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

/**
 * Adds support for WAV sound files.
 */
class WAVLoader
{
public:

    static bool load(FileReader& file, SoundInterface::AudioFormat& format, unsigned int& channelCount,
                     unsigned int& frequency, Vector<byte_t>& data)
    {
        try
        {
            // Read header
            if (file.readFourCC() != FileSystem::makeFourCC("RIFF"))
                throw Exception("Not a WAV file");

            unsigned int fileSize;
            file.read(fileSize);
            if (fileSize > file.getSize())
                throw Exception("Invalid file size in header");

            if (file.readFourCC() != FileSystem::makeFourCC("WAVE"))
                throw Exception("Not a WAV file");

            // Loop reading WAV chunks
            while (!file.isEOF())
            {
                // Read chunk type and size
                unsigned int chunkID, chunkSize;
                file.read(chunkID);
                file.read(chunkSize);

                // Check chunk size is valid
                unsigned int nextChunkOffset = file.getPosition() + chunkSize;
                if (nextChunkOffset > file.getSize())
                    throw Exception("Invalid WAV chunk size");

                if (chunkID == FileSystem::makeFourCC("fmt "))
                {
                    // Read format chunk

                    unsigned short wFormatTag, nChannels, nBlockAlign, wBitsPerSample;
                    unsigned int nSamplesPerSec, nAvgBytesPerSec;

                    file.read(wFormatTag);
                    file.read(nChannels);
                    file.read(nSamplesPerSec);
                    file.read(nAvgBytesPerSec);
                    file.read(nBlockAlign);
                    file.read(wBitsPerSample);

                    // Check the format and bits per sample are valid
                    if (wFormatTag != 0x0001)
                        throw Exception("Compressed data is not supported");
                    if (wBitsPerSample != 8 && wBitsPerSample != 16)
                        throw Exception("Must be 8 bit or 16 bit");

                    // Get audio format
                    channelCount = nChannels;
                    format = (wBitsPerSample == 8) ? SoundInterface::PCM8Bit : SoundInterface::PCM16Bit;
                    frequency = nSamplesPerSec;
                }
                else if (chunkID == FileSystem::makeFourCC("data"))
                {
                    // Read data chunk

                    if (!data.empty())
                        throw Exception("Multiple data chunks found");

                    try
                    {
                        data.resize(chunkSize);
                    }
                    catch (const std::bad_alloc&)
                    {
                        throw Exception("Failed allocating memory for the waveform data");
                    }

                    file.readBytes(data.getData(), data.size());
                }
                else
                {
                    // Skip over this chunk
                    file.skip(chunkSize);
                }
            }

            return true;
        }
        catch (const Exception&)
        {
            data.clear();

            return false;
        }
    }
};

CARBON_REGISTER_SOUND_FILE_FORMAT(wav, WAVLoader::load, nullptr)

}
