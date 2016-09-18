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
 * This is the primary class for reading file data and is generally used in conjunction with the FileSystem class and
 * the FileSystem::open() method. This class provides methods for standard file input tasks, reading common data types,
 * and automatically handles any endian conversions that are needed (all data is stored in little endian format). Files
 * can be read either from the local file system or to a memory buffer.
 */
class CARBON_API FileReader : private Noncopyable
{
public:

    FileReader();

    /**
     * Copy constructor (not implemented).
     */
    FileReader(const FileReader& other);

    ~FileReader();

    /**
     * Swaps the contents of two FileReader instances.
     */
    friend void swap(FileReader& first, FileReader& second) { std::swap(first.m, second.m); }

    /**
     * Methods performed on the FileSystem class typically return a simple success flag, however if more detailed error
     * information is required then it can be retrieved either by using this method directly on the relevant FileReader
     * or by handling FileSystemErrorEvent.
     */
    FileSystemError getLastError() const;

    /**
     * Sets the error that will be returned by FileReader::getLastError(). If \a error is not NoFileSystemError then
     * this method will also queue a FileSystemErrorEvent using EventManager::queueEvent().
     */
    void setLastError(FileSystemError error) const;

    /**
     * Closes any currently open file, this is run automatically in the destructor.
     */
    void close();

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    /**
     * Opens the specified file on the local file system so it can be read from. Returns success.flag
     */
    bool openLocalFile(const UnicodeString& filename);
#endif

    /**
     * Opens this file as an in-memory file which will read from the passed data. This is used when file data reside
     * somewhere in memory instead of on the local file system. This FileReader instance steals the contents the passed
     * data for itself and leaves \a data empty. Calls to this method should usually be accompanied by a call to
     * FileReader::setName() so that the in-memory file can be identified in logs and error reports.
     */
    bool openMemoryFile(Vector<byte_t>& data);

    /**
     * Returns whether a file is currently open and ready for reading.
     */
    bool isOpen() const;

    /**
     * Returns the name of this file that was passed to FileReader::openLocalFile() or set by a call to
     * FileReader::setName().
     */
    const UnicodeString& getName() const;

    /**
     * Sets the name of this file, the current name can be retrieved using FileReader::getName().
     */
    void setName(const UnicodeString& name);

    /**
     * Returns the size of this file in bytes.
     */
    unsigned int getSize() const;

    /**
     * Returns a pointer to a buffer that contains all the data in this file. In many cases this method can return a
     * pointer to an already existing buffer in which case no extra memory will be allocated by this method. However, in
     * the cases where memory does need to be allocated it will be allocated in the passed \a fileDataStorage Vector and
     * a pointer to the first element in that vector will then be returned. In order to cover this case the caller needs
     * to supply this vector, even though it is only needed in some instances. Throws an Exception if an error occurs.
     */
    byte_t* getData(Vector<byte_t>& fileDataStorage);

    /**
     * Returns the current byte offset in this file, this is the position where the next read will start from.
     */
    unsigned int getPosition() const;

    /**
     * Sets the byte offset in this file where the next read will start from. Throws an Exception if an error occurs.
     */
    void setPosition(unsigned int position);

    /**
     * Returns whether all the data in this file has been read and the current read position is right at the end of the
     * file.
     */
    bool isEOF() const;

    /**
     * Moves the file read position forward by the given number of bytes. Throws an Exception if an error occurs.
     */
    void skip(unsigned int amount);

    /**
     * Reads \a count raw bytes from the file into the specified buffer. When \a bytesRead is not null the integer it
     * points to will be set to the number of bytes that were actually read from the file. Throws an Exception if an
     * error occurs or if the specified number of bytes could not be read.
     */
    void readBytes(void* buffer, unsigned int count, unsigned int* bytesRead = nullptr);

    /**
     * Reads a boolean from the file. Throws an Exception if an error occurs.
     */
    void read(bool& b)
    {
        auto c = byte_t();
        read(c);
        b = (c != 0);
    }

