/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_PLATFORM_SDL

#ifndef CARBON_INCLUDE_OPENGL11
    #error The OpenGL 1.1 graphics backend must be included in the build when using the SDL platform backend
#endif

#ifdef CARBON_INCLUDE_OPENGL41
    #error The OpenGL 4.1 graphics backend must be excluded from the build when using the SDL platform backend
#endif

#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SharedLibrary.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Vec2i.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/SDL/PlatformSDL.h"

namespace Carbon
{

PlatformSDL::PlatformSDL()
{
    events().addHandler<UpdateEvent>(this, true);

    sdlkkcTable_[SDLK_BACKSPACE] = KeyBackspace;
    sdlkkcTable_[SDLK_TAB] = KeyTab;
    sdlkkcTable_[SDLK_RETURN] = KeyEnter;
    sdlkkcTable_[SDLK_ESCAPE] = KeyEscape;
    sdlkkcTable_[SDLK_SPACE] = KeySpacebar;
    sdlkkcTable_[SDLK_EXCLAIM] = Key1;
    sdlkkcTable_[SDLK_QUOTEDBL] = KeyApostrophe;
    sdlkkcTable_[SDLK_HASH] = Key3;
    sdlkkcTable_[SDLK_DOLLAR] = Key4;
    sdlkkcTable_[SDLK_AMPERSAND] = Key7;
    sdlkkcTable_[SDLK_QUOTE] = KeyApostrophe;
    sdlkkcTable_[SDLK_LEFTPAREN] = Key9;
    sdlkkcTable_[SDLK_RIGHTPAREN] = Key0;
    sdlkkcTable_[SDLK_ASTERISK] = Key8;
    sdlkkcTable_[SDLK_PLUS] = KeyPlus;
    sdlkkcTable_[SDLK_COMMA] = KeyComma;
    sdlkkcTable_[SDLK_MINUS] = KeyMinus;
    sdlkkcTable_[SDLK_PERIOD] = KeyPeriod;
    sdlkkcTable_[SDLK_SLASH] = KeyForwardSlash;
    sdlkkcTable_[SDLK_0] = Key0;
    sdlkkcTable_[SDLK_1] = Key1;
    sdlkkcTable_[SDLK_2] = Key2;
    sdlkkcTable_[SDLK_3] = Key3;
    sdlkkcTable_[SDLK_4] = Key4;
    sdlkkcTable_[SDLK_5] = Key5;
    sdlkkcTable_[SDLK_6] = Key6;
    sdlkkcTable_[SDLK_7] = Key7;
    sdlkkcTable_[SDLK_8] = Key8;
    sdlkkcTable_[SDLK_9] = Key9;
    sdlkkcTable_[SDLK_COLON] = KeySemicolon;
    sdlkkcTable_[SDLK_SEMICOLON] = KeySemicolon;
    sdlkkcTable_[SDLK_LESS] = KeyComma;
    sdlkkcTable_[SDLK_EQUALS] = KeyEquals;
    sdlkkcTable_[SDLK_GREATER] = KeyPeriod;
    sdlkkcTable_[SDLK_QUESTION] = KeyForwardSlash;
    sdlkkcTable_[SDLK_AT] = Key2;
    sdlkkcTable_[SDLK_LEFTBRACKET] = KeyLeftBracket;
    sdlkkcTable_[SDLK_BACKSLASH] = KeyBackSlash;
    sdlkkcTable_[SDLK_RIGHTBRACKET] = KeyRightBracket;
    sdlkkcTable_[SDLK_CARET] = Key6;
    sdlkkcTable_[SDLK_UNDERSCORE] = KeyMinus;
    sdlkkcTable_[SDLK_BACKQUOTE] = KeyGraveAccent;
    sdlkkcTable_[SDLK_a] = KeyA;
    sdlkkcTable_[SDLK_b] = KeyB;
    sdlkkcTable_[SDLK_c] = KeyC;
    sdlkkcTable_[SDLK_d] = KeyD;
    sdlkkcTable_[SDLK_e] = KeyE;
    sdlkkcTable_[SDLK_f] = KeyF;
    sdlkkcTable_[SDLK_g] = KeyG;
    sdlkkcTable_[SDLK_h] = KeyH;
    sdlkkcTable_[SDLK_i] = KeyI;
    sdlkkcTable_[SDLK_j] = KeyJ;
    sdlkkcTable_[SDLK_k] = KeyK;
    sdlkkcTable_[SDLK_l] = KeyL;
    sdlkkcTable_[SDLK_m] = KeyM;
    sdlkkcTable_[SDLK_n] = KeyN;
    sdlkkcTable_[SDLK_o] = KeyO;
    sdlkkcTable_[SDLK_p] = KeyP;
    sdlkkcTable_[SDLK_q] = KeyQ;
    sdlkkcTable_[SDLK_r] = KeyR;
    sdlkkcTable_[SDLK_s] = KeyS;
    sdlkkcTable_[SDLK_t] = KeyT;
    sdlkkcTable_[SDLK_u] = KeyU;
    sdlkkcTable_[SDLK_v] = KeyV;
    sdlkkcTable_[SDLK_w] = KeyW;
    sdlkkcTable_[SDLK_x] = KeyX;
    sdlkkcTable_[SDLK_y] = KeyY;
    sdlkkcTable_[SDLK_z] = KeyZ;
    sdlkkcTable_[SDLK_DELETE] = KeyDelete;
    sdlkkcTable_[SDLK_UP] = KeyUpArrow;
    sdlkkcTable_[SDLK_DOWN] = KeyDownArrow;
    sdlkkcTable_[SDLK_RIGHT] = KeyRightArrow;
    sdlkkcTable_[SDLK_LEFT] = KeyLeftArrow;
    sdlkkcTable_[SDLK_INSERT] = KeyInsert;
    sdlkkcTable_[SDLK_HOME] = KeyHome;
    sdlkkcTable_[SDLK_END] = KeyEnd;
    sdlkkcTable_[SDLK_PAGEUP] = KeyPageUp;
    sdlkkcTable_[SDLK_PAGEDOWN] = KeyPageDown;
    sdlkkcTable_[SDLK_F1] = KeyF1;
    sdlkkcTable_[SDLK_F2] = KeyF2;
    sdlkkcTable_[SDLK_F3] = KeyF3;
    sdlkkcTable_[SDLK_F4] = KeyF4;
    sdlkkcTable_[SDLK_F5] = KeyF5;
    sdlkkcTable_[SDLK_F6] = KeyF6;
    sdlkkcTable_[SDLK_F7] = KeyF7;
    sdlkkcTable_[SDLK_F8] = KeyF8;
    sdlkkcTable_[SDLK_F9] = KeyF9;
    sdlkkcTable_[SDLK_F10] = KeyF10;
    sdlkkcTable_[SDLK_F11] = KeyF11;
    sdlkkcTable_[SDLK_F12] = KeyF12;
    sdlkkcTable_[SDLK_CAPSLOCK] = KeyCapsLock;
    sdlkkcTable_[SDLK_LSHIFT] = KeyLeftShift;
    sdlkkcTable_[SDLK_RSHIFT] = KeyRightShift;
    sdlkkcTable_[SDLK_LCTRL] = KeyLeftControl;
    sdlkkcTable_[SDLK_RCTRL] = KeyRightControl;
    sdlkkcTable_[SDLK_LALT] = KeyLeftAlt;
    sdlkkcTable_[SDLK_RALT] = KeyRightAlt;
    sdlkkcTable_[SDLK_LGUI] = KeyLeftMeta;
    sdlkkcTable_[SDLK_RGUI] = KeyRightMeta;
}

PlatformSDL::~PlatformSDL()
{
    destroyWindow();
}

bool PlatformSDL::setup()
{
    PlatformInterface::setup();

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        LOG_ERROR << "Failed initializing SDL";
        return false;
    }

