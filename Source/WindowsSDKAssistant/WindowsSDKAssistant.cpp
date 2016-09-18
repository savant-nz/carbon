/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * This little utility is included with the Carbon SDK for Windows and is used in both the installer and uninstaller to
 * carry out certain parts of the install that would be difficult to do in NSIS code. It also provides a small interface
 * to install exporters when run with the /exporterinstallerdialog parameter. The command line parameters detailed below
 * are used to direct the action of the program during the installation and uninstallation processes.
 *
 *  /exporters                  Copies all exporter plugins to the plugin directories of their respective applications.
 *                              If the application for the plugin is not installed then it is ignored.
 *
 *  /environment                Adds the SDK's Bin\ directory to the current user's PATH environment variable. This also
 *                              sets the SDK's required environment variables.
 *
 *  /projecttemplates           Makes the Visual Studio project templates and wizards available in all the supported
 *                              versions.
 *
 *  /delete                     If this is specified then the above commands will delete the the relevant files and
 *                              paths rather than adding them. This is used by the uninstaller.
 *
 *  /silent                     Stops the above commands from showing any error messages they otherwise might.
 *
 *  /exporterinstallerdialog    Shows the exporter installer dialog.
 */

#include "WindowsSDKAssistant/Common.h"

const std::wstring dialogTitle(L"Carbon SDK Setup");

std::wstring sdkPath;
bool deleteMode = false;
bool silentMode = true;

// Registry key and value used to retrieve the Carbon SDK install directory
const auto rkSDKDirectory = std::wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Carbon SDK");
const auto rvSDKDirectory = std::wstring(L"InstallLocation");

std::wstring getRegistryString(const std::wstring& subkey, const std::wstring& value, HKEY hKey, bool use64BitRegistry)
{
    // If the 64-bit registry is requested then check that we're actually running on a 64-bit system. This is necessary
    // because a KEY_WOW64_64KEY registry access on a 32-bit machine will silently redirect to the 32-bit registry
    if (use64BitRegistry)
    {
        auto isWow64 = BOOL();
        if (!IsWow64Process(GetCurrentProcess(), &isWow64) || !isWow64)
            return L"";
    }

    auto options = KEY_QUERY_VALUE | (use64BitRegistry ? KEY_WOW64_64KEY : 0);

    auto hOpenedKey = HKEY();
    if (RegOpenKeyEx(hKey, subkey.c_str(), 0, options, &hOpenedKey) != ERROR_SUCCESS)
        return L"";

    auto text = std::array<wchar_t, 4096>();
    auto type = DWORD();
    auto size = DWORD(text.size());
    if (RegQueryValueEx(hOpenedKey, value.c_str(), nullptr, &type, LPBYTE(text.data()), &size) != ERROR_SUCCESS)
    {
        RegCloseKey(hOpenedKey);
        return L"";
    }

    RegCloseKey(hOpenedKey);

    if (type != REG_SZ && type != REG_EXPAND_SZ)
        return L"";

    return text.data();
}

bool setRegistryString(const std::wstring& subkey, const std::wstring& value, const std::wstring& contents, HKEY hKey)
{
    auto hOpenedKey = HKEY();
    if (RegOpenKeyEx(hKey, subkey.c_str(), 0, KEY_SET_VALUE, &hOpenedKey) != ERROR_SUCCESS)
        return false;

    auto result = RegSetValueEx(hOpenedKey, value.c_str(), 0, REG_SZ, LPCBYTE(contents.c_str()),
                                DWORD(contents.length() + 1) * 2);

    RegCloseKey(hOpenedKey);

    return result == ERROR_SUCCESS;
}

bool deleteRegistryKeyValue(const std::wstring& subkey, const std::wstring& value, HKEY hKey)
{
    auto hOpenedKey = HKEY();
    if (RegOpenKeyEx(hKey, subkey.c_str(), 0, KEY_SET_VALUE, &hOpenedKey) != ERROR_SUCCESS)
        return false;

    auto result = RegDeleteValue(hOpenedKey, value.c_str());

    RegCloseKey(hOpenedKey);

    return result == ERROR_SUCCESS;
}

bool shellOperation(UINT wFunc, const std::wstring& from, const std::wstring& to)
{
    // Create buffers that have two null terminators
    auto f = from;
    auto t = to;
    f.resize(f.length() + 1);
    t.resize(t.length() + 1);

    // Setup the shell operation
    auto op = SHFILEOPSTRUCT();
    op.hwnd = nullptr;
    op.wFunc = wFunc;
    op.pFrom = f.c_str();
    op.pTo = t.c_str();
    op.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SILENT;

    return SHFileOperation(&op) == 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
    // Initialize the standard set of common controls
    auto iccex = INITCOMMONCONTROLSEX();
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccex);

    // Check the SDK is installed and get its install location
    sdkPath = getRegistryString(rkSDKDirectory, rvSDKDirectory, HKEY_LOCAL_MACHINE);
    if (!sdkPath.length())
    {
        MessageBox(nullptr, L"The Carbon SDK is not installed.", dialogTitle.c_str(), MB_ICONEXCLAMATION);
        return 0;
    }

    auto commandLine = std::wstring(lpCmdLine);

    // Check if /delete and /silent have been specified on the command line
    deleteMode = (commandLine.find(L"/delete") != std::wstring::npos);
    silentMode = (commandLine.find(L"/silent") != std::wstring::npos);

    // Execute the commands that were given on the command line

    if (commandLine.find(L"/exporters") != std::wstring::npos)
        setupExporters();

    if (commandLine.find(L"/environment") != std::wstring::npos)
        setupEnvironment();

    if (commandLine.find(L"/projecttemplates") != std::wstring::npos)
        setupProjectTemplates();

    if (commandLine.find(L"/exporterinstallerdialog") != std::wstring::npos)
        showExporterInstallerDialog();

    return 0;
}
