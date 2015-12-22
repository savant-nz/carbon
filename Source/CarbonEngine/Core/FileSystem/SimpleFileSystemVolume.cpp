/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/FileSystem/SimpleFileSystemVolume.h"

namespace Carbon
{

void SimpleFileSystemVolume::clear()
{
    auto lock = ScopedMutexLock(mutex_);

    for (auto entry : entries_)
        delete entry;

    entries_.clear();
}

FileSystemError SimpleFileSystemVolume::open(const UnicodeString& filename, FileReader& file) const
{
    auto lock = ScopedMutexLock(mutex_);

    for (auto entry : entries_)
    {
        if (filename == entry->name)
        {
            auto data = Vector<byte_t>();

            try
            {
                data.resize(entry->data.size());
            }
            catch (const std::bad_alloc&)
            {
                return OutOfMemoryFileSystemError;
            }

            memcpy(data.getData(), entry->data.getData(), data.size());

            file.openMemoryFile(data);

            return NoFileSystemError;
        }
    }

    return ResourceMissingFileSystemError;
}

bool SimpleFileSystemVolume::onFileClosed(FileWriter& file, void* userData)
{
    auto volume = reinterpret_cast<InProgressFileWrite*>(userData)->volume;
    auto filename = reinterpret_cast<InProgressFileWrite*>(userData)->filename;

    delete reinterpret_cast<InProgressFileWrite*>(userData);

    // Delete any existing file with this name
    volume->deleteFile(filename);

    {
        auto lock = ScopedMutexLock(volume->mutex_);

        // Allocate a new entry for this file and set its data
        volume->entries_.append(new FileEntry(filename));
        swap(volume->entries_.back()->data, file.getMemoryFileData());
    }

    return true;
}

FileSystemError SimpleFileSystemVolume::open(const UnicodeString& filename, FileWriter& file, bool asText)
{
    file.openMemoryFile(onFileClosed, new InProgressFileWrite(this, filename));

    return NoFileSystemError;
}

bool SimpleFileSystemVolume::doesFileExist(const UnicodeString& filename) const
{
    auto lock = ScopedMutexLock(mutex_);

    return entries_.has([&](const FileEntry* entry) { return entry->name == filename; });
}

FileSystemError SimpleFileSystemVolume::deleteFile(const UnicodeString& filename)
{
    auto lock = ScopedMutexLock(mutex_);

    auto entry = entries_.detect([&](const FileEntry* e) { return e->name == filename; }, nullptr);

    if (!entry)
        return ResourceMissingFileSystemError;

    delete entry;
    entries_.eraseValue(entry);

    return NoFileSystemError;
}

FileSystemError SimpleFileSystemVolume::enumerateFiles(const UnicodeString& directory, const UnicodeString& extension,
                                                       bool recursive, Vector<UnicodeString>& files) const
{
    auto lock = ScopedMutexLock(mutex_);

    for (auto entry : entries_)
    {
        auto& name = entry->name;

        if (name.startsWith(directory) && name.endsWith(extension) && (recursive || !name.has('/', directory.length())))
            files.append(name);
    }

    return NoFileSystemError;
}

bool SimpleFileSystemVolume::renameFile(const UnicodeString& currentName, const UnicodeString& newName)
{
    auto lock = ScopedMutexLock(mutex_);

    if (doesFileExist(newName))
        return false;

    for (auto entry : entries_)
    {
        if (entry->name == currentName)
        {
            entry->name = newName;
            return true;
        }
    }

    return false;
}

void SimpleFileSystemVolume::save(FileWriter& file) const
{
    auto lock = ScopedMutexLock(mutex_);

    file.writePointerVector(entries_);
}

void SimpleFileSystemVolume::load(FileReader& file)
{
    auto lock = ScopedMutexLock(mutex_);

    file.readPointerVector(entries_);
}

}
