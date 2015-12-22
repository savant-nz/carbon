/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Global engine functions. Contains routines to initialize and uninitialize the engine and manage its execution.
 */
class CARBON_API Globals
{
public:

    /**
     * Does core engine initialization. This should be called prior to any other use of the engine. Once the global objects have
     * been created, all the functions registered with the CARBON_REGISTER_STARTUP_FUNCTION() macro are called. This function
     * immediately returns true if the engine is already initialized. If false is returned then there was an error on startup.
     * The \a clientName parameter identifies the current client application and is used to save and restore settings and other
     * client-local data.
     */
    static bool initializeEngine(const String& clientName);

    /**
     * Does engine shutdown. This should be called at the end of the program. After this is executed, the engine should not be
     * used until after Globals::initializeEngine() is called again. Before the global objects are deleted, all the functions
     * registered with the CARBON_REGISTER_SHUTDOWN_FUNCTION() macro are called.
     */
    static void uninitializeEngine();

    /**
     * Returns whether the engine is currently initialized.
     */
    static bool isEngineInitialized() { return isInitialized_; }

    /**
     * Returns true when the process is in the middle of static initialization. Once the application's \a main() routine has
     * been entered this method will return false.
     */
    static bool isInStaticInitialization() { return inStaticInitialization_; }

    /**
     * Sets the return value from Globals::isInStaticInitialization(), for internal use only.
     */
    static void setInStaticInitialization(bool value) { inStaticInitialization_ = value; }

#ifdef WINDOWS
    /**
     * On Windows, returns the instance of the module (DLL or EXE) that contains the main engine code.
     */
    static HINSTANCE getHInstance();
#endif

    /**
     * Returns the name of the engine's developer.
     */
    static String getDeveloperName() { return "Carbon Development Team"; }

    /**
     * Returns the engine's display name.
     */
    static String getEngineName() { return "Carbon"; }

    /**
     * Returns the client name that was passed to Globals::initializeEngine(). The client name will be at most 127 characters
     * long.
     */
    static const String& getClientName();

    /**
     * This is the same as Globals::getClientName() however the returned client name is stored in a static buffer and so this
     * method can be safely used after engine shutdown, e.g. during static deinitialization.
     */
    static const char* getClientNameBuffer() { return clientNameBuffer_.data(); }

#ifdef CARBON_INCLUDE_LOGGING
    /**
     * This is a logging function that takes printf-style arguments and writes the result to the stdout stream, this is
     * particularly useful when wanting to print messages inside MemoryInterceptor or MemoryValidator callbacks, or in other
     * places where logging must not trigger memory allocations or frees. On Windows the logged string is also output as a debug
     * string. No memory is allocated on the heap to write the output and this method is thread-safe. A newline is automatically
     * added.
     */
    static void debugLog(const char* format, ...);
#else
    static void debugLog(const char* format, ...) {}
#endif

    /**
     * Adds a startup function that will be run by Globals::initializeEngine() following engine initialization. Higher priority
     * functions are run before lower priority functions. The CARBON_REGISTER_STARTUP_FUNCTION() macro provides an easy way to
     * automatically add a startup function.
     */
    static void addStartupFunction(VoidFunction fn, int priority);

    /**
     * Removes a startup function added with Globals::addStartupFunction().
     */
    static void removeStartupFunction(VoidFunction fn);

    /**
     * Adds a shutdown function that will be run by Globals::uninitializeEngine() prior to engine shutdown. Higher priority
     * functions are run before lower priority functions. The CARBON_REGISTER_SHUTDOWN_FUNCTION() macro provides an easy way to
     * automatically add a shutdown function.
     */
    static void addShutdownFunction(VoidFunction fn, int priority);

    /**
     * Removes a shutdown function added with Globals::addShutdownFunction().
     */
    static void removeShutdownFunction(VoidFunction fn);

    /**
     * For use following engine shutdown, this returns whether or not any resources such as scenes or entities were leaked by
     * the application.
     */
    static unsigned int getLeakedResourceCount() { return leakedResourceCount_; }

    /**
     * For use during engine shutdown, this increases the leaked resource count by the given amount. The total leaked resource
     * count can be retrieved using Globals::getLeakedResourceCount().
     */
    static void increaseLeakedResourceCount(unsigned int count) { leakedResourceCount_ += count; }

    /**
     * Returns the command line parameters as set by the last call to Globals::setCommandLineParameters(). The command line
     * parameters are set automatically if the built-in application entry points are used.
     */
    static const Vector<UnicodeString>& getCommandLineParameters() { return commandLineParameters_; }

    /**
     * Sets the command line parameters based on the given command line string.
     */
    static void setCommandLineParameters(const UnicodeString& commandLine) { commandLineParameters_ = commandLine.getTokens(); }

    /**
     * Sets the command line parameters based on the given argc and argv values.
     */
    static void setCommandLineParameters(int argc, const char**);

    /**
     * Returns the exit code that will be returned by the application's `main()` routine when the application shuts down. By
     * default the exit code is zero when the application ran successfully, and one if there were any initialization failures.
     * The exit code can be set using Globals::setExitCode().
     */
    static int getExitCode() { return exitCode_; }

    /**
     * Sets the exit code that will be returned by the \a main() routine when the application shuts down. See \a
     * Globals::getExitCode() for details.
     */
    static void setExitCode(int value) { exitCode_ = value; }

    /**
     * Returns the fully qualified path and filename of the executable that was used to launch the running application.
     */
    static const UnicodeString& getExecutableName();

    /**
     * Destroys the current graphics interface and recreates it using InterfaceRegistry<GraphicsInterface>::create(). For
     * internal use only.
     */
    static void recreateGraphicsInterface();

