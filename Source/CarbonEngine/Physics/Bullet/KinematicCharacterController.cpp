/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_BULLET

#include "CarbonEngine/Physics/Bullet/KinematicCharacterController.h"

namespace Carbon
{

const btVector3 KinematicCharacterController::UpAxis(0.0, 1.0, 0.0);

const auto MarginFudgeDistance = btScalar(0.02);

// This ray intersection callback is used to exclude intersections with a single specific collision object.
class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:

    ClosestNotMeRayResultCallback(btCollisionObject* me)
        : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)), me_(me)
    {
    }

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override
    {
        if (rayResult.m_collisionObject == me_)
            return 1.0;

        return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }

private:

    btCollisionObject* me_ = nullptr;
};

// This convex intersection callback is used to exclude intersections with a single specific collision object as well as
// intersections with surfaces that are steeper than a specified maximum slope value.
class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:

    ClosestNotMeConvexResultCallback(btCollisionObject* me, const btVector3& up, btScalar minSlopeDot)
        : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
          me_(me),
          up_(up),
          minSlopeDot_(minSlopeDot)
    {
    }

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
    {
        if (convexResult.m_hitCollisionObject == me_)
            return 1.0;

        auto hitNormalWorld = btVector3();
        if (normalInWorldSpace)
            hitNormalWorld = convexResult.m_hitNormalLocal;
        else
            hitNormalWorld = convexResult.m_hitCollisionObject->getWorldTransform().getBasis() * convexResult.m_hitNormalLocal;

        if (up_.dot(hitNormalWorld) < minSlopeDot_)
            return 1.0;

        return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
    }

private:

    btCollisionObject* me_ = nullptr;
    const btVector3 up_;
    btScalar minSlopeDot_ = 0.0;
};

KinematicCharacterController::KinematicCharacterController(btPairCachingGhostObject* ghostObject, btConvexShape* convexShape,
                                                           btScalar stepHeight)
    : ghostObject_(ghostObject), convexShape_(convexShape), stepHeight_(stepHeight)
{
    setVelocityForTimeInterval(btVector3(0.0, 0.0, 0.0), 0.0);
    setMaxSlope(btRadians(45.0));

    walkVelocity_.setZero();
    normalizedWalkVelocity_.setZero();
}

