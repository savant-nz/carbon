/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WindowsSDKAssistant/Common.h"

void setupSystemPath()
{
    // Get the location of the SDK's bin directory
    auto sdkBinPath = sdkPath + L"\\Bin";

    // Get the current system path
    auto path = getRegistryString(L"Environment", L"PATH", HKEY_CURRENT_USER);

    // Search for the SDK path in the current path
    auto pathIndex = path.find(sdkBinPath);

    if (!deleteMode)
    {
        // If the path is already present then there's nothing to do
        if (pathIndex != std::wstring::npos)
            return;

        // Append the SDK path, including a semicolon as a path separator if needed
        if (path.length() && path.at(path.length() - 1) != L';')
            path += L';';

        path += sdkBinPath;
    }
    else
    {
        // If the path wasn't found then there's nothing to do
        if (pathIndex == std::wstring::npos)
            return;

        // Remove the SDK path as well as any trailing semicolons
        path.erase(pathIndex, sdkBinPath.length());
        while (path.length() && path.at(path.length() - 1) == L';')
            path.erase(path.length() - 1, 1);
    }

    // Put updated path into the registry
    setRegistryString(L"Environment", L"PATH", path, HKEY_CURRENT_USER);
}

void setupEnvironmentVariable(const std::wstring& name, const std::wstring& value)
{
    if (!deleteMode)
        setRegistryString(L"Environment", name, value, HKEY_CURRENT_USER);
    else
        deleteRegistryKeyValue(L"Environment", name, HKEY_CURRENT_USER);
}

void setupEnvironment()
{
    setupSystemPath();

    setupEnvironmentVariable(L"CARBON_SDK_PATH", sdkPath);
    setupEnvironmentVariable(L"CARBON_SHARED_SCRIPT", sdkPath + L"\\Scripts\\Shared.rb");
    setupEnvironmentVariable(L"CARBON_CREATE_INSTALLER_SCRIPT",
                             sdkPath + L"\\Scripts\\CreateInstaller\\CreateInstaller.rb");
    setupEnvironmentVariable(L"CARBON_SHARED_BUILD_SCONSCRIPT", sdkPath + L"\\Scripts\\SCons\\Shared.sconscript.py");

    // Send a system broadcast message to notify of the change to the environment
    auto result = DWORD_PTR();
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, LPARAM(L"Environment"),
                       SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 10000, &result);
}
