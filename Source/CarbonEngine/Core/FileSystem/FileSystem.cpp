/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/FileSystem/FileSystemVolume.h"
#include "CarbonEngine/Core/FileSystem/LocalDirectoryFileSystemVolume.h"
#include "CarbonEngine/Core/FileSystem/SimpleFileSystemVolume.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Core/Threads/Mutex.h"
#include "CarbonEngine/Core/Threads/Thread.h"
#include "CarbonEngine/Globals.h"

namespace Carbon
{

const UnicodeString FileSystem::InvalidCharacters = "\\*?\"<>|";
const UnicodeString FileSystem::LocalFilePrefix = "$LOCAL$";

FileSystem::FileSystem() : builtInVolume_(".BuiltIn")
{
    // Add the built-in simple file system volume that is used to implement FileSystem::addVirtualFile()
    addVolume(&builtInVolume_);

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    initialWorkingDirectory_ = getCurrentWorkingDirectory();
    LOG_INFO << "Working directory: " << initialWorkingDirectory_;

    // Add the $SAVE$ file system volume, platforms that don't support local file system access are responsible for
    // providing their own SAVE volume
    createSaveVolume();
#endif
}

FileSystem::~FileSystem()
{
    // Clean up any orphaned file load threads
    while (true)
    {
        auto thread = pointer_to<FileLoadThread>::type();

        // Check if there are any more file load threads to wait on
        {
            auto lock = ScopedMutexLock(mutex_);
            if (fileLoadThreads_.empty())
                break;

            thread = fileLoadThreads_.popBack();
            LOG_INFO << "Cleaning up unused asynchronous load of file '" << thread->filename << "'";
        }

        // Wait for the file load thread to complete, we are not holding the file system mutex so this is safe
        if (thread->isRunning())
            thread->wait();

        // Clean up the thread
        delete thread;
    }

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    removeVolume(saveVolume_);
    delete saveVolume_;
    saveVolume_ = nullptr;

    clearLocalAssetDirectories();
#endif
}

void FileSystem::FileLoadThread::main()
{
    try
    {
        // A small variable-length sleep at the start of a file load thread helps in situations where a number of file
        // load threads are spawned as a group
        sleep(Math::random(2, 10));

        fileSystem().open(filename, file);

        succeeded = true;
    }
    catch (const Exception&)
    {
        succeeded = false;
    }
}

void FileSystem::open(const UnicodeString& filename, FileReader& file)
{
    auto lock = ScopedMutexLock(mutex_);

    file.setName(filename);
    file.setLastError(NoFileSystemError);

    if (!isValidFileName(filename))
    {
        file.setLastError(InvalidResourceNameFileSystemError);
        throw Exception() << "Invalid filename: " << filename;
    }

    if (filename.startsWith(LocalFilePrefix))
    {
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
        if (!file.openLocalFile(filename.withoutPrefix(LocalFilePrefix)))
            throw Exception("Failed opening file");

        return;
#else
        throw Exception("Local file system access is not included in this build");
#endif
    }

    // Check whether a file system volume is specified in the filename
    auto specifiedVolume = getVolumeSpecifiedByFilename(filename);
    if (specifiedVolume)
    {
        file.setLastError(specifiedVolume->open(stripVolumeName(filename, specifiedVolume), file));
        if (file.getLastError() != NoFileSystemError)
            throw Exception("Failed opening file");

        return;
    }

    // Search all file system volumes
    auto virtualFilename = joinPaths("/", filename);
    for (auto volume : volumes_)
    {
        if (virtualFilename.startsWith(volume->getMountLocation()))
        {
            auto error = volume->open(virtualFilename.withoutPrefix(volume->getMountLocation()), file);
            if (error == NoFileSystemError)
                return;

            // Certain errors need to be reported back even if the attempt to open failed as the application may need to
            // know about them
            if (error == OutOfMemoryFileSystemError || error == InvalidOperationFileSystemError ||
                error == InvalidDataFileSystemError || error == IncompleteFileSystemError ||
                error == DataCorruptionFileSystemError || error == HardwareFailureFileSystemError ||
                error == UnknownFileSystemError)
                file.setLastError(error);
        }
    }

    file.setLastError(ResourceMissingFileSystemError);

    throw Exception("Failed opening file");
}

FileSystem::AsynchronousLoadID FileSystem::openAsync(const UnicodeString& filename)
{
    auto lock = ScopedMutexLock(mutex_);

    auto thread = new FileLoadThread(filename);

    // File load threads are run a little below the default thread priority level
    thread->run(0.25f);

    fileLoadThreads_.append(thread);

    return thread;
}

FileSystem::AsynchronousLoadState FileSystem::getOpenAsyncResult(AsynchronousLoadID loadID, FileReader& file, bool wait)
{
    auto lock = ScopedMutexLock(mutex_);

    auto thread = reinterpret_cast<FileLoadThread*>(loadID);

    // Check the load ID is valid
    if (!fileLoadThreads_.has(thread))
        return AsynchronousLoadInvalidID;

    // If the thread is still running then the load is incomplete
    if (thread->isRunning())
    {
        if (wait)
        {
            // Wait for this file load thread to complete. To avoid deadlock it is important not to hold the file system
            // mutex while waiting, as the file load thread is likely to need it.
            lock.release();
            thread->wait();
            lock.acquire();
        }
        else
            return AsynchronousLoadIncomplete;
    }

    AsynchronousLoadState result;

    if (thread->succeeded)
    {
        // The load was successful, return the file data in the passed file parameter

        swap(file, thread->file);
        result = AsynchronousLoadSucceeded;
    }
    else
        result = AsynchronousLoadFailed;

    // Clean up the file load thread
    delete thread;
    fileLoadThreads_.eraseValue(thread);

    return result;
}

void FileSystem::open(const UnicodeString& filename, FileWriter& file, bool asText)
{
    auto lock = ScopedMutexLock(mutex_);

    file.setName(filename);
    file.setLastError(NoFileSystemError);

    if (!isValidFileName(filename))
    {
        file.setLastError(InvalidResourceNameFileSystemError);
        throw Exception() << "Invalid filename: " << filename;
    }

    if (filename.startsWith(LocalFilePrefix))
    {
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
        if (!file.openLocalFile(filename.withoutPrefix(LocalFilePrefix), asText))
            throw Exception("Failed opening file");

        return;
#else
        throw Exception("Local file system access is not included in this build");
#endif
    }

    // Check whether a file system volume is specified in the filename
    auto specifiedVolume = getVolumeSpecifiedByFilename(filename);
    if (specifiedVolume)
    {
        file.setLastError(specifiedVolume->open(stripVolumeName(filename, specifiedVolume), file, asText));
        if (file.getLastError() != NoFileSystemError)
            throw Exception("Failed opening file");

        return;
    }

    auto virtualFilename = joinPaths("/", filename);

    // See if the file is under the mount location of a volume and if so then try and save it into that volume, volumes
    // mounted under "/" are skipped
    for (auto volume : volumes_)
    {
        if (volume->getMountLocation() != "/" && virtualFilename.startsWith(volume->getMountLocation()))
        {
            auto name = virtualFilename;
            name.removePrefix(volume->getMountLocation());

            if (volume->open(name, file, asText) == NoFileSystemError)
                return;
        }
    }

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    if (assetDirectoryVolumes_.empty())
    {
        LOG_ERROR << "There is nowhere to save this file: " << filename;
        file.setLastError(InvalidOperationFileSystemError);
    }
    else if (assetDirectoryVolumes_[0]->open(filename, file, asText) == NoFileSystemError)
        return;
#endif

    throw Exception("Failed opening file");
}

bool FileSystem::doesFileExist(const UnicodeString& filename) const
{
    auto lock = ScopedMutexLock(mutex_);

    if (!isValidFileName(filename))
        return false;

    if (filename.startsWith(LocalFilePrefix))
    {
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
        return getPathToLocalFile(filename.withoutPrefix(LocalFilePrefix)).length() != 0;
#else
        return false;
#endif
    }

    // Check whether a file system volume is specified in the filename
    auto specifiedVolume = getVolumeSpecifiedByFilename(filename);
    if (specifiedVolume)
        return specifiedVolume->doesFileExist(stripVolumeName(filename, specifiedVolume));

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    // Look in asset directories
    if (getPathToLocalFile(filename).length())
        return true;
#endif

    // Search file system volumes
    auto virtualFilename = joinPaths("/", filename);
    for (auto volume : volumes_)
    {
        if (virtualFilename.startsWith(volume->getMountLocation()))
        {
            if (volume->doesFileExist(virtualFilename.withoutPrefix(volume->getMountLocation())))
                return true;
        }
    }

    return false;
}

FileSystemError FileSystem::deleteFile(const UnicodeString& filename)
{
    try
    {
        auto lock = ScopedMutexLock(mutex_);

        if (filename.length() == 0)
            throw InvalidResourceNameFileSystemError;

        if (!isValidFileName(filename))
            throw InvalidResourceNameFileSystemError;

        if (filename.startsWith(LocalFilePrefix))
        {
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
            if (deleteLocalFile(filename.withoutPrefix(LocalFilePrefix)))
                throw NoFileSystemError;
#else
            throw NotSupportedFileSystemError;
#endif
        }

        // Check whether a file system volume is specified in the filename
        auto specifiedVolume = getVolumeSpecifiedByFilename(filename);
        if (specifiedVolume)
            throw specifiedVolume->deleteFile(stripVolumeName(filename, specifiedVolume));

        // Try to delete the file from a file system volume
        auto virtualFilename = joinPaths("/", filename);
        for (auto volume : volumes_)
        {
            if (virtualFilename.startsWith(volume->getMountLocation()))
            {
                auto error = volume->deleteFile(virtualFilename.withoutPrefix(volume->getMountLocation()));
                if (error == NoFileSystemError)
                    throw error;

                // Not supported and resource missing errors are fine here, other ones need to be reported back as a
                // real error
                if (error != NotSupportedFileSystemError && error != ResourceMissingFileSystemError)
                    throw error;
            }
        }

        return ResourceMissingFileSystemError;
    }
    catch (FileSystemError error)
    {
        if (error != NoFileSystemError)
            events().queueEvent(new FileSystemErrorEvent(error, filename));

        return error;
    }
}

bool FileSystem::addVolume(FileSystemVolume* volume, const UnicodeString& mountLocation)
{
    auto lock = ScopedMutexLock(mutex_);

    if (!volume)
        return false;

    // Check the mount location is valid
    if (!mountLocation.startsWith("/") || !isValidDirectoryName(mountLocation))
    {
        LOG_ERROR << "Mount location for volume '" << volume->getName() << "' is invalid: " << mountLocation;
        return false;
    }

    // Check the volume name is unique
    if (getVolume(volume->getName()))
    {
        LOG_WARNING << "Volume name '" << volume->getName() << "' is already in use";
        return true;
    }

    // Check the given mount location doesn't overlap with any existing volumes
    if (mountLocation != "/")
    {
        for (auto existingVolume : volumes_)
        {
            if (existingVolume->getMountLocation() != "/" &&
                (mountLocation.startsWith(existingVolume->getMountLocation()) ||
                 existingVolume->getMountLocation().startsWith(mountLocation)))
            {
                LOG_ERROR << "The mount location '" << mountLocation
                          << "' overlaps with the mount location of existing volume '" << existingVolume->getName()
                          << "'";

                return false;
            }
        }
    }

    volume->mountLocation_ = mountLocation;
    volumes_.append(volume);

    return true;
}

bool FileSystem::removeVolume(FileSystemVolume* volume)
{
    auto lock = ScopedMutexLock(mutex_);

    if (volume && volumes_.eraseValue(volume))
    {
        volume->mountLocation_.clear();
        return true;
    }

    return false;
}

Vector<UnicodeString> FileSystem::getVolumeNames() const
{
    auto lock = ScopedMutexLock(mutex_);

    return volumes_.map<UnicodeString>([](const FileSystemVolume* volume) { return volume->getName(); });
}

const FileSystemVolume* FileSystem::getVolume(const UnicodeString& name) const
{
    auto lock = ScopedMutexLock(mutex_);

    return volumes_.detect([&](const FileSystemVolume* volume) { return volume->getName() == name; }, nullptr);
}

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

void FileSystem::createSaveVolume()
{
    saveVolume_ = nullptr;

    auto saveDirectory = getUserDataLocalDirectory();
    if (!saveDirectory.length())
    {
        LOG_ERROR << "Failed getting the path to the $SAVE$ directory on this platform";
        return;
    }
    saveDirectory = FileSystem::joinPaths(getUserDataLocalDirectory(), "Save");

    LOG_INFO << "Save directory: " << saveDirectory;

    // Create the actual volume that will map $SAVE$ to the save directory for this platform
    saveVolume_ = new LocalDirectoryFileSystemVolume("SAVE", saveDirectory);

    addVolume(saveVolume_);
}

UnicodeString FileSystem::getPathToLocalFile(const UnicodeString& filename) const
{
    for (auto volume : assetDirectoryVolumes_)
    {
        if (volume->doesFileExist(filename))
            return joinPaths(volume->getLocalDirectory(), filename);
    }

    return {};
}

const UnicodeString& FileSystem::getInitialWorkingDirectory() const
{
    return initialWorkingDirectory_;
}

UnicodeString FileSystem::getCurrentWorkingDirectory()
{
    auto directory = UnicodeString();

#ifdef WINDOWS
    auto buffer = std::array<wchar_t, MAX_PATH>();
    GetCurrentDirectoryW(buffer.size(), buffer.data());
    directory = fromUTF16(buffer.data());
#else
    auto buffer = std::array<char, 1024>();
    if (getcwd(buffer.data(), buffer.size()))
        directory = fromUTF8(buffer.data());
#endif

    // Ensure the working directory uses forward slashes and has a single forward slash at the end
    directory.replace('\\', '/');
    directory.trimRight("/");
    directory << "/";

    return directory;
}

bool FileSystem::addLocalAssetDirectory(const UnicodeString& directory)
{
    if (!directory.length())
        return false;

    auto lock = ScopedMutexLock(mutex_);

    auto newAssetDirectory = directory;

    // Force relative paths to be relative to the initial working directory
    if (newAssetDirectory.startsWith("."))
        newAssetDirectory = joinPaths(getInitialWorkingDirectory(), newAssetDirectory);

    // Process '..' and '.' directory names
    auto pathComponents = newAssetDirectory.split("/");
    for (auto i = 0U; i < pathComponents.size(); i++)
    {
        if (i && pathComponents[i] == "..")
        {
            pathComponents.erase(i - 1);
            pathComponents.erase(i - 1);
        }
        else if (pathComponents[i] == "." || (i && !pathComponents[i].length()))
            pathComponents.erase(i);
    }
    newAssetDirectory = getCanonicalPath(UnicodeString(pathComponents, "/"));

    // Skip directories that don't exist
    if (!doesLocalDirectoryExist(newAssetDirectory))
        return false;

    // Ensure there is a trailing slash
    newAssetDirectory = FileSystem::joinPaths(newAssetDirectory, "");

    // Check if this directory has already been added
    if (assetDirectoryVolumes_.has(
            [&](const LocalDirectoryFileSystemVolume* v) { return v->getLocalDirectory() == newAssetDirectory; }))
        return true;

    // Validate directory name
    if (!isValidDirectoryName(newAssetDirectory))
    {
        LOG_ERROR << "Asset directory name '" << newAssetDirectory << "' is invalid";
        return false;
    }

    // Add the new asset directory
    assetDirectoryVolumes_.append(new LocalDirectoryFileSystemVolume(
        UnicodeString() + ".SEARCH" + assetDirectoryVolumes_.size(), newAssetDirectory));
    addVolume(assetDirectoryVolumes_.back());

    LOG_INFO << "Added asset directory: " << assetDirectoryVolumes_.back()->getLocalDirectory();

    return true;
}

bool FileSystem::removeLocalAssetDirectory(const UnicodeString& directory)
{
    auto lock = ScopedMutexLock(mutex_);

    for (auto volume : assetDirectoryVolumes_)
    {
        if (volume->getLocalDirectory() == directory)
        {
            removeVolume(volume);
            delete volume;

            assetDirectoryVolumes_.eraseValue(volume);

            return true;
        }
    }

    return false;
}

void FileSystem::clearLocalAssetDirectories()
{
    auto lock = ScopedMutexLock(mutex_);

    for (auto volume : assetDirectoryVolumes_)
    {
        removeVolume(volume);
        delete volume;
    }

    assetDirectoryVolumes_.clear();
}

void FileSystem::enumerateLocalFiles(const UnicodeString& directory, const UnicodeString& extension, bool recursive,
                                     Vector<UnicodeString>& files)
{
#ifdef WINDOWS

    auto fileData = WIN32_FIND_DATAW();

    // First file match
    auto hSearch = FindFirstFileW(joinPaths(directory, "*").toUTF16().as<wchar_t>(), &fileData);
    if (hSearch == INVALID_HANDLE_VALUE)
        return;

    while (true)
    {
        auto name = fromUTF16(fileData.cFileName);

        // Skip files and folders that start with a '.', as well as those that are explicitly flagged as hidden
        if (name.at(0) != '.' && !(fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Recurse into this subdirectory
                if (recursive)
                    enumerateLocalFiles(joinPaths(directory, name), extension, true, files);
            }
            else
            {
                // Check the extension matches
                if (name.endsWith(extension))
                    files.append(joinPaths(directory, name));
            }
        }

        if (!FindNextFileW(hSearch, &fileData))
            break;
    }

    FindClose(hSearch);

#elif defined(POSIX)

    auto dir = opendir(directory.toUTF8().as<char>());
    if (!dir)
        return;

    while (auto entry = readdir(dir))
    {
        auto name = fromUTF8(entry->d_name);

        // Skip files and folders that start with a '.'
        if (name.at(0) != '.')
        {
            // Check it's not a directory, we don't want to return directories
            auto dirtest = opendir(joinPaths(directory, name).toUTF8().as<char>());
            if (dirtest)
            {
                closedir(dirtest);

                // Recurse into this subdirectory
                if (recursive)
                    enumerateLocalFiles(joinPaths(directory, name), extension, recursive, files);
            }
            else
            {
                // Check the extension matches
                if (name.endsWith(extension))
                    files.append(joinPaths(directory, name));
            }
        }
    }

    closedir(dir);

#else

    LOG_ERROR << "Not supported on this platform";

#endif
}

void FileSystem::enumerateLocalDirectories(const UnicodeString& directory, bool recursive,
                                           Vector<UnicodeString>& directories)
{
#ifdef WINDOWS

    auto fileData = WIN32_FIND_DATAW();

    // First file match
    auto hSearch = FindFirstFileW(joinPaths(directory, "*").toUTF16().as<wchar_t>(), &fileData);
    if (hSearch == INVALID_HANDLE_VALUE)
        return;

    while (true)
    {
        if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !(fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            auto subdirectory = fromUTF16(fileData.cFileName);

            if (subdirectory.at(0) != '.')
            {
                directories.append(joinPaths(directory, subdirectory));

                if (recursive)
                    enumerateLocalDirectories(joinPaths(directory, subdirectory), recursive, directories);
            }
        }

        if (!FindNextFileW(hSearch, &fileData))
            break;
    }

    FindClose(hSearch);

#elif defined(POSIX)

    auto dir = opendir(directory.toUTF8().as<char>());
    if (!dir)
        return;

    while (auto entry = readdir(dir))
    {
        auto dirtest = opendir(joinPaths(directory, fromUTF8(entry->d_name)).toUTF8().as<char>());
        if (dirtest)
        {
            closedir(dirtest);
            dirtest = nullptr;

            auto subdirectory = fromUTF8(entry->d_name);
            if (subdirectory.at(0) != '.')
            {
                directories.append(joinPaths(directory, subdirectory));

                if (recursive)
                    enumerateLocalDirectories(joinPaths(directory, subdirectory), recursive, directories);
            }
        }
    }

    closedir(dir);
    dir = nullptr;

#else

    LOG_ERROR << "Not supported on this platform";

#endif
}

#ifdef WINDOWS

UnicodeString FileSystem::getCanonicalPath(const UnicodeString& path)
{
    auto shortPath = std::array<wchar_t, MAX_PATH>();
    auto longPath = std::array<wchar_t, MAX_PATH>();

    GetShortPathNameW(path.toUTF16().as<wchar_t>(), shortPath.data(), shortPath.size());
    GetLongPathNameW(shortPath.data(), longPath.data(), longPath.size());

    return fromUTF16(longPath.data());
}

#elif defined(LINUX)

UnicodeString FileSystem::getCanonicalPath(const UnicodeString& path)
{
    return path;
}

#endif

#ifdef POSIX

UnicodeString FileSystem::getHomeDirectory()
{
    // Get the location of the current user's home directory
    auto home = getenv("HOME");
    if (!home)
    {
        LOG_ERROR << "Failed getting $HOME environment variable";
        return "";
    }

    return fromUTF8(home);
}

#endif

UnicodeString FileSystem::getUserDataLocalDirectory()
{
    auto directory = UnicodeString();

    auto scopeToApplication = true;

// The user data local directory depends on the platform
#if defined(WINDOWS)
    auto applicationData = std::array<wchar_t, MAX_PATH>();
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, applicationData.data());
    directory = joinPaths(fromUTF16(applicationData.data()), "");
#elif defined(LINUX)
    directory = joinPaths(getHomeDirectory(), UnicodeString::Period);
#elif defined(APPLE)
    directory = joinPaths(getUserLibraryDirectory(), "Preferences/");

#ifndef MACOS
    scopeToApplication = false;
#endif

#endif

