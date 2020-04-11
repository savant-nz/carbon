/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Math/Vec2.h"
#include "CarbonEngine/Platform/GameControllerState.h"
#include "CarbonEngine/Platform/KeyConstant.h"
#include "CarbonEngine/Platform/MouseButton.h"
#include "CarbonEngine/Platform/Resolution.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * This is the base platform class which defines an interface for creating a rendering surface, resizing, fullscreen
 * switching, input state, timing, and is responsible for firing input events generated by the user. Actual
 * implementations inherit from this class and provide a backend for a specific platform.
 */
class CARBON_API PlatformInterface : public EventHandler, private Noncopyable
{
public:

    /**
     * The window modes for windowed and fullscreen.
     */
    enum WindowMode
    {
        /**
         * Windowed mode with a window caption and border.
         */
        Windowed,

        /**
         * Fullscreen mode with no window caption or border.
         */
        Fullscreen
    };

    /**
     * Enum used to specify a full screen antialiasing mode. Not all FSAA modes will be available depending on the
     * hardware configuration. If a FSAA mode is requested that can't be done in hardware then the next highest
     * available mode will be used.
     */
    enum FSAAMode
    {
        /**
         * No FSAA.
         */
        FSAANone = 0,

        /**
         * 2x FSAA.
         */
        FSAA2x = 2,

        /**
         * 4x FSAA.
         */
        FSAA4x = 4,

        /**
         * 8x FSAA.
         */
        FSAA8x = 8,

        /**
         * 16x FSAA.
         */
        FSAA16x = 16
    };

    /**
     * Set of enum values used to set and retrieve platform-specific values from the active platform implementation.
     */
    enum PlatformSpecificValue
    {
        /**
         * On Windows, this is the HWND of the rendering window. Read only.
         */
        WindowsHWnd,

        /**
         * On Windows, this is the HICON of the window class and can be set to change the icon of the rendering window.
         */
        WindowsHIcon,

        /**
         * On platforms that support the Oculus Rift this value is the underlying `ovrSession` instance.
         */
        OculusRiftSession,

        /**
         * On iOS, the primary OpenGL ES framebuffer object. Read only.
         */
        iOSOpenGLESFramebuffer
    };

    PlatformInterface();
    ~PlatformInterface() override;

    /**
     * Does initial setup for the platform. Returns success flag.
     */
    virtual bool setup();

    /**
     * Handles events that affect the platform implementation.
     */
    bool processEvent(const Event& e) override;

    /**
     * Returns the given platform-specific value. If the specified value is not supported then zero is returned.
     */
    virtual uintptr_t getPlatformSpecificValue(PlatformSpecificValue value) const { return 0; }

    /**
     * Returns the given platform-specific value. If the specified value is not supported then zero is returned.
     */
    template <typename ValueType> ValueType getPlatformSpecificValue(PlatformSpecificValue value) const
    {
        return reinterpret_cast<ValueType>(getPlatformSpecificValue(value));
    }

    /**
     * Sets the given platform-specific value. If the specified value is not supported or can't be set then false is
     * returned.
     */
    virtual bool setPlatformSpecificValue(PlatformSpecificValue value, uintptr_t newValue) { return false; }

    /**
     * Returns whether this platform and display device support windowed mode rendering, if this is false then all
     * rendering will be done in fullscreen regardless of the requested window mode.
     */
    bool isWindowedModeSupported() const { return isWindowedModeSupported_; }

    /**
     * Creates the rendering window using the current startup resolution. If creation using the startup resolution isn't
     * possible then the fallback resolution is attempted. If this fails then the fullscreen setting is flipped and
     * another attempt is made. The previous fullscreen and FSAA settings are preserved if possible. Returns success
     * flag.
     */
    bool createWindow();

