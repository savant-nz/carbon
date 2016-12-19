/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_PLATFORM_MACOS

#include <mach/mach_time.h>
#include <OpenGL/gl.h>

#undef new
#include <Cocoa/Cocoa.h>
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"

#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SharedLibrary.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/macOS/PlatformMacOS.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

@interface WindowDelegate : NSObject <NSWindowDelegate>
- (BOOL)windowShouldClose:(id)sender;
- (void)windowDidEnterFullScreen:(NSNotification*)notification;
- (void)windowDidExitFullScreen:(NSNotification*)notification;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
    Carbon::events().dispatchEvent(Carbon::ShutdownRequestEvent());
    return NO;
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
    static_cast<Carbon::PlatformMacOS&>(Carbon::platform()).windowDidEnterFullScreen();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
    static_cast<Carbon::PlatformMacOS&>(Carbon::platform()).windowDidExitFullScreen();
}
@end

namespace Carbon
{

class PlatformMacOS::Members
{
public:

    SharedLibrary openGLFramework;

    mach_timebase_info_data_t timebaseInfo;

    NSWindow* nsWindow = nil;
    WindowDelegate* windowDelegate = nil;
    NSOpenGLContext* nsOpenGLContext = nil;

    bool isCursorVisible = true;
    std::array<KeyConstant, 0x7F> keyMappings = {};

    NSUInteger lastKeyboardModifiers = 0;

    bool ignoreNextNonZeroMouseMovement = false;

