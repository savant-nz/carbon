/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/EntityController/EntityController.h"

namespace Carbon
{

/**
 * This entity controller allows an entity to be controlled in a manner suitable for use in a 2D platformer-style game.
 * Using a platformer entity controller on an entity that has a physics character controller will result in a
 * controllable entity that collides with the scene's physical simulation.
 */
class CARBON_API PlatformerEntityController : public EntityController
{
public:

    PlatformerEntityController() { clear(); }

    /**
     * Resets this platformer controller to its default settings.
     */
    void clear();

    /**
     * Returns the maximum horizontal speed that this platformer controller can move at, in units per second. Defaults
     * to 20.
     */
    float getMaximumHorizontalSpeed() const { return maximumHorizontalSpeed_; }

    /**
     * Sets the maximum horizontal speed that this platformer controller can move at, in units per second. Defaults to
     * 20.
     */
    void setMaximumHorizontalSpeed(float speed) { maximumHorizontalSpeed_ = speed; }

    /**
     * Returns the maximum vertical speed that this platformer controller can move at, in units per second. Defaults to
     * 50.
     */
    float getMaximumVerticalSpeed() const { return maximumVerticalSpeed_; }

    /**
     * Sets the maximum vertical speed that this platformer controller can move at, in units per second. Defaults to 50.
     */
    void setMaximumVerticalSpeed(float speed) { maximumVerticalSpeed_ = speed; }

    /**
     * Returns the time in seconds needed to reach the maximum horizontal speed, defaults to 1.
     */
    float getTimeToMaximumHorizontalSpeed() const { return timeToMaximumHorizontalSpeed_.toSeconds(); }

    /**
     * Sets the time in seconds needed to reach the maximum horizontal speed, defaults to 1.
     */
    void setTimeToMaximumHorizontalSpeed(float time)
    {
        timeToMaximumHorizontalSpeed_ = std::max(TimeValue(time), TimeValue(0.01f));
    }

    /**
     * Returns the time in seconds needed to reach the maximum vertical speed, defaults to 1.
     */
    float getTimeToMaximumVerticalSpeed() const { return timeToMaximumVerticalSpeed_.toSeconds(); }

    /**
     * Sets the time in seconds needed to reach the maximum vertical speed, defaults to 1.
     */
    void setTimeToMaximumVerticalSpeed(float time)
    {
        timeToMaximumVerticalSpeed_ = std::max(TimeValue(time), TimeValue(0.01f));
    }

    /**
     * Returns whether or not user control of this platform controller is allowed, defaults to true.
     */
    bool isUserInputAllowed() const { return isUserInputAllowed_; }

    /**
     * Sets whether or not user control of this platform controller is allowed, defaults to true.
     */
    void setUserInputAllowed(bool allowed) { isUserInputAllowed_ = allowed; }

    /**
     * Sets the keys to use to move this controller left and right, these keys are used by
     * PlatformerEntityController::update(). The default values are left and right arrow.
     */
    void setMovementKeys(KeyConstant left, KeyConstant right)
    {
        moveLeftKey_ = left;
        moveRightKey_ = right;
    }

    /**
     * Updates the position of the entity being affected by this controller based on world collisions and whether the
     * movement keys are currently pressed. Specifying false for \a allowInput prevents user control over the platformer
     * controller, this is useful for when the entity being controlled is dead or disabled.
     */
    bool update(TimeValue time) override;

    /**
     * Returns whether this platfomer controller is currently in mid-air.
     */
    bool isInMidAir() const;

    /**
     * Causes this platformer controller to jump up into the air to the specified height. If the entity is already in
     * mid-air then calling this method will do nothing. The return value indicates whether the jump was able to be
     * initiated.
     */
    bool jump(float height, float time = 0.4f);

    /**
     * Returns whether this platformer controller is currently in the middle of a jump.
     */
    bool isJumping() const { return isJumping_; }

    /**
     * Returns the current scale on horizontal movement that affects how fast the platformer controller moves
     * horizontally while jumping. Defaults to 1.0.
     */
    float getJumpHorizontalMovementScale() const { return jumpHorizontalMovementScale_; }

    /**
     * Sets the scale on horizontal movement that affects how fast the platformer controller moves horizontally while
     * jumping. Defaults to 1.0.
     */
    void setJumpHorizontalMovementScale(float scale = 1.0f) { jumpHorizontalMovementScale_ = scale; }

    /**
     * Returns the distance fallen by the platformer controller if it was falling in previous frames and hit the ground
     * in the current frame. Note that this method will return zero while the platformer controller is falling through
     * the air as well as when it is just sitting on the ground, this means that to reliably detect falls an application
     * must poll this method every frame.
     */
    float getFallDistance() const { return fallDistance_; }

    /**
     * Returns whether gravity is currently being used on this platformer controller. Defaults to true.
     */
    bool isGravityEnabled() const { return isGravityEnabled_; }

    /**
     * Sets whether gravity should be used on this platformer controller. Defaults to true.
     */
    void setGravityEnabled(bool enabled) { isGravityEnabled_ = enabled; }

private:

    bool isUserInputAllowed_ = true;
    KeyConstant moveLeftKey_ = KeyNone;
    KeyConstant moveRightKey_ = KeyNone;

    Vec2 velocity_;

    float maximumHorizontalSpeed_ = 0.0f;
    float maximumVerticalSpeed_ = 0.0f;
    TimeValue timeToMaximumHorizontalSpeed_;
    TimeValue timeToMaximumVerticalSpeed_;
    float jumpHorizontalMovementScale_ = 0.0f;

    struct PastWorldPosition
    {
        TimeValue time;
        Vec2 position;

        PastWorldPosition() {}
        PastWorldPosition(TimeValue time_, const Vec2& position_) : time(time_), position(position_) {}
    };
    Vector<PastWorldPosition> pastWorldPositions_;

    bool isGravityEnabled_ = true;

    bool isJumping_ = false;
    TimeValue jumpStartTime_;
    float jumpHeight_ = 0.0f;
    float jumpTime_ = 0.0f;

    // Falling distances are tracked manually and the most recent fall can be accessed with getFallDistance()
    bool reportFallWhenNextOnGround_ = false;
    float maximumYSinceLastOnGround_ = 0.0f;
    float fallDistance_ = 0.0f;

    TimeValue timeSinceLastUpdate_;
};

}
