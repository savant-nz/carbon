/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileSystem/FileSystemVolume.h"

namespace Carbon
{

/**
 * Allows a compressed Zip archive to be mounted onto the virtual file system and have its stored files enumerated and
 * read from without having to extract the whole archive. This can be useful for reading data from sources that use the
 * Zip format for storing files.
 */
class CARBON_API ZipFileSystemVolume : public FileSystemVolume
{
public:

    /**
     * Constructs this Zip file system volume with the given name.
     */
    ZipFileSystemVolume(const UnicodeString& name) : FileSystemVolume(name) {}

    /**
     * Sets this file system volume up to read from the specified Zip file. Returns success flag.
     */
    bool setup(const UnicodeString& zipFilename);

    FileSystemError open(const UnicodeString& filename, FileReader& file) const override;
    FileSystemError open(const UnicodeString& filename, FileWriter& file, bool asText) override
    {
        return NotSupportedFileSystemError;
    }

    bool doesFileExist(const UnicodeString& filename) const override;
    FileSystemError enumerateFiles(const UnicodeString& directory, const UnicodeString& extension, bool recursive,
                                   Vector<UnicodeString>& files) const override;

private:

    mutable FileReader zipFile_;

    struct Entry
    {
        UnicodeString name;

        uint32_t headerOffset = 0;
        uint32_t compressedSize = 0;
        uint32_t uncompressedSize = 0;
        uint16_t compressionMethod = 0;

        FileSystemError read(FileReader& zipFile, FileReader& file) const;
    };

    Vector<Entry> entries_;
};

}