    /**
     * Reads a char from the file. Throws an Exception if an error occurs.
     */
    void read(char& c) { readBytes(&c, 1); }

    /**
     * Reads a UTF-32 character from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(char32_t& c) { readScalar(c); }

    /**
     * Reads an unsigned char from the file. Throws an Exception if an error occurs.
     */
    void read(unsigned char& uc) { readBytes(&uc, 1); }

    /**
     * Reads a short integer from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(short& s) { readScalar(s); }

    /**
     * Reads an unsigned short integer from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(unsigned short& us) { readScalar(us); }

    /**
     * Reads a signed integer from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(int& i) { readScalar(i); }

    /**
     * Reads an unsigned integer from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(unsigned int& ui) { readScalar(ui); }

    /**
     * Reads a signed 64-bit integer from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(int64_t& i) { readScalar(i); }

    /**
     * Reads an unsigned 64-bit integer from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(uint64_t& ui) { readScalar(ui); }

    /**
     * Reads a 32-bit float from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(float& f) { readScalar(f); }

    /**
     * Reads a 64-bit double from the file (endian safe). Throws an Exception if an error occurs.
     */
    void read(double& d) { readScalar(d); }

    /**
     * Reads and validates a DataType value from the file (endian safe). Throws an Exception if an error ocurs.
     */
    void read(DataType& dataType)
    {
        auto type = int();
        read(type);

        // For backwards compatibility the following values are also recognized as data types (these are the OpenGL data
        // type constants)
        if (type == 0x1400)
            type = TypeInt8;
        else if (type == 0x1401)
            type = TypeUInt8;
        else if (type == 0x1402)
            type = TypeInt16;
        else if (type == 0x1403)
            type = TypeUInt16;
        else if (type == 0x1404)
            type = TypeInt32;
        else if (type == 0x1405)
            type = TypeUInt32;
        else if (type == 0x1406)
            type = TypeFloat;
        else if (type == 0x140A)
            type = TypeDouble;

        if (type != TypeNone && type != TypeUInt8 && type != TypeInt16 && type != TypeUInt16 && type != TypeInt32 &&
            type != TypeUInt32 && type != TypeInt64 && type != TypeUInt64 && type != TypeFloat && type != TypeDouble)
            throw Exception("Invalid data type");

        dataType = DataType(type);
    }

    /**
     * Reads a `std::array` from the file.
     */
    template <typename T, size_t Size> void read(std::array<T, Size>& array)
    {
        for (auto& item : array)
            read(item);
    }

    /**
     * Reads from the file any class that defines a method in the form `load(FileReader& file)`. Throws an Exception if
     * an error occurs.
     */
    template <typename T> void read(T& t) { t.load(*this); }

    /**
     * This is a convenience method that allows two items of any type to be read from this file in one call.
     */
    template <typename FirstType, typename... OtherTypes> void read(FirstType& first, OtherTypes&&... others)
    {
        read(first);
        read(std::forward<OtherTypes>(others)...);
    }

    /**
     * Reads an enum value from the file as a signed integer. Throws an Exception if an error occurs.
     */
    template <typename T> void readEnum(T& t) { read(reinterpret_cast<int&>(t)); }

    /**
     * Reads an enum value from the file as a signed integer. \a enumSize is used to check that the read enumeration
     * value is valid and should be set to the lowest possible invalid value, any read values that are equal to or
     * greater than \a enumSize will cause an Exception to be thrown. Throws an Exception if an error occurs.
     */
    template <typename EnumType> void readEnum(EnumType& t, EnumType enumSize)
    {
        auto value = 0;

        read(value);
        t = EnumType(value);

        if (t >= enumSize)
            throw Exception("Read enum value is out of range");
    }

    /**
     * Reads the specified number of bytes, parses them as UTF8, and returns the resulting UnicodeString. Throws an
     * Exception if an error occurs.
     */
    UnicodeString readUTF8Data(unsigned int byteCount);

