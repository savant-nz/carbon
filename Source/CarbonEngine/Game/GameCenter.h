/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventDelegate.h"
#include "CarbonEngine/Core/EventHandler.h"

#ifdef APPLE

namespace Carbon
{

/**
 * Wrapper class for integrating Game Center functionality on iOS and macOS, currently exposes leaderboards and
 * achievements.
 */
class CARBON_API GameCenter : public EventHandler, private Noncopyable
{
public:

    GameCenter();
    ~GameCenter() override;

    /**
     * This event is fired every time the active Game Center player changes. This can happen at any point, and
     * applications should handle this in order to keep their display of Game Center content current. An empty string
     * means no Game Center player is active.
     */
    EventDispatcher<GameCenter, const UnicodeString&> onPlayerChanged;

    /**
     * This event is fired following an onPlayerChanged event when the achievements for the newly active player have
     * been loaded.
     */
    EventDispatcher<GameCenter, unsigned int> onAchievementsLoaded;

    /**
     * This event is fired following an onPlayerChanged when a leaderboard for the newly active player has been loaded.
     */
    EventDispatcher<GameCenter, const String&> onLeaderboardLoaded;

    /**
     * This method must be called at application startup in order to use Game Center, pass the names of any leaderboards
     * that will be used.
     */
    void enable(const Vector<String>& leaderboards = {});

    /**
     * Returns whether or not there is currently an active local player that has been successfully authenticated with
     * Game Center.
     */
    bool isAuthenticated() const;

    /**
     * Returns the name of the currently active and authenticated local player, or an empty string if there is no active
     * player.
     */
    UnicodeString getPlayerName() const;

    /**
     * The Game Center UIs that can be displayed using GameCenter::showUI().
     */
    enum GameCenterUI
    {
        Achievements,
        Leaderboards
    };

    /**
     * Shows the specified Game Center UI as an overlay.
     */
    void showUI(GameCenterUI ui);

    /**
     * Returns the active player's score on the specified leaderboard, or the lowest possible negative value if the
     * leaderboard is invalid.
     */
    int64_t getLeaderboardScore(const String& leaderboardID) const;

    /**
     * Reports a score for the given leaderboard to Game Center.
     */
    void reportLeaderboardScore(const String& leaderboardID, int64_t score);

    /**
     * Returns whether the active player's achievements have been successfully loaded.
     */
    bool areAchievementsLoaded() const;

    /**
     * Returns the active player's progress on the specified achievement as a percentage. If there is no active player
     * or the achievements for the active player are not currently loaded or available then -1.0f is returned.
     */
    float getAchievementProgress(const String& achievementID) const;

    /**
     * Sets the active player's progress on the specified achievement.
     */
    void reportAchievementProgress(const String& achievementID, float percentComplete);

    /**
     * Resets all achievements for the active player.
     */
    void resetAchievements();

    /**
     * Shows a Game Center UI banner with the given title and message.
     */
    static void showBanner(const UnicodeString& title, const UnicodeString& message);

    /**
     * GameCenter event handling.
     */
    bool processEvent(const Event& e) override;

private:

    class Members;
    Members* m = nullptr;

    void updateLeaderboard(const String& leaderboardID);
};

}

#endif