    std::array<std::array<float, 256>, 3> originalGammaRamps = {};
};

PlatformMacOS::PlatformMacOS()
{
    m = new Members;

    mach_timebase_info(&m->timebaseInfo);
    setTicksPerSecond(1000000);

    events().addHandler<UpdateEvent>(this, true);
    events().addHandler<ApplicationGainFocusEvent>(this);

    m->keyMappings[0x00] = KeyA;
    m->keyMappings[0x01] = KeyS;
    m->keyMappings[0x02] = KeyD;
    m->keyMappings[0x03] = KeyF;
    m->keyMappings[0x04] = KeyH;
    m->keyMappings[0x05] = KeyG;
    m->keyMappings[0x06] = KeyZ;
    m->keyMappings[0x07] = KeyX;
    m->keyMappings[0x08] = KeyC;
    m->keyMappings[0x09] = KeyV;
    m->keyMappings[0x0B] = KeyB;
    m->keyMappings[0x0C] = KeyQ;
    m->keyMappings[0x0D] = KeyW;
    m->keyMappings[0x0E] = KeyE;
    m->keyMappings[0x0F] = KeyR;
    m->keyMappings[0x10] = KeyY;
    m->keyMappings[0x11] = KeyT;
    m->keyMappings[0x12] = Key1;
    m->keyMappings[0x13] = Key2;
    m->keyMappings[0x14] = Key3;
    m->keyMappings[0x15] = Key4;
    m->keyMappings[0x16] = Key6;
    m->keyMappings[0x17] = Key5;
    m->keyMappings[0x18] = KeyEquals;
    m->keyMappings[0x19] = Key9;
    m->keyMappings[0x1A] = Key7;
    m->keyMappings[0x1B] = KeyMinus;
    m->keyMappings[0x1C] = Key8;
    m->keyMappings[0x1D] = Key0;
    m->keyMappings[0x1E] = KeyRightBracket;
    m->keyMappings[0x1F] = KeyO;
    m->keyMappings[0x20] = KeyU;
    m->keyMappings[0x21] = KeyLeftBracket;
    m->keyMappings[0x22] = KeyI;
    m->keyMappings[0x23] = KeyP;
    m->keyMappings[0x24] = KeyEnter;
    m->keyMappings[0x25] = KeyL;
    m->keyMappings[0x26] = KeyJ;
    m->keyMappings[0x27] = KeyApostrophe;
    m->keyMappings[0x28] = KeyK;
    m->keyMappings[0x29] = KeySemicolon;
    m->keyMappings[0x2A] = KeyBackSlash;
    m->keyMappings[0x2B] = KeyComma;
    m->keyMappings[0x2C] = KeyForwardSlash;
    m->keyMappings[0x2D] = KeyN;
    m->keyMappings[0x2E] = KeyM;
    m->keyMappings[0x2F] = KeyPeriod;
    m->keyMappings[0x30] = KeyTab;
    m->keyMappings[0x31] = KeySpacebar;
    m->keyMappings[0x32] = KeyGraveAccent;
    m->keyMappings[0x33] = KeyBackspace;
    m->keyMappings[0x35] = KeyEscape;
    m->keyMappings[0x37] = KeyLeftMeta;
    m->keyMappings[0x38] = KeyLeftShift;
    m->keyMappings[0x39] = KeyCapsLock;
    m->keyMappings[0x3A] = KeyLeftAlt;
    m->keyMappings[0x3B] = KeyLeftControl;
    m->keyMappings[0x3C] = KeyRightShift;
    m->keyMappings[0x3D] = KeyRightAlt;
    m->keyMappings[0x3E] = KeyRightControl;
    m->keyMappings[0x41] = KeyNumpadPeriod;
    m->keyMappings[0x43] = KeyNumpadStar;
    m->keyMappings[0x45] = KeyNumpadPlus;
    m->keyMappings[0x4B] = KeyNumpadForwardSlash;
    m->keyMappings[0x4C] = KeyNumpadEnter;
    m->keyMappings[0x4E] = KeyNumpadMinus;
    m->keyMappings[0x51] = KeyNumpadEquals;
    m->keyMappings[0x52] = KeyNumpad0;
    m->keyMappings[0x53] = KeyNumpad1;
    m->keyMappings[0x54] = KeyNumpad2;
    m->keyMappings[0x55] = KeyNumpad3;
    m->keyMappings[0x56] = KeyNumpad4;
    m->keyMappings[0x57] = KeyNumpad5;
    m->keyMappings[0x58] = KeyNumpad6;
    m->keyMappings[0x59] = KeyNumpad7;
    m->keyMappings[0x5B] = KeyNumpad8;
    m->keyMappings[0x5C] = KeyNumpad9;
    m->keyMappings[0x60] = KeyF5;
    m->keyMappings[0x61] = KeyF6;
    m->keyMappings[0x62] = KeyF7;
    m->keyMappings[0x63] = KeyF3;
    m->keyMappings[0x64] = KeyF8;
    m->keyMappings[0x65] = KeyF9;
    m->keyMappings[0x67] = KeyF11;
    m->keyMappings[0x6D] = KeyF10;
    m->keyMappings[0x6F] = KeyF12;
    m->keyMappings[0x73] = KeyHome;
    m->keyMappings[0x74] = KeyPageUp;
    m->keyMappings[0x75] = KeyDelete;
    m->keyMappings[0x76] = KeyF4;
    m->keyMappings[0x77] = KeyEnd;
    m->keyMappings[0x78] = KeyF2;
    m->keyMappings[0x79] = KeyPageDown;
    m->keyMappings[0x7A] = KeyF1;
    m->keyMappings[0x7B] = KeyLeftArrow;
    m->keyMappings[0x7C] = KeyRightArrow;
    m->keyMappings[0x7D] = KeyDownArrow;
    m->keyMappings[0x7E] = KeyUpArrow;
}

PlatformMacOS::~PlatformMacOS()
{
    destroyWindow();

    delete m;
    m = nullptr;
}

bool PlatformMacOS::setup()
{
    PlatformInterface::setup();

    // Load OpenGL library
    if (!m->openGLFramework.load("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"))
    {
        LOG_ERROR << "Failed opening OpenGL library";
        return false;
    }

    // Enumerate supported resolutions on the main screen
    auto modeList = CGDisplayCopyAllDisplayModes(CGMainDisplayID(), nil);
    for (auto i = CFIndex(0); i < CFArrayGetCount(modeList); i++)
    {
        auto mode = CGDisplayModeRef(CFArrayGetValueAtIndex(modeList, i));

        resolutions_.emplace(uint(CGDisplayModeGetWidth(mode)), uint(CGDisplayModeGetHeight(mode)));
    }
    CFRelease(modeList);

    // Store native resolution
    auto mode = CGDisplayCopyDisplayMode(CGMainDisplayID());
    nativeResolution_ = findResolution(uint(CGDisplayModeGetWidth(mode)), uint(CGDisplayModeGetHeight(mode)));
    CGDisplayModeRelease(mode);

    sortResolutions();

    // Read original display gammas
    auto sampleCount = 0U;
    if (CGGetDisplayTransferByTable(CGMainDisplayID(), 256, m->originalGammaRamps[0].data(),
                                    m->originalGammaRamps[1].data(), m->originalGammaRamps[2].data(),
                                    &sampleCount) != CGDisplayNoErr ||
        sampleCount != 256)
    {
        // Fall back to an identity gamma curve as the default
        calculateGammaRamp(1.0f, m->originalGammaRamps[0], m->originalGammaRamps[0]);
        calculateGammaRamp(1.0f, m->originalGammaRamps[1], m->originalGammaRamps[1]);
        calculateGammaRamp(1.0f, m->originalGammaRamps[2], m->originalGammaRamps[2]);

        LOG_ERROR << "Failed reading display gammas";
    }

    return true;
}

bool PlatformMacOS::createWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa)
{
    try
    {
        if (!resolutions_.has(resolution))
            throw Exception() << "Invalid resolution: " << resolution;

        // Find a pixel format to use
        auto attributes = Vector<NSOpenGLPixelFormatAttribute>();
        attributes.append(NSOpenGLPFAMinimumPolicy);
        attributes.append(NSOpenGLPFAAccelerated);
        attributes.append(NSOpenGLPFADoubleBuffer);
        attributes.append(NSOpenGLPFABackingStore);
        attributes.append(NSOpenGLPFAColorSize);
        attributes.append(24);
        attributes.append(NSOpenGLPFAAlphaSize);
        attributes.append(8);
        attributes.append(NSOpenGLPFADepthSize);
        attributes.append(24);
        attributes.append(NSOpenGLPFAStencilSize);
        attributes.append(8);
        if (fsaa != FSAANone)
        {
            attributes.append(NSOpenGLPFAMultisample);
            attributes.append(NSOpenGLPFASampleBuffers);
            attributes.append(1);
            attributes.append(NSOpenGLPFASamples);
            attributes.append(fsaa);
        }

#ifdef CARBON_INCLUDE_OPENGL41
        // If the OpenGL 4.1 graphics backend is active then request an OpenGL 4.1 Core Profile pixel format
        if (InterfaceRegistry<GraphicsInterface>::getActiveImplementation()->getName() == "OpenGL41")
        {
            attributes.append(NSOpenGLPFAOpenGLProfile);
            attributes.append(0x4100);
        }
#endif

        attributes.append(0);

        auto pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes.getData()];
        if (!pixelFormat)
            throw Exception("Failed finding a suitable pixel format");

        // Create window
        auto frame = NSMakeRect((nativeResolution_.getWidth() - resolution.getWidth()) / 2,
                                (nativeResolution_.getHeight() - resolution.getHeight()) / 2, resolution.getWidth(),
                                resolution.getHeight());

        m->nsWindow =
            [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask
                                          backing:NSBackingStoreBuffered
                                            defer:NO];
        if (!m->nsWindow)
            throw Exception("Failed creating window");

        m->windowDelegate = [[WindowDelegate alloc] init];
        m->nsWindow.delegate = m->windowDelegate;

        // Prepare and display the window
        setWindowTitle(windowTitle_);
        [m->nsWindow setReleasedWhenClosed:NO];
        [m->nsWindow setAcceptsMouseMovedEvents:YES];
        [m->nsWindow setBackgroundColor:[NSColor blackColor]];
        [m->nsWindow setOpaque:YES];
        [m->nsWindow
            setCollectionBehavior:NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorFullScreenPrimary];

        // Create an OpenGL rendering context
        m->nsOpenGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];

        // When the rendering resolution is lower than the window's resolution, which happens when running fullscreen at
        // a resolution lower than the native resolution, the rendered surface is upscaled to fill the screen
        auto dimensions = std::array<GLint, 2>{{GLint(resolution.getWidth()), GLint(resolution.getHeight())}};
        [m->nsOpenGLContext setValues:dimensions.data() forParameter:NSOpenGLCPSurfaceBackingSize];
        CGLEnable(CGLContextObj([m->nsOpenGLContext CGLContextObj]), kCGLCESurfaceBackingSize);

        // Activate the OpenGL rendering context
        [m->nsOpenGLContext setView:[m->nsWindow contentView]];
        [m->nsOpenGLContext update];
        [m->nsOpenGLContext makeCurrentContext];

        // Clear the new window to black
        glClear(GL_COLOR_BUFFER_BIT);
        swap();

        // Set initial gammas
        setGamma(gammas_);

        // Toggle into fullscreen if needed
        if (windowMode == Fullscreen)
            [m->nsWindow toggleFullScreen:nil];

        // Show the window
        [m->nsWindow makeKeyAndOrderFront:NSApp];

        hideCursor();

        // Setup keyboard input
        for (auto i = 0U; i < KeyLast; i++)
            setIsKeyPressed(KeyConstant(i), false);

        setVerticalSyncEnabled(isVerticalSyncEnabled_);

        currentResolution_ = resolution;
        windowMode_ = windowMode;
        fsaaMode_ = fsaa;
        updatePersistentSettings();

        sendResizeEvent();
        processEvent(ApplicationGainFocusEvent());

        LOG_INFO << "Window created, resolution: " << resolution << " with " << int(fsaaMode_) << "xAA";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        destroyWindow();

        return false;
    }
}

