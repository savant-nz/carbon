/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Application.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Platform/iOS/PlatformiOS.h"
#include "CarbonEngine/Platform/PlatformEvents.h"

#ifdef iOS

namespace Carbon
{

CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PlatformInterface, PlatformiOS, 100)

// These two methods are defined by the client application when it includes CarbonEngine/EntryPoint.h
extern String iOSGetApplicationName();
extern Application* iOSCreateApplication();

}

// Define the application delegate to use
@interface CarboniOSApplicationDelegate : NSObject <UIApplicationDelegate>
@property (atomic) Carbon::Application* application;
@property (atomic) NSTimer* animationTimer;
@end

@implementation CarboniOSApplicationDelegate
@synthesize application;
@synthesize animationTimer;

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    // Initialize the engine
    if (!Carbon::Globals::initializeEngine(Carbon::iOSGetApplicationName()))
    {
        LOG_ERROR << "Failed initializing the engine";
        exit(0);
    }

    // Initialize the application
    self.application = Carbon::iOSCreateApplication();
    if (!self.application->run(false))
    {
        LOG_ERROR << "Failed initializing the application";

        delete self.application;
        self.application = nullptr;
    }

    self.animationTimer = nil;

    return YES;
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    LOG_INFO << "iOS application became active";
    Carbon::events().dispatchEvent(Carbon::ApplicationGainFocusEvent());

    if (self.animationTimer)
    {
        [self.animationTimer invalidate];
        self.animationTimer = nil;
    }

    self.animationTimer =
        [NSTimer scheduledTimerWithTimeInterval:1.0 / 240.0 target:self selector:@selector(tick) userInfo:nil repeats:YES];
}

- (void)applicationWillResignActive:(UIApplication*)application
{
    LOG_INFO << "iOS application will resign active";
    Carbon::events().dispatchEvent(Carbon::ApplicationLoseFocusEvent());

    [self.animationTimer invalidate];
    self.animationTimer = nil;
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    LOG_INFO << "iOS application entered the background";
    Carbon::events().dispatchEvent(Carbon::ApplicationLoseFocusEvent(true));
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    LOG_INFO << "iOS application will enter the foreground";
    Carbon::events().dispatchEvent(Carbon::ApplicationGainFocusEvent(true));
}

- (void)tick
{
    // Cycle the main loop
    if (self.application && !self.application->mainLoop())
    {
        LOG_ERROR << "iOS applications are not allowed to self-terminate";
        assert(false);
        exit(0);
    }
}

- (void)applicationWillTerminate:(UIApplication*)application
{
    LOG_INFO << "iOS application will terminate";

    [self.animationTimer invalidate];
    self.animationTimer = nil;

    // Shut down the application
    if (self.application)
        self.application->shutdown();

    // Shut down the engine
    Carbon::Globals::uninitializeEngine();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
    LOG_INFO << "iOS low memory warning received";
    Carbon::events().dispatchEvent(Carbon::LowMemoryWarningEvent());
}
@end

int main(int argc, char* argv[])
{
    @autoreleasepool
    {
        Carbon::Globals::setInStaticInitialization(false);
        Carbon::Globals::setCommandLineParameters(argc, const_cast<const char**>(argv));

        return UIApplicationMain(argc, argv, nil, @"CarboniOSApplicationDelegate");
    };
}

#endif
