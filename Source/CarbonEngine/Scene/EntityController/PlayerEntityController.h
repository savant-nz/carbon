/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Math/WeightedFilter.h"
#include "CarbonEngine/Platform/KeyConstant.h"
#include "CarbonEngine/Platform/TimeValue.h"
#include "CarbonEngine/Scene/EntityController/EntityController.h"

namespace Carbon
{

/**
 * This entity controller allows an entity to be moved using standard FPS-style or free-camera movement, using either mouse look
 * or touch look. Using a player entity controller on an entity that has a physics character controller will result in a
 * controllable entity (often a camera) that collides with the scene's physical simulation.
 */
class CARBON_API PlayerEntityController : public EntityController, public EventHandler
{
public:

    /**
     * Whether to invert mouse look on this player controller. Defaults to false.
     */
    static bool InvertMouse;

    PlayerEntityController();
    ~PlayerEntityController() override;

    /**
     * Resets this player controller to its default settings.
     */
    void clear();

    /**
     * Returns whether movement is enabled on this player controller, movement behavior can be controlled using the
     * PlayerEntityController::setMovementKeys() method. Movement is enabled by default.
     */
    bool isMovementEnabled() const { return isMovementEnabled_; }

    /**
     * Sets whether movement is enabled on this player controller, see PlayerEntityController::isMovementEnabled() for details.
     */
    void setMovementEnabled(bool enabled) { isMovementEnabled_ = enabled; }

    /**
     * Returns the acceleration applied to this player controller by the movement keys
     */
    float getMovementAcceleration() const { return movementAcceleration_; }

    /**
     * Sets the acceleration applied to this player controller by the movement keys. Defaults to 100.0.
     */
    void setMovementAcceleration(float acceleration) { movementAcceleration_ = acceleration; }

    /**
     * Returns the current horizontal friction being applied to this player controller while not in mid-air. Defaults to 0.999.
     * The friction value specifies what fraction of the current velocity will be lost per second, and so is clamped between
     * zero and one where zero is no friction at all and one will prohibit any movement.
     */
    float getHorizontalFriction() const { return horizontalFriction_; }

    /**
     * Sets the current horizontal friction being applied to this player controller while not in mid-air. See
     * PlayerEntityController::getHorizontalFriction() for details.
     */
    void setHorizontalFriction(float friction) { horizontalFriction_ = friction; }

    /**
     * Returns the current horizontal friction being applied to this player controller while in mid-air when mid-air movement is
     * not enabled (see PlayerEntityController::isMidAirMovementEnabled() for details). Defaults to 0.01. See
     * PlayerEntityController::getHorizontalFriction() for a description of the friction value.
     */
    float getMidAirHorizontalFriction() const { return midAirHorizontalFriction_; }

    /**
     * Sets the current horizontal friction being applied to this player controller while in mid-air. See
     * PlayerEntityController::getMidAirHorizontalFriction() for details.
     */
    void setMidAirHorizontalFriction(float friction) { midAirHorizontalFriction_ = friction; }

    /**
     * Returns the current vertical friction being applied to this player controller while not in mid-air. Defaults to 0.999.
     * See PlayerEntityController::getHorizontalFriction() for a description of the friction value.
     */
    float getVerticalFriction() const { return verticalFriction_; }

    /**
     * Sets the current vertical friction being applied to this player controller while not in mid-air. See
     * PlayerEntityController::getVerticalFriction() for details.
     */
    void setVerticalFriction(float friction) { verticalFriction_ = friction; }

    /**
     * Returns the current vertical friction being applied to this player controller while in mid-air. Defaults to 0.01. See
     * PlayerEntityController::getHorizontalFriction() for a description of the friction value.
     */
    float getMidAirVerticalFriction() const { return midAirVerticalFriction_; }

    /**
     * Sets the current vertical friction being applied to this player controller while in mid-air. See
     * PlayerEntityController::getMidAirVerticalFriction() for details.
     */
    void setMidAirVerticalFriction(float friction) { midAirVerticalFriction_ = friction; }

    /**
     * Returns whether this player controller is restricted to moving only in the XZ plane.
     */
    bool isXZPlaneOnly() const { return isXZPlaneOnly_; }

    /**
     * Sets whether this player controller is only restricted to moving only in the XZ plane. Defaults to false.
     */
    void setXZPlaneOnly(bool value) { isXZPlaneOnly_ = value; }

    /**
     * Sets the keys to use to move this controller forward, backward, left and right. The set keys are used in the
     * PlayerEntityController::updateMovement() method. The default values are KeyW, KeyS, KeyA, and KeyD respectively (WSAD
     * movement).
     */
    void setMovementKeys(KeyConstant forward, KeyConstant back, KeyConstant left, KeyConstant right);

