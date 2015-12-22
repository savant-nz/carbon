/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Parameter.h"
#include "CarbonEngine/Platform/ThemeManager.h"

namespace Carbon
{

const UnicodeString ThemeManager::ThemeDirectory = "GUIThemes/";
const UnicodeString ThemeManager::ThemeExtension = ".guitheme";

const auto FallbackColor = Color(1.0f, 0.0f, 1.0f, 1.0f);

const Color& ThemeManager::get(const String& name)
{
    if (!isThemeLoaded_)
        load("Default");

    return themeColors_.get(name, Parameter(FallbackColor)).getColor();
}

void ThemeManager::set(const String& name, const Color& color)
{
    themeColors_[name].setColor(color);
}

bool ThemeManager::load(const UnicodeString& name)
{
    isThemeLoaded_ = true;

    if (!fileSystem().readTextFile(ThemeDirectory + name + ThemeExtension, themeColors_))
    {
        LOG_ERROR << "Failed loading theme: " << name;
        return false;
    }

    return true;
}

}
