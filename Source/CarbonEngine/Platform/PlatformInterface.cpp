/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/Android/PlatformAndroid.h"
#include "CarbonEngine/Platform/MacOSX/PlatformMacOSX.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/SDL/PlatformSDL.h"
#include "CarbonEngine/Platform/Windows/PlatformWindows.h"

namespace Carbon
{

CARBON_DEFINE_INTERFACE_REGISTRY(PlatformInterface)
{
    return true;
}

typedef PlatformInterface NullInterface;
CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PlatformInterface, NullInterface, 0)

#ifdef ANDROID
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PlatformInterface, PlatformAndroid, 100)
#endif
#ifdef CARBON_INCLUDE_PLATFORM_MACOSX
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PlatformInterface, PlatformMacOSX, 100)
#endif
#ifdef CARBON_INCLUDE_PLATFORM_SDL
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PlatformInterface, PlatformSDL, 50)
#endif
#ifdef CARBON_INCLUDE_PLATFORM_WINDOWS
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PlatformInterface, PlatformWindows, 100)
#endif

const GameControllerState GameControllerState::Empty;
const Resolution Resolution::Zero;

const auto WindowWidthSetting = String("WindowWidth");
const auto WindowHeightSetting = String("WindowHeight");
const auto FullscreenSetting = String("Fullscreen");
const auto VerticalSyncSetting = String("VerticalSync");
const auto FSAASetting = String("FSAA");
const auto GammaSetting = String("Gamma");
const auto RetinaResolutionsEnabled = String("RetinaResolutionsEnabled");

PlatformInterface::PlatformInterface()
{
    // The default input mappings do nothing
    for (auto i = uint(KeyNone); i < KeyLast; i++)
        inputMappings_[i].target = KeyConstant(i);
    for (auto i = uint(LeftMouseButton); i < MBLast; i++)
        inputMappings_[KeyLast + i].target = KeyConstant(KeyLast + i);

    setTicksPerSecond(1000);
}

PlatformInterface::~PlatformInterface()
{
    events().removeHandler(this);
}

bool PlatformInterface::setup()
{
    // Vertical sync mode
    isVerticalSyncEnabled_ = settings().getBoolean(VerticalSyncSetting, true);
    gammas_ = settings().getColor(GammaSetting);
    areRetinaResolutionsEnabled_ = settings().getBoolean(RetinaResolutionsEnabled, true);

    // Initialize timing
    lastFrameStartTime_ = getTime();
    timePassed_.clear();
    secondsPassed_ = 0.0f;

    LOG_INFO << "System details: " << getOperatingSystemName() << ", CPU count: " << getCPUCount()
             << ", CPU frequency: " << (getCPUFrequency() / 1000000)
             << "MHz, RAM: " << FileSystem::formatByteSize(getSystemMemorySize());

    return true;
}

bool PlatformInterface::processEvent(const Event& e)
{
    if (e.as<UpdateEvent>())
    {
        // Update timing
        auto currentTime = getTime();
        if (currentTime < lastFrameStartTime_)
            LOG_WARNING_WITHOUT_CALLER << "The 64-bit timing counter has wrapped around, this may cause problems";

        timePassed_ = currentTime - lastFrameStartTime_;
        lastFrameStartTime_ = currentTime;
        secondsPassed_ = timePassed_.toSeconds();

        sendRepeatingKeyDownEvents();
    }

    return true;
}

