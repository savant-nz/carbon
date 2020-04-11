# CarbonEngine Preprocessor Defines

The following defines are used at compile time to control the build.

#### `DEBUG`

This is automatically defined in any debug build.

#### `CARBON_DEBUG`

This is defined by default in debug builds and toggles the inclusion of additional debugging code and asserts. It also
turns on the memory interceptor and enables memory leak reporting.

#### `CARBON_DISABLE_DEBUG`

`CARBON_DEBUG` is defined by default in debug builds, however if `CARBON_DISABLE_DEBUG` is defined then the automatic
definition of `CARBON_DEBUG` will not occur, resulting in a somewhat faster debug build that excludes the normal extra
debugging code present in a standard debug build.

#### `CARBON_STATIC_LIBRARY`

The engine can be linked as either a dynamic library or a static library, and the default depends on the platform. Some
platforms require that static linking be used. When doing static library builds the `CARBON_STATIC_LIBRARY` token must
be defined both when building the engine as well as when building the client application. If the platform being targeted
requires static linking then the `CARBON_STATIC_LIBRARY` token will be defined automatically.

#### `CARBON_VERSION`

This is an optional define that specifies the version of the engine. Primarily used when building the SDK to tag SDK
builds with the appropriate version information. This define must only be used by `Core/BuildInfo.cpp`, applications
must access the engine version through `BuildInfo::getVersion()`.

#### `CARBON_INCLUDE_ANGELSCRIPT`

Enables scripting through the AngelScript library.

#### `CARBON_INCLUDE_BULLET`

Enables physical simulation of objects through the Bullet Physics library.

#### `CARBON_INCLUDE_CONSOLE_COMMANDS`

Includes all console commands in the build, these are all implemented in `ConsoleCommands.h`. When this token is not
defined all the built-in console commands are stripped from the build. `CARBON_INCLUDE_CONSOLE_COMMANDS` is defined
automatically except when `CARBON_DISABLE_CONSOLE_COMMANDS` is defined.

#### `CARBON_INCLUDE_FREEIMAGE`

Enables the use of FreeImage for loading a variety of otherwise unsupported image and texture formats.

#### `CARBON_INCLUDE_FREETYPE`

Enables the use of FreeType for loading fonts from system TrueType and OpenType fonts.

#### `CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS`

Enables the use of system headers and functions for doing local filesystem access. Without this only files made
available through `FileSystemVolume` instances will be accessible on the virtual filesystem. This define is
automatically present on platforms that support local filesystem access, and will not be defined on those platforms that
do not support standard local filesystem access APIs.

#### `CARBON_INCLUDE_LOGGING`

Includes all logfile output in the build, most logfile output comes from uses of the `Log*()` macros. When this token is
not defined all logging is stripped out of the build. Note that the `Logfile::Enabled` setting can be used to stop the
creation of logfiles while still keeping the logging itself, this allows the logging output to then be redirected
elsewhere using `Logfile::addOutputSink()`. `CARBON_INCLUDE_LOGGING` is defined automatically except when
`CARBON_DISABLE_LOGGING` is defined.

#### `CARBON_INCLUDE_MAX_EXPORTER` (Windows only)

Enables the engine DLL to be loaded into 3D Studio Max as a plugin and provide exporter functionality.

#### `CARBON_INCLUDE_MAYA_EXPORTER`

Enables the compiled library to be loaded into Maya as a plugin and provide exporter functionality.

#### `CARBON_INCLUDE_MEMORY_INTERCEPTOR`

Includes the memory interceptor in the build. By default it is only included in debug builds, however defining this in
release builds will cause them to also include the memory interceptor and memory leak reporting.

#### `CARBON_DISABLE_MEMORY_INTERCEPTOR`

This can be defined in debug builds in order to exclude just the memory interceptor from the build while retaining all
other debugging code. This also removes memory leak reporting from the build.

#### `CARBON_INCLUDE_OPENAL`

Enables the playing of audio through OpenAL.

#### `CARBON_INCLUDE_OPENASSETIMPORT`

Enables the use of Open Asset Import for loading a variety of otherwise unsupported mesh formats.

#### `CARBON_INCLUDE_OCULUSRIFT`

Enables support for Oculus Rift devices.

#### `CARBON_INCLUDE_OPENGL11`

Enables the use of the OpenGL 1.1 graphics interface backend, this interface requires OpenGL 1.1 plus a number of
additional OpenGL extensions in order to operate. The supported GLSL version is 1.10. This is the default interface used
on Windows, Linux and macOS.

#### `CARBON_INCLUDE_OPENGL41`

Enables the use of the OpenGL 4.1 Core Profile graphics interface backend. The supported GLSL versions are 1.10 and
4.10. This graphics interface is currently only supported on macOS.

#### `CARBON_INCLUDE_OPENGLES2`

Enables the use of the OpenGL ES 2 graphics interface backend. This is the default on iOS and Android.

#### `CARBON_INCLUDE_PHYSX`

Enables physical simulation of objects through the NVIDIA PhysX libraries. The PhysX libraries are loaded dynamically at
runtime, which means that including PhysX support does not result in PhysX having to be installed on target machines.
PhysX will only be available at runtime if it is installed on the target machine.

#### `CARBON_INCLUDE_PLATFORM_MACOS`

Enables the engine to run on the macOS Cocoa API. This is preferred on macOS over the SDL platform implementation.

#### `CARBON_INCLUDE_PLATFORM_SDL`

Enables the engine to run on SDL 2. Intended for use on Linux though it does work on other platforms support by SDL 2.

#### `CARBON_INCLUDE_PLATFORM_WINDOWS`

Enables the engine to run on the Windows API and DirectInput 8. This is preferred on Windows over the SDL platform
implementation.

#### `CARBON_INCLUDE_VORBIS`

Enables support for playing back audio files compressed with the OGG vorbis format.

#### `CARBON_INCLUDE_ZLIB`

Enables the use of the ZLib library for compression and decompression. This is required if `CARBON_INCLUDE_FREEIMAGE` or
`CARBON_INCLUDE_OPENASSETIMPORT` are defined.
