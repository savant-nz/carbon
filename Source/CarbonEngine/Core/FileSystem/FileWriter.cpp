/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/Endian.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/FileSystem/FileWriter.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

class FileWriter::Members
{
public:

    UnicodeString name;

    FileSystemError lastError = NoFileSystemError;

    enum
    {
        NoOpenFile,
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
        LocalFile,
#endif
        MemoryFile
    } type = NoOpenFile;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    // Used when writing to a file
    FILE* file = nullptr;
#endif

    // Used when writing to a memory location
    Vector<byte_t> memoryFileData;          // Memory the file is being written into
    OnCloseFunction fnOnClose = nullptr;    // Function to call when the memory file is closed
    void* fnOnCloseUserData = nullptr;      // User-specified pointer to pass to fnOnClose() when it is called

    // Holds the byte offset where each section begins
    Vector<unsigned int> versionedSectionStack;
};

FileWriter::FileWriter()
{
    m = new Members;
}

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

FileWriter::FileWriter(const UnicodeString& filename, bool asText)
{
    m = new Members;

    if (filename.length())
        openLocalFile(filename, asText);
}

#endif

FileWriter::~FileWriter()
{
    close();

    delete m;
    m = nullptr;
}

FileSystemError FileWriter::getLastError() const
{
    return m->lastError;
}

void FileWriter::setLastError(FileSystemError error) const
{
    if (error != NoFileSystemError)
    {
        if (Globals::isEngineInitialized())
            events().queueEvent(new FileSystemErrorEvent(error, getName()));
        else
        {
            // Couldn't send file system error because the engine isn't initialized. An error can't be logged because this
            // situation can happen as part of log initialization, and so logging here could cause infinite recursion.
        }
    }

    m->lastError = error;
}

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

bool FileWriter::openLocalFile(const UnicodeString& filename, bool asText)
{
    close();

    if (filename.length() == 0)
    {
        setLastError(InvalidResourceNameFileSystemError);
        return false;
    }

    // Go through the path string and create all directories it requires
    auto index = filename.findFirstOf("/");
    while (index != -1)
    {
        auto directory = filename.substr(0, index + 1);
        if (!FileSystem::doesLocalDirectoryExist(directory))
        {
#ifdef WINDOWS
            auto result = _wmkdir(directory.toUTF16().as<wchar_t>());
#else
            auto result = mkdir(directory.toUTF8().as<char>(), 0755);
#endif
            if (result != 0 && errno != EISDIR && errno != EEXIST)
                return false;
        }

        index = filename.findFirstOf("/", index + 1);
    }

#ifdef WINDOWS
    m->file = _wfopen(filename.toUTF16().as<wchar_t>(), asText ? L"wt" : L"wb");
#else
    m->file = fopen(filename.toUTF8().as<char>(), asText ? "wt" : "wb");
#endif
    if (!m->file)
    {
        setLastError(ResourceMissingFileSystemError);
        return false;
    }

    m->name = filename;
    m->type = Members::LocalFile;

    return true;
}

#endif

void FileWriter::openMemoryFile(OnCloseFunction fnOnClose, void* fnOnCloseUserData)
{
    // Preserve the file name across the call to close()
    auto name = m->name;
    close();
    m->name = name;

    m->type = Members::MemoryFile;
    m->fnOnClose = fnOnClose;
    m->fnOnCloseUserData = fnOnCloseUserData;
}

void FileWriter::setName(const UnicodeString& name)
{
    m->name = name;
}

const UnicodeString& FileWriter::getName() const
{
    return m->name;
}

void FileWriter::flush()
{
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile && m->file)
        fflush(m->file);
#endif
}

void FileWriter::close()
{
#ifdef CARBON_DEBUG
    if (m->versionedSectionStack.size())
        LOG_WARNING << "Section stack is not empty";
#endif

    auto fnOnCloseResult = true;

    if (m->fnOnClose)
    {
        fnOnCloseResult = m->fnOnClose(*this, m->fnOnCloseUserData);
        m->fnOnClose = nullptr;
        m->fnOnCloseUserData = nullptr;
    }

    // If the onClose function returns false then it should also have set the last error
    if (fnOnCloseResult)
        setLastError(NoFileSystemError);

    m->name.clear();
    m->type = Members::NoOpenFile;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->file)
        fclose(m->file);
    m->file = nullptr;
