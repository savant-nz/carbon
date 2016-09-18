/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileSystem/SimpleFileSystemVolume.h"
#include "CarbonEngine/Core/Threads/Thread.h"

namespace Carbon
{

/**
 * This class provides an interface to the virtual filesystem in which all files and assets are stored. It allows
 * multiple paths on the local file system to be used to load assets from, and these are combined with other custom
 * file system volumes into one final virtual file system. See the FileSystemVolume class for further details on how an
 * individual file system volume is specified, there are several provided subclasses of FileSystemVolume that are used
 * internally, including LocalDirectoryFileSystemVolume and SimpleFileSystemVolume. The default search paths on the
 * local file system are "./Assets" and "./", both derived from the application's working directory at startup, and
 * these are both are set by the default implementation of Application::setupAssetDirectories().
 *
 * When file system volumes are mounted into the virtual file system they can be put at a specific location using the \a
 * mountLocation parameter of the FileSystem::addVolume() method. The contents of a file system volume can also be
 * accessed directly using a filename in the format "$<volume name>$/<filename>" when calling a method such as
 * FileSystem::open(). If no volume name surrounded by $ characters is present at the start of a filename then it will
 * be treated as a path on the virtual file system and searched for in the registered asset directories and file system
 * volumes.
 *
 * If a filename passed to a method path is prepended with "$LOCAL$" then the rest of the filename is treated as a
 * absolute path on the local file system, e.g. "$LOCAL$C:/Windows/notepad.exe".
 *
 * There is a special file system volume called SAVE which should be used to store any data the client application
 * wishes to save across executions, e.g. saved games. This volume can be accessed using the "$SAVE$/" prefix on
 * filenames, the same way as for any file system volume. The $SAVE$ file system volume will always map to a proper
 * location for the storage of this kind of persistent per-user data on the active platform. The engine will never alter
 * the contents of $SAVE$, and the application is expected to manage any files and folders that it chooses to create
 * inside the volume. On Windows $SAVE$/ maps to "%APPDATA%/<application name>/Save", on Mac OS X $SAVE$/ maps to
 * "~/Library/Preferences/<application name>/Save", and on Linux $SAVE$/ maps to "~/.<application name>/Save".
 *
 * File system access is thread-safe, and there is support for asynchromous loading of files through
 * FileSystem::openAsync().
 */
class CARBON_API FileSystem : private Noncopyable
{
public:

    /**
     * The ID byte written before each versioned section in a file. See FileWriter::beginVersionedSection() for details.
     */
    static const byte_t SectionBeginID = 0xAF;

    /**
     * The ID byte written before after each versioned section in a file. See FileWriter::beginVersionedSection() for
     * details.
     */
    static const byte_t SectionEndID = 0xFA;

    /**
     * The prefix to use on a filename passed to a method on the FileSystem class in order to force the rest of the
     * filename to be treated as a path on the local file system with no further processing, currently this prefix is
     * "$LOCAL$".
     */
    static const UnicodeString LocalFilePrefix;

    /**
     * Opens the file with the specified name for reading. This method searches all the asset directories and volumes
     * for the requested file and opens the first valid match that it finds. If \a filename specifies a known file
     * system volume using the "$<volume name>$/<filename>" format then only the specified volume will be searched in to
     * find the file. If \a filename starts with "$LOCAL$" then the rest of the filename will be treated as a path on
     * the local file system. Throws an Exception if an error occurs, detailed error information is available using
     * FileReader::getLastError(). If an error occurs then FileSystemErrorEvent will also be sent.
     */
    void open(const UnicodeString& filename, FileReader& file);

    /**
     * Opens the file with the specified name for writing. If \a filename specifies a known file system volume using the
     * "$<volume name>$/<filename>" format then the file will be saved in the specified volume (assuming the volume
     * supports the writing of files). If the path portion of \a filename is inside the mount location of a file system
     * volume then the file will be saved into the corresponding volume (assuming the volume supports the writing of
     * files). If \a filename starts with "$LOCAL$" then the rest of the filename will be treated as a path on the local
     * file system. If none of the preceding criteria are met then this method will save the file into the first asset
     * directory on the local file system (which will typically be "Assets/" under the application's initial working
     * directory). If the given file already exists then this method will erase and overwrite it. Throws an Exception if
     * an error occurs, detailed error information is available using FileWriter::getLastError(). If an error occurs
     * then FileSystemErrorEvent will also be sent.
     */
    void open(const UnicodeString& filename, FileWriter& file, bool asText = false);

