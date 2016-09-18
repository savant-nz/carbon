/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * Handles input of a line of text, including common keyboard shortcuts.
 */
class CARBON_API TextInput : private Noncopyable
{
public:

    /**
     * Constructs the TextInput object with a reference to the text string to work with.
     */
    TextInput(UnicodeString& text) : text_(text) {}

    /**
     * Returns the position of the cursor in this text input field.
     */
    unsigned int getCursorPosition() const { return cursorPosition_; }

    /**
     * Sets the position of the cursor in this text input field.
     */
    void setCursorPosition(unsigned int position) { cursorPosition_ = position; }

    /**
     * Sets the characters that are allowed to be entered in the text input field. If this string is empty then all
     * characters are allowed.
     */
    void setAllowedCharacters(UnicodeString characters) { allowedCharacters_ = std::move(characters); }

    /**
     * Processes a key down event for this text input instance and uses it to update the string which was passed to the
     * constructor.
     */
    bool onKeyDownEvent(const KeyDownEvent& kde);

    /**
     * Processes a character input event for this text input instance and uses it to update the string which was passed
     * to the constructor.
     */
    bool onCharacterInputEvent(const CharacterInputEvent& cie);

    /**
     * Returns whether the input field for this text input object should currently be drawing the cursor. This ensures
     * the on/off cursor blinking happens in the expected way as the input field is being used. If \a checkConsoleState
     * is true then this method will always return false if the console is currently showing, meaning that text input
     * fields will not have blinking cursors when the console is down.
     */
    bool isCursorOn(bool checkConsoleState = true) const;

    /**
     * Restarts the cursor blink cycle.
     */
    void restartCursorBlink();

private:

    UnicodeString& text_;

    unsigned int cursorPosition_ = 0;

    TimeValue lastInputTime_;

    UnicodeString allowedCharacters_;
};

}
