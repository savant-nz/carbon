/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Version information class that holds a major and minor version, used mainly for binary file format versioning.
 */
class CARBON_API VersionInfo
{
public:

    VersionInfo() {}

    /**
     * Constructor that initializes the major and minor versions.
     */
    VersionInfo(uint16_t major, uint16_t minor) : major_(major), minor_(minor) {}

    /**
     * Major version number. A change in the major version number indicates an alteration to the file format that will prevent
     * existing code from loading the new version, i.e. v1.x loaders will not load v2.x files.
     */
    uint16_t getMajor() const { return major_; }

    /**
     * Minor version number. A change in the minor version number indicates a backwards/forwards compatible addition or
     * extension to the file format. So a v1.x loader should be able to load any file that has a major version of 1. It may
     * not understand the information added in later minor revisions, but it can just skip over this data and load the parts it
     * understands, i.e. a v1.4 loader will load a v1.6 file and just ignore the new data.
     */
    uint16_t getMinor() const { return minor_; }

    /**
     * Loads this version info from a file stream.
     */
    void load(FileReader& file) { file.read(major_, minor_); }

    /**
     * Saves this version info to a file stream.
     */
    void save(FileWriter& file) { file.write(major_, minor_); }

    /**
     * Converts this version info into a human-readable string in the form "v<major>.<minor>".
     */
    operator UnicodeString() const { return UnicodeString() << "v" << major_ << "." << minor_; }

private:

    uint16_t major_ = 0;
    uint16_t minor_ = 0;
};

}
