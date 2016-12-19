/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_PLATFORM_MACOS

#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

/**
 * macOS platform implementation.
 */
class PlatformMacOS : public PlatformInterface
{
public:

    PlatformMacOS();
    ~PlatformMacOS() override;

    bool setup() override;
    bool createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa) override;
    bool resizeWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa) override;
    void destroyWindow() override;
    bool setWindowTitle(const UnicodeString& title) override;
    VoidFunction getOpenGLFunctionAddress(const String& function) const override;
    void swap() override;
    bool areCustomResolutionsSupported() const override { return true; }
    bool setVerticalSyncEnabled(bool enabled) override;
    float getFinalDisplayAspectRatio() const override;
    bool releaseInputLock() override;
    bool processEvent(const Event& e) override;
    TimeValue getTime() const override;
    bool openWithDefaultApplication(const UnicodeString& resource) const override;
    bool showMessageBox(const UnicodeString& text, const UnicodeString& title, MessageBoxButtons buttons,
                        MessageBoxIcon icon) override;
    bool setGamma(const Color& gammas) override;
    String getOperatingSystemName() const override;
    unsigned int getCPUCount() const override;
    uint64_t getCPUFrequency() const override;
    uint64_t getSystemMemorySize() const override;

    void windowDidEnterFullScreen();
    void windowDidExitFullScreen();

private:

    class Members;
    Members* m = nullptr;

    void hideCursor();
    uint64_t getSysctl(const char* name) const;
};

}

#endif
