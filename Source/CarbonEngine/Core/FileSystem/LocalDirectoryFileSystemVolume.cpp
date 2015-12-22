/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/FileSystem/LocalDirectoryFileSystemVolume.h"

namespace Carbon
{

FileSystemError LocalDirectoryFileSystemVolume::open(const UnicodeString& filename, FileReader& file) const
{
    auto fullPath = FileSystem::joinPaths(localDirectory_, filename);

    if (file.openLocalFile(fullPath))
    {
        auto correctFullPath = FileSystem::getCanonicalPath(fullPath);

        // Report incorrect case in paths on Windows and Mac OS X, helps catch errors only seen on case-sensitive platforms
        if (fullPath != correctFullPath)
        {
            LOG_WARNING << "Incorrect case for " << filename.quoteIfHasSpaces() << ", the correct case is '"
                        << correctFullPath.substr(localDirectory_.length()).quoteIfHasSpaces();
        }

        return NoFileSystemError;
    }

    return ResourceMissingFileSystemError;
}

FileSystemError LocalDirectoryFileSystemVolume::open(const UnicodeString& filename, FileWriter& file, bool asText)
{
    if (file.openLocalFile(FileSystem::joinPaths(localDirectory_, filename), asText))
        return NoFileSystemError;

    return AccessDeniedFileSystemError;
}

bool LocalDirectoryFileSystemVolume::doesFileExist(const UnicodeString& filename) const
{
    return FileSystem::doesLocalFileExist(FileSystem::joinPaths(localDirectory_, filename));
}

FileSystemError LocalDirectoryFileSystemVolume::enumerateFiles(const UnicodeString& directory, const UnicodeString& extension,
                                                               bool recursive, Vector<UnicodeString>& files) const
{
    auto localFiles = Vector<UnicodeString>();
    FileSystem::enumerateLocalFiles(FileSystem::joinPaths(localDirectory_, directory), extension, recursive, localFiles);

    for (const auto& localFile : localFiles)
        files.append(localFile.withoutPrefix(localDirectory_));

    return NoFileSystemError;
}

FileSystemError LocalDirectoryFileSystemVolume::deleteFile(const UnicodeString& filename)
{
    if (!doesFileExist(filename))
        return ResourceMissingFileSystemError;

    if (FileSystem::deleteLocalFile(FileSystem::joinPaths(localDirectory_, filename)))
        return NoFileSystemError;

    return AccessDeniedFileSystemError;
}

}

#endif
