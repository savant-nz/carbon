/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef iOS

#undef new

#include <CoreFoundation/CFDate.h>
#include <OpenGLES/EAGL.h>
#include <OpenGLES/EAGLDrawable.h>
#include <QuartzCore/QuartzCore.h>
#include <UIKit/UIKit.h>

#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

/**
 * iOS platform implementation.
 */
class PlatformiOS : public PlatformInterface
{
public:

    PlatformiOS();
    ~PlatformiOS() override;

    bool setup() override;
    uintptr_t getPlatformSpecificValue(PlatformSpecificValue value) const override;
    bool createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa) override;
    void destroyWindow() override;
    VoidFunction getOpenGLFunctionAddress(const String& function) const override;
    void swap() override;
    TimeValue getTime() const override;
    bool showOnscreenKeyboard() override;
    void hideOnscreenKeyboard() override;
    bool isTouchEventEnabled(unsigned int eventTypeID) const override;
    void setTouchEventEnabled(unsigned int eventTypeID, bool enabled) override;
    Vector<Vec2> getTouches() const override;
    bool isPhone() const override;
    bool isTablet() const override;
    String getOperatingSystemName() const override;
    bool openWithDefaultApplication(const UnicodeString& resource) const override;
    bool showMessageBox(const UnicodeString& text, const UnicodeString& title, MessageBoxButtons buttons = OKButton,
                        MessageBoxIcon icon = InformationIcon) override;

    void onTouchesBegan(NSSet<UITouch*>* touches, UIEvent* event);
    void onTouchesEnded(NSSet<UITouch*>* touches, UIEvent* event);
    void onTouchesMoved(NSSet<UITouch*>* touches, UIEvent* event);
    void onGestureRecognized(UIGestureRecognizer* gesture);

private:

    UIWindow* window_ = nil;
    UIView* view_ = nil;
    EAGLContext* context_ = nil;

    GLuint framebuffer_ = 0;
    GLuint colorRenderbuffer_ = 0;
    GLuint depthStencilRenderbuffer_ = 0;

    UITextField* keyboardInputTextField_ = nil;
    NSObject<UITextFieldDelegate>* keyboardInputTextFieldDelegate_ = nil;

    Vec2 convertScreenPosition(CGPoint p) const;
    Vec2 convertScreenVector(CGPoint v) const;

    Vector<UITapGestureRecognizer*> tapGestures_;
    UIPinchGestureRecognizer* pinchGesture_ = nil;
    UIRotationGestureRecognizer* rotationGesture_ = nil;
    UIPanGestureRecognizer* panGesture_ = nil;
    Vector<UISwipeGestureRecognizer*> swipeGestures_;

    Vector<UITouch*> touches_;

    static bool isOrientationsArrayValid(NSArray* orientations);
};

}

#endif