    // Log SDL versions
    auto version = SDL_version();
    SDL_GetVersion(&linked);

    LOG_INFO << "Initialized SDL " << version.major << "." << version.minor << "." << version.patch;

    // Check there is a display to render into
    if (SDL_GetNumVideoDisplays() == 0)
    {
        LOG_ERROR << "No video displays found";
        return false;
    }

    // Get the available resolutions
    auto mode = SDL_DisplayMode();
    for (auto i = 0; i < SDL_GetNumDisplayModes(0); i++)
    {
        SDL_GetDisplayMode(0, i, &mode);
        resolutions_.emplace(mode.w, mode.h);
    }

    // Find native resolution
    if (SDL_GetDesktopDisplayMode(0, &mode) == 0)
        nativeResolution_ = findResolution(mode.w, mode.h);

    sortResolutions();

    return true;
}

bool PlatformSDL::createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa)
{
    try
    {
        if (!resolutions_.has(resolution))
            throw Exception() << "Invalid resolution: " << resolution;

        if (!isWindowedModeSupported())
            windowMode = Fullscreen;

        // Surface flags
        auto windowFlags = int(SDL_WINDOW_OPENGL);
        if (windowMode == Fullscreen)
            windowFlags |= SDL_WINDOW_FULLSCREEN;

        // OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        // Set FSAA values if FSAA is on
        if (fsaa != FSAANone)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, int(fsaa));
        }

        // Create SDL surface, this creates the window
        window_ = SDL_CreateWindow(getWindowTitle().toUTF8().as<char>(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   resolution.getWidth(), resolution.getHeight(), windowFlags);
        glContext_ = SDL_GL_CreateContext(window_);
        if (!window_ || !glContext_)
            throw Exception("Failed creating rendering window");

        // Store the original gamma ramps
        SDL_GetWindowGammaRamp(window_, originalGammaRamps_[0].data(), originalGammaRamps_[1].data(),
                               originalGammaRamps_[2].data());

        // Give the new window a black background straight away
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        swap();

        // If using FSAA do a check to see if we got what we asked for
        if (fsaa != FSAANone)
        {
            auto value = 0;
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &value);

            auto actualFSAA = FSAANone;
            if (value == 0)
                actualFSAA = FSAANone;
            else if (value == 2)
                actualFSAA = FSAA2x;
            else if (value == 4)
                actualFSAA = FSAA4x;
            else if (value == 8)
                actualFSAA = FSAA8x;
            else if (value == 16)
                actualFSAA = FSAA16x;
            else
                LOG_WARNING << "SDL chose an unknown FSAA value: " << value << "xAA";

            // Log a warning saying we fell back to an FSAA setting that wasn't what was requested
            if (actualFSAA != fsaa)
            {
                LOG_WARNING << "Fell back to " << value << "xAA";
                fsaa = actualFSAA;
            }
        }

        // Setup keyboard input
        for (auto i = 0; i < KeyLast; i++)
            setIsKeyPressed(KeyConstant(i), false);

        // Setup window manager interactions
        setWindowTitle(windowTitle_);
        if (windowMode == Fullscreen || (windowMode_ == Windowed && isWindowedModeInputLockEnabled_))
        {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            isHoldingInputLock_ = true;
        }

        setGamma(gammas_);

        // Set vertical sync state
        setVerticalSyncEnabled(isVerticalSyncEnabled_);

        currentResolution_ = resolution;
        windowMode_ = windowMode;
        fsaaMode_ = fsaa;
        updatePersistentSettings();

        sendResizeEvent();

        LOG_INFO << "Window created, resolution: " << resolution << " with " << int(fsaaMode_) << "xAA";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e << ", SDL error: " << SDL_GetError();

        destroyWindow();

        return false;
    }
}

