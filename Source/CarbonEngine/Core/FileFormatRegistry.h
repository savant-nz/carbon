/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Globals.h"

namespace Carbon
{

/**
 * This class contains shared functionality for managing multiple different file formats that load into the same
 * resource type, for example image files such as PNG, JPEG and DDS all get loaded into the Image class. This class
 * handles format registration, format lookup, automatic filename extension determination, and all related FileSystem
 * interaction. Each supported format is identified by a unique file extension (e.g. bmp, png, or jpg for images). This
 * class is subclassed by classes that adapt it for specific use cases, see ImageFormatRegistry, SoundFormatRegistry and
 * MeshFormatRegistry for details. The CARBON_REGISTER_FILE_FORMAT() macro is used to automatically register a file
 * format on startup.
 */
template <typename ReadFileFormatFunction, typename WriteFileFormatFunction>
class CARBON_API FileFormatRegistry : private Noncopyable
{
public:

    /**
     * Registers reader and writer functions for the file format that uses the given extension. Both the \a reader and
     * \a writer parameters are allowed to be null. Usually the CARBON_REGISTER_FILE_FORMAT() macro is used instead of
     * calling this method directly.
     */
    static void registerFormat(const UnicodeString& extension, ReadFileFormatFunction reader,
                               WriteFileFormatFunction writer)
    {
        auto format = findFormat(extension);

        if (format)
        {
            if (reader)
                format->fnReader = reader;

            if (writer)
                format->fnWriter = writer;
        }
        else
            formats_.emplace(extension, reader, writer);
    }

    /**
     * Returns the reader function that is registered for the given extension, or null if one is not available.
     */
    static ReadFileFormatFunction getReaderForExtension(const UnicodeString& extension)
    {
        auto format = findFormat(extension);

        return format ? format->fnReader : nullptr;
    }

    /**
     * Returns the writer function that is registered for the given extension, or null if one is not available.
     */
    static WriteFileFormatFunction getWriterForExtension(const UnicodeString& extension)
    {
        auto format = findFormat(extension);

        return format ? format->fnWriter : nullptr;
    }

    /**
     * Takes a filename and returns the extension if one is present and is recognized as a known file format extension.
     * If no known file format extension is present then an empty string is returned.
     */
    static const UnicodeString& getSupportedExtension(const UnicodeString& filename)
    {
        auto lastPeriod = filename.findLastOf(UnicodeString::Period);

        if (lastPeriod > 0 && filename.at(lastPeriod - 1) != '/')
        {
            auto format = findFormat(filename.substr(lastPeriod + 1));
            if (format)
                return format->extension;
        }

        return UnicodeString::Empty;
    }

    /**
     * Returns the passed filename with any recognized file format extension stripped off it (if one is present).
     */
    static UnicodeString stripSupportedExtension(const UnicodeString& filename)
    {
        auto extension = getSupportedExtension(filename);

        if (extension.length())
            return filename.withoutSuffix(UnicodeString::Period + extension);

        return filename;
    }

    /**
     * If the passed filename ends with a supported file format extension then the return value is whether or not the
     * passed filename exists, but if no known file format extension is present on the passed filename then each of the
     * registered file format extensions are appended to the filename to see if a file of that format exists, and if one
     * is found then true is returned. If no matching filename can be found then false is returned.
     */
    static bool doesFileExist(const UnicodeString& filename)
    {
        if (!filename.length())
            return false;

        // Check if a recognized extension has been specified and use that format if possible
        auto extension = getSupportedExtension(filename);
        if (extension.length())
            return fileSystem().doesFileExist(filename);

        // Try and guess the extension by looking through all supported formats
        for (const auto& format : formats_)
        {
            if (format.fnReader && fileSystem().doesFileExist(filename + UnicodeString::Period + format.extension))
                return true;
        }

        return false;
    }

protected:

    /**
     * Takes a filename and tries to open the relevant file, if the name ends with a supported file extension then that
     * format will be assumed, otherwise the filename will be assumed to lack an extension and the filesystem will be
     * searched for possible matches based on all the registered file formats and their extensions. If this process
     * finds a file then the \a ReadFileFormatFunction for the matching format is returned and the opened file is
     * returned by the \a file parameter. Returns null on failure.
     */
    static ReadFileFormatFunction loadFile(const UnicodeString& filename, FileReader& file)
    {
        if (!filename.length())
            return nullptr;

        // Check if a recognized extension has been specified and use that format if possible
        auto extension = getSupportedExtension(filename);
        if (extension.length())
        {
            try
            {
                fileSystem().open(filename, file);
                return getReaderForExtension(extension);
            }
            catch (const Exception&)
            {
                return nullptr;
            }
        }

        // Determine the extension by searching the filesystem for possible matches
        for (const auto& format : formats_)
        {
            auto fullFilename = filename + UnicodeString::Period + format.extension;

            if (format.fnReader && fileSystem().doesFileExist(fullFilename))
            {
                try
                {
                    fileSystem().open(fullFilename, file);
                    return format.fnReader;
                }
                catch (const Exception&)
                {
                }
            }
        }

        return nullptr;
    }

    /**
     * If the passed filename ends with an extension that has an associated writer function then this method will open
     * the file for writing and return the relevant WriteFileFormatFunction for the format. Returns null on failure.
     */
    static WriteFileFormatFunction saveFile(const UnicodeString& filename, FileWriter& file)
    {
        // Get the extension
        auto extension = getSupportedExtension(filename);
        if (!extension.length())
        {
            LOG_ERROR << "No known file format extension recognized on filename '" << filename << "'";
            return nullptr;
        }

        // Get the writer function for this file format
        auto fnWriter = getWriterForExtension(extension);
        if (!fnWriter)
        {
            LOG_ERROR << "Writing to the " << extension << " file format is not supported";
            return nullptr;
        }

        // Open the output file
        try
        {
            fileSystem().open(filename, file);
        }
        catch (const Exception&)
        {
            return nullptr;
        }

        return fnWriter;
    }

private:

    struct SupportedFileFormat
    {
        UnicodeString extension;
        ReadFileFormatFunction fnReader = nullptr;
        WriteFileFormatFunction fnWriter = nullptr;

        SupportedFileFormat() {}
        SupportedFileFormat(UnicodeString extension_, ReadFileFormatFunction fnReader_,
                            WriteFileFormatFunction fnWriter_)
            : extension(std::move(extension_)), fnReader(fnReader_), fnWriter(fnWriter_)
        {
        }
    };

    static Vector<SupportedFileFormat> formats_;

    static SupportedFileFormat* findFormat(const UnicodeString& extension)
    {
        for (auto& format : formats_)
        {
            if (format.extension == extension)
                return &format;
        }

        return nullptr;
    }
};

/**
 * \file
 */

/**
 * Declares a FileFormatRegistry by declaring the static \a formats_ member that it requires.
 */
#define CARBON_DECLARE_FILE_FORMAT_REGISTRY(ReadFileFormatFunction, WriteFileFormatFunction) \
    template <> Vector<FileFormatRegistry<ReadFileFormatFunction, WriteFileFormatFunction>::SupportedFileFormat> FileFormatRegistry<ReadFileFormatFunction, WriteFileFormatFunction>::formats_

/**
 * Defines a FileFormatRegistry by instantiating the static \a formats_ member that it requires.
 */
#define CARBON_DEFINE_FILE_FORMAT_REGISTRY(ReadFileFormatFunction, WriteFileFormatFunction) \
    typedef FileFormatRegistry<ReadFileFormatFunction, WriteFileFormatFunction> Registry;   \
    template <> Vector<Registry::SupportedFileFormat> Registry::formats_ = Vector<Registry::SupportedFileFormat>();

/**
 * Registers reading and writing functions for the given extension with the specified file format registry, both \a
 * ReaderFunction and \a WriterFunction are allowed to be null. Usually one of CARBON_REGISTER_IMAGE_FILE_FORMAT(),
 * CARBON_REGISTER_SOUND_FILE_FORMAT() or CARBON_REGISTER_MESH_FILE_FORMAT() is used instead of using this macro
 * directly.
 */
#define CARBON_REGISTER_FILE_FORMAT(Registry, Extension, ReaderFunction, WriterFunction)                           \
    CARBON_UNIQUE_NAMESPACE                                                                                        \
    {                                                                                                              \
        static void registerFileFormat() { Registry::registerFormat(#Extension, ReaderFunction, WriterFunction); } \
        CARBON_REGISTER_STARTUP_FUNCTION(registerFileFormat, 0)                                                    \
    }                                                                                                              \
    CARBON_UNIQUE_NAMESPACE_END
}
