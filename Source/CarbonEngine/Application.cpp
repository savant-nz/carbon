/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Application.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/RenderEvents.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/SceneEvents.h"
#include "CarbonEngine/Scripting/ScriptManager.h"
#include "CarbonEngine/Sound/SoundEvents.h"
#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

CARBON_DEFINE_FRAME_TIMER(UpdateTimer, Color(0.7f, 0.8f, 1.0f))
CARBON_DEFINE_FRAME_TIMER(ApplicationTimer, Color(1.0f, 0.5f, 0.0f))

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    CARBON_DEFINE_FRAME_TIMER(HeapValidationTimer, Color(0.0f, 0.5f, 0.8f))
#endif

Application::Application()
{
    // Register for events
    events().addHandler<ApplicationGainFocusEvent>(this);
    events().addHandler<ApplicationLoseFocusEvent>(this);
    events().addHandler<BeforeTextureImageLoadEvent>(this);
    events().addHandler<CharacterInputEvent>(this);
    events().addHandler<ConsoleTextChangedEvent>(this);
    events().addHandler<DeviceShakeEvent>(this);
    events().addHandler<EntityEnterRegionEvent>(this);
    events().addHandler<EntityExitRegionEvent>(this);
    events().addHandler<FileSystemErrorEvent>(this);
    events().addHandler<FontLoadedEvent>(this);
    events().addHandler<FrameBeginEvent>(this);
    events().addHandler<GameControllerButtonDownEvent>(this);
    events().addHandler<GameControllerButtonUpEvent>(this);
    events().addHandler<GatherMemorySummaryEvent>(this);
    events().addHandler<GUIComboboxItemSelectEvent>(this);
    events().addHandler<GUIGainFocusEvent>(this);
    events().addHandler<GUILoseFocusEvent>(this);
    events().addHandler<GUIMouseButtonDownEvent>(this);
    events().addHandler<GUIMouseButtonUpEvent>(this);
    events().addHandler<GUIMouseEnterEvent>(this);
    events().addHandler<GUIMouseExitEvent>(this);
    events().addHandler<GUIMouseMoveEvent>(this);
    events().addHandler<GUISliderChangedEvent>(this);
    events().addHandler<GUIWindowPressedEvent>(this);
    events().addHandler<KeyDownEvent>(this);
    events().addHandler<KeyUpEvent>(this);
    events().addHandler<LowMemoryWarningEvent>(this);
    events().addHandler<MouseButtonDownEvent>(this);
    events().addHandler<MouseButtonUpEvent>(this);
    events().addHandler<MouseMoveEvent>(this);
    events().addHandler<MouseWheelEvent>(this);
    events().addHandler<ResizeEvent>(this);
    events().addHandler<ShutdownRequestEvent>(this);
    events().addHandler<TextureLoadedEvent>(this);
    events().addHandler<TouchBeginEvent>(this);
    events().addHandler<TouchEndEvent>(this);
    events().addHandler<TouchMoveEvent>(this);
    events().addHandler<TouchPanEvent>(this);
    events().addHandler<TouchPinchEvent>(this);
    events().addHandler<TouchRotationEvent>(this);
    events().addHandler<TouchSwipeEvent>(this);
    events().addHandler<TouchTapEvent>(this);
    events().addHandler<UpdateEvent>(this);
}

Application::~Application()
{
    events().removeHandler(this);
}

bool Application::run(bool doMainLoop)
{
    try
    {
        // Setup platform layer
        if (!platform().setup())
            throw PlatformInitializationFailed;

        // Startup the application
        if (!startup())
            throw ApplicationStartupMethodFailed;

        setupAssetDirectories();

        // Create window
        platform().setWindowTitle(Globals::getClientName());
        if (!platform().createWindow())
            throw WindowCreationFailed;

        // Setup renderer
        if (!renderer().setup())
            throw RendererInitializationFailed;

        // Setup sound manager
        if (!sounds().setup())
            throw SoundInitializationFailed;

        // Setup scripting
        if (!scripts().setup())
            throw ScriptingInitializationFailed;

        // Initialize application
        if (!initialize())
            throw ApplicationInitializeMethodFailed;

        LOG_INFO << "Application initialized successfully";

        isInitialized_ = true;
    }
    catch (InitializationFailureReason reason)
    {
        onInitializationFailed(reason);

        shutdown();

        return false;
    }

    if (doMainLoop)
    {
        LOG_INFO << "Entered application loop";

        while (mainLoop())
            ;

        LOG_INFO << "Exited application loop";

        validateHeap();

        shutdown();
        isInitialized_ = false;
    }

    validateHeap();

    return true;
}

void Application::setupAssetDirectories()
{
#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    fileSystem().addLocalAssetDirectory("./Assets");

#ifdef APPLE
    if (Globals::getExecutableName().find(".app/") != -1)
    {
        // On Apple platforms add the application bundle's Resources folder as an asset directory
        auto resourcesDirectory = FileSystem::getApplicationResourcesDirectory();
        fileSystem().addLocalAssetDirectory(resourcesDirectory);

        // Also add any immediate subdirectories of the bundle's Resources folder as asset directories if if they have "assets"
        // in their name
        auto subdirectories = Vector<UnicodeString>();
        FileSystem::enumerateLocalDirectories(resourcesDirectory, false, subdirectories);
        for (const auto& subdirectory : subdirectories)
        {
            if (subdirectory.asLower().find("assets") != -1)
                fileSystem().addLocalAssetDirectory(subdirectory);
        }
    }
    else
#endif
        fileSystem().addLocalAssetDirectory(".");

#ifdef WINDOWS
    // On Windows automatically pull in the sample assets from the SDK if this is a sample application
    if (Globals::getClientName().endsWith("Sample"))
        fileSystem().addSDKSampleAssetsDirectory();
#endif

#endif
}

