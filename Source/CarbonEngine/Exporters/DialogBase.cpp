/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Exporters/DialogBase.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Resource.h"

namespace Carbon
{

Color DialogBase::DefaultBackgroundColor(1.0f);

DialogBase::DialogBase() : title_(Globals::getClientName()), backgroundColor_(DefaultBackgroundColor)
{
}

#ifdef WINDOWS

bool DialogBase::backgroundColorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& returnValue)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Create a colored brush for drawing the background of the dialog and all its controls
            hBackgroundBrush_ = CreateSolidBrush(backgroundColor_.toCOLORREF());

            break;
        }

        case WM_DESTROY:
        {
            // Delete the brush on shutdown
            DeleteObject(hBackgroundBrush_);
            hBackgroundBrush_ = nullptr;

            break;
        }

        // Dialog controls are forced to use a transparent background
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        {
            SetBkMode(HDC(wParam), TRANSPARENT);

            // Deliberate fall through
        }

        // The dialog and its controls use the same background brush
        case WM_CTLCOLORDLG:
        {
            returnValue = LRESULT(hBackgroundBrush_);
            return true;
        }
    }

    return false;
}

LRESULT CALLBACK DialogBase::staticDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Store the pointer to the DialogBase class on WM_INITDIALOG
    if (message == WM_INITDIALOG)
        SetWindowLongPtr(hDlg, GWLP_USERDATA, LONG_PTR(lParam));

    // Get the pointer to the DialogBase class
    auto userData = GetWindowLongPtr(hDlg, GWLP_USERDATA);
    if (!userData)
        return FALSE;

    // Cast to a DialogBase pointer
    auto dialogBase = reinterpret_cast<DialogBase*>(userData);

    // Handle the dialog background color
    auto returnValue = LRESULT();
    if (dialogBase->backgroundColorDialogProc(hDlg, message, wParam, lParam, returnValue))
        return returnValue;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Set dialog caption
            SetWindowTextW(hDlg, dialogBase->getTitle().toUTF16().as<wchar_t>());

            // Set dialog icon
            SetClassLongPtr(hDlg, GCLP_HICON, LONG_PTR(LoadIcon(Globals::getHInstance(), MAKEINTRESOURCE(IDI_CARBON))));

            break;
        }
    }

    // Pass the message through to the dialog subclass
    return dialogBase->dialogProc(hDlg, message, wParam, lParam);
}

#endif
}
