/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/FileSystem/ZipFileSystemVolume.h"

#ifdef CARBON_INCLUDE_ZLIB
    #include "zlib.h"

    #ifdef _MSC_VER
        #pragma comment(lib, "ZLib" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
    #endif
#endif

namespace Carbon
{

bool ZipFileSystemVolume::setup(const UnicodeString& zipFilename)
{
    zipFile_.close();
    entries_.clear();

    try
    {
        fileSystem().open(zipFilename, zipFile_);

        // Read the 'end of central directory record' at the end of the zip file
        auto signature = uint32_t();
        auto centralDirectoryOffset = uint32_t();
        auto centralDirectoryRecordCount = uint16_t();

        zipFile_.setPosition(zipFile_.getSize() - 22);
        zipFile_.read(signature);
        zipFile_.skip(6);
        zipFile_.read(centralDirectoryRecordCount);
        zipFile_.skip(4);
        zipFile_.read(centralDirectoryOffset);

        if (signature != 0x06054b50)
            throw Exception("Invalid central directory header signature");

        // Read central directory entries
        zipFile_.setPosition(centralDirectoryOffset);
        for (auto i = 0U; i < centralDirectoryRecordCount; i++)
        {
            auto entry = Entry();

            auto nameLength = uint16_t();
            auto extraFieldLength = uint16_t();
            auto commentLength = uint16_t();

            zipFile_.read(signature);
            zipFile_.skip(6);
            zipFile_.read(entry.compressionMethod);
            zipFile_.skip(8);
            zipFile_.read(entry.compressedSize, entry.uncompressedSize, nameLength, extraFieldLength, commentLength);
            zipFile_.skip(8);
            zipFile_.read(entry.headerOffset);

            if (signature != 0x02014b50)
                throw Exception("Invalid signature for central directory entry");

            entry.name = zipFile_.readUTF8Data(nameLength);

            if (entry.uncompressedSize)
                entries_.append(entry);

            // Move to next central directory entry
            zipFile_.skip(extraFieldLength + commentLength);
        }

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << getName() << "' - " << e;

        zipFile_.close();
        entries_.clear();

        return false;
    }
}

FileSystemError ZipFileSystemVolume::open(const UnicodeString& filename, FileReader& file) const
{
    for (auto& entry : entries_)
    {
        if (entry.name == filename)
            return entry.read(zipFile_, file);
    }

    return ResourceMissingFileSystemError;
}

FileSystemError ZipFileSystemVolume::Entry::read(FileReader& zipFile, FileReader& file) const
{
    // Read the local file header so we can seek to the start of the data
    auto signature = uint32_t();
    auto nameLength = uint16_t();
    auto extraFieldLength = uint16_t();

    try
    {
        zipFile.setPosition(headerOffset);
        zipFile.read(signature);
        zipFile.skip(22);
        zipFile.read(nameLength, extraFieldLength);
        zipFile.skip(nameLength + extraFieldLength);
    }
    catch (const Exception&)
    {
        return zipFile.getLastError();
    }

    // Validate local file header signature
    if (signature != 0x04034B50)
    {
        LOG_ERROR << "Invalid file header signature";
        return InvalidDataFileSystemError;
    }

    // Allocate space for the resulting data
    auto data = Vector<byte_t>();
    try
    {
        data.resize(uncompressedSize);
    }
    catch (const std::bad_alloc&)
    {
        return OutOfMemoryFileSystemError;
    }

    if (compressionMethod == 0)
    {
        // Read uncompressed file data
        try
        {
            zipFile.readBytes(data.getData(), compressedSize);
        }
        catch (const Exception&)
        {
            return zipFile.getLastError();
        }

        file.openMemoryFile(data);
        return NoFileSystemError;
    }

#ifdef CARBON_INCLUDE_ZLIB
    if (compressionMethod == Z_DEFLATED)
    {
        // Read compressed data into memory
        auto compressedData = Vector<byte_t>();
        try
        {
            compressedData.resize(compressedSize);
        }
        catch (const std::bad_alloc&)
        {
            return OutOfMemoryFileSystemError;
        }

        try
        {
            zipFile.readBytes(compressedData.getData(), compressedData.size());
        }
        catch (const Exception&)
        {
            return zipFile.getLastError();
        }

        // Prepare zlib stream
        auto stream = z_stream();
        stream.next_out = data.getData();
        stream.avail_out = data.size();
        stream.next_in = compressedData.getData();
        stream.avail_in = compressedData.size();

        if (inflateInit2_(&stream, -MAX_WBITS, ZLIB_VERSION, sizeof(stream)))
            return NotSupportedFileSystemError;

        // Run the zlib decompression
        auto succeeded = (inflate(&stream, Z_FINISH) == Z_STREAM_END);
        inflateEnd(&stream);

        if (!succeeded)
        {
            LOG_ERROR << "Zlib decompression failed for file: " << name;
            return InvalidDataFileSystemError;
        }

        file.openMemoryFile(data);
        return NoFileSystemError;
    }
#endif

    LOG_ERROR << "Unsupported compression method on file: " << name;

    return NotSupportedFileSystemError;
}

bool ZipFileSystemVolume::doesFileExist(const UnicodeString& filename) const
{
    return entries_.has([&](const Entry& entry) { return entry.name == filename; });
}

FileSystemError ZipFileSystemVolume::enumerateFiles(const UnicodeString& directory, const UnicodeString& extension,
                                                    bool recursive, Vector<UnicodeString>& files) const
{
    for (auto& entry : entries_)
    {
        if (entry.name.startsWith(directory) && entry.name.endsWith(extension))
        {
            if (!recursive && entry.name.has('/', directory.length()))
                continue;

            files.append(entry.name);
        }
    }

    return NoFileSystemError;
}

}
