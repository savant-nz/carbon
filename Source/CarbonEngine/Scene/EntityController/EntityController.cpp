/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Scene/EntityController/EntityController.h"

namespace Carbon
{

CARBON_DEFINE_SUBCLASS_REGISTRY(EntityController)

Scene* EntityController::getScene()
{
    return entity_ ? entity_->getScene() : nullptr;
}

const Scene* EntityController::getScene() const
{
    return entity_ ? entity_->getScene() : nullptr;
}

}
