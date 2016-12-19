/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/DataBufferManager.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Mesh/MeshManager.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scripting/ScriptManager.h"
#include "CarbonEngine/Sound/SoundInterface.h"
#include "CarbonEngine/Sound/SoundShaderManager.h"

#include "CarbonEngine/ConsoleCommands.h"
#include "CarbonEngine/EngineAssets.h"

namespace Carbon
{

Vector<Globals::PrioritizedFunction>* Globals::runAtStartup_;
Vector<Globals::PrioritizedFunction>* Globals::runAtShutdown_;

unsigned int Globals::leakedResourceCount_;

Vector<UnicodeString> Globals::commandLineParameters_;
UnicodeString Globals::executableName_;
unsigned int Globals::exitCode_;

bool Globals::isInitialized_;
bool Globals::inStaticInitialization_ = true;
std::array<char, 256> Globals::clientNameBuffer_ = {};
String Globals::clientName_;

// Define globals
#define DEFINE_GLOBAL(Type, MemberVariable, AccessorMethod) \
    Type* Globals::MemberVariable;                          \
    CARBON_API Type& AccessorMethod() { return Globals::AccessorMethod(); }

DEFINE_GLOBAL(Console, console_, console)
DEFINE_GLOBAL(DataBufferManager, dataBufferManager_, dataBuffers)
DEFINE_GLOBAL(EffectManager, effectManager_, effects)
DEFINE_GLOBAL(EventManager, eventManager_, events)
DEFINE_GLOBAL(FileSystem, fileSystem_, fileSystem)
DEFINE_GLOBAL(FontManager, fontManager_, fonts)
DEFINE_GLOBAL(GraphicsInterface, graphicsInterface_, graphics)
DEFINE_GLOBAL(MaterialManager, materialManager_, materials)
DEFINE_GLOBAL(MeshManager, meshManager_, meshes)
DEFINE_GLOBAL(PhysicsInterface, physicsInterface_, physics)
DEFINE_GLOBAL(PlatformInterface, platformInterface_, platform)
DEFINE_GLOBAL(Renderer, renderer_, renderer)
DEFINE_GLOBAL(ScriptManager, scriptManager_, scripts)
DEFINE_GLOBAL(SettingsManager, settingsManager_, settings)
DEFINE_GLOBAL(SoundInterface, soundInterface_, sounds)
DEFINE_GLOBAL(SoundShaderManager, soundShaderManager_, soundShaders)
DEFINE_GLOBAL(TextureManager, textureManager_, textures)
DEFINE_GLOBAL(ThemeManager, themeManager_, theme)

#undef DEFINE_GLOBAL

#ifndef CONSOLE
// This logfile output sink is active for the duration of the engine's execution on all non-console platforms. On
// Windows, macOS and Linux it logs all warnings, errors and debug output to stderr, and on iOS all log output is sent
// straight to stdout. On Windows all output is also sent to the debugger output. Console platforms handle their own
// log display.
static class LogfileOutputPrinter : public Logfile::OutputSink
{
public:

    void processLogfileOutput(Logfile::OutputType type, const UnicodeString& line) override
    {
#ifdef iOS
        std::cout << line.toUTF8().as<char>() << String::Newline.cStr();
#else
        if (type == Logfile::Debug || type == Logfile::Error || type == Logfile::Warning)
        {
            auto full = line + UnicodeString::Newline;

#ifdef WINDOWS
            auto utf16 = full.toUTF16();
            std::wcerr << utf16.as<wchar_t>();
            OutputDebugStringW(utf16.as<wchar_t>());
#elif defined(CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS)
            std::cerr << full.toUTF8().as<char>();
#endif
        }
#endif
    }
} logfileOutputPrinter;
#endif

bool Globals::initializeEngine(const String& clientName)
{
    if (isInitialized_)
        return true;

    setExitCode(0);

#ifdef WINDOWS
    // Initialize the standard set of common controls
    auto commonControls = INITCOMMONCONTROLSEX();
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&commonControls);
#endif

#ifndef CONSOLE
    Logfile::addOutputSink(&logfileOutputPrinter);
#endif