    /**
     * Identifier used for an asynchronous file load, see FileSystem::openAsync() for details.
     */
    typedef void* AsynchronousLoadID;

    /**
     * Opens the file with the specified name for reading, this is different from FileSystem::open() in that the load
     * and file read occurs on a worker thread. The result of the file load can be queried using
     * FileSystem::getOpenAsyncResult(). The return value uniquely identifies this asynchronous load and should be
     * passed to FileSystem::getOpenAsyncResult() to monitor the progress of the asynchronous load thread. Asynchronous
     * loads can be initiated from any thread.
     */
    AsynchronousLoadID openAsync(const UnicodeString& filename);

    /**
     * The possible asynchronous file load states that can be returned by FileSystem::getOpenAsyncResult()
     */
    enum AsynchronousLoadState
    {
        /**
         * Indicates that the passed \a loadID value was invalid.
         */
        AsynchronousLoadInvalidID,

        /**
         * Indicates that the file load thread for the asynchronous load is still processing the load, the caller should
         * allow some time for the thread to continue processing and then recheck the result again.
         */
        AsynchronousLoadIncomplete,

        /**
         * Indicates that the asynchronous load failed, details regarding the failure are sent as a
         * FileSystemErrorEvent.
         */
        AsynchronousLoadFailed,

        /**
         * Indicates that the asynchronous load succeeded, the data from the file load is returned in the \a file
         * parameter passed to FileSytem::getOpenAsyncResult().
         */
        AsynchronousLoadSucceeded
    };

    /**
     * Queries the result of an asynchronous load, \a loadID must be the return value from a call to
     * FileSystem::openAsync(). The return value of FileSystem::getOpenAsyncResult() indicates the state of the
     * asynchronous load, if either of AsynchronousLoadFailed or AsynchronousLoadSucceeded are returned then the passed
     * \a loadID is cleaned up internally and so is no longer valid and should be discarded. If the return value is
     * AsynchronousLoadSucceeded then the result of the file load will be returned in the \a file parameter. If \a wait
     * is set to true and the asynchronous load is still incomplete then the calling thread will block until the load
     * thread completes. The return value will never be AsynchronousLoadIncomplete when \a wait is set to true, all
     * other return values are still possible.
     */
    AsynchronousLoadState getOpenAsyncResult(AsynchronousLoadID loadID, FileReader& file, bool wait = false);

    /**
     * Returns whether the a file with the specified name exists on the file system. This method searches all asset
     * directories and file system volumes for the requested file. If \a filename specifies a file system volume using
     * the "$<volume name>$/<filename>" format then only the specified volume will be searched. If \a filename starts
     * with "$LOCAL$" then the rest of the filename will be treated as a path on the local file system to check for.
     */
    bool doesFileExist(const UnicodeString& filename) const;

    /**
     * Deletes the specified file. If \a filename specifies a file system volume using the "$<volume name>$/<filename>"
     * format then the file will be deleted from the specified volume (assuming the volume supports the deleting of
     * files) and no asset directories or other volumes will be checked. If \a filename starts with "$LOCAL$" then the
     * rest of the filename will be treated as a file on the local file system to delete. Returns success flag. If an
     * error occurs then FileSystemErrorEvent will also be sent.
     */
    FileSystemError deleteFile(const UnicodeString& filename);

    /**
     * Adds a file system volume into the virtual file system. If \a mountLocation is specified then the contents of the
     * volume will be located in that position in the virtual file system, which means in order to access the files in
     * the volume using methods such as FileSystem::open() the passed filename must start with the mount location. The
     * mount location must begin with a forward slash, and it defaults to "/". Mount locations must not overlap, i.e.
     * mount location is allowed to lie inside another mount location. Mount locations must also be unique with the
     * exception that multiple volumes can be mounted at the root "/" location. The volume's name must be unique.
     * Returns success flag.
     */
    bool addVolume(FileSystemVolume* volume, const UnicodeString& mountLocation = "/");

    /**
     * Removes a custom file system volume from the main virtual file system. Returns success flag.
     */
    bool removeVolume(FileSystemVolume* volume);

    /**
     * Returns the names of all the file system volumes that have been attached with FileSystem::addVolume().
     */
    Vector<UnicodeString> getVolumeNames() const;

    /**
     * Returns the FileSystemVolume with the given name, or null if there is no volume with that name.
     */
    const FileSystemVolume* getVolume(const UnicodeString& name) const;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    /**
     * Given a filename to open this looks in all registered asset directories on the local file system for a matching
     * file, and if one is found then the absolute path is returned. If no such file is found then an empty string is
     * returned.
     */
    UnicodeString getPathToLocalFile(const UnicodeString& filename) const;

