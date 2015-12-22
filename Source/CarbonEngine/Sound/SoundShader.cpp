/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Sound/SoundEvents.h"
#include "CarbonEngine/Sound/SoundInterface.h"
#include "CarbonEngine/Sound/SoundShader.h"

namespace Carbon
{

const auto SoundShaderDirectory = UnicodeString("SoundShaders/");
const auto SoundShaderExtension = UnicodeString(".soundshader");

void SoundShader::setVolume(float volume)
{
    volume = Math::clamp01(volume);

    if (volume == volume_)
        return;

    volume_ = volume;
    sendSoundShaderChangedEvent();
}

void SoundShader::setPitch(float pitch)
{
    if (pitch < 0.0f)
    {
        LOG_ERROR << "Pitch must be greater than zero";
        return;
    }

    if (pitch == pitch_)
        return;

    pitch_ = pitch;
    sendSoundShaderChangedEvent();
}

void SoundShader::setRadius(float radius)
{
    if (radius < 0.0f)
        return;

    radius_ = std::max(radius, 0.0f);
    sendSoundShaderChangedEvent();
}

void SoundShader::setLooping(bool looping)
{
    if (looping == isLooping_)
        return;

    isLooping_ = looping;
    sendSoundShaderChangedEvent();
}

void SoundShader::clear()
{
    name_.clear();
    description_.clear();
    file_.clear();
    volume_ = 1.0f;
    pitch_ = 1.0f;
    radius_ = 10.0f;
    isLooping_ = false;
    isBackgroundLoadRequired_ = false;

    isLoaded_ = false;

    sounds().releaseBuffer(bufferObject_);
    bufferObject_ = nullptr;
}

bool SoundShader::load(const String& name)
{
    try
    {
        clear();

        name_ = name;

        // Open this sound shader file
        auto lineTokens = Vector<Vector<String>>();
        if (!fileSystem().readTextFile(SoundShaderDirectory + name + SoundShaderExtension, lineTokens))
            throw Exception("Failed opening file");

        // Read sound shader definition
        for (auto& line : lineTokens)
        {
            if (line[0].asLower() == "description")
            {
                // Read "description <...>"
                if (line.size() != 2)
                    throw Exception("Invalid description");

                description_ = line[1];
            }
            else if (line[0].asLower() == "file")
            {
                // Read "file <name>"
                if (line.size() != 2)
                    throw Exception("Invalid file");

                file_ = line[1];
            }
            else if (line[0].asLower() == "volume" || line[0].asLower() == "gain")
            {
                // Read "volume <value>"
                if (line.size() != 2 || !line[1].isFloat())
                    throw Exception("Invalid volume");

                setVolume(line[1].asFloat());
            }
            else if (line[0].asLower() == "pitch")
            {
                // Read "pitch <value>"
                if (line.size() != 2 || !line[1].isFloat())
                    throw Exception("Invalid pitch");

                setPitch(line[1].asFloat());
            }
            else if (line[0].asLower() == "radius")
            {
                // Read "radius <value>"
                if (line.size() != 2 || !line[1].isFloat())
                    throw Exception("Invalid radius");

                setRadius(line[1].asFloat());
            }
            else if (line[0].asLower() == "looping")
            {
                // Read "looping <value>"
                if (line.size() != 2 || !line[1].isBoolean())
                    throw Exception("Invalid looping setting");

                setLooping(line[1].asBoolean());
            }
            else if (line[0].asLower() == "backgroundloadrequired")
            {
                // Read "backgroundloadrequired <value>"
                if (line.size() != 2 || !line[1].isBoolean())
                    throw Exception("Invalid background load required setting");

                setBackgroundLoadRequired(line[1].asBoolean());
            }
            else
                throw Exception() << "Unexpected token: " << line[0];
        }

        // Check that a file was specified
        if (file_.length() == 0)
            throw Exception("No file specified");

        LOG_INFO << "Loaded sound shader - '" << name << "'";

        isLoaded_ = true;
        bufferObject_ = sounds().setupBuffer(file_);

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        name_ = name;

        return false;
    }
}

void SoundShader::sendSoundShaderChangedEvent()
{
    events().dispatchEvent(SoundShaderChangedEvent(this));
}

}
