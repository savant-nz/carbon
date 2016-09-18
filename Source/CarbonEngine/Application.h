/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Memory/MemoryStatistics.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Render/RenderEvents.h"

namespace Carbon
{

/**
 * Base class for Carbon applications that streamlines common engine setup work and provides a base application
 * structure for updating, responding to events, and other common tasks. Applications should not inherit from this class
 * directly but rather they should inherit from the class defined by the CARBON_APPLICATION_CLASS macro, which is
 * Carbon::Application by default. This is because external platform modules may add extra functionality specific to
 * that platform into a Carbon::Application subclass and then redefine the CARBON_APPLICATION_CLASS macro to point to
 * that new class.
 */
class CARBON_API Application : public EventHandler, private Noncopyable
{
public:

    Application();
    ~Application() override;

    /**
     * This method is called on application startup following engine initialization with Globals::initializeEngine() and
     * before any other code is run. It can be used by applications to alter engine parameters prior to the creation of
     * rendering windows and other core startup procedures that will occur following base initialization. If the return
     * value is false then application startup will be aborted and Application::onInitializationFailed() will be called.
     */
    virtual bool startup() { return true; }

    /**
     * This method sets the default assets directories: "./Assets" and "./". Applications can alter this if they require
     * additional or alternative asset directories.
     */
    virtual void setupAssetDirectories();

    /**
     * This method does application-specific setup. If false is returned then application startup will be aborted and
     * Application::onInitializationFailed() will be called.
     */
    virtual bool initialize() { return true; }

    /**
     * Called every frame to update the application.
     */
    virtual void frameUpdate() {}

    /**
     * Called every frame to tell the application to queue scenes that should be rendered. Scenes are queued using
     * Scene::queueForRendering().
     */
    virtual void queueScenes() {}

    /**
     * Cleans up all application resources prior to shutdown.
     */
    virtual void shutdown() {}

    /**
     * The list of reasons for application or engine initialization failure, if such a failure occurs then
     * Application::onInitializationFailed() is called and passed one of the following reasons. The values in this
     * enumeration appear in the same order as they are executed during initialization.
     */
    enum InitializationFailureReason
    {
        PlatformInitializationFailed,
        ApplicationStartupMethodFailed,
        WindowCreationFailed,
        RendererInitializationFailed,
        SoundInitializationFailed,
        ScriptingInitializationFailed,
        ApplicationInitializeMethodFailed
    };

    /**
     * This method is called when application or engine startup fails, the reason for the failure is given by \a reason.
     * The default implementation of this method will report the error using a message box and offer to open the logfile
     * if one was created. Applications can override this method to customize the handling of application or engine
     * startup errors.
     */
    virtual void onInitializationFailed(InitializationFailureReason reason);

    /**
     * Entry point where execution control is passed to the application class, see the Carbon::main() function that is
     * defined by EntryPoint.h for details. By default this method will not return until the application has finished
     * running, i.e. the main loop will be run inside this method. However, if \a doMainLoop is set to false then the
     * main loop will not be run by this method, and the user of this class is then responsible for calling
     * Application::mainLoop() at regular intervals to keep the appliation active and responsive. This separation allows
     * main loops to be run based on external timers which can then do things such as run the main loop 30 times a
     * second and allow the hardware to sleep in between.
     */
    bool run(bool doMainLoop = true);

    /**
     * Runs a single tick of the main application/game loop. Returns whether or not execution should continue or whether
     * the application should now exit. This method should only ever be called when the \a doMainLoop parameter to
     * Application::run() was set to false.
     */
    bool mainLoop();

    /**
     * Returns whether the application has successfully initialized (i.e. Application::initialize() has succeeded).
     */
    bool isInitialized() const { return isInitialized_; }

    /**
     * Returns whether the application's internal exit flag is set which will cause it to break out of the main loop as
     * soon as possible and end the application.
     */
    virtual bool isExiting() const { return isExiting_; }

    /**
     * Handler for ApplicationGainFocusEvent.
     */
    virtual bool onApplicationGainFocusEvent(const ApplicationGainFocusEvent& e) { return true; }