    /**
     * Returns whether or not the given file exists on the local file system.
     */
    static bool doesLocalFileExist(const UnicodeString& filename);

    /**
     * Returns whether or not the given directory exists on the local file system.
     */
    static bool doesLocalDirectoryExist(const UnicodeString& directory);

    /**
     * Deletes the specified file from the local file system. Returns success flag.
     */
    static bool deleteLocalFile(const UnicodeString& filename);

    /**
     * Returns the working directory that was current when the engine was initialized, any relative paths will be
     * relative to this directory.
     */
    const UnicodeString& getInitialWorkingDirectory() const;

    /**
     * Returns the current working directory. It is usually better to use FileSystem::getInitialWorkingDirectory()
     * rather than this method.
     */
    static UnicodeString getCurrentWorkingDirectory();

    /**
     * Takes a directory on the local file system to search in when opening files and maps it to the root of the virtual
     * file system. Internally this method adds a LocalDirectoryFileSystemVolume instance to the virtual file system.
     * The passed directory can be a relative or absolute path. Returns success flag.
     */
    bool addLocalAssetDirectory(const UnicodeString& directory);

    /**
     * Removes the specified directory on the local file system from the list of asset directories to search in. See
     * FileSystem::addLocalAssetDirectory() for details. Returns success flag.
     */
    bool removeLocalAssetDirectory(const UnicodeString& directory);

    /**
     * Clears the all directories added with FileSystem::addLocalAssetDirectory().
     */
    void clearLocalAssetDirectories();

    /**
     * Returns all the files in a given directory on the local file system, all returned filenames will be absolute
     * paths on the local file system. Note that this method only searches under a single directory on the local file
     * system, to search the entire virtual file system use FileSystem::enumerateFiles(). A required file extension can
     * be given, in which case only files ending with that extension will be returned. All matching files are returned
     * in \a files. If the \a recursive argument is true then the search will proceed through all subdirectories of the
     * specified local directory.
     */
    static void enumerateLocalFiles(const UnicodeString& directory, const UnicodeString& extension, bool recursive,
                                    Vector<UnicodeString>& files);

    /**
     * Returns a list of all the directories in a given directory on the local file system. The directory names are
     * returned in \a directories. If \a recursive is true then the enuemration will also proceed through all
     * subdirectories.
     */
    static void enumerateLocalDirectories(const UnicodeString& directory, bool recursive,
                                          Vector<UnicodeString>& directories);

    /**
     * On case-insensitive platforms this returns the correct case for the specified file or directory as stored on the
     * local file system. On Mac OS X and iOS this method also expands any symlinks in the path. On Linux this method
     * returns \a path unchanged.
     */
    static UnicodeString getCanonicalPath(const UnicodeString& path);

#ifdef POSIX
    /**
     * On Linux and Mac OS X this returns the current user's home directory as an absolute path.
     */
    static UnicodeString getHomeDirectory();
#endif

    /**
     * Returns the local directory to use for persistent data such as settings files and savedata. Applications should
     * not use this method directly but instead use the $SAVE$ file system volume in order to get maximum portability.
     */
    static UnicodeString getUserDataLocalDirectory();

#ifdef APPLE
    /**
     * On Apple platforms this returns the absolute path to the current user's `Library` directory.
     */
    static UnicodeString getUserLibraryDirectory();

    /**
     * On Apple platforms this returns the absolute path to the current application's `Resources` directory.
     */
    static UnicodeString getApplicationResourcesDirectory();
#endif

#ifdef WINDOWS
    /**
     * On Windows, if the SDK is installed then this returns the install path (which is usually '%ProgramFiles%/Carbon
     * SDK/'). The path will use forward slashes and will contain the trailing forward slash at the end. If the SDK is
     * not installed then an empty string is returned.
     */
    static UnicodeString getSDKInstallDirectory();

    /**
     * On Windows, adds the Samples/Assets/ directory in the SDK install directory as an asset directory. Returns
     * success flag.
     */
    bool addSDKSampleAssetsDirectory();
#endif

#endif

