/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Scene/EntityController/PlatformerEntityController.h"

namespace Carbon
{

void PlatformerEntityController::clear()
{
    isUserInputAllowed_ = true;
    moveLeftKey_ = KeyLeftArrow;
    moveRightKey_ = KeyRightArrow;

    velocity_ = Vec2::Zero;

    maximumHorizontalSpeed_ = 20.0f;
    maximumVerticalSpeed_ = 50.0f;
    timeToMaximumHorizontalSpeed_ = 1.0f;
    timeToMaximumVerticalSpeed_ = 1.0f;
    jumpHorizontalMovementScale_ = 1.0f;

    pastWorldPositions_.clear();

    isGravityEnabled_ = true;

    isJumping_ = false;
    jumpStartTime_.clear();
    jumpHeight_ = 0.0f;
    jumpTime_ = 0.0f;

    reportFallWhenNextOnGround_ = false;
    maximumYSinceLastOnGround_ = 0.0f;
    fallDistance_ = 0.0f;

    timeSinceLastUpdate_.clear();
}

bool PlatformerEntityController::update(TimeValue time)
{
    // Check there is a character controller to work with
    if (!getEntity()->characterController_)
    {
        LOG_ERROR << "Entity does not have a character controller: " << *getEntity();
        return true;
    }

    // The maximum update rate is tied to the physics update rate
    auto timePerSubstep = physics().getSubstepSize();

    // Work out how many physics steps there are to run
    timeSinceLastUpdate_ += std::min(time, TimeValue(0.1f));
    auto substepCount = timeSinceLastUpdate_ / timePerSubstep;
    if (substepCount == 0)
        return true;

    // Roll over any unused time to future frames
    timeSinceLastUpdate_ -= timePerSubstep * substepCount;

    auto seconds = (physics().getSubstepSize() * substepCount).toSeconds();

    // If 10ms has elapsed then add a new past world position
    auto currentTime = platform().getTime();
    if (pastWorldPositions_.empty() || (currentTime - pastWorldPositions_[0].time).toMilliseconds() > 10.0f)
        pastWorldPositions_.prepend(PastWorldPosition(currentTime, getEntity()->getWorldPosition().toVec2()));

    // Cull outdated past world positions (> 100ms old)
    while ((currentTime - pastWorldPositions_.back().time).toMilliseconds() > 100.0f)
        pastWorldPositions_.popBack();

    // Determine actual velocity after accounting for collisions, this is only possible if there are sufficient samples
    if (pastWorldPositions_.size() >= 2)
    {
        const auto& p0 = pastWorldPositions_[0];

        for (auto i = 1U; i < pastWorldPositions_.size(); i++)
        {
            if ((p0.time - pastWorldPositions_[i].time).toSeconds() > 0.05f)
            {
                velocity_ =
                    (p0.position - pastWorldPositions_[i].position) / (p0.time - pastWorldPositions_[i].time).toSeconds();
                break;
            }
        }
    }

    // Check for a down axis collision and use the result to track falling distances and report falls
    fallDistance_ = 0.0f;
    auto collisionNormal = Vec3();
    if (physics().getCharacterControllerDownAxisCollision(getEntity()->characterController_, collisionNormal))
    {
        // Report a fall if the character controller was previously not on the ground
        if (reportFallWhenNextOnGround_)
        {
            fallDistance_ = maximumYSinceLastOnGround_ - getEntity()->getWorldPosition().y;
            reportFallWhenNextOnGround_ = false;
        }

        maximumYSinceLastOnGround_ = getEntity()->getWorldPosition().y;
    }
    else
    {
        // The controller is not in contact with the ground so a fall should be reported when the controller is next on the
        // ground, the final fall distance is determined based on how high the controller got since it was last on the ground.
        reportFallWhenNextOnGround_ = true;
        maximumYSinceLastOnGround_ = std::max(maximumYSinceLastOnGround_, getEntity()->getWorldPosition().y);
    }

    // Keyboard control of movement
    auto movement = Vec2();
    if (isUserInputAllowed_ && platform().isKeyPressed(moveLeftKey_))
        movement -= Vec2::UnitX;
    if (isUserInputAllowed_ && platform().isKeyPressed(moveRightKey_))
        movement += Vec2::UnitX;
    if (isJumping_)
        movement.x *= jumpHorizontalMovementScale_;

    // Calculate any movement offset needed for jumping
    auto jumpOffset = 0.0f;
    if (isJumping_)
    {
        // Check whether the character controller has hit something above it. If it has, and the surface hit is fairly close to
        // horizontal, then the jump terminates immediately
        if (physics().getCharacterControllerUpAxisCollision(getEntity()->characterController_, collisionNormal))
            isJumping_ = collisionNormal.dot(-Vec3::UnitY) < 0.95f;

        if (isJumping_)
        {
            auto jumpTimeElapsed = (currentTime - jumpStartTime_).toSeconds();
            if (jumpTimeElapsed >= jumpTime_ * 2.0f)
                isJumping_ = false;
            else
            {
                // Calculate the vertical jump movement for this timestep
                auto t0 = jumpTimeElapsed / jumpTime_;
                auto t1 = std::max(jumpTimeElapsed - seconds, 0.0f) / jumpTime_;

                auto jumpExponent = 2.0f;

                jumpOffset = jumpHeight_ * (powf(fabsf(1.0f - t1), jumpExponent) - powf(fabsf(1.0f - t0), jumpExponent));
            }
        }
    }

    // Apply gravity as a movement vector
    if (!isJumping_ && isGravityEnabled_)
        movement += physics().getGravityVector().toVec2().normalized();

    // Calculate accelerations
    auto horizontalAcceleration = maximumHorizontalSpeed_ / timeToMaximumHorizontalSpeed_.toSeconds();
    auto verticalAcceleration = maximumVerticalSpeed_ / timeToMaximumVerticalSpeed_.toSeconds();

    if (movement.x != 0.0f)
        velocity_.x += movement.x * horizontalAcceleration;
    else
        velocity_.x -= Math::absClamp(Math::getSign(velocity_.x) * horizontalAcceleration, fabsf(velocity_.x));

    if (movement.y != 0.0f)
        velocity_.y += movement.y * verticalAcceleration;
    else
        velocity_.y -= Math::absClamp(Math::getSign(velocity_.y) * verticalAcceleration, fabsf(velocity_.y));

    // Clamp to maximum velocities
    velocity_.x = Math::absClamp(velocity_.x, maximumHorizontalSpeed_);
    velocity_.y = Math::absClamp(velocity_.y, maximumVerticalSpeed_);

    // If the fall following a jump has hit the maximum fall speed then clamp to the maximum speed and terminate the jump
    if (velocity_.y + (jumpOffset / seconds) < -maximumVerticalSpeed_)
    {
        velocity_.y = -maximumVerticalSpeed_;
        isJumping_ = false;
        jumpOffset = 0.0f;
    }

    // Move the character controller
    physics().moveCharacterController(getEntity()->characterController_, velocity_ * seconds + Vec2(0.0f, jumpOffset), seconds);

    return true;
}

bool PlatformerEntityController::isInMidAir() const
{
    auto normal = Vec3();

    return isJumping_ || !physics().getCharacterControllerDownAxisCollision(getEntity()->characterController_, normal);
}

bool PlatformerEntityController::jump(float height, float time)
{
    if (isJumping() || !getEntity() || !getEntity()->hasCharacterController())
        return false;

    // Can only jump when there is a surface beneath the player
    auto collisionNormal = Vec3();
    if (!physics().getCharacterControllerDownAxisCollision(getEntity()->characterController_, collisionNormal))
        return false;

    // Can't launch off surfaces steeper than 45 degrees
    if (collisionNormal.dot(Vec3::UnitY) < 0.707f)
        return false;

    isJumping_ = true;
    jumpStartTime_ = platform().getTime();
    jumpHeight_ = height;
    jumpTime_ = time;

    return true;
}

}