    directory.replace('\\', '/');

    if (directory.length() && scopeToApplication)
        directory << Globals::getClientName();

    return directory;
}

#ifdef WINDOWS

UnicodeString FileSystem::getSDKInstallDirectory()
{
    // Read install directory of the SDK out of the registry
    auto rkSDKDirectory = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Carbon SDK";
    auto rvSDKDirectory = L"InstallLocation";

    auto hOpenedKey = HKEY();

    auto openKeyOptions = KEY_QUERY_VALUE;
#ifdef CARBON_64BIT
    openKeyOptions |= KEY_WOW64_32KEY;
#endif

    auto result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, rkSDKDirectory, 0, openKeyOptions, &hOpenedKey);
    if (result != ERROR_SUCCESS)
        return {};    // SDK isn't installed

    auto text = std::array<wchar_t, MAX_PATH>();
    auto type = DWORD();
    auto dataSize = DWORD(sizeof(text));

    result = RegQueryValueExW(hOpenedKey, rvSDKDirectory, nullptr, &type, LPBYTE(text.data()), &dataSize);
    if (result != ERROR_SUCCESS)
    {
        RegCloseKey(hOpenedKey);
        return {};
    }

    RegCloseKey(hOpenedKey);

    // Construct final directory
    auto directory = fromUTF16(text.data());
    directory.replace('\\', '/');

    return directory;
}