    leakedResourceCount_ = 0;

    // Check validity of client name, everything before the last colon is removed in order to strip out any namespaces
    // that may be present
    clientName_ = clientName.substr(clientName.findLastOf(":") + 1);
    if (!clientName_.length() || !clientName_.isAlphaNumeric("_") || clientName_.length() >= clientNameBuffer_.size())
    {
        debugLog("Invalid client name: %s. Only letters, numbers and underscores are allowed. Maximum length is %i "
                 "characters.",
                 clientName.cStr(), int(clientNameBuffer_.size() - 1));
        return false;
    }

    // Copy the client name into the clientNameBuffer_ buffer, this is exposed through Globals::getClientNameBuffer() so
    // that the client name can be queried during static deinitialization, e.g. when writing the memory leaks report
    // HTML file header
    strncpy(clientNameBuffer_.data(), clientName_.cStr(), clientNameBuffer_.size() - 1);
    clientNameBuffer_.back() = 0;

    // Write the build info to the main logfile
    Logfile::get().writeCollapsibleSection("Carbon Build Info", U(BuildInfo::getBuildInfo()));

    // Log the executable name and command line parameters
    LOG_INFO << "Executable name: " << getExecutableName();
    LOG_INFO << "Command line parameters: " << UnicodeString(Globals::getCommandLineParameters(), " ");

    LOG_INFO << "Graphics interfaces: " << InterfaceRegistry<GraphicsInterface>::getImplementationNames();
    LOG_INFO << "Physics interfaces: " << InterfaceRegistry<PhysicsInterface>::getImplementationNames();
    LOG_INFO << "Platform interfaces: " << InterfaceRegistry<PlatformInterface>::getImplementationNames();
    LOG_INFO << "Sound interfaces: " << InterfaceRegistry<SoundInterface>::getImplementationNames();

    // Core
    fileSystem_ = new FileSystem;
    eventManager_ = new EventManager;
    settingsManager_ = new SettingsManager;

    // Platform
    console_ = new Console;
    platformInterface_ = InterfaceRegistry<PlatformInterface>::create();
    themeManager_ = new ThemeManager;

    // Graphics
    graphicsInterface_ = InterfaceRegistry<GraphicsInterface>::create();

    // Render
    dataBufferManager_ = new DataBufferManager;
    effectManager_ = new EffectManager;
    textureManager_ = new TextureManager;
    fontManager_ = new FontManager;
    renderer_ = new Renderer;

    // Sound
    soundInterface_ = InterfaceRegistry<SoundInterface>::create();
    soundShaderManager_ = new SoundShaderManager;

    // Physics
    physicsInterface_ = InterfaceRegistry<PhysicsInterface>::create();

    // Scene
    materialManager_ = new MaterialManager;
    meshManager_ = new MeshManager;

    // Scripting
    scriptManager_ = new ScriptManager;

    isInitialized_ = true;

    // Run the startup functions
    if (runAtStartup_)
    {
        LOG_INFO << "Running " << runAtStartup_->size() << " startup functions";

        runAtStartup_->sortBy(std::greater<PrioritizedFunction>());
        for (const auto& startupFn : *runAtStartup_)
            startupFn.second();
    }

    // Report if the $SAVE$ file system volume is missing
    if (!fileSystem().getVolume("SAVE"))
        LOG_INFO << "No $SAVE$ file system volume has been defined for this platform";

    return true;
}