    /**
     * Creates the rendering window with the given resolution and window mode.
     */
    virtual bool createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa) { return false; }

    /**
     * Resizes the rendering window, takes the same parameters as PlatformInterface::createWindow(). Sends a ResizeEvent
     * for the new window size. Returns success flag.
     */
    virtual bool resizeWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa);

    /**
     * Destroys the rendering window.
     */
    virtual void destroyWindow() {}

    /**
     * Sets the window title of the rendering window. The default title is the name of the application's main
     * Application subclass, however this can be changed using this method. Returns success flag.
     */
    virtual bool setWindowTitle(const UnicodeString& title) { return false; }

    /**
     * If OpenGL is being used for rendering then this method returns the address of an OpenGL function, or null if that
     * function doesn't exist.
     */
    virtual VoidFunction getOpenGLFunctionAddress(const String& function) const { return nullptr; }

    /**
     * Templated variant of PlatformInterface::getOpenGLFunctionAddress() that does a reinterpret_cast internally to
     * make calling it less verbose.
     */
    template <typename FunctionType> FunctionType getOpenGLFunctionAddress(const String& function) const
    {
        return reinterpret_cast<FunctionType>(getOpenGLFunctionAddress(function));
    }

    /**
     * Swaps the front and back buffers of the rendering window.
     */
    virtual void swap() {}

    /**
     * Returns whether vertical sync is currently enabled.
     */
    bool isVerticalSyncEnabled() const { return isVerticalSyncEnabled_; }

    /**
     * Sets whether vertical sync is enabled, vertical sync prevents tearing artifacts by synchronizing buffer swaps
     * with display updates, but will limit the frame rate to the refresh rate of the display. Returns success flag.
     */
    virtual bool setVerticalSyncEnabled(bool enabled) { return false; }

    /**
     * Returns the requested resolution. Resolutions are referenced by a unique ID and are sorted by size from smallest
     * to largest. If \a index is invalid then a zeroed Resolution object is returned. The valid range for resolution ID
     * values is from 1 to the number of resolutions returned by PlatformInterface::getResolutionCount().
     */
    const Vector<Resolution>& getResolutions() const { return resolutions_; }

    /**
     * Returns whether the specified resolution is supported.
     */
    bool hasResolution(unsigned int width, unsigned int height) const
    {
        return findResolution(width, height).isValid();
    }

    /**
     * Searches the list of supported resolutions for one with the specified dimensions and returns it if found, if the
     * specified resolution is not supported then Resolution::Zero is returned.
     */
    Resolution findResolution(unsigned int width, unsigned int height) const;

    /**
     * Returns the current resolution for the render window, or Resolution::Zero if no window is active. The current
     * dimensions of the render window can be queried with PlatformInterface::getWindowWidth() and
     * PlatformInterface::getWindowHeight().
     */
    Resolution getCurrentResolution() const { return currentResolution_; }

    /**
     * Returns the preferred native resolution of the active device, or Resolution::Zero if it can't be determined.
     */
    Resolution getNativeResolution() const { return nativeResolution_; }

    /**
     * Returns the resolution that will be used on startup, see PlatformInterface::setStartupResolution() for details.
     */
    Resolution getStartupResolution() const;

    /**
     * Sets the startup resolution and window mode to use. By default the startup resolution is the resolution that was
     * in use when the application was last shut down. If this is the first run of the application then the native
     * resolution as returned by PlatformInterface::getNativeResolution() will be used as the startup resolution. Note
     * that if an application wants to use this method to control the initial resolution then it should do so in its
     * Application::startup() override as that will be called prior to window creation, its Application::initialize()
     * override is called following window creation which is after the startup resolution will have been used. Returns
     * success flag.
     */
    bool setStartupResolution(const Resolution& resolution, WindowMode windowMode);

    /**
     * Returns whether this platform supports custom resolutions added using PlatformInterface::addCustomResolution().
     */
    virtual bool areCustomResolutionsSupported() const { return false; }

    /**
     * Adds the specified resolution to the list of supported resolutions, this will only work if
     * PlatformInterface::areCustomResolutionsSupported() returns true. If the given resolution is already supported
     * then nothing is done. Custom resolutions allow arbitrary window dimensions to be used, however do note that they
     * are generally not compatible with fullscreen rendering and so may limit the application to running in windowed
     * mode. Returns success flag.
     */
    bool addCustomResolution(unsigned int width, unsigned int height);

    /**
     * Returns whether retina resolutions are enabled, when this is true resolutions that return true from
     * Resolution::isRetinaResolution() are allowed to be used, and when it is false an attempt to create a window using
     * a retina resolution will result in a fallback to the matching non-retina resolution. Retina resolutions are
     * enabled by default. If an application wants to disable retina rendering then it should call
     * `platform().setRetinaResolutionsEnabled(false)` in its Application::startup() method.
     */
    bool areRetinaResolutionsEnabled() const { return areRetinaResolutionsEnabled_; }

    /**
     * Sets whether retina resolutions are enabled, see \a PlatformInterface::areRetinaResolutionsEnabled() for details.
     * Calling this method does not change the current resolution, but will take effect on subsequent resolution
     * changes.
     */
    void setRetinaResolutionsEnabled(bool enabled);

    /**
     * Returns the width of the rendering window.
     */
    unsigned int getWindowWidth() const { return currentResolution_.getWidth(); }

    /**
     * Returns the height of the rendering window.
     */
    unsigned int getWindowHeight() const { return currentResolution_.getHeight(); }

    /**
     * Returns the width of the rendering window as a floating point value.
     */
    float getWindowWidthf() const { return float(getWindowWidth()); }

    /**
     * Returns the height of the rendering window as a floating point value.
     */
    float getWindowHeightf() const { return float(getWindowHeight()); }

    /**
     * Returns the point in the middle of the window.
     */
    Vec2 getWindowMiddle() const { return {getWindowWidthf() * 0.5f, getWindowHeightf() * 0.5f}; }

    /**
     * Returns a rect with left and bottom set to zero, right set to the window width, and top set to the window height.
     */
    Rect getWindowRect() const { return {0.0f, 0.0f, getWindowWidthf(), getWindowHeightf()}; }

    /**
     * Returns the aspect ratio of the current window.
     */
    float getWindowAspectRatio() const { return getWindowWidthf() / getWindowHeightf(); }

    /**
     * Sometimes the aspect ratio for the final displayed image is different to the aspect ratio of the primary
     * rendering surface, i.e. the final render is stretched for display. If this is the case then the final display
     * aspect ratio returned from this method will be the aspect ratio after any such stretching. The default
     * implementation of this method returns PlatformInterface::getWindowAspectRatio().
     */
    virtual float getFinalDisplayAspectRatio() const { return getWindowAspectRatio(); }

    /**
     * Returns the current window mode.
     */
    WindowMode getWindowMode() const { return windowMode_; }

    /**
     * Returns the current FSAA mode.
     */
    FSAAMode getFSAAMode() const { return fsaaMode_; }

    /**
     * Returns the current window title.
     */
    const UnicodeString& getWindowTitle() const { return windowTitle_; }

    /**
     * Returns the current setting as to whether the main window should grab an exclusive lock on user input when it
     * gains focus. Defaults to true. The main window will always release any exclusive input lock it is holding when it
     * loses focus. Note that this only applies when in windowed mode, fullscreen rendering will always take an
     * exclusive input lock regardless of this value.
     */
    bool isInputLockEnabled() const { return isWindowedModeInputLockEnabled_; }

    /**
     * Sets whether the main window should grab an exclusive lock on user input when it gains focus. Note that setting
     * this to false will not release any current input lock, use the PlatformInterface::releaseInputLock() method to
     * force that. This method only applies when in windowed mode, fullscreen rendering will always take an exclusive
     * input lock regardless of what this value is set to.
     */
    void setInputLockEnabled(bool enabled) { isWindowedModeInputLockEnabled_ = enabled; }

    /**
     * Returns whether the main window is currently holding an exclusive input lock.
     */
    bool isHoldingInputLock() const { return isHoldingInputLock_; }

    /**
     * Releases the main window's exclusive lock on user input if it is currently holding one. Any input lock is
     * automatically released when the main window loses focus, and retaken when it regains focus.
     */
    virtual bool releaseInputLock() { return false; }

    /**
     * Returns the current absolute mouse position.
     */
    const Vec2& getMousePosition() const { return mousePosition_; }

    /**
     * Sets the current absolute mouse position, the given position is clamped inside the window bounds. This method
     * will dispatch a MouseMoveEvent if the new position is different to the previous position. Applications can use
     * this method to control the position of the mouse on platforms that don't have a default pointing device.
     */
    virtual void setMousePosition(const Vec2& position);

    /**
     * This is a small helper method that moves the mouse position by the specified delta, thus avoiding having to
     * manually add the delta onto the current mouse position.
     */
    void moveMousePosition(const Vec2& delta) { setMousePosition(getMousePosition() + delta); }

    /**
     * Returns the current relative mouse position.
     */
    const Vec2& getMouseRelative() const { return mouseRelative_; }

    /**
     * Returns whether the given mouse button is pressed.
     */
    bool isMouseButtonPressed(MouseButton button) const { return isMouseButtonPressed_[button]; }

    /**
     * Returns whether the given key is pressed.
     */
    bool isKeyPressed(KeyConstant key, bool ignoreAllowIsKeyPressed = false) const
    {
        return keyState_[key].isPressed && (allowIsKeyPressed_ || ignoreAllowIsKeyPressed);
    }

    /**
     * Sets whether PlatformInterface::isKeyPressed() is allowed to return true. Key down and up events and still sent.
     */
    void setAllowIsKeyPressed(bool allow) { allowIsKeyPressed_ = allow; }

    /**
     * Sets whether all the mouse input events will be sent, internally this just calls EventManager::setEventAllowed()
     * for each of the four mouse input events: MouseButtonDownEvent, MouseButtonUpEvent, MouseMoveEvent and
     * MouseWheelEvent.
     */
    void setMouseInputEventsAllowed(bool allowed);

    /**
     * Sets whether all the keyboard input events will be sent, internally this just calls
     * EventManager::setEventAllowed() for each of the three keyboard input events: KeyDownEvent, KeyUpEvent and
     * CharacterInputEvent.
     */
    void setKeyboardInputEventsAllowed(bool allowed);

    /**
     * Returns the current time. Should be implemented by subclasses.
     */
    virtual TimeValue getTime() const { return {}; }

    /**
     * Returns the amount of time passed since the last frame as a TimeValue instance.
     */
    TimeValue getTimePassed() const { return timePassed_; }

    /**
     * Returns the number of seconds passed since the last frame.
     */
    float getSecondsPassed() const { return secondsPassed_; }

    /**
     * Sends out a ResizeEvent for the current resolution of this window.
     */
    bool sendResizeEvent() const;

    /**
     * The available message box button arrangements.
     */
    enum MessageBoxButtons
    {
        /**
         * A single OK button.
         */
        OKButton,

        /**
         * Separate OK and Cancel buttons.
         */
        OKCancelButtons,

        /**
         * Separate Yes and No buttons.
         */
        YesNoButtons
    };

    /**
     * The available message box icons.
     */
    enum MessageBoxIcon
    {
        /**
         * An information icon will appear in the message box.
         */
        InformationIcon,

        /**
         * An error icon will appear in the message box.
         */
        ErrorIcon
    };

    /**
     * Displays a custom message box and returns the button the user pressed, true means either OK or Yes was clicked
     * and false means either Cancel or No was clicked.
     */
    virtual bool showMessageBox(const UnicodeString& text, const UnicodeString& title,
                                MessageBoxButtons buttons = OKButton, MessageBoxIcon icon = InformationIcon)
    {
        LOG_ERROR << "Not supported on this platform - text: " << text << ", title: " << title;

        return false;
    }

    /**
     * Refreshes the list of known game controllers, any controllers that have been added or removed from the system
     * will appear or disappear from the game controller list. The game controller list is refreshed automatically by
     * PlatformInterface::createWindow() and PlatformInterface::resizeWindow(). Calling this method invalidates all
     * previously existing game controller IDs.
     */
    virtual void refreshGameControllerList() {}

    /**
     * Clears all game controllers that are currently initialized. After this is called
     * PlatformInterface::getGameControllers() will return an empty vector and no game controllers will be able to be
     * used until PlatformInterface::refreshGameControllerList() is called again to reinitialize any game controllers
     * attached to the system. Clearing the game controller list is useful because it releases any internal lock on the
     * device that may prevent it being used by other applications. This method also erases any force feedback effects
     * that have been created.
     */
    virtual void clearGameControllers() {}

    /**
     * Returns the list of the ID values of all game controllers attached to the system.
     */
    virtual Vector<unsigned int> getGameControllers() const { return {}; }

    /**
     * Returns the display string to use for the given game controller, or an empty string if the given controller ID is
     * invalid. If zero is passed in for the controller ID then the first game controller in the game controller list is
     * used.
     */
    virtual UnicodeString getGameControllerDisplayName(unsigned int controllerID) { return UnicodeString::Empty; }

    /**
     * Returns the state of a game controller. If zero is passed in for the controller ID then the first game controller
     * in the game controller list is used. If \a controllerID is non-zero and is not a valid game controller ID or
     * there are no game controllers attached to the system then GameControllerState::Empty is returned.
     */
    virtual const GameControllerState& getGameControllerState(unsigned int controllerID)
    {
        return GameControllerState::Empty;
    }

    /**
     * Creates a force feedback effect for a game controller and returns the ID value for the new effect. If the device
     * does not support the given effect then zero is returned. If zero is passed in for the controller ID then the
     * first game controller in the game controller list is used.
     */
    virtual unsigned int createForceFeedbackEffect(unsigned int controllerID, const ForceFeedbackEffect& effect)
    {
        return 0;
    }

    /**
     * Clears a force feedback effect. If zero is passed in for the controller ID then the first game controller in the
     * game controller list is used.
     */
    virtual bool deleteForceFeedbackEffect(unsigned int controllerID, unsigned int effectID) { return false; }

    /**
     * Plays a force feedback effect on the given game controller. The effect must first be created by the
     * PlatformInterface::createForceFeedbackEffect() method. If zero is passed in for the controller ID then the first
     * game controller in the game controller list is used.
     */
    virtual bool playForceFeedbackEffect(unsigned int controllerID, unsigned int effectID, unsigned int iterations)
    {
        return false;
    }

    /**
     * Stops a force feedback effect that is currently playing on a game controller. If zero is passed in for the
     * controller ID then the first game controller in the game controller list is used.
     */
    virtual bool stopForceFeedbackEffect(unsigned int controllerID, unsigned int effectID) { return false; }

    /**
     * Opens the given file or URL with the system's default application or web browser. Returns success flag.
     */
    virtual bool openWithDefaultApplication(const UnicodeString& resource) const
    {
        LOG_ERROR << "Not supported";
        return false;
    }

    /**
     * Returns the current gamma ramp values in use. A gamma ramp value of zero means the gamma curve present at
     * application startup is being used.
     */
    const Color& getGamma() const { return gammas_; }

    /**
     * Sets the current red, green and blue gamma values. A gamma of zero means the gamma curve present at application
     * startup should be used, which is the default. Returns success flag.
     */
    virtual bool setGamma(const Color& gammas) { return false; }

    /**
     * Sets the current red, green and blue gammas to the same value. Returns success flag.
     */
    bool setGamma(float gamma) { return setGamma(Color(gamma)); }

    /**
     * Creates an input mapping from one key to another, i.e. when a raw '\a from' keypress is received it will be
     * translated into a '\a to' keypress before any further processing is done. Mapping a key to \a KeyNone disables
     * it. Note that input mappings do not affect text entry.
     */
    void setInputMapping(KeyConstant from, KeyConstant to);

    /**
     * Creates an input mapping from a key to a mouse button, i.e. when a raw '\a from' keypress is received it will be
     * translated into a '\a to' mouse button press before any further processing is done. Note that input mappings do
     * not affect text entry.
     */
    void setInputMapping(KeyConstant from, MouseButton to);

    /**
     * Creates an input mapping from one mouse button to another, i.e. when a raw '\a from' mouse button press is
     * received it will be translated into a '\a to' mouse button press before any further processing is done.
     */
    void setInputMapping(MouseButton from, MouseButton to);

    /**
     * Creates an input mapping from a mouse button to a key, i.e. when a raw '\a from' mouse button press is received
     * it will be translated into a '\a to' keypress before any further processing is done. Mapping a mouse button to
     * \a KeyNone disables it. Note that input mappings do not affect text entry.
     */
    void setInputMapping(MouseButton from, KeyConstant to);

    /**
     * On platforms that support an onscreen keyboard such as iOS this method will show it on the screen on top of the
     * rendered scenes. Returns success flag.
     */
    virtual bool showOnscreenKeyboard() { return false; }

    /**
     * On platforms that support an onscreen keyboard such as iOS this method will hide it if it is currently showing.
     */
    virtual void hideOnscreenKeyboard() {}

    /**
     * Returns whether or not the passed touch event is currently enabled.
     */
    virtual bool isTouchEventEnabled(unsigned int eventTypeID) const { return false; }

    /**
     * Returns whether or not the passed touch event is currently enabled, disabled events will not be detected. The
     * valid event types are TouchTapEvent, \a TouchPinchEvent, \a TouchRotationEvent, \a TouchSwipeEvent and
     * \a TouchPanEvent, and none of these are enabled by default. On devices without touchscreens this method always
     * returns false.
     */
    template <typename EventType> bool isTouchEventEnabled() const
    {
        return isTouchEventEnabled(events().getEventTypeID<EventType>());
    }

    /**
     * For use on touchscreen devices, sets whether the specified touch event should be enabled or disabled, see
     * PlatformInterface::isTouchEventEnabled() for more details.
     */
    virtual void setTouchEventEnabled(unsigned int eventTypeID, bool enabled) {}

    /**
     * \copydoc PlatformInterface::setTouchEventEnabled(unsigned int, bool)
     */
    template <typename EventType> void setTouchEventEnabled(bool enabled)
    {
        setTouchEventEnabled(events().getEventTypeID<EventType>(), enabled);
    }

    /**
     * On platforms that support touch input this returns the positions of all current touches.
     */
    virtual Vector<Vec2> getTouches() const { return {}; }

    /**
     * Returns whether or not the current device is a smartphone style of device such as the iPhone, iPod Touch or
     * similar.
     */
    virtual bool isPhone() const { return false; }

    /**
     * Returns whether or not the current device is a tablet style of device such as the iPad or similar.
     */
    virtual bool isTablet() const { return false; }

    /**
     * Returns a human-readable string describing the active platform and operating system.
     */
    virtual String getOperatingSystemName() const { return {}; }

    /**
     * Returns the number of primary CPUs on the current platform, this counts each core of a multicore processor as a
     * separate CPU.
     */
    virtual unsigned int getCPUCount() const { return 1; }

    /**
     * Returns the clock frequency of the primary CPUs on the current platform. If the CPU frequency is unknown then
     * zero is returned.
     */
    virtual uint64_t getCPUFrequency() const { return 0; }

    /**
     * On platforms that allow application control of the CPU frequency, this method can be used to throttle the CPU
     * down or up as needed, generally with the aim of giving a desirable performance/battery life tradeoff. Returns
     * success flag.
     */
    virtual bool setCPUFrequency(uint64_t frequency) { return false; }

    /**
     * Returns the amount of system memory on the current platform. If the size is unknown then zero is returned.
     */
    virtual uint64_t getSystemMemorySize() const { return 0; }

    /**
     * Returns whether Oculus Rift rendering is supported on this platform. Use PlatformInterface::isOculusRiftPresent()
     * to check for the presence of an actual connected device.
     */
    virtual bool isOculusRiftSupported() const { return false; }

    /**
     * Returns whether there is an Oculus Rift device present and ready for use.
     */
    virtual bool isOculusRiftPresent() const { return false; }

    /**
     * Returns the Oculus Rift's left eye transform.
     */
    virtual const SimpleTransform& getOculusRiftTransformLeftEye() const { return SimpleTransform::Identity; }

    /**
     * Returns the Oculus Rift's right eye transform.
     */
    virtual const SimpleTransform& getOculusRiftTransformRightEye() const { return SimpleTransform::Identity; }

    /**
     * Returns the projection matrix to use for the Oculus Rift's left eye given the specified clipping plane distances.
     */
    virtual Matrix4 getOculusRiftProjectionMatrixLeftEye(float nearPlaneDistance, float farPlaneDistance) const
    {
        return {};
    }

    /**
     * Returns the projection matrix to use for the Oculus Rift's right eye given the specified clipping plane
     * distances.
     */
    virtual Matrix4 getOculusRiftProjectionMatrixRightEye(float nearPlaneDistance, float farPlaneDistance) const
    {
        return {};
    }

    /**
     * Returns the texture dimensions to use when rendering to the Oculus Rift.
     */
    virtual const Rect& getOculusRiftTextureDimensions() const { return Rect::Zero; }

