/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileSystem/FileSystemVolume.h"
#include "CarbonEngine/Core/Threads/Mutex.h"

namespace Carbon
{

/**
 * This is a simple subclass of FileSystemVolume that exposes a specified set of files to the virtual filesystem. The files to
 * expose are added with SimpleFileSystemVolume::addFile() and they will then be accessible wherever this volume is mounted on
 * the virtual file system. See FileSystem::addVolume() for details.
 */
class CARBON_API SimpleFileSystemVolume : public FileSystemVolume
{
public:

    /**
     * Constructs this simple file system volume with the given name.
     */
    SimpleFileSystemVolume(const UnicodeString& name) : FileSystemVolume(name) {}

    ~SimpleFileSystemVolume() override { clear(); }

    /**
     * Removes all files from this simple file system volume.
     */
    void clear();

    FileSystemError open(const UnicodeString& filename, FileReader& file) const override;
    FileSystemError open(const UnicodeString& filename, FileWriter& file, bool asText) override;
    bool doesFileExist(const UnicodeString& filename) const override;
    FileSystemError enumerateFiles(const UnicodeString& directory, const UnicodeString& extension, bool recursive,
                                   Vector<UnicodeString>& files) const override;
    FileSystemError deleteFile(const UnicodeString& filename) override;

    /**
     * Renames a file in this simple file system volume. Returns success flag.
     */
    bool renameFile(const UnicodeString& currentName, const UnicodeString& newName);

    /**
     * Saves this simple file system volume to a file stream.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this simple file system volume from a file stream.
     */
    void load(FileReader& file);

private:

    mutable Mutex mutex_;

    struct FileEntry
    {
        UnicodeString name;
        Vector<byte_t> data;

        FileEntry() {}
        FileEntry(UnicodeString name_) : name(std::move(name_)) {}

        void save(FileWriter& file) const { file.write(name, true, data); }

        void load(FileReader& file)
        {
            file.read(name);
            file.skip(1);
            file.read(data);
        }
    };

    Vector<FileEntry*> entries_;

    struct InProgressFileWrite
    {
        SimpleFileSystemVolume* volume = nullptr;
        UnicodeString filename;

        InProgressFileWrite() {}
        InProgressFileWrite(SimpleFileSystemVolume* volume_, UnicodeString filename_)
            : volume(volume_), filename(std::move(filename_))
        {
        }
    };

    static bool onFileClosed(FileWriter& file, void* userData);
};

}
