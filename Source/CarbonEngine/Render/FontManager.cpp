/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/RenderEvents.h"

namespace Carbon
{

const auto SystemMonospaceFont = String("Consolas");
const auto SystemVariableWidthFont = String("Helvetica");

FontManager::~FontManager()
{
    releaseFont(systemMonospaceFont_);
    systemMonospaceFont_ = nullptr;

    releaseFont(systemVariableWidthFont_);
    systemVariableWidthFont_ = nullptr;

    for (auto font : fonts_)
    {
        LOG_WARNING << "Unreleased font: " << font->getName();
        delete font;
    }

    fonts_.clear();
}

const Font* FontManager::setupFont(const String& name)
{
    auto font = fonts_.detect([&](Font* f) { return f->getName() == name; }, nullptr);

    if (!font)
    {
        font = new Font;
        if (font->load(name))
        {
            font->setup();
            events().dispatchEvent(FontLoadedEvent(font));
        }
        else
            font->name_ = name;

        fonts_.append(font);
    }

    font->referenceCount_++;

    return font;
}

bool FontManager::releaseFont(const Font* font)
{
    auto index = fonts_.find<const Font*>(font);
    if (index == -1)
        return false;

    fonts_[index]->referenceCount_--;

    if (fonts_[index]->referenceCount_ == 0)
    {
        delete fonts_[index];
        fonts_.erase(index);
    }

    return true;
}

const Font* FontManager::getFont(const String& name)
{
    return fonts_.detect([&](const Font* font) { return font->getName() == name; }, nullptr);
}

const Font* FontManager::getSystemMonospaceFont()
{
    if (!systemMonospaceFont_)
        systemMonospaceFont_ = setupFont(SystemMonospaceFont);

    return systemMonospaceFont_;
}

const Font* FontManager::getSystemVariableWidthFont()
{
    if (!systemVariableWidthFont_)
        systemVariableWidthFont_ = setupFont(SystemVariableWidthFont);

    return systemVariableWidthFont_;
}

}
