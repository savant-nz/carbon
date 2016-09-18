/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * \file
 */

/**
 * \public
 *
 * The possible errors that can be reported by a file system operation. See FileReader::getLastError() or
 * FileWriter::getLastError() for details. File system errors can be handled centrally in an application by registering
 * to receive FileSystemErrorEvent.
 */
enum FileSystemError
{
    /**
     * The requested operation succeeded without any errors.
     */
    NoFileSystemError,

    /**
     * The specified file or directory resource does not exist and so the requested operation could not be completed.
     */
    ResourceMissingFileSystemError,

    /**
     * The specified file or directory name was invalid, this usually occurs when a name is empty or contains invalid
     * characters.
     */
    InvalidResourceNameFileSystemError,

    /**
     * The privilege level is not sufficient to perform the requested operation. This can be due to lacking read
     * permissions when reading a file or not having write permission when trying to write a file.
     */
    AccessDeniedFileSystemError,

    /**
     * Invalid data was read while trying to perform the requested operation.
     */
    InvalidDataFileSystemError,

    /**
     * The target does not have sufficient free space to perform the requested operation.
     */
    FreeSpaceFileSystemError,

    /**
     * The target does not support the requested operation.
     */
    NotSupportedFileSystemError,

    /**
     * There was not enough free memory or other resources such as available file handles to perform the requested
     * operation.
     */
    OutOfMemoryFileSystemError,

    /**
     * The requested operation is not valid given the current state or setup of the relevant FileReader or FileWriter
     * class.
     */
    InvalidOperationFileSystemError,

    /**
     * For some reason the requested operation wasn't able to be fully completed, one possible cause for this error is
     * that fewer than the requested number of bytes were able to be read or written.
     */
    IncompleteFileSystemError,

    /**
     * When working with versioned file sections (see FileReader::beginVersionedSection() for details), this error
     * indicates there was an error in the header for a versioned file section.
     */
    VersionedSectionFileSystemError,

    /**
     * Data corruption was encountered while performing the requested operation, this can be caused by faulty hardware
     * or by illegal tampering with memory reserved for exclusive access by the underlying file system.
     */
    DataCorruptionFileSystemError,

    /**
     * A hardware failure was encountered while performing the requested operation that prevented it from being
     * completed successfully.
     */
    HardwareFailureFileSystemError,

    /**
     * The specified file system could not be accessed because it uses removable media and no media is currently
     * present, e.g. memory stick is not inserted.
     */
    RemovableMediaNotPresentError,

    /**
     * An unknown error occurred while performing the requested operation.
     */
    UnknownFileSystemError
};

}
