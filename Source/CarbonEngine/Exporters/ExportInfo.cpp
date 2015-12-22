/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Exporters/ExportInfo.h"

namespace Carbon
{

ExportInfo ExportInfo::TempExportInfo;
const auto ExportInfoVersionInfo = VersionInfo(2, 0);

void ExportInfo::clear()
{
    clientName_.clear();
    version_.clear();
}

ExportInfo ExportInfo::get()
{
    return {Globals::getClientName(), BuildInfo::getVersion()};
}

ExportInfo::operator UnicodeString() const
{
    if (!clientName_.length())
        return "[unknown]";

    return clientName_ + " version " + version_;
}

bool ExportInfo::save(FileWriter& file) const
{
    try
    {
        file.beginVersionedSection(ExportInfoVersionInfo);
        file.write(clientName_, version_);
        file.endVersionedSection();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

void ExportInfo::load(FileReader& file)
{
    file.beginVersionedSection(ExportInfoVersionInfo);
    file.read(clientName_, version_);
    file.endVersionedSection();
}

}