bool PlatformMacOS::resizeWindow(const Resolution& resolution, WindowMode windowMode, FSAAMode fsaa)
{
    if (!resolutions_.has(resolution))
        return false;

    if (resolution == getCurrentResolution())
    {
        if (windowMode != windowMode_)
            [m->nsWindow toggleFullScreen:nil];

        return true;
    }

    LOG_INFO << "Resizing main window to resolution " << resolution;

    // Update the size of the GL backing surface
    auto dimensions = std::array<GLint, 2>{{GLint(resolution.getWidth()), GLint(resolution.getHeight())}};
    [m->nsOpenGLContext setValues:dimensions.data() forParameter:NSOpenGLCPSurfaceBackingSize];

    // Center the window on resize
    if (windowMode == Windowed)
    {
        auto contentRect = NSMakeRect((nativeResolution_.getWidth() - resolution.getWidth()) / 2,
                                      (nativeResolution_.getHeight() - resolution.getHeight()) / 2,
                                      resolution.getWidth(), resolution.getHeight());

        [m->nsWindow setFrame:[m->nsWindow frameRectForContentRect:contentRect] display:YES];
    }

    // Tell the OpenGL context its view has been altered
    [m->nsOpenGLContext update];

    currentResolution_ = resolution;
    windowMode_ = windowMode;
    fsaaMode_ = fsaa;
    updatePersistentSettings();

    sendResizeEvent();

    return true;
}