bool FileSystem::addSDKSampleAssetsDirectory()
{
    // When a sample application is running in Visual Studio from under a Carbon checkout its working directory will be
    // /Source/<sample name>, and if this is the case then there is a ../../Assets/Samples directory that should be used
    // to source sample data. If this directory is there then use it. This ensures the sample data is accessible when
    // the SDK is not installed.
    if (addLocalAssetDirectory("../../Assets/Samples"))
        return true;

    // If the SDK is installed then use the sample assets from there
    auto sdkDirectory = getSDKInstallDirectory();
    if (sdkDirectory.length())
        return addLocalAssetDirectory(joinPaths(sdkDirectory, "Samples/Assets"));

    return false;
}

#endif

bool FileSystem::doesLocalFileExist(const UnicodeString& filename)
{
    auto file =
#ifdef WINDOWS
        _wfopen(filename.toUTF16().as<wchar_t>(), L"r");
#else
        fopen(filename.toUTF8().as<char>(), "r");
#endif
    if (!file)
        return false;

    fclose(file);

    return true;
}

bool FileSystem::doesLocalDirectoryExist(const UnicodeString& directory)
{
#ifdef WINDOWS
    auto attributes = GetFileAttributesW(directory.toUTF16().as<wchar_t>());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    auto dir = opendir(directory.toUTF8().as<char>());
    if (dir)
        closedir(dir);
    return dir != nullptr;
#endif
}