#endif

    m->memoryFileData.clear();

    m->versionedSectionStack.clear();

    if (!fnOnCloseResult)
        throw Exception("Failed closing file");
}

bool FileWriter::isOpen() const
{
    return m->type != Members::NoOpenFile;
}

unsigned int FileWriter::getPosition() const
{
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
        return uint(ftell(m->file));
#endif

    if (m->type == Members::MemoryFile)
        return m->memoryFileData.size();

    return 0;
}

void FileWriter::setPosition(int offset, bool relative)
{
    if (!isOpen())
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("File is not open");
    }

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        if (relative)
            fseek(m->file, offset, SEEK_CUR);
        else
        {
            if (offset > 0)
                fseek(m->file, offset, SEEK_SET);
        }
    }
#endif

    if (m->type == Members::MemoryFile)
        throw Exception("Seeking is not supported in memory files");
}

Vector<byte_t>& FileWriter::getMemoryFileData()
{
    return m->memoryFileData;
}

void FileWriter::writeBytes(const void* data, unsigned int size)
{
    if (!isOpen())
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("File is not open");
    }

    if (!size)
        return;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        if (!data)
        {
            // Write zeros to the file
            for (auto i = 0U; i < size; i++)
            {
                if (fwrite("", 1, 1, m->file) != 1)
                {
                    setLastError(IncompleteFileSystemError);
                    throw Exception("Failed writing to file");
                }
            }
        }
        else
        {
            if (fwrite(data, 1, size, m->file) != size)
            {
                setLastError(IncompleteFileSystemError);
                throw Exception("Failed writing to file");
            }
        }
    }
#endif

    if (m->type == Members::MemoryFile)
    {
        auto initialSize = m->memoryFileData.size();

        // Increase allocation size
        try
        {
            m->memoryFileData.resize(initialSize + size);
        }
        catch (const std::bad_alloc&)
        {
            setLastError(OutOfMemoryFileSystemError);
            throw Exception("Failed allocating memory for file storage");
        }

        // Write data to memory file
        if (data)
            memcpy(&m->memoryFileData[initialSize], data, size);
        else
            memset(&m->memoryFileData[initialSize], 0, size);
    }
}

void FileWriter::writeText(const UnicodeString& text, unsigned int newlineCount)
{
    auto utf8 = text.toUTF8();

    writeBytes(utf8.getData(), utf8.size() - 1);

    while (newlineCount--)
        writeBytes("\n", 1);
}

// Writes the section begin ID, major version, minor version and four placeholder bytes for the section size that will be filled
// in later by the call to FileWriter::endVersionedSection()
void FileWriter::beginVersionedSection(const VersionInfo& versionInfo)
{
    write(FileSystem::SectionBeginID);
    write(versionInfo.getMajor());
    write(versionInfo.getMinor());

    auto placeholder = 0U;
    write(placeholder);

    m->versionedSectionStack.append(getPosition());
}

void FileWriter::endVersionedSection()
{
    if (!isOpen())
    {
        setLastError(InvalidOperationFileSystemError);
        throw Exception("File is not open");
    }

    if (m->versionedSectionStack.empty())
    {
        setLastError(VersionedSectionFileSystemError);
        throw Exception("Unmatched section");
    }

    // The size of this section is the number of bytes written since the section header
    auto sectionSize = getPosition() - m->versionedSectionStack.back();

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (m->type == Members::LocalFile)
    {
        auto sectionSizeOffset = m->versionedSectionStack.back() - sizeof(unsigned int);

        if (fseek(m->file, sectionSizeOffset, SEEK_SET) != 0 || fwrite(&sectionSize, 1, 4, m->file) != 4 ||
            fseek(m->file, 0, SEEK_END) != 0)
        {
            setLastError(VersionedSectionFileSystemError);
            throw Exception("Failed writing section size into header");
        }
    }
#endif

    if (m->type == Members::MemoryFile)
    {
        auto sectionSizeOffset = uint(m->versionedSectionStack.back() - sizeof(unsigned int));

        *reinterpret_cast<unsigned int*>(&m->memoryFileData[sectionSizeOffset]) = sectionSize;
    }

    // Write section end ID
    write(FileSystem::SectionEndID);

    m->versionedSectionStack.popBack();
}

}
