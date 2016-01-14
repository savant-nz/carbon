/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef MACOSX

#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/PlatformEvents.h"

// On Mac OS X a Cocoa NSApplication subclass is used to run the application

#undef new
#include <Cocoa/Cocoa.h>
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"

// A pointer to the main routine of the application
static auto fnApplicationMain = std::function<int()>();

static auto exitCode = int();

// Define an NSApplication subclass to use
@interface CarbonApplicationMacOSX : NSApplication
@end

@implementation CarbonApplicationMacOSX

// Call the application's main routine once the application has launched
- (void)applicationDidFinishLaunching:(NSNotification*)note
{
    exitCode = fnApplicationMain();

    // Stop the main event loop
    [super stop:self];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    if (Carbon::Globals::isEngineInitialized())
        Carbon::events().dispatchEvent(Carbon::ShutdownRequestEvent());

    return NSTerminateNow;
}

- (void)terminateApplication:(id)sender
{
    if (Carbon::Globals::isEngineInitialized())
        Carbon::events().dispatchEvent(Carbon::ShutdownRequestEvent());
}

- (void)applicationWillBecomeActive:(NSNotification*)aNotification
{
    if (Carbon::Globals::isEngineInitialized())
        Carbon::events().dispatchEvent(Carbon::ApplicationGainFocusEvent());
}

- (void)applicationDidResignActive:(NSNotification*)aNotification
{
    if (Carbon::Globals::isEngineInitialized())
        Carbon::events().dispatchEvent(Carbon::ApplicationLoseFocusEvent());
}
@end

namespace Carbon
{

static void createMenus(const UnicodeString& applicationName)
{
    // Create menu object
    auto menu = [[NSMenu alloc] initWithTitle:@""];

    auto appName = applicationName.toNSString();

    // Add 'Hide' and 'Hide Others' menu items
    auto title = [@"Hide " stringByAppendingString:appName];
    auto item = [menu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];
    [item setKeyEquivalentModifierMask:(NSCommandKeyMask)];

    item = [menu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [item setKeyEquivalentModifierMask:(NSAlternateKeyMask | NSCommandKeyMask)];

    // Add 'Quit' menu item
    title = [@"Quit " stringByAppendingString:appName];
    [menu addItemWithTitle:title action:@selector(terminateApplication:) keyEquivalent:@"q"];
    [item setKeyEquivalentModifierMask:(NSCommandKeyMask)];

    // Put the menu into the menubar and tell the application that it is the application menu
    item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [item setSubmenu:menu];
    [[NSApp mainMenu] addItem:item];
}

CARBON_API int runMacOSXApplication(const std::function<int()>& fnMain, const UnicodeString& applicationName)
{
    @autoreleasepool
    {
        // Initialize and activate the main application object
        [CarbonApplicationMacOSX sharedApplication];
        [static_cast<NSApplication*>(NSApp) setDelegate:NSApp];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp disableRelaunchOnLogin];

        // Create menu
        [NSApp setMainMenu:[[NSMenu alloc] init]];
        createMenus(applicationName);

        // Run the application
        fnApplicationMain = fnMain;
        [NSApp run];

        // Clean up
        [NSApp setMainMenu:nil];
    }

    return exitCode;
}

}

#endif