bool FileSystem::deleteLocalFile(const UnicodeString& filename)
{
#ifdef WINDOWS
    return DeleteFileW(filename.toUTF16().as<wchar_t>()) != 0;
#else
    return std::remove(filename.toUTF8().as<char>()) == 0;
#endif
}

#endif

bool FileSystem::enumerateFiles(UnicodeString directory, const UnicodeString& extension, bool recursive,
                                Vector<UnicodeString>& files)
{
    try
    {
        auto lock = ScopedMutexLock(mutex_);

        if (!directory.endsWith("/"))
            directory.append('/');

        // Validate directory name
        if (!isValidDirectoryName(directory))
        {
            LOG_ERROR << "Directory name is invalid";
            throw InvalidResourceNameFileSystemError;
        }

        // Check whether a file system volume is specified in the directory
        auto specifiedVolume = getVolumeSpecifiedByFilename(directory);
        if (specifiedVolume)
        {
            auto volumeFiles = Vector<UnicodeString>();

            auto result = specifiedVolume->enumerateFiles(stripVolumeName(directory, specifiedVolume), extension,
                                                          recursive, volumeFiles);

            for (const auto& volumeFile : volumeFiles)
                files.append(getVolumeNamePrefix(specifiedVolume->getName()) + volumeFile);

            throw result;
        }

        // Enumerate files in volumes
        auto virtualDirectory = joinPaths(joinPaths("/", directory), "/");
        for (auto volume : volumes_)
        {
            auto error = NoFileSystemError;

            auto volumeFiles = Vector<UnicodeString>();

            if (virtualDirectory.startsWith(volume->getMountLocation()))
            {
                error = volume->enumerateFiles(virtualDirectory.withoutPrefix(volume->getMountLocation()), extension,
                                               recursive, volumeFiles);
            }
            else if (recursive && volume->getMountLocation().startsWith(virtualDirectory))
                error = volume->enumerateFiles("/", extension, true, volumeFiles);

            for (const auto& volumeFile : volumeFiles)
                files.append(getVolumeNamePrefix(volume->getName()) + volumeFile);

            // Some file system errors are fine here, other more serious ones need to be reported
            if (error != NoFileSystemError && error != ResourceMissingFileSystemError &&
                error != NotSupportedFileSystemError && error != AccessDeniedFileSystemError)
                throw error;
        }

        return true;
    }
    catch (FileSystemError error)
    {
        if (error != NoFileSystemError)
        {
            events().queueEvent(new FileSystemErrorEvent(error, directory));
            return false;
        }

        return true;
    }
}

