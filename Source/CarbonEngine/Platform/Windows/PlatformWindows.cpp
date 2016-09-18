/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_PLATFORM_WINDOWS

#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/ForceFeedbackEffects.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/Windows/PlatformWindows.h"
#include "CarbonEngine/Resource.h"

namespace Carbon
{

// Window styles used in windowed and fullscreen mode
const auto WindowedStyles = WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX;
const auto FullscreenStyles = WS_POPUP;

const auto WindowClass = L"CarbonWindowClass";
const auto KeyboardBufferSize = 256U;
const auto MouseBufferSize = 256U;

const auto PrimaryDisplayDevice = String("\\\\.\\DISPLAY1");

PlatformWindows::PlatformWindows()
{
    events().addHandler<UpdateEvent>(this, true);

    // Force the main thread to always run on the same CPU, this is required to make QueryPerformanceCounter() work
    // reliably on multi-CPU systems
    SetThreadAffinityMask(GetCurrentThread(), 1);

    // Initialize timing
    auto frequency = LARGE_INTEGER();
    QueryPerformanceFrequency(&frequency);
    setTicksPerSecond(frequency.QuadPart);

    // Mapping from DIK_* to KeyConstant
    dikToKeyConstant_[DIK_0] = Key0;
    dikToKeyConstant_[DIK_1] = Key1;
    dikToKeyConstant_[DIK_2] = Key2;
    dikToKeyConstant_[DIK_3] = Key3;
    dikToKeyConstant_[DIK_4] = Key4;
    dikToKeyConstant_[DIK_5] = Key5;
    dikToKeyConstant_[DIK_6] = Key6;
    dikToKeyConstant_[DIK_7] = Key7;
    dikToKeyConstant_[DIK_8] = Key8;
    dikToKeyConstant_[DIK_9] = Key9;
    dikToKeyConstant_[DIK_A] = KeyA;
    dikToKeyConstant_[DIK_B] = KeyB;
    dikToKeyConstant_[DIK_C] = KeyC;
    dikToKeyConstant_[DIK_D] = KeyD;
    dikToKeyConstant_[DIK_E] = KeyE;
    dikToKeyConstant_[DIK_F] = KeyF;
    dikToKeyConstant_[DIK_G] = KeyG;
    dikToKeyConstant_[DIK_H] = KeyH;
    dikToKeyConstant_[DIK_I] = KeyI;
    dikToKeyConstant_[DIK_J] = KeyJ;
    dikToKeyConstant_[DIK_K] = KeyK;
    dikToKeyConstant_[DIK_L] = KeyL;
    dikToKeyConstant_[DIK_M] = KeyM;
    dikToKeyConstant_[DIK_N] = KeyN;
    dikToKeyConstant_[DIK_O] = KeyO;
    dikToKeyConstant_[DIK_P] = KeyP;
    dikToKeyConstant_[DIK_Q] = KeyQ;
    dikToKeyConstant_[DIK_R] = KeyR;
    dikToKeyConstant_[DIK_S] = KeyS;
    dikToKeyConstant_[DIK_T] = KeyT;
    dikToKeyConstant_[DIK_U] = KeyU;
    dikToKeyConstant_[DIK_V] = KeyV;
    dikToKeyConstant_[DIK_W] = KeyW;
    dikToKeyConstant_[DIK_X] = KeyX;
    dikToKeyConstant_[DIK_Y] = KeyY;
    dikToKeyConstant_[DIK_Z] = KeyZ;
    dikToKeyConstant_[DIK_F1] = KeyF1;
    dikToKeyConstant_[DIK_F2] = KeyF2;
    dikToKeyConstant_[DIK_F3] = KeyF3;
    dikToKeyConstant_[DIK_F4] = KeyF4;
    dikToKeyConstant_[DIK_F5] = KeyF5;
    dikToKeyConstant_[DIK_F6] = KeyF6;
    dikToKeyConstant_[DIK_F7] = KeyF7;
    dikToKeyConstant_[DIK_F8] = KeyF8;
    dikToKeyConstant_[DIK_F9] = KeyF9;
    dikToKeyConstant_[DIK_F10] = KeyF10;
    dikToKeyConstant_[DIK_F11] = KeyF11;
    dikToKeyConstant_[DIK_F12] = KeyF12;
    dikToKeyConstant_[DIK_UPARROW] = KeyUpArrow;
    dikToKeyConstant_[DIK_DOWNARROW] = KeyDownArrow;
    dikToKeyConstant_[DIK_LEFTARROW] = KeyLeftArrow;
    dikToKeyConstant_[DIK_RIGHTARROW] = KeyRightArrow;
    dikToKeyConstant_[DIK_INSERT] = KeyInsert;
    dikToKeyConstant_[DIK_DELETE] = KeyDelete;
    dikToKeyConstant_[DIK_HOME] = KeyHome;
    dikToKeyConstant_[DIK_END] = KeyEnd;
    dikToKeyConstant_[DIK_PRIOR] = KeyPageUp;
    dikToKeyConstant_[DIK_NEXT] = KeyPageDown;
    dikToKeyConstant_[DIK_MINUS] = KeyMinus;
    dikToKeyConstant_[DIK_EQUALS] = KeyEquals;
    dikToKeyConstant_[DIK_BACKSPACE] = KeyBackspace;
    dikToKeyConstant_[DIK_LBRACKET] = KeyLeftBracket;
    dikToKeyConstant_[DIK_RBRACKET] = KeyRightBracket;
    dikToKeyConstant_[DIK_RETURN] = KeyEnter;
    dikToKeyConstant_[DIK_SEMICOLON] = KeySemicolon;
    dikToKeyConstant_[DIK_APOSTROPHE] = KeyApostrophe;
    dikToKeyConstant_[DIK_COMMA] = KeyComma;
    dikToKeyConstant_[DIK_PERIOD] = KeyPeriod;
    dikToKeyConstant_[DIK_SLASH] = KeyForwardSlash;
    dikToKeyConstant_[DIK_OEM_102] = KeyBackSlash;
    dikToKeyConstant_[DIK_ESCAPE] = KeyEscape;
    dikToKeyConstant_[DIK_GRAVE] = KeyGraveAccent;
    dikToKeyConstant_[DIK_CAPSLOCK] = KeyCapsLock;
    dikToKeyConstant_[DIK_TAB] = KeyTab;
    dikToKeyConstant_[DIK_LALT] = KeyLeftAlt;
    dikToKeyConstant_[DIK_RALT] = KeyRightAlt;
    dikToKeyConstant_[DIK_LCONTROL] = KeyLeftControl;
    dikToKeyConstant_[DIK_RCONTROL] = KeyRightControl;
    dikToKeyConstant_[DIK_LSHIFT] = KeyLeftShift;
    dikToKeyConstant_[DIK_RSHIFT] = KeyRightShift;
    dikToKeyConstant_[DIK_LWIN] = KeyLeftMeta;
    dikToKeyConstant_[DIK_RWIN] = KeyRightMeta;
    dikToKeyConstant_[DIK_SPACE] = KeySpacebar;
    dikToKeyConstant_[DIK_NUMPAD0] = KeyNumpad0;
    dikToKeyConstant_[DIK_NUMPAD1] = KeyNumpad1;
    dikToKeyConstant_[DIK_NUMPAD2] = KeyNumpad2;
    dikToKeyConstant_[DIK_NUMPAD3] = KeyNumpad3;
    dikToKeyConstant_[DIK_NUMPAD4] = KeyNumpad4;
    dikToKeyConstant_[DIK_NUMPAD5] = KeyNumpad5;
    dikToKeyConstant_[DIK_NUMPAD6] = KeyNumpad6;
    dikToKeyConstant_[DIK_NUMPAD7] = KeyNumpad7;
    dikToKeyConstant_[DIK_NUMPAD8] = KeyNumpad8;
    dikToKeyConstant_[DIK_NUMPAD9] = KeyNumpad9;
    dikToKeyConstant_[DIK_NUMPADPLUS] = KeyNumpadPlus;
    dikToKeyConstant_[DIK_NUMPADMINUS] = KeyNumpadMinus;
    dikToKeyConstant_[DIK_NUMPADEQUALS] = KeyNumpadEquals;
    dikToKeyConstant_[DIK_NUMPADENTER] = KeyNumpadEnter;
    dikToKeyConstant_[DIK_NUMPADCOMMA] = KeyNumpadComma;
    dikToKeyConstant_[DIK_NUMPADPERIOD] = KeyNumpadPeriod;
    dikToKeyConstant_[DIK_NUMPADSLASH] = KeyNumpadForwardSlash;
    dikToKeyConstant_[DIK_NUMPADSTAR] = KeyNumpadStar;
    dikToKeyConstant_[DIK_KANJI] = KeyKanji;
}

PlatformWindows::~PlatformWindows()
{
    events().removeHandler(this);

    destroyWindow();
}

bool PlatformWindows::setup()
{
    PlatformInterface::setup();

    // Enumerate supported resolutions
    auto mode = 0U;
    while (true)
    {
        auto devMode = DEVMODE();
        devMode.dmSize = sizeof(DEVMODE);

        if (EnumDisplaySettings(nullptr, mode++, &devMode) == FALSE)
            break;

        resolutions_.emplace(devMode.dmPelsWidth, devMode.dmPelsHeight);
    }

    // Store native resolution
    auto hDisplayDC = CreateDC(L"DISPLAY", nullptr, nullptr, nullptr);
    nativeResolution_ = findResolution(GetDeviceCaps(hDisplayDC, HORZRES), GetDeviceCaps(hDisplayDC, VERTRES));
    DeleteDC(hDisplayDC);

    sortResolutions();

    if (!registerWindowClass())
    {
        LOG_ERROR << "Failed registering window class";
        return false;
    }

#ifdef CARBON_INCLUDE_OCULUSRIFT
    oculusRiftInitialize();
#endif

    return true;
}

uintptr_t PlatformWindows::getPlatformSpecificValue(PlatformSpecificValue value) const
{
    switch (value)
    {
        case WindowsHWnd:
            return uintptr_t(hWnd_);
        case WindowsHIcon:
            return uintptr_t(hIcon_);

#ifdef CARBON_INCLUDE_OCULUSRIFT
        case OculusRiftSession:
            return uintptr_t(oculusRiftSession_);
#endif
    }

    return 0;
}

bool PlatformWindows::setPlatformSpecificValue(PlatformSpecificValue value, uintptr_t newValue)
{
    if (value == WindowsHIcon)
    {
        hIcon_ = HICON(newValue);

        if (hWnd_)
            SetClassLongPtr(hWnd_, GCLP_HICON, LONG_PTR(hIcon_));

        return true;
    }

    return false;
}

bool PlatformWindows::changeScreenResolution(const Resolution& resolution) const
{
    auto dmScreenSettings = DEVMODE();
    dmScreenSettings.dmSize = sizeof(DEVMODE);
    dmScreenSettings.dmPelsWidth = resolution.getWidth();
    dmScreenSettings.dmPelsHeight = resolution.getHeight();
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmDisplayFrequency = 60;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    // Set the new screen resolution
    if (ChangeDisplaySettingsEx(nullptr, &dmScreenSettings, nullptr, CDS_FULLSCREEN, nullptr) != DISP_CHANGE_SUCCESSFUL)
    {
        LOG_ERROR << "ChangeDisplaySettingsEx(" << resolution << ") failed";
        return false;
    }

    LOG_INFO << "Changed resolution to " << uint(dmScreenSettings.dmPelsWidth) << "x"
             << uint(dmScreenSettings.dmPelsHeight);

    return true;
}

bool PlatformWindows::registerWindowClass() const
{
    auto windowClass = WNDCLASSEX();
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = WNDPROC(staticWindowProc);
    windowClass.hInstance = Globals::getHInstance();
    windowClass.hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH));
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WindowClass;

    // If an icon has been set then use it, otherwise use the default icon
    if (hIcon_)
        windowClass.hIcon = hIcon_;
    else
        windowClass.hIcon = LoadIcon(Globals::getHInstance(), MAKEINTRESOURCE(IDI_CARBON));

    return RegisterClassEx(&windowClass) != 0;
}