bool KinematicCharacterController::recoverFromPenetration(btCollisionWorld* world)
{
    auto maxPenetration = btScalar(0.0);

    world->getDispatcher()->dispatchAllCollisionPairs(ghostObject_->getOverlappingPairCache(), world->getDispatchInfo(),
                                                      world->getDispatcher());

    currentPosition_ = ghostObject_->getWorldTransform().getOrigin();

    for (auto i = 0; i < ghostObject_->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
    {
        manifoldArray_.resize(0);

        auto collisionPair = &ghostObject_->getOverlappingPairCache()->getOverlappingPairArray()[i];

        if (collisionPair->m_algorithm)
            collisionPair->m_algorithm->getAllContactManifolds(manifoldArray_);

        for (auto j = 0; j < manifoldArray_.size(); j++)
        {
            auto manifold = manifoldArray_[j];
            auto directionSign = (manifold->getBody0() == ghostObject_) ? btScalar(-1.0) : btScalar(1.0);

            for (auto p = 0; p < manifold->getNumContacts(); p++)
            {
                auto& pt = manifold->getContactPoint(p);

                auto distance = pt.getDistance();

                // If this is a penetration then the distance will be negative, otherwise the two bodies are just touching
                if (distance < 0.0)
                {
                    if (distance < maxPenetration)
                    {
                        maxPenetration = distance;
                        penetrationNormal_ = pt.m_normalWorldOnB * directionSign;
                    }

                    // Add partial correction for this penetration
                    currentPosition_ += pt.m_normalWorldOnB * directionSign * distance * btScalar(0.2);
                }
            }
        }
    }

    // Push the ghost object away from the penetration
    auto newTransform = ghostObject_->getWorldTransform();
    newTransform.setOrigin(currentPosition_);
    ghostObject_->setWorldTransform(newTransform);

    return maxPenetration > btScalar(0.0);
}

void KinematicCharacterController::stepUp(btCollisionWorld* world)
{
    auto start = btTransform::getIdentity();
    auto end = btTransform::getIdentity();

    // Phase 1: up
    targetPosition_ = currentPosition_ + UpAxis * stepHeight_;

    // FIXME: Handle penetration properly
    start.setOrigin(currentPosition_ + UpAxis * (convexShape_->getMargin() + MarginFudgeDistance));
    end.setOrigin(targetPosition_);

    auto callback = ClosestNotMeConvexResultCallback(ghostObject_, UpAxis, maxSlopeCosine_);
    callback.m_collisionFilterGroup = ghostObject_->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = ghostObject_->getBroadphaseHandle()->m_collisionFilterMask;

    ghostObject_->convexSweepTest(convexShape_, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

    if (callback.hasHit())
    {
        // Only modify the position if the hit was a slope and not a wall or ceiling
        if (callback.m_hitNormalWorld.dot(UpAxis) > btScalar(0.0))
        {
            // We moved up only a fraction of the step height
            currentStepOffset_ = stepHeight_ * callback.m_closestHitFraction;
            currentPosition_.setInterpolate3(currentPosition_, targetPosition_, callback.m_closestHitFraction);
        }

        // TODO: hit something above us - need to cancel jumps?
    }
    else
    {
        currentStepOffset_ = stepHeight_;
        currentPosition_ = targetPosition_;
    }
}

void KinematicCharacterController::updateTargetPositionBasedOnCollision(const btVector3& hitNormal)
{
    auto movementDirection = targetPosition_ - currentPosition_;
    auto movementLength = movementDirection.length();

    if (movementLength > SIMD_EPSILON)
    {
        movementDirection /= movementLength;

        // Calculate reflection of movementDirection in the normal of the hit surface
        auto reflectDir = movementDirection - (btScalar(2.0) * movementDirection.dot(hitNormal)) * hitNormal;
        reflectDir.normalize();

        // Get the perpendicular component of the reflection vector
        auto perp = reflectDir - hitNormal * reflectDir.dot(hitNormal);

        targetPosition_ = currentPosition_ + perp * movementLength;
    }
}

void KinematicCharacterController::stepForwardAndStrafe(btCollisionWorld* world, const btVector3& move)
{
    auto start = btTransform::getIdentity();
    auto end = btTransform::getIdentity();

    targetPosition_ = currentPosition_ + move;

    auto fraction = btScalar(1.0);

    if (isPenetrating_ && normalizedWalkVelocity_.dot(penetrationNormal_) > btScalar(0.0))
        updateTargetPositionBasedOnCollision(penetrationNormal_);

    auto maxIterations = 10U;
    while (fraction > btScalar(0.01) && maxIterations-- > 0)
    {
        auto sweepDirNegative = btVector3(currentPosition_ - targetPosition_);

        auto callback = ClosestNotMeConvexResultCallback(ghostObject_, sweepDirNegative, btScalar(0.0));
        callback.m_collisionFilterGroup = ghostObject_->getBroadphaseHandle()->m_collisionFilterGroup;
        callback.m_collisionFilterMask = ghostObject_->getBroadphaseHandle()->m_collisionFilterMask;

        auto originalMargin = convexShape_->getMargin();
        convexShape_->setMargin(originalMargin + MarginFudgeDistance);

        start.setOrigin(currentPosition_);
        end.setOrigin(targetPosition_);

        ghostObject_->convexSweepTest(convexShape_, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

        convexShape_->setMargin(originalMargin);

        fraction -= callback.m_closestHitFraction;

        if (callback.hasHit())
        {
            updateTargetPositionBasedOnCollision(callback.m_hitNormalWorld);

            // Don't slide if the the walk direction and the normal of the hit surface are opposed by less than ~11 degrees,
            // this prevents sliding down shallow slopes
            if (callback.m_hitNormalWorld.dot(normalizedWalkVelocity_) < btScalar(-0.98))
                break;
            else
            {
                auto newDirection = targetPosition_ - currentPosition_;
                if (newDirection.length2() > SIMD_EPSILON)
                {
                    // If velocity is against original velocity, stop dead to avoid tiny oscillations in sloping corners
                    newDirection.normalize();
                    if (newDirection.dot(normalizedWalkVelocity_) <= btScalar(0.0))
                        break;
                }
                else
                    break;
            }
        }
        else
        {
            // We moved the whole way
            currentPosition_ = targetPosition_;
            break;
        }
    }
}

void KinematicCharacterController::stepDown(btCollisionWorld* world, btScalar dt)
{
    auto stepDrop = UpAxis * currentStepOffset_;
    targetPosition_ -= stepDrop;

    auto start = btTransform(btQuaternion::getIdentity(), currentPosition_);
    auto end = btTransform(btQuaternion::getIdentity(), targetPosition_);

    auto callback = ClosestNotMeConvexResultCallback(ghostObject_, UpAxis, maxSlopeCosine_);
    callback.m_collisionFilterGroup = ghostObject_->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = ghostObject_->getBroadphaseHandle()->m_collisionFilterMask;

    ghostObject_->convexSweepTest(convexShape_, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

    if (callback.hasHit())
    {
        // We dropped a fraction of the height, i.e. hit the floor
        currentPosition_.setInterpolate3(currentPosition_, targetPosition_, callback.m_closestHitFraction);

        // TODO: cancel jumps?
    }
    else
    {
        // we dropped the full height
        currentPosition_ = targetPosition_;
    }
}

const float UpAndDownAxisCollisionMargin = 1.0f;

void KinematicCharacterController::checkForUpAxisCollision(btCollisionWorld* world)
{
    auto start = btTransform::getIdentity();
    auto end = btTransform::getIdentity();

    start.setOrigin(currentPosition_ + UpAxis * (convexShape_->getMargin() + MarginFudgeDistance));
    end.setOrigin(start.getOrigin() + UpAxis * UpAndDownAxisCollisionMargin);

    auto callback = ClosestNotMeConvexResultCallback(ghostObject_, UpAxis, -1.0);
    callback.m_collisionFilterGroup = ghostObject_->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = ghostObject_->getBroadphaseHandle()->m_collisionFilterMask;

    ghostObject_->convexSweepTest(convexShape_, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

    hasUpAxisCollision_ = callback.hasHit();
    if (hasUpAxisCollision_)
        upAxisCollisionNormal_ = callback.m_hitNormalWorld;
}

void KinematicCharacterController::checkForDownAxisCollision(btCollisionWorld* world)
{
    auto start = btTransform::getIdentity();
    auto end = btTransform::getIdentity();

    start.setOrigin(currentPosition_ + -UpAxis * (convexShape_->getMargin() + MarginFudgeDistance));
    end.setOrigin(start.getOrigin() + -UpAxis * UpAndDownAxisCollisionMargin);

    auto callback = ClosestNotMeConvexResultCallback(ghostObject_, -UpAxis, -1.0);
    callback.m_collisionFilterGroup = ghostObject_->getBroadphaseHandle()->m_collisionFilterGroup;
    callback.m_collisionFilterMask = ghostObject_->getBroadphaseHandle()->m_collisionFilterMask;

    ghostObject_->convexSweepTest(convexShape_, start, end, callback, world->getDispatchInfo().m_allowedCcdPenetration);

    hasDownAxisCollision_ = callback.hasHit();
    if (hasDownAxisCollision_)
        downAxisCollisionNormal_ = callback.m_hitNormalWorld;
}

void KinematicCharacterController::setVelocityForTimeInterval(const btVector3& velocity, btScalar time)
{
    if (time <= btScalar(0.0))
        return;

    if (velocityTimeRemaining_ > btScalar(0.1))
        velocityTimeRemaining_ = btScalar(0.1);

    walkVelocity_ = (walkVelocity_ * velocityTimeRemaining_ + velocity * time);
    velocityTimeRemaining_ += time;
    walkVelocity_ /= velocityTimeRemaining_;

    normalizedWalkVelocity_ = walkVelocity_.length() < SIMD_EPSILON ? btVector3(0.0, 0.0, 0.0) : walkVelocity_.normalized();
}

void KinematicCharacterController::setWorldPosition(const btVector3& origin)
{
    ghostObject_->setWorldTransform(btTransform(btQuaternion::getIdentity(), origin));
}

void KinematicCharacterController::preStep(btCollisionWorld* world)
{
    auto numPenetrationLoops = 0;
    isPenetrating_ = false;

    while (recoverFromPenetration(world))
    {
        isPenetrating_ = true;

        // Could not recover from penetration after 4 loops, bail
        if (++numPenetrationLoops > 4)
            break;
    }

    currentPosition_ = ghostObject_->getWorldTransform().getOrigin();
    targetPosition_ = currentPosition_;
}

void KinematicCharacterController::playerStep(btCollisionWorld* world, btScalar dt)
{
    if (velocityTimeRemaining_ <= btScalar(0.0))
        return;

    auto transform = ghostObject_->getWorldTransform();

    const auto enableVerticalStep = false;

    if (enableVerticalStep)
        stepUp(world);

    auto dtMoving = (dt < velocityTimeRemaining_) ? dt : velocityTimeRemaining_;
    velocityTimeRemaining_ -= dtMoving;
    if (dtMoving > btScalar(0.001))
        stepForwardAndStrafe(world, walkVelocity_ * dtMoving);

    if (enableVerticalStep)
        stepDown(world, dt);

    transform.setOrigin(currentPosition_);
    ghostObject_->setWorldTransform(transform);

    checkForUpAxisCollision(world);
    checkForDownAxisCollision(world);
}

void KinematicCharacterController::updateAction(btCollisionWorld* world, btScalar deltaTime)
{
    preStep(world);
    playerStep(world, deltaTime);
}

void KinematicCharacterController::setMaxSlope(btScalar slopeRadians)
{
    maxSlopeCosine_ = btCos(slopeRadians);
}

}

#endif