    /**
     * Reads a 32-bit FOURCC code from this file and returns it, this value can be compared to one returned by
     * FileSystem::makeFourCC() when verifying file headers. Throws an Exception if an error occurs.
     */
    unsigned int readFourCC()
    {
        auto fourCC = 0U;

        read(fourCC);

        return fourCC;
    }

    /**
     * Reads a Vector from the file. Throws an Exception if an error occurs.
     */
    template <typename T> void read(Vector<T>& vector)
    {
        auto size = 0U;
        read(size);

        try
        {
            vector.clear();

            try
            {
                vector.resize(size);
            }
            catch (const std::bad_alloc&)
            {
                throw Exception() << "Failed resizing vector to size " << size << ", memory allocation failed";
            }

            for (auto& item : vector)
                read(item);
        }
        catch (const Exception&)
        {
            vector.clear();
            throw;
        }
    }

    /**
     * Reads a vector of pointers and its contents from the file. Throws an Exception if an error occurs.
     */
    template <typename T> void readPointerVector(Vector<T*>& vector)
    {
        auto size = 0U;
        read(size);

        try
        {
            vector.clear();

            try
            {
                vector.resize(size, nullptr);
            }
            catch (const std::bad_alloc&)
            {
                throw Exception() << "Failed resizing vector to size " << size << ", memory allocation failed";
            }

            for (auto& item : vector)
            {
                item = new T;
                read(*item);
            }
        }
        catch (const Exception&)
        {
            for (auto item : vector)
                delete item;

            vector.clear();
            throw;
        }
    }

    /**
     * Reads all of this file's data as UTF-8 and puts it the result into \a data. Returns success flag.
     */
    bool getDataAsString(UnicodeString& string);

    /**
     * Reads all of this file's data as UTF-8 then converts it to ASCII and puts the result into \a data. Returns
     * success flag.
     */
    bool getDataAsString(String& string);

    /**
     * Reads all of this file's data as a string, splits it into lines, and returns the lines in \a lines. Returns
     * success flag.
     */
    template <typename StringType> bool getLines(Vector<StringType>& lines, bool keepEmptyLines = true)
    {
        auto s = StringType();
        if (!getDataAsString(s))
            return false;

        lines = s.getLines(keepEmptyLines);

        return true;
    }

    /**
     * Parses this file as UTF-8, splits it into lines, then divides each line into a Vector of tokens using
     * UnicodeString::getTokens() and puts the result into \a lineTokens. Empty lines and any '#' style comments are
     * ignored. Returns success flag.
     */
    bool getLineTokens(Vector<Vector<UnicodeString>>& lineTokens);

    /**
     * Identical to the Unicode version of this method but converts the read UTF-8 file to prior to returning the result
     * in \a lineTokens.
     */
    bool getLineTokens(Vector<Vector<String>>& lineTokens);

    /**
     * Begins reading a versioned section, these are the system by which features can be added to binary file formats
     * while maintaining a high level of both backward and forward compatibility. The return value indicates the major
     * and minor of the versioned section. There must be a corresponding call to FileReader::endVersionedSection(). This
     * method checks that the read major version is not greater than the major version in the passed \a versionInfo
     * parameter, other version checking must be done by the caller. Throws an Exception if an error occurs.
     */
    VersionInfo beginVersionedSection(const VersionInfo& versionInfo);

    /**
     * Ends reading the most recently begun versioned section started with FileReader::beginVersionedSection(). This
     * method makes sure that the file read position is put at the end of the versioned section, so any data in the
     * section that has not been read will be skipped over. Throws an Exception if an error occurs.
     */
    void endVersionedSection();

    /**
     * Searches the currently active versioned sections looking the most recent entry that used the specified \a
     * versionInfo and returns the same value as from its originating call to FileReader::beginVersionedSection().
     * Throws an Exception if the specified version info is not found.
     */
    VersionInfo findVersionedSection(const VersionInfo& versionInfo) const;

private:

    template <typename T> void readScalar(T& t)
    {
        readBytes(&t, sizeof(T));
#ifdef CARBON_BIG_ENDIAN
        Endian::convert(t);
#endif
    }

    class Members;
    Members* m = nullptr;
};

}