void PlatformWindows::centerWindowOnScreen() const
{
    // Query monitor's current rect, this tells us both the resolution and its position in the situation when there are
    // multiple monitors
    auto info = MONITORINFOEX();
    info.cbSize = sizeof(info);
    if (!GetMonitorInfo(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY), &info))
        return;

    auto screenWidth = info.rcMonitor.right - info.rcMonitor.left;
    auto screenHeight = info.rcMonitor.bottom - info.rcMonitor.top;

    auto rect = RECT();
    GetWindowRect(hWnd_, &rect);

    auto x = info.rcMonitor.left + (screenWidth - (rect.right - rect.left)) / 2;
    auto y = info.rcMonitor.top + (screenHeight - (rect.bottom - rect.top)) / 2;

    MoveWindow(hWnd_, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

void PlatformWindows::createRawWindow(const RECT& rect, WindowMode windowMode)
{
    auto styles = (windowMode == Windowed) ? WindowedStyles : FullscreenStyles;

    hWnd_ = CreateWindow(WindowClass, windowTitle_.toUTF16().as<wchar_t>(), styles, 0, 0, rect.right - rect.left,
                         rect.bottom - rect.top, HWND_DESKTOP, 0, Globals::getHInstance(), nullptr);

    if (!hWnd_)
        throw Exception("Failed creating window");

    hDC_ = GetDC(hWnd_);
    if (!hDC_)
        throw Exception("Failed getting window DC");
}

void PlatformWindows::closeRawWindow()
{
    // Close window
    if (hWnd_)
    {
        if (hDC_)
        {
            SetDeviceGammaRamp(hDC_, originalGammaRamps_.data());

            ReleaseDC(hWnd_, hDC_);
            hDC_ = nullptr;
        }

        // Destroy the window we created
        DestroyWindow(hWnd_);
        hWnd_ = nullptr;
    }
}

#ifdef CARBON_INCLUDE_OPENGL11

void PlatformWindows::createGLWindow(const RECT& rect, WindowMode windowMode,
                                     const PIXELFORMATDESCRIPTOR& pixelFormatDescriptor, int pf)
{
    createRawWindow(rect, windowMode);

    // If pf is -1 then we need to choose a pixel format
    if (pf == -1)
    {
        pf = ChoosePixelFormat(hDC_, &pixelFormatDescriptor);
        if (!pf)
            throw Exception("Failed choosing pixel format");
    }

    if (!SetPixelFormat(hDC_, pf, &pixelFormatDescriptor))
        throw Exception("Failed setting pixel format");

    hGLRC_ = wglCreateContext(hDC_);
    if (!hGLRC_)
        throw Exception("Failed creating context");

    if (!wglMakeCurrent(hDC_, hGLRC_))
        throw Exception("Failed making RC current");
}

void PlatformWindows::createGLWindow(const RECT& rect, WindowMode windowMode, FSAAMode fsaa)
{
    createRawWindow(rect, windowMode);

    auto flags =
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE | PFD_SUPPORT_COMPOSITION;

    // Setup pixel format
    auto pixelFormatDescriptor = PIXELFORMATDESCRIPTOR();
    pixelFormatDescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormatDescriptor.nVersion = 1;
    pixelFormatDescriptor.dwFlags = flags;
    pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
    pixelFormatDescriptor.cColorBits = 24;
    pixelFormatDescriptor.cAlphaBits = 8;
    pixelFormatDescriptor.cDepthBits = 24;
    pixelFormatDescriptor.cStencilBits = 8;

    createGLWindow(rect, windowMode, pixelFormatDescriptor, -1);

    // Do FSAA setup
    if (fsaa != FSAANone)
    {
        auto pixelFormat = -1;

        try
        {
            // For FSAA we check for a pixel format that supports the requested number of samples. If one is found then
            // we recreate the main window and set it to use the new pixel format, otherwise we iterate down the AA
            // levels starting at the one requested till we find a supported FSAA mode

            auto wglChoosePixelFormatARB =
                PlatformInterface::getOpenGLFunctionAddress<PFnwglChoosePixelFormatARB>("wglChoosePixelFormatARB");
            if (!wglChoosePixelFormatARB)
                throw Exception("Missing WGL extensions required for FSAA");

            while (true)
            {
                auto attributes = std::array<int, 20>{{WGL_DRAW_TO_WINDOW_ARB,
                                                       GL_TRUE,
                                                       WGL_SUPPORT_OPENGL_ARB,
                                                       GL_TRUE,
                                                       WGL_ACCELERATION_ARB,
                                                       WGL_FULL_ACCELERATION_ARB,
                                                       WGL_COLOR_BITS_ARB,
                                                       24,
                                                       WGL_ALPHA_BITS_ARB,
                                                       8,
                                                       WGL_DEPTH_BITS_ARB,
                                                       24,
                                                       WGL_DOUBLE_BUFFER_ARB,
                                                       GL_TRUE,
                                                       WGL_SAMPLE_BUFFERS_ARB,
                                                       GL_TRUE,
                                                       WGL_SAMPLES_ARB,
                                                       int(fsaa),
                                                       0,
                                                       0}};

                auto numFormats = 0U;

                // Select a FSAA pixel format to use
                if (!wglChoosePixelFormatARB(hDC_, attributes.data(), nullptr, 1, &pixelFormat, &numFormats))
                    throw Exception("wglChoosePixelFormatARB() failed");

                // Check we got at least one format
                if (!numFormats)
                {
                    // If 2xAA failed to find a pixel format then FSAA is just not available
                    if (fsaa == FSAA2x)
                        throw Exception("No FSAA modes are supported by this hardware");

                    // Otherwise, try to find a pixel format for the next lowest FSAA level
                    auto nextLowest = FSAAMode(int(fsaa) / 2);
                    LOG_INFO << int(fsaa) << "xAA not supported, trying " << nextLowest << "xAA";
                    fsaa = nextLowest;
                }
                else
                {
                    // Found an FSAA pixel format, so we can now create the final window
                    closeGLWindow();
                    createGLWindow(rect, windowMode, pixelFormatDescriptor, pixelFormat);

                    break;
                }
            }
        }
        catch (const Exception& e)
        {
            fsaa = FSAANone;
            LOG_INFO << e << ", not using FSAA";
        }
    }

    fsaaMode_ = fsaa;
}

void PlatformWindows::closeGLWindow()
{
    if (hWnd_ && hDC_)
    {
        wglMakeCurrent(nullptr, nullptr);

        if (hGLRC_)
        {
            wglDeleteContext(hGLRC_);
            hGLRC_ = nullptr;
        }
    }
}

#endif

bool PlatformWindows::createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa)
{
    try
    {
        if (!resolutions_.has(resolution))
            throw Exception() << "Invalid resolution: " << resolution;

        if (!isWindowedModeSupported())
            windowMode = Fullscreen;

        // If fullscreen mode is chosen then change the screen resolution
        if (windowMode == Fullscreen)
        {
            if (!changeScreenResolution(resolution))
            {
                LOG_WARNING << "Failed changing screen resolution, falling back to windowed mode";
                windowMode = Windowed;
            }
        }

        // Get window rectangle and adjust for border sizes if in windowed mode
        auto windowRect = RECT{0, 0, LONG(resolution.getWidth()), LONG(resolution.getHeight())};
        if (windowMode == Windowed)
            AdjustWindowRect(&windowRect, WindowedStyles, 0);

#ifdef CARBON_INCLUDE_OPENGL11
        // Create the main window
        createGLWindow(windowRect, windowMode, fsaa);
#endif
        if (!hWnd_)
            createRawWindow(windowRect, windowMode);

        // Store the current gamma ramp
        if (!GetDeviceGammaRamp(hDC_, originalGammaRamps_.data()))
        {
            // Fall back to an identity gamma curve as the default
            calculateGammaRamp(1.0f, originalGammaRamps_[0], originalGammaRamps_[0]);
            calculateGammaRamp(1.0f, originalGammaRamps_[1], originalGammaRamps_[1]);
            calculateGammaRamp(1.0f, originalGammaRamps_[2], originalGammaRamps_[2]);
        }

        setGamma(gammas_);

        // Setup DirectInput
        if (!setupDirectInput())
            throw Exception("Failed setting up DirectInput");

        // Set vertical sync state
        setVerticalSyncEnabled(isVerticalSyncEnabled_);

        // Position and show the window
        centerWindowOnScreen();
        ShowWindow(hWnd_, SW_SHOWNORMAL);
        while (ShowCursor(FALSE) >= 0)
            ;

#ifdef CARBON_INCLUDE_OPENGL11
        // Give the new window a black background straight away
        if (isOpenGLWindow())
        {
            glClear(GL_COLOR_BUFFER_BIT);
            swap();
        }
#endif
        currentResolution_ = resolution;
        windowMode_ = windowMode;
        updatePersistentSettings();

        sendResizeEvent();

        LOG_INFO << "Window created for the "
                 << InterfaceRegistry<GraphicsInterface>::getActiveImplementation()->getName()
                 << " graphics backend, resolution: " << resolution << " with " << int(fsaaMode_) << "xAA";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        // Log Windows API error
        LOG_ERROR << "GetLastError() returned " << uint(GetLastError());

        destroyWindow();

        return false;
    }
}

