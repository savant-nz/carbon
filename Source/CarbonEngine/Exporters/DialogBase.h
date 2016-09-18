/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Color.h"

namespace Carbon
{

/**
 * Base class for displaying dialogs, contains a number of shared routines and boilerplate code.
 */
class CARBON_API DialogBase : private Noncopyable
{
public:

    /**
     * The default dialog background color. The default value for this is white.
     */
    static Color DefaultBackgroundColor;

    DialogBase();

    virtual ~DialogBase() {}

    /**
     * Returns the title of the dialog. The default title is the return value of Globals::getClientName().
     */
    const UnicodeString& getTitle() const { return title_; }

    /**
     * Sets the title of the dialog. This should be called prior to DialogBase::show() as it has no effect on currently
     * visible dialogs.
     */
    void setTitle(const UnicodeString& title) { title_ = title; }

    /**
     * Returns the background color of the dialog. The default background color is specified by DefaultBackgroundColor.
     */
    const Color& getBackgroundColor() const { return backgroundColor_; }

    /**
     * Sets the background color of the dialog. This should be called prior to DialogBase::show() as it has no effect on
     * currently visible dialogs.
     */
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }

protected:

#ifdef WINDOWS

    /**
     * Background brush for this dialog
     */
    HBRUSH hBackgroundBrush_ = nullptr;

    /**
     * Main dialog procedure, must be implemented by subclasses.
     */
    virtual LRESULT dialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) = 0;

    /**
     * Handles passing dialog window messages to the correct DialogBase instance. All dialogs must be created with
     * DialogBoxParam() and pass their DialogBase pointer as the parameter.
     */
    static LRESULT CALLBACK staticDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    /**
     * This is an incomplete dialog procedure that is used to control the background color of a dialog. The main dialog
     * procedure must call this for every message it receives, and if the return value is true then it should return the
     * value in the returnValue parameter.
     */
    virtual bool backgroundColorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& returnValue);

#endif

private:

    UnicodeString title_;
    Color backgroundColor_;
};

}
