/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef iOS

#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Graphics/iOS/iOSOpenGLES2.h"
#include "CarbonEngine/Platform/iOS/PlatformiOS.h"
#include "CarbonEngine/Platform/PlatformEvents.h"

// This UIView subclass wraps the CAEAGLLayer from Core Animation. The view content is an EAGL context that an OpenGL ES scene
// can be rendered into. Input events on the view are passed through to PlatformiOS.
@interface EAGLView : UIView
- (Carbon::PlatformiOS&)platform;
@end
@implementation EAGLView
+ (Class)layerClass
{
    // Tell the view to use a CAEAGLLayer as its backing
    return [CAEAGLLayer class];
}

- (BOOL)isMultipleTouchEnabled
{
    return YES;
}

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

- (Carbon::PlatformiOS&)platform
{
    return static_cast<Carbon::PlatformiOS&>(Carbon::platform());
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self platform].onTouchesBegan(touches, event);
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self platform].onTouchesMoved(touches, event);
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self platform].onTouchesEnded(touches, event);
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self platform].onTouchesEnded(touches, event);
}

- (IBAction)onGestureRecognized:(UIGestureRecognizer*)gestureRecognizer
{
    [self platform].onGestureRecognized(gestureRecognizer);
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent*)event
{
    if ([event subtype] == UIEventSubtypeMotionShake)
    {
        LOG_INFO << "iOS device shake detected";
        Carbon::events().dispatchEvent(Carbon::DeviceShakeEvent());
    }
}
@end

// UIViewController which allows autorotation and hides the status bar.
@interface CustomViewController : UIViewController
@end
@implementation CustomViewController
- (BOOL)shouldAutorotate
{
    return YES;
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}
@end

// Delegate used to proxy onscreen keyboard input to the corresponding engine events.
@interface KeyboardInputTextFieldDelegate : NSObject <UITextFieldDelegate>
@end
@implementation KeyboardInputTextFieldDelegate
- (BOOL)textField:(UITextField*)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString*)s
{
    if (range.length == 1)
    {
        Carbon::events().dispatchEvent(Carbon::KeyDownEvent(Carbon::KeyBackspace, false));
        Carbon::events().dispatchEvent(Carbon::KeyUpEvent(Carbon::KeyBackspace));
    }
    else
        Carbon::events().dispatchEvent(Carbon::CharacterInputEvent(s));

    return NO;
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
    Carbon::events().dispatchEvent(Carbon::KeyDownEvent(Carbon::KeyEnter, false));
    Carbon::events().dispatchEvent(Carbon::KeyUpEvent(Carbon::KeyEnter));

    [textField resignFirstResponder];

    return YES;
}
@end

namespace Carbon
{

PlatformiOS::PlatformiOS()
{
    setTicksPerSecond(10000);

    isWindowedModeSupported_ = false;
}

bool PlatformiOS::isOrientationsArrayValid(NSArray* orientations)
{
    if (!orientations)
        return false;

    auto portrait = false;
    auto landscape = false;

    for (NSString* nsOrientation in orientations)
    {
        auto o = String(nsOrientation);

        portrait |= (o == "UIInterfaceOrientationPortrait" || o == "UIInterfaceOrientationPortraitUpsideDown");
        landscape |= (o == "UIInterfaceOrientationLandscapeLeft" || o == "UIInterfaceOrientationLandscapeRight");
    }

    return portrait ^ landscape;
}

bool PlatformiOS::setup()
{
    PlatformInterface::setup();

    // Check that the supported interface orientations do not include both portrait and landscape modes
    NSArray* supportedOrientations = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"UISupportedInterfaceOrientations"];
    if (!isOrientationsArrayValid(supportedOrientations))
    {
        LOG_ERROR << "Info.plist must specify either landscape or portrait orientations";
        return false;
    }

    // Create and setup a main window
    window_ = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    [window_ setRootViewController:[CustomViewController alloc]];
    [window_ setUserInteractionEnabled:YES];
    [window_ setMultipleTouchEnabled:YES];
    [window_ makeKeyAndVisible];

    // Create an EAGL view and add it to the main window
    view_ = [[EAGLView alloc] initWithFrame:window_.frame];
    window_.rootViewController.view = view_;