bool PlatformInterface::createWindow()
{
    if (resolutions_.empty())
    {
        LOG_ERROR << "Can't create a window because there are no supported resolutions";
        return false;
    }

    // Get window mode to use
    auto windowMode = settings().getBoolean(FullscreenSetting, true) ? Fullscreen : Windowed;

    // Get FSAA mode
    auto fsaa = FSAAMode(settings().getInteger(FSAASetting));
    if (fsaa != FSAANone && fsaa != FSAA2x && fsaa != FSAA4x && fsaa != FSAA8x && fsaa != FSAA16x)
        fsaa = FSAANone;

    // Attempt to create the main window with various fallbacks if problems are encountered
    if (createWindow(getStartupResolution(), windowMode, fsaa))
        return true;
    if (createWindow(getStartupResolution(), windowMode == Windowed ? Fullscreen : Windowed, fsaa))
        return true;

    // Now try again but without FSAA
    if (fsaa != FSAANone)
    {
        if (createWindow(getStartupResolution(), windowMode, FSAANone))
            return true;
        if (createWindow(getStartupResolution(), windowMode == Windowed ? Fullscreen : Windowed, FSAANone))
            return true;
    }

    return false;
}

bool PlatformInterface::resizeWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa)
{
    if (!resolutions_.has(resolution))
        return false;

    // To change resolution, window mode or FSAA mode the main window is destroyed and recreated using the new settings.
    // If the new settings fail the previous ones will be used as a fallback. The RecreateWindowEvent class is used to
    // notify the engine that the platform layer is being shutdown/reinitialized.

    LOG_INFO << "Recreating main window";

    // Store previous settings in case we have to revert back on failure
    auto oldResolution = getCurrentResolution();
    auto oldWindowMode = getWindowMode();
    auto oldFsaaMode = getFSAAMode();

    // Close current window
    events().dispatchEvent(RecreateWindowEvent(RecreateWindowEvent::CloseWindow));
    destroyWindow();

    events().setEventAllowed<ResizeEvent>(false);

    auto result = true;

    try
    {
        // Try new window settings
        if (!createWindow(resolution, windowMode, fsaa))
            throw Exception("Failed changing window settings, reverting to previous settings");

        // Setup engine for the the new window
        if (!events().dispatchEvent(RecreateWindowEvent(RecreateWindowEvent::NewWindow)))
        {
            events().dispatchEvent(RecreateWindowEvent(RecreateWindowEvent::CloseWindow));
            destroyWindow();
            throw Exception("The new window was not accepted by the engine, reverting to previous settings");
        }
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        // Put the window back how it was
        if (createWindow(oldResolution, oldWindowMode, oldFsaaMode))
        {
            // Setup engine for the the new window
            events().dispatchEvent(RecreateWindowEvent(RecreateWindowEvent::NewWindow));
        }
        else
        {
            // This really should never happen, as the window was successfully created earlier with these settings. This
            // is basically a fatal/unrecoverable error as we now have no rendering window
            LOG_ERROR << "Failed reverting window settings, there is now no rendering window";
        }

        result = false;
    }

    events().setEventAllowed<ResizeEvent>(true);
    sendResizeEvent();

    return result;
}

Resolution PlatformInterface::findResolution(unsigned int width, unsigned int height) const
{
    return resolutions_.detect([&](const Resolution& r) { return r.getWidth() == width && r.getHeight() == height; },
                               Resolution::Zero);
}

Resolution PlatformInterface::getStartupResolution() const
{
    // Look in the persistent settings for the startup resolution to use
    auto resolution =
        findResolution(settings().getInteger(WindowWidthSetting, 0), settings().getInteger(WindowHeightSetting, 0));

    // First fallback is to native resolution
    if (!resolution.isValid())
        resolution = nativeResolution_;

    // Last resort fallback is to the first resolution in the resolutions list
    if (!resolution.isValid() && resolutions_.size())
        resolution = resolutions_[0];

    return resolution;
}

bool PlatformInterface::setStartupResolution(const Resolution& resolution, WindowMode windowMode)
{
    if (!resolutions_.has(resolution))
    {
        LOG_ERROR << "Invalid startup resolution: " << resolution;
        return false;
    }

    settings().set(WindowWidthSetting, resolution.getWidth());
    settings().set(WindowHeightSetting, resolution.getHeight());
    settings().set(FullscreenSetting, windowMode == Fullscreen);

    return true;
}

