/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/TextInput.h"

namespace Carbon
{

bool TextInput::onKeyDownEvent(const KeyDownEvent& kde)
{
    if (cursorPosition_ > text_.length())
        cursorPosition_ = text_.length();

    if (kde.getKey() == KeyBackspace)
    {
        if (cursorPosition_ > 0)
            text_.erase(--cursorPosition_);
    }
    else if (kde.getKey() == KeyDelete)
    {
        if (cursorPosition_ < text_.length())
            text_.erase(cursorPosition_);
    }
    else if (kde.getKey() == KeyLeftArrow)
    {
        if (cursorPosition_ > 0)
        {
            if (cursorPosition_ == 1)
                cursorPosition_ = 0;
            else
            {
                // Handle Ctrl+Left
                if (platform().isKeyPressed(KeyLeftControl, true) || platform().isKeyPressed(KeyRightControl, true))
                {
                    auto nonSpaceCharacterIndex = cursorPosition_ - 1;

                    // Skip past spaces
                    while (nonSpaceCharacterIndex != 0 && text_.at(nonSpaceCharacterIndex) == ' ')
                        nonSpaceCharacterIndex--;

                    auto index = text_.findLastOf(" ,.()\n", nonSpaceCharacterIndex) + 1;
                    if (index == -1)
                        cursorPosition_ = 0;
                    else if (uint(index) == cursorPosition_)
                        cursorPosition_--;
                    else
                        cursorPosition_ = index;
                }
                else
                    cursorPosition_--;
            }
        }
    }
    else if (kde.getKey() == KeyRightArrow)
    {
        if (cursorPosition_ < text_.length())
        {
            // Handle Ctrl+Right
            if (platform().isKeyPressed(KeyLeftControl, true) || platform().isKeyPressed(KeyRightControl, true))
            {
                if (text_.at(cursorPosition_) == '\n' || cursorPosition_ == text_.length() - 1)
                    cursorPosition_++;
                else
                {
                    auto index = text_.findFirstOf(" ,.()\n", cursorPosition_);
                    if (index == -1)
                        cursorPosition_ = text_.length();
                    else if (uint(index) == cursorPosition_)
                        cursorPosition_++;
                    else
                        cursorPosition_ = index;

                    // Move past spaces
                    while (cursorPosition_ < text_.length())
                    {
                        if (text_.at(cursorPosition_) == ' ')
                            cursorPosition_++;
                        else
                            break;
                    }
                }
            }
            else
                cursorPosition_++;
        }
    }
    else if (kde.getKey() == KeyHome)
        cursorPosition_ = 0;
    else if (kde.getKey() == KeyEnd)
        cursorPosition_ = text_.length();
    else
        return false;

    // The cursor blinking cycle is started from the time of the last input event, which ensures that the cursor is
    // constantly visible while inputting text. To do this, TextInput::isCursorOn() needs to know when the last input
    // happened, and that is stored here.
    restartCursorBlink();

    return true;
}

bool TextInput::onCharacterInputEvent(const CharacterInputEvent& cie)
{
    restartCursorBlink();

    if (!allowedCharacters_.length() || allowedCharacters_.find(cie.getInput()) != -1)
    {
        text_.insert(cursorPosition_++, cie.getInput());
        return true;
    }

    return false;
}

bool TextInput::isCursorOn(bool checkConsoleState) const
{
    // Never blink when the console is down
    if (checkConsoleState && console().isVisible())
        return false;

    return Math::fract(lastInputTime_.getSecondsSince()) < 0.5f;
}

void TextInput::restartCursorBlink()
{
    lastInputTime_ = platform().getTime();
}

}
