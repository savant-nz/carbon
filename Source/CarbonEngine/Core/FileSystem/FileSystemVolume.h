/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

/**
 * Describes a file system volume that can be mounted into the main file system so that its contents are accessible by the whole
 * application. This class is subclassed to make content available on the main file system. Note that because file system access
 * can occur on any thread this class needs to manage any additional synchronization that is needed if there are situations
 * where the synchronization by the main file system mutex is insufficient.
 */
class CARBON_API FileSystemVolume : private Noncopyable
{
public:

    /**
     * Initializes this volume with the given name.
     */
    FileSystemVolume(UnicodeString name) : name_(std::move(name)) {}

    virtual ~FileSystemVolume() {}

    /**
     * Returns the name of this volume.
     */
    const UnicodeString& getName() const { return name_; }

    /**
     * Attempts to open a file on this volume for reading. Returns a file system error flag.
     */
    virtual FileSystemError open(const UnicodeString& filename, FileReader& file) const { return NotSupportedFileSystemError; }

    /**
     * Attempts to open a file on this volume for writing. Returns a file system error flag.
     */
    virtual FileSystemError open(const UnicodeString& filename, FileWriter& file, bool asText)
    {
        return NotSupportedFileSystemError;
    }

    /**
     * Returns whether or not this file system volume contains the given file.
     */
    virtual bool doesFileExist(const UnicodeString& filename) const { return false; }

    /**
     * Identical to FileSystem::enumerateFiles() except only returns the file matches for this volume. Returns error flag.
     */
    virtual FileSystemError enumerateFiles(const UnicodeString& directory, const UnicodeString& extension, bool recursive,
                                           Vector<UnicodeString>& files) const
    {
        return NotSupportedFileSystemError;
    }

    /**
     * Deletes a file from the file system volume. Returns error flag.
     */
    virtual FileSystemError deleteFile(const UnicodeString& filename) { return NotSupportedFileSystemError; }

    /**
     * Returns whether this volume has enough free space to create the given number of directories and create new files of the
     * given sizes. The size of each file to check whether there is space for should be passed in the \a fileSizes vector.
     */
    virtual FileSystemError hasSpaceFor(unsigned int directoryCount, const Vector<unsigned int>& fileSizes) const
    {
        return NotSupportedFileSystemError;
    }

    /**
     * Returns whether this volume has enough free space to create and write a single file of the given size in an existing
     * directory. This is a wrapper for the more general functionality provided by FileSystemVolume::hasSpaceFor().
     */
    FileSystemError hasSpaceFor(unsigned int fileSizeInBytes) const
    {
        return hasSpaceFor(0, Vector<unsigned int>(1, fileSizeInBytes));
    }

    /**
     * Returns the amount of free space on this volume in bytes. Note that this method should not be used to determine whether
     * or not there is space to write a given set of files of specific sizes, to check that use one of the
     * FileSystemVolume::hasSpaceFor() methods. Volumes do not have to support this method, and an error is logged and zero is
     * returned if this method is called on a volume which does not support checking for free space.
     */
    virtual unsigned int getFreeSpaceInBytes() const
    {
        LOG_ERROR << "Not supported on this volume";
        return 0;
    }

    // The following methods are for use with file system volumes that provide detailed information about used and free file
    // system blocks that needs to be accessible by the user. Errors will be logged if any of the methods below are used on file
    // system volumes that don't support them.

    /**
     * For file system volumes that provide information about blocks, this returns the size in bytes of a single file system
     * block. The value returned by FileSystemVolume::getFreeSpaceInBytes() will be a multiple of the block size. An error is
     * logged and zero is returned if this method is not supported by this file system volume.
     */
    virtual unsigned int getBlockSize() const
    {
        LOG_ERROR << "Not supported on this volume";
        return 0;
    }

    /**
     * For file system volumes that provide information about blocks, this returns the number of file system blocks that are
     * required to store a file of the given size. An error is logged and zero is returned if this method is not supported by
     * this file system volume.
     */
    unsigned int getBlockCountForFileSize(unsigned int fileSizeInBytes) const
    {
        if (!getBlockSize())
            return 0;

        return std::max(1U, Math::roundUp(fileSizeInBytes, getBlockSize()) / getBlockSize());
    }

    /**
     * For file system volumes that provide information about blocks, this returns the number of free file system blocks that
     * are currently available for use by the application. This value is calculated by dividing the free space returned by
     * FileSystemVolume::getFreeSpaceInBytes() by the block size returned by FileSystemVolume::getBlockSize(). Note that this
     * method should not be used to determine whether or not there is space to write a given file to this volume, instead use
     * one of the FileSystemVolume::hasSpaceFor() methods. An error is logged and zero is returned if this method is not
     * supported by the file system volume.
     */
    unsigned int getFreeBlockCount() const
    {
        if (!getBlockSize())
            return 0;

        assert((getFreeSpaceInBytes() % getBlockSize()) == 0 && "Free space is not a multiple of the block size");

        return getFreeSpaceInBytes() / getBlockSize();
    }

    /**
     * For file system volumes that provide information about blocks, this returns the number of file system blocks that need to
     * be freed up in order to save a file of the given size. An error is logged and zero is returned if this method is not
     * supported by this file system volume. Note that this method should not be used to determine whether or not there is space
     * to write a given file to this volume, to check that use one of the FileSystemVolume::hasSpaceFor() methods.
     */
    unsigned int getAdditionalBlocksRequiredForFileSize(unsigned fileSizeInBytes) const
    {
        if (!getBlockSize())
            return 0;

        unsigned int requiredBlocks = getBlockCountForFileSize(fileSizeInBytes);
        if (getFreeBlockCount() >= requiredBlocks)
            return 0;

        return requiredBlocks - getFreeBlockCount();
    }

    /**
     * Returns the value of the \a mountLocation argument used when this volume was mounted into the virtual file system by
     * FileSystem::addVolume().
     */
    const UnicodeString& getMountLocation() const { return mountLocation_; }

private:

    const UnicodeString name_;

    // The mount location on the file system, or an empty string when this volume is not mounted
    friend class FileSystem;
    UnicodeString mountLocation_;
};

}