bool FileSystem::addVirtualFile(const UnicodeString& filename, const byte_t* data, unsigned int size)
{
    try
    {
        auto file = FileWriter();
        if (builtInVolume_.open(filename, file, false) != NoFileSystemError)
            return false;

        file.writeBytes(data, size);

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << filename << " - " << e;
        return false;
    }
}

unsigned int FileSystem::makeFourCC(const char* code)
{
    return uint(code[0]) | (uint(code[1]) << 8) | (uint(code[2]) << 16) | (uint(code[3]) << 24);
}

String FileSystem::formatByteSize(uint64_t size)
{
    auto unit = uint64_t(1);
    auto unitSuffix = String(" bytes");

    if (size > 105)
    {
        static const auto unitSuffixes = std::array<String, 6>{{"KB", "MB", "GB", "TB", "PB", "EB"}};

        unit = 1024;

        for (auto i = 0U; i < 6; i++)
        {
            if (size < (unit * unit) || i == 5)
            {
                unitSuffix = unitSuffixes[i];
                break;
            }

            unit *= 1024;
        }
    }

    // Work out major and minor compoments of the "n.nn" value to return
    auto major = size / unit;
    auto minor = uint((float(size % unit) / unit) * 100.0f);
    if (minor == 100)
    {
        major += 1;
        minor = 0;
    }

    // Return formatted byte size
    auto result = String(major);
    if (unit != 1)
        result << String::Period << (minor < 10 ? "0" : String::Empty) << minor;

    return result + unitSuffix;
}

