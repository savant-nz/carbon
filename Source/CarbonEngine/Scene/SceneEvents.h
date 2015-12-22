/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Event.h"
#include "CarbonEngine/Scene/Entity.h"

namespace Carbon
{

/**
 * Holds details common to all entity events.
 */
class CARBON_API EntityEventDetails
{
public:

    /**
     * Constructs this entity event with the given entity pointer.
     */
    EntityEventDetails(const Entity* entity) : entity_(entity) {}

    virtual ~EntityEventDetails() {}

    /**
     * Returns the entity that this event pertains to.
     */
    const Entity* getEntity() const { return entity_; }

    /**
     * Returns the scene that this event pertains to, equivalent to `EntityEventDetails::getEntity()->getScene()`.
     */
    const Scene* getScene() const { return entity_->getScene(); }

private:

    const Entity* const entity_;
};

/**
 * This event is sent when an entity enters one of its active regions.
 */
class CARBON_API EntityEnterRegionEvent : public Event, public EntityEventDetails
{
public:

    /**
     * Constructs this event with the given entity pointer and region name.
     */
    EntityEnterRegionEvent(Entity* entity, String region) : EntityEventDetails(entity), region_(std::move(region)) {}

    /**
     * Returns the region that was entered.
     */
    const String& getRegion() const { return region_; }

    operator UnicodeString() const override { return UnicodeString() << "region: " << getRegion(); }

private:

    const String region_;
};

/**
 * This event is sent when an entity exits one of its active regions.
 */
class CARBON_API EntityExitRegionEvent : public Event, public EntityEventDetails
{
public:

    /**
     * Constructs this event with the given entity pointer and region name.
     */
    EntityExitRegionEvent(Entity* entity, String region) : EntityEventDetails(entity), region_(std::move(region)) {}

    /**
     * Returns the region that was exited.
     */
    const String& getRegion() const { return region_; }

    operator UnicodeString() const override { return UnicodeString() << "region: " << getRegion(); }

private:

    const String region_;
};

}
