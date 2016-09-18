/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/Endian.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileReader.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

class FileReader::Members
{
public:

    UnicodeString name;

    enum
    {
        NoOpenFile,
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
        LocalFile,
#endif
        MemoryFile
    } type = NoOpenFile;

    FileSystemError lastError = NoFileSystemError;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

    // For files on the local file system this is the descriptor
    FILE* localFile = nullptr;

    // Memory mapping of a file open on the lcoal file system
    byte_t* localFileMemoryMap = nullptr;
#ifdef WINDOWS
    HANDLE hLocalFileMemoryMap = nullptr;
#endif

#endif

    // The size of this file in bytes
    unsigned int size = 0;

    // Storage for files whose data is contained entirely in memory rather than loaded from the local file system
    Vector<byte_t> fileData;

    // Current file read position
    unsigned int position = 0;

    // Holds details about each versioned section in the file
    struct VersionedSectionStackEntry
    {
        const VersionInfo* versionInfo = nullptr;

        // The version numbers read from the file
        VersionInfo readVersion;

        // Byte offset in the file to the end of data in this versioned section
        unsigned int sectionEndOffset = 0;

        VersionedSectionStackEntry() {}
        VersionedSectionStackEntry(const VersionInfo* versionInfo_, const VersionInfo& readVersion_,
                                   unsigned int sectionEndOffset_)
            : versionInfo(versionInfo_), readVersion(readVersion_), sectionEndOffset(sectionEndOffset_)
        {
        }
    };
    Vector<VersionedSectionStackEntry> versionedSectionStack;
};

FileReader::FileReader()
{
    m = new Members;
    close();
}

FileReader::~FileReader()
{
    close();

    delete m;
    m = nullptr;
}

FileSystemError FileReader::getLastError() const
{
    return m->lastError;
}

void FileReader::setLastError(FileSystemError error) const
{
    if (error != NoFileSystemError)
        events().queueEvent(new FileSystemErrorEvent(error, getName()));

    m->lastError = error;
}

void FileReader::close()
{
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

    // Close any memory mapping on this file
    if (m->localFileMemoryMap)
    {
#ifdef WINDOWS
        UnmapViewOfFile(m->localFileMemoryMap);
        CloseHandle(m->hLocalFileMemoryMap);
        m->hLocalFileMemoryMap = nullptr;
#else
        munmap(m->localFileMemoryMap, m->size);
#endif
        m->localFileMemoryMap = nullptr;
    }

    if (m->localFile)
    {
        fclose(m->localFile);
        m->localFile = nullptr;
    }
#endif

    m->type = Members::NoOpenFile;
    m->fileData.clear();
    m->lastError = NoFileSystemError;
    m->size = 0;
    m->position = 0;
    m->versionedSectionStack.clear();
}

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

bool FileReader::openLocalFile(const UnicodeString& filename)
{
    close();

    if (!fileSystem().isValidFileName(filename))
    {
        setLastError(InvalidResourceNameFileSystemError);
        return false;
    }

#ifdef WINDOWS
    m->localFile = _wfopen(filename.toUTF16().as<wchar_t>(), L"rb");
#else
    m->localFile = fopen(filename.toUTF8().as<char>(), "rb");
#endif
    if (!m->localFile)
        return false;

    // Determine file size
    fseek(m->localFile, 0, SEEK_END);
    auto fileSize = int64_t(ftell(m->localFile));
    fseek(m->localFile, 0, SEEK_SET);

    // Files larger than 2GB aren't supported
    if (fileSize > std::numeric_limits<int>::max())
    {
        LOG_ERROR << "File '" << filename
                  << "' can't be opened because it is karger than 2GB, size: " << FileSystem::formatByteSize(fileSize);

        close();
        setLastError(InvalidOperationFileSystemError);
        return false;
    }

    m->type = Members::LocalFile;
    m->size = uint(fileSize);

#ifdef WINDOWS
    // Try and memory map the file on Windows
    auto hFile = HANDLE(_get_osfhandle(_fileno(m->localFile)));
    m->hLocalFileMemoryMap = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (m->hLocalFileMemoryMap)
    {
        m->localFileMemoryMap =
            reinterpret_cast<byte_t*>(MapViewOfFile(m->hLocalFileMemoryMap, FILE_MAP_READ, 0, 0, 0));

        if (!m->localFileMemoryMap)
        {
            CloseHandle(m->hLocalFileMemoryMap);
            m->hLocalFileMemoryMap = nullptr;
        }
    }
#else
    // Try to memory map the file on POSIX
    auto mapResult = mmap(nullptr, m->size, PROT_READ, MAP_PRIVATE | MAP_FILE, fileno(m->localFile), 0);
    if (mapResult != MAP_FAILED)
        m->localFileMemoryMap = reinterpret_cast<byte_t*>(mapResult);
#endif

    setName(filename);

    return true;
}

