/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef APPLE

#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/Threads/Thread.h"
#include "CarbonEngine/Game/GameCenter.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

#undef new
#include <GameKit/GameKit.h>
#ifdef iOS
    #include <UIKit/UIKit.h>
#endif
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"

#if defined(iOS) || MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_10
    #define CARBON_USE_MODERN_GAMECENTER_API
#endif

@interface GameCenterDelegate : NSObject <GKGameCenterControllerDelegate>
@property (atomic) bool* isUIShowing;
#ifdef MACOSX
@property (atomic) GKDialogController* gkDialogController;
#endif
@end

@implementation GameCenterDelegate
@synthesize isUIShowing;
#ifdef MACOSX
@synthesize gkDialogController;
#endif

- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController*)gkViewController
{
    LOG_INFO << "Closed the Game Center UI";

    *self.isUIShowing = false;

#ifdef iOS
    [[[UIApplication sharedApplication] keyWindow].rootViewController dismissViewControllerAnimated:YES completion:nil];
#elif defined(MACOSX)
    [self.gkDialogController dismiss:self];

    // Grab input lock and allow input events again
    Carbon::events().dispatchEvent(Carbon::ApplicationGainFocusEvent());
    Carbon::platform().setKeyboardInputEventsAllowed(true);
    Carbon::platform().setMouseInputEventsAllowed(true);
#endif
}
@end

namespace Carbon
{

class GameCenter::Members
{
public:

    std::unordered_map<String, std::pair<GKLeaderboard*, int64_t>> leaderboards;

    bool isAuthenticated = false;

    NSMutableDictionary* achievements = nil;

    GKGameCenterViewController* gkViewController = nil;
    GameCenterDelegate* gkDelegate = nil;