    /**
     * Returns a list of all the files in the given directory on the virtual file system. If \a directory specifies a
     * file system volume using the "$<volume name>$/<directory>/" format then the only the files in the specified
     * volume will be enumerated, otherwise this method will look in all asset directories and file system volumes. A
     * required file extension can be given, in which case only files ending with that extension will be returned. All
     * matching files are returned in \a files. If the \a recursive argument is true then the search will proceed
     * through all subdirectories of the given directory. Specifying a directory of "/" will search the entire virtual
     * file system including all asset directories and file system volumes. Returns success flag. If an error occurs
     * then FileSystemErrorEvent will also be sent.
     */
    bool enumerateFiles(UnicodeString directory, const UnicodeString& extension, bool recursive,
                        Vector<UnicodeString>& files);

    /**
     * Adds a single file that will then be accessible to the entire application on the file system. The filename can
     * contain a directory structure if desired. Returns success flag.
     */
    bool addVirtualFile(const UnicodeString& filename, const byte_t* data, unsigned int size);

    /**
     * \copydoc addVirtualFile(const UnicodeString &, const byte_t *, unsigned int)
     */
    bool addVirtualFile(const UnicodeString& filename, const Vector<byte_t>& data)
    {
        return addVirtualFile(filename, data.getData(), data.size());
    }

    /**
     * \copydoc addVirtualFile(const UnicodeString &, const byte_t *, unsigned int)
     */
    bool addVirtualFile(const UnicodeString& filename, const String& data)
    {
        return addVirtualFile(filename, reinterpret_cast<const byte_t*>(data.cStr()), data.length());
    }

    /**
     * Takes a four character string and turns it into a 32-bit FOURCC code.
     */
    static unsigned int makeFourCC(const char* code);

    /**
     * Takes a byte size and returns a formatted displayable string for it. For example, an input of 16000 would return
     * "15.62KB".
     */
    static String formatByteSize(uint64_t size);

    /**
     * Returns the directory portion of the given file path, i.e. everything up to the last forward or back slash.
     */
    static UnicodeString getDirectory(const UnicodeString& fullPath);

    /**
     * Returns the final name portion of the given file or directory, i.e. everything after the last directory
     * separator.
     */
    static UnicodeString getBaseName(const UnicodeString& fullPath);

    /**
     * Takes a path to a resource (e.g. a material) and returns just the resource name, e.g. if
     * "Materials/GUI/Test.material" is passed then the return value will be "GUI/Test".
     */
    static UnicodeString getResourceName(const UnicodeString& name, const UnicodeString& resourceDirectory,
                                         const UnicodeString& resourceExtension)
    {
        UnicodeString resource = name.withoutSuffix(resourceExtension);

        int index = resource.find(resourceDirectory);
        if (index != -1)
            resource = resource.substr(index + resourceDirectory.length());

        return resource;
    }

    /**
     * Takes a path to a resource (e.g. a material) and sandwiches it between the passed \a resourceDirectory and \a
     * resourceExtension. If the passed name starts with a '$' character then it is assumed to be a fully specified path
     * and so is returned unchanged.
     */
    static UnicodeString getResourceFilename(const UnicodeString& name, const UnicodeString& resourceDirectory,
                                             const UnicodeString& resourceExtension)
    {
        UnicodeString filename = name;

        if (!filename.startsWith("$"))
            filename = joinPaths(resourceDirectory, filename) + resourceExtension;

        return filename;
    }

    /**
     * Concatenates the two path strings and ensures there is exactly one forward slash between them in the resulting
     * string.
     */
    static UnicodeString joinPaths(const UnicodeString& path1, const UnicodeString& path2);

    /**
     * Returns the current date and time in the format: "<day name>, <dd> <month> <yyyy> at <h>:<mm>[am|pm]".
     */
    static String getDateTime();

    /**
     * This is the same as FileSystem::getDateTime() except that it does no internal allocations and instead puts its
     * output into the provided buffer. Returns success flag.
     */
    static bool getDateTime(char* buffer, unsigned int bufferSize);

    /**
     * Returns the current date and time in the format: "<yyyy>/<mm>/<dd> <hh>:<mm>:<ss>".
     */
    static String getShortDateTime();

    /**
     * This is the same as FileSystem::getShortDateTime() except that it does no internal allocations and instead puts
     * its output into the provided buffer. Returns success flag.
     */
    static bool getShortDateTime(char* buffer, unsigned int bufferSize);

    /**
     * Returns a date/time string formatted according to the given format string. The format string has the same
     * structure as that provided to the \a strftime() function.
     */
    static String getFormattedDateTime(const String& format);

