/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * \file
 */

/**
 * Keyboard key constants, used to identify keys for keyboard input.
 */
enum KeyConstant
{
    KeyNone,

    // Numbers
    Key0,
    Key1,
    Key2,
    Key3,
    Key4,
    Key5,
    Key6,
    Key7,
    Key8,
    Key9,

    // Letters
    KeyA,
    KeyB,
    KeyC,
    KeyD,
    KeyE,
    KeyF,
    KeyG,
    KeyH,
    KeyI,
    KeyJ,
    KeyK,
    KeyL,
    KeyM,
    KeyN,
    KeyO,
    KeyP,
    KeyQ,
    KeyR,
    KeyS,
    KeyT,
    KeyU,
    KeyV,
    KeyW,
    KeyX,
    KeyY,
    KeyZ,

    // Function keys
    KeyF1,
    KeyF2,
    KeyF3,
    KeyF4,
    KeyF5,
    KeyF6,
    KeyF7,
    KeyF8,
    KeyF9,
    KeyF10,
    KeyF11,
    KeyF12,

    // Arrow keys
    KeyUpArrow,
    KeyDownArrow,
    KeyLeftArrow,
    KeyRightArrow,

    // Arrow keypad
    KeyInsert,
    KeyDelete,
    KeyHome,
    KeyEnd,
    KeyPageUp,
    KeyPageDown,

    KeyPlus,
    KeyMinus,
    KeyEquals,
    KeyBackspace,
    KeyLeftBracket,
    KeyRightBracket,
    KeyEnter,
    KeySemicolon,
    KeyApostrophe,
    KeyComma,
    KeyPeriod,
    KeyForwardSlash,
    KeyBackSlash,
    KeyStar,

    KeyEscape,
    KeyGraveAccent,
    KeyCapsLock,
    KeyTab,

    KeyLeftAlt,
    KeyRightAlt,
    KeyLeftControl,
    KeyRightControl,
    KeyLeftShift,
    KeyRightShift,
    KeyLeftMeta,
    KeyRightMeta,

    KeySpacebar,

    KeyStart,
    KeySelect,
    KeySquare,
    KeyCross,
    KeyCircle,
    KeyTriangle,
    KeyMove,

    KeyLeftButton1,
    KeyLeftButton2,
    KeyLeftButton3,
    KeyLeftButton4,
    KeyLeftButton5,

    KeyRightButton1,
    KeyRightButton2,
    KeyRightButton3,
    KeyRightButton4,
    KeyRightButton5,

    // Numpad
    KeyNumpad0,
    KeyNumpad1,
    KeyNumpad2,
    KeyNumpad3,
    KeyNumpad4,
    KeyNumpad5,
    KeyNumpad6,
    KeyNumpad7,
    KeyNumpad8,
    KeyNumpad9,
    KeyNumpadPlus,
    KeyNumpadMinus,
    KeyNumpadEquals,
    KeyNumpadEnter,
    KeyNumpadComma,
    KeyNumpadPeriod,
    KeyNumpadForwardSlash,
    KeyNumpadStar,

    KeyKanji,

    KeyLast
};

/**
 * Converts the passed KeyConstant value to a human readable string.
 */
extern CARBON_API const String& getKeyConstantAsString(KeyConstant key);

}