void PlatformSDL::destroyWindow()
{
    if (glContext_)
    {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }

    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    currentResolution_ = Resolution::Zero;
    windowMode_ = Windowed;
    fsaaMode_ = FSAANone;

    LOG_INFO << "Window destroyed";
}

void PlatformSDL::centerWindowOnScreen() const
{
    if (window_ && windowMode_ == Windowed)
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

bool PlatformSDL::setWindowTitle(const UnicodeString& title)
{
    windowTitle_ = title;

    if (window_)
        SDL_SetWindowTitle(window_, title.toUTF8().as<char>());

    return true;
}

VoidFunction PlatformSDL::getOpenGLFunctionAddress(const String& function) const
{
    auto fn = SDL_GL_GetProcAddress(function.cStr());
    auto fnVoid = VoidFunction();

    static_assert(sizeof(fn) == sizeof(fnVoid), "Function pointer size mismatch, bitwise copy is invalid");
    memcpy(&fnVoid, &fn, sizeof(fnVoid));

    return fnVoid;
}

void PlatformSDL::setMousePosition(const Vec2& position)
{
    PlatformInterface::setMousePosition(position);

    SDL_WarpMouseInWindow(window_, int(mousePosition_.x), int(getWindowHeightf() - mousePosition_.y - 1.0f));
}

void PlatformSDL::swap()
{
    SDL_GL_SwapWindow(window_);
}

bool PlatformSDL::releaseInputLock()
{
    if (windowMode_ == Windowed)
    {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        isHoldingInputLock_ = false;
    }

    return true;
}

bool PlatformSDL::processEvent(const Event& e)
{
    if (!PlatformInterface::processEvent(e))
        return false;

    if (!e.as<UpdateEvent>())
        return true;

    while (true)
    {
        auto sdlEvent = SDL_Event();
        if (SDL_PollEvent(&sdlEvent) == 0)
            break;

        switch (sdlEvent.type)
        {
            case SDL_WINDOWEVENT:
            {
                if (sdlEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                {
                    events().dispatchEvent(ApplicationGainFocusEvent());

                    if (windowMode_ == Fullscreen || (windowMode_ == Windowed && isWindowedModeInputLockEnabled_))
                    {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        isHoldingInputLock_ = true;
                    }
                }
                else if (sdlEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                    events().dispatchEvent(ApplicationLoseFocusEvent());

                break;
            }

            case SDL_QUIT:
            {
                events().dispatchEvent(ShutdownRequestEvent());
                break;
            }

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                auto isKeyDownEvent = (sdlEvent.type == SDL_KEYDOWN);

                // Convert SDLK code to a KeyConstant enum value
                auto itKey = sdlkkcTable_.find(sdlEvent.key.keysym.sym);
                auto key = itKey == sdlkkcTable_.end() ? KeyNone : itKey->second;

                if (key != KeyNone)
                {
                    // Update the key pressed state
                    setIsKeyPressed(key, isKeyDownEvent);

                    // Don't send messages for alt-tab
                    if (key == KeyTab && (sdlEvent.key.keysym.mod & KMOD_ALT))
                        break;

                    if (isKeyDownEvent)
                    {
                        if (!sdlEvent.key.repeat)
                        {
                            onInputDownEvent(key);

#ifdef MACOSX
                            // Cmd+Q quits the application on Mac OS X
                            if (key == KeyQ && (sdlEvent.key.keysym.mod & (KMOD_LGUI | KMOD_RGUI)))
                                events().dispatchEvent(ShutdownRequestEvent());
#endif
                        }
                    }
                    else
                        onInputUpEvent(key);
                }

                break;
            }

            case SDL_TEXTINPUT:
            {
                events().dispatchEvent(CharacterInputEvent(fromUTF8(sdlEvent.text.text), KeyNone));
                break;
            }

            case SDL_MOUSEMOTION:
            {
                PlatformInterface::setMousePosition(
                    Vec2(float(sdlEvent.motion.x), float(getWindowHeight() - sdlEvent.motion.y - 1)));

                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                auto button = MouseButton();

                if (sdlEvent.button.button == SDL_BUTTON_LEFT)
                    button = LeftMouseButton;
                else if (sdlEvent.button.button == SDL_BUTTON_MIDDLE)
                    button = MiddleMouseButton;
                else if (sdlEvent.button.button == SDL_BUTTON_RIGHT)
                    button = RightMouseButton;
                else
                    break;

                // Store mouse button state
                isMouseButtonPressed_[button] = sdlEvent.button.type == SDL_MOUSEBUTTONDOWN;

                PlatformInterface::setMousePosition(
                    Vec2(float(sdlEvent.button.x), float(getWindowHeight() - sdlEvent.button.y - 1)));

                if (isMouseButtonPressed_[button])
                    onInputDownEvent(button);
                else
                    onInputUpEvent(button);

                break;
            }

            case SDL_MOUSEWHEEL:
            {
                auto direction = MouseWheelEvent::AwayFromUser;
                if (sdlEvent.wheel.y < 0)
                    direction = MouseWheelEvent::TowardsUser;

                events().dispatchEvent(MouseWheelEvent(direction, getMousePosition()));
                break;
            }
        }
    }

    // Update relative mouse value
    auto mouseState = Vec2i();
    SDL_GetRelativeMouseState(&mouseState.x, &mouseState.y);
    mouseRelative_ = mouseState;

    return true;
}

TimeValue PlatformSDL::getTime() const
{
    return TimeValue(int64_t(SDL_GetTicks()));
}

bool PlatformSDL::setGamma(const Color& gammas)
{
    auto ramps = std::array<std::array<uint16_t, 256>, 3>();

    calculateGammaRamp(gammas.r, ramps[0], originalGammaRamps_[0]);
    calculateGammaRamp(gammas.g, ramps[1], originalGammaRamps_[1]);
    calculateGammaRamp(gammas.b, ramps[2], originalGammaRamps_[2]);

    if (!window_ || SDL_SetWindowGammaRamp(window_, ramps[0].data(), ramps[1].data(), ramps[2].data()))
        return false;

    gammas_ = gammas;
    updatePersistentSettings();

    return true;
}

String PlatformSDL::getOperatingSystemName() const
{
    auto pipe = popen("uname -srm", "r");
    if (!pipe)
        return {};

    auto output = std::array<char, 1024>();
    if (!fgets(output.data(), output.size(), pipe) && ferror(pipe))
        LOG_ERROR << "Failed reading uname output";

    pclose(pipe);

    return String(output.data()).trimmed();
}

bool PlatformSDL::openWithDefaultApplication(const UnicodeString& resource) const
{
#ifdef LINUX
    return system(("open \"" + resource + "\"").toUTF8().as<char>()) != -1;
#else
    return false;
#endif
}

}

#endif
