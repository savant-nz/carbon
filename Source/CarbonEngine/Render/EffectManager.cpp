/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

const auto DefaultShaderQuality = Effect::HighShaderQuality;

void EffectManager::clear()
{
    for (auto effect : effects_)
        delete effect;

    effects_.clear();
}

void EffectManager::loadEffects(bool updateActiveShaders)
{
    clear();

    // Enumerate .effect files and load them
    auto effectFiles = Vector<UnicodeString>();
    fileSystem().enumerateFiles(Effect::EffectDirectory, Effect::EffectExtension, true, effectFiles);

    for (const auto& effectFile : effectFiles)
    {
        auto effect = std::unique_ptr<Effect>(new Effect);

        if (effect->load(effectFile))
        {
            effects_.append(effect.release());

            if (updateActiveShaders)
                effects_.back()->updateActiveShader(DefaultShaderQuality);
        }
    }

    LOG_INFO << "Loaded " << effects_.size() << " effect definitions";
}

Vector<String> EffectManager::getEffectNames() const
{
    return effects_.map<String>([](const Effect* effect) { return effect->getName(); });
}

Effect* EffectManager::getEffect(const String& name)
{
    return effects_.detect([&](Effect* effect) { return effect->getName() == name; }, nullptr);
}

Shader* EffectManager::getEffectActiveShader(const String& name)
{
    auto effect = getEffect(name);
    if (!effect)
        return nullptr;

    return effect->getActiveShader();
}

Vector<Shader*> EffectManager::getAllActiveShaders()
{
    auto shaders = Vector<Shader*>();

    for (auto effect : effects_)
    {
        if (effect->getActiveShader())
            shaders.append(effect->getActiveShader());
    }

    return shaders;
}

void EffectManager::onRecreateWindowEvent(const RecreateWindowEvent& rwe)
{
    // When the window is recreated all shaders need to be reinitialized

    if (rwe.getWindowEventType() == RecreateWindowEvent::CloseWindow)
    {
        for (auto effect : effects_)
            effect->clearActiveShader();
    }
    else if (rwe.getWindowEventType() == RecreateWindowEvent::NewWindow)
    {
        // Note: shaders are not currently reinitialized after a window recreation, they will be done JIT, which may lead to
        // stuttering
        for (auto effect : effects_)
            effect->updateActiveShader(effect->getQuality());
    }
}

}