    // Setup the EAGL layer for the new view
    auto eaglLayer = static_cast<CAEAGLLayer*>(view_.layer);
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties =
        @{ kEAGLDrawablePropertyRetainedBacking : @NO,
           kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8 };

    // Create an OpenGL ES 2 rendering context
    context_ = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:context_];

    // Set the view as the topmost surface that will receive touch events
    [view_ becomeFirstResponder];

    // Create a hidden text field that can be used for iOS keyboard entry
    keyboardInputTextField_ = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
    [view_ addSubview:keyboardInputTextField_];
    keyboardInputTextField_.autocapitalizationType = UITextAutocapitalizationTypeNone;
    keyboardInputTextField_.autocorrectionType = UITextAutocorrectionTypeNo;
    keyboardInputTextField_.returnKeyType = UIReturnKeyDone;
    keyboardInputTextField_.spellCheckingType = UITextSpellCheckingTypeNo;
    [keyboardInputTextField_ setHidden:TRUE];
    [keyboardInputTextField_ setText:@" "];
    keyboardInputTextFieldDelegate_ = [[KeyboardInputTextFieldDelegate alloc] init];
    [keyboardInputTextField_ setDelegate:keyboardInputTextFieldDelegate_];

    // Setup resolutions array with the main resolution
    resolutions_.emplace(int(view_.frame.size.width), int(view_.frame.size.height));
    nativeResolution_ = resolutions_.back();

    // Retina resolution support
    if ([UIScreen mainScreen].scale == 2.0)
    {
        resolutions_.emplace(int(view_.frame.size.width * 2), int(view_.frame.size.height * 2), false, true);

        if (areRetinaResolutionsEnabled())
            nativeResolution_ = resolutions_.back();
    }

    sortResolutions();

    events().addHandler<UpdateEvent>(this, true);

    return true;
}

PlatformiOS::~PlatformiOS()
{
    keyboardInputTextField_ = nil;
    keyboardInputTextFieldDelegate_ = nil;

    pinchGesture_ = nil;
    rotationGesture_ = nil;
    panGesture_ = nil;

    context_ = nil;
    view_ = nil;
    window_ = nil;
}

uintptr_t PlatformiOS::getPlatformSpecificValue(PlatformSpecificValue value) const
{
    if (value == iOSOpenGLESFramebuffer)
        return framebuffer_;

    return 0;
}

bool PlatformiOS::createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa)
{
    if (!resolutions_.has(resolution))
    {
        LOG_ERROR << "Invalid resolution: " << resolution;
        return false;
    }

    auto newResolution = resolution;

    // If a retina resolution was requested but they are disabled then fall back to the equivalent non-retina resolution
    if (newResolution.isRetinaResolution() && !areRetinaResolutionsEnabled())
        newResolution = findResolution(newResolution.getWidth() / 2, newResolution.getHeight() / 2);

    // Use retina display if requested
    view_.contentScaleFactor = newResolution.isRetinaResolution() ? 2.0f : 1.0f;
    view_.layer.contentsScale = view_.contentScaleFactor;

    // Create and bind a framebuffer
    glGenFramebuffers(1, &framebuffer_);
    CARBON_CHECK_OPENGL_ERROR(glGenFramebuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    CARBON_CHECK_OPENGL_ERROR(glBindFramebuffer);

    // Create and bind a render buffer, set it to be the primary color output of the framebuffer
    glGenRenderbuffers(1, &colorRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glGenRenderbuffers);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glBindRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glFramebufferRenderbuffer);

    // Tell the layer to display the contents of the main render buffer
    [context_ renderbufferStorage:GL_RENDERBUFFER fromDrawable:static_cast<CAEAGLLayer*>(view_.layer)];

    // Get the dimensions of the render buffer
    auto framebufferWidth = GLint();
    auto framebufferHeight = GLint();
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
    CARBON_CHECK_OPENGL_ERROR(glGetRenderbufferParameteriv);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
    CARBON_CHECK_OPENGL_ERROR(glGetRenderbufferParameteriv);

    // Create and bind a depth/stencil buffer
    glGenRenderbuffers(1, &depthStencilRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glGenRenderbuffers);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glBindRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, framebufferWidth, framebufferHeight);
    CARBON_CHECK_OPENGL_ERROR(glRenderbufferStorage);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glFramebufferRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glFramebufferRenderbuffer);

    // Bind the main render buffer
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer_);
    CARBON_CHECK_OPENGL_ERROR(glBindRenderbuffer);

    currentResolution_ = newResolution;
    updatePersistentSettings();
    sendResizeEvent();

    LOG_INFO << "Window created, resolution: " << newResolution;

    return true;
}

