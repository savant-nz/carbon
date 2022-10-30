/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Endian.h"
#include "CarbonEngine/Core/FileSystem/FileSystemError.h"

namespace Carbon
{

/**
 * This is the primary class for writing file data and is generally used in conjunction with the FileSystem class and
 * the FileSystem::open() method. This class provides methods for standard file output tasks, reading common data types,
 * and automatically handles any endian conversions that are needed (all data is stored in little endian format). Files
 * can be written to either the local file system or to a memory buffer.
 */
class CARBON_API FileWriter : private Noncopyable
{
public:

    FileWriter();

    /**
     * Copy constructor (not implemented).
     */
    FileWriter(const FileWriter& other);

    /**
     * Convenience constructor that automatically calls FileWriter::openLocalFile() with the specified parameters. Note
     * that FileWriter::openLocalFile() will still throw an Exception if an error occurs.
     */
    explicit FileWriter(const UnicodeString& filename, bool asText = false);

    ~FileWriter();

    /**
     * Methods performed on the FileSystem class typically return a simple success flag, however if more detailed error
     * information is required then it can be retrieved either by using this method or by handling FileSystemErrorEvent.
     */
    FileSystemError getLastError() const;

    /**
     * Sets the error code that will be returned by FileWriter::getLastError(). If \a error is not \a NoFileSystemError
     * then this method will queue a FileSystemErrorEvent.
     */
    void setLastError(FileSystemError error) const;

    /**
     * Typedef for a function that can optionally be called when a memory file is closed, implementations of this
     * function prototype must be thread-safe. See FileWriter::openMemoryFile() for details.
     */
    typedef std::function<bool(FileWriter& file, void* userData)> OnCloseFunction;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    /**
     * Opens a file on the local file system for writing. \a asText should be true if text will be written to the file
     * in order to write the correct type of newline for the current platform. Any directories in the given file path
     * that do not exist will be created prior to opening the file for writing. This method automatically overwrites any
     * existing file of the same name.
     */
    bool openLocalFile(const UnicodeString& filename, bool asText = false);
#endif

    /**
     * Opens this file for writing data directly into memory. Memory files have a few limitations compared to standard
     * files: they can't be seeked in and they can't be opened as text files. The contents of a memory file can be
     * retrieved through the FileWriter::getMemoryFileData() method. If required, a function pointer can be specified
     * that will be called when this file is closed, this can be used as a hook to write out the final contents of the
     * memory file to another location (i.e. using the memory file as a temporary buffer before writing the data out to
     * the true destination). A return value of false from this callback function indicates there was a problem and will
     * cause an Exception to be thrown by the FileWriter::close() method. Note that this callback function must be
     * thread-safe. Calls to this method should usually be accompanied by a call to FileWriter::setName() so that the
     * in-memory file can be identified in logs and error reports.
     */
    void openMemoryFile(OnCloseFunction fnOnClose = nullptr, void* fnOnCloseUserData = nullptr);

    /**
     * Sets the name of this file.
     */
    void setName(const UnicodeString& name);

    /**
     * Returns the name of this file if it is open, or an empty string if it is closed.
     */
    const UnicodeString& getName() const;

    /**
     * Forces any buffered write operations to be executed, only affects files on the local file system.
     */
    void flush();

    /**
     * Closes the file and frees all memory held by this class. Throws an Exception if an error occurs, this will only
     * happen if any buffered writes are unsuccessful. If the file does not close successfully then its final contents
     * are undefined.
     */
    void close();

    /**
     * Returns whether this file is currently open for writing.
     */
    bool isOpen() const;

    /**
     * Returns the current write position in the file. For memory files this will always be at the end of the file.
     */
    unsigned int getPosition() const;

    /**
     * Sets the position of the file pointer where the next write will start. The new position can be set to an absolute
     * byte offset in the file or to offset relative to its current position. Seeking in memory files is not supported.
     * Throws an Exception if an error occurs.
     */
    void setPosition(int offset, bool relative = false);

    /**
     * If this is a memory file then this returns a vector containing the current contents.
     */
    Vector<byte_t>& getMemoryFileData();

    /**
     * Writes binary data directly to the file. If \a data is null then zeros are written to the file. Throws an
     * Exception if an error occurs.
     */
    void writeBytes(const void* data, unsigned int size);