void PlatformWindows::destroyWindow()
{
    closeDirectInput();

#ifdef CARBON_INCLUDE_OPENGL11
    if (isOpenGLWindow())
        closeGLWindow();
#endif

    closeRawWindow();

    ChangeDisplaySettingsEx(nullptr, nullptr, nullptr, 0, nullptr);
    ClipCursor(nullptr);
    while (ShowCursor(TRUE) < 0)
        ;

    currentResolution_ = Resolution::Zero;
    windowMode_ = Windowed;
    fsaaMode_ = FSAANone;

    LOG_INFO << "Window destroyed";
}

bool PlatformWindows::setupDirectInput()
{
    try
    {
        closeDirectInput();

        // DirectInput
        if (FAILED(DirectInput8Create(Globals::getHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8,
                                      reinterpret_cast<void**>(&DI_), nullptr)))
            throw Exception("Failed creating DirectInput object");

        // Keyboard device
        if (FAILED(DI_->CreateDevice(GUID_SysKeyboard, &keyboardDevice_, nullptr)))
            throw Exception("Failed creating keyboard device");

        if (FAILED(keyboardDevice_->SetDataFormat(&c_dfDIKeyboard)))
            throw Exception("Failed setting keyboard device data format");

        if (FAILED(keyboardDevice_->SetCooperativeLevel(hWnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY)))
            throw Exception("Failed setting keyboard cooperative level");

        // Request buffered keyboard input
        auto property = DIPROPDWORD();
        property.diph.dwSize = sizeof(DIPROPDWORD);
        property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        property.diph.dwObj = 0;
        property.diph.dwHow = DIPH_DEVICE;
        property.dwData = KeyboardBufferSize;
        if (FAILED(keyboardDevice_->SetProperty(DIPROP_BUFFERSIZE, &property.diph)))
            throw Exception("Failed setting keyboard input buffer size");

        // Mouse device
        if (FAILED(DI_->CreateDevice(GUID_SysMouse, &mouseDevice_, nullptr)))
            throw Exception("Failed creating mouse device");

        if (FAILED(mouseDevice_->SetDataFormat(&c_dfDIMouse)))
            throw Exception("Failed setting mouse device data format");

        if (FAILED(mouseDevice_->SetCooperativeLevel(hWnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
            throw Exception("Failed setting mouse cooperative level");

        // Request buffered mouse input
        property.diph.dwSize = sizeof(DIPROPDWORD);
        property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        property.diph.dwObj = 0;
        property.diph.dwHow = DIPH_DEVICE;
        property.dwData = MouseBufferSize;
        if (FAILED(mouseDevice_->SetProperty(DIPROP_BUFFERSIZE, &property.diph)))
            throw Exception("Failed setting mouse input buffer size");

        // Enumerate game controller devices
        refreshGameControllerList();

        LOG_INFO << "DirectInput setup complete";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        closeDirectInput();

        return false;
    }
}

void PlatformWindows::closeDirectInput()
{
    // Close DirectInput
    if (DI_)
    {
        if (keyboardDevice_)
        {
            keyboardDevice_->Unacquire();
            keyboardDevice_->Release();
            keyboardDevice_ = nullptr;
        }

        if (mouseDevice_)
        {
            mouseDevice_->Unacquire();
            mouseDevice_->Release();
            mouseDevice_ = nullptr;
        }

        clearGameControllers();

        DI_->Release();
        DI_ = nullptr;
    }
}

PlatformWindows::GameController* PlatformWindows::findGameController(unsigned int controllerID)
{
    for (auto& controller : gameControllers_)
    {
        if (controllerID == 0 || controller.id == controllerID)
            return &controller;
    }

    return nullptr;
}

void PlatformWindows::clearGameControllers()
{
    for (auto& controller : gameControllers_)
    {
        // Clean up any force feedback effects
        for (auto& effect : controller.ffEffects)
        {
            delete effect.second.effect;
            effect.second.effect = nullptr;

            effect.second.diEffect->Release();
        }

        // Release device
        controller.device->Unacquire();
        controller.device->Release();
    }

    gameControllers_.clear();
}

void PlatformWindows::refreshGameControllerList()
{
    try
    {
        clearGameControllers();

        if (FAILED(DI_->EnumDevices(DI8DEVCLASS_GAMECTRL, enumGameControllersCallback, nullptr, DIEDFL_ATTACHEDONLY)))
            throw Exception("Failed enumerating game controllers");

        for (auto& controller : gameControllers_)
        {
            auto device = controller.device;

            if (FAILED(device->SetDataFormat(&c_dfDIJoystick2)))
                throw Exception("Failed setting game controller data format");

            if (FAILED(device->SetCooperativeLevel(hWnd_, DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
                throw Exception("Failed setting game controller cooperative level");

            if (FAILED(device->EnumObjects(enumGameControllerAxesCallback, &controller, DIDFT_AXIS)))
                throw Exception("Failed enumerating game controller axes");

            // Get the display name of this device
            auto prop = DIPROPSTRING();
            prop.diph.dwHeaderSize = sizeof(prop.diph);
            prop.diph.dwSize = sizeof(prop);
            if (FAILED(device->GetProperty(DIPROP_PRODUCTNAME, &prop.diph)))
                LOG_WARNING << "Failed getting instance name for game controller device";

            // Get the instance number of this device
            auto prop2 = DIPROPDWORD();
            prop2.diph.dwHeaderSize = sizeof(prop2.diph);
            prop2.diph.dwSize = sizeof(prop2);
            if (FAILED(device->GetProperty(DIPROP_JOYSTICKID, &prop2.diph)))
                LOG_WARNING << "Failed getting ID for game controller device";

            // Convert name to an ASCII string
            controller.displayName = fromUTF16(prop.wsz);

            controller.id = 1 + prop2.dwData;

            LOG_INFO << "Found game controller: " << controller.displayName
                     << ", force feedback axis count: " << controller.forceFeedbackAxisCount;
        }
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;
    }
}

Vector<unsigned int> PlatformWindows::getGameControllers() const
{
    return gameControllers_.map<unsigned int>([](const GameController& controller) { return controller.id; });
}

UnicodeString PlatformWindows::getGameControllerDisplayName(unsigned int controllerID)
{
    auto controller = findGameController(controllerID);

    return controller ? controller->displayName : UnicodeString::Empty;
}

const GameControllerState& PlatformWindows::getGameControllerState(unsigned int controllerID)
{
    auto controller = findGameController(controllerID);

    return controller ? controller->state : GameControllerState::Empty;
}

BOOL CALLBACK PlatformWindows::enumGameControllersCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    auto device = LPDIRECTINPUTDEVICE8(nullptr);

    // Attempt to obtain an interface to the enumerated device
    if (!FAILED(static_cast<PlatformWindows&>(platform()).DI_->CreateDevice(lpddi->guidInstance, &device, nullptr)) &&
        device)
        static_cast<PlatformWindows&>(platform()).gameControllers_.emplace(device);

    return DIENUM_CONTINUE;
}

BOOL CALLBACK PlatformWindows::enumGameControllerAxesCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    // Check if this axis can have force feedback effects applied to it
    if (lpddoi->dwFlags & DIDOI_FFACTUATOR)
        reinterpret_cast<GameController*>(pvRef)->forceFeedbackAxisCount++;

    // Set the range of this axis to between -1000 and 1000
    auto diPropRange = DIPROPRANGE();
    diPropRange.diph.dwSize = sizeof(DIPROPRANGE);
    diPropRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diPropRange.diph.dwHow = DIPH_BYID;
    diPropRange.diph.dwObj = lpddoi->dwType;
    diPropRange.lMin = -1000;
    diPropRange.lMax = 1000;

    if (FAILED(reinterpret_cast<GameController*>(pvRef)->device->SetProperty(DIPROP_RANGE, &diPropRange.diph)))
        LOG_ERROR << "Failed setting range of game controller axis";

    return DIENUM_CONTINUE;
}

unsigned int PlatformWindows::createForceFeedbackEffect(unsigned int controllerID, const ForceFeedbackEffect& effect)
{
    // Find the controller to create the new effect for
    auto controller = findGameController(controllerID);
    if (!controller)
        return 0;

    // Check that this game controller supports force feedback
    if (!controller->forceFeedbackAxisCount)
    {
        LOG_ERROR << "Game controller has no force feedback axes";
        return 0;
    }

    auto newEffect = LPDIRECTINPUTEFFECT(nullptr);
    auto direction = std::array<LONG, 2>{{effect.direction[0], effect.direction[1]}};
    auto rgdwAxes = std::array<DWORD, 2>{{DIJOFS_X, DIJOFS_Y}};

    auto diEffect = DIEFFECT();
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwDuration = effect.duration;
    diEffect.dwGain = effect.gain;
    diEffect.rglDirection = direction.data();
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.cAxes = controller->forceFeedbackAxisCount > 2 ? 2 : controller->forceFeedbackAxisCount;
    diEffect.rgdwAxes = rgdwAxes.data();

    auto guid = GUID();
    auto diCF = DICONSTANTFORCE();
    auto diRF = DIRAMPFORCE();
    auto diPF = DIPERIODIC();

    auto effectCopy = pointer_to<ForceFeedbackEffect>::type();

    if (auto cf = dynamic_cast<const ForceFeedbackConstantForceEffect*>(&effect))
    {
        diEffect.cbTypeSpecificParams = sizeof(diCF);
        diEffect.lpvTypeSpecificParams = &diCF;
        diCF.lMagnitude = cf->magnitude;

        effectCopy = new ForceFeedbackConstantForceEffect(*cf);
        guid = GUID_ConstantForce;
    }
    else if (auto rf = dynamic_cast<const ForceFeedbackRampForceEffect*>(&effect))
    {
        diEffect.cbTypeSpecificParams = sizeof(diRF);
        diEffect.lpvTypeSpecificParams = &diRF;
        diRF.lStart = rf->startMagnitude;
        diRF.lEnd = rf->endMagnitude;

        effectCopy = new ForceFeedbackRampForceEffect(*rf);
        guid = GUID_RampForce;
    }
    else if (auto pf = dynamic_cast<const ForceFeedbackPeriodicEffect*>(&effect))
    {
        diEffect.cbTypeSpecificParams = sizeof(diPF);
        diEffect.lpvTypeSpecificParams = &diPF;
        diPF.dwMagnitude = pf->magnitude;
        diPF.lOffset = pf->offset;
        diPF.dwPhase = pf->phase;
        diPF.dwPeriod = pf->period;

        effectCopy = new ForceFeedbackPeriodicEffect(*pf);

        switch (pf->waveform)
        {
            case ForceFeedbackPeriodicEffect::WaveformSquare:
                guid = GUID_Square;
                break;
            case ForceFeedbackPeriodicEffect::WaveformSine:
                guid = GUID_Sine;
                break;
            case ForceFeedbackPeriodicEffect::WaveformTriangle:
                guid = GUID_Triangle;
                break;
            case ForceFeedbackPeriodicEffect::WaveformSawtoothUp:
                guid = GUID_SawtoothUp;
                break;
            case ForceFeedbackPeriodicEffect::WaveformSawtoothDown:
                guid = GUID_SawtoothDown;
                break;
        }
    }

    if (FAILED(controller->device->CreateEffect(guid, &diEffect, &newEffect, nullptr)))
    {
        LOG_ERROR << "Failed creating force feedback effect";
        return 0;
    }

    auto id = controller->ffEffects.size() + 1;
    controller->ffEffects[id] = GameController::FFEffect(effectCopy, newEffect);

    return id;
}

bool PlatformWindows::deleteForceFeedbackEffect(unsigned int controllerID, unsigned int effectID)
{
    // Find the controller
    auto controller = findGameController(controllerID);
    if (!controller)
        return false;

    // Check the effect ID is valid
    if (controller->ffEffects.find(effectID) == controller->ffEffects.end())
        return false;

    // Delete the effect
    delete controller->ffEffects[effectID].effect;
    controller->ffEffects[effectID].effect = nullptr;

    controller->ffEffects[effectID].diEffect->Release();
    controller->ffEffects.erase(effectID);

    return true;
}

bool PlatformWindows::playForceFeedbackEffect(unsigned int controllerID, unsigned int effectID, unsigned int iterations)
{
    // Find the controller
    auto controller = findGameController(controllerID);
    if (!controller)
        return false;

    // Check the effect ID is valid
    if (controller->ffEffects.find(effectID) == controller->ffEffects.end())
        return false;

    return SUCCEEDED(controller->ffEffects[effectID].diEffect->Start(iterations, 0));
}

bool PlatformWindows::stopForceFeedbackEffect(unsigned int controllerID, unsigned int effectID)
{
    // Find the controller
    auto controller = findGameController(controllerID);
    if (!controller)
        return false;

    // Check the effect ID is valid
    if (controller->ffEffects.find(effectID) == controller->ffEffects.end())
        return false;

    return SUCCEEDED(controller->ffEffects[effectID].diEffect->Stop());
}

bool PlatformWindows::setWindowTitle(const UnicodeString& title)
{
    windowTitle_ = title;

    if (hWnd_)
        SetWindowText(hWnd_, title.toUTF16().as<wchar_t>());

    return true;
}

VoidFunction PlatformWindows::getOpenGLFunctionAddress(const String& function) const
{
#ifdef CARBON_INCLUDE_OPENGL11
    if (isOpenGLWindow())
        return reinterpret_cast<VoidFunction>(wglGetProcAddress(function.cStr()));
#endif

    return nullptr;
}

void PlatformWindows::setMousePosition(const Vec2& position)
{
    PlatformInterface::setMousePosition(position);

    auto point = POINT{LONG(mousePosition_.x), LONG(getWindowHeightf() - mousePosition_.y - 1.0f)};

    ClientToScreen(hWnd_, &point);
    SetCursorPos(point.x, point.y);
}

void PlatformWindows::swap()
{
#ifdef CARBON_INCLUDE_OPENGL11
    if (isOpenGLWindow())
        SwapBuffers(hDC_);
#endif
}

bool PlatformWindows::setVerticalSyncEnabled(bool enabled)
{
#ifdef CARBON_INCLUDE_OPENGL11
    if (isOpenGLWindow())
    {
        auto wglSwapIntervalEXT =
            PlatformInterface::getOpenGLFunctionAddress<PFnwglSwapIntervalEXT>("wglSwapIntervalEXT");
        if (!wglSwapIntervalEXT)
            return false;

        if (enabled)
            LOG_WARNING << "Vertical sync is being enabled, possible incompatibility with Oculus Rift";

        if (!wglSwapIntervalEXT(enabled ? GL_TRUE : GL_FALSE))
        {
            LOG_ERROR << "wglSwapIntervalEXT() call failed";
            return false;
        }

        isVerticalSyncEnabled_ = enabled;
        updatePersistentSettings();

        return true;
    }
#endif

    return false;
}

bool PlatformWindows::releaseInputLock()
{
    if (windowMode_ == Windowed)
    {
        ClipCursor(nullptr);
        isHoldingInputLock_ = false;
    }

    return true;
}

bool PlatformWindows::processEvent(const Event& e)
{
    if (!PlatformInterface::processEvent(e))
        return false;

    if (!e.as<UpdateEvent>())
        return true;

    // Run the main window message pump
    auto msg = MSG();
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    mouseRelative_ = Vec2::Zero;

    // DirectInput buffered mouse input
    auto mouseData = std::array<DIDEVICEOBJECTDATA, MouseBufferSize>();
    auto mouseDataSize = DWORD(mouseData.size());
    if (FAILED(mouseDevice_->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), mouseData.data(), &mouseDataSize, 0)))
        mouseDevice_->Acquire();
    else
    {
        for (auto i = 0U; i < mouseDataSize; i++)
        {
            const auto& data = mouseData[i];

            switch (data.dwOfs)
            {
                case DIMOFS_BUTTON0:
                    isMouseButtonPressed_[LeftMouseButton] = (data.dwData & 0x80) != 0;
                    break;

                case DIMOFS_BUTTON1:
                    isMouseButtonPressed_[RightMouseButton] = (data.dwData & 0x80) != 0;
                    break;

                case DIMOFS_BUTTON2:
                    isMouseButtonPressed_[MiddleMouseButton] = (data.dwData & 0x80) != 0;
                    break;

                case DIMOFS_X:
                    mouseRelative_.x += float(int(data.dwData));
                    break;

                case DIMOFS_Y:
                    mouseRelative_.y -= float(int(data.dwData));
                    break;
            }
        }
    }

    // DirectInput buffered keyboard input
    auto keyboardData = std::array<DIDEVICEOBJECTDATA, KeyboardBufferSize>();
    auto keyboardDataSize = DWORD(keyboardData.size());
    if (FAILED(keyboardDevice_->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), keyboardData.data(), &keyboardDataSize, 0)))
        keyboardDevice_->Acquire();
    else
    {
        for (auto i = 0U; i < keyboardDataSize; i++)
        {
            auto key = dikToKeyConstant_[keyboardData[i].dwOfs];

            if (key == KeyNone)
                continue;

            setIsKeyPressed(key, (keyboardData[i].dwData & 0x80) != 0);

            if (keyboardData[i].dwData & 0x80)
                onInputDownEvent(key);
            else
                onInputUpEvent(key);
        }
    }

    // Read the immediate keyboard state to ensure everything is right up to date
    auto immediateKeyboardState = std::array<byte_t, 256>();
    if (FAILED(keyboardDevice_->GetDeviceState(immediateKeyboardState.size(), &immediateKeyboardState[0])))
        keyboardDevice_->Acquire();
    else
    {
        for (auto i = 0U; i < immediateKeyboardState.size(); i++)
        {
            auto key = dikToKeyConstant_[i];
            if (key != KeyNone)
                setIsKeyPressed(key, (immediateKeyboardState[i] & 0x80) != 0);
        }
    }

    // Get game controller states
    for (auto i = 0U; i < gameControllers_.size(); i++)
    {
        auto device = gameControllers_[i].device;
        auto deviceState = DIJOYSTATE2();

        if (FAILED(device->Poll()))
            device->Acquire();

        if (device->GetDeviceState(sizeof(deviceState), &deviceState) == DI_OK)
        {
            auto& state = gameControllers_[i].state;

            state.axisPosition.x = float(deviceState.lX) / 1000.0f;
            state.axisPosition.y = float(deviceState.lY) / 1000.0f;
            state.axisPosition.z = float(deviceState.lZ) / 1000.0f;

            // Update button states
            state.isButtonPressed.resize(sizeof(deviceState.rgbButtons), false);
            for (auto j = 0U; j < state.isButtonPressed.size(); j++)
            {
                auto newState = deviceState.rgbButtons[j] != 0;

                // Send out an event if the button state has changed
                if (newState != state.isButtonPressed[j])
                {
                    if (newState)
                        events().dispatchEvent(GameControllerButtonDownEvent(gameControllers_[i].id, j));
                    else
                        events().dispatchEvent(GameControllerButtonUpEvent(gameControllers_[i].id, j));
                }

                state.isButtonPressed[j] = newState;
            }
        }
    }

#ifdef CARBON_INCLUDE_OCULUSRIFT
    oculusRiftUpdate();
#endif

    return true;
}