    /**
     * Returns the global Console instance.
     */
    static Console& console()
    {
        assert(console_);
        return *console_;
    }

    /**
     * Returns the global DataBufferManager instance.
     */
    static DataBufferManager& dataBuffers()
    {
        assert(dataBufferManager_);
        return *dataBufferManager_;
    }

    /**
     * Returns the global EffectManager instance.
     */
    static EffectManager& effects()
    {
        assert(effectManager_);
        return *effectManager_;
    }

    /**
     * Returns the global EventManager instance.
     */
    static EventManager& events()
    {
        assert(eventManager_);
        return *eventManager_;
    }

    /**
     * Returns the global FileSystem instance.
     */
    static FileSystem& fileSystem()
    {
        assert(fileSystem_);
        return *fileSystem_;
    }

    /**
     * Returns the global FontManager instance.
     */
    static FontManager& fonts()
    {
        assert(fontManager_);
        return *fontManager_;
    }

    /**
     * Returns the global GraphicsInterface instance.
     */
    static GraphicsInterface& graphics()
    {
        assert(graphicsInterface_);
        return *graphicsInterface_;
    }

    /**
     * Returns the global MaterialManager instance.
     */
    static MaterialManager& materials()
    {
        assert(materialManager_);
        return *materialManager_;
    }

    /**
     * Returns the global MeshManager instance.
     */
    static MeshManager& meshes()
    {
        assert(meshManager_);
        return *meshManager_;
    }

    /**
     * Returns the global PhysicsInterface instance.
     */
    static PhysicsInterface& physics()
    {
        assert(physicsInterface_);
        return *physicsInterface_;
    }

    /**
     * Returns the global PlatformInterface instance.
     */
    static PlatformInterface& platform()
    {
        assert(platformInterface_);
        return *platformInterface_;
    }

    /**
     * Returns the global Renderer instance.
     */
    static Renderer& renderer()
    {
        assert(renderer_);
        return *renderer_;
    }

    /**
     * Returns the global ScriptManager instance.
     */
    static ScriptManager& scripts()
    {
        assert(scriptManager_);
        return *scriptManager_;
    }

    /**
     * Returns the global SettingsManager instance.
     */
    static SettingsManager& settings()
    {
        assert(settingsManager_);
        return *settingsManager_;
    }

    /**
     * Returns the global SoundInterface instance.
     */
    static SoundInterface& sounds()
    {
        assert(soundInterface_);
        return *soundInterface_;
    }

    /**
     * Returns the global SoundShaderManager instance.
     */
    static SoundShaderManager& soundShaders()
    {
        assert(soundShaderManager_);
        return *soundShaderManager_;
    }

    /**
     * Returns the global TextureManager instance.
     */
    static TextureManager& textures()
    {
        assert(textureManager_);
        return *textureManager_;
    }

    /**
     * Returns the global ThemeManager instance.
     */
    static ThemeManager& theme()
    {
        assert(themeManager_);
        return *themeManager_;
    }

private:

    typedef std::pair<int, VoidFunction> PrioritizedFunction;

    static Vector<PrioritizedFunction>* runAtStartup_;
    static Vector<PrioritizedFunction>* runAtShutdown_;

    static unsigned int leakedResourceCount_;

    static Vector<UnicodeString> commandLineParameters_;
    static UnicodeString executableName_;
    static unsigned int exitCode_;

    static bool isInitialized_;
    static bool inStaticInitialization_;
    static std::array<char, 256> clientNameBuffer_;
    static String clientName_;

    static Console* console_;
    static DataBufferManager* dataBufferManager_;
    static EffectManager* effectManager_;
    static EventManager* eventManager_;
    static FileSystem* fileSystem_;
    static FontManager* fontManager_;
    static GraphicsInterface* graphicsInterface_;
    static MaterialManager* materialManager_;
    static MeshManager* meshManager_;
    static PhysicsInterface* physicsInterface_;
    static PlatformInterface* platformInterface_;
    static Renderer* renderer_;
    static ScriptManager* scriptManager_;
    static SettingsManager* settingsManager_;
    static SoundInterface* soundInterface_;
    static SoundShaderManager* soundShaderManager_;
    static TextureManager* textureManager_;
    static ThemeManager* themeManager_;
};

/**
 * \file
 */

/**
 * Registers a function to be called at startup. The \a Function parameter should be a function that follows the
 * Carbon::Globals::StartupFunction typedef.
 */
#define CARBON_REGISTER_STARTUP_FUNCTION(Function, Priority)                                       \
    CARBON_UNIQUE_NAMESPACE                                                                        \
    {                                                                                              \
        static struct RegisterStartupFunction                                                      \
        {                                                                                          \
            RegisterStartupFunction() { Carbon::Globals::addStartupFunction(Function, Priority); } \
            ~RegisterStartupFunction() { Carbon::Globals::removeStartupFunction(Function); }       \
        } registerStartupFunction;                                                                 \
    }                                                                                              \
    CARBON_UNIQUE_NAMESPACE_END

/**
 * Registers a function to be called at shutdown. The \a Function parameter should be a function that follows the
 * Carbon::Globals::ShutdownFunction typedef.
 */
#define CARBON_REGISTER_SHUTDOWN_FUNCTION(Function, Priority)                                        \
    CARBON_UNIQUE_NAMESPACE                                                                          \
    {                                                                                                \
        static struct RegisterShutdownFunction                                                       \
        {                                                                                            \
            RegisterShutdownFunction() { Carbon::Globals::addShutdownFunction(Function, Priority); } \
            ~RegisterShutdownFunction() { Carbon::Globals::removeShutdownFunction(Function); }       \
        } registerShutdownFunction;                                                                  \
    }                                                                                                \
    CARBON_UNIQUE_NAMESPACE_END
}
