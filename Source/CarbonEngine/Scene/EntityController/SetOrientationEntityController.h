/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/EntityController/EntityController.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

/**
 * This entity controller automatically sets its entity's orientation so that it always points either at another entity or at a
 * specified world space point.
 */
class CARBON_API SetOrientationEntityController : public EntityController
{
public:

    /**
     * Initializes this orientation controller with the specified target point and orient style.
     */
    void initialize(const Vec3& targetPoint, bool isCylindrical = false)
    {
        targetPoint_ = targetPoint;
        targetEntity_ = nullptr;
        isCylindrical_ = isCylindrical;

        update(TimeValue());
    }

    /**
     * Initializes this orientation controller with the specified target entity and orient style.
     */
    void initialize(const Entity* targetEntity, bool isCylindrical = false)
    {
        targetEntity_ = targetEntity;
        isCylindrical_ = isCylindrical;

        update(TimeValue());
    }

    bool update(TimeValue time) override
    {
        auto target = targetEntity_ ? targetEntity_->getWorldPosition() : targetPoint_;

        if (isCylindrical_)
            target.y = getEntity()->getWorldPosition().y;

        getEntity()->lookAtPoint(target);

        return true;
    }

    void save(FileWriter& file) const override
    {
        EntityController::save(file);
        getScene()->saveEntityReference(file, targetEntity_);
        file.write(targetPoint_, isCylindrical_);
    }

    void load(FileReader& file) override
    {
        EntityController::load(file);
        targetEntity_ = getScene()->loadEntityReference(file);
        file.read(targetPoint_, isCylindrical_);
    }

private:

    const Entity* targetEntity_ = nullptr;
    Vec3 targetPoint_;
    bool isCylindrical_ = false;
};

}
