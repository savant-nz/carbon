/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Holds information about the origins of an exported resource. The information recorded is currently the program the resource
 * was exported from and the version of the engine the export was done by.
 */
class CARBON_API ExportInfo
{
public:

    /**
     * This ExportInfo instance is used as a default parameter to a couple of routines that have an optional 'ExportInfo&'
     * parameter to output export info in.
     */
    static ExportInfo TempExportInfo;

    ExportInfo() {}

    /**
     * Constructs this export info with the specified client name and version strings.
     */
    ExportInfo(String clientName, String version) : clientName_(std::move(clientName)), version_(std::move(version)) {}

    /**
     * Returns the client name of the engine when the export was done, See Globals::getClientName() for details.
     */
    const String& getClientName() const { return clientName_; }

    /**
     * Returns the version of the engine that the export was done by. See BuildInfo::getVersion().
     */
    const String& getVersion() const { return version_; }

    /**
     * Clears all values on this class.
     */
    void clear();

    /**
     * Converts this export info into a single human-readable string.
     */
    operator UnicodeString() const;

    /**
     * Returns an ExportInfo class set with the current values in the engine.
     */
    static ExportInfo get();

    /**
     * Saves this export info to a file stream. Throws an Exception if an error occurs.
     */
    bool save(FileWriter& file) const;

    /**
     * Loads this export info from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

private:

    String clientName_;
    String version_;
};

}
