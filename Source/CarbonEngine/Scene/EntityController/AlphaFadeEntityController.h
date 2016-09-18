/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Scene/EntityController/EntityController.h"

namespace Carbon
{

/**
 * This entity controller does simple linear fades and interpolations of an entity's alpha.
 */
class CARBON_API AlphaFadeEntityController : public EntityController
{
public:

    /**
     * Initializes this alpha fade controller with the specified initial and target alpha values along with the length
     * of time that fade should take in seconds.
     */
    void initialize(float initialAlpha, float targetAlpha, float transitionTime)
    {
        initialAlpha_ = initialAlpha;
        targetAlpha_ = targetAlpha;
        transitionTime_ = transitionTime;
        timeElapsed_.clear();

        getEntity()->setAlpha(initialAlpha);
    }

    bool update(TimeValue time) override
    {
        timeElapsed_ += time;

        // If the full amount of time has elapsed then this controller is complete and so is removed
        if (timeElapsed_ >= transitionTime_)
        {
            getEntity()->setAlpha(targetAlpha_);
            return false;
        }

        getEntity()->setAlpha(
            Interpolate::linear(initialAlpha_, targetAlpha_, timeElapsed_ % transitionTime_.toSeconds()));

        return true;
    }

    void save(FileWriter& file) const override
    {
        EntityController::save(file);
        file.write(initialAlpha_, targetAlpha_, transitionTime_, timeElapsed_);
    }

    void load(FileReader& file) override
    {
        EntityController::load(file);
        file.read(initialAlpha_, targetAlpha_, transitionTime_, timeElapsed_);
    }

private:

    float initialAlpha_ = 0.0f;
    float targetAlpha_ = 1.0f;
    TimeValue transitionTime_{1.0f};
    TimeValue timeElapsed_;
};

}