    /**
     * Handler for ApplicationLoseFocusEvent.
     */
    virtual bool onApplicationLoseFocusEvent(const ApplicationLoseFocusEvent& e) { return true; }

    /**
     * Handler for BeforeTextureImageLoadEvent.
     */
    virtual bool onBeforeTextureImageLoadEvent(const BeforeTextureImageLoadEvent& e) { return true; }

    /**
     * Handler for CharacterInputEvent.
     */
    virtual bool onCharacterInputEvent(const CharacterInputEvent& e) { return true; }

    /**
     * Handler for ConsoleTextChangedEvent.
     */
    virtual bool onConsoleTextChangedEvent(const ConsoleTextChangedEvent& e) { return true; }

    /**
     * Handler for DeviceShakeEvent.
     */
    virtual bool onDeviceShakeEvent(const DeviceShakeEvent& e) { return true; }

    /**
     * Handler for EntityEnterRegionEvent.
     */
    virtual bool onEntityEnterRegionEvent(const EntityEnterRegionEvent& e) { return true; }

    /**
     * Handler for EntityExitRegionEvent.
     */
    virtual bool onEntityExitRegionEvent(const EntityExitRegionEvent& e) { return true; }

    /**
     * Handler for FileSystemErrorEvent.
     */
    virtual bool onFileSystemErrorEvent(const FileSystemErrorEvent& e) { return true; }

    /**
     * Handler for FontLoadedEvent.
     */
    virtual bool onFontLoadedEvent(const FontLoadedEvent& e) { return true; }

    /**
     * Handler for FrameBeginEvent.
     */
    virtual bool onFrameBeginEvent(const FrameBeginEvent& e) { return true; }

    /**
     * Handler for GameControllerButtonDownEvent.
     */
    virtual bool onGameControllerButtonDownEvent(const GameControllerButtonDownEvent& e) { return true; }

    /**
     * Handler for GameControllerButtonUpEvent.
     */
    virtual bool onGameControllerButtonUpEvent(const GameControllerButtonUpEvent& e) { return true; }

    /**
     * Handler for GatherMemorySummaryEvent.
     */
    virtual bool onGatherMemorySummaryEvent(const GatherMemorySummaryEvent& e) { return true; }

    /**
     * Handler for GUIComboboxItemSelectEvent.
     */
    virtual bool onGUIComboboxItemSelectEvent(const GUIComboboxItemSelectEvent& e) { return true; }

    /**
     * Handler for GUIGainFocusEvent.
     */
    virtual bool onGUIGainFocusEvent(const GUIGainFocusEvent& e) { return true; }

    /**
     * Handler for GUILoseFocusEvent.
     */
    virtual bool onGUILoseFocusEvent(const GUILoseFocusEvent& e) { return true; }

    /**
     * Handler for GUIMouseButtonDownEvent.
     */
    virtual bool onGUIMouseButtonDownEvent(const GUIMouseButtonDownEvent& e) { return true; }

    /**
     * Handler for GUIMouseButtonUpEvent.
     */
    virtual bool onGUIMouseButtonUpEvent(const GUIMouseButtonUpEvent& e) { return true; }

    /**
     * Handler for GUIMouseEnterEvent.
     */
    virtual bool onGUIMouseEnterEvent(const GUIMouseEnterEvent& e) { return true; }

    /**
     * Handler for GUIMouseExitEvent.
     */
    virtual bool onGUIMouseExitEvent(const GUIMouseExitEvent& e) { return true; }

    /**
     * Handler for GUIMouseMoveEvent.
     */
    virtual bool onGUIMouseMoveEvent(const GUIMouseMoveEvent& e) { return true; }

    /**
     * Handler for GUISliderChangedEvent.
     */
    virtual bool onGUISliderChangedEvent(const GUISliderChangedEvent& e) { return true; }

    /**
     * Handler for GUIWindowPressedEvent.
     */
    virtual bool onGUIWindowPressedEvent(const GUIWindowPressedEvent& e) { return true; }

    /**
     * Handler for onKeyDownEvent.
     */
    virtual bool onKeyDownEvent(const KeyDownEvent& e)
    {
        if (e.getKey() == KeyEscape)
            isExiting_ = true;

        return true;
    }