void Application::onInitializationFailed(InitializationFailureReason reason)
{
    auto error = UnicodeString("Failed ");

    switch (reason)
    {
        case PlatformInitializationFailed:
            error << "initializing platform layer";
            break;
        case ApplicationStartupMethodFailed:
            error << "starting up application";
            break;
        case WindowCreationFailed:
            error << "creating main window";
            break;
        case RendererInitializationFailed:
            error << "initializing renderer";
            break;
        case SoundInitializationFailed:
            error << "initializing sound interface";
            break;
        case ScriptingInitializationFailed:
            error << "initializing scripting engine";
            break;
        case ApplicationInitializeMethodFailed:
            error << "initializing application";
            break;
    };

    LOG_ERROR_WITHOUT_CALLER << error;

#if defined(CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS) && !defined(iOS) && !defined(ANDROID)
    if (Logfile::Enabled)
    {
        if (platform().showMessageBox(error + ".\n\nPress OK to open the logfile or Cancel to exit.", Globals::getClientName(),
                                      PlatformInterface::OKCancelButtons, PlatformInterface::ErrorIcon))
            platform().openWithDefaultApplication(Logfile::get().getFilename());
    }
    else
#endif
        platform().showMessageBox(error, Globals::getClientName(), PlatformInterface::OKButton, PlatformInterface::ErrorIcon);
}

bool Application::mainLoop()
{
    if (isExiting_)
        return false;

    events().dispatchEvent(FrameBeginEvent());

    validateHeap();

    // Internal updates
    {
        auto timer = ScopedFrameTimer(UpdateTimer);

        physics().update(platform().getTimePassed());

        events().dispatchQueuedEvents();
        events().dispatchEvent(UpdateEvent());
    }

    if (isExiting_)
        return false;

    // Application callbacks
    {
        auto timer = ScopedFrameTimer(ApplicationTimer);

        frameUpdate();
        queueScenes();
    }

    if (isExiting_)
        return false;

    renderer().render();

    return true;
}

bool Application::processEvent(const Event& e)
{

#define CALL_EVENT_HANDLER(Type)           \
    if (auto eventSubclass = e.as<Type>()) \
        return on##Type(*eventSubclass);

    CALL_EVENT_HANDLER(ApplicationGainFocusEvent)
    CALL_EVENT_HANDLER(ApplicationLoseFocusEvent)
    CALL_EVENT_HANDLER(BeforeTextureImageLoadEvent)
    CALL_EVENT_HANDLER(CharacterInputEvent)
    CALL_EVENT_HANDLER(DeviceShakeEvent)
    CALL_EVENT_HANDLER(EntityEnterRegionEvent)
    CALL_EVENT_HANDLER(EntityExitRegionEvent)
    CALL_EVENT_HANDLER(FileSystemErrorEvent)
    CALL_EVENT_HANDLER(FontLoadedEvent)
    CALL_EVENT_HANDLER(FrameBeginEvent)
    CALL_EVENT_HANDLER(GameControllerButtonDownEvent)
    CALL_EVENT_HANDLER(GameControllerButtonUpEvent)
    CALL_EVENT_HANDLER(GatherMemorySummaryEvent)
    CALL_EVENT_HANDLER(GUIComboboxItemSelectEvent)
    CALL_EVENT_HANDLER(GUIGainFocusEvent)
    CALL_EVENT_HANDLER(GUILoseFocusEvent)
    CALL_EVENT_HANDLER(GUIMouseButtonDownEvent)
    CALL_EVENT_HANDLER(GUIMouseButtonUpEvent)
    CALL_EVENT_HANDLER(GUIMouseEnterEvent)
    CALL_EVENT_HANDLER(GUIMouseExitEvent)
    CALL_EVENT_HANDLER(GUIMouseMoveEvent)
    CALL_EVENT_HANDLER(GUISliderChangedEvent)
    CALL_EVENT_HANDLER(GUIWindowPressedEvent)
    CALL_EVENT_HANDLER(KeyDownEvent)
    CALL_EVENT_HANDLER(KeyUpEvent)
    CALL_EVENT_HANDLER(LowMemoryWarningEvent)
    CALL_EVENT_HANDLER(MouseButtonDownEvent)
    CALL_EVENT_HANDLER(MouseButtonUpEvent)
    CALL_EVENT_HANDLER(MouseMoveEvent)
    CALL_EVENT_HANDLER(MouseWheelEvent)
    CALL_EVENT_HANDLER(ResizeEvent)
    CALL_EVENT_HANDLER(ShutdownRequestEvent)
    CALL_EVENT_HANDLER(TextureLoadedEvent)
    CALL_EVENT_HANDLER(TouchBeginEvent)
    CALL_EVENT_HANDLER(TouchEndEvent)
    CALL_EVENT_HANDLER(TouchMoveEvent)
    CALL_EVENT_HANDLER(TouchPanEvent)
    CALL_EVENT_HANDLER(TouchPinchEvent)
    CALL_EVENT_HANDLER(TouchRotationEvent)
    CALL_EVENT_HANDLER(TouchSwipeEvent)
    CALL_EVENT_HANDLER(TouchTapEvent)
    CALL_EVENT_HANDLER(UpdateEvent)

    return true;
}

void Application::validateHeap()
{
#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
    auto timer = ScopedFrameTimer(HeapValidationTimer);
    MemoryInterceptor::validateAllAllocations();
#endif
}

}