void PlatformiOS::destroyWindow()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CARBON_CHECK_OPENGL_ERROR(glBindFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    CARBON_CHECK_OPENGL_ERROR(glBindRenderbuffer);

    if (framebuffer_)
    {
        glDeleteFramebuffers(1, &framebuffer_);
        CARBON_CHECK_OPENGL_ERROR(glDeleteFramebuffers);
        framebuffer_ = 0;
    }

    if (colorRenderbuffer_)
    {
        glDeleteRenderbuffers(1, &colorRenderbuffer_);
        CARBON_CHECK_OPENGL_ERROR(glDeleteRenderbuffers);
        colorRenderbuffer_ = 0;
    }

    if (depthStencilRenderbuffer_)
    {
        glDeleteRenderbuffers(1, &depthStencilRenderbuffer_);
        CARBON_CHECK_OPENGL_ERROR(glDeleteRenderbuffers);
        depthStencilRenderbuffer_ = 0;
    }

    currentResolution_ = Resolution::Zero;
    fsaaMode_ = FSAANone;

    LOG_INFO << "Window destroyed";
}

VoidFunction PlatformiOS::getOpenGLFunctionAddress(const String& function) const
{
    // There may be a more correct way to map OpenGL ES extension functions on iOS, but this is sufficient at present

    if (function == "glBindVertexArrayOES")
        return VoidFunction(glBindVertexArrayOES);
    if (function == "glDeleteVertexArraysOES")
        return VoidFunction(glDeleteVertexArraysOES);
    if (function == "glGenVertexArraysOES")
        return VoidFunction(glGenVertexArraysOES);
    if (function == "glIsVertexArrayOES")
        return VoidFunction(glIsVertexArrayOES);

    return {};
}

void PlatformiOS::swap()
{
    [context_ presentRenderbuffer:GL_RENDERBUFFER];
}

TimeValue PlatformiOS::getTime() const
{
    return TimeValue(int64_t(CFAbsoluteTimeGetCurrent() * 10000));
}

bool PlatformiOS::showOnscreenKeyboard()
{
    [keyboardInputTextField_ becomeFirstResponder];

    return true;
}

void PlatformiOS::hideOnscreenKeyboard()
{
    [keyboardInputTextField_ resignFirstResponder];
}

bool PlatformiOS::openWithDefaultApplication(const UnicodeString& resource) const
{
    auto escaped =
        [resource.toNSString() stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];

    // Open as a URL
    return [[UIApplication sharedApplication] openURL:[NSURL URLWithString:escaped]];
}

bool PlatformiOS::showMessageBox(const UnicodeString& text, const UnicodeString& title, MessageBoxButtons buttons,
                                 MessageBoxIcon icon)
{
    auto alert = [UIAlertController alertControllerWithTitle:title.toNSString()
                                                     message:text.toNSString()
                                              preferredStyle:UIAlertControllerStyleAlert];

    if (buttons == OKButton || buttons == OKCancelButtons)
    {
        auto action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:action];

        if (buttons == OKCancelButtons)
        {
            action = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil];
            [alert addAction:action];
        }
    }
    else if (buttons == YesNoButtons)
    {
        auto action = [UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:action];

        action = [UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleCancel handler:nil];
        [alert addAction:action];
    }

    [window_.rootViewController presentViewController:alert animated:YES completion:nil];

    return true;
}