    /**
     * Handler for KeyUpEvent.
     */
    virtual bool onKeyUpEvent(const KeyUpEvent& e) { return true; }

    /**
     * Handler for LowMemoryWarningEvent.
     */
    virtual bool onLowMemoryWarningEvent(const LowMemoryWarningEvent& e)
    {
        GatherMemorySummaryEvent::report();

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
        MemoryStatistics::logAllocationDetails();
#endif

        return true;
    }

    /**
     * Handler for MouseButtonDownEvent.
     */
    virtual bool onMouseButtonDownEvent(const MouseButtonDownEvent& e) { return true; }

    /**
     * Handler for MouseButtonUpEvent.
     */
    virtual bool onMouseButtonUpEvent(const MouseButtonUpEvent& e) { return true; }

    /**
     * Handler for MouseMoveEvent.
     */
    virtual bool onMouseMoveEvent(const MouseMoveEvent& e) { return true; }

    /**
     * Handler for MouseWheelEvent.
     */
    virtual bool onMouseWheelEvent(const MouseWheelEvent& e) { return true; }

    /**
     * Handler for ResizeEvent.
     */
    virtual bool onResizeEvent(const ResizeEvent& e) { return true; }

    /**
     * Handler for ShutdownRequestEvent.
     */
    virtual bool onShutdownRequestEvent(const ShutdownRequestEvent& e)
    {
        isExiting_ = true;
        return true;
    }

    /**
     * Handler for TextureLoadedEvent.
     */
    virtual bool onTextureLoadedEvent(const TextureLoadedEvent& e)
    {
        // Unsupported pixel formats fallback here based on a recommendation from the graphics interface
        e.setNewPixelFormat(graphics().getFallbackPixelFormat(e.getTextureType(), e.getNewPixelFormat()));

        return true;
    }

    /**
     * Handler for TouchBeginEvent.
     */
    virtual bool onTouchBeginEvent(const TouchBeginEvent& e) { return true; }

    /**
     * Handler for TouchEndEvent.
     */
    virtual bool onTouchEndEvent(const TouchEndEvent& e) { return true; }

    /**
     * Handler for TouchMoveEvent.
     */
    virtual bool onTouchMoveEvent(const TouchMoveEvent& e) { return true; }

    /**
     * Handler for TouchPanEvent.
     */
    virtual bool onTouchPanEvent(const TouchPanEvent& e) { return true; }

    /**
     * Handler for TouchPinchEvent.
     */
    virtual bool onTouchPinchEvent(const TouchPinchEvent& e) { return true; }

    /**
     * Handler for TouchRotationEvent.
     */
    virtual bool onTouchRotationEvent(const TouchRotationEvent& e) { return true; }

    /**
     * Handler for TouchSwipeEvent.
     */
    virtual bool onTouchSwipeEvent(const TouchSwipeEvent& e) { return true; }

    /**
     * Handler for TouchTapEvent.
     */
    virtual bool onTouchTapEvent(const TouchTapEvent& e) { return true; }

    /**
     * Handler for UpdateEvent.
     */
    virtual bool onUpdateEvent(const UpdateEvent& e) { return true; }

protected:

    /**
     * Passes events to the event handler methods.
     */
    bool processEvent(const Event& e) override;

    /**
     * This method is called at the start of every frame in order to validate the main heap, however it is only active
     * when the memory interceptor is included in the build and so does nothing in release builds. Applications can
     * override this method to alter or eliminate the automatic per-frame heap validation.
     */
    virtual void validateHeap();

    /**
     * When this value is true the main loop will stop and the application will terminate.
     */
    bool isExiting_ = false;

private:

    bool isInitialized_ = false;
};

/**
 * An application's main class should inherit from the class defined by this macro rather than from Carbon::Application,
 * this is because external platform implementations are then able to add platform-specific additions into a
 * Carbon::Application subclass, redefine this macro, and have all applications automatically use their new customized
 * application base class.
 */
#define CARBON_APPLICATION_CLASS Carbon::Application

}
