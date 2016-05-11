/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CARBONENGINE_COMMON_H
#define CARBONENGINE_COMMON_H

#ifndef DOXYGEN

// Determine what platform is being targeted. Either WINDOWS, LINUX, MACOSX, iOS, ANDROID or CONSOLE will be defined, and
// these defines should be used when conditional compilation for a specific platform is required. APPLE is defined on all Apple
// platforms. Android is a Linux platform and so both LINUX and ANDROID will be defined when building for Android.

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WINDOWS
        #define WINDOWS 1
    #endif
    #ifdef _MSC_VER
        #if _MSC_VER < 1900
            #error This version of Visual Studio is not supported, please upgrade
        #elif _MSC_VER == 1900
            #define CARBON_MSVC_VERSION "2015"
        #else
            #define CARBON_MSVC_VERSION "Unknown"
        #endif

        // The CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX macro is used to automatically select the correct static library file for
        // a dependency such as FreeImage, it is aware of debug and release variants as well as the MSVC version being used
        #ifdef _DEBUG
            #define CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX "VisualStudio" CARBON_MSVC_VERSION "Debug.lib"
        #else
            #define CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX "VisualStudio" CARBON_MSVC_VERSION ".lib"
        #endif

        // Check that the statically linked multithreaded runtime is being used
        #if defined(_DLL) || !defined(_MT)
            #error The statically linked multithreaded runtime must be used, compile with either /MT or /MTd
        #endif

        #define _CRT_SECURE_NO_WARNINGS
    #else
        #error This compiler is not supported on this platform
    #endif

    #define CARBON_INCLUDE_DEFAULT_MEMORY_INTERCEPTOR_BACKEND
    #define CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
#elif defined(__linux__)
    #ifndef LINUX
        #define LINUX 1
    #endif
    #ifndef POSIX
        #define POSIX
    #endif
    #ifndef __GNUC__
        #error This compiler is not supported on this platform
    #endif

    #ifdef __ANDROID__
        #define ANDROID

        // Automatically configure the included components for Android
        #define CARBON_INCLUDE_ANGELSCRIPT
        #define CARBON_INCLUDE_BULLET
        #define CARBON_INCLUDE_FREEIMAGE
        #define CARBON_INCLUDE_OPENAL
        #define CARBON_INCLUDE_OPENASSETIMPORT
        #define CARBON_INCLUDE_OPENGLES2
        #define CARBON_INCLUDE_VORBIS
        #define CARBON_INCLUDE_ZLIB

        // Android builds always use a static library
        #ifndef CARBON_STATIC_LIBRARY
            #define CARBON_STATIC_LIBRARY
        #endif
    #else
        #define CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    #endif

    #define CARBON_INCLUDE_DEFAULT_MEMORY_INTERCEPTOR_BACKEND
#elif defined(__APPLE__)
    #ifndef APPLE
        #define APPLE 1
    #endif
    #ifndef POSIX
        #define POSIX
    #endif
    #ifndef __clang__
        #error This compiler is not supported on this platform
    #endif

    #include <Availability.h>
    #include <AvailabilityMacros.h>
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #ifndef iOS
            #define iOS 1
        #endif

        #if __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_8_0
            #error The minimum required iOS version should be set to at least 8.0
        #endif

        // iOS builds always use a static library
        #ifndef CARBON_STATIC_LIBRARY
            #define CARBON_STATIC_LIBRARY
        #endif

        // Automatically configure the included components for iOS
        #define CARBON_INCLUDE_ANGELSCRIPT
        #define CARBON_INCLUDE_BULLET
        #define CARBON_INCLUDE_FREEIMAGE
        #define CARBON_INCLUDE_OPENAL
        #define CARBON_INCLUDE_OPENASSETIMPORT
        #define CARBON_INCLUDE_OPENGLES2
        #define CARBON_INCLUDE_VORBIS
        #define CARBON_INCLUDE_ZLIB
    #else
        #ifndef MACOSX
            #define MACOSX 1
        #endif

        #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_9
            #error The minimum required Mac OS X version should be set to at least 10.9
        #endif
    #endif

    #define CARBON_INCLUDE_DEFAULT_MEMORY_INTERCEPTOR_BACKEND
    #define CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
