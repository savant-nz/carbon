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
#include "CarbonEngine/Scene/EntityController/PlayerEntityController.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

const String InvertMouseSetting = "InvertMouse";

bool PlayerEntityController::InvertMouse = false;

// Persist the invert mouse setting
CARBON_PERSISTENT_SETTING(InvertMouse, Boolean, PlayerEntityController::InvertMouse, false)

PlayerEntityController::PlayerEntityController()
{
    clear();
    events().addHandler<TouchMoveEvent>(this);
}

PlayerEntityController::~PlayerEntityController()
{
    events().removeHandler<TouchMoveEvent>(this);
}

void PlayerEntityController::clear()
{
    isMovementEnabled_ = true;
    moveForwardKey_ = KeyW;
    moveBackwardKey_ = KeyS;
    moveLeftKey_ = KeyA;
    moveRightKey_ = KeyD;
    movementAcceleration_ = 100.0f;
    isXZPlaneOnly_ = false;

    isMouseLookEnabled_ = true;
    mouseLookSensitivity_.setXY(0.0075f);

    isTouchLookEnabled_ = true;
    touchLookSensitivity_.setXY(0.0075f);

    lookAngleFilter_.weightModifier = 0.75f;
    lookAngleFilter_.setAll(Vec2::Zero);

    velocity_ = Vec3::Zero;

    horizontalFriction_ = 0.999f;
    midAirHorizontalFriction_ = 0.01f;
    verticalFriction_ = 0.999f;
    midAirVerticalFriction_ = 0.01f;

    isMidAirMovementEnabled_ = true;
    isGravityEnabled_ = false;

    heaviness_ = 20.0f;
}

void PlayerEntityController::applyLook(const Vec2& delta)
{
    // Put new value into the look filter that is used to smooth out movements
    lookAngleFilter_.add(delta);
    auto final = lookAngleFilter_.calculateWeightedAverage();

    // Clamp the pitch so the entity can't do somersaults
    auto pitchMargin = 0.05f;
    auto pitch = getEntity()->getDirection().getPitch();
    final.y = Math::clamp(pitch + final.y, -Math::HalfPi + pitchMargin, Math::HalfPi - pitchMargin) - pitch;

    // Apply look rotation to the entity
    getEntity()->rotateAxisAngle(getEntity()->getLocalOrientation().getXVector(), final.y);
    getEntity()->rotateAroundY(final.x);
}

void PlayerEntityController::setMovementKeys(KeyConstant forward, KeyConstant back, KeyConstant left, KeyConstant right)
{
    moveForwardKey_ = forward;
    moveBackwardKey_ = back;
    moveLeftKey_ = left;
    moveRightKey_ = right;
}

bool PlayerEntityController::update(TimeValue time)
{
    // Apply mouse look if enabled
    if (isMouseLookEnabled() && getScene()->is3D())
        applyLook(platform().getMouseRelative() * mouseLookSensitivity_ * Vec2(1.0f, InvertMouse ? 1.0f : -1.0f));

    // Apply touch look if enabled
    if (isTouchLookEnabled())
    {
        applyLook(touchLookDelta_ * touchLookSensitivity_ * Vec2(1.0f, -1.0f));
        touchLookDelta_ = Vec2::Zero;
    }

    // If 10ms has elapsed then add a new past world position
    auto currentTime = platform().getTime();
    if (pastWorldPositions_.empty() || (currentTime - pastWorldPositions_[0].time).toMilliseconds() > 10.0f)
        pastWorldPositions_.prepend(PastWorldPosition(currentTime, getEntity()->getWorldPosition()));

    // Cull outdated past world positions (> 100ms old)
    while ((currentTime - pastWorldPositions_.back().time).toMilliseconds() > 100.0f)
        pastWorldPositions_.popBack();

    auto inMidAir = isInMidAir() && isGravityEnabled();

    auto seconds = std::min(time.toSeconds(), 0.05f);

    // Gravity
    if (isGravityEnabled())
        velocity_ += physics().getGravityVector() * seconds * heaviness_;

    // User control of entity movement
    if (isMovementEnabled() && (!getEntity()->hasCharacterController() || !inMidAir || isMidAirMovementEnabled()))
    {
        auto forward = platform().isKeyPressed(moveForwardKey_);
        auto back = platform().isKeyPressed(moveBackwardKey_);
        auto left = platform().isKeyPressed(moveLeftKey_);
        auto right = platform().isKeyPressed(moveRightKey_);

        auto movement = getMovementVector(seconds, forward, back, left, right);

        if (inMidAir)
            movement.y = 0.0f;

        velocity_ += movement;
    }

    auto horizontalFriction = horizontalFriction_;
    auto verticalFriction = verticalFriction_;

    // Use mid-air frictions on falling character controllers
    if (getEntity()->hasCharacterController())
    {
        horizontalFriction = (inMidAir && !isMidAirMovementEnabled()) ? midAirHorizontalFriction_ : horizontalFriction_;
        verticalFriction = inMidAir ? midAirVerticalFriction_ : verticalFriction_;
    }

    // Friction
    velocity_.x *= powf(1.0f - horizontalFriction, seconds);
    velocity_.y *= powf(1.0f - verticalFriction, seconds);
    velocity_.z *= powf(1.0f - horizontalFriction, seconds);

    // Move the entity
    if (getEntity()->hasCharacterController())
        physics().moveCharacterController(getEntity()->characterController_, velocity_ * seconds, seconds);
    else
        getEntity()->move(velocity_ * seconds);

    return true;
}

Vec3 PlayerEntityController::getMovementVector(float time, bool forward, bool backward, bool left, bool right)
{
    auto is2D = getEntity()->getScene()->is2D();

    auto vForward = is2D ? Vec3::UnitY : -getEntity()->getLocalOrientation().getZVector();
    auto vLeft = is2D ? -Vec3::UnitX : -getEntity()->getLocalOrientation().getXVector();

    auto direction = Vec3();

    if (forward)
        direction += vForward;
    if (backward)
        direction -= vForward;
    if (left)
        direction += vLeft;
    if (right)
        direction -= vLeft;
    if (isXZPlaneOnly_)
        direction.y = 0.0f;
    if (is2D)
        direction.z = 0.0f;

    return direction.ofLength(movementAcceleration_ * time);
}

bool PlayerEntityController::isInMidAir() const
{
    if (!getEntity()->hasCharacterController())
        return true;

    if (pastWorldPositions_.size() < 3)
        return false;

    auto minY = pastWorldPositions_[0].position.y;
    auto maxY = pastWorldPositions_[0].position.y;

    for (auto i = 1U; i < pastWorldPositions_.size(); i++)
    {
        minY = std::min(minY, pastWorldPositions_[i].position.y);
        maxY = std::max(maxY, pastWorldPositions_[i].position.y);
    }

    return maxY - minY > 0.1f;
}

bool PlayerEntityController::processEvent(const Event& e)
{
    if (auto tme = e.as<TouchMoveEvent>())
    {
        // Accumulate touch movements when touch look is enabled
        if (isTouchLookEnabled())
            touchLookDelta_ += tme->getPosition() - tme->getPreviousPosition();
    }

    return true;
}

}
