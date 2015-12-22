/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_PLATFORM_SDL

#include "CarbonEngine/Platform/PlatformInterface.h"

#include <SDL2/SDL.h>

namespace Carbon
{

/**
 * SDL 2 platform implementation. Intended for use on Linux though it does work on other platforms supported by SDL.
 */
class PlatformSDL : public PlatformInterface
{
public:

    PlatformSDL();
    ~PlatformSDL() override;

    bool setup() override;
    bool createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa) override;
    void destroyWindow() override;
    bool setWindowTitle(const UnicodeString& title) override;
    VoidFunction getOpenGLFunctionAddress(const String& function) const override;
    void setMousePosition(const Vec2& position) override;
    void swap() override;
    bool areCustomResolutionsSupported() const override { return true; }
    bool releaseInputLock() override;
    bool processEvent(const Event& e) override;
    TimeValue getTime() const override;
    bool openWithDefaultApplication(const UnicodeString& resource) const override;
    bool setGamma(const Color& gammas) override;
    String getOperatingSystemName() const override;

private:

    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;

    void centerWindowOnScreen() const;

    // Translation from SDLK code to KeyConstant enumeration value
    std::unordered_map<SDL_Keycode, KeyConstant> sdlkkcTable_;

    std::array<std::array<uint16_t, 256>, 3> originalGammaRamps_ = {};
};

}

#endif