void PlatformMacOS::destroyWindow()
{
    releaseInputLock();

    [NSOpenGLContext clearCurrentContext];

    if (m->nsOpenGLContext)
    {
        [m->nsOpenGLContext clearDrawable];
        m->nsOpenGLContext = nil;
    }

    if (m->nsWindow)
    {
        [m->nsWindow close];
        m->nsWindow = nil;
    }

    m->windowDelegate = nil;

    currentResolution_ = Resolution::Zero;
    windowMode_ = Windowed;
    fsaaMode_ = FSAANone;

    LOG_INFO << "Window destroyed";
}

bool PlatformMacOS::setWindowTitle(const UnicodeString& title)
{
    windowTitle_ = title;

    if (m->nsWindow)
        [m->nsWindow setTitle:title.toNSString()];

    return true;
}

VoidFunction PlatformMacOS::getOpenGLFunctionAddress(const String& function) const
{
    return m->openGLFramework.mapFunction(function.cStr());
}

void PlatformMacOS::swap()
{
    if (m->nsOpenGLContext)
        [m->nsOpenGLContext flushBuffer];
}

bool PlatformMacOS::setVerticalSyncEnabled(bool enabled)
{
    auto sync = enabled ? 1 : 0;
    if (CGLSetParameter(CGLContextObj([m->nsOpenGLContext CGLContextObj]), kCGLCPSwapInterval, &sync) != 0)
    {
        LOG_ERROR << "CGLSetParameter() failed";
        return false;
    }

    isVerticalSyncEnabled_ = enabled;
    updatePersistentSettings();

    return true;
}

