/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"

namespace Carbon
{

/**
 * Manages the current theme colors for the GUI. Themes can be set at runtime or loaded from a .guitheme file. There are
 * 'ListGUIThemes' and 'GUITheme' console commands available. Initially the theme colors are set from the default system
 * theme.
 */
class CARBON_API ThemeManager : private Noncopyable
{
public:

    /**
     * The theme directory, currently "GUIThemes/".
     */
    static const UnicodeString ThemeDirectory;

    /**
     * The theme file extension, currently ".guitheme".
     */
    static const UnicodeString ThemeExtension;

    /**
     * Loads the specified theme file. Return success flag.
     */
    bool load(const UnicodeString& name);

    /**
     * Returns the specified theme color. If there is no theme color with the specified name then a highlighter purple
     * will be returned to make the missing theme color apparent.
     */
    const Color& get(const String& name);

    /**
     * \copydoc ThemeManager::get()
     */
    const Color& operator[](const String& name) { return get(name); }

    /**
     * Sets the specified theme color.
     */
    void set(const String& name, const Color& color);

private:

    ThemeManager() {}
    friend class Globals;

    bool isThemeLoaded_ = false;

    ParameterArray themeColors_;
};

}
