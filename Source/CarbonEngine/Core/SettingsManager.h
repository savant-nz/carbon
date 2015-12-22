/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"

namespace Carbon
{

/**
 * An interface for handling persistent settings. Settings are identified by a string and are mainly used to store user
 * preferences. Settings are read in from a settings file on startup, can be altered at runtime, and are then saved back to the
 * file on shutdown. The location of the settings file is in the current user's directory and varies depending on the platform.
 * The name of the settings file is based on the client name that was passed to the Globals::initializeEngine() function at
 * startup, which enables different clients of the engine to have separate non-overlapping settings files.
 */
class CARBON_API SettingsManager : private Noncopyable
{
public:

    /**
     * The name of the settings file. Currently this is "Settings.txt".
     */
    static const UnicodeString SettingsFilename;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    /**
     * Returns the path to the settings file on the local file system.
     */
    UnicodeString getFullSettingsFilename() const;
#endif

    /**
     * Loads all the settings from the settings file for this application into this class. This method is only allowed to be run
     * once, subsequent calls are no-ops, and it is called automatically on startup by Globals::initializeEngine(). This method
     * does nothing if local file system access was not included in the build.
     */
    void load();

    /**
     * Returns the current value for the given boolean setting, if the setting is not set then \a defaultValue is returned.
     */
    bool getBoolean(const String& name, bool defaultValue = false) const;

    /**
     * Returns the current value for the given integer setting, if the setting is not set then \a defaultValue is returned.
     */
    unsigned int getInteger(const String& name, unsigned int defaultValue = 0) const;

    /**
     * Returns the current value for the given float setting, if the setting is not set then \a defaultValue is returned.
     */
    float getFloat(const String& name, float defaultValue = 0.0f) const;

    /**
     * Returns the current value for the given color setting, if the setting is not set then \a defaultValue is returned.
     */
    Color getColor(const String& name, const Color& defaultValue = Color::Zero) const;

    /**
     * Sets the value of a single named setting. \a name cannot be empty and may only contain numbers, letters and the period
     * character. \a value may contain any characters except the double quote, \\r and \\n characters. Returns success flag.
     */
    bool set(const String& name, const String& value);

    /**
     * Removes the specified named settings. Returns success flag.
     */
    bool remove(const String& name) { return settings_.remove(name); }

private:

    SettingsManager() {}
    ~SettingsManager();
    friend class Globals;

    ParameterArray settings_;
    bool areSettingsLoaded_ = false;
};

/**
 * Registers a global setting with the given name that will automatically load into the specified variable on startup and then
 * save that variable's final value on shutdown, which means the variable's value will persist across executions of the engine.
 * \a Type must be Boolean, Integer, Float or Color.
 */
#define CARBON_PERSISTENT_SETTING(Name, Type, Variable, DefaultValue)                               \
    CARBON_UNIQUE_NAMESPACE                                                                         \
    {                                                                                               \
        static void loadSetting() { Variable = Carbon::settings().get##Type(#Name, DefaultValue); } \
        static void saveSetting() { Carbon::settings().set(#Name, Variable); }                      \
        CARBON_REGISTER_STARTUP_FUNCTION(loadSetting, 0)                                            \
        CARBON_REGISTER_SHUTDOWN_FUNCTION(saveSetting, 0)                                           \
    }                                                                                               \
    CARBON_UNIQUE_NAMESPACE_END
}