void PlatformWindows::onInputDownEvent(KeyConstant key)
{
    // Suppress tab and F4 key events when Alt is held down
    if ((key == KeyTab || key == KeyF4) && (GetKeyState(VK_MENU) & 0x8000))
    {
        keyState_[inputMappings_[key].target].lastKeyDownEventTime = getTime();
        return;
    }

    PlatformInterface::onInputDownEvent(key);
}

bool PlatformWindows::setGamma(const Color& gammas)
{
    const auto minimumGamma = 0.25f;
    const auto maximumGamma = 4.4f;

    auto r = gammas.r > 0.0f ? Math::clamp(gammas.r, minimumGamma, maximumGamma) : 0.0f;
    auto g = gammas.g > 0.0f ? Math::clamp(gammas.g, minimumGamma, maximumGamma) : 0.0f;
    auto b = gammas.b > 0.0f ? Math::clamp(gammas.b, minimumGamma, maximumGamma) : 0.0f;

    auto ramps = std::array<std::array<uint16_t, 256>, 3>();

    calculateGammaRamp(r, ramps[0], originalGammaRamps_[0]);
    calculateGammaRamp(g, ramps[1], originalGammaRamps_[1]);
    calculateGammaRamp(b, ramps[2], originalGammaRamps_[2]);

    if (!SetDeviceGammaRamp(hDC_, ramps.data()))
        return false;

    gammas_.setRGBA(r, g, b, 1.0f);
    updatePersistentSettings();

    return true;
}

