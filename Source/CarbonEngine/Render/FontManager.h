/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * The font manager handles the loading and reference counting of fonts currently being used. It also manages two
 * built-in system fonts, one monospace and one variable width.
 */
class CARBON_API FontManager : private Noncopyable
{
public:

    /**
     * Returns the default system monospace font.
     */
    const Font* getSystemMonospaceFont();

    /**
     * Returns the default system variable width font.
     */
    const Font* getSystemVariableWidthFont();

    /**
     * Loads the given font if it hasn't been seen before, otherwise returns a pointer to the existing font with the
     * given name and increases its reference count. Font references should be released with FontManager::releaseFont()
     * when the font is no longer needed.
     */
    const Font* setupFont(const String& name);

    /**
     * Looks through the list of loaded fonts for one with the specified name and returns it if it is found. Returns
     * null if there is no loaded font with the specified name. Note that this method does not attempt to load a new
     * font, use FontManager::setupFont() to do that, or use the Font class directly.
     */
    const Font* getFont(const String& name);

    /**
     * Releases a reference to the given font that was given out by a call to FontManager::setupFont().
     */
    bool releaseFont(const Font* font);

private:

    FontManager() {}
    ~FontManager();
    friend class Globals;

    Vector<Font*> fonts_;
    const Font* systemMonospaceFont_ = nullptr;
    const Font* systemVariableWidthFont_ = nullptr;
};

}
