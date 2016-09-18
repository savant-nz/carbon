/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WindowsSDKAssistant/Common.h"

// Exporter plugin filenames
const auto Max8ExporterFilename = std::wstring(L"CarbonExporterMax8.dle");
const auto Maya2009ExporterFilename = std::wstring(L"CarbonExporterMaya2009.mll");
const auto Maya200964BitExporterFilename = std::wstring(L"CarbonExporterMaya200964.mll");
const auto Maya2014ExporterFilename = std::wstring(L"CarbonExporterMaya2014.mll");

// Registry keys and values that are used to install the exporters
const auto rkMax8 = std::wstring(L"Software\\Autodesk\\3dsmax\\8.0");
const auto rvMax = std::wstring(L"InstallDir");
const auto rkMaya2009 = std::wstring(L"Software\\Autodesk\\Maya\\2009\\Setup\\InstallPath");
const auto rkMaya2014 = std::wstring(L"Software\\Autodesk\\Maya\\2014\\Setup\\InstallPath");
const auto rvMaya = std::wstring(L"MAYA_INSTALL_LOCATION");

const auto MaxPluginDirectory = std::wstring(L"plugins\\");
const auto MayaPluginDirectory = std::wstring(L"bin\\plug-ins\\");

// Install directories
static auto max8Dir = std::wstring();
static auto maya2009Dir = std::wstring();
static auto maya200964BitDir = std::wstring();
static auto maya2014Dir = std::wstring();

static bool showErrorDialog(const std::wstring& message)
{
    if (silentMode)
        return false;

    // Show the error message with a Retry/Cancel option.
    return MessageBox(nullptr, message.c_str(), dialogTitle.c_str(), MB_RETRYCANCEL | MB_ICONEXCLAMATION) != IDCANCEL;
}

bool copyFile(const std::wstring& source, const std::wstring& dest, const std::wstring& appName)
{
    // Try copying the file, show an error dialog if the copy fails
    while (!CopyFile(source.c_str(), dest.c_str(), FALSE))
    {
        if (!showErrorDialog(std::wstring(L"Failed installing the ") + appName + L" exporter plugin.\n\nCheck that " +
                             appName + L" is not currently running."))
            return false;
    }

    return true;
}

bool deleteFile(const std::wstring& name, const std::wstring& appName)
{
    // Check that the file exists
    auto file = std::ifstream();
    file.open(name.c_str(), std::ifstream::in);
    file.close();
    if (file.fail())
        return false;

    // Try deleting the file, show an error dialog if the delete fails
    while (!DeleteFile(name.c_str()))
    {
        if (!showErrorDialog(std::wstring(L"Failed removing the ") + appName + L" exporter plugin.\n\nCheck that " +
                             appName + L" is not currently running."))
            return false;
    }

    return true;
}

bool installMax8Exporter()
{
    if (!max8Dir.length())
        return false;

    return copyFile(sdkPath + L"\\Exporters\\" + Max8ExporterFilename,
                    max8Dir + MaxPluginDirectory + Max8ExporterFilename, L"3D Studio Max 8");
}

bool uninstallMax8Exporter()
{
    return deleteFile(max8Dir + MaxPluginDirectory + Max8ExporterFilename, L"3D Studio Max 8");
}

bool installMaya2009Exporter()
{
    if (!maya2009Dir.length())
        return false;

    return copyFile(sdkPath + L"\\Exporters\\" + Maya2009ExporterFilename,
                    maya2009Dir + MayaPluginDirectory + Maya2009ExporterFilename, L"Maya 2009");
}

bool uninstallMaya2009Exporter()
{
    return deleteFile(maya2009Dir + MayaPluginDirectory + Maya2009ExporterFilename, L"Maya 2009");
}

bool installMaya200964BitExporter()
{
    if (!maya200964BitDir.length())
        return false;

    return copyFile(sdkPath + L"\\Exporters\\" + Maya200964BitExporterFilename,
                    maya200964BitDir + MayaPluginDirectory + Maya200964BitExporterFilename, L"Maya 2009 64-bit");
}

bool uninstallMaya200964BitExporter()
{
    return deleteFile(maya200964BitDir + MayaPluginDirectory + Maya200964BitExporterFilename, L"Maya 2009 64-bit");
}

bool installMaya2014Exporter()
{
    if (!maya2014Dir.length())
        return false;

    return copyFile(sdkPath + L"\\Exporters\\" + Maya2014ExporterFilename,
                    maya2014Dir + MayaPluginDirectory + Maya2014ExporterFilename, L"Maya 2014");
}