protected:

#ifndef DOXYGEN

    // Resolution management, the resolutions vector is filled by PlatformInterface::setup()
    Vector<Resolution> resolutions_;
    void sortResolutions();
    Resolution nativeResolution_;
    bool areRetinaResolutionsEnabled_ = true;
    bool isWindowedModeSupported_ = true;

    // The current window setup
    Resolution currentResolution_;
    WindowMode windowMode_ = Windowed;
    FSAAMode fsaaMode_ = FSAANone;
    UnicodeString windowTitle_;

    bool isVerticalSyncEnabled_ = true;

    // Updates the value of all the platform layer's persistent settings in the settings manager
    void updatePersistentSettings() const;

    // Input
    bool allowIsKeyPressed_ = true;
    std::array<bool, MBLast> isMouseButtonPressed_ = {};
    Vec2 mousePosition_;
    Vec2 mouseRelative_;
    bool isWindowedModeInputLockEnabled_ = true;
    bool isHoldingInputLock_ = false;

    // Input mappings, keyboard and mouse buttons are merged into a single array to make the code simpler
    struct InputMapping
    {
        KeyConstant target = KeyNone;
        bool mapRepeats = true;
    } inputMappings_[KeyLast + MBLast];

    // Platform subclasses should call these methods when they receive raw input events in order to to make sure that
    // input mappings are correctly obeyed
    virtual void onInputDownEvent(KeyConstant key);
    virtual void onInputDownEvent(MouseButton button) { onInputDownEvent(KeyConstant(KeyLast + button)); }
    virtual void onInputUpEvent(KeyConstant key);
    virtual void onInputUpEvent(MouseButton button) { onInputUpEvent(KeyConstant(KeyLast + button)); }
    virtual void setIsKeyPressed(KeyConstant key, bool isPressed);

    // This array holds details on all keys, it is used to send out repeating KeyDownEvents
    struct KeyState
    {
        TimeValue lastKeyDownEventTime;
        bool isPressed = false;
        bool hadInitialRepeatDelay = false;
    };
    std::array<KeyState, KeyLast> keyState_ = {};

    void sendRepeatingKeyDownEvents();

    // Timing
    TimeValue lastFrameStartTime_;
    TimeValue timePassed_;
    float secondsPassed_ = 0.0f;
    void setTicksPerSecond(int64_t ticksPerSecond) { TimeValue::ticksPerSecond_ = ticksPerSecond; }

    // Gamma ramping
    Color gammas_;
    void calculateGammaRamp(float gamma, std::array<uint16_t, 256>& ramp, const std::array<uint16_t, 256>& defaultRamp);
    void calculateGammaRamp(float gamma, std::array<float, 256>& ramp, const std::array<float, 256>& defaultRamp);

#endif
};

CARBON_DECLARE_INTERFACE_REGISTRY(PlatformInterface);

}
