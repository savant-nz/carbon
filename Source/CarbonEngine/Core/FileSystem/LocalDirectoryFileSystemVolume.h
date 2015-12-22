/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

#include "CarbonEngine/Core/FileSystem/FileSystemVolume.h"

namespace Carbon
{

/**
 * This is a subclass of FileSystemVolume that passes all its file I/O off to a local file system directory, this effectively
 * means that the root of the file system volume is equivalent to the local file system directory which is passed to the
 * constructor.
 */
class CARBON_API LocalDirectoryFileSystemVolume : public FileSystemVolume
{
public:

    /**
     * Initializes this local directory file system volume with the given name and the directory on the local file system that
     * it should use as its root.
     */
    LocalDirectoryFileSystemVolume(const UnicodeString& name, const UnicodeString& localDirectory)
        : FileSystemVolume(name), localDirectory_(FileSystem::joinPaths(localDirectory, UnicodeString::Empty))
    {
    }

    /**
     * Returns the local directory in use by this local directory file system volume.
     */
    const UnicodeString& getLocalDirectory() const { return localDirectory_; }

    FileSystemError open(const UnicodeString& filename, FileReader& file) const override;
    FileSystemError open(const UnicodeString& filename, FileWriter& file, bool asText) override;
    bool doesFileExist(const UnicodeString& filename) const override;
    FileSystemError enumerateFiles(const UnicodeString& directory, const UnicodeString& extension, bool recursive,
                                   Vector<UnicodeString>& files) const override;
    FileSystemError deleteFile(const UnicodeString& filename) override;

private:

    const UnicodeString localDirectory_;
};

}

#endif