float PlatformMacOS::getFinalDisplayAspectRatio() const
{
    return float(m->nsWindow.contentView.frame.size.width) / float(m->nsWindow.contentView.frame.size.height);
}

bool PlatformMacOS::releaseInputLock()
{
    if (!m->isCursorVisible)
    {
        CGDisplayShowCursor(kCGNullDirectDisplay);
        LOG_INFO << "Showing the macOS cursor";
        m->isCursorVisible = true;
    }

    CGAssociateMouseAndMouseCursorPosition(YES);

    isHoldingInputLock_ = false;

    return true;
}

void PlatformMacOS::hideCursor()
{
    if (m->isCursorVisible)
    {
        CGDisplayHideCursor(kCGNullDirectDisplay);
        LOG_INFO << "Hiding the macOS cursor";
        m->isCursorVisible = false;
    }

    CGAssociateMouseAndMouseCursorPosition(NO);
}

bool PlatformMacOS::processEvent(const Event& e)
{
    if (!PlatformInterface::processEvent(e))
        return false;

    if (e.as<UpdateEvent>())
    {
        mouseRelative_ = Vec2::Zero;

        // Pass on window events
        while (true)
        {
            auto event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
            if (!event)
                break;

            switch ([event type])
            {
                case NSKeyDown:
                case NSKeyUp:
                {
                    auto isKeyDownEvent = ([event type] == NSKeyDown);

                    // Convert key code to a KeyConstant enum value
                    auto keyCode = [event keyCode];
                    auto key = keyCode < m->keyMappings.size() ? m->keyMappings[keyCode] : KeyNone;

                    if (key != KeyNone && ![event isARepeat])
                    {
                        // Update the key pressed state
                        setIsKeyPressed(key, isKeyDownEvent);

                        if (isKeyDownEvent)
                        {
                            onInputDownEvent(key);

                            // Cmd+Q quits the application
                            if (key == KeyQ && ([NSEvent modifierFlags] & NSCommandKeyMask))
                                events().dispatchEvent(ShutdownRequestEvent());

                            // Cmd+H hides the application
                            else if (key == KeyH && ([NSEvent modifierFlags] & NSCommandKeyMask))
                                [NSApp hide:nil];

                            // Cmd+Option+H hides other applications
                            else if (key == KeyH && ([NSEvent modifierFlags] & NSCommandKeyMask) &&
                                     ([NSEvent modifierFlags] & NSAlternateKeyMask))
                                [NSApp hideOtherApplications:nil];

                            // Cmd+Control+F toggles fullscreen
                            else if (key == KeyF && ([NSEvent modifierFlags] & NSCommandKeyMask) &&
                                     ([NSEvent modifierFlags] & NSControlKeyMask))
                                resizeWindow(currentResolution_, windowMode_ == Fullscreen ? Windowed : Fullscreen,
                                             fsaaMode_);
                        }
                        else
                            onInputUpEvent(key);
                    }

                    if (isKeyDownEvent)
                    {
                        auto characters = UnicodeString([event characters]);

                        // Get rid of non-renderable characters below U+0020 and any in the private use area
                        for (auto i = 0U; i < characters.length(); i++)
                        {
                            if (characters.at(i) < 0x20 || characters.at(i) == 0x7F ||
                                (characters.at(i) >= 0xE000 && characters.at(i) <= 0xF8FF))
                                characters.erase(i--);
                        }

                        if (characters.length())
                            events().dispatchEvent(CharacterInputEvent(characters, key));
                    }

                    continue;
                }

                case NSFlagsChanged:
                {
                    auto modifiers = [event modifierFlags];

                    static const auto keyModifierMappings = std::unordered_map<NSUInteger, KeyConstant>{
                        {NSControlKeyMask | 0x0001, KeyLeftControl}, {NSControlKeyMask | 0x2000, KeyRightControl},
                        {NSShiftKeyMask | 0x0002, KeyLeftShift},     {NSShiftKeyMask | 0x0004, KeyRightShift},
                        {NSAlternateKeyMask | 0x0020, KeyLeftAlt},   {NSAlternateKeyMask | 0x0040, KeyRightAlt},
                        {NSCommandKeyMask | 0x0008, KeyLeftMeta},    {NSCommandKeyMask | 0x0010, KeyRightMeta}};

                    for (auto mapping : keyModifierMappings)
                    {
                        auto wasPressed = (m->lastKeyboardModifiers & mapping.first) != 0;
                        auto isPressed = (modifiers & mapping.first) != 0;

                        setIsKeyPressed(mapping.second, isPressed);

                        if (!wasPressed && isPressed)
                            onInputDownEvent(mapping.second);
                        else if (wasPressed && !isPressed)
                            onInputUpEvent(mapping.second);
                    }

                    m->lastKeyboardModifiers = modifiers;
                    break;
                }

                case NSLeftMouseDown:
                case NSLeftMouseUp:
                case NSRightMouseDown:
                case NSRightMouseUp:
                {
                    auto isLeftMouseButton = ([event type] == NSLeftMouseDown || [event type] == NSLeftMouseUp);
                    auto button = isLeftMouseButton ? LeftMouseButton : RightMouseButton;

                    isMouseButtonPressed_[button] =
                        ([event type] == NSLeftMouseDown || [event type] == NSRightMouseDown);

                    if (isMouseButtonPressed_[button])
                        onInputDownEvent(button);
                    else
                        onInputUpEvent(button);

                    break;
                }

                case NSScrollWheel:
                {
                    if ([event scrollingDeltaY] < 0.0)
                        events().dispatchEvent(MouseWheelEvent(MouseWheelEvent::AwayFromUser, getMousePosition()));
                    else if ([event scrollingDeltaY] > 0.0)
                        events().dispatchEvent(MouseWheelEvent(MouseWheelEvent::TowardsUser, getMousePosition()));

                    break;
                }

                case NSMouseMoved:
                case NSLeftMouseDragged:
                case NSRightMouseDragged:
                {
                    if (events().isEventAllowed<MouseMoveEvent>())
                        mouseRelative_ += Vec2(float([event deltaX]), -float([event deltaY]));

                    break;
                }

                default:
                    break;
            }

            [NSApp sendEvent:event];
        }

        // Ignore this relative mouse change if requested
        if (mouseRelative_ != Vec2::Zero && m->ignoreNextNonZeroMouseMovement)
        {
            mouseRelative_ = Vec2::Zero;
            m->ignoreNextNonZeroMouseMovement = false;
        }

        // Update mouse position, this sends any needed mouse move events
        PlatformInterface::setMousePosition(getMousePosition() + mouseRelative_);
    }
    else if (e.as<ApplicationGainFocusEvent>())
    {
        // Move the mouse back to the center of the window
        auto frame = [m->nsWindow frame];
        auto p = CGPoint{frame.origin.x + frame.size.width * 0.5, frame.origin.y + frame.size.height * 0.5};

        CGWarpMouseCursorPosition(p);

        hideCursor();

        m->ignoreNextNonZeroMouseMovement = true;
    }

    return true;
}