    bool isUIShowing = false;

#ifdef MACOSX
    GKDialogController* gkDialogController = nil;
#endif
};

GameCenter::GameCenter() : onPlayerChanged(this), onAchievementsLoaded(this), onLeaderboardLoaded(this)
{
    m = new Members;

    m->gkDelegate = [[GameCenterDelegate alloc] init];
    m->gkDelegate.isUIShowing = &m->isUIShowing;

#ifdef MACOSX
    m->gkDialogController = [[GKDialogController alloc] init];
    m->gkDelegate.gkDialogController = m->gkDialogController;

    events().addHandler<ApplicationGainFocusEvent>(this, true);
#endif
}

GameCenter::~GameCenter()
{
#ifdef MACOSX
    [m->gkDialogController dismiss:m->gkDelegate];
    m->gkDialogController = nil;
#endif

    m->achievements = nil;
    m->gkViewController = nil;
    m->gkDelegate = nil;

    events().removeHandler(this);

    delete m;
    m = nullptr;
}

void GameCenter::enable(const Vector<String>& leaderboards)
{
    auto localPlayer = [GKLocalPlayer localPlayer];
    if (localPlayer.authenticated == YES)
        return;

    for (auto& leaderboard : leaderboards)
        m->leaderboards[leaderboard] = std::pair<GKLeaderboard*, int64_t>(nil, 0);

    LOG_INFO << "Setting Game Center authenticate handler";

#ifdef iOS
    localPlayer.authenticateHandler = ^(UIViewController* viewController, NSError* authenticationError)
#elif defined(MACOSX)
    localPlayer.authenticateHandler = ^(NSViewController* viewController, NSError* authenticationError)
#endif
    {
        assert(Thread::isRunningInMainThread());

        if ([GKLocalPlayer localPlayer].authenticated == YES)
        {
            m->isAuthenticated = true;
            m->achievements = nil;

            onPlayerChanged.fire(getPlayerName());

            LOG_INFO << "GameCenter authentication succeeded, player name: " << getPlayerName();

            // Try and load current leaderboards and get the local player's scores for them
            for (const auto& leaderboard : m->leaderboards)
                updateLeaderboard(leaderboard.first);

            // Try and load current achievements
            [GKAchievement loadAchievementsWithCompletionHandler:^(NSArray* achievements, NSError* loadError) {
                if (loadError)
                    LOG_ERROR_WITHOUT_CALLER << "Game Center achievements load error: " << [loadError localizedDescription];
                else
                {
                    assert(Thread::isRunningInMainThread());

                    // Store the achievements
                    auto dict = [[NSMutableDictionary alloc] init];
                    for (GKAchievement* achievement in achievements)
                    {
                        dict[achievement.identifier] = achievement;
                        LOG_INFO << "Read Game Center achivement: " << achievement.identifier << " = "
                                 << achievement.percentComplete << "%";
                    }
                    m->achievements = dict;

                    onAchievementsLoaded.fire(uint([achievements count]));
                    LOG_INFO << "Game Center achievements loaded";
                }
            }];
        }
        else
        {
            // Clear out any previously authenticated player
            if (m->isAuthenticated)
            {
                m->isAuthenticated = false;
                m->achievements = nil;
                for (auto& leaderboard : leaderboards)
                    m->leaderboards[leaderboard] = std::pair<GKLeaderboard*, int64_t>(nil, 0);

                onPlayerChanged.fire(UnicodeString::Empty);
            }

            if (viewController)
            {
                LOG_INFO << "Showing Game Center view controller";

#ifdef iOS
                auto rootViewController = [[UIApplication sharedApplication] keyWindow].rootViewController;
                [rootViewController presentViewController:viewController animated:YES completion:nil];
#elif defined(MACOSX)
                // TODO: work out how to show this view controller, is this path ever even used on Mac OS X?
                LOG_ERROR_WITHOUT_CALLER << "Got a view controller from game center that should be shown";
#endif
            }
            else
            {
                LOG_ERROR_WITHOUT_CALLER << "Game Center authentication error: '" << [authenticationError localizedDescription]
                                         << "'";
            }
        }
    };
}

void GameCenter::updateLeaderboard(const String& leaderboardID)
{
    m->leaderboards[leaderboardID] = std::pair<GKLeaderboard*, int64_t>(nil, 0);

    auto localPlayer = [GKLocalPlayer localPlayer];

    auto leaderboard = [[GKLeaderboard alloc]
#ifdef CARBON_USE_MODERN_GAMECENTER_API
        initWithPlayers:@[ localPlayer ]
#else
        initWithPlayerIDs:@[ [localPlayer playerID] ]
#endif
    ];

#ifdef CARBON_USE_MODERN_GAMECENTER_API
    leaderboard.identifier = leaderboardID.toNSString();
#else
    leaderboard.category = leaderboardID.toNSString();
#endif

    [leaderboard loadScoresWithCompletionHandler:^(NSArray* scores, NSError* error) {
        assert(Thread::isRunningInMainThread());

        if (error)
            LOG_ERROR_WITHOUT_CALLER << "Failed loading Game Center leaderboard, error: " << [error localizedDescription];
        else
        {
            m->leaderboards[leaderboardID] =
                std::pair<GKLeaderboard*, int64_t>(leaderboard, [leaderboard.localPlayerScore value]);

            onLeaderboardLoaded.fire(leaderboardID);

            LOG_INFO << "Loaded leaderboard: " << leaderboardID << ", score: " << [leaderboard.localPlayerScore value];
        }
    }];
}

bool GameCenter::isAuthenticated() const
{
    return m->isAuthenticated;
}

UnicodeString GameCenter::getPlayerName() const
{
    return isAuthenticated() ? [[GKLocalPlayer localPlayer] alias] : UnicodeString::Empty;
}

void GameCenter::showUI(GameCenterUI ui)
{
    if (!isAuthenticated())
    {
        LOG_ERROR << "Can't show the Game Center UI when there is no local player";
        return;
    }

    if (m->isUIShowing)
        return;

    LOG_INFO << "Showing the Game Center UI";

    if (!m->gkViewController)
    {
        m->gkViewController = [[GKGameCenterViewController alloc] init];
        m->gkViewController.gameCenterDelegate = m->gkDelegate;
    }

    if (ui == Achievements)
        m->gkViewController.viewState = GKGameCenterViewControllerStateAchievements;
    else if (ui == Leaderboards)
        m->gkViewController.viewState = GKGameCenterViewControllerStateLeaderboards;
    else
        m->gkViewController.viewState = GKGameCenterViewControllerStateDefault;

#ifdef iOS
    auto rootViewController = [[UIApplication sharedApplication] keyWindow].rootViewController;
    [rootViewController presentViewController:m->gkViewController animated:YES completion:nil];
#elif defined(MACOSX)
    // Set the parent window every time as it can change as a result of fullscreen or resolution switching
    m->gkDialogController.parentWindow = [NSApp mainWindow];
    [m->gkDialogController presentViewController:m->gkViewController];

    // Release input lock so the mouse cursor is visible, and disable input events while the Game Center UI is showing
    platform().releaseInputLock();
    platform().setKeyboardInputEventsAllowed(false);
    platform().setMouseInputEventsAllowed(false);
#endif

    m->isUIShowing = true;
}

bool GameCenter::processEvent(const Event& e)
{
    return !(e.as<ApplicationGainFocusEvent>() && m->isUIShowing);
}

int64_t GameCenter::getLeaderboardScore(const String& leaderboardID) const
{
    if (!isAuthenticated())
    {
        LOG_ERROR << "Can't retrieve leaderboard scores when there is no local player";
        return std::numeric_limits<int64_t>::min();
    }

    auto i = m->leaderboards.find(leaderboardID);
    if (i == m->leaderboards.end())
    {
        LOG_ERROR << "Unknown leaderboard: " << leaderboardID;
        return std::numeric_limits<int64_t>::min();
    }

    auto leaderboard = i->second.first;

    return leaderboard ? i->second.second : std::numeric_limits<int64_t>::min();
}

void GameCenter::reportLeaderboardScore(const String& leaderboardID, int64_t score)
{
    if (!isAuthenticated())
    {
        LOG_ERROR << "Can't retrieve leaderboard scores when there is no local player";
        return;
    }

    if (m->leaderboards.find(leaderboardID) == m->leaderboards.end())
    {
        LOG_ERROR << "Unknown leaderboard: " << leaderboardID;
        return;
    }

#ifdef CARBON_USE_MODERN_GAMECENTER_API
    auto gkScore = [[GKScore alloc] initWithLeaderboardIdentifier:leaderboardID.toNSString()];
#else
    auto gkScore = [[GKScore alloc] initWithCategory:leaderboardID.toNSString()];
#endif
    gkScore.value = score;
    gkScore.context = 0;

    auto scores = pointer_to<NSArray>::type();

    // Report the score
    [GKScore reportScores:scores
        withCompletionHandler:^(NSError* error) {
            if (error)
                LOG_ERROR_WITHOUT_CALLER << "Error reporting Game Center score: " << [error localizedDescription];
            else
            {
                LOG_INFO << "Reported score of " << score << " to Game Center leaderboard '" << leaderboardID << "'";

                if (!m->leaderboards[leaderboardID].first)
                    updateLeaderboard(leaderboardID);
                else
                    m->leaderboards[leaderboardID].second = std::max(m->leaderboards[leaderboardID].second, score);
            }
        }];
}

bool GameCenter::areAchievementsLoaded() const
{
    return m->achievements != nil;
}

float GameCenter::getAchievementProgress(const String& achievementID) const
{
    if (!isAuthenticated())
    {
        LOG_ERROR << "Can't retrieve achievement progress when there is no local player";
        return -1.0f;
    }

    if (!areAchievementsLoaded())
    {
        LOG_ERROR << "Can't retrieve achievement progress when achievements haven't been loaded";
        return -1.0f;
    }

    auto achievement = static_cast<GKAchievement*>(m->achievements[achievementID.toNSString()]);
    if (!achievement)
        return 0.0f;

    return float(achievement.percentComplete);
}

void GameCenter::reportAchievementProgress(const String& achievementID, float percentComplete)
{
    if (!isAuthenticated())
    {
        LOG_ERROR << "Can't report achievement progress when there is no local player";
        return;
    }

    if (!areAchievementsLoaded())
    {
        LOG_ERROR << "Can't report achievement progress when achievements haven't been loaded";
        return;
    }

    auto achievement = [[GKAchievement alloc] initWithIdentifier:achievementID.toNSString()];
    achievement.percentComplete = percentComplete;

    auto achievements = pointer_to<NSArray>::type();

    [GKAchievement reportAchievements:achievements
                withCompletionHandler:^(NSError* error) {
                    if (error)
                        LOG_ERROR_WITHOUT_CALLER << "Error reporting Game Center achievement: " << [error localizedDescription];
                    else
                    {
                        LOG_INFO << "Updated Game Center achievement: " << achievement.identifier << " = "
                                 << achievement.percentComplete << "%";

                        if (m->achievements)
                            m->achievements[achievement.identifier] = achievement;
                    }
                }];
}

void GameCenter::resetAchievements()
{
    if (!isAuthenticated())
    {
        LOG_ERROR << "Can't reset achievements when there is no local player";
        return;
    }

    if (!areAchievementsLoaded())
    {
        LOG_ERROR << "Can't reset achievements when they haven't been loaded";
        return;
    }

    // Clear stored achievements
    m->achievements = nil;

    // Clear all achievement progress saved in Game Center
    [GKAchievement resetAchievementsWithCompletionHandler:^(NSError* error) {
        if (error)
            LOG_ERROR_WITHOUT_CALLER << "Error resetting Game Center achievements: " << [error localizedDescription];
        else
        {
            m->achievements = [[NSMutableDictionary alloc] init];

            LOG_INFO << "Game Center achievements were reset";
        }
    }];
}

void GameCenter::showBanner(const UnicodeString& title, const UnicodeString& message)
{
    [GKNotificationBanner showBannerWithTitle:title.toNSString() message:message.toNSString() completionHandler:nil];
}

}

#endif
