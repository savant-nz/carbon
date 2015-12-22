/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_PLATFORM_WINDOWS

#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

// DirectInput header and libraries
#define DIRECTINPUT_VERSION 0x0800
#include <InitGuid.h>
#include <dinput.h>
#ifdef _MSC_VER
    #pragma comment(lib, "dinput8.lib")
#endif

#ifdef CARBON_INCLUDE_OCULUSRIFT
    #include "CarbonEngine/Platform/Windows/OculusRiftIncludeWrapper.h"
    #ifdef _MSC_VER
        #pragma comment(lib, "OculusRift" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
    #endif
#endif

namespace Carbon
{

/**
 * Windows/DirectInput platform implementation. See the documentation for the PlatformInterface class for more information.
 */
class PlatformWindows : public PlatformInterface
{
public:

    PlatformWindows();
    ~PlatformWindows() override;

    bool setup() override;
    uintptr_t getPlatformSpecificValue(PlatformSpecificValue value) const override;
    bool setPlatformSpecificValue(PlatformSpecificValue value, uintptr_t newValue) override;
    bool createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa) override;
    void destroyWindow() override;
    bool setWindowTitle(const UnicodeString& title) override;
    VoidFunction getOpenGLFunctionAddress(const String& function) const override;
    void setMousePosition(const Vec2& position) override;
    void swap() override;
    bool areCustomResolutionsSupported() const override { return true; }
    bool setVerticalSyncEnabled(bool enabled) override;
    bool releaseInputLock() override;
    bool processEvent(const Event& e) override;
    TimeValue getTime() const override;
    bool openWithDefaultApplication(const UnicodeString& resource) const override;
    bool showMessageBox(const UnicodeString& text, const UnicodeString& title, MessageBoxButtons buttons,
                        MessageBoxIcon icon) override;
    void refreshGameControllerList() override;
    void clearGameControllers() override;
    Vector<unsigned int> getGameControllers() const override;
    UnicodeString getGameControllerDisplayName(unsigned int controllerID) override;
    const GameControllerState& getGameControllerState(unsigned int controllerID) override;
    unsigned int createForceFeedbackEffect(unsigned int controllerID, const ForceFeedbackEffect& effect) override;
    bool deleteForceFeedbackEffect(unsigned int controllerID, unsigned int effectID) override;
    bool playForceFeedbackEffect(unsigned int controllerID, unsigned int effectID, unsigned int iterations) override;
    bool stopForceFeedbackEffect(unsigned int controllerID, unsigned int effectID) override;
    bool setGamma(const Color& gammas) override;
    String getOperatingSystemName() const override;
    unsigned int getCPUCount() const override;
    uint64_t getCPUFrequency() const override;
    uint64_t getSystemMemorySize() const override;

#ifdef CARBON_INCLUDE_OCULUSRIFT
    bool isOculusRiftSupported() const override;
    bool isOculusRiftPresent() const override;
    const SimpleTransform& getOculusRiftTransformLeftEye() const override { return oculusRiftEyeTransforms_[ovrEye_Left]; }
    const SimpleTransform& getOculusRiftTransformRightEye() const override { return oculusRiftEyeTransforms_[ovrEye_Right]; }
    Matrix4 getOculusRiftProjectionMatrixLeftEye(float nearPlaneDistance, float farPlaneDistance) const override;
    Matrix4 getOculusRiftProjectionMatrixRightEye(float nearPlaneDistance, float farPlaneDistance) const override;
    const Rect& getOculusRiftTextureDimensions() const override { return oculusRiftEyeTextureDimensions_; }
#endif

private:

    // Core handles
    HWND hWnd_ = nullptr;
    HDC hDC_ = nullptr;
    HICON hIcon_ = nullptr;

    void createRawWindow(const RECT& rect, WindowMode windowMode);
    void closeRawWindow();

#ifdef CARBON_INCLUDE_OPENGL11
    HGLRC hGLRC_ = nullptr;
    void createGLWindow(const RECT& rect, WindowMode windowMode, const PIXELFORMATDESCRIPTOR& pixelFormatDescriptor, int pf);
    void createGLWindow(const RECT& rect, WindowMode windowMode, FSAAMode fsaa);
    void closeGLWindow();
#endif

