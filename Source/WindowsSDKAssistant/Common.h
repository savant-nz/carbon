/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifndef _UNICODE
    #define _UNICODE
#endif
#ifndef UNICODE
    #define UNICODE
#endif

#define _WIN32_WINNT 0x0501
#define NTDDI_VERSION NTDDI_WINXP
#include <Windows.h>

#include <AclAPI.h>
#include <CommCtrl.h>
#include <Psapi.h>
#include <ShlObj.h>

#include <array>
#include <fstream>
#include <string>
#include <vector>

#include "WindowsSDKAssistant/Resource.h"

#ifdef _MSC_VER
    #pragma comment(lib, "AdvAPI32.lib")
    #pragma comment(lib, "ComCtl32.lib")
    #pragma comment(lib, "shell32.lib")
    #pragma comment(lib, "User32.lib")

    #pragma warning(disable: 4100)    // unreferenced formal parameter
    #pragma warning(disable: 4127)    // conditional expression is constant
#endif

// Path to the Carbon SDK
extern std::wstring sdkPath;

// Whether running in delete or silent modes
extern bool deleteMode;
extern bool silentMode;

// Title of any message boxes that are shown
extern const std::wstring dialogTitle;

// Helper functions to access the registry and carry out operations on files
extern std::wstring getRegistryString(const std::wstring& key, const std::wstring& value, HKEY hKey,
                                      bool use64BitRegistry = false);
extern bool setRegistryString(const std::wstring& key, const std::wstring& value, const std::wstring& contents, HKEY hKey);
extern bool deleteRegistryKeyValue(const std::wstring& subkey, const std::wstring& value, HKEY hKey);
extern bool shellOperation(UINT wFunc, const std::wstring& from, const std::wstring& to);

// Functions for all the individual command line flags
extern void setupExporters();
extern void setupEnvironment();
extern void setupProjectTemplates();
extern void showExporterInstallerDialog();