void Globals::uninitializeEngine()
{
    if (!isInitialized_)
        return;

    if (runAtShutdown_)
    {
        runAtShutdown_->sortBy(std::greater<PrioritizedFunction>());
        for (const auto& shutdownFn : *runAtShutdown_)
            shutdownFn.second();
    }

    // Log warnings about any leaked scenes
    Globals::increaseLeakedResourceCount(Scene::getAllScenes().size());
    for (auto scene : Scene::getAllScenes())
        LOG_WARNING_WITHOUT_CALLER << "Leaked scene '" << scene->getName();

    // Scripting
    delete scriptManager_;
    scriptManager_ = nullptr;

    // Scene
    delete meshManager_;
    meshManager_ = nullptr;
    delete materialManager_;
    materialManager_ = nullptr;

    // Physics
    InterfaceRegistry<PhysicsInterface>::destroy();
    physicsInterface_ = nullptr;

    // Sound
    sounds().clear();
    delete soundShaderManager_;
    soundShaderManager_ = nullptr;
    InterfaceRegistry<SoundInterface>::destroy();
    soundInterface_ = nullptr;

    // Render
    delete renderer_;
    renderer_ = nullptr;
    delete fontManager_;
    fontManager_ = nullptr;
    delete effectManager_;
    effectManager_ = nullptr;
    delete textureManager_;
    textureManager_ = nullptr;
    delete dataBufferManager_;
    dataBufferManager_ = nullptr;

    // Graphics
    InterfaceRegistry<GraphicsInterface>::destroy();
    graphicsInterface_ = nullptr;

    // Platform
    delete themeManager_;
    themeManager_ = nullptr;
    InterfaceRegistry<PlatformInterface>::destroy();
    platformInterface_ = nullptr;
    delete console_;
    console_ = nullptr;

    // Core
    delete settingsManager_;
    settingsManager_ = nullptr;
    delete eventManager_;
    eventManager_ = nullptr;
    delete fileSystem_;
    fileSystem_ = nullptr;

    clientName_.clear();

    isInitialized_ = false;

    LOG_INFO << "Engine uninitialized";
}

void Globals::recreateGraphicsInterface()
{
    InterfaceRegistry<GraphicsInterface>::destroy();
    graphicsInterface_ = InterfaceRegistry<GraphicsInterface>::create();
}

#ifdef WINDOWS

HINSTANCE Globals::getHInstance()
{
    static auto hInstance = HINSTANCE();

    if (!hInstance)
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, LPCTSTR(&hInstance), &hInstance);

    return hInstance;
}

#endif

const String& Globals::getClientName()
{
    if (isInStaticInitialization())
        debugLog("Warning: a call to Globals::getClientName() was made during static initialization.");

    return clientName_;
}

#if defined(CARBON_INCLUDE_LOGGING) && !defined(CONSOLE) && !defined(ANDROID)

void Globals::debugLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);

#ifdef WINDOWS
    auto buffer = std::array<char, 4096>();
    vsnprintf(buffer.data(), buffer.size() - 1, format, args);

    OutputDebugStringA(buffer.data());
    OutputDebugStringA("\n");
#endif
}

#endif

void Globals::addStartupFunction(VoidFunction fn, int priority)
{
    if (!runAtStartup_)
        runAtStartup_ = new Vector<PrioritizedFunction>;

    runAtStartup_->emplace(priority, fn);
}

void Globals::removeStartupFunction(VoidFunction fn)
{
    if (!runAtStartup_)
        return;

    runAtStartup_->eraseIf([&](const PrioritizedFunction& p) { return p.second == fn; });

    if (runAtStartup_->empty())
    {
        delete runAtStartup_;
        runAtStartup_ = nullptr;
    }
}

void Globals::addShutdownFunction(VoidFunction fn, int priority)
{
    if (!runAtShutdown_)
        runAtShutdown_ = new Vector<PrioritizedFunction>;

    runAtShutdown_->emplace(priority, fn);
}

void Globals::removeShutdownFunction(VoidFunction fn)
{
    if (!runAtShutdown_)
        return;

    runAtShutdown_->eraseIf([&](const PrioritizedFunction& p) { return p.second == fn; });

    if (runAtShutdown_->empty())
    {
        delete runAtShutdown_;
        runAtShutdown_ = nullptr;
    }
}

void Globals::setCommandLineParameters(int argc, const char** argv)
{
    commandLineParameters_.clear();

    executableName_ = argv[0];

    for (auto i = 1; i < argc; i++)
        commandLineParameters_.append(argv[i]);

#ifdef MACOS
    // Remove the process serial number that macOS passes on the command line when an application is run from the Finder
    // or Dock
    commandLineParameters_.eraseIf([](const UnicodeString& s) { return s.startsWith("-psn_"); });
#endif
}