    bool isOpenGLWindow() const
    {
#ifdef CARBON_INCLUDE_OPENGL11
        return hGLRC_ != nullptr;
#else
        return false;
#endif
    }

    // Window procedure callback, calls into windowProc()
    static LRESULT staticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Helper functions
    bool registerWindowClass() const;
    void centerWindowOnScreen() const;
    bool changeScreenResolution(const Resolution& resolution) const;
    void clipCursorToWindow() const;
    Vec2 lParamToVec2(LPARAM lParam) { return {float(LOWORD(lParam)), float(getWindowHeight() - HIWORD(lParam) - 1)}; }

    // DirectInput
    LPDIRECTINPUT8 DI_ = nullptr;
    LPDIRECTINPUTDEVICE8 keyboardDevice_ = nullptr;
    LPDIRECTINPUTDEVICE8 mouseDevice_ = nullptr;
    std::array<KeyConstant, 256> dikToKeyConstant_ = {};
    class GameController
    {
    public:

        LPDIRECTINPUTDEVICE8 device = nullptr;    // The actual DirectInput device
        UnicodeString displayName;                // The human-friendly name to display
        unsigned int id = 0;                      // ID of this controller
        GameControllerState state;                // Most recent input state of the controller

        unsigned int forceFeedbackAxisCount = 0;    // Number of force feedback axes this device has

        // Force-feedback effects on this device
        class FFEffect
        {
        public:

            ForceFeedbackEffect* effect = nullptr;     // Effect description this effect was created with
            LPDIRECTINPUTEFFECT diEffect = nullptr;    // Internal DirectInput effect instance

            FFEffect() {}
            FFEffect(ForceFeedbackEffect* effect, LPDIRECTINPUTEFFECT diEffect) : effect(effect), diEffect(diEffect) {}
        };
        std::unordered_map<unsigned int, FFEffect> ffEffects;

        GameController() {}
        GameController(LPDIRECTINPUTDEVICE8 device) : device(device) {}
    };
    Vector<GameController> gameControllers_;
    GameController* findGameController(unsigned int controllerID);
    bool setupDirectInput();
    void closeDirectInput();
    static BOOL CALLBACK enumGameControllersCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
    static BOOL CALLBACK enumGameControllerAxesCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);

    void onInputDownEvent(KeyConstant key) override;
    void onInputDownEvent(MouseButton button) override { onInputDownEvent(KeyConstant(KeyLast + button)); }

    std::array<std::array<uint16_t, 256>, 3> originalGammaRamps_ = {};

#ifdef CARBON_INCLUDE_OCULUSRIFT
    ovrSession oculusRiftSession_ = nullptr;
    ovrHmdDesc oculusRiftHMDDesc_ = {};
    Rect oculusRiftEyeTextureDimensions_;
    long long oculusRiftFrameIndex_ = 0;
    std::array<SimpleTransform, 2> oculusRiftEyeTransforms_;
    bool oculusRiftInitialize();
    void oculusRiftShutdown();
    void oculusRiftUpdate();
#endif
};

#ifdef CARBON_INCLUDE_OPENGL11

// WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

// WGL_ARB_pixel_format
typedef BOOL(APIENTRY* PFnwglGetPixelFormatAttribivARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes,
                                                        const int* piAttributes, int* piValues);
typedef BOOL(APIENTRY* PFnwglGetPixelFormatAttribfvARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes,
                                                        const int* piAttributes, FLOAT* pfValues);
typedef BOOL(APIENTRY* PFnwglChoosePixelFormatARB)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList,
                                                   UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C

// WGL_EXT_swap_interval
typedef BOOL(APIENTRY* PFnwglSwapIntervalEXT)(int interval);
typedef int(APIENTRY* PFnwglGetSwapIntervalEXT)();

#endif
}

#endif