UnicodeString FileSystem::getDirectory(const UnicodeString& fullPath)
{
    auto index = fullPath.findLastOf("/\\");
    if (index == -1)
        return {};

    return fullPath.substr(0, index);
}

UnicodeString FileSystem::getBaseName(const UnicodeString& fullPath)
{
    return fullPath.substr(fullPath.findLastOf("/\\") + 1);
}

UnicodeString FileSystem::joinPaths(const UnicodeString& path1, const UnicodeString& path2)
{
    static const auto forwardSlash = UnicodeString("/");

    return path1.trimmedRight(forwardSlash) + forwardSlash + path2.trimmedLeft(forwardSlash);
}

String FileSystem::getDateTime()
{
    auto buffer = std::array<char, 256>();

    if (!getDateTime(buffer.data(), buffer.size()))
        return {};

    return buffer.data();
}

bool FileSystem::getDateTime(char* buffer, unsigned int bufferSize)
{
    if (bufferSize < 256)
        return false;

    if (!getFormattedDateTime("%A, %d %B %Y", buffer, bufferSize))
        return false;

    strcat(buffer, " at ");

    auto hour = std::array<char, 8>();
    if (!getFormattedDateTime("%I:", hour.data(), hour.size()))
        return false;
    if (hour[0] == '0')
        memmove(&hour[0], &hour[1], strlen(hour.data()));

    strcat(buffer, hour.data());

    auto minute = std::array<char, 16>();
    if (!getFormattedDateTime("%M%p", minute.data(), minute.size()))
        return false;

    auto length = strlen(minute.data());
    for (auto i = 0U; i < length; i++)
        minute[i] = char(tolower(minute[i]));

    strcat(buffer, minute.data());

    return true;
}