#else
    // External platform modules can hook in their Common.h here
#endif

// Static library builds and exporter builds always define CARBON_EXPORTS
#if defined(CARBON_STATIC_LIBRARY) || defined(CARBON_INCLUDE_MAX_EXPORTER) || defined(CARBON_INCLUDE_MAYA_EXPORTER)
    #ifndef CARBON_EXPORTS
        #define CARBON_EXPORTS
    #endif
#endif

#ifdef WINDOWS

    // Use Unicode-aware Windows and system APIs. The currently supported versions of 3D Studio Max are not Unicode compatible,
    // so the Max exporter plugin builds don't define these tokens.
    #undef _UNICODE
    #undef UNICODE
    #ifndef CARBON_INCLUDE_MAX_EXPORTER
        #define _UNICODE
        #define UNICODE
    #endif

    // Build against the Windows Vista APIs
    #define WINVER 0x0600
    #define _WIN32_WINNT  0x0600
    #define NTDDI_VERSION NTDDI_VISTA

    #define ISOLATION_AWARE_ENABLED 1

    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>

    #ifdef CARBON_EXPORTS
        #include <commctrl.h>
        #include <direct.h>
        #include <io.h>
        #include <process.h>
        #include <shellapi.h>
        #include <shlobj.h>
    #endif

    #ifdef _MSC_VER

        #ifdef CARBON_EXPORTS
            #pragma comment(lib, "AdvAPI32.lib")
            #pragma comment(lib, "ComCtl32.lib")
            #pragma comment(lib, "Gdi32.lib")
            #pragma comment(lib, "Ole32.lib")
            #pragma comment(lib, "shell32.lib")
            #pragma comment(lib, "User32.lib")
        #endif

        // Disable some compiler warnings
        #pragma warning(disable: 4100)
        #pragma warning(disable: 4127)
        #pragma warning(disable: 4251)
        #pragma warning(disable: 4267)
        #pragma warning(disable: 4290)
        #pragma warning(disable: 4355)
        #pragma warning(disable: 4458)
        #pragma warning(disable: 4512)

        // DLL interface
        #ifndef CARBON_STATIC_LIBRARY
            #ifdef CARBON_EXPORTS
                #define CARBON_API __declspec(dllexport)
            #else
                #define CARBON_API __declspec(dllimport)
            #endif
        #endif

        #define CARBON_EXPORTER_API __declspec(dllexport)

    #endif

    #ifdef _WIN64
        #define CARBON_64BIT
    #endif

#else

    #ifdef LINUX
        #define __STDC_CONSTANT_MACROS
    #endif

    #ifdef CARBON_EXPORTS
        #include <unistd.h>
        #include <sys/time.h>

        #ifdef LINUX
            #include <sys/prctl.h>
        #endif

        #ifdef APPLE
            #include <sys/sysctl.h>
        #endif

        #ifdef POSIX
            #include <dlfcn.h>
        #endif

        #ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
            #include <dirent.h>
            #include <sys/mman.h>
            #include <sys/stat.h>
        #endif
    #endif

    // Use symbol hiding on GCC when creating a shared library
    #if defined(__GNUC__) && !defined(CARBON_STATIC_LIBRARY)
        #ifdef CARBON_EXPORTS
            #define CARBON_API __attribute__((visibility("default")))
        #endif
        #define CARBON_EXCEPTION_API __attribute__((visibility("default")))
        #define CARBON_EXPORTER_API __attribute__((visibility("default")))
    #endif

    #ifdef __LP64__
        #define CARBON_64BIT
    #endif

    #if defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
        #define CARBON_BIG_ENDIAN
    #endif

    // Automatic reference counting must be used when building Objective C/C++ code
    #ifdef __OBJC__
        #if !__has_feature(objc_arc)
            #error Automatic reference couning (ARC) must be enabled when building Objective C/C++ code
        #endif

        #include <Foundation/Foundation.h>
    #endif
