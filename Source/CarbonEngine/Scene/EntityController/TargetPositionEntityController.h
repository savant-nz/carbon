/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Scene/EntityController/EntityController.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

/**
 * This entity controller automatically moves its entity towards a specified entity or world space point at a constant speed,
 * then removes itself once the target has been reached.
 */
class CARBON_API TargetPositionEntityController : public EntityController
{
public:

    /**
     * Initializes this orient controller with the specified target point and orient style.
     */
    void initialize(const Vec3& targetPoint, float speed = 1.0f, bool followWorldGeometry = false)
    {
        targetPoint_ = targetPoint;
        targetEntity_ = nullptr;
        speed_ = speed;
        isFollowingWorldGeometry_ = followWorldGeometry;
        lastWorldGeometryAlignmentTime_.clear();
    }

    /**
     * Initializes this orient controller with the specified target entity and orient style.
     */
    void initialize(const Entity* targetEntity, float speed = 1.0f, bool followWorldGeometry = false)
    {
        targetEntity_ = targetEntity;
        speed_ = speed;
        isFollowingWorldGeometry_ = followWorldGeometry;
        lastWorldGeometryAlignmentTime_.clear();
    }

    bool update(TimeValue time) override
    {
        auto target = Vec3();

        // Get the world space point to move towards
        if (targetEntity_)
            target = targetEntity_->getWorldPosition();
        else
            target = targetPoint_;

        // Direction and distance to move
        auto move = target - getEntity()->getWorldPosition();
        auto distance = speed_ * platform().getSecondsPassed();

        // Check if the entity can reach the target this frame
        if (distance > move.length())
        {
            getEntity()->setWorldPosition(target);

            // Get rid of this controller now that the target has been reached
            return false;
        }

        if (isFollowingWorldGeometry_ && distance > Vec3(move.x, 0.0f, move.z).length())
        {
            target.y = getEntity()->getWorldPosition().y;
            getEntity()->setWorldPosition(target);

            // Get rid of this controller now that the target has been reached
            return false;
        }

        if (isFollowingWorldGeometry_)
        {
            // This is the distance moved between sampling a new world geometry height to align against. Using this distance
            // value avoids having to sample a height on every update
            const auto distanceBetweenUpdates = 2.5f;

            // Check if we're due to update the world geometry alignment vector
            if (lastWorldGeometryAlignmentTime_.getSecondsSince() * speed_ > distanceBetweenUpdates)
            {
                lastWorldGeometryAlignmentTime_ = platform().getTime();

                // Get the world space point we're aiming for
                auto& initialPosition = getEntity()->getWorldPosition();
                auto ahead = initialPosition + (target - initialPosition).ofLength(distanceBetweenUpdates);

                // Now shoot a ray into the world geometry to see where this entity should be, this entity is made invisible
                // to avoid self-intersection
                auto ray = Ray(ahead + Vec3::UnitY * 100.0f, -Vec3::UnitY);

                auto wasVisible = getEntity()->isVisibleIgnoreAlpha(false);
                getEntity()->setVisible(false);

                auto result = getScene()->intersect(ray, true);
                if (result)
                    ahead.y = result.getPoint().y;

                getEntity()->setVisible(wasVisible);

                worldGeometryAlignmentVector_ = (ahead - initialPosition).normalized();
            }

            // Move along the world geometry vector towards the target
            getEntity()->move(worldGeometryAlignmentVector_ * distance);
        }
        else
        {
            // Move directly towards the target
            getEntity()->move(move.ofLength(distance));
        }

        return true;
    }

    void save(FileWriter& file) const override
    {
        EntityController::save(file);
        getScene()->saveEntityReference(file, targetEntity_);
        file.write(targetPoint_, speed_, isFollowingWorldGeometry_);
    }

    void load(FileReader& file) override
    {
        EntityController::load(file);
        targetEntity_ = getScene()->loadEntityReference(file);
        file.read(targetPoint_, speed_, isFollowingWorldGeometry_);
    }

private:

    const Entity* targetEntity_ = nullptr;
    Vec3 targetPoint_;

    float speed_ = 0.0f;

    bool isFollowingWorldGeometry_ = false;
    TimeValue lastWorldGeometryAlignmentTime_;
    Vec3 worldGeometryAlignmentVector_;
};

}
