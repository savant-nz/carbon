/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Platform/TextInput.h"
#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Editbox GUI item. A GUIEditbox's text highlights when the mouse is over it and when it has input focus. Text
 * alignment defaults to Font::AlignCenterLeft.
 */
class CARBON_API GUIEditbox : public GUIWindow
{
public:

    GUIEditbox();
    ~GUIEditbox() override;

    /**
     * The character used to render password editboxes, currently set to '*'.
     */
    static const UnicodeString PasswordCharacter;

    /**
     * Sets the characters that are allowed to be typed into this editbox. By default this is an empty string meaning
     * all characters are allowed.
     */
    void setAllowedCharacters(const UnicodeString& characters);

    /**
     * Returns whether this editbox is for password entry, password editboxes display '*' characters instead of their
     * full text, the GUIEditbox::getText() and GUIEditbox::setText() methods are not affected. Defaults to false.
     */
    bool isPasswordEditbox() const { return isPasswordEditbox_; }

    /**
     * Sets whether this editbox is for password entry. Defaults to false.
     */
    void setPasswordEditbox(bool passwordEditbox);

    bool isInteractive() const override;
    bool processEvent(const Event& e) override;
    Color getTextColor() const override;
    void setText(const UnicodeString& text) override;

    void clear() override;
    bool gatherGeometry(GeometryGather& gather) override;

protected:

    void updateLines() override;

private:

    TextInput textInput_;

    unsigned int characterOffset_ = 0;
    void calculateCharacterOffset();

    Vec2 cursorDrawPosition_;
    void updateCursorDrawPosition();
    void setCursorPositionFromLocalPoint(const Vec2& p);

    bool isPasswordEditbox_ = false;
};

}