    /**
     * This is the same as FileSystem::getFormattedDateTimeString() except that it does no internal allocations and
     * instead puts its output into the provided buffer. Returns success flag.
     */
    static bool getFormattedDateTime(const char* format, char* buffer, unsigned int bufferSize);

    /**
     * Returns the contents of the specified ASCII text file in \a string. Returns success flag.
     */
    bool readTextFile(const UnicodeString& filename, String& string);

    /**
     * Returns the contents of the specified UTF-8 text file in \a string. Returns success flag.
     */
    bool readTextFile(const UnicodeString& filename, UnicodeString& string);

    /**
     * Returns the contents of the specified ASCII text file in the \a lineTokens vector, where each line in the input
     * file is split into tokens using the String::getTokens() method. Empty lines and '#'-style comments such as those
     * used by Python and Ruby are ignored. Returns success flag.
     */
    bool readTextFile(const UnicodeString& filename, Vector<Vector<String>>& lineTokens);

    /**
     * Parses the contents of the specified ASCII text file as a list of "key = value" lines, and returns the result in
     * \a parameters. Empty lines and '#'-style comments such as those used by Python and Ruby are ignored. Returns
     * success flag.
     */
    bool readTextFile(const UnicodeString& filename, ParameterArray& parameters);

    /**
     * Writes the specified ParameterArray to an ASCII text file with one parameter on each line in the format "key =
     * value". Returns success flag.
     */
    bool writeTextFile(const UnicodeString& filename, const ParameterArray& parameters);

    /**
     * Converts a FileSystemError enumeration value to the equivalent human readable string.
     */
    static String errorToString(FileSystemError error);

    /**
     * Returns whether or not the passed name is a valid file name.
     */
    static bool isValidFileName(const UnicodeString& filename)
    {
        return filename.length() && filename.findFirstOf(InvalidCharacters) == -1 && !filename.endsWith("/");
    }

    /**
     * Returns whether or not the passed name is a valid directory name, directory names always end with a forward
     * slash.
     */
    static bool isValidDirectoryName(const UnicodeString& filename)
    {
        return filename.length() && filename.findFirstOf(InvalidCharacters) == -1 && filename.endsWith("/");
    }

private:

    FileSystem();
    ~FileSystem();
    friend class Globals;

    static const UnicodeString InvalidCharacters;

    mutable Mutex mutex_;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    UnicodeString initialWorkingDirectory_;
    Vector<LocalDirectoryFileSystemVolume*> assetDirectoryVolumes_;
    // The $SAVE$ volume, this is only for platforms that support local filesystem access, other platforms need to add a
    // $SAVE$ volume themselves
    LocalDirectoryFileSystemVolume* saveVolume_ = nullptr;
    void createSaveVolume();
#endif

    Vector<FileSystemVolume*> volumes_;

    static UnicodeString getVolumeNamePrefix(const UnicodeString& volumeName);
    static UnicodeString stripVolumeName(const UnicodeString& filename, const FileSystemVolume* volume);
    FileSystemVolume* getVolumeSpecifiedByFilename(const UnicodeString& filename);
    const FileSystemVolume* getVolumeSpecifiedByFilename(const UnicodeString& filename) const;

    SimpleFileSystemVolume builtInVolume_;

    // This is a simple Thread subclass that is used to load a single file asynchronously, it is used by
    // FileSystem::openAsync()
    class FileLoadThread : public Thread
    {
    public:

        const UnicodeString filename;
        FileReader file;

        bool succeeded = false;

        FileLoadThread(UnicodeString filename_) : Thread("FileLoadThread"), filename(std::move(filename_)) {}

        void main() override;
    };
    Vector<FileLoadThread*> fileLoadThreads_;
};

/**
 * This is a helper macro that adds a simple text file to the virtual file system on startup using
 * FileSystem::addVirtualFile(). \a Filename must be the path to the file and can include a directory hierarchy if
 * desired. \a Content must be a string that contains the desired contents of the virtual file that will be created.
 */
#define CARBON_CREATE_VIRTUAL_FILE(Filename, Content)                                               \
    CARBON_UNIQUE_NAMESPACE                                                                         \
    {                                                                                               \
        static void createVirtualFile()                                                             \
        {                                                                                           \
            Carbon::fileSystem().addVirtualFile(Filename, reinterpret_cast<const byte_t*>(Content), \
                                                uint(strlen(Content)));                             \
        }                                                                                           \
        CARBON_REGISTER_STARTUP_FUNCTION(createVirtualFile, 0)                                      \
    }                                                                                               \
    CARBON_UNIQUE_NAMESPACE_END
}