String FileSystem::getShortDateTime()
{
    auto buffer = std::array<char, 256>();

    if (!getShortDateTime(buffer.data(), buffer.size()))
        return {};

    return buffer.data();
}

bool FileSystem::getShortDateTime(char* buffer, unsigned int bufferSize)
{
    return getFormattedDateTime("%Y/%m/%d %H:%M:%S", buffer, bufferSize);
}

String FileSystem::getFormattedDateTime(const String& format)
{
    auto buffer = std::array<char, 256>();

    if (!getFormattedDateTime(format.cStr(), buffer.data(), buffer.size()))
        return {};

    return buffer.data();
}

bool FileSystem::getFormattedDateTime(const char* format, char* buffer, unsigned int bufferSize)
{
    // Get the current time
    auto utcTime = time(nullptr);
    if (utcTime == time_t(-1))
        return false;

    // Convert to a local time
    auto localTime = tm();
#ifdef _MSC_VER
    if (localtime_s(&localTime, &utcTime))
#else
    if (!localtime_r(&utcTime, &localTime))
#endif
        return false;

    // Convert time to a formatted string
    return strftime(buffer, bufferSize, format, &localTime) != 0;
}

bool FileSystem::readTextFile(const UnicodeString& filename, String& string)
{
    try
    {
        auto file = FileReader();
        open(filename, file);

        return file.getDataAsString(string);
    }
    catch (const Exception&)
    {
        return false;
    }
}