TimeValue PlatformMacOS::getTime() const
{
    return TimeValue(int64_t((mach_absolute_time() * m->timebaseInfo.numer) / (int64_t(1000) * m->timebaseInfo.denom)));
}

uint64_t PlatformMacOS::getSysctl(const char* name) const
{
    auto result = uint64_t();
    auto size = sizeof(result);

    sysctlbyname(name, &result, &size, nullptr, 0);

    return result;
}

String PlatformMacOS::getOperatingSystemName() const
{
    return String() << "macOS " << [[NSProcessInfo processInfo] operatingSystemVersionString];
}

unsigned int PlatformMacOS::getCPUCount() const
{
    return uint(getSysctl("hw.ncpu"));
}

uint64_t PlatformMacOS::getCPUFrequency() const
{
    return getSysctl("hw.cpufrequency");
}

uint64_t PlatformMacOS::getSystemMemorySize() const
{
    return getSysctl("hw.memsize");
}

bool PlatformMacOS::openWithDefaultApplication(const UnicodeString& resource) const
{
    return system(("open \"" + resource + "\"").toUTF8().as<char>()) != -1;
}

bool PlatformMacOS::showMessageBox(const UnicodeString& text, const UnicodeString& title, MessageBoxButtons buttons,
                                    MessageBoxIcon icon)
{
    auto alert = [[NSAlert alloc] init];

    alert.messageText = title.toNSString();
    alert.informativeText = text.toNSString();

    if (buttons == OKButton)
        [alert addButtonWithTitle:@"OK"];
    else if (buttons == OKCancelButtons)
    {
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
    }
    else if (buttons == YesNoButtons)
    {
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
    }

    if (icon == ErrorIcon)
        alert.alertStyle = NSCriticalAlertStyle;
    else
        alert.alertStyle = NSInformationalAlertStyle;

    return [alert runModal] == NSAlertFirstButtonReturn;
}

