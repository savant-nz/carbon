/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "WindowsSDKAssistant/Common.h"

static void writeProjectTemplateVSZFile(const std::wstring& outputVSZFile, const std::string& version)
{
    try
    {
        auto vszFile = std::ifstream(sdkPath + L"\\ProjectTemplate\\CarbonApplication.vsz");

        // Read VSZ file
        auto lines = std::vector<std::string>();
        auto line = std::string();
        while (std::getline(vszFile, line))
            lines.push_back(line);

        // Replace $VISUAL_STUDIO_VERSION$ with the specified version string
        auto stringToReplace = std::string("$VISUAL_STUDIO_VERSION$");
        lines.at(1).replace(lines.at(1).find(stringToReplace), stringToReplace.length(), version);

        // Write out the final VSZ file, this also ensures the file has CRLF line endings which are required
        auto outputFile = std::ofstream(outputVSZFile);
        for (auto& ouputLine : lines)
            outputFile << ouputLine << "\n";
    }
    catch (const std::exception&)
    {
    }
}

static void setupProjectTemplate(const std::string& version, const std::wstring& key, const std::wstring& projectsDirectory)
{
    auto fullKey = std::wstring(L"Software\\Microsoft\\") + key + L"\\" + std::wstring(version.begin(), version.end());
    auto installDir = getRegistryString(fullKey, L"InstallDir", HKEY_LOCAL_MACHINE);
    if (!installDir.length())
        return;

    auto vcDir = installDir + L"..\\..\\VC\\";

    // Delete template files
    shellOperation(FO_DELETE, vcDir + projectsDirectory + L"\\CarbonApplication.*", L"");
    shellOperation(FO_DELETE, vcDir + L"VCWizards\\AppWiz\\CarbonApplication", L"");

    if (deleteMode)
        return;

    // Install project template if not in delete mode
    shellOperation(FO_COPY, sdkPath + L"\\ProjectTemplate\\CarbonApplication.ico", vcDir + projectsDirectory);
    shellOperation(FO_COPY, sdkPath + L"\\ProjectTemplate\\CarbonApplication.vsdir", vcDir + projectsDirectory);
    shellOperation(FO_COPY, sdkPath + L"\\ProjectTemplate", vcDir + L"VCWizards\\AppWiz\\CarbonApplication");
    shellOperation(FO_DELETE, vcDir + L"VCWizards\\AppWiz\\CarbonApplication\\CarbonApplication.*", L"");

    writeProjectTemplateVSZFile(vcDir + projectsDirectory + L"\\CarbonApplication.vsz", version);
}

void setupProjectTemplates()
{
    // Visual Studio 2015
    setupProjectTemplate("14.0", L"VisualStudio", L"vcprojects");
    setupProjectTemplate("14.0", L"WDExpress", L"VCProjects_WDExpress");
}