#endif

// The standard exporting of symbols unnecessarily increases the size of exporter plugin builds, so turn off the default symbol
// exporting behavior for these builds. Any required public symbols will be exported using CARBON_EXPORTER_API instead.
#if defined(CARBON_INCLUDE_MAX_EXPORTER) || defined(CARBON_INCLUDE_MAYA_EXPORTER)
    #undef CARBON_API
#endif

// Ensure that CARBON_API, CARBON_EXCEPTION_API and CARBON_EXPORTER_API are defined
#ifndef CARBON_API
    #define CARBON_API
#endif
#ifndef CARBON_EXCEPTION_API
    #define CARBON_EXCEPTION_API CARBON_API
#endif
#ifndef CARBON_EXPORTER_API
    #define CARBON_EXPORTER_API CARBON_API
#endif

#ifndef CARBON_BIG_ENDIAN
    #define CARBON_LITTLE_ENDIAN
#endif

// Macro to get the current function
#if defined(__GNUC__) || defined(__MWERKS__) || defined(__SNC__)
    #define CARBON_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
    #define CARBON_CURRENT_FUNCTION __FUNCTION__
#endif

// GCC versions prior to 4.9 are not supported
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 4 || __GNUC__ == 4 && __GNUC_MINOR__ < 9)
    #error This version of GCC is not supported, version 4.9 or later is required
#endif

// Helper macro that quotes a macro parameter
#define CARBON_QUOTE_MACRO_VALUE_(x) #x
#define CARBON_QUOTE_MACRO_VALUE(x) CARBON_QUOTE_MACRO_VALUE_(x)

// Helper macro that evaluates and joins together two values
#define CARBON_JOIN_MACRO_VALUES_(x, y) x##y
#define CARBON_JOIN_MACRO_VALUES(x, y) CARBON_JOIN_MACRO_VALUES_(x, y)

