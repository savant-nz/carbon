/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Sound/SoundInterface.h"
#include "CarbonEngine/Sound/SoundShader.h"
#include "CarbonEngine/Sound/SoundShaderManager.h"

namespace Carbon
{

void SoundShaderManager::clear()
{
    for (auto soundShader : soundShaders_)
        delete soundShader;

    soundShaders_.clear();
}

const SoundShader* SoundShaderManager::setupSoundShader(const String& name)
{
    for (auto soundShader : soundShaders_)
    {
        if (soundShader->getName() == name)
        {
            soundShader->referenceCount_++;
            return soundShader;
        }
    }

    soundShaders_.append(new SoundShader);
    soundShaders_.back()->load(name);
    soundShaders_.back()->referenceCount_ = 1;

    return soundShaders_.back();
}

void SoundShaderManager::releaseSoundShader(const SoundShader* soundShader)
{
    if (!soundShader)
        return;

    auto shader = const_cast<SoundShader*>(soundShader);

#ifdef CARBON_DEBUG
    if (!soundShaders_.has(shader))
        LOG_WARNING << "Shader " << shader << " is not known by the sound shader manager, this may result in a crash";
#endif

    if (shader->referenceCount_ < 2)
    {
        soundShaders_.unorderedEraseValue(shader);

        delete shader;
        shader = nullptr;
    }
    else
        shader->referenceCount_--;
}

SoundShader* SoundShaderManager::getSoundShader(const String& name)
{
    for (auto soundShader : soundShaders_)
    {
        if (soundShader->getName() == name)
            return soundShader;
    }

    return nullptr;
}

Vector<String> SoundShaderManager::getSoundShaderNames() const
{
    return soundShaders_.map<String>([](const SoundShader* shader) { return shader->getName(); });
}

}