    /**
     * Writes the passed text to the file as UTF8. If this file is a local file opened with \a asText set to true then
     * this method will convert to the current system line ending style as appropriate. Throws an Exception if an error
     * occurs.
     */
    void writeText(const UnicodeString& text, unsigned int newlineCount = 1);

    /**
     * Writes a boolean to the file. Throws an Exception if an error occurs.
     */
    void write(bool b)
    {
        auto c = byte_t(b ? 1 : 0);

        return writeBytes(&c, 1);
    }

    /**
     * Writes a character to the file. Throws an Exception if an error occurs.
     */
    void write(char c) { writeBytes(&c, 1); }

    /**
     * Writes a UTF-32 character to the file. Throws an Exception if an error occurs.
     */
    void write(char32_t c) { writeValue(c); }

    /**
     * Writes an unsigned char to the file. Throws an Exception if an error occurs.
     */
    void write(unsigned char uc) { writeBytes(&uc, 1); }

    /**
     * Writes a short integer to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(short s) { writeValue(s); }

    /**
     * Writes an unsigned short integer to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(unsigned short us) { writeValue(us); }

    /**
     * Writes a signed integer to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(int i) { writeValue(i); }

    /**
     * Writes an unsigned integer to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(unsigned int ui) { writeValue(ui); }

    /**
     * Writes a signed 64-bit integer to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(int64_t i) { writeValue(i); }

    /**
     * Writes an unsigned 64-bit integer to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(uint64_t ui) { writeValue(ui); }

    /**
     * Writes a 32-bit float to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(float f) { writeValue(f); }

    /**
     * Writes a 64-bit double to the file (endian safe). Throws an Exception if an error occurs.
     */
    void write(double d) { writeValue(d); }

    /**
     * Writes to the file any class that has the `save(FileWriter& file) const` method defined. Throws an Exception if
     * an error occurs.
     */
    template <typename T> void write(const T& t) { t.save(*this); }

    /**
     * Writes all passed arguments to the file using FileWriter::write().
     */
    template <typename FirstType, typename... OtherTypes> void write(const FirstType& first, OtherTypes&&... others)
    {
        write(first);
        write(std::forward<OtherTypes>(others)...);
    }

    /**
     * Writes an enum value to the file as a signed integer. Throws an Exception if an error occurs.
     */
    template <typename EnumType> void writeEnum(EnumType value) { write(int(value)); }

    /**
     * Saves a `std::array` to the file. Throws an Exception if an error occurs.
     */
    template <typename T, size_t Size> void write(const std::array<T, Size>& array)
    {
        for (auto& item : array)
            write(item);
    }

    /**
     * Saves a Vector to the file. Throws an Exception if an error occurs.
     */
    template <typename T> void write(const Vector<T>& vector)
    {
        write(vector.size());

        for (auto& item : vector)
            write(item);
    }

    /**
     * Writes a vector of pointers to the file. Each item is dereferenced and written to the file. Throws an Exception
     * if an error occurs.
     */
    template <typename T> void writePointerVector(const Vector<T*>& vector)
    {
        write(vector.size());

        for (auto& item : vector)
            write(*item);
    }

    /**
     * Begins writing a versioned section, these are the system by which features can be added to binary file formats
     * while maintaining a high level of both backward and forward compatibility. There must be a corresponding call to
     * FileWriter::endVersionedSection(). Throws an Exception if an error occurs. This method writes a versioned section
     * header which consists of the the major version and minor version in the passed \a versionInfo, followed by a
     * placeholder for the final section size value that will be filled in by the corresponding call to
     * FileWriter::endVersionedSection().
     */
    void beginVersionedSection(const VersionInfo& versionInfo);

    /**
     * Ends writing the most recently begun versioned section started with FileWriter::beginVersionedSection(). This
     * method goes back and fills in the section byte size value in the section header with how large the section was.
     * Throws an Exception if an error occurs.
     */
    void endVersionedSection();

private:

    template <typename T> void writeValue(T t)
    {
#ifdef CARBON_BIG_ENDIAN
        Endian::convert(t);
#endif
        writeBytes(&t, sizeof(T));
    }

    class Members;
    Members* m = nullptr;
};

}