bool uninstallMaya2014Exporter()
{
    return deleteFile(maya2014Dir + MayaPluginDirectory + Maya2014ExporterFilename, L"Maya 2014");
}

void setupExporterDirectories()
{
    max8Dir = getRegistryString(rkMax8, rvMax, HKEY_LOCAL_MACHINE);
    maya2009Dir = getRegistryString(rkMaya2009, rvMaya, HKEY_LOCAL_MACHINE);
    maya200964BitDir = getRegistryString(rkMaya2009, rvMaya, HKEY_LOCAL_MACHINE, true);
    maya2014Dir = getRegistryString(rkMaya2014, rvMaya, HKEY_LOCAL_MACHINE, true);
}

void setupExporters()
{
    setupExporterDirectories();

    if (!deleteMode)
    {
        installMax8Exporter();
        installMaya2009Exporter();
        installMaya200964BitExporter();
        installMaya2014Exporter();
    }
    else
    {
        uninstallMax8Exporter();
        uninstallMaya2009Exporter();
        uninstallMaya200964BitExporter();
        uninstallMaya2014Exporter();
    }
}

LRESULT CALLBACK dialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Set dialog icon
            auto hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_WINDOWSSDKASSISTANT));
            SetClassLongPtr(hDlg, GCLP_HICON, LONG_PTR(hIcon));

            setupExporterDirectories();

            if (max8Dir.length())
                SetWindowText(GetDlgItem(hDlg, IDC_MAX8_INSTALL_DIRECTORY), max8Dir.c_str());
            else
            {
                SetWindowText(GetDlgItem(hDlg, IDC_MAX8_INSTALL_DIRECTORY), L"3D Studio Max 8 is not installed");
                EnableWindow(GetDlgItem(hDlg, IDC_INSTALL_MAX8_EXPORTER), FALSE);
            }

            if (maya2009Dir.length())
                SetWindowText(GetDlgItem(hDlg, IDC_MAYA2009_INSTALL_DIRECTORY), maya2009Dir.c_str());
            else
            {
                SetWindowText(GetDlgItem(hDlg, IDC_MAYA2009_INSTALL_DIRECTORY), L"Maya 2009 is not installed");
                EnableWindow(GetDlgItem(hDlg, IDC_INSTALL_MAYA2009_EXPORTER), FALSE);
            }

            if (maya200964BitDir.length())
                SetWindowText(GetDlgItem(hDlg, IDC_MAYA2009_64BIT_INSTALL_DIRECTORY), maya200964BitDir.c_str());
            else
            {
                SetWindowText(GetDlgItem(hDlg, IDC_MAYA2009_64BIT_INSTALL_DIRECTORY),
                              L"Maya 2009 64-bit is not installed");
                EnableWindow(GetDlgItem(hDlg, IDC_INSTALL_MAYA2009_64BIT_EXPORTER), FALSE);
            }

            if (maya2014Dir.length())
                SetWindowText(GetDlgItem(hDlg, IDC_MAYA2014_INSTALL_DIRECTORY), maya2014Dir.c_str());
            else
            {
                SetWindowText(GetDlgItem(hDlg, IDC_MAYA2014_INSTALL_DIRECTORY), L"Maya 2014 is not installed");
                EnableWindow(GetDlgItem(hDlg, IDC_INSTALL_MAYA2014_EXPORTER), FALSE);
            }

            return TRUE;
        }

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;

        case WM_COMMAND:
        {
            auto command = LOWORD(wParam);

            auto result = false;
            if (command == IDC_INSTALL_MAX8_EXPORTER)
                result = installMax8Exporter();
            else if (command == IDC_INSTALL_MAYA2009_EXPORTER)
                result = installMaya2009Exporter();
            else if (command == IDC_INSTALL_MAYA2009_64BIT_EXPORTER)
                result = installMaya200964BitExporter();
            else if (command == IDC_INSTALL_MAYA2014_EXPORTER)
                result = installMaya2014Exporter();
            else
                break;

            if (result)
                MessageBox(hDlg, L"Installed exporter", L"Carbon Exporter Installer", MB_ICONINFORMATION);
            else
                MessageBox(hDlg, L"Failed installing exporter", L"Carbon Exporter Installer", MB_ICONERROR);

            return TRUE;
        }
    }

    return FALSE;
}

void showExporterInstallerDialog()
{
    DialogBox(GetModuleHandle(nullptr), LPCTSTR(IDD_EXPORTER_INSTALLER), nullptr, DLGPROC(dialogProc));
}