const UnicodeString& Globals::getExecutableName()
{
#ifdef WINDOWS
    if (!executableName_.length())
    {
        auto path = std::array<wchar_t, MAX_PATH>();
        if (GetModuleFileNameW(0, path.data(), path.size()))
            executableName_ = fromUTF16(path.data());
    }
#endif

    return executableName_;
}

unsigned int getDataTypeSize(DataType dataType)
{
    if (dataType == TypeNone)
        return 0;

    if (dataType == TypeInt8 || dataType == TypeUInt8)
        return 1;

    if (dataType == TypeInt16 || dataType == TypeUInt16)
        return 2;

    if (dataType == TypeInt32 || dataType == TypeUInt32 || dataType == TypeFloat)
        return 4;

    if (dataType == TypeInt64 || dataType == TypeUInt64 || dataType == TypeDouble)
        return 8;

    assert(false && "Unknown data type");

    return 0;
}

}

// Register all of the built-in entity classes
#include "CarbonEngine/Scene/Camera.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::Camera)
#include "CarbonEngine/Scene/ComplexEntity.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::ComplexEntity)
#include "CarbonEngine/Scene/CullingNode.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::CullingNode)
#include "CarbonEngine/Scene/Entity.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::Entity)
#include "CarbonEngine/Scene/GUI/GUIButton.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUIButton)
#include "CarbonEngine/Scene/GUI/GUICombobox.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUICombobox)
#include "CarbonEngine/Scene/GUI/GUIConsoleWindow.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUIConsoleWindow)
#include "CarbonEngine/Scene/GUI/GUIEditbox.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUIEditbox)
#include "CarbonEngine/Scene/GUI/GUILabel.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUILabel)
#include "CarbonEngine/Scene/GUI/GUIMousePointer.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUIMousePointer)
#include "CarbonEngine/Scene/GUI/GUIProgressBar.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUIProgressBar)
#include "CarbonEngine/Scene/GUI/GUISlider.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUISlider)
#include "CarbonEngine/Scene/GUI/GUIWindow.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::GUIWindow)
#include "CarbonEngine/Scene/Light.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::Light)
#include "CarbonEngine/Scene/Region.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::Region)
#include "CarbonEngine/Scene/SkeletalMesh.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::SkeletalMesh)
#include "CarbonEngine/Scene/SkyDome.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::SkyDome)
#include "CarbonEngine/Scene/SoundEmitter.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::SoundEmitter)
#include "CarbonEngine/Scene/SoundListener.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::SoundListener)
#include "CarbonEngine/Scene/Terrain.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::Terrain)

#include "CarbonEngine/Game/ScrollingLayer.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::ScrollingLayer)
#include "CarbonEngine/Game/Sprite.h"
CARBON_REGISTER_ENTITY_SUBCLASS(Carbon::Sprite)

// Register all of the built-in entity controller classes
#include "CarbonEngine/Scene/EntityController/AlphaFadeEntityController.h"
CARBON_REGISTER_ENTITY_CONTROLLER_SUBCLASS(Carbon::AlphaFadeEntityController)
#include "CarbonEngine/Scene/EntityController/PlatformerEntityController.h"
CARBON_REGISTER_ENTITY_CONTROLLER_SUBCLASS(Carbon::PlatformerEntityController)
#include "CarbonEngine/Scene/EntityController/PlayerEntityController.h"
CARBON_REGISTER_ENTITY_CONTROLLER_SUBCLASS(Carbon::PlayerEntityController)
#include "CarbonEngine/Scene/EntityController/SetOrientationEntityController.h"
CARBON_REGISTER_ENTITY_CONTROLLER_SUBCLASS(Carbon::SetOrientationEntityController)
#include "CarbonEngine/Scene/EntityController/TargetPositionEntityController.h"
CARBON_REGISTER_ENTITY_CONTROLLER_SUBCLASS(Carbon::TargetPositionEntityController)