bool PlatformInterface::addCustomResolution(unsigned int width, unsigned int height)
{
    if (!width || !height)
    {
        LOG_ERROR << "Invalid custom resolution: " << width << "x" << height;
        return false;
    }

    if (!areCustomResolutionsSupported())
    {
        LOG_ERROR << "Adding custom resolutions is not supported on this platform";
        return false;
    }

    if (findResolution(width, height).isValid())
        return true;

    resolutions_.emplace(width, height, true);
    LOG_INFO << "Added custom resolution: " << width << "x" << height;

    sortResolutions();

    return true;
}

void PlatformInterface::setRetinaResolutionsEnabled(bool enabled)
{
    areRetinaResolutionsEnabled_ = enabled;
    settings().set(RetinaResolutionsEnabled, areRetinaResolutionsEnabled_);
}

void PlatformInterface::calculateGammaRamp(float gamma, std::array<uint16_t, 256>& ramp,
                                           const std::array<uint16_t, 256>& defaultRamp)
{
    if (gamma > 0.0f)
    {
        gamma = 1.0f / gamma;

        for (auto i = 0U; i < 256; i++)
            ramp[i] = uint16_t(Math::clamp01(powf(float(i) / 255.0f, gamma)) * 65535.0f);
    }
    else
        ramp = defaultRamp;
}

void PlatformInterface::calculateGammaRamp(float gamma, std::array<float, 256>& ramp,
                                           const std::array<float, 256>& defaultRamp)
{
    if (gamma > 0.0f)
    {
        gamma = 1.0f / gamma;

        for (auto i = 0U; i < 256; i++)
            ramp[i] = Math::clamp01(powf(float(i) / 255.0f, gamma));
    }
    else
        ramp = defaultRamp;
}

bool PlatformInterface::sendResizeEvent() const
{
    return events().dispatchEvent(ResizeEvent(getWindowWidth(), getWindowHeight()));
}

void PlatformInterface::sortResolutions()
{
    // Get rid of any duplicate resolutions
    auto newResolutions = Vector<Resolution>();
    for (auto& resolution : resolutions_)
    {
        if (!resolution.isValid())
            continue;

        if (!newResolutions.has([&](const Resolution& r) {
                return r.getWidth() == resolution.getWidth() && r.getHeight() == resolution.getHeight();
            }))
            newResolutions.append(resolution);
    }

    resolutions_ = newResolutions.sorted();

    // Log the resolutions
    auto resolutions = UnicodeString(resolutions_.map<UnicodeString>());

    LOG_INFO << "Available resolutions: " << resolutions;
    LOG_INFO << "Native resolution: " << nativeResolution_;
}

void PlatformInterface::updatePersistentSettings() const
{
    // If we are currently in native resolution then don't store window width and height in the persistent settings, the
    // reason is that if an application is rendering in native resolution then we want it to keep rendering in native
    // resolution in subsequent runs, even if the computer's native resolution changes for some reason. This behavior
    // means that unless there is intervention of some kind the default behavior is to always be rendering at native
    // resolution, which is the desired outcome. However, if a non-native resolution is chosen then that resolution will
    // always be persisted to the next run of the application.

    if (currentResolution_ == nativeResolution_ && windowMode_ == Fullscreen)
    {
        settings().remove(WindowWidthSetting);
        settings().remove(WindowHeightSetting);
        settings().remove(FullscreenSetting);
    }
    else
    {
        settings().set(WindowWidthSetting, getWindowWidth());
        settings().set(WindowHeightSetting, getWindowHeight());
        settings().set(FullscreenSetting, windowMode_ == Fullscreen);
    }

    settings().set(VerticalSyncSetting, isVerticalSyncEnabled_);
    settings().set(FSAASetting, fsaaMode_);
    settings().set(GammaSetting, A(gammas_));
}

void PlatformInterface::setMousePosition(const Vec2& position)
{
    auto clampedPosition = Vec2(Math::clamp(position.x, 0.0f, platform().getWindowWidth() - 1.0f),
                                Math::clamp(position.y, 0.0f, platform().getWindowHeight() - 1.0f));

    if (mousePosition_ != clampedPosition)
    {
        mousePosition_ = clampedPosition;
        events().dispatchEvent(MouseMoveEvent(mousePosition_));
    }
}