bool FileSystem::readTextFile(const UnicodeString& filename, UnicodeString& string)
{
    try
    {
        auto file = FileReader();
        open(filename, file);

        return file.getDataAsString(string);
    }
    catch (const Exception&)
    {
        return false;
    }
}

bool FileSystem::readTextFile(const UnicodeString& filename, Vector<Vector<String>>& lineTokens)
{
    try
    {
        lineTokens.clear();

        auto file = FileReader();
        open(filename, file);

        return file.getLineTokens(lineTokens);
    }
    catch (const Exception&)
    {
        return false;
    }
}

bool FileSystem::readTextFile(const UnicodeString& filename, ParameterArray& parameters)
{
    parameters.clear();

    auto lineTokens = Vector<Vector<String>>();
    if (!readTextFile(filename, lineTokens))
        return false;

    for (const auto& line : lineTokens)
    {
        if (line.size() >= 3 && line[1] == "=")
            parameters[line[0]].setString(String(line.slice(2), " "));
        else
            LOG_WARNING << "In file '" << filename << "' the line '" << String(line, " ")
                        << "' is not formatted as 'key = value'";
    }

    return true;
}

bool FileSystem::writeTextFile(const UnicodeString& filename, const ParameterArray& parameters)
{
    try
    {
        auto file = FileWriter();
        open(filename, file, true);

        for (auto parameter : parameters)
        {
            // Check value has no newlines as this would mess things up
            if (parameter.getValue().getString().has('\n'))
            {
                LOG_WARNING << "Skipping parameter value with newline: '" << parameter.getName() << "'";
                continue;
            }

            file.writeText(UnicodeString(parameter.getName()) + " = " + parameter.getValue());
        }

        return true;
    }
    catch (const Exception&)
    {
        LOG_ERROR << "Failed opening file: " << filename;

        return false;
    }
}

String FileSystem::errorToString(FileSystemError error)
{
#define HANDLE_FILE_SYSTEM_ERROR(Error) \
    if (error == Error)                 \
        return #Error;

    HANDLE_FILE_SYSTEM_ERROR(NoFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(ResourceMissingFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(InvalidResourceNameFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(AccessDeniedFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(InvalidDataFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(FreeSpaceFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(NotSupportedFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(OutOfMemoryFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(InvalidOperationFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(IncompleteFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(VersionedSectionFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(DataCorruptionFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(HardwareFailureFileSystemError)
    HANDLE_FILE_SYSTEM_ERROR(RemovableMediaNotPresentError)

#undef HANDLE_FILE_SYSTEM_ERROR

    return "UnknownFileSystemError";
}

UnicodeString FileSystem::getVolumeNamePrefix(const UnicodeString& volumeName)
{
    return "$" + volumeName + "$/";
}

UnicodeString FileSystem::stripVolumeName(const UnicodeString& filename, const FileSystemVolume* volume)
{
    return volume ? filename.withoutPrefix(getVolumeNamePrefix(volume->getName())) : filename;
}

FileSystemVolume* FileSystem::getVolumeSpecifiedByFilename(const UnicodeString& filename)
{
    return volumes_.detect(
        [&](const FileSystemVolume* v) { return filename.startsWith(getVolumeNamePrefix(v->getName())); }, nullptr);
}

const FileSystemVolume* FileSystem::getVolumeSpecifiedByFilename(const UnicodeString& filename) const
{
    return volumes_.detect(
        [&](const FileSystemVolume* v) { return filename.startsWith(getVolumeNamePrefix(v->getName())); }, nullptr);
}

}