bool PlatformiOS::isPhone() const
{
    return [[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone;
}

bool PlatformiOS::isTablet() const
{
    return [[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad;
}

String PlatformiOS::getOperatingSystemName() const
{
    return String() << "iOS " << [[UIDevice currentDevice] systemVersion];
}

void PlatformiOS::onTouchesBegan(NSSet<UITouch*>* touches, UIEvent* event)
{
    for (UITouch* touch in touches)
    {
        touches_.append(touch);

        events().dispatchEvent(TouchBeginEvent(uintptr_t(touch), convertScreenPosition([touch locationInView:view_])));
    }
}

void PlatformiOS::onTouchesEnded(NSSet<UITouch*>* touches, UIEvent* event)
{
    for (UITouch* touch in touches)
    {
        events().dispatchEvent(TouchEndEvent(uintptr_t(touch), convertScreenPosition([touch locationInView:view_])));

        if (!touches_.eraseValue(touch))
        {
            LOG_WARNING << "Unknown touch event was ended: " << uint64_t(touch)
                        << ", position: " << convertScreenPosition([touch locationInView:view_]);
        }
    }
}

void PlatformiOS::onTouchesMoved(NSSet<UITouch*>* touches, UIEvent* event)
{
    for (UITouch* touch in touches)
    {
        events().dispatchEvent(TouchMoveEvent(uintptr_t(touch), convertScreenPosition([touch locationInView:view_]),
                                              convertScreenPosition([touch previousLocationInView:view_])));
    }
}

bool PlatformiOS::isTouchEventEnabled(unsigned int eventTypeID) const
{
    if (eventTypeID == events().getEventTypeID<TouchTapEvent>())
        return tapGestures_.size() != 0;

    if (eventTypeID == events().getEventTypeID<TouchPinchEvent>())
        return pinchGesture_ != nil;

    if (eventTypeID == events().getEventTypeID<TouchRotationEvent>())
        return rotationGesture_ != nil;

    if (eventTypeID == events().getEventTypeID<TouchSwipeEvent>())
        return swipeGestures_.size() != 0;

    if (eventTypeID == events().getEventTypeID<TouchPanEvent>())
        return panGesture_ != nil;

    return false;
}

void PlatformiOS::setTouchEventEnabled(unsigned int eventTypeID, bool enabled)
{
    // Internally the touch events are enabled/disabled by creating/destroying the corresponding iOS gesture recognizers

    if (eventTypeID == events().getEventTypeID<TouchTapEvent>())
    {
        // Single tap with 1-4 fingers
        if (!tapGestures_.size() && enabled)
        {
            for (auto j = 0U; j < 4; j++)
            {
                auto tap = [[UITapGestureRecognizer alloc] initWithTarget:view_ action:@selector(onGestureRecognized:)];
                tap.numberOfTapsRequired = 1;
                tap.numberOfTouchesRequired = 4 - j;

                for (auto tapGesture : tapGestures_)
                    [tap requireGestureRecognizerToFail:tapGesture];

                [view_ addGestureRecognizer:tap];
                tapGestures_.append(tap);
            }
        }
        else if (tapGestures_.size() && !enabled)
        {
            for (auto tapGesture : tapGestures_)
                [view_ removeGestureRecognizer:tapGesture];

            tapGestures_.clear();
        }
    }
    else if (eventTypeID == events().getEventTypeID<TouchPinchEvent>())
    {
        // Pinch
        if (pinchGesture_ == nil && enabled)
        {
            pinchGesture_ = [[UIPinchGestureRecognizer alloc] initWithTarget:view_ action:@selector(onGestureRecognized:)];

            [view_ addGestureRecognizer:pinchGesture_];
        }
        else if (pinchGesture_ && !enabled)
        {
            [view_ removeGestureRecognizer:pinchGesture_];
            pinchGesture_ = nil;
        }
    }
    else if (eventTypeID == events().getEventTypeID<TouchRotationEvent>())
    {
        // Rotation
        if (rotationGesture_ == nil && enabled)
        {
            rotationGesture_ =
                [[UIRotationGestureRecognizer alloc] initWithTarget:view_ action:@selector(onGestureRecognized:)];

            [view_ addGestureRecognizer:rotationGesture_];
        }
        else if (rotationGesture_ == nil && !enabled)
        {
            [view_ removeGestureRecognizer:rotationGesture_];
            rotationGesture_ = nil;
        }
    }
    else if (eventTypeID == events().getEventTypeID<TouchSwipeEvent>())
    {
        // Swipe, all four directions with 1-4 fingers
        if (!swipeGestures_.size() && enabled)
        {
            for (auto i = 0U; i < 4; i++)
            {
                for (auto j = 0U; j < 4; j++)
                {
                    auto swipe = [[UISwipeGestureRecognizer alloc] initWithTarget:view_ action:@selector(onGestureRecognized:)];
                    swipe.direction = UISwipeGestureRecognizerDirection(1 << i);
                    swipe.numberOfTouchesRequired = 4 - j;

                    [view_ addGestureRecognizer:swipe];
                    swipeGestures_.append(swipe);
                }
            }
        }
        else if (swipeGestures_.size() && !enabled)
        {
            for (auto swipeGesture : swipeGestures_)
                [view_ removeGestureRecognizer:swipeGesture];

            swipeGestures_.clear();
        }
    }
    else if (eventTypeID == events().getEventTypeID<TouchPanEvent>())
    {
        // Pan with a single finger
        if (panGesture_ == nil)
        {
            panGesture_ = [[UIPanGestureRecognizer alloc] initWithTarget:view_ action:@selector(onGestureRecognized:)];
            panGesture_.minimumNumberOfTouches = 1;
            panGesture_.maximumNumberOfTouches = 1;

            [view_ addGestureRecognizer:panGesture_];
        }
        else if (panGesture_ && !enabled)
        {
            [view_ removeGestureRecognizer:panGesture_];
            panGesture_ = nil;
        }
    }
}

Vector<Vec2> PlatformiOS::getTouches() const
{
    return touches_.map<Vec2>([&](const UITouch* touch) { return convertScreenPosition([touch locationInView:view_]); });
}

void PlatformiOS::onGestureRecognized(UIGestureRecognizer* gestureRecognizer)
{
    if ([gestureRecognizer isKindOfClass:[UITapGestureRecognizer class]])
    {
        auto tap = static_cast<UITapGestureRecognizer*>(gestureRecognizer);

        events().dispatchEvent(TouchTapEvent(convertScreenPosition([tap locationInView:view_]), uint(tap.numberOfTapsRequired),
                                             uint(tap.numberOfTouchesRequired)));
    }
    else if ([gestureRecognizer isKindOfClass:[UIPinchGestureRecognizer class]])
    {
        auto pinch = static_cast<UIPinchGestureRecognizer*>(gestureRecognizer);

        events().dispatchEvent(TouchPinchEvent(float(pinch.scale), float(pinch.velocity)));
    }
    else if ([gestureRecognizer isKindOfClass:[UIRotationGestureRecognizer class]])
    {
        auto rotation = static_cast<UIRotationGestureRecognizer*>(gestureRecognizer);

        events().dispatchEvent(TouchRotationEvent(float(rotation.rotation), float(rotation.velocity)));
    }
    else if ([gestureRecognizer isKindOfClass:[UISwipeGestureRecognizer class]])
    {
        auto swipe = static_cast<UISwipeGestureRecognizer*>(gestureRecognizer);

        auto direction = TouchSwipeEvent::SwipeDirection();
        switch ([swipe direction])
        {
            case UISwipeGestureRecognizerDirectionLeft:
                direction = TouchSwipeEvent::SwipeLeft;
                break;
            case UISwipeGestureRecognizerDirectionRight:
                direction = TouchSwipeEvent::SwipeRight;
                break;
            case UISwipeGestureRecognizerDirectionUp:
                direction = TouchSwipeEvent::SwipeUp;
                break;
            case UISwipeGestureRecognizerDirectionDown:
                direction = TouchSwipeEvent::SwipeDown;
                break;
            default:
                return;
        }

        events().dispatchEvent(TouchSwipeEvent(convertScreenPosition([swipe locationInView:view_]), direction,
                                               uint(swipe.numberOfTouchesRequired)));
    }
    else if ([gestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]])
    {
        auto pan = static_cast<UIPanGestureRecognizer*>(gestureRecognizer);
        auto translation = [pan translationInView:view_];

        events().dispatchEvent(TouchPanEvent(convertScreenVector(translation), uint(pan.minimumNumberOfTouches)));
    }
}

Vec2 PlatformiOS::convertScreenPosition(CGPoint p) const
{
    return {float(p.x), float(view_.frame.size.height - p.y - 1.0) * float(view_.contentScaleFactor)};
}

Vec2 PlatformiOS::convertScreenVector(CGPoint v) const
{
    return {float(v.x), -float(v.y)};
}

}

#endif