String PlatformWindows::getOperatingSystemName() const
{
    return "Microsoft Windows";
}

unsigned int PlatformWindows::getCPUCount() const
{
    auto sysinfo = SYSTEM_INFO();
    GetSystemInfo(&sysinfo);

    return sysinfo.dwNumberOfProcessors;
}

uint64_t PlatformWindows::getCPUFrequency() const
{
    auto hKey = HKEY();

    auto result =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
        return 0;

    auto megahertz = DWORD();
    auto outSize = DWORD(sizeof(DWORD));
    RegQueryValueEx(hKey, L"~MHz", nullptr, nullptr, LPBYTE(&megahertz), &outSize);
    RegCloseKey(hKey);

    return uint64_t(megahertz) * 1000000;
}

uint64_t PlatformWindows::getSystemMemorySize() const
{
    auto status = MEMORYSTATUSEX();
    status.dwLength = sizeof(status);

    GlobalMemoryStatusEx(&status);

    return status.ullTotalPhys;
}

void PlatformWindows::clipCursorToWindow() const
{
    auto clipRect = RECT();
    auto adjust = RECT();

    AdjustWindowRect(&adjust, GetWindowLong(hWnd_, GWL_STYLE), 0);
    GetWindowRect(hWnd_, &clipRect);

    clipRect.left -= adjust.left;
    clipRect.top -= adjust.top;
    clipRect.right -= adjust.right;
    clipRect.bottom -= adjust.bottom;

    ClipCursor(&clipRect);
}

