/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Parameter.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Globals.h"

namespace Carbon
{

const UnicodeString SettingsManager::SettingsFilename = "Settings.txt";

static void loadSettings()
{
    settings().load();
}
CARBON_REGISTER_STARTUP_FUNCTION(loadSettings, 1000)

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

UnicodeString SettingsManager::getFullSettingsFilename() const
{
    return FileSystem::LocalFilePrefix +
        FileSystem::joinPaths(fileSystem().getUserDataLocalDirectory(), SettingsFilename);
}

#endif

SettingsManager::~SettingsManager()
{
    if (!areSettingsLoaded_)
        return;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (fileSystem().writeTextFile(getFullSettingsFilename(), settings_))
        LOG_INFO << "Wrote settings file";
    else
        LOG_ERROR << "Failed writing settings file";
#endif
}

void SettingsManager::load()
{
    if (areSettingsLoaded_)
        return;

    areSettingsLoaded_ = true;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (fileSystem().readTextFile(getFullSettingsFilename(), settings_))
    {
        // Print loaded settings to the logfile
        auto settings = settings_.getParameterNames().map<UnicodeString>(
            [&](const String& name) { return name + ": " + settings_[name].getString(); });

        Logfile::get().writeCollapsibleSection(UnicodeString() + "Loaded " + settings.size() + " settings", settings);
    }
    else
#endif
        LOG_INFO << "Unable to load settings file";
}

bool SettingsManager::getBoolean(const String& name, bool defaultValue) const
{
    auto lookup = ParameterArray::Lookup(name);

    if (settings_.has(lookup))
        return settings_[lookup].getBoolean();

    return defaultValue;
}

unsigned int SettingsManager::getInteger(const String& name, unsigned int defaultValue) const
{
    auto lookup = ParameterArray::Lookup(name);

    if (settings_.has(lookup))
        return settings_[lookup].getInteger();

    return defaultValue;
}

float SettingsManager::getFloat(const String& name, float defaultValue) const
{
    auto lookup = ParameterArray::Lookup(name);

    if (settings_.has(lookup))
        return settings_[lookup].getFloat();

    return defaultValue;
}

Color SettingsManager::getColor(const String& name, const Color& defaultValue) const
{
    auto lookup = ParameterArray::Lookup(name);

    if (settings_.has(lookup))
        return settings_[lookup].getColor();

    return defaultValue;
}

bool SettingsManager::set(const String& name, const String& value)
{
    if (!name.length() || !Parameter::isValidParameterName(name))
    {
        LOG_ERROR << "Invalid setting name: '" << name << "'";
        return false;
    }

    settings_[name] = value;

    return true;
}

}