// Defines a namespace with a unique name based off the __LINE__ macro, an anonymous namespace is also put in place to avoid
// interactions between uses of CARBON_UNIQUE_NAMESPACE in different files that happen to have the same expansion of the
// __LINE__ macro
#define CARBON_UNIQUE_NAMESPACE                                   \
    namespace CARBON_JOIN_MACRO_VALUES(UniqueNamespace, __LINE__) \
    {                                                             \
    namespace
#define CARBON_UNIQUE_NAMESPACE_END }

// Include the C++ standard library headers needed by the engine, these are not included for client applications because client
// applications are responsible for making sure they include all the headers they need rather than relying on an include in this
// header which could end up changing.
//
// This also helps to enforce STL containers never appearing in the engine's public API. This separation is important for
// maintaining compatibility when the engine is used by an application that doesn't use the exact same STL library version as
// was used to compile the engine. Mixing STL implementation versions can cause memory corruption or crashes.
//
// Because of this STL classes should not be part of the public API, particularly those which make allocations. An exception is
// made in the case of certain low-level types that don't allocate, e.g. std::array, std::exception, std::function, std::pair
// and std::type_info.
#ifdef CARBON_EXPORTS
    #include <cctype>
    #include <cerrno>
    #include <chrono>
    #include <climits>
    #include <cstdarg>
    #include <cstdint>
    #include <cstring>

    #ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
        #include <iostream>
    #endif

    #include <iomanip>
    #include <limits>
    #include <map>
    #include <memory>
    #include <random>
    #include <set>
    #include <sstream>
    #include <unordered_map>
    #include <unordered_set>
    #include <vector>
#endif

// The following standard library headers must be included in client applications because they contain definitions that are used
// by engine headers
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <typeinfo>

// On Apple platforms check that libc++ is being used
#if defined(APPLE) && !defined(_LIBCPP_VERSION)
    #error The libc++ library must be used on Apple platforms
#endif

// CARBON_DEBUG should be used for conditional compilation of extra debugging code useful in development, by default it is
// defined in debug builds, but it can be explicitly removed by defining CARBON_DISABLE_DEBUG
#if defined(_DEBUG) && !defined(DEBUG)
    #define DEBUG
#endif
#if defined(DEBUG) && !defined(CARBON_DISABLE_DEBUG)
    #define CARBON_DEBUG
#endif

// If CARBON_DEBUG is defined then assert() calls are included in the build, otherwise they are explicitly turned into no-ops
#ifdef CARBON_DEBUG
    #include <cassert>
#else
    #undef assert
    #define assert(condition) ((void)0)

    // Disable calls to NSAssert() in non-debug builds
    #if defined(APPLE) && !defined(NS_BLOCK_ASSERTIONS)
        #define NS_BLOCK_ASSERTIONS 1
    #endif
#endif

// The CARBON_INCLUDE_LOGGING token is used throughout the engine to toggle inclusion of logging code, it is defined
// automatically unless CARBON_DISABLE_LOGGING has been specified
#undef CARBON_INCLUDE_LOGGING
#ifndef CARBON_DISABLE_LOGGING
    #define CARBON_INCLUDE_LOGGING
#endif

// Silence compiler warnings that occur when logging is excluded from the build
#if !defined(CARBON_INCLUDE_LOGGING) && defined(_MSC_VER)
    #pragma warning(disable: 4101)    // 'identifier' : unreferenced local variable
#endif

// The CARBON_INCLUDE_CONSOLE_COMMANDS token toggles whether or not all of the built-in console commands are included in the
// build, it is defined automatically unless CARBON_DISABLE_CONSOLE_COMMANDS has been specified
#undef CARBON_INCLUDE_CONSOLE_COMMANDS
#ifndef CARBON_DISABLE_CONSOLE_COMMANDS
    #define CARBON_INCLUDE_CONSOLE_COMMANDS
#endif

namespace Carbon
{

// Forward declarations of all public classes
class CARBON_API AABB;
class CARBON_API ABTCompiler;
class CARBON_API AlphaFadeEntityController;
class CARBON_API Application;
class CARBON_API ApplicationGainFocusEvent;
class CARBON_API ApplicationLoseFocusEvent;
class CARBON_API AStarTraversal;
class CARBON_API BeforeTextureImageLoadEvent;
class CARBON_API BlockAllocator;
class CARBON_API BlockAllocatorSet;
class CARBON_API BuildInfo;
class CARBON_API Camera;
class CARBON_API ChangeTransformRenderQueueItem;
class CARBON_API CharacterInputEvent;
class CARBON_API Color;
class CARBON_API ComplexEntity;
class CARBON_API Console;
class CARBON_API ConsoleCommand;
class CARBON_API ConsoleTextChangedEvent;
class CARBON_API ConvexHull;
class CARBON_API CullingNode;
class CARBON_API DeviceShakeEvent;
class CARBON_API DrawGeometryChunkRenderQueueItem;
class CARBON_API DrawItem;
class CARBON_API DrawRectangleRenderQueueItem;
class CARBON_API DrawTextRenderQueueItem;
class CARBON_API Effect;
class CARBON_API EffectManager;
class CARBON_API EffectQueue;
class CARBON_API EffectQueueArray;
class CARBON_API Endian;
class CARBON_API Entity;
class CARBON_API EntityController;
class CARBON_API EntityEnterRegionEvent;
class CARBON_API EntityEventDetails;
class CARBON_API EntityExitRegionEvent;
class CARBON_API Event;
class CARBON_API EventHandler;
class CARBON_API EventManager;
class CARBON_API Exception;
class CARBON_API FileReader;
class CARBON_API FileSystem;
class CARBON_API FileSystemErrorEvent;
class CARBON_API FileSystemVolume;
class CARBON_API FileWriter;
class CARBON_API Font;
class CARBON_API FontLoadedEvent;
class CARBON_API FontManager;
class CARBON_API ForceFeedbackEffect;
class CARBON_API ForceFeedbackConstantForceEffect;
class CARBON_API ForceFeedbackRampForceEffect;
class CARBON_API ForceFeedbackPeriodicEffect;
class CARBON_API FrameBeginEvent;
#ifdef APPLE
    class CARBON_API GameCenter;
#endif
class CARBON_API GameControllerButtonDownEvent;
class CARBON_API GameControllerButtonUpEvent;
class CARBON_API GameControllerState;
class CARBON_API GatherMemorySummaryEvent;
class CARBON_API GeometryChunk;
class CARBON_API GeometryGather;
class CARBON_API Globals;
class CARBON_API GraphicsInterface;
class CARBON_API GridNavigationGraph;
class CARBON_API GUIButton;
class CARBON_API GUICombobox;
class CARBON_API GUIComboboxItemSelectEvent;
class CARBON_API GUIConsoleWindow;
class CARBON_API GUIEditbox;
class CARBON_API GUIEventDetails;
class CARBON_API GUIGainFocusEvent;
class CARBON_API GUILabel;
class CARBON_API GUILoseFocusEvent;
class CARBON_API GUIMouseButtonDownEvent;
class CARBON_API GUIMouseButtonUpEvent;
class CARBON_API GUIMouseEnterEvent;
class CARBON_API GUIMouseExitEvent;
class CARBON_API GUIMouseMoveEvent;
class CARBON_API GUIMousePointer;
class CARBON_API GUIProgressBar;
class CARBON_API GUISlider;
class CARBON_API GUISliderChangedEvent;
class CARBON_API GUIWindow;
class CARBON_API GUIWindowPressedEvent;
class CARBON_API HashFunctions;
class CARBON_API HexagonalNavigationGraph;
class CARBON_API Image;
class CARBON_API ImageFormatRegistry;
#ifdef APPLE
    class CARBON_API InAppPurchase;
#endif
class CARBON_API Interpolate;
class CARBON_API Intersection;
class CARBON_API IntersectionResult;
class CARBON_API KeyDownEvent;
class CARBON_API KeyUpEvent;
class CARBON_API Light;
class CARBON_API LightmapPacker;
class CARBON_API Line;
class CARBON_API LocalDirectoryFileSystemVolume;
class CARBON_API Logfile;
class CARBON_API LowMemoryWarningEvent;
class CARBON_API Material;
class CARBON_API MaterialManager;
class CARBON_API Matrix3;
class CARBON_API Matrix4;
class CARBON_API MemoryInterceptor;
class CARBON_API MemoryLeakDetector;
class CARBON_API MemoryStatistics;
class CARBON_API MemoryValidator;
class CARBON_API Mesh;
class CARBON_API MeshFormatRegistry;
class CARBON_API MeshManager;
class CARBON_API MouseButtonDownEvent;
class CARBON_API MouseButtonUpEvent;
class CARBON_API MouseMoveEvent;
class CARBON_API MouseWheelEvent;
class CARBON_API Mutex;
class CARBON_API NavigationEdge;
class CARBON_API NavigationGraph;
class CARBON_API NavigationNode;
class CARBON_API Noise;
class CARBON_API Noncopyable;
class CARBON_API OculusRiftCameraController;
class CARBON_API Parameter;
class CARBON_API ParameterArray;
class CARBON_API PeriodicTimer;
class CARBON_API PhysicsInterface;
class CARBON_API PhysicsIntersectResult;
class CARBON_API Plane;
class CARBON_API PlatformerEntityController;
class CARBON_API PlatformInterface;
class CARBON_API PlayerEntityController;
class CARBON_API Quaternion;
class CARBON_API RandomNumberGenerator;
class CARBON_API Ray;
class CARBON_API RecreateWindowEvent;
class CARBON_API Rect;
class CARBON_API Region;
class CARBON_API Renderer;
class CARBON_API RenderQueueItem;
class CARBON_API RenderQueueItemArray;
class CARBON_API RenderTarget;
class CARBON_API Resolution;
class CARBON_API ResizeEvent;
class CARBON_API Runnable;
class CARBON_API Scene;
class CARBON_API ScopedMutexLock;
class CARBON_API ScopedTimer;
class CARBON_API ScriptManager;
class CARBON_API ScrollingLayer;
class CARBON_API SettingsManager;
class CARBON_API Shader;
class CARBON_API ShaderChangeEvent;
class CARBON_API ShaderConstant;
class CARBON_API ShaderProgram;
class CARBON_API ShaderRegistry;
class CARBON_API SharedLibrary;
class CARBON_API ShutdownRequestEvent;
class CARBON_API SimpleFileSystemVolume;
class CARBON_API SimpleTimer;
class CARBON_API SimpleTransform;
class CARBON_API SkeletalAnimation;
class CARBON_API SkeletalMesh;
class CARBON_API SkyDome;
class CARBON_API SoundFormatRegistry;
class CARBON_API SoundInterface;
class CARBON_API SoundListener;
class CARBON_API SoundShader;
class CARBON_API SoundShaderChangedEvent;
class CARBON_API SoundShaderManager;
class CARBON_API SoundEmitter;
class CARBON_API Sphere;
class CARBON_API SplashScreen;
class CARBON_API Sprite;
class CARBON_API Terrain;
class CARBON_API TextInput;
class CARBON_API Texture;
class CARBON_API Texture2D;
class CARBON_API Texture3D;
class CARBON_API TextureCubemap;
class CARBON_API TextureLoadedEvent;
class CARBON_API TextureManager;
class CARBON_API TextureProperties;
class CARBON_API ThemeManager;
class CARBON_API Thread;
class CARBON_API TimeValue;
class CARBON_API TouchBeginEvent;
class CARBON_API TouchEndEvent;
class CARBON_API TouchMoveEvent;
class CARBON_API TouchPanEvent;
class CARBON_API TouchPinchEvent;
class CARBON_API TouchRotationEvent;
class CARBON_API TouchSwipeEvent;
class CARBON_API TouchTapEvent;
class CARBON_API Triangle;
class CARBON_API TriangleArray;
class CARBON_API TriangleArraySet;
class CARBON_API UpdateEvent;
class CARBON_API Vec2;
class CARBON_API Vec3;
class CARBON_API VersionInfo;
class CARBON_API VertexStream;
class CARBON_API DataBufferManager;
class CARBON_API World;

namespace States
{

class CARBON_API CachedState;
class CARBON_API StateCacher;

}

template <typename T> struct pointer_to
{
    typedef T* type;
};

template <typename ClassType, typename... ArgTypes> void initializeIfArgsPassed(ClassType* instance, ArgTypes&&... args)
{
    instance->initialize(std::forward<ArgTypes>(args)...);
}

template <typename ClassType> void initializeIfArgsPassed(ClassType* instance)
{
    // When no additional arguments are given there is no call to instance->initialize()
}

}

#if __cplusplus == 201103L

namespace std
{

// Backported from C++14, currently our target is C++11
template <typename T, typename... ArgTypes> std::unique_ptr<T> make_unique(ArgTypes&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<ArgTypes>(args)...));
}

}