bool PlatformMacOS::setGamma(const Color& gammas)
{
    const auto minimumGamma = 0.25f;
    const auto maximumGamma = 4.4f;

    auto r = gammas.r > 0.0f ? Math::clamp(gammas.r, minimumGamma, maximumGamma) : 0.0f;
    auto g = gammas.g > 0.0f ? Math::clamp(gammas.g, minimumGamma, maximumGamma) : 0.0f;
    auto b = gammas.b > 0.0f ? Math::clamp(gammas.b, minimumGamma, maximumGamma) : 0.0f;

    auto ramps = std::array<std::array<float, 256>, 3>();

    calculateGammaRamp(r, ramps[0], m->originalGammaRamps[0]);
    calculateGammaRamp(g, ramps[1], m->originalGammaRamps[1]);
    calculateGammaRamp(b, ramps[2], m->originalGammaRamps[2]);

    if (CGSetDisplayTransferByTable(CGMainDisplayID(), 256, ramps[0].data(), ramps[1].data(), ramps[2].data()) !=
        CGDisplayNoErr)
    {
        LOG_ERROR << "Failed setting display gamma";
        return false;
    }

    gammas_.setRGBA(r, g, b, 1.0f);
    updatePersistentSettings();

    return true;
}

void PlatformMacOS::windowDidEnterFullScreen()
{
    windowMode_ = Fullscreen;
    updatePersistentSettings();
}

void PlatformMacOS::windowDidExitFullScreen()
{
    windowMode_ = Windowed;
    updatePersistentSettings();
}

}

#endif