    /**
     * Returns whether mouse look is enabled on this player controller, mouse look behavior can be controlled using
     * PlayerEntityController::InvertMouse and PlayerEntityController::setMouseLookSensitivity(). Mouse look is enabled by
     * default.
     */
    bool isMouseLookEnabled() const { return isMouseLookEnabled_; }

    /**
     * Sets whether mouse look is enabled on this player controller, see PlayerEntityController::isMouseLookEnabled() for
     * details.
     */
    void setMouseLookEnabled(bool enabled) { isMouseLookEnabled_ = enabled; }

    /**
     * Returns the X and Y sensitivities used when mouse look is enabled on this player controller. Defaults to 0.0075.
     */
    const Vec2& getMouseLookSensitivity() const { return mouseLookSensitivity_; }

    /**
     * Sets the X and Y sensitivities used when mouse look is enabled on this player controller. Defaults to 0.0075.
     */
    void setMouseLookSensitivity(const Vec2& sensitivity) { mouseLookSensitivity_ = sensitivity; }

    /**
     * Returns whether touch look is enabled on this player controller, touch look behavior can be controlled using
     * PlayerEntityController::setTouchLookSensitivity(). Touch look is enabled by default.
     */
    bool isTouchLookEnabled() const { return isTouchLookEnabled_; }

    /**
     * Sets whether touch look is enabled on this player controller, see PlayerEntityController::isTouchLookEnabled() for
     * details.
     */
    void setTouchLookEnabled(bool enabled)
    {
        isTouchLookEnabled_ = enabled;
        touchLookDelta_ = Vec2::Zero;
    }

    /**
     * Returns the X and Y sensitivities used when touch look is enabled on this player controller. Defaults to 0.0075.
     */
    const Vec2& getTouchLookSensitivity() const { return touchLookSensitivity_; }

    /**
     * Sets the X and Y sensitivities used when touch look is enabled on this player controller. Defaults to 0.0075.
     */
    void setTouchLookSensitivity(const Vec2& sensitivity) { touchLookSensitivity_ = sensitivity; }

    /**
     * Returns whether this player controller is currently in mid-air.
     */
    bool isInMidAir() const;

    /**
     * Returns the heaviness factor of the player which affects the rate of fall due to gravity. Defaults to 80.0.
     */
    float getHeaviness() const { return heaviness_; }

    /**
     * Sets the heaviness of the player, see PlayerEntityController::getHeaviness() for details.
     */
    void setHeaviness(float mass) { heaviness_ = mass; }

    /**
     * Returns whether this player controller will allow movement control while the player is in mid-air. If this is false then
     * the player will be unable to alter their velocity until they next touch the ground. If this is true then the player will
     * be able to change direction in mid-air. Defaults to true.
     */
    bool isMidAirMovementEnabled() const { return isMidAirMovementEnabled_; }

    /**
     * Sets whether this player controller will allow movement control while in mid-air. See
     * PlayerEntityController::isMidAirMovementEnabled() for details.
     */
    void setMidAirMovementEnabled(bool enabled) { isMidAirMovementEnabled_ = enabled; }

    /**
     * Returns whether gravity is currently being used on this player controller.
     */
    bool isGravityEnabled() const { return isGravityEnabled_; }

    /**
     * Sets whether gravity should be used on this player controller.
     */
    void setGravityEnabled(bool enabled) { isGravityEnabled_ = enabled; }

    bool update(TimeValue time) override;
    bool processEvent(const Event& e) override;

    // TODO: implement controller save and load methods

private:

    bool isMovementEnabled_ = true;
    KeyConstant moveForwardKey_ = KeyNone;
    KeyConstant moveBackwardKey_ = KeyNone;
    KeyConstant moveLeftKey_ = KeyNone;
    KeyConstant moveRightKey_ = KeyNone;
    float movementAcceleration_ = 0.0f;
    bool isXZPlaneOnly_ = false;
    Vec3 getMovementVector(float time, bool forward, bool backward, bool left, bool right);

    bool isMouseLookEnabled_ = true;
    bool isTouchLookEnabled_ = true;
    Vec2 mouseLookSensitivity_;
    Vec2 touchLookSensitivity_;

    Vec2 touchLookDelta_;

    WeightedFilter<Vec2, 5> lookAngleFilter_;
    void applyLook(const Vec2& delta);

    Vec3 velocity_;

    float horizontalFriction_ = 0.0f;
    float midAirHorizontalFriction_ = 0.0f;
    float verticalFriction_ = 0.0f;
    float midAirVerticalFriction_ = 0.0f;

    float heaviness_ = 0.0f;

    bool isMidAirMovementEnabled_ = false;
    bool isGravityEnabled_ = false;

    struct PastWorldPosition
    {
        TimeValue time;
        Vec3 position;

        PastWorldPosition() {}
        PastWorldPosition(TimeValue time_, const Vec3& position_) : time(time_), position(position_) {}
    };
    Vector<PastWorldPosition> pastWorldPositions_;
};

}
