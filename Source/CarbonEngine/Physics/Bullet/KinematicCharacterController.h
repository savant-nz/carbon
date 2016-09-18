/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Physics/Bullet/Bullet.h"

#ifdef _MSC_VER
    #undef new
#endif

namespace Carbon
{

/**
 * This is a customized version of btKinematicCharacterController that supports a sliding motion in a world. It uses a
 * ghost object and convex sweep test to test for upcoming collisions, combined with discrete collision detection to
 * recover from penetrations.
 */
class KinematicCharacterController : public btActionInterface
{
public:

    static const btVector3 UpAxis;

    KinematicCharacterController(btPairCachingGhostObject* ghostObject, btConvexShape* convexShape,
                                 btScalar stepHeight);
    ~KinematicCharacterController() override {}

    /**
     * Explicitly repositions this character controller, no collisions are done as a result of this repositioning.
     */
    void setWorldPosition(const btVector3& origin);

    /**
     * Sets the velocity of this character controller as well as the time period that it should move at that velocity
     * for. Multiple calls to this method accumulate any leftover movement from previous calls.
     */
    void setVelocityForTimeInterval(const btVector3& velocity, btScalar time);

    /**
     * Sets the maximum slope thath the controller can walk up/down.
     */
    void setMaxSlope(btScalar slopeRadians);

    /**
     * Returns whether there was any collision directly above this character controller.
     */
    bool getUpAxisCollision(btVector3& collisionNormal) const
    {
        collisionNormal = upAxisCollisionNormal_;

        return hasUpAxisCollision_;
    }

    /**
     * Returns whether there was any collision directly above this character controller.
     */
    bool getDownAxisCollision(btVector3& collisionNormal) const
    {
        collisionNormal = downAxisCollisionNormal_;

        return hasDownAxisCollision_;
    }

    void updateAction(btCollisionWorld* world, btScalar deltaTime) override;
    void debugDraw(btIDebugDraw* debugDrawer) override {}

private:

    btPairCachingGhostObject* ghostObject_ = nullptr;
    btConvexShape* convexShape_ = nullptr;

    // Maximum slope angle that can be stepped up/down
    btScalar maxSlopeCosine_ = 0.0;

    // Size of step that can be automatically stepped up/down
    btScalar stepHeight_ = 0.0;

    // Current velocity details
    btVector3 walkVelocity_, normalizedWalkVelocity_;
    btScalar velocityTimeRemaining_ = 0.0;

    // Positioning for the current update cycle - these are not maintained across updates
    btVector3 currentPosition_, targetPosition_;
    btScalar currentStepOffset_ = 0.0;

    // Keep track of the contact manifolds
    btManifoldArray manifoldArray_;

    // Whether there are any penetrations, and the normal of the largest penetration
    bool isPenetrating_ = false;
    btVector3 penetrationNormal_;

    bool hasUpAxisCollision_ = false;
    btVector3 upAxisCollisionNormal_;

    bool hasDownAxisCollision_ = false;
    btVector3 downAxisCollisionNormal_;

    void preStep(btCollisionWorld* world);
    void playerStep(btCollisionWorld* world, btScalar dt);
    bool recoverFromPenetration(btCollisionWorld* world);
    void stepUp(btCollisionWorld* world);
    void updateTargetPositionBasedOnCollision(const btVector3& hitNormal);
    void stepForwardAndStrafe(btCollisionWorld* world, const btVector3& move);
    void stepDown(btCollisionWorld* world, btScalar dt);
    void checkForUpAxisCollision(btCollisionWorld* world);
    void checkForDownAxisCollision(btCollisionWorld* world);

public:

#ifdef _MSC_VER
    // MSVC requires operator new and delete be overloaded in order to guarantee 16-byte alignment when this class is
    // allocated on the heap. MemoryInterceptor.h is re-included below in order to redefine its operator new
    // interception macro.
    static void* operator new(size_t size) { return _aligned_malloc(size, 16); }
    static void operator delete(void* memory) { _aligned_free(memory); }
#endif
};

}

#ifdef _MSC_VER
    #include "CarbonEngine/Core/Memory/MemoryInterceptor.h"
#endif