#endif

#endif

/**
 * \file
 *
 * The primary namespace for the engine.
 */
namespace Carbon
{

/**
 * 8-bit unsigned integer typedef used for raw bytes.
 */
typedef uint8_t byte_t;

/**
 * Shorthand type for `unsigned int`, intended for use when casting to avoid the verbose `static_cast<unsigned int>()`.
 */
typedef unsigned int uint;

/**
 * Data type identifiers.
 */
enum DataType
{
    TypeNone,
    TypeInt8,
    TypeUInt8,
    TypeInt16,
    TypeUInt16,
    TypeInt32,
    TypeUInt32,
    TypeInt64,
    TypeUInt64,
    TypeFloat,
    TypeDouble
};

/**
 * Returns the size in bytes of the given DataType enumeration value.
 */
extern CARBON_API unsigned int getDataTypeSize(DataType dataType);

/**
 * Typedef for a pointer to a function that takes no arguments and returns nothing, this is used as a generic function pointer
 * type.
 */
typedef void (*VoidFunction)();

/**
 * Returns the global Console instance, this is shorthand for Globals::console().
 */
extern CARBON_API Console& console();

/**
 * Returns the global DataBufferManager instance, this is shorthand for Globals::dataBuffers().
 */
extern CARBON_API DataBufferManager& dataBuffers();

/**
 * Returns the global EffectManager instance, this is shorthand for Globals::effects().
 */
extern CARBON_API EffectManager& effects();

/**
 * Returns the global EventManager instance, this is shorthand for Globals::events().
 */
extern CARBON_API EventManager& events();

/**
 * Returns the global FileSystem instance, this is shorthand for Globals::fileSystem().
 */
extern CARBON_API FileSystem& fileSystem();

/**
 * Returns the global FontManager instance, this is shorthand for Globals::fonts().
 */
extern CARBON_API FontManager& fonts();

/**
 * Returns the global GraphicsInterface instance, this is shorthand for Globals::graphics().
 */
extern CARBON_API GraphicsInterface& graphics();

/**
 * Returns the global MaterialManager instance, this is shorthand for Globals::materials().
 */
extern CARBON_API MaterialManager& materials();

/**
 * Returns the global MeshManager instance, this is shorthand for Globals::meshes().
 */
extern CARBON_API MeshManager& meshes();

/**
 * Returns the global PhysicsInterface instance, this is shorthand for Globals::physics().
 */
extern CARBON_API PhysicsInterface& physics();

/**
 * Returns the global PlatformInterface instance, this is shorthand for Globals::platform().
 */
extern CARBON_API PlatformInterface& platform();

/**
 * Returns the global Renderer instance, this is shorthand for Globals::renderer().
 */
extern CARBON_API Renderer& renderer();

/**
 * Returns the global ScriptManager instance, this is shorthand for Globals::scripts().
 */
extern CARBON_API ScriptManager& scripts();

/**
 * Returns the global SettingsManager instance, this is shorthand for Globals::settings().
 */
extern CARBON_API SettingsManager& settings();

/**
 * Returns the global SoundInterface instance, this is shorthand for Globals::sounds().
 */
extern CARBON_API SoundInterface& sounds();

/**
 * Returns the global SoundShaderManager instance, this is shorthand for Globals::soundShaders().
 */
extern CARBON_API SoundShaderManager& soundShaders();

/**
 * Returns the global TextureManager instance, this is shorthand for Globals::textures().
 */
extern CARBON_API TextureManager& textures();

/**
 * Returns the global ThemeManager instance, this is shorthand for Globals::theme().
 */
extern CARBON_API ThemeManager& theme();

}

// Core headers
#include "CarbonEngine/Core/Noncopyable.h"
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
#include "CarbonEngine/Core/Vector.h"
#include "CarbonEngine/Core/StringBase.h"
#include "CarbonEngine/Core/Exception.h"
#include "CarbonEngine/Core/FileSystem/FileReader.h"
#include "CarbonEngine/Core/FileSystem/FileWriter.h"
#include "CarbonEngine/Core/Logfile.h"

#endif