void PlatformInterface::setMouseInputEventsAllowed(bool allowed)
{
    events().setEventAllowed<MouseButtonDownEvent>(allowed);
    events().setEventAllowed<MouseButtonUpEvent>(allowed);
    events().setEventAllowed<MouseMoveEvent>(allowed);
    events().setEventAllowed<MouseWheelEvent>(allowed);
}

void PlatformInterface::setKeyboardInputEventsAllowed(bool allowed)
{
    events().setEventAllowed<KeyDownEvent>(allowed);
    events().setEventAllowed<KeyUpEvent>(allowed);
    events().setEventAllowed<CharacterInputEvent>(allowed);
}

void PlatformInterface::setInputMapping(KeyConstant from, KeyConstant to)
{
    inputMappings_[from].target = to;
}

void PlatformInterface::setInputMapping(KeyConstant from, MouseButton to)
{
    inputMappings_[from].target = KeyConstant(KeyLast + to);
}

void PlatformInterface::setInputMapping(MouseButton from, MouseButton to)
{
    inputMappings_[KeyLast + from].target = KeyConstant(KeyLast + to);
}

void PlatformInterface::setInputMapping(MouseButton from, KeyConstant to)
{
    inputMappings_[KeyLast + from].target = to;
}

void PlatformInterface::onInputDownEvent(KeyConstant key)
{
    auto targetKey = inputMappings_[key].target;

    if (key == KeyNone || targetKey == KeyNone)
        return;

    if (targetKey < KeyLast)
    {
        events().dispatchEvent(KeyDownEvent(targetKey, false));

        keyState_[targetKey].lastKeyDownEventTime = getTime();
        keyState_[targetKey].hadInitialRepeatDelay = false;
    }
    else
        events().dispatchEvent(MouseButtonDownEvent(MouseButton(targetKey - KeyLast), platform().getMousePosition()));
}

void PlatformInterface::onInputUpEvent(KeyConstant key)
{
    auto targetKey = inputMappings_[key].target;

    if (key == KeyNone || targetKey == KeyNone)
        return;

    if (targetKey < KeyLast)
        events().dispatchEvent(KeyUpEvent(targetKey));
    else
        events().dispatchEvent(MouseButtonUpEvent(MouseButton(targetKey - KeyLast), platform().getMousePosition()));
}

void PlatformInterface::setIsKeyPressed(KeyConstant key, bool isPressed)
{
    auto targetKey = inputMappings_[key].target;

    if (key == KeyNone || targetKey == KeyNone)
        return;

    if (targetKey < KeyLast)
        keyState_[targetKey].isPressed = isPressed;
    else
        isMouseButtonPressed_[targetKey - KeyLast] = isPressed;
}

void PlatformInterface::sendRepeatingKeyDownEvents()
{
    auto currentTime = getTime();

    // The key repeat delay and repeat rate are currently hardcoded to typical values of 0.5s and 33Hz
    auto initialRepeatDelay = TimeValue(0.5f);
    auto repeatDelay = TimeValue(1.0f / 33.0f);

    // Loop over all keys and send repeating KeyDownEvents for those that are currently pressed and the appropriate
    // amount of time has elapsed since the previous KeyDownEvent
    for (auto i = 0U; i < keyState_.size(); i++)
    {
        auto& keyState = keyState_[i];

        if (keyState.isPressed)
        {
            auto delay = keyState.hadInitialRepeatDelay ? repeatDelay : initialRepeatDelay;
            if (keyState.lastKeyDownEventTime + delay < currentTime)
            {
                events().dispatchEvent(KeyDownEvent(KeyConstant(i), true));
                keyState.hadInitialRepeatDelay = true;
                keyState.lastKeyDownEventTime = currentTime;
            }
        }
    }
}

}