LRESULT PlatformWindows::staticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return static_cast<PlatformWindows&>(platform()).windowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT PlatformWindows::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_ACTIVATE:
        {
            if (windowMode_ == Fullscreen)
            {
                if (LOWORD(wParam) == WA_INACTIVE)
                {
                    ChangeDisplaySettingsEx(nullptr, nullptr, nullptr, 0, nullptr);
                    ShowWindow(hWnd, SW_MINIMIZE);
                }
                else
                {
                    changeScreenResolution(getCurrentResolution());
                    SetWindowPos(hWnd_, HWND_TOP, 0, 0, getWindowWidth(), getWindowHeight(), 0);
                }
            }

            auto isMinimized = HIWORD(wParam) != 0;

            if (LOWORD(wParam) == WA_INACTIVE || isMinimized)
            {
                events().dispatchEvent(ApplicationLoseFocusEvent());
                releaseInputLock();
            }
            else
            {
                events().dispatchEvent(ApplicationGainFocusEvent());

                // Hide the system cursor
                while (ShowCursor(FALSE) >= 0)
                    ;

                // Lock input if in fullscreen or if input locking is enabled
                if (windowMode_ == Fullscreen || windowMode_ == Windowed && isWindowedModeInputLockEnabled_)
                {
                    clipCursorToWindow();
                    isHoldingInputLock_ = true;
                }
            }

            break;
        }

        case WM_SIZE:
        {
            if (isHoldingInputLock_)
                clipCursorToWindow();

            break;
        }

        case WM_CLOSE:
        {
            events().dispatchEvent(ShutdownRequestEvent());

            // Eat the WM_CLOSE because the client app is responsible for actually invoking appropriate shutdown
            // procedures, and if we let this slip through then the default handler would unhelpfully kill the main
            // window
            return 0;
        }

        case WM_CHAR:
        {
            if (wParam < 0x20)
                break;

            // Printable character
            auto u = UnicodeString();
            u.append(UnicodeCharacter(wParam));
            events().dispatchEvent(CharacterInputEvent(u, dikToKeyConstant_[(lParam >> 16) & 0xFF]));

            break;
        }

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            // Stop a single press on Alt entering the system menu loop, because it pauses the whole window
            if (wParam == VK_MENU)
                return 0;

            break;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            auto button = MouseButton();

            if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
                button = LeftMouseButton;
            else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
                button = MiddleMouseButton;
            else
                button = RightMouseButton;

            auto pressed = (uMsg == WM_LBUTTONDOWN || uMsg == WM_MBUTTONDOWN || uMsg == WM_RBUTTONDOWN);

            PlatformInterface::setMousePosition(lParamToVec2(lParam));

            if (pressed)
                onInputDownEvent(button);
            else
                onInputUpEvent(button);

            break;
        }

        case WM_MOUSEMOVE:
        {
            PlatformInterface::setMousePosition(lParamToVec2(lParam));
            break;
        }

        case WM_MOUSEWHEEL:
        {
            auto direction = MouseWheelEvent::Direction();

            if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
                direction = MouseWheelEvent::TowardsUser;
            else
                direction = MouseWheelEvent::AwayFromUser;

            events().dispatchEvent(MouseWheelEvent(direction, lParamToVec2(lParam)));
        }

        case WM_ERASEBKGND:
        {
            // Sent when the window background must be erased, swallow this to avoid conflicting with the render loop

            return TRUE;
        }

        case WM_SYSCOMMAND:
        {
            // Don't allow the screen saver to run
            if (wParam == SC_SCREENSAVE)
                return 0;

            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

TimeValue PlatformWindows::getTime() const
{
    auto counter = LARGE_INTEGER();
    QueryPerformanceCounter(&counter);

    return TimeValue(counter.QuadPart);
}

bool PlatformWindows::openWithDefaultApplication(const UnicodeString& resource) const
{
    return ShellExecuteW(nullptr, L"open", resource.toUTF16().as<wchar_t>(), nullptr, nullptr, SW_SHOWNORMAL) >
        HINSTANCE(32);
}

bool PlatformWindows::showMessageBox(const UnicodeString& text, const UnicodeString& title, MessageBoxButtons buttons,
                                     MessageBoxIcon icon)
{
    auto type = 0U;

    switch (buttons)
    {
        case PlatformInterface::OKButton:
            type |= MB_OK;
            break;
        case PlatformInterface::OKCancelButtons:
            type |= MB_OKCANCEL;
            break;
        case PlatformInterface::YesNoButtons:
            type |= MB_YESNO;
            break;
    }

    switch (icon)
    {
        case PlatformInterface::InformationIcon:
            type |= MB_ICONINFORMATION;
            break;
        case PlatformInterface::ErrorIcon:
            type |= MB_ICONERROR;
            break;
    }

    auto result = MessageBox(hWnd_, text.toUTF16().as<wchar_t>(), title.toUTF16().as<wchar_t>(), type);

    return result == IDOK || result == IDYES;
}

}

#endif