#endif

bool FileReader::openMemoryFile(Vector<byte_t>& data)
{
    close();

    m->type = Members::MemoryFile;
    m->size = data.size();
    swap(m->fileData, data);

    return true;
}

bool FileReader::isOpen() const
{
    return m->type != Members::NoOpenFile;
}

const UnicodeString& FileReader::getName() const
{
    return m->name;
}

void FileReader::setName(const UnicodeString& name)
{
    m->name = name;
}

unsigned int FileReader::getSize() const
{
    return m->size;
}

byte_t* FileReader::getData(Vector<byte_t>& fileDataStorage)
{
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        if (m->localFileMemoryMap)
            return m->localFileMemoryMap;

        // Allocate space in the provided vector
        try
        {
            fileDataStorage.resize(m->size);
        }
        catch (const std::bad_alloc&)
        {
            throw Exception("Failed allocating memory for file data");
        }

        // Reset to the start of the file
        auto previousPosition = getPosition();
        setPosition(0);

        // Read the data into the provided vector
        try
        {
            auto bytesRead = 0U;
            readBytes(fileDataStorage.getData(), getSize(), &bytesRead);
            if (bytesRead != getSize())
                throw Exception("Failed reading file data into memory buffer");
        }
        catch (const Exception&)
        {
            fileDataStorage.clear();
            setPosition(previousPosition);

            throw;
        }

        // Return to the previous position in the file
        setPosition(previousPosition);

        return fileDataStorage.getData();
    }
#endif

    if (m->type == Members::MemoryFile)
    {
        // For memory files the internal pointer can be returned directly
        return m->fileData.getData();
    }

    throw Exception("File is not open");
}

void FileReader::skip(unsigned int amount)
{
    if (!isOpen())
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("File is not open");
    }

    if (!amount)
        return;

    // Check the new position is valid
    if (m->position + amount > m->size)
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("Skip amount goes past end of file");
    }

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        if (m->localFileMemoryMap)
            m->position += amount;
        else
        {
            fseek(m->localFile, amount, SEEK_CUR);
            m->position = uint(ftell(m->localFile));
        }
    }
#endif

    if (m->type == Members::MemoryFile)
        m->position += amount;
}

unsigned int FileReader::getPosition() const
{
    return m->position;
}

void FileReader::setPosition(unsigned int position)
{
    if (position > m->size)
        throw Exception() << "The position " << position << " is past the end of the file, file size: " << m->size;

    if (!isOpen())
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("File is not open");
    }

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        if (m->localFileMemoryMap)
            m->position = position;
        else
        {
            fseek(m->localFile, position, SEEK_SET);
            m->position = uint(ftell(m->localFile));
        }
    }
#endif

    if (m->type == Members::MemoryFile)
        m->position = position;
}

bool FileReader::isEOF() const
{
    return m->position >= m->size;
}

void FileReader::readBytes(void* buffer, unsigned int count, unsigned int* bytesRead)
{
    if (!isOpen())
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("File is not open");
    }

    auto fallbackBytesRead = 0U;
    if (!bytesRead)
        bytesRead = &fallbackBytesRead;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        if (m->localFileMemoryMap)
        {
            *bytesRead = std::min(count, m->size - m->position);
            memcpy(buffer, &m->localFileMemoryMap[m->position], *bytesRead);
            m->position += *bytesRead;
        }
        else
        {
            *bytesRead = uint(fread(buffer, 1, count, m->localFile));
            m->position = uint(ftell(m->localFile));
        }
    }
