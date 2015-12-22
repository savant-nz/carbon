/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/Entity.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

/**
 * A SoundListener entity can be added to a scene to define the positional audio listener, it is often attached as a child of
 * another entity where the other entity is the main game object being controlled by the player.
 */
class CARBON_API SoundListener : public Entity
{
public:

    bool isPerFrameUpdateRequired() const override { return true; }

    void update() override
    {
        sounds().setListenerTransform(getWorldTransform());
        Entity::update();
    }
};

}