#endif

    if (m->type == Members::MemoryFile)
    {
        *bytesRead = std::min(count, m->size - m->position);
        memcpy(buffer, &m->fileData[m->position], *bytesRead);
        m->position += *bytesRead;
    }

    // Throw an exception if some requested data was not read
    if (*bytesRead != count)
    {
        setLastError(IncompleteFileSystemError);
        throw Exception() << "Failed reading file, requested " << count << " bytes but only read " << *bytesRead
                          << " bytes";
    }
}

UnicodeString FileReader::readUTF8Data(unsigned int byteCount)
{
    auto utf8 = Vector<byte_t>();

    try
    {
        utf8.resize(byteCount);
    }
    catch (const std::bad_alloc&)
    {
        throw Exception("Failed allocating space for UTF8 data");
    }

    readBytes(utf8.getData(), byteCount);

    return fromUTF8(utf8.getData(), byteCount);
}

bool FileReader::getDataAsString(UnicodeString& string)
{
    try
    {
        auto storage = Vector<byte_t>();
        auto data = getData(storage);

        string = fromUTF8(data, m->size);

        return true;
    }
    catch (const Exception&)
    {
        string.clear();
        return false;
    }
}

bool FileReader::getDataAsString(String& string)
{
    auto s = UnicodeString();

    auto result = getDataAsString(s);
    string = s.toASCII();

    return result;
}

bool FileReader::getLineTokens(Vector<Vector<UnicodeString>>& lineTokens)
{
    // Split into lines
    auto lines = Vector<UnicodeString>();
    if (!getLines(lines, false))
        return false;

    // Remove comments
    for (auto i = 0U; i < lines.size(); i++)
    {
        lines[i].removeComments();
        if (!lines[i].length())
            lines.erase(i--);
    }

    // Split lines into tokens
    lineTokens.resize(lines.size());
    for (auto i = 0U; i < lines.size(); i++)
        lineTokens[i] = lines[i].getTokens();

    return true;
}

bool FileReader::getLineTokens(Vector<Vector<String>>& lineTokens)
{
    auto unicodeTokens = Vector<Vector<UnicodeString>>();
    if (!getLineTokens(unicodeTokens))
        return false;

    lineTokens.resize(unicodeTokens.size());
    for (auto i = 0U; i < lineTokens.size(); i++)
        lineTokens[i] = A(unicodeTokens[i]);

    return true;
}

VersionInfo FileReader::beginVersionedSection(const VersionInfo& versionInfo)
{
    // Read section begin ID
    auto id = byte_t();
    read(id);
    if (id != FileSystem::SectionBeginID)
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception("Invalid section begin ID");
    }

    auto sectionSize = 0U;

    // Read version numbers
    auto readVersion = VersionInfo();
    read(readVersion, sectionSize);

    // Check major version
    if (readVersion.getMajor() > versionInfo.getMajor())
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception() << "Unsupported section version " << readVersion << ", only versions compatible with "
                          << versionInfo << " are supported";
    }

    // Check section does not go past the end of the file
    if (m->position + sectionSize > m->size)
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception("Invalid section size, extends past end of file");
    }

    m->versionedSectionStack.emplace(&versionInfo, readVersion, m->position + sectionSize);

    return readVersion;
}

void FileReader::endVersionedSection()
{
    if (m->versionedSectionStack.empty())
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception("Unexpected empty versioned section stack");
    }

    // We should only be seeking forwards, seeking backwards would mean something has gone wrong
    if (m->versionedSectionStack.back().sectionEndOffset < m->position)
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception("Section size does not match up");
    }

    // Seek to end of versioned section
    skip(m->versionedSectionStack.back().sectionEndOffset - m->position);

    // Read section end ID
    auto id = byte_t();
    read(id);
    if (id != FileSystem::SectionEndID)
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception("Invalid section end ID");
    }

    m->versionedSectionStack.popBack();
}

VersionInfo FileReader::findVersionedSection(const VersionInfo& versionInfo) const
{
    for (auto i = 0U; i < m->versionedSectionStack.size(); i++)
    {
        const auto& entry = m->versionedSectionStack[m->versionedSectionStack.size() - i - 1];

        if (entry.versionInfo == &versionInfo)
            return entry.readVersion;
    }

    throw Exception("Failed finding versioned section");
}

}
